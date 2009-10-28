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
#define private public /* Sleazy trick to avoid publishing internal names in public header. */
#include "rml_omp.h"
#undef private

#include "tbb/tbb_allocator.h"
#include "tbb/cache_aligned_allocator.h"
#include "job_automaton.h"
#include "wait_counter.h"
#include "thread_monitor.h"
#include "tbb/aligned_space.h"
#include "tbb/atomic.h"
#include "tbb/tbb_misc.h"           // Get DetectNumberOfWorkers() from here.
#if _MSC_VER==1500 && !defined(__INTEL_COMPILER)
// VS2008/VC9 seems to have an issue; 
#pragma warning( push )
#pragma warning( disable: 4985 )
#endif
#include "tbb/concurrent_vector.h"
#if _MSC_VER==1500 && !defined(__INTEL_COMPILER)
#pragma warning( pop )
#endif

namespace rml {

namespace internal {

//! Number of hardware contexts
static inline unsigned hardware_concurrency() {
    static unsigned DefaultNumberOfThreads = 0;
    unsigned n = DefaultNumberOfThreads;
    if( !n ) DefaultNumberOfThreads = n = tbb::internal::DetectNumberOfWorkers();
    return n;
}

using tbb::internal::rml::tbb_client;
using tbb::internal::rml::tbb_server;

using __kmp::rml::omp_client;
using __kmp::rml::omp_server;

typedef versioned_object::version_type version_type;

const version_type SERVER_VERSION = 1;

static const size_t cache_line_size = tbb::internal::NFS_MaxLineSize;

template<typename Server, typename Client> class generic_connection;
class tbb_connection_v1;
class omp_connection_v1;

enum request_kind {
    rk_none,
    rk_initialize_tbb_job,
    rk_terminate_tbb_job,
    rk_initialize_omp_job,
    rk_terminate_omp_job
};

//! State of a server_thread
/** Below is a diagram of legal state transitions.

    OMP
              ts_omp_busy               
              ^          ^       
             /            \       
            /              V       
    ts_asleep <-----------> ts_idle 

    TBB 
              ts_tbb_busy               
              ^          ^       
             /            \       
            /              V       
    ts_asleep <-----------> ts_idle --> ts_done

    For TBB only. Extra state transition.

    ts_created -> ts_started -> ts_visited
 */
enum thread_state_t {
    //! Thread not doing anything useful, but running and looking for work. 
    ts_idle,
    //! Thread not doing anything useful and is asleep */
    ts_asleep,
    //! Thread is enlisted into OpenMP team
    ts_omp_busy,
    //! Thread is busy doing TBB work.
    ts_tbb_busy,
    //! For tbb threads only
    ts_done,
    ts_created,
    ts_started,
    ts_visited
};

#if TBB_USE_ASSERT
#define PRODUCE_ARG(x) ,x
#else
#define PRODUCE_ARG(x) 
#endif

//! Synchronizes dispatch of OpenMP work.
class omp_dispatch_type {
    typedef ::rml::job job_type;
    omp_client* client;
    void* cookie;
    omp_client::size_type index;
    tbb::atomic<job_type*> job;
#if TBB_USE_ASSERT
    omp_connection_v1* server;
#endif /* TBB_USE_ASSERT */
public:
    omp_dispatch_type() {job=NULL;}
    void consume();
    void produce( omp_client& c, job_type& j, void* cookie_, omp_client::size_type index_ PRODUCE_ARG( omp_connection_v1& s )) {
        __TBB_ASSERT( &j, NULL );
        __TBB_ASSERT( !job, "job already set" );
        client = &c;
#if TBB_USE_ASSERT
        server = &s;
#endif /* TBB_USE_ASSERT */
        cookie = cookie_;
        index = index_;
        // Must be last
        job = &j;
    }
};

//! A reference count.
/** No default constructor, because clients must be very careful about whether the 
    initial reference count is 0 or 1. */
class ref_count: no_copy {
    tbb::atomic<int> my_ref_count;
public:
    ref_count(int k ) {my_ref_count=k;}
    ~ref_count() {__TBB_ASSERT( !my_ref_count, "premature destruction of refcounted object" );}
    //! Add one and return new value.
    int add_ref() {
        int k = ++my_ref_count;
        __TBB_ASSERT(k>=1,"reference count underflowed before add_ref");
        return k;
    }
    //! Subtract one and return new value.
    int remove_ref() {
        int k = --my_ref_count; 
        __TBB_ASSERT(k>=0,"reference count underflow");
        return k;
    }
};

//! Forward declaration
class server_thread;
class thread_map;

//! thread_map_base; we need to make the iterator type available to server_thread
struct thread_map_base {
    //! A value in the map
    class value_type {
    public:
        server_thread& thread() {
            __TBB_ASSERT( my_thread, "thread_map::value_type::thread() called when !my_thread" );
            return *my_thread;
        }
        rml::job& job() {
            __TBB_ASSERT( my_job, "thread_map::value_type::job() called when !my_job" );
            return *my_job;
        }
        value_type() : my_thread(NULL), my_job(NULL) {}
        server_thread& wait_for_thread() const {
            for(;;) {
                server_thread* ptr=const_cast<server_thread*volatile&>(my_thread);
                if( ptr ) 
                    return *ptr;
                __TBB_Yield();
            } 
        }
        /** Shortly after when a connection is established, it is possible for the server
            to grab a server_thread that has not yet created a job object for that server. */
        rml::job& wait_for_job() const {
            if( !my_job ) {
                my_job = &my_automaton.wait_for_job();
            }
            return *my_job;
        }
    private:
        server_thread* my_thread;
        /** Marked mutable because though it is physically modified, conceptually it is a duplicate of 
            the job held by job_automaton. */
        mutable rml::job* my_job;
        job_automaton my_automaton;
// FIXME - pad out to cache line, because my_automaton is hit hard by thread()
        friend class thread_map;
    };
    typedef tbb::concurrent_vector<value_type,tbb::zero_allocator<value_type,tbb::cache_aligned_allocator> > array_type;
};

template<typename T>
class padded: public T {
    char pad[cache_line_size - sizeof(T)%cache_line_size];
};

// FIXME - should we pad out memory to avoid false sharing of our global variables?

static tbb::atomic<int> the_balance;
static tbb::atomic<int> the_balance_inited;

//! Per thread information 
/** ref_count holds number of clients that are using this, 
    plus 1 if a host thread owns this instance. */
class server_thread: public ref_count {
    friend class thread_map;
    template<typename Server, typename Client> friend class generic_connection;
    //! Integral type that can hold a thread_state_t
    typedef int thread_state_rep_t;
    tbb::atomic<thread_state_rep_t> state;
public:
    thread_monitor monitor;
    // FIXME: make them private...
    bool    is_omp_thread;
    tbb::atomic<thread_state_rep_t> tbb_state;
    server_thread* link; // FIXME: this is a temporary fix. Remove when all is done.
    thread_map_base::array_type::iterator my_map_pos;
private:
    rml::server *my_conn;
    rml::job* my_job;
    job_automaton* my_ja;
    size_t my_index;

#if TBB_USE_ASSERT
    //! Flag used to check if thread is still using *this.
    bool has_active_thread;
#endif /* TBB_USE_ASSERT */

