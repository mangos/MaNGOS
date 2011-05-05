# -*- Makefile -*-

#----------------------------------------------------------------------------
#  $Id: wrapper_macros.GNU 92151 2010-10-05 07:27:20Z johnnyw $
#----------------------------------------------------------------------------
#
#  ACE supports the following flags.  They can be enabled either on
#  the command line, e.g., "make purify=1", or added to your
#  platform_macros.GNU.  To disable the option, set the flag to null,
#  e.g., "make debug=".  Some flags support setting to 0 disable,
#  e.g., "make debug=0".  debug=1 is enabled in the platform files
#  that are released with ACE.
#
#  Please note that the effects of a flag may be platform specific.
#  Also, combinations of certain flags may or may not be allowed on
#  specific platforms, e.g., debug=1 opt=1 is supported by g++ but
#  not all other C++ compilers.
#
#  Flag             Description
#  ----             -----------
#  ace_for_tao      Build "ace_for_tao" subset.  Primarily useful for
#                   reducing the size of the ACE shared library when
#                   using TAO.
#  debug            Enable debugging; see DCFLAGS and DCCFLAGS.
#  dmalloc          Indicate build with dmalloc; adds PLATFORM_DMALLOC_CPPFLAGS
#                   to CPPFLAGS, PLATFORM_DMALLOC_LDFLAGS to LDFLAGS, and
#                   PLATFORM_DMALLOC_LIBS to LIBS. For more information on
#                   dmalloc, see http://dmalloc.com.
#  fast             Enable -fast option, e.g., with Sun C++.
#  inline           Enable ACE inlining.  Some platforms enable inlining by
#                   default, others do not. If platform_macros.GNU does not
#                   set this, inline is enabled.
#  minimum_corba    Enable minimumCORBA support.  (Documented here, but
#                   implemented in TAO/rules.TAO.GNU.)
#  mtrace           Indicate build with mtrace; adds PLATFORM_MTRACE_CPPFLAGS
#                   to CPPFLAGS. Mtrace is an extension in GNU libc.
#  optimize         Enable optimization; see OCFLAGS and OCCFLAGS.
#  probe            Enable ACE_Timeprobes.
#  profile          Enable profiling; see PCFLAGS and PCCFLAGS.
#  purify           Purify all executables.  purify must be in user's PATH!
#  quantify         Quantify all executables.  quantify must be in user's PATH!
#  repo             Use GNU template repository (g++ with repo patches and
#                   egcs only).
#  shared_libs      Build shared libraries. Ignored if static_libs_only is set.
#  static_libs      Build static libraries. Ignored if shared_libs_only is set.
#  shared_libs_only Only build shared libraries.  Ignored if no SHLIBs are
#                   specified by the Makefile, as in performance-tests/Misc.
#  static_libs_only Only build static libraries.
#  static_link      Link only static libs to executables, e.g., uses "-static"
#                   flag with the GNU ld linker.
#  symlinks         Make symbolic links (on appropriate platforms) in the lib
#                   directory be either "absolute" or "relative".
#  templates	    Type of template instantiation to perform. Can be one of
#                   automatic, used. Defaults to automatic.
#  threads          Build with thread support.
#  trio             Indicate build with trio; adds PLATFORM_TRIO_CPPFLAGS
#                   to CPPFLAGS, PLATFORM_TRIO_LDFLAGS to LDFLAGS, and
#                   PLATFORM_TRIO_LIBS to LIBS. For more information on
#                   trio, see http://sourceforge.net/projects/ctrio/
#  xt               Build with Xt (X11 Toolkit) support.
#  fl               Build with FlTk (Fast Light Toolkit) support.
#  fox              Build with Fox Toolkit support.
#  tk               Build with Tk (Tcl/Tk) support.
#  qt               Build with Qt (Trolltech Qt) support.
#  ssl              Build with OpenSSL support.
#  rapi             Build with RAPI
#  stlport          Build with STLPort support
#  rwho             Build with rwho, this results in building apps/drwho
#  pipes            Build with pipe support, resulting in adding -pipe to the
#                   compiler options.
#  sctp             Build support for TAO SCIOP and for ACE SCTP
#                   wrapper-facades. The sctp macro should be set to a string
#                   value representing a particular SCTP implementation.
#                   Recognized values include: openss7 lksctp
#
#  versioned_so     Add versioning to libraries.  Defaults to 1 (true). If 0,
#                   no version number is appended to shared library names.
#                   If versioned_so is 1 (enabled), the SOVERSION make
#                   variable can also be set to specify the version info
#                   to append to the name - it should have a leading '.' -
#                   else it defaults to .ACE_MAJOR.ACE_MINOR.ACE_BETA.
#  wfmo             Build with wfmo support (Win32 only)
#  winregistry      Build with windows registry support (Win32 only)
#  winnt            Build WinNT-specific projects (Win32 only)
#  link_groups      This indicates that statically linked executable library
#                   names will be surrounded by -Wl,--start-group and
#                   -Wl,--end-group for the GNU compiler only.
#
#  Usually, users do not need to be concerned with make targets.
#  Just enter "make" on the command line to build.  A few notable
#  targets are listed below.
#
#  Target             Description
#  ------             -----------
#  show_statics       Lists all static objects in object files built for
#                       current directory.  Only supported for g++.
#  show_uninit        Lists all uninitialized in object files built for
#                       current directory.  Only supported for g++.
#
#  The following describes the ACE wrapper macros:
#
#  Variable     Description
#  --------     -----------
#  ARFLAGS      Flags for the archive utility (ar)
#  CC           C compiler command
#  CXX          C++ compiler command
#  RC           Resource compiler command
#  COMPILE.c    Flags for compiling with C
#  COMPILE.cc   Flags for compiling with C++
#  COMPILE.rc   Flags for compiling with the resource compiler
#  CPPFLAGS     C pre-processor flags
#  CFLAGS       C compilation flags
#  CCFLAGS      C++ compilation flags
#  DCFLAGS      C compilation flags for debugging
#  DCCFLAGS     C++ compilation flags for debugging
#  DEFFLAGS     C++ preprocessor flag for defining symbols
#  DLD          Name of dynamic linker
#  LD           Name of linker
#  IDL          Name of the CORBA IDL compiler
#  TEMPLATES_FLAG
#               Flag to pass to the compiler to control the instantiation of
# 		templates.
#  INSBIN       Binary (executable) installation directory
#  INSINC       Include file installation directory
#  INSMAN       Manpage installation directory
#  INSLIB       Library installation directory
#  LDFLAGS      ld linker flags
#  LINK.c       Flags for linking with C
#  LINK.cc      Flags for linking with C++
#  MAKEFLAGS    Flags that are passed into the compilation from the commandline
#  OCFLAGS      Optimizing C compilation flags
#  OCCFLAGS     Optimizing C++ compilation flags
#  PCFLAGS      C compilation flags for profiling
#  PCCFLAGS     C++ compilation flags for profiling
#  PLATFORM_XT_CPPFLAGS
#               Platform CPP options for X11 (some require -I...)
#  PLATFORM_XT_LDFLAGS
#               Platform LD options for X11 (some require -L...)
#  PLATFORM_XT_LIBS
#               Platform libraries require with X11
#  PLATFORM_SSL_CPPFLAGS
#               Platform CPP options for OpenSSL (may need -I...)
#  PLATFORM_SSL_LDFLAGS
#               Platform LD options for OpenSSL (may need -L...)
#  PLATFORM_SSL_LIBS
#               Platform libraries required with OpenSSL
#  PLATFORM_RAPI_CPPFLAGS
#               Platform CPP options for RAPI
#  PLATFORM_RAPI_LDFLAGS
#               Platform LD options for RAPI
#  PLATFORM_RAPI_LIBS
#               Platform libraries required with RAPI
#  PLATFORM_SCTP_CPPFLAGS
#               Platform CPP options required for SCTP
#  PLATFORM_SCTP_LDFLAGS
#               Platform LD options required for SCTP
#  PLATFORM_SCTP_LIBS
#               Platform libraries required for SCTP
#  PRELINK      Executable to precede linking, such as quantify
#  PURELINK     If set, contains purify executable as well as all options
#  PURE_CACHE_BASE_DIR
#               The base directory in which Pure products will produce
#               instrumented libraries.  If you don't want that to be below
#               /tmp, set this variable in your environment or on the make
#               command line.
#  PURE_CACHE_DIR
#               The full name of the directory in which Pure produces will
#               produce instrumented libraries.
#  PWD          Command to get the current working directory
#  PTDIRS       Pathnames of directories containing template code
#  RM           Name of program to use to remove files
#  ACE_MKDIR    Name of program to (recursively) create directories
#  SOFLAGS      Flags used to build a shared library
#  SOLINK.cc    Link line necessary to build a share library
#  VAR          Variant identifier suffix
#  VDIR         Directory for object code
#  VSHDIR       Directory for shared object code
#  ACE_ROOT     Pathname for the root of the build tree
#
#----------------------------------------------------------------------------
#      Platform-dependent macro definitions
#      (link to the appropriate platform-specific config file).
#----------------------------------------------------------------------------

