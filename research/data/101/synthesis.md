# Synthesis — 101 select() reports closed fds as READY (the lossy-bitmap lie)

## Contradiction scan (swarm.md rule 2)

- **L2 (spec)** says the lie is an *artifact* — but a **forced** one: given a
  1-bit result channel, "ready" is the only sane encoding of "invalid".
- **L3 (history)** confirms the ABI is frozen (fd_set bitmap, 1024 cap, 40
  years of binaries); POSIX declared the case "undefined" instead of fixing it.
- **L4 (physics)** agrees with L2: information theory forces the encoding
  choice; a 1-bit channel cannot transmit a 3-state fact, and of the two
  possible lies, "ready" is the one that avoids silent deadlock.
- **No contradiction between layers.** All three independently conclude: the
  *need* is principled, the *bitmap* is arbitrary, the *lie* is forced, and the
  *lock-in* is total.

Genuine finding: **the kernel knowingly ships a documented lie** (man page
BUGS section literally says closed-mid-wait fds are reported ready) because the
ABI cannot represent the truth — and the lie has a DoS shape (busy-loop).
Second finding: select() also corrupts its own input argument (writes remaining
time into the caller's struct timeval) — a Linux-only deviation from POSIX.

## Decisive statement

> **"If we change select()'s result set, it breaks every binary — the fd_set
> bitmap cannot encode errors, so the lie is locked in by ABI. The fix that
> works anywhere already exists: a per-fd event mask with an explicit invalid
> symbol (POLLNVAL) and no input mutation — i.e., poll/ppoll. select() is the
> fossil; poll/epoll are the accreted parallel."**

## Cross-topic pattern (→ findings/)

- **PATTERN-1 confirmed on the kernel:** select (wrong, frozen) → poll/epoll
  (correct, accreted) is the same disease as JS's `===`/`indexOf` vs
  `Object.is`/`includes`. The oldest API is frozen wrong; the right one is
  added beside it; the wrong one never dies.
- **NEW PATTERN-2 (kernel):** *the alphabet of a result type is the contract.*
  Interfaces with insufficient output capacity export their truth through
  whatever port is available — including corrupting their own arguments
  (select's timeval mutation). Under-powered APIs are forced to lie.

## Fork

Logged **FORK-003** in forks.md: is select()'s "ready for a closed fd" a kernel
flaw, or is the *contract* "don't close fds while select waits" (POSIX:
undefined) making it user error? Chosen: **it is a kernel flaw** — because the
kernel actively *chooses* "ready" (documented, not accidental), multi-threaded
apps cannot always prevent cross-thread fd close, and the expressiveness of the
API is the true cause.