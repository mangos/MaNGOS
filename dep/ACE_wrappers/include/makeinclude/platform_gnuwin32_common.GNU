# -*- Makefile -*-
# $Id: platform_gnuwin32_common.GNU 91285 2010-08-05 08:29:30Z johnnyw $

ifneq ($(mingw32),1)
  ifneq ($(cygwin32),1)
    fail := \
      $(shell \
          echo "You should not use this file but one of"; \
          echo "platform_mingw32.GNU or platform_cygwin32.GNU"; \
          exit 1)
  endif
endif

no_hidden_visibility ?= 1

GNUWIN32_CXX_VERSION := $(shell $(CXX) -dumpversion)

winsock2 ?= 1
debug ?= 1
optimize ?= 1
threads ?= 1
pipes ?= 1

# When building shared libraries
ifeq ($(shared_libs), 1)
  ifneq ($(static_libs_only), 1)
    inline ?= 0
  endif
endif

ifeq ($(debug),0)
CFLAGS += -DNDEBUG
endif
CC      = ${COMPARCH}gcc
CXX     = ${COMPARCH}g++
RC      = ${COMPARCH}windres
DLLTOOL = ${COMPARCH}dlltool

RC_OUTPUT_FLAG = -o

ifeq (2.9,$(findstring 2.9,$(GNUWIN32_CXX_VERSION)))
  cpumodelflag ?= 1
  tunemodelflag ?= 0
else
  cpumodelflag ?= 0
  tunemodelflag ?= 1
endif

ifeq ($(tunemodelflag),1)
  MODEL_FLAGS += -mtune=$(TCPU)
endif

ifeq ($(cpumodelflag),1)
  MODEL_FLAGS += -mcpu=$(TCPU)
endif

CFLAGS += -Wpointer-arith
ifeq ($(threads),1)
  CFLAGS += -mthreads
endif # threads

CFLAGS += $(MODEL_FLAGS)

EXEEXT = .exe

DCFLAGS += -g
DLD     = $(CXX)
LD      = $(CXX)

ifeq ($(mingw32),1)
ifeq ($(winsock2),1)
LIBS    += -lws2_32 -lmswsock
endif
LIBS    += -lwsock32 -lnetapi32
endif

OCFLAGS += -O3
PIC        =
AR         = ${COMPARCH}ar
ARFLAGS    = rsuv
RANLIB     = ${COMPARCH}ranlib
SOEXT      = dll
SOFLAGS    += $(MODEL_FLAGS) $(CPPFLAGS)
SOBUILD    = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<

PLATFORM_FL_CPPFLAGS  ?=
PLATFORM_FL_LIBS      ?= -lfltk -lfltk_forms -lfltk_gl -lfltk_images
PLATFORM_FL_LDFLAGS   ?=

PLATFORM_GL_CPPFLAGS  ?= -I/usr/X11R6/include
PLATFORM_GL_LIBS      ?= -lGL
PLATFORM_GL_LDFLAGS   ?= -L/usr/X11R6/lib

PLATFORM_WX_CPPFLAGS  ?= $(shell wx-config --cxxflags) -I/usr/local/include
PLATFORM_WX_LIBS      ?= $(shell wx-config --libs)
PLATFORM_WX_LDFLAGS   ?= $(shell wx-config --ldflags)

# The sed below is to strip comments on the .def file,
# to workaround to a bug in ld.
CMDSEP ?= ;
SHLIBBUILD = \
  $(RM) $(VSHDIR)/$@.def.old $(VSHDIR)/$@.def $(CMDSEP) \
  $(DLLTOOL) --export-all --output-def $(VSHDIR)/$@.def --dllname $@ \
    $(VSHOBJS) \
  && mv $(VSHDIR)/$@.def $(VSHDIR)/$@.def.old \
  && sed "s/;.*$$//g" < $(VSHDIR)/$@.def.old > $(VSHDIR)/$@.def \
  && $(SOLINK.cc) -Wl,--enable-auto-image-base -Wl,--out-implib,$@.a \
    -shared -o $@ $(LDFLAGS) -Wl,$(VSHDIR)/$@.def \
    $(VSHOBJS) $(ACE_SHLIBS) $(LIBS)

PRELIB  = @true

# Test for template instantiation, add to SOFLAGS if SONAME set,
# add -E to LDFLAGS if using GNU ld
#
include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU

ifeq ($(GXX_4_OR_BETTER), 1)
  no_attributes ?= 1
  wall ?= 1
else
  no_attributes ?= 0
  wall ?= 0
endif

ifeq ($(no_attributes), 1)
  CFLAGS += -Wno-attributes
endif

ifeq ($(wall),1)
  CFLAGS += -Wall -W
endif

CCFLAGS += $(CFLAGS) $(TEMPLATES_FLAG)
