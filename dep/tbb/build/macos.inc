# Copyright 2005-2009 Intel Corporation.  All Rights Reserved.
#
# This file is part of Threading Building Blocks.
#
# Threading Building Blocks is free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# Threading Building Blocks is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Threading Building Blocks; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As a special exception, you may use this file as part of a free software
# library without restriction.  Specifically, if other files instantiate
# templates or use macros or inline functions from this file, or you compile
# this file and link it with other files to produce an executable, this
# file does not by itself cause the resulting executable to be covered by
# the GNU General Public License.  This exception does not however
# invalidate any other reasons why the executable file might be covered by
# the GNU General Public License.

####### Detections and Commands ###############################################
ifndef arch
 ifeq ($(shell /usr/sbin/sysctl -n hw.machine),Power Macintosh)
   ifeq ($(shell /usr/sbin/sysctl -n hw.optional.64bitops),1)
     export arch:=ppc64
   else
     export arch:=ppc32
   endif
 else
   ifeq ($(shell /usr/sbin/sysctl -n hw.optional.x86_64 2>/dev/null),1)
     export arch:=intel64
   else
     export arch:=ia32
   endif
 endif
endif

ifndef runtime
 #gcc_version:=$(shell gcc -v 2>&1 | grep 'gcc version' | sed -e 's/^gcc version //' | sed -e 's/ .*$$//' )
 gcc_version_full=$(shell gcc --version | grep 'gcc'| egrep -o ' [0-9]+\.[0-9]+\.[0-9]+.*' | sed -e 's/^\ //')
 gcc_version=$(shell echo "$(gcc_version_full)" | egrep -o '^[0-9]+\.[0-9]+\.[0-9]+\s*' | head -n 1 | sed -e 's/ *//g')
 os_version:=$(shell /usr/bin/sw_vers -productVersion)
 export runtime:=cc$(gcc_version)_os$(os_version)
endif

native_compiler := gcc
export compiler ?= gcc
debugger ?= gdb

CMD=$(SHELL) -c
CWD=$(shell pwd)
RM?=rm -f
RD?=rmdir
MD?=mkdir -p
NUL= /dev/null
SLASH=/
MAKE_VERSIONS=sh $(tbb_root)/build/version_info_macos.sh $(CPLUS) $(CPLUS_FLAGS) $(INCLUDES) >version_string.tmp
MAKE_TBBVARS=sh $(tbb_root)/build/generate_tbbvars.sh

####### Build settings ########################################################

OBJ=o
DLL=dylib
LIBEXT=dylib

def_prefix = $(if $(findstring 32,$(arch)),mac32,mac64)

TBB.DEF = $(tbb_root)/src/tbb/$(def_prefix)-tbb-export.def
TBB.DLL = libtbb$(DEBUG_SUFFIX).$(DLL)
TBB.LIB = $(TBB.DLL)
LINK_TBB.LIB = $(TBB.LIB)

MALLOC.DEF = $(MALLOC_ROOT)/$(def_prefix)-tbbmalloc-export.def
MALLOC.DLL = libtbbmalloc$(DEBUG_SUFFIX).$(DLL)
MALLOC.LIB = $(MALLOC.DLL)

TBB_NOSTRICT=1

TEST_LAUNCHER=sh $(tbb_root)/build/test_launcher.sh
