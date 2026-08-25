#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>
#include <sys/mman.h>
int main(void){
    struct sched_param p = {0};
    errno=0; long r = syscall(__NR_sched_setscheduler, 0, SCHED_FIFO, &p);
    printf("sched_setscheduler(SCHED_FIFO,valid) ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"");
    errno=0; r = syscall(__NR_sched_setscheduler, 0, SCHED_IDLE, &p);
    printf("sched_setscheduler(SCHED_IDLE,valid) ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"");
    errno=0; r = syscall(__NR_sched_getscheduler, 0);
    printf("sched_getscheduler(self) ret=%ld errno=%d\n", r, errno);
    return 0;
}
