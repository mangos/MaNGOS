# -*- Makefile -*-
#----------------------------------------------------------------------------
#       $Id: rules.local.GNU 90650 2010-06-16 21:54:18Z mitza $
#
#       Local targets
#       GNU version
#       Requires GNU make
#----------------------------------------------------------------------------


CLEANUP_OBJDIRS =
CLEANUP_DIRS =

ifdef CLEANUP_BIN
  CLEANUP_OBJDIRS = $(VDIR)
else # ! CLEANUP_BIN
  ifdef CLEANUP_LIB
    CLEANUP_OBJDIRS = $(VDIR)
  endif # CLEANUP_LIB
endif # CLEANUP_BIN

ifdef CLEANUP_SHLIB
  CLEANUP_OBJDIRS += $(VSHDIR)
endif # CLEANUP_VSHLIB

ifdef TEMPINCDIR
  CLEANUP_DIRS += $(TEMPINCDIR)
endif

ifdef sun
  CLEANUP_DIRS += SunWS_cache
endif

vxworks_ntbuild ?= 0
lacks_touch ?= 0

#### TEMPLATE_REPOSITORY can be set in individual platform files
#### to create template repository directories.  See
#### platform_sunos5_sunc++.GNU for an example.
ifdef TEMPLATE_REPOSITORY
CLEANUP_DIRS += $(TEMPLATE_REPOSITORY)
endif

ifneq ($(mingw32),1)
makefile_name.local:
	@echo
	@echo GNUmakefile: $(shell pwd)/$(MAKEFILE) MAKEFLAGS=$(MAKEFLAGS)
	@echo
else
ECHO_BLANK_LINE ?= echo
makefile_name.local:
	@$(ECHO_BLANK_LINE)
	@echo GNUmakefile: $(call PWD)/$(MAKEFILE) MAKEFLAGS=$(MAKEFLAGS)
	@$(ECHO_BLANK_LINE)
endif

all.local: makefile_name.local build.objdirs build.local binout.local

#----------------------------------------------------------------------------
#       C/C++ compilation targets
#----------------------------------------------------------------------------


IDL_SRC ?=
VLIBS ?=
BUILD ?=
BIN ?=
VBIN ?= $(ARCH_BIN:%=%$(VAR))
ifneq ($(VBIN),)
  $(VBIN): $(VLIBS)
endif

# Always build the V* targets in this order.  We control assignment to them
# elsewhere. We add the existing $(BUILD) targets to the end to allow the
# caller to set additional ones, e.g., when defining multiple targets in a
# single makefile--note that we filter out any that are already present.
VBUILD = $(IDL_SRC) $(VLIBS) $(VBIN) $(filter-out $(IDL_SRC) $(VLIBS) $(VBIN), $(BUILD))

build.local: $(VBUILD)

# By setting SUFFIXES to nothing, we turn off all implicit rules, and force
# make to only use the explicit ones we define.
.SUFFIXES:

# C++ related targets

STATIC_LINK_FLAG ?=
static_link ?= 0
ifeq ($(static_link),1)
  LDFLAGS += $(STATIC_LINK_FLAG)
endif # static_link = 1

OS ?=

# Define a canned command set to create the output object directory if needed.
ifeq ($(vxworks_ntbuild),1)
define mk_obj_out_dir
@if not exist $(subst /,\,$(@D)) $(MKDIR) $(subst /,\,$(@D)) $(ACE_NUL_STDERR)
endef
else
define mk_obj_out_dir
@-test -d $(@D) || $(MKDIR) $(@D) $(ACE_NUL_STDERR)
endef
endif

nullstring :=
CC_OUTPUT_FLAG_SEP ?= $(nullstring) #space

$(VDIR)%.$(OBJEXT): %.c
	$(mk_obj_out_dir)
	$(COMPILE.c) $(CC_OUTPUT_FLAG)$(CC_OUTPUT_FLAG_SEP)$@ $<
	${MVCMD}

$(VDIR)%.$(OBJEXT): %.C
	$(mk_obj_out_dir)
	$(COMPILE.c) $(CC_OUTPUT_FLAG)$(CC_OUTPUT_FLAG_SEP)$@ $<
	${MVCMD}

$(VDIR)%.$(OBJEXT): %.cpp
	$(mk_obj_out_dir)
	$(COMPILE.cc) $(CC_OUTPUT_FLAG)$(CC_OUTPUT_FLAG_SEP)$@ $<
	${MVCMD}

$(VDIR)%.$(OBJEXT): %.cc
	$(mk_obj_out_dir)
	$(COMPILE.cc) $(CC_OUTPUT_FLAG)$(CC_OUTPUT_FLAG_SEP)$@ $<
	${MVCMD}

