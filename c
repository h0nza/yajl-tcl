#!/bin/sh
make clean

: ${TCL_VERSION:=8.2} #je static!?
# yalj.pc ne/ma Libs.private pro libyajl_s.a static library? 

: ${prefix:=$HOME/$DESTIMG}

#TODO: 2011-05-17 yajl 2.0.3 have pkg-config file!

: ${PKG_CONFIG_PATH:=$prefix/share/pkgconfig} ; export PKG_CONFIG_PATH
: ${yajldir:=$prefix}

#pkg-config yajl --libs ; exit

test -x ./configure || { autoconf ; }
find . -newer configure -name configure.in -exec autoconf  \;
find . -newer configure -name aclocal.m4   -exec autoconf  \;

shared=" --enable-shared --disable-static"
debug=" --enable-symbols"
./configure --prefix=${prefix} --with-tcl=${prefix}/lib/tcl$TCL_VERSION $shared $debug

#make YAJLINCLUDES="`pkg-config yajl --cflags-only-I`" "YAJLLIBS=`pkg-config yajl --libs`"
 make

#make -C ./tests

 TCLSH=tclsh$TCL_VERSION
 TCLSH=$HOME/cgish8_x/bin/tclsh$TCL_VERSION
#TCLSH=$HOME/cgish8_x/bin/ccgish$TCL_VERSION.10
 TCLSH=$HOME/cgish1.2.9/bin/ccgish1.2.9
echo 'foreach cmd {
{package require Tcl       }
{info patchlevel           }
{package require yajltcl   }
{package require dict      }
{package require yajltcl   }
{set yajl [yajl create #auto]}
{$yajl x ;#init cache        }
{yajl::json2dict           }
{yajl::json2dict null      }
{yajl::json2dict {{"a":1}} }
} { set rc [catch $cmd result];puts $cmd:$rc:$result }' | TCLLIBPATH=. $TCLSH

#make test
