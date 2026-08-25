#!/bin/bash
# AGENT: terminal — can we WRITE to the agent's terminal? Hunt all input paths.
IL=/workspaces/agent-mothership/research/beacon/tools
OUT=/workspaces/agent-mothership/research/swarm-escape/data/terminal.md
mkdir -p "$(dirname "$OUT")"
{
echo "# Agent terminal — every terminal input/write mechanism in the agent"
echo
echo "## 1. gRPC methods mentioning terminal/input/write/keys (all ILs)"
grep -noE '(Terminal|TerminalInput|WriteTo|SendKeys|Keyboard|KeyStroke|TerminalInput)[A-Za-z]*' "$IL/codespaces.il" 2>/dev/null | sort -t: -k2 | uniq -c -f1 | sort -rn | head -20
echo
echo "## 2. Client-streaming RPCs (client can push data) — search for IAsyncStreamReader in service impls"
grep -n 'IAsyncStreamReader' "$IL/codespaces.il" 2>/dev/null | head -10
echo
echo "## 3. TerminalStreamRequest fields (is it REALLY empty?)"
awk '/class public auto ansi sealed beforefieldinit TerminalStreamRequest/,/^  \}/' "$IL/codespaces.il" | grep -E 'FieldNumber|field ' | head -10
echo
echo "## 4. EventStream event TYPES the agent emits (looks for command-ish types)"
grep -oE '"(Subscribed|EnvironmentConfigured|[A-Za-z]+Command[A-Za-z]*|Terminal[A-Za-z]*|Output[A-Za-z]*|Command[A-Za-z]*Completed)"' "$IL/codespaces.il" 2>/dev/null | sort | uniq -c | sort -rn | head -25
echo
echo "## 5. Any pty / pts / ioctl / TIOCSTI (terminal device writes)"
grep -noE '(TIOCSTI|/dev/pts|pts/[0-9]|ioctl|pty)[A-Za-z0-9/._]*' "$IL/codespaces.il" "$IL/vsonline_core.il" 2>/dev/null | sort -u | head -15
echo
echo "## 6. ShellHelper::ExecuteAndGetOutput + ProcessStartInfo with RedirectStandardInput=true (interactive = writable stdin)"
grep -n 'set_RedirectStandardInput' "$IL/vsonline_core.il" "$IL/codespaces.il" 2>/dev/null | head -10
} > "$OUT" 2>&1
echo "terminal agent done -> $OUT ($(wc -l < "$OUT") lines)"
