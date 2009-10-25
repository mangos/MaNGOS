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

/* This file contains the TBB task scheduler. There are many classes
   lumped together here because very few are exposed to the outside
   world, and by putting them in a single translation unit, the
   compiler's optimizer might be able to do a better job. */

#if USE_PTHREAD

    // Some pthreads documentation says that <pthread.h> must be first header.
    #include <pthread.h>
    #define __TBB_THREAD_ROUTINE 

#elif USE_WINTHREAD

    #include <windows.h>
    #include <process.h>        /* Need _beginthreadex from there */
    #include <malloc.h>         /* Need _alloca from there */
    #define __TBB_THREAD_ROUTINE WINAPI

#else

    #error Must define USE_PTHREAD or USE_WINTHREAD

#endif

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <new>
#include "tbb/tbb_stddef.h"

/* Temporarily change "private" to "public" while including "tbb/task.h".
   This hack allows us to avoid publishing internal types and methods
   in the public header files just for sake of friend declarations. */
#define private public
#include "tbb/task.h"
#if __TBB_EXCEPTIONS
#include "tbb/tbb_exception.h"
#endif /* __TBB_EXCEPTIONS */
#undef private

#include "tbb/task_scheduler_init.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/tbb_machine.h"
#include "tbb/mutex.h"
#include "tbb/atomic.h"
#if __TBB_SCHEDULER_OBSERVER
#include "tbb/task_scheduler_observer.h"
#include "tbb/spin_rw_mutex.h"
#include "tbb/aligned_space.h"
#endif /* __TBB_SCHEDULER_OBSERVER */
#if __TBB_EXCEPTIONS
#include "tbb/spin_mutex.h"
#endif /* __TBB_EXCEPTIONS */

#include "tbb/partitioner.h"

#include "../rml/include/rml_tbb.h"

namespace tbb {
    namespace internal {
        namespace rml {
            tbb_server* make_private_server( tbb_client& client );
        }
    }
}

#if DO_TBB_TRACE
#include <cstdio>
#define TBB_TRACE(x) ((void)std::printf x)
#else
#define TBB_TRACE(x) ((void)(0))
#endif /* DO_TBB_TRACE */

#if TBB_USE_ASSERT
#define COUNT_TASK_NODES 1
#endif /* TBB_USE_ASSERT */

/* If nonzero, then gather statistics */
#ifndef STATISTICS
#define STATISTICS 0
#endif /* STATISTICS */

#if STATISTICS
#define GATHER_STATISTIC(x) (x)
#else
#define GATHER_STATISTIC(x) ((void)0)
#endif /* STATISTICS */

#if __TBB_EXCEPTIONS
// The standard offsetof macro does not work for us since its usage is restricted 
// by POD-types only. Using 0x1000 (not NULL) is necessary to appease GCC.
#define __TBB_offsetof(class_name, member_name) \
    ((ptrdiff_t)&(reinterpret_cast<class_name*>(0x1000)->member_name) - 0x1000)
// Returns address of the object containing a member with the given name and address
#define __TBB_get_object_addr(class_name, member_name, member_addr) \
    reinterpret_cast<class_name*>((char*)member_addr - __TBB_offsetof(class_name, member_name))
#endif /* __TBB_EXCEPTIONS */

// This macro is an attempt to get rid of ugly ifdefs in the shared parts of the code. 
// It drops the second argument depending on whether the controlling macro is defined. 
// The first argument is just a convenience allowing to keep comma before the macro usage.
#if __TBB_EXCEPTIONS
    #define __TBB_CONTEXT_ARG(arg1, context) arg1, context
#else /* !__TBB_EXCEPTIONS */
    #define __TBB_CONTEXT_ARG(arg1, context) arg1
#endif /* !__TBB_EXCEPTIONS */

#if _MSC_VER && !defined(__INTEL_COMPILER)
    // Workaround for overzealous compiler warnings
    // These particular warnings are so ubquitous that no attempt is made to narrow 
    // the scope of the warnings.
    #pragma warning (disable: 4100 4127 4312 4244 4267 4706)
#endif

// internal headers
#include "tbb_misc.h"
#include "itt_notify.h"
#include "tls.h"

namespace tbb {

using namespace std;

#if DO_ITT_NOTIFY
    const tchar 
            *SyncType_GlobalLock = _T("TbbGlobalLock"),
            *SyncType_Scheduler = _T("%Constant")
            ;
    const tchar 
            *SyncObj_SchedulerInitialization = _T("TbbSchedulerInitialization"),
            *SyncObj_SchedulersList = _T("TbbSchedulersList"),
            *SyncObj_WorkerLifeCycleMgmt = _T("TBB Scheduler"),
            *SyncObj_TaskStealingLoop = _T("TBB Scheduler"),
            *SyncObj_WorkerTaskPool = _T("TBB Scheduler"),
            *SyncObj_MasterTaskPool = _T("TBB Scheduler"),
            *SyncObj_TaskPoolSpinning = _T("TBB Scheduler"),
            *SyncObj_Mailbox = _T("TBB Scheduler"),
            *SyncObj_TaskReturnList = _T("TBB Scheduler"),
            *SyncObj_GateLock = _T("TBB Scheduler"),
            *SyncObj_Gate = _T("TBB Scheduler"),
            *SyncObj_ContextsList = _T("TBB Scheduler")
            ;
#endif /* DO_ITT_NOTIFY */

namespace internal {

const stack_size_type MByte = 1<<20;
#if !defined(__TBB_WORDSIZE)
const stack_size_type ThreadStackSize = 1*MByte;
#elif __TBB_WORDSIZE<=4
const stack_size_type ThreadStackSize = 2*MByte;
#else
const stack_size_type ThreadStackSize = 4*MByte;
#endif

#if USE_PTHREAD
typedef void* thread_routine_return_type;
#else
typedef unsigned thread_routine_return_type;
#endif

//------------------------------------------------------------------------
// General utility section
//------------------------------------------------------------------------

#if TBB_USE_ASSERT
    #define __TBB_POISON_DEQUE 1
#endif /* TBB_USE_ASSERT */

#if __TBB_POISON_DEQUE
    #if __ia64__
        task* const poisoned_taskptr = (task*)0xDDEEAADDDEADBEEF;
    #elif _WIN64
        task* const poisoned_taskptr = (task*)0xDDEEAADDDEADBEEF;
    #else
        task* const poisoned_taskptr = (task*)0xDEADBEEF;
    #endif

    #define __TBB_POISON_TASK_PTR(ptr) ptr = poisoned_taskptr
    #define __TBB_ASSERT_VALID_TASK_PTR(ptr) __TBB_ASSERT( ptr != poisoned_taskptr, "task pointer in the deque is poisoned" )
#else /* !__TBB_POISON_DEQUE */
    #define __TBB_POISON_TASK_PTR(ptr) ((void)0)
    #define __TBB_ASSERT_VALID_TASK_PTR(ptr) ((void)0)
#endif /* !__TBB_POISON_DEQUE */


//! Vector that grows without reallocations, and stores items in the reverse order.
/** Requires to initialize its first segment with a preallocated memory chunk
    (usually it is static array or an array allocated on the stack).
    The second template parameter specifies maximal number of segments. Each next 
    segment is twice as large as the previous one. **/
template<typename T, size_t max_segments = 16>
class fast_reverse_vector
{
public:
    fast_reverse_vector ( T* initial_segment, size_t segment_size )
        : m_cur_segment(initial_segment)
        , m_cur_segment_size(segment_size)
        , m_pos(segment_size)
        , m_num_segments(0)
        , m_size(0)
    {
        __TBB_ASSERT ( initial_segment && segment_size, "Nonempty initial segment must be supplied");
    }

    ~fast_reverse_vector ()
    {
        for ( size_t i = 1; i < m_num_segments; ++i )
            NFS_Free( m_segments[i] );
    }

    size_t size () const { return m_size + m_cur_segment_size - m_pos; }

    void push_back ( const T& val )
    {
        if ( !m_pos ) {
            m_segments[m_num_segments++] = m_cur_segment;
            __TBB_ASSERT ( m_num_segments < max_segments, "Maximal capacity exceeded" );
            m_size += m_cur_segment_size;
            m_cur_segment_size *= 2;
            m_pos = m_cur_segment_size;
            m_cur_segment = (T*)NFS_Allocate( m_cur_segment_size * sizeof(T), 1, NULL );
        }
        m_cur_segment[--m_pos] = val;
    }

    //! Copies the contents of the vector into the dst array. 
    /** Can only be used when T is a POD type, as copying does not invoke copy constructors. **/
    void copy_memory ( T* dst ) const
    {
        size_t size = m_cur_segment_size - m_pos;
        memcpy( dst, m_cur_segment + m_pos, size * sizeof(T) );
        dst += size;
        size = m_cur_segment_size / 2;
        for ( long i = (long)m_num_segments - 1; i >= 0; --i ) {
            memcpy( dst, m_segments[i], size * sizeof(T) );
            dst += size;
            size /= 2;
        }
    }

protected:
    //! The current (not completely filled) segment
    T       *m_cur_segment;

    //! Capacity of m_cur_segment
    size_t  m_cur_segment_size;

    //! Insertion position in m_cur_segment
    size_t  m_pos;

    //! Array of filled segments (has fixed size specified by the second template parameter)
    T       *m_segments[max_segments];
    
    //! Number of filled segments (the size of m_segments)
    size_t  m_num_segments;

    //! Number of items in the segments in m_segments
    size_t  m_size;

}; // class fast_reverse_vector

//------------------------------------------------------------------------
// End of general utility section
//------------------------------------------------------------------------

//! Alignment for a task object
const size_t task_alignment = 16;

//! Number of bytes reserved for a task prefix
/** If not exactly sizeof(task_prefix), the extra bytes *precede* the task_prefix. */
const size_t task_prefix_reservation_size = ((sizeof(internal::task_prefix)-1)/task_alignment+1)*task_alignment;

template<typename SchedulerTraits> class CustomScheduler;


class mail_outbox;

struct task_proxy: public task {
    static const intptr pool_bit = 1;
    static const intptr mailbox_bit = 2;
    /* All but two low-order bits represent a (task*).
       Two low-order bits mean:
       1 = proxy is/was/will be in task pool
       2 = proxy is/was/will be in mailbox */
    intptr task_and_tag;

    //! Pointer to next task_proxy in a mailbox
    task_proxy* next_in_mailbox;

    //! Mailbox to which this was mailed.
    mail_outbox* outbox;
};

//! Internal representation of mail_outbox, without padding.
class unpadded_mail_outbox {
protected:
    //! Pointer to first task_proxy in mailbox, or NULL if box is empty. 
    task_proxy* my_first;

    //! Pointer to last task_proxy in mailbox, or NULL if box is empty. 
    /** Low-order bit set to 1 to represent lock on the box. */
    task_proxy* my_last;

    //! Owner of mailbox is not executing a task, and has drained its own task pool.
    bool my_is_idle;
};

//! Class representing where mail is put.
/** Padded to occupy a cache line. */
class mail_outbox: unpadded_mail_outbox {
    char pad[NFS_MaxLineSize-sizeof(unpadded_mail_outbox)];

    //! Acquire lock on the box.
    task_proxy* acquire() {
        atomic_backoff backoff;
        for(;;) {
            // No fence on load, because subsequent compare-and-swap has the necessary fence.
            intptr last = (intptr)my_last;
            if( (last&1)==0 && __TBB_CompareAndSwapW(&my_last,last|1,last)==last) {
                __TBB_ASSERT( (my_first==NULL)==((intptr(my_last)&~1)==0), NULL );
                return (task_proxy*)last;
            }
            backoff.pause();
        }
    }
    task_proxy* internal_pop() {
        //! No fence on load of my_first, because if it is NULL, there's nothing further to read from another thread.
        task_proxy* result = my_first;
        if( result ) {
            if( task_proxy* f = __TBB_load_with_acquire(result->next_in_mailbox) ) {
                // No lock required
                __TBB_store_with_release( my_first, f );
            } else {
                // acquire() has the necessary fence.
                task_proxy* l = acquire();
                __TBB_ASSERT(result==my_first,NULL); 
                if( !(my_first = result->next_in_mailbox) ) 
                    l=0;
                __TBB_store_with_release( my_last, l );
            }
        }
        return result;
    }
public:
    friend class mail_inbox;

    //! Push task_proxy onto the mailbox queue of another thread.
    void push( task_proxy& t ) {
        __TBB_ASSERT(&t!=NULL, NULL);
        t.next_in_mailbox = NULL; 
        if( task_proxy* l = acquire() ) {
            l->next_in_mailbox = &t;
        } else {
            my_first=&t;
        }
        // Fence required because caller is sending the task_proxy to another thread.
        __TBB_store_with_release( my_last, &t );
    }
#if TBB_USE_ASSERT
    //! Verify that *this is initialized empty mailbox.
    /** Raise assertion if *this is not in initialized state, or sizeof(this) is wrong.
        Instead of providing a constructor, we provide this assertion, because for
        brevity and speed, we depend upon a memset to initialize instances of this class */
    void assert_is_initialized() const {
        __TBB_ASSERT( sizeof(*this)==NFS_MaxLineSize, NULL );
        __TBB_ASSERT( !my_first, NULL );
        __TBB_ASSERT( !my_last, NULL );
        __TBB_ASSERT( !my_is_idle, NULL );
    }
#endif /* TBB_USE_ASSERT */

    //! Drain the mailbox 
    intptr drain() {
        intptr k = 0;
        // No fences here because other threads have already quit.
        for( ; task_proxy* t = my_first; ++k ) {
            my_first = t->next_in_mailbox;
            NFS_Free((char*)t-task_prefix_reservation_size);
        }
        return k;  
    }

    //! True if thread that owns this mailbox is looking for work.
    bool recipient_is_idle() {
        return my_is_idle;
    }
};

//! Class representing source of mail.
class mail_inbox {
    //! Corresponding sink where mail that we receive will be put.
    mail_outbox* my_putter;
public:
    //! Construct unattached inbox
    mail_inbox() : my_putter(NULL) {}

    //! Attach inbox to a corresponding outbox. 
    void attach( mail_outbox& putter ) {
        __TBB_ASSERT(!my_putter,"already attached");
        my_putter = &putter;
    }
    //! Detach inbox from its outbox
    void detach() {
        __TBB_ASSERT(my_putter,"not attached");
        my_putter = NULL;
    }
    //! Get next piece of mail, or NULL if mailbox is empty.
    task_proxy* pop() {
        return my_putter->internal_pop();
    }
    //! Indicate whether thread that reads this mailbox is idle.
    /** Raises assertion failure if mailbox is redundantly marked as not idle. */
    void set_is_idle( bool value ) {
        if( my_putter ) {
            __TBB_ASSERT( my_putter->my_is_idle || value, "attempt to redundantly mark mailbox as not idle" );
            my_putter->my_is_idle = value;
        }
    }
#if TBB_USE_ASSERT
    //! Indicate whether thread that reads this mailbox is idle.
    bool assert_is_idle( bool value ) const {
        __TBB_ASSERT( !my_putter || my_putter->my_is_idle==value, NULL );
        return true;
    }
#endif /* TBB_USE_ASSERT */
#if DO_ITT_NOTIFY
    //! Get pointer to corresponding outbox used for ITT_NOTIFY calls.
    void* outbox() const {return my_putter;}
#endif /* DO_ITT_NOTIFY */ 
};

#if __TBB_SCHEDULER_OBSERVER
//------------------------------------------------------------------------
// observer_proxy
//------------------------------------------------------------------------
class observer_proxy {
    friend class task_scheduler_observer_v3;
    //! Reference count used for garbage collection.
    /** 1 for reference from my task_scheduler_observer.
        1 for each local_last_observer_proxy that points to me. 
        No accounting for predecessor in the global list. 
        No accounting for global_last_observer_proxy that points to me. */
    atomic<int> gc_ref_count;
    //! Pointer to next task_scheduler_observer 
    /** Valid even when *this has been removed from the global list. */
    observer_proxy* next; 
    //! Pointer to previous task_scheduler_observer in global list.
    observer_proxy* prev; 
    //! Associated observer
    task_scheduler_observer* observer;
    //! Account for removing reference from p.  No effect if p is NULL.
    void remove_ref_slow();
    void remove_from_list(); 
    observer_proxy( task_scheduler_observer_v3& wo ); 
public:
    static observer_proxy* process_list( observer_proxy* local_last, bool is_worker, bool is_entry );
};
#endif /* __TBB_SCHEDULER_OBSERVER */


//------------------------------------------------------------------------
// Arena
//------------------------------------------------------------------------

class Arena;
class GenericScheduler;

struct WorkerDescriptor {
    //! NULL until worker is published.  -1 if worker should not be published.
    GenericScheduler* scheduler;

};

//! The useful contents of an ArenaPrefix
class UnpaddedArenaPrefix: no_copy 
   ,rml::tbb_client
{
    friend class GenericScheduler;
    template<typename SchedulerTraits> friend class internal::CustomScheduler;
    friend class Arena;
    friend class Governor;
    friend struct WorkerDescriptor;

    //! Arena slot to try to acquire first for the next new master.
    unsigned limit;

    //! Number of masters that own this arena.
    /** This may be smaller than the number of masters who have entered the arena. */
    unsigned number_of_masters;

    //! Total number of slots in the arena
    const unsigned number_of_slots;

    //! Number of workers that belong to this arena
    const unsigned number_of_workers;

    //! Pointer to the RML server object that services requests for this arena.
    rml::tbb_server* server;
    //! Counter used to allocate job indices
    tbb::atomic<size_t> next_job_index;

    //! Stack size of worker threads
    stack_size_type stack_size;

    //! Array of workers.
    WorkerDescriptor* worker_list;

#if COUNT_TASK_NODES
    //! Net number of nodes that have been allocated from heap.
    /** Updated each time a scheduler is destroyed. */
    atomic<intptr> task_node_count;
#endif /* COUNT_TASK_NODES */

    //! Estimate of number of available tasks.  
    /** The estimate is either 0 (SNAPSHOT_EMPTY), infinity (SNAPSHOT_FULL), or a special value. 
        The implementation of Arena::is_busy_or_empty requires that pool_state_t be unsigned. */
    typedef uintptr_t pool_state_t;

    //! Current estimate of number of available tasks.  
    tbb::atomic<pool_state_t> pool_state;
 
protected:
    UnpaddedArenaPrefix( unsigned number_of_slots_, unsigned number_of_workers_ ) :
        number_of_masters(1),
        number_of_slots(number_of_slots_),
        number_of_workers(number_of_workers_)
    {
#if COUNT_TASK_NODES
        task_node_count = 0;
#endif /* COUNT_TASK_NODES */
        limit = number_of_workers_;
        server = NULL;
        stack_size = 0;
        next_job_index = 0;
    }
    void open_connection_to_rml();

private:
    //! Return reference to corresponding arena.
    Arena& arena();

    /*override*/ version_type version() const {
        return 0;
    }

    /*override*/ unsigned max_job_count() const {
        return number_of_workers;
    }

    /*override*/ size_t min_stack_size() const {
        return stack_size;
    }

    /*override*/ policy_type policy() const {
        return throughput;
    }

    /*override*/ job* create_one_job();

    /*override*/ void cleanup( job& j );

    /*override*/ void acknowledge_close_connection();

    /*override*/ void process( job& j );
};

//! The prefix to Arena with padding.
class ArenaPrefix: public UnpaddedArenaPrefix {
    //! Padding to fill out to multiple of cache line size.
    char pad[(sizeof(UnpaddedArenaPrefix)/NFS_MaxLineSize+1)*NFS_MaxLineSize-sizeof(UnpaddedArenaPrefix)];

public:
    ArenaPrefix( unsigned number_of_slots_, unsigned number_of_workers_ ) :
        UnpaddedArenaPrefix(number_of_slots_,number_of_workers_)
    {
    }
};


struct ArenaSlot {
    // Task pool (the deque of task pointers) of the scheduler that owns this slot
    /** Also is used to specify if the slot is empty or locked:
         0 - empty
        -1 - locked **/
    task** task_pool;

