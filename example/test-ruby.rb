#!/usr/bin/ruby

$:.unshift '../src'
require("rfuse")

class MyDir < Hash
  attr_accessor :name, :mode , :actime, :modtime, :uid, :gid
  def initialize(name,mode)
    @uid=0
    @gid=0
    @actime=0     #of couse you should use now() here!
    @modtime=0    # -''-
    @xattr=Hash.new
    @name=name
    @mode=mode | (4 << 12) #yes! we have to do this by hand
  end
  def listxattr()
    @xattr.each {|key,value| list=list+key+"\0"}
  end
  def setxattr(name,value,flag)
    @xattr[name]=value #TODO:don't ignore flag
  end
  def getxattr(name)
    return @xattr[name]
  end
  def removexattr(name)
    @xattr.delete(name)
  end
  def dir_mode
    return (@mode & 170000)>> 12 #see dirent.h
  end
  def size
    return 48 #for testing only
  end
  def isdir
    true
  end
  def insert_obj(obj,path)
    d=self.search(File.dirname(path))
    if d.isdir then
      d[obj.name]=obj
    else
      raise Errno::ENOTDIR.new(d.name)
    end
    return d
  end
  def search(path)
    puts "searching: " + path
    p=path.split('/').delete_if {|x| x==''}
    if p.length==0 then
      puts "found root"
      return self
    else
      return self.follow(p)
    end
  end
  def follow (path_array)
    puts "following: " + path_array.to_s
    if path_array.length==0 then
      puts "found me!" + @name
      return self
    else
      d=self[path_array.shift]
      if d then
	return d.follow(path_array)
      else
	raise Errno::ENOENT.new
      end
    end
  end
  def to_s
    return "Dir: " + @name + "(" + @mode.to_s + ")"
  end
end

class MyFile
  attr_accessor :name, :mode, :actime, :modtime, :uid, :gid, :content
  def initialize(name,mode,uid,gid)
    @actime=0
    @modtime=0
    @xattr=Hash.new
    @content=""
    @uid=uid
    @gid=gid
    @name=name
    @mode=mode
  end
  def listxattr() #hey this is a raw interface you have to care about the \0
    list=""
    @xattr.each {|key,value| 
      list=list+key+"\0"}
    return list
  end
  def setxattr(name,value,flag)
    @xattr[name]=value #TODO:don't ignore flag
  end
  def getxattr(name)
    return @xattr[name]
  end
  def removexattr(name)
    @xattr.delete(name)
  end
  def size
    return content.size
  end 
  def dir_mode
    return (@mode & 170000) >> 12
  end
  def isdir
    false
  end
  def follow(path_array)
    if path_array.length != 0 then
      raise Errno::ENOTDIR.new
    else
      return self
    end
  end
  def to_s
    return "File: " + @name + "(" + @mode.to_s + ")"
  end
end

#TODO: atime,mtime,ctime...nicer classes not only fixnums
  class Stat
    attr_accessor :uid,:gid,:mode,:size,:atime,:mtime,:ctime 
    attr_accessor :dev,:ino,:nlink,:rdev,:blksize,:blocks
    def initialize(uid,gid,mode,size,atime,mtime,ctime,rdev,blocks,nlink,dev,ino,blksize)
      @uid=uid
      @gid=gid
      @mode=mode
      @size=size
      @atime=atime
      @mtime=mtime
      @ctime=ctime
      @dev=dev
      @ino=ino
      @nlink=nlink
      @rdev=rdev
      @blksize=blksize
      @blocks=blocks
    end
  end #class Stat

  module RFuse
    class Context
      def to_s
	'uid:' + uid.to_s + ' gid:' + gid.to_s + ' pid:' + pid.to_s
      end
    end
    class FileInfo
      def to_s
	'File_Info:---' + flags.to_s #+ ' writepage:' + writepage.to_s
      end
    end
  end
  class MyFuse < RFuse::Fuse
    def initialize(mnt,kernelopt,libopt,root)
      super(mnt,kernelopt,libopt)
      @root=root
    end
    def readdir(ctx,path,filler,offset,ffi)
      puts "readdir:"+path
      puts ctx
      d=@root.search(path)
      if d.isdir then
	puts "getdir: listing directory"
	d.each {|name,obj| 
	  stat=Stat.new(obj.uid,obj.gid,obj.mode,obj.size,obj.actime,obj.modtime,
		    0,0,0,0,0,0,0)
	  filler.push(name,stat,0)
	}
      else
	raise Errno::ENOTDIR.new(path)
      end
    end

    def getattr(ctx,path)
      puts "getattr:" + path
      puts ctx
      d=@root.search(path)
      stat=Stat.new(d.uid,d.gid,d.mode,d.size,d.actime,d.modtime,
		    0,0,0,0,0,0,0)
      puts d
      return stat
    end #getattr

    def mkdir(ctx,path,mode)
      puts "mkdir:" + path + " Mode:" + mode.to_s
      puts ctx
      @root.insert_obj(MyDir.new(File.basename(path),mode),path)
    end #mkdir

    def mknod(ctx,path,mode,dev)
      puts "mknod:" + path + " Mode:" + mode.to_s + " Device:" + dev.to_s
      puts ctx
      @root.insert_obj(MyFile.new(File.basename(path),mode,ctx.uid,ctx.gid),path)
    end #mknod
    def open(ctx,path,ffi)
      puts "open:" + path
      puts ctx
