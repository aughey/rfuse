
#include <fuse.h>

#define MOUNTNAME_MAX 1024
struct intern_fuse {
  int fd;
  struct fuse *fuse;
  struct fuse_operations fuse_op;
  struct fuse_context *fuse_ctx;
  char   mountname[MOUNTNAME_MAX];
  int state; //created,mounted,running
};

struct intern_fuse *intern_fuse_new();

int intern_fuse_init(struct intern_fuse *inf,
		     const char *mountpoint, 
		     const char *kernelopts,
		     const char *libopts);

int intern_fuse_destroy(struct intern_fuse *inf);
