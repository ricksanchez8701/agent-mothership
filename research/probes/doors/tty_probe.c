#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/stat.h>

volatile sig_atomic_t caught = 0;
void on_sig(int s) { caught = 1; fprintf(stderr, "  !! caught signal %d\n", s); }

int main(void) {
    printf("[10] TIOCSTI on stdin:\n");
    errno = 0;
    char c = '\n';
    int r = ioctl(STDIN_FILENO, TIOCSTI, &c);
    printf("   ioctl(stdin,TIOCSTI)  ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"ok");

    printf("[11] F_SETOWN / F_SETOWN_EX / F_SETSIG on pty:\n");
    /* open a pty master */
    int m = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    if (m < 0) { printf("   open /dev/ptmx failed: %s\n", strerror(errno)); }
    else {
        int r2;
        errno = 0; r2 = fcntl(m, F_SETOWN, getppid());           /* arbitrary other process */
        printf("   F_SETOWN(other pid %d) ret=%d errno=%d %s\n", getppid(), r2, errno, errno?strerror(errno):"");
        struct f_owner_ex oe = { .type = F_OWNER_PID, .pid = 1 };
        errno = 0; r2 = fcntl(m, F_SETOWN_EX, &oe);             /* root pid 1 */
        printf("   F_SETOWN_EX(pid 1)     ret=%d errno=%d %s\n", r2, errno, errno?strerror(errno):"");
        errno = 0; r2 = fcntl(m, F_SETSIG, SIGURG);
        printf("   F_SETSIG(SIGURG)       ret=%d errno=%d %s\n", r2, errno, errno?strerror(errno):"");
        struct f_owner_ex got;
        errno = 0; r2 = fcntl(m, F_GETOWN_EX, &got);
        printf("   F_GETOWN_EX            ret=%d errno=%d type=%d pid=%d\n", r2, errno, r2==0?got.type:-1, r2==0?got.pid:-1);
        close(m);
    }

    printf("[12] kill broadcast semantics as nobody:\n");
    errno = 0; int r3 = kill(0, 0);
    printf("   kill(0,0)      ret=%d errno=%d %s\n", r3, errno, errno?strerror(errno):"");
    errno = 0; r3 = kill(-1, 0);
    printf("   kill(-1,0)     ret=%d errno=%d %s\n", r3, errno, errno?strerror(errno):"");
    errno = 0; r3 = kill(1, 0);
    printf("   kill(1,0)      ret=%d errno=%d %s\n", r3, errno, errno?strerror(errno):"");

    /* Check permission to signal a root process without sending */
    pid_t rpid = (pid_t)strtol(getenv("ROOTPID")?getenv("ROOTPID"):"1", NULL, 10);
    errno = 0; r3 = kill(rpid, 0);
    printf("   kill(rootpid %d,0) ret=%d errno=%d %s\n", rpid, r3, errno, errno?strerror(errno):"");

    printf("[D-extra] signal delivery test (SIGURG to own group):\n");
    signal(SIGURG, on_sig);
    errno = 0; r3 = kill(0, SIGURG);
    printf("   kill(0,SIGURG) ret=%d errno=%d caught=%d\n", r3, errno, caught);
    return 0;
}