#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif
//FOR LINUX ONLY
#include <linux/stat.h> 

#include <ruby.h>
#include <fuse.h>
#include <errno.h>
#include <sys/statfs.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "helper.h"
#include "intern_rfuse.h"
#include "filler.h"
#include "context.h"
#include "file_info.h"

static VALUE fuse_object; //this is a global variable where we store the fuse object

static int unsafe_return_error(VALUE *args){
  VALUE info;
  info = rb_inspect(ruby_errinfo);
  rb_backtrace();
  printf ("ERROR %s\n",STR2CSTR(info));
  return rb_funcall(info,rb_intern("errno"),0);
}
static int return_error(int def_error){
  /*if the raised error has a method errno the return that value else
    return def(ault)_error */
  VALUE res;
  int error = 0;
  res=rb_protect((VALUE (*)())unsafe_return_error,Qnil,&error);
  if (error) { //this exception has no errno method or something else 
               //went wrong
    printf ("ERROR: Not an Errno::xxx error or exception in exception!\n");
    return def_error;
  } else {
    return FIX2INT(res);
  }
}

static VALUE unsafe_readdir(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE filler = values[1];
  VALUE offset = values[2];
  VALUE ffi = values[3];
  struct fuse_context *ctx = fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("readdir"),5,wrap_context(ctx),path,filler,
		    offset,ffi);
}

  //call readdir with an Filler object
static int rf_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		     off_t offset,struct fuse_file_info *ffi)
{
  VALUE fuse_module;
  VALUE rfiller_class;
  VALUE rfiller_instance;
  VALUE args[4];
  VALUE res;
  struct filler_t *fillerc;
  int error = 0;

  //create a filler object
  args[0]=rb_str_new2(path);

  fuse_module = rb_const_get(rb_cObject, rb_intern("RFuse"));
  rfiller_class=rb_const_get(fuse_module,rb_intern("Filler"));
  rfiller_instance=rb_funcall(rfiller_class,rb_intern("new"),0);
  Data_Get_Struct(rfiller_instance,struct filler_t,fillerc);
  fillerc->filler=filler;//Init the filler by hand.... TODO: cleaner
  fillerc->buffer=buf;
  args[1]=rfiller_instance;
  args[2]=INT2NUM(offset);
  args[3]=wrap_file_info(ffi);
  res=rb_protect((VALUE (*)())unsafe_readdir,(VALUE)args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------------------------------

static VALUE unsafe_readlink(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE size = values[1];
  struct fuse_context *ctx = fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("readlink"),3,wrap_context(ctx),path,size);
}

static int rf_readlink(const char *path, char *buf, size_t size)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=INT2NUM(size);
  char *rbuf;
  res=rb_protect((VALUE (*)())unsafe_readlink,(VALUE)args,&error);  
  if (error) {
    return -(return_error(ENOENT));
  } else {
    rbuf=STR2CSTR(res);
    strncpy(buf,rbuf,size);
    return 0;
  }
}
//-----------------------------------------
static VALUE unsafe_getattr(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("getattr"),2,wrap_context(ctx),path);
}

//calls getattr with path and expects something like FuseStat back
static int rf_getattr(const char *path, struct stat *stbuf)
{
  VALUE args[1];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  res=rb_protect((VALUE (*)())unsafe_getattr,(VALUE) args,&error);
  if (error || (res == Qnil)) {
    return -(return_error(ENOENT));
  } else {
    rstat2stat(res,stbuf);
    return 0;
  }
}
//----------------------MKDIR
static VALUE unsafe_mkdir(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE mode = values[1];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("mkdir"),3,wrap_context(ctx),path,mode);
}

//calls getattr with path and expects something like FuseStat back
static int rf_mkdir(const char *path, mode_t mode)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=INT2FIX(mode);
  res=rb_protect((VALUE (*)())unsafe_mkdir,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------

//----------------------MKNOD
static VALUE unsafe_mknod(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE mode = values[1];
  VALUE dev = values[2];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("mknod"),4,wrap_context(ctx),path,mode,dev);
}

