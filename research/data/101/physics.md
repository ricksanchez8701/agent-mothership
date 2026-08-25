# L4 — Physics: the deeper principle

## What it touches: channel capacity / information theory

select()'s result is a **channel**: a set of bits whose alphabet is
{0, 1} = {not-ready, ready}. The actual state of an fd at any moment belongs to a
larger alphabet — at least {readable, writable, exceptional, EOF, error,
**invalid/closed**}. The channel must transmit the state to the caller, but it
has only two symbols.

Information theory is blunt about this: **a 1-bit channel cannot transmit a
3-state fact.** The sender (kernel) must encode the 3rd state ("invalid") as
either 0 or 1, because there is no other symbol. Encoding it as "not-ready" (0)
means the caller sleeps forever waiting for an event that will never come — a
**silent deadlock**. Encoding it as "ready" (1) means the caller wakes and gets
EBADF — a **detectable, if misleading, error**. Faced with deadlock-vs-lie, the
kernel chose the lie. That is not malice; it is the only correct engineering
choice a 1-bit channel permits. The *flaw* is the channel, not the encoder.

## The sharp statement

> select() is a lossy channel. The truth it destroys ("which fd is invalid")
> is precisely the information the caller needs to recover. poll()/epoll()
> have a richer alphabet and therefore don't have to lie.

## Why the lie is the *maximally dangerous* one

Of the two possible encodings, "ready" is the one that looks like *action
required*. It turns a hard failure into a busy-loop: select → "ready" → read →
EBADF → select → "ready" → ... The caller burns CPU chasing a phantom. This is a
DoS-shaped failure hiding inside a documented compatibility note.

## The second principle: side effects on arguments

select() writing remaining time into the caller's `struct timeval` violates a
deeper contract — **a function should not mutate the inputs it was given to
read**. (POSIX agrees; BSD agrees; Linux alone scribbles.) This is the same
class of violation as the fd_set problem: the interface *smuggles extra output*
through the input, because the output channel was too small. Both facets are the
same disease: **an interface with insufficient output capacity will export its
truth through whatever port is available** — including corrupting its own
arguments.

## Principled or arbitrary?

- The **need** to report invalid fds is principled (a readiness API must
  distinguish "nothing to do" from "the thing is gone").
- The **bitmap result** is arbitrary and historical.
- The **lie** (encode invalid as ready) is forced — given the bitmap, it is the
  information-theoretically correct choice. It is a *constrained* artifact:
  wrong, but uniquely the least-wrong option available.
- The **timeout mutation** is pure artifact: unforced, gratuitous, and wrong.