    //! Volunteer to sleep. 
    void sleep_perhaps( thread_state_t asleep );

    //! Destroy job corresponding to given client
    /** Return true if thread must quit. */
    template<typename Connection>
    bool destroy_job( Connection& c );

    //! Process requests
    /** Return true if thread must quit. */
    bool process_requests();

    void loop();
    static __RML_DECL_THREAD_ROUTINE thread_routine( void* arg ); 
public:
    thread_state_t read_state() const {
        thread_state_rep_t s = state;
        __TBB_ASSERT( unsigned(s)<=unsigned(ts_done), "corrupted server thread?" );
        return thread_state_t(s);
    }

    tbb::atomic<request_kind> request;

    omp_dispatch_type omp_dispatch;

    server_thread();
    ~server_thread();

    //! Launch a thread that is bound to *this.
    void launch( size_t stack_size );

    //! Attempt to wakeup a thread 
    /** The value "to" is the new state for the thread, if it was woken up.
        Returns true if thread was woken up, false otherwise. */
    bool wakeup( thread_state_t to, thread_state_t from );

    //! Attempt to enslave a thread for OpenMP/TBB.
    bool try_grab_for( thread_state_t s );
};

//! Bag of threads that are private to a client.
class private_thread_bag {
    struct list_thread: server_thread {
       list_thread* next;
    };
    //! Root of atomic linked list of list_thread
    /** ABA problem is avoided because items are only atomically pushed, never popped. */
    tbb::atomic<list_thread*> my_root; 
    tbb::cache_aligned_allocator<padded<list_thread> > my_allocator; 
public:
    //! Construct empty bag
    private_thread_bag() {my_root=NULL;}

    //! Create a fresh server_thread object.
    server_thread& add_one_thread() {
        list_thread* t = my_allocator.allocate(1);
        new( t ) list_thread;
        // Atomically add to list
        list_thread* old_root;
        do {
            old_root = my_root;
            t->next = old_root;
        } while( my_root.compare_and_swap( t, old_root )!=old_root );
        return *t;  
    }

    //! Destroy the bag and threads in it. 
    ~private_thread_bag() {
        while( my_root ) {
            // Unlink thread from list.
            list_thread* t = my_root;
            my_root = t->next;
            // Destroy and deallocate the thread.
            t->~list_thread();
            my_allocator.deallocate(static_cast<padded<list_thread>*>(t),1);    
        }
    }
};

//! Forward declaration
void wakeup_some_tbb_threads();

//! Type-independent part of class generic_connection. *
/** One to one map from server threads to jobs, and associated reference counting. */
class thread_map : public thread_map_base {
public:
    typedef rml::client::size_type size_type;
    //! ctor
    thread_map( wait_counter& fc, ::rml::client& client ) : 
        all_visited_at_least_once(false), my_min_stack_size(0), my_server_ref_count(1),
        my_client_ref_count(1), my_client(client), my_factory_counter(fc)
    { my_unrealized_threads = 0; }
    //! dtor
    ~thread_map() {}
    typedef array_type::iterator iterator;
    iterator begin() {return my_array.begin();}
    iterator end() {return my_array.end();}
    void bind( /* rml::server& server, message_kind initialize */ );
    void unbind( request_kind terminate );
    void assist_cleanup( bool assist_null_only );

    /** Returns number of unrealized threads to create. */
    size_type wakeup_tbb_threads( size_type n );
    bool wakeup_next_thread( iterator i, tbb_connection_v1& conn );
    void release_tbb_threads( server_thread* t );
    void adjust_balance( int delta );

    //! Add a server_thread object to the map, but do not bind it.
    /** Return NULL if out of unrealized threads. */
    value_type* add_one_thread( bool is_omp_thread_ );

    void bind_one_thread( rml::server& server, request_kind initialize, value_type& x );

    void remove_client_ref();
    int add_server_ref() {return my_server_ref_count.add_ref();}
    int remove_server_ref() {return my_server_ref_count.remove_ref();}

    ::rml::client& client() const {return my_client;}

