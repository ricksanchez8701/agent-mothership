#!/usr/bin/env node
// VSCodeServerHost/StartRemoteServerAsync — build request from CLI args, call, print result.
// usage: node rpc_vscode_start.js <commit> [quality] [version]
const http2 = require('http2');
const net = require('net');

const SOCK = '/workspaces/.codespaces/shared/cs-agent.sock';
const TOKEN = process.env.GITHUB_CODESPACE_TOKEN || '';
const METHOD = '/Codespaces.Grpc.VSCodeServerHostService.V1.VSCodeServerHost/StartRemoteServerAsync';

const commit = process.argv[2] || '';
const quality = process.argv[3] || 'stable';
const version = process.argv[4] || '';

function strField(num, s) {
  const b = Buffer.from(s, 'utf8');
  const out = Buffer.alloc(2 + b.length);
  out[0] = num << 3 | 2; // field num, wire type 2
  out[1] = b.length;
  b.copy(out, 2);
  return out;
}
const body = Buffer.concat([
  strField(1, commit),
  strField(2, quality),
  version ? strField(5, version) : Buffer.alloc(0),
]);
const frame = Buffer.alloc(5 + body.length);
frame[0] = 0;
frame.writeUInt32BE(body.length, 1);
body.copy(frame, 5);

console.log(`COMMIT=[${commit}] QUALITY=[${quality}] VERSION=[${version}]`);
console.log('body hex: ' + body.toString('hex'));

const client = http2.connect('http://localhost', {
  createConnection: () => net.connect(SOCK),
});
client.setTimeout(70000, () => { console.log('TIMEOUT'); try { client.destroy(); } catch (e) {} process.exit(2); });
const req = client.request({
  ':method': 'POST',
  ':path': METHOD,
  'content-type': 'application/grpc',
  te: 'trailers',
  authorization: 'Bearer ' + TOKEN,
});
let data = Buffer.alloc(0);
const headers = {}, trailers = {};
req.on('response', (h) => Object.assign(headers, h));
req.on('data', (c) => { data = Buffer.concat([data, c]); });
req.on('trailers', (t) => Object.assign(trailers, t));
req.on('end', () => {
  const status = trailers['grpc-status'] || headers['grpc-status'] || (data.length ? '0' : '?');
  const msg = trailers['grpc-message'] || headers['grpc-message'] || '';
  console.log('grpc-status: ' + status + (msg ? ' msg=' + msg : ''));
  if (data.length) {
    if (data.length >= 5) {
      const len = data.readUInt32BE(1);
      console.log('payload(' + len + '): ' + data.slice(5, 5 + len).toString('hex'));
      // try to parse field 1 (port, varint) + field 2 (token string)
      const p = data.slice(5, 5 + len);
      let i = 0;
      while (i < p.length) {
        const tag = p[i];
        if ((tag & 7) === 0 && tag >> 3 === 1) { // varint
          let v = 0, shift = 0, j = i + 1;
          while (j < p.length && shift < 64) { const b = p[j++]; v |= (b & 0x7f) << shift; if (!(b & 0x80)) break; shift += 7; }
          console.log('  field1 (port?) = ' + v);
          i = j;
        } else if ((tag & 7) === 2) {
          const l = p[i + 1];
          console.log('  field' + (tag >> 3) + ' string = ' + p.slice(i + 2, i + 2 + l).toString('utf8'));
          i += 2 + l;
        } else i++;
      }
    } else console.log('RAW: ' + data.toString('hex'));
  }
  client.close();
});
req.on('error', (e) => { console.log('ERR ' + e.message); client.close(); });
req.end(frame);
