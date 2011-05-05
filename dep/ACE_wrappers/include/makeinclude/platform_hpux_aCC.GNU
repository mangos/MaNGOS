# $Id: platform_hpux_aCC.GNU 91285 2010-08-05 08:29:30Z johnnyw $
#
# This is for HP-UX 11.x using the aC++ compiler.
#
# HP-UX 11 supports both 32-bit and 64-bit builds, regardless of which
# architecture the build is done on.  This file is set up to build native
# to the machine it's running on.  To cross-build for another platform, add
# "buildbits=32" to make a 32-bit build, and "buildbits=64" to do a 64-bit
# build.  Note that if you explicitly specify a 64-bit build, the option
#  -Wl,+vnocompatwarnings is added to shut up the "At least one PA 2.0 object
# file detected..." messages.

buildbits ?= 0
debug ?= 1
distrib ?= 0
optimize ?= 0
stdcpplib ?= 1
threads ?= 1
versioned_so ?= 0

# First, extract the OS version number.
HPVERS_WORDS := $(subst ., ,$(shell uname -r))
HPMODEL := $(shell uname -m)
HPUX_VERS       := $(join $(word 2,$(HPVERS_WORDS)),$(word 3,$(HPVERS_WORDS)))
#
CC              = /bin/cc
CXX             = aCC

# Determine compiler version
CXXVERS_WORDS := $(subst ., , $(shell $(CXX) -V 2>&1))
CXXINFO := $(word 3, $(CXXVERS_WORDS))
# aCC++ 3.67 gives "aCC: HP ANSI C++ B3910B A.03.67"
# aCC++ 3.73 gives "aCC: HP ANSI C++ B3910B A.03.73"
#
ifeq (ANSI, $(CXXINFO))
  CXXMINORVERS := $(word 8, $(CXXVERS_WORDS))
  CXXMAJORVERS := $(word 7, $(CXXVERS_WORDS))
else
  ifeq (aC++/ANSI, $(CXXINFO))
    CXXMINORVERS := $(word 8, $(CXXVERS_WORDS))
    CXXMAJORVERS := $(word 7, $(CXXVERS_WORDS))
  else
    CXXMINORVERS := $(word 7, $(CXXVERS_WORDS))
    CXXMAJORVERS := $(word 6, $(CXXVERS_WORDS))
  endif
endif

ifeq (1,$(stdcpplib))
  CCFLAGS += -AA
  ifeq (03,$(CXXMAJORVERS))
    # Warning 930 is spurious when new(std::nothrow) is used. Reported to HP
    # as support call 3201224717. (Steve Huston, 23-Nov-2002)
    CCFLAGS += +W930
    # aCC 3.31 has poor member template support.
    ifeq (31,$(CXXMINORVERS))
      CCFLAGS += -DACE_LACKS_MEMBER_TEMPLATES
    endif
  endif
# Compile errors using -AA and +noeh together - HP problem report
# JAGad12608, fix slated for Dec 2001 Release -- Steve Huston, April 10, 2001.
else
  CCFLAGS += -AP
endif

# Set the appropriate preprocessor defs for threading based on OS version
# and specified compiler options.
ifeq ($(threads),1)
  THR_DEFS = -DACE_HAS_THREADS

  # x.30 was the first version offering -mt. So, if it's a .2x version,
  # set the defs by hand; anything else, use -mt. This leaves out .1x
  # versions, which should be no problem at this point.
  ifeq (,$(filter 2%,$(CXXMINORVERS)))
    CC_THR_DEFS += -mt
  else
    # Different for v2 vs. v1 C++ library selection.
    ifeq (1,$(stdcpplib))
      THR_DEFS += -D_RWSTD_MULTI_THREAD
    else
      THR_DEFS += -DRWSTD_MULTI_THREAD -D_THREAD_SAFE
    endif

    # For HP-UX 11.x, choose kernel threads over CMA (user) threads.
    ifeq ($(word 2,$(HPVERS_WORDS)), 11)
      THR_DEFS += -D_POSIX_C_SOURCE=199506L
    endif
  endif
else
  THR_DEFS = -DACE_HAS_THREADS=0
  # On HP-UX 11, even without building for threads, the *_r reentrant
  # functions are available, and _REENTRANT enables them.
  THR_DEFS += -D_REENTRANT
endif

ifeq ($(HPMODEL), ia64)
  itanium = 1
endif

