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

#if _WIN32||_WIN64
#include <process.h>        /* Need _beginthreadex from there */
#include <stdexcept>        /* Need std::runtime_error from there */
#include <string>           /* Need std::string from there */
#endif // _WIN32||_WIN64
#include "tbb_misc.h" // for handle_perror
#include "tbb/tbb_stddef.h"
#include "tbb/tbb_thread.h"
#include "tbb/tbb_allocator.h"
#include "tbb/task_scheduler_init.h" /* Need task_scheduler_init::default_num_threads() */

namespace tbb {

namespace internal {

//! Allocate a closure
void* allocate_closure_v3( size_t size )
{
    return allocate_via_handler_v3( size );
}

//! Free a closure allocated by allocate_closure_v3
void free_closure_v3( void *ptr )
{
    deallocate_via_handler_v3( ptr );
}

#if _WIN32||_WIN64 
#if defined(__EXCEPTIONS) || defined(_CPPUNWIND)
// The above preprocessor symbols are defined by compilers when exception handling is enabled.

void handle_win_error( int error_code ) 
{
    LPTSTR msg_buf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_code,
        0,
        (LPTSTR) &msg_buf,
        0, NULL );
    const std::string msg_str(msg_buf);
    LocalFree(msg_buf);
    throw std::runtime_error(msg_str);
}
#endif //__EXCEPTIONS || _CPPUNWIND
#endif // _WIN32||_WIN64

void tbb_thread_v3::join()
{
    __TBB_ASSERT( joinable(), "thread should be joinable when join called" );
#if _WIN32||_WIN64 
    DWORD status = WaitForSingleObject( my_handle, INFINITE );
    if ( status == WAIT_FAILED )
        handle_win_error( GetLastError() );
    BOOL close_stat = CloseHandle( my_handle );
    if ( close_stat == 0 )
        handle_win_error( GetLastError() );
    my_thread_id = 0;
#else
    int status = pthread_join( my_handle, NULL );
    if( status )
        handle_perror( status, "pthread_join" );
#endif // _WIN32||_WIN64 
    my_handle = 0;
}

void tbb_thread_v3::detach() {
    __TBB_ASSERT( joinable(), "only joinable thread can be detached" );
#if _WIN32||_WIN64
    BOOL status = CloseHandle( my_handle );
    if ( status == 0 )
      handle_win_error( GetLastError() );
    my_thread_id = 0;
#else
    int status = pthread_detach( my_handle );
    if( status )
        handle_perror( status, "pthread_detach" );
#endif // _WIN32||_WIN64
    my_handle = 0;
}

const size_t MB = 1<<20;
#if !defined(__TBB_WORDSIZE)
const size_t ThreadStackSize = 1*MB;
#elif __TBB_WORDSIZE<=4
const size_t ThreadStackSize = 2*MB;
#else
const size_t ThreadStackSize = 4*MB;
#endif

void tbb_thread_v3::internal_start( __TBB_NATIVE_THREAD_ROUTINE_PTR(start_routine),
                                    void* closure ) {
#if _WIN32||_WIN64
    unsigned thread_id;
    // The return type of _beginthreadex is "uintptr_t" on new MS compilers,
    // and 'unsigned long' on old MS compilers.  Our uintptr works for both.
    uintptr status = _beginthreadex( NULL, ThreadStackSize, start_routine,
                                     closure, 0, &thread_id ); 
    if( status==0 )
        handle_perror(errno,"__beginthreadex");
    else {
        my_handle = (HANDLE)status;
        my_thread_id = thread_id;
    }
#else
    pthread_t thread_handle;
    int status;
    pthread_attr_t stack_size;
    status = pthread_attr_init( &stack_size );
    if( status )
        handle_perror( status, "pthread_attr_init" );
    status = pthread_attr_setstacksize( &stack_size, ThreadStackSize );
    if( status )
        handle_perror( status, "pthread_attr_setstacksize" );

    status = pthread_create( &thread_handle, &stack_size, start_routine, closure );
    if( status )
        handle_perror( status, "pthread_create" );

    my_handle = thread_handle;
#endif // _WIN32||_WIN64
}

unsigned tbb_thread_v3::hardware_concurrency() {
    return task_scheduler_init::default_num_threads();
}

tbb_thread_v3::id thread_get_id_v3() {
#if _WIN32||_WIN64
    return tbb_thread_v3::id( GetCurrentThreadId() );
#else
    return tbb_thread_v3::id( pthread_self() );
#endif // _WIN32||_WIN64
}
    
void move_v3( tbb_thread_v3& t1, tbb_thread_v3& t2 )
{
    if (t1.joinable())
        t1.detach();
    t1.my_handle = t2.my_handle;
    t2.my_handle = 0;
#if _WIN32||_WIN64
    t1.my_thread_id = t2.my_thread_id;
    t2.my_thread_id = 0;
#endif // _WIN32||_WIN64
}

void thread_yield_v3()
{
    __TBB_Yield();
}

void thread_sleep_v3(const tick_count::interval_t &i)
{
#if _WIN32||_WIN64
     tick_count t0 = tick_count::now();
     tick_count t1 = t0;
     for(;;) {
         double remainder = (i-(t1-t0)).seconds()*1e3;  // milliseconds remaining to sleep
         if( remainder<=0 ) break;
         DWORD t = remainder>=INFINITE ? INFINITE-1 : DWORD(remainder);
         Sleep( t );
         t1 = tick_count::now();
    }
#else
    struct timespec req;
    double sec = i.seconds();

    req.tv_sec = static_cast<long>(sec);
    req.tv_nsec = static_cast<long>( (sec - req.tv_sec)*1e9 );
    nanosleep(&req, NULL);
#endif // _WIN32||_WIN64
}

} // internal

} // tbb