#### Default to building shared libraries only.  Set it here to give the
#### platform a chance to override.
ifndef shared_libs
  shared_libs = 1
endif # shared_libs
ifndef static_libs
  static_libs = 0
endif # static_libs

#### Set up the build ACE version in case the platform_macros.GNU wants it.
ifeq (cmd,$(findstring cmd,$(SHELL)))
ACE_MAJOR_VERSION := $(shell awk "/ACE_MAJOR_VERSION/ { print $$3}" ${ACE_ROOT}/ace/Version.h)
ACE_MINOR_VERSION := $(shell awk "/ACE_MINOR_VERSION/ { print $$3}" ${ACE_ROOT}/ace/Version.h)
ACE_BETA_VERSION := $(shell awk "/ACE_BETA_VERSION/ { print $$3}" ${ACE_ROOT}/ace/Version.h)
else
ACE_MAJOR_VERSION := $(shell awk '/ACE_MAJOR_VERSION/ { print $$3}' ${ACE_ROOT}/ace/Version.h)
ACE_MINOR_VERSION := $(shell awk '/ACE_MINOR_VERSION/ { print $$3}' ${ACE_ROOT}/ace/Version.h)
ACE_BETA_VERSION := $(shell awk '/ACE_BETA_VERSION/ { print $$3}' ${ACE_ROOT}/ace/Version.h)
endif

