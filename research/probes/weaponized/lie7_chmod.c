#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

/* LIE-7: chmod(symlink) transparency under fs.protected_symlinks=1.
   Scenarios:
     A) symlink in /tmp directly (sticky world-writable) -> protected_symlinks
        SHOULD block following to a root-owned file
     B) symlink in a NON-sticky subdir owned by nobody -> protection does NOT
        apply -> chmod follows the link and changes TARGET mode
   Root "victim" child runs the chmod(path, 0777) on the attacker path.        */

static void maketarget(const char *p, mode_t mode) {
    int fd = open(p, O_CREAT|O_WRONLY, mode);
    write(fd, "x", 1); close(fd);
    chown(p, 0, 0);      /* root-owned */
    chmod(p, 0644);
}

static mode_t getmode(const char *p) {
    struct stat st; stat(p, &st); return st.st_mode & 07777;
}

int main(void) {
    FILE *pf = fopen("/proc/sys/fs/protected_symlinks","r");
    int psl=0; if (pf) { fscanf(pf,"%d",&psl); fclose(pf); }
    printf("fs.protected_symlinks=%d\n", psl);

    /* Scenario A: /tmp (sticky) */
    system("mkdir -p /tmp/attA && chmod 777 /tmp/attA");
    maketarget("/tmp/attA/rootfile", 0644);
    /* attacker (nobody) drops symlink in the sticky dir /tmp? sticky dir itself is
       /tmp (owned root, sticky). nobody creates symlink directly in /tmp. */
    {
        pid_t atk = fork();
        if (atk == 0) {
            if (setgid(65534)||setuid(65534)) _exit(9);
            unlink("/tmp/rootlinkA");
            int r = symlink("/tmp/attA/rootfile", "/tmp/rootlinkA");
            _exit(r < 0 ? 1 : 0);
        }
        int st; waitpid(atk, &st, 0);
        printf("A) nobody created /tmp/rootlinkA -> %s (owner: root)\n", st==0?"OK":"FAILED");
        /* root victim chmods the symlink path */
        mode_t before = getmode("/tmp/attA/rootfile");
        int r = chmod("/tmp/rootlinkA", 0777);
        mode_t after = getmode("/tmp/attA/rootfile");
        printf("   root chmod(/tmp/rootlinkA,0777) rc=%d ; rootfile mode %o->%o => %s\n",
               r, before, after,
               r!=0 ? "BLOCKED (protected_symlinks held)" : "FOLLOWED (vuln!)");
        unlink("/tmp/rootlinkA");
    }

    /* Scenario B: NON-sticky subdir owned by nobody */
    {
        pid_t atk = fork();
        if (atk == 0) {
            if (setgid(65534)||setuid(65534)) _exit(9);
            mkdir("/tmp/weap/attB", 0777);   /* non-sticky, owned by nobody */
            chmod("/tmp/weap/attB", 0777);
            unlink("/tmp/weap/attB/rootfile"); unlink("/tmp/weap/attB/link");
            _exit(0);
        }
        waitpid(atk, NULL, 0);
        maketarget("/tmp/weap/attB/rootfile", 0644);
        atk = fork();
        if (atk == 0) {
            if (setgid(65534)||setuid(65534)) _exit(9);
            symlink("/tmp/weap/attB/rootfile", "/tmp/weap/attB/link");
            _exit(0);
        }
        waitpid(atk, NULL, 0);
        mode_t before = getmode("/tmp/weap/attB/rootfile");
        errno = 0;
        int r = chmod("/tmp/weap/attB/link", 0777);
        mode_t after = getmode("/tmp/weap/attB/rootfile");
        printf("B) root chmod(nonsticky-dir symlink,0777) rc=%d errno=%d ; rootfile mode %o->%o => %s\n",
               r, errno, before, after,
               (after&0777)==0777 ? "FOLLOWED — root file mode changed via symlink!" :
               "target unchanged");
        /* also chown through the link */
        errno = 0;
        r = chown("/tmp/weap/attB/link", 65534, 65534);
        struct stat st; stat("/tmp/weap/attB/rootfile", &st);
        printf("   chown(link, nobody) rc=%d ; rootfile now owner uid=%d => %s\n",
               r, st.st_uid, st.st_uid==65534 ? "OWNERSHIP TRANSFERRED via symlink!" : "unchanged");
        unlink("/tmp/weap/attB/link"); unlink("/tmp/weap/attB/rootfile");
    }
    return 0;
}