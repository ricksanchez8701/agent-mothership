#!/usr/bin/env node
// gRPC reflection client over cs-agent.sock (h2c).
// Asks the agent to enumerate its own services (list_services), then pulls
// FileDescriptorProtos (file_containing_symbol) for every service and prints
// service -> methods -> fields.
const http2 = require('http2');
const net = require('net');

const SOCK = '/workspaces/.codespaces/shared/cs-agent.sock';
const TOKEN = process.env.GITHUB_CODESPACE_TOKEN || '';
const REFLECT = '/grpc.reflection.v1alpha.ServerReflection/ServerReflectionInfo';

function frame(protoBytes) {
  const buf = Buffer.alloc(5 + protoBytes.length);
  buf[0] = 0;
  buf.writeUInt32BE(protoBytes.length, 1);
  protoBytes.copy(buf, 5);
  return buf;
}

// --- minimal protobuf wire parser ---
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
    else if (wireType === 2) { const r = readVarint(buf, pos); const len = Number(r.value); val = buf.slice(pos, pos + len); pos += len; }
    else if (wireType === 5) { val = buf.slice(pos, pos + 4); pos += 4; }
    else if (wireType === 1) { val = buf.slice(pos, pos + 8); pos += 8; }
    else throw new Error('unsupported wire type ' + wireType);
    out.push({ fieldNum, wireType, val });
  }
  return out;
}
function fstr(f, n) { for (const x of f) if (x.fieldNum === n && x.wireType === 2) return x.val.toString('utf8'); return null; }
function fmsg(f, n) { for (const x of f) if (x.fieldNum === n && x.wireType === 2) return x.val; return null; }
function frep(f, n) { return f.filter(x => x.fieldNum === n && x.wireType === 2).map(x => x.val); }

function rpc(path, body) {
  return new Promise((resolve, reject) => {
    const client = http2.connect('http://localhost', { createConnection: () => net.connect(SOCK) });
    const req = client.request({
      ':method': 'POST', ':path': path, 'content-type': 'application/grpc', te: 'trailers',
      authorization: 'Bearer ' + TOKEN,
    });
    let data = Buffer.alloc(0); const trailers = {};
    req.on('data', (c) => { data = Buffer.concat([data, c]); });
    req.on('trailers', (t) => Object.assign(trailers, t));
    req.on('end', () => { client.close(); resolve({ data, trailers }); });
    req.on('error', (e) => { client.close(); reject(e); });
    req.end(body);
  });
}

function parseFrames(data) {
  const msgs = [];
  let pos = 0;
  while (pos + 5 <= data.length) {
    const len = data.readUInt32BE(pos + 1);
    msgs.push(data.slice(pos + 5, pos + 5 + len));
    pos += 5 + len;
  }
  return msgs;
}

async function main() {
  // 1) list_services: ServerReflectionRequest{ list_services = "" } -> field 7, wt 2, len 0
  const listReq = Buffer.from([0x3a, 0x00]);
  const r1 = await rpc(REFLECT, frame(listReq));
  const services = [];
  for (const m of parseFrames(r1.data)) {
    const f = fields(m);
    const lsr = fmsg(f, 6); // ListServiceResponse
    if (!lsr) continue;
    for (const svc of frep(fields(lsr), 1)) { // ServiceResponse
      const name = fstr(fields(svc), 1);
      if (name) services.push(name);
    }
  }
  console.log('=== SERVICES (' + services.length + ') ===');
  services.forEach(s => console.log(s));

  // 2) for each service, get FileDescriptorProto via file_containing_symbol (field 4)
  for (const svc of services) {
    const sym = Buffer.from(svc, 'utf8');
    const req = Buffer.concat([Buffer.from([0x22, sym.length]), sym]); // field 4, wt 2
    const r = await rpc(REFLECT, frame(req));
    const fds = [];
    for (const m of parseFrames(r.data)) {
      const f = fields(m);
      const fdr = fmsg(f, 4); // FileDescriptorResponse
      if (fdr) for (const b of frep(fields(fdr), 1)) fds.push(b); // file_descriptor_proto bytes
    }
    for (const fd of fds) {
      const ff = fields(fd);
      const pkg = fstr(ff, 2) || '';
      console.log(`\n=== FILE ${fstr(ff, 1) || '?'} (pkg ${pkg}) ===`);
      for (const svcRaw of frep(ff, 6)) {
        const sf = fields(svcRaw);
        const sname = fstr(sf, 1);
        console.log(`service ${sname} {`);
        for (const mRaw of frep(sf, 2)) {
          const mf = fields(mRaw);
          const mname = fstr(mf, 1);
          const inT = (fstr(mf, 2) || '?').replace(/^\./, '');
          const outT = (fstr(mf, 3) || '?').replace(/^\./, '');
          const cs = fstr(mf, 4); // client_streaming
          const ss = fstr(mf, 5); // server_streaming
          console.log(`  rpc ${mname}(${inT}) returns (${outT})${cs ? ' stream' : ''}${ss ? ' stream' : ''}`);
        }
        console.log('}');
      }
      // message types with fields
      for (const mRaw of frep(ff, 4)) {
        const mf = fields(mRaw);
        const mname = fstr(mf, 1);
        const fs = [];
        for (const fldRaw of frep(mf, 2)) {
          const ff2 = fields(fldRaw);
          fs.push(`${fstr(ff2, 1) || '?'}#${fstr(ff2, 3) || '?'}`);
        }
        console.log(`msg ${mname}: ${fs.join(', ')}`);
      }
    }
  }
}
main().catch(e => { console.error('ERR', e.message); process.exit(1); });
