#!/bin/bash
# AGENT: rpccmd — the definitive "commands the agent will run" table.
IL=/workspaces/agent-mothership/research/beacon/tools
OUT=/workspaces/agent-mothership/research/swarm-escape/data/rpccmd.md
mkdir -p "$(dirname "$OUT")"
{
echo "# Agent rpccmd — every command the agent executes on OUR input"
echo
echo "## RPC -> program -> where -> user input reach"
echo "| RPC | program | where | user input |"
echo "|---|---|---|---|"
echo "| CodespaceHost/GetFileContentAsync | (file read) | HOST root | Path (lexical-checked, symlink = host read) |"
echo "| CodespaceHost/InvokeSecondaryEditsAsync | python codeplan.py --config-file | HOST | ConfigJson/Task/Params |"
echo "| CodespaceHost/ConfigureEnvironmentAsync | codespaces configure | HOST | none |"
echo "| CodespaceHost/TerminateOryxTaskAsync | pkill -f oryx (docker) | container | none |"
echo "| VSCodeServerHost/StartRemoteServerAsync | vscode-dev-containers-cli start-server | HOST proc / container exec | VSCodeCommit, Quality, Extensions[], Version |"
echo "| VSCodeServerHost/ShutdownRemoteServerAsync | (server stop) | container | none |"
echo "| SshServerHost/StartRemoteServerAsync | installSSH.sh (docker exec) | container | UserPublicKey |"
echo "| JupyterServerHost/GetRunningServer | sh -c 'command -v jupyter' (docker exec) | container | none |"
echo "| OobControl/Shutdown | host shutdown | HOST | none (grpc 7 denied) |"
echo
echo "## docker exec commands the agent builds (container-side, but is any string USER-influenced?)"
echo "### jupyter / oom / oryx / ssh flows — find the sh -c strings"
grep -oE 'sh -c [^;]{0,80}' "$IL/codespaces.il" 2>/dev/null | head -10 | sed 's/^/  /'
echo
echo "### StartRemoteServerAsync commit->command flow (candidate injection)"
grep -n 'StartRemoteServerUsingContainerCliAsync\|VSCodeCommit' "$IL/codespaces.il" 2>/dev/null | head -8 | sed 's/^/  /'
echo
echo "## Interpolated-string commands with ldloc/ldarg (user data) near docker exec"
grep -n 'AppendFormatted' "$IL/codespaces.il" | head -15 | sed 's/^/  /'
} > "$OUT" 2>&1
echo "rpccmd agent done -> $OUT ($(wc -l < "$OUT") lines)"
