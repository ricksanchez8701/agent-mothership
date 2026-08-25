# L3 — History: why select() got this way

- **1983, 4.2BSD** — select() is introduced (with the fd_set bitmap and
  FD_SETSIZE of 1024/256) to multiplex sockets. It is one of the first
  asynchronous-I/O primitives. The result is a **bitmap** because that matched
  the bitmap *input*: an in-place, no-allocation design for a 1980s kernel. The
  error model is an afterthought — the API returns `int` + bitmap, and an error
  only names "the whole call", never "which fd".
- **~1986, System V** — poll() arrives as SysV's competing multiplexer. It was
  designed around a per-fd **event mask** (`revents`), explicitly including
  `POLLNVAL` — "fd not open". SysV had already learned the lesson select's ABI
  could not absorb: per-fd error symbols. Two families coexist to this day.
- **1990s** — POSIX standardizes both (POSIX.1-2001). POSIX **declares the
  closed-during-select case "undefined"** rather than fixing it — an explicit
  admission that the ABI cannot be repaired. The man page's Linux-specific
  "reported as ready" note fossilizes the lie.
- **Linux** — select() is implemented in core/select.c; the timeout-mutation
  (writing remaining time into the caller's struct timeval) is a Linux
  deviation from POSIX that persists for compat with apps that *expect* it.
- **2002 (Linux 2.5.44)** — Davide Libenzi merges **epoll**: O(1) readiness
  with an interest list the kernel owns. The third generation. Each generation
  exists because the previous one could not be fixed:
  select (1-bit bitmap, no error symbol, 1024 cap) → poll (per-fd events,
  O(n)) → epoll (O(1), edge-triggered). **None replaces the others; all four
  (select/pselect/poll/ppoll/epoll) ship in the same kernel today.**
- **Back-compat lock** — the fd_set layout is a public ABI: every C program,
  every interpreter, every libc calls into it. Changing the result encoding or
  adding error symbols breaks 40 years of binaries. The lie is permanent.

**Pattern:** the oldest API is frozen wrong, and the kernel accretes a parallel
"correct" family beside it — the same accretion disease we found in JavaScript
(`===`/`indexOf` frozen, `Object.is`/`includes` added alongside). PATTERN-1 of
our findings log holds for the kernel.