$(VDIR)%.$(OBJEXT): %.cxx
	$(mk_obj_out_dir)
	$(COMPILE.cc) $(CC_OUTPUT_FLAG)$(CC_OUTPUT_FLAG_SEP)$@ $<
	${MVCMD}

RC_OUTPUT_FLAG ?=
  ifneq (,$(COMPILE.rc))
$(VDIR)%.rc.o: %.rc
	$(mk_obj_out_dir)
	$(COMPILE.rc) $(RC_OUTPUT_FLAG) $@ $<
	${MVCMD}
  endif # COMPILE.rc

# If SOLINK is defined, then the .$(SOEXT) file is built from the
# .$(OBJEXT) file via separate rules in the same directory.
# Otherwise, the .$(SOEXT) and .$(OBJEXT) are built via the same rule.
# SOLINK is required for the repository under gcc.
ifndef SOLINK
$(VSHDIR)%.$(OBJEXT): %.c
	$(mk_obj_out_dir)
	$(COMPILE.c) $(PIC) $(CC_OUTPUT_FLAG)$(CC_OUTPUT_FLAG_SEP)$@ $<

$(VSHDIR)%.$(OBJEXT): %.C
	$(mk_obj_out_dir)
	$(COMPILE.c) $(PIC) $(CC_OUTPUT_FLAG)$(CC_OUTPUT_FLAG_SEP)$@ $<

# I added the "Executable Shared Object (ESO)" define to separate between
# normal shared object files and executable shared object files (the kind
# that the service configurator needs to be able to function).
# 970104 Marius Kjeldahl <mariusk@sn.no, marius@funcom.com>
  ifdef (ESOBUILD)
$(VSHDIR)%.$(SOEXT): %.cpp
	$(mk_obj_out_dir)
	$(ESOBUILD)
  else # ! ESOBUILD
$(VSHDIR)%.$(SOEXT): %.cpp
	$(mk_obj_out_dir)
	$(SOBUILD)
  endif # ESOBUILD

$(VSHDIR)%.$(OBJEXT): %.cc
	$(mk_obj_out_dir)
	$(COMPILE.cc) $(PIC) $(CC_OUTPUT_FLAG)$(CC_OUTPUT_FLAG_SEP)$@ $<

$(VSHDIR)%.$(OBJEXT): %.cpp
	$(mk_obj_out_dir)
	$(COMPILE.cc) $(PIC) $(CC_OUTPUT_FLAG)$(CC_OUTPUT_FLAG_SEP)$@ $<

$(VSHDIR)%.$(OBJEXT): %.cxx
	$(mk_obj_out_dir)
	$(COMPILE.cc) $(PIC) $(CC_OUTPUT_FLAG)$(CC_OUTPUT_FLAG_SEP)$@ $<

  ifneq (,$(COMPILE.rc))
$(VSHDIR)%.rc.$(OBJEXT): %.rc
	$(mk_obj_out_dir)
	$(COMPILE.rc) $(RC_OUTPUT_FLAG) $@ $<
  endif # COMPILE.rc

$(VSHDIR)%.$(SOEXT): $(VSHDIR)%.$(OBJEXT)
	$(mk_obj_out_dir)
	$(SOLINK)
endif # SOLINK

#----------------------------------------------------------------------------
#       Library generation targets
#----------------------------------------------------------------------------

#### show_statics shows static objects in locally-created object files.
#### It assumes that the object files were built using g++.
#### TOOLENV selects the proper nm in VxWorks host environments.
#### TOOLDIR allows specification of the full path to nm via definition
#### in the platform_macros.GNU file.
show_statics:
	-@$(TOOLDIR)nm$(TOOLENV) -Co $(VSHDIR)*o | \
	  egrep ' global destructors '; true

#### show_uninit shows uninitialized data in locally-created object files.
#### TOOLENV selects the proper nm in VxWorks host environments.
show_uninit:
	-@$(TOOLDIR)nm$(TOOLENV) -Co $(VSHDIR)*o | egrep ' b '

#----------------------------------------------------------------------------
#       Installation targets
#----------------------------------------------------------------------------

# Always add LIB_INSTALL to INSTALL, so libs will get installed even no BIN
# is built.  LIB_INSTALL is a dependency for BIN, so libs are installed prior
# to use.  Also, make LIB_INSTALL depend on VLIBS, so libs get built prior to
# installation.
ifdef LIB_INSTALL
  INSTALL ?= $(LIB_INSTALL)
  $(LIB_INSTALL): $(VLIBS)
endif

