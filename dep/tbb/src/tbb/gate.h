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

#ifndef _TBB_Gate_H
#define _TBB_Gate_H

#include "itt_notify.h"

namespace tbb {

namespace internal {

#if __TBB_RML
//! Fake version of Gate for use with RML.
/** Really just an atomic intptr_t with a compare-and-swap operation,
    but wrapped in syntax that makes it look like a normal Gate object,
    in order to minimize source changes for RML in task.cpp. */
class Gate {
public:
    typedef intptr_t state_t;
   
    //! Get current state of gate
    state_t get_state() const {
        return state;
    }

#if defined(_MSC_VER) && defined(_Wp64)
    // Workaround for overzealous compiler warnings in /Wp64 mode
    #pragma warning (disable: 4244)
#endif

    bool try_update( intptr_t value, intptr_t comparand ) {
        return state.compare_and_swap(value,comparand)==comparand;
    }
private:
    atomic<state_t> state;
};

#elif __TBB_USE_FUTEX

//! Implementation of Gate based on futex.
/** Use this futex-based implementation where possible, because it is the simplest and usually fastest. */
class Gate {
public:
    typedef intptr_t state_t;

    Gate() {
        ITT_SYNC_CREATE(&state, SyncType_Scheduler, SyncObj_Gate);
    }

    //! Get current state of gate
    state_t get_state() const {
        return state;
    }
    //! Update state=value if state==comparand (flip==false) or state!=comparand (flip==true)
    void try_update( intptr_t value, intptr_t comparand, bool flip=false ) {
        __TBB_ASSERT( comparand!=0 || value!=0, "either value or comparand must be non-zero" );
retry:
        state_t old_state = state;
        // First test for condition without using atomic operation
        if( flip ? old_state!=comparand : old_state==comparand ) {
            // Now atomically retest condition and set.
            state_t s = state.compare_and_swap( value, old_state );
            if( s==old_state ) {
                // compare_and_swap succeeded
                if( value!=0 )   
                    futex_wakeup_all( &state );  // Update was successful and new state is not SNAPSHOT_EMPTY
            } else {
                // compare_and_swap failed.  But for != case, failure may be spurious for our purposes if
                // the value there is nonetheless not equal to value.  This is a fairly rare event, so
                // there is no need for backoff.  In event of such a failure, we must retry.
                if( flip && s!=value ) 
                    goto retry;
            }
        }
    }
    //! Wait for state!=0.
    void wait() {
        if( state==0 )
            futex_wait( &state, 0 );
    }
private:
    atomic<state_t> state;
};

#elif USE_WINTHREAD

class Gate {
public:
    typedef intptr_t state_t;
private:
    //! If state==0, then thread executing wait() suspend until state becomes non-zero.
    state_t state;
    CRITICAL_SECTION critical_section;
    HANDLE event;
public:
    //! Initialize with count=0
    Gate() : state(0) {
        event = CreateEvent( NULL, true, false, NULL );
        InitializeCriticalSection( &critical_section );
        ITT_SYNC_CREATE(&event, SyncType_Scheduler, SyncObj_Gate);
        ITT_SYNC_CREATE(&critical_section, SyncType_Scheduler, SyncObj_GateLock);
    }
    ~Gate() {
        // Fake prepare/acquired pair for Intel(R) Parallel Amplifier to correctly attribute the operations below
        ITT_NOTIFY( sync_prepare, &event );
        CloseHandle( event );
        DeleteCriticalSection( &critical_section );
        ITT_NOTIFY( sync_acquired, &event );
    }
    //! Get current state of gate
    state_t get_state() const {
        return state;
    }
    //! Update state=value if state==comparand (flip==false) or state!=comparand (flip==true)
    void try_update( intptr_t value, intptr_t comparand, bool flip=false ) {
        __TBB_ASSERT( comparand!=0 || value!=0, "either value or comparand must be non-zero" );
        EnterCriticalSection( &critical_section );
        state_t old = state;
        if( flip ? old!=comparand : old==comparand ) {
            state = value;
            if( !old )
                SetEvent( event );
            else if( !value )
                ResetEvent( event );
        }
        LeaveCriticalSection( &critical_section );
    }
    //! Wait for state!=0.
    void wait() {
        if( state==0 ) {
            WaitForSingleObject( event, INFINITE );
        }
    }
};

#elif USE_PTHREAD

class Gate {
public:
    typedef intptr_t state_t;
private:
    //! If state==0, then thread executing wait() suspend until state becomes non-zero.
    state_t state;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
public:
    //! Initialize with count=0
    Gate() : state(0)
    {
        pthread_mutex_init( &mutex, NULL );
        pthread_cond_init( &cond, NULL);
        ITT_SYNC_CREATE(&cond, SyncType_Scheduler, SyncObj_Gate);
        ITT_SYNC_CREATE(&mutex, SyncType_Scheduler, SyncObj_GateLock);
    }
    ~Gate() {
        pthread_cond_destroy( &cond );
        pthread_mutex_destroy( &mutex );
    }
    //! Get current state of gate
    state_t get_state() const {
        return state;
    }
    //! Update state=value if state==comparand (flip==false) or state!=comparand (flip==true)
    void try_update( intptr_t value, intptr_t comparand, bool flip=false ) {
        __TBB_ASSERT( comparand!=0 || value!=0, "either value or comparand must be non-zero" );
        pthread_mutex_lock( &mutex );
        state_t old = state;
        if( flip ? old!=comparand : old==comparand ) {
            state = value;
            if( !old )
                pthread_cond_broadcast( &cond );
        }
        pthread_mutex_unlock( &mutex );
    }
    //! Wait for state!=0.
    void wait() {
        if( state==0 ) {
            pthread_mutex_lock( &mutex );
            while( state==0 ) {
                pthread_cond_wait( &cond, &mutex );
            }
            pthread_mutex_unlock( &mutex );
        }
    }
};

#else
#error Must define USE_PTHREAD or USE_WINTHREAD
#endif  /* threading kind */

} // namespace Internal

} // namespace ThreadingBuildingBlocks

#endif /* _TBB_Gate_H */
