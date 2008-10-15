# Copyright (C) 2005-2008 MaNGOS <http://getmangos.com/>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

## Process this file with automake to produce Makefile.in

## Sub-directories to parse
SUBDIRS = Auth Config Database vmap

## CPP flags for includes, defines, etc.
AM_CPPFLAGS = $(MANGOS_INCLUDES) -I$(top_builddir)/src/shared -I$(srcdir) -I$(srcdir)/../../dep/include -I$(srcdir)/../framework -I$(srcdir)/../shared -I$(srcdir)/../../dep/include/g3dlite
## AM_CPPFLAGS += -I$(srcdir)/../game -I$(srcdir)/../realmd

## Build MaNGOS shared library and its parts as convenience library.
#  All libraries will be convenience libraries. Might be changed to shared
#  later.
noinst_LIBRARIES = libmangosshared.a

#  libmangosshared library will later be reused by ...
libmangosshared_a_SOURCES = \
    Base.cpp \
    Base.h \
    ByteBuffer.h \
    Common.cpp \
    Common.h \
    Errors.h \
    Log.cpp \
    Log.h \
    MemoryLeaks.cpp \
    MemoryLeaks.h \
    Mthread.cpp \
    Mthread.h \
    ProgressBar.cpp \
    ProgressBar.h \
    Timer.h \
    Util.cpp \
    Util.h \
    WorldPacket.h \
    svn_revision.h

# Get svn revision
SVN_REVISION_FILE = svn_revision.h

BUILT_SOURCES = $(SVN_REVISION_FILE)
CLEANFILES = $(SVN_REVISION_FILE)

FORCE:

$(SVN_REVISION_FILE) : $(top_builddir)/src/tools/gensvnrevision/gensvnrevision FORCE
	$(top_builddir)/src/tools/gensvnrevision/gensvnrevision $(top_srcdir)

## Additional files to include when running 'make dist'
#  Disabled packet logger
EXTRA_DIST = \
    PacketLog.cpp \
    PacketLog.h

# System configuration
EXTRA_DIST += \
    SystemConfig.h

# System Win32 files
EXTRA_DIST += \
    ServiceWin32.cpp \
    ServiceWin32.h \
    WheatyExceptionReport.cpp \
    WheatyExceptionReport.h