ifneq ($(INSBIN),)
  $(INSBIN)/$(VBIN): $(VBIN)
endif

INSTALL ?=
binout.local: $(INSTALL)

clean_binout.local:
ifdef CLEANUP_INSTALL
  ifneq ($(strip $(CLEANUP_INSTALL)),)
	$(RM) $(CLEANUP_INSTALL)
  endif # CLEANUP_INSTALL
endif # CLEANUP_INSTALL

ifndef LN_S
  ifndef COMSPEC
    ifdef ComSpec
      #### ACE+TAO use COMSPEC, but ComSpec is defined.
      COMSPEC = $(ComSpec)
    endif # ComSpec
  endif # ! COMPSPEC

  ifdef COMSPEC
    #### Assume we're on a WIN32 host.
    LN_S = cp -p
  else  # ! WIN32
    ifeq ($(symlinks),absolute)
      LN_S = ln -s
    else
      LN_S = $(ACE_ROOT)/bin/add_rel_link.sh
    endif # symlinks
  endif # ! WIN32
endif # ! LN_S

# Don't call shell/call directly below since we may need to 
# add the architecture specific suffix to the directory
ifneq ($(mingw32),1)
ARCH_SOURCE_DIR := $(patsubst %/,%,$(shell pwd)/$(ARCH))
else
ARCH_SOURCE_DIR := $(patsubst %/,%,$(call PWD)/$(ARCH))
endif

$(INSBIN)/%$(VAR)$(EXEEXT) \
$(INSINC)/ace% \
$(INSLIB)/% \
$(INSMAN)/man1/% \
$(INSMAN)/man2/% \
$(INSMAN)/man3/% \
$(INSMAN)/man4/% \
$(INSMAN)/man5/% \
$(INSMAN)/man6/% \
$(INSMAN)/man7/% \
$(INSMAN)/man8/% \
$(INSMAN)/manl/% \
$(INSMAN)/mann/% :
ifeq ($(vxworks_ntbuild),1)
	@if not exist $@ $(RM) $(@:%.$(SOEXT)$(SOVERSION)=%.$(SOEXT)) && \
		$(LN_S) $(ARCH_SOURCE_DIR)/$(@F) $(@:%.$(SOEXT)$(SOVERSION)=%.$(SOEXT)) && \
		$(RM) $@ && \
		echo $(LN_S) $(ARCH_SOURCE_DIR)/$(@F) $@ && \
		$(LN_S) $(ARCH_SOURCE_DIR)/$(@F) $@
else #not vxworks_ntbuild
    ifeq ($(firstword $(LN_S)),cp)
      ifneq ($(SOVERSION),)
	@if test $(ARCH_SOURCE_DIR) != $(@D) ; then \
		$(RM) $(@:%.$(SOEXT)$(SOVERSION)=%.$(SOEXT)); \
		echo $(LN_S) $(ARCH_SOURCE_DIR)/$(@F) $(@:%.$(SOEXT)$(SOVERSION)=%.$(SOEXT)); \
		$(LN_S) $(ARCH_SOURCE_DIR)/$(@F) $(@:%.$(SOEXT)$(SOVERSION)=%.$(SOEXT)); \
	fi
      endif #SOVERSION
	@if test $(ARCH_SOURCE_DIR) != $(@D) ; then \
		$(RM) $@; \
		echo $(LN_S) $(ARCH_SOURCE_DIR)/$(@F) $@; \
		$(LN_S) $(ARCH_SOURCE_DIR)/$(@F) $@; \
	fi
    else #firstword of LN_S is not cp, so we're using real symlinks
      ifneq ($(SOVERSION),)
	@if test $(ARCH_SOURCE_DIR) != $(@D) ; then \
		if test ! -s $@ ; then \
			$(RM) $(@:%.$(SOEXT)$(SOVERSION)=%.$(SOEXT)); \
			echo $(LN_S) $(ARCH_SOURCE_DIR)/$(@F) $(@:%.$(SOEXT)$(SOVERSION)=%.$(SOEXT)); \
			$(LN_S) $(ARCH_SOURCE_DIR)/$(@F) $(@:%.$(SOEXT)$(SOVERSION)=%.$(SOEXT)); \
		fi ; \
	fi
      endif #SOVERSION
	@if test $(ARCH_SOURCE_DIR) != $(@D) ; then \
		if test ! -s $@ ; then \
			$(RM) $@; \
			echo $(LN_S) $(ARCH_SOURCE_DIR)/$(@F) $@; \
			$(LN_S) $(ARCH_SOURCE_DIR)/$(@F) $@; \
		fi ; \
	fi
    endif #firstword
