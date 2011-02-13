/*
    Copyright 2005-2009 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

// Please define version number in the file:
#include "tbb/tbb_stddef.h"

// And don't touch anything below
#ifndef ENDL
#define ENDL "\n"
#endif
#include "version_string.tmp"

#ifndef __TBB_VERSION_STRINGS
#pragma message("Warning: version_string.tmp isn't generated properly by version_info.sh script!")
// here is an example of macros value:
#define __TBB_VERSION_STRINGS \
"TBB: BUILD_HOST\tUnknown\n" \
"TBB: BUILD_ARCH\tUnknown\n" \
"TBB: BUILD_OS\t\tUnknown\n" \
"TBB: BUILD_CL\t\tUnknown\n" \
"TBB: BUILD_COMPILER\tUnknown\n" \
"TBB: BUILD_COMMAND\tUnknown\n"
#endif
#ifndef __TBB_DATETIME
#ifdef RC_INVOKED
#define __TBB_DATETIME "Unknown"
#else
#define __TBB_DATETIME __DATE__ __TIME__
#endif
#endif

#define __TBB_VERSION_NUMBER "TBB: VERSION\t\t" __TBB_STRING(TBB_VERSION_MAJOR.TBB_VERSION_MINOR) ENDL
#define __TBB_INTERFACE_VERSION_NUMBER "TBB: INTERFACE VERSION\t" __TBB_STRING(TBB_INTERFACE_VERSION) ENDL
#define __TBB_VERSION_DATETIME "TBB: BUILD_DATE\t\t" __TBB_DATETIME ENDL
#ifndef TBB_USE_DEBUG
    #define __TBB_VERSION_USE_DEBUG "TBB: TBB_USE_DEBUG\tundefined" ENDL
#elif TBB_USE_DEBUG==0
    #define __TBB_VERSION_USE_DEBUG "TBB: TBB_USE_DEBUG\t0" ENDL
#elif TBB_USE_DEBUG==1
    #define __TBB_VERSION_USE_DEBUG "TBB: TBB_USE_DEBUG\t1" ENDL
#elif TBB_USE_DEBUG==2
    #define __TBB_VERSION_USE_DEBUG "TBB: TBB_USE_DEBUG\t2" ENDL
#else
    #error Unexpected value for TBB_USE_DEBUG
#endif
#ifndef TBB_USE_ASSERT
    #define __TBB_VERSION_USE_ASSERT "TBB: TBB_USE_ASSERT\tundefined" ENDL
#elif TBB_USE_ASSERT==0
    #define __TBB_VERSION_USE_ASSERT "TBB: TBB_USE_ASSERT\t0" ENDL
#elif TBB_USE_ASSERT==1
    #define __TBB_VERSION_USE_ASSERT "TBB: TBB_USE_ASSERT\t1" ENDL
#elif TBB_USE_ASSERT==2
    #define __TBB_VERSION_USE_ASSERT "TBB: TBB_USE_ASSERT\t2" ENDL
#else
    #error Unexpected value for TBB_USE_ASSERT
#endif
#ifndef DO_ITT_NOTIFY
    #define __TBB_VERSION_DO_NOTIFY "TBB: DO_ITT_NOTIFY\tundefined" ENDL
#elif DO_ITT_NOTIFY==1
    #define __TBB_VERSION_DO_NOTIFY "TBB: DO_ITT_NOTIFY\t1" ENDL
#elif DO_ITT_NOTIFY==0
    #define __TBB_VERSION_DO_NOTIFY
#else
    #error Unexpected value for DO_ITT_NOTIFY
#endif

#define TBB_VERSION_STRINGS __TBB_VERSION_NUMBER __TBB_INTERFACE_VERSION_NUMBER __TBB_VERSION_DATETIME __TBB_VERSION_STRINGS __TBB_VERSION_USE_DEBUG __TBB_VERSION_USE_ASSERT __TBB_VERSION_DO_NOTIFY

// numbers
#ifndef __TBB_VERSION_YMD
#define __TBB_VERSION_YMD 0, 0
#endif

#define TBB_VERNUMBERS TBB_VERSION_MAJOR, TBB_VERSION_MINOR, __TBB_VERSION_YMD

#define TBB_VERSION __TBB_STRING(TBB_VERNUMBERS)
