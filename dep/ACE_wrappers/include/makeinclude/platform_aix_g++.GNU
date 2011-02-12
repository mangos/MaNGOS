# $Id: platform_aix_g++.GNU 91285 2010-08-05 08:29:30Z johnnyw $
#
# Platform macros for building on AIX with g++.  This has been tested on
# AIX 4.3 and 5 with gcc 3. Earlier versions of gcc may compile, but are
# known to have serious problems on AIX.
#
# Contributed by Ian Wright <I.Wright@elsevier.co.uk>
# Updated for AIX 4.3 and gcc 3 by Steve Huston  <shuston@riverace.com>

debug ?= 1
distrib ?= 0
optimize ?= 1
threads ?= 1
buildbits ?= 32
with_ld ?= aix
no_hidden_visibility ?= 1

PLATFORM_XT_CPPFLAGS=
PLATFORM_XT_LIBS=-lXt
PLATFORM_XT_LDFLAGS=

PLATFORM_X11_CPPFLAGS=
PLATFORM_X11_LIBS=-lX11
PLATFORM_X11_LDFLAGS=

# Yes, believe it or not...
# The way to make a shared lib on AIX is to make a shared object file and
# then put that in an archive.
# Non-shared archives are named lib<x>ns.a - ns = non-shared.
SOEXT    = so
LDFLAGS += -Wl,-brtl

ifeq (1,$(threads))
  CFLAGS  += -pthread
endif

ifeq ($(shared_libs),1)
ACELIB      = -lACE
else
ACELIB      = -lACEns
endif

# In case anything here or in the config depends on OS version number,
# grab it here and pass it all to the compiler as well.
AIX_MAJOR_VERS := $(shell uname -v)
AIX_MINOR_VERS := $(shell uname -r)
AIX_VERS := $(AIX_MAJOR_VERS)0$(AIX_MINOR_VERS)

CC             = gcc
CXX            = g++

CFLAGS          += -W -Wall -DACE_AIX_VERS=$(AIX_VERS)
ifeq ($(buildbits),32)
  CFLAGS    += -maix32
endif
ifeq ($(buildbits),64)
  CFLAGS    += -maix64
  LDFLAGS   += -maix64
  ARFLAGS   += -X64
endif
DCFLAGS         += -g
DLD             = $(CXX)
LD              = $(CXX)
# Linking TAO_IDL runs out of TOC space unless -bbigtoc is given to ld.
# Don't do it for everything because it decreases run-time performance.
ifeq ($(notdir $(shell pwd)), TAO_IDL)
LDFLAGS        += -Wl,-bbigtoc
endif

OCFLAGS         = -O2
OCCFLAGS        = $(OCFLAGS)
ifeq ($(threads),1)
  SOFLAGS        += -pthread
endif

# AIX 5 has a new ld option, -bexpfull, that exports all the symbols that
# g++ 3.3+ needs to work correctly. The new option is available by default
# in AIX 5.2 and later, but must be installed by a patch to AIX 5.1.
ifeq ($(AIX_MAJOR_VERS),5)
  SOFLAGS        += -shared -Wl,-bexpfull # -Wl,-bloadmap:lib.mapelse
else
  SOFLAGS        += -shared -Wl,-bexpall # -Wl,-bloadmap:lib.map
endif

LIBS           += -lxti -ldl
ARFLAGS        += cruv
RANLIB          = ranlib

# Test for template instantiation, add to SOFLAGS if versioned_so set,
# add -E to LDFLAGS if using GNU ld
#
include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU

CCFLAGS         += $(CFLAGS) $(TEMPLATES_FLAG)
