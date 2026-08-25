#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <sched.h>
#include <sys/wait.h>

int main(void) {
    /* clone3 default */
    struct clone_args ca;
    memset(&ca, 0, sizeof(ca));
    ca.exit_signal = SIGCHLD;
    errno = 0;
    long r = syscall(__NR_clone3, &ca, sizeof(ca));
    printf("[19] clone3(default)          ret=%ld errno=%d %s\n", r, errno, r>0?("child made"):(errno?strerror(errno):""));
    if (r > 0) { waitpid((pid_t)r, NULL, 0); }

    memset(&ca, 0, sizeof(ca));
    ca.flags = CLONE_NEWUSER;
    ca.exit_signal = SIGCHLD;
    errno = 0;
    r = syscall(__NR_clone3, &ca, sizeof(ca));
    printf("    clone3(CLONE_NEWUSER)    ret=%ld errno=%d %s\n", r, errno, r>0?("child made"):(errno?strerror(errno):""));
    if (r > 0) { waitpid((pid_t)r, NULL, 0); }

    memset(&ca, 0, sizeof(ca));
    ca.flags = CLONE_NEWNS;
    ca.exit_signal = SIGCHLD;
    errno = 0;
    r = syscall(__NR_clone3, &ca, sizeof(ca));
    printf("    clone3(CLONE_NEWNS)      ret=%ld errno=%d %s\n", r, errno, r>0?("child made"):(errno?strerror(errno):""));
    if (r > 0) { waitpid((pid_t)r, NULL, 0); }
    return 0;
}