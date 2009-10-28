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

#include "TypeDefinitions.h" // Customize.h and proxy.h get included

#include "tbb/itt_notify.cpp"

#if MALLOC_CHECK_RECURSION

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#if __sun
#include <string.h> /* for memset */
#include <errno.h>
#endif

#if MALLOC_LD_PRELOAD

extern "C" {

void   safer_scalable_free( void*, void (*)(void*) );
void * safer_scalable_realloc( void*, size_t, void* );

bool __TBB_internal_find_original_malloc(int num, const char *names[], void *table[])  __attribute__ ((weak));

}

#endif /* MALLOC_LD_PRELOAD */
#endif /* MALLOC_CHECK_RECURSION */

namespace rml {
namespace internal {

#if MALLOC_CHECK_RECURSION

void* (*original_malloc_ptr)(size_t) = 0;
void  (*original_free_ptr)(void*) = 0;
static void* (*original_calloc_ptr)(size_t,size_t) = 0;
static void* (*original_realloc_ptr)(void*,size_t) = 0;

#endif /* MALLOC_CHECK_RECURSION */

#if __TBB_NEW_ITT_NOTIFY
extern "C" 
#endif
void ITT_DoOneTimeInitialization() {} // required for itt_notify.cpp to work

#if DO_ITT_NOTIFY
/** Caller is responsible for ensuring this routine is called exactly once. */
void MallocInitializeITT() {
#if __TBB_NEW_ITT_NOTIFY
    tbb::internal::__TBB_load_ittnotify();
#else
    bool success = false;
    // Check if we are running under control of VTune.
    if( GetBoolEnvironmentVariable("KMP_FOR_TCHECK") || GetBoolEnvironmentVariable("KMP_FOR_TPROFILE") ) {
        // Yes, we are under control of VTune.  Check for libittnotify library.
        success = dynamic_link( LIBITTNOTIFY_NAME, ITT_HandlerTable, 5 );
    }
    if (!success){
        for (int i = 0; i < 5; i++)
            *ITT_HandlerTable[i].handler = NULL;
    }
#endif /* !__TBB_NEW_ITT_NOTIFY */
}
#endif /* DO_ITT_NOTIFY */

void init_tbbmalloc() {
#if MALLOC_LD_PRELOAD
    if (malloc_proxy && __TBB_internal_find_original_malloc) {
        const char *alloc_names[] = { "malloc", "free", "realloc", "calloc"};
        void *orig_alloc_ptrs[4];

        if (__TBB_internal_find_original_malloc(4, alloc_names, orig_alloc_ptrs)) {
            (void *&)original_malloc_ptr  = orig_alloc_ptrs[0];
            (void *&)original_free_ptr    = orig_alloc_ptrs[1];
            (void *&)original_realloc_ptr = orig_alloc_ptrs[2];
            (void *&)original_calloc_ptr  = orig_alloc_ptrs[3];
            MALLOC_ASSERT( original_malloc_ptr!=malloc_proxy,
                           "standard malloc not found" );
/* It's workaround for a bug in GNU Libc 2.9 (as it shipped with Fedora 10).
   1st call to libc's malloc should be not from threaded code.
 */
            original_free_ptr(original_malloc_ptr(1024));
            original_malloc_found = 1;
        }
    }
#endif /* MALLOC_LD_PRELOAD */

#if DO_ITT_NOTIFY
    MallocInitializeITT();
#endif
}

#if !(_WIN32||_WIN64)
struct RegisterProcessShutdownNotification {
    ~RegisterProcessShutdownNotification() {
        mallocProcessShutdownNotification();
    }
};

static RegisterProcessShutdownNotification reg;
#endif

#if MALLOC_CHECK_RECURSION

bool  original_malloc_found;

#if MALLOC_LD_PRELOAD

extern "C" {

void * __TBB_internal_malloc(size_t size)
{
    return scalable_malloc(size);
}

void * __TBB_internal_calloc(size_t num, size_t size)
{
    return scalable_calloc(num, size);
}

int __TBB_internal_posix_memalign(void **memptr, size_t alignment, size_t size)
{
    return scalable_posix_memalign(memptr, alignment, size);
}

void* __TBB_internal_realloc(void* ptr, size_t sz)
{
    return safer_scalable_realloc(ptr, sz, (void*&)original_realloc_ptr);
}

void __TBB_internal_free(void *object)
{
    safer_scalable_free(object, original_free_ptr);
}

} /* extern "C" */

#endif /* MALLOC_LD_PRELOAD */
#endif /* MALLOC_CHECK_RECURSION */

} } // namespaces

#ifdef _WIN32
#include <windows.h>

extern "C" BOOL WINAPI DllMain( HINSTANCE hInst, DWORD callReason, LPVOID )
{

    if (callReason==DLL_THREAD_DETACH)
    {
        mallocThreadShutdownNotification(NULL);
    }
    else if (callReason==DLL_PROCESS_DETACH)
    {
        mallocProcessShutdownNotification();
    }
    return TRUE;
}

#endif //_WIN32

