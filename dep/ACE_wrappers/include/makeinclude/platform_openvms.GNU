# $Id: platform_openvms.GNU 91285 2010-08-05 08:29:30Z johnnyw $

# for OpenVMS GNV

ACE_OPENVMS = 1
versioned_so ?= 0
debug ?= 1
optimize ?= 0
threads ?= 1
pthread ?= 1
ssl ?= 0
rwho ?= 0
buildbits ?= 32

VDIR            ?= obj/
VSHDIR          ?= shobj/
PRELINK_TYPE    ?= USE_OLB

LN_S = cp -p
DEFFLAGS        +=
CCFLAGS         += -D__USE_STD_IOSTREAM -ieee -names_as_is_short
ifeq ($(HOSTTYPE),ia64)
  CCFLAGS         += "-Wc/warn=DISABLE=(INTOVERFLOW,REFTEMPORARY,INTSIGNCHANGE)"
else
  CCFLAGS         += -msg_disable intoverflow,reftemporary,intsignchange
endif
CCFLAGS         += -Wc/template=auto

ifeq ($(buildbits),64)
  CCFLAGS += -Wc/MODEL=ANSI
endif

ifeq ($(HOSTTYPE),ia64)
  ACE_OPENVMS_IA64 = 1
  SYMVEC_OPT_FILE ?= $(LIB_NAME)_symvec.opt
  ifeq ($(PRJ_TYPE),library)
    ifeq ($(shared_libs),1)
      CCFLAGS         += "-Wc/export_sym=(OPTIONS_FILE=$(SYMVEC_OPT_FILE),ALL,NOTEMPLATES)"
    endif
  endif
else
  LDFLAGS         += -Wl/PRELINK=$(PRELINK_TYPE)/MAP/CROSS
endif

ifeq ($(debug),1)
 LDFLAGS         += -g
endif
LDFLAGS         += -threads
DCCFLAGS        += -g
OCCFLAGS        += -O

CFLAGS          += -ieee -names_as_is_short
DCFLAGS         += -g
OCFLAGS         += -O

PIC             =
RANLIB          = @true
ifeq ($(HOSTTYPE),ia64)
  ARFLAGS         = -r
  SOFLAGS         = -shared $(SYMVEC_OPT_FILE)
else
  ARFLAGS         = -c
  SOFLAGS         = -shared -auto_symvec
endif
ifeq ($(pthread),1)
  LIBS            += -lpthread
endif

CC              = cc
CXX             = cxx
LD              = $(CXX)
DLD             = $(CXX)
SOVERSION       =
SOEXT           = exe
EXEEXT          = .exe

ifeq ($(HOSTTYPE),ia64)
  REALCLEAN_FILES += $(SYMVEC_OPT_FILE) $(CLEANUP_VSHLIB_NO_VER:%.exe=%.DSF) \
	$(CLEANUP_BIN:%=%.DSF) $(basename $(BIN))
else
  REALCLEAN_FILES += $(CLEANUP_VSHLIB_NO_VER:%.exe=%_symvec.opt) $(CLEANUP_VSHLIB_NO_VER:%.exe=%.DSF) \
	$(CLEANUP_BIN:%=%.DSF) $(basename $(BIN))
endif

ifneq ($(MAKEFILE),$(DEPENDENCY_FILE))
VMS_DUMMY_ := $(shell touch $(DEPENDENCY_FILE))
VMS_DUMMY_ := $(VMS_DUMMY_)
endif

ifeq ($(HOSTTYPE),ia64)
  ifeq ($(shared_libs),1)
   ifneq ($(SHLIB),)
BUILD          := post_build.local

.PHONY: post_build.local

post_build.local:
	@-if [ ! -z "$(VSHLIB)" ]; then dcl "set image/success $(VSHLIB)"; echo ""; fi

   endif
  endif
endif
