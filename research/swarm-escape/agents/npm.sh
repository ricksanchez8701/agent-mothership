#!/bin/bash
# AGENT: npm — writable mount files the agent EXECUTES as a host process.
# A replaceable host-run script = host RCE. Deliver the verdict per file.
OUT=/workspaces/agent-mothership/research/swarm-escape/data/npm.md
IL=/workspaces/agent-mothership/research/beacon/tools
mkdir -p "$(dirname "$OUT")"
{
echo "# Agent npm — mount files the agent might run (host vs container)"
echo
echo "## Candidates in writable mount (/.codespaces/bin = host /.codespaces/agent/mount)"
echo
for f in file-syncer.js file-syncer-bridge.js codespaceStatusTool.js gitcredential_github.sh start_jupyter_server.sh installSSH.sh sourcer.sh mount_data_disk.sh backup_data_disk_images.sh smbclientlogs.sh installCWTools.sh; do
  found=$(grep -l "$f" "$IL"/*.il 2>/dev/null)
  hits=$(grep -c "$f" "$IL"/*.il 2>/dev/null | grep -v ':0' | tr '\n' ' ')
  echo "### $f"
  echo "- in-mount: $([ -e "/.codespaces/bin/$f" ] && echo yes || echo no)"
  echo "- IL references: ${hits:-NONE}"
  if [ -n "$found" ]; then
    echo "- first reference context:"
    grep -n "$f" "$IL/codespaces.il" "$IL/vsonline_core.il" "$IL/vsonline_common.il" 2>/dev/null | head -4 | sed 's/^/    /'
  fi
  echo
done
echo "## KEY QUESTION: does anything run these from the MOUNT path on the HOST?"
echo "- VmCLICopyFolder getter (vsonline_common.il) decides the mount path;"
echo "- grep for get_VmCLICopyFolder call sites to see what runs from it."
grep -n 'get_VmCLICopyFolder' "$IL/vsonline_common.il" "$IL/codespaces.il" 2>/dev/null | head -10 | sed 's/^/  /'
} > "$OUT" 2>&1
echo "npm agent done -> $OUT ($(wc -l < "$OUT") lines)"
