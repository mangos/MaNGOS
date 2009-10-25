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

#include "itt_notify.h"
#include "tbb/tbb_machine.h"

#include <stdio.h>

namespace tbb {
    namespace internal {

#if __TBB_NEW_ITT_NOTIFY
#if DO_ITT_NOTIFY

    extern "C" int __TBB_load_ittnotify();

    bool InitializeITT () {
        return __TBB_load_ittnotify() != 0;
    }


#endif /* DO_ITT_NOTIFY */
#endif /* __TBB_NEW_ITT_NOTIFY */

    void itt_store_pointer_with_release_v3( void* dst, void* src ) {
        ITT_NOTIFY(sync_releasing, dst);
        __TBB_store_with_release(*static_cast<void**>(dst),src);
    }

    void* itt_load_pointer_with_acquire_v3( const void* src ) {
        void* result = __TBB_load_with_acquire(*static_cast<void*const*>(src));
        ITT_NOTIFY(sync_acquired, const_cast<void*>(src));
        return result;
    }

    void* itt_load_pointer_v3( const void* src ) {
        void* result = *static_cast<void*const*>(src);
        return result;
    }

    void itt_set_sync_name_v3( void* obj, const tchar* name) {
        ITT_SYNC_RENAME(obj, name);
        (void)obj, (void)name;  // Prevents compiler warning when ITT support is switched off
    }

    } // namespace internal
} // namespace tbb


#if !__TBB_NEW_ITT_NOTIFY

#include "tbb_misc.h"
#include "dynamic_link.h"
#include "tbb/cache_aligned_allocator.h" /* NFS_MaxLineSize */

#if _WIN32||_WIN64
    #include <windows.h>
#else /* !WIN */
    #include <dlfcn.h>
#if __TBB_WEAK_SYMBOLS
    #pragma weak __itt_notify_sync_prepare
    #pragma weak __itt_notify_sync_acquired
    #pragma weak __itt_notify_sync_releasing
    #pragma weak __itt_notify_sync_cancel
    #pragma weak __itt_thr_name_set
    #pragma weak __itt_thread_set_name
    #pragma weak __itt_sync_create
    #pragma weak __itt_sync_rename
    extern "C" {
        void __itt_notify_sync_prepare(void *p);
        void __itt_notify_sync_cancel(void *p);
        void __itt_notify_sync_acquired(void *p);
        void __itt_notify_sync_releasing(void *p);
        int __itt_thr_name_set (void* p, int len);
        void __itt_thread_set_name (const char* name);
        void __itt_sync_create( void* obj, const char* name, const char* type, int attribute );
        void __itt_sync_rename( void* obj, const char* new_name );
    }
#endif /* __TBB_WEAK_SYMBOLS */
#endif /* !WIN */

namespace tbb {
namespace internal {

#if DO_ITT_NOTIFY


//! Table describing the __itt_notify handlers.
static const dynamic_link_descriptor ITT_HandlerTable[] = {
    DLD( __itt_notify_sync_prepare, ITT_Handler_sync_prepare),
    DLD( __itt_notify_sync_acquired, ITT_Handler_sync_acquired),
    DLD( __itt_notify_sync_releasing, ITT_Handler_sync_releasing),
    DLD( __itt_notify_sync_cancel, ITT_Handler_sync_cancel),
# if _WIN32||_WIN64
    DLD( __itt_thr_name_setW, ITT_Handler_thr_name_set),
    DLD( __itt_thread_set_nameW, ITT_Handler_thread_set_name),
# else
    DLD( __itt_thr_name_set, ITT_Handler_thr_name_set),
    DLD( __itt_thread_set_name, ITT_Handler_thread_set_name),
# endif /* _WIN32 || _WIN64 */


# if _WIN32||_WIN64
    DLD( __itt_sync_createW, ITT_Handler_sync_create),
    DLD( __itt_sync_renameW, ITT_Handler_sync_rename)
# else
    DLD( __itt_sync_create, ITT_Handler_sync_create),
    DLD( __itt_sync_rename, ITT_Handler_sync_rename)
# endif
};

static const int ITT_HandlerTable_size = 
    sizeof(ITT_HandlerTable)/sizeof(dynamic_link_descriptor);

// LIBITTNOTIFY_NAME is the name of the ITT notification library 
# if _WIN32||_WIN64
#  define LIBITTNOTIFY_NAME "libittnotify.dll"
# elif __linux__
#  define LIBITTNOTIFY_NAME "libittnotify.so"
# else
#  error Intel(R) Threading Tools not provided for this OS
# endif

//! Performs tools support initialization.
/** Is called by DoOneTimeInitializations and ITT_DoOneTimeInitialization in 
    a protected (one-time) manner. Not to be invoked directly. **/
bool InitializeITT() {
    bool result = false;
    // Check if we are running under a performance or correctness tool
    bool t_checker = GetBoolEnvironmentVariable("KMP_FOR_TCHECK");
    bool t_profiler = GetBoolEnvironmentVariable("KMP_FOR_TPROFILE");
	__TBB_ASSERT(!(t_checker&&t_profiler), NULL);
    if ( t_checker || t_profiler ) {
        // Yes, we are in the tool mode. Try to load libittnotify library.
        result = dynamic_link( LIBITTNOTIFY_NAME, ITT_HandlerTable, ITT_HandlerTable_size, 4 );
    }
    if (result){
        if ( t_checker ) {
            current_tool = ITC;
        } else if ( t_profiler ) {
            current_tool = ITP;
        }
    } else {
        // Clear away the proxy (dummy) handlers
        for (int i = 0; i < ITT_HandlerTable_size; i++)
            *ITT_HandlerTable[i].handler = NULL;
        current_tool = NONE;
    }
    PrintExtraVersionInfo( "ITT", result?"yes":"no" );
    return result;
}

//! Performs one-time initialization of tools interoperability mechanisms.
/** Defined in task.cpp. Makes a protected do-once call to InitializeITT(). **/
void ITT_DoOneTimeInitialization();

/** The following dummy_xxx functions are proxies that correspond to tool notification 
    APIs and are used to initialize corresponding pointers to the tool notifications
    (ITT_Handler_xxx). When the first call to ITT_Handler_xxx takes place before 
    the whole library initialization (done by DoOneTimeInitializations) happened,
    the proxy handler performs initialization of the tools support. After this
    ITT_Handler_xxx will be set to either tool notification pointer or NULL. **/
void dummy_sync_prepare( volatile void* ptr ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_sync_prepare!=&dummy_sync_prepare, NULL );
    if (ITT_Handler_sync_prepare)
        (*ITT_Handler_sync_prepare) (ptr);
}

void dummy_sync_acquired( volatile void* ptr ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_sync_acquired!=&dummy_sync_acquired, NULL );
    if (ITT_Handler_sync_acquired)
        (*ITT_Handler_sync_acquired) (ptr);
}

