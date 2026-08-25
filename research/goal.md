# GOAL — read before every cycle (the anchor)

Find genuine flaws, inconsistencies, and "patterns that don't match" in
**Linux** — the kernel, its ABI, the core libc (glibc), and POSIX — in their
full existence, not just our own distro. Cross-reference our environment only
as evidence.

For every finding, answer four layers:

- **L1 Surface** — the surprising observable (with a repro that actually runs)
- **L2 Mechanism** — the exact kernel source / POSIX section / man page / ABI
  reason (citable)
- **L3 History** — why it got that way: design decision, accident, back-compat
  (name dates, kernel versions, people where possible)
- **L4 Principle** — the deeper idea it touches: logic, identity, resource
  accounting, information, concurrency

And produce the decisive answer every time:

> "If we change **X**, it works anywhere." — the principled, universal fix
> **or** the reason it can never be fixed (locked in by ABI/back-compat).

## Hard rules

1. Distinguish "counterintuitive but fully explained" from "genuinely open."
   Never dress a known mechanism up as a mystery. The depth is in WHY.
2. **Real flaws only.** No trivia, no "rm -rf on /" jokes. Prefer flaws where
   the kernel *lies about its own semantics* or accretes a parallel "correct"
   API next to a frozen wrong one.
3. Every fork in the road (a real choice of interpretation or branch) is
   logged in `forks.md` *before* we choose a path.
4. Save findings as we go — the loop checkpoints every 4 minutes.
5. Restate this goal at the top of every cycle so we never steer away.