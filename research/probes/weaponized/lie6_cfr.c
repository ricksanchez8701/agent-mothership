#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

/* LIE-6: copy_file_range EXDEV across filesystems.
   /etc (overlayfs) -> /dev/shm (tmpfs) must return EXDEV.
   Then show a NAIVE tool that ignores the error produces an EMPTY file.    */

int main(void) {
    int in = open("/etc/hostname", O_RDONLY);
    if (in < 0) { perror("open src"); return 1; }
    int out = open("/dev/shm/cfr_out", O_CREAT|O_WRONLY|O_TRUNC, 0644);

    char buf[4096];
    ssize_t got = read(in, buf, sizeof buf);
    loff_t off_in = 0, off_out = 0;

    errno = 0;
    ssize_t n = copy_file_range(in, &off_in, out, &off_out, (size_t)got, 0);
    printf("copy_file_range(/etc/hostname -> /dev/shm) returned %zd, errno=%d [%s]\n",
           n, errno, strerror(errno));
    close(in); close(out);

    /* naive tool that ignores the return: produces empty file */
    in = open("/etc/hostname", O_RDONLY);
    out = open("/dev/shm/cfr_naive", O_CREAT|O_WRONLY|O_TRUNC, 0644);
    errno = 0;
    copy_file_range(in, &off_in, out, &off_out, (size_t)got, 0); /* error ignored */
    close(in); close(out);

    int sz = 0; char c;
    in = open("/dev/shm/cfr_naive", O_RDONLY);
    while (read(in, &c, 1) > 0) sz++;
    close(in);
    printf("naive tool (ignores EXDEV) produced /dev/shm/cfr_naive of size %d bytes (src was %zd) => DATA LOSS\n",
           sz, got);

    /* now show EXDEV is avoidable: the copy_file_range_alternative (read/write) works */
    in = open("/etc/hostname", O_RDONLY);
    out = open("/dev/shm/cfr_ok", O_CREAT|O_WRONLY|O_TRUNC, 0644);
    char b2[4096]; ssize_t rd;
    while ((rd = read(in, b2, sizeof b2)) > 0) write(out, b2, rd);
    close(in); close(out);
    int ok = 0; in = open("/dev/shm/cfr_ok", O_RDONLY);
    while (read(in, &c, 1) > 0) ok++;
    close(in);
    printf("read/write fallback produced size %d bytes => correct handling fixes it\n", ok);
    return 0;
}