void dummy_sync_releasing( volatile void* ptr ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_sync_releasing!=&dummy_sync_releasing, NULL );
    if (ITT_Handler_sync_releasing)
        (*ITT_Handler_sync_releasing) (ptr);
}

void dummy_sync_cancel( volatile void* ptr ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_sync_cancel!=&dummy_sync_cancel, NULL );
    if (ITT_Handler_sync_cancel)
        (*ITT_Handler_sync_cancel) (ptr);
}

int dummy_thr_name_set( const tchar* str, int number ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_thr_name_set!=&dummy_thr_name_set, NULL );
    if (ITT_Handler_thr_name_set)
        return (*ITT_Handler_thr_name_set) (str, number);
    return -1;
}

void dummy_thread_set_name( const tchar* name ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_thread_set_name!=&dummy_thread_set_name, NULL );
    if (ITT_Handler_thread_set_name)
        (*ITT_Handler_thread_set_name)( name );
}

void dummy_sync_create( void* obj, const tchar* objname, const tchar* objtype, int /*attribute*/ ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_sync_create!=&dummy_sync_create, NULL );
    ITT_SYNC_CREATE( obj, objtype, objname );
}

void dummy_sync_rename( void* obj, const tchar* new_name ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_sync_rename!=&dummy_sync_rename, NULL );
    ITT_SYNC_RENAME(obj, new_name);
}



//! Leading padding before the area where tool notification handlers are placed.
/** Prevents cache lines where the handler pointers are stored from thrashing.
    Defined as extern to prevent compiler from placing the padding arrays separately
    from the handler pointers (which are declared as extern).
    Declared separately from definition to get rid of compiler warnings. **/
extern char __ITT_Handler_leading_padding[NFS_MaxLineSize];

//! Trailing padding after the area where tool notification handlers are placed.
extern char __ITT_Handler_trailing_padding[NFS_MaxLineSize];

char __ITT_Handler_leading_padding[NFS_MaxLineSize] = {0};
PointerToITT_Handler ITT_Handler_sync_prepare = &dummy_sync_prepare;
PointerToITT_Handler ITT_Handler_sync_acquired = &dummy_sync_acquired;
PointerToITT_Handler ITT_Handler_sync_releasing = &dummy_sync_releasing;
PointerToITT_Handler ITT_Handler_sync_cancel = &dummy_sync_cancel;
PointerToITT_thr_name_set ITT_Handler_thr_name_set = &dummy_thr_name_set;
PointerToITT_thread_set_name ITT_Handler_thread_set_name = &dummy_thread_set_name;
PointerToITT_sync_create ITT_Handler_sync_create = &dummy_sync_create;
PointerToITT_sync_rename ITT_Handler_sync_rename = &dummy_sync_rename;
char __ITT_Handler_trailing_padding[NFS_MaxLineSize] = {0};

target_tool current_tool = TO_BE_INITIALIZED;

#endif /* DO_ITT_NOTIFY */
} // namespace internal 

} // namespace tbb

#endif /* !__TBB_NEW_ITT_NOTIFY */
