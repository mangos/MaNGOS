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
WARNING_KEY = -w1
DYLIB_KEY = -shared
LIBDL = -ldl
export COMPILER_VERSION := ICC: $(shell icc -V </dev/null 2>&1 | grep 'Version')
#TODO: autodetection of arch from COMPILER_VERSION!!

TBB_NOSTRICT = 1

CPLUS = icpc 
CONLY = icc

ifeq (release,$(cfg))
CPLUS_FLAGS = -O2 -strict_ansi -DUSE_PTHREAD
else
CPLUS_FLAGS = -O0 -g -strict_ansi -DUSE_PTHREAD -DTBB_USE_DEBUG
endif

ifneq (,$(codecov))
    CPLUS_FLAGS += -prof-genx
else
    CPLUS_FLAGS += -DDO_ITT_NOTIFY
endif

OPENMP_FLAG = -openmp
LIB_LINK_FLAGS = -shared -i-static -Wl,-soname=$(BUILDING_LIBRARY)
LIBS = -lpthread -lrt 
C_FLAGS = $(CPLUS_FLAGS)

ASM=
ASM_FLAGS=

TBB_ASM.OBJ=

ifeq (ia64,$(arch))
# Position-independent code (PIC) is a must on IA-64, even for regular (not shared) executables
    CPLUS_FLAGS += $(PIC_KEY)
endif 

ifneq (00,$(lambdas)$(cpp0x))
	CPLUS_FLAGS += -std=c++0x -D_TBB_CPP0X
endif

#------------------------------------------------------------------------------
# Setting assembler data.
#------------------------------------------------------------------------------
ASSEMBLY_SOURCE=$(arch)-gas
ifeq (ia64,$(arch))
    ASM=ias
    TBB_ASM.OBJ = atomic_support.o lock_byte.o log2.o pause.o ia64_misc.o
endif 
#------------------------------------------------------------------------------
# End of setting assembler data.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Setting tbbmalloc data.
#------------------------------------------------------------------------------

M_CPLUS_FLAGS = $(CPLUS_FLAGS) -fno-rtti -fno-exceptions

#------------------------------------------------------------------------------
# End of setting tbbmalloc data.
#------------------------------------------------------------------------------

