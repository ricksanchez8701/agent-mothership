#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

int main(void) {
    int m = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    if (m < 0) { printf("open /dev/ptmx: %s\n", strerror(errno)); return 1; }
    unlockpt(m);
    grantpt(m);
    char *sname = ptsname(m);
    printf("pty master=%d slave=%s\n", m, sname);
    int s = open(sname, O_RDWR | O_NOCTTY);
    printf("open slave ret=%d errno=%d %s\n", s, errno, errno?strerror(errno):"");

    char c = 'x';
    errno = 0;
    int r = ioctl(s, TIOCSTI, &c);
    printf("TIOCSTI on pty slave   ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"INJECTED");

    errno = 0;
    r = ioctl(m, TIOCSTI, &c);
    printf("TIOCSTI on pty master  ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"INJECTED");
    return 0;
}