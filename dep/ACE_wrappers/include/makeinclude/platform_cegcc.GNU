# $Id: platform_cegcc.GNU 86432 2009-08-09 18:56:15Z johnnyw $
#
# This file should allow to build ACE for Windows CE with GCC tools.
#
# Don't forget to define the ACE_ROOT environment variable!
#

#
# Chose your target CPU (by default we set it to ARM. In your
# platform_macros.GNU file you can override this to pentium, i486 or i386
#
TCPU ?= arm

cygwin32 = 1

# Cygwin doesn't have rwho
rwho = 0

# Cygwin doesn't allow to add the version number of ACE to the dll name
# because fe ace.dll.5.2.3 isn't a valid dll name
versioned_so=0

ACE_CE_VER ?= 0x600

qos ?= 1

# Disable auto-import warnings. The Cygwin linker has a problem with imports
# See https://sourceforge.net/tracker/?func=detail&atid=102435&aid=683455&group_id=2435
# for the details why to do this.
LDFLAGS += -Wl,--enable-auto-import
CPPFLAGS += -DACE_HAS_CEGCC 

CPPFLAGS += -D_WIN32_WCE=$(ACE_CE_VER) -DUNICODE -D_UNICODE -D_WINDOWS -DCEVER -DWIN32 -D_WINSOCKAPI_
RCFLAGS += -D_WIN32_WCE=$(ACE_CE_VER) -DUNICODE -D_UNICODE -D_WINDOWS -DCEVER -DWIN32 -D_WINSOCKAPI_ -D_WIN32_IE=0x0500

cpumodelflag=0
tunemodelflag=0

COMPARCH ?= i386-mingw32ce-
#COMPARCH ?= arm-wince-mingw32ce-

LIBS += -lcoredll -lmingw32 -lmingwex -lws2 -lsupc++ -liphlpapi

no_hidden_visibility ?= 1
qos ?= 1

ifdef COMSPEC
  HOST_EXE_EXT = .exe
endif

ifneq (,$(HOST_ROOT))
TAO_IDLFLAGS += -g $(HOST_ROOT)/bin/ace_gperf
TAO_IDL = $(HOST_ROOT)/bin/tao_idl$(HOST_EXE_EXT)
TAO_IDL3_TO_IDL2 = $(HOST_ROOT)/TAO/CIAO/bin/tao_idl3_to_idl2$(HOST_EXE_EXT)
TAO_IDL_DEP = $(TAO_IDL)
TAO_IDL3_TO_IDL2_DEP = $(TAO_IDL3_TO_IDL2)
else
# Append the HOST_EXE_EXT if it's not already there
TAO_IDL_DEP = $(TAO_IDL:$(HOST_EXE_EXT)=)$(HOST_EXE_EXT)
TAO_IDL3_TO_IDL2_DEP = $(TAO_IDL3_TO_IDL2:$(HOST_EXE_EXT)=)$(HOST_EXE_EXT)
endif

include $(ACE_ROOT)/include/makeinclude/platform_gnuwin32_common.GNU
