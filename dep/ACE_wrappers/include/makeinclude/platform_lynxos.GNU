# $Id: platform_lynxos.GNU 91285 2010-08-05 08:29:30Z johnnyw $
#
# LynxOS with g++.  Defaults to LynxOS Version 4.0.0.  For Version 4.2.0,
# for example, add "VERSION=4.2.0" to your make invocation.

ifeq ($(shell uname -s),LynxOS)
  export VERSION=$(shell uname -r)

  # LynxOS's mkdir needs -f option to prevent it returning error
  # if directory already exists.
  ACE_MKDIR=mkdir -pf
else
  CROSS-COMPILE = 1

  ifeq (,$(VERSION))
    export VERSION = 4.0.0
  endif # VERSION
endif # ! LynxOS

LYNXOS_MAJOR = $(shell echo $(VERSION) | awk -F. '{print $$1;}')
LYNXOS_MINOR = $(shell echo $(VERSION) | awk -F. '{print $$2;}')

PLATFORM_XT_CPPFLAGS=
PLATFORM_XT_LIBS=-lXm -lXt
PLATFORM_XT_LDFLAGS=

PLATFORM_X11_CPPFLAGS=
PLATFORM_X11_LIBS=-lXp -lXpm -lXmu -lXext -lX11 -lSM -lICE
PLATFORM_X11_LDFLAGS=

PLATFORM_TCL_CPPFLAGS=$(shell . /usr/lib/tclConfig.sh && echo -n $$TCL_INC_DIR $$TCL_DEFS)
PLATFORM_TCL_LIBS=-ltcl8.0
PLATFORM_TCL_LDFLAGS=

PLATFORM_TK_CPPFLAGS=$(shell . /usr/lib/tkConfig.sh && echo -n $$TK_INC_DIR $$TK_DEFS) $(PLATFORM_X11_CPPFLAGS)
PLATFORM_TK_LIBS=-ltk8.0 $(PLATFORM_XT_LIBS) $(PLATFORM_X11_LIBS)
PLATFORM_TK_LDFLAGS=$(PLATFORM_X11_LDFLAGS)

shared_libs ?= 1
optimize ?= 1
debug ?= 1
pipes ?= 1
threads ?= 1

ifeq ($(threads),1)
  CFLAGS  += -mthreads
  SOFLAGS += -mthreads
endif

ifeq ($(rpc),1)
  LIBS    += -lrpc
endif

ifeq ($(shared_libs),1)
  CPPFLAGS += -DACE_HAS_SVR4_DYNAMIC_LINKING
  CFLAGS   += -mshared
  SOFLAGS  += -mshared
  LIBS     += -ldl
  ifeq ($(CROSS-COMPILE),1)
    LDFLAGS += -Wl,-rpath-link,$(ACE_ROOT)/lib
  endif
else
  LIBS     += -lnetinet -lnsl
endif

ifeq (5.0.0,$(VERSION))
  LIBS    += -lbsd
endif

CC              = gcc
CXX             = g++
PIC             = -fPIC
CFLAGS          += -Wpointer-arith -Wall -fsigned-char $(PIC)
CFLAGS          += -DACE_LYNXOS_MAJOR=$(LYNXOS_MAJOR) -DACE_LYNXOS_MINOR=$(LYNXOS_MINOR)
DCFLAGS         += -g
DLD             = $(CXX)
LD              = $(CXX)
OCFLAGS         += -O2
AR              = ar
ARFLAGS         = ruv
RANLIB          = @true
SOFLAGS         += $(CPPFLAGS) -shared
SOBUILD         = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<; \
                  $(SOLINK.cc) -o $@ $(LDFLAGS) $(VSHDIR)$*.o
PRELIB          = @true

# Test for template instantiation, add to SOFLAGS if SONAME set,
# add -E to LDFLAGS if using GNU ld
#
include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU

CCFLAGS         += $(CFLAGS) $(TEMPLATES_FLAG)

ifeq ($(CXX_MAJOR_VERSION), 3)
  LDFLAGS       += -L/usr/lib/thread
endif

# To save much disk space, strip all executables.  Comment the
# following line out if you want to debug.  Or, add "POSTLINK="
# to your make invocation.
ifeq ($(static_libs),1)
  POSTLINK       = ; strip $@
endif
