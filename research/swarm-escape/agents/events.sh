#!/bin/bash
# AGENT: events — what does the agent broadcast, and does any event type carry command/terminal data?
IL=/workspaces/agent-mothership/research/beacon/tools
OUT=/workspaces/agent-mothership/research/swarm-escape/data/events.md
mkdir -p "$(dirname "$OUT")"
{
echo "# Agent events — the event bus as a command channel"
echo
echo "## 1. Event types seen live in the wiretap (so far)"
grep -aoE '"type": "[^"]*"' /tmp/wiretap_events.log 2>/dev/null | sort | uniq -c | sort -rn | head -15
echo
echo "## 2. EventStreamResponse.Type values the client code handles (IL)"
grep -oE 'ldstr "[A-Za-z]+(Event|Changed|Updated|Started|Stopped|Output|Command|Terminal)[A-Za-z]*"' "$IL/codespaces.il" 2>/dev/null | sort | uniq -c | sort -rn | head -20
echo
echo "## 3. NotifyCodespaceOfClientActivity — what activities does the client send?"
awk '/class public auto ansi sealed beforefieldinit NotifyCodespaceOfClientActivityRequest/,/^  \}/' "$IL/codespaces.il" | grep -E 'FieldNumber|field ' | head -12
echo
echo "## 4. Is there any client->agent event (publish) path? IGrpcEventManager methods"
grep -n 'IGrpcEventManager' "$IL/codespaces.il" | head -10 | sed 's/^/  /'
echo
echo "## 5. EventStream impl: what does it DO with the subscription id / any request fields?"
sed -n '4085,4140p' "$IL/codespaces.il" | grep -E 'get_Id|AddSubscription|EventStreamRequest|EventStreamResponse|Type|Payload' | head -10 | sed 's/^/  /'
} > "$OUT" 2>&1
echo "events agent done -> $OUT ($(wc -l < "$OUT") lines)"
