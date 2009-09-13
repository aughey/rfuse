#include "helper.h"

void rstat2stat(VALUE rstat, struct stat *statbuf){
    statbuf->st_dev=FIX2ULONG(rb_funcall(rstat,rb_intern("dev"),0));
    statbuf->st_ino=FIX2ULONG(rb_funcall(rstat,rb_intern("ino"),0));
    statbuf->st_mode=FIX2UINT(rb_funcall(rstat,rb_intern("mode"),0));
    statbuf->st_nlink=FIX2UINT(rb_funcall(rstat,rb_intern("nlink"),0));
    statbuf->st_uid=FIX2UINT(rb_funcall(rstat,rb_intern("uid"),0));
    statbuf->st_gid=FIX2UINT(rb_funcall(rstat,rb_intern("gid"),0));
    statbuf->st_rdev=FIX2ULONG(rb_funcall(rstat,rb_intern("rdev"),0));
    statbuf->st_size=FIX2ULONG(rb_funcall(rstat,rb_intern("size"),0));
    statbuf->st_blksize=NUM2ULONG(rb_funcall(rstat,rb_intern("blksize"),0));
    statbuf->st_blocks=NUM2ULONG(rb_funcall(rstat,rb_intern("blocks"),0));
    statbuf->st_atime=NUM2ULONG(rb_funcall(rb_funcall(rstat,
			rb_intern("atime"),0),rb_intern("to_i"),0));
    statbuf->st_mtime=NUM2ULONG(rb_funcall(rb_funcall(rstat,
                        rb_intern("mtime"),0),rb_intern("to_i"),0));
    statbuf->st_ctime=NUM2ULONG(rb_funcall(rb_funcall(rstat,
                        rb_intern("ctime"),0),rb_intern("to_i"),0));
};
