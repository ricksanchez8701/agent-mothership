#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
int main(void){
    struct sock_filter insns[] = { BPF_STMT(BPF_RET|BPF_K, SECCOMP_RET_ALLOW) };
    struct sock_fprog prog = { .len = 1, .filter = insns };
    errno=0; long r = syscall(__NR_seccomp, SECCOMP_SET_MODE_FILTER, 0, &prog);
    printf("seccomp(SET_MODE_FILTER, allow-all) ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"OK-installed");
    errno=0; r = syscall(__NR_seccomp, SECCOMP_GET_ACTION_AVAIL, 0, &(int){SECCOMP_RET_KILL_PROCESS});
    printf("seccomp(GET_ACTION_AVAIL)            ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"");
    return 0;
}
