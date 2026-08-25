# L1 — Recon: select() reports a closed fd as READY — the lossy-bitmap lie

## Smallest runnable repro (empirically confirmed on kernel 6.8)

```c
int pfd[2]; pipe(pfd);
pid_t p = fork();
if (p == 0) { usleep(150000); close(pfd[0]); _exit(0); }  /* child closes read end mid-select */
close(pfd[0]);

fd_set rf; FD_ZERO(&rf); FD_SET(pfd[1], &rf);             /* watch the WRITE end (never readable) */
struct timeval tv = {3, 0};
int n = select(pfd[1] + 1, &rf, NULL, NULL, &tv);

/* result on Linux 6.8: n=1, FD_ISSET(pfd[1]) == 1 */
/* the fd was CLOSED — there is no data, no EOF, nothing. select lied. */
```

The read end of the pipe was closed while select() was waiting. The write end has
no data and can never have data (its reader is gone). Yet select() returned
"ready". The subsequent `read()`/`write()` returns EBADF — the program must then
figure out on its own which of its possibly-thousands of fds is dead.

Compare the honest API:

```c
struct pollfd p = {.fd = pfd[1], .events = POLLIN};
poll(&p, 1, 0);    /* revents == POLLNVAL — poll NAMES the bad fd explicitly */
```

## Naive expectation

`select()` returns only when an fd is actually readable/writable/exceptional, or
on timeout. A closed fd is neither readable nor writable — it should not be
reported ready, and if something is wrong, the error should say **which** fd.

## What exactly surprised me

1. select() returned "READY" for an fd that was closed while it waited — a lie
   the process cannot detect. poll() on the identical fd gives POLLNVAL, naming
   the culprit. One problem, two answers — same disease as JS's `typeof null`.
2. select() has **no error symbol at all**: the result is a bitmap of
   {ready, not-ready} and nothing else. It literally cannot express "invalid".
3. Even the *input* is corrupted: on Linux, select() **writes the remaining
   time back into your `struct timeval`** — a function that mutates its own
   argument.
4. This causes real busy-loop DoS: a select-based server whose client vanishes
   mid-wait spins forever on "ready → read → EBADF → select → ready".
5. It is unfixable: the fd_set ABI (a fixed 1024-bit bitmap) has no room for
   error codes, and 40 years of code depends on its layout.