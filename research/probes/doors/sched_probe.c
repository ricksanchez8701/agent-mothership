#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sched.h>
#include <linux/sched.h>
#include <sys/mman.h>

int main(void) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    errno = 0;
    long r = syscall(__NR_sched_setaffinity, 0, sizeof(set), &set);
    printf("sched_setaffinity(self, cpu0) ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"OK");
    errno = 0;
    r = syscall(__NR_sched_setaffinity, 1, sizeof(set), &set); /* root pid 1 */
    printf("sched_setaffinity(root pid1)  ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"");
    errno = 0;
    r = syscall(__NR_sched_setscheduler, 0, SCHED_BATCH, NULL);
    printf("sched_setscheduler(BATCH)     ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"");
    errno = 0;
    r = syscall(__NR_sched_setscheduler, 0, SCHED_IDLE, NULL);
    printf("sched_setscheduler(IDLE)      ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"");

    /* mlock with RLIMIT check */
    errno = 0;
    void *p = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    r = syscall(__NR_mlock, p, 4096);
    printf("mlock(4k)                     ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"OK");
    return 0;
}