    size_type get_unrealized_threads() { return my_unrealized_threads; }

private:
    private_thread_bag my_private_threads;
    bool all_visited_at_least_once;
    array_type my_array;
    size_t my_min_stack_size;
    tbb::atomic<size_type> my_unrealized_threads;

    //! Number of threads referencing *this, plus one extra.
    /** When it becomes zero, the containing server object can be safely deleted. */
    ref_count my_server_ref_count;

    //! Number of jobs that need cleanup, plus one extra.
    /** When it becomes zero, acknowledge_close_connection is called. */
    ref_count my_client_ref_count;
    ::rml::client& my_client;
    //! Counter owned by factory that produced this thread_map.
    wait_counter& my_factory_counter;
};

void thread_map::bind_one_thread( rml::server& server, request_kind initialize, value_type& x ) {
    // Add one to account for the thread referencing this map hereforth.
    server_thread& t = x.thread();
    my_server_ref_count.add_ref();
    my_client_ref_count.add_ref();
#if TBB_USE_ASSERT
    __TBB_ASSERT( t.add_ref()==1, NULL );
#else
    t.add_ref();
#endif
    // Have responsibility to start the thread.
    t.my_conn = &server;
    t.my_ja = &x.my_automaton;
    t.request = initialize;
    t.launch( my_min_stack_size );
    // Must wakeup thread so it can fill in its "my_job" field in *this.
    // Otherwise deadlock can occur where wait_for_job spins on thread that is sleeping.
    __TBB_ASSERT( t.state!=ts_tbb_busy, NULL );
    t.wakeup( ts_idle, ts_asleep );
}

thread_map::value_type* thread_map::add_one_thread( bool is_omp_thread_ ) {
    size_type u;
    do {
        u = my_unrealized_threads;
        if( !u ) return NULL;
    } while( my_unrealized_threads.compare_and_swap(u-1,u)!=u );
    server_thread& t = my_private_threads.add_one_thread();
    t.is_omp_thread = is_omp_thread_;
    __TBB_ASSERT( u>=1, NULL );
    t.my_index = u - 1;
    __TBB_ASSERT( t.state!=ts_tbb_busy, NULL );
    if( !t.is_omp_thread )
        t.tbb_state = ts_created;
    iterator i = t.my_map_pos = my_array.grow_by(1);
    value_type& v = *i;
    v.my_thread = &t;
    return &v;
}

void thread_map::bind( /* rml::server& server, request_kind initialize */ ) {
    ++my_factory_counter;
    my_min_stack_size = my_client.min_stack_size();
    __TBB_ASSERT( my_unrealized_threads==0, "already called bind?" );
    my_unrealized_threads = my_client.max_job_count();
}

void thread_map::unbind( request_kind terminate ) {
    // Ask each server_thread to cleanup its job for this server.
    for( iterator i=begin(); i!=end(); ++i ) {
        server_thread& t = i->thread();
        // The last parameter of the message is not used by the recipient. 
        t.request = terminate;
        t.wakeup( ts_idle, ts_asleep );
    }
    // Remove extra ref to client.
    remove_client_ref();
}

void thread_map::assist_cleanup( bool assist_null_only ) {
    // To avoid deadlock, the current thread *must* help out with cleanups that have not started,
    // becausd the thread that created the job may be busy for a long time.
    for( iterator i = begin(); i!=end(); ++i ) {
        rml::job* j=0;
        job_automaton& ja = i->my_automaton;
        if( assist_null_only ? ja.try_plug_null() : ja.try_plug(j) ) {
            if( j ) {
                my_client.cleanup(*j);
            } else {
                // server thread did not get a chance to create a job.
            }
            remove_client_ref();
        } 
    }
}

thread_map::size_type thread_map::wakeup_tbb_threads( size_type n ) {
    __TBB_ASSERT(n>0,"must specify positive number of threads to wake up");
    iterator e = end();
    for( iterator k=begin(); k!=e; ++k ) {
        // If another thread added *k, there is a tiny timing window where thread() is invalid.
        server_thread& t = k->wait_for_thread();
        if( t.tbb_state==ts_created || t.read_state()==ts_tbb_busy )
            continue;
        if( --the_balance>=0 ) { // try to withdraw a coin from the deposit
            while( !t.try_grab_for( ts_tbb_busy ) ) {
                if( t.read_state()==ts_tbb_busy ) {
                    // we lost; move on to the next.
                    ++the_balance;
                    goto skip;
                }
            }
            if( --n==0 ) 
                return 0;
        } else {
            // overdraft.
            ++the_balance;
            break;
        }
skip:
        ;
    }
    return n<my_unrealized_threads ? n : my_unrealized_threads;
}

void thread_map::remove_client_ref() {
    int k = my_client_ref_count.remove_ref();
    if( k==0 ) {
        // Notify factory that thread has crossed back into RML.
        --my_factory_counter;
        // Notify client that RML is done with the client object.
        my_client.acknowledge_close_connection();
    } 
}

//------------------------------------------------------------------------
// generic_connection
//------------------------------------------------------------------------

template<typename Server, typename Client>
struct connection_traits {};

static tbb::atomic<tbb_connection_v1*> this_tbb_connection;

template<typename Server, typename Client>
class generic_connection: public Server, no_copy {
    /*override*/ version_type version() const {return SERVER_VERSION;}
    /*override*/ void yield() {thread_monitor::yield();}
    /*override*/ void independent_thread_number_changed( int delta ) {my_thread_map.adjust_balance( -delta );}
    /*override*/ unsigned default_concurrency() const {return hardware_concurrency()-1;}

protected:
    thread_map my_thread_map;
    void do_open() {my_thread_map.bind();}
    void request_close_connection();
    //! Make destructor virtual
    virtual ~generic_connection() {}
    generic_connection( wait_counter& fc, Client& c ) : my_thread_map(fc,c) {}

public:
    Client& client() const {return static_cast<Client&>(my_thread_map.client());}
    int add_server_ref () {return my_thread_map.add_server_ref();}
    void remove_server_ref() {if( my_thread_map.remove_server_ref()==0 ) delete this;}
    void remove_client_ref() {my_thread_map.remove_client_ref();}
    void make_job( server_thread& t, job_automaton& ja );
};

//------------------------------------------------------------------------
// TBB server
//------------------------------------------------------------------------

template<>
struct connection_traits<tbb_server,tbb_client> {
    static const request_kind initialize = rk_initialize_tbb_job;
    static const request_kind terminate = rk_terminate_tbb_job;
    static const bool assist_null_only = true;
    static const bool is_tbb = true;
};

//! Represents a server and client binding.
/** The internal representation uses inheritance for the server part and a pointer for the client part. */
class tbb_connection_v1: public generic_connection<tbb_server,tbb_client> {
    friend void wakeup_some_tbb_threads();
    /*override*/ void adjust_job_count_estimate( int delta );
    //! Estimate on number of jobs without threads working on them.
    tbb::atomic<int> my_slack;
    friend class dummy_class_to_shut_up_gratuitous_warning_from_gcc_3_2_3;
#if TBB_USE_ASSERT
    tbb::atomic<int> my_job_count_estimate;
#endif /* TBB_USE_ASSERT */

