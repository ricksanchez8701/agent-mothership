#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

/* LIE-2: mmap + truncate -> SIGBUS kill.
   Usage: lie2_sigbus <victim-uid-as-str-root-or-nobody> 
   Role split:
   - victim child maps /tmp/victim-map.XXXX (must be writable by attacker=nobody)
   - attacker (nobody) ftruncates it
   - victim touches a page PAST the new EOF -> SIGBUS death

   Two modes for the victim file:
     "rootfile"  : file owned by root, mode 0666 -> nobody attacker truncates root victim
     "nobodyfile": file owned by nobody, mode 0666 -> nobody attacker truncates nobody victim
*/

static volatile sig_atomic_t sigbus_count = 0;
static void onbus(int s) {
    sigbus_count++;
    if (sigbus_count > 3) _exit(42); /* can't skip the faulting access; bail */
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "nobodyfile";
    char path[128];
    int uid = getuid();

    /* Victim maps file with 1 page, faults it in, then idles on a pipe. */
    snprintf(path, sizeof(path), "/tmp/victim-map.%d.XXXXXX", (int)getpid());
    int fd = mkstemp(path);
    if (fd < 0) { perror("mkstemp"); return 1; }
    ftruncate(fd, 4096);
    if (strcmp(mode, "rootfile") == 0) {
        chown(path, 0, 0);      /* root-owned */
    }
    chmod(path, 0666);          /* writable by nobody -> attacker */

    pid_t victim = fork();
    if (victim == 0) {
        void *p = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (p == MAP_FAILED) { perror("mmap"); _exit(2); }
        memset(p, 0x41, 4096);  /* fault pages in */
        printf("[victim uid=%d] mapped %s, idling...\n", getuid(), path);
        sleep(3);               /* let attacker truncate */
        printf("[victim uid=%d] reading mapping after truncate...\n", getuid());
        volatile unsigned char v = ((volatile unsigned char*)p)[3000];
        printf("[victim uid=%d] SURVIVED, read byte=0x%02x (not killed!)\n", getuid(), v);
        _exit(0);
    }

    /* attacker truncates (fork + drop to nobody via setuid) */
    pid_t atk = fork();
    if (atk == 0) {
        if (setgid(65534) || setuid(65534)) { perror("setuid"); _exit(3); }
        sleep(1);
        int afd = open(path, O_WRONLY);
        if (afd < 0) { perror("attacker open"); _exit(4); }
        printf("[attacker uid=%d] truncating %s to 0 bytes\n", getuid(), path);
        ftruncate(afd, 0);
        close(afd);
        _exit(0);
    }

    waitpid(atk, NULL, 0);
    int st; waitpid(victim, &st, 0);

    printf("[parent] victim died via signal=%d (%s), exit_status=%d, WIFSIGNALED=%d\n",
           WIFSIGNALED(st) ? WTERMSIG(st) : 0,
           WIFSIGNALED(st) ? (WTERMSIG(st)==SIGBUS?"SIGBUS":(WTERMSIG(st)==SIGSEGV?"SIGSEGV":"other")) : "normal",
           WIFEXITED(st)?WEXITSTATUS(st):-1, WIFSIGNALED(st));
    unlink(path);

    /* Part 2: can the victim CATCH SIGBUS? */
    printf("--- part 2: SIGBUS catchability test ---\n");
    char p2[128];
    snprintf(p2, sizeof(p2), "/tmp/victim-map2.%d.XXXXXX", (int)getpid());
    fd = mkstemp(p2);
    ftruncate(fd, 4096); chmod(p2, 0666);
    victim = fork();
    if (victim == 0) {
        struct sigaction sa; memset(&sa,0,sizeof sa); sa.sa_handler = onbus; sigaction(SIGBUS, &sa, NULL);
        void *p = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        memset(p, 0x42, 4096);
        printf("[victim] installed SIGBUS handler, mapping up...\n"); fflush(NULL);
        sleep(2); /* attacker truncates at ~1s, deterministic */
        printf("[victim] reading past truncate...\n"); fflush(NULL);
        while (1) {
            volatile unsigned char v = ((volatile unsigned char*)p)[3500];
            (void)v;
            printf("[victim] read OK (no fault) sigbus_count=%d\n", sigbus_count); fflush(NULL);
            _exit(0);
        }
    }
    atk = fork();
    if (atk == 0) {
        if (setgid(65534) || setuid(65534)) { perror("setuid"); _exit(3); }
        sleep(1);
        int afd = open(p2, O_WRONLY);
        ftruncate(afd, 0); close(afd);
        _exit(0);
    }
    waitpid(atk, NULL, 0);
    waitpid(victim, &st, 0);
    printf("[parent] catch-test victim: signal=%d WIFSIGNALED=%d\n",
           WIFSIGNALED(st)?WTERMSIG(st):0, WIFSIGNALED(st));
    unlink(p2);
    return 0;
}