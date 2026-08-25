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
#include <sys/uio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

/* [16] ICMP ping as nobody */
int try_ping(void) {
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (s < 0) { printf("   ping socket create ret=%d errno=%d %s\n", s, errno, strerror(errno)); return -1; }
    struct sockaddr_in dst = {0};
    dst.sin_family = AF_INET;
    dst.sin_port = 0;
    inet_pton(AF_INET, "127.0.0.1", &dst.sin_addr);
    struct icmphdr ic;
    memset(&ic, 0, sizeof(ic));
    ic.type = ICMP_ECHO;
    ic.un.echo.id = htons(getpid() & 0xffff);
    ic.un.echo.sequence = htons(1);
    ic.checksum = 0; /* kernel fills for dgram ping */
    int r = sendto(s, &ic, sizeof(ic), 0, (struct sockaddr*)&dst, sizeof(dst));
    printf("   sendto ICMP echo 127.0.0.1 ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"");
    struct sockaddr_in from; socklen_t fl = sizeof(from);
    char rbuf[200]; struct timeval tv = {.tv_sec=2};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    errno = 0;
    ssize_t nr = recvfrom(s, rbuf, sizeof(rbuf), 0, (struct sockaddr*)&from, &fl);
    printf("   recvfrom ICMP reply ret=%zd errno=%d %s\n", nr, errno, errno?strerror(errno):"PING WORKED");
    close(s);
    return 0;
}

/* [17] NETLINK_ROUTE dump */
int rt_dump(void) {
    int s = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (s < 0) { printf("   netlink route socket ret=%d errno=%d %s\n", s, errno, strerror(errno)); return -1; }
    struct sockaddr_nl sa = { .nl_family = AF_NETLINK };
    if (bind(s, (struct sockaddr*)&sa, sizeof(sa)) < 0) { printf("   bind failed %s\n", strerror(errno)); return -1; }
    struct { struct nlmsghdr n; struct ifaddrmsg ifa; char buf[256]; } req;
    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    req.n.nlmsg_type = RTM_GETADDR;
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    req.ifa.ifa_family = AF_UNSPEC;
    send(s, &req, req.n.nlmsg_len, 0);
    char buf[8192];
    ssize_t n = recv(s, buf, sizeof(buf), 0);
    if (n < 0) { printf("   recv failed errno=%d %s\n", errno, strerror(errno)); return -1; }
    int addrs = 0, links = 0, routes = 0;
    for (struct nlmsghdr *nh = (struct nlmsghdr*)buf; NLMSG_OK(nh, (unsigned)n); nh = NLMSG_NEXT(nh, n)) {
        if (nh->nlmsg_type == NLMSG_DONE) break;
        if (nh->nlmsg_type == RTM_NEWADDR) { addrs++; }
        else if (nh->nlmsg_type == RTM_NEWLINK) links++;
        else if (nh->nlmsg_type == RTM_NEWROUTE) routes++;
    }
    printf("   NETLINK_ROUTE dump: links=%d addrs=%d routes=%d  (topology leak OK)\n", links, addrs, routes);
    close(s);
    return 0;
}

int main(void) {
    printf("[16] unprivileged ICMP (ping):\n");
    try_ping();
    printf("[17] NETLINK_ROUTE topology dump:\n");
    rt_dump();
    return 0;
}