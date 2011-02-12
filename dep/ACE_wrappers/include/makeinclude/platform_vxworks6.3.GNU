# -*- Makefile -*-
# $Id: platform_vxworks6.3.GNU 91326 2010-08-09 16:04:31Z mitza $
# VxWorks 6.3

#### Notes:
#### 1) This file requires that the WIND_BASE and WIND_HOST_TYPE environment
####    variables be set.  If the target CPU is not SIMNT, then your CPU
####    environment variable must be set.
#### 2) If you have problems with munch output not being compilable
####    because it contains identifiers with ".", e.g., ".cpp", in them:
####    add a global variable or function to that .cpp file.  See
####    ace/IOStream_T.cpp for an explanation and example.
#### 3) The TOOL environment variable may be set to "diab" (default is "gnu").
#### 4) If perl is not on your path, you'll also need to set your PERL_PATH
####    environment variable to the full path to perl.

VXWORKS = 1
CROSS-COMPILE = 1

debug ?= 1
optimize ?= 1
threads ?= 1
rtp ?= 1
ifeq ($(rtp),0)
  pthread ?= 0
else
  pthread ?= 1
endif
xerces ?= 0
aio ?= 0
versioned_so ?= 0
footprint ?= 0

ifeq ($(static_libs),1)
  shared_libs = 0
else
  static_libs = 0
endif
ifeq ($(static_libs_only),1)
  shared_libs = 0
endif

# VxWorks doesn't have rwho
rwho = 0

ifeq (,$(WIND_BASE))
  default:
	@ERROR: you must set your WIND_BASE environment variable
endif # WIND_BASE

ifeq (,$(WIND_HOST_TYPE))
  default:
	@ERROR: you must set your WIND_HOST_TYPE environment variable
endif # WIND_HOST_TYPE

ifeq ("$(WIND_HOST_TYPE)","x86-win32")
  vxworks_ntbuild ?= 0
  mingw32 ?= 1
  lacks_touch ?= 0
  PWD=$(subst \,/,$(shell pwd))
  ACE_ROOT:=$(subst \,/,$(ACE_ROOT))
  TAO_ROOT:=$(subst \,/,$(TAO_ROOT))
  CIAO_ROOT:=$(subst \,/,$(CIAO_ROOT))
  HOST_ROOT:=$(subst \,/,$(HOST_ROOT))
  override RM=rm -f
  HOST_EXE_EXT = .exe
endif # x86-win32

ifneq (,$(HOST_ROOT))
TAO_IDLFLAGS += -g $(HOST_ROOT)/bin/ace_gperf
TAO_IDL = $(HOST_ROOT)/bin/tao_idl$(HOST_EXE_EXT)
TAO_IDL3_TO_IDL2 = $(HOST_ROOT)/TAO/CIAO/bin/tao_idl3_to_idl2$(HOST_EXE_EXT)
TAO_IDL_DEP = $(TAO_IDL)
TAO_IDL3_TO_IDL2_DEP = $(TAO_IDL3_TO_IDL2)
else
# Append the HOST_EXE_EXT if it's not already there
TAO_IDL_DEP = $(TAO_IDL:$(HOST_EXE_EXT)=)$(HOST_EXE_EXT)
TAO_IDL3_TO_IDL2_DEP = $(TAO_IDL3_TO_IDL2:$(HOST_EXE_EXT)=)$(HOST_EXE_EXT)
endif

HOST_DIR = $(WIND_BASE)/host/$(WIND_HOST_TYPE)

ifeq (,$(PERL_PATH))
  PERL_PATH = perl
endif # ! PERL_PATH

ifeq ($(VXWORKS_VERSION_FLAG),)
  VXWORKS_VERSION_FLAG = -DACE_VXWORKS=0x630
endif # VXWORKS_VERSION_FLAG

ifeq ($(TOOL),)
  override TOOL = gnu
else
  override TOOL := $(TOOL)
endif
ifeq ($(TOOL_FAMILY),)
  ifeq ($(findstring gnu,$(TOOL)),gnu)
    override TOOL_FAMILY := gnu
  else
    override TOOL_FAMILY := diab
  endif
endif

ifeq ("$(TOOL_FAMILY)","gnu")
  templates ?= automatic
endif

ifeq ($(rtp),0)
  ifeq ($(findstring PPC,$(CPU)),PPC)
    CPPFLAGS += -mlongcall
  endif
  PRJ_TYPE = vxApp
