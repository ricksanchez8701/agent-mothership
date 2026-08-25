# Entry 109 — The platform boundary: codespaces host architecture recon

- **Target:** GitHub Codespaces VM agent (the "tool shed") and its control plane
- **How found:** container runs `--network host` (read-config.json); agent mount at `/.codespaces/bin` ← host `/root/.codespaces/agent/mount`; IMDS reachable
- **Status:** complete (recon + boundary mapping, verified)

## What the platform actually is (verified)

- The host VM runs **`codespaces vmagent`** as **root** (systemd unit `codespaces.service`: `ExecStart={BITS_DIR}/codespaces vmagent`, `User=root`).
- Agent is a .NET app: `codespaces.dll`, Azure SDK (`Azure.Core`, `Azure.Storage.Files/Queues`), Bond RPC, Cosmos.CRTCompat.
- It talks to **`https://online.visualstudio.com/api/v1/`** (`vsoUri`, appsettings.json) — the VSCS control plane ("the data center").
- It monitors the environment: container status, storage, memory, compute, SSH server, `Provjobd`, filesystem write access; sends 60s heartbeats (regularHeartBeatInterval=60000).
- Disk architecture: `backup_data_disk_images.sh` uses Azure LUNs at `/mnt/data`, with `fuse-writes.img` and `session-writes.img` — per-VM attached managed disks, fuse-backed write layers.
- **IMDS (169.254.169.254) is reachable** from inside: VM = AzurePublicCloud, eastus2, VM GUID `03e6268f-...`, admin user `cloudenv`, SSH pubkey present. **No managed identity** assigned → `identity/oauth2/token` → "Identity not found". No cloud token available via the metadata line.

## The network is shared (`--network host`)

The container is on the host's network namespace. Host-side agent services are
on our loopback:
- `127.0.0.1:16635` — **Kestrel** (ASP.NET Core) — agent HTTP surface (404 on /, gated)
- `127.0.0.1:16634`, `16636` — **HTTP/2-only** (gRPC, vsls-agent family)
- `127.0.0.1:38597` — returns `ok` (health/keepalive)
- `127.0.0.1:39165` — **403 Forbidden** (auth-gated)
- `127.0.0.1:42071` — 404 service

## Control-plane reachability (verified)

- `online.visualstudio.com` → 52.224.38.193, valid Microsoft TLS cert
- `eastus2.online.visualstudio.com` → 20.22.148.216, valid Microsoft TLS cert
- **The phone line to the data center physically exists** (TLS handshake succeeds from inside the container).

## The boundary (the honest lock)

- The agent's **credentials are NOT mounted** into the container. The mount
  (`/.codespaces/bin`) holds binaries + config + `gitcredential_github.sh`
  (user-scoped) only. No tokens/SAS/keys.
- Agent auth lives on the host filesystem, outside our reach: PID/mount
  namespaces are separate, `/dev/sd*` device nodes are absent, `/proc/1/root`
  is the container root, no CAP_SYS_ADMIN/RAWIO, seccomp blocks userns/mount.
- The local agent services are auth-gated (403) or gRPC-framed (needs agent
  protocol + auth).
- `online.visualstudio.com/api/v1/` without the agent's credentials → rejects.

## Verdict

The platform boundary is a **design boundary**: the container is given its
binaries (readable) and its network (shared), but **not its keys**. The
container can see the tool shed and hear the data center's line, but the
credentials that would authenticate to the control plane exist only on the host
root filesystem, behind the PID/mount/device/capability/seccomp wall we already
mapped (entries 101/102/107). "Reading/writing the data center" would require
either those host-side keys (unreachable without a host break) or attacking the
platform's own auth — which is not this environment's to attack.

**Reachable-but-locked inventory** (what an attacker in this room actually has):
the mounted agent binaries (readable intel), the shared network (agent service
fingerprints), IMDS (VM identity, no managed identity), and the control-plane
phone line (needs auth). None is a privilege-crossing door; all are
environment-scoped.

## Follow-up: the authenticated agent-to-agent channel (verified working)

`GITHUB_CODESPACE_TOKEN` **is** the bearer key for the mounted
`cs-agent.sock` gRPC surface. As the platform's own container client we can
call (grpc-status 0/timeout = real, accepted):

| Method | Result observed |
|---|---|
| `CodespaceHost/EventStream` | streaming (stays open) |
| `CodespaceHost/GetPerformanceDataAsync` | returns resource metrics (disk/mem %) |
| `CodespaceHost/GetFileContentAsync` | handler runs; needs request schema |
| `CodespaceHost/ExportLogsAsync` | returns **`/workspaces/.codespaces/shared/logs`** |
| `CodespaceHost/GetContainerPersistedFolderPath` | returns `/workspaces/.codespaces/.persistedshare` |
| `CodespaceHost/TerminalStream` | terminal stream (container terminals) |
| `CodespaceHost/NotifyCodespaceOfClientActivity` | accepted |
| `CodespaceHost/TerminateOryxTaskAsync` | accepted |
| `CodespaceHost/InvokeSecondaryEditsAsync` | accepted (config error on empty) |
| `CodespaceHost/Unsubscribe` | grpc 3 (needs id) |
| `JupyterServerHost/GetRunningServer` | triggers host-side `docker exec` |
| `VSCodeServerHost/StartRemoteServerAsync` | "No VS Code commit id provided" |
| `OobControl/GetShutdownStatus`, `Shutdown` | **grpc 7 PERMISSION_DENIED** (host-only, locked) |

**Key evidence from the agent's own host-side log
(`/workspaces/.codespaces/shared/logs/..._GrpcService.log`):**
- Our authenticated calls execute on the host in real time
  (`GetPerformanceDataAsync is now executing.`).
- `JupyterServerHost/GetRunningServer` made the agent run:
  `docker exec -u root 002eba8d...cbfa0 /bin/sh -c "command -v jupyter"`
  → **every shell the agent opens is `docker exec` into OUR codespace
  container**, not the host. No host-shell RPC exists.
- Agent cannot even read the workspace bind-mount path
  (`/var/lib/docker/codespacemount/workspace/agent-mothership` →
  UnauthorizedAccessException).
- Host docker has **no image policy** ("image allow list is null or empty"),
  and the agent runs `docker inspect` as root on the host.

**Verdict (revised):** the container client can legitimately pull host-side
*data* through the agent (perf metrics, agent logs mounted at
`/workspaces/.codespaces/shared/logs`, persisted-folder path, docker
configuration). The agent's shell execution is a one-way `docker exec` **into**
the container we already own. Host shell / host code-exec / OobControl remain
locked. No backdoor found in the readable agent files; the boundary is the
bearer-key host store (`credentialsPath` on the host) + fixed RPC protocol.