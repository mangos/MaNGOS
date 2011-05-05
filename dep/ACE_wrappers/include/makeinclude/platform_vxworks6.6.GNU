# $Id: platform_vxworks6.6.GNU 86847 2009-09-30 09:59:46Z johnnyw $
# VxWorks 6.6

ifeq ($(VXWORKS_VERSION_FLAG),)
  VXWORKS_VERSION_FLAG = -DACE_VXWORKS=0x660
endif # VXWORKS_VERSION_FLAG

include $(ACE_ROOT)/include/makeinclude/platform_vxworks6.5.GNU

ifeq ("$(WIND_HOST_TYPE)","sun4-solaris2")
  override RM=rm -f
endif # sun4-solaris2

# Disable strict aliasing warnings, these are given on some VxWorks system
# header files and has been reported to WindRiver
ifeq ("$(TOOL_FAMILY)","gnu")
  CPPFLAGS += -fno-strict-aliasing
endif

# Workaround for bug in diab compiler with just 6.6
ifeq (vxworks-6.6, $(findstring vxworks-6.6,$(WIND_PLATFORM)))
  ifeq ("$(TOOL_FAMILY)","diab")
    C++FLAGS += -W:c++:,-Xusing-std-on
  endif
endif
