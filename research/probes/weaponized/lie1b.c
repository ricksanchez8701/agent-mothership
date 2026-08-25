#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <string.h>
int fd;
void *closer(void *x){ usleep(500000); close(fd); printf("[thread] closed fd %d mid-select\n", fd); return NULL; }
int main(void){
  int sv[2]; pipe(sv); fd = sv[0];
  pthread_t t; pthread_create(&t, NULL, closer, NULL);
  unsigned long long spins=0;
  struct timeval tv; tv.tv_sec=2; tv.tv_usec=0;
  fd_set rfds; struct timeval t2;
  while (spins < 200000000) {
    FD_ZERO(&rfds); FD_SET(fd,&rfds); t2=tv;
    int r = select(fd+1, &rfds, NULL, NULL, &t2);
    if (r < 0) { spins++; if (errno==EBADF) { if(spins==1) printf("[main] select returned -1 EBADF; naive loop ignores & re-selects -> spin\n"); } }
    else if (r>0 && FD_ISSET(fd,&rfds)) { spins++; char b; read(fd,&b,1); }
    if (spins % 1000000 == 0 && spins) { printf("...spinning, spins=%llu\n", spins); break; }
  }
  return 0;
}
