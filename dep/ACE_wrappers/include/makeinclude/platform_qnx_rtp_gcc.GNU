# $Id: platform_qnx_rtp_gcc.GNU 91626 2010-09-07 10:59:20Z johnnyw $
#
# QNX/RTP hosted, using GCC

inline ?= 1
debug ?= 1
optimize ?= 0
threads ?= 1
CCFLAGS += -fexceptions
LDFLAGS += -fexceptions

ifeq ($(inline),1)
  CCFLAGS += -finline-functions
else
  CCFLAGS += -fno-inline
endif # inline

ifeq ($(debug),1)
	DCFLAGS += -gdwarf-2
else # debug excludes optimize due to g++ internal compiler error
  ifeq ($(optimize),1)
	OCFLAGS += -O3
  endif #optimize
endif #debug
CC      ?= gcc
CXX     ?= g++

CFLAGS  += -pipe -W -Wall -Wpointer-arith

DLD     = $(CXX)
LD      = $(CXX)
LIBS    += -lsocket -lstdc++ -lm

PIC     = -fPIC
AR      ?= ar
ARFLAGS = ruv
SOFLAGS += $(CPPFLAGS) -shared
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
PRELIB  = @true

# Test for template  instantiation, add to SOFLAGS if SONAME set,
# add -E to LDFLAGS if using GNU ld
#
include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU
CCFLAGS += $(CFLAGS) $(TEMPLATES_FLAG)

CFLAGS  += $(FLAGS_C_CC)
CCFLAGS += $(FLAGS_C_CC) 
