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

#include "dynamic_link.h"

#ifndef LIBRARY_ASSERT
#include "tbb/tbb_stddef.h"
#define LIBRARY_ASSERT(x,y) __TBB_ASSERT(x,y)
#endif /* LIBRARY_ASSERT */

#if _WIN32||_WIN64
    #include <malloc.h>     /* alloca */
#else
    #include <dlfcn.h>
#if __FreeBSD__
    #include <stdlib.h>     /* alloca */
#else
    #include <alloca.h>
#endif
#endif

OPEN_INTERNAL_NAMESPACE

#if __TBB_WEAK_SYMBOLS

bool dynamic_link( void*, const dynamic_link_descriptor descriptors[], size_t n, size_t required )
{
    if ( required == ~(size_t)0 )
        required = n;
    LIBRARY_ASSERT( required<=n, "Number of required entry points exceeds their total number" );
    size_t k = 0;
    // Check if the first required entries are present in what was loaded into our process
    while ( k < required && descriptors[k].ptr )
        ++k;
    if ( k < required )
        return false;
    // Commit all the entry points.
    for ( k = 0; k < n; ++k )
        *descriptors[k].handler = (pointer_to_handler) descriptors[k].ptr;
    return true;
}

#else /* !__TBB_WEAK_SYMBOLS */

bool dynamic_link( void* module, const dynamic_link_descriptor descriptors[], size_t n, size_t required )
{
    pointer_to_handler *h = (pointer_to_handler*)alloca(n * sizeof(pointer_to_handler));
    if ( required == ~(size_t)0 )
        required = n;
    LIBRARY_ASSERT( required<=n, "Number of required entry points exceeds their total number" );
    size_t k = 0;
    for ( ; k < n; ++k ) {
#if _WIN32||_WIN64
        h[k] = pointer_to_handler(GetProcAddress( (HMODULE)module, descriptors[k].name ));
#else
        // Lvalue casting is used; this way icc -strict-ansi does not warn about nonstandard pointer conversion
        (void *&)h[k] = dlsym( module, descriptors[k].name );
#endif /* _WIN32||_WIN64 */
        if ( !h[k] && k < required )
            return false;
    }
    LIBRARY_ASSERT( k == n, "if required entries are initialized, all entries are expected to be walked");
    // Commit the entry points.
    // Cannot use memset here, because the writes must be atomic.
    for( k = 0; k < n; ++k )
        *descriptors[k].handler = h[k];
    return true;
}

#endif /* !__TBB_WEAK_SYMBOLS */
bool dynamic_link( const char* library, const dynamic_link_descriptor descriptors[], size_t n, size_t required, dynamic_link_handle* handle )
{
#if _WIN32||_WIN64
    // Interpret non-NULL handle parameter as request to really link against another library.
    if ( !handle && dynamic_link( GetModuleHandle(NULL), descriptors, n, required ) )
        // Target library was statically linked into this executable
        return true;
    // Prevent Windows from displaying silly message boxes if it fails to load library
    // (e.g. because of MS runtime problems - one of those crazy manifest related ones)
    UINT prev_mode = SetErrorMode (SEM_FAILCRITICALERRORS);
    dynamic_link_handle module = LoadLibrary (library);
    SetErrorMode (prev_mode);
#else
    dynamic_link_handle module = dlopen( library, RTLD_LAZY ); 
#endif /* _WIN32||_WIN64 */
    if( module ) {
        if( !dynamic_link( module, descriptors, n, required ) ) {
            // Return true if the library is there and it contains all the expected entry points.
            dynamic_unlink(module);
            module = NULL;
        }
    }
    if( handle ) 
        *handle = module;
    return module!=NULL;
}

void dynamic_unlink( dynamic_link_handle handle ) {
    if( handle ) {
#if _WIN32||_WIN64
        FreeLibrary( handle );
#else
        dlclose( handle );
#endif /* _WIN32||_WIN64 */    
    }
}

CLOSE_INTERNAL_NAMESPACE