    //! Index of the first ready task in the deque.
    /** Modified by thieves, and by the owner during compaction/reallocation **/
    size_t head;

    //! Padding to avoid false sharing caused by the thieves accessing this slot
    char pad1[NFS_MaxLineSize - sizeof(size_t) - sizeof(task**)];

    //! Index of the element following the last ready task in the deque.
    /** Modified by the owner thread. **/
    size_t tail;

    //! Padding to avoid false sharing caused by the thieves accessing the next slot
    char pad2[NFS_MaxLineSize - sizeof(size_t)];
};


class Arena {
    friend class UnpaddedArenaPrefix;
    friend class GenericScheduler;
    template<typename SchedulerTraits> friend class internal::CustomScheduler;
    friend class Governor;
    friend struct WorkerDescriptor;

    //! Get reference to prefix portion
    ArenaPrefix& prefix() const {return ((ArenaPrefix*)(void*)this)[-1];}

    //! Get reference to mailbox corresponding to given affinity_id.
    mail_outbox& mailbox( affinity_id id ) {
        __TBB_ASSERT( 0<id, "id must be positive integer" );
        __TBB_ASSERT( id <= prefix().number_of_slots, "id out of bounds" );
        return ((mail_outbox*)&prefix())[-(int)id];
    }

    //! Allocate an instance of Arena, and prepare everything to start workers.
    static Arena* allocate_arena( unsigned number_of_slots, unsigned number_of_workers, stack_size_type stack_size );

    void free_arena() {
        // Drain mailboxes
        // TODO: each scheduler should plug-and-drain its own mailbox when it terminates.
        intptr drain_count = 0;
        for( unsigned i=1; i<=prefix().number_of_slots; ++i )
            drain_count += mailbox(i).drain();
#if COUNT_TASK_NODES
        prefix().task_node_count -= drain_count;
#if !TEST_ASSEMBLY_ROUTINES
        if( prefix().task_node_count ) {
            fprintf(stderr,"warning: leaked %ld task objects\n", long(prefix().task_node_count));
        }
#endif /* !TEST_ASSEMBLY_ROUTINES */
#endif /* COUNT_TASK_NODES */
        void* storage  = &mailbox(prefix().number_of_slots);
        delete[] prefix().worker_list;
        prefix().~ArenaPrefix();
        NFS_Free( storage );
    }

    typedef ArenaPrefix::pool_state_t pool_state_t;

    //! No tasks to steal since last snapshot was taken
    static const pool_state_t SNAPSHOT_EMPTY = 0;

    //! At least one task has been offered for stealing since the last snapshot started
    static const pool_state_t SNAPSHOT_FULL = pool_state_t(-1);

    //! Server is going away and hence further calls to adjust_job_count_estimate are unsafe.
    static const pool_state_t SNAPSHOT_SERVER_GOING_AWAY = pool_state_t(-2);

    //! No tasks to steal or snapshot is being taken.
    static bool is_busy_or_empty( pool_state_t s ) {return s<SNAPSHOT_SERVER_GOING_AWAY;}

    //! If necessary, inform gate that task was added to pool recently.
    void mark_pool_full();

    //! Check if pool is empty.
    /** Return true if pool is empty or being cleaned up. */
    bool check_if_pool_is_empty();

    //! Terminate worker threads
    /** Wait for worker threads to complete. */
    void terminate_workers();


#if COUNT_TASK_NODES
    //! Returns the number of task objects "living" in worker threads
    inline intptr workers_task_node_count();
#endif

    /** Must be last field */
    ArenaSlot slot[1];
};

//------------------------------------------------------------------------
//! Traits classes for scheduler
//------------------------------------------------------------------------

struct DefaultSchedulerTraits {
    static const int id = 0;
    static const bool itt_possible = true;
    static const bool has_slow_atomic = false;
};

struct IntelSchedulerTraits {
    static const int id = 1;
    static const bool itt_possible = false;
#if __TBB_x86_32||__TBB_x86_64
    static const bool has_slow_atomic = true;
#else
    static const bool has_slow_atomic = false;
#endif /* __TBB_x86_32||__TBB_x86_64 */
};

//------------------------------------------------------------------------
// Class __TBB_InitOnce
//------------------------------------------------------------------------

//! Class that supports TBB initialization. 
/** It handles acquisition and release of global resources (e.g. TLS) during startup and shutdown,
    as well as synchronization for DoOneTimeInitializations. */
class __TBB_InitOnce {
    friend void DoOneTimeInitializations();
    friend void ITT_DoUnsafeOneTimeInitialization ();

    static atomic<int> count;

    //! Platform specific code to acquire resources.
    static void acquire_resources();

    //! Platform specific code to release resources.
    static void release_resources();

    static bool InitializationDone;

    // Scenarios are possible when tools interop has to be initialized before the
    // TBB itself. This imposes a requirement that the global initialization lock 
    // has to support valid static initialization, and does not issue any tool
    // notifications in any build mode.
    typedef unsigned char mutex_type;

    // Global initialization lock
    static mutex_type InitializationLock;

public:
    static void lock()   { __TBB_LockByte( InitializationLock ); }

    static void unlock() { __TBB_store_with_release( InitializationLock, 0 ); }

    static bool initialization_done() { return __TBB_load_with_acquire(InitializationDone); }

    //! Add initial reference to resources. 
    /** We assume that dynamic loading of the library prevents any other threads from entering the library
        until this constructor has finished running. */
    __TBB_InitOnce() { add_ref(); }

    //! Remove the initial reference to resources.
    /** This is not necessarily the last reference if other threads are still running.
        If the extra reference from DoOneTimeInitializations is present, remove it as well.*/
    ~__TBB_InitOnce();

    //! Add reference to resources.  If first reference added, acquire the resources.
    static void add_ref() {
        if( ++count==1 )
            acquire_resources();
    }
    //! Remove reference to resources.  If last reference removed, release the resources.
    static void remove_ref() {
        int k = --count;
        __TBB_ASSERT(k>=0,"removed __TBB_InitOnce ref that was not added?"); 
        if( k==0 ) 
            release_resources();
    }
}; // class __TBB_InitOnce

//------------------------------------------------------------------------
// Class Governor
//------------------------------------------------------------------------

//! The class handles access to the single instance of Arena, and to TLS to keep scheduler instances.
/** It also supports automatic on-demand intialization of the TBB scheduler.
    The class contains only static data members and methods.*/
class Governor {
    friend class __TBB_InitOnce;
    friend void ITT_DoUnsafeOneTimeInitialization ();

    static basic_tls<GenericScheduler*> theTLS;
    static Arena* theArena;
    static mutex  theArenaMutex;

    //! Create key for thread-local storage.
    static void create_tls() {
#if USE_PTHREAD
        int status = theTLS.create(auto_terminate);
#else
        int status = theTLS.create();
#endif
        if( status )
            handle_perror(status, "TBB failed to initialize TLS storage\n");
    }

    //! Destroy the thread-local storage key.
    static void destroy_tls() {
#if TBB_USE_ASSERT
        if( __TBB_InitOnce::initialization_done() && theTLS.get() ) 
            fprintf(stderr, "TBB is unloaded while tbb::task_scheduler_init object is alive?");
#endif
        int status = theTLS.destroy();
        if( status )
            handle_perror(status, "TBB failed to destroy TLS storage");
    }
    
    //! Obtain the instance of arena to register a new master thread
    /** If there is no active arena, create one. */
    static Arena* obtain_arena( int number_of_threads, stack_size_type thread_stack_size )
    {
        mutex::scoped_lock lock( theArenaMutex );
        Arena* a = theArena;
        if( a ) {
            a->prefix().number_of_masters += 1;
        } else {
            if( number_of_threads==task_scheduler_init::automatic )
                number_of_threads = task_scheduler_init::default_num_threads();
            a = Arena::allocate_arena( 2*number_of_threads, number_of_threads-1,
                                       thread_stack_size?thread_stack_size:ThreadStackSize );
            __TBB_ASSERT( a->prefix().number_of_masters==1, NULL );
            // Publish the Arena.  
            // A memory release fence is not required here, because workers have not started yet,
            // and concurrent masters inspect theArena while holding theArenaMutex.
            __TBB_ASSERT( !theArena, NULL );
            theArena = a;
            // Must create server under lock, otherwise second master might see arena without a server.
            a->prefix().open_connection_to_rml();
        }
        return a;
    }

    //! The internal routine to undo automatic initialization.
    /** The signature is written with void* so that the routine
        can be the destructor argument to pthread_key_create. */
    static void auto_terminate(void* scheduler);

public:
    //! Processes scheduler initialization request (possibly nested) in a master thread
    /** If necessary creates new instance of arena and/or local scheduler.
        The auto_init argument specifies if the call is due to automatic initialization. **/
    static GenericScheduler* init_scheduler( int num_threads, stack_size_type stack_size, bool auto_init = false );

    //! Processes scheduler termination request (possibly nested) in a master thread
    static void terminate_scheduler( GenericScheduler* s );

    //! Dereference arena when a master thread stops using TBB.
    /** If no more masters in the arena, terminate workers and destroy it. */
    static void finish_with_arena() {
        mutex::scoped_lock lock( theArenaMutex );
        Arena* a = theArena;
        __TBB_ASSERT( a, "theArena is missing" );
        if( --(a->prefix().number_of_masters) )
            a = NULL;
        else {
            theArena = NULL;
            // Must do this while holding lock, otherwise terminate message might reach
            // RML thread *after* initialize message reaches it for the next arena, which
            // which causes TLS to be set to new value before old one is erased!
            a->terminate_workers();
        }
    }

    static size_t number_of_workers_in_arena() {
        __TBB_ASSERT( theArena, "thread did not activate a task_scheduler_init object?" );
        // No fence required to read theArena, because it does not change after the thread starts.
        return theArena->prefix().number_of_workers;
    }

    //! Register TBB scheduler instance in thread local storage.
    inline static void sign_on(GenericScheduler* s);

    //! Unregister TBB scheduler instance from thread local storage.
    inline static void sign_off(GenericScheduler* s);

    //! Used to check validity of the local scheduler TLS contents.
    static bool is_set ( GenericScheduler* s ) { return theTLS.get() == s; }

    //! Obtain the thread local instance of the TBB scheduler.
    /** If the scheduler has not been initialized yet, initialization is done automatically.
        Note that auto-initialized scheduler instance is destroyed only when its thread terminates. **/
    static GenericScheduler* local_scheduler () {
        GenericScheduler* s = theTLS.get();
        return s ? s : init_scheduler( task_scheduler_init::automatic, 0, true );
    }

    //! Undo automatic initialization if necessary; call when a thread exits.
    static void terminate_auto_initialized_scheduler() {
        auto_terminate( theTLS.get() );
    }
}; // class Governor

//------------------------------------------------------------------------
// Begin shared data layout.
//
// The following global data items are read-only after initialization.
// The first item is aligned on a 128 byte boundary so that it starts a new cache line.
//------------------------------------------------------------------------

basic_tls<GenericScheduler*> Governor::theTLS;
Arena * Governor::theArena;
mutex   Governor::theArenaMutex;

//! Number of hardware threads
/** One more than the default number of workers. */
static int DefaultNumberOfThreads;

//! T::id for the scheduler traits type T to use for the scheduler
/** For example, the default value is DefaultSchedulerTraits::id. */
static int SchedulerTraitsId;

//! Counter of references to global shared resources such as TLS.
atomic<int> __TBB_InitOnce::count;

__TBB_InitOnce::mutex_type __TBB_InitOnce::InitializationLock;

//! Flag that is set to true after one-time initializations are done.
bool __TBB_InitOnce::InitializationDone;

#if DO_ITT_NOTIFY
    static bool ITT_Present;
    static bool ITT_InitializationDone;
#endif

static rml::tbb_factory rml_server_factory;
//! Set to true if private statically linked RML server should be used instead of shared server.
static bool use_private_rml;

#if !(_WIN32||_WIN64) || __TBB_TASK_CPP_DIRECTLY_INCLUDED
    static __TBB_InitOnce __TBB_InitOnceHiddenInstance;
#endif

#if __TBB_SCHEDULER_OBSERVER
typedef spin_rw_mutex::scoped_lock task_scheduler_observer_mutex_scoped_lock;
/** aligned_space used here to shut up warnings when mutex destructor is called while threads are still using it. */
static aligned_space<spin_rw_mutex,1> the_task_scheduler_observer_mutex;
static observer_proxy* global_first_observer_proxy;
static observer_proxy* global_last_observer_proxy;
#endif /* __TBB_SCHEDULER_OBSERVER */

//! Table of primes used by fast random-number generator.
/** Also serves to keep anything else from being placed in the same
    cache line as the global data items preceding it. */
static const unsigned Primes[] = {
    0x9e3779b1, 0xffe6cc59, 0x2109f6dd, 0x43977ab5,
    0xba5703f5, 0xb495a877, 0xe1626741, 0x79695e6b,
    0xbc98c09f, 0xd5bee2b3, 0x287488f9, 0x3af18231,
    0x9677cd4d, 0xbe3a6929, 0xadc6a877, 0xdcf0674b,
    0xbe4d6fe9, 0x5f15e201, 0x99afc3fd, 0xf3f16801,
    0xe222cfff, 0x24ba5fdb, 0x0620452d, 0x79f149e3,
    0xc8b93f49, 0x972702cd, 0xb07dd827, 0x6c97d5ed,
    0x085a3d61, 0x46eb5ea7, 0x3d9910ed, 0x2e687b5b,
    0x29609227, 0x6eb081f1, 0x0954c4e1, 0x9d114db9,
    0x542acfa9, 0xb3e6bd7b, 0x0742d917, 0xe9f3ffa7,
    0x54581edb, 0xf2480f45, 0x0bb9288f, 0xef1affc7,
    0x85fa0ca7, 0x3ccc14db, 0xe6baf34b, 0x343377f7,
    0x5ca19031, 0xe6d9293b, 0xf0a9f391, 0x5d2e980b,
    0xfc411073, 0xc3749363, 0xb892d829, 0x3549366b,
    0x629750ad, 0xb98294e5, 0x892d9483, 0xc235baf3,
    0x3d2402a3, 0x6bdef3c9, 0xbec333cd, 0x40c9520f
};

#if STATISTICS
//! Class for collecting statistics
/** There should be only one instance of this class. 
    Results are written to a file "statistics.txt" in tab-separated format. */
static class statistics {
public:
    statistics() {
        my_file = fopen("statistics.txt","w");
        if( !my_file ) {
            perror("fopen(\"statistics.txt\"\")");
            exit(1);
        }
        fprintf(my_file,"%13s\t%13s\t%13s\t%13s\t%13s\t%13s\n", "execute", "steal", "mail", "proxy_execute", "proxy_steal", "proxy_bypass" );
    }
    ~statistics() {
        fclose(my_file);
    }
    void record( long execute_count, long steal_count, long mail_received_count, 
                 long proxy_execute_count, long proxy_steal_count, long proxy_bypass_count ) {
        mutex::scoped_lock lock(my_mutex);
        fprintf (my_file,"%13ld\t%13ld\t%13ld\t%13ld\t%13ld\t%13ld\n", execute_count, steal_count, mail_received_count, 
                                                           proxy_execute_count, proxy_steal_count, proxy_bypass_count );
    }
private:
    //! File into which statistics are written.
    FILE* my_file;
    //! Mutex that serializes accesses to my_file
    mutex my_mutex;
} the_statistics;
#endif /* STATISTICS */

#if __TBB_EXCEPTIONS
    struct scheduler_list_node_t {
        scheduler_list_node_t *my_prev,
                              *my_next;
    };

    //! Head of the list of master thread schedulers.
    static scheduler_list_node_t the_scheduler_list_head;

    //! Mutex protecting access to the list of schedulers.
    static mutex the_scheduler_list_mutex;

//! Counter that is incremented whenever new cancellation signal is sent to a task group.
/** Together with GenericScheduler::local_cancel_count forms cross-thread signaling
    mechanism that allows to avoid locking at the hot path of normal execution flow.

    When a descendant task group context is being registered or unregistered,
    the global and local counters are compared. If they differ, it means that 
    a cancellation signal is being propagated, and registration/deregistration
    routines take slower branch that may block (at most one thread of the pool
    can be blocked at any moment). Otherwise the control path is lock-free and fast. **/
    static uintptr_t global_cancel_count = 0;

