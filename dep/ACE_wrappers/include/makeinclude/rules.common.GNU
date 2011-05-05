# -*- Makefile -*-

#----------------------------------------------------------------------------
#	$Id: rules.common.GNU 90163 2010-05-18 21:42:20Z mitza $
#
#	Common targets
#----------------------------------------------------------------------------

.PHONY: __prebuild__

all:              __prebuild__ all.nested all.local
debug:	          debug.nested debug.local
profile:          profile.nested profile.local
binout:           binout.nested binout.local
clean_binout:     clean_binout.nested clean_binout.local
clean_binaries:   clean_binaries.nested clean_binaries.local
clean_idl_stubs:  clean_idl_stubs.nested clean_idl_stubs.local
clean:            clean.nested clean.local
realclean:        realclean.nested realclean.local
clobber:          clobber.nested clobber.local
depend:	          depend.nested depend.local
rcs_info:         rcs_info.nested rcs_info.local
idl_stubs:        idl_stubs.nested idl_stubs.local
svnignore:	  svnignore.nested svnignore.local
