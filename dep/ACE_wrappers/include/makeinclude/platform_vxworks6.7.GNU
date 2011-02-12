# $Id: platform_vxworks6.7.GNU 85207 2009-04-29 06:08:59Z johnnyw $
# VxWorks 6.7

ifeq ($(VXWORKS_VERSION_FLAG),)
  VXWORKS_VERSION_FLAG = -DACE_VXWORKS=0x670
endif # VXWORKS_VERSION_FLAG

include $(ACE_ROOT)/include/makeinclude/platform_vxworks6.6.GNU

ifeq ("$(WIND_HOST_TYPE)","x86-linux2")
ifeq ($(SHARED_LIBS), 1)
ifeq ("$(TOOL_FAMILY)","gnu")
  LDFLAGS += -shared
else
  LDFLAGS += -Xshared
endif
endif
endif
