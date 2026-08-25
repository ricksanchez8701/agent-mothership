#!/bin/bash
# AGENT: config — which devcontainer.json fields make the CLI execute something
# ON THE HOST, and which are honored on --expect-existing-container runs.
CLI=/workspaces/.codespaces/bin/node_modules/@microsoft/vscode-dev-containers-cli/dist/node/devContainersCLI.js
OUT=/workspaces/agent-mothership/research/swarm-escape/data/config.md
mkdir -p "$(dirname "$OUT")"
{
echo "# Agent config — devcontainer CLI host-exec surface"
echo
echo "CLI: $CLI ($([ -f "$CLI" ] && wc -c < "$CLI" || echo MISSING) bytes)"
echo
echo "## Config fields that run commands (where: HOST vs CONTAINER)"
for field in initializeCommand onCreateCommand updateContentCommand postCreateCommand postStartCommand postAttachCommand postCreateCommands; do
  echo "### $field"
  grep -oE ".{80}${field}.{120}" "$CLI" 2>/dev/null | head -2 | sed 's/^/  /'
  echo
done
echo "## cliHost.exec call sites (host exec channel) — what args?"
grep -oE ".{60}cliHost\.exec.{100}" "$CLI" 2>/dev/null | head -6 | sed 's/^/  /'
echo
echo "## dockerExec call sites (container exec channel) — count"
grep -oE 'dockerExec[A-Za-z]*' "$CLI" 2>/dev/null | sort | uniq -c | sort -rn | head -8 | sed 's/^/  /'
echo
echo "## Does anything run initializeCommand outside container-create?"
grep -oE ".{100}initializeCommand.{100}" "$CLI" 2>/dev/null | head -4 | sed 's/^/  /'
} > "$OUT" 2>&1
echo "config agent done -> $OUT ($(wc -l < "$OUT") lines)"
