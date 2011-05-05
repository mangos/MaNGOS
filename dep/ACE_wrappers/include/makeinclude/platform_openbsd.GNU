# $Id: platform_openbsd.GNU 91285 2010-08-05 08:29:30Z johnnyw $

# platform_openbsd.GNU

versioned_so = 1

debug ?= 1
optimize ?= 1
threads ?= 1
pipes ?= 1

CC              = gcc
CXX		= g++

CFLAGS          += -W -Wall -Wpointer-arith
DCFLAGS         += -g
LDFLAGS		+= -Wl,-rpath $(ACE_ROOT)/lib
DLD             = $(CXX)
LD		= $(CXX)
LIBS            +=
OCFLAGS         += -O2
PIC             = -fpic
AR              = ar
ARFLAGS         = ruv
RANLIB          = ranlib

SOFLAGS += $(CPPFLAGS) -shared
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so  $<
PRELIB = @echo

ifeq ($(threads),1)
  LDFLAGS       += -pthread
  CFLAGS        += -D_THREAD_SAFE -DACE_HAS_THREADS
endif # threads

PLATFORM_X11_CPPFLAGS=-I/usr/X11R6/include
PLATFORM_X11_LIBS=-lX11
PLATFORM_X11_LDFLAGS=-L/usr/X11R6/lib

PLATFORM_TK_CPPFLAGS=$(shell . /usr/local/lib/tk*/tkConfig.sh && echo -n $$TK_INCLUDE_SPEC) $(PLATFORM_X11_CPPFLAGS)
PLATFORM_TK_LIBS=$(shell . /usr/local/lib/tk*/tkConfig.sh && echo -n $$TK_LIB_FLAG) $(PLATFORM_X11_LIBS)
PLATFORM_TK_LDFLAGS=$(PLATFORM_X11_LDFLAGS)

PLATFORM_TCL_CPPFLAGS=$(shell . /usr/local/lib/tcl*/tclConfig.sh && echo -n $$TCL_INCLUDE_SPEC)
PLATFORM_TCL_LIBS=$(shell . /usr/local/lib/tcl*/tclConfig.sh && echo -n $$TCL_LIB_FLAG)
PLATFORM_TCL_LDFLAGS=

CPPFLAGS+=-I/usr/local/include
LDFLAGS+=-L/usr/local/lib

# Test for template instantiation, add to SOFLAGS if versioned_so set,
# add -E to LDFLAGS if using GNU ld
#
include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU

CCFLAGS         += $(CFLAGS) $(TEMPLATES_FLAG)