ACE_VERSION = $(ACE_MAJOR_VERSION).$(ACE_MINOR_VERSION).$(ACE_BETA_VERSION)

# Define some variables to silence warnings
SHR_FILTER ?=

# Include this before the below variables to make it possible for
# platform_macros.GNU to set default values for them.
ifneq ($(alternate_platform_macros),)
  include $(alternate_platform_macros)
else
  include $(ACE_ROOT)/include/makeinclude/platform_macros.GNU
endif # alternate_platform_macros

# Define some variables to silence warnings
GHS ?=
CHORUS ?=
AIX_TEMPLATE_HACK ?=

# $(ACE_PLATFORM_CONFIG) is used in dependency rules and corresponds to the
# platform dependent config-*.h file included in config.h, and can be set
# in your platform_*.GNU file or platform_macros.GNU.
ACE_PLATFORM_CONFIG ?= config.h

ifeq ($(threads),0)
  ifneq ($(findstring -DACE_MT_SAFE=0,$(CPPFLAGS)),-DACE_MT_SAFE=0)
    CPPFLAGS += -DACE_MT_SAFE=0
  endif # -DACE_MT_SAFE=0
endif # threads

####
#### Defaults are fast off (0), threads on (1)
#### and versioned_so on (1).
####
fast ?= 0
versioned_so ?= 1
use_dep_libs ?= 1
threads ?= 1
symlinks ?= relative
link_groups ?= 0
mcpp ?= 0

SOVERSION ?=
SONAME ?=

ifeq ($(PWD),)
  PWD=$(shell pwd)
endif

ifeq ($(ACE_MKDIR),)
  MKDIR = mkdir -p
else
  MKDIR = $(ACE_MKDIR)
endif

#----------------------------------------------------------------------------
#      Platform-independent macro definitions
#----------------------------------------------------------------------------

INCLDIRS ?=
DEFFLAGS ?=

ifeq ($(debug),0)
  DEFFLAGS        += -DNDEBUG
endif

ifeq (,$(findstring -I$(ACE_ROOT),$(INCLDIRS)))
  INCLDIRS        += -I$(ACE_ROOT)
endif

CPPFLAGS          += $(DEFFLAGS) $(INCLDIRS)

# Define default extensions for IDL-generated files
IDL_CLIENT_HDR_EXT ?= C.h
IDL_CLIENT_INL_EXT ?= C.inl
IDL_CLIENT_SRC_EXT ?= C.cpp
IDL_SERVER_HDR_EXT ?= S.h
IDL_SERVER_INL_EXT ?= S.inl
IDL_SERVER_SRC_EXT ?= S.cpp
IDL_SERVER_THDR_EXT ?= S_T.h
IDL_SERVER_TINL_EXT ?= S_T.inl
IDL_SERVER_TSRC_EXT ?= S_T.cpp

ifeq (default,$(origin LEX))
LEX = flex
endif

EXEEXT ?=

ifndef COMSPEC
  ifdef ComSpec
    #### ACE+TAO use COMSPEC, but ComSpec is defined.
    COMSPEC = $(ComSpec)
  endif # ComSpec
endif # ! COMPSPEC

ifdef COMSPEC
  #### Assume we're on a WIN32 host.
  ACE_NUL = nul
