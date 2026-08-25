#!/bin/bash
# AGENT: sinks — every host-side ProcessStartInfo / ExecuteCommand call site.
OUT=/workspaces/agent-mothership/research/swarm-escape/data/sinks.md
IL=/workspaces/agent-mothership/research/beacon/tools
mkdir -p "$(dirname "$OUT")"
{
echo "# Agent sinks — host-side command execution call sites in the IL"
echo
echo "## 1. CommandLineProvider::ExecuteCommandAsync (host runner: setsid --wait <program> <args>)"
grep -n 'ExecuteCommandAsync' "$IL/vsonline_core.il" | head -8 | sed 's/^/  /'
echo
echo "## 2. Call sites that reach a ProcessStartInfo with a program string — dump with context"
for f in "$IL/codespaces.il" "$IL/vsonline_core.il" "$IL/vsonline_common.il"; do
  echo "### $(basename "$f")"
  # find lines with ldstr program names near ProcessStartInfo
  grep -n 'ProcessStartInfo::' "$f" | head -12 | sed 's/^/  /'
  echo
done
echo "## 3. Every ldstr of a command/program name (heuristic: likely FileName values)"
grep -rhoE 'ldstr "(bash|sh|/bin/[a-z]+|python[0-9.]*|git|docker|node|setsid|azcopy|jupyter|vscode-dev-containers-cli|codespaces)"' "$IL"/*.il 2>/dev/null | sort | uniq -c | sort -rn | head -20 | sed 's/^/  /'
echo
echo "## 4. ExecuteReadOutputAndErrorOutputAsync (host command + capture)"
grep -n 'ExecuteReadOutputAndErrorOutputAsync' "$IL/codespaces.il" | head -10 | sed 's/^/  /'
} > "$OUT" 2>&1
echo "sinks agent done -> $OUT ($(wc -l < "$OUT") lines)"
