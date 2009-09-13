#include "file_info.h"
#include <fuse.h>


VALUE wrap_file_info(struct fuse_file_info *ffi) {
  VALUE rRFuse;
  VALUE rFileInfo;
  rRFuse=rb_const_get(rb_cObject,rb_intern("RFuse"));
  rFileInfo=rb_const_get(rRFuse,rb_intern("FileInfo"));
  return Data_Wrap_Struct(rFileInfo,0,0,ffi); //shouldn't be freed!

};


VALUE file_info_initialize(VALUE self){
  return self;
}

VALUE file_info_new(VALUE class){
  VALUE self;
  struct fuse_file_info *f;
  self = Data_Make_Struct(class, struct fuse_file_info, 0,NULL,f);
  return self;
}

VALUE file_info_writepage(VALUE self) {
  struct fuse_file_info *f;
  Data_Get_Struct(self,struct fuse_file_info,f);
  return INT2FIX(f->writepage);
}

VALUE file_info_flags(VALUE self) {
  struct fuse_file_info *f;
  Data_Get_Struct(self,struct fuse_file_info,f);
  return INT2FIX(f->flags);
}

VALUE file_info_init(VALUE module) {
  VALUE cFileInfo=rb_define_class_under(module,"FileInfo",rb_cObject);
  rb_define_alloc_func(cFileInfo,file_info_new);
  rb_define_method(cFileInfo,"initialize",file_info_initialize,0);
  rb_define_method(cFileInfo,"flags",file_info_flags,0);
  rb_define_method(cFileInfo,"writepage",file_info_writepage,0);
  return cFileInfo;
}
