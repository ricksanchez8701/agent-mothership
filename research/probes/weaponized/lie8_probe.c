#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <linux/io_uring.h>
#include <sched.h>

#ifndef SYS_memfd_create
#define SYS_memfd_create 319
#endif

static void probe(const char *name, long r, const char *tag) {
    if (r == -1)
        printf("%-20s errno=%3d %-22s | %s\n", name, errno, strerror(errno), tag);
    else
        printf("%-20s OK (fd=%ld) %s\n", name, r, tag);
    if (r >= 0) close(r);
}

static const char *clas(int e) {
    return (e == EPERM)  ? "hard-block (seccomp or kernel EPERM)"
         : (e == ENOSYS) ? "ENOSYS-emulation (seccomp)"
         : (e == EFAULT || e == EINVAL || e == ESRCH) ? "LIVE syscall (real kernel errno)" : "other";
}

int main(void) {
    if (getuid() != 0) {
        struct io_uring_params p; memset(&p, 0, sizeof p);
        char buf[8];
        struct probe_s { const char *n; long r; } r;
        int e;
        long rr;
#define P(n, expr) do { errno=0; rr=(expr); probe(n, rr, clas(errno)); } while(0)
        P("memfd_create",      syscall(SYS_memfd_create, "x", MFD_CLOEXEC));
        P("io_uring_setup",    syscall(425, 1, &p));
        P("bpf",               syscall(321, 0, NULL, 0));
        P("perf_event_open",   syscall(298, NULL, 0, -1, -1, 0));
        P("userfaultfd",       syscall(323, 0));
        P("add_key",           syscall(248, 0, NULL, NULL, 0, 0));
        P("clone3",            syscall(435, NULL, 0));
        P("unshare",           syscall(272, CLONE_NEWUSER));
        P("mount",             syscall(165, NULL, NULL, NULL, 0, NULL));
        P("ptrace",            syscall(101, 0, 0, 0, 0));
        P("finit_module",      syscall(313, -1, NULL, 0));
        P("kexec_load",        syscall(246, 0, NULL, 0, 0));
        P("setns",             syscall(308, -1, 0));
        P("open_by_handle_at", syscall(304, -1, NULL, 0));
        P("personality",       syscall(135, 0xffffffffUL));
        P("pivot_root",        syscall(155, NULL, NULL));
        P("swapon",            syscall(87, NULL, 0));
        P("quotactl",          syscall(179, 0, NULL, 0, NULL));
        P("acct",              syscall(51, NULL));
        P("sethostname",       syscall(170, "x", 1));
        P("getrandom",         syscall(317, buf, 8, 0));
        P("membarrier",        syscall(283, 0, 0));
        P("pidfd_open",        syscall(434, getpid(), 0));
        P("openat2",           syscall(437, AT_FDCWD, "/", NULL, 0));
        return 0;
    }
    execl("/usr/bin/setpriv","setpriv","--reuid=65534","--regid=65534","--clear-groups",
          "/tmp/weap/lie8_probe", NULL);
    return 1;
}