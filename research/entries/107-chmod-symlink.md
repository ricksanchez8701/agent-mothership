# Entry 107 — chmod/chown symlink transparency: the ownership-transfer door

- **Language:** Linux (kernel 6.8, verified live)
- **Topic:** chmod()/chown() follow symlinks and act on the TARGET
- **Status:** complete
- **Severity:** HIGH — crosses the privilege boundary (ownership transfer)
- **Repro:** research/probes/weaponized/lie7_chmod.c

---

## L1 — Surface (the surprising observable, verified)

`chmod()` and `chown()` on a **symlink** do not touch the symlink — the kernel
*transparently follows* the link and changes the **target's** mode/owner.
Verified on kernel 6.8 with `fs.protected_symlinks=1`:

```
fs.protected_symlinks=1
A) nobody created /tmp/rootlinkA -> root chmod 0777
   BLOCKED  (protected_symlinks held in the STICKY /tmp dir)
B) root chmod(nonsticky-dir symlink, 0777) rc=0 ; rootfile mode 644->777
   => FOLLOWED — root file mode changed via symlink!
   root chown(link, nobody) rc=0 ; rootfile owner now uid=65534
   => OWNERSHIP TRANSFERRED via symlink!
```

So: put a symlink in **any world-writable directory that is not sticky** (or
get the target program to `chmod`/`chown` a path whose final component you
control), and a privileged program operating on that path will:
- change the mode of **your chosen target** (setuid-capable mode), or
- **transfer ownership of a root file to you** (uid 65534), giving you full
  control of a root-owned file.

## Naive expectation

`chmod(symlink)` should fail or act on the link itself. `chown` transferring a
root file's ownership to an unprivileged user through a symlink is a
privacy/privilege catastrophe that "should" require a capability check on the
*target* as seen by the *caller* — not the follower's privileges.

## What actually happened

The kernel applies the caller's (root's) privileges to the target discovered by
following the link. The permission model checks **who the caller is** against
the **target inode** — it never asks "did the caller intend this file?"
followed a symlink.

---

## L2 — Mechanism

- `chmod(path, mode)` → `do_fchmodat` with `AT_SYMLINK_NOFOLLOW` **not set** →
  the VFS resolves the final component **following symlinks** (`lookup_flags`
  with `LOOKUP_FOLLOW`), landing on the target inode, then calls `notify_change`
  on it. Permissions are checked against the **target** using the **caller's**
  creds. Since the caller (a privileged program / root) has CAP_FOWNER or owns
  the target, the operation succeeds.
- `chown(path, uid)` behaves identically: follow, then `setattr` on the target.
- `fs.protected_symlinks=1` only closes the **sticky world-writable dir** case
  (the /tmp door): the kernel refuses to follow a symlink owned by someone
  other than the follower when the link sits in a sticky dir whose owner is
  not the follower. It does **not** protect:
  - world-writable **non-sticky** directories,
  - any directory the attacker owns (mode 0777, not sticky),
  - intermediate-component symlinks in normal dirs (e.g. `/var/lib/x -> /etc`).

---

## L3 — History

Symlink transparency is a POSIX decision (1980s): symlinks are "names that
resolve." chmod/chown were specified to follow because that's what the link
*is*. The security retcon came decades later: `open()` got `O_NOFOLLOW` (2.1.126,
1998); `fchmodat`/`fchownat` with `AT_SYMLINK_NOFOLLOW` (2.6.16, 2006);
`protected_symlinks`/`protected_hardlinks` (3.6, 2012) patched only the sticky
dir. Every one of these is an **accreted parallel fix** (PATTERN-1) layered on
top of an API whose default remains "follow silently." The default — the door —
is still open everywhere protected_symlinks doesn't reach.

---

## L4 — Principle

This is a classic **TOCTOU / confused-deputy** failure, but the deeper lesson
is about **delegation through path resolution**. When a privileged program
accepts a path from the environment and applies a *privileged operation* to it,
the kernel is being asked to act as the program's deputy — but the deputy
resolves the name in a world the caller does not control. The "name" is not
owned by the caller; the link owner rewrites what the name means *between
check and use*.

The principle: **a privileged operation on a path should be an operation on an
inode the caller already vetted, not on whatever the path currently resolves
to.** That is why the durable fixes are all fd-based (`fchmod`, `fchown`):
an fd is a handle to one specific inode; a path is a promise that anyone with
write access to a parent directory can break.

---

## L5 — Verdict + decisive answer

**Severity: HIGH.** It crosses the privilege boundary: an unprivileged user can
(1) have a root file's mode rewritten to setuid-capable 0777, and (2) **take
ownership of a root-owned file**, given a privileged program that chmod/chowns
an attacker-influenced path. The classic /tmp attack is closed by
`protected_symlinks=1`, but the door is open in every non-sticky
attacker-writable directory and for every intermediate-component symlink.

**Defense (works without changing the kernel):**
- Privileged programs: operate on **fds** (`open` + `fchmod`/`fchown`), never
  paths; or use `fchmodat`/`fchownat` with `AT_SYMLINK_NOFOLLOW` and *reject*
  the link (`EOPNOTSUPP`) instead of following.
- Sysadmins: never leave world-writable non-sticky dirs; `t` bit on shared dirs;
  don't let unprivileged users create directories where root later chmod/chowns.

**"If we change X, it works anywhere":** the fix that works in any kernel is to
**default to NOFOLLOW for privileged setters and force an explicit flag to
follow** (which is precisely the `AT_SYMLINK_NOFOLLOW` design — the bug is that
the *default* is follow). Inside Linux today: the default cannot change without
breaking POSIX-name semantics — so the door is permanent for any code that
passes attacker-influenced paths to chmod/chown.

**One-liner:** *symlink transparency means chmod/chown apply the caller's
privilege to whatever the name currently resolves to — and since paths are a
promise anyone can break, "chmod this path" as root is "let anyone redirect
root's mode rewrite to a file of their choice." The kernel's permission check
never asks whether the caller meant the target.*