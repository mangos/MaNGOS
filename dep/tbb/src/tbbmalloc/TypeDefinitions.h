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

#ifndef _itt_shared_malloc_TypeDefinitions_H_
#define _itt_shared_malloc_TypeDefinitions_H_

// Define preprocessor symbols used to determine architecture
#if _WIN32||_WIN64
#   if defined(_M_AMD64)
#       define __ARCH_x86_64 1
#   elif defined(_M_IA64)
#       define __ARCH_ipf 1
#   elif defined(_M_IX86)||defined(__i386__) // the latter for MinGW support
#       define __ARCH_x86_32 1
#   else
#       error Unknown processor architecture for Windows
#   endif
#   define USE_WINTHREAD 1
#else /* Assume generic Unix */
#   if __x86_64__
#       define __ARCH_x86_64 1
#   elif __ia64__
#       define __ARCH_ipf 1
#   elif __i386__ || __i386
#       define __ARCH_x86_32 1
#   else
#       define __ARCH_other 1
#   endif
#   define USE_PTHREAD 1
#endif

// Include files containing declarations of intptr_t and uintptr_t
#include <stddef.h>  // size_t
#if _MSC_VER
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

namespace rml {
namespace internal {

extern bool  original_malloc_found;
extern void* (*original_malloc_ptr)(size_t);
extern void  (*original_free_ptr)(void*);

} } // namespaces

//! PROVIDE YOUR OWN Customize.h IF YOU FEEL NECESSARY
#include "Customize.h"

/*
 * Functions to align an integer down or up to the given power of two,
 * and test for such an alignment, and for power of two.
 */
template<typename T>
static inline T alignDown(T arg, uintptr_t alignment) {
    return T( (uintptr_t)arg                & ~(alignment-1));
}
template<typename T>
static inline T alignUp  (T arg, uintptr_t alignment) {
    return T(((uintptr_t)arg+(alignment-1)) & ~(alignment-1));
    // /*is this better?*/ return (((uintptr_t)arg-1) | (alignment-1)) + 1;
}
template<typename T>
static inline bool isAligned(T arg, uintptr_t alignment) {
    return 0==((uintptr_t)arg & (alignment-1));
}
static inline bool isPowerOfTwo(uintptr_t arg) {
    return arg && (0==(arg & (arg-1)));
}
static inline bool isPowerOfTwoMultiple(uintptr_t arg, uintptr_t divisor) {
    // Divisor is assumed to be a power of two (which is valid for current uses).
    MALLOC_ASSERT( isPowerOfTwo(divisor), "Divisor should be a power of two" );
    return arg && (0==(arg & (arg-divisor)));
}

#endif /* _itt_shared_malloc_TypeDefinitions_H_ */
