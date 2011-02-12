# $Id: platform_macosx_panther.GNU 91285 2010-08-05 08:29:30Z johnnyw $
# platform_macosx.GNU
# support for Mac OS X 10.3 (Panther)

threads ?= 1
debug ?= 1
optimize ?= 0
versioned_so ?= 0
with_ld = macosx

pipes ?= 1

CC              = gcc
CXX             = g++
CFLAGS          += -Wall -Wpointer-arith -Wno-long-double

DCFLAGS         += -g
DLD             = libtool
LD              = $(CXX)
LIBS            += -lcc_dynamic -lstdc++ -lSystem

OCFLAGS         += -O2 -mcpu=G3 -mtune=G4
RANLIB          = ranlib
SOEXT		= dylib
SOFLAGS += -dynamic
SOBUILD = -o $(VSHDIR)$*.dylib $<

# Test for template instantiation, add to SOFLAGS if versioned_so set,
# add -E to LDFLAGS if using GNU ld
#
include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU

LDFLAGS         += -flat_namespace -undefined warning
CCFLAGS         += $(CFLAGS) $(TEMPLATES_FLAG)
