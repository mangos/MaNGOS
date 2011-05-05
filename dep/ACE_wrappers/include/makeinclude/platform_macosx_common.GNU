# $Id: platform_macosx_common.GNU 91285 2010-08-05 08:29:30Z johnnyw $
# platform_macosx.GNU
# support for Mac OS X 10.4 (Tiger)
# By default, this uses the default compiler selected using gcc_select.

boost = 1
zzip = 1
zlib = 1
threads ?= 1
debug ?= 1
optimize ?= 0
ssl ?= 1
versioned_so ?= 0
universal ?= 0

ifeq (,$(no_hidden_visibility))
# Disable symbol visibility support by default.
#
# Apple's g++ 4 compiler doesn't appear to correctly support
# visibility attributes, at least as well as the vanilla g++.
  no_hidden_visibility = 1
endif

with_ld = macosx

pipes ?= 1

CFLAGS  += $(FLAGS_C_CC)

CC              ?= gcc
CXX             ?= g++
CFLAGS          += -Wall -Wpointer-arith 

DCFLAGS         += -g
DLD				= $(CXX)
LD				= $(CXX)
#LIBS += -lstdc++.6 -lSystem -lSystemStubs

OCFLAGS		   += -O2 
RANLIB          = ranlib
SOEXT			= dylib
SOFLAGS		   += -dynamiclib
SOBUILD			= -o $(VSHDIR)$*.dylib $<

# Test for template instantiation, add to SOFLAGS if versioned_so set,
# add -E to LDFLAGS if using GNU ld
ifeq ($(findstring g++,$(CXX)),)#
  include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU
else
  include $(ACE_ROOT)/include/makeinclude/platform_clang_common.GNU
endif

LDFLAGS         += -flat_namespace
CCFLAGS         += $(CFLAGS) $(TEMPLATES_FLAG)

ifeq ($(universal),1)
  CFLAGS += -arch i386 -arch ppc
  LDFLAGS += -arch i386 -arch ppc
endif