endif #vxworks_ntbuild

ifdef MVSLIB
#special for MVS in order to use the .x files
	@if test -w $(@D) ; then \
		if test ! -s $(@:.$(SOEXT)=.x) ; then \
			$(RM) $(@); $(LN_S) $(ARCH_SOURCE_DIR)/$(@F:.$(SOEXT)=.x) $(@:.$(SOEXT)=.x) ; \
		fi \
	fi
endif

#----------------------------------------------------------------------------
#       Cleanup targets
#----------------------------------------------------------------------------

clean_binaries.local:
ifneq ($(BIN_UNCHECKED),)
	-$(RM) $(BIN_UNCHECKED)
endif
ifneq ($(SHLIB_UNCHECKED),)
	-$(RM) $(SHLIB_UNCHECKED)
ifeq ($(versioned_so),1)
	-$(RM) $(SHLIB_UNCHECKED)$(SOVERSION)
endif
endif

clean_idl_stubs.local: makefile_name.local
ifdef IDL_FILES
	-$(RM) $(foreach ext, $(IDL_EXT), $(addsuffix $(ext), $(IDL_FILES)))
endif

CLEANUP_BIN ?=
ifneq ($(CLEANUP_BIN),)
  DO_CLEANUP = 1
  REALCLEAN_FILES += $(CLEANUP_BIN:%=%$(EXEEXT)) $(CLEANUP_BIN:%=%_debug$(EXEEXT)) $(CLEANUP_BIN:%=%_profile$(EXEEXT)) $(CLEANUP_BIN:%=%_optimize$(EXEEXT))
endif # !CLEANUP_BIN

ifdef static_libs
  CLEANUP_LIB ?=
  ifneq ($(CLEANUP_LIB),)
    DO_CLEANUP = 1
    REALCLEAN_FILES += $(CLEANUP_LIB:%=%) $(CLEANUP_LIB:%=%_debug) $(CLEANUP_LIB:%=%_profile) $(CLEANUP_LIB:%=%_optimize)
  endif # !CLEANUP_LIB
endif # static_libs

ifdef shared_libs
  CLEANUP_SHLIB ?=
  ifneq ($(CLEANUP_SHLIB),)
    DO_CLEANUP = 1
    REALCLEAN_FILES += $(CLEANUP_VSHLIB:%=%) $(CLEANUP_VSHLIB_NO_VER:%=%*) $(CLEANUP_VSHLIB_NO_VER:%=%_debug) $(CLEANUP_VSHLIB_NO_VER:%=%_profile) $(CLEANUP_VSHLIB_NO_VER:%=%_optimize)
  endif # !CLEANUP_SHLIB
endif # shared_libs

ifdef OBJS
  CLEANUP_OBJS += $(addprefix $(VDIR), $(OBJS))
endif # OBJS
ifneq ($(CLEANUP_BIN),)
  CLEANUP_OBJS += $(addsuffix .$(OBJEXT),$(addprefix $(VDIR),$(basename $(CLEANUP_BIN:$(ARCH)/%=%)))) $(VOBJS)
endif # CLEANUP_BIN
ifdef VSHOBJS
  CLEANUP_OBJS += $(VSHOBJS)
endif # VSHOBJS

clean.local: makefile_name.local
ifdef DO_CLEANUP
	-$(RM) -r \
	*.$(OBJEXT) *~ *.bak *.rpo *.sym lib*.*_pure_* \
	GNUmakefile.old core-r $(CLEANUP_DIRS) \
	cxx_repository ptrepository ti_files \
	gcctemp.c gcctemp so_locations *.ics \
	templateregistry templateregistry.* ir.out core.* *.core $(CLEANUP_OBJS)
endif # DO_CLEANUP

realclean.local: clean.local clean_binout.local clean_idl_stubs.local
ifdef DO_CLEANUP
	-$(RM) -r $(REALCLEAN_FILES) $(sort $(CLEANUP_OBJDIRS)) $(ARCH)
endif # DO_CLEANUP


#----------------------------------------------------------------------------
#       Dependency generation target
#----------------------------------------------------------------------------

MAKEFILE ?= GNUmakefile
DEPENDENCY_FILE ?= $(MAKEFILE)

TAO_ROOT ?= $(ACE_ROOT)/TAO

# If no libraries are build in the current directory (actually, if
# rules.lib.GNU isn't included), then SOEXT might not be set.
SOEXT ?= so

