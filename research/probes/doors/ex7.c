#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
static void t(const char*label, const char *path, char **argv){ errno=0; long r=syscall(__NR_execve,path,argv,(char*[]){NULL}); printf("%-28s ret=%ld errno=%d %s\n", label, r, errno, errno?strerror(errno):"EXEC'd"); fflush(stdout); }
int main(void){
    t("1 /bin/true", "/bin/true", (char*[]){"/bin/true",NULL});
    t("2 /bin/sh -c", "/bin/sh", (char*[]){"/bin/sh","-c","true",NULL});
    t("3 /tmp/door/empty", "/tmp/door/empty", (char*[]){"/tmp/door/empty",NULL});
    t("4 /tmp/door/nope", "/tmp/door/nope", (char*[]){"/tmp/door/nope",NULL});
    t("5 /tmp/door(symlink)", "/tmp/door/sym", (char*[]){"/tmp/door/sym",NULL});
    t("6 /bin/echo via s.sh", "/tmp/door/s.sh", (char*[]){"/tmp/door/s.sh",NULL});
    t("7 /proc/self/exe", "/proc/self/exe", (char*[]){"/proc/self/exe","x",NULL});
    t("8 /usr/bin/printf", "/usr/bin/printf", (char*[]){"/usr/bin/printf","x",NULL});
    return 0;
}