else  # ! WIN32
  ACE_NUL = /dev/null
endif # ! WIN32

#### Redirect stderr to nul device, and mask exit status.
ACE_NUL_STDERR = 2>$(ACE_NUL) || true
PCFLAGS         += #### set in platform_macros.GNU file, e.g., -p
PCCFLAGS        += #### set in platform_macros.GNU file, e.g., -p
VAR             ?=

# User-written makefiles that are not MPC-generated often use LIB to specify
# what to build. This is in contrast to MPC-generated files that use
# LIB_CHECKED for this purpose. LIB causes conflicts on Windows with gmake
# and Visual Studio which uses the LIB environment variable as a search path
# for libraries to link. So, to allow non-MPC makefiles to keep working, set
# LIB_CHECKED based on LIB if the former is not set and the latter is.
ifndef COMSPEC
  LIB_CHECKED ?= $(LIB)
endif # ! COMPSPEC (and, therefore, assumed !Windows

# default to not using an architecture-specific output directory
ARCH ?=

ifdef ARCH
  INSLIB := $(INSLIB:%=%/$(ARCH))

  # ADD_ARCH is a function used with $(call) to "ARCHIFY" a variable which
  # contains the name and path of an executable or library.  If the file is not
  # located in the ARCH subdir, the non-ARCH location is used.  First we need
  # some helper functions to deal with quoting/protecting spaces.
  # ADD_ARCH can return a path containing a space, the caller must quote it or
  # escape the space, depending on the context.
  PROT_SPC = $(subst $(SPACE),?,$(1))
  UNPROT_SPC = $(subst ?,$(SPACE),$(1))
  FILE_EXISTS = $(wildcard $(subst $(SPACE),\$(SPACE),$(1)))
  ADD_ARCH_HLP = $(call UNPROT_SPC,$(dir $(1))$(ARCH)/$(notdir $(1)))
  ARCH_FILE = $(call FILE_EXISTS,$(call ADD_ARCH_HLP,$(call PROT_SPC,$(1))))
  ADD_ARCH = $(if $(call ARCH_FILE,$(1)),$(call ARCH_FILE,$(1)),$(call FILE_EXISTS,$(1)))

  ARCH_BIN     = $(if $(BIN),$(ARCH)/$(BIN))
  ARCH_BIN2    = $(if $(BIN2),$(ARCH)/$(BIN2))
  ARCH_LIB     = $(if $(LIB_CHECKED),$(ARCH)/$(LIB_CHECKED))
  ARCH_LIB2    = $(if $(LIB2),$(ARCH)/$(LIB2))
  ARCH_SHLIB   = $(if $(SHLIB),$(ARCH)/$(SHLIB))
  ARCH_SHLIB2  = $(if $(SHLIB2),$(ARCH)/$(SHLIB2))
  ARCH_SHLIBA  = $(if $(SHLIBA),$(ARCH)/$(SHLIBA))
  ARCH_SHLIBA2 = $(if $(SHLIBA2),$(ARCH)/$(SHLIBA2))

else #ARCH is not defined

  ADD_ARCH = $(1)

  ARCH_BIN     = $(BIN)
  ARCH_BIN2    = $(BIN2)
  ARCH_LIB     = $(LIB_CHECKED)
  ARCH_LIB2    = $(LIB2)
  ARCH_SHLIB   = $(SHLIB)
  ARCH_SHLIB2  = $(SHLIB2)
  ARCH_SHLIBA  = $(SHLIBA)
  ARCH_SHLIBA2 = $(SHLIBA2)

endif #ARCH

# for use with the result of ADD_ARCH
ADD_QUOTE = $(if $(findstring $(SPACE),$(1)),"$(1)",$(1))


INSINC      ?= $(ACE_ROOT)/ace
INSLIB      ?= $(ACE_ROOT)/lib
INSMAN      ?= $(ACE_ROOT)/man

ifeq (,$(findstring -L$(INSLIB),$(LDFLAGS)))
  LDFLAGS         += -L$(INSLIB)
endif

#ifdef ARCH
#  LDFLAGS_ARCH := $(LDFLAGS:-L%=-L%/$(ARCH))
#  LDFLAGS += $(LDFLAGS_ARCH)
#endif

# Set the cleanup targets.  These targets are used by realclean even if
# the component isn't being built, i.e., the decision to build it was based
# on components built in another makefile and written a file (that might no
# longer be available) and accessed by $(ACE_ROOT)/bin/ace_components.
#
# If a component should always be built, just use the normal BIN, LIB, or
# SHLIB variables.  If it's dependent on a component, use the BIN2 or
# BIN_UNCHECKED variety, then assign that to BIN if the required component
# is available.

