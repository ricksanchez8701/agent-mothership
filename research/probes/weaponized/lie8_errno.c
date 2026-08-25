#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef SYS_memfd_create
#define SYS_memfd_create 319
#endif

/* LIE-8: errno lies + seccomp fingerprint.
   (a) mmap(pipe) -> ENODEV, pread(pipe) -> ESPIPE
   (b) logic-confusion: a naive caller treats ENODEV/ESPIPE as feature indicators
   (c) battery of blocked syscalls as nobody -> record exact errno per call    */

static void show(const char *name, long r, int e) {
    printf("%-24s -> %s%s\n", name,
           r == -1 ? "errno" : "OK",
           r == -1 ? " errno=?" : "");
    if (r == -1) printf("%-24s -> errno=%3d [%s]\n", "", e, strerror(e));
}

int main(void) {
    /* (a) mmap on a pipe -> ENODEV */
    int pfd[2]; pipe(pfd);
    errno = 0;
    void *m = mmap(NULL, 4096, PROT_READ, MAP_SHARED, pfd[0], 0);
    printf("(a) mmap(pipe_fd)  -> %s errno=%d [%s]\n",
           m==MAP_FAILED?"ENODEV":"OK", errno, strerror(errno));
    errno = 0;
    char b[16];
    ssize_t n = pread(pfd[0], b, 16, 0);
    printf("    pread(pipe_fd)  -> %zd errno=%d [%s]\n", n, errno, strerror(errno));
    errno = 0;
    n = pwrite(pfd[1], "x", 1, 0);
    printf("    pwrite(pipe_fd) -> %zd errno=%d [%s]\n", n, errno, strerror(errno));

    /* (b) logic confusion: code that decides "is this seekable?" by pread errno */
    errno = 0;
    n = pread(pfd[0], b, 16, 0);
    int espipe = (errno == ESPIPE);
    printf("(b) naive seek-support probe: pread(pipe) errno=ESPIPE -> logic path decides "
           "device is a NON-SEEKABLE STREAM (fallback path enabled) -> %s\n",
           espipe ? "enabled" : "unused");
    errno = 0;
    void *m2 = mmap(NULL, 4096, PROT_READ, MAP_SHARED, pfd[0], 0);
    if (m2 == MAP_FAILED && errno == ENODEV)
        printf("    naive mmap-probe: errno=ENODEV -> code disables 'device present' feature\n");

    /* (c) seccomp/blocked-syscall fingerprint as nobody */
    printf("\n(c) syscall fingerprint (nobody):\n");
    if (getuid() != 0) {
        long r; int e;
        errno=0; r=syscall(SYS_memfd_create, "x", 0); show("memfd_create", r, errno);
        errno=0; r=syscall(425); show("io_uring_setup", r, errno);
        errno=0; r=syscall(321); show("bpf", r, errno);
        errno=0; r=syscall(298); show("perf_event_open", r, errno);
        errno=0; r=syscall(323); show("userfaultfd", r, errno);
        errno=0; r=syscall(248); show("add_key", r, errno);
        errno=0; r=syscall(435); show("clone3", r, errno);
        errno=0; r=syscall(272); show("unshare", r, errno);
        errno=0; r=syscall(165); show("mount", r, errno);
        errno=0; r=syscall(101); show("ptrace", r, errno);
        errno=0; r=syscall(313); show("finit_module", r, errno);
        errno=0; r=syscall(246); show("kexec_load", r, errno);
        errno=0; r=syscall(308); show("setns", r, errno);
        errno=0; r=syscall(304); show("open_by_handle_at", r, errno);
        errno=0; r=syscall(317); show("getrandom", r, errno);
        errno=0; r=syscall(434); show("pidfd_open", r, errno);
        errno=0; r=syscall(273); show("set_mempolicy", r, errno);
        errno=0; r=syscall(283); show("membarrier", r, errno);
        errno=0; r=syscall(300); show("pidfd_send_signal", r, errno);
    } else {
        execl("/usr/bin/setpriv","setpriv","--reuid=65534","--regid=65534","--clear-groups",
              "/tmp/weap/lie8_errno", NULL);
    }
    return 0;
}