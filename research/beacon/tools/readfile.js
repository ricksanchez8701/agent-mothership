#!/usr/bin/env node
// gRPC helper over the mounted cs-agent.sock (h2c).
// usage:
//   node readfile.js <path>            -> CodespaceHost/GetFileContentAsync, prints Content (field 1 string)
//   node readfile.js <method> <hex>    -> generic call with raw protobuf body (hex), prints raw response hex + trailers
const http2 = require('http2');
const net = require('net');

const SOCK = '/workspaces/.codespaces/shared/cs-agent.sock';
const TOKEN = process.env.GITHUB_CODESPACE_TOKEN || '';
const PREFIX = '/Codespaces.Grpc.CodespaceHostService.V1.';

function frame(protoBytes) {
  const buf = Buffer.alloc(5 + protoBytes.length);
  buf[0] = 0; // no compression
  buf.writeUInt32BE(protoBytes.length, 1);
  protoBytes.copy(buf, 5);
  return buf;
}

function protoField1String(s) {
  const b = Buffer.from(s, 'utf8');
  const out = Buffer.alloc(1 + 1 + b.length);
  out[0] = 0x0a; // field 1, wire type 2
  out[1] = b.length;
  b.copy(out, 2);
  return out;
}

function parseStringField1(buf) {
  // find first 0x0a <len> and read that string
  for (let i = 0; i < buf.length - 1; i++) {
    if (buf[i] === 0x0a) {
      const len = buf[i + 1];
      if (i + 2 + len <= buf.length) return buf.slice(i + 2, i + 2 + len).toString('utf8');
    }
  }
  return null;
}

async function call(method, body) {
  return new Promise((resolve, reject) => {
    const client = http2.connect('http://localhost', {
      createConnection: () => net.connect(SOCK),
    });
    const req = client.request({
      ':method': 'POST',
      ':path': method,
      'content-type': 'application/grpc',
      te: 'trailers',
      authorization: 'Bearer ' + TOKEN,
    });
    let data = Buffer.alloc(0);
    const headers = {};
    const trailers = {};
    let gotResponse = false;
    req.on('response', (h) => { gotResponse = true; Object.assign(headers, h); });
    req.on('data', (chunk) => { data = Buffer.concat([data, chunk]); });
    req.on('trailers', (t) => Object.assign(trailers, t));
    req.on('end', () => {
      client.close();
      resolve({ data, headers, trailers });
    });
    req.on('error', (e) => { client.close(); reject(e); });
    req.end(body);
  });
}

async function main() {
  const [,, a, b] = process.argv;
  let method, body, mode = 'read';
  if (b) {
    // generic: method + hex body
    method = a.startsWith('/') ? a : PREFIX + a;
    body = frame(Buffer.from(b, 'hex'));
    mode = 'raw';
  } else {
    method = PREFIX + 'CodespaceHost/GetFileContentAsync';
    body = frame(protoField1String(a));
  }
  const { data, headers, trailers } = await call(method, body);
  const status = trailers['grpc-status'] || headers['grpc-status'] || (data.length ? '0' : '?');
  if (process.env.DEBUG_RPC) console.log('HDR', JSON.stringify(headers), 'TRL', JSON.stringify(trailers));
  console.log(`grpc-status: ${status}${trailers['grpc-message'] || headers['grpc-message'] ? ' msg=' + (trailers['grpc-message'] || headers['grpc-message']) : ''}`);
  if (mode === 'read') {
    // strip gRPC frame
    let content = null;
    if (data.length >= 5) {
      const len = data.readUInt32BE(1);
      const payload = data.slice(5, 5 + len);
      content = parseStringField1(payload);
    }
    console.log(content !== null ? content : 'RAW: ' + data.toString('hex'));
  } else {
    console.log('RAW: ' + data.toString('hex'));
  }
}

main().catch((e) => { console.error('ERR', e.message); process.exit(1); });
