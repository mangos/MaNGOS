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

CPLUS = g++
CONLY = gcc
COMPILE_ONLY = -c -MMD
PREPROC_ONLY = -E -x c
INCLUDE_KEY = -I
DEFINE_KEY = -D
OUTPUT_KEY = -o #
OUTPUTOBJ_KEY = -o #
PIC_KEY = -fPIC
WARNING_AS_ERROR_KEY = -Werror
WARNING_KEY = -Wall
WARNING_SUPPRESS =
DYLIB_KEY = -dynamiclib
EXPORT_KEY = -Wl,-exported_symbols_list,
LIBDL = -ldl

LIBS = -lpthread
LINK_FLAGS = 
LIB_LINK_FLAGS = -dynamiclib
C_FLAGS = $(CPLUS_FLAGS)

ifeq ($(cfg), release)
    CPLUS_FLAGS = -O2
else
    CPLUS_FLAGS = -g -O0 -DTBB_USE_DEBUG
endif

CPLUS_FLAGS += -DUSE_PTHREAD

ifeq (intel64,$(arch))
    CPLUS_FLAGS += -m64
    LINK_FLAGS += -m64
    LIB_LINK_FLAGS += -m64
endif

ifeq (ia32,$(arch))
    CPLUS_FLAGS += -m32
    LINK_FLAGS += -m32
    LIB_LINK_FLAGS += -m32
endif

ifeq (ppc64,$(arch))
    CPLUS_FLAGS += -arch ppc64
    LINK_FLAGS += -arch ppc64
    LIB_LINK_FLAGS += -arch ppc64
endif

ifeq (ppc,$(arch))
    CPLUS_FLAGS += -arch ppc
    LINK_FLAGS += -arch ppc
    LIB_LINK_FLAGS += -arch ppc
endif

#------------------------------------------------------------------------------
# Setting tbbmalloc data.
#------------------------------------------------------------------------------

M_CPLUS_FLAGS = $(CPLUS_FLAGS) -fno-rtti -fno-exceptions -fno-schedule-insns2

#------------------------------------------------------------------------------
# End of setting tbbmalloc data.
#------------------------------------------------------------------------------

