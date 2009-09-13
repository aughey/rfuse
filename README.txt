rfuse -- ruby - FUSE interface

WARNINGS:
=========
Beware, this is beta software (to be honest these are my first steps in C) and is minimally tested. So don't blame me if your filesystem gets corrupted.

LICENSE:
========
see COPYING in /docs.

GENERAL:
========
This is a Ruby interface to FUSE.
FUSE (Filessystem in USErspace) is a simple interface for userspace programs to export a virtual filesaystem to the linux kernel. FUSE aims to provide a secure method for non privileged users to create and mount their own filesystem implementations.

DEPENDENCIES:
=============
ruby 1.8
fuse 2.3

USAGE:
======
Create a class by extending/deriving from RFuse::Fuse.
Implement the operations you need. Every operation is passed a context and the parameters you know from fuse.h. You should look into the /example directory.

LIMITATIONS:
============
Currently the signalhandler is just called if a ruby callback is run.
So you'll have to do something like
killall /usr/bin/ruby; ls /myfilesystem
to shut down the filesystem.

Due to ruby's lack of native threading the multithreading loop_mt is disabled.

TODO:
=====
- find some Makefile/Autoconf GURU
- improve error handling
- better docs
- more examples
- multithreading won't work until RIKE is out (the implementation of ruby 2.0 with true native thread support)
- correct signal handling (possibly with RIKE)

Have fun!

Peter


Maintainer:       mailto: peter.schrammel AT gmx.de
                  jabber: popel AT jabber.ccc.de

Project homepage: http://rfuse.rubyforge.org