endif

ifeq ($(rtp),1)
  TARGET_DIR = $(WIND_BASE)/target/usr
else
  TARGET_DIR = $(WIND_BASE)/target/h
endif
TGT_DIR = $(WIND_BASE)/target

ifeq ($(CPU),)
  # default CPU
  CPU = SIMNT
else
  override CPU := $(CPU)
endif

ifeq ($(shared_libs),1)
  SHARED_LIBS = 1
endif
ifeq ($(shared_libs_only),1)
  SHARED_LIBS = 1
endif

ifeq ($(rtp),0)
  ifeq ($(SHARED_LIBS),1)
    ACELIB = -L./
    #### There's no difference between non-shared and shared object code.
    VSHDIR = $(VDIR)
  endif # shared_libs
endif

BIN_UNCHECKED ?=
SHLIB_UNCHECKED ?=

ifneq ("$(BIN_UNCHECKED)","")
VX_APP_BUILD := 1
else
  ifeq ($(SHARED_LIBS),1)
    ifneq ("$(SHLIB_UNCHECKED)","")
	  VX_SO_BUILD := 1
    endif
  endif
  ifneq ($(VX_SO_BUILD),1)
    VX_LIB_BUILD := 1
  endif
endif

ifneq ($(VX_LIB_BUILD),1)
  ifeq ($(rtp),0)
    VXWORKSLINK ?= 1
  endif
  ifeq ($(VX_APP_BUILD),1)
    ## set variables for VxWorks linktarget identification
    ifeq ($(rtp),0)
      EXEEXT = .out
      PRJ_FILE = $(basename $(notdir $(BIN_UNCHECKED))).vxworks
    else
      PRJ_FILE = $(basename $(notdir $(BIN_UNCHECKED))).vxe
      EXEEXT = .vxe
      VXE_DIR := $(OUTPUT_DIRECTORY)
      EXE = $(BIN_UNCHECKED)
    endif
    PRJ_DIR = .
    CLEANUP_OBJS := ctdt.c $(PRJ_DIR)/$(PRJ_FILE) $(CLEANUP_OBJS)
  else
    ## set variables for VxWorks .SO linktarget identification
    PRJ_FILE = $(basename $(notdir $(SHLIB_UNCHECKED))).vxworks
    PRJ_DIR = .
    CLEANUP_OBJS := ctdt.c $(PRJ_DIR)/$(PRJ_FILE) *.out $(CLEANUP_OBJS)
  endif
endif

ifeq ($(rtp),1)
  include $(WIND_USR)/make/defs.default
  include $(WIND_USR)/tool/$(TOOL_FAMILY)/make.$(CPU)$(TOOL)
  include $(WIND_USR)/make/defs.$(WIND_HOST_TYPE)
  ifeq ("$(TOOL_FAMILY)","gnu")
    CPPFLAGS += -mrtp
    SOFLAGS += -mrtp
  endif
  ifeq ($(SHARED_LIBS), 1)
    ifeq ($(aio),1)
      CPPFLAGS += -DACE_HAS_AIO_CALLS
      LIBS += AioPx
    endif
    ifeq ($(PRJ_TYPE),library)
      PICDIR = /PIC
    endif
  endif
  LDFLAGS := $(filter-out -X, $(LDFLAGS))
  LDFLAGS := $(filter-out -N, $(LDFLAGS))
  LD := $(CPLUS)
  DLD := $(CPLUS)
  WIND_USR_LIB ?= lib
  ifeq ($(findstring *sf,*$(TOOL)),*sf)
    LDFLAGS += -L$(WIND_USR)/$(WIND_USR_LIB)/$(VX_CPU_FAMILY)/$(CPU)/sfcommon$(PICDIR)
  else
    LDFLAGS += -L$(WIND_USR)/$(WIND_USR_LIB)/$(VX_CPU_FAMILY)/$(CPU)/common$(PICDIR)
  endif
else
  include $(TGT_DIR)/h/make/defs.default
  include $(TGT_DIR)/h/make/defs.$(WIND_HOST_TYPE)
  include $(TGT_DIR)/h/make/defs.$(PRJ_TYPE)
  include $(TGT_DIR)/h/tool/$(TOOL_FAMILY)/make.$(CPU)$(TOOL)
endif

