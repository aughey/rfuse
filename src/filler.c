#include "filler.h"
#include <fuse.h>
#include "helper.h"

VALUE rfiller_initialize(VALUE self){
  return self;
}


VALUE rfiller_new(VALUE class){
  VALUE self;
  struct filler_t *f;
  self = Data_Make_Struct(class, struct filler_t, 0,free,f);
  return self;
}

VALUE rfiller_push(VALUE self,VALUE name, VALUE stat,VALUE offset) {
  struct filler_t *f;
  Data_Get_Struct(self,struct filler_t,f);
  struct stat st;
  memset(&st, 0, sizeof(st));
  rstat2stat(stat,&st);
  f->filler(f->buffer,STR2CSTR(name),&st,NUM2LONG(offset));
  return self;
}

VALUE rfiller_init(VALUE module) {
  VALUE cFiller=rb_define_class_under(module,"Filler",rb_cObject);
  rb_define_alloc_func(cFiller,rfiller_new);
  rb_define_method(cFiller,"initialize",rfiller_initialize,0);
  rb_define_method(cFiller,"push",rfiller_push,3);
  return cFiller;
}
