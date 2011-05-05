# $Id: platform_macosx.GNU 91285 2010-08-05 08:29:30Z johnnyw $
# platform_macosx.GNU
# support for Mac OS X 10.2 (jaguar), 10.3 (panther)
# Note: /sw/lib & /sw/include are inserted for the convience of Fink
#       users. Non-Fink users should simply create these directories to
#       eliminate the warnings.

threads ?= 1
debug ?= 1
optimize ?= 0
versioned_so ?= 0
pipes ?= 1
with_ld = macosx

CC              = gcc
CXX             = g++
CFLAGS          += -Wall -Wpointer-arith -Wno-long-double -I/sw/include

DCFLAGS         += -g
DLD             = libtool
LD              = $(CXX)
LDFLAGS		+= -L/sw/lib -flat_namespace -undefined warning
LIBS            += -lcc_dynamic -lstdc++ -lSystem

## dlcompat package (not part of base Darwin) is needed for dlopen() on 10.2.
## Fink installer puts libraries in /sw/lib and headers in /sw/include
## In order to install dlcompat do the following:
##   - download fink from http://fink.sf.net
##   - type:
##        fink install dlcompat
## 10.3 does not need this package.
LIBS            += -ldl
# 10.3 cannot do -03, this could be version dependent (probably on gcc)
OCFLAGS         += -O2
RANLIB          = ranlib
SOEXT		= dylib
SOFLAGS += -dynamic
SOBUILD = -o $(VSHDIR)$*.dylib $<

# Test for template instantiation, add to SOFLAGS if versioned_so set,
# add -E to LDFLAGS if using GNU ld
#
include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU

CCFLAGS         += $(CFLAGS) $(TEMPLATES_FLAG)
