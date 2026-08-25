#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(void){
    char *argv[] = {"s.sh", NULL};
    char *envp[] = {NULL};
    long r;
    errno=0; r = syscall(__NR_execve, "/tmp/door/s.sh", argv, envp);
    printf("execve shebang script ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    errno=0; r = syscall(__NR_execve, "/tmp/door/empty", argv, envp);
    printf("execve empty file     ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    errno=0; r = syscall(__NR_execve, "/tmp/door", argv, envp);
    printf("execve directory      ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    errno=0; r = syscall(__NR_execve, "/tmp/door/nope", argv, envp);
    printf("execve nonexistent    ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    errno=0; r = syscall(__NR_execveat, AT_FDCWD, "/tmp/door/s.sh", argv, envp, AT_SYMLINK_NOFOLLOW);
    printf("execveat nofollow     ret=%ld errno=%d %s\n", r, errno, errno?strerror(errno):"EXEC'd");
    printf("if you see SHEBANG-EXEC-OK above, shebang worked\n");
    return 0;
}