BIN_UNCHECKED ?=
CLEANUP_BIN ?=
ifdef BIN
  CLEANUP_BIN = $(ARCH_BIN)
endif # !BIN
ifdef BIN2
  CLEANUP_BIN += $(ARCH_BIN2)
endif # !BIN2
ifneq ($(BIN_UNCHECKED),)
  CLEANUP_BIN += $(BIN_UNCHECKED)
endif # !BIN_UNCHECKED

LIB_UNCHECKED ?=
CLEANUP_LIB ?=
ifdef LIB_CHECKED
  CLEANUP_LIB = $(ARCH_LIB)
endif # !LIB_CHECKED
ifdef LIB2
  CLEANUP_LIB += $(ARCH_LIB2)
endif # !LIB2
ifneq ($(LIB_UNCHECKED),)
  CLEANUP_LIB = $(LIB_UNCHECKED)
endif # !LIB_UNCHECKED

SHLIB_UNCHECKED ?=
CLEANUP_SHLIB ?=
ifdef SHLIB
  CLEANUP_SHLIB = $(ARCH_SHLIB)
endif # !SHLIB
ifdef SHLIB2
  CLEANUP_SHLIB += $(ARCH_SHLIB2)
endif # SHLIB2
ifneq ($(SHLIB_UNCHECKED),)
  CLEANUP_SHLIB += $(SHLIB_UNCHECKED)
endif # ! SHLIB_UNCHECKED

SHLIBA_UNCHECKED ?=
CLEANUP_SHLIBA ?=
ifdef SHLIBA
  CLEANUP_SHLIBA = $(ARCH_SHLIBA)
endif # SHLIBA
ifdef SHLIBA2
  CLEANUP_SHLIBA = $(ARCH_SHLIBA2)
endif # SHLIB2A
ifneq ($(SHLIBA_UNCHECKED),)
  CLEANUP_SHLIBA = $(SHLIBA_UNCHECKED)
endif # SHLIBA_UNCHECKED

# Let users override the default VDIR directories
ifdef ARCH
VDIR          ?= $(ARCH)/.obj/
VSHDIR        ?= $(ARCH)/.shobj/
else
VDIR          ?= .obj/
VSHDIR        ?= .shobj/
endif

# NOTE: VLDLIBS is defined below, in case it is overridden for static-
#       or shared-only builds.
ifeq (default,$(origin YACC))
YACC = bison
endif

#----------------------------------------------------------------------------
#      Platform-dependent macros that require platform_macros.GNU.
#----------------------------------------------------------------------------

OBJEXT ?= o
SOEXT ?= so
LIBEXT ?= a
LIB_PREFIX ?= lib

CC_OUTPUT_FLAG ?= -o
SO_OUTPUT_FLAG ?= -o
LINK_OUTPUT_FLAG ?= -o

#----------------------------------------------------------------------------
#      make flags
#----------------------------------------------------------------------------

ifeq ($(optimize),0)
  override optimize =
endif # optimize

ifneq ($(optimize),)
  CFLAGS  += $(OCFLAGS)
  CCFLAGS += $(OCCFLAGS)
endif # optimize

ifeq ($(debug),0)
  override debug =
endif # debug

ifeq ($(debug),)
  CPPFLAGS += -DACE_NDEBUG
# Disable the RCSID for release/non-debug builds.
  ifeq (,$(findstring ACE_USE_RCSID,$(CPPFLAGS)))
    CPPFLAGS += -DACE_USE_RCSID=0
  endif
else
  CFLAGS  += $(DCFLAGS)
  CCFLAGS += $(DCCFLAGS)
endif # debug

dmalloc ?=
ifeq ($(dmalloc),0)
  override dmalloc =
endif # dmalloc

ifdef dmalloc
  CPPFLAGS += $(PLATFORM_DMALLOC_CPPFLAGS)
  LDFLAGS += $(PLATFORM_DMALLOC_LDFLAGS)
  LIBS += $(PLATFORM_DMALLOC_LIBS)
endif # dmalloc

mtrace ?=
ifeq ($(mtrace),0)
  override mtrace =
endif # mtrace

ifdef mtrace
  CPPFLAGS += $(PLATFORM_MTRACE_CPPFLAGS)
endif # mtrace

ifeq ($(threads),0)
  override threads =
endif # threads

trio ?=
ifeq ($(trio),0)
  override trio =
endif # trio

ifdef trio
  CPPFLAGS += -DACE_HAS_TRIO $(PLATFORM_TRIO_CPPFLAGS)
  LDFLAGS += $(PLATFORM_TRIO_LDFLAGS)
  LIBS += -ltrio $(PLATFORM_TRIO_LIBS)
  ACE_MAKE_OPTIONS += trio
