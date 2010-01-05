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

#ifndef _itt_shared_malloc_MapMemory_H
#define _itt_shared_malloc_MapMemory_H

#if __linux__ || __APPLE__ || __sun || __FreeBSD__

#if __sun && !defined(_XPG4_2)
 // To have void* as mmap's 1st argument
 #define _XPG4_2 1
 #define XPG4_WAS_DEFINED 1
#endif

#include <sys/mman.h>

#if XPG4_WAS_DEFINED
 #undef _XPG4_2
 #undef XPG4_WAS_DEFINED
#endif

#define MEMORY_MAPPING_USES_MALLOC 0
void* MapMemory (size_t bytes)
{
    void* result = 0;
#ifndef MAP_ANONYMOUS
// Mac OS* X defines MAP_ANON, which is deprecated in Linux.
#define MAP_ANONYMOUS MAP_ANON
#endif /* MAP_ANONYMOUS */
    result = mmap(result, bytes, (PROT_READ | PROT_WRITE), MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    return result==MAP_FAILED? 0: result;
}

int UnmapMemory(void *area, size_t bytes)
{
    return munmap(area, bytes);
}

#elif _WIN32 || _WIN64
#include <windows.h>

#define MEMORY_MAPPING_USES_MALLOC 0
void* MapMemory (size_t bytes)
{
    /* Is VirtualAlloc thread safe? */
    return VirtualAlloc(NULL, bytes, (MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN), PAGE_READWRITE);
}

int UnmapMemory(void *area, size_t bytes)
{
    BOOL result = VirtualFree(area, 0, MEM_RELEASE);
    return !result;
}

#else
#include <stdlib.h>

#define MEMORY_MAPPING_USES_MALLOC 1
void* MapMemory (size_t bytes)
{
    return malloc( bytes );
}

int UnmapMemory(void *area, size_t bytes)
{
    free( area );
    return 0;
}

#endif /* OS dependent */

#if MALLOC_CHECK_RECURSION && MEMORY_MAPPING_USES_MALLOC
#error Impossible to protect against malloc recursion when memory mapping uses malloc.
#endif

#endif /* _itt_shared_malloc_MapMemory_H */
