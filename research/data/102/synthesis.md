# Synthesis — 102 mmap past EOF + truncate → SIGBUS: the kernel kills reads of a mapping it allowed

Run the swarm synthesis (see swarm.md):
1. Collect the 5 agent outputs in this folder.
2. Find contradictions between L2/L3/L4.
3. Extract the decisive statement ("if we change X, it works anywhere" or "locked in").
4. Log any forks in forks.md.
5. Write the entry to entries/ and update findings/.