ifeq ($(SHARED_LIBS), 1)
ifeq ("$(TOOL_FAMILY)","gnu")
  ifeq ($(rtp),1)
    PIC = -fpic
    SOFLAGS += -fpic
    LDFLAGS += -Wl,-rpath-link,$(ACE_ROOT)/lib
  endif
  SOFLAGS += -shared
  LDFLAGS += -non-static
else
  ifeq ($(rtp),1)
    PIC = -Xpic
  endif
  SOFLAGS += -Xshared
  LDFLAGS += -Xdynamic
endif
endif

no_cflags_ansi ?= 0
ifeq ($(no_cflags_ansi), 1)
  ifeq ("$(TOOL_FAMILY)","gnu")
    CC_COMPILER := $(filter-out -ansi, $(CC_COMPILER))
  else
    CC_COMPILER := $(filter-out -Xansi, $(CC_COMPILER))
  endif
endif

no_ccflags_ansi ?= 0
ifeq ($(no_ccflags_ansi), 1)
  ifeq ("$(TOOL_FAMILY)","gnu")
    C++_COMPILER := $(filter-out -ansi, $(C++_COMPILER))
  else
    C++_COMPILER := $(filter-out -Xansi, $(C++_COMPILER))
  endif
endif

ifeq ($(footprint),1)
  LD_PARTIALFLAGS += -Xlinker -M -Xlinker -Map -Xlinker $@.map
endif

ifeq ($(VXWORKSLINK), 1)
  # prevent possible clash with static LIB rules
  # in VxWorks make includes and rules.lib.GNU
  override ARCHIVE =
  override LARGE_PROJECT =
  PRJ_OBJS = $(addprefix $(VDIR), $(OBJS)) $(VSHOBJS)
  ifeq ($(SHARED_LIBS),1)
    ##LD_PARTIAL += -L$(ACE_ROOT)/lib -L./ $(LDLIBPATH) $(LD_LINK_PATH)
    PRJ_OBJS_FOR_LD_PARTIAL=$(addprefix $(VDIR), $(OBJS)) $(VSHOBJS)
    ifeq ($(VX_SO_BUILD),1)
      ## Shared lib builds for VxWorks will also build 'import' libraries
      ifeq ($(rtp),1)
        LIB_INSTALL += $(VLIB:%=$(INSLIB)/%)
        CLEANUP_INSTALL += $(CLEANUP_VLIB:%=$(INSLIB)/%)
        REALCLEAN_FILES += $(CLEANUP_LIB:%=%) $(CLEANUP_LIB:%=%_debug) $(CLEANUP_LIB:%=%_profile) $(CLEANUP_LIB:%=%_optimize)
      endif
    endif
  else
    LD_PARTIAL += -L$(ACE_ROOT)/ace -L./ $(LDLIBPATH) $(LD_LINK_PATH) $(LD_PARTIALFLAGS)
    ifeq ($(footprint),0)
      PRJ_OBJS_FOR_LD_PARTIAL=$(addprefix $(VDIR), $(OBJS)) $(VSHOBJS) $(VLDLIBS) $(ACE_SHLIBS) $(LIBS)
    else
      PRJ_OBJS_FOR_LD_PARTIAL=$(VSHOBJS) $(VLDLIBS) $(ACE_SHLIBS) $(LIBS)
      LD_PARTIAL_LAST_FLAGS += $(addprefix $(VDIR), $(OBJS)) $(VSHOBJS) $(VLDLIBS)
    endif
  endif
  PRJ_OBJS_FOR_LD=$(addprefix $(VDIR), $(OBJS)) $(VSHOBJS)
  PRJ_OBJS_FOR_NM=$(addprefix $(VDIR), $(OBJS)) $(VSHOBJS)
  PRJ_OBJS_FOR_AR=$(addprefix $(VDIR), $(OBJS)) $(VSHOBJS)
  POST_BUILD_RULE = $(POSTLINK)
  COMPILE_TRADITIONAL = $(CC) $(OPTION_OBJECT_ONLY) $(OPTION_DOLLAR_SYMBOLS)\
			  $(filter-out -ansi, $(CFLAGS))

  ## make sure default target stays the ACE/TAO default target
  ##
default: all

endif

CPPFLAGS += $(VXWORKS_VERSION_FLAG)

ifeq ($(debug),1)
  DCFLAGS += -g
  DCCFLAGS += -g
  CPPFLAGS +=
endif

ifeq ($(pthread),1)
  CPPFLAGS += -DACE_HAS_PTHREADS
endif

