# -*- Makefile -*-
# $Id: platform_g++_common.GNU 91626 2010-09-07 10:59:20Z johnnyw $

ifeq ($(insure),1)
  CC  = insure
  CXX = insure
else
  ifneq ($(CROSS_COMPILE),)
    # Build using the cross-tools
    CC  = ${CROSS_COMPILE}gcc
    CXX = ${CROSS_COMPILE}g++
    AR  = ${CROSS_COMPILE}ar
    # Cross-linker requires this for linked in shared libs that depend
    # themselves on other shared libs (not directly linked in)
    LDFLAGS += -Wl,-rpath-link,$(ACE_ROOT)/lib
    ifneq (,$(HOST_ROOT))
      TAO_IDLFLAGS += -g $(HOST_ROOT)/bin/ace_gperf
      TAO_IDL = $(HOST_ROOT)/bin/tao_idl
      TAO_IDL_DEP = $(TAO_IDL)
      TAO_IDL3_TO_IDL2 = $(HOST_ROOT)/TAO/CIAO/bin/tao_idl3_to_idl2
      TAO_IDL3_TO_IDL2_DEP = $(TAO_IDL3_TO_IDL2)
      # make sure to use the target compiler, not the cross-compiler
      # as preprocessor for the cross-compiled idl tools
      TAO_IDL_PREPROCESSOR = gcc
    endif
  endif
endif

ifneq ($(TCPU),)
  ifeq ($(tunemodelflag),1)
    FLAGS_C_CC += -mtune=$(TCPU)
  endif

  ifeq ($(cpumodelflag),1)
    FLAGS_C_CC += -mcpu=$(TCPU)
  endif

  ifeq ($(archmodelflag),1)
    FLAGS_C_CC += -march=$(TCPU)
  endif
endif

ifeq ($(CXX),insure)
  # insure does not pass through the -dumpversion option.
  CXX_FOR_VERSION_TEST ?= g++
else
  CXX_FOR_VERSION_TEST ?= $(CXX)
endif

CXX_VERSION := $(shell $(CXX_FOR_VERSION_TEST) -dumpversion)
CXX_MACHINE := $(shell $(CXX_FOR_VERSION_TEST) -dumpmachine)
ifeq (cmd,$(findstring cmd,$(SHELL)))
CXX_MAJOR_VERSION := $(firstword $(subst ., ,$(CXX_VERSION)))
else
CXX_MAJOR_VERSION := $(shell $(CXX_FOR_VERSION_TEST) -dumpversion | sed -e 's/[^0-9\.]//g' | sed -e 's/\..*$$//')
endif
ifeq ($(findstring $(CXX_MAJOR_VERSION),1 2 3),$(CXX_MAJOR_VERSION))
GXX_4_OR_BETTER := 0
else
GXX_4_OR_BETTER := 1
endif

# If no option has been specified, set templates to automatic
# version of the compiler.
#
templates ?= automatic

# The correct flags to pass to the linker for ELF dynamic shared library
# versioning.
# If the platform file didn't already set versioned_so, default to 1.
versioned_so ?= 1
with_ld ?=
ifeq ($(versioned_so),1)
  ifeq ($(with_ld),hpux)
    SOFLAGS += -Wl,+h -Wl,$(SONAME)
  else
    ifeq ($(with_ld),aix)
    # What to do here???
    else
      SOFLAGS += -Wl,-h -Wl,$(SONAME)
    endif
  endif
endif

static_libs_only ?=

CXX_FULL_VERSION := $(shell $(CXX_FOR_VERSION_TEST) --version)
gcc_template_instantiation_visibility ?= 0

# Only modify LDFLAGS if DLD has been set.
ifneq ($(DLD),)
  ifeq ($(DLD),$(CXX_FOR_VERSION_TEST)) # only try this is we are using ld through gcc
    LD_FOR_VERSION_TEST = $(shell $(CXX_FOR_VERSION_TEST) -print-prog-name=ld)
  else
    LD_FOR_VERSION_TEST = $(DLD)
  endif # DLD = CXX_FOR_VERSION_TEST
  # The -E option is GNU ld specific
  ifneq ($(mingw32),1)
    GNU_LD := $(shell sh -c '$(LD_FOR_VERSION_TEST) -v 2>&1 | grep -c "GNU ld"')
  else
    GNU_LD := $(if $(findstring GNU ld,$(shell $(LD_FOR_VERSION_TEST) -v)), 1, 0)
  endif # mingw32
endif # DLD

ifeq ($(GNU_LD),1)
  STATIC_LINK_FLAG ?= -static
endif # GNU_LD

ifeq ($(no_strict_aliasing), 1)
  CCFLAGS += -fno-strict-aliasing
endif

ifeq ($(shared_libs), 1)
  ifneq ($(static_libs_only), 1)
    # Add all symbols to the dynamic symbol table.  Needed to enable
    # dynamic_cast<> for shared libraries. (see
    # http://gcc.gnu.org/faq.html#dso)

    ifeq ($(GNU_LD),1)
      # Make sure this version of ld supports the -E option.
      ifneq ($(mingw32),1)
        LD_EXPORT_DEFINED := $(shell sh -c '$(LD_FOR_VERSION_TEST) -E 2>&1 | egrep -i "(option|flag)" /dev/null; echo $$?')
      else
        LD_EXPORT_DEFINED := $(shell $(LD_FOR_VERSION_TEST) -E 2>&1 | grep -c -i -e '(option|flag)')
	    ifeq ($(LD_EXPORT_DEFINED),0)
	      LD_EXPORT_DEFINED:=1
	    else
	      LD_EXPORT_DEFINED:=0
	    endif
      endif # mingw32
      ifeq ($(LD_EXPORT_DEFINED),1)
        LDFLAGS += -Wl,-E
      endif # LD_EXPORT_DEFINED = 1
    endif # GNU ld

    # Take advantage of G++ (>= 4.x) visibility attributes to generate
    # improved shared library binaries.
    ifeq ($(GXX_4_OR_BETTER), 1)
      ifneq ($(no_hidden_visibility),1)
        CCFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
        ifeq ($(gcc_template_instantiation_visibility),1)
          CCFLAGS += -DACE_GCC_HAS_TEMPLATE_INSTANTIATION_VISIBILITY_ATTRS=1
        endif # gcc_template_instantiation_visibility
      else
        CPPFLAGS += -DACE_HAS_CUSTOM_EXPORT_MACROS=0
      endif # no_hidden_visibility
    endif # GXX_4_OR_BETTER == 1
  else
    # Explicitly disable G++ (>= 4.x) visibility attributes since
    # they are only meant for use when building shared libraries.
    ifeq ($(GXX_4_OR_BETTER), 1)
      no_hidden_visibility = 1
      CPPFLAGS += -DACE_HAS_CUSTOM_EXPORT_MACROS=0
    endif # GXX_4_OR_BETTER == 1
  endif # static_libs_only
else
  # Explicitly disable G++ (>= 4.x) visibility attributes since
  # they are only meant for use when building shared libraries.
  ifeq ($(GXX_4_OR_BETTER), 1)
    no_hidden_visibility = 1
    CPPFLAGS += -DACE_HAS_CUSTOM_EXPORT_MACROS=0
  endif # GXX_4_OR_BETTER == 1
endif # shared_libs

# GCC 2.x needs this to compile TAO correctly 
ifeq ($(CXX_MAJOR_VERSION), 2)
  CCFLAGS += -ftemplate-depth-23
endif

minimaltoc ?= 0

ifeq ($(minimaltoc),1)
  FLAGS_C_CC += -mminimal-toc
endif


