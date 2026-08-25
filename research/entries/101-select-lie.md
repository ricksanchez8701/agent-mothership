# Entry 101 — select() reports a closed fd as READY: the lossy-bitmap lie

- **Language:** Linux (kernel 6.8, glibc 2.36) — empirically tested
- **Topic:** select() / closed-mid-wait fd / lossy result channel
- **Status:** complete
- **Fork:** FORK-003
- **Repro:** research/probes/discover.c (probes 1, 2, 3)

---

## L1 — Surface (the surprising observable)

```c
int pfd[2]; pipe(pfd);
pid_t p = fork();
if (p == 0) { usleep(150000); close(pfd[0]); _exit(0); }  /* close read end mid-wait */
close(pfd[0]);

fd_set rf; FD_ZERO(&rf); FD_SET(pfd[1], &rf);   /* watch write end — can never be readable */
struct timeval tv = {3, 0};
int n = select(pfd[1] + 1, &rf, NULL, NULL, &tv);
/* observed: n=1, FD_ISSET(pfd[1]) == 1  — but the fd is CLOSED. select lied. */
```

A closed fd is reported **READY**. The subsequent operation returns EBADF, and
the caller must hunt for which of its fds died. `poll()` on the identical fd
returns `POLLNVAL`, naming the culprit. One problem, two answers — and the
select answer is a lie the process cannot detect.

**The naive expectation:** select returns when an fd is actually ready, or on
timeout. A closed fd is neither readable nor writable; it should not be reported
ready, and an error should name the fd.

**Bonus lie:** on Linux, select() **writes the remaining time back into the
caller's `struct timeval`** — a function that corrupts its own input argument
(POSIX and BSD do not; Linux alone does).

---

## L2 — Mechanism (kernel source + man page)

The fd_set is a fixed 1024-bit bitmap. select()'s output alphabet per fd is
exactly {bit-set, bit-clear} = {ready, not-ready}. **There is no symbol for
"invalid".**

Man page `select(2)`, BUGS section (quoted):
> "If a file descriptor in the monitored set is closed by another thread while
> select() is blocked, the result is undefined. **On Linux, the file descriptor
> will be reported as ready for reading/writing.**"

That is a *documented* lie. The kernel pins the file object at entry (`fdget`),
installs its poll table entry; on close the fd vanishes from the table but the
object outlives it, and on re-scan, unable to report "invalid" through the
bitmap, the ready bit is set — the least-bad lie available.

Also documented (select(2), NOTES): the timeout mutation — "On Linux, select()
modifies timeout to reflect the amount of time not slept; most other
implementations do not."

poll()/ppoll() are honest where select() lies: their per-fd `revents` mask has
an explicit `POLLNVAL` symbol. epoll extends the family. The same event is
expressible by poll, inexpressible by select.

**Verdict:** the *need* is principled, the *bitmap* is arbitrary/historical,
the *lie* is forced by the 1-bit channel, the *lock-in* is total (ABI).

---

## L3 — History (how it got this way)

- **1983, 4.2BSD** — select() ships with the fd_set bitmap (FD_SETSIZE 1024).
  A bitmap result matching the bitmap input: in-place, no allocation, 1980s
  kernel. Error reporting was an afterthought — errors name the call, never
  the fd.
- **~1986, System V** — poll() is designed as the competing multiplexer with a
  per-fd event mask including `POLLNVAL` ("fd not open"). SysV had already
  learned the lesson select's ABI could not absorb.
- **1990s** — POSIX standardizes both and **declares closed-during-select
  "undefined"** rather than fixing it. The Linux man page fossilizes the lie.
- **2002, Linux 2.5.44** — Davide Libenzi merges **epoll**: O(1) readiness,
  edge-triggered. Third generation. All of select/pselect/poll/ppoll/epoll
  ship in the same kernel today; none replaced the others.
- **Back-compat lock** — the fd_set layout is public ABI. Every program, every
  libc, every interpreter calls into it. Changing it breaks 40 years of
  binaries. The lie is permanent.

**Pattern:** the oldest API is frozen wrong; the kernel accretes a parallel
"correct" family beside it. Same disease as JS (`===`/`indexOf` frozen,
`Object.is`/`includes` added). PATTERN-1 confirmed for the kernel.

---

## L4 — Principle (the deeper idea)

select()'s result is a **channel** with alphabet {0,1} = {not-ready, ready}. The
actual fd state belongs to a larger alphabet that includes **invalid/closed**.
Information theory is blunt: **a 1-bit channel cannot transmit a 3-state fact.**
The encoder must map "invalid" to 0 or 1:

- as **0 (not-ready)** → the caller sleeps forever → *silent deadlock*
- as **1 (ready)** → the caller wakes, gets EBADF → *detectable, misleading*

The kernel chose "ready" — the only correct engineering choice a 1-bit channel
permits. **select() is a lossy channel; the truth it destroys ("which fd is
invalid") is exactly what the caller needs.** And of the two possible lies,
"ready" is the one that looks like *action required* — turning a hard failure
into a busy-loop. A DoS-shaped failure hiding in a compatibility note.

Second principle: **an interface with insufficient output capacity exports its
truth through whatever port is available — including mutating its own
arguments.** The timeval scribbling is the same disease as the bitmap lie.

---

## L5 — Verdict + decisive answer

**What it reveals:** Unix treats its oldest, most-used primitives as unchangeable
sculpture. select() predates the OS community's error-model thinking, and its
ABI is now sacred. The kernel never fixes; it accretes (poll, epoll) and waits
for migration — leaving the most common path permanently the lying one.

**Is the obvious fix philosophically right?** Yes — a readiness API must be able
to say "this is invalid" — but it is **impossible**: the result bitmap has no
error bits and cannot grow. POSIX's response — declaring the case "undefined" —
is a footnote that ships the lie, not a fix.

**Verdict:** genuinely locked in, permanently. The lie is information-forced AND
ABI-frozen. The only honest escape is the accreted family. Purest kernel
instance of PATTERN-1.

> **If we change X, it works anywhere:**
> 1. **Inside Linux: nothing works.** The fd_set bitmap cannot encode errors;
>    the timeout mutation is equally frozen. Locked in by ABI.
> 2. **A new design:** ship **one** readiness API with a per-fd event mask
>    including an explicit "invalid" symbol and no input mutation — exactly
>    poll/ppoll. *The alphabet of your result type is the contract; if it
>    cannot express the truth, your API will lie forever.*

**One-liner:** *select() is a 1-bit channel reporting a 3-state fact — it must
encode "this fd is gone" as "ready", the kernel documents this lie, POSIX
papers over it with "undefined", and poll/epoll exist only because the bitmap
can never be fixed.*