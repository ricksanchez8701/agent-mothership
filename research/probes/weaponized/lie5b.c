#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
static int present(int pm, void *addr){
  uint64_t e=0;
  off_t off = ((uintptr_t)addr / 4096) * 8;
  if (pread(pm, &e, 8, off) != 8) return -1;
  return (e & (1ULL<<63)) ? 1 : 0;
}
int main(void){
  size_t sz=64*1024*1024;
  char *p=mmap(NULL,sz,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  memset(p,0xAB,sz);
  int pm=open("/proc/self/pagemap",O_RDONLY);
  char *addr=p + 3*1024*1024;
  printf("RSS before MADV_FREE  = %ld MB ; present-bit(p[3MB])=%d\n", sysconf(_SC_PAGESIZE)*(long)getpagesize(), present(pm,addr)); 
  /* print RSS properly */
  FILE *f=fopen("/proc/self/statm","r"); long t,r2; fscanf(f,"%ld %ld",&t,&r2); fclose(f);
  printf("RSS before MADV_FREE  = %ld MB ; present-bit(p[3MB])=%d\n", r2*4096/1024/1024, present(pm,addr));
  madvise(p, sz, MADV_FREE);
  FILE *g=fopen("/proc/self/statm","r"); long t2,r3; fscanf(g,"%ld %ld",&t2,&r3); fclose(g);
  printf("RSS after  MADV_FREE  = %ld MB ; present-bit STILL set=%d ; byte=0x%02x\n", r3*4096/1024/1024, present(pm,addr), (unsigned char)p[3*1024*1024]);
  return 0;
}