ORIGdepend.local: $(MAKEFILE)
	@$(RM) $(MAKEFILE).old
	@cp $(MAKEFILE) $(MAKEFILE).old
	$(ACE_ROOT)/bin/g++dep -f $(MAKEFILE) $(CPPFLAGS) -DMAKEDEPEND $(LSRC) $(SRC) $(PSRC)
	@cat $(MAKEFILE) | \
	sed -e "s;$(TAO_ROOT);\$$(TAO_ROOT);g" \
	    -e "s;$(ACE_ROOT);\$$(ACE_ROOT);g" \
	    $(ACE_DEPEND_SED_CMD) \
	    -e '/$$(ACE_ROOT)\/ace\/config[^\.]*\.h/d' \
	    -e "s; /[-a-zA-Z0-9_./]*\.h;;g" \
	    -e "s;\([-a-zA-Z0-9._]*\)\.$(OBJEXT):;.obj/\1.$(OBJEXT) .obj/\1.$(SOEXT) $(VSHDIR)\1.$(OBJEXT) $(VSHDIR)\1.$(SOEXT):;" \
	> $(MAKEFILE).new
	@cat $(MAKEFILE).new >$(MAKEFILE)
	@$(RM) $(MAKEFILE).new
	@if	cmp -s $(MAKEFILE) $(MAKEFILE).old ;\
	then	echo "GNUmakefile dependencies unchanged." ;\
	else \
		echo "GNUmakefile dependencies updated." ;\
	fi ;\
	$(RM) $(MAKEFILE).old ;

ifneq ($(MAKEFILE),$(DEPENDENCY_FILE))
$(DEPENDENCY_FILE):
ifeq ($(lacks_touch),1)
	copy NUL "$(DEPENDENCY_FILE)"
else
	touch $(DEPENDENCY_FILE)
endif
depend.local: $(DEPENDENCY_FILE)
-include $(DEPENDENCY_FILE)
endif

ifdef COMSPEC
  DEPGEN ?= perl $(ACE_ROOT)/bin/depgen.pl
else
  DEPGEN ?= $(ACE_ROOT)/bin/depgen.pl
  DEPGEN_ENV := ACE_PLATFORM_CONFIG=$(ACE_PLATFORM_CONFIG) \
ACE_DEPEND_SED_CMD="$(ACE_DEPEND_SED_CMD)"
endif

# Use findstring just in case the script was preceded by /usr/bin/perl
ifeq ($(findstring depgen.pl, $(notdir $(DEPGEN))), depgen.pl)
  DEPEND_CMD_ARGS += -i
  ifeq ($(inline), 0)
    DEPEND_CMD_ARGS += -n
  endif
endif


depend.local: $(MAKEFILE) idl_stubs.local
	@$(RM) $(DEPENDENCY_FILE).old
	@cp $(DEPENDENCY_FILE) $(DEPENDENCY_FILE).old
	$(DEPGEN_ENV) $(DEPGEN) -A $(DEPEND_CMD_ARGS) -e config.h -f $(DEPENDENCY_FILE) \
	$(CPPFLAGS) -t gnu -DMAKEDEPEND $(CCFLAGS) $(LSRC) $(SRC) $(PSRC)
	@if cmp -s $(DEPENDENCY_FILE) $(DEPENDENCY_FILE).old ;\
	then echo "GNUmakefile dependencies unchanged." ;\
	else \
		echo "GNUmakefile dependencies updated." ;\
	fi ;\
	$(RM) $(DEPENDENCY_FILE).old ;

#----------------------------------------------------------------------------
#       RCS info target
#----------------------------------------------------------------------------

IDL_FILES ?=
IDL_CLIENT_HDR_EXT ?=

idl_stubs.local: $(foreach file, $(IDL_FILES), $(file)$(IDL_CLIENT_HDR_EXT))

#----------------------------------------------------------------------------
#       RCS info target
#----------------------------------------------------------------------------

rcs_info.local:
	@rcs info

#----------------------------------------------------------------------------
#       Variant targets and conditional macros
#----------------------------------------------------------------------------

build.objdirs: $(CLEANUP_DIRS)

ifeq ($(MKDIR_NTSTYLE),1)
$(CLEANUP_DIRS):
	@if not exist $@ $(MKDIR) $@ $(ACE_NUL_STDERR)
else
$(CLEANUP_DIRS):
	@test -d $@ || $(MKDIR) $@ $(ACE_NUL_STDERR)
endif

svnignore.local:
	if [ "n$(ARCH_BIN)" != "n" ]; then \
	  (svn propget svn:ignore .; echo $(ARCH_BIN)) | sort | uniq | sed '/^$$/d' >SvNiGnOrE; \
	  svn propset svn:ignore -F SvNiGnOrE .; \
	  rm SvNiGnOrE; \
	fi
