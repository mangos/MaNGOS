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

#ifndef _TBB_malloc_proxy_H_
#define _TBB_malloc_proxy_H_

#if __linux__
#define MALLOC_LD_PRELOAD 1
#endif

// MALLOC_LD_PRELOAD depends on MALLOC_CHECK_RECURSION stuff
#if __linux__ || __APPLE__ || __sun || __FreeBSD__ || MALLOC_LD_PRELOAD
#define MALLOC_CHECK_RECURSION 1
#endif

#include <stddef.h>

extern "C" {
    void * scalable_malloc(size_t size);
    void * scalable_calloc(size_t nobj, size_t size);
    void   scalable_free(void *ptr);
    void * scalable_realloc(void* ptr, size_t size);
    void * scalable_aligned_malloc(size_t size, size_t alignment);
    void * scalable_aligned_realloc(void* ptr, size_t size, size_t alignment);
    int    scalable_posix_memalign(void **memptr, size_t alignment, size_t size);
    size_t scalable_msize(void *ptr);
    void   safer_scalable_free( void *ptr, void (*original_free)(void*));
    void * safer_scalable_realloc( void *ptr, size_t, void* );
    void * safer_scalable_aligned_realloc( void *ptr, size_t, size_t, void* );
    size_t safer_scalable_msize( void *ptr, size_t (*orig_msize_crt80d)(void*));

    void * __TBB_internal_malloc(size_t size);
    void * __TBB_internal_calloc(size_t num, size_t size);
    void   __TBB_internal_free(void *ptr);
    void * __TBB_internal_realloc(void* ptr, size_t sz);
    int    __TBB_internal_posix_memalign(void **memptr, size_t alignment, size_t size);
    
    bool   __TBB_internal_find_original_malloc(int num, const char *names[], void *table[]);
} // extern "C"

// Struct with original free() and _msize() pointers
struct orig_ptrs {
    void   (*orig_free) (void*);  
    size_t (*orig_msize)(void*); 
};

#endif /* _TBB_malloc_proxy_H_ */
