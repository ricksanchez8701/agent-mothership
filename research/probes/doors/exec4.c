#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(void){
    errno=0; int tf = open("/tmp", O_TMPFILE | O_RDWR, 0600);
    printf("open O_TMPFILE        ret=%d errno=%d %s\n", tf, errno, tf<0?strerror(errno):"");
    if (tf>=0){
        char pbuf[64]; snprintf(pbuf,sizeof(pbuf),"/proc/self/fd/%d",tf);
        errno=0; int r = linkat(AT_FDCWD, pbuf, AT_FDCWD, "/tmp/mat1", AT_SYMLINK_FOLLOW);
        printf("linkat /proc/self/fd  ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"ok");
        if (r==0){ unlink("/tmp/mat1"); printf("   materialized OK\n"); }
        errno=0; r = linkat(tf, "", AT_FDCWD, "/tmp/mat2", AT_EMPTY_PATH);
        printf("linkat AT_EMPTY_PATH  ret=%d errno=%d %s\n", r, errno, errno?strerror(errno):"ok");
        if (r==0){ unlink("/tmp/mat2"); printf("   materialized OK\n"); }
        close(tf);
    }
    return 0;
}
