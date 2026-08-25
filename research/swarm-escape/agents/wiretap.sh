#!/bin/bash
# AGENT: wiretap — long-running watcher on EventStream + TerminalStream.
# Triggers benign RPCs periodically to provoke agent activity, logs everything.
OUT=/workspaces/agent-mothership/research/swarm-escape/data/wiretap.log
mkdir -p "$(dirname "$OUT")"
WINDOW="${WIRETAP_SECONDS:-1500}"   # 25 min default
cp /workspaces/agent-mothership/research/beacon/tools/agentchat.js /tmp/agentchat.js 2>/dev/null
cp /workspaces/agent-mothership/research/beacon/tools/readfile.js /tmp/readfile.js 2>/dev/null

{
echo "=== wiretap started $(date -u +%FT%TZ) window=${WINDOW}s ==="
# passive listeners first (background), then provoke
(node /tmp/agentchat.js --events 50 --dur "$WINDOW" > /tmp/wiretap_events.log 2>&1 &) 
(node /tmp/agentchat.js --terminal 500 --dur "$WINDOW" > /tmp/wiretap_term.log 2>&1 &)
echo "listeners up: events->/tmp/wiretap_events.log term->/tmp/wiretap_term.log"
i=0
while [ "$i" -lt "$((WINDOW / 90))" ]; do
  echo "--- provoke round $i $(date -u +%FT%TZ) ---"
  node /tmp/readfile.js /Codespaces.Grpc.CodespaceHostService.V1.CodespaceHost/GetPerformanceDataAsync 0a00 2>&1 | head -1
  node /tmp/readfile.js /Codespaces.Grpc.CodespaceHostService.V1.CodespaceHost/IsRecoveryContainerAsync 0a00 2>&1 | head -1
  node /tmp/readfile.js /Codespaces.Grpc.JupyterServerHostService.V1.JupyterServerHost/GetRunningServer 0a00 2>&1 | head -1
  sleep 90
  i=$((i+1))
done
echo "=== wiretap done $(date -u +%FT%TZ) ==="
} > "$OUT" 2>&1
# summarize
{
  echo
  echo "=== event stream summary ==="
  grep -aoE '"(type|id)": "[^"]*"' /tmp/wiretap_events.log 2>/dev/null | sort | uniq -c | sort -rn | head -15
  echo "=== terminal stream tail ==="
  tail -20 /tmp/wiretap_term.log 2>/dev/null
} >> "$OUT" 2>&1
echo "wiretap agent done -> $OUT"
