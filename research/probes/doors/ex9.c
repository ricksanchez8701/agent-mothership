#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
static void t(const char*label, const char *path){
    fflush(stdout);
    pid_t p = fork();
    if (p==0){ char *e[]={NULL}; char *a[]={path,NULL}; errno=0; long r=syscall(__NR_execve,path,a,e); dprintf(2,"%-34s ret=%ld errno=%d %s\n",label,r,errno,errno?strerror(errno):"EXEC'd"); _exit(0); }
    int st; waitpid(p,&st,0);
    if (WIFEXITED(st)&&WEXITSTATUS(st)==0) printf("%-34s -> EXEC'd OK\n",label);
    else printf("%-34s -> exit %d\n",label,WEXITSTATUS(st));
}
int main(void){
    t("A /tmp/door/ex8 (ELF)", "/tmp/door/ex8");
    t("B /bin/nope (nonexist)", "/bin/nope");
    t("C /usr/bin/nope", "/usr/bin/nope");
    t("D /root/nope", "/root/nope");
    t("E /var/tmp/nope", "/var/tmp/nope");
    t("F /tmp/nope", "/tmp/nope");
    t("G /workspaces/agent-mothership/README.md", "/workspaces/agent-mothership/README.md");
    t("H /bin/echo via absolute symlink /tmp/door/sym", "/tmp/door/sym");
    t("I /usr/bin/echo", "/usr/bin/echo");
    return 0;
}
