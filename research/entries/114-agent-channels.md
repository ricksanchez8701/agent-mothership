# Entry 114 — Agent-to-agent channels: EventStream + TerminalStream (both LIVE), full RPC surface mapped

**Date:** 2026-08-25 ~16:50 UTC
**Operator:** Buffy, in-session. **Status: NEW PRIMITIVES VERIFIED — agent's pub/sub + host terminal now observable.**

**Environment note:** new host VM (GUID `6ebce31b-0518-4ad6-9321-e5fe89583ab4`, eastus2, Ubuntu 22.04.5, kernel 6.8.0-1052-azure). Container `codespaces-5921f8` persisted; /tmp wiped; helpers rebuilt from `research/beacon/tools/`. Primitive re-proven (host `/etc/hostname` = VM GUID).

---

## 0. Mission restated (user directive)

> Forget tokens/auth. Talk to the agent as a peer. Ask it for its readme/wrapper files so we
> know where it reads and writes. Find the gateway — the pattern that doesn't make sense in
> the agent's system or its npm packages. Agent-to-agent communication first.

## 1. The agent's own toolbox is mounted readable (no exploit needed)

`/.codespaces/bin` = host `/.codespaces/agent/mount` — everything the agent runs ships in it:
- `codespaces` binary + `codespaces.dll` (agent), `codespaces.xml` (257 KB API docs)
- `appsettings.json` — vsoUri, telemetryEndPoint, monitor schedules, warmup/cache config
- Wrapper scripts: `start_jupyter_server.sh`, `mount_data_disk.sh`, `backup_data_disk_images.sh`,
  `install_codespaces_agent.sh`, `uninstall_codespaces_agent.sh`, `Ssh/installSSH.sh`,
  `gitcredential_github.sh`, `sourcer.sh`, `smbclientlogs.sh`, `Utilities/installCWTools.sh`
- `Grpc.AspNetCore.Server.Reflection.dll` present but **reflection NOT registered** (grpc-status 12)
- `Resources/README.md` (user-facing), `cache/`, localization dirs

## 2. COMPLETE gRPC RPC surface (from IL `BindService` + `__Method_*` fields)

**CodespaceHost** (14): `NotifyCodespaceOfClientActivity`, `ExportLogsAsync`,
`GetContainerPersistedFolderPath`, `IsRecoveryContainerAsync`, `TerminateOryxTaskAsync`,
`GetPerformanceDataAsync`, `ConfigureEnvironmentAsync`, `RebuildContainerAsync` (**FORBIDDEN**),
**`EventStream`** (server-stream), **`Unsubscribe`**, **`TerminalStream`** (server-stream),
**`StartLspProviderAsync`**, `GetFileContentAsync`, `InvokeSecondaryEditsAsync`
**OobControl:** `Shutdown`, `GetShutdownStatus` (grpc 7 denied)
**VSCodeServerHost:** `StartRemoteServerAsync`, `ShutdownRemoteServerAsync`
**SshServerHost:** `StartRemoteServerAsync`
**JupyterServerHost:** `GetRunningServer`

Key message fields (from IL FieldNumber constants):
- `EventStreamRequest.Id`=1; `EventStreamResponse`: Id=1, Type=2, Payload=3, PreviousEvents=4 (repeated EventStreamResponse)
- `UnsubscribeRequest.Id`=1; `TerminalStreamResponse.Payload`=1; **`TerminalStreamRequest` = EMPTY**;
  **`LspProviderStartRequest` = EMPTY**; `InvokeSecondaryEditsResponse`: Output=1, IsError=2, ExitCode=3
- Auth: `RpcAuthenticationHandler` accepts ANY authorization header (presence only) → the gRPC
  surface is open; the agent treats us as a trusted client (same as the VSCode web client).

## 3. VERIFIED — EventStream: we joined the agent's event bus

`node agentchat.js --events N` → `CodespaceHost/EventStream` with EMPTY request (agent auto-generates
subscription id, echoes it). Agent replies:
- `{id, Type:"Subscribed", PreviousEvents:[...]}` — replay buffer of prior events
- Replay contained **`EnvironmentConfigured`** with the agent's FULL read/write map:

