#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <fcntl.h>

/* [18] SCM_RIGHTS fd passing root -> nobody */
int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "recv")) {
        int sv[2];
        if (sscanf(argv[2], "%d,%d", &sv[0], &sv[1]) != 2) return 1;
        close(sv[1]);
        char cbuf[64];
        struct iovec iov = { .iov_base = cbuf, .iov_len = 1 };
        char cmsgbuf[CMSG_SPACE(sizeof(int))];
        struct msghdr msg = {0};
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgbuf;
        msg.msg_controllen = sizeof(cmsgbuf);
        ssize_t n = recvmsg(sv[0], &msg, 0);
        if (n < 0) { printf("RECV recvmsg errno=%d %s\n", errno, strerror(errno)); return 1; }
        struct cmsghdr *c = CMSG_FIRSTHDR(&msg);
        int gotfd = -1;
        if (c && c->cmsg_type == SCM_RIGHTS) memcpy(&gotfd, CMSG_DATA(c), sizeof(int));
        printf("RECV(nobody) got fd=%d\n", gotfd);
        if (gotfd >= 0) {
            struct stat st;
            fstat(gotfd, &st);
            printf("RECV fd points to: mode=%o size=%lld\n", st.st_mode, (long long)st.st_size);
            char buf[128] = {0};
            ssize_t nr = read(gotfd, buf, sizeof(buf)-1);
            printf("RECV first bytes (%zd): %.80s\n", nr, buf);
            if (st.st_size > 0) printf("RECV => READ ACCESS ON FD PASSED BY ROOT = SUCCESS\n");
            close(gotfd);
        }
        return 0;
    }

    if (getuid() != 0) { printf("must run as root to pass fd\n"); return 1; }
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    printf("ROOT socketpair=%d,%d passing /etc/shadow fd to nobody child...\n", sv[0], sv[1]);
    int fd = open("/etc/shadow", O_RDONLY);
    if (fd < 0) { printf("open /etc/shadow as root failed: %s\n", strerror(errno)); return 1; }
    pid_t kid = fork();
    if (kid == 0) {
        setuid(65534); setgid(65534);
        char p[32];
        snprintf(p, sizeof(p), "%d,%d", sv[0], sv[1]);
        execl("/proc/self/exe", "scm", "recv", p, NULL);
        perror("exec"); _exit(1);
    }
    sleep(1);
    struct iovec iov = { .iov_base = (void*)"X", .iov_len = 1 };
    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    struct msghdr msg = {0};
    msg.msg_iov = &iov; msg.msg_iovlen = 1;
    msg.msg_control = cmsgbuf; msg.msg_controllen = sizeof(cmsgbuf);
    struct cmsghdr *c = CMSG_FIRSTHDR(&msg);
    c->cmsg_len = CMSG_LEN(sizeof(int));
    c->cmsg_level = SOL_SOCKET;
    c->cmsg_type = SCM_RIGHTS;
    memcpy(CMSG_DATA(c), &fd, sizeof(int));
    sendmsg(sv[1], &msg, 0);
    waitpid(kid, NULL, 0);
    return 0;
}