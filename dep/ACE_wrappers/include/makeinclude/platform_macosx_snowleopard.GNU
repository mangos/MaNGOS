# $Id: platform_macosx_snowleopard.GNU 87538 2009-11-13 01:54:36Z wotte $

ifeq ($(buildbits),32)
  FLAGS_C_CC += -m32
  LDFLAGS    += -m32
endif
ifeq ($(buildbits),64)
  FLAGS_C_CC += -m64
  LDFLAGS    += -m64
endif
ifeq ($(buildbits),universal)
  FLAGS_C_CC += -arch i386 -arch x86_64
  LDFLAGS    += -arch i386 -arch x86_64
endif

include $(ACE_ROOT)/include/makeinclude/platform_macosx_common.GNU

