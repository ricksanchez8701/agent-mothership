#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

/* Victim: event loop that multiplexes a socketpair fd with select().
   Attacker (child) closes the far end. Victim's loop does NOT remove
   the fd on EOF/error, so select() instantly reports READY forever.
   Loop: select -> ready -> read(0/EOF) -> select -> ... 100% CPU.   */

int main(void) {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv)) { perror("socketpair"); return 1; }

    pid_t victim = fork();
    if (victim == 0) {
        close(sv[0]);
        int fd = sv[1];
        struct timespec start, now;
        clock_gettime(CLOCK_MONOTONIC, &start);
        unsigned long long spins = 0;
        char buf[256];
        fd_set rfds;
        /* busy loop for exactly 1.5 seconds */
        for (;;) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            if ((now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec)/1000000 > 1500) break;
            FD_ZERO(&rfds); FD_SET(fd, &rfds);
            int r = select(fd + 1, &rfds, NULL, NULL, NULL);
            if (r > 0 && FD_ISSET(fd, &rfds)) {
                ssize_t n = read(fd, buf, sizeof(buf));
                spins++;
                if (n == 0) {
                    /* EOF: peer closed. select() STILL reports ready. */
                    spins += 1; /* count the spin */
                } else if (n < 0 && errno == EBADF) {
                    /* fd closed mid-wait by another party */
                    spins += 1;
                }
            }
        }
        printf("[victim] select() reported fd READY and loop spun %llu times in 1.5s "
               "(%.0f iterations/sec => busy-loop CPU burn)\n",
               spins, spins / 1.5);
        fflush(NULL);
        _exit(0);
    }

    /* attacker: send one byte then close far end */
    close(sv[1]);
    sleep(1);
    close(sv[0]);
    sleep(2);

    int st;
    waitpid(victim, &st, 0);
    printf("[parent] victim exited %s\n", WIFEXITED(st) ? "normally" : "abnormally");
    return 0;
}