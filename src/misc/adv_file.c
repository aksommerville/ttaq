#include "adv.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* read whole file
 *****************************************************************************/
 
int adv_file_read(void *dstpp,const char *path) {
  if (!dstpp||!path) return -1;
  int fd=open(path,O_RDONLY);
  if (fd<0) { 
    fprintf(stderr,"%s: %s\n",path,strerror(errno));
    return -1;
  }
  off_t flen=lseek(fd,0,SEEK_END);
  if (flen<0) {
    fprintf(stderr,"%s: %s\n",path,strerror(errno));
    close(fd);
    return -1;
  }
  if (!flen) { close(fd); return 0; }
  if (flen>INT_MAX) flen=INT_MAX;
  if (lseek(fd,0,SEEK_SET)) {
    fprintf(stderr,"%s: %s\n",path,strerror(errno));
    close(fd);
    return -1;
  }
  char *dst=malloc(flen);
  if (!dst) { close(fd); return -1; }
  int dstc=0; while (dstc<flen) {
    int err=read(fd,dst+dstc,flen-dstc);
    if (err<0) {
      fprintf(stderr,"%s: %s\n",path,strerror(errno));
      close(fd);
      free(dst);
      return -1;
    }
    if (!err) break;
    dstc+=err;
  }
  close(fd);
  *(void**)dstpp=dst;
  return dstc;
}

/* write whole file
 *****************************************************************************/
  
int adv_file_write(const char *path,const void *src,int srcc) {
  if (!path||(srcc<0)||(srcc&&!src)) return -1;
  int fd=open(path,O_WRONLY|O_CREAT|O_TRUNC,0666);
  if (fd<0) {
    fprintf(stderr,"%s: %s\n",path,strerror(errno));
    return -1;
  }
  int srcp=0; while (srcp<srcc) {
    int err=write(fd,(char*)src+srcp,srcc-srcp);
    if (err<=0) {
      fprintf(stderr,"%s: %s\n",path,strerror(errno));
      close(fd);
      unlink(path);
      return -1;
    }
    srcp+=err;
  }
  close(fd);
  return 0;
}
