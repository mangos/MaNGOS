# -*- Makefile -*-

#----------------------------------------------------------------------------
#  $Id: component_check.GNU 80826 2008-03-04 14:51:23Z wotte $
#----------------------------------------------------------------------------

# Load all components.
ALL_COMPONENTS := $(ACE_MAKE_OPTIONS)
ALL_COMPONENTS += $(shell sh $(ACE_ROOT)/bin/ace_components --ace)
ALL_COMPONENTS += $(shell sh $(ACE_ROOT)/bin/ace_components --tao)
ALL_COMPONENTS += $(shell sh $(ACE_ROOT)/bin/ace_components --orbsvcs)
ALL_COMPONENTS += $(shell sh $(ACE_ROOT)/bin/ace_components --pace)

# Filter out all the components that are currently available from the
# list of requirements.
MISSING_COMPONENTS := $(filter-out $(ALL_COMPONENTS), $(REQUIRED_COMPONENTS))
