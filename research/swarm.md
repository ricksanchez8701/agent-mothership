# Swarm — synthesis rules

After every agent for a topic returns, the SYNTHESIZER:

1. Collects `data/{topic}/*.md` from all 5 agents.
2. Looks for CONTRADICTIONS between layers:
   - L2 says "principled" but L4 says "arbitrary" → the real finding.
   - L3 says "accident" but L4 says "deep" → investigate the tension.
3. Extracts the decisive statement:
   > "If we change X, it works anywhere" — or — "locked in by back-compat."
4. Identifies any FORKS and logs them in `forks.md`.
5. Writes `entries/{id}-{slug}.md` (the single source of truth).
6. Updates `findings/` (cross-topic patterns) and `queue.json` status.
7. Checkpoints (saves) every 4 minutes.

## Agent → layer map

| Agent      | Layer it owns |
|------------|---------------|
| recon      | L1 (surface)  |
| spec       | L2 (mechanism)|
| history    | L3 (history)  |
| physics    | L4 (principle)|
| philosophy | verdict + "change this → works anywhere" |