//calls getattr with path and expects something like FuseStat back
static int rf_mknod(const char *path, mode_t mode,dev_t dev)
{
  VALUE args[3];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=INT2FIX(mode);
  args[2]=INT2FIX(dev);
  res=rb_protect((VALUE (*)())unsafe_mknod,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------OPEN
static VALUE unsafe_open(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE ffi =  values[1];
 struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("open"),3,wrap_context(ctx),path,ffi);
}

//calls getattr with path and expects something like FuseStat back
static int rf_open(const char *path,struct fuse_file_info *ffi)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=wrap_file_info(ffi);
  res=rb_protect((VALUE (*)())unsafe_open,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------RELEASE
static VALUE unsafe_release(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE ffi = values[1];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("release"),3,wrap_context(ctx),path,ffi);
}

static int rf_release(const char *path, struct fuse_file_info *ffi)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=wrap_file_info(ffi);
  res=rb_protect((VALUE (*)())unsafe_release,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------FLUSH
static VALUE unsafe_flush(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE ffi=values[1];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("flush"),3,wrap_context(ctx),path,ffi);
}

static int rf_flush(const char *path,struct fuse_file_info *ffi)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=wrap_file_info(ffi);
  res=rb_protect((VALUE (*)())unsafe_flush,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------TRUNCATE
static VALUE unsafe_truncate(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE offset = values[1];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("truncate"),3,wrap_context(ctx),path,offset);
}

static int rf_truncate(const char *path,off_t offset)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=INT2FIX(offset);
  res=rb_protect((VALUE (*)())unsafe_truncate,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------UTIME
static VALUE unsafe_utime(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE actime = values[1];
  VALUE modtime = values[2];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("utime"),4,wrap_context(ctx),path,actime,modtime);
}

static int rf_utime(const char *path,struct utimbuf *utim)
{
  VALUE args[3];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=INT2NUM(utim->actime);
  args[2]=INT2NUM(utim->modtime);
  res=rb_protect((VALUE (*)())unsafe_utime,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------CHOWN
static VALUE unsafe_chown(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE uid = values[1];
  VALUE gid = values[2];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("chown"),4,wrap_context(ctx),path,uid,gid);
}

static int rf_chown(const char *path,uid_t uid,gid_t gid)
{
  VALUE args[3];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=INT2FIX(uid);
  args[2]=INT2FIX(gid);
  res=rb_protect((VALUE (*)())unsafe_chown,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------CHMOD
static VALUE unsafe_chmod(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE mode = values[1];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("chmod"),3,wrap_context(ctx),path,mode);
}

static int rf_chmod(const char *path,mode_t mode)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=INT2FIX(mode);
  res=rb_protect((VALUE (*)())unsafe_chmod,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------UNLINK
static VALUE unsafe_unlink(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("unlink"),2,wrap_context(ctx),path);
}

static int rf_unlink(const char *path)
{
  VALUE args[1];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  res=rb_protect((VALUE (*)())unsafe_unlink,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------RMDIR
static VALUE unsafe_rmdir(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("rmdir"),2,wrap_context(ctx),path);
}

static int rf_rmdir(const char *path)
{
  VALUE args[1];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  res=rb_protect((VALUE (*)())unsafe_rmdir,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------SYMLINK
static VALUE unsafe_symlink(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE as = values[1];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("symlink"),3,wrap_context(ctx),path,as);
}

static int rf_symlink(const char *path,const char *as)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=rb_str_new2(as);
  res=rb_protect((VALUE (*)())unsafe_symlink,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------RENAME
static VALUE unsafe_rename(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE as = values[1];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("rename"),3,wrap_context(ctx),path,as);
}

static int rf_rename(const char *path,const char *as)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=rb_str_new2(as);
  res=rb_protect((VALUE (*)())unsafe_rename,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------LINK
static VALUE unsafe_link(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE as = values[1];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("link"),3,wrap_context(ctx),path,as);
}

static int rf_link(const char *path,const char * as)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=rb_str_new2(as);
  res=rb_protect((VALUE (*)())unsafe_link,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------READ
static VALUE unsafe_read(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE size = values[1];
  VALUE offset = values[2];
  VALUE ffi = values[3];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("read"),5,
		    wrap_context(ctx),path,size,offset,ffi);
}

static int rf_read(const char *path,char * buf, size_t size,off_t offset,struct fuse_file_info *ffi)
{
  VALUE args[4];
  VALUE res;
  int error = 0;
  long length=0;
  char* rbuf;
  args[0]=rb_str_new2(path);
  args[1]=INT2NUM(size);
  args[2]=INT2NUM(offset);
  args[3]=wrap_file_info(ffi);
  res=rb_protect((VALUE (*)())unsafe_read,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    rbuf=rb_str2cstr(res,&length); //TODO protect this, too
    if (length<=size) {
      memcpy(buf,rbuf,length);
      return length;
    } else {
      memcpy(buf,rbuf,size); //TODO: This could be dangerous. 
                               //Perhaps raise an exception or return an error
      return size;
    }
  };
}
//----------------------
//----------------------WRITE
static VALUE unsafe_write(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE buffer = values[1];
  VALUE size = values[2];
  VALUE offset = values[3];
  VALUE ffi = values[4];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("write"),6,
		    wrap_context(ctx),path,buffer,size,offset,ffi);
}

static int rf_write(const char *path,const char *buf,size_t size, off_t offset,struct fuse_file_info *ffi)
{
  VALUE args[4];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=rb_str_new2(buf);
  args[2]=INT2NUM(size);
  args[3]=INT2NUM(offset);
  args[4]=wrap_file_info(ffi);
  res=rb_protect((VALUE (*)())unsafe_write,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------SETXATTR
static VALUE unsafe_setxattr(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE name = values[1];
  VALUE value = values[2];
  VALUE size = values[3];
  VALUE flags = values[4];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("setxattr"),6,
		    wrap_context(ctx),path,name,value,size,flags);
}

static int rf_setxattr(const char *path,const char *name,
		       const char *value, size_t size, int flags)
{
  VALUE args[5];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=rb_str_new2(name);
  args[2]=rb_str_new(value,size);
  args[3]=INT2NUM(size); //TODO:FIX would be faster
  args[4]=INT2NUM(flags);
  res=rb_protect((VALUE (*)())unsafe_setxattr,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------GETXATTR
static VALUE unsafe_getxattr(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE name = values[1];
  VALUE size = values[2];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("getxattr"),4,
		    wrap_context(ctx),path,name,size);
}

static int rf_getxattr(const char *path,const char *name,char *buf,
		       size_t size)
{
  VALUE args[3];
  VALUE res;
  char *rbuf;
  long length = 0;
  int error = 0;
  size_t i;
  args[0]=rb_str_new2(path);
  args[1]=rb_str_new2(name);
  args[2]=INT2NUM(size);
  res=rb_protect((VALUE (*)())unsafe_getxattr,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    rbuf=rb_str2cstr(res,&length); //TODO protect this, too
    if (buf != NULL) {
      memcpy(buf,rbuf,length); //First call is just to get the length
      //TODO optimize
    }
    return length;
  }
}
//----------------------
//----------------------LISTXATTR
static VALUE unsafe_listxattr(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE size = values[1];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("listxattr"),3,
		    wrap_context(ctx),path,size);
}

static int rf_listxattr(const char *path,char *buf,
		       size_t size)
{
  VALUE args[2];
  VALUE res;
  char *rbuf;
  size_t length =0;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=INT2NUM(size);
  res=rb_protect((VALUE (*)())unsafe_listxattr,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    rbuf=rb_str2cstr(res,(long *)&length); //TODO protect this, too
    if (buf != NULL){
      if (length<=size) {
	memcpy(buf,rbuf,length); //check for size
      } else {
	return -ERANGE;
      }
      printf("destination: %s,%d\n",buf,size);
      printf("source:      %s,%d\n",rbuf,length);
      return length;
      //TODO optimize,check lenght
    } else {
      printf ("not copied: %s, %d\n",buf,length);
      return length;
    }
  }
}
//----------------------
//----------------------REMOVEXATTR
static VALUE unsafe_removexattr(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE name = values[1];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("removexattr"),3,
		    wrap_context(ctx),path,name);
}

static int rf_removexattr(const char *path,const char *name)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=rb_str_new2(name);
  res=rb_protect((VALUE (*)())unsafe_removexattr,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------OPENDIR
static VALUE unsafe_opendir(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE ffi =  values[1];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("opendir"),3,wrap_context(ctx),path,ffi);
}

static int rf_opendir(const char *path,struct fuse_file_info *ffi)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=wrap_file_info(ffi);
  res=rb_protect((VALUE (*)())unsafe_opendir,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------RELEASEDIR
static VALUE unsafe_releasedir(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE ffi =  values[1];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("releasedir"),3,wrap_context(ctx),path,ffi);
}

static int rf_releasedir(const char *path,struct fuse_file_info *ffi)
{
  VALUE args[2];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=wrap_file_info(ffi);
  res=rb_protect((VALUE (*)())unsafe_releasedir,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------
//----------------------FSYNCDIR
static VALUE unsafe_fsyncdir(VALUE *args){
  VALUE *values=(VALUE*)args;
  VALUE path = values[0];
  VALUE meta = values[1];
  VALUE ffi =  values[2];
  struct fuse_context *ctx=fuse_get_context();
  return rb_funcall(fuse_object,rb_intern("fsyncdir"),3,wrap_context(ctx),path,
		    meta,ffi);
}

static int rf_fsyncdir(const char *path,int meta,struct fuse_file_info *ffi)
{
  VALUE args[3];
  VALUE res;
  int error = 0;
  args[0]=rb_str_new2(path);
  args[1]=INT2NUM(meta);
  args[2]=wrap_file_info(ffi);
  res=rb_protect((VALUE (*)())unsafe_fsyncdir,(VALUE) args,&error);
  if (error) {
    return -(return_error(ENOENT));
  } else {
    return 0;
  };
}
//----------------------

static VALUE rf_loop(VALUE self){
  struct intern_fuse *inf;
  Data_Get_Struct(self,struct intern_fuse,inf);
  fuse_loop(inf->fuse);
  return Qnil;
}
static VALUE rf_loop_mt(VALUE self){
  struct intern_fuse *inf;
  Data_Get_Struct(self,struct intern_fuse,inf);
  fuse_loop_mt(inf->fuse);
  return Qnil;
}

VALUE rf_exit(VALUE self){
  struct intern_fuse *inf;
  Data_Get_Struct(self,struct intern_fuse,inf);
  fuse_exit(inf->fuse);
  return Qnil;
}

VALUE rf_unmount(VALUE self){
  struct intern_fuse *inf;
  Data_Get_Struct(self,struct intern_fuse,inf);
  fuse_unmount(inf->mountname);
  return Qnil;
}

VALUE rf_mountname(VALUE self){
  struct intern_fuse *inf;
  Data_Get_Struct(self,struct intern_fuse,inf);
  return rb_str_new2(inf->mountname);
}
VALUE rf_invalidate(VALUE self,VALUE path){
  struct intern_fuse *inf;
  Data_Get_Struct(self,struct intern_fuse,inf);
  return fuse_invalidate(inf->fuse,STR2CSTR(path)); //TODO: check if str?
}
//-------------RUBY


static VALUE rf_initialize(VALUE self,VALUE mountpoint,VALUE kernelopts,
			   VALUE libopts) {
  struct intern_fuse *inf;
  Data_Get_Struct(self,struct intern_fuse,inf);
  inf->fuse_op.getattr=rf_getattr;
  //  inf->fuse_op.getdir=rf_getdir;
  inf->fuse_op.readlink=rf_readlink;
  inf->fuse_op.mkdir=rf_mkdir;
  inf->fuse_op.mknod=rf_mknod;
  inf->fuse_op.open=rf_open;
  inf->fuse_op.release=rf_release;  //optional
  inf->fuse_op.flush=rf_flush;      //optional
  inf->fuse_op.chmod=rf_chmod;      //setattr
  inf->fuse_op.chown=rf_chown;      //setattr
  inf->fuse_op.truncate=rf_truncate;//setattr
  inf->fuse_op.utime=rf_utime;      //settattr
  inf->fuse_op.unlink=rf_unlink;
  inf->fuse_op.rmdir=rf_rmdir;
  inf->fuse_op.symlink=rf_symlink;
  inf->fuse_op.rename=rf_rename;
  inf->fuse_op.link=rf_link;
  inf->fuse_op.read=rf_read;
  inf->fuse_op.write=rf_write;
  inf->fuse_op.setxattr=rf_setxattr;
  inf->fuse_op.getxattr=rf_getxattr;
  inf->fuse_op.listxattr=rf_listxattr;
  inf->fuse_op.removexattr=rf_removexattr;
  inf->fuse_op.readdir=rf_readdir;
  inf->fuse_op.opendir=rf_opendir;
  inf->fuse_op.releasedir=rf_releasedir;
  inf->fuse_op.fsyncdir=rf_fsyncdir;

  /* TODO
  inf->fuse_op.statfs=rf_statfs;
  inf->fuse_op.fsnyc=rf_fsync; //option
  */
  
  intern_fuse_init(inf,STR2CSTR(mountpoint),STR2CSTR(kernelopts),
		   STR2CSTR(libopts));
  fuse_object=self;  // this won't work with multithreading!!!
  return self;
}

static VALUE rf_new(VALUE class){
  struct intern_fuse *inf;
  VALUE self;
  inf = intern_fuse_new();
  self=Data_Wrap_Struct(class, 0,intern_fuse_destroy,inf);
  return self;
}

VALUE rfuse_init(VALUE module){
  VALUE cFuse=rb_define_class_under(module,"Fuse",rb_cObject);
  rb_define_alloc_func(cFuse,rf_new);
  //initialize: string mountpoint,string kernel_opts,string lib_opts
  rb_define_method(cFuse,"initialize",rf_initialize,3);
  rb_define_method(cFuse,"loop",rf_loop,0);
  //rb_define_method(cFuse,"loop_mt",rf_loop_mt,0); TODO: not until RIKE!
  rb_define_method(cFuse,"exit",rf_exit,0);
  rb_define_method(cFuse,"invalidate",rf_invalidate,1);
  rb_define_method(cFuse,"unmount",rf_unmount,0);
  rb_define_method(cFuse,"mountname",rf_mountname,0);
  return cFuse;
}