    //! Context to be associated with dummy tasks of worker threads schedulers.
    /** It is never used for its direct purpose, and is introduced solely for the sake 
        of avoiding one extra conditional branch in the end of wait_for_all method. **/
    static task_group_context dummy_context(task_group_context::isolated);
#endif /* __TBB_EXCEPTIONS */

//------------------------------------------------------------------------
// End of shared data layout
//------------------------------------------------------------------------

//! Amount of time to pause between steals.
/** The default values below were found to be best empirically for K-Means
    on the 32-way Altix and 4-way (*2 for HT) fxqlin04. */
#if __TBB_ipf
static const long PauseTime = 1500;
#else 
static const long PauseTime = 80;
#endif

//------------------------------------------------------------------------
// One-time Initializations
//------------------------------------------------------------------------

//! Defined in cache_aligned_allocator.cpp
extern void initialize_cache_aligned_allocator();

#if DO_ITT_NOTIFY
//! Performs initialization of tools support.
/** Defined in itt_notify.cpp. Must be called in a protected do-once manner.
    \return true if notification hooks were installed, false otherwise. **/
bool InitializeITT();

/** Thread-unsafe lazy one-time initialization of tools interop.
    Used by both dummy handlers and general TBB one-time initialization routine. **/
void ITT_DoUnsafeOneTimeInitialization () {
    if ( !ITT_InitializationDone ) {
        ITT_Present = InitializeITT();
        ITT_InitializationDone = true;
        ITT_SYNC_CREATE(&Governor::theArenaMutex, SyncType_GlobalLock, SyncObj_SchedulerInitialization);
    }
}

/** Thread-safe lazy one-time initialization of tools interop.
    Used by dummy handlers only. **/
extern "C"
void ITT_DoOneTimeInitialization() {
    __TBB_InitOnce::lock();
    ITT_DoUnsafeOneTimeInitialization();
    __TBB_InitOnce::unlock();
}
#endif /* DO_ITT_NOTIFY */

//! Performs thread-safe lazy one-time general TBB initialization.
void DoOneTimeInitializations() {
    __TBB_InitOnce::lock();
    // No fence required for load of InitializationDone, because we are inside a critical section.
    if( !__TBB_InitOnce::InitializationDone ) {
        __TBB_InitOnce::add_ref();
        if( GetBoolEnvironmentVariable("TBB_VERSION") )
            PrintVersion();
        bool have_itt = false;
#if DO_ITT_NOTIFY
        ITT_DoUnsafeOneTimeInitialization();
        have_itt = ITT_Present;
#endif /* DO_ITT_NOTIFY */
        initialize_cache_aligned_allocator();
        ::rml::factory::status_type status = rml_server_factory.open(); 
        if( status!=::rml::factory::st_success ) {
            use_private_rml = true;
            PrintExtraVersionInfo( "RML", "private" );
        } else {
            PrintExtraVersionInfo( "RML", "shared" );
            rml_server_factory.call_with_server_info( PrintRMLVersionInfo, (void*)"" );
        }
        if( !have_itt )
            SchedulerTraitsId = IntelSchedulerTraits::id;
#if __TBB_EXCEPTIONS
        else {
            ITT_SYNC_CREATE(&the_scheduler_list_mutex, SyncType_GlobalLock, SyncObj_SchedulersList);
        }
#endif /* __TBB_EXCEPTIONS */
        PrintExtraVersionInfo( "SCHEDULER",
                               SchedulerTraitsId==IntelSchedulerTraits::id ? "Intel" : "default" );
#if __TBB_EXCEPTIONS
        the_scheduler_list_head.my_next = &the_scheduler_list_head;
        the_scheduler_list_head.my_prev = &the_scheduler_list_head;
#endif /* __TBB_EXCEPTIONS */
        __TBB_InitOnce::InitializationDone = true;
    }
    __TBB_InitOnce::unlock();
}

//------------------------------------------------------------------------
// Methods of class __TBB_InitOnce
//------------------------------------------------------------------------

__TBB_InitOnce::~__TBB_InitOnce() { 
    remove_ref();
    // It is assumed that InitializationDone is not set after file-scope destructors start running,
    // and thus no race on InitializationDone is possible.
    if( initialization_done() ) {
        // Remove reference that we added in DoOneTimeInitializations.
        remove_ref();  
    }
} 

void __TBB_InitOnce::acquire_resources() {
    Governor::create_tls();
}

void __TBB_InitOnce::release_resources() {
    rml_server_factory.close();
    Governor::destroy_tls();
}

#if (_WIN32||_WIN64) && !__TBB_TASK_CPP_DIRECTLY_INCLUDED
//! Windows "DllMain" that handles startup and shutdown of dynamic library.
extern "C" bool WINAPI DllMain( HANDLE /*hinstDLL*/, DWORD reason, LPVOID /*lpvReserved*/ ) {
    switch( reason ) {
        case DLL_PROCESS_ATTACH:
            __TBB_InitOnce::add_ref();
            break;
        case DLL_PROCESS_DETACH:
            __TBB_InitOnce::remove_ref();
            // It is assumed that InitializationDone is not set after DLL_PROCESS_DETACH,
            // and thus no race on InitializationDone is possible.
            if( __TBB_InitOnce::initialization_done() ) {
                // Remove reference that we added in DoOneTimeInitializations.
                __TBB_InitOnce::remove_ref();
            }
            break;
        case DLL_THREAD_DETACH:
            Governor::terminate_auto_initialized_scheduler();
            break;
    }
    return true;
}
#endif /* (_WIN32||_WIN64) && !__TBB_TASK_CPP_DIRECTLY_INCLUDED */

//------------------------------------------------------------------------
// FastRandom
//------------------------------------------------------------------------

//! A fast random number generator.
/** Uses linear congruential method. */
class FastRandom {
    unsigned x, a;
public:
    //! Get a random number.
    unsigned short get() {
        unsigned short r = x>>16;
        x = x*a+1;
        return r;
    }
    //! Construct a random number generator.
    FastRandom( unsigned seed ) {
        x = seed;
        a = Primes[seed%(sizeof(Primes)/sizeof(Primes[0]))];
    }
};

//------------------------------------------------------------------------
// GenericScheduler
//------------------------------------------------------------------------

//  A pure virtual destructor should still have a body
//  so the one for tbb::internal::scheduler::~scheduler() is provided here
scheduler::~scheduler( ) {}

    #define EmptyTaskPool ((task**)0u)
    #define LockedTaskPool ((task**)~0u)

    #define LocalSpawn local_spawn

//! Cilk-style task scheduler.
/** None of the fields here are every read or written by threads other than
    the thread that creates the instance.

    Class GenericScheduler is an abstract base class that contains most of the scheduler,
    except for tweaks specific to processors and tools (e.g. VTune).
    The derived template class CustomScheduler<SchedulerTraits> fills in the tweaks. */
class GenericScheduler: public scheduler 
   ,public ::rml::job
{
    friend class tbb::task;
    friend class UnpaddedArenaPrefix;
    friend class Arena;
    friend class allocate_root_proxy;
    friend class Governor;
#if __TBB_EXCEPTIONS
    friend class allocate_root_with_context_proxy;
    friend class tbb::task_group_context;
#endif /* __TBB_EXCEPTIONS */
#if __TBB_SCHEDULER_OBSERVER
    friend class task_scheduler_observer_v3;
#endif /* __TBB_SCHEDULER_OBSERVER */
    friend class scheduler;
    template<typename SchedulerTraits> friend class internal::CustomScheduler;

    //! If sizeof(task) is <=quick_task_size, it is handled on a free list instead of malloc'd.
    static const size_t quick_task_size = 256-task_prefix_reservation_size;

    //! Definitions for bits in task_prefix::extra_state
    enum internal_state_t {
        //! Tag for TBB <3.0 tasks.
        es_version_2_task = 0,
        //! Tag for TBB 3.0 tasks.
        es_version_3_task = 1,
        //! Tag for TBB 3.0 task_proxy.
        es_task_proxy = 2,
        //! Set if ref_count might be changed by another thread.  Used for debugging.
        es_ref_count_active = 0x40
    };
    
    static bool is_version_3_task( task& t ) {
        return (t.prefix().extra_state & 0x3F)==0x1;
    }

    //! Position in the call stack specifying its maximal filling when stealing is still allowed
    uintptr_t my_stealing_threshold;
#if __TBB_ipf
    //! Position in the RSE backup area specifying its maximal filling when stealing is still allowed
    uintptr_t my_rsb_stealing_threshold;
#endif

    static const size_t null_arena_index = ~0u;

    //! Index of the arena slot the scheduler occupies now, or occupied last time.
    size_t arena_index;

    //! Capacity of ready tasks deque (number of elements - pointers to task).
    size_t task_pool_size;

    //! Dummy slot used when scheduler is not in arena
    /** Only its "head" and "tail" members are ever used. The scheduler uses 
        the "task_pool" shortcut to access the task deque. **/
    ArenaSlot dummy_slot;

    //! Pointer to the slot in the arena we own at the moment.
    /** When out of arena it points to this scheduler's dummy_slot. **/
    mutable ArenaSlot* arena_slot;

    bool in_arena () const { return arena_slot != &dummy_slot; }

    bool is_local_task_pool_empty () {
        return arena_slot->task_pool == EmptyTaskPool || arena_slot->head >= arena_slot->tail;
    }

    //! The arena that I own (if master) or belong to (if worker)
    Arena* const arena;

    //! Random number generator used for picking a random victim from which to steal.
    FastRandom random;

    //! Free list of small tasks that can be reused.
    task* free_list;

    //! Innermost task whose task::execute() is running.
    task* innermost_running_task;

    //! Fake root task created by slave threads.
    /** The task is used as the "parent" argument to method wait_for_all. */
    task* dummy_task;

    //! Reference count for scheduler
    /** Number of task_scheduler_init objects that point to this scheduler */
    long ref_count;

    mail_inbox inbox;

    void attach_mailbox( affinity_id id ) {
        __TBB_ASSERT(id>0,NULL);
        inbox.attach( arena->mailbox(id) );
        my_affinity_id = id;
    }

    //! The mailbox id assigned to this scheduler.
    /** The id is assigned upon first entry into the arena.
        TODO: how are id's being garbage collected? 
        TODO: master thread may enter arena and leave and then reenter.
                We want to give it the same affinity_id upon reentry, if practical.
      */
    affinity_id my_affinity_id;

    /* A couple of bools can be located here because space is otherwise just padding after my_affinity_id. */

    //! True if this is assigned to thread local storage by registering with Governor.
    bool is_registered;

    //! True if *this was created by automatic TBB initialization
    bool is_auto_initialized;

#if __TBB_SCHEDULER_OBSERVER
    //! Last observer_proxy processed by this scheduler
    observer_proxy* local_last_observer_proxy;

    //! Notify any entry observers that have been created since the last call by this thread.
    void notify_entry_observers() {
        local_last_observer_proxy = observer_proxy::process_list(local_last_observer_proxy,is_worker(),/*is_entry=*/true);
    }
 
    //! Notify all exit observers that this thread is no longer participating in task scheduling.
    void notify_exit_observers( bool is_worker ) {
        observer_proxy::process_list(local_last_observer_proxy,is_worker,/*is_entry=*/false);
    }
#endif /* __TBB_SCHEDULER_OBSERVER */

#if COUNT_TASK_NODES
    //! Net number of big task objects that have been allocated but not yet freed.
    intptr task_node_count;
#endif /* COUNT_TASK_NODES */

#if STATISTICS
    long current_active;
    long current_length;
    //! Number of big tasks that have been malloc'd.
    /** To find total number of tasks malloc'd, compute (current_big_malloc+small_task_count) */
    long current_big_malloc;
    long execute_count;
    //! Number of tasks stolen
    long steal_count;
    //! Number of tasks received from mailbox
    long mail_received_count;
    long proxy_execute_count;
    long proxy_steal_count;
    long proxy_bypass_count;
#endif /* STATISTICS */

    //! Sets up the data necessary for the stealing limiting heuristics
    void init_stack_info ();

    //! Returns true if stealing is allowed
    bool can_steal () {
        int anchor;
#if __TBB_ipf
        return my_stealing_threshold < (uintptr_t)&anchor && (uintptr_t)__TBB_get_bsp() < my_rsb_stealing_threshold;
#else
        return my_stealing_threshold < (uintptr_t)&anchor;
#endif
    }

    //! Actions common to enter_arena and try_enter_arena
    void do_enter_arena();

    //! Used by workers to enter the arena 
    /** Does not lock the task pool in case if arena slot has been successfully grabbed. **/
    void enter_arena();

    //! Used by masters to try to enter the arena
    /** Does not lock the task pool in case if arena slot has been successfully grabbed. **/
    void try_enter_arena();

    //! Leave the arena
    void leave_arena();

    //! Locks victim's task pool, and returns pointer to it. The pointer can be NULL.
    task** lock_task_pool( ArenaSlot* victim_arena_slot ) const;

    //! Unlocks victim's task pool
    void unlock_task_pool( ArenaSlot* victim_arena_slot, task** victim_task_pool ) const;


    //! Locks the local task pool
    void acquire_task_pool() const;

    //! Unlocks the local task pool
    void release_task_pool() const;

    //! Get a task from the local pool.
    //! Checks if t is affinitized to another thread, and if so, bundles it as proxy.
    /** Returns either t or proxy containing t. **/
    task* prepare_for_spawning( task* t );

    /** Called only by the pool owner.
        Returns the pointer to the task or NULL if the pool is empty. 
        In the latter case compacts the pool. **/
    task* get_task();

    //! Attempt to get a task from the mailbox.
    /** Called only by the thread that owns *this.
        Gets a task only if there is one not yet executed by another thread.
        If successful, unlinks the task and returns a pointer to it.
        Otherwise returns NULL. */
    task* get_mailbox_task();

    //! True if t is a task_proxy
    static bool is_proxy( const task& t ) {
        return t.prefix().extra_state==es_task_proxy;
    }

    //! Extracts task pointer from task_proxy, and frees the proxy.
    /** Return NULL if underlying task was claimed by mailbox. */
    task* strip_proxy( task_proxy* result );

    //! Steal task from another scheduler's ready pool.
    task* steal_task( ArenaSlot& victim_arena_slot );

    /** Initial size of the task deque sufficient to serve without reallocation
        4 nested paralle_for calls with iteration space of 65535 grains each. **/
    static const size_t min_task_pool_size = 64;

    //! Allocate task pool containing at least n elements.
    task** allocate_task_pool( size_t n );

    //! Deallocate task pool that was allocated by means of allocate_task_pool.
    static void free_task_pool( task** pool ) {
        __TBB_ASSERT( pool, "attempt to free NULL TaskPool" );
        NFS_Free( pool );
    }

    //! Grow ready task deque to at least n elements.
    void grow( size_t n );

    //! Initialize a scheduler for a master thread.
    static GenericScheduler* create_master( Arena* a );

    //! Perform necessary cleanup when a master thread stops using TBB.
    void cleanup_master();

    //! Initialize a scheduler for a worker thread.
    static GenericScheduler* create_worker( Arena& a, size_t index );


    //! Top-level routine for worker threads
    /** Argument arg is a WorkerDescriptor*, cast to a (void*). */
    static thread_routine_return_type __TBB_THREAD_ROUTINE worker_routine( void* arg );

    //! Perform necessary cleanup when a worker thread finishes.
    static void cleanup_worker( void* arg );

protected:
    GenericScheduler( Arena* arena );

#if TBB_USE_ASSERT
    //! Check that internal data structures are in consistent state.
    /** Raises __TBB_ASSERT failure if inconsistency is found. */
    bool assert_okay() const;
#endif /* TBB_USE_ASSERT */

public:
    void local_spawn( task& first, task*& next );
    void local_spawn_root_and_wait( task& first, task*& next );

    /*override*/ 
    void spawn( task& first, task*& next ) {
        Governor::local_scheduler()->local_spawn( first, next );
    }
    /*override*/ 
    void spawn_root_and_wait( task& first, task*& next ) {
        Governor::local_scheduler()->local_spawn_root_and_wait( first, next );
    }

    //! Allocate and construct a scheduler object.
    static GenericScheduler* allocate_scheduler( Arena* arena );

    //! Destroy and deallocate scheduler that was created with method allocate.
    void free_scheduler();

    //! Allocate task object, either from the heap or a free list.
    /** Returns uninitialized task object with initialized prefix. */
    task& allocate_task( size_t number_of_bytes, 
                       __TBB_CONTEXT_ARG(task* parent, task_group_context* context) );

    //! Optimization hint to free_task that enables it omit unnecessary tests and code.
    enum hint {
        //! No hint 
        no_hint=0,
        //! Task is known to have been allocated by this scheduler
        is_local=1,
        //! Task is known to be a small task.
        /** Task should be returned to the free list of *some* scheduler, possibly not this scheduler. */
        is_small=2,
        //! Bitwise-OR of is_local and is_small.  
        /** Task should be returned to free list of this scheduler. */
        is_small_local=3
    };

    //! Put task on free list.
    /** Does not call destructor. */
    template<hint h>
    void free_task( task& t );

    void free_task_proxy( task_proxy& tp ) {
#if TBB_USE_ASSERT
        poison_pointer( tp.outbox );
        poison_pointer( tp.next_in_mailbox );
        tp.task_and_tag = 0xDEADBEEF;
#endif /* TBB_USE_ASSERT */
        free_task<is_small>(tp);
    }

    //! Return task object to the memory allocator.
    void deallocate_task( task& t ) {
#if TBB_USE_ASSERT
        task_prefix& p = t.prefix();
        p.state = 0xFF;
        p.extra_state = 0xFF; 
        poison_pointer(p.next);
#endif /* TBB_USE_ASSERT */
        NFS_Free((char*)&t-task_prefix_reservation_size);
#if COUNT_TASK_NODES
        task_node_count -= 1;
#endif /* COUNT_TASK_NODES */
    }

    //! True if running on a worker thread, false otherwise.
    inline bool is_worker() {
        return arena_index < arena->prefix().number_of_workers;
    }

#if TEST_ASSEMBLY_ROUTINES
    /** Defined in test_assembly.cpp */
    void test_assembly_routines();
#endif /* TEST_ASSEMBLY_ROUTINES */

#if COUNT_TASK_NODES
    intptr get_task_node_count( bool count_arena_workers = false ) {
        return task_node_count + (count_arena_workers? arena->workers_task_node_count(): 0);
    }
#endif /* COUNT_TASK_NODES */

    //! Special value used to mark return_list as not taking any more entries.
    static task* plugged_return_list() {return (task*)(intptr)(-1);}

    //! Number of small tasks that have been allocated by this scheduler. 
    intptr small_task_count;

    //! List of small tasks that have been returned to this scheduler by other schedulers.
    task* return_list;

    //! Free a small task t that that was allocated by a different scheduler 
    void free_nonlocal_small_task( task& t ); 

#if __TBB_EXCEPTIONS
    //! Padding isolating thread local members from members that can be written to by other threads.
    char _padding1[NFS_MaxLineSize - sizeof(context_list_node_t)];

    //! Head of the thread specific list of task group contexts.
    context_list_node_t context_list_head;

    //! Mutex protecting access to the list of task group contexts.
    spin_mutex context_list_mutex;

    //! Used to form the list of master thread schedulers.
    scheduler_list_node_t my_node;

    //! Thread local counter of cancellation requests.
    /** When this counter equals global_cancel_count, the cancellation state known
        to this thread is synchronized with the global cancellation state.
        \sa #global_cancel_count **/
    uintptr_t local_cancel_count;

    //! Propagates cancellation request to all descendants of the argument context.
    void propagate_cancellation ( task_group_context* ctx );

    //! Propagates cancellation request to contexts registered by this scheduler.
    void propagate_cancellation ();
#endif /* __TBB_EXCEPTIONS */
}; // class GenericScheduler

//------------------------------------------------------------------------
// auto_empty_task
//------------------------------------------------------------------------

//! Smart holder for the empty task class with automatic destruction
class auto_empty_task {
    task* my_task;
    GenericScheduler* my_scheduler;
public:
    auto_empty_task ( __TBB_CONTEXT_ARG(GenericScheduler *s, task_group_context* context) ) 
        : my_task( new(&s->allocate_task(sizeof(empty_task), __TBB_CONTEXT_ARG(NULL, context))) empty_task )
        , my_scheduler(s)
    {}
    // empty_task has trivial destructor, so there's no need to call it.
    ~auto_empty_task () { my_scheduler->free_task<GenericScheduler::is_small_local>(*my_task); }

