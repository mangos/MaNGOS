# -*- Makefile -*-
# $Id: platform_mingw32.GNU 90650 2010-06-16 21:54:18Z mitza $

# This file should allow to build ACE for mingw32 with mingw tools.
# Edit to change TCPU below.
#
# Don't forget to define the ACE_ROOT environment variable!

# mingw32 packages as distributed from sourceforge:
# http://sourceforge.net/project/showfiles.php?group_id=2435&release_id=15084
# See also http://www.mingw.org.
#
# You will also need a GNU Make for win32 (so you can actualy
# USE this file :-). Tested with MSYS from MinGW
#
# Caveat:
#
#   If ld complains about not finding crt2.o, dllcrt2.o or gcrt2.o
#   when linking executables, you will need to modify the specs file for gcc:
# 	lib/gcc-lib/mingw32/2.95.2/specs
#   look for the line after the one begining ``*startfile:'', and add
#   full path as prefix for crt2, dllcrt2 and gcrt2
#   (i.e., change
#    ... %{!shared:%{!mdll:crt2%O%s}} ...
#    with
#    ... %{!shared:%{!mdll:c:/mingw32/lib/gcc-lib/mingw32/2.95.2/crt2%O%s}} ...
#    or something similar depending on the location of your instalation).

#
# Chose your target CPU (by default we set it to pentiumpro. In your
# platform_macros.GNU file you can override this to pentium, i486 or i386
#
TCPU ?= pentiumpro

mingw32 ?= 1

# MingW doesn't allow to add the version number of ACE to the dll name
# because fe ace.dll.5.2.3 isn't a valid dll name
versioned_so=0

# MinGW doesn't have rwho
rwho ?= 0

# MinGW has wfmo and registry
wfmo ?= 1
winregistry ?= 1

# We can use the QoS support
qos ?= 1

# Disable auto-import warnings. The MingW linker has a problem with imports
# See https://sourceforge.net/tracker/?func=detail&atid=102435&aid=683455&group_id=2435
# for the details why to do this.
LDFLAGS += -Wl,--enable-auto-import

PWD=$(subst \,/,$(shell pwd))

PLATFORM_SSL_LIBS ?= -lssl32 -leay32

ifeq (cmd,$(findstring cmd,$(SHELL)))
  CMDSEP = &
  vxworks_ntbuild = 1
  MKDIR_NTSYTLE = 1
  ACE_MKDIR = mkdir
  ECHO_BLANK_LINE = cmd /c echo.
endif

include $(ACE_ROOT)/include/makeinclude/platform_gnuwin32_common.GNU

PRELIB:=
