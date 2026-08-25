# Unprivileged Attack Surface — live assessment (kernel 6.8, Debian 12 container)

*All findings verified empirically as uid 65534 (`nobody`) unless marked.
PoCs: research/probes/weaponized/ (the 8 lies) and research/probes/doors/ (door battery).
Deep entries: 101-select-lie, 102-sigbus-kill, 107-chmod-symlink.*

## Bottom line

Two of the eight discovered "lies" are **privilege-boundary-crossing primitives**
(the only two that are): a nobody process can **kill a root process** (SIGBUS
via shared-file truncate) and **transfer ownership of a root file to itself**
(chmod/chown symlink transparency). Six others are real but stay inside the
user boundary (DoS / integrity / reliability / fingerprinting). The sandbox's
seccomp profile is itself fingerprintable from errno alone.

## The severity matrix (verified)

| # | Lie | Works as nobody | Crosses boundary | Severity | Exploit shape |
|---|-----|----------------|------------------|----------|---------------|
| 2 | mmap + truncate → SIGBUS | YES | **YES — nobody kills root victim** | **HIGH** | Any process mapping a file an attacker can write dies unrecoverably (SIGBUS storm; handler can't resume) |
| 7 | chmod/chown symlink transparency | YES | **YES — root file mode rewritten + ownership transferred to nobody** | **HIGH** | Privileged program chmod/chowns an attacker-influenced path; classic /tmp door closed by protected_symlinks, open in non-sticky world-writable dirs |
| 8 | errno lies (mmap→ENODEV, pread→ESPIPE) + seccomp fingerprint | YES | No (info) | Med | Unprivileged process maps the exact seccomp allow/deny list from errno alone; errno-checking software misbehaves |
| 4 | pwrite + O_APPEND ignores offset; O_APPEND not a boundary | YES | No | Med | "Append-only" audit log rewritten/truncated via a second open without O_APPEND (same inode, any path/hardlink) |
| 5 | MADV_FREE lazy-free (RSS unchanged) | YES | No | Med | Secret-retention: "released" crypto/key buffers stay resident & pagemap-present until memory pressure |
| 1 | select() reports closed fd as READY | YES | No | Low | Busy-loop CPU exhaustion (~224k/s, full core) in any event loop that doesn't unregister fds on EBADF |
| 3 | setpriority silent clamp (nice 21→19, returns 0) | YES | No | Low-Med | Error-checking code sees "success" and starves itself; same-uid starvation works, root boundary holds (EPERM) |
| 6 | copy_file_range EXDEV | YES | No | Low | Naive copy tools silently produce empty/partial files (verified 0-byte output from 18-byte source) |

## The two cross-boundary primitives (detail)

**Kill primitive (LIE-2)** — `research/probes/weaponized/lie2_sigbus.c`:
nobody `ftruncate`s a file a root victim has mmap'd → kernel SIGBUS-kills the
root process. Verified: `victim died via signal=7 (SIGBUS)`. A SIGBUS handler
cannot resume (faulting instruction re-executes → infinite storm). The kill is
executed by the kernel; nobody never touches the victim's credentials.

**Ownership-transfer primitive (LIE-7)** — `research/probes/weaponized/lie7_chmod.c`:
root chmod/chown through a symlink in a non-sticky attacker dir. Verified:
`rootfile mode 644->777` and `rootfile owner now uid=65534`. protected_symlinks
only blocks sticky dirs; every non-sticky world-writable dir and every
intermediate-component symlink is an open door.

## Doors battery (unprivileged, verified)

| Door | Status | What it gives |
|------|--------|---------------|
| `/proc/<pid>/status` cross-uid (incl root) | **OPEN** | CapEff, Seccomp+filter count, NoNewPrivs, NSpid of every process — runtime fingerprint |
| `/proc/<pid>/environ` + `/proc/<pid>/maps` same-uid | **OPEN** | Live secrets from other same-uid processes; ASLR layout (cross-uid is **gated here**, see correction below) |
| `memfd_create` + `execveat` fileless exec | **OPEN** | Arbitrary fileless code as nobody; bypasses AppArmor path-based exec denial (self-owned anon memfd sails through) |
| `io_uring_setup` | **OPEN** | Live async-IO surface (io_uring_disabled=0, not in seccomp deny-list) |
| `ptrace`/`process_vm_readv` (parent→child, same-uid) | OPEN (scoped) | Same-uid memory R/W for descendants (Yama scope=1); cross-uid & child→parent EPERM |
| `/proc/self/mem` rw | OPEN | Self-injection only |
| `O_TMPFILE` + linkat | OPEN | Materialize invisible files |
| NETLINK_ROUTE dump | **OPEN** | Full network topology (lo, eth0, docker0) |
| dgram ICMP ping socket | OPEN | Unprivileged ICMP (raw → EPERM) |
| SCM_RIGHTS fd passing | OPEN | If any root proc forwards an fd, nobody inherits its access (verified: /etc/shadow read via passed fd) |
| `F_SETOWN_EX`/`F_SETSIG` | PARTIAL | Signal-injection primitive (delivery to root unproven) |
| /proc/modules, zoneinfo, buddyinfo, ioports | PARTIAL | Low-value leaks (addrs masked; slabinfo/timer_list EPERM/tmpfs) |
| userns, unshare, setns, mount, chroot, iopl, TIOCSTI, raw sockets, perf, eBPF, userfaultfd, keyctl, kcore, /dev/mem | **CLOSED** | Hard walls (seccomp EPERM / sysctls / missing caps) |

## Seccomp fingerprint (LIE-8, verified — `lie8_probe.c`)

The filter is a uniform **EPERM default-deny** with **exactly one errno
exception: `clone3 → ENOSYS`** (the "force glibc clone fallback" rule) and a
specific allow-list:

| Result | Syscalls |
|--------|----------|
| **live** | memfd_create, io_uring_setup, ptrace, pidfd_open, personality, membarrier, process_vm_readv, exec* |
| **EPERM (hard-blocked)** | bpf, perf_event_open, userfaultfd, add_key, unshare, mount, finit_module, kexec_load, setns, open_by_handle_at, pivot_root, quotactl, sethostname, ... |
| **ENOSYS (emulated)** | clone3 only |
| **live, real kernel errno** | getrandom/openat2/swapon (EINVAL/EFAULT on bad args = live) |

An unprivileged process learns the entire profile with zero privileges, zero
traces. This uniquely identifies a RuntimeDefault-style container profile and
separates filter semantics from kernel semantics per syscall.

## Correction to recon claims (honest note)

Agent A initially claimed nobody could read a *root* process's `environ` and
pull GITHUB_TOKENs. **Re-verified: cross-uid environ/maps are DENIED in this
container** (nobody→root: Permission denied on /proc/1/environ and /proc/1/maps).
The truthful finding is:
- **same-uid** environ/maps **are** readable (verified: `SECRETVAR=HELLO`
  leaked between two nobody processes; ASLR maps readable) — real leak in
  multi-tenant same-uid setups;
- **cross-uid** `/proc/<pid>/status` **is** world-readable (CapEff/Seccomp
  fingerprint) even though environ/maps are gated.
On a stock desktop Linux (no such gating, hidepid off) the cross-uid environ/
maps leak is typically real; here the boundary holds.

## Operational notes for this environment

1. Any process running in this container shares root's uid (everything runs as
   root) — but cross-uid /proc memory is gated, so the practical leaks are
   same-uid /proc and world-readable status.
2. The mothership's `.env.memory` / GitHub tokens: if any process forks with
   those in its environment, a **same-uid** sibling can read them via
   `/proc/<pid>/environ`. Prefer not to export secrets into long-lived
   process environments.
3. Do not mmap shared/world-writable files in services, and never let
   privileged code chmod/chown attacker-influenced paths (use fd-based
   operations).
4. The seccomp fingerprint means the container's security posture is visible
   to any unprivileged code it runs.

## What is CLOSED here but usually OPEN on a stock desktop

userns (`unshare(CLONE_NEWUSER)`), userfaultfd (desktop sysctl often allows),
perf_event_open (equal — paranoid=4 both), kcore/slabinfo (gated here),
TIOCSTI (dead since 6.2 everywhere).

## What is OPEN here and matters on a desktop too

memfd+execveat fileless exec, io_uring, dgram ICMP, NETLINK_ROUTE dump, SCM_RIGHTS
fd-passing, same-uid /proc environ/maps, the two cross-boundary file primitives
(SIGBUS kill, symlink chmod/chown), and the seccomp errno fingerprint.