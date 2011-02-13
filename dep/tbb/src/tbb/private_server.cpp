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

#include "rml_tbb.h"
#include "../server/thread_monitor.h"
#include "tbb/atomic.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/spin_mutex.h"
#include "tbb/tbb_thread.h"

using rml::internal::thread_monitor;

namespace tbb {
namespace internal {
namespace rml {

class private_server;

class private_worker: no_copy {
    //! State in finite-state machine that controls the worker.
    /** State diagram:
        open --> normal --> quit
          |
          V
        plugged
      */ 
    enum state_t {
        //! *this is initialized
        st_init,
        //! Associated thread is doing normal life sequence.
        st_normal,
        //! Associated thread is end normal life sequence.
        st_quit,
        //! Associated thread should skip normal life sequence, because private_server is shutting down.
        st_plugged
    };
    atomic<state_t> my_state;
    
    //! Associated server
    private_server& my_server; 

    //! Associated client
    tbb_client& my_client; 

    //! index used for avoiding the 64K aliasing problem
    const size_t my_index;

    //! Monitor for sleeping when there is no work to do.
    /** The invariant that holds for sleeping workers is:
        "my_slack<=0 && my_state==st_normal && I am on server's list of asleep threads" */
    thread_monitor my_thread_monitor;

    //! Link for list of sleeping workers
    private_worker* my_next;

    friend class private_server;

    //! Actions executed by the associated thread 
    void run();

    //! Called by a thread (usually not the associated thread) to commence termination.
    void start_shutdown();

    static __RML_DECL_THREAD_ROUTINE thread_routine( void* arg );

protected:
    private_worker( private_server& server, tbb_client& client, const size_t i ) : 
        my_server(server),
        my_client(client),
        my_index(i)
    {
        my_state = st_init;
    }

};

static const size_t cache_line_size = tbb::internal::NFS_MaxLineSize;


#if _MSC_VER && !defined(__INTEL_COMPILER)
    // Suppress overzealous compiler warnings about uninstantiatble class
    #pragma warning(push)
    #pragma warning(disable:4510 4610)
#endif
class padded_private_worker: public private_worker {
    char pad[cache_line_size - sizeof(private_worker)%cache_line_size];
public:
    padded_private_worker( private_server& server, tbb_client& client, const size_t i ) : private_worker(server,client,i) {}
};
#if _MSC_VER && !defined(__INTEL_COMPILER)
    #pragma warning(pop)
#endif

class private_server: public tbb_server, no_copy {
    tbb_client& my_client;
    const tbb_client::size_type my_n_thread;

    //! Number of jobs that could use their associated thread minus number of active threads.
    /** If negative, indicates oversubscription.
        If positive, indicates that more threads should run. 
        Can be lowered asynchronously, but must be raised only while holding my_asleep_list_mutex,
        because raising it impacts the invariant for sleeping threads. */
    atomic<int> my_slack;

    //! Counter used to determine when to delete this.
    atomic<int> my_ref_count;

    padded_private_worker* my_thread_array;

    //! List of workers that are asleep or committed to sleeping until notified by another thread.
    tbb::atomic<private_worker*> my_asleep_list_root;

    //! Protects my_asleep_list_root
    tbb::spin_mutex my_asleep_list_mutex;

#if TBB_USE_ASSERT
    atomic<int> my_net_slack_requests;
#endif /* TBB_USE_ASSERT */

    //! Used for double-check idiom
    bool has_sleepers() const {
        return my_asleep_list_root!=NULL;
    }

    //! Try to add t to list of sleeping workers
    bool try_insert_in_asleep_list( private_worker& t );

    //! Equivalent of adding additional_slack to my_slack and waking up to 2 threads if my_slack permits.
    void wake_some( int additional_slack );

    virtual ~private_server();
    
    void remove_server_ref() {
        if( --my_ref_count==0 ) {
            my_client.acknowledge_close_connection();
            this->~private_server();
            tbb::cache_aligned_allocator<private_server>().deallocate( this, 1 );
        } 
    }

    friend class private_worker;
public:
    private_server( tbb_client& client );

    /*override*/ version_type version() const {
        return 0;
    } 

    /*override*/ void request_close_connection() {
        for( size_t i=0; i<my_n_thread; ++i ) 
            my_thread_array[i].start_shutdown();
        remove_server_ref();
    }

    /*override*/ void yield() {__TBB_Yield();}

    /*override*/ void independent_thread_number_changed( int ) {__TBB_ASSERT(false,NULL);}

    /*override*/ unsigned default_concurrency() const {return tbb::tbb_thread::hardware_concurrency()-1;}

