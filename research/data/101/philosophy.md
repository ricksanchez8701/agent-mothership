# L5 — Philosophy: what this reveals + the honest verdict

## What it reveals about the kernel

Linux (and Unix) treat the oldest, most-used primitives as **unchangeable
sculpture**. select() was designed before the OS community understood error
models, and its ABI is now sacred. The kernel's response to its own flaw is
never to fix it — it is to **ship a parallel correct API** and wait for the
world to migrate. Four readiness APIs (select/pselect/poll/ppoll/epoll) now
live in the same kernel. That is honest engineering under a "never break ABI"
religion, but it means the most common path (select, still used by countless
programs and libs) is permanently the lying one.

## Is the obvious fix philosophically right?

The obvious fix — "make select report the bad fd" — is philosophically correct
(a readiness API must be able to say "this is invalid") but **impossible**: the
result set is a bitmap with no error bits, and the ABI cannot grow. The second
fix — "at least define the closed-during-select case" — POSIX *already* did the
only thing it could: declared it **undefined**. Declaring a documented lie
"undefined" is not a fix; it is a footnote that ships the lie.

## The honest verdict

**Genuinely locked in, permanently.** select()'s lie is information-theoretically
forced (see L4) AND ABI-frozen (see L3). It can never be repaired in place. The
only honest escape is the accreted family — poll/epoll — which is why the kernel
built them. This is the purest kernel instance of our PATTERN-1: *the wrong API
is frozen, the right one is added beside it, and the wrong one never dies.*

## "If we change X, it works anywhere"

1. **Inside Linux: nothing works.** The fd_set bitmap cannot encode errors;
   changing it breaks every binary. The timeout mutation is equally frozen.
   Locked in permanently.
2. **A new design / new kernel:** ship **one** readiness API with a per-fd
   event mask including an explicit "invalid" symbol — i.e., what poll/epoll
   already are — and **never mutate input arguments**. That is precisely the
   design that "works anywhere": it is why poll() survived and select() is the
   fossil. The lesson: *the alphabet of your result type is the contract; if it
   cannot express the truth, your API will lie forever.*

**One-liner for the findings log:** *select() is a 1-bit channel reporting a
3-state fact — it must encode "this fd is gone" as "ready", the kernel documents
this lie, POSIX papers over it with "undefined", and poll/epoll exist only
because the bitmap can never be fixed.*