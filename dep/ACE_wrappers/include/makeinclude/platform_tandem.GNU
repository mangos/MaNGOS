# $Id: platform_tandem.GNU 91285 2010-08-05 08:29:30Z johnnyw $

# NonStop-UX NCC 3.20

debug = 1

# Please see  the config-NonS... file for more information about status
# This file is built from Sunos5(os) and sgic++(compiler)

CC		= cc
CXX		= NCC
DLD		= $(CXX)
LD		= $(CXX)
CPPFLAGS	+= -I/usr/include3.18
CPPFLAGS	+= -D_REENTRANT
#CPPFLAGS	+= -ptused -prelink
CPPFLAGS	+= -pta
#CPPFLAGS	+= +pp
#CPPFLAGS	+= -show
#-D_SGI_MP_SOURCE
#3106 nested comment is not allowed
#3203 statement is unreachable
#3252 argument of type "X" is incompatible with parameter of type "Y"
#3262 Parameter "X" declared and never referenced
#3611 a value of "X" cannot be assigned to an entity of type "Y"
#3140 macro redefined differently (param redefines NULL)

CPPFLAGS	+= -woff 3106,3203,3262
#CPPFLAGS	+= -woff 3106,3203,3262,3611,3252
#CPPFLAGS	+= -woff 3106,3203,3212,3252,3262,3611

# For the new 32 bit C++ compiler (-n32)
#CPPFLAGS	+= -n32 -woff 1174,1209,1375,1506,1110,1552,1021,1171
# For the old C++ compiler (-32)
#CPPFLAGS	+= +pp
LDFLAGS		+= -rpath "$(ACE_ROOT)/lib"
#LDFLAGS	+= -Wl,-woff,85
LIBS		+= -lsocket
LIBS		+= -ldl
LIBS		+= -lnsl
LIBS		+= -lgen
LIBS		+= -lthread
PIC		= -KPIC
AR		= ar
ARFLAGS		= r
RANLIB		= echo
SOFLAGS		+= -shared $(CPPFLAGS) -all
SOBUILD		= $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.o $<; \
		 $(SOLINK.cc) -o $@ $(LDFLAGS) $(VSHDIR)$*.o

#3106 nested comment is not allowed
#3203 statement is unreachable
#3252 argument of type "X" is incompatible with parameter of type "Y"
#3262 Parameter declared and never referenced
#3611 a value of "X" cannot be assigned to an entity of type "Y"
#WARNOFF 	+= -woff 3106,3203,3262,3611
#INCDIR		+= -I/usr/include -I/usr/include3.18
# -I /usr/ucbinclude
#### No threads
#CCFLAGS         += -pta $(WARNOFF) $(INCDIR)
#### Threads
#CCFLAGS         += -D_REENTRANT -pta $(WARNOFF)

#DLD             = $(CXX)
#LD		= $(CXX)
#LDFLAGS         += -R $(ACE_ROOT)/lib
#LIBS            += -lsocket -ldl -lnsl -lgen -lthread  # Threads
#LIBS            += -lsocket -ldl -lnsl -lgen # No threads
#PIC             = -PIC
# SUNC++ 4.1 has a bug with the -xar command:
#
# from 'CC -readme' the following bug is reported:
#
# 4. Using -xar to add generated templates
# ----------------------------------------
#
#        When using -xar to add generated templates to an archive,
#        please be aware that this command will both update
#        existing object files in the archive, and add new ones.
#        Existing object files that may be obsolete will still
#        be present. In these cases, the safest course of action
#        would be to delete the .a file prior to the invocation of -xar.
#
# so here we remove the archive prior to making the new one
#
#AR              = CC
#ARFLAGS         = -xar -o
#RANLIB          = echo
#SOFLAGS         = -G $(CPPFLAGS)
#SOBUILD		= $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.o $<; \
		  $(SOLINK.cc) -o $@ -h $@ $(LDFLAGS) $(VSHDIR)$*.o
