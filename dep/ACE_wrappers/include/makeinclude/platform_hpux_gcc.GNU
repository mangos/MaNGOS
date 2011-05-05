# $Id: platform_hpux_gcc.GNU 91285 2010-08-05 08:29:30Z johnnyw $
#

buildbits ?= 64
debug ?= 1
distrib ?= 0
optimize ?= 1
stdcpplib ?= 1
threads ?= 1
versioned_so ?= 0
with_ld ?= hpux

# First, extract the OS version number.
HPVERS_WORDS := $(subst ., ,$(shell uname -r))
HPMODEL := $(shell uname -m)
HPUX_VERS	:= $(join $(word 2,$(HPVERS_WORDS)),$(word 3,$(HPVERS_WORDS)))
#
CC		= gcc
CXX		= g++
CFLAGS		+= -w

#
# Set the appropriate preprocessor defs for threading based on OS version.
# If the user has requested building without threads, then don't set any.
#
ifeq ($(threads),1)
  ifeq (3.,$(findstring 3.,$(CXX_VERSION)))
    THR_DEFS = -threads
  endif
  ifeq ($(word 2,$(HPVERS_WORDS)), 11)
    THR_DEFS += -DACE_HAS_THREADS -D_POSIX_C_SOURCE=199506L
  else
    THR_DEFS += -DACE_HAS_THREADS -D_REENTRANT
  endif
endif

ifeq ($(HPMODEL), ia64)
  itanium = 1
endif

# On HP-UX 11, there's a conflict with pthread_atfork in sys/unistd.h and
# sys/pthread.h - both define the function, but slightly differently.
# -fstrict-prototype resolves the issue.
#ifeq ($(word 2,$(HPVERS_WORDS)), 11)
#CCFLAGS		+= -fstrict-prototype
#endif
DCFLAGS		+= -g
DLD		= $(CXX)
LD		= $(CXX)
OCFLAGS		+= -O2
PIC		= -fPIC
# GNU ar doesn't work (missing libfl.sl?) so use HP ar.
AR              = /usr/bin/ar
ARFLAGS		= ruv
RANLIB		= echo
LDFLAGS		= -L. -L$(ACE_ROOT)/lib -Wl,+s
SOFLAGS		+= $(CPPFLAGS) -shared -nostdlib $(PIC) -Wl,-E
SOBUILD 	= $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.$(SOEXT) $<
PRELIB = @true

ifeq ($(itanium),1)
  SOEXT           = so
else
  SOEXT           = sl
endif

ifeq ($(itanium),1)
  ifeq ($(buildbits),64)
    CCFLAGS     += -mlp64
    CFLAGS     += -mlp64
  else
    CCFLAGS     += -milp32
    CFLAGS     += -milp32
  endif
endif

#
# Grab the appropriate libraries based on the version of HP-UX we're building
# on and whether or not thread support is being used.
#
ifeq ($(word 2,$(HPVERS_WORDS)), 11)

  #11.x:
  ifeq ($(threads),1)
    LIBS		+= -lxti -lpthread -lrt -ldld
  else
    LIBS		+= -lxti -lrt -ldld
  endif

else

  # 10.x:
  LIBS		+= -lxti -ldld
  ifeq ($(threads),1)
    LIBS		+= -lcma
  endif

endif

# Test for template instantiation, add to SOFLAGS if versioned_so set,
# add -E to LDFLAGS if using GNU ld
#
include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU

CCFLAGS		+= $(CFLAGS) $(TEMPLATES_FLAG) -DHPUX_VERS=$(HPUX_VERS) $(THR_DEFS)
