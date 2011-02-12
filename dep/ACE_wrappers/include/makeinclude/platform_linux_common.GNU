# -*- Makefile -*-
# $Id: platform_linux_common.GNU 91626 2010-09-07 10:59:20Z johnnyw $

# We always include config-linux.h on Linux platforms.
ACE_PLATFORM_CONFIG ?= config-linux.h

debug ?= 1
optimize ?= 1
threads ?= 1
insure ?= 0

LSB_RELEASE_ID := $(shell lsb_release -i 2> /dev/null || echo Distributor ID: Unknown)
LSB_RELEASE_RELEASE := $(shell lsb_release -r 2> /dev/null || echo Release: Unknown)

PLATFORM_XT_CPPFLAGS=
PLATFORM_XT_LIBS=-lXt
PLATFORM_XT_LDFLAGS=

PLATFORM_FL_CPPFLAGS=
PLATFORM_FL_LIBS=-lfltk -lfltk_forms -lfltk_gl
PLATFORM_FL_LDFLAGS=

PLATFORM_X11_CPPFLAGS=-I/usr/X11R6/include
PLATFORM_X11_LIBS=-lX11
PLATFORM_X11_LDFLAGS=-L/usr/X11R6/lib

PLATFORM_GL_CPPFLAGS=-I/usr/X11R6/include
PLATFORM_GL_LIBS    =-lGL
PLATFORM_GL_LDFLAGS =-L/usr/X11R6/lib

PLATFORM_GTK_CPPFLAGS=$(shell gtk-config --cflags)
PLATFORM_GTK_LIBS    =$(shell gtk-config --libs)
PLATFORM_GTK_LDFLAGS =

PLATFORM_FOX_CPPFLAGS ?= -I/usr/include/fox
PLATFORM_FOX_LIBS     ?= -lFOX
PLATFORM_FOX_LDFLAGS  ?=

# NOTE: we only support wxWindows over GTK
PLATFORM_WX_CPPFLAGS= $(shell wx-config --cxxflags) $(PLATFORM_GTK_CPPFLAGS)
PLATFORM_WX_LIBS    = $(shell wx-config --libs)     $(PLATFORM_GTK_LIBS)
PLATFORM_WX_LDFLAGS = $(shell wx-config --ldflags)  $(PLATFORM_GTK_LDFLAGS)

PLATFORM_BOOST_CPPFLAGS ?=
PLATFORM_BOOST_LDLAGS ?=
PLATFORM_BOOST_UTF_LIBS ?= -lboost_unit_test_framework

ifeq (Ubuntu, $(findstring Ubuntu,$(LSB_RELEASE_ID)))
  PLATFORM_TK_CPPFLAGS=$(shell . /usr/lib/tk*/tkConfig.sh && echo -n $$TK_INCLUDE_SPEC $$TK_DEFS)
  PLATFORM_TK_LIBS=$(shell . /usr/lib/tk*/tkConfig.sh && echo -n $$TK_LIB_FLAG)
  PLATFORM_TK_LDFLAGS=

  PLATFORM_TCL_CPPFLAGS=
  PLATFORM_TCL_LIBS=-ltcl8.4
  PLATFORM_TCL_LDFLAGS=
else
  PLATFORM_TK_CPPFLAGS=-I$(shell . /usr/lib*/tkConfig.sh && echo -n $$TK_INC_DIR $$TK_DEFS)
  PLATFORM_TK_LIBS=$(shell . /usr/lib*/tkConfig.sh && echo -n $$TK_LIB_FLAG)
  PLATFORM_TK_LDFLAGS=


  PLATFORM_TCL_CPPFLAGS=
  PLATFORM_TCL_LIBS=-ltcl
  PLATFORM_TCL_LDFLAGS=
endif

PLATFORM_QT_CPPFLAGS ?= -I$(QTDIR)/include
PLATFORM_QT_LIBS ?= -lqt-mt
PLATFORM_QT_LDFLAGS ?= -L$(QTDIR)/lib

sctp ?=
# support for OpenSS7 SCTP
ifeq ($(sctp),openss7)
  PLATFORM_SCTP_CPPFLAGS+= -DACE_HAS_OPENSS7_SCTP
  PLATFORM_SCTP_LDFLAGS?=
  PLATFORM_SCTP_LIBS?=
endif

# support for LKSCTP (Linux Kernel 2.5)
ifeq ($(sctp),lksctp)
  PLATFORM_SCTP_CPPFLAGS+= -DACE_HAS_LKSCTP
  PLATFORM_SCTP_LDFLAGS?= -L/usr/local/lib
  PLATFORM_SCTP_LIBS?= -lsctp
endif

PLATFORM_AIO_SUPPORT := \
       $(shell test "`ls -L /usr/lib*/librt.so* /lib*/librt.so*`" && echo -DACE_HAS_AIO_CALLS)

GNU_LIBPTHREAD_VERSION := $(shell getconf GNU_LIBPTHREAD_VERSION 2> /dev/null || echo Unknown)
ifeq (NPTL, $(word 1,$(GNU_LIBPTHREAD_VERSION)))
  NPTL_VERS := $(subst ., ,$(word 2,$(GNU_LIBPTHREAD_VERSION)))
  ifneq (0, $(word 1,$(NPTL_VERS)))
    nptl ?= 1
  endif
endif
nptl ?= 0
ifeq ($(nptl),0)
  CPPFLAGS += -DACE_LACKS_LINUX_NPTL
endif

ssl ?= 0
ifeq ($(ssl),1)
  # Some Linux OpenSSL installations compile in Kerberos support.  Add
  # the Kerberos include path to preprocessor include path.
  #
  # We should probably also add the Kerberos libraries to
  # PLATFORM_SSL_LIBS but we can't be sure if they are needed without
  # a more sophisticated check.  This will only be a problem when
  # statically linking the OpenSSL library.  The majority of
  # installations use shared OpenSSL libraries so we should be okay,
  # at least until we migrate to Autoconf.
  PLATFORM_SSL_CPPFLAGS += -I/usr/kerberos/include
endif # ssl

SYSARCH := $(shell uname -m)

PIC      = -fPIC
AR      ?= ar
ARFLAGS ?= rsuv
RANLIB   = @true