ifeq ("$(TOOL_FAMILY)","gnu")
# Test for template instantiation, add to SOFLAGS if SONAME set,
# add -E to LDFLAGS if using GNU ld
include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU

ifeq ("$(templates)","automatic")
ifeq ($(repo),1)
ifeq ($(VXWORKSLINK), 1)
  ifeq ($(VX_APP_BUILD),1)
	ifneq ($(SHARED_LIBS),1)
          LD_PARTIAL += -frepo
	endif
  endif
else
  ifeq ($(rtp),0)
    ace_lib_prelink := 1
    LINK.cc          = override
    LINK.cc.override = $(LD_PARTIAL) -frepo -L$(ACE_ROOT)/ace -L$(INSLIB) -L./ $(LDLIBPATH) $(LD_LINK_PATH)
    ifneq ($(SHARED_LIBS),1)
      LDFLAGS        = $(ACE_SHLIBS)
    endif
  endif
endif
endif
endif

# TEMPLATES_FLAG is set by platform_g++_common.GNU
ifneq ($(TEMPLATES_FLAG),) # If specified, add it to the CCFLAGS
  CPPFLAGS += $(TEMPLATES_FLAG)
endif
endif  # TOOL_FAMILY == gnu

ifeq ("$(TOOL_FAMILY)","diab")
  C++FLAGS += -Xexceptions
else
ifeq ("$(TOOL_FAMILY)","gnu")
  C++FLAGS += -fexceptions
endif
endif

ifeq ("$(TOOL_FAMILY)","diab")
  LIBS += -lstlstd
endif

ifneq ($(findstring $(VX_CPU_FAMILY),ppc arm),)
ifeq ("$(TOOL_FAMILY)","diab")
  C++FLAGS += -Xchar-signed
else
ifeq ("$(TOOL_FAMILY)","gnu")
  C++FLAGS += -fsigned-char
endif
endif
endif

ifeq ($(CPU),SIMNT)
override MUNCH_FLAGS := -asm $(TOOLENV)
endif

ifneq ($(optimize),1)
  CC_OPTIM_NORMAL =
endif

CCFLAGS         += $(C++FLAGS)

ifeq ($(VXWORKSLINK), 1)

  ifeq ($(repo),1)
    ifeq ($(SHARED_LIBS),1)
      BACKUP_RPO_RULE = for f in $(basename $(addprefix $(VDIR), $(OBJS)) $(VSHOBJS)) ; do if [ -a $$f.rpo ] ; then mv $$f.rpo $$f.rpobak $(ACE_NUL_STDERR) ; fi ; done ;
      POST_BUILD_RULE = for f in $(basename $(addprefix $(VDIR), $(OBJS)) $(VSHOBJS)) ; do if [ -a $$f.rpobak ] ; then mv $$f.rpobak $$f.rpo $(ACE_NUL_STDERR) ; fi ; done ;
    endif
  endif

.PHONY: $(PRJ_DIR)/$(PRJ_FILE)
  ## special target to satisfy VxWorks dependencies for images
$(PRJ_DIR)/$(PRJ_FILE):
  ifeq ($(repo),1)
    ifeq ($(SHARED_LIBS),1)
    #### execute prelink step to make compiler instantiate all needed
    #### templates; discard image and repo files (*.rpo) after linkstep
    #### this step holds for both .SO as well as 'app' images so use both VSHOBJS and OBJS
	-$(LD_PARTIAL) -frepo -L$(ACE_ROOT)/ace -L./ $(LDLIBPATH) $(LD_LINK_PATH)$(LINK_OUTPUT_FLAG) ace-templ_inst $? $(ACE_SHLIBS) $(addprefix $(VDIR), $(OBJS)) $(VSHOBJS)
	-$(RM) ace-templ_inst
    #### We need to backup the .RPO files or otherwise the link step of the final image will
    #### start instantiating templates again even if we do not specify '-frepo'!
	-$(BACKUP_RPO_RULE)
      ifeq ($(VX_SO_BUILD),1)
    #### build library of objects in .SO to use as a sort of import library for VxWorks
	echo $(filter %.o, $(VSHOBJS)) | xargs $(AR) $(ARFLAGS) $(VLIB) $(AREXTRA)
	-chmod a+r $(VLIB)
        ifneq (,$(RANLIB))
	-$(RANLIB) $(VLIB)
        endif # RANLIB
      endif # VX_SO_BUILD
    endif
  endif
	@echo 1>$@

endif
