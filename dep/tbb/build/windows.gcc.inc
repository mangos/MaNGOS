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

#------------------------------------------------------------------------------
# Overriding settings from windows.inc
#------------------------------------------------------------------------------

SLASH= $(strip \)
OBJ = o
LIBEXT = dll # MinGW allows linking with DLLs directly

TBB.RES =
MALLOC.RES =
TBB.MANIFEST =
MALLOC.MANIFEST =

# TODO: do better when/if mingw64 support is added
TBB.DEF = $(tbb_root)/src/tbb/lin32-tbb-export.def
MALLOC.DEF = $(MALLOC_ROOT)/win-gcc-tbbmalloc-export.def

LINK_TBB.LIB = $(TBB.LIB)

#------------------------------------------------------------------------------
# End of overridden settings
#------------------------------------------------------------------------------
# Compiler-specific variables
#------------------------------------------------------------------------------

CPLUS = g++ 
COMPILE_ONLY = -c -MMD
PREPROC_ONLY = -E -x c
INCLUDE_KEY = -I
DEFINE_KEY = -D
OUTPUT_KEY = -o #
OUTPUTOBJ_KEY = -o #
PIC_KEY =
WARNING_AS_ERROR_KEY = -Werror
WARNING_KEY = -Wall  -Wno-uninitialized
WARNING_SUPPRESS = -Wno-parentheses
DYLIB_KEY = -shared
LIBDL = 
EXPORT_KEY = -Wl,--version-script,
LIBS = -lpsapi

#------------------------------------------------------------------------------
# End of compiler-specific variables
#------------------------------------------------------------------------------
# Command lines
#------------------------------------------------------------------------------

LINK_FLAGS = -Wl,--enable-auto-import
LIB_LINK_FLAGS = $(DYLIB_KEY)

ifeq ($(cfg), release)
        CPLUS_FLAGS = -O2
endif
ifeq ($(cfg), debug)
        CPLUS_FLAGS = -g -O0 -DTBB_USE_DEBUG
endif
CPLUS_FLAGS += -DUSE_WINTHREAD

# MinGW specific
CPLUS_FLAGS += -D__MSVCRT_VERSION__=0x0700 -msse -mthreads

CONLY = gcc
C_FLAGS = $(CPLUS_FLAGS)

ifeq (intel64,$(arch))
    CPLUS_FLAGS += -m64
    LIB_LINK_FLAGS += -m64
endif 

ifeq (ia32,$(arch))
    CPLUS_FLAGS += -m32
    LIB_LINK_FLAGS += -m32
endif 

#------------------------------------------------------------------------------
# End of command lines
#------------------------------------------------------------------------------
# Setting assembler data
#------------------------------------------------------------------------------

ASM=
ASM_FLAGS=
TBB_ASM.OBJ=
ASSEMBLY_SOURCE=$(arch)-gas

#------------------------------------------------------------------------------
# End of setting assembler data
#------------------------------------------------------------------------------
# Setting tbbmalloc data
#------------------------------------------------------------------------------

M_CPLUS_FLAGS = $(CPLUS_FLAGS) -fno-rtti -fno-exceptions

#------------------------------------------------------------------------------
# End of setting tbbmalloc data
#------------------------------------------------------------------------------
