# $Id: platform_win32_interix.GNU 91285 2010-08-05 08:29:30Z johnnyw $

# platform_win32_interix.GNU

versioned_so ?= 0

debug ?= 1
optimize ?= 1
threads ?= 1
pipes ?= 1

CC              = gcc
CXX             = g++

CFLAGS          += -W -Wall -Wpointer-arith
CPPFLAGS        += -D_ALL_SOURCE
DCFLAGS         += -g
DLD             = $(CXX)
LD              = $(CXX)
LIBS            += -ldl
OCFLAGS         += -O2
PIC             =
AR              = ar
ARFLAGS         = ruv
RANLIB          = ranlib
LN_S            = cp

SOFLAGS         += $(CPPFLAGS) -shared
SOBUILD         = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
PRELIB          = @echo

ifeq ($(threads),1)
  LDFLAGS       += -lpthread
  CFLAGS        += -D_THREAD_SAFE -DACE_HAS_THREADS
endif # threads

PLATFORM_X11_CPPFLAGS=-I/usr/X11R6/include
PLATFORM_X11_LIBS=-lX11
PLATFORM_X11_LDFLAGS=-L/usr/X11R6/lib

# Test for template instantiation, add to SOFLAGS if versioned_so set,
# add -E to LDFLAGS if using GNU ld
#
#include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU

CCFLAGS         += $(CFLAGS) $(TEMPLATES_FLAG)