    /*override*/ void adjust_job_count_estimate( int delta );
};

//------------------------------------------------------------------------
// Methods of private_worker
//------------------------------------------------------------------------
#if _MSC_VER && !defined(__INTEL_COMPILER)
    // Suppress overzealous compiler warnings about an initialized variable 'sink_for_alloca' not referenced
    #pragma warning(push)
    #pragma warning(disable:4189)
#endif
__RML_DECL_THREAD_ROUTINE private_worker::thread_routine( void* arg ) {
    private_worker* self = static_cast<private_worker*>(arg);
    AVOID_64K_ALIASING( self->my_index );
    self->run();
    return NULL;
}
#if _MSC_VER && !defined(__INTEL_COMPILER)
    #pragma warning(pop)
#endif

void private_worker::start_shutdown() {
    state_t s; 
    // Transition from st_init or st_normal to st_plugged or st_quit
    do {
        s = my_state;
        __TBB_ASSERT( s==st_init||s==st_normal, NULL );
    } while( my_state.compare_and_swap( s==st_init? st_plugged : st_quit, s )!=s );
    if( s==st_normal ) {
        // May have invalidated invariant for sleeping, so wake up the thread.
        // Note that the notify() here occurs without maintaining invariants for my_slack.
        // It does not matter, because my_state==st_quit overrides checking of my_slack.
        my_thread_monitor.notify();
    } 
}

void private_worker::run() {
    if( my_state.compare_and_swap( st_normal, st_init )==st_init ) {
        ::rml::job& j = *my_client.create_one_job();
        --my_server.my_slack;
        while( my_state==st_normal ) {
            if( my_server.my_slack>=0 ) {
                my_client.process(j);
            } else {
                thread_monitor::cookie c;
                // Prepare to wait
                my_thread_monitor.prepare_wait(c);
                // Check/set the invariant for sleeping
                if( my_state==st_normal && my_server.try_insert_in_asleep_list(*this) ) {
                    my_thread_monitor.commit_wait(c);
                    // Propagate chain reaction
                    if( my_server.has_sleepers() )
                        my_server.wake_some(0);
                } else {
                    // Invariant broken
                    my_thread_monitor.cancel_wait();
                }
            }
        }
        my_client.cleanup(j);
        ++my_server.my_slack;
    }
    my_server.remove_server_ref();
}

//------------------------------------------------------------------------
// Methods of private_server
//------------------------------------------------------------------------
private_server::private_server( tbb_client& client ) : 
    my_client(client), 
    my_n_thread(client.max_job_count()),
    my_thread_array(NULL) 
{
    my_ref_count = my_n_thread+1;
    my_slack = 0;
#if TBB_USE_ASSERT
    my_net_slack_requests = 0;
#endif /* TBB_USE_ASSERT */
    my_asleep_list_root = NULL;
    size_t stack_size = client.min_stack_size();
    my_thread_array = tbb::cache_aligned_allocator<padded_private_worker>().allocate( my_n_thread );
    memset( my_thread_array, 0, sizeof(private_worker)*my_n_thread );
    // FIXME - use recursive chain reaction to launch the threads.
    for( size_t i=0; i<my_n_thread; ++i ) {
        private_worker* t = new( &my_thread_array[i] ) padded_private_worker( *this, client, i ); 
        thread_monitor::launch( private_worker::thread_routine, t, stack_size );
    } 
}

private_server::~private_server() {
    __TBB_ASSERT( my_net_slack_requests==0, NULL );
    for( size_t i=my_n_thread; i--; ) 
        my_thread_array[i].~padded_private_worker();
    tbb::cache_aligned_allocator<padded_private_worker>().deallocate( my_thread_array, my_n_thread );
    tbb::internal::poison_pointer( my_thread_array );
}

inline bool private_server::try_insert_in_asleep_list( private_worker& t ) {
    tbb::spin_mutex::scoped_lock lock(my_asleep_list_mutex);
    // Contribute to slack under lock so that if another takes that unit of slack,
    // it sees us sleeping on the list and wakes us up.
    int k = ++my_slack;
    if( k<=0 ) {
        t.my_next = my_asleep_list_root;
        my_asleep_list_root = &t;
        return true;
    } else {
        --my_slack;
        return false;
    }
}

void private_server::wake_some( int additional_slack ) {
    __TBB_ASSERT( additional_slack>=0, NULL );
    private_worker* wakee[2];
    private_worker**w = wakee;
    {
        tbb::spin_mutex::scoped_lock lock(my_asleep_list_mutex);
        while( my_asleep_list_root && w<wakee+2 ) {
            if( additional_slack>0 ) {
                --additional_slack;
            } else {
                // Try to claim unit of slack
                int old;
                do {
                    old = my_slack;
                    if( old<=0 ) goto done;
                } while( my_slack.compare_and_swap(old-1,old)!=old );
            }
            // Pop sleeping worker to combine with claimed unit of slack
            my_asleep_list_root = (*w++ = my_asleep_list_root)->my_next;
        }
        if( additional_slack ) {
            // Contribute our unused slack to my_slack.
            my_slack += additional_slack;
        }
    }
done:
    while( w>wakee ) 
        (*--w)->my_thread_monitor.notify();
}

void private_server::adjust_job_count_estimate( int delta ) {
#if TBB_USE_ASSERT
    my_net_slack_requests+=delta;
#endif /* TBB_USE_ASSERT */
    if( delta<0 ) {
        my_slack+=delta;
    } else if( delta>0 ) {
        wake_some( delta );
    }
}

//! Factory method called from task.cpp to create a private_server.
tbb_server* make_private_server( tbb_client& client ) {
    return new( tbb::cache_aligned_allocator<private_server>().allocate(1) ) private_server(client);
}
        
} // namespace rml
} // namespace internal
} // namespace tbb