    // pad these? or use a single variable w/ atomic add/subtract?
    tbb::atomic<int> n_adjust_job_count_requests;
    ~tbb_connection_v1();

public:
    enum tbb_conn_t {
        c_empty  =  0,
        c_init   = -1,
        c_locked = -2
    };

    //! True if there is slack that try_process can use.
    bool has_slack() const {return my_slack>0;}

    bool try_process( job& job ) {
        bool visited = false;
        // No check for my_slack>0 here because caller is expected to do that check.
        int k = --my_slack;
        if( k>=0 ) {
            client().process(job);
            visited = true;
        }
        ++my_slack; 
        return visited;
    }

    tbb_connection_v1( wait_counter& fc, tbb_client& client ) : generic_connection<tbb_server,tbb_client>(fc,client) {
        my_slack = 0;
#if TBB_USE_ASSERT
        my_job_count_estimate = 0;
#endif /* TBB_USE_ASSERT */
        __TBB_ASSERT( !my_slack, NULL );
        do_open();
        __TBB_ASSERT( this_tbb_connection==reinterpret_cast<tbb_connection_v1*>(tbb_connection_v1::c_init), NULL );
        n_adjust_job_count_requests = 0;
        this_tbb_connection = this;
    }

    void wakeup_tbb_threads( unsigned n ) {my_thread_map.wakeup_tbb_threads( n );}
    bool wakeup_next_thread( thread_map::iterator i ) {return my_thread_map.wakeup_next_thread( i, *this );}
    thread_map::size_type get_unrealized_threads () {return my_thread_map.get_unrealized_threads();}
};

/* to deal with cases where the machine is oversubscribed; we want each thread to trip to try_process() at least once */
/* this should not involve computing the_balance */
bool thread_map::wakeup_next_thread( thread_map::iterator this_thr, tbb_connection_v1& conn ) {
    if( all_visited_at_least_once ) 
        return false;

    iterator e = end();
retry:
    bool exist = false;
    iterator k=this_thr; 
    for( ++k; k!=e; ++k ) {
        // If another thread added *k, there is a tiny timing window where thread() is invalid.
        server_thread& t = k->wait_for_thread();
        if( t.tbb_state!=ts_visited )
            exist = true;
        if( t.read_state()!=ts_tbb_busy && t.tbb_state==ts_started )
            if( t.try_grab_for( ts_tbb_busy ) )
                return true;
    }
    for( k=begin(); k!=this_thr; ++k ) {
        server_thread& t = k->wait_for_thread();
        if( t.tbb_state!=ts_visited )
            exist = true;
        if( t.read_state()!=ts_tbb_busy && t.tbb_state==ts_started )
            if( t.try_grab_for( ts_tbb_busy ) )
                return true;
    }

    if( exist ) 
        if( conn.has_slack() )
            goto retry;
    else 
        all_visited_at_least_once = true;
    return false;
}

void thread_map::release_tbb_threads( server_thread* t ) {
    for( ; t; t = t->link ) {
        while( t->read_state()!=ts_asleep )
            __TBB_Yield();
        t->tbb_state = ts_started;
    }
}

void thread_map::adjust_balance( int delta ) {
    int new_balance = the_balance += delta;
    if( new_balance>0 && 0>=new_balance-delta /*== old the_balance*/ )
        wakeup_some_tbb_threads();
}

//------------------------------------------------------------------------
// OpenMP server
//------------------------------------------------------------------------

template<>
struct connection_traits<omp_server,omp_client> {
    static const request_kind initialize = rk_initialize_omp_job;
    static const request_kind terminate = rk_terminate_omp_job;
    static const bool assist_null_only = false;
    static const bool is_tbb = false;
};

class omp_connection_v1: public generic_connection<omp_server,omp_client> {
    /*override*/ int current_balance() const {return the_balance;}
    /*override*/ int try_increase_load( size_type n, bool strict ); 
    /*override*/ void decrease_load( size_type n ); 
    /*override*/ void get_threads( size_type request_size, void* cookie, job* array[] );
public:
#if TBB_USE_ASSERT
    //! Net change in delta caused by this connection.
    /** Should be zero when connection is broken */
    tbb::atomic<int> net_delta;
#endif /* TBB_USE_ASSERT */

