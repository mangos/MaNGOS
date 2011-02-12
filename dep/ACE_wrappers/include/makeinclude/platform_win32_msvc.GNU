# -*- Makefile -*-
# $Id: platform_win32_msvc.GNU 88756 2010-01-27 22:52:17Z mitza $

# TODO:
# - per-source-file flag_overrides (gnu.mpd)

# Include this file from platform_macros.GNU to build with sh and GNU make
# and the Visual C++ compiler and linker.
# Tested with the MSYS shell and make from msysCORE-1.0.11-20080826.tar.gz and
# the Visual C++ 9 toolchain (Visual C++ 2008 Express Edition) - SP1.

# Assuming the msysCORE archive is extracted to c:\msys...
# Start the Visual C++ Command Prompt
# Set ACE_ROOT (TAO_ROOT, CIAO_ROOT, DDS_ROOT)
# Add C:\msys\bin to PATH along with %ACE_ROOT%\lib and %ACE_ROOT%\bin
# If necessary, generate GNUmakefiles with MPC (set MPC_ROOT, use -type gnuace)
# Run "make"

# - library naming
#   - dynamic: ACE.dll, ACE.lib is the "import library"
#   - static:  ACE.lib
#   - no "s" suffix added for static, so static and dynamic can't coexist
#   - no other modifiers are added to the name (no ACEd.dll, ACEmfc.dll)
# - user-customizable make variables
#   - everything in the "Defaults" section (plus inline)
#   - SUBSYSTEM: defaults to CONSOLE for exes, WINDOWS for libs
#   - CRT_TYPE: defaults to D (dynamic) for shared libs, T (static) for static

# Defaults
debug       ?= 1
optimize    ?= 0
#inline determined below, defaults to !debug
CPU         ?= X86
winnt       ?= 1
winregistry ?= 1
wfmo        ?= 1
qos         ?= 1

# Programs
CC  = cl
CXX = $(CC)
RC  = rc
LD  = link
DLD = $(LD)
AR  = $(LD)

# File naming
EXEEXT = .exe
SOEXT  = dll
OBJEXT = obj
LIBEXT = lib
LIB_PREFIX =
versioned_so = 0

# Need forward slashes for paths going through sh.exe
ACE_ROOT := $(subst \,/,$(ACE_ROOT))

# Options
PIC =
CC_OUTPUT_FLAG = -Fo
CC_OUTPUT_FLAG_SEP =
CPPFLAGS += -DWIN32 -D_WINDOWS -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE
CFLAGS += -Zc:wchar_t -nologo -W3 -EHsc -GR -wd4355 -Fd$(INTDIR)
ARFLAGS = -lib -nologo -machine:$(CPU)
MSVC_LDFLAGS = -incremental:no -nologo -subsystem:$(SUBSYSTEM) -machine:$(CPU)
ifeq (,$(BIN_UNCHECKED))
  SUBSYSTEM ?= WINDOWS
else
  SUBSYSTEM ?= CONSOLE
  REALCLEAN_FILES += $(BIN).manifest $(BIN_UNCHECKED).pdb
endif
ifeq (1,$(optimize))
  CFLAGS += -O2
else
  CFLAGS += -Ob0
endif
ifeq (1,$(static_libs_only))
  static_libs = 1
endif
ifeq (1,$(static_libs))
  shared_libs = 0
  shared_libs_only = 0
  CRT_TYPE ?= T
  INTDIR = $(VDIR)
else
  static_libs = 0
  static_libs_only = 0
  CRT_TYPE ?= D
  ifneq (,$(BIN_UNCHECKED))
    INTDIR = $(VDIR)
  else
    INTDIR = $(VSHDIR)
  endif
  MSVC_IMPLIB = $(LIB_NAME).lib
  MSVC_DLLFLAGS += -dll -implib:$(MSVC_IMPLIB)
  REALCLEAN_FILES += $(LIB_NAME).dll $(INSLIB)/$(LIB_NAME).dll $(LIB_NAME).exp
  REALCLEAN_FILES += $(LIB_NAME).dll.manifest
endif
ifneq (,$(LIB_NAME))
  REALCLEAN_FILES += $(LIB_NAME).lib $(INSLIB)/$(LIB_NAME).lib $(LIB_NAME).pdb
endif
ifeq (1,$(debug))
  CPPFLAGS += -D_DEBUG
  CFLAGS += -Zi -M$(CRT_TYPE)d -Gy -Gm
  inline ?= 0
  MSVC_LDFLAGS += -DEBUG
  MSVC_DLLFLAGS += -pdb:$(subst dll,pdb,$@)
  MSVC_EXEFLAGS += -pdb:$(subst exe,pdb,$@)
else
  CPPFLAGS += -DNDEBUG
  CFLAGS += -M$(CRT_TYPE)
  inline ?= 1
endif
CCFLAGS = $(CFLAGS)
CXXFLAGS = $(CFLAGS)
RC_INCLUDE_FLAG = -I
RC_OUTPUT_FLAG = -fo
SYSTEMLIBS += advapi32.lib user32.lib
MSVC_LINK = $(LD) $(SYSTEMLIBS) $(MSVC_LDFLAGS) $(patsubst %\,%,$(patsubst -L%,-libpath:%, $(subst /,\,$(LDFLAGS)))) $(patsubst -l%,%.lib,$(ACE_SHLIBS)) $(LIBS) -out:$@
define SOLINK.cc.override
$(MSVC_LINK) $(MSVC_DLLFLAGS) $^
	mt -nologo -manifest $@.manifest -outputresource:$@\;2
	@if test . != $(INSLIB) -a $(call PWD) != $(INSLIB) -a -r $(MSVC_IMPLIB) ; then \
		cp $(MSVC_IMPLIB) $(INSLIB) ; \
	fi
endef
define LINK.cc.override
$(MSVC_LINK) $(MSVC_EXEFLAGS) $(filter-out %.lib,$(filter-out %.a,$^)) $(patsubst -l%,%.lib,$(VLDLIBS)) $(POSTLINK)
	@if test -r $@.manifest ; then \
		mt -nologo -manifest $@.manifest -outputresource:$@\;1 ; \
	fi
endef
AR.cc.override = $(AR) $(ARFLAGS) -out:$@ $^ $(AREXTRA)

#this is not mingw32, but the makefiles in ACE should act like it is
mingw32 = 1
