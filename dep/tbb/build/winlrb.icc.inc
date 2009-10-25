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


include $(tbb_root)/build/winlrb.cl.inc

TEST_EXT = so
.PRECIOUS: %.$(TEST_EXT)

include $(tbb_root)/build/freebsd.gcc.inc

WARNING_KEY = -w1
CPLUS = icpc
CONLY = icc
#LIBS = -u _read -lcprts -lthr -lc
#LIBS = -lthr
LIBS = -u _read -lcprts -lthr -limf -lc
LINK_FLAGS = -L$(LRB_LIB_DIR) $(DYLIB_KEY) -lxn$(XN_VER)_lrb64$(CFG_LETTER)
CPLUS_FLAGS += -xR $(PIC_KEY) -I$(LRB_INC_DIR) -DXENSIM
C_FLAGS = $(CPLUS_FLAGS)
LIB_LINK_FLAGS = $(LINK_FLAGS)

ifeq ($(cfg), release)
    # workaround for LRB compiler issues
    CPLUS_FLAGS := $(subst -O2,-O0, $(CPLUS_FLAGS))
endif
