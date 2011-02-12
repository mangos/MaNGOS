# -*- Makefile -*-

# $Id: platform_sunos5_sunc++.GNU 92140 2010-10-04 12:37:52Z johnnyw $

# SunOS 5.x (Solaris 2.x) with Sun C++ 4.2 and 5.x
#
# NOTE: Sun C++ 5.0 users might need to add this to their CCFLAGS:
#   -compat=4 and/or remove -instances=explicit.  Please note that
#   inlining is disabled by default with Sun C++ 5.0.  If you'd like
#   to try enabling it, comment out the "inline = 0" line below.
#
# To enable compilation of 64-bit binaries with Sun CC 5.0, a
# platform_macros.GNU file such as this can be used:
#     fast=1
#     include $(ACE_ROOT)/include/makeinclude/platform_sunos5_sunc++.GNU
#     CFLAGS += -xtarget=ultra2 -xarch=v9a #### CPU specific!
#     SOFLAGS += -xtarget=ultra2 -xarch=v9a
#     LDFLAGS += -L/opt/SUNWspro/SC5.0/lib/v9 -L/usr/lib/sparcv9
# Please note that the xtarget and xarch settings are specific to
# the target CPU.  The /opt/SUNWspro/ Sun C++ installation directory
# is site-specific.
#
# To build 64-bit binaries with Forte 6 and later, build with the
# "buildbits=64" option (make buildbits=64).
#
# With slight modification, this file could be used with Sun C++ 4.1.
# However, it's likely that you won't be able to build all of ACE
# with Sun C++ 4.1.
#
# NOTE: some ACE files might generate this warning:
#   "Warning: Could not find source for " one of the following:
#    default constructor, copy constructor, destructor, or assignment
#    operator.  It appears that with +w, Sun C++ 4.x issues this warning
#    whenever it has to generate one of these functions (because it wasn't
#    defined by the user).  This isn't really a problem.
#    This warning appears to be fixed by Sun C++ 4.2 jumbo patch 104631-02.
#
# -pta instantiates all template members, which makes libraries bigger.
# But, it's supposed to put each member into it's own .o, so executable
# size should not be penalized.  We don't use it because it's not
# necessary with automatic template instantiation.

compat4 ?= 0
debug ?= 1
distrib ?= 1
rwtools ?= 0
stdcpplib ?= 1
stlport ?= 0
threads ?= 1
versioned_so ?= 1
tk_reactor ?= 0
ace_with_x11 ?= 0
no_annotations ?= 1
no_hidden_visibility ?= 1
templates ?= automatic

## If you set this to 1 you will need to add -xarch=v8plus (or higher)
## to CFLAGS or the assembler code will not compile
atomic_ops_sparc ?= 0

ifeq ($(threads),1)
  CFLAGS += -mt
  LDFLAGS += -mt
endif # threads

CC              = cc
CXX             = CC

CC_VERSION := $(shell $(CXX) -V 2>&1)

### CC 5.4 provides insufficient preprocessor output (at 111715-17 2005/10/13)
### We need to use cc instead - see bugzilla #2478.
ifeq (C++ 5.4,$(findstring C++ 5.4,$(CC_VERSION)))
  ACE_CC_PREPROCESSOR = cc
endif

ifeq ($(atomic_ops_sparc),1)
  CFLAGS += -DACE_HAS_BUILTIN_ATOMIC_OP -DACE_INCLUDE_ATOMIC_OP_SPARC
endif

#### The following macro overrides enable creation of fast executables.
#### They _don't_ support fast compilation :-)
#### To use, add fast=1 to your "make" invocation.
####
#### -g is incompatible with -fast.  If you need -g, you can
#### use "-fast -O3".
####
#### Have to do this _before_ adding any -xarch, -xtarget, etc. flags since
#### -fast is shorthand for a list of internally-assigned options, some of
#### which will are -xarch, -xtarget. Thus, explicit -xarch needs to go
#### to the right of -fast.
DCFLAGS         += -g
DCCFLAGS        +=
DLD             = $(CXX)
ifeq (1,$(fast))
  CFLAGS += -fast
  DCFLAGS =
  DCCFLAGS =
  LDFLAGS += -fast