    omp_connection_v1( wait_counter& fc, omp_client& client ) : generic_connection<omp_server,omp_client>(fc,client) {
#if TBB_USE_ASSERT
        net_delta = 0;
#endif /* TBB_USE_ASSERT */
        do_open();
    }
    ~omp_connection_v1() {__TBB_ASSERT( net_delta==0, "net increase/decrease of load is nonzero" );}
};

template<typename Server, typename Client>    
void generic_connection<Server,Client>::request_close_connection() {
#if _MSC_VER && !defined(__INTEL_COMPILER)
// Suppress "conditional expression is constant" warning.
#pragma warning( push )
#pragma warning( disable: 4127 ) 
#endif          
    if( connection_traits<Server,Client>::is_tbb ) {
        __TBB_ASSERT( this_tbb_connection==reinterpret_cast<tbb_connection_v1*>(this), NULL );
        tbb_connection_v1* conn;
        do {
            while( (conn=this_tbb_connection)==reinterpret_cast<tbb_connection_v1*>(tbb_connection_v1::c_locked) )
                __TBB_Yield();
        } while  ( this_tbb_connection.compare_and_swap(0, conn)!=conn );
    }
#if _MSC_VER && !defined(__INTEL_COMPILER)
#pragma warning( pop )
#endif
    my_thread_map.unbind( connection_traits<Server,Client>::terminate );
    my_thread_map.assist_cleanup( connection_traits<Server,Client>::assist_null_only );
    // Remove extra reference
    remove_server_ref();
}

template<typename Server, typename Client>
void generic_connection<Server,Client>::make_job( server_thread& t, job_automaton& ja ) {
    if( ja.try_acquire() ) {
        rml::job& j = *client().create_one_job();
        __TBB_ASSERT( &j!=NULL, "client:::create_one_job returned NULL" );
        __TBB_ASSERT( (intptr_t(&j)&1)==0, "client::create_one_job returned misaligned job" );
        ja.set_and_release( j );
        __TBB_ASSERT( t.my_conn && t.my_ja && t.my_job==NULL, NULL );
        t.my_job  = &j;
    }
}

tbb_connection_v1::~tbb_connection_v1() {
#if TBB_USE_ASSERT
    if( my_job_count_estimate!=0 ) {
        fprintf(stderr, "TBB client tried to disconnect with non-zero net job count estimate of %d\n", int(my_job_count_estimate ));
        abort();
    }
    __TBB_ASSERT( !my_slack, "attempt to destroy tbb_server with nonzero slack" );
    __TBB_ASSERT( this!=this_tbb_connection, "request_close_connection() must be called" );
#endif /* TBB_USE_ASSERT */
    // if the next connection has unstarted threads, start one of them.
    wakeup_some_tbb_threads();
}

void tbb_connection_v1::adjust_job_count_estimate( int delta ) {
#if TBB_USE_ASSERT
    my_job_count_estimate += delta;
#endif /* TBB_USE_ASSERT */
    // Atomically update slack.
    int c = my_slack+=delta;
    if( c>0 ) {
        ++n_adjust_job_count_requests;
        // The client has work to do and there are threads available
        thread_map::size_type n = my_thread_map.wakeup_tbb_threads(c); 

        server_thread* new_threads_anchor = NULL;
        thread_map::size_type i;
        for( i=0; i<n; ++i ) {
            // Obtain unrealized threads
            thread_map::value_type* k = my_thread_map.add_one_thread( false );
            if( !k ) 
                // No unrealized threads left.
                break;
            // eagerly start the thread off.
            my_thread_map.bind_one_thread( *this, rk_initialize_tbb_job, *k );
            server_thread& t = k->thread();
            __TBB_ASSERT( !t.link, NULL );
            t.link = new_threads_anchor;
            new_threads_anchor = &t;
        }

        thread_map::size_type j=0; 
        for( ; the_balance>0 && j<i; ++j ) {
            if( --the_balance>=0 ) {
                // withdraw a coin from the bank
                __TBB_ASSERT( new_threads_anchor, NULL );

                server_thread* t = new_threads_anchor;
                new_threads_anchor = t->link;
                while( !t->try_grab_for( ts_tbb_busy ) ) 
                    __TBB_Yield();
                t->tbb_state = ts_started;
            } else {
                // overdraft. return it to the bank
                ++the_balance;
                break;
            }
        }
        __TBB_ASSERT( i-j!=0||new_threads_anchor==NULL, NULL );
        // mark the ones that did not get started as eligible for being snatched.
        if( new_threads_anchor )
            my_thread_map.release_tbb_threads( new_threads_anchor );

        --n_adjust_job_count_requests;
    }
}

//! wake some available tbb threads
/**
     First, atomically grab the connection, then increase the server ref count to keep it from being released prematurely.
     Second, check if the balance is available for TBB and the tbb conneciton has slack to exploit.
     If the answer is true, go ahead and try to wake some up.
 */
void wakeup_some_tbb_threads()
{
    for( ;; ) {
        tbb_connection_v1* conn = this_tbb_connection;
        /*
            if( conn==0 or conn==tbb_connection_v1::c_init )
                the next connection will see my last change to the deposit; do nothing
            if( conn==tbb_connection_v1::c_locked )
                a thread is already in the region A-B below. 
                it will read the change made by threads of my connection to the_balance;
                do nothing

            0==c_empty, -1==c_init, -2==c_locked
        */
        if( ((-ptrdiff_t(conn))&~3 )==0 )
            return;

        // FIXME: place the_balance next to tbb_this_connection ? to save some cache moves ?
        /* region A: this is the only place to set this_tbb_connection to c_locked */
        tbb_connection_v1* old_ttc = this_tbb_connection.compare_and_swap( reinterpret_cast<tbb_connection_v1*>(tbb_connection_v1::c_locked), conn );
        if( old_ttc==conn ) {
#if USE_TBB_ASSERT
            __TBB_ASSERT( conn->add_server_ref()>1, NULL );
#else
            conn->add_server_ref();
#endif
            /* region B: this is the only place to restore this_tbb_connection from c_locked */
            this_tbb_connection = conn; // restoring it means releasing it

            /* some threads are creating tbb server threads; they may not see my changes made to the_balance */
            while( conn->n_adjust_job_count_requests>0 )
                __TBB_Yield();

            int bal = the_balance;
            if( bal>0 && conn->has_slack() ) 
                conn->wakeup_tbb_threads( bal );
            conn->remove_server_ref();
            break;
        } else if( ((-ptrdiff_t(old_ttc))&~3)==0 ) {
            return; /* see above */
        } else {
            __TBB_Yield();
        }
    }
}

int omp_connection_v1::try_increase_load( size_type n, bool strict ) { 
    __TBB_ASSERT(int(n)>=0,NULL);
    if( strict ) {
        the_balance-=int(n);
    } else {
        int avail, old;
        do {
            avail = the_balance;
            if( avail<=0 ) {
                // No atomic read-write-modify operation necessary.
                return avail;
            }
            // don't read the_balance; if it changes, compare_and_swap will fail anyway.
            old = the_balance.compare_and_swap( int(n)<avail ? avail-n : 0, avail );
        } while( old!=avail );
        if( int(n)>avail ) 
            n=avail;
    }
#if TBB_USE_ASSERT
    net_delta += n;
#endif /* TBB_USE_ASSERT */
    return n;
}

void omp_connection_v1::decrease_load( size_type n ) {
    __TBB_ASSERT(int(n)>=0,NULL);
    my_thread_map.adjust_balance(int(n));
#if TBB_USE_ASSERT
    net_delta -= n;
#endif /* TBB_USE_ASSERT */
}

void omp_connection_v1::get_threads( size_type request_size, void* cookie, job* array[] ) {

    if( !request_size ) 
        return;

    unsigned index = 0;
    for(;;) { // don't return until all request_size threads are grabbed.
        // Need to grab some threads
        thread_map::iterator k_end=my_thread_map.end();
        // FIXME - this search is going to be *very* slow when there is a large number of threads and most are in use.
        // Consider starting search at random point, or high water mark of sorts.
        for( thread_map::iterator k=my_thread_map.begin(); k!=k_end; ++k ) {
            // If another thread added *k, there is a tiny timing window where thread() is invalid.
            server_thread& t = k->wait_for_thread();
            if( t.try_grab_for( ts_omp_busy ) ) {
                // The preincrement instead of post-increment of index is deliberate.
                job& j = k->wait_for_job();
                    array[index] = &j;
                t.omp_dispatch.produce( client(), j, cookie, index PRODUCE_ARG(*this) );
                if( ++index==request_size ) 
                    return;
            } 
        }
        // Need to allocate more threads
        for( unsigned i=index; i<request_size; ++i ) {
            __TBB_ASSERT( index<request_size, NULL );
            thread_map::value_type* k = my_thread_map.add_one_thread( true );
            if( !k ) {
                // Client erred
                fprintf(stderr,"server::get_threads: exceeded job_count\n");
                __TBB_ASSERT(false, NULL);
            }
            my_thread_map.bind_one_thread( *this, rk_initialize_omp_job, *k );
            server_thread& t = k->thread();
            if( t.try_grab_for( ts_omp_busy ) ) {
                job& j = k->wait_for_job();
                array[index] = &j;
                // The preincrement instead of post-increment of index is deliberate.
                t.omp_dispatch.produce( client(), j, cookie, index PRODUCE_ARG(*this) );
                if( ++index==request_size ) 
                    return;
            } // else someone else snatched it.
        }
    }
}

//------------------------------------------------------------------------
// Methods of omp_dispatch_type
//------------------------------------------------------------------------
void omp_dispatch_type::consume() {
    job_type* j; 
    // Wait for short window between when master sets state of this thread to ts_omp_busy
    // and master thread calls produce.
    // FIXME - this is a very short spin while the producer is setting fields of *this, 
    // but nonetheless the loop should probably use exponential backoff, or at least pause instructions.
    do {
        j = job;
    } while( !j );
    job = static_cast<job_type*>(NULL);
    client->process(*j,cookie,index);
#if TBB_USE_ASSERT
    // Return of method process implies "decrease_load" from client's viewpoint, even though
    // the actual adjustment of the_balance only happens when this thread really goes to sleep.
    --server->net_delta;
#endif /* TBB_USE_ASSERT */
}

//------------------------------------------------------------------------
// Methods of server_thread
//------------------------------------------------------------------------

server_thread::server_thread() : 
    ref_count(0),
    link(NULL), // FIXME: remove when all fixes are done.
    my_map_pos(),
    my_conn(NULL), my_job(NULL), my_ja(NULL)
{
    state = ts_idle;
#if TBB_USE_ASSERT
    has_active_thread = false;
#endif /* TBB_USE_ASSERT */
}

server_thread::~server_thread() {
    __TBB_ASSERT( !has_active_thread, NULL );
}

#if _MSC_VER && !defined(__INTEL_COMPILER)
    // Suppress overzealous compiler warnings about an initialized variable 'sink_for_alloca' not referenced
    #pragma warning(push)
    #pragma warning(disable:4189)
#endif
__RML_DECL_THREAD_ROUTINE server_thread::thread_routine( void* arg ) {
    server_thread* self = static_cast<server_thread*>(arg);
    AVOID_64K_ALIASING( self->my_index );
#if TBB_USE_ASSERT
    __TBB_ASSERT( !self->has_active_thread, NULL );
    self->has_active_thread = true;
#endif /* TBB_USE_ASSERT */
    self->loop();
    return NULL;
}
#if _MSC_VER && !defined(__INTEL_COMPILER)
    #pragma warning(pop)
#endif

void server_thread::launch( size_t stack_size ) {
    thread_monitor::launch( thread_routine, this, stack_size );
}

void server_thread::sleep_perhaps( thread_state_t asleep ) {
    __TBB_ASSERT( asleep==ts_asleep, NULL );
    thread_monitor::cookie c; 
    monitor.prepare_wait(c);
    if( state.compare_and_swap( asleep, ts_idle )==ts_idle ) {
        if( request==rk_none ) {
            monitor.commit_wait(c);
            // Someone else woke me up.  The compare_and_swap further below deals with spurious wakeups.
        } else {
            monitor.cancel_wait();
        }
        // Following compare-and-swap logic tries to transition from asleep to idle while both ignoring the
        // preserving the reserved_flag bit in state, because some other thread may be asynchronously clearing
        // the reserved_flag bit within state.
        thread_state_t s = read_state();
        if( s==ts_asleep ) {
            state.compare_and_swap( ts_idle, ts_asleep );
            // I woke myself up, either because I cancelled the wait or suffered a spurious wakeup.
        } else {
            // Someone else woke me up; there the_balance is decremented by 1. -- tbb only
            if( !is_omp_thread ) {
                __TBB_ASSERT( state==ts_tbb_busy||state==ts_idle, NULL );
            }
        }
    } else {
        // someone else made it busy ; see try_grab_for when state==ts_idle.
        __TBB_ASSERT( state==ts_omp_busy||state==ts_tbb_busy, NULL );
        monitor.cancel_wait();
    }
    __TBB_ASSERT( read_state()!=asleep, "a thread can only put itself to sleep" );
}

bool server_thread::wakeup( thread_state_t to, thread_state_t from ) {
    bool success = false;
    __TBB_ASSERT( from==ts_asleep && (to==ts_idle||to==ts_omp_busy||to==ts_tbb_busy), NULL );
    if( state.compare_and_swap( to, from )==from ) {
        if( !is_omp_thread ) __TBB_ASSERT( to==ts_idle||to==ts_tbb_busy, NULL );
        // There is a small timing window that permits balance to become negative,
        // but such occurrences are probably rare enough to not worry about, since
        // at worst the result is slight temporary oversubscription.
        monitor.notify();
        success = true;
    } 
    return success;
}

//! Attempt to change a thread's state to ts_omp_busy, and waking it up if necessary. 
bool server_thread::try_grab_for( thread_state_t target_state ) {
    bool success = false;
    switch( read_state() ) {
        case ts_asleep: 
            success = wakeup( target_state, ts_asleep );
            break;
        case ts_idle:
            success = state.compare_and_swap( target_state, ts_idle )==ts_idle;
            break;
        default:
            // Thread is not available to be part of an OpenMP thread team.
            break;
    }
    return success;
}

template<typename Connection>
bool server_thread::destroy_job( Connection& c ) {
    __TBB_ASSERT( !is_omp_thread||state==ts_idle, NULL );
    __TBB_ASSERT( is_omp_thread||(state==ts_idle||state==ts_tbb_busy), NULL );
    if( !is_omp_thread ) {
        __TBB_ASSERT( state==ts_idle||state==ts_tbb_busy, NULL );
        if( state==ts_idle )
            state.compare_and_swap( ts_done, ts_idle );
        // 'state' may be set to ts_tbb_busy by another thread..

        if( state==ts_tbb_busy ) { // return the coin to the deposit
            // need to deposit first to let the next connection see the change
            ++the_balance;
            state = ts_done; // no other thread changes the state when it is ts_*_busy
        }
    }
    if( job_automaton* ja = my_ja ) {
        rml::job* j;
        if( ja->try_plug(j) ) {
            __TBB_ASSERT( j, NULL );
            c.client().cleanup(*j);
            c.remove_client_ref();
        } else {
            // Some other thread took responsibility for cleaning up the job.
        }
    }
    //! Must do remove client reference first, because execution of c.remove_ref() can cause *this to be destroyed.
    int k = remove_ref();
    __TBB_ASSERT_EX( k==0, "more than one references?" );
#if TBB_USE_ASSERT
    has_active_thread = false;
#endif /* TBB_USE_ASSERT */
    c.remove_server_ref();
    return true;
}

bool server_thread::process_requests() {
    __TBB_ASSERT( request!=rk_none, "should only be called when at least one request is present" );
    do {
        request_kind my_req = request;
        request.compare_and_swap( rk_none, my_req );
        switch( my_req ) {
            case rk_initialize_tbb_job: 
                static_cast<tbb_connection_v1*>(my_conn)->make_job( *this, *my_ja );
                break;
            
            case rk_initialize_omp_job: 
                static_cast<omp_connection_v1*>(my_conn)->make_job( *this, *my_ja );
                break;

            case rk_terminate_tbb_job:
                if( destroy_job( *static_cast<tbb_connection_v1*>(my_conn) ) )
                    return true;
                break; 

            case rk_terminate_omp_job:
                if( destroy_job( *static_cast<omp_connection_v1*>(my_conn) ) )
                    return true;
                break; 
            default:
                break;
         }
    } while( request!=rk_none );
    return false;
}

//! Loop that each thread executes
void server_thread::loop() {
    for(;;) {
        __TBB_Yield();
        if( state==ts_idle )
            sleep_perhaps( ts_asleep );   

        // Drain mailbox before reading the state.
        if( request!=rk_none ) 
            if( process_requests() )
                return;     
             
        // read the state after draining the mail box
        thread_state_t s = read_state();
        __TBB_ASSERT( s==ts_idle||s==ts_omp_busy||s==ts_tbb_busy, NULL );

        if( s==ts_omp_busy ) {
            // Enslaved by OpenMP team.  
            omp_dispatch.consume();
            /* here wake a tbb thread up if feasible */
            int bal = ++the_balance;
            if( bal>0 )
                wakeup_some_tbb_threads();
            state = ts_idle;
        } else if( s==ts_tbb_busy ) {
            // do some TBB work.
            __TBB_ASSERT( my_conn && my_job, NULL );
            tbb_connection_v1& conn = *static_cast<tbb_connection_v1*>(my_conn);
            // give openmp higher priority
            bool has_coin = true;
            while( has_coin && conn.has_slack() && the_balance>=0 ) {
                if( conn.try_process(*my_job) ) {
                    tbb_state = ts_visited;
                    if( conn.has_slack() && the_balance>=0 )
                        has_coin = !conn.wakeup_next_thread( my_map_pos );
                }
            }
            state = ts_idle;
            if( has_coin ) {
                ++the_balance; // return the coin back to the deposit
                if( conn.has_slack() ) { // a new adjust_job_request_estimate() is in progress
                                         // it may have missed my changes to state and/or the_balance
                    int bal = --the_balance; // try to grab the coin back
                    if( bal>=0 ) { // I got the coin
                        if( state.compare_and_swap( ts_tbb_busy, ts_idle )!=ts_idle )
                            ++the_balance; // someone else enlisted me.
                    } else {
                        // overdraft. return the coin
                        ++the_balance;
                    }
                } // else the new request will see my changes to state & the_balance.
            }
        }
    }
}

template<typename Connection, typename Server, typename Client>
static factory::status_type connect( factory& f, Server*& server, Client& client ) {
#if _MSC_VER && !defined(__INTEL_COMPILER)
// Suppress "conditional expression is constant" warning.
#pragma warning( push )
#pragma warning( disable: 4127 ) 
#endif          
    if( connection_traits<Server,Client>::is_tbb )
        if( this_tbb_connection.compare_and_swap(reinterpret_cast<tbb_connection_v1*>(-1), reinterpret_cast<tbb_connection_v1*>(0))!=0 )
            return factory::st_connection_exists;
#if _MSC_VER && !defined(__INTEL_COMPILER)
#pragma warning( pop )
#endif
    server = new Connection(*static_cast<wait_counter*>(f.scratch_ptr),client);
    return factory::st_success; 
}

extern "C" factory::status_type __RML_open_factory( factory& f, version_type& server_version, version_type client_version ) {
    // Hack to keep this library from being closed by causing the first client's dlopen to not have a corresponding dlclose. 
    // This code will be removed once we figure out how to do shutdown of the RML perfectly.
    static tbb::atomic<bool> one_time_flag;
    if( one_time_flag.compare_and_swap(true,false)==false) {
        f.library_handle = NULL;
    }
    // End of hack

    // initialize the_balance only once
    if( the_balance_inited==0 ) {
        if( the_balance_inited.compare_and_swap( 1, 0 )==0 ) {
            the_balance = hardware_concurrency()-1;
            the_balance_inited = 2;
        } else {
            tbb::internal::spin_wait_until_eq( the_balance_inited, 2 );
        }
    }

    server_version = SERVER_VERSION;
    f.scratch_ptr = 0;
    if( client_version==0 ) {
        return factory::st_incompatible;
    } else {
        f.scratch_ptr = new wait_counter;
        return factory::st_success;
    }
}

extern "C" void __RML_close_factory( factory& f ) {
    if( wait_counter* fc = static_cast<wait_counter*>(f.scratch_ptr) ) {
        f.scratch_ptr = 0;
        fc->wait();
        delete fc;
    }
}

void call_with_build_date_str( ::rml::server_info_callback_t cb, void* arg );

}} // rml::internal 

