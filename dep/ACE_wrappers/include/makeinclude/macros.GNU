#-*- Makefile -*-

#----------------------------------------------------------------------------
#	$Id: macros.GNU 90163 2010-05-18 21:42:20Z mitza $
#
#	Local and nested target definitions
#----------------------------------------------------------------------------

TARGETS_LOCAL  = \
	all.local \
	debug.local \
	profile.local \
	optimize.local \
	install.local \
	binout.local \
	clean_binout.local \
	clean_binaries.local \
	clean_idl_stubs.local \
	clean.local \
	realclean.local \
	clobber.local \
	depend.local \
	rcs_info.local \
	idl_stubs.local \
	svnignore.local

TARGETS_NESTED   = \
	$(TARGETS_LOCAL:.local=.nested)

