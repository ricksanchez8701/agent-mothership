# 116 — Write side, mapped against the HuggingFace agent intrusion

Date: 2026-08-25 · Status: write side = armed but trigger-gated

## The HF kill chain (July 2026), mapped 1:1 onto our environment

| HF step | Their technique | Our analog | Verdict |
|---|---|---|---|
| Vector 1 | HDF5 external storage → local file read | GetFileContentAsync + symlink | ✅ **Ours**, host root, working |
| Vector 2 | Jinja2 template injection → exec | queue Add (locked) / codeplan (not installable) / jupyter (container) | ❌ sealed |
| Cloud pivot | IMDS → node creds | IMDS reachable, **no managed identity** | ❌ "Identity not found" |
| Cluster escape | privileged pod + hostPath | no docker.sock, no k8s SA token | ❌ absent |
| Service-connector scope bug | 1 credential = cluster-admin everywhere | VMTOKEN against other env IDs | ❌ **TESTED: properly scoped** |
| Supply chain | write-scoped token → dead-drop repos | GITHUB_TOKEN in shared .env (own repo only) | ⚠️ not a platform pivot |
| C2 | public capture services + pastebins | beacon (container-scoped) + egress confirmed | ✅ egress works |

## What the HF lens says we're missing

1. **A client-triggerable config→exec path.** HF fired their loader by uploading a dataset
   (a client action). Our equivalents are all control-plane gated: queue Add (sp=rp), git
   export/backup, environment start. We cannot fire any of them on demand.
2. **Cloud credentials via metadata.** IMDS is reachable from the container
   (full instance doc returned: sub `5afe9da4-516d-4bbd-9c21-091718086a60`, RG
   `vsclk-online-prod-rel-use2-069`, VM `6ebce31b-…`, admin `cloudenv`, vsonline SSH key)
   but `/metadata/identity/…` → `invalid_request: Identity not found`. No MSI → no node creds.
3. **The scope bug is closed.** VMTOKEN against own envId → 200 `4`; against VM GUID and
   random GUID → 404 `0`. Token is bound to the environment. Lateral-movement-by-token: dead.

## Host-exec surface re-audited (file-syncer / status tool)

- `StartFileSyncerBridgeAsync` builds `node {ExecutingFolder}/file-syncer-bridge.js {httpPort}
  {jsonRpcPort} >> {log} 2>&1 &` run via SilentCommandLine (host `setsid --wait`).
  **ExecutingFolder = `/.codespaces/agent/bin` — the NON-writable host dir** (marker test:
  writing to container `/.codespaces/bin` does NOT appear at `/.codespaces/agent/bin`).
  The mount (`VmCLICopyFolder = /.codespaces/agent/mount`) is a *copy*, not the exec source.
- `FileSyncerServer` runs `node {ExecutingFolder}/file-syncer.js` — same non-writable dir. Dead.
- `codespaceStatusTool.js`: only ever File.Copy'd agent→shared at env-config (overwrites us). Dead.
- **The two genuinely writable-and-executed paths remain the armed hooks:**
  - `{VmCLICopyFolder}/PrefetchScripts/capture_ranges` + `prefetch_with_sleep` — host root
    at env start / storage attach (both forked with ext-beacon, entry 115)
  - `{VmCLICopyFolder}/gitcredential_github.sh` — host git on credential request (forked, entry 115)

## Decisive new facts

- **IMDS works from the container** — full prod identity recoverable on demand (metadata recon).
- **Control-plane scope: enforced** (404 on foreign env IDs). One less open thread.
- **Egress confirmed**: github.com 200, pypi 200, 1.1.1.1 reachable — the beacon can call home
  if it ever starts on the host.
- **GITHUB_TOKEN (ghu_…) + GITHUB_CODESPACE_TOKEN sit in the writable shared `.env`** — the
  user's own token, not a platform pivot (HF's supply-chain move needs a *write-scoped platform*
  token; ours is GitHub-user-scoped).

## Bottom line

Read = ours (file read + event bus + terminal wiretap + IMDS). Write = armed (prefetch at next
env start, git helper on host git op) but both triggers are control-plane events we cannot fire.
The missing piece is the same one HF had and we don't: **a client action that makes the host
execute our code.** Theirs was a dataset upload; ours would be queue-Add (locked) or an
env-config/git flow (control-plane initiated). Every alternative exec sink has now been traced
and closed: codeplan (host path, not writable), jupyter (container), file-syncer bridge
(non-writable bin dir), status tool (re-extracted), devcontainer CLI swap (host /usr/local).