#      puts fi
    end
    def release(ctx,path,fi)
      puts "release:" + path 
      puts ctx
#      puts fi
    end
    def flush(ctx,path,fi)
      puts "flush:" + path 
      puts ctx
#      puts fi
    end
    def chmod(ctx,path,mode)
      puts "chmod:" + path + " Mode:" + mode.to_s
      puts ctx
      d=@root.search(path)
      d.mode=mode #TODO: check if this is ok for dir
      #raise Errno::EPERM.new(path)
    end
    def chown(ctx,path,uid,gid)
      puts "chown:" + path + " UID:" + uid.to_s + " GID:" + gid.to_s
      puts ctx
      d=@root.search(path)
      d.uid=uid
      d.gid=gid
    end
    def truncate(ctx,path,offset)
      puts "truncate:" + path + " offset: " + offset.to_s
      puts ctx
    end
    def utime(ctx,path,actime,modtime)
      puts "utime:" + path + " actime:" + actime.to_s + 
	" modtime:" + modtime.to_s
      puts ctx
      d=@root.search(path)
      d.actime=actime
      d.modtime=modtime
    end
    def unlink(ctx,path)
      puts "utime:" + path
      puts ctx
    end
    def rmdir(ctx,path)
      puts "rmdir:" + path
      puts ctx
    end
    def symlink(ctx,path,as)
      puts "symlink:" + path + " as:" + as
      puts ctx
    end
    def rename(ctx,path,as)
      puts "rename:" + path + " as:" + as
      puts ctx
    end
    def link(ctx,path,as)
      puts "link:" + path + " as:" + as
      puts ctx
    end
    def read(ctx,path,size,offset,fi)
      puts "read:" + path + " size:" + size.to_s + " offset:" + offset.to_s
      puts ctx
      d=@root.search(path)
      if (d.isdir) 
	raise Errno::EISDIR.new(path)
	return nil
      else
	return d.content
      end
    end
    def write(ctx,path,buf,size,offset,fi)
      puts "write:" + path + " size:" + size.to_s + " offset:" + offset.to_s
      puts ctx
      puts "content:" + buf
      d=@root.search(path)
      if (d.isdir) 
	  raise Errno::EISDIR.new(path)
      else
	d.content=buf
      end
      return nil
    end
    def setxattr(ctx,path,name,value,size,flags)
      puts "setxattr:" + path + " name:" + name + 
	" value:" + value.inspect + " size:" + size.to_s + " flags:" + flags.to_s +
	" rubysize:" + value.size.to_s
      puts ctx
      d=@root.search(path)
      d.setxattr(name,value,flags)
    end
    def getxattr(ctx,path,name,size)
      puts "getxattr:" + path + " name:" + name + 
	" size:" + size.to_s
      puts ctx
      d=@root.search(path)
      if (d) 
	puts "found:" + d.name
        value=d.getxattr(name)
	if (value)  
	    puts "return: "+value.to_s + " size:"+value.size.to_s
	else 
	  value=""
	  #raise Errno::ENOENT.new #TODO raise the correct error :
	  #NOATTR which is not implemented in Linux/glibc
	end
      else
	raise Errno::ENOENT.new #TODO put this into DIR and FILE?
      end
      return value
    end
    def listxattr(ctx,path,size)
      puts "listxattr:" + path + " size:" + size.to_s
      puts ctx
      d=@root.search(path)
      value= d.listxattr()
      puts "listxattr return: "+ value
      return value
    end
    def removexattr(ctx,path,name)
      puts "removexattr:" + path + " name:" + name
      puts ctx
      d=@root.search(path)
      d.removexattr(name)
    end
    def opendir(ctx,path,ffi)
      puts 'opendir:'+ path
    end
    def releasedir(ctx,path,ffi)
      puts 'releasedir:'+ path
    end
    def fsyncdir(ctx,path,meta,ffi)
      puts 'fsyncdir:'+ path
    end
  end #class Fuse

fo=MyFuse.new("/tmp/fuse","allow_other","debug",MyDir.new("",493));
#kernel:  default_permissions,allow_other,kernel_cache,large_read,direct_io
#         max_read=N,fsname=NAME
#library: debug,hard_remove

Signal.trap("TERM") do
  fo.exit
  fo.unmount
end

begin
  fo.loop
rescue
  f=File.new("/tmp/error","w+")
  f.puts "Error:" + $!
  f.close
end


