# $Id: platform_integrity_ghs.GNU 91285 2010-08-05 08:29:30Z johnnyw $
CROSS-COMPILE = 1

debug ?= 1
optimize ?= 1

shared_libs =
static_libs = 1

#### CPU type
ifndef CPU
  CPU = ppc
endif # CPU

INTEGRITYTARGET = 1

#### Green Hills location, and target-specific definitions.
ifndef GHSROOT
  GHSROOT = /home/scooter2/i40_solaris2
endif

ifndef RTOSROOT
  RTOSROOT = /export/integrity/rtos
endif

ifndef BSP
  BSP = mcp750
endif

ifndef TARGET_BSP
  TARGET_BSP = $(ACE_ROOT)/tests/ACE.bsp
endif

ifndef TARGET_LD
  TARGET_LD = $(ACE_ROOT)/tests/INTEGRITY.ld
endif

#### Set Integrity version
verline := $(shell ($(GHSROOT)/gversion 2>&1|grep "Green Hills Software"))
ifeq "$(verline)" ""
  $(error Not Green Hills or the environment is not set-up)
endif
INTEGRITY_VERSION_STR := $(strip $(if ifeq $(word 4, $(verline)) "INTEGRITY", $(word 5, $(verline))))
ifeq "$(INTEGRITY_VERSION_STR)" ""
  $(error This Green Hills setup is not for Integrity)
endif
ifeq "$(INTEGRITY_VERSION_STR)" "4.1.8"
  CCFLAGS += -DINTEGRITY_VERSION=40108
endif

CC = $(GHSROOT)/cc$(CPU)
CXX = $(GHSROOT)/cx$(CPU)

# undefine "vector" because it's predefined for 7400 boards
IOPS = -bspname=$(RTOSROOT)/target/$(BSP).bld -os_dir $(RTOSROOT) --one_instantiation_per_object -non_shared -Uvector
IOPS += --exceptions

CCFLAGS  += $(CFLAGS) -integrate -dynamic -I$(RTOSROOT)/INTEGRITY-include-SCA $(IOPS) -bspfile=$(TARGET_BSP) $(TARGET_LD)
DCFLAGS  += -G
SOFLAGS  += -shared
DLD      = $(CXX)
LD       = $(CXX)
LIBS    += -lshm_client -lnet -livfssca -lposixsca -lsocket
PIC      =
AR := $(CXX) -archive $(IOPS)
ARFLAGS = -o

