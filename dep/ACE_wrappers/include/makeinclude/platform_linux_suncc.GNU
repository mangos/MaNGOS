# $Id: platform_linux_suncc.GNU 86702 2009-09-14 06:55:44Z olli $

# This file should allow ACE to be built on Linux, using the Sun compiler.
# The Sun compiler on linux is still in the Alpha stage, so no guarantees
# given at this point

include $(ACE_ROOT)/include/makeinclude/platform_linux_common.GNU

no_hidden_visibility ?= 1

ifeq ($(insure),0)
  CC  = CC
  CXX = CC
endif

ifeq ($(threads),1)
    CPPFLAGS  += -D_REENTRANT $(PLATFORM_AIO_SUPPORT)
endif # threads

GNU_LIBPTHREAD_VERSION := $(shell getconf GNU_LIBPTHREAD_VERSION)
ifeq (NPTL, $(word 1,$(GNU_LIBPTHREAD_VERSION)))
  CPPFLAGS += -DACE_HAS_LINUX_NPTL
endif

CCFLAGS += $(CFLAGS) $(IMPLICIT_TEMPLATES_FLAG)
DCFLAGS += -g
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

ifeq ($(optimize),0)
  # Disable all optimizing in code
  CPPFLAGS += -O0
endif

PIC     =
SOFLAGS += $(CPPFLAGS) -shared
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<; \
          $(SOLINK.cc) -o $@ $(LDFLAGS) $(VSHDIR)$*.o
PRELIB  = @true

ifeq ($(shared_libs), 1)
  ifneq ($static_libs_only), 1)
    ifneq ($(no_hidden_visibility),1)
      CCFLAGS += -fvisibility=hidden
    else
      CPPFLAGS += -DACE_HAS_CUSTOM_EXPORT_MACROS=0
    endif # no_hidden_visibility
  endif
endif

ifeq ($(buildbits),32)
  CFLAGS   += -m32
  LDFLAGS  += -m32
endif
ifeq ($(buildbits),64)
  CFLAGS   += -m64
endif

# Enable GNU extensions
CPPFLAGS+= -D_GNU_SOURCE

# Added line below to support "Executable Shared Object" files (as
# needed by the service configurator).
# Marius Kjeldahl <mariusk@sn.no, marius@funcom.com>
ifeq ($(threads),1)
    ESOBUILD = $(COMPILEESO.cc) $(PIC) -shared -o $(VSHDIR)$*.so $<
    ifndef PRELIB
       PRELIB = @true
    endif # ! PRELIB
endif
