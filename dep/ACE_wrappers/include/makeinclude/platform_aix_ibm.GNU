# $Id: platform_aix_ibm.GNU 91285 2010-08-05 08:29:30Z johnnyw $

# This file is for use with AIX 5.x and up, using either of the IBM
# compilers (Visual Age C++ 6 or newer compiler).

debug ?= 1
distrib ?= 0
optimize ?= 0
threads ?= 1
buildbits ?= 32

ifeq (,$(inline))
ifeq (1,$(debug))
  inline = 0
else
  inline = 1
endif
endif

# First, find out which compiler we're building with. The settings we
# use for compiling, as well as how to build shared libraries, depend on
# the compiler version. There's no easy way to ask lslpp which is installed.
# Instead, compile a file and see which compiler is set up for use. This
# trick was submitted by Craig Rodrigues <rodrigc@mediaone.net>, originally
# from the vacpp compiler newsgroup.
# It relies on the preprocessor defining __xlC__ to the proper version
# number of the compiler.
XLCVERSION := $(shell echo "__xlC__" > ./testxlCvers.cpp;xlC -E ./testxlCvers.cpp | tail -n 1;$(RM) testxlCvers.cpp)
#XLCVERSION := $(shell xlC -E ./testAIXCompilerVersion.cpp | tail -1')
#ACE_AIX_CLEAN := $(shell $(RM) ./testAIXCompilerVersion.cpp)

# In case anything here or in the config depends on OS version number,
# grab it here and pass it all to the compiler as well.
AIX_MAJOR_VERS := $(shell uname -v)
AIX_MINOR_VERS := $(shell uname -r)
AIX_VERS := $(AIX_MAJOR_VERS)0$(AIX_MINOR_VERS)

SOEXT = so
SOFLAGS += -G
LDFLAGS += -brtl
#SHR_FILTER=$(ACE_ROOT)/bin/aix_shr

ifeq ($(threads),1)
# Note - the _r form of the compiler command turns on -qthreads and the
# thread-required preprocessor defines, as well as -lpthreads for linking.
# If this is Visual Age C++ 5, DLD will get reset below.
CC	    = xlc_r
CXX	    = xlC_r
DLD	    = makeC++SharedLib_r
else
CC          = xlc
CXX         = xlC
DLD	    = makeC++SharedLib
endif

ARFLAGS	    = ruv
AR	    = ar
LD          = $(SHR_FILTER) $(CXX)
RANLIB	    = ranlib

CPPFLAGS    += -DACE_AIX_VERS=$(AIX_VERS)

# AIX 5.2 offers POSIX aio, but the Visual Age C++ compiler can't grok the
# aio.h header file. This is a known problem and will be addressed in
# May 2004 (per IBM). This is a workaround for it, supplied by IBM.
# When the compiler fix is released, this workaround (and the change
# to -qflag, below) can be removed. (Steve Huston)
ifeq (502,$(AIX_VERS))
CPPFLAGS += -U__C99_RESTRICT
endif

# -qinitauto seems like an interesting option, but some tests (specifically
# Process_Strategy_Test) hang with it turned on. This should be investigated
# at some point.
DCCFLAGS    += -g -qcheck=nobounds:div:null
DCFLAGS     += -g

ifeq ($(XLCVERSION),0x0600)
  # This is just for the workaround for aio, above. Without it, the
  # compiler warns that __C99_RESTRICT is reserved. When that workaround
  # is gone, this ifeq can be removed also. -qflag=w:w is what we want
  # in the end.
  CCFLAGS += -qtemplateregistry=templateregistry.$(MAKEFILE)
  ifeq (502,$(AIX_VERS))
    CCFLAGS    += -qflag=e:e
  else
    CCFLAGS    += -qflag=w:w
   endif
else
  ifeq ($(XLCVERSION),0x0700)
    CCFLAGS += -qflag=w:w
    ifeq ($(templates),manual)
      CCFLAGS += -qnotempinc -qnotemplateregistry
    else
      CCFLAGS += -qtemplateregistry=templateregistry.$(MAKEFILE)
    endif
    # According to documentation, default is -qeh, which is equivalent
    # to -qeh=v5.  Apparently 6.0 fixed some problems with nested
    # try-catch blocks.
    CCFLAGS     += -qeh=v6
  else
    ifeq ($(XLCVERSION),0x0800)
      CCFLAGS += -qflag=w:w
      ifeq ($(templates),manual)
        CCFLAGS += -qnotempinc -qnotemplateregistry
      else
        CCFLAGS += -qtemplateregistry=templateregistry.$(MAKEFILE)
      endif
      # According to documentation, default is -qeh, which is equivalent
      # to -qeh=v5.  Apparently 6.0 fixed some problems with nested
      # try-catch blocks.
      CCFLAGS     += -qeh=v6
    else
      ifeq ($(XLCVERSION),0x0900)
        CCFLAGS += -qflag=w:w
        ifeq ($(templates),manual)
          CCFLAGS += -qnotempinc -qnotemplateregistry
        else
          CCFLAGS += -qtemplateregistry=templateregistry.$(MAKEFILE)
        endif
        # According to documentation, default is -qeh, which is equivalent
        # to -qeh=v5.  Apparently 6.0 fixed some problems with nested
        # try-catch blocks.
        CCFLAGS     += -qeh=v6
      else
        CXX = echo "Unrecognized compiler version $(XLCVERSION)\n"
      endif
    endif
  endif
endif

DLD         = $(CXX) -qmkshrobj
SOFLAGS    += $(CCFLAGS) $(CPPFLAGS) $(INCLDIRS)

ifeq ($(buildbits),64)
  DLD     += -q64
  CFLAGS  += -q64 -qwarn64
  CCFLAGS += -q64 -qwarn64
  ARFLAGS := -X64 $(ARFLAGS)

  # This option results in a crash of TAO when building 64bit with
  # Visual Age 6. No idea why but don't use it when using Visual Age 6.
  ifneq ($(XLCVERSION),0x0600)
    # CCFLAGS += -qstaticinline
  endif
else
  # Using 32bit the staticinline option works.
  #CCFLAGS += -qstaticinline
endif

CCFLAGS += -qfuncsect

# The Visual Age preprocessor is not usable with the TAO_IDL compiler.
# At the moment an idl file just contains includes of other idl files
# the Visual Age preprocessor doesn't report that file in the list of
# files that is used. The only way we can workaround this is using the
# gcc preprocessor, do this in your platform_macros.GNU is this
# causes problems
# TAO_IDL_PREPROCESSOR = gcc

ifeq ($(exceptions),0)
  CCFLAGS     += -qnoeh
endif

CCFLAGS += -qrtti=all

ifeq ($(shared_libs),1)
ACELIB      = -lACE
else
ACELIB      = -lACEns
endif

LIBS	    += -lxti -ldl

SONAME	    =
SOVERSION   =
# Default OCCFLAGS builds common code for all RS/6000 architectures but
# this can be set to optimize for your particular computer. The commented
# out example optimizes for RS/6000 43P.
# OCCFLAGS is not used by default.  To used it, set optimize=1
# either in this file or on the command line.
#OCCFLAGS    += -qarch=ppc -qtune=604

OCCFLAGS    += -qlibansi -O2 -qarch=com
