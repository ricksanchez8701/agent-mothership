#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(void) {
    int r;
    char *argv[] = {"s.sh", NULL};
    char *envp[] = {NULL};

    printf("[8] O_TMPFILE materialization:\n");
    errno = 0;
    int tf = open("/tmp", O_TMPFILE | O_RDWR, 0600);
    printf("   open O_TMPFILE        ret=%d errno=%d %s\n", tf, errno, tf<0?strerror(errno):"");
    if (tf >= 0) {
        char pbuf[64];
        snprintf(pbuf, sizeof(pbuf), "/proc/self/fd/%d", tf);
        errno = 0;
        r = linkat(AT_FDCWD, pbuf, AT_FDCWD, "/tmp/door/materialized", AT_SYMLINK_FOLLOW);
        printf("   linkat /proc/self/fd  ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"ok");
        if (r == 0) { printf("   file created: %s\n", access("/tmp/door/materialized", F_OK)==0?"YES":"no"); unlink("/tmp/door/materialized"); }
        errno = 0;
        r = linkat(tf, "", AT_FDCWD, "/tmp/door/materialized2", AT_EMPTY_PATH);
        printf("   linkat AT_EMPTY_PATH  ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"ok");
        if (r == 0) { unlink("/tmp/door/materialized2"); }
        close(tf);
    }

    printf("[9] execve edge cases:\n");
    errno = 0;
    system("printf '#!/bin/echo\\nhi\\n' > /tmp/door/s.sh; chmod +x /tmp/door/s.sh");
    r = syscall(__NR_execve, "/tmp/door/s.sh", argv, envp);
    printf("   execve shebang script ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"exec'd");
    int ef = open("/tmp/door/empty", O_CREAT|O_WRONLY, 0644); close(ef);
    errno = 0;
    r = syscall(__NR_execve, "/tmp/door/empty", argv, envp);
    printf("   execve empty file     ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"exec'd");
    errno = 0;
    r = syscall(__NR_execve, "/tmp/door", argv, envp);
    printf("   execve directory      ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"exec'd");
    errno = 0;
    r = syscall(__NR_execve, "/tmp/door/nonexistent", argv, envp);
    printf("   execve nonexistent    ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"exec'd");
    errno = 0;
    r = syscall(__NR_execveat, AT_FDCWD, "/tmp/door/s.sh", argv, envp, AT_SYMLINK_NOFOLLOW);
    printf("   execveat nofollow     ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"exec'd");
    printf("   NOTE: shebang exec should have replaced process; if you see this the exec failed\n");
    return 0;
}