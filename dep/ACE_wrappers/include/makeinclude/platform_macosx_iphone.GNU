# $Id: platform_macosx_iphone.GNU 88772 2010-01-28 16:31:28Z sowayaa $

static_libs_only=1

include $(ACE_ROOT)/include/makeinclude/platform_macosx_common.GNU

XCODE:=/Developer

ifeq ($(IPHONE_TARGET), SIMULATOR)
  IPHONE_PLATFORM:=$(XCODE)/Platforms/iPhoneSimulator.platform/Developer
  IPHONE_SDK:=$(IPHONE_PLATFORM)/SDKs/iPhoneSimulator${IPHONE_VERSION}.sdk
  IPHONE_HARDWARE_ARCHITECTURE=i386

  # crt1.10.6.o is missing under MacOSX 10.6, fallback to 10.5
  # until apple provides the missing crt1.10.6.o
  CFLAGS   += -mmacosx-version-min=10.5
  CXXFLAGS += -mmacosx-version-min=10.5
  LDFLAGS  += -mmacosx-version-min=10.5
endif

ifeq ($(IPHONE_TARGET), HARDWARE)
  CROSS-COMPILE=1
  IPHONE_PLATFORM:=$(XCODE)/Platforms/iPhoneOS.platform/Developer
  IPHONE_SDK:=$(IPHONE_PLATFORM)/SDKs/iPhoneOS${IPHONE_VERSION}.sdk
  IPHONE_HARDWARE_ARCHITECTURE=armv6
  CODESIGN_ALLOCATE:=$(IPHONE_PLATFORM)/usr/bin/codesign_allocate
  POSTLINK=; codesign -f -s "iPhone Developer" $(BIN)
  rwho = 0
endif

CC:=$(IPHONE_PLATFORM)/usr/bin/gcc
CXX:=$(IPHONE_PLATFORM)/usr/bin/g++
AR:=$(IPHONE_PLATFORM)/usr/bin/ar
RANLIB:=$(IPHONE_PLATFORM)/usr/bin/ranlib
DLD = $(CXX)
LD  = $(CXX)
CFLAGS   += -arch $(IPHONE_HARDWARE_ARCHITECTURE) -isysroot $(IPHONE_SDK)
CXXFLAGS += -arch $(IPHONE_HARDWARE_ARCHITECTURE) -isysroot $(IPHONE_SDK)

ifneq (,$(HOST_ROOT))
  TAO_IDLFLAGS += -g $(HOST_ROOT)/bin/ace_gperf
  TAO_IDL = $(HOST_ROOT)/bin/tao_idl
  TAO_IDL3_TO_IDL2 = $(HOST_ROOT)/bin/tao_idl3_to_idl2
  TAO_IDL_DEP = $(TAO_IDL)
  TAO_IDL3_TO_IDL2_DEP = $(TAO_IDL3_TO_IDL2)
  # make sure to use the target compiler, not the cross-compiler
  # as preprocessor for the cross-compiled idl tools
  TAO_IDL_PREPROCESSOR = gcc
endif

