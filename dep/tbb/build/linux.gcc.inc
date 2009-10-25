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

COMPILE_ONLY = -c -MMD
PREPROC_ONLY = -E -x c
INCLUDE_KEY = -I
DEFINE_KEY = -D
OUTPUT_KEY = -o #
OUTPUTOBJ_KEY = -o #
PIC_KEY = -fPIC
WARNING_AS_ERROR_KEY = -Werror
WARNING_KEY = -Wall
WARNING_SUPPRESS = -Wno-parentheses
RML_WARNING_SUPPRESS = -Wno-non-virtual-dtor
DYLIB_KEY = -shared
LIBDL = -ldl

TBB_NOSTRICT = 1

CPLUS = g++ 
CONLY = gcc
LIB_LINK_FLAGS = -shared -Wl,-soname=$(BUILDING_LIBRARY)
LIBS = -lpthread -lrt 
C_FLAGS = $(CPLUS_FLAGS)

ifeq ($(cfg), release)
        CPLUS_FLAGS = -DDO_ITT_NOTIFY -O2 -DUSE_PTHREAD
endif
ifeq ($(cfg), debug)
        CPLUS_FLAGS = -DTBB_USE_DEBUG -DDO_ITT_NOTIFY -g -O0 -DUSE_PTHREAD
endif

ifneq (0,$(cpp0x))
    CXX_ONLY_FLAGS = -std=c++0x
endif

ASM=
ASM_FLAGS=

TBB_ASM.OBJ=

ifeq (ia64,$(arch))
# Position-independent code (PIC) is a must on IA-64, even for regular (not shared) executables
    CPLUS_FLAGS += $(PIC_KEY)
endif 

ifeq (intel64,$(arch))
    CPLUS_FLAGS += -m64
    LIB_LINK_FLAGS += -m64
endif 

ifeq (ia32,$(arch))
    CPLUS_FLAGS += -m32
    LIB_LINK_FLAGS += -m32
endif 

# for some gcc versions on Solaris, -m64 may imply V9, but perhaps not everywhere (TODO: verify)
ifeq (sparc,$(arch))
    CPLUS_FLAGS    += -mcpu=v9 -m64
    LIB_LINK_FLAGS += -mcpu=v9 -m64
endif 

#------------------------------------------------------------------------------
# Setting assembler data.
#------------------------------------------------------------------------------
ASSEMBLY_SOURCE=$(arch)-gas
ifeq (ia64,$(arch))
    ASM=as
    ASM_FLAGS += -xexplicit
    TBB_ASM.OBJ = atomic_support.o lock_byte.o log2.o pause.o ia64_misc.o
endif 
#------------------------------------------------------------------------------
# End of setting assembler data.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Setting tbbmalloc data.
#------------------------------------------------------------------------------

M_CPLUS_FLAGS = $(CPLUS_FLAGS) -fno-rtti -fno-exceptions -fno-schedule-insns2

#------------------------------------------------------------------------------
# End of setting tbbmalloc data.
#------------------------------------------------------------------------------