endif # trio

# ace_for_tao subsetting is disabled by default.
ace_for_tao ?=
ifeq ($(ace_for_tao),0)
  override ace_for_tao =
endif # ace_for_tao

ifneq ($(ace_for_tao),)
  ACE_MAKE_OPTIONS += ace_for_tao
endif # ace_for_tao


ifeq ($(xt),0)
  override xt =
endif # xt

ifneq ($(xt),)
  override x11=1
  override xt=1
  override ace_xtreactor=1
  override tao_xtresource=1
  ACE_MAKE_OPTIONS += xt
endif # xt

fl ?=
ifeq ($(fl),0)
  override fl =
endif # fl

ifneq ($(fl),)
  override x11=1
  override gl=1
  override ace_flreactor=1
  override tao_flresource=1
  ACE_MAKE_OPTIONS += fl
endif # fl

tk ?=
ifeq ($(tk),0)
  override tk =
endif # tk

ifneq ($(tk),)
  override x11=1
  override tk=1
  override ace_tkreactor=1
  override tao_tkresource=1
  ACE_MAKE_OPTIONS += tk
endif # tk

qt ?=
ifeq ($(qt),0)
  override qt =
endif # qt

ifneq ($(qt),)
  override qt=1
  override ace_qtreactor=1
  override tao_qtresource=1
  ACE_MAKE_OPTIONS += qt
endif # qt
gtk_reactor ?=

ifeq ($(gtk_reactor),0)
  override gtk_reactor =
endif # gtk_reactor

ifneq ($(gtk_reactor),)
  override ace_with_gtk=1
  CPPFLAGS += -DACE_HAS_GTK $(PLATFORM_GTK_CPPFLAGS)
  LIBS += $(PLATFORM_GTK_LIBS)
  LDFLAGS += $(PLATFORM_GTK_LDFLAGS)
  ACE_MAKE_OPTIONS += gtk_reactor
endif # gtk_reactor

fox ?=
ifeq ($(fox),0)
  override fox =
endif # fox

ifneq ($(fox),)
  override x11=1
  override gl=1
  override ace_foxreactor=1
  override tao_foxresource=1
  ACE_MAKE_OPTIONS += fox
endif #fox

QTDIR ?=
ifneq ($(QTDIR),)
  MOC = ${QTDIR}/bin/moc
else
  MOC = moc
endif

rapi ?=
ifeq ($(rapi),0)
   override rapi =
endif #rapi

ifneq ($(rapi),)
  CPPFLAGS += -DACE_HAS_RAPI $(PLATFORM_RAPI_CPPFLAGS)
  LIBS += $(PLATFORM_RAPI_LIBS)
  LDFLAGS += $(PLATFORM_RAPI_LDFLAGS)
  ACE_MAKE_OPTIONS += rapi
endif # rapi

sctp ?=
ifeq ($(sctp),0)
     override sctp =
endif #rapi

# if any sctp implementation is present then add
ifneq ($(sctp),)
  CPPFLAGS += -DACE_HAS_SCTP $(PLATFORM_SCTP_CPPFLAGS)
  LDFLAGS  += $(PLATFORM_SCTP_LDFLAGS)
  LIBS     += $(PLATFORM_SCTP_LIBS)
  ACE_MAKE_OPTIONS += sctp
endif

gl ?=
ifeq ($(gl),0)
  override gl =
endif # gl

ifneq ($(gl),)
  ACE_MAKE_OPTIONS += gl
endif # gl

ssl ?= 0
ifeq ($(ssl),0)
  override ssl =
else
  ifneq ($(SSL_ROOT), /usr)
    ifneq ($(SSL_ROOT),)
      PLATFORM_SSL_CPPFLAGS += -I$(SSL_ROOT)/include
      PLATFORM_SSL_LDFLAGS  += -L$(SSL_ROOT)/lib
    endif
  endif
  PLATFORM_SSL_LIBS ?= -lssl -lcrypto
  ACE_MAKE_OPTIONS  += ssl
endif

## These are related to MPC.  When 'requires' is used in a project
## it controls whether or not the project is generated.  There is also
## a second level of control in platform macros and thus requires
## some features to be set here as well.
ec_typed_events ?= 1
zlib ?= 1

valgrind ?=
ifeq ($(valgrind),1)
  CPPFLAGS += -DACE_HAS_VALGRIND
endif

profile ?=
ifeq ($(profile),0)
  override profile =
endif # profile

