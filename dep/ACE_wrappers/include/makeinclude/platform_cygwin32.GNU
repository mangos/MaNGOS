# $Id: platform_cygwin32.GNU 80826 2008-03-04 14:51:23Z wotte $
#
# This file should allow to build ACE for cygwin32 with cygwin tools.
#
# Edit to change TCPU and w32api defs below.
#
# Don't forget to define the ACE_ROOT environment variable!
#

#
# Chose your target CPU (by default we set it to pentiumpro. In your
# platform_macros.GNU file you can override this to pentium, i486 or i386
#
TCPU ?= pentiumpro

cygwin32 = 1

# Cygwin doesn't have rwho
rwho = 0

# Cygwin doesn't allow to add the version number of ACE to the dll name
# because fe ace.dll.5.2.3 isn't a valid dll name
versioned_so=0

# Disable auto-import warnings. The Cygwin linker has a problem with imports
# See https://sourceforge.net/tracker/?func=detail&atid=102435&aid=683455&group_id=2435
# for the details why to do this.
LDFLAGS += -Wl,--enable-auto-import

include $(ACE_ROOT)/include/makeinclude/platform_gnuwin32_common.GNU
