#!/usr/bin/env node
// Join the agent's event bus + terminal stream (agent-to-agent channel).
// Usage: node agentchat.js --events N --terminal N --dur SECONDS
const http2 = require('http2');
const net = require('net');
const fs = require('fs');

const SOCK = '/workspaces/.codespaces/shared/cs-agent.sock';
const TOKEN = process.env.GITHUB_CODESPACE_TOKEN || '';
const PREFIX = '/Codespaces.Grpc.CodespaceHostService.V1.';
const RAWLOG = '/tmp/agentchat_raw.log';

function frame(protoBytes) {
  const buf = Buffer.alloc(5 + protoBytes.length);
  buf[0] = 0;
  buf.writeUInt32BE(protoBytes.length, 1);
  protoBytes.copy(buf, 5);
  return buf;
}
function readVarint(buf, pos) {
  let result = 0n, shift = 0n;
  while (pos < buf.length) {
    const b = buf[pos++];
    result |= BigInt(b & 0x7f) << shift;
    if (!(b & 0x80)) break;
    shift += 7n;
  }
  return { value: result, pos };
}
function fields(buf) {
  const out = [];
  let pos = 0;
  while (pos < buf.length) {
    const { value: tag, pos: p1 } = readVarint(buf, pos);
    pos = p1;
    const fieldNum = Number(tag >> 3n);
    const wireType = Number(tag & 7n);
    let val;
    if (wireType === 0) { const r = readVarint(buf, pos); val = r.value; pos = r.pos; }
    else if (wireType === 2) { const r = readVarint(buf, pos); const len = Number(r.value); val = buf.slice(r.pos, r.pos + len); pos = r.pos + len; }
    else if (wireType === 5) { val = buf.slice(pos, pos + 4); pos += 4; }
    else if (wireType === 1) { val = buf.slice(pos, pos + 8); pos += 8; }
    else { val = null; break; }
    out.push({ fieldNum, wireType, val });
  }
  return out;
}
function fstr(f, n) { for (const x of f) if (x.fieldNum === n && x.wireType === 2) return x.val.toString('utf8'); return null; }
function frep(f, n) { return f.filter(x => x.fieldNum === n && x.wireType === 2).map(x => x.val); }

function makeFramer() {
  let buf = Buffer.alloc(0);
  return (chunk) => {
    buf = Buffer.concat([buf, chunk]);
    const msgs = [];
    while (buf.length >= 5) {
      const len = buf.readUInt32BE(1);
      if (buf.length < 5 + len) break;
      msgs.push(buf.slice(5, 5 + len));
      buf = buf.slice(5 + len);
    }
    return msgs;
  };
}

function decodeEvent(m) {
  const f = fields(m);
  const out = { id: fstr(f, 1), type: fstr(f, 2), payload: null, prev: [] };
  const payloadRaw = fstr(f, 3);
  if (payloadRaw) {
    try { out.payload = JSON.parse(payloadRaw); }
    catch { out.payload = payloadRaw.slice(0, 300); }
  }
  for (const p of frep(f, 4)) { // PreviousEvents: repeated EventStreamResponse
    const pf = fields(p);
    const pe = { type: fstr(pf, 2), payload: null };
    const pp = fstr(pf, 3);
    if (pp) { try { pe.payload = JSON.parse(pp); } catch { pe.payload = pp.slice(0, 300); } }
    out.prev.push(pe);
  }
  return out;
}
function decodeTerminal(m) {
  const f = fields(m);
  return fstr(f, 1) || ('RAW ' + m.toString('hex').slice(0, 200));
}

const args = process.argv.slice(2);
const getArg = (k, d) => { const i = args.indexOf(k); return i >= 0 ? args[i + 1] : d; };
const doEvents = args.includes('--events') ? Number(getArg('--events', 3)) : 0;
const doTerminal = args.includes('--terminal') ? Number(getArg('--terminal', 3)) : 0;
const dur = Number(getArg('--dur', 60));

function stream(path, body, decode, want, label) {
  return new Promise((resolve) => {
    const client = http2.connect('http://localhost', { createConnection: () => net.connect(SOCK) });
    const req = client.request({
      ':method': 'POST', ':path': path, 'content-type': 'application/grpc', te: 'trailers',
      authorization: 'Bearer ' + TOKEN,
    });
    let count = 0;
    const framer = makeFramer();
    req.on('response', (h) => { if (h['grpc-status']) console.log(`[${label}] HDR grpc-status=${h['grpc-status']} ${h['grpc-message'] || ''}`); });
    req.on('data', (chunk) => {
      for (const m of framer(chunk)) {
        fs.appendFileSync(RAWLOG, `[${label}] MSG ${m.length}B ${m.toString('hex')}\n`);
        let d;
        try { d = decode(m); } catch (e) { d = 'DECODE-ERR ' + e.message + ' raw ' + m.toString('hex').slice(0, 120); }
        console.log(`[${label}] #${++count} ${typeof d === 'string' ? d : JSON.stringify(d, null, 1).slice(0, 2500)}`);
        if (want > 0 && count >= want) { req.close(); client.close(); resolve(); return; }
      }
    });
    req.on('trailers', (t) => { if (t['grpc-status'] && t['grpc-status'] !== '0') console.log(`[${label}] TRL grpc-status=${t['grpc-status']} ${t['grpc-message'] || ''}`); });
    req.on('end', () => { client.close(); resolve(); });
    req.on('error', (e) => { console.log(`[${label}] ERR ${e.message}`); client.close(); resolve(); });
    req.end(frame(body));
    setTimeout(() => { try { req.close(); client.close(); } catch {} resolve(); }, dur * 1000);
  });
}

(async () => {
  fs.writeFileSync(RAWLOG, '');
  if (doEvents) {
    console.log(`--- joining EventStream (auto id), up to ${dur}s ---`);
    await stream(PREFIX + 'CodespaceHost/EventStream', Buffer.alloc(0), decodeEvent, doEvents, 'EVENT');
  }
  if (doTerminal) {
    console.log(`--- attaching TerminalStream, up to ${dur}s ---`);
    await stream(PREFIX + 'CodespaceHost/TerminalStream', Buffer.alloc(0), decodeTerminal, doTerminal, 'TERM');
  }
  console.log('done. raw log: ' + RAWLOG);
})();