ifneq ($(profile),)
  CFLAGS  += $(PCFLAGS)
  CCFLAGS += $(PCCFLAGS)
endif # profile

inline ?= 1
ifeq ($(inline),0)
  CPPFLAGS += -DACE_NO_INLINE
else
ifeq ($(inline),1)
  CPPFLAGS += -D__ACE_INLINE__
endif # inline eq 1
endif # inline eq 0

ifeq ($(shared_libs),0)
  override shared_libs =
endif # shared_libs
ifeq ($(static_libs),0)
  override static_libs =
endif # static_libs

shared_libs_only ?=
ifeq ($(shared_libs_only),0)
  override shared_libs_only =
endif # shared_libs_only

static_libs_only ?=
ifeq ($(static_libs_only),0)
  override static_libs_only =
endif # static_libs_only

ifdef shared_libs_only
  ifdef static_libs_only
    $(error Both static_libs_only and shared_libs_only defined.)
  else
    override shared_libs = 1
    override static_libs =
  endif
endif # shared_libs_only
ifdef static_libs_only
  override shared_libs =
  override static_libs = 1
endif # static_libs_only

ifdef shared_libs
  ifndef static_libs
    override shared_libs_only = 1
  endif
  ACE_MAKE_OPTIONS += shared_libs
endif

ifdef static_libs
  ifndef shared_libs
    override static_libs_only = 1
  endif
  ACE_MAKE_OPTIONS += static_libs
endif

ACE_SHLIBS ?=

ifdef shared_libs
  ifdef SHLIBA
    LDLIBS := $(LDLIBS:-l%=-l%shr)
    ACE_SHLIBS := $(ACE_SHLIBS:-l%=-l%shr)
  endif # SHLIBA
else # ! shared_libs
  ifdef static_libs
    #### Don't build shared libs.
    PIC             =
    SHLIB           =
    SOEXT           = o
    VSHDIR          = $(VDIR)
    VLDLIBS         = $(LDLIBS:%.so=%.o)
  endif # static_libs
endif # shared_libs

VLDLIBS = $(LDLIBS) $(LIBS)

probe ?=
ifeq ($(probe),0)
  override probe =
endif # probe

ifeq ($(probe),1)
    CPPFLAGS += -DACE_COMPILE_TIMEPROBES
endif # probe

purify ?=
ifeq ($(purify),0)
  override purify =
endif # purify

ifdef purify
PURE_CACHE_BASE_DIR = /tmp/purifycache
PURE_CACHE_DIR = $(PURE_CACHE_BASE_DIR)-$(LOGNAME)-$(notdir $(CXX) )

  #### Pick up Purify directory from the users PATH.
  ACE_PURIFY_DIR := \
    $(shell type purify | sed -e 's/.* is //' -e 's%/purify$$%%')

  #### You might want to adjust the Purify options below.
  #### -best-effort is undocumented but supported, and seems
  #### to help avoid occasional link failure.
  PURELINK += purify -best-effort -cache-dir=$(PURE_CACHE_DIR) \
              -chain-length=20 -fds-inuse-at-exit=no \
              -inuse-at-exit -max_threads=100 $(PLATFORM_PURIFY_OPTIONS)
  CPPFLAGS += -DACE_HAS_PURIFY -I$(ACE_PURIFY_DIR)
endif # purify

quantify ?=
ifeq ($(quantify),0)
  override quantify =
endif # quantify

ifdef quantify
  #### Pick up Quantify directory from the users PATH.
  ACE_QUANTIFY_DIR := \
    $(shell type quantify | sed -e 's/.* is //' -e 's%/quantify$$%%')

  #### You might want to adjust the Quantify options below.
  #### -best-effort is undocumented but supported, and seems
  #### to help avoid occasional link failure.
  PURELINK += quantify -best-effort -cache-dir=$(PURE_CACHE_DIR) \
              -max_threads=100 $(PLATFORM_QUANTIFY_OPTIONS)
  CPPFLAGS += -DACE_HAS_QUANTIFY -I$(ACE_QUANTIFY_DIR)

  ifeq ($(shell uname -p),sparc)
    PURELINK += -record-register-window-traps
  endif # sparc
endif # quantify

repo ?=
ifeq ($(repo),0)
  override repo =
endif # repo

ifdef repo
  #### Remove -fno-implicit-templates from, and add -frepo to, CCFLAGS.
  CCFLAGS := $(strip $(subst -fno-implicit-templates,,$(CCFLAGS)))
  CCFLAGS += -frepo
  CPPFLAGS += -DACE_HAS_GNU_REPO
#  ace_lib_prelink := 1
endif # repo

# If pipes is not set, we default to 0
pipes ?= 0