    operator task& () { return *my_task; }
    task* operator & () { return my_task; }
    task_prefix& prefix () { return my_task->prefix(); }
}; // class auto_empty_task

//------------------------------------------------------------------------
// Methods of class Governor that need full definition of GenericScheduler
//------------------------------------------------------------------------

void Governor::sign_on(GenericScheduler* s) {
    __TBB_ASSERT( !s->is_registered, NULL );  
    s->is_registered = true;
    __TBB_InitOnce::add_ref();
    theTLS.set(s);
}

void Governor::sign_off(GenericScheduler* s) {
    if( s->is_registered ) {
#if USE_PTHREAD
        __TBB_ASSERT( theTLS.get()==s || (!s->is_worker() && !theTLS.get()), "attempt to unregister a wrong scheduler instance" );
#else
        __TBB_ASSERT( theTLS.get()==s, "attempt to unregister a wrong scheduler instance" );
#endif /* USE_PTHREAD */
        theTLS.set(NULL);
        s->is_registered = false;
        __TBB_InitOnce::remove_ref();
    }
}

GenericScheduler* Governor::init_scheduler( int num_threads, stack_size_type stack_size, bool auto_init ) {
    if( !__TBB_InitOnce::initialization_done() )
        DoOneTimeInitializations();
    GenericScheduler* s = theTLS.get();
    if( s ) {
        s->ref_count += 1;
        return s;
    }
    s = GenericScheduler::create_master( obtain_arena(num_threads, stack_size) );
    __TBB_ASSERT(s, "Somehow a local scheduler creation for a master thread failed");
    s->is_auto_initialized = auto_init;
    return s;
}

void Governor::terminate_scheduler( GenericScheduler* s ) {
    __TBB_ASSERT( s == theTLS.get(), "Attempt to terminate non-local scheduler instance" );
    if( !--(s->ref_count) )
        s->cleanup_master();
}

void Governor::auto_terminate(void* arg){
    GenericScheduler* s = static_cast<GenericScheduler*>(arg);
    if( s && s->is_auto_initialized ) {
        if( !--(s->ref_count) ) {
            if ( !theTLS.get() && !s->is_local_task_pool_empty() ) {
                // This thread's TLS slot is already cleared. But in order to execute
                // remaining tasks cleanup_master() will need TLS correctly set.
                // So we temporarily restore its value.
                theTLS.set(s);
                s->cleanup_master();
                theTLS.set(NULL);
            }
            else
                s->cleanup_master();
        }
    }
}

//------------------------------------------------------------------------
// GenericScheduler implementation
//------------------------------------------------------------------------

inline task& GenericScheduler::allocate_task( size_t number_of_bytes, 
                                            __TBB_CONTEXT_ARG(task* parent, task_group_context* context) ) {
    GATHER_STATISTIC(current_active+=1);
    task* t = free_list;
    if( number_of_bytes<=quick_task_size ) {
        if( t ) {
            GATHER_STATISTIC(current_length-=1);
            __TBB_ASSERT( t->state()==task::freed, "free list of tasks is corrupted" );
            free_list = t->prefix().next;
        } else if( return_list ) {
            // No fence required for read of return_list above, because __TBB_FetchAndStoreW has a fence.
            t = (task*)__TBB_FetchAndStoreW( &return_list, 0 );
            __TBB_ASSERT( t, "another thread emptied the return_list" );
            __TBB_ASSERT( t->prefix().origin==this, "task returned to wrong return_list" );
            ITT_NOTIFY( sync_acquired, &return_list );
            free_list = t->prefix().next;
        } else {
            t = (task*)((char*)NFS_Allocate( task_prefix_reservation_size+quick_task_size, 1, NULL ) + task_prefix_reservation_size );
#if COUNT_TASK_NODES
            ++task_node_count;
#endif /* COUNT_TASK_NODES */
            t->prefix().origin = this;
            ++small_task_count;
        }
    } else {
        GATHER_STATISTIC(current_big_malloc+=1);
        t = (task*)((char*)NFS_Allocate( task_prefix_reservation_size+number_of_bytes, 1, NULL ) + task_prefix_reservation_size );
#if COUNT_TASK_NODES
        ++task_node_count;
#endif /* COUNT_TASK_NODES */
        t->prefix().origin = NULL;
    }
    task_prefix& p = t->prefix();
#if __TBB_EXCEPTIONS
    p.context = context;
#endif /* __TBB_EXCEPTIONS */
    p.owner = this;
    p.ref_count = 0;
    // Assign some not outrageously out-of-place value for a while
    p.depth = 0;
    p.parent = parent;
    // In TBB 3.0 and later, the constructor for task sets extra_state to indicate the version of the tbb/task.h header.
    // In TBB 2.0 and earlier, the constructor leaves extra_state as zero.
    p.extra_state = 0;
    p.affinity = 0;
    p.state = task::allocated;
    return *t;
}

template<GenericScheduler::hint h>
inline void GenericScheduler::free_task( task& t ) {
    GATHER_STATISTIC(current_active-=1);
    task_prefix& p = t.prefix();
    // Verify that optimization hints are correct.
    __TBB_ASSERT( h!=is_small_local || p.origin==this, NULL );
    __TBB_ASSERT( !(h&is_small) || p.origin, NULL );
#if TBB_USE_ASSERT
    p.depth = 0xDEADBEEF;
    p.ref_count = 0xDEADBEEF;
    poison_pointer(p.owner);
#endif /* TBB_USE_ASSERT */
    __TBB_ASSERT( 1L<<t.state() & (1L<<task::executing|1L<<task::allocated), NULL );
    p.state = task::freed;
    if( h==is_small_local || p.origin==this ) {
        GATHER_STATISTIC(current_length+=1);
        p.next = free_list;
        free_list = &t;
    } else if( !(h&is_local) && p.origin ) {
        free_nonlocal_small_task(t);
    } else {
        deallocate_task(t);
    }
}

void GenericScheduler::free_nonlocal_small_task( task& t ) {
    __TBB_ASSERT( t.state()==task::freed, NULL );
    GenericScheduler& s = *static_cast<GenericScheduler*>(t.prefix().origin);
    __TBB_ASSERT( &s!=this, NULL );
    for(;;) {
        task* old = s.return_list;
        if( old==plugged_return_list() ) 
            break;
        // Atomically insert t at head of s.return_list
        t.prefix().next = old; 
        ITT_NOTIFY( sync_releasing, &s.return_list );
        if( __TBB_CompareAndSwapW( &s.return_list, (intptr)&t, (intptr)old )==(intptr)old ) 
            return;
    }
    deallocate_task(t);
    if( __TBB_FetchAndDecrementWrelease( &s.small_task_count )==1 ) {
        // We freed the last task allocated by scheduler s, so it's our responsibility
        // to free the scheduler.
        NFS_Free( &s );
    }
}

//------------------------------------------------------------------------
// CustomScheduler
//------------------------------------------------------------------------

//! A scheduler with a customized evaluation loop.
/** The customization can use SchedulerTraits to make decisions without needing a run-time check. */
template<typename SchedulerTraits>
class CustomScheduler: private GenericScheduler {
    //! Scheduler loop that dispatches tasks.
    /** If child is non-NULL, it is dispatched first.
        Then, until "parent" has a reference count of 1, other task are dispatched or stolen. */
    void local_wait_for_all( task& parent, task* child );

    /*override*/
    void wait_for_all( task& parent, task* child ) {
        static_cast<CustomScheduler*>(Governor::local_scheduler())->local_wait_for_all( parent, child );
    }

    typedef CustomScheduler<SchedulerTraits> scheduler_type;

    //! Construct a CustomScheduler
    CustomScheduler( Arena* arena ) : GenericScheduler(arena) {}

