#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
static void t(const char*label, const char *path, char **argv){
    fflush(stdout);
    pid_t p = fork();
    if (p==0){ char *e[]={NULL}; errno=0; long r=syscall(__NR_execve,path,argv,e); dprintf(2,"%-30s ret=%ld errno=%d %s\n",label,r,errno,errno?strerror(errno):"EXEC'd"); _exit(0); }
    int st; waitpid(p,&st,0);
    if (WIFEXITED(st) && WEXITSTATUS(st)==0) printf("%-30s -> EXEC'd OK\n", label);
    else printf("%-30s -> (child exited status %d)\n", label, WEXITSTATUS(st));
}
int main(void){
    t("1 /bin/true", "/bin/true", (char*[]){"/bin/true",NULL});
    t("2 /bin/sh -c true", "/bin/sh", (char*[]){"/bin/sh","-c","true",NULL});
    t("3 /tmp/door/empty", "/tmp/door/empty", (char*[]){"/tmp/door/empty",NULL});
    t("4 /tmp/door/nope", "/tmp/door/nope", (char*[]){"/tmp/door/nope",NULL});
    t("5 /tmp/door/sym->echo", "/tmp/door/sym", (char*[]){"/tmp/door/sym",NULL});
    t("6 /tmp/door/s.sh shebang", "/tmp/door/s.sh", (char*[]){"/tmp/door/s.sh",NULL});
    t("7 /proc/self/exe re-exec", "/proc/self/exe", (char*[]){"/proc/self/exe",NULL});
    t("8 /tmp/copied_bin", "/tmp/door/echo_cp", (char*[]){"/tmp/door/echo_cp",NULL});
    return 0;
}
