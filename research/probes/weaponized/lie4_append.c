#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* LIE-4: pwrite+O_APPEND ignores offset.
   (1) O_APPEND fd: pwrite(fd, ..., 0) appends at END.
   (2) The real hole: O_APPEND only protects the fd that holds it. Another
       process opening the SAME file via a second path without O_APPEND can
       overwrite/truncate/corrupt records despite the daemon's O_APPEND.      */

int main(void) {
    const char *path = "/tmp/weap/audit.log";
    unlink(path);

    /* daemon writes two records */
    int d = open(path, O_CREAT|O_WRONLY|O_APPEND, 0666);
    write(d, "RECORD-1\n", 9);
    write(d, "RECORD-2\n", 9);

    /* (1) pwrite with offset 0 on O_APPEND fd */
    off_t before = lseek(d, 0, SEEK_END);
    ssize_t w = pwrite(d, "RECORD-3\n", 9, 0);
    off_t after = lseek(d, 0, SEEK_END);
    printf("(1) O_APPEND pwrite(fd,...,offset=0) wrote %zd bytes; file grew %ld->%ld "
           "(data landed at END, offset IGNORED)\n", w, before, after);
    close(d);

    printf("    file content:\n");
    { int r = open(path, O_RDONLY); char b[512]; ssize_t n; while((n=read(r,b,sizeof b))>0) fwrite(b,1,n,stdout); close(r); }

    /* (2) attacker opens second path WITHOUT O_APPEND and overwrites a record */
    printf("(2) attacker opens %s WITHOUT O_APPEND (second pathname/hardlink) and overwrites RECORD-1..\n", path);
    int a = open(path, O_WRONLY);   /* no O_APPEND */
    pwrite(a, "EVIL-RECORD\n", 12, 0);
    pwrite(a, "EVIL-RECORD\n", 12, 9);  /* clobber RECORD-2 too */
    close(a);

    int r = open(path, O_RDONLY); char b[512]; ssize_t n;
    printf("    corrupted content: ");
    while((n=read(r,b,sizeof b))>0) fwrite(b,1,n,stdout);
    printf("\n"); close(r);

    /* (3) truncate via second path even though daemon holds O_APPEND */
    a = open(path, O_WRONLY);
    ftruncate(a, 0);
    printf("(3) ftruncate via non-O_APPEND path succeeded (size now %ld) -> audit log destroyed\n", (long)lseek(a,0,SEEK_END));
    close(a);
    unlink(path);
    return 0;
}