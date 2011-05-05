# $Id: platform_aix.GNU 91285 2010-08-05 08:29:30Z johnnyw $

# AIX 5.x using the IBM C Set++ compiler.
#
# NOTE! IBM is no longer supporting AIX at any version earlier than 4.3,
# and Riverace is not testing or supporting ACE on them either. No steps
# have been taken to intentionally disable anything, but it is not being
# tested either.
#
# NOTE 2! This file is only for use with the older CSet++ compiler, which
# IBM doesn't sell or support any longer. Riverace doesn't support ACE
# with it either. This file is still here in case there is some site
# still using the old compiler, but it is completely unsupported.
#
# On 4.1, this file assumes that the user has installed the AIX patch
# containing the dl*() APIs.  To use these APIs, IBM has created a
# separate product (free to AIX licensees) called shared library
# hookable symbols (or slhs/6000).  If they don't have this patch, the
# sv* commands for compiling and linking will not be present on the
# system.
#
# On 4.2, the AIX fileset bos.rte.bind_cmds must be at version 4.2.0.2 or
# higher in order to build libACEshr.a, and at 4.2.0.4 or higher for dl*()
# routines to work properly. Best to apply the update available at:
# ftp://service.boulder.ibm.com/aix/fixes/v4/os/bos.rte.bind_cmds.4.2.0.4.bff
# or a successor.

debug ?= 1
distrib ?= 0
optimize ?= 0
threads ?= 1

# In case anything here or in the config depends on OS version number,
# grab it here and pass it all to the compiler as well.
AIX_MAJOR_VERS := $(shell uname -v)
AIX_MINOR_VERS := $(shell uname -r)

# aix_build turns on the funky shared library building code.
aix_build = 1
# ibmcxx_build turns on the extra step for instantiating and compiling
# template instances.
ibmcxx_build = 1

ifeq ($(threads),1)
CC          = xlc_r
CXX         = xlC_r
DLD         = makeC++SharedLib_r
else
CC          = xlc
CXX         = xlC
DLD         = makeC++SharedLib
endif

# -qextchk is handy, but produces false type mismatches when linking
# netsvcs with 3.1.4, so it's disabled.  IBM reports this fixed in 3.6.4.
TEMPINCDIR   = tempinc
CCFLAGS     += $(CFLAGS) -qtempinc=$(TEMPINCDIR) -qinfo

# IBM C/C++ compiler 3.6.x produces a bazillion warnings about 0-valued
# preprocessor defs. Since both 3.1 and 3.4 could be installed, don't ask
# lslpp for one or the other. Instead, compile a file and see which compiler
# the user has set up for use. This trick was submitted by Craig Rodrigues
# <rodrigc@mediaone.net>, originally from the vacpp compiler newsgroup.
# It relies on the preprocessor defining __xlC__ to the proper version
# number of the compiler.
XLCVERSION := $(shell echo "__xlC__" > ./testAIXCompilerVersion.cpp)
XLCVERSION := $(shell $(CXX) -E ./testAIXCompilerVersion.cpp | tail -1)
ACE_AIX_CLEAN := $(shell $(RM) ./testAIXCompilerVersion.cpp)
ifeq ($(XLCVERSION),0x0306)
 CCFLAGS    += -qflag=e:e
else
 CCFLAGS    += -qflag=w:w
endif
CPPFLAGS    += -qlanglvl=ansi -DACE_AIX_MAJOR_VERS=$(AIX_MAJOR_VERS) -DACE_AIX_MINOR_VERS=$(AIX_MINOR_VERS)

# -qinitauto seems useful, but when built with -qinitauto=5F, the
# Process_Strategy_Test went CPU bound in ostream, so I removed it (S. Huston)
DCFLAGS     += -g -qcheck=nobounds:div:null

ACELIB      = -lACE
LD          = $(CXX)

ifeq ($(AIX_MINOR_VERS),1)
LLIBS       = -lsvld -ltli_r $(ACELIB)
LIBS        += $(filter-out $(SHLIBA:lib%.a=-l%), $(LLIBS))
else
LIBS        += -ltli_r -ldl
endif

ARFLAGS     = ruv
AR          = ar
LDFLAGS     += -bI:/lib/pse.exp
# Default OCCFLAGS builds common code for all RS/6000 architectures but
# this can be set to optimize for your particular computer. The commented
# out example optimizes for RS/6000 43P.
# OCCFLAGS is not used by default.  To used it, set optimize=1
# either in this file or on the command line.
#OCCFLAGS    += -qarch=ppc -qtune=604
OCCFLAGS    += -O2 -qarch=com
RANLIB      = ranlib
SOFLAGS     = -p 0

SOVERSION  =
SONAME     =