ifeq (03,$(CXXMAJORVERS))
  # Suppress Warning 302 ((...) parameter list is a non-portable feature)
  # Suppress Warning 361 (missing return from non-void function)
  # Suppress Warning 1039(Binary incompatabilty between old and new compilers)
  CCFLAGS                += +W302,361,1039
  ifeq (57,$(CXXMINORVERS))
    # aCC 3.57 has a bug. When a class has private constructors
    # and is trying to be created from a friend class we end up with
    # a compile error. We disable array optimization in the compiler
    # by setting this environment variable. HP CR JAGaf43094
    export aCC_ARRAY_OPT=OFF
  endif
  ifeq (60,$(CXXMINORVERS))
    # Same bug as above
    export aCC_ARRAY_OPT=OFF
  endif
  ifeq (80,$(CXXMINORVERS))
    # Enabled template meta programming
    CCFLAGS += +hpxstd98
  endif
  ifeq (85,$(CXXMINORVERS))
    # Enabled template meta programming
    CCFLAGS += +hpxstd98
  endif
endif

ifeq (05,$(CXXMAJORVERS))
  # Warnings about binary incompatiblity
  CCFLAGS += +W1016,1031
endif

ifeq (06,$(CXXMAJORVERS))
  ifeq (13,$(CXXMINORVERS))
    # Warnings dllexport/dllimport conflict with "x" (declared at line y); dllexport assumed
    CCFLAGS += +W3390,3397
  endif
endif

CPPFLAGS        += $(THR_DEFS) -D_HPUX_SOURCE -DHPUX_VERS=$(HPUX_VERS) -DACE_LACKS_PRAGMA_ONCE
CCFLAGS         += $(CC_THR_DEFS)

DCCFLAGS        += -g
DLD             = $(CXX) $(CCFLAGS)
LD              = $(CXX)
OCCFLAGS        = -O
PIC             = +Z

AR              = /usr/ccs/bin/ar
ARFLAGS         = ruv
RANLIB          = echo
LDFLAGS         = -Wl,+s -z
SOFLAGS         += -b

ifeq ($(distrib),1)

ifeq ($(buildbits),32)
SOFLAGS         += -Wl,-x -Wl,+h$(@F) -Wl,+b/usr/lib
endif
ifeq ($(buildbits),64)
  ifeq ($(itanium),1)
    SOFLAGS     += -Wl,-x -Wl,+h$(@F) -Wl,+b/usr/lib/hpux64
  else
    SOFLAGS     += -Wl,-x -Wl,+h$(@F) -Wl,+b/usr/lib/pa20_64
  endif
endif

endif

ifeq ($(itanium),1)
 SOEXT		= so
else
 SOEXT          = sl
endif

ifeq ($(buildbits),32)
  ifeq ($(itanium),1)
    CFLAGS += +DD32
    CCFLAGS += +DD32
  else
    CFLAGS += +DA1.1 +DS1.1
    CCFLAGS += +DA1.1 +DS1.1
  endif
else
ifeq ($(buildbits),64)
  ifeq ($(itanium),1)
    CFLAGS += +DD64
    CCFLAGS += +DD64
  else
    ## Without the W, it isn't really a 64-bit build
    CFLAGS += +DA2.0W +DS2.0W
    CCFLAGS += +DA2.0W +DS2.0W
  endif
LDFLAGS         += -Wl,+vnocompatwarnings
else
  ifneq ($(itanium),1)
    # HP aCC on Itanium doesn't support this option, defaults to 32bit then.
    CFLAGS += +DAportable
    CCFLAGS += +DAportable
  endif
endif
endif

#11.x:
ifeq ($(threads),1)
  # If -mt is in CCFLAGS, we don't need to add anything to LIBS; -mt does it.
  ifeq ($(findstring -mt,$(CCFLAGS)),)
    ## -L./ must directly preceed the system libraries for 64-bit builds
    ## with aCC 3.27.  Without this change, libpthread.sl can not be found.
    LIBS += -L./ -lpthread
  endif
endif
LIBS          += -lxti -lrt -ldld

# The correct flags to pass to the linker for dynamic shared library
# versioning
#
ifneq ($(SONAME),)
  SOFLAGS += -Wl,+h -Wl,$(SONAME)
endif

# if _FILE_OFFSET_BITS==64 then the HP headers will produce
# spurious 600 warnings, except on 11.23
# Can also (possible) be fixed with patch PHKL_34670 on HP-UX 11.11
#
ifeq ($(64bitoffsets),1)
  ifneq ($(HPUX_VERS), 1123)
    CCFLAGS += +W600
  endif
endif
