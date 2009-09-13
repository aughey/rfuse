#include <stdlib.h>
#include <string.h>
#include <fuse.h>
#include "intern_rfuse.h"

struct intern_fuse *intern_fuse_new() {
  struct intern_fuse *inf;
  inf = (struct intern_fuse *) malloc(sizeof(struct intern_fuse));
  return inf;
};

int intern_fuse_destroy(struct intern_fuse *inf){
  //you have to take care, that fuse is unmounted yourself!
  fuse_destroy(inf->fuse);
  free(inf);
  return 0;
};

int intern_fuse_init(struct intern_fuse *inf,
		     const char *mountpoint, 
		     const char *kernelopts,
		     const char *libopts) {
  int fd;
  fd=fuse_mount(mountpoint,kernelopts);
  if (fd==-1) 
    return -1;
  inf->fuse=fuse_new(fd,libopts,&(inf->fuse_op),sizeof(struct fuse_operations));
  inf->fd=fd;
  //TODO: check length
  strncpy(inf->mountname,mountpoint,MOUNTNAME_MAX);
  return 0;
};
