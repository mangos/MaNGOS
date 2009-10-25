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

#ifndef __TBB_machine_H
#error Do not include this file directly; include tbb_machine.h instead
#endif

#define __TBB_WORDSIZE 8
#define __TBB_BIG_ENDIAN 1

#include <stdint.h>
#include <unistd.h>
#include <sched.h>

extern "C" {

int32_t __TBB_machine_cas_32 (volatile void* ptr, int32_t value, int32_t comparand);
int64_t __TBB_machine_cas_64 (volatile void* ptr, int64_t value, int64_t comparand);
#define __TBB_fence_for_acquire() __TBB_machine_flush ()
#define __TBB_fence_for_release() __TBB_machine_flush ()

}

#define __TBB_CompareAndSwap4(P,V,C) __TBB_machine_cas_32(P,V,C)
#define __TBB_CompareAndSwap8(P,V,C) __TBB_machine_cas_64(P,V,C)
#define __TBB_CompareAndSwapW(P,V,C) __TBB_machine_cas_64(P,V,C)
#define __TBB_Yield() sched_yield()
