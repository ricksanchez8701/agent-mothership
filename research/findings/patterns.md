# Findings — cross-topic patterns

Emerging patterns that recur across entries. Updated by the swarm synthesizer.

## PATTERN-1: The accreted parallel — "never fixes, always adds" (JS entries 001/002, kernel 101)

When a platform discovers its own frozen-in behavior is wrong, it does not fix
the old API (it can't — web/ABI compat). Instead it ships a parallel "correct"
API next to the wrong one, and the wrong one stays forever:

| Wrong (frozen)                 | Correct (added)          | World  |
|--------------------------------|--------------------------|--------|
| `===` / `indexOf` (NaN)        | `Object.is` / `includes` | JS     |
| `typeof null` → "object"       | `[object Null]`          | JS     |
| `select()` (1-bit bitmap)      | `poll`/`ppoll`/`epoll`   | Linux  |

**The disease:** multiple semantics, one name — or one semantics, an alphabet
too small to tell the truth — silently disagreeing. The wrong one is frozen;
the right one is accreted; the wrong one never dies.

## PATTERN-2: The alphabet of a result type is the contract (kernel 101)

Interfaces with insufficient output capacity are **forced to lie** — a 1-bit
channel cannot transmit a 3-state fact, so "invalid" must be encoded as one of
the two symbols. Worse: the truth that can't fit the output is exported through
whatever port is available, **including mutating the input** (select writes
remaining time into the caller's struct timeval).

**The rule:** if your result type cannot express the truth, your API will lie
forever. The lie's encoding is chosen to avoid the *worst* failure (deadlock) —
which is why the lie is usually the one that looks like "action required"
(busy-loop, not silence).

## Predictions (test on queued topics)

- `mmap`/truncate → SIGBUS (102): expect the "mmap success is a lease, not a
  promise" version of PATTERN-2 — the kernel grants a mapping it can't back,
  and the *signal* is the lie.
- `copy_file_range` EXDEV (106): expect an errno that punts the contract to the
  caller ("you reimplement copy"), a documentation-driven lie.

## PATTERN-3: The dangerous kernel lies are file-mediated, not signal/fd-mediated (kernel 102, 107)

The two findings that **cross the privilege boundary** both operate through
**shared files**, not signals or fd tricks:

- SIGBUS kill: process B does a *legal* `ftruncate` on a file process A mapped;
  the kernel kills A. B never touches A's credentials.
- chmod/chown transparency: the privileged *caller's* capabilities are applied
  to whatever inode a name resolves to; the link owner redirects them.

**The rule:** when a low-privilege process can write/own the *backing object*
(file, parent directory, symlink) of a privileged process's operation, the
kernel mediates the damage **without a capability check on the victim**. The
checks are all "does the actor have rights on the object" — never "is the
object's other holder protected from this actor." Cross-boundary impact comes
from sharing state, not from touching the other process.

## PATTERN-5: A string check is not a namespace check (entry 110 — the CVE)

A boundary control that validates a **lexical string** instead of the **resolved
object** is defeated by a symlink the attacker already owns:

- `Path.GetFullPath(join(workdir, path))` normalizes `.`/`..` but never resolves
  links. `StartsWith(workdir)` passes for a link *inside* the workspace even when
  its target is anywhere on the host.
- The read (StreamReader on the host, as root) follows the link to the real
  target. Result: **arbitrary host file read** from a container whose "scoped"
  file API was supposed to be confined to the workspace.

**The rule:** any scoping check on a path must be re-run on the resolved target
(`realpath`/`RESOLVE_BENEATH`), never trusted from the lexical string. This is
the same "shared object, victim's checks check the attacker's string" shape as
PATTERN-3, but against a *platform* (agent RPC) instead of the kernel.

## PATTERN-4: An errno is a fingerprint (kernel 108)

Seccomp filters translate a *policy* into *errno values*. Because the errno
choice is part of the profile (uniform EPERM, one ENOSYS exception for clone3),
an unprivileged process can enumerate the entire allow/deny list with zero
permissions and zero traces. Security posture leaks through error codes —
every blocked action teaches the attacker the shape of the wall.