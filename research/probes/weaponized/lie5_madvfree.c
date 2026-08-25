#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

/* LIE-5: MADV_FREE lazy-free RSS lie (fixed: direct mmap, not malloc). */

static long rss_kb(void) {
    FILE *f = fopen("/proc/self/statm", "r");
    long tot, res; fscanf(f, "%ld %ld", &tot, &res); fclose(f);
    return res * (long)sysconf(_SC_PAGESIZE) / 1024;
}

int main(void) {
    const size_t MB = 128;
    size_t sz = MB * 1024 * 1024;
    char *p = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }
    memset(p, 0xAB, sz);
    long real = rss_kb();
    printf("after fill:          RSS = %ld MB\n", real / 1024);

    errno = 0;
    int r = madvise(p, sz, MADV_FREE);
    long rss_after = rss_kb();
    printf("madvise(MADV_FREE) returned %d (errno=%d)\n", r, errno);
    printf("after MADV_FREE:     RSS = %ld MB  -> kernel reclaimed ~0 MB (pages STILL resident)\n",
           rss_after / 1024);

    unsigned char sample = (unsigned char)p[42];
    printf("secret byte p[42] reads 0x%02x (0xab) -> sensitive data STILL in RAM after 'free'\n", sample);

    /* compare: MADV_DONTNEED genuinely drops RSS */
    errno = 0;
    r = madvise(p, sz, MADV_DONTNEED);
    printf("madvise(MADV_DONTNEED) RSS now = %ld MB (real release)\n", rss_kb() / 1024);
    munmap(p, sz);
    return 0;
}