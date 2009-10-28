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

tbb_root = $(srcdir)

include $(tbb_root)/build/common.inc

# change these
override work_dir = $(CWD)
export work_dir
override tbb_root = $(srcdir)
export work_dir

.PHONY: all tbb tbbmalloc

#workaround for non-depend targets tbb and tbbmalloc which both depend on version_string.tmp
#According to documentation submakes should run in parallel
.NOTPARALLEL: tbb tbbmalloc

all: tbb tbbmalloc

tbb:
	$(MAKE) -r -f $(tbb_root)/build/Makefile.tbb cfg=release tbb_root=$(tbb_root)

tbbmalloc:
	$(MAKE) -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=release malloc tbb_root=$(tbb_root)

install-exec-local:
	$(INSTALL) $(work_dir)/lib*.so* $(libdir)

clean-local:
	-rm -f *.d *.o
	-rm -f lib*.so*
	-rm -f *.def *.tmp tbbvars.*

