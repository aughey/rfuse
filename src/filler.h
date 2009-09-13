#include <fuse.h>
#include <ruby.h>

struct filler_t {
  fuse_fill_dir_t filler;
  void           *buffer;
};

VALUE rfiller_initialize(VALUE self);
VALUE rfiller_new(VALUE class);
VALUE rfiller_push(VALUE self,VALUE name, VALUE stat,VALUE offset);

VALUE rfiller_init(VALUE module);