    static bool tally_completion_of_one_predecessor( task& s ) {
        task_prefix& p = s.prefix();
        if( SchedulerTraits::itt_possible )
            ITT_NOTIFY(sync_releasing, &p.ref_count);
        if( SchedulerTraits::has_slow_atomic && p.ref_count==1 ) {
            p.ref_count=0;
        } else {
            reference_count k = __TBB_FetchAndDecrementWrelease(&p.ref_count);
            __TBB_ASSERT( k>0, "completion of task caused parent's reference count to underflow" );
            if( k!=1 ) 
                return false;
        }
        if( SchedulerTraits::itt_possible )
            ITT_NOTIFY(sync_acquired, &p.ref_count);
        return true;
    }

public:
    static GenericScheduler* allocate_scheduler( Arena* arena ) {
        __TBB_ASSERT( arena, "missing arena" );
        scheduler_type* s = (scheduler_type*)NFS_Allocate(sizeof(scheduler_type),1,NULL);
        new( s ) scheduler_type(  arena );
        __TBB_ASSERT( s->assert_okay(), NULL );
        ITT_SYNC_CREATE(s, SyncType_Scheduler, SyncObj_TaskPoolSpinning);
        return s;
    }
};

//------------------------------------------------------------------------
// AssertOkay
//------------------------------------------------------------------------
#if TBB_USE_ASSERT
/** Logically, this method should be a member of class task.
    But we do not want to publish it, so it is here instead. */
static bool AssertOkay( const task& task ) {
    __TBB_ASSERT( &task!=NULL, NULL );
    __TBB_ASSERT( (uintptr)&task % task_alignment == 0, "misaligned task" );
    __TBB_ASSERT( (unsigned)task.state()<=(unsigned)task::recycle, "corrupt task (invalid state)" );
    return true;
}
#endif /* TBB_USE_ASSERT */

//------------------------------------------------------------------------
// Methods of Arena
//------------------------------------------------------------------------
Arena* Arena::allocate_arena( unsigned number_of_slots, unsigned number_of_workers, stack_size_type stack_size) {
    __TBB_ASSERT( sizeof(ArenaPrefix) % NFS_GetLineSize()==0, "ArenaPrefix not multiple of cache line size" );
    __TBB_ASSERT( sizeof(mail_outbox)==NFS_MaxLineSize, NULL );
    size_t n = sizeof(ArenaPrefix) + number_of_slots*(sizeof(mail_outbox)+sizeof(ArenaSlot));

    unsigned char* storage = (unsigned char*)NFS_Allocate( n, 1, NULL );
    memset( storage, 0, n );
    Arena* a = (Arena*)(storage + sizeof(ArenaPrefix)+ number_of_slots*(sizeof(mail_outbox)));
    __TBB_ASSERT( sizeof(a->slot[0]) % NFS_GetLineSize()==0, "Arena::slot size not multiple of cache line size" );
    __TBB_ASSERT( (uintptr)a % NFS_GetLineSize()==0, NULL );
    new( &a->prefix() ) ArenaPrefix( number_of_slots, number_of_workers );

    // Allocate the worker_list
    WorkerDescriptor * w = new WorkerDescriptor[number_of_workers];
    memset( w, 0, sizeof(WorkerDescriptor)*(number_of_workers));
    a->prefix().worker_list = w;

#if TBB_USE_ASSERT
    // Verify that earlier memset initialized the mailboxes.
    for( unsigned j=1; j<=number_of_slots; ++j ) {
        a->mailbox(j).assert_is_initialized();
    }
#endif /* TBB_USE_ASSERT */

    a->prefix().stack_size = stack_size;
    size_t k;
    // Mark each worker slot as locked and unused
    for( k=0; k<number_of_workers; ++k ) {
        // All slots are set to null meaning that they are free
        ITT_SYNC_CREATE(a->slot + k, SyncType_Scheduler, SyncObj_WorkerTaskPool);
        ITT_SYNC_CREATE(&w[k].scheduler, SyncType_Scheduler, SyncObj_WorkerLifeCycleMgmt);
        ITT_SYNC_CREATE(&a->mailbox(k+1), SyncType_Scheduler, SyncObj_Mailbox);
    }
    // Mark rest of slots as unused
    for( ; k<number_of_slots; ++k ) {
        ITT_SYNC_CREATE(a->slot + k, SyncType_Scheduler, SyncObj_MasterTaskPool);
        ITT_SYNC_CREATE(&a->mailbox(k+1), SyncType_Scheduler, SyncObj_Mailbox);
    }

    return a;
}

inline void Arena::mark_pool_full() {
    // Double-check idiom that is deliberately sloppy about memory fences.
    // Technically, to avoid missed wakeups, there should be a full memory fence between the point we 
    // released the task pool (i.e. spawned task) and read the gate's state.  However, adding such a 
    // fence might hurt overall performance more than it helps, because the fence would be executed 
    // on every task pool release, even when stealing does not occur.  Since TBB allows parallelism, 
    // but never promises parallelism, the missed wakeup is not a correctness problem.
    pool_state_t snapshot = prefix().pool_state;
    if( is_busy_or_empty(snapshot) ) {
        // Attempt to mark as full.  The compare_and_swap below is a little unusual because the 
        // result is compared to a value that can be different than the comparand argument.
        if( prefix().pool_state.compare_and_swap( SNAPSHOT_FULL, snapshot )==SNAPSHOT_EMPTY ) {
            if( snapshot!=SNAPSHOT_EMPTY ) {
                // This thread initialized s1 to "busy" and then another thread transitioned 
                // pool_state to "empty" in the meantime, which caused the compare_and_swap above 
                // to fail.  Attempt to transition pool_state from "empty" to "full".
                if( prefix().pool_state.compare_and_swap( SNAPSHOT_FULL, SNAPSHOT_EMPTY )!=SNAPSHOT_EMPTY ) {
                    // Some other thread transitioned pool_state from "empty", and hence became
                    // responsible for waking up workers.
                    return;
                }
            }
            // This thread transitioned pool from empty to full state, and thus is responsible for
            // telling RML that there is work to do.
            prefix().server->adjust_job_count_estimate( int(prefix().number_of_workers) );
        }
    }
}

bool Arena::check_if_pool_is_empty() 
{
    for(;;) {
        pool_state_t snapshot = prefix().pool_state;
        switch( snapshot ) {
            case SNAPSHOT_EMPTY:
            case SNAPSHOT_SERVER_GOING_AWAY:
                return true;
            case SNAPSHOT_FULL: {
                // Use unique id for "busy" in order to avoid ABA problems.
                const pool_state_t busy = pool_state_t(this);
                // Request permission to take snapshot
                if( prefix().pool_state.compare_and_swap( busy, SNAPSHOT_FULL )==SNAPSHOT_FULL ) {
                    // Got permission.  Take the snapshot.
                    size_t n = prefix().limit;
                    size_t k; 
                    for( k=0; k<n; ++k ) 
                        if( slot[k].task_pool != EmptyTaskPool && slot[k].head < slot[k].tail )
                            break;
                    // Test and test-and-set.
                    if( prefix().pool_state==busy ) {
                        if( k>=n ) {
                            if( prefix().pool_state.compare_and_swap( SNAPSHOT_EMPTY, busy )==busy ) {
                                // This thread transitioned pool to empty state, and thus is responsible for
                                // telling RML that there is no other work to do.
                                prefix().server->adjust_job_count_estimate( -int(prefix().number_of_workers) );
                                return true;
                            }
                        } else {
                            // Undo previous transition SNAPSHOT_FULL-->busy, unless another thread undid it.
                            prefix().pool_state.compare_and_swap( SNAPSHOT_FULL, busy );
                        }
                    }
                } 
                return false;
            }
            default:
                // Another thread is taking a snapshot.
                return false;
        }
    }
}

void Arena::terminate_workers() {
    for(;;) {
        pool_state_t snapshot = prefix().pool_state;
        if( snapshot==SNAPSHOT_SERVER_GOING_AWAY ) 
            break;
        if( prefix().pool_state.compare_and_swap( SNAPSHOT_SERVER_GOING_AWAY, snapshot )==snapshot ) {
            if( snapshot!=SNAPSHOT_EMPTY )
                prefix().server->adjust_job_count_estimate( -int(prefix().number_of_workers) );
            break;
        }
    }
    prefix().server->request_close_connection();
}


#if COUNT_TASK_NODES
intptr Arena::workers_task_node_count() {
    intptr result = 0;
    for( unsigned i=0; i<prefix().number_of_workers; ++i ) {
        GenericScheduler* s = prefix().worker_list[i].scheduler;
        if( s )
            result += s->task_node_count;
    }
    return result;
}
#endif

//------------------------------------------------------------------------
// Methods of GenericScheduler
//------------------------------------------------------------------------
#if _MSC_VER && !defined(__INTEL_COMPILER)
    // Suppress overzealous compiler warning about using 'this' in base initializer list. 
    #pragma warning(push)
    #pragma warning(disable:4355)
#endif

GenericScheduler::GenericScheduler( Arena* arena_ ) :
    arena_index(null_arena_index),
    task_pool_size(0),
    arena_slot(&dummy_slot),
    arena(arena_),
    random( unsigned(this-(GenericScheduler*)NULL) ),
    free_list(NULL),
    innermost_running_task(NULL),
    dummy_task(NULL),
    ref_count(1),
    my_affinity_id(0),
    is_registered(false),
    is_auto_initialized(false),
#if __TBB_SCHEDULER_OBSERVER
    local_last_observer_proxy(NULL),
#endif /* __TBB_SCHEDULER_OBSERVER */
#if COUNT_TASK_NODES
    task_node_count(0),
#endif /* COUNT_TASK_NODES */
#if STATISTICS
    current_active(0),
    current_length(0),
    current_big_malloc(0),
    execute_count(0),
    steal_count(0),
    mail_received_count(0),
    proxy_execute_count(0),
    proxy_steal_count(0),
    proxy_bypass_count(0),
#endif /* STATISTICS */
    small_task_count(1),   // Extra 1 is a guard reference
    return_list(NULL)
{
    dummy_slot.task_pool = allocate_task_pool( min_task_pool_size );
    dummy_slot.head = dummy_slot.tail = 0;
    dummy_task = &allocate_task( sizeof(task), __TBB_CONTEXT_ARG(NULL, NULL) );
#if __TBB_EXCEPTIONS
    context_list_head.my_prev = &context_list_head;
    context_list_head.my_next = &context_list_head;
    ITT_SYNC_CREATE(&context_list_mutex, SyncType_Scheduler, SyncObj_ContextsList);
#endif /* __TBB_EXCEPTIONS */
    dummy_task->prefix().ref_count = 2;
    ITT_SYNC_CREATE(&dummy_task->prefix().ref_count, SyncType_Scheduler, SyncObj_WorkerLifeCycleMgmt);
    ITT_SYNC_CREATE(&return_list, SyncType_Scheduler, SyncObj_TaskReturnList);
    __TBB_ASSERT( assert_okay(), "constructor error" );
}

#if _MSC_VER && !defined(__INTEL_COMPILER)
    #pragma warning(pop)
#endif // warning 4355 is back

#if TBB_USE_ASSERT
bool GenericScheduler::assert_okay() const {
#if TBB_USE_ASSERT>=2||TEST_ASSEMBLY_ROUTINES
    acquire_task_pool();
    task** tp = dummy_slot.task_pool;
    __TBB_ASSERT( task_pool_size >= min_task_pool_size, NULL );
    __TBB_ASSERT( arena_slot->head <= arena_slot->tail, NULL );
    for ( size_t i = arena_slot->head; i < arena_slot->tail; ++i ) {
        __TBB_ASSERT( (uintptr_t)tp[i] + 1 > 1u, "nil or invalid task pointer in the deque" );
        __TBB_ASSERT( tp[i]->prefix().state == task::ready ||
                      tp[i]->prefix().extra_state == es_task_proxy, "task in the deque has invalid state" );
    }
    release_task_pool();
#endif /* TBB_USE_ASSERT>=2||TEST_ASSEMBLY_ROUTINES */
    return true;
}
#endif /* TBB_USE_ASSERT */

#if __TBB_EXCEPTIONS

void GenericScheduler::propagate_cancellation () {
    spin_mutex::scoped_lock lock(context_list_mutex);
    // Acquire fence is necessary to ensure that the subsequent node->my_next load 
    // returned the correct value in case it was just inserted in another thread.
    // The fence also ensures visibility of the correct my_parent value.
    context_list_node_t *node = __TBB_load_with_acquire(context_list_head.my_next);
    while ( node != &context_list_head ) {
        task_group_context *ctx = __TBB_get_object_addr(task_group_context, my_node, node);
            // The absence of acquire fence while reading my_cancellation_requested may result 
            // in repeated traversals of the same parents chain if another group (precedent or 
            // descendant) belonging to the tree being canceled sends cancellation request of 
            // its own around the same time.
        if ( !ctx->my_cancellation_requested )
            ctx->propagate_cancellation_from_ancestors();
        node = node->my_next;
        __TBB_ASSERT( ctx->is_alive(), "Walked into a destroyed context while propagating cancellation" );
    }
}

/** Propagates cancellation down the tree of dependent contexts by walking each 
    thread's local list of contexts **/
void GenericScheduler::propagate_cancellation ( task_group_context* ctx ) {
    __TBB_ASSERT ( ctx->my_cancellation_requested, "No cancellation request in the context" );
    // The whole propagation algorithm is under the lock in order to ensure correctness 
    // in case of parallel cancellations at the different levels of the context tree.
    // See the note 2 at the bottom of the file.
    mutex::scoped_lock lock(the_scheduler_list_mutex);
    // Advance global cancellation state
    __TBB_FetchAndAddWrelease(&global_cancel_count, 1);
    // First propagate to workers using arena to access their context lists
    size_t num_workers = arena->prefix().number_of_workers;
    for ( size_t i = 0; i < num_workers; ++i ) {
        // No fence is necessary here since the context list of worker's scheduler 
        // can contain anything of interest only after the first stealing was done
        // by that worker. And doing it applies the necessary fence
        GenericScheduler *s = arena->prefix().worker_list[i].scheduler;
        // If the worker is in the middle of its startup sequence, skip it.
        if ( s )
            s->propagate_cancellation();
    }
    // Then propagate to masters using the global list of master's schedulers
    scheduler_list_node_t *node = the_scheduler_list_head.my_next;
    while ( node != &the_scheduler_list_head ) {
        __TBB_get_object_addr(GenericScheduler, my_node, node)->propagate_cancellation();
        node = node->my_next;
    }
    // Now sync up the local counters
    for ( size_t i = 0; i < num_workers; ++i ) {
        GenericScheduler *s = arena->prefix().worker_list[i].scheduler;
        // If the worker is in the middle of its startup sequence, skip it.
        if ( s )
            s->local_cancel_count = global_cancel_count;
    }
    node = the_scheduler_list_head.my_next;
    while ( node != &the_scheduler_list_head ) {
        __TBB_get_object_addr(GenericScheduler, my_node, node)->local_cancel_count = global_cancel_count;
        node = node->my_next;
    }
}
#endif /* __TBB_EXCEPTIONS */



void GenericScheduler::init_stack_info () {
    // Stacks are growing top-down. Highest address is called "stack base", 
    // and the lowest is "stack limit".
#if USE_WINTHREAD
#if defined(_MSC_VER)&&_MSC_VER<1400 && !_WIN64
    NT_TIB  *pteb = (NT_TIB*)__TBB_machine_get_current_teb();
#else
    NT_TIB  *pteb = (NT_TIB*)NtCurrentTeb();
#endif
    __TBB_ASSERT( &pteb < pteb->StackBase && &pteb > pteb->StackLimit, "invalid stack info in TEB" );
    __TBB_ASSERT( arena->prefix().stack_size>0, "stack_size not initialized?" );
    // When a thread is created with the attribute STACK_SIZE_PARAM_IS_A_RESERVATION, stack limit 
    // in the TIB points to the committed part of the stack only. This renders the expression
    // "(uintptr_t)pteb->StackBase / 2 + (uintptr_t)pteb->StackLimit / 2" virtually useless.
    // Thus for worker threads we use the explicit stack size we used while creating them.
    // And for master threads we rely on the following fact and assumption:
    // - the default stack size of a master thread on Windows is 1M;
    // - if it was explicitly set by the application it is at least as large as the size of a worker stack.
    if ( is_worker() || arena->prefix().stack_size < MByte )
        my_stealing_threshold = (uintptr_t)pteb->StackBase - arena->prefix().stack_size / 2;
    else
        my_stealing_threshold = (uintptr_t)pteb->StackBase - MByte / 2;
#else /* USE_PTHREAD */
    // There is no portable way to get stack base address in Posix, so we use 
    // non-portable method (on all modern Linux) or the simplified approach 
    // based on the common sense assumptions. The most important assumption 
    // is that the main thread's stack size is not less than that of other threads.
    size_t  stack_size = arena->prefix().stack_size;
    void    *stack_base = &stack_size;
#if __TBB_ipf
    void    *rsb_base = __TBB_get_bsp();
#endif
#if __linux__
    size_t  np_stack_size = 0;
    void    *stack_limit = NULL;
    pthread_attr_t  attr_stack, np_attr_stack;
    if( 0 == pthread_getattr_np(pthread_self(), &np_attr_stack) ) {
        if ( 0 == pthread_attr_getstack(&np_attr_stack, &stack_limit, &np_stack_size) ) {
            if ( 0 == pthread_attr_init(&attr_stack) ) {
                if ( 0 == pthread_attr_getstacksize(&attr_stack, &stack_size) )
                {
                    stack_base = (char*)stack_limit + np_stack_size;
                    if ( np_stack_size < stack_size ) {
                        // We are in a secondary thread. Use reliable data.
#if __TBB_ipf
                        // IA64 stack is split into RSE backup and memory parts
                        rsb_base = stack_limit;
                        stack_size = np_stack_size/2;
#else
                        stack_size = np_stack_size;
#endif /* !__TBB_ipf */
                    }
                    // We are either in the main thread or this thread stack 
                    // is bigger that that of the main one. As we cannot discern
                    // these cases we fall back to the default (heuristic) values.
                }
                pthread_attr_destroy(&attr_stack);
            }
        }
        pthread_attr_destroy(&np_attr_stack);
    }
#endif /* __linux__ */
    __TBB_ASSERT( stack_size>0, "stack size must be positive" );
    my_stealing_threshold = (uintptr_t)((char*)stack_base - stack_size/2);
#if __TBB_ipf
    my_rsb_stealing_threshold = (uintptr_t)((char*)rsb_base + stack_size/2);
#endif
#endif /* USE_PTHREAD */
}

task** GenericScheduler::allocate_task_pool( size_t n ) {
    __TBB_ASSERT( n > task_pool_size, "Cannot shrink the task pool" );
    size_t byte_size = ((n * sizeof(task*) + NFS_MaxLineSize - 1) / NFS_MaxLineSize) * NFS_MaxLineSize;
    task_pool_size = byte_size / sizeof(task*);
    task** new_pool = (task**)NFS_Allocate( byte_size, 1, NULL );
    // No need to clear the fresh deque since valid items are designated by the head and tail members.
#if TBB_USE_ASSERT>=2
    // But clear it in the high vigilance debug mode
    memset( new_pool, -1, n );
#endif /* TBB_USE_ASSERT>=2 */
    return new_pool;
}

void GenericScheduler::grow( size_t new_size ) {
    __TBB_ASSERT( assert_okay(), NULL );
    if ( new_size < 2 * task_pool_size )
        new_size = 2 * task_pool_size;
    task** new_pool = allocate_task_pool( new_size ); // updates task_pool_size
    task** old_pool = dummy_slot.task_pool;
    acquire_task_pool();    // requires the old dummy_slot.task_pool value
    // arena_slot->tail should not be updated before arena_slot->head because their
    // values are used by other threads to check if this task pool is empty.
    size_t new_tail = arena_slot->tail - arena_slot->head;
    __TBB_ASSERT( new_tail <= task_pool_size, "new task pool is too short" );
    memcpy( new_pool, old_pool + arena_slot->head, new_tail * sizeof(task*) );
    arena_slot->head = 0;
    arena_slot->tail = new_tail;
    dummy_slot.task_pool = new_pool;
    release_task_pool();    // updates the task pool pointer in our arena slot
    free_task_pool( old_pool );
    __TBB_ASSERT( assert_okay(), NULL );
}


GenericScheduler* GenericScheduler::allocate_scheduler( Arena* arena ) {
    switch( SchedulerTraitsId ) {
        /* DefaultSchedulerTraits::id is listed explicitly as a case so that the host compiler
           will issue an error message if it is the same as another id in the list. */
        default:
        case DefaultSchedulerTraits::id:
            return CustomScheduler<DefaultSchedulerTraits>::allocate_scheduler(arena);
        case IntelSchedulerTraits::id:
            return CustomScheduler<IntelSchedulerTraits>::allocate_scheduler(arena);
    }
}

void GenericScheduler::free_scheduler() {
    if( in_arena() ) {
        acquire_task_pool();
        leave_arena();
    }
#if __TBB_EXCEPTIONS
    task_group_context* &context = dummy_task->prefix().context;
    // Only master thread's dummy task has a context
    if ( context != &dummy_context) {
        //! \todo Add assertion that master's dummy task context does not have children
        context->task_group_context::~task_group_context();
        NFS_Free(context);
        {
            mutex::scoped_lock lock(the_scheduler_list_mutex);
            my_node.my_next->my_prev = my_node.my_prev;
            my_node.my_prev->my_next = my_node.my_next;
        }
    }
#endif /* __TBB_EXCEPTIONS */
    free_task<is_small_local>( *dummy_task );

    // k accounts for a guard reference and each task that we deallocate.
    intptr k = 1;
    for(;;) {
        while( task* t = free_list ) {
            free_list = t->prefix().next;
            deallocate_task(*t);
            ++k;
        }
        if( return_list==plugged_return_list() ) 
            break;
        free_list = (task*)__TBB_FetchAndStoreW( &return_list, (intptr)plugged_return_list() );
    }

#if COUNT_TASK_NODES
    arena->prefix().task_node_count += task_node_count;
#endif /* COUNT_TASK_NODES */
#if STATISTICS
    the_statistics.record( execute_count, steal_count, mail_received_count,
                           proxy_execute_count, proxy_steal_count, proxy_bypass_count );
#endif /* STATISTICS */
    free_task_pool( dummy_slot.task_pool );
    dummy_slot.task_pool = NULL;
    // Update small_task_count last.  Doing so sooner might cause another thread to free *this.
    __TBB_ASSERT( small_task_count>=k, "small_task_count corrupted" );
    Governor::sign_off(this);
    if( __TBB_FetchAndAddW( &small_task_count, -k )==k ) 
        NFS_Free( this );
}

/** ATTENTION: 
    This method is mostly the same as GenericScheduler::lock_task_pool(), with 
    a little different logic of slot state checks (slot is either locked or points 
    to our task pool).
    Thus if either of them is changed, consider changing the counterpart as well. **/
inline void GenericScheduler::acquire_task_pool() const {
    if ( !in_arena() )
        return; // we are not in arena - nothing to lock
    atomic_backoff backoff;
    bool sync_prepare_done = false;
    for(;;) {
#if TBB_USE_ASSERT
        __TBB_ASSERT( arena_slot == arena->slot + arena_index, "invalid arena slot index" );
        // Local copy of the arena slot task pool pointer is necessary for the next 
        // assertion to work correctly to exclude asynchronous state transition effect.
        task** tp = arena_slot->task_pool;
        __TBB_ASSERT( tp == LockedTaskPool || tp == dummy_slot.task_pool, "slot ownership corrupt?" );
#endif
        if( arena_slot->task_pool != LockedTaskPool && 
            __TBB_CompareAndSwapW( &arena_slot->task_pool, (intptr_t)LockedTaskPool, 
                                   (intptr_t)dummy_slot.task_pool ) == (intptr_t)dummy_slot.task_pool )
        {
            // We acquired our own slot
            ITT_NOTIFY(sync_acquired, arena_slot);
            break;
        } 
        else if( !sync_prepare_done ) {
            // Start waiting
            ITT_NOTIFY(sync_prepare, arena_slot);
            sync_prepare_done = true;
        }
        // Someone else acquired a lock, so pause and do exponential backoff.
        backoff.pause();
#if TEST_ASSEMBLY_ROUTINES
        __TBB_ASSERT( arena_slot->task_pool == LockedTaskPool || 
                      arena_slot->task_pool == dummy_slot.task_pool, NULL );
#endif /* TEST_ASSEMBLY_ROUTINES */
    }
    __TBB_ASSERT( arena_slot->task_pool == LockedTaskPool, "not really acquired task pool" );
} // GenericScheduler::acquire_task_pool

inline void GenericScheduler::release_task_pool() const {
    if ( !in_arena() )
        return; // we are not in arena - nothing to unlock
    __TBB_ASSERT( arena_slot, "we are not in arena" );
    __TBB_ASSERT( arena_slot->task_pool == LockedTaskPool, "arena slot is not locked" );
    ITT_NOTIFY(sync_releasing, arena_slot);
    __TBB_store_with_release( arena_slot->task_pool, dummy_slot.task_pool );
}

/** ATTENTION: 
    This method is mostly the same as GenericScheduler::acquire_task_pool(), 
    with a little different logic of slot state checks (slot can be empty, locked 
    or point to any task pool other than ours, and asynchronous transitions between 
    all these states are possible).
    Thus if any of them is changed, consider changing the counterpart as well **/
inline task** GenericScheduler::lock_task_pool( ArenaSlot* victim_arena_slot ) const {
    task** victim_task_pool;
    atomic_backoff backoff;
    bool sync_prepare_done = false;
    for(;;) {
        victim_task_pool = victim_arena_slot->task_pool;
        // TODO: Investigate the effect of bailing out on the locked pool without trying to lock it.
        //       When doing this update assertion in the end of the method.
        if ( victim_task_pool == EmptyTaskPool ) {
            // The victim thread emptied its task pool - nothing to lock
            if( sync_prepare_done )
                ITT_NOTIFY(sync_cancel, victim_arena_slot);
            break;
        }
        if( victim_task_pool != LockedTaskPool && 
            __TBB_CompareAndSwapW( &victim_arena_slot->task_pool, 
                (intptr_t)LockedTaskPool, (intptr_t)victim_task_pool ) == (intptr_t)victim_task_pool )
        {
            // We've locked victim's task pool
            ITT_NOTIFY(sync_acquired, victim_arena_slot);
            break;
        }
        else if( !sync_prepare_done ) {
            // Start waiting
            ITT_NOTIFY(sync_prepare, victim_arena_slot);
            sync_prepare_done = true;
        }
        // Someone else acquired a lock, so pause and do exponential backoff.
        backoff.pause();
    }
    __TBB_ASSERT( victim_task_pool == EmptyTaskPool || 
                  (victim_arena_slot->task_pool == LockedTaskPool && victim_task_pool != LockedTaskPool), 
                  "not really locked victim's task pool?" );
    return victim_task_pool;
} // GenericScheduler::lock_task_pool

inline void GenericScheduler::unlock_task_pool( ArenaSlot* victim_arena_slot, 
                                                task** victim_task_pool ) const {
    __TBB_ASSERT( victim_arena_slot, "empty victim arena slot pointer" );
    __TBB_ASSERT( victim_arena_slot->task_pool == LockedTaskPool, "victim arena slot is not locked" );
    ITT_NOTIFY(sync_releasing, victim_arena_slot);
    __TBB_store_with_release( victim_arena_slot->task_pool, victim_task_pool );
}


inline task* GenericScheduler::prepare_for_spawning( task* t ) {
    __TBB_ASSERT( t->state()==task::allocated, "attempt to spawn task that is not in 'allocated' state" );
    t->prefix().owner = this;
    t->prefix().state = task::ready;
#if TBB_USE_ASSERT
    if( task* parent = t->parent() ) {
        internal::reference_count ref_count = parent->prefix().ref_count;
        __TBB_ASSERT( ref_count>=0, "attempt to spawn task whose parent has a ref_count<0" );
        __TBB_ASSERT( ref_count!=0, "attempt to spawn task whose parent has a ref_count==0 (forgot to set_ref_count?)" );
        parent->prefix().extra_state |= es_ref_count_active;
    }
#endif /* TBB_USE_ASSERT */
    affinity_id dst_thread = t->prefix().affinity;
    __TBB_ASSERT( dst_thread == 0 || is_version_3_task(*t), "backwards compatibility to TBB 2.0 tasks is broken" );
    if( dst_thread != 0 && dst_thread != my_affinity_id ) {
        task_proxy& proxy = (task_proxy&)allocate_task( sizeof(task_proxy), 
                                                      __TBB_CONTEXT_ARG(NULL, NULL) );
        // Mark as a proxy
        proxy.prefix().extra_state = es_task_proxy;
        proxy.outbox = &arena->mailbox(dst_thread);
        proxy.task_and_tag = intptr(t)|3;
        proxy.next_in_mailbox = NULL;
        ITT_NOTIFY( sync_releasing, proxy.outbox );
        // Mail the proxy - after this point t may be destroyed by another thread at any moment.
        proxy.outbox->push(proxy);
        return &proxy;
    }
    return t;
}

/** Conceptually, this method should be a member of class scheduler.
    But doing so would force us to publish class scheduler in the headers. */
void GenericScheduler::local_spawn( task& first, task*& next ) {
    __TBB_ASSERT( Governor::is_set(this), NULL );
    __TBB_ASSERT( assert_okay(), NULL );
    if ( &first.prefix().next == &next ) {
        // Single task is being spawned
        if ( arena_slot->tail == task_pool_size ) {
            // 1 compensates for head possibly temporarily incremented by a thief
            if ( arena_slot->head > 1 ) {
                // Move the busy part of the deque to the beginning of the allocated space
                acquire_task_pool();
                arena_slot->tail -= arena_slot->head;
                memmove( dummy_slot.task_pool, dummy_slot.task_pool + arena_slot->head, arena_slot->tail * sizeof(task*) );
                arena_slot->head = 0;
                release_task_pool();
            }
            else {
                grow( task_pool_size + 1 );
            }
        }
        dummy_slot.task_pool[arena_slot->tail] = prepare_for_spawning( &first );
        ITT_NOTIFY(sync_releasing, arena_slot);
        // The following store with release is required on ia64 only
        size_t new_tail = arena_slot->tail + 1;
        __TBB_store_with_release( arena_slot->tail, new_tail );
        __TBB_ASSERT ( arena_slot->tail <= task_pool_size, "task deque end was overwritten" );
    }
    else {
        // Task list is being spawned
        const size_t initial_capacity = 64;
        task *arr[initial_capacity];
        fast_reverse_vector<task*> tasks(arr, initial_capacity);
        task *t_next = NULL;
        for( task* t = &first; ; t = t_next ) {
            // After prepare_for_spawning returns t may already have been destroyed. 
            // So milk it while it is alive.
            bool end = &t->prefix().next == &next;
            t_next = t->prefix().next;
            tasks.push_back( prepare_for_spawning(t) );
            if( end )
                break;
        }
        size_t num_tasks = tasks.size();
        __TBB_ASSERT ( arena_index != null_arena_index, "invalid arena slot index" );
        if ( arena_slot->tail + num_tasks > task_pool_size ) {
            // 1 compensates for head possibly temporarily incremented by a thief
            size_t new_size = arena_slot->tail - arena_slot->head + num_tasks + 1;
            if ( new_size <= task_pool_size ) {
                // Move the busy part of the deque to the beginning of the allocated space
                acquire_task_pool();
                arena_slot->tail -= arena_slot->head;
                memmove( dummy_slot.task_pool, dummy_slot.task_pool + arena_slot->head, arena_slot->tail * sizeof(task*) );
                arena_slot->head = 0;
                release_task_pool();
            }
            else {
                grow( new_size );
            }
        }
#if DO_ITT_NOTIFY
        else {
            // The preceding if-branch issues the same ittnotify inside release_task_pool() or grow() methods
            ITT_NOTIFY(sync_releasing, arena_slot);
        }
#endif /* DO_ITT_NOTIFY */
        tasks.copy_memory( dummy_slot.task_pool + arena_slot->tail );
        // The following store with release is required on ia64 only
        size_t new_tail = arena_slot->tail + num_tasks;
        __TBB_store_with_release( arena_slot->tail, new_tail );
        __TBB_ASSERT ( arena_slot->tail <= task_pool_size, "task deque end was overwritten" );
    }
    if ( !in_arena() ) {
        if ( is_worker() )
            enter_arena();
        else
            try_enter_arena();
    }

    arena->mark_pool_full();
    __TBB_ASSERT( assert_okay(), NULL );

    TBB_TRACE(("%p.internal_spawn exit\n", this ));
}

void GenericScheduler::local_spawn_root_and_wait( task& first, task*& next ) {
    __TBB_ASSERT( Governor::is_set(this), NULL );
    __TBB_ASSERT( &first, NULL );
    auto_empty_task dummy( __TBB_CONTEXT_ARG(this, first.prefix().context) );
    internal::reference_count n = 0;
    for( task* t=&first; ; t=t->prefix().next ) {
        ++n;
        __TBB_ASSERT( !t->prefix().parent, "not a root task, or already running" );
        t->prefix().parent = &dummy;
        if( &t->prefix().next==&next ) break;
#if __TBB_EXCEPTIONS
        __TBB_ASSERT( t->prefix().context == t->prefix().next->prefix().context, 
                    "all the root tasks in list must share the same context");
#endif /* __TBB_EXCEPTIONS */
    }
    dummy.prefix().ref_count = n+1;
    if( n>1 )
        LocalSpawn( *first.prefix().next, next );
    TBB_TRACE(("spawn_root_and_wait((task_list*)%p): calling %p.loop\n",&first,this));
    wait_for_all( dummy, &first );
    TBB_TRACE(("spawn_root_and_wait((task_list*)%p): return\n",&first));
}

inline task* GenericScheduler::get_mailbox_task() {
    __TBB_ASSERT( my_affinity_id>0, "not in arena" );
    task* result = NULL;
    while( task_proxy* t = inbox.pop() ) {
        intptr tat = __TBB_load_with_acquire(t->task_and_tag);
        __TBB_ASSERT( tat==task_proxy::mailbox_bit || (tat==(tat|3)&&tat!=3), NULL );
        if( tat!=task_proxy::mailbox_bit && __TBB_CompareAndSwapW( &t->task_and_tag, task_proxy::pool_bit, tat )==tat ) {
            // Successfully grabbed the task, and left pool seeker with job of freeing the proxy.
            ITT_NOTIFY( sync_acquired, inbox.outbox() );
            result = (task*)(tat & ~3);
            result->prefix().owner = this;
            break;
        }
        free_task_proxy( *t );
    }
    return result;
}

inline task* GenericScheduler::strip_proxy( task_proxy* tp ) {
    __TBB_ASSERT( tp->prefix().extra_state==es_task_proxy, NULL );
    intptr tat = __TBB_load_with_acquire(tp->task_and_tag);
    if( (tat&3)==3 ) {
        // proxy is shared by a pool and a mailbox.
        // Attempt to transition it to "empty proxy in mailbox" state.
        if( __TBB_CompareAndSwapW( &tp->task_and_tag, task_proxy::mailbox_bit, tat )==tat ) {
            // Successfully grabbed the task, and left the mailbox with the job of freeing the proxy.
            return (task*)(tat&~3);
        }
        __TBB_ASSERT( tp->task_and_tag==task_proxy::pool_bit, NULL );
    } else {
        // We have exclusive access to the proxy
        __TBB_ASSERT( (tat&3)==task_proxy::pool_bit, "task did not come from pool?" );
        __TBB_ASSERT ( !(tat&~3), "Empty proxy in the pool contains non-zero task pointer" );
    }
#if TBB_USE_ASSERT
    tp->prefix().state = task::allocated;
#endif
    free_task_proxy( *tp );
    // Another thread grabbed the underlying task via their mailbox
    return NULL;
}

inline task* GenericScheduler::get_task() {
    task* result = NULL;
retry:
    --arena_slot->tail;
    __TBB_rel_acq_fence();
    if ( (intptr_t)arena_slot->head > (intptr_t)arena_slot->tail ) {
        acquire_task_pool();
        if ( (intptr_t)arena_slot->head <= (intptr_t)arena_slot->tail ) {
            // The thief backed off - grab the task
            __TBB_ASSERT_VALID_TASK_PTR( dummy_slot.task_pool[arena_slot->tail] );
            result = dummy_slot.task_pool[arena_slot->tail];
            __TBB_POISON_TASK_PTR( dummy_slot.task_pool[arena_slot->tail] );
        }
        else {
            __TBB_ASSERT ( arena_slot->head == arena_slot->tail + 1, "victim/thief arbitration algorithm failure" );
        }
        if ( (intptr_t)arena_slot->head < (intptr_t)arena_slot->tail ) {
            release_task_pool();
        }
        else {
            // In any case the deque is empty now, so compact it
            arena_slot->head = arena_slot->tail = 0;
            if ( in_arena() )
                leave_arena();
        }
    }
    else {
        __TBB_ASSERT_VALID_TASK_PTR( dummy_slot.task_pool[arena_slot->tail] );
        result = dummy_slot.task_pool[arena_slot->tail];
        __TBB_POISON_TASK_PTR( dummy_slot.task_pool[arena_slot->tail] );
    }
    if( result && is_proxy(*result) ) {
        result = strip_proxy((task_proxy*)result);
        if( !result ) {
            goto retry;
        }
        GATHER_STATISTIC( ++proxy_execute_count );
        // Following assertion should be true because TBB 2.0 tasks never specify affinity, and hence are not proxied.
        __TBB_ASSERT( is_version_3_task(*result), "backwards compatibility with TBB 2.0 broken" );
        // Task affinity has changed.
        innermost_running_task = result;
        result->note_affinity(my_affinity_id);
    }
    return result;
} // GenericScheduler::get_task

task* GenericScheduler::steal_task( ArenaSlot& victim_slot ) {
    task** victim_pool = lock_task_pool( &victim_slot );
    if ( !victim_pool )
        return NULL;
    const size_t none = ~0u;
    size_t first_skipped_proxy = none;
    task* result = NULL;
retry:
    ++victim_slot.head;
    __TBB_rel_acq_fence();
    if ( (intptr_t)victim_slot.head > (intptr_t)victim_slot.tail ) {
        --victim_slot.head;
    }
    else {
        __TBB_ASSERT_VALID_TASK_PTR( victim_pool[victim_slot.head - 1]);
        result = victim_pool[victim_slot.head - 1];
        if( is_proxy(*result) ) {
            task_proxy& tp = *static_cast<task_proxy*>(result);
            // If task will likely be grabbed by whom it was mailed to, skip it.
            if( (tp.task_and_tag & 3) == 3 && tp.outbox->recipient_is_idle() ) {
                if ( first_skipped_proxy == none )
                    first_skipped_proxy = victim_slot.head - 1;
                result = NULL;
                goto retry;
            }
        }
        __TBB_POISON_TASK_PTR(victim_pool[victim_slot.head - 1]);
    }
    if ( first_skipped_proxy != none ) {
        if ( result ) {
            victim_pool[victim_slot.head - 1] = victim_pool[first_skipped_proxy];
            __TBB_POISON_TASK_PTR( victim_pool[first_skipped_proxy] );
            __TBB_store_with_release( victim_slot.head, first_skipped_proxy + 1 );
        }
        else
            __TBB_store_with_release( victim_slot.head, first_skipped_proxy );
    }
    unlock_task_pool( &victim_slot, victim_pool );
    return result;
}


#define ConcurrentWaitsEnabled(t) (t.prefix().context->my_version_and_traits & task_group_context::concurrent_wait)
#define CancellationInfoPresent(t) (t->prefix().context->my_cancellation_requested)

#if TBB_USE_CAPTURED_EXCEPTION
    inline tbb_exception* TbbCurrentException( task_group_context*, tbb_exception* src) { return src->move(); }
    inline tbb_exception* TbbCurrentException( task_group_context*, captured_exception* src) { return src; }
#else
    // Using macro instead of an inline function here allows to avoid evaluation of the 
    // TbbCapturedException expression when exact propagation is enabled for the context.
    #define TbbCurrentException(context, TbbCapturedException) \
        context->my_version_and_traits & task_group_context::exact_exception    \
            ? tbb_exception_ptr::allocate()    \
            : tbb_exception_ptr::allocate( *(TbbCapturedException) );
#endif /* !TBB_USE_CAPTURED_EXCEPTION */

#define TbbRegisterCurrentException(context, TbbCapturedException) \
    if ( context->cancel_group_execution() ) {  \
        /* We are the first to signal cancellation, so store the exception that caused it. */  \
        context->my_exception = TbbCurrentException( context, TbbCapturedException ); \
    }

#define TbbCatchAll(context)  \
    catch ( tbb_exception& exc ) {  \
        TbbRegisterCurrentException( context, &exc );   \
    } catch ( std::exception& exc ) {   \
        TbbRegisterCurrentException( context, captured_exception::allocate(typeid(exc).name(), exc.what()) ); \
    } catch ( ... ) {   \
        TbbRegisterCurrentException( context, captured_exception::allocate("...", "Unidentified exception") );\
    }

template<typename SchedulerTraits>
void CustomScheduler<SchedulerTraits>::local_wait_for_all( task& parent, task* child ) {
    __TBB_ASSERT( Governor::is_set(this), NULL );
    if( child ) {
        child->prefix().owner = this;
    }
    __TBB_ASSERT( parent.ref_count() >= (child && child->parent() == &parent ? 2 : 1), "ref_count is too small" );
    __TBB_ASSERT( assert_okay(), NULL );
    // Using parent's refcount in sync_prepare (in the stealing loop below) is 
    // a workaround for TP. We need to name it here to display correctly in Ampl.
    if( SchedulerTraits::itt_possible )
        ITT_SYNC_CREATE(&parent.prefix().ref_count, SyncType_Scheduler, SyncObj_TaskStealingLoop);
#if __TBB_EXCEPTIONS
    __TBB_ASSERT( parent.prefix().context || (is_worker() && &parent == dummy_task), "parent task does not have context" );
#endif /* __TBB_EXCEPTIONS */
    task* t = child;
    // Constants all_work_done and all_local_work_done are actually unreacheable 
    // refcount values that prevent early quitting the dispatch loop. They are 
    // defined to be in the middle of the range of negative values representable 
    // by the reference_count type.
    static const reference_count 
        // For nested dispatch loops in masters and any dispatch loops in workers
        parents_work_done = 1,
        // For outermost dispatch loops in masters
        all_work_done = (reference_count)3 << (sizeof(reference_count) * 8 - 2),
        // For termination dispatch loops in masters
        all_local_work_done = all_work_done + 1;
    reference_count quit_point;
    if( innermost_running_task == dummy_task ) {
        // We are in the outermost task dispatch loop of a master thread,
        __TBB_ASSERT( !is_worker(), NULL );
        quit_point = &parent == dummy_task ? all_local_work_done : all_work_done;
    } else {
        quit_point = parents_work_done;
    }
    task* old_innermost_running_task = innermost_running_task;
#if __TBB_EXCEPTIONS
exception_was_caught:
    try {
#endif /* __TBB_EXCEPTIONS */
    // Outer loop steals tasks when necessary.
    for(;;) {
        // Middle loop evaluates tasks that are pulled off "array".
        do {
            // Inner loop evaluates tasks that are handed directly to us by other tasks.
            while(t) {
                __TBB_ASSERT( inbox.assert_is_idle(false), NULL );
#if TBB_USE_ASSERT
                __TBB_ASSERT(!is_proxy(*t),"unexpected proxy");
                __TBB_ASSERT( t->prefix().owner==this, NULL );
#if __TBB_EXCEPTIONS
                if ( !t->prefix().context->my_cancellation_requested ) 
#endif
                    __TBB_ASSERT( 1L<<t->state() & (1L<<task::allocated|1L<<task::ready|1L<<task::reexecute), NULL );
                __TBB_ASSERT(assert_okay(),NULL);
#endif /* TBB_USE_ASSERT */
                task* t_next = NULL;
                innermost_running_task = t;
                t->prefix().state = task::executing;
#if __TBB_EXCEPTIONS
                if ( !t->prefix().context->my_cancellation_requested )
#endif
                {
                    TBB_TRACE(("%p.wait_for_all: %p.execute\n",this,t));
                    GATHER_STATISTIC( ++execute_count );
                    t_next = t->execute();
#if STATISTICS
                    if (t_next) {
                        affinity_id next_affinity=t_next->prefix().affinity;
                        if (next_affinity != 0 && next_affinity != my_affinity_id)
                            GATHER_STATISTIC( ++proxy_bypass_count );
                    }
#endif
                }
                if( t_next ) {
                    __TBB_ASSERT( t_next->state()==task::allocated,
                                "if task::execute() returns task, it must be marked as allocated" );
                    // The store here has a subtle secondary effect - it fetches *t_next into cache.
                    t_next->prefix().owner = this;
                }
                __TBB_ASSERT(assert_okay(),NULL);
                switch( task::state_type(t->prefix().state) ) {
                    case task::executing: {
                        // this block was copied below to case task::recycle
                        // when making changes, check it too
                        task* s = t->parent();
                        __TBB_ASSERT( innermost_running_task==t, NULL );
                        __TBB_ASSERT( t->prefix().ref_count==0, "Task still has children after it has been executed" );
                        t->~task();
                        if( s ) {
                            if( tally_completion_of_one_predecessor(*s) ) {
#if TBB_USE_ASSERT
                                s->prefix().extra_state &= ~es_ref_count_active;
#endif /* TBB_USE_ASSERT */
                                s->prefix().owner = this;

                                if( !t_next ) {
                                    t_next = s;
                                } else {
                                    LocalSpawn( *s, s->prefix().next );
                                    __TBB_ASSERT(assert_okay(),NULL);
                                }
                            }
                        }
                        free_task<no_hint>( *t );
                        break;
                    }

                    case task::recycle: { // state set by recycle_as_safe_continuation()
                        t->prefix().state = task::allocated;
                        // for safe continuation, need atomically decrement ref_count;
                        // the block was copied from above case task::executing, and changed.
                        // Use "s" here as name for t, so that code resembles case task::executing more closely.
                        task* const& s = t;
                        if( tally_completion_of_one_predecessor(*s) ) {
                            // Unused load is put here for sake of inserting an "acquire" fence.
#if TBB_USE_ASSERT
                            s->prefix().extra_state &= ~es_ref_count_active;
                            __TBB_ASSERT( s->prefix().owner==this, "ownership corrupt?" );
#endif /* TBB_USE_ASSERT */
                            if( !t_next ) {
                                t_next = s;
                            } else {
                                LocalSpawn( *s, s->prefix().next );
                                __TBB_ASSERT(assert_okay(),NULL);
                            }
                        }
                        break;
                    }

                    case task::reexecute: // set by recycle_to_reexecute()
                        __TBB_ASSERT( t_next && t_next != t, "reexecution requires that method 'execute' return another task" );
                        TBB_TRACE(("%p.wait_for_all: put task %p back into array",this,t));
                        t->prefix().state = task::allocated;
                        LocalSpawn( *t, t->prefix().next );
                        __TBB_ASSERT(assert_okay(),NULL);
                        break;
#if TBB_USE_ASSERT
                    case task::allocated:
                        break;
                    case task::ready:
                        __TBB_ASSERT( false, "task is in READY state upon return from method execute()" );
                        break;
                    default:
                        __TBB_ASSERT( false, "illegal state" );
#else
                    default: // just to shut up some compilation warnings
                        break;
#endif /* TBB_USE_ASSERT */
                }

                t = t_next;
            } // end of scheduler bypass loop
            __TBB_ASSERT(assert_okay(),NULL);

            // If the parent's descendants are finished with and we are not in 
            // the outermost dispatch loop of a master thread, then we are done.
            // This is necessary to prevent unbounded stack growth in case of deep
            // wait_for_all nesting. 
            // Note that we cannot return from master's outermost dispatch loop 
            // until we process all the tasks in the local pool, since in case 
            // of multiple masters this could have left some of them forever 
            // waiting for their stolen children to be processed.
            if ( parent.prefix().ref_count == quit_point )
                break;
            t = get_task();
            __TBB_ASSERT(!t || !is_proxy(*t),"unexpected proxy");
#if TBB_USE_ASSERT
            __TBB_ASSERT(assert_okay(),NULL);
            if(t) {
                AssertOkay(*t);
                __TBB_ASSERT( t->prefix().owner==this, "thread got task that it does not own" );
            }
#endif /* TBB_USE_ASSERT */
        } while( t ); // end of local task array processing loop

        if ( quit_point == all_local_work_done ) {
            __TBB_ASSERT( arena_slot == &dummy_slot && arena_slot->head == 0 && arena_slot->tail == 0, NULL );
            innermost_running_task = old_innermost_running_task;
            return;
        }
        inbox.set_is_idle( true );
        __TBB_ASSERT( arena->prefix().number_of_workers>0||parent.prefix().ref_count==1, "deadlock detected" );
        // The state "failure_count==-1" is used only when itt_possible is true,
        // and denotes that a sync_prepare has not yet been issued.
        for( int failure_count = -static_cast<int>(SchedulerTraits::itt_possible);; ++failure_count) {
            if( parent.prefix().ref_count==1 ) {
                if( SchedulerTraits::itt_possible ) {
                    if( failure_count!=-1 ) {
                        ITT_NOTIFY(sync_prepare, &parent.prefix().ref_count);
                        // Notify Intel(R) Thread Profiler that thread has stopped spinning.
                        ITT_NOTIFY(sync_acquired, this);
                    }
                    ITT_NOTIFY(sync_acquired, &parent.prefix().ref_count);
                }
                inbox.set_is_idle( false );
                goto done;
            }
            // Try to steal a task from a random victim.
            size_t n = arena->prefix().limit;
            if( n>1 ) {
                if( !my_affinity_id || !(t=get_mailbox_task()) ) {
                    if ( !can_steal() )
                        goto fail;
                    size_t k = random.get() % (n-1);
                    ArenaSlot* victim = &arena->slot[k];
                    // The following condition excludes the master that might have 
                    // already taken our previous place in the arena from the list .
                    // of potential victims. But since such a situation can take 
                    // place only in case of significant oversubscription, keeping
                    // the checks simple seems to be preferable to complicating the code.
                    if( k >= arena_index )
                        ++victim;               // Adjusts random distribution to exclude self
                    t = steal_task( *victim );
                    if( !t ) goto fail;
                    if( is_proxy(*t) ) {
                        t = strip_proxy((task_proxy*)t);
                        if( !t ) goto fail;
                        GATHER_STATISTIC( ++proxy_steal_count );
                    }
                    GATHER_STATISTIC( ++steal_count );
                    if( is_version_3_task(*t) ) {
                        innermost_running_task = t;
                        t->note_affinity( my_affinity_id );
                    }
                } else {
                    GATHER_STATISTIC( ++mail_received_count );
                }
                __TBB_ASSERT(t,NULL);
#if __TBB_SCHEDULER_OBSERVER
                // No memory fence required for read of global_last_observer_proxy, because prior fence on steal/mailbox suffices.
                if( local_last_observer_proxy!=global_last_observer_proxy ) {
                    notify_entry_observers();
                }
#endif /* __TBB_SCHEDULER_OBSERVER */
                {
                    if( SchedulerTraits::itt_possible ) {
                        if( failure_count!=-1 ) {
                            // FIXME - might be victim, or might be selected from a mailbox
                            // Notify Intel(R) Thread Profiler that thread has stopped spinning.
                            ITT_NOTIFY(sync_acquired, this);
                            // FIXME - might be victim, or might be selected from a mailbox
                        }
                    }
                    __TBB_ASSERT(t,NULL);
                    inbox.set_is_idle( false );
                    break;
                }
            }
fail:
            if( SchedulerTraits::itt_possible && failure_count==-1 ) {
                // The first attempt to steal work failed, so notify Intel(R) Thread Profiler that
                // the thread has started spinning.  Ideally, we would do this notification
                // *before* the first failed attempt to steal, but at that point we do not
                // know that the steal will fail.
                ITT_NOTIFY(sync_prepare, this);
                failure_count = 0;
            }
            // Pause, even if we are going to yield, because the yield might return immediately.
            __TBB_Pause(PauseTime);
            int yield_threshold = 2*int(n);
            if( failure_count>=yield_threshold ) {
                __TBB_Yield();
                if( failure_count>=yield_threshold+100 ) {
                    if( !old_innermost_running_task && arena->check_if_pool_is_empty() ) {
                        // Current thread was created by RML and has nothing to do, so return it to the RML.
                        // For purposes of affinity support, the thread is considered idle while it is in RML.
                        // Restore innermost_running_task to its original value.
                        innermost_running_task = NULL;
                        return;
                    }
                    failure_count = yield_threshold;
                }
            }
        }
        __TBB_ASSERT(t,NULL);
        __TBB_ASSERT(!is_proxy(*t),"unexpected proxy");
        t->prefix().owner = this;
    } // end of stealing loop
#if __TBB_EXCEPTIONS
    } TbbCatchAll( t->prefix().context );

    if( task::state_type(t->prefix().state) == task::recycle ) { // state set by recycle_as_safe_continuation()
        t->prefix().state = task::allocated;
        // for safe continuation, need to atomically decrement ref_count;
        if( SchedulerTraits::itt_possible )
            ITT_NOTIFY(sync_releasing, &t->prefix().ref_count);
        if( __TBB_FetchAndDecrementWrelease(&t->prefix().ref_count)==1 ) {
            if( SchedulerTraits::itt_possible )
                ITT_NOTIFY(sync_acquired, &t->prefix().ref_count);
        }else{
            t = NULL;
        }
    }
    goto exception_was_caught;
#endif /* __TBB_EXCEPTIONS */
done:
    if ( !ConcurrentWaitsEnabled(parent) )
        parent.prefix().ref_count = 0;
#if TBB_USE_ASSERT
    parent.prefix().extra_state &= ~es_ref_count_active;
#endif /* TBB_USE_ASSERT */
    innermost_running_task = old_innermost_running_task;
#if __TBB_EXCEPTIONS
    __TBB_ASSERT(parent.prefix().context && dummy_task->prefix().context, NULL);
    task_group_context* parent_ctx = parent.prefix().context;
    if ( parent_ctx->my_cancellation_requested ) {
        task_group_context::exception_container_type *pe = parent_ctx->my_exception;
        if ( innermost_running_task == dummy_task && parent_ctx == dummy_task->prefix().context ) {
            // We are in the outermost task dispatch loop of a master thread, and 
            // the whole task tree has been collapsed. So we may clear cancellation data.
            parent_ctx->my_cancellation_requested = 0;
            __TBB_ASSERT(dummy_task->prefix().context == parent_ctx || !CancellationInfoPresent(dummy_task), 
                         "Unexpected exception or cancellation data in the dummy task");
            // If possible, add assertion that master's dummy task context does not have children
        }
        if ( pe )
            pe->throw_self();
    }
    __TBB_ASSERT(!is_worker() || !CancellationInfoPresent(dummy_task), 
                 "Worker's dummy task context modified");
    __TBB_ASSERT(innermost_running_task != dummy_task || !CancellationInfoPresent(dummy_task), 
                 "Unexpected exception or cancellation data in the master's dummy task");
#endif /* __TBB_EXCEPTIONS */
    __TBB_ASSERT( assert_okay(), NULL );
}

#undef CancellationInfoPresent

inline void GenericScheduler::do_enter_arena() {
    arena_slot = &arena->slot[arena_index];
    __TBB_ASSERT ( arena_slot->head == arena_slot->tail, "task deque of a free slot must be empty" );
    arena_slot->head = dummy_slot.head;
    arena_slot->tail = dummy_slot.tail;
    // Release signal on behalf of previously spawned tasks (when this thread was not in arena yet)
    ITT_NOTIFY(sync_releasing, arena_slot);
    __TBB_store_with_release( arena_slot->task_pool, dummy_slot.task_pool );
    // We'll leave arena only when it's empty, so clean up local instances of indices.
    dummy_slot.head = dummy_slot.tail = 0;
}

void GenericScheduler::enter_arena() {
    __TBB_ASSERT ( is_worker(), "only workers should use enter_arena()" );
    __TBB_ASSERT ( arena, "no arena: initialization not completed?" );
    __TBB_ASSERT ( !in_arena(), "worker already in arena?" );
    __TBB_ASSERT ( arena_index < arena->prefix().number_of_workers, "invalid worker arena slot index" );
    __TBB_ASSERT ( arena->slot[arena_index].task_pool == EmptyTaskPool, "someone else grabbed my arena slot?" );
    do_enter_arena();
}

void GenericScheduler::try_enter_arena() {
    __TBB_ASSERT ( !is_worker(), "only masters should use try_enter_arena()" );
    __TBB_ASSERT ( arena, "no arena: initialization not completed?" );
    __TBB_ASSERT ( !in_arena(), "master already in arena?" );
    __TBB_ASSERT ( arena_index >= arena->prefix().number_of_workers && 
                   arena_index < arena->prefix().number_of_slots, "invalid arena slot hint value" );


    size_t h = arena_index;
    // We do not lock task pool upon successful entering arena
    if( arena->slot[h].task_pool != EmptyTaskPool || 
        __TBB_CompareAndSwapW( &arena->slot[h].task_pool, (intptr_t)LockedTaskPool, 
                                                          (intptr_t)EmptyTaskPool ) != (intptr_t)EmptyTaskPool )
    {
        // Hinted arena slot is already busy, try some of the others at random
        unsigned first = arena->prefix().number_of_workers,
                 last = arena->prefix().number_of_slots;
        unsigned n = last - first - 1;
        /// \todo Is this limit reasonable?
        size_t max_attempts = last - first;
        for (;;) {
            size_t k = first + random.get() % n;
            if( k >= h )
                ++k;    // Adjusts random distribution to exclude previously tried slot
            h = k;
            if( arena->slot[h].task_pool == EmptyTaskPool && 
                __TBB_CompareAndSwapW( &arena->slot[h].task_pool, (intptr_t)LockedTaskPool, 
                                                                  (intptr_t)EmptyTaskPool ) == (intptr_t)EmptyTaskPool )
            {
                break;
            }
            if ( --max_attempts == 0 ) {
                // After so many attempts we are still unable to find a vacant arena slot.
                // Cease the vain effort and work outside of arena for a while.
                return;
            }
        }
    }
    // Successfully claimed a slot in the arena.
    ITT_NOTIFY(sync_acquired, &arena->slot[h]);
    __TBB_ASSERT ( arena->slot[h].task_pool == LockedTaskPool, "Arena slot is not actually acquired" );
    arena_index = h;
    do_enter_arena();
    attach_mailbox( affinity_id(h+1) );
}

void GenericScheduler::leave_arena() {
    __TBB_ASSERT( in_arena(), "Not in arena" );
    // Do not reset arena_index. It will be used to (attempt to) re-acquire the slot next time
    __TBB_ASSERT( &arena->slot[arena_index] == arena_slot, "Arena slot and slot index mismatch" );
    __TBB_ASSERT ( arena_slot->task_pool == LockedTaskPool, "Task pool must be locked when leaving arena" );
    __TBB_ASSERT ( arena_slot->head == arena_slot->tail, "Cannot leave arena when the task pool is not empty" );
    if ( !is_worker() ) {
        my_affinity_id = 0;
        inbox.detach();
    }
    ITT_NOTIFY(sync_releasing, &arena->slot[arena_index]);
    __TBB_store_with_release( arena_slot->task_pool, EmptyTaskPool );
    arena_slot = &dummy_slot;
}


GenericScheduler* GenericScheduler::create_worker( Arena& a, size_t index ) {
    GenericScheduler* s = GenericScheduler::allocate_scheduler(&a);

    // Put myself into the arena
#if __TBB_EXCEPTIONS
    s->dummy_task->prefix().context = &dummy_context;
    // Sync up the local cancellation state with the global one. No need for fence here.
    s->local_cancel_count = global_cancel_count;
#endif /* __TBB_EXCEPTIONS */
    s->attach_mailbox( index+1 );
    s->arena_index = index;
    s->init_stack_info();

    __TBB_store_with_release( a.prefix().worker_list[index].scheduler, s );
    return s;
}


GenericScheduler* GenericScheduler::create_master( Arena* arena ) {
    GenericScheduler* s = GenericScheduler::allocate_scheduler( arena );
    task& t = *s->dummy_task;
    s->innermost_running_task = &t;
    t.prefix().ref_count = 1;
    Governor::sign_on(s);
#if __TBB_EXCEPTIONS
    // Context to be used by root tasks by default (if the user has not specified one).
    // Allocation is done by NFS allocator because we cannot reuse memory allocated 
    // for task objects since the free list is empty at the moment.
    t.prefix().context = new ( NFS_Allocate(sizeof(task_group_context), 1, NULL) ) task_group_context(task_group_context::isolated);
    scheduler_list_node_t &node = s->my_node;
    {
        mutex::scoped_lock lock(the_scheduler_list_mutex);
        node.my_next = the_scheduler_list_head.my_next;
        node.my_prev = &the_scheduler_list_head;
        the_scheduler_list_head.my_next->my_prev = &node;
        the_scheduler_list_head.my_next = &node;
#endif /* __TBB_EXCEPTIONS */
        unsigned last = arena->prefix().number_of_slots,
                 cur_limit = arena->prefix().limit;
        // This slot index assignment is just a hint to ...
        if ( cur_limit < last ) {
            // ... to prevent competition between the first few masters.
            s->arena_index = cur_limit++;
            // In the absence of exception handling this code is a subject to data 
            // race in case of multiple masters concurrently entering empty arena.
            // But it does not affect correctness, and can only result in a few 
            // masters competing for the same arena slot during the first acquisition.
            // The cost of competition is low in comparison to that of oversubscription.
            arena->prefix().limit = cur_limit;
        }
        else {
            // ... to minimize the probability of competition between multiple masters.
            unsigned first = arena->prefix().number_of_workers;
            s->arena_index = first + s->random.get() % (last - first);
        }
#if __TBB_EXCEPTIONS
    }
#endif
    s->init_stack_info();
#if __TBB_EXCEPTIONS
    // Sync up the local cancellation state with the global one. No need for fence here.
    s->local_cancel_count = global_cancel_count;
#endif
    __TBB_ASSERT( &task::self()==&t, NULL );
#if __TBB_SCHEDULER_OBSERVER
    // Process any existing observers.
    s->notify_entry_observers();
#endif /* __TBB_SCHEDULER_OBSERVER */
    return s;
}


void GenericScheduler::cleanup_worker( void* arg ) {
    TBB_TRACE(("%p.cleanup_worker entered\n",arg));
    GenericScheduler& s = *(GenericScheduler*)arg;
    __TBB_ASSERT( s.dummy_slot.task_pool, "cleaning up worker with missing task pool" );
#if __TBB_SCHEDULER_OBSERVER
    s.notify_exit_observers(/*is_worker=*/true);
#endif /* __TBB_SCHEDULER_OBSERVER */
    __TBB_ASSERT( s.arena_slot->task_pool == EmptyTaskPool || s.arena_slot->head == s.arena_slot->tail, 
                  "worker has unfinished work at run down" );
    s.free_scheduler();
}

void GenericScheduler::cleanup_master() {
    TBB_TRACE(("%p.cleanup_master entered\n",this));
    GenericScheduler& s = *this; // for similarity with cleanup_worker
    __TBB_ASSERT( s.dummy_slot.task_pool, "cleaning up master with missing task pool" );
#if __TBB_SCHEDULER_OBSERVER
    s.notify_exit_observers(/*is_worker=*/false);
#endif /* __TBB_SCHEDULER_OBSERVER */
    if ( !is_local_task_pool_empty() ) {
        __TBB_ASSERT ( Governor::is_set(this), "TLS slot is cleared before the task pool cleanup" );
        s.wait_for_all( *dummy_task, NULL );
        __TBB_ASSERT ( Governor::is_set(this), "Other thread reused our TLS key during the task pool cleanup" );
    }
    s.free_scheduler();
    Governor::finish_with_arena();
}

//------------------------------------------------------------------------
// UnpaddedArenaPrefix
//------------------------------------------------------------------------
inline Arena& UnpaddedArenaPrefix::arena() {
    return *static_cast<Arena*>(static_cast<void*>( static_cast<ArenaPrefix*>(this)+1 ));
}

void UnpaddedArenaPrefix::process( job& j ) {
    GenericScheduler& s = static_cast<GenericScheduler&>(j);
    __TBB_ASSERT( Governor::is_set(&s), NULL );
    __TBB_ASSERT( !s.innermost_running_task, NULL );
    s.wait_for_all(*s.dummy_task,NULL);
    __TBB_ASSERT( !s.innermost_running_task, NULL );
}

void UnpaddedArenaPrefix::cleanup( job& j ) {
    GenericScheduler& s = static_cast<GenericScheduler&>(j);
    GenericScheduler::cleanup_worker( &s );
}

void UnpaddedArenaPrefix::open_connection_to_rml() {
    __TBB_ASSERT( !server, NULL );
    __TBB_ASSERT( stack_size>0, NULL );
    if( !use_private_rml ) {
        ::rml::factory::status_type status = rml_server_factory.make_server( server, *this );
        if( status==::rml::factory::st_success ) {
            __TBB_ASSERT( server, NULL );
            return;
        }
        use_private_rml = true;
        fprintf(stderr,"warning from TBB: make_server failed with status %x, falling back on private rml",status);
    }
    server = rml::make_private_server( *this );
}

void UnpaddedArenaPrefix::acknowledge_close_connection() {
    arena().free_arena();
}

::rml::job* UnpaddedArenaPrefix::create_one_job() {
    GenericScheduler* s = GenericScheduler::create_worker( arena(), next_job_index++ );
    Governor::sign_on(s);
    return s;
}

//------------------------------------------------------------------------
// Methods of allocate_root_proxy
//------------------------------------------------------------------------
task& allocate_root_proxy::allocate( size_t size ) {
    internal::GenericScheduler* v = Governor::local_scheduler();
    __TBB_ASSERT( v, "thread did not activate a task_scheduler_init object?" );
#if __TBB_EXCEPTIONS
    task_prefix& p = v->innermost_running_task->prefix();
#endif
    // New root task becomes part of the currently running task's cancellation context
    return v->allocate_task( size, __TBB_CONTEXT_ARG(NULL, p.context) );
}

void allocate_root_proxy::free( task& task ) {
    internal::GenericScheduler* v = Governor::local_scheduler();
    __TBB_ASSERT( v, "thread does not have initialized task_scheduler_init object?" );
#if __TBB_EXCEPTIONS
    // No need to do anything here as long as there is no context -> task connection
#endif /* __TBB_EXCEPTIONS */
    v->free_task<GenericScheduler::is_local>( task );
}

#if __TBB_EXCEPTIONS
//------------------------------------------------------------------------
// Methods of allocate_root_with_context_proxy
//------------------------------------------------------------------------
task& allocate_root_with_context_proxy::allocate( size_t size ) const {
    internal::GenericScheduler* v = Governor::local_scheduler();
    __TBB_ASSERT( v, "thread did not activate a task_scheduler_init object?" );
    task_prefix& p = v->innermost_running_task->prefix();
    task& t = v->allocate_task( size, __TBB_CONTEXT_ARG(NULL, &my_context) );
    // The supported usage model prohibits concurrent initial binding. Thus we 
    // do not need interlocked operations or fences here.
    if ( my_context.my_kind == task_group_context::binding_required ) {
        __TBB_ASSERT ( my_context.my_owner, "Context without owner" );
        __TBB_ASSERT ( !my_context.my_parent, "Parent context set before initial binding" );
        // If we are in the outermost task dispatch loop of a master thread, then
        // there is nothing to bind this context to, and we skip the binding part.
        if ( v->innermost_running_task != v->dummy_task ) {
            // By not using the fence here we get faster code in case of normal execution 
            // flow in exchange of a bit higher probability that in cases when cancellation 
            // is in flight we will take deeper traversal branch. Normally cache coherency 
            // mechanisms are efficient enough to deliver updated value most of the time.
            uintptr_t local_count_snapshot = ((GenericScheduler*)my_context.my_owner)->local_cancel_count;
            __TBB_store_with_release(my_context.my_parent, p.context);
            uintptr_t global_count_snapshot = __TBB_load_with_acquire(global_cancel_count);
            if ( !my_context.my_cancellation_requested ) {
                if ( local_count_snapshot == global_count_snapshot ) {
                    // It is possible that there is active cancellation request in our 
                    // parents chain. Fortunately the equality of the local and global 
                    // counters means that if this is the case it's already been propagated
                    // to our parent.
                    my_context.my_cancellation_requested = p.context->my_cancellation_requested;
                } else {
                    // Another thread was propagating cancellation request at the moment 
                    // when we set our parent, but since we do not use locks we could've 
                    // been skipped. So we have to make sure that we get the cancellation 
                    // request if one of our ancestors has been canceled.
                    my_context.propagate_cancellation_from_ancestors();
                }
            }
        }
        my_context.my_kind = task_group_context::binding_completed;
    }
    // else the context either has already been associated with its parent or is isolated
    return t;
}

void allocate_root_with_context_proxy::free( task& task ) const {
    internal::GenericScheduler* v = Governor::local_scheduler();
    __TBB_ASSERT( v, "thread does not have initialized task_scheduler_init object?" );
    // No need to do anything here as long as unbinding is performed by context destructor only.
    v->free_task<GenericScheduler::is_local>( task );
}
#endif /* __TBB_EXCEPTIONS */

//------------------------------------------------------------------------
// Methods of allocate_continuation_proxy
//------------------------------------------------------------------------
task& allocate_continuation_proxy::allocate( size_t size ) const {
    task& t = *((task*)this);
    __TBB_ASSERT( AssertOkay(t), NULL );
    GenericScheduler* s = Governor::local_scheduler();
    task* parent = t.parent();
    t.prefix().parent = NULL;
    return s->allocate_task( size, __TBB_CONTEXT_ARG(parent, t.prefix().context) );
}

void allocate_continuation_proxy::free( task& mytask ) const {
    // Restore the parent as it was before the corresponding allocate was called.
    ((task*)this)->prefix().parent = mytask.parent();
    Governor::local_scheduler()->free_task<GenericScheduler::is_local>(mytask);
}

//------------------------------------------------------------------------
// Methods of allocate_child_proxy
//------------------------------------------------------------------------
task& allocate_child_proxy::allocate( size_t size ) const {
    task& t = *((task*)this);
    __TBB_ASSERT( AssertOkay(t), NULL );
    GenericScheduler* s = Governor::local_scheduler();
    return s->allocate_task( size, __TBB_CONTEXT_ARG(&t, t.prefix().context) );
}

void allocate_child_proxy::free( task& mytask ) const {
    Governor::local_scheduler()->free_task<GenericScheduler::is_local>(mytask);
}

//------------------------------------------------------------------------
// Methods of allocate_additional_child_of_proxy
//------------------------------------------------------------------------
task& allocate_additional_child_of_proxy::allocate( size_t size ) const {
    __TBB_ASSERT( AssertOkay(self), NULL );
    parent.increment_ref_count();
    GenericScheduler* s = Governor::local_scheduler();
    return s->allocate_task( size, __TBB_CONTEXT_ARG(&parent, parent.prefix().context) );
}

void allocate_additional_child_of_proxy::free( task& task ) const {
    // Undo the increment.  We do not check the result of the fetch-and-decrement.
    // We could consider be spawning the task if the fetch-and-decrement returns 1.
    // But we do not know that was the programmer's intention.
    // Furthermore, if it was the programmer's intention, the program has a fundamental
    // race condition (that we warn about in Reference manual), because the
    // reference count might have become zero before the corresponding call to
    // allocate_additional_child_of_proxy::allocate.
    parent.internal_decrement_ref_count();
    Governor::local_scheduler()->free_task<GenericScheduler::is_local>(task);
}

//------------------------------------------------------------------------
// Support for auto_partitioner
//------------------------------------------------------------------------
size_t get_initial_auto_partitioner_divisor() {
    const size_t X_FACTOR = 4;
    return X_FACTOR * (Governor::number_of_workers_in_arena()+1);
}

//------------------------------------------------------------------------
// Methods of affinity_partitioner_base_v3
//------------------------------------------------------------------------
void affinity_partitioner_base_v3::resize( unsigned factor ) {
    // Check factor to avoid asking for number of workers while there might be no arena.
    size_t new_size = factor ? factor*(Governor::number_of_workers_in_arena()+1) : 0;
    if( new_size!=my_size ) {
        if( my_array ) {
            NFS_Free( my_array );
            // Following two assignments must be done here for sake of exception safety.
            my_array = NULL;
            my_size = 0;
        } 
        if( new_size ) {
            my_array = static_cast<affinity_id*>(NFS_Allocate(new_size,sizeof(affinity_id), NULL ));
            memset( my_array, 0, sizeof(affinity_id)*new_size );
            my_size = new_size;
        } 
    } 
}

} // namespace internal

using namespace tbb::internal;

#if __TBB_EXCEPTIONS

//------------------------------------------------------------------------
// captured_exception
//------------------------------------------------------------------------

inline 
void copy_string ( char*& dst, const char* src ) {
    if ( src ) {
        size_t len = strlen(src) + 1;
        dst = (char*)allocate_via_handler_v3(len);
        strncpy (dst, src, len);
    }
    else
        dst = NULL;
}

void captured_exception::set ( const char* name, const char* info ) throw()
{
    copy_string(const_cast<char*&>(my_exception_name), name);
    copy_string(const_cast<char*&>(my_exception_info), info);
}

void captured_exception::clear () throw() {
    deallocate_via_handler_v3 (const_cast<char*>(my_exception_name));
    deallocate_via_handler_v3 (const_cast<char*>(my_exception_info));
}

captured_exception* captured_exception::move () throw() {
    captured_exception *e = (captured_exception*)allocate_via_handler_v3(sizeof(captured_exception));
    if ( e ) {
        ::new (e) captured_exception();
        e->my_exception_name = my_exception_name;
        e->my_exception_info = my_exception_info;
        e->my_dynamic = true;
        my_exception_name = my_exception_info = NULL;
    }
    return e;
}

void captured_exception::destroy () throw() {
    __TBB_ASSERT ( my_dynamic, "Method destroy can be used only on objects created by clone or allocate" );
    if ( my_dynamic ) {
        this->captured_exception::~captured_exception();
        deallocate_via_handler_v3 (this);
    }
}

captured_exception* captured_exception::allocate ( const char* name, const char* info ) {
    captured_exception *e = (captured_exception*)allocate_via_handler_v3( sizeof(captured_exception) );
    if ( e ) {
        ::new (e) captured_exception(name, info);
        e->my_dynamic = true;
    }
    return e;
}

const char* captured_exception::name() const throw() {
    return my_exception_name;
}

const char* captured_exception::what() const throw() {
    return my_exception_info;
}


//------------------------------------------------------------------------
// tbb_exception_ptr
//------------------------------------------------------------------------

#if !TBB_USE_CAPTURED_EXCEPTION

namespace internal {

template<typename T>
tbb_exception_ptr* AllocateExceptionContainer( const T& src ) {
    tbb_exception_ptr *eptr = (tbb_exception_ptr*)allocate_via_handler_v3( sizeof(tbb_exception_ptr) );
    if ( eptr )
        new (eptr) tbb_exception_ptr(src);
    return eptr;
}

tbb_exception_ptr* tbb_exception_ptr::allocate () {
    return AllocateExceptionContainer( std::current_exception() );
}

tbb_exception_ptr* tbb_exception_ptr::allocate ( const tbb_exception& ) {
    return AllocateExceptionContainer( std::current_exception() );
}

tbb_exception_ptr* tbb_exception_ptr::allocate ( captured_exception& src ) {
    tbb_exception_ptr *res = AllocateExceptionContainer( src );
    src.destroy();
    return res;
}

void tbb_exception_ptr::destroy () throw() {
    this->tbb_exception_ptr::~tbb_exception_ptr();
    deallocate_via_handler_v3 (this);
}

} // namespace internal
#endif /* !TBB_USE_CAPTURED_EXCEPTION */


//------------------------------------------------------------------------
// task_group_context
//------------------------------------------------------------------------

task_group_context::~task_group_context () {
    if ( my_kind != isolated ) {
        GenericScheduler *s = (GenericScheduler*)my_owner;
        __TBB_ASSERT ( Governor::is_set(s), "Task group context is destructed by wrong thread" );
        my_node.my_next->my_prev = my_node.my_prev;
        uintptr_t local_count_snapshot = s->local_cancel_count;
        my_node.my_prev->my_next = my_node.my_next;
        __TBB_rel_acq_fence();
        if ( local_count_snapshot != global_cancel_count ) {
            // Another thread was propagating cancellation request when we removed
            // ourselves from the list. We must ensure that it does not access us 
            // when this destructor finishes. We'll be able to acquire the lock 
            // below only after the other thread finishes with us.
            spin_mutex::scoped_lock lock(s->context_list_mutex);
        }
    }
#if TBB_USE_DEBUG
    my_version_and_traits = 0xDeadBeef;
#endif /* TBB_USE_DEBUG */
    if ( my_exception )
        my_exception->destroy();
}

void task_group_context::init () {
    __TBB_ASSERT ( sizeof(uintptr_t) < 32, "Layout of my_version_and_traits must be reconsidered on this platform" );
    __TBB_ASSERT ( sizeof(task_group_context) == 2 * NFS_MaxLineSize, "Context class has wrong size - check padding and members alignment" );
    __TBB_ASSERT ( (uintptr_t(this) & (sizeof(my_cancellation_requested) - 1)) == 0, "Context is improperly aligned" );
    __TBB_ASSERT ( my_kind == isolated || my_kind == bound, "Context can be created only as isolated or bound" );
    my_parent = NULL;
    my_cancellation_requested = 0;
    my_exception = NULL;
    if ( my_kind == bound ) {
        GenericScheduler *s = Governor::local_scheduler();
        my_owner = s;
        __TBB_ASSERT ( my_owner, "Thread has not activated a task_scheduler_init object?" );
        // Backward links are used by this thread only, thus no fences are necessary
        my_node.my_prev = &s->context_list_head;
        s->context_list_head.my_next->my_prev = &my_node;
        // The only operation on the thread local list of contexts that may be performed 
        // concurrently is its traversal by another thread while propagating cancellation
        // request. Therefore the release fence below is necessary to ensure that the new 
        // value of my_node.my_next is visible to the traversing thread 
        // after it reads new value of v->context_list_head.my_next.
        my_node.my_next = s->context_list_head.my_next;
        __TBB_store_with_release(s->context_list_head.my_next, &my_node);
    }
}

bool task_group_context::cancel_group_execution () {
    __TBB_ASSERT ( my_cancellation_requested == 0 || my_cancellation_requested == 1, "Invalid cancellation state");
    if ( my_cancellation_requested || __TBB_CompareAndSwapW(&my_cancellation_requested, 1, 0) ) {
        // This task group has already been canceled
        return false;
    }
    Governor::local_scheduler()->propagate_cancellation(this);
    return true;
}

bool task_group_context::is_group_execution_cancelled () const {
    return my_cancellation_requested != 0;
}

// IMPORTANT: It is assumed that this method is not used concurrently!
void task_group_context::reset () {
    //! \todo Add assertion that this context does not have children
    // No fences are necessary since this context can be accessed from another thread
    // only after stealing happened (which means necessary fences were used).
    if ( my_exception )  {
        my_exception->destroy();
        my_exception = NULL;
    }
    my_cancellation_requested = 0;
}

void task_group_context::propagate_cancellation_from_ancestors () {
    task_group_context *parent = my_parent;
    while ( parent && !parent->my_cancellation_requested )
        parent = parent->my_parent;
    if ( parent ) {
        // One of our ancestor groups was canceled. Cancel all its descendants.
        task_group_context *ctx = this;
        do {
            __TBB_store_with_release(ctx->my_cancellation_requested, 1);
            ctx = ctx->my_parent;
        } while ( ctx != parent );
    }
}

void task_group_context::register_pending_exception () {
    if ( my_cancellation_requested )
        return;
    try {
        throw;
    } TbbCatchAll( this );
}

#endif /* __TBB_EXCEPTIONS */

//------------------------------------------------------------------------
// task
//------------------------------------------------------------------------

void task::internal_set_ref_count( int count ) {
    __TBB_ASSERT( count>=0, "count must not be negative" );
    __TBB_ASSERT( !(prefix().extra_state&GenericScheduler::es_ref_count_active), "ref_count race detected" );
    ITT_NOTIFY(sync_releasing, &prefix().ref_count);
    prefix().ref_count = count;
}

internal::reference_count task::internal_decrement_ref_count() {
    ITT_NOTIFY( sync_releasing, &prefix().ref_count );
    internal::reference_count k = __TBB_FetchAndDecrementWrelease( &prefix().ref_count );
    __TBB_ASSERT( k>=1, "task's reference count underflowed" );
    if( k==1 )
        ITT_NOTIFY( sync_acquired, &prefix().ref_count );
    return k-1;
}

task& task::self() {
    GenericScheduler *v = Governor::local_scheduler();
    __TBB_ASSERT( v->assert_okay(), NULL );
    __TBB_ASSERT( v->innermost_running_task, NULL );
    return *v->innermost_running_task;
}

bool task::is_owned_by_current_thread() const {
    return true;
}

void task::destroy( task& victim ) {
    __TBB_ASSERT( victim.prefix().ref_count== (ConcurrentWaitsEnabled(victim) ? 1 : 0), "Task being destroyed must not have children" );
    __TBB_ASSERT( victim.state()==task::allocated, "illegal state for victim task" );
    task* parent = victim.parent();
    victim.~task();
    if( parent ) {
        __TBB_ASSERT( parent->state()==task::allocated, "attempt to destroy child of running or corrupted parent?" );
        parent->internal_decrement_ref_count();
    }
    Governor::local_scheduler()->free_task<GenericScheduler::no_hint>( victim );
}

void task::spawn_and_wait_for_all( task_list& list ) {
    scheduler* s = Governor::local_scheduler();
    task* t = list.first;
    if( t ) {
        if( &t->prefix().next!=list.next_ptr )
            s->spawn( *t->prefix().next, *list.next_ptr );
        list.clear();
    }
    s->wait_for_all( *this, t );
}

/** Defined out of line so that compiler does not replicate task's vtable. 
    It's pointless to define it inline anyway, because all call sites to it are virtual calls
    that the compiler is unlikely to optimize. */
void task::note_affinity( affinity_id ) {
}

//------------------------------------------------------------------------
// task_scheduler_init
//------------------------------------------------------------------------

/** Left out-of-line for the sake of the backward binary compatibility **/
void task_scheduler_init::initialize( int number_of_threads ) {
    initialize( number_of_threads, 0 );
}

void task_scheduler_init::initialize( int number_of_threads, stack_size_type thread_stack_size ) {
    if( number_of_threads!=deferred ) {
        __TBB_ASSERT( !my_scheduler, "task_scheduler_init already initialized" );
        __TBB_ASSERT( number_of_threads==-1 || number_of_threads>=1,
                    "number_of_threads for task_scheduler_init must be -1 or positive" );
        my_scheduler = Governor::init_scheduler( number_of_threads, thread_stack_size );
    } else {
        __TBB_ASSERT( !thread_stack_size, "deferred initialization ignores stack size setting" );
    }
}

void task_scheduler_init::terminate() {
    GenericScheduler* s = static_cast<GenericScheduler*>(my_scheduler);
    my_scheduler = NULL;
    __TBB_ASSERT( s, "task_scheduler_init::terminate without corresponding task_scheduler_init::initialize()");
    Governor::terminate_scheduler(s);
}

int task_scheduler_init::default_num_threads() {
    // No memory fence required, because at worst each invoking thread calls NumberOfHardwareThreads.
    int n = DefaultNumberOfThreads;
    if( !n ) {
        DefaultNumberOfThreads = n = DetectNumberOfWorkers();
    }
    return n;
}

#if __TBB_SCHEDULER_OBSERVER
//------------------------------------------------------------------------
// Methods of observer_proxy
//------------------------------------------------------------------------
namespace internal {

#if TBB_USE_ASSERT
static atomic<int> observer_proxy_count;

struct check_observer_proxy_count {
    ~check_observer_proxy_count() {
        if( observer_proxy_count!=0 ) {
            fprintf(stderr,"warning: leaked %ld observer_proxy objects\n", long(observer_proxy_count));
        }
    }
};

static check_observer_proxy_count the_check_observer_proxy_count;
#endif /* TBB_USE_ASSERT */

observer_proxy::observer_proxy( task_scheduler_observer_v3& tso ) : next(NULL), observer(&tso) {
#if TBB_USE_ASSERT
    ++observer_proxy_count;
#endif /* TBB_USE_ASSERT */
    // 1 for observer
    gc_ref_count = 1;
    {
        // Append to the global list
        task_scheduler_observer_mutex_scoped_lock lock(the_task_scheduler_observer_mutex.begin()[0],/*is_writer=*/true);
        observer_proxy* p = global_last_observer_proxy;
        prev = p;
        if( p ) 
            p->next=this;
        else 
            global_first_observer_proxy = this;
        global_last_observer_proxy = this;
    }
}

void observer_proxy::remove_from_list() {
    // Take myself off the global list.  
    if( next ) 
        next->prev = prev;
    else 
        global_last_observer_proxy = prev;
    if( prev )
        prev->next = next;
    else 
        global_first_observer_proxy = next;
#if TBB_USE_ASSERT
    poison_pointer(prev);
    poison_pointer(next);
    gc_ref_count = -666;
#endif /* TBB_USE_ASSERT */
}

void observer_proxy::remove_ref_slow() {
    int r = gc_ref_count;
    while(r>1) {
        __TBB_ASSERT( r!=0, NULL );
        int r_old = gc_ref_count.compare_and_swap(r-1,r);
        if( r_old==r ) {
            // Successfully decremented count.
            return;
        } 
        r = r_old;
    } 
    __TBB_ASSERT( r==1, NULL );
    // Reference count might go to zero
    {
        task_scheduler_observer_mutex_scoped_lock lock(the_task_scheduler_observer_mutex.begin()[0],/*is_writer=*/true);
        r = --gc_ref_count;
        if( !r ) {
            remove_from_list();
        } 
    }
    if( !r ) {
        __TBB_ASSERT( gc_ref_count == -666, NULL );
#if TBB_USE_ASSERT
        --observer_proxy_count;
#endif /* TBB_USE_ASSERT */
        delete this;
    }
}

observer_proxy* observer_proxy::process_list( observer_proxy* local_last, bool is_worker, bool is_entry ) {
    // Pointer p marches though the list.
    // If is_entry, start with our previous list position, otherwise start at beginning of list.
    observer_proxy* p = is_entry ? local_last : NULL;
    for(;;) { 
        task_scheduler_observer* tso=NULL;
        // Hold lock on list only long enough to advance to next proxy in list.
        { 
            task_scheduler_observer_mutex_scoped_lock lock(the_task_scheduler_observer_mutex.begin()[0],/*is_writer=*/false);
            do {
                if( local_last && local_last->observer ) {
                    // 2 = 1 for observer and 1 for local_last
                    __TBB_ASSERT( local_last->gc_ref_count>=2, NULL );  
                    // Can decrement count quickly, because it cannot become zero here.
                    --local_last->gc_ref_count;
                    local_last = NULL;
                } else {
                    // Use slow form of decrementing the reference count, after lock is released.
                }  
                if( p ) {
                    // We were already processing the list.
                    if( observer_proxy* q = p->next ) {
                        // Step to next item in list.
                        p=q;
                    } else {
                        // At end of list.
                        if( is_entry ) {  
                            // Remember current position in the list, so we can start at on the next call.
                            ++p->gc_ref_count;
                        } else {
                            // Finishin running off the end of the list
                            p=NULL;
                        }
                        goto done;
                    }
                } else {
                    // Starting pass through the list
                    p = global_first_observer_proxy;
                    if( !p ) 
                        goto done;
                } 
                tso = p->observer;
            } while( !tso );
            ++p->gc_ref_count;
            ++tso->my_busy_count;
        }
        __TBB_ASSERT( !local_last || p!=local_last, NULL );
        if( local_last )
            local_last->remove_ref_slow();
        // Do not hold any locks on the list while calling user's code.
        try {    
            if( is_entry )
                tso->on_scheduler_entry( is_worker );
            else
                tso->on_scheduler_exit( is_worker );
        } catch(...) {
            // Suppress exception, because user routines are supposed to be observing, not changing
            // behavior of a master or worker thread.
#if TBB_USE_ASSERT
            fprintf(stderr,"warning: %s threw exception\n",is_entry?"on_scheduler_entry":"on_scheduler_exit"); 
#endif /* __TBB_USE_ASSERT */        
        }
        intptr bc = --tso->my_busy_count;
        __TBB_ASSERT_EX( bc>=0, "my_busy_count underflowed" );
        local_last = p;
    }
done:
    // Return new value to be used as local_last next time.
    if( local_last )
        local_last->remove_ref_slow();
    __TBB_ASSERT( !p || is_entry, NULL );
    return p;
}

void task_scheduler_observer_v3::observe( bool state ) {
    if( state ) {
        if( !my_proxy ) {
            if( !__TBB_InitOnce::initialization_done() )
                DoOneTimeInitializations();
            my_busy_count = 0;
            my_proxy = new observer_proxy(*this);
            if( GenericScheduler* s = Governor::local_scheduler() ) {
                // Notify newly created observer of its own thread.
                // Any other pending observers are notified too.
                s->notify_entry_observers();
            }
        } 
    } else {
        if( observer_proxy* proxy = my_proxy ) {
            my_proxy = NULL;
            __TBB_ASSERT( proxy->gc_ref_count>=1, "reference for observer missing" );
            {
                task_scheduler_observer_mutex_scoped_lock lock(the_task_scheduler_observer_mutex.begin()[0],/*is_writer=*/true);
                proxy->observer = NULL;
            }
            proxy->remove_ref_slow();
            while( my_busy_count ) {
                __TBB_Yield();
            }
        }
    }
}

} // namespace internal
#endif /* __TBB_SCHEDULER_OBSERVER */

} // namespace tbb


