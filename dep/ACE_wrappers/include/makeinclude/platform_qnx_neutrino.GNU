# $Id: platform_qnx_neutrino.GNU 91626 2010-09-07 10:59:20Z johnnyw $

# QNX 4.25 hosted GNU g++ for Neutrino 2.0

CROSS-COMPILE = 1
static_libs_only = 1

debug ?= 1
optimize ?= 1
pipes ?= 1
CC      ?= i386-nto-gcc
CXX     ?= i386-nto-g++

CFLAGS  += -W -Wall -Wpointer-arith
DCFLAGS += -g
DLD     = $(CXX)
LD      = $(CXX)
LDFLAGS +=
LIBS    += -lsocket -lstdc++
OCFLAGS += -O2
PIC     = -fPIC
AR     ?= i386-nto-ar
ARFLAGS = ruv
RANLIB ?= i386-nto-ranlib
SOFLAGS += -G $(CPPFLAGS) -shared
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
PRELIB  = @true

# Test for template instantiation, add to SOFLAGS if SONAME set,
# add -E to LDFLAGS if using GNU ld
#
include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU

CCFLAGS += $(CFLAGS) $(TEMPLATES_FLAG)