namespace tbb {
namespace internal {
namespace rml {

extern "C" tbb_factory::status_type __TBB_make_rml_server( tbb_factory& f, tbb_server*& server, tbb_client& client ) {
    return ::rml::internal::connect< ::rml::internal::tbb_connection_v1>(f,server,client);
}

extern "C" void __TBB_call_with_my_server_info( ::rml::server_info_callback_t cb, void* arg ) {
    return ::rml::internal::call_with_build_date_str( cb, arg );
}

}}}

namespace __kmp {
namespace rml {

extern "C" omp_factory::status_type __KMP_make_rml_server( omp_factory& f, omp_server*& server, omp_client& client ) {
    return ::rml::internal::connect< ::rml::internal::omp_connection_v1>(f,server,client);
}

extern "C" void __KMP_call_with_my_server_info( ::rml::server_info_callback_t cb, void* arg ) {
    return ::rml::internal::call_with_build_date_str( cb, arg );
}

}}

/*
 * RML server info
 */
#include "version_string.tmp"

#ifndef __TBB_VERSION_STRINGS
#pragma message("Warning: version_string.tmp isn't generated properly by version_info.sh script!")
#endif

// We pass the build time as the RML server info.  TBB is required to build RML, so we make it the same as the TBB build time.
#ifndef __TBB_DATETIME
#define __TBB_DATETIME __DATE__ " " __TIME__
#endif
#define RML_SERVER_INFO "Intel(R) RML library built: " __TBB_DATETIME

namespace rml {
namespace internal {
void call_with_build_date_str( ::rml::server_info_callback_t cb, void* arg )
{
    (*cb)( arg, RML_SERVER_INFO );
}
}} // rml::internal 
