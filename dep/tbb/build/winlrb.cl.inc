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

include $(tbb_root)/build/windows.cl.inc

ifeq ($(cfg), debug)
    CFG_LETTER = d
else
    CFG_LETTER = r
endif

_CPLUS_FLAGS_HOST := $(CPLUS_FLAGS) /I$(LRB_INC_DIR) $(LINK_FLAGS) /LIBPATH:$(LRB_LIB_DIR) xn_host$(LRB_HOST_ARCH)$(CFG_LETTER).lib

TEST_EXT = dll
CPLUS_FLAGS += /I$(LRB_INC_DIR) /D__LRB__
LIB_LINK_FLAGS += /LIBPATH:$(LRB_LIB_DIR) xn_lrb$(LRB_HOST_ARCH)$(CFG_LETTER).lib
LINK_FLAGS = $(LIB_LINK_FLAGS)
OPENMP_FLAG =

ifdef TEST_RESOURCE
LINK_FLAGS += $(TEST_RESOURCE)

TEST_LAUNCHER_NAME = harness_lrb_host
AUX_TEST_DEPENDENCIES = $(TEST_LAUNCHER_NAME).exe

$(TEST_LAUNCHER_NAME).exe: $(TEST_LAUNCHER_NAME).cpp
	cl /Fe$@ $< $(_CPLUS_FLAGS_HOST)

NO_LEGACY_TESTS = 1
NO_C_TESTS = 1
TEST_LAUNCHER=
endif # TEST_RESOURCE

#test_model_plugin.%:
#	@echo test_model_plugin is not supported for LRB architecture so far

ifeq ($(BUILDING_PHASE),0)  # examples
    export RM = del /Q /F
    export LIBS = -shared -lthr -z muldefs -L$(work_dir)_debug -L$(work_dir)_release
    export UI = con
    export x64 = 64
    export CXXFLAGS = -xR -I..\..\..\include
endif # examples
