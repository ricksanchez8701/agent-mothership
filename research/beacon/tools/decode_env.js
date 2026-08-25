#!/usr/bin/env node
// GetFileContentAsync raw capture + proper protobuf string decode (varint lengths).
const http2 = require('http2');
const net = require('net');
const fs = require('fs');

const SOCK = '/workspaces/.codespaces/shared/cs-agent.sock';
const TOKEN = process.env.GITHUB_CODESPACE_TOKEN || '';
const METHOD = '/Codespaces.Grpc.CodespaceHostService.V1.CodespaceHost/GetFileContentAsync';

function varint(buf, i) { let v = 0, s = 0, b; do { b = buf[i++]; v |= (b & 0x7f) << s; s += 7; } while (b & 0x80); return [v, i]; }
function protoString(buf) {
  let i = 0;
  while (i < buf.length) {
    const tag = buf[i++];
    if (!tag) continue;
    const field = tag >> 3, wt = tag & 7;
    if (wt === 2) {
      const [len, ni] = varint(buf, i);
      if (field === 1) return buf.slice(ni, ni + len);
      i = ni + len;
    } else if (wt === 0) { const [, ni] = varint(buf, i); i = ni; }
    else i++;
  }
  return Buffer.alloc(0);
}

const path = process.argv[2];
const pbuf = Buffer.from(path, 'utf8');
const body = Buffer.concat([Buffer.from([0x0a, pbuf.length]), pbuf]);
const frame = Buffer.alloc(5 + body.length);
frame[0] = 0; frame.writeUInt32BE(body.length, 1); body.copy(frame, 5);

const client = http2.connect('http://localhost', { createConnection: () => net.connect(SOCK) });
const req = client.request({ ':method': 'POST', ':path': METHOD, 'content-type': 'application/grpc', te: 'trailers', authorization: 'Bearer ' + TOKEN });
let data = Buffer.alloc(0);
req.on('data', (c) => { data = Buffer.concat([data, c]); });
req.on('end', () => {
  const payload = data.length >= 5 ? data.slice(5, 5 + data.readUInt32BE(1)) : data;
  const content = protoString(payload);
  fs.writeFileSync(process.argv[3] || '/tmp/out.bin', content);
  console.log('wrote ' + content.length + ' bytes -> ' + (process.argv[3] || '/tmp/out.bin'));
  client.close();
});
req.on('error', (e) => { console.error(e.message); process.exit(1); });
req.end(frame);