endif # fast
# Make sure we resolve all symbols at link time, see bugzilla 2710
LDFLAGS         += -z now

ifeq (C++ 5,$(findstring C++ 5,$(CC_VERSION)))
  #### CC 5.0 or later
  #### Inlining appears to cause link problems with early releases of
  #### CC 5.0.
  ifeq (C++ 5.0,$(findstring C++ 5.0,$(CC_VERSION)))
      inline = 0
  else
    ifeq (C++ 5.1 ,$(findstring C++ 5.1 ,$(CC_VERSION)))
      inline = 0
    else # ! 5.1
      inline ?= 1
    endif # 5.2 or greater
  endif #! 5.0

  CCFLAGS += $(CFLAGS)

  #### If compat=4 is desired, set up for that. There are syntactic diffs
  #### in the compatibility modes, but here all we need to be concerned with
  #### is turning on -compat=4, and not trying to enable the new libraries.
  ifeq ($(compat4),1)
    CCFLAGS += -compat=4
    CCFLAGS += -features=rtti
  else
    ifneq (C++ 5.0,$(findstring C++ 5.0,$(CC_VERSION)))
      # If 64-bit build is desired, turn that on. If no code gen options
      # are given to Forte 6, the default build is 32-bit, even when
      # building on a 64-bit system. Specifying xarch=v9 generates code
      # that runs on any Sparc v9 (64-bit) processor.
      # Beginning at (I believe) Studio 10 (C++ 5.7) there's a
      # -xarch=generic64 option that works for both Sparc and x86.
      # If you want to tune this further, you should extract and insert
      # from CCFLAGS in a private platform_macros.GNU file.
      ifeq (64,$(buildbits))
        ifeq (C++ 5.7,$(findstring C++ 5.7,$(CC_VERSION)))
          CFLAGS += -xarch=generic64
          LDFLAGS += -xarch=generic64
        else
          ifeq (C++ 5.8,$(findstring C++ 5.8,$(CC_VERSION)))
            CFLAGS += -xarch=generic64
            LDFLAGS += -xarch=generic64
          else
            ifeq (C++ 5.9,$(findstring C++ 5.9,$(CC_VERSION)))
              CFLAGS += -m64
              LDFLAGS += -m64
            else
              ifeq (C++ 5.10,$(findstring C++ 5.10,$(CC_VERSION)))
                CFLAGS += -m64
                LDFLAGS += -m64
              else
                CFLAGS += -xarch=v9
                LDFLAGS += -xarch=v9
              endif # C++ 5.10
            endif # C++ 5.9
          endif # C++ 5.8
        endif # C++ 5.7
      endif # buildbits
    endif # CC 5.0
  endif # compat4 requested
endif # CC 5.0 or later

CCFLAGS         += $(TEMPLATES_FLAG)

ifeq ($(shared_libs), 1)
  ifneq ($(static_libs_only), 1)

   # Sun Studio 8 or better has version greater than 5.4 (Studio 8 == CC 5.5).
    SUN_STUDIO_8_OR_BETTER := $(shell a=`echo $(CC_VERSION) | sed -e 's/[^0-9]*\([0-9]*\)\.\([0-9]*\).*$$/\1\2/g'`; if test "$$a" -gt 54; then echo 1; else echo 0; fi)

    # Take advantage of Sun Studio 8 (v5.5) symbol scopes to
    # generate improved shared library binaries.
    ifeq ($(SUN_STUDIO_8_OR_BETTER),1)
      ifneq ($(no_hidden_visibility),1)
        CCFLAGS += -xldscope=hidden
      else
        CPPFLAGS += -DACE_HAS_CUSTOM_EXPORT_MACROS=0
      endif # no_hidden_visibility
    endif # Sun Studio 8 (C++ 5.5) or better.
  endif # static_libs_only
