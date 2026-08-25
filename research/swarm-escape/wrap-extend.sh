#!/bin/bash
# wrap-extend.sh — fork a wrapper script with our extension (plug-in).
#
# Usage:
#   ./wrap-extend.sh <target>          patch target (idempotent)
#   ./wrap-extend.sh --revert <target> restore the backed-up original
#   ./wrap-extend.sh --list            show patch state of known wrappers
#
# Design:
#   - The agent executes wrappers from fixed paths; we cannot change the path,
#     so we patch IN PLACE: original body preserved verbatim, our extension
#     inserted right after the shebang. Behavior of the base command is kept.
#   - First patch backs the original up to $BACKUP_DIR (host-visible, survives
#     rebuilds) so the fork is always revertible.
#   - Idempotent: a marker line in the header prevents double injection, even
#     after the agent re-extracts a fresh copy (re-run patches again).
#   - Extensions are kept OUTSIDE the target as a file the injected header
#     sources, so updating the payload never requires re-patching.

set -u

KIT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXT_SRC="${KIT_DIR}/ext-beacon.sh"
# Backup dir lives in a host-visible mount so originals survive rebuilds.
BACKUP_DIR="${WRAP_BACKUP_DIR:-/workspaces/.codespaces/shared/wrappers/orig}"
MARKER="WRAP-EXT-BEACON"

require_target() {
  if [ -z "${1:-}" ] || [ ! -f "$1" ]; then
    echo "usage: wrap-extend.sh [--revert] <target-script>" >&2
    exit 2
  fi
}

already_patched() {
  grep -q "$MARKER" "$1" 2>/dev/null
}

patch() {
  local target="$1"
  require_target "$target"
  mkdir -p "$BACKUP_DIR"
  local name; name="$(basename "$target")"
  local orig="$BACKUP_DIR/${name}.orig"

  if already_patched "$target"; then
    echo "[wrap-extend] already patched: $target"
    return 0
  fi

  # First-time backup (never overwrite an existing original)
  if [ ! -f "$orig" ]; then
    cp "$target" "$orig"
    echo "[wrap-extend] original saved -> $orig"
  fi

  local tmp; tmp="$(mktemp)"
  # Preserve the shebang (or emit a sh one), then inject the extension.
  if head -1 "$target" | grep -q '^#!'; then
    head -1 "$target" > "$tmp"
    echo "# ${MARKER} — fork of $(basename "$target") (see $BACKUP_DIR)" >> "$tmp"
    echo "[ -r \"${EXT_SRC}\" ] && . \"${EXT_SRC}\"" >> "$tmp"
    tail -n +2 "$target" >> "$tmp"
  else
    { echo '#!/bin/sh'
      echo "# ${MARKER} — fork of $(basename "$target") (see $BACKUP_DIR)"
      echo "[ -r \"${EXT_SRC}\" ] && . \"${EXT_SRC}\""
      cat "$target"
    } >> "$tmp"
  fi

  chmod --reference="$target" "$tmp"
  cp "$tmp" "$target"
  chmod +x "$target"
  rm -f "$tmp"
  echo "[wrap-extend] PATCHED: $target  (extension sources ${EXT_SRC})"
  echo "             original at: $orig"
}

revert() {
  local target="$1"
  require_target "$target"
  local name; name="$(basename "$target")"
  local orig="$BACKUP_DIR/${name}.orig"
  if [ ! -f "$orig" ]; then
    echo "[wrap-extend] no backup for $target (was it patched by us?)" >&2
    exit 1
  fi
  cp "$orig" "$target"
  chmod +x "$target"
  echo "[wrap-extend] REVERTED: $target <- $orig"
}

list_state() {
  echo "== wrapper patch state =="
  for w in "${KNOWN_WRAPPERS[@]:-}"; do
    if [ -f "$w" ]; then
      if already_patched "$w"; then echo "PATCHED  $w"; else echo "clean    $w"; fi
    else
      echo "missing  $w"
    fi
  done
  echo "== backups =="
  ls -la "$BACKUP_DIR" 2>/dev/null || echo "(no backups yet)"
}

case "${1:-}" in
  --revert) revert "${2:-}" ;;
  --list)   list_state ;;
  *)        patch "${1:-}" ;;
esac