# If pipes is set to 1 then add the compiler flag -pipe. This uses pipe instead
# of intermediate files. This can be disabled by setting pipes to 0 in the
# platform_macros.GNU file.
ifeq ($(pipes),1)
   CFLAGS += -pipe
   CCFLAGS += -pipe
endif # pipes

# RMCast is turned on by default
rmcast ?= 1
ifeq ($(rmcast),1)
  ACE_MAKE_OPTIONS += rmcast
endif

# If not explicitly is set that we don't have rwho, assume we have it.
rwho ?= 1

# If not explicitly is set that we don't have stlport, assume we
# don't have it.
stlport ?= 0
ifeq ($(stlport),1)
  CPPFLAGS += -DACE_HAS_STLPORT $(PLATFORM_STLPORT_CPPFLAGS)
  CCFLAGS  += $(PLATFORM_STLPORT_CCFLAGS)
  LDFLAGS  += $(PLATFORM_STLPORT_LDFLAGS)
  LIBS     += $(PLATFORM_STLPORT_LIBS)
endif

# If not explicitly is set that we have wfmo, assume we don't have it.
wfmo ?= 0

# If not explicitly is set that we have winregistry, assume we don't have it.
winregistry ?= 0

# If not explicitly is set that we have winnt, assume we don't have it.
winnt ?= 0

# In MPC, these are now features that are enabled by default.
ace_other     ?= 1
ace_codecs    ?= 1
ace_token     ?= 1
ace_svcconf   ?= 1
ace_uuid      ?= 1
ace_filecache ?= 1
acexml        ?= 1

# Icecream distributed compilation support
ifeq ($(icecream),1)
  # check availability of icecc
  ifneq ($(ICECC_ROOT),)
    ICECC_PROG := $(ICECC_ROOT)/icecc
  else
    ICECC_PROG := $(shell which icecc 2>/dev/null)
  endif
  ICECC_VERTEST := $(shell test -x "$(ICECC_PROG)" && $(ICECC_PROG) --version)
  ifneq (ICECC,$(findstring ICECC,$(ICECC_VERTEST)))
    # not good, try alternative install location
    ICECC_PROG := /opt/icecream/bin/icecc
    ICECC_VERTEST := $(shell test -x "$(ICECC_PROG)" && $(ICECC_PROG) --version)
    ifneq (ICECC,$(findstring ICECC,$(ICECC_VERTEST)))
      override icecream := 0
      $(error Cannot locate valid Icecream compiler.)
    endif
  endif
  ifeq ($(icecream),1)
    # force LD and DLD to be 'normal' tools
    override LD := $(LD)
    override DLD := $(DLD)
    # define tools that icecc should call for local jobs
    ICECC_CC := $(CC)
    ICECC_CXX := $(CXX)
    export ICECC_CC ICECC_CXX
    # let all compilation go through icecc
    CC := $(ICECC_PROG)
    CXX := $(ICECC_PROG)
  endif
endif

#----------------------------------------------------------------------------
#      Conditional macro definitions
#----------------------------------------------------------------------------

PTDIRS ?=
PRELINK ?=
POSTLINK ?=
PURELINK ?=
TEMPLATES_FLAG ?=
MVCMD ?=

COMPILE.c       = $(CC) $(CFLAGS) $(CPPFLAGS) -c
COMPILE.cc      = $(CXX) $(CCFLAGS) $(CPPFLAGS) $(PTDIRS) -c
RC ?=

COMPILE.rc      = $(RC) $(RCFLAGS)
COMPILE-NO_DASH_G.cc = `echo $(COMPILE.cc) | sed 's/-g //'`
# 960905 Marius Kjeldahl <marius@funcom.com>
# Added the line below to be used for compiling executable shared objects
COMPILEESO.cc   = $(CXX) $(CCFLAGS) $(CPPFLAGS) $(PTDIRS)

ifeq ($(LINK.c),override)
  LINK.c  = $(LINK.c.override)
else
  LINK.c  = $(PURELINK) $(PRELINK) $(LD) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) \
            $(LDLIBS) $(LIBS)
endif # LINK.c.override

ifeq ($(LINK.cc),override)
  LINK.cc = $(LINK.cc.override)
else
  LINK.cc = $(PURELINK) $(PRELINK) $(LD) $(CCFLAGS) $(CPPFLAGS) $(PTDIRS)
endif # LINK.cc.override

ifeq ($(SOLINK.cc),override)
  SOLINK.cc = $(SOLINK.cc.override)
else
  SOLINK.cc = $(PRELINK) $(DLD) $(SOFLAGS)
endif # SOLINK.cc.override