endif # shared_libs

LD              = $(CXX)
ifeq ($(distrib),0)
  LDFLAGS       += -R $(ACE_ROOT)/lib -R./
endif

# Get common Solaris settings
include $(ACE_ROOT)/include/makeinclude/platform_sunos5_common.GNU

LIBS            += -lsocket -lrt -laio -ldl -lnsl -lgen -lposix4 -ladm

# Support alternate CC libraries (i.e. STLport, Tools.h++, Cstd)
# See: http://docs.sun.com/app/docs/doc/820-7599/bkaty?a=view
ifeq ($(stlport),1)
  CC_LIBRARY = stlport4
  PLATFORM_STLPORT_CCFLAGS += -library=$(CC_LIBRARY)
  PLATFORM_STLPORT_LDFLAGS += -library=$(CC_LIBRARY)
else
  ifeq ($(rwtools),1)
    ifeq ($(stdcpplib),1)
      CC_LIBRARY = rwtools7_std
    else
      CC_LIBRARY = rwtools7,iostream
      CPPFLAGS += -DACE_USES_OLD_IOSTREAMS
    endif
  else  # default
    ifeq ($(stdcpplib),1)
      CC_LIBRARY = Cstd
    else
      CC_LIBRARY = no%Cstd,iostream
      CPPFLAGS += -DACE_USES_OLD_IOSTREAMS
    endif
  endif
  CCFLAGS += -library=$(CC_LIBRARY)
  LDFLAGS += -library=$(CC_LIBRARY)
endif

### CC 5.10 enables annotation support by default; allow user to override
### new behavior. This also provides a work around for 64-bit linker crashes,
### see: http://defect.opensolaris.org/bz/show_bug.cgi?id=9720
ifeq (C++ 5.10,$(findstring C++ 5.10,$(CC_VERSION)))
  ifeq ($(no_annotations),1)
    LDFLAGS += -xannotate=no
  endif
endif

LDFLAGS         += -xildoff

OCFLAGS         += -xO4
PIC             = -KPIC
AR              = CC
ARFLAGS         = -xar -o
RANLIB          = echo
SOFLAGS         += -G $(CPPFLAGS)

# The correct flags to pass to the linker for ELF dynamic shared library
# versioning. This knows that wrapper_macros.GNU will set versioned_so to
# 1 if it's not set. So, the only way SOFLAGS should _not_ get this is if
# versioned_so has been explicitly disabled.
#
ifeq ($(versioned_so),1)
  SOFLAGS += -h $(SONAME)
endif

ifdef CLEANUP_BIN
  #### Builds .shobj/$*.o file, then .shobj/$*.so file.  The .o files
  #### are used for building libraries and executables.  But, it seems
  #### to be necessary to build the .so files (with -G) in order to get
  #### all template instantiations.
  SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.o $<; \
            $(SOLINK.cc) -o $@ -h $@ $(LDFLAGS) $(VSHDIR)$*.o
else
  #### Optimize builds when no executables are built in the current
  #### directory.  Only a library is being created, and -G is used
  #### in that step.  Therefore, all templates instantations are
  #### included in the library.  This optimization saves almost 11 Mb
  #### (2.6 percent) and 27 minutes (22 percent) on the entire ACE
  #### build on a 168 MHz Sun Ultra2.
  ####
  #### Sun C++ won't allow the output from a compile to be named with
  #### a .so extension.  Rather than muck with the ACE build rules and
  #### risk upsetting builds on other platforms, just ln the output
  #### after building it.
  SOBUILD = $(RM) $@; $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.o $< && \
                      /bin/ln $(VSHDIR)$*.o $@
endif # CLEANUP_BIN
