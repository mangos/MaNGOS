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

#include "proxy.h"

#if MALLOC_LD_PRELOAD

/*** service functions and variables ***/

#include <unistd.h> // for sysconf
#include <dlfcn.h>

static long memoryPageSize;

static inline void initPageSize()
{
    memoryPageSize = sysconf(_SC_PAGESIZE);
}

/* For the expected behaviour (i.e., finding malloc/free/etc from libc.so, 
   not from ld-linux.so) dlsym(RTLD_NEXT) should be called from 
   a LD_PRELOADed library, not another dynamic library.
   So we have to put find_original_malloc here.
 */
extern "C" bool __TBB_internal_find_original_malloc(int num, const char *names[],
                                                        void *ptrs[])
{
    for (int i=0; i<num; i++)
        if (NULL == (ptrs[i] = dlsym (RTLD_NEXT, names[i])))
            return false;

    return true;
}

/* __TBB_malloc_proxy used as a weak symbol by libtbbmalloc for: 
   1) detection that the proxy library is loaded
   2) check that dlsym("malloc") found something different from our replacement malloc
*/
extern "C" void *__TBB_malloc_proxy() __attribute__ ((alias ("malloc")));

#ifndef __THROW
#define __THROW
#endif

/*** replacements for malloc and the family ***/

extern "C" {

void *malloc(size_t size) __THROW
{
    return __TBB_internal_malloc(size);
}

void * calloc(size_t num, size_t size) __THROW
{
    return __TBB_internal_calloc(num, size);
}

void free(void *object) __THROW
{
    __TBB_internal_free(object);
}

void * realloc(void* ptr, size_t sz) __THROW
{
    return __TBB_internal_realloc(ptr, sz);
}

int posix_memalign(void **memptr, size_t alignment, size_t size) __THROW
{
    return __TBB_internal_posix_memalign(memptr, alignment, size);
}

/* The older *NIX interface for aligned allocations;
   it's formally substituted by posix_memalign and deprecated,
   so we do not expect it to cause cyclic dependency with C RTL. */
void * memalign(size_t alignment, size_t size)  __THROW
{
    return scalable_aligned_malloc(size, alignment);
}

/* valloc allocates memory aligned on a page boundary */
void * valloc(size_t size) __THROW
{
    if (! memoryPageSize) initPageSize();

    return scalable_aligned_malloc(size, memoryPageSize);
}

/* pvalloc allocates smallest set of complete pages which can hold 
   the requested number of bytes. Result is aligned on page boundary. */
void * pvalloc(size_t size) __THROW
{
    if (! memoryPageSize) initPageSize();
    // align size up to the page size
    size = ((size-1) | (memoryPageSize-1)) + 1;

    return scalable_aligned_malloc(size, memoryPageSize);
}

int mallopt(int /*param*/, int /*value*/) __THROW
{
    return 1;
}

} /* extern "C" */

#if __linux__
#include <malloc.h>
#include <string.h> // for memset

extern "C" struct mallinfo mallinfo() __THROW
{
    struct mallinfo m;
    memset(&m, 0, sizeof(struct mallinfo));

    return m;
}
#endif /* __linux__ */

/*** replacements for global operators new and delete ***/

#include <new>

void * operator new(size_t sz) throw (std::bad_alloc) {
    void *res = scalable_malloc(sz);
    if (NULL == res) throw std::bad_alloc();
    return res;
}
void* operator new[](size_t sz) throw (std::bad_alloc) {
    void *res = scalable_malloc(sz);
    if (NULL == res) throw std::bad_alloc();
    return res;
}
void operator delete(void* ptr) throw() {
    scalable_free(ptr);
}
void operator delete[](void* ptr) throw() {
    scalable_free(ptr);
}
void* operator new(size_t sz, const std::nothrow_t&) throw() {
    return scalable_malloc(sz);
}
void* operator new[](std::size_t sz, const std::nothrow_t&) throw() {
    return scalable_malloc(sz);
}
void operator delete(void* ptr, const std::nothrow_t&) throw() {
    scalable_free(ptr);
}
void operator delete[](void* ptr, const std::nothrow_t&) throw() {
    scalable_free(ptr);
}

#endif /* MALLOC_LD_PRELOAD */


#ifdef _WIN32
#include <windows.h>

#include <stdio.h>
#include "tbb_function_replacement.h"

void safer_scalable_free2( void *ptr)
{
    safer_scalable_free( ptr, NULL );
}

