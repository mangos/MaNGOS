# $Id: platform_vxworks6.4.GNU 80826 2008-03-04 14:51:23Z wotte $
# VxWorks 6.4

#### Notes:
#### 1) This file requires that the WIND_BASE and WIND_HOST_TYPE environment
####    variables be set.  If the target CPU is not SIMNT, then your CPU
####    environment variable must be set.
#### 2) The TOOL environment variable may be set to "diab" (default is "gnu").
#### 3) If perl is not on your path, you'll also need to set your PERL_PATH
####    environment variable to the full path to perl.

ifeq ($(VXWORKS_VERSION_FLAG),)
  VXWORKS_VERSION_FLAG = -DACE_VXWORKS=0x640
endif # VXWORKS_VERSION_FLAG

include $(ACE_ROOT)/include/makeinclude/platform_vxworks6.3.GNU
