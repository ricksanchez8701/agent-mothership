# Entry 102 — mmap + truncate → SIGBUS: the nobody→root kill primitive

- **Language:** Linux (kernel 6.8, verified live)
- **Topic:** mmap's unenforceable promise + the shared-file kill
- **Status:** complete
- **Severity:** HIGH — crosses the user privilege boundary
- **Repro:** research/probes/weaponized/lie2_sigbus.c

---

## L1 — Surface (the surprising observable, verified)

A root process `mmap()`s a file it owns (mode 0666) and touches the pages. An
unprivileged process (`nobody`, uid 65534) that can also write that file (or any
file in a shared/world-writable location) calls `ftruncate(fd, 0)`. The next
read from the mapping **kills the root process with SIGBUS** — the kernel
executes the kill; nobody cannot even `kill(rootpid, 0)` (EPERM), yet the root
victim dies.

Verified output:
```
[parent] victim died via signal=7 (SIGBUS), WIFSIGNALED=1
```

Even with a `sigaction(SIGBUS, handler)` installed, the victim **cannot
resume**: the faulting instruction re-executes after the handler → infinite
SIGBUS storm. The victim must `_exit()` in the handler or die.

## Naive expectation

`mmap()` returned success. A file operation (truncate) is a normal thing. A
mapped region should not become *lethal* because the file behind it shrank.
And "you can't hurt a process you can't signal" should be a safety property.

## What actually happened

The kernel allowed the mapping (a *lease* on the file's pages), then allowed a
second process to revoke the backing store, and answered the resulting fault
with SIGBUS — an uncatchable-in-practice crash. The victim process had no way
to discover the truncation or defend against it.

---

## L2 — Mechanism

`mmap()` pins `file->f_mapping` pages. `truncate()`/`ftruncate()` moves the
inode size (`i_size`) below the mapped extent. On the next access to a page
beyond the new `i_size`, the page-fault handler (`mm/filemap.c`, `filemap_fault`)
finds the page is beyond EOF and delivers SIGBUS — there is no backing page and
no valid zero-fill contract for file mappings. SIGBUS on a mapped-IO fault
re-executes the faulting instruction after any handler, so a handler cannot
"recover"; the only exits are `_exit` or death.

The boundary mechanics that make this a *cross-privilege* primitive:
- `mmap` grants a handle on the file's pages to process A (root).
- `ftruncate` only needs write access to the *fd* process B holds — which is
  satisfied by file ownership/mode, **not by any check against process A**.
- SIGBUS is delivered by the kernel to A as a side effect of B's legal
  operation. There is no capability check at the moment of death: B never
  touches A.

---

## L3 — History

mmap's contract dates to the 1980s (4.2BSD, SunOS `mmap` of `/dev/zero`);
the "SIGBUS on truncation" behavior is inherited from the original design where
a mapping was a view of the file. The model predates shared-multiuser threat
modeling: on a single-user workstation, "your file shrank" was benign. In a
multiuser/container world, the mapping is a **standing vulnerability surface**
— anyone who can write the backing file can kill every process that mapped it.
The `mmap_min_addr`, `MAP_FIXED` and fd-based fixes all accreted around it;
the SIGBUS-on-truncate contract itself was never redesigned (it cannot be: it
is the memory model).

---

## L4 — Principle

A mapping is a promise the kernel cannot keep: "these pages will be there." The
file is a second process's property; truncation revokes the promise. The kernel
must answer a fault on a revoked page — and its only answers are "give data"
(impossible), "zero-fill" (a lie), or "SIGBUS" (death). It chose death, and
because the death signal re-faults after a handler, it is **unrecoverable** —
the kernel's error is a one-way door.

Deeply: **the failure mode of a shared resource is delivered as a process
kill, not an error.** The kernel has no channel to say "your mapping was
revoked, please retry"; it can only terminate. This is PATTERN-2 again: an
under-powered notification channel (signal-with-faulting-instruction) forces
the worst encoding (death).

---

## L5 — Verdict + decisive answer

**Severity: HIGH.** It crosses the privilege boundary: nobody (or any
low-privilege tenant) can kill processes that map files the attacker can write.
Real-world shapes: kill a log-rotation daemon, a cache daemon, a database that
maps shared files, any process mapping a world-writable temp file.

**Defense (works without changing the kernel):**
- Never mmap files you do not exclusively own/control; open the backing file
  `O_RDONLY` won't help if the file itself is writable by others — the fix is
  **file mode / directory ownership** (protected_regular, private dirs).
- Install a SIGBUS handler that treats the fault as fatal-but-graceful.
- Avoid mapping shared/world-writable files at all (the real fix).

**"If we change X, it works anywhere":** a redesign gives file mappings a
*revocable read* contract — on truncation, the kernel wakes mapped readers with
an explicit error (or zero-fills and reports via a per-mapping flag) instead of
a lethal unhandled signal. Inside Linux today: locked in — SIGBUS-on-truncate
is the memory model and cannot change without breaking every mmap user.

**One-liner:** *mmap is a lease the kernel can't keep; when the file shrinks,
the kernel answers the fault with an unrecoverable kill — turning "you can
write this shared file" into "you can kill every process that mapped it," with
no capability check at the moment of death.*