// we do not support _expand();
void* safer_expand( void *, size_t )
{
    return NULL;
}

#define __TBB_QV(EXP) #EXP
#define __TBB_ORIG_ALLOCATOR_REPLACEMENT_WRAPPER(CRTLIB)\
void (*orig_free_##CRTLIB)(void*);                                                        \
void safer_scalable_free_##CRTLIB( void *ptr)                                             \
{                                                                                         \
    safer_scalable_free( ptr, orig_free_##CRTLIB );                                       \
}                                                                                         \
                                                                                          \
size_t (*orig_msize_##CRTLIB)(void*);                                                     \
size_t safer_scalable_msize_##CRTLIB( void *ptr)                                          \
{                                                                                         \
    return safer_scalable_msize( ptr, orig_msize_##CRTLIB );                              \
}                                                                                         \
                                                                                          \
void* safer_scalable_realloc_##CRTLIB( void *ptr, size_t size )                           \
{                                                                                         \
    orig_ptrs func_ptrs = {orig_free_##CRTLIB, orig_msize_##CRTLIB};                      \
    return safer_scalable_realloc( ptr, size, &func_ptrs );                               \
}                                                                                         \
                                                                                          \
void* safer_scalable_aligned_realloc_##CRTLIB( void *ptr, size_t size, size_t aligment )  \
{                                                                                         \
    orig_ptrs func_ptrs = {orig_free_##CRTLIB, orig_msize_##CRTLIB};                      \
    return safer_scalable_aligned_realloc( ptr, size, aligment, &func_ptrs );             \
} 

#if _WIN64
#define __TBB_ORIG_ALLOCATOR_REPLACEMENT_CALL(CRT_VER)\
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ## d.dll), "free",  (FUNCPTR)safer_scalable_free_ ## CRT_VER ## d,  9, (FUNCPTR*)&orig_free_ ## CRT_VER ## d );  \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ##.dll),   "free",  (FUNCPTR)safer_scalable_free_ ## CRT_VER,       0, NULL );                                   \
    orig_free_ ## CRT_VER = NULL;                                                                                                                               \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ## d.dll), "_msize",(FUNCPTR)safer_scalable_msize_ ## CRT_VER ## d, 9, (FUNCPTR*)&orig_msize_ ## CRT_VER ## d ); \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ##.dll),   "_msize",(FUNCPTR)safer_scalable_msize_ ## CRT_VER,      7, (FUNCPTR*)&orig_msize_ ## CRT_VER );      \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ## d.dll), "realloc",         (FUNCPTR)safer_scalable_realloc_ ## CRT_VER ## d,         0, NULL); \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ##.dll),   "realloc",         (FUNCPTR)safer_scalable_realloc_ ## CRT_VER,              0, NULL); \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ## d.dll), "_aligned_free",   (FUNCPTR)safer_scalable_free_ ## CRT_VER ## d,            0, NULL); \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ##.dll),   "_aligned_free",   (FUNCPTR)safer_scalable_free_ ## CRT_VER,                 0, NULL); \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ## d.dll), "_aligned_realloc",(FUNCPTR)safer_scalable_aligned_realloc_ ## CRT_VER ## d, 0, NULL); \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ##.dll),   "_aligned_realloc",(FUNCPTR)safer_scalable_aligned_realloc_ ## CRT_VER,      0, NULL);
#else
#define __TBB_ORIG_ALLOCATOR_REPLACEMENT_CALL(CRT_VER)\
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ## d.dll), "free",  (FUNCPTR)safer_scalable_free_ ## CRT_VER ## d,  5, (FUNCPTR*)&orig_free_ ## CRT_VER ## d );  \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ##.dll),   "free",  (FUNCPTR)safer_scalable_free_ ## CRT_VER,       7, (FUNCPTR*)&orig_free_ ## CRT_VER );       \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ## d.dll), "_msize",(FUNCPTR)safer_scalable_msize_ ## CRT_VER ## d, 5, (FUNCPTR*)&orig_msize_ ## CRT_VER ## d ); \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ##.dll),   "_msize",(FUNCPTR)safer_scalable_msize_ ## CRT_VER,      7, (FUNCPTR*)&orig_msize_ ## CRT_VER );      \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ## d.dll), "realloc",         (FUNCPTR)safer_scalable_realloc_ ## CRT_VER ## d,         0, NULL); \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ##.dll),   "realloc",         (FUNCPTR)safer_scalable_realloc_ ## CRT_VER,              0, NULL); \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ## d.dll), "_aligned_free",   (FUNCPTR)safer_scalable_free_ ## CRT_VER ## d,            0, NULL); \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ##.dll),   "_aligned_free",   (FUNCPTR)safer_scalable_free_ ## CRT_VER,                 0, NULL); \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ## d.dll), "_aligned_realloc",(FUNCPTR)safer_scalable_aligned_realloc_ ## CRT_VER ## d, 0, NULL); \
    ReplaceFunctionWithStore( __TBB_QV(CRT_VER ##.dll),   "_aligned_realloc",(FUNCPTR)safer_scalable_aligned_realloc_ ## CRT_VER,      0, NULL);
#endif

__TBB_ORIG_ALLOCATOR_REPLACEMENT_WRAPPER(msvcr70d);
__TBB_ORIG_ALLOCATOR_REPLACEMENT_WRAPPER(msvcr70);
__TBB_ORIG_ALLOCATOR_REPLACEMENT_WRAPPER(msvcr71d);
__TBB_ORIG_ALLOCATOR_REPLACEMENT_WRAPPER(msvcr71);
__TBB_ORIG_ALLOCATOR_REPLACEMENT_WRAPPER(msvcr80d);
__TBB_ORIG_ALLOCATOR_REPLACEMENT_WRAPPER(msvcr80);
__TBB_ORIG_ALLOCATOR_REPLACEMENT_WRAPPER(msvcr90d);
__TBB_ORIG_ALLOCATOR_REPLACEMENT_WRAPPER(msvcr90);


/*** replacements for global operators new and delete ***/

#include <new>

#if _MSC_VER && !defined(__INTEL_COMPILER)
#pragma warning( push )
#pragma warning( disable : 4290 )
#endif

void * operator_new(size_t sz) throw (std::bad_alloc) {
    void *res = scalable_malloc(sz);
    if (NULL == res) throw std::bad_alloc();
    return res;
}
void* operator_new_arr(size_t sz) throw (std::bad_alloc) {
    void *res = scalable_malloc(sz);
    if (NULL == res) throw std::bad_alloc();
    return res;
}
void operator_delete(void* ptr) throw() {
    safer_scalable_free2(ptr);
}
#if _MSC_VER && !defined(__INTEL_COMPILER)
#pragma warning( pop )
#endif

void operator_delete_arr(void* ptr) throw() {
    safer_scalable_free2(ptr);
}
void* operator_new_t(size_t sz, const std::nothrow_t&) throw() {
    return scalable_malloc(sz);
}
void* operator_new_arr_t(std::size_t sz, const std::nothrow_t&) throw() {
    return scalable_malloc(sz);
}
void operator_delete_t(void* ptr, const std::nothrow_t&) throw() {
    safer_scalable_free2(ptr);
}
void operator_delete_arr_t(void* ptr, const std::nothrow_t&) throw() {
    safer_scalable_free2(ptr);
}

const char* modules_to_replace[] = {
    "msvcr80d.dll",
    "msvcr80.dll",
    "msvcr90d.dll",
    "msvcr90.dll",
    "msvcr70d.dll",
    "msvcr70.dll",
    "msvcr71d.dll",
    "msvcr71.dll",
    };

/*
We need to replace following functions:
malloc
calloc
_aligned_malloc
_expand (by dummy implementation)
??2@YAPAXI@Z      operator new                         (ia32)
??_U@YAPAXI@Z     void * operator new[] (size_t size)  (ia32)
??3@YAXPAX@Z      operator delete                      (ia32)  
??_V@YAXPAX@Z     operator delete[]                    (ia32)
??2@YAPEAX_K@Z    void * operator new(unsigned __int64)   (intel64)
??_V@YAXPEAX@Z    void * operator new[](unsigned __int64) (intel64)
??3@YAXPEAX@Z     operator delete                         (intel64)  
??_V@YAXPEAX@Z    operator delete[]                       (intel64)
??2@YAPAXIABUnothrow_t@std@@@Z      void * operator new (size_t sz, const std::nothrow_t&) throw()  (optional)
??_U@YAPAXIABUnothrow_t@std@@@Z     void * operator new[] (size_t sz, const std::nothrow_t&) throw() (optional)

and these functions have runtime-specific replacement:
realloc
free
_msize
_aligned_realloc
_aligned_free
*/

typedef struct FRData_t {
    //char *_module;
    const char *_func;
    FUNCPTR _fptr;
    FRR_ON_ERROR _on_error;
} FRDATA;

FRDATA routines_to_replace[] = {
    { "malloc",  (FUNCPTR)scalable_malloc, FRR_FAIL },
    { "calloc",  (FUNCPTR)scalable_calloc, FRR_FAIL },
    { "_aligned_malloc",  (FUNCPTR)scalable_aligned_malloc, FRR_FAIL },
    { "_expand",  (FUNCPTR)safer_expand, FRR_IGNORE },
#if _WIN64
    { "??2@YAPEAX_K@Z", (FUNCPTR)operator_new, FRR_FAIL },
    { "??_U@YAPEAX_K@Z", (FUNCPTR)operator_new_arr, FRR_FAIL },
    { "??3@YAXPEAX@Z", (FUNCPTR)operator_delete, FRR_FAIL },
    { "??_V@YAXPEAX@Z", (FUNCPTR)operator_delete_arr, FRR_FAIL },
#else 
    { "??2@YAPAXI@Z", (FUNCPTR)operator_new, FRR_FAIL },
    { "??_U@YAPAXI@Z", (FUNCPTR)operator_new_arr, FRR_FAIL },
    { "??3@YAXPAX@Z", (FUNCPTR)operator_delete, FRR_FAIL },
    { "??_V@YAXPAX@Z", (FUNCPTR)operator_delete_arr, FRR_FAIL },
#endif
    { "??2@YAPAXIABUnothrow_t@std@@@Z", (FUNCPTR)operator_new_t, FRR_IGNORE },
    { "??_U@YAPAXIABUnothrow_t@std@@@Z", (FUNCPTR)operator_new_arr_t, FRR_IGNORE }
};

#ifndef UNICODE
void ReplaceFunctionWithStore( const char*dllName, const char *funcName, FUNCPTR newFunc, UINT opcodesNumber, FUNCPTR* origFunc )
#else
void ReplaceFunctionWithStore( const wchar_t *dllName, const char *funcName, FUNCPTR newFunc, UINT opcodesNumber, FUNCPTR* origFunc )
#endif
{
    FRR_TYPE type = ReplaceFunction( dllName, funcName, newFunc, opcodesNumber, origFunc );
    if (type == FRR_NODLL) return;
    if ( type != FRR_OK )
    {
        fprintf(stderr, "Failed to replace function %s in module %s\n",
                funcName, dllName);
        exit(1);
    }
}

void doMallocReplacement()
{
    int i,j;

    // Replace functions without storing original code
    int modules_to_replace_count = sizeof(modules_to_replace) / sizeof(modules_to_replace[0]);
    int routines_to_replace_count = sizeof(routines_to_replace) / sizeof(routines_to_replace[0]);
    for ( j=0; j<modules_to_replace_count; j++ )
        for (i = 0; i < routines_to_replace_count; i++)
        {
            FRR_TYPE type = ReplaceFunction( modules_to_replace[j], routines_to_replace[i]._func, routines_to_replace[i]._fptr, NULL, NULL );
            if (type == FRR_NODLL) break;
            if (type != FRR_OK && routines_to_replace[i]._on_error==FRR_FAIL)
            {
                fprintf(stderr, "Failed to replace function %s in module %s\n",
                        routines_to_replace[i]._func, modules_to_replace[j]);
                exit(1);
            }
        }

    // Replace functions and keep backup of original code (separate for each runtime)
    __TBB_ORIG_ALLOCATOR_REPLACEMENT_CALL(msvcr70)
    __TBB_ORIG_ALLOCATOR_REPLACEMENT_CALL(msvcr71)
    __TBB_ORIG_ALLOCATOR_REPLACEMENT_CALL(msvcr80)
    __TBB_ORIG_ALLOCATOR_REPLACEMENT_CALL(msvcr90)
}

extern "C" BOOL WINAPI DllMain( HINSTANCE hInst, DWORD callReason, LPVOID reserved )
{

    if ( callReason==DLL_PROCESS_ATTACH && reserved && hInst ) {
#if TBBMALLOC_USE_TBB_FOR_ALLOCATOR_ENV_CONTROLLED
        char pinEnvVariable[50];
        if( GetEnvironmentVariable("TBBMALLOC_USE_TBB_FOR_ALLOCATOR", pinEnvVariable, 50))
        {
            doMallocReplacement();
        }
#else
        doMallocReplacement();
#endif
    }

    return TRUE;
}

// Just to make the linker happy and link the DLL to the application
extern "C" __declspec(dllexport) void __TBB_malloc_proxy()
{

}

#endif //_WIN32
