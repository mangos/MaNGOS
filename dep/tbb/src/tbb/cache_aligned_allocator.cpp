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

#include "tbb/cache_aligned_allocator.h"
#include "tbb/tbb_allocator.h"
#include "tbb_misc.h"
#include "dynamic_link.h"
#include <cstdlib>

#if _WIN32||_WIN64
#include <windows.h>
#else
#include <dlfcn.h>
#endif /* _WIN32||_WIN64 */

using namespace std;

#if __TBB_WEAK_SYMBOLS

#pragma weak scalable_malloc
#pragma weak scalable_free

extern "C" {
    void* scalable_malloc( size_t );
    void scalable_free( void* );
}

#endif /* __TBB_WEAK_SYMBOLS */

#define __TBB_IS_SCALABLE_MALLOC_FIX_READY 0

namespace tbb {

namespace internal {

//! Dummy routine used for first indirect call via MallocHandler.
static void* DummyMalloc( size_t size );

//! Dummy routine used for first indirect call via FreeHandler.
static void DummyFree( void * ptr );

//! Handler for memory allocation
static void* (*MallocHandler)( size_t size ) = &DummyMalloc;

//! Handler for memory deallocation
static void (*FreeHandler)( void* pointer ) = &DummyFree;

//! Table describing the how to link the handlers.
static const dynamic_link_descriptor MallocLinkTable[] = {
    DLD(scalable_malloc, MallocHandler),
    DLD(scalable_free, FreeHandler),
};

#if __TBB_IS_SCALABLE_MALLOC_FIX_READY 
//! Dummy routine used for first indirect call via padded_allocate_handler.
static void* dummy_padded_allocate( size_t bytes, size_t alignment );

//! Dummy routine used for first indirect call via padded_free_handler.
static void dummy_padded_free( void * ptr );

// ! Allocates memory using standard malloc. It is used when scalable_allocator is not available
static void* padded_allocate( size_t bytes, size_t alignment );

// ! Allocates memory using scalable_malloc
static void* padded_allocate_via_scalable_malloc( size_t bytes, size_t alignment );

// ! Allocates memory using standard free. It is used when scalable_allocator is not available
static void padded_free( void* p );

//! Handler for padded memory allocation
static void* (*padded_allocate_handler)( size_t bytes, size_t alignment ) = &dummy_padded_allocate;

//! Handler for padded memory deallocation
static void (*padded_free_handler)( void* p ) = &dummy_padded_free;

#endif // #if __TBB_IS_SCALABLE_MALLOC_FIX_READY 


#if TBB_USE_DEBUG
#define DEBUG_SUFFIX "_debug"
#else
#define DEBUG_SUFFIX
#endif /* TBB_USE_DEBUG */

// MALLOCLIB_NAME is the name of the TBB memory allocator library.
#if _WIN32||_WIN64
#define MALLOCLIB_NAME "tbbmalloc" DEBUG_SUFFIX ".dll"
#elif __APPLE__
#define MALLOCLIB_NAME "libtbbmalloc" DEBUG_SUFFIX ".dylib"
#elif __linux__
#define MALLOCLIB_NAME "libtbbmalloc" DEBUG_SUFFIX  __TBB_STRING(.so.TBB_COMPATIBLE_INTERFACE_VERSION)
#elif __FreeBSD__ || __sun
#define MALLOCLIB_NAME "libtbbmalloc" DEBUG_SUFFIX ".so"
#else
#error Unknown OS
#endif

//! Initialize the allocation/free handler pointers.
/** Caller is responsible for ensuring this routine is called exactly once.
    The routine attempts to dynamically link with the TBB memory allocator.
    If that allocator is not found, it links to malloc and free. */
void initialize_cache_aligned_allocator() {
    __TBB_ASSERT( MallocHandler==&DummyMalloc, NULL );
    bool success = dynamic_link( MALLOCLIB_NAME, MallocLinkTable, 2 );
    if( !success ) {
        // If unsuccessful, set the handlers to the default routines.
        // This must be done now, and not before FillDynanmicLinks runs, because if other
        // threads call the handlers, we want them to go through the DoOneTimeInitializations logic,
        // which forces them to wait.
        FreeHandler = &free;
        MallocHandler = &malloc;
#if __TBB_IS_SCALABLE_MALLOC_FIX_READY 
        padded_allocate_handler = &padded_allocate;
        padded_free_handler = &padded_free;
    }else{
        padded_allocate_handler = &padded_allocate_via_scalable_malloc;
        __TBB_ASSERT(FreeHandler != &free && FreeHandler != &DummyFree, NULL);
        padded_free_handler = FreeHandler;
#endif // __TBB_IS_SCALABLE_MALLOC_FIX_READY 
    }
#if !__TBB_RML_STATIC
    PrintExtraVersionInfo( "ALLOCATOR", success?"scalable_malloc":"malloc" );
#endif
}

//! Defined in task.cpp
extern void DoOneTimeInitializations();

//! Executed on very first call through MallocHandler
static void* DummyMalloc( size_t size ) {
    DoOneTimeInitializations();
    __TBB_ASSERT( MallocHandler!=&DummyMalloc, NULL );
    return (*MallocHandler)( size );
}

//! Executed on very first call throught FreeHandler
static void DummyFree( void * ptr ) {
    DoOneTimeInitializations();
    __TBB_ASSERT( FreeHandler!=&DummyFree, NULL );
    (*FreeHandler)( ptr );
}

#if __TBB_IS_SCALABLE_MALLOC_FIX_READY 
//! Executed on very first call through padded_allocate_handler
static void* dummy_padded_allocate( size_t bytes, size_t alignment ) {
    DoOneTimeInitializations();
    __TBB_ASSERT( padded_allocate_handler!=&dummy_padded_allocate, NULL );
    return (*padded_allocate_handler)(bytes, alignment);
}

//! Executed on very first call throught padded_free_handler
static void dummy_padded_free( void * ptr ) {
    DoOneTimeInitializations();
    __TBB_ASSERT( padded_free_handler!=&dummy_padded_free, NULL );
    (*padded_free_handler)( ptr );
}    
#endif // __TBB_IS_SCALABLE_MALLOC_FIX_READY 

static size_t NFS_LineSize = 128;

size_t NFS_GetLineSize() {
    return NFS_LineSize;
}

//! Requests for blocks this size and higher are handled via malloc/free,
const size_t BigSize = 4096;

#if _MSC_VER && !defined(__INTEL_COMPILER)
    // unary minus operator applied to unsigned type, result still unsigned
    #pragma warning( disable: 4146 4706 )
#endif

void* NFS_Allocate( size_t n, size_t element_size, void* /*hint*/ ) {
    size_t m = NFS_LineSize;
    __TBB_ASSERT( m<=NFS_MaxLineSize, "illegal value for NFS_LineSize" );
    __TBB_ASSERT( (m & m-1)==0, "must be power of two" );
    size_t bytes = n*element_size;
#if __TBB_IS_SCALABLE_MALLOC_FIX_READY 

    if (bytes<n || bytes+m<bytes) {
        // Overflow
        throw bad_alloc();
    }
    
    void* result = (*padded_allocate_handler)( bytes, m );
#else
    unsigned char* base;
    if( bytes<n || bytes+m<bytes || !(base=(unsigned char*)(bytes>=BigSize?malloc(m+bytes):(*MallocHandler)(m+bytes))) ) {
        // Overflow
        throw bad_alloc();
    }
    // Round up to next line
    unsigned char* result = (unsigned char*)((uintptr)(base+m)&-m);
    // Record where block actually starts.  Use low order bit to record whether we used malloc or MallocHandler.
    ((uintptr*)result)[-1] = uintptr(base)|(bytes>=BigSize);
#endif // __TBB_IS_SCALABLE_MALLOC_FIX_READY    
    /** The test may fail with TBB_IS_SCALABLE_MALLOC_FIX_READY = 1 
        because scalable_malloc returns addresses aligned to 64 when large block is allocated */
    __TBB_ASSERT( ((size_t)result&(m-1)) == 0, "The address returned isn't aligned to cache line size" );
    return result;
}

void NFS_Free( void* p ) {
#if __TBB_IS_SCALABLE_MALLOC_FIX_READY 
    (*padded_free_handler)( p );
#else
    if( p ) {
        __TBB_ASSERT( (uintptr)p>=0x4096, "attempt to free block not obtained from cache_aligned_allocator" );
        // Recover where block actually starts
        unsigned char* base = ((unsigned char**)p)[-1];
        __TBB_ASSERT( (void*)((uintptr)(base+NFS_LineSize)&-NFS_LineSize)==p, "not allocated by NFS_Allocate?" );
        if( uintptr(base)&1 ) {
            // Is a big block - use free
            free(base-1);
        } else {
            // Is a small block - use scalable allocator
            (*FreeHandler)( base );
        }
    }
#endif // __TBB_IS_SCALABLE_MALLOC_FIX_READY
}

#if __TBB_IS_SCALABLE_MALLOC_FIX_READY
static void* padded_allocate_via_scalable_malloc( size_t bytes, size_t alignment  ) {  
    unsigned char* base;
    if( !(base=(unsigned char*)(*MallocHandler)((bytes+alignment)&-alignment))) {
        throw bad_alloc();
    }        
    return base; // scalable_malloc returns aligned pointer
}

static void* padded_allocate( size_t bytes, size_t alignment ) {    
    unsigned char* base;
    if( !(base=(unsigned char*)malloc(alignment+bytes)) ) {        
        throw bad_alloc();
    }
    // Round up to the next line
    unsigned char* result = (unsigned char*)((uintptr)(base+alignment)&-alignment);
    // Record where block actually starts.
    ((uintptr*)result)[-1] = uintptr(base);
    return result;    
}

static void padded_free( void* p ) {
    if( p ) {
        __TBB_ASSERT( (uintptr)p>=0x4096, "attempt to free block not obtained from cache_aligned_allocator" );
        // Recover where block actually starts
        unsigned char* base = ((unsigned char**)p)[-1];
        __TBB_ASSERT( (void*)((uintptr)(base+NFS_LineSize)&-NFS_LineSize)==p, "not allocated by NFS_Allocate?" );
        free(base);
    }
}
#endif // #if __TBB_IS_SCALABLE_MALLOC_FIX_READY

void* __TBB_EXPORTED_FUNC allocate_via_handler_v3( size_t n ) {    
    void* result;
    result = (*MallocHandler) (n);
    if (!result) {
        // Overflow
        throw bad_alloc();
    }
    return result;
}

void __TBB_EXPORTED_FUNC deallocate_via_handler_v3( void *p ) {
    if( p ) {        
        (*FreeHandler)( p );
    }
}

bool __TBB_EXPORTED_FUNC is_malloc_used_v3() {
    if (MallocHandler == &DummyMalloc) {
        void* void_ptr = (*MallocHandler)(1);
        (*FreeHandler)(void_ptr);
    }
    __TBB_ASSERT( MallocHandler!=&DummyMalloc && FreeHandler!=&DummyFree, NULL );
    __TBB_ASSERT(MallocHandler==&malloc && FreeHandler==&free ||
                  MallocHandler!=&malloc && FreeHandler!=&free, NULL );
    return MallocHandler == &malloc;
}

} // namespace internal

} // namespace tbb

#if __TBB_RML_STATIC
#include "tbb/atomic.h"
static tbb::atomic<int> module_inited;
namespace tbb {
namespace internal {
void DoOneTimeInitializations() {
    if( module_inited!=2 ) {
        if( module_inited.compare_and_swap(1, 0)==0 ) {
            initialize_cache_aligned_allocator();
            module_inited = 2;
        } else {
            do {
                __TBB_Yield();
            } while( module_inited!=2 );
        }
    }
}
}} //namespace tbb::internal
#endif
