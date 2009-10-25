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

ifndef XN_VER
export LRBSDK = $(LARRABEE_CORE_LATEST)
export LRB_LIB_DIR = "$(LRBSDK)lib"
export LRB_INC_DIR = "$(LRBSDK)include"

# Function $(wildcard pattern) does not work with paths containing spaces!
_lrb_lib = $(shell cmd /C "dir /B "$(LRBSDK)lib\libxn*_lrb64d.so")
export XN_VER = $(patsubst libxn%_lrb64d.so,%,$(_lrb_lib))

ifeq (1,$(NETSIM_LRB_32_OVERRIDE))
    export LRB_HOST_ARCH = 32
else
    export LRB_HOST_ARCH = 64
endif

export run_cmd = harness_lrb_host.exe

export UI = con

endif #XN_VER

include $(tbb_root)/build/windows.inc

ifneq (1,$(netsim))
# Target environment is native LRB or LrbFSim

export compiler = icc
export arch := lrb

target_machine = $(subst -,_,$(shell icpc -dumpmachine))
runtime = $(subst _lrb_,_,$(target_machine))
# -dumpmachine option does not work in R9 Core SDK 5
ifeq ($(runtime),)
    runtime = x86_64_freebsd
endif
export runtime:=$(runtime)_xn$(XN_VER)

OBJ = o
DLL = so
LIBEXT = so

TBB.DEF =
TBB.DLL = libtbb$(DEBUG_SUFFIX).$(DLL)
TBB.LIB = $(TBB.DLL)
LINK_TBB.LIB = $(TBB.DLL)
TBB.RES =

MALLOC.DEF :=
MALLOC.DLL = libtbbmalloc$(DEBUG_SUFFIX).$(DLL)
MALLOC.LIB = $(MALLOC.DLL)
MALLOC.RES = 

MAKE_VERSIONS = cmd /C cscript /nologo /E:jscript $(subst \,/,$(tbb_root))/build/version_info_winlrb.js $(compiler) $(arch) $(subst \,/,"$(CPLUS) $(CPLUS_FLAGS) $(INCLUDES)") > version_string.tmp
MAKE_TBBVARS  = cmd /C "$(subst /,\,$(tbb_root))\build\generate_tbbvars.bat"

ifneq (1,$(XENSIM_ENABLED))
    export run_cmd = rem
endif

TBB_NOSTRICT = 1

endif # lrbfsim
