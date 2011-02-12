# $Id: platform_macosx_tiger.GNU 87237 2009-10-27 08:22:44Z wotte $
# platform_macosx.GNU
# support for Mac OS X 10.4 (Tiger)
# By default, this uses the default compiler selected using gcc_select.

include $(ACE_ROOT)/include/makeinclude/platform_macosx_common.GNU

CFLAGS += -Wno-long-double