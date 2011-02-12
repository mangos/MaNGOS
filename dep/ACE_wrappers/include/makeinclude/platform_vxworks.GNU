# $Id: platform_vxworks.GNU 87036 2009-10-10 18:21:39Z johnnyw $
# VxWorks main platform file. This requires that you have set the WIND_PLATFORM
# environmeht variable

ifeq (,$(WIND_PLATFORM))
  default:
        @ERROR: you must set your WIND_PLATFORM environment variable
endif # WIND_BASE

ifeq (vxworks-6.2, $(findstring vxworks-6.2,$(WIND_PLATFORM)))
  include $(ACE_ROOT)/include/makeinclude/platform_vxworks6.2.GNU
endif
ifeq (vxworks-6.3, $(findstring vxworks-6.3,$(WIND_PLATFORM)))
  include $(ACE_ROOT)/include/makeinclude/platform_vxworks6.3.GNU
endif
ifeq (vxworks-6.4, $(findstring vxworks-6.4,$(WIND_PLATFORM)))
  include $(ACE_ROOT)/include/makeinclude/platform_vxworks6.4.GNU
endif
ifeq (vxworks-6.5, $(findstring vxworks-6.5,$(WIND_PLATFORM)))
  include $(ACE_ROOT)/include/makeinclude/platform_vxworks6.5.GNU
endif
ifeq (vxworks-6.6, $(findstring vxworks-6.6,$(WIND_PLATFORM)))
  include $(ACE_ROOT)/include/makeinclude/platform_vxworks6.6.GNU
endif
ifeq (vxworks-6.7, $(findstring vxworks-6.7,$(WIND_PLATFORM)))
  include $(ACE_ROOT)/include/makeinclude/platform_vxworks6.7.GNU
endif
ifeq (vxworks-6.8, $(findstring vxworks-6.8,$(WIND_PLATFORM)))
  include $(ACE_ROOT)/include/makeinclude/platform_vxworks6.8.GNU
endif


