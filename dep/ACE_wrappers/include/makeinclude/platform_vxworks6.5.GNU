# $Id: platform_vxworks6.5.GNU 80826 2008-03-04 14:51:23Z wotte $
# VxWorks 6.5

ifeq ($(VXWORKS_VERSION_FLAG),)
  VXWORKS_VERSION_FLAG = -DACE_VXWORKS=0x650
endif # VXWORKS_VERSION_FLAG

include $(ACE_ROOT)/include/makeinclude/platform_vxworks6.4.GNU
