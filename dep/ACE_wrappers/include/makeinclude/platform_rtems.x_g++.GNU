# $Id: platform_rtems.x_g++.GNU 91285 2010-08-05 08:29:30Z johnnyw $
#
# RTEMS 4.6.6 and later with GNU g++.

CROSS-COMPILE = 1

debug ?= 1
optimize ?= 1
threads ?= 1
pipes ?= 1

shared_libs =
static_libs = 1
rwho = 0

ifneq (,$(HOST_ROOT))
  TAO_IDL = $(HOST_ROOT)/bin/tao_idl
  TAO_IDL3_TO_IDL2 = $(HOST_ROOT)/TAO/CIAO/bin/tao_idl3_to_idl2
  TAO_IDL_DEP = $(TAO_IDL)
  TAO_IDL3_TO_IDL2_DEP = $(TAO_IDL3_TO_IDL2)
endif

ifeq (,$(RTEMS_MAKEFILE_PATH))
  default:
	@ERROR: you must set your RTEMS_MAKEFILE_PATH environment variable
endif # RTEMS_MAKEFILE_PATH

ifeq (,$(PERL_PATH))
  PERL_PATH = perl
endif # ! PERL_PATH

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_MAKEFILE_PATH)/make/target.cfg

# Test for template instantiation.
include $(ACE_ROOT)/include/makeinclude/platform_g++_common.GNU

ifeq (no,$(RTEMS_HAS_NETWORKING))
 CFLAGS += -DACE_LACKS_NETWORKING
 CCFLAGS += -DACE_LACKS_NETWORKING
endif

CFLAGS          += -specs bsp_specs -qrtems -D_REENTRANT -fasm -fno-builtin -fno-defer-pop -fvolatile -W -Wall -DACE_HAS_RTEMS -B$(RTEMS_MAKEFILE_PATH)/lib
CCFLAGS         += -specs bsp_specs -qrtems -D_REENTRANT -fasm -fno-builtin -fno-defer-pop -fvolatile -W -Wall -DACE_HAS_RTEMS -B$(RTEMS_MAKEFILE_PATH)/lib
DCFLAGS         += -g
DCCFLAGS        += -g
DLD             = $(LD)
INCLDIRS        +=

LD              = $(CXX)
LDFLAGS         += -Wl,--allow-multiple-definition -B$(RTEMS_MAKEFILE_PATH)/lib -Wl,-Ttext,0x00100000
OCFLAGS         += -O
PIC             =

