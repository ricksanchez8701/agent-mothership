#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

/* LIE-3: setpriority silent clamp. Runs as ROOT initially so we can spawn a
   genuine root child, THEN drops to nobody for the attack tests.           */

int main(void) {
    pid_t root_child = fork();
    if (root_child == 0) { sleep(30); _exit(0); }   /* real root sleeper */
    usleep(300000);

    /* drop to nobody */
    if (setgid(65534) || setuid(65534)) { perror("setuid"); return 1; }
    printf("now running as uid=%d gid=%d\n", getuid(), getgid());

    /* (a) own process -> silent clamp */
    errno = 0;
    int r = setpriority(PRIO_PROCESS, getpid(), 21);
    int got = getpriority(PRIO_PROCESS, getpid());
    printf("(a) setpriority(self,21) rc=%d errno=%d ; getpriority=%d  => SILENT CLAMP to 19, "
           "caller sees success\n", r, errno, got);

    /* (b) ROOT process -> expect EPERM (boundary should hold) */
    errno = 0;
    r = setpriority(PRIO_PROCESS, root_child, 19);
    printf("(b) setpriority(root_pid=%d,19) rc=%d errno=%d [%s]  => %s\n",
           (int)root_child, r, errno, strerror(errno),
           r == 0 ? "CLAMPED a ROOT process (boundary crossed!)" : "EPERM (boundary holds)");

    /* (c) same-uid nobody daemon -> starvation */
    pid_t np = fork();
    if (np == 0) { sleep(30); _exit(0); }
    usleep(200000);
    errno = 0;
    r = setpriority(PRIO_PROCESS, np, 19);
    int g = getpriority(PRIO_PROCESS, np);
    printf("(c) setpriority(nobody_daemon=%d,19) rc=%d errno=%d ; daemon nice=%d  => %s\n",
           (int)np, r, errno, g,
           (r == 0 && g == 19) ? "SAME-UID starvation vector works" : "failed");

    kill(np, SIGKILL); waitpid(np, NULL, 0);
    errno = 0;
    r = kill(root_child, SIGKILL);
    printf("    kill(root_child) from nobody rc=%d errno=%d [%s] (child will self-exit in 30s)\n",
           r, errno, strerror(errno));
    return 0;
}