# $Id: platform_linux_icc.GNU 86019 2009-07-14 12:13:09Z wotte $

# This file should allow ACE to be built on Linux, using the Intel compiler.

AR      = xiar

include $(ACE_ROOT)/include/makeinclude/platform_linux_common.GNU

fast ?= 0
ipo ?= 0

ifeq ($(insure),0)
  CC  = icc
  CXX = icpc
endif

ifndef CXX_VERSION
  CXX_VERSION := $(shell $(CXX) --version)
endif

ifeq (8.0,$(findstring 8.0,$(CXX_VERSION)))
  CFLAGS += -wd1476,1505
endif
ifeq (8.1,$(findstring 8.1,$(CXX_VERSION)))
  CFLAGS += -wd1476,1505,1572 -no-gcc
endif
ifeq (9.0,$(findstring 9.0,$(CXX_VERSION)))
  CFLAGS += -wd1684
endif
ifeq (9.1,$(findstring 9.1,$(CXX_VERSION)))
  CFLAGS += -wd1684
endif
ifeq (10.0,$(findstring 10.0,$(CXX_VERSION)))
  CFLAGS += -wd1684
endif
ifeq (10.1,$(findstring 10.1,$(CXX_VERSION)))
  CFLAGS += -wd1684
endif
ifeq (11.0,$(findstring 11.0,$(CXX_VERSION)))
  CFLAGS += -wd1684
endif

no_hidden_visibility ?= 1

ifeq ($(inline),0)
  CPPFLAGS += -fno-inline
endif

ifeq ($(fast),1)
  CPPFLAGS += -fast
endif

ifeq ($(ipo),1)
  CPPFLAGS += -ipo
endif

CFLAGS  += -w1
ifeq ($(threads),1)
    CPPFLAGS  += -D_REENTRANT $(PLATFORM_AIO_SUPPORT)
endif # threads

CCFLAGS += $(CFLAGS) $(IMPLICIT_TEMPLATES_FLAG) -ip
DCFLAGS += -g -debug full
DLD     = $(CXX)
LD      = $(CXX)
LIBS    += -ldl

ifeq ($(threads),1)
  LIBS += -lpthread
  ifeq (-DACE_HAS_AIO_CALLS,$(PLATFORM_AIO_SUPPORT))
    LIBS += -lrt
  endif
endif

OCFLAGS += -O3

# Disable floating point optimizer solves problem with
# min/max float values in the TAO_IDL compiler tests
# If these options are not passed we can get overflows
# when testing min/max
CPPFLAGS += -fp-model double

ifeq ($(optimize),0)
  # Disable all optimizing in code
  CPPFLAGS += -O0
endif

SOFLAGS += $(CPPFLAGS) -shared
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<; \
          $(SOLINK.cc) -o $@ $(LDFLAGS) $(VSHDIR)$*.o
PRELIB  = @true

ifeq ($(shared_libs), 1)
  ifneq ($static_libs_only), 1)
    LDFLAGS += -Wl,-E
    ifneq ($(no_hidden_visibility),1)
      CCFLAGS += -fvisibility=hidden
    else
      CPPFLAGS += -DACE_HAS_CUSTOM_EXPORT_MACROS=0
    endif # no_hidden_visibility
  endif
endif


# Added line below to support "Executable Shared Object" files (as
# needed by the service configurator).
# Marius Kjeldahl <mariusk@sn.no, marius@funcom.com>
ifeq ($(threads),1)
    ESOBUILD = $(COMPILEESO.cc) $(PIC) -shared -o $(VSHDIR)$*.so $<
    ifndef PRELIB
       PRELIB = @true
    endif # ! PRELIB
endif

