#!/bin/bash
# probe_host.sh <host-path> [label]
# Creates a fresh symlink in the workspace pointing at <host-path>, waits for
# fuse propagation (~3s), reads it via the gRPC primitive, then removes the link.
# CRITICAL: always use a FRESH link name + sleep 3 — stale links read stale targets.
set -u
TARGET="$1"
LABEL="${2:-$(basename "$TARGET")}"
LINK="/workspaces/agent-mothership/research/.ph_$(date +%s%N)_$RANDOM"
ln -sf "$TARGET" "$LINK"
sleep 3
echo "=== probe $LABEL -> $TARGET ==="
node /tmp/readfile.js "research/$(basename "$LINK")"
rm -f "$LINK"
