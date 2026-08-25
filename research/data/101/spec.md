# L2 — Spec: the exact mechanism (kernel source + man page)

## The lossy result set

select()'s interface is the fd_set: a fixed `FD_SETSIZE` (1024 on Linux/glibc)
bit bitmap. The caller ANDs FD_SET bits in, the kernel ANDs result bits out. The
output alphabet is exactly {bit-set, bit-clear} per fd — **"ready" or "not
ready".** There is no third symbol for "this fd is invalid". The result set's
expressive power is the root cause (see L4).

## The mechanism (Linux man page, `select(2)`, BUGS section — quoted behavior)

> "If a file descriptor in the monitored set is closed by another thread while
> select() is blocked, the result is undefined. **On Linux, the file descriptor
> will be reported as ready for reading/writing**."

That is a *documented* lie. The kernel knows what it does and writes it down.
The mechanism behind it (core/select.c, `do_select()`): the fd is resolved at
entry via `fdget()`, which pins the file object, and the file's poll table entry
is installed. When the fd is closed mid-wait, the file is removed from the
process's fd table but the pinned file object outlives it; on the final re-scan
the code sees an entry that no longer corresponds to a live fd and, unable to
report "invalid" through the bitmap, the ready bit is set — the least-bad lie it
can tell, because *some* signal is safer than none, and the channel cannot carry
the truth.

(The exact expression differs across kernels — sometimes the bit is set by the
close wakeup, sometimes by the re-scan — but the *contract* is stable and
documented: closed-mid-wait ⇒ reported ready.)

## The input corruption, also documented

> "On Linux, select() modifies timeout to reflect the amount of time not slept;
> most other implementations do not do this." (select(2), NOTES)

POSIX.1g says the timeout is not modified. Linux writes remaining time back into
the caller's struct — a syscall that scribbles on its argument.

## Why poll() is honest where select() lies

poll() (and its variant ppoll) return a `revents` **bitmask per fd** with an
alphabet that includes `POLLNVAL` ("invalid fd"). It is not a 1-bit channel; it
has a symbol for "this is broken, here is which". So the same event (fd closed)
is expressible: select must lie, poll tells the truth. epoll extends the family
further (edge-triggered, scale).

## Principled or artifact?

The lie itself is an **artifact** — a historical design whose result set cannot
represent the event space. But note it is a *forced* artifact: given a 1-bit
channel, "ready" is the only sane encoding of "unknown/invalid" (see L4). The
input-timeout mutation is a pure, unnecessary artifact — it could have been
spec-correct and Linux chose not to be.