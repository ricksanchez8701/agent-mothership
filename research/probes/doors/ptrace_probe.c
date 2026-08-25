#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <fcntl.h>

static long do_readv(pid_t pid, unsigned long addr, void *buf, size_t len) {
    struct iovec local = { .iov_base = buf, .iov_len = len };
    struct iovec remote = { .iov_base = (void*)addr, .iov_len = len };
    return syscall(__NR_process_vm_readv, pid, &local, 1, &remote, 1, 0);
}

volatile int gv = 0x11223344;

int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "child")) {
        volatile char cbuf[64];
        memset((void*)cbuf, 'A', sizeof(cbuf));
        fprintf(stderr, "CHILD pid=%d sleeping, addr of cbuf=%p\n", getpid(), cbuf);
        sleep(60);
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "attachroot")) {
        pid_t rp = atoi(argv[2]);
        errno = 0;
        long r = ptrace(PTRACE_ATTACH, rp, 0, 0);
        printf("ptrace ATTACH root pid %d -> ret=%ld errno=%d %s\n", rp, r, errno, errno?strerror(errno):"");
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "readroot")) {
        pid_t rp = atoi(argv[2]);
        unsigned long addr = strtoul(argv[3], NULL, 16);
        char buf[64] = {0};
        errno = 0;
        long r = do_readv(rp, addr, buf, 32);
        printf("process_vm_readv root pid %d @0x%lx -> ret=%ld errno=%d %s\n", rp, addr, r, errno, errno?strerror(errno):"");
        if (r > 0) { printf("  data: "); for(int i=0;i<r;i++) printf("%02x ", (unsigned char)buf[i]); printf("\n"); }
        return 0;
    }

    pid_t child = fork();
    if (child == 0) { execl("/proc/self/exe", "probe", "child", NULL); perror("exec child"); exit(1); }
    sleep(1);

    printf("child pid=%d\n", child);
    errno = 0;
    long r1 = ptrace(PTRACE_ATTACH, child, 0, 0);
    printf("A) parent ptrace(ATTACH) child        -> ret=%ld errno=%d %s\n", r1, errno, errno?strerror(errno):"");
    if (r1 == 0) { ptrace(PTRACE_DETACH, child, 0, 0); }

    errno = 0;
    char buf[32];
    long r2 = do_readv(child, (unsigned long)buf, buf, sizeof(buf)); /* address won't be valid, but we care about perm check */
    printf("B) process_vm_readv(child) (bad addr) -> ret=%ld errno=%d %s\n", r2, errno, errno?strerror(errno):"");

    errno = 0;
    int fd = open("/proc/self/mem", O_RDWR);
    printf("C) open /proc/self/mem O_RDWR         -> fd=%d errno=%d %s\n", fd, errno, errno?strerror(errno):"");
    if (fd >= 0) {
        off_t off = (off_t)(unsigned long)&gv;
        errno = 0;
        ssize_t nr = pread(fd, buf, 4, off);
        printf("   pread(0x%lx) -> %zd errno=%d val_before=%x\n", (unsigned long)off, nr, errno, *(unsigned int*)buf);
        errno = 0;
        unsigned int nv = 0xdeadbeef;
        ssize_t nw = pwrite(fd, &nv, 4, off);
        printf("   pwrite -> %zd errno=%d\n", nw, errno);
        printf("   gv now = 0x%x (expected 0xdeadbeef)\n", (unsigned int)gv);
        close(fd);
    }

    errno = 0;
    long r3 = ptrace(PTRACE_ATTACH, getppid(), 0, 0);
    printf("D) ptrace(ATTACH) parent              -> ret=%ld errno=%d %s\n", r3, errno, errno?strerror(errno):"");

    kill(child, SIGKILL);
    waitpid(child, NULL, 0);
    return 0;
}
