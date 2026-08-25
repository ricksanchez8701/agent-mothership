// DISCOVER.c — kernel self-contract probe battery.
// Attacks the question: where does Linux lie about its own documented
// semantics? Every probe is isolated; dangerous ones fork a child.
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/statfs.h>
#include <linux/random.h>

#define P(...) printf(__VA_ARGS__)
#define PN(name) P("[%-2d] %-44s ", ++seq, name)

static int seq = 0;

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    P("=== DISCOVER battery  (kernel %s) ===\n", "6.8");
    fflush(stdout);

    /* -- 1/2: select() vs poll() on a closed fd (the POLLNVAL blind spot) -- */
    int c = dup(0); close(c);
    { fd_set rf; FD_ZERO(&rf); FD_SET(c, &rf);
      struct timeval tv = {0,0};
      errno=0; int n = select(c+1, &rf, NULL, NULL, &tv);
      PN("select() on CLOSED fd");
      P("n=%d errno=%s (errno only, NOT which fd)\n", n, n<0?strerror(errno):"none"); }
    { struct pollfd p = {.fd=c, .events=POLLIN};
      int n = poll(&p, 1, 0);
      PN("poll() on same CLOSED fd");
      P("n=%d revents=0x%x (poll NAMES the bad fd via POLLNVAL: %s)\n", n, p.revents,
        (p.revents & POLLNVAL) ? "yes" : "no"); }

    /* -- 1b: the SELECT RACE — fd closed while select() waits -- */
    { int pfd[2]; pipe(pfd);
      pid_t p = fork();
      if (p==0) { /* child: close the read end after a delay */
        usleep(150000); close(pfd[0]); _exit(0); }
      close(pfd[0]);
      fd_set rf; FD_ZERO(&rf); FD_SET(pfd[1], &rf); /* write end in readfds: will never be ready */
      struct timeval tv = {3, 0};
      int n = select(pfd[1]+1, &rf, NULL, NULL, &tv);
      PN("select() race: fd closed while waiting");
      P("n=%d FD_ISSET=%d (Linux reports the closed fd as READY: %s)\n", n, FD_ISSET(pfd[1],&rf),
        (n>0 && FD_ISSET(pfd[1],&rf)) ? "the lie — no way to tell it's not" : "timed out");
      waitpid(p, NULL, 0); close(pfd[1]); }

    /* -- 3: mmap() on a pipe: ENODEV? -- */
    { int pfd[2]; pipe(pfd);
      void *m = mmap(NULL, 4096, PROT_READ, MAP_SHARED, pfd[0], 0);
      PN("mmap() a pipe fd");
      P("%s (%s)\n", m==MAP_FAILED ? strerror(errno) : "WORKED?!", errno==ENODEV ? "ENODEV = 'no device'" : errno ? "non-ENODEV errno" : "no error");
      close(pfd[0]); close(pfd[1]); }

    /* -- 4: pread() on a pipe: ESPIPE? -- */
    { int pfd[2]; pipe(pfd); char b[16]; errno=0;
      ssize_t r = pread(pfd[0], b, 1, 0);
      PN("pread() on a pipe");
      P("ret=%zd errno=%s\n", r, r<0 ? strerror(errno) : "none"); close(pfd[0]); close(pfd[1]); }

    /* -- 5: pread() on a socket: ESPIPE? -- */
    { int s = socket(AF_UNIX, SOCK_STREAM, 0); char b[16]; errno=0;
      ssize_t r = pread(s, b, 1, 0);
      PN("pread() on a socket");
      P("ret=%zd errno=%s\n", r, r<0 ? strerror(errno) : "none"); close(s); }

    /* -- 6: ftruncate() on an O_RDONLY fd -- */
    { char tmpl[]="/tmp/discover.XXXXXX"; int fd = mkstemp(tmpl); unlink(tmpl);
      close(fd); fd = open(tmpl, O_RDONLY);
      errno=0; int r = ftruncate(fd, 100);
      PN("ftruncate() on O_RDONLY fd");
      P("ret=%d errno=%s\n", r, r<0 ? strerror(errno) : "none"); close(fd); }

    /* -- 7: sendfile() from a directory -- */
    { int in = open("/", O_RDONLY); char tmp[]="/tmp/dsc-sf.XXXXXX";
      int out = mkstemp(tmp); unlink(tmp);
      errno=0; ssize_t r = sendfile(out, in, NULL, 4096);
      PN("sendfile() dir -> file");
      P("ret=%zd errno=%s\n", r, r<0 ? strerror(errno) : "none"); close(in); close(out); }

    /* -- 8: getdents (readdir) on a socket -- */
    { int s = socket(AF_UNIX, SOCK_STREAM, 0);
      DIR *d = fdopendir(s);
      PN("fdopendir() on a socket");
      P("%s\n", d ? "WORKED?! (can readdir a socket)" : strerror(errno));
      if (d) closedir(d); else close(s); }

    /* -- 9: O_TMPFILE (invisible file) -- */
    { errno=0; int fd = open("/tmp", O_TMPFILE | O_RDWR, 0600);
      PN("O_TMPFILE in /tmp");
      P("%s (%s)\n", fd>=0 ? "works" : strerror(errno), errno ? "" : "fd>=0");
      if (fd>=0) close(fd); }

    /* -- 10: execve with NULL argv (Linux allows? POSIX EFAULT?) -- */
    { pid_t p = fork();
      if (p==0) { errno=0; execve("/bin/true", NULL, NULL); _exit(2); }
      int st; waitpid(p, &st, 0);
      PN("execve(NULL argv, NULL envp)");
      P("child %s\n", WIFEXITED(st) && WEXITSTATUS(st)==0 ? "RAN (NULL argv allowed)" : "kernel rejected"); }

    /* -- 11: execve of a shebang-less script (no /bin/sh fallback) -- */
    { char tmp[]="/tmp/dsc-script.XXXXXX"; int fd = mkstemp(tmp);
      write(fd, "echo hi\n", 8); close(fd); chmod(tmp, 0755);
      pid_t p = fork();
      if (p==0) { errno=0; execve(tmp, (char*[]){tmp, NULL}, NULL); _exit(42); }
      int st; waitpid(p, &st, 0);
      PN("execve(text, no shebang)");
      P("errno-path exit=%d (kernel returns ENOEXEC, NO shell fallback)\n", WEXITSTATUS(st));
      unlink(tmp); }

    /* -- 12: open trailing-slash matrix -- */
    { mkdir("/tmp/dsc-d", 0755);
      errno=0; int a = open("/tmp/dsc-d/", O_RDONLY);
      PN("open(dir/)");
      P("fd=%d\n", a); if (a>=0) close(a);
      errno=0; int b = open("/tmp/dsc-d/", O_CREAT, 0600);
      PN("open(dir/, O_CREAT)");
      P("%s (%s)\n", b<0 ? strerror(errno) : "created?!", b<0 ? "" : "");
      if (b>=0) close(b);
      char tmp[]="/tmp/dsc-f.XXXXXX"; int f = mkstemp(tmp); close(f);
      errno=0; int c = open(tmp, O_RDONLY | O_CREAT, 0600); close(c);
      char tslash[128]; snprintf(tslash, sizeof tslash, "%s/", tmp);
      errno=0; c = open(tslash, O_RDONLY);
      PN("open(file/) trailing slash");
      P("%s\n", c<0 ? strerror(errno) : "opened as dir?!"); if (c>=0) close(c);
      unlink(tmp); rmdir("/tmp/dsc-d"); }

    /* -- 13: rename matrix -- */
    { char a[]="/tmp/dsc-ra.XXXXXX", b[]="/tmp/dsc-rb.XXXXXX";
      int fa=mkstemp(a), fb=mkstemp(b); close(fa); close(fb);
      errno=0; int r = rename(a, b);
      PN("rename(file->existing file)");
      P("%s\n", r==0 ? "overwrites silently (no EEXIST)" : strerror(errno));
      unlink(a); unlink(b); }

    /* -- 16: chmod on a symlink silently changes the TARGET -- */
    { char t[]="/tmp/dsc-t.XXXXXX"; int fd=mkstemp(t); close(fd); chmod(t, 0644);
      char l[]="/tmp/dsc-l.XXXXXX"; int ld=mkstemp(l); close(ld); unlink(l);
      symlink(t, l);
      struct stat before, after; stat(t, &before);
      errno=0; int r = chmod(l, 0600);
      stat(t, &after);
      PN("chmod(symlink)");
      P("ret=%d mode %o->%o (kernel followed the link and changed the TARGET: %s)\n", r, before.st_mode&0777, after.st_mode&0777,
        (before.st_mode&0777)!=(after.st_mode&0777) ? "YES — the lie" : "no");
      unlink(l); unlink(t); }

    /* -- 15: file persists after unlink while open; /proc name gets a lie suffix -- */
    { char tmp[]="/tmp/dsc-del.XXXXXX"; int fd = mkstemp(tmp);
      struct stat st1, st2; fstat(fd, &st1);
      unlink(tmp);
      char link[128]; sprintf(link, "/proc/self/fd/%d", fd);
      ssize_t n = readlink(link, tmp, sizeof(tmp)-1); tmp[n]=0;
      fstat(fd, &st2);
      PN("unlink() while open");
      P("st_nlink %ld->%ld; readlink=%s\n", st1.st_nlink, st2.st_nlink,
        strstr(tmp, "(deleted)") ? "WITH '(deleted)' suffix" : "NO suffix?! path unchanged");
      close(fd); }

    /* -- 16: SIGPIPE: write to broken pipe KILLS you (default) vs EPIPE (ignored) -- */
    { pid_t p; int pfd[2]; pipe(pfd); close(pfd[0]);
      p = fork();
      if (p==0) { write(pfd[1], "x", 1); /* default SIGPIPE */ _exit(3); }
      int st; waitpid(p, &st, 0);
      PN("write broken pipe, SIGPIPE default");
      P("%s\n", WIFSIGNALED(st) && WTERMSIG(st)==SIGPIPE ? "killed by SIGPIPE (no error returned)" : "returned");
      p = fork();
      if (p==0) { signal(SIGPIPE, SIG_IGN); errno=0; ssize_t r=write(pfd[1],"x",1); _exit(r<0&&errno==EPIPE?4:5); }
      waitpid(p, &st, 0);
      PN("write broken pipe, SIGPIPE ignored");
      P("ret EPIPE (errno path) %s\n", WEXITSTATUS(st)==4 ? "yes" : "no");
      close(pfd[1]); }

    /* -- 17: mmap + truncate -> SIGBUS (in child) -- */
    { char tmp[]="/tmp/dsc-bus.XXXXXX"; int fd=mkstemp(tmp);
      ftruncate(fd, 4096);
      pid_t p = fork();
      if (p==0) { char *m = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
        ftruncate(fd, 0);
        volatile char x = m[0]; /* should SIGBUS */
        _exit(x==0?6:7); }
      int st; waitpid(p, &st, 0);
      PN("mmap then truncate, read");
      P("%s\n", WIFSIGNALED(st) && WTERMSIG(st)==SIGBUS ? "SIGBUS: kernel killed the read of an allowed mapping" : "survived");
      close(fd); unlink(tmp); }

    /* -- 18: setpriority silently CLAMPS out-of-range nice -- */
    { errno=0; int r = setpriority(PRIO_PROCESS, 0, 21);
      int nice_now = getpriority(PRIO_PROCESS, 0);
      PN("setpriority(21) — outside [-20,19]");
      P("ret=%d (%s) nice_now=%d (clamp is %s)\n", r, r<0?strerror(errno):"no error", nice_now,
        nice_now==19 ? "SILENT, returns 0" : "rejected"); }

    /* -- 19: nice(-20) without privilege -> EPERM -- */
    { pid_t p = fork();
      if (p==0) { errno=0; int r = setpriority(PRIO_PROCESS, 0, -20);
        P("[19] setpriority(-20) in child: %s\n", r<0 ? strerror(errno) : "allowed?!"); _exit(0); }
      waitpid(p, NULL, 0); }

    /* -- 22: O_APPEND + pwrite: does the offset argument still matter? -- */
    { char tmp[]="/tmp/dsc-ap.XXXXXX"; int f0=mkstemp(tmp); write(f0, "AAAAAAAAAA", 10); close(f0);
      int fd = open(tmp, O_WRONLY | O_APPEND);
      errno=0; ssize_t r = pwrite(fd, "BB", 2, 0);
      struct stat st; fstat(fd, &st);
      PN("O_APPEND + pwrite(fd, 2, offset 0)");
      P("wrote=%zd size=%lld (Linux pwrite + O_APPEND appends, offset ignored: %s)\n", r, (long long)st.st_size,
        st.st_size==12 ? "YES — offset ignored, append won" : "offset won");
      close(fd); unlink(tmp); }

    /* -- 21: MADV_FREE vs MADV_DONTNEED: RSS accounting lie -- */
    { size_t SZ = 64*1024*1024;
      long pages = sysconf(_SC_PAGESIZE);
      char *m = mmap(NULL, SZ, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
      memset(m, 1, SZ);
      long rss_before = 0; /* read statm */
      FILE *f = fopen("/proc/self/statm", "r"); long t,rss; fscanf(f, "%ld %ld", &t, &rss); fclose(f); rss_before = rss*pages/1024;
      madvise(m, SZ, MADV_FREE);
      f = fopen("/proc/self/statm", "r"); fscanf(f, "%ld %ld", &t, &rss); fclose(f);
      PN("MADV_FREE 64MB, RSS immediately");
      P("RSS %ldKB -> %ldKB (FREE is lazy: %s)\n", rss_before, rss*pages/1024,
        (rss*pages/1024) >= rss_before/2 ? "pages still resident = the 'free' lie" : "actually freed");
      munmap(m, SZ); }

    /* -- 22: copy_file_range across filesystems -- */
    { int in = open("/etc/hostname", O_RDONLY);
      char tmp[]="/dev/shm/dsc-cfr.XXXXXX"; int out = mkstemp(tmp);
      errno=0; ssize_t r = copy_file_range(in, NULL, out, NULL, 4096, 0);
      PN("copy_file_range(/ -> /dev/shm)");
      P("ret=%zd errno=%s (cross-fs: caller must re-implement copy)\n", r, r<0?strerror(errno):"none");
      close(in); close(out); unlink(tmp); }

    /* -- 23: clock_getres: the resolution lie -- */
    { struct timespec rs; clock_getres(CLOCK_MONOTONIC, &rs);
      PN("clock_getres(CLOCK_MONOTONIC)");
      P("%ldns nominal (actual granularity is a hardware lie too)\n", rs.tv_nsec); }

    /* -- 24: getrusage ru_maxrss units -- */
    { char *m = mmap(NULL, 100*1024*1024, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
      memset(m, 1, 100*1024*1024);
      struct rusage ru; getrusage(RUSAGE_SELF, &ru);
      PN("getrusage ru_maxrss after 100MB touch");
      P("%ld (Linux=KB, but macOS=bytes: same field, different units)\n", ru.ru_maxrss);
      munmap(m, 100*1024*1024); }

    /* -- 25: gettid() == getpid() only in the main thread -- */
    { pid_t tid = syscall(SYS_gettid), pid = getpid();
      PN("gettid() vs getpid() (main thread)");
      P("tid=%d pid=%d %s\n", tid, pid, tid==pid ? "equal (the identity collapse)" : "different"); }

    /* -- 26: orphan reparenting: getppid() silently becomes 1 -- */
    { int pfd[2]; pipe(pfd);
      pid_t p = fork();
      if (p==0) { close(pfd[1]); /* child: keep read end only */
        char c; ssize_t r = read(pfd[0], &c, 1); /* returns 0 (EOF) when parent dies */
        pid_t now = getppid();
        P("[26] child getppid() after parent exits: %d (%s)\n", now,
          now==1 ? "reparented to pid 1 — ppid changed behind our back" : "still has a parent");
        _exit(0); }
      close(pfd[0]);          /* parent: keep write end */
      usleep(200000);
      /* parent now dies; its end of the pipe closes, child read() hits EOF */
      _exit(0); }
    /* NOTE: parent exits here; main never reaches the rest on this branch */

    P("=== battery complete ===\n");
    return 0;
}