#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(void){
    char *e[] = {NULL};
    char *a_sh[]  = {"/tmp/door/s.sh", NULL};
    char *a_emp[] = {"/tmp/door/empty", NULL};
    char *a_dir[] = {"/tmp/door", NULL};
    char *a_no[]  = {"/tmp/door/nope", NULL};
    char *a_te[]  = {"/bin/echo", "ONE", NULL};
    long r;
    errno=0; r = syscall(__NR_execve, "/tmp/door/s.sh", a_sh, e);
    printf("A shebang       ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    errno=0; r = syscall(__NR_execve, "/tmp/door/empty", a_emp, e);
    printf("B empty file    ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    errno=0; r = syscall(__NR_execve, "/tmp/door", a_dir, e);
    printf("C directory     ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    errno=0; r = syscall(__NR_execve, "/tmp/door/nope", a_no, e);
    printf("D nonexistent   ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    errno=0; r = syscall(__NR_execveat, AT_FDCWD, "/tmp/door/s.sh", a_sh, e, AT_SYMLINK_NOFOLLOW);
    printf("E execveat nf   ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    errno=0; r = syscall(__NR_execve, "/bin/echo", a_te, e);
    printf("F echo retry    ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    printf("done (should not print if F succeeded)\n");
    return 0;
}
