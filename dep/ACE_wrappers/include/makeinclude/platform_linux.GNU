# -*- Makefile -*-
# $Id: platform_linux.GNU 91626 2010-09-07 10:59:20Z johnnyw $

# According to Bryon G. Rigg <bgrigg@opus.bcbnet.com>, this file
# should allow ACE to be built on Linux.

include $(ACE_ROOT)/include/makeinclude/platform_linux_common.GNU

ifeq ($(insure),0)
  CC  ?= gcc
  CXX ?= g++
  CXX_FOR_VERSION_TEST ?= $(CXX)
else
  CXX_FOR_VERSION_TEST ?= g++
endif

# fix this
pipes ?= 1

CXX_FULL_VERSION := $(shell $(CXX_FOR_VERSION_TEST) --version)
CXX_VERSION := $(shell $(CXX_FOR_VERSION_TEST) -dumpversion)

ifeq (Ubuntu, $(findstring Ubuntu,$(LSB_RELEASE_ID)))
  ifeq (7.10, $(findstring 7.10,$(LSB_RELEASE_RELEASE)))
    no_hidden_visibility ?= 1
  endif
  ifeq (7.04, $(findstring 7.04,$(LSB_RELEASE_RELEASE)))
    no_hidden_visibility ?= 1
  endif
endif
ifeq (Red Hat, $(findstring Red Hat,$(CXX_FULL_VERSION)))
  ifeq (4.1.1, $(findstring 4.1.1,$(CXX_VERSION)))
    gcc_template_instantiation_visibility ?= 1
  endif
  ifeq (4.1.2, $(findstring 4.1.2,$(CXX_VERSION)))
    gcc_template_instantiation_visibility ?= 1
  endif
endif
# Mandriva 2007
ifeq (4.1.1-3mdk, $(findstring 4.1.1-3mdk,$(CXX_FULL_VERSION)))
  gcc_template_instantiation_visibility ?= 1
endif

FLAGS_C_CC += -W -Wall -Wpointer-arith
ifeq ($(threads),1)
    CPPFLAGS  += -D_REENTRANT $(PLATFORM_AIO_SUPPORT)
endif # threads

ifeq ($(buildbits),32)
  FLAGS_C_CC += -m32
  LDFLAGS    += -m32
endif
ifeq ($(buildbits),64)
  FLAGS_C_CC += -m64
  LDFLAGS    += -m64
endif

# Rely on _GNU_SOURCE to set these defaults defined in /usr/include/features.h
# instead of setting them directly here (older versions of gcc don't set it
# for you): _SVID_SOURCE _BSD_SOURCE _POSIX_SOURCE _POSIX_C_SOURCE=199506L, ...
CPPFLAGS += -D_GNU_SOURCE

DCFLAGS  += -ggdb
DCCFLAGS += -ggdb
DLD      = $(CXX)
LD       = $(CXX)
LIBS     += -ldl

ifeq ($(threads),1)
  LIBS += -lpthread
  # look for the rt library in the usual places
  LIBS += $(shell test "`ls -L /usr/lib*/librt.so* /lib*/librt.so*`" && echo -lrt)
endif

ifeq ($(optimize),1)
  SOFLAGS += -Wl,-O3
endif

SOFLAGS += $(CPPFLAGS) -shared
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<; \
          $(SOLINK.cc) -o $@ $(LDFLAGS) $(VSHDIR)$*.o
PRELIB  = @true

# Test for template instantiation, add to SOFLAGS if SONAME set,
# add -E to LDFLAGS if using GNU ld
#
include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU

# TAO with GCC 4.0.2 and -O3 seems to result in runtime issues, for example
# the ForwardRequest PI test will fail. For GCC 4.0.2 we default to -O2
ifeq ($(CXX_VERSION),4.0.2)
  OCFLAGS ?= -O2
  OCCFLAGS ?=-O2
else
  OCFLAGS ?= -O3
  OCCFLAGS ?= -O3
endif

CFLAGS  += $(FLAGS_C_CC)
CCFLAGS += $(FLAGS_C_CC) $(TEMPLATES_FLAG)

# Added line below to support "Executable Shared Object" files (as
# needed by the service configurator).
# Marius Kjeldahl <mariusk@sn.no, marius@funcom.com>
ifeq ($(threads),1)
    ESOBUILD = $(COMPILEESO.cc) $(PIC) -shared -o $(VSHDIR)$*.so $<
    ifndef PRELIB
       PRELIB = @true
    endif # ! PRELIB
endif

#### GNU gas has a string limit of 4096 characters.  On Alphas,
#### builds will fail due to running over that limit.  There are
#### at least two workarounds:
#### 1) Change the limit to 8192 characters and rebuild gas.  See
####    ACE-INSTALL.html for more information.
#### 2) Don't use -g when compiling those files.
#### If you're building on an Alpha and you haven't hacked and
#### rebuilt gas, you might need to uncomment the following.
#### ifeq ($(debug),1)
####   SUPPRESS_DASH_G = 1
#### endif # debug
