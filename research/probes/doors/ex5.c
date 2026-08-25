#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
int main(void){
    char *a1[] = {"/bin/echo", "HI", NULL};
    char *a2[] = {"sh", "-c", "echo GLIBC-EXEC-OK", NULL};
    char *e[] = {NULL};
    long r;
    errno=0; r = syscall(__NR_execve, "/bin/echo", a1, e);
    printf("execve /bin/echo    ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    errno=0; r = syscall(__NR_execve, "/bin/sh", a2, e);
    printf("execve /bin/sh      ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    printf("--- trying glibc execl ---\n");
    fflush(stdout);
    execl("/bin/echo", "/bin/echo", "GLIBC-EXEC-OK", (char*)NULL);
    printf("glibc execl /bin/echo ret=%d errno=%d %s\n", -1, errno, strerror(errno));
    return 0;
}
