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

// All platform-specific threading support is encapsulated here. */
 
#ifndef __RML_thread_monitor_H
#define __RML_thread_monitor_H

#if USE_WINTHREAD
#include <windows.h>
#include <process.h>
#include <malloc.h> //_alloca
#elif USE_PTHREAD
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#else
#error Unsupported platform
#endif 
#include <stdio.h>

// All platform-specific threading support is in this header.

#if (_WIN32||_WIN64)&&!__TBB_ipf
// Deal with 64K aliasing.  The formula for "offset" is a Fibonacci hash function,
// which has the desirable feature of spreading out the offsets fairly evenly
// without knowing the total number of offsets, and furthermore unlikely to
// accidentally cancel out other 64K aliasing schemes that Microsoft might implement later.
// See Knuth Vol 3. "Theorem S" for details on Fibonacci hashing.
// The second statement is really does need "volatile", otherwise the compiler might remove the _alloca.
#define AVOID_64K_ALIASING(idx)                       \
    size_t offset = (idx+1) * 40503U % (1U<<16);      \
    void* volatile sink_for_alloca = _alloca(offset); \
    __TBB_ASSERT_EX(sink_for_alloca, "_alloca failed");
#else
// Linux thread allocators avoid 64K aliasing.
#define AVOID_64K_ALIASING(idx)
#endif /* _WIN32||_WIN64 */

namespace rml {

namespace internal {

//! Monitor with limited two-phase commit form of wait.  
/** At most one thread should wait on an instance at a time. */
class thread_monitor {
public:
    class cookie {
        friend class thread_monitor;
        unsigned long long my_version;
    };
    thread_monitor();
    ~thread_monitor();

    //! If a thread is waiting or started a two-phase wait, notify it.
    /** Can be called by any thread. */
    void notify();

    //! Begin two-phase wait.
    /** Should only be called by thread that owns the monitor. 
        The caller must either complete the wait or cancel it. */
    void prepare_wait( cookie& c );

    //! Complete a two-phase wait and wait until notification occurs after the earlier prepare_wait.
    void commit_wait( cookie& c );

    //! Cancel a two-phase wait.
    void cancel_wait();

#if USE_WINTHREAD
#define __RML_DECL_THREAD_ROUTINE unsigned WINAPI
    typedef unsigned (WINAPI *thread_routine_type)(void*);
#endif /* USE_WINTHREAD */

#if USE_PTHREAD
#define __RML_DECL_THREAD_ROUTINE void*
    typedef void*(*thread_routine_type)(void*);
#endif /* USE_PTHREAD */

    //! Launch a thread
    static void launch( thread_routine_type thread_routine, void* arg, size_t stack_size );
    static void yield();

private:
    cookie my_cookie;
#if USE_WINTHREAD
    CRITICAL_SECTION critical_section;
    HANDLE event;
#endif /* USE_WINTHREAD */
#if USE_PTHREAD
    pthread_mutex_t my_mutex;
    pthread_cond_t my_cond;
    static void check( int error_code, const char* routine );
#endif /* USE_PTHREAD */
};


#if USE_WINTHREAD
#ifndef STACK_SIZE_PARAM_IS_A_RESERVATION
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000
#endif
inline void thread_monitor::launch( thread_routine_type thread_routine, void* arg, size_t stack_size ) {
    unsigned thread_id;
    uintptr_t status = _beginthreadex( NULL, unsigned(stack_size), thread_routine, arg, STACK_SIZE_PARAM_IS_A_RESERVATION, &thread_id );
    if( status==0 ) {
        fprintf(stderr,"thread_monitor::launch: _beginthreadex failed\n");
        exit(1); 
    } else {
        CloseHandle((HANDLE)status);
    }
}

inline void thread_monitor::yield() {
    SwitchToThread();
}

inline thread_monitor::thread_monitor() {
    event = CreateEvent( NULL, /*manualReset=*/true, /*initialState=*/false, NULL );
    InitializeCriticalSection( &critical_section );
    my_cookie.my_version = 0;
}

inline thread_monitor::~thread_monitor() {
    CloseHandle( event );
    DeleteCriticalSection( &critical_section );
}
     
inline void thread_monitor::notify() {
    EnterCriticalSection( &critical_section );
    ++my_cookie.my_version;
    SetEvent( event );
    LeaveCriticalSection( &critical_section );
}

inline void thread_monitor::prepare_wait( cookie& c ) {
    EnterCriticalSection( &critical_section );
    c = my_cookie;
}

inline void thread_monitor::commit_wait( cookie& c ) {
    ResetEvent( event );
    LeaveCriticalSection( &critical_section );
    while( my_cookie.my_version==c.my_version ) {
        WaitForSingleObject( event, INFINITE );
        ResetEvent( event );
    }
}

inline void thread_monitor::cancel_wait() {
    LeaveCriticalSection( &critical_section );
}
#endif /* USE_WINTHREAD */

#if USE_PTHREAD
inline void thread_monitor::check( int error_code, const char* routine ) {
    if( error_code ) {
        fprintf(stderr,"thread_monitor %s\n", strerror(error_code) );
        exit(1);
    }
}

inline void thread_monitor::launch( void* (*thread_routine)(void*), void* arg, size_t stack_size ) {
    // FIXME - consider more graceful recovery than just exiting if a thread cannot be launched.
    // Note that there are some tricky situations to deal with, such that the thread is already 
    // grabbed as part of an OpenMP team, or is being launched as a replacement for a thread with
    // too small a stack.
    pthread_attr_t s;
    check(pthread_attr_init( &s ), "pthread_attr_init");
    if( stack_size>0 ) {
        check(pthread_attr_setstacksize( &s, stack_size ),"pthread_attr_setstack_size");
    }
    pthread_t handle;
    check( pthread_create( &handle, &s, thread_routine, arg ), "pthread_create" );
    check( pthread_detach( handle ), "pthread_detach" );
}

inline void thread_monitor::yield() {
    sched_yield();
}

inline thread_monitor::thread_monitor() {
    check( pthread_mutex_init(&my_mutex,NULL), "pthread_mutex_init" );
    check( pthread_cond_init(&my_cond,NULL), "pthread_cond_init" );
    my_cookie.my_version = 0;
}

inline thread_monitor::~thread_monitor() {
    pthread_cond_destroy(&my_cond);
    pthread_mutex_destroy(&my_mutex);
}

inline void thread_monitor::notify() {
    check( pthread_mutex_lock( &my_mutex ), "pthread_mutex_lock" );
    ++my_cookie.my_version;
    check( pthread_mutex_unlock( &my_mutex ), "pthread_mutex_unlock" );
    check( pthread_cond_signal(&my_cond), "pthread_cond_signal" );
}

inline void thread_monitor::prepare_wait( cookie& c ) {
    check( pthread_mutex_lock( &my_mutex ), "pthread_mutex_lock" );
    c = my_cookie;
}

inline void thread_monitor::commit_wait( cookie& c ) {
    while( my_cookie.my_version==c.my_version ) {
        pthread_cond_wait( &my_cond, &my_mutex );
    }
    check( pthread_mutex_unlock( &my_mutex ), "pthread_mutex_unlock" );
}

inline void thread_monitor::cancel_wait() {
    check( pthread_mutex_unlock( &my_mutex ), "pthread_mutex_unlock" );
}
#endif /* USE_PTHREAD */

} // namespace internal
} // namespace rml

#endif /* __RML_thread_monitor_H */
