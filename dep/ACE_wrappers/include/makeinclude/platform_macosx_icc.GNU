# $Id: platform_macosx_icc.GNU 91285 2010-08-05 08:29:30Z johnnyw $

threads ?= 1
debug ?= 1
optimize ?= 0
ssl ?= 1
versioned_so ?= 0
universal ?= 0
insure ?= 0

AR      = xiar

ifeq ($(insure),0)
  CC  = icc
  CXX = icpc
endif

ifndef CXX_VERSION
  CXX_VERSION := $(shell $(CXX) --version)
endif

ifeq (10.0,$(findstring 10.0,$(CXX_VERSION)))
  CFLAGS += -wd1684
endif
ifeq (10.1,$(findstring 10.1,$(CXX_VERSION)))
  CFLAGS += -wd1684
endif

no_hidden_visibility ?= 1

ifeq ($(inline),0)
  CPPFLAGS += -fno-inline
endif

ifeq ($(fast),1)
  CPPFLAGS += -fast
endif

ifeq ($(ipo),1)
  CPPFLAGS += -ipo
endif

CFLAGS  += -w1
ifeq ($(threads),1)
    CPPFLAGS  += -D_REENTRANT $(PLATFORM_AIO_SUPPORT)
endif # threads

CCFLAGS += $(CFLAGS) $(IMPLICIT_TEMPLATES_FLAG) -ip
DCFLAGS += -g -debug full
DLD     = $(CXX)
LD      = $(CXX)
LIBS    += -ldl
RANLIB = ranlib
ifeq ($(threads),1)
  LIBS += -lpthread
  ifeq (-DACE_HAS_AIO_CALLS,$(PLATFORM_AIO_SUPPORT))
    LIBS += -lrt
  endif
endif

OCFLAGS += -O3

# Disable floating point optimizer solves problem with
# min/max float values in the TAO_IDL compiler tests
# If these options are not passed we can get overflows
# when testing min/max
ifeq ($(SYSARCH),ia64)
  CPPFLAGS += -mp
else
  CPPFLAGS += -fp-model double
endif

ifeq ($(optimize),0)
  # Disable all optimizing in code
  CPPFLAGS += -O0
endif

SOEXT   = dylib
SOFLAGS += $(CPPFLAGS) -dynamiclib
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.dylib $<; \
          $(SOLINK.cc) -o $@ $(LDFLAGS) $(SOFLAGS) $(VSHDIR)$*.o
PRELIB  = @true

ifeq ($(shared_libs), 1)
  ifneq ($static_libs_only), 1)
    LDFLAGS +=

    ifneq ($(no_hidden_visibility),1)
      CCFLAGS += -fvisibility=hidden
    else
      CPPFLAGS += -DACE_HAS_CUSTOM_EXPORT_MACROS=0
    endif # no_hidden_visibility
  endif
endif


# Added line below to support "Executable Shared Object" files (as
# needed by the service configurator).
# Marius Kjeldahl <mariusk@sn.no, marius@funcom.com>
ifeq ($(threads),1)
    ESOBUILD = $(COMPILEESO.cc) $(PIC) -dynamiclib -o $(VSHDIR)$*.dylib $<
    ifndef PRELIB
       PRELIB = @true
    endif # ! PRELIB
endif
