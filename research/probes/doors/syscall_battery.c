#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <linux/io_uring.h>
#include <linux/bpf.h>
#include <linux/perf_event.h>
#include <linux/memfd.h>
#include <linux/filter.h>
#include <linux/keyctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sched.h>
#include <stdint.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <linux/netlink.h>
#include <linux/if_packet.h>
#include <fcntl.h>
#include <sys/sysmacros.h>
#include <signal.h>

#define T(name, call) do { \
    errno = 0; \
    long r = (long)(call); \
    printf("%-28s ret=%-10ld errno=%-5d %s\n", name, r, errno, r==-1?strerror(errno):""); \
} while(0)

int main(void) {
    struct io_uring_params iop;
    memset(&iop, 0, sizeof(iop));
    int efd = memfd_create("x", MFD_CLOEXEC);
    printf("memfd_create(valid arg)   ret=%d errno=%d %s\n", efd, errno, efd<0?strerror(errno):"ok");
    int iofd = (int)syscall(__NR_io_uring_setup, 32, &iop);
    printf("io_uring_setup(valid arg) ret=%d errno=%d %s\n", iofd, errno, iofd<0?strerror(errno):"ok");
    int iofd0 = (int)syscall(__NR_io_uring_setup, 0, &iop);
    printf("io_uring_setup(entries=0) ret=%d errno=%d %s\n", iofd0, errno, iofd0<0?strerror(errno):"ok");
    int bfd = (int)syscall(__NR_bpf, BPF_MAP_CREATE, NULL, 0);
    printf("bpf(BPF_MAP_CREATE)       ret=%d errno=%d %s\n", bfd, errno, bfd<0?strerror(errno):"ok");
    int pfd = (int)syscall(__NR_perf_event_open, NULL, 0, -1, -1, 0);
    printf("perf_event_open           ret=%d errno=%d %s\n", pfd, errno, pfd<0?strerror(errno):"ok");
    int ufd = (int)syscall(__NR_userfaultfd, 0);
    printf("userfaultfd               ret=%d errno=%d %s\n", ufd, errno, ufd<0?strerror(errno):"ok");
    int ak = (int)syscall(__NR_add_key, "user", "t", "v", 1, -2);
    printf("add_key                   ret=%d errno=%d %s\n", ak, errno, ak<0?strerror(errno):"ok");
    int kc = (int)syscall(__NR_keyctl, KEYCTL_GET_KEYRING_ID, KEY_SPEC_PROCESS_KEYRING, 0);
    printf("keyctl(GET_PROC_RING)     ret=%d errno=%d %s\n", kc, errno, kc<0?strerror(errno):"ok");
    int obh = (int)syscall(__NR_open_by_handle_at, AT_FDCWD, NULL, O_RDONLY);
    printf("open_by_handle_at         ret=%d errno=%d %s\n", obh, errno, obh<0?strerror(errno):"ok");
    int nth = (int)syscall(__NR_name_to_handle_at, AT_FDCWD, ".", NULL, NULL, 0);
    printf("name_to_handle_at         ret=%d errno=%d %s\n", nth, errno, nth<0?strerror(errno):"ok");
    int kc2 = (int)syscall(__NR_kcmp, getpid(), getpid(), 0, 0, 0);
    printf("kcmp(self,self)           ret=%d errno=%d %s\n", kc2, errno, kc2<0?strerror(errno):"ok");
    int gmp = (int)syscall(__NR_get_mempolicy, NULL, NULL, 0, 0, 0);
    printf("get_mempolicy             ret=%d errno=%d %s\n", gmp, errno, gmp<0?strerror(errno):"ok");
    int mlock = mlockall(MCL_CURRENT);
    printf("mlockall                  ret=%d errno=%d %s\n", mlock, errno, mlock<0?strerror(errno):"ok");
    void *p = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    int mad = madvise(p, 4096, MADV_HUGEPAGE);
    printf("madvise(HUGEPAGE)         ret=%d errno=%d %s\n", mad, errno, mad<0?strerror(errno):"ok");
    int madh = madvise(p, 4096, MADV_DONTNEED);
    printf("madvise(DONTNEED)         ret=%d errno=%d %s\n", madh, errno, madh<0?strerror(errno):"ok");
    T("finit_module", syscall(__NR_finit_module, -1, "", 0));
    T("init_module", syscall(__NR_init_module, 0, 0, ""));
    T("delete_module", syscall(__NR_delete_module, "x", 0));
    T("reboot", syscall(__NR_reboot, 0xfee1dead, 672274793, 0x1234567, NULL));
    T("sethostname", syscall(__NR_sethostname, "x", 1));
    T("kexec_load", syscall(__NR_kexec_load, 0, 0, 0, 0, 0));
    T("ptrace(ATTACH self)", ptrace(PTRACE_ATTACH, getpid(), 0, 0));
    T("process_vm_readv(-1)", syscall(__NR_process_vm_readv, -1, NULL, 0, NULL, 0, 0));
    T("seccomp(SET_MODE_FILTER)", syscall(__NR_seccomp, 1, 0, NULL));
    T("socket(AF_PACKET,SOCK_RAW)", socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)));
    T("socket(AF_PACKET,SOCK_DGRAM)", socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL)));
    T("socket(AF_INET,SOCK_RAW,ICMP)", socket(AF_INET, SOCK_RAW, IPPROTO_ICMP));
    T("socket(AF_INET,SOCK_DGRAM,ICMP)", socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP));
    T("socket(AF_NETLINK,ROUTE)", socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE));
    T("socket(AF_NETLINK,NETFILTER)", socket(AF_NETLINK, SOCK_RAW, NETLINK_NETFILTER));
    T("socket(AF_INET6,SOCK_RAW,ICMPv6)", socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6));
    T("socket(AF_INET,SOCK_DGRAM,RAW_TCP_svc)", socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP));
    T("bind(AF_INET, :0)", 0);
    T("unshare(CLONE_NEWUSER)", syscall(__NR_unshare, CLONE_NEWUSER));
    T("unshare(CLONE_NEWNS)", syscall(__NR_unshare, CLONE_NEWNS));
    T("unshare(CLONE_NEWUTS)", syscall(__NR_unshare, CLONE_NEWUTS));
    T("setns(0, 0)", syscall(__NR_setns, 0, 0));
    T("mount(proc)", syscall(__NR_mount, "x", "/tmp", "proc", 0, ""));
    T("umount2", syscall(__NR_umount2, "/tmp", 0));
    T("pivot_root", syscall(__NR_pivot_root, "/tmp", "/tmp"));
    T("chroot", syscall(__NR_chroot, "/tmp"));
    T("iopl", syscall(__NR_iopl, 3));
    T("ioperm", syscall(__NR_ioperm, 0, 1, 1));
    T("sched_setaffinity(pid0)", syscall(__NR_sched_setaffinity, 0, 0, NULL));
    T("sched_setscheduler(pid0)", syscall(__NR_sched_setscheduler, 0, SCHED_FIFO, NULL));
    T("sched_getattr", syscall(__NR_sched_getattr, 0, NULL, 0, 0));
    T("acct", syscall(__NR_acct, "/tmp/acct"));
    T("quotactl", syscall(__NR_quotactl, 0, 0, NULL));
    T("swapon", syscall(__NR_swapon, "/tmp/x", 0));
    T("klogctl", syscall(__NR_syslog, 3, NULL, 0));
    T("gettid", syscall(__NR_gettid));
    T("getrlimit(RSS)", syscall(__NR_getrlimit, RLIMIT_RSS, NULL));
    T("pidfd_open(1)", syscall(__NR_pidfd_open, 1, 0));
    T("kill(0,0)", kill(0, 0));
    T("kill(-1,0)", kill(-1, 0));
    T("statx", syscall(__NR_statx, AT_FDCWD, "/etc/shadow", 0, 0, NULL));
    T("faccessat2(open shadow)", syscall(__NR_faccessat2, AT_FDCWD, "/etc/shadow", R_OK, AT_EACCESS));
    T("open(/etc/shadow)", open("/etc/shadow", O_RDONLY));
    T("open(/proc/kcore)", open("/proc/kcore", O_RDONLY));
    return 0;
}