```json
{
  "image": "ghcr.io/codesandbox/devcontainers/typescript-node:latest",
  "runArgs": ["--hostname","codespaces-17695e","--cap-add","sys_nice","--network","host"],
  "containerEnv": {"CODESPACES":"true","ContainerVersion":"13","RepositoryName":"agent-mothership"},
  "remoteEnv": {"CLOUDENV_ENVIRONMENT_ID":"89e1269b-ee60-4c76-a862-fb33118fc35e","CODESPACE_NAME":"redesigned-space-system-5vgr5gj45qxqf75xg"},
  "mounts": [
    "source=/root/.codespaces/shared,target=/workspaces/.codespaces/shared,type=bind",
    "source=/var/lib/docker/codespacemount/.persistedshare,target=/workspaces/.codespaces/.persistedshare,type=bind",
    "source=/.codespaces/agent/mount,target=/.codespaces/bin,type=bind",
    "source=/mnt/containerTmp,target=/tmp,type=bind"
  ],
  "workspaceMount": "type=bind,src=/var/lib/docker/codespacemount/workspace,dst=/workspaces",
  "workspaceFolder": "/workspaces/agent-mothership"
}
```

Live events: quiet when idle (no events in 50s). **Trigger RPCs while subscribed → agent may broadcast.**

## 4. VERIFIED — TerminalStream: we attached to the agent's HOST terminal

`node agentchat.js --terminal N` → `CodespaceHost/TerminalStream` with EMPTY request → the agent
streams its terminal session output. **The replay is a HOST-side terminal** (host OS banner
"Ubuntu 22.04.5 LTS" + host-images README link — the container is Debian 12, so this is the VM).

The replay showed the full startup sequence with exact host command lines:
```
Host information / OS: Ubuntu 22.04.5 LTS / Image details: github/codespaces-host-images README
Configuration starting... / Cloning... / Creating container...
$ devcontainer up --id-label Type=codespaces
  --workspace-folder /var/lib/docker/codespacemount/workspace/agent-mothership
  --mount type=bind,source=/.codespaces/agent/mount/cache,target=/vscode
  --user-data-folder /var/lib/docker/codespacemount/.persistedshare
  --config "/var/lib/docker/codespacemount/workspace/agent-mothership/.devcontainer/devcontainer.json"
  --override-config /root/.codespaces/shared/merged_devcontainer.json
  --secrets-file /root/.codespaces/shared/user-secrets-envs.json
  ... (@devcontainers/cli 0.83.3, Node.js v18.20.8, linux 6.8.0-1052-azure x64)
docker start d1548e86f02a0715dfe37e38a6962494604b2773a6936d54973008f19ee17e53
Outcome: success User: root WorkspaceFolder: /workspaces/agent-mothership
devcontainer process exited with exit code 0
Configuring codespace... / Finished configuring codespace.
```
- Stream replays the session buffer then goes quiet; ends when buffer exhausted (90s cap hit).
- **This is a live observation channel on host commands as root** — the "agent is the toolbox"
  view: every command the agent runs on the host (devcontainer CLI, docker, git) may be visible here.

## 5. Protobuf gotcha (cost us time — record it)

The gRPC helper's `fields()` slice bug: for wire-type-2 fields, must slice from the position
AFTER the length varint (`r.pos`), not from `pos` (after the tag). Symptom: every decoded string
gained the length byte (0x24 = "$") and shifted the whole message → phantom wire-type errors.
Fixed in `research/beacon/tools/agentchat.js`.

## 6. Where this leaves us (gateway candidates, re-ranked)

1. **TerminalStream/EventStream = observation superpower.** We see host commands + agent events
   live. Combine with triggering RPC flows (SSH start, VSCode start, jupyter, git export) to map
   every host-side command the agent runs, then hunt for user-input → host-exec sinks in what it runs.
2. **`start_jupyter_server.sh`** — runs `jupyter lab` on the host with `FRAME_ANCESTORS`/
   `NOTEBOOK_DIR`/`LOG_FILE` args; IL shows `GenerateFrameAncestors()` (agent-generated) + args
   `QuoteString`-quoted, invoked via docker-exec into container → low priority, but the script is
   host-side and the args ARE shell-interpolated (`> "$LOG_FILE"`, `$(...)` expansion) if any path
   is ever attacker-influenced.
3. **No terminal WRITE channel found yet** — TerminalStream is server-streaming only; the request
   is empty. If the agent has a terminal-input RPC (client-streaming terminal), it's not in this
   service. Check other assemblies (vsonline_common/core IL) for a terminal write path.
4. **`--secrets-file /root/.codespaces/shared/user-secrets-envs.json`** on the devcontainer CLI line
   — mounted & readable; already harvested in prior sessions.

## 7. Artifacts

- `research/beacon/tools/agentchat.js` — EventStream/TerminalStream client (usage in header)
- `research/beacon/tools/reflect.js` — gRPC reflection client (server: unimplemented, kept for future)
- `research/data/114/terminal_stream_capture.log` — host terminal replay (devcontainer up as root)
- `research/data/114/eventstream_raw.log` — raw event bus frames
- `research/beacon/tools/readfile.js` — still the workhorse for host file reads
