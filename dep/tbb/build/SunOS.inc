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

ifndef arch
        arch:=$(shell uname -p)
        ifeq ($(arch),i386)
            ifeq ($(shell isainfo -b),64)
                arch:=intel64
            else
                arch:=ia32
            endif
        endif
        export arch
# For non-IA systems running Sun OS, 'arch' will contain whatever is printed by uname -p.
# In particular, for SPARC architecture it will contain "sparc".
endif

ifndef runtime
        gcc_version:=$(shell gcc -v 2>&1 | grep 'gcc version' | sed -e 's/^gcc version //' | sed -e 's/ .*$$//')
        os_version:=$(shell uname -r)
        os_kernel_version:=$(shell uname -r | sed -e 's/-.*$$//')
        export runtime:=cc$(gcc_version)_kernel$(os_kernel_version)
endif

native_compiler := suncc
export compiler ?= suncc
# debugger ?= gdb

CMD=$(SHELL) -c
CWD=$(shell pwd)
RM?=rm -f
RD?=rmdir
MD?=mkdir -p
NUL= /dev/null
SLASH=/
MAKE_VERSIONS=bash $(tbb_root)/build/version_info_sunos.sh $(CPLUS) $(CPLUS_FLAGS) $(INCLUDES) >version_string.tmp
MAKE_TBBVARS=bash $(tbb_root)/build/generate_tbbvars.sh

ifeq ($(compiler),suncc)
        export TBB_CUSTOM_VARS_SH=CXXFLAGS="-I$(CWD)/../include -library=stlport4 $(CXXFLAGS) -M$(CWD)/../build/suncc.map.pause"
        export TBB_CUSTOM_VARS_CSH=CXXFLAGS "-I$(CWD)/../include -library=stlport4 $(CXXFLAGS) -M$(CWD)/../build/suncc.map.pause"
endif
	
ifdef LD_LIBRARY_PATH
        export LD_LIBRARY_PATH := .:$(LD_LIBRARY_PATH)
else
        export LD_LIBRARY_PATH := .
endif

####### Build settings ########################################################

OBJ = o
DLL = so

TBB.DEF = 
TBB.DLL = libtbb$(DEBUG_SUFFIX).$(DLL)
TBB.LIB = $(TBB.DLL)
LINK_TBB.LIB = $(TBB.LIB)

MALLOC.DLL = libtbbmalloc$(DEBUG_SUFFIX).$(DLL)
MALLOC.LIB = $(MALLOC.DLL)

MALLOCPROXY.DLL = libtbbmalloc_proxy$(DEBUG_SUFFIX).$(DLL)

TBB_NOSTRICT=1

TEST_LAUNCHER=sh $(tbb_root)/build/test_launcher.sh
