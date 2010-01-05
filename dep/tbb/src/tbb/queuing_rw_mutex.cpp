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

/** Before making any changes in the implementation, please emulate algorithmic changes
    with SPIN tool using <TBB directory>/tools/spin_models/ReaderWriterMutex.pml.
    There could be some code looking as "can be restructured" but its structure does matter! */

#include "tbb/tbb_machine.h"
#include "tbb/tbb_stddef.h"
#include "tbb/tbb_machine.h"
#include "tbb/queuing_rw_mutex.h"
#include "itt_notify.h"


namespace tbb {

using namespace internal;

//! Flag bits in a state_t that specify information about a locking request.
enum state_t_flags {
    STATE_NONE = 0,
    STATE_WRITER = 1,
    STATE_READER = 1<<1,
    STATE_READER_UNBLOCKNEXT = 1<<2,
    STATE_COMBINED_WAITINGREADER = STATE_READER | STATE_READER_UNBLOCKNEXT,
    STATE_ACTIVEREADER = 1<<3,
    STATE_COMBINED_READER = STATE_COMBINED_WAITINGREADER | STATE_ACTIVEREADER,
    STATE_UPGRADE_REQUESTED = 1<<4,
    STATE_UPGRADE_WAITING = 1<<5,
    STATE_UPGRADE_LOSER = 1<<6,
    STATE_COMBINED_UPGRADING = STATE_UPGRADE_WAITING | STATE_UPGRADE_LOSER
};

const unsigned char RELEASED = 0;
const unsigned char ACQUIRED = 1;
 
template<typename T>
inline atomic<T>& as_atomic( T& t ) {
    return *(atomic<T>*)&t;
}

inline bool queuing_rw_mutex::scoped_lock::try_acquire_internal_lock()
{
    return as_atomic(internal_lock).compare_and_swap<tbb::acquire>(ACQUIRED,RELEASED) == RELEASED;
}

inline void queuing_rw_mutex::scoped_lock::acquire_internal_lock()
{
    // Usually, we would use the test-test-and-set idiom here, with exponential backoff.
    // But so far, experiments indicate there is no value in doing so here.
    while( !try_acquire_internal_lock() ) {
        __TBB_Pause(1);
    }
}

inline void queuing_rw_mutex::scoped_lock::release_internal_lock()
{
    __TBB_store_with_release(internal_lock,RELEASED);
}

inline void queuing_rw_mutex::scoped_lock::wait_for_release_of_internal_lock()
{
    spin_wait_until_eq(internal_lock, RELEASED);
}

inline void queuing_rw_mutex::scoped_lock::unblock_or_wait_on_internal_lock( uintptr_t flag ) {
    if( flag )
        wait_for_release_of_internal_lock();
    else
        release_internal_lock();
}

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    // Workaround for overzealous compiler warnings
    #pragma warning (push)
    #pragma warning (disable: 4311 4312)
#endif

//! A view of a T* with additional functionality for twiddling low-order bits.
template<typename T>
class tricky_atomic_pointer: no_copy {
public:
    typedef typename atomic_rep<sizeof(T*)>::word word;

    template<memory_semantics M>
    static T* fetch_and_add( T* volatile * location, word addend ) {
        return reinterpret_cast<T*>( atomic_traits<sizeof(T*),M>::fetch_and_add(location, addend) );
    }
    template<memory_semantics M>
    static T* fetch_and_store( T* volatile * location, T* value ) {
        return reinterpret_cast<T*>( atomic_traits<sizeof(T*),M>::fetch_and_store(location, reinterpret_cast<word>(value)) );
    }
    template<memory_semantics M>
    static T* compare_and_swap( T* volatile * location, T* value, T* comparand ) {
        return reinterpret_cast<T*>(
                 atomic_traits<sizeof(T*),M>::compare_and_swap(location, reinterpret_cast<word>(value),
                                                              reinterpret_cast<word>(comparand))
               );
    }

    T* & ref;
    tricky_atomic_pointer( T*& original ) : ref(original) {};
    tricky_atomic_pointer( T* volatile & original ) : ref(original) {};
    T* operator&( word operand2 ) const {
        return reinterpret_cast<T*>( reinterpret_cast<word>(ref) & operand2 );
    }
    T* operator|( word operand2 ) const {
        return reinterpret_cast<T*>( reinterpret_cast<word>(ref) | operand2 );
    }
};

typedef tricky_atomic_pointer<queuing_rw_mutex::scoped_lock> tricky_pointer;

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    // Workaround for overzealous compiler warnings
    #pragma warning (pop)
#endif

//! Mask for low order bit of a pointer.
static const tricky_pointer::word FLAG = 0x1;

inline
uintptr get_flag( queuing_rw_mutex::scoped_lock* ptr ) { 
    return uintptr(tricky_pointer(ptr)&FLAG);
}

//------------------------------------------------------------------------
// Methods of queuing_rw_mutex::scoped_lock
//------------------------------------------------------------------------

void queuing_rw_mutex::scoped_lock::acquire( queuing_rw_mutex& m, bool write )
{
    __TBB_ASSERT( !this->mutex, "scoped_lock is already holding a mutex");

    // Must set all fields before the fetch_and_store, because once the
    // fetch_and_store executes, *this becomes accessible to other threads.
    mutex = &m;
    prev  = NULL;
    next  = NULL;
    going = 0;
    state = state_t(write ? STATE_WRITER : STATE_READER);
    internal_lock = RELEASED;

    queuing_rw_mutex::scoped_lock* pred = m.q_tail.fetch_and_store<tbb::release>(this);

    if( write ) {       // Acquiring for write

        if( pred ) {
            ITT_NOTIFY(sync_prepare, mutex);
            pred = tricky_pointer(pred) & ~FLAG;
            __TBB_ASSERT( !( tricky_pointer(pred) & FLAG ), "use of corrupted pointer!" );
            __TBB_ASSERT( !pred->next, "the predecessor has another successor!");
            // ensure release semantics on IPF
           __TBB_store_with_release(pred->next,this);
            spin_wait_until_eq(going, 1);
        }

    } else {            // Acquiring for read
#if DO_ITT_NOTIFY
        bool sync_prepare_done = false;
#endif
        if( pred ) {
            unsigned short pred_state;
            __TBB_ASSERT( !this->prev, "the predecessor is already set" );
            if( tricky_pointer(pred)&FLAG ) {
                /* this is only possible if pred is an upgrading reader and it signals us to wait */
                pred_state = STATE_UPGRADE_WAITING;
                pred = tricky_pointer(pred) & ~FLAG;
            } else {
                // Load pred->state now, because once pred->next becomes
                // non-NULL, we must assume that *pred might be destroyed.
                pred_state = pred->state.compare_and_swap<tbb::acquire>(STATE_READER_UNBLOCKNEXT, STATE_READER);
            }
            this->prev = pred;
            __TBB_ASSERT( !( tricky_pointer(pred) & FLAG ), "use of corrupted pointer!" );
            __TBB_ASSERT( !pred->next, "the predecessor has another successor!");
            // ensure release semantics on IPF
           __TBB_store_with_release(pred->next,this);
            if( pred_state != STATE_ACTIVEREADER ) {
#if DO_ITT_NOTIFY
                sync_prepare_done = true;
                ITT_NOTIFY(sync_prepare, mutex);
#endif
                spin_wait_until_eq(going, 1);
            }
        }
        unsigned short old_state = state.compare_and_swap<tbb::acquire>(STATE_ACTIVEREADER, STATE_READER);
        if( old_state!=STATE_READER ) {
#if DO_ITT_NOTIFY
            if( !sync_prepare_done )
                ITT_NOTIFY(sync_prepare, mutex);
#endif
            // Failed to become active reader -> need to unblock the next waiting reader first
            __TBB_ASSERT( state==STATE_READER_UNBLOCKNEXT, "unexpected state" );
            spin_wait_while_eq(next, (scoped_lock*)NULL);
            /* state should be changed before unblocking the next otherwise it might finish
               and another thread can get our old state and left blocked */
            state = STATE_ACTIVEREADER;
            // ensure release semantics on IPF
           __TBB_store_with_release(next->going,1);
        }
    }

    ITT_NOTIFY(sync_acquired, mutex);

    // Force acquire so that user's critical section receives correct values
    // from processor that was previously in the user's critical section.
    __TBB_load_with_acquire(going);
}

bool queuing_rw_mutex::scoped_lock::try_acquire( queuing_rw_mutex& m, bool write )
{
    __TBB_ASSERT( !this->mutex, "scoped_lock is already holding a mutex");

    // Must set all fields before the fetch_and_store, because once the
    // fetch_and_store executes, *this becomes accessible to other threads.
    prev  = NULL;
    next  = NULL;
    going = 0;
    state = state_t(write ? STATE_WRITER : STATE_ACTIVEREADER);
    internal_lock = RELEASED;

    if( m.q_tail ) return false;
    // The CAS must have release semantics, because we are
    // "sending" the fields initialized above to other processors.
    queuing_rw_mutex::scoped_lock* pred = m.q_tail.compare_and_swap<tbb::release>(this, NULL);

    // Force acquire so that user's critical section receives correct values
    // from processor that was previously in the user's critical section.
    // try_acquire should always have acquire semantic, even if failed.
    __TBB_load_with_acquire(going);

    if( !pred ) {
        mutex = &m;
        ITT_NOTIFY(sync_acquired, mutex);
        return true;
    } else return false;

}

void queuing_rw_mutex::scoped_lock::release( )
{
    __TBB_ASSERT(this->mutex!=NULL, "no lock acquired");

    ITT_NOTIFY(sync_releasing, mutex);

    if( state == STATE_WRITER ) { // Acquired for write

        // The logic below is the same as "writerUnlock", but restructured to remove "return" in the middle of routine.
        // In the statement below, acquire semantics of reading 'next' is required
        // so that following operations with fields of 'next' are safe.
        scoped_lock* n = __TBB_load_with_acquire(next);
        if( !n ) {
            if( this == mutex->q_tail.compare_and_swap<tbb::release>(NULL, this) ) {
                // this was the only item in the queue, and the queue is now empty.
                goto done;
            }
            spin_wait_while_eq( next, (scoped_lock*)NULL );
            n = next;
        }
        n->going = 2; // protect next queue node from being destroyed too early
        if( n->state==STATE_UPGRADE_WAITING ) {
            // the next waiting for upgrade means this writer was upgraded before.
            acquire_internal_lock();
            queuing_rw_mutex::scoped_lock* tmp = tricky_pointer::fetch_and_store<tbb::release>(&(n->prev), NULL);
            n->state = STATE_UPGRADE_LOSER;
            __TBB_store_with_release(n->going,1);
            unblock_or_wait_on_internal_lock(get_flag(tmp));
        } else {
            __TBB_ASSERT( state & (STATE_COMBINED_WAITINGREADER | STATE_WRITER), "unexpected state" );
            __TBB_ASSERT( !( tricky_pointer(n->prev) & FLAG ), "use of corrupted pointer!" );
            n->prev = NULL;
            // ensure release semantics on IPF
            __TBB_store_with_release(n->going,1);
        }

    } else { // Acquired for read

        queuing_rw_mutex::scoped_lock *tmp = NULL;
retry:
        // Addition to the original paper: Mark this->prev as in use
        queuing_rw_mutex::scoped_lock *pred = tricky_pointer::fetch_and_add<tbb::acquire>(&(this->prev), FLAG);

        if( pred ) {
            if( !(pred->try_acquire_internal_lock()) )
            {
                // Failed to acquire the lock on pred. The predecessor either unlinks or upgrades.
                // In the second case, it could or could not know my "in use" flag - need to check
                tmp = tricky_pointer::compare_and_swap<tbb::release>(&(this->prev), pred, tricky_pointer(pred)|FLAG );
                if( !(tricky_pointer(tmp)&FLAG) ) {
                    // Wait for the predecessor to change this->prev (e.g. during unlink)
                    spin_wait_while_eq( this->prev, tricky_pointer(pred)|FLAG );
                    // Now owner of pred is waiting for _us_ to release its lock
                    pred->release_internal_lock();
                }
                else ; // The "in use" flag is back -> the predecessor didn't get it and will release itself; nothing to do

                tmp = NULL;
                goto retry;
            }
            __TBB_ASSERT(pred && pred->internal_lock==ACQUIRED, "predecessor's lock is not acquired");
            this->prev = pred;
            acquire_internal_lock();

            __TBB_store_with_release(pred->next,reinterpret_cast<scoped_lock *>(NULL));

            if( !next && this != mutex->q_tail.compare_and_swap<tbb::release>(pred, this) ) {
                spin_wait_while_eq( next, (void*)NULL );
            }
            __TBB_ASSERT( !get_flag(next), "use of corrupted pointer" );

            // ensure acquire semantics of reading 'next'
            if( __TBB_load_with_acquire(next) ) { // I->next != nil
                // Equivalent to I->next->prev = I->prev but protected against (prev[n]&FLAG)!=0
                tmp = tricky_pointer::fetch_and_store<tbb::release>(&(next->prev), pred);
                // I->prev->next = I->next;
                __TBB_ASSERT(this->prev==pred, NULL);
                __TBB_store_with_release(pred->next,next);
            }
            // Safe to release in the order opposite to acquiring which makes the code simplier
            pred->release_internal_lock();

        } else { // No predecessor when we looked
            acquire_internal_lock();  // "exclusiveLock(&I->EL)"
            // ensure acquire semantics of reading 'next'
            scoped_lock* n = __TBB_load_with_acquire(next);
            if( !n ) {
                if( this != mutex->q_tail.compare_and_swap<tbb::release>(NULL, this) ) {
                    spin_wait_while_eq( next, (scoped_lock*)NULL );
                    n = next;
                } else {
                    goto unlock_self;
                }
            }
            n->going = 2; // protect next queue node from being destroyed too early
            tmp = tricky_pointer::fetch_and_store<tbb::release>(&(n->prev), NULL);
            // ensure release semantics on IPF
            __TBB_store_with_release(n->going,1);
        }
unlock_self:
        unblock_or_wait_on_internal_lock(get_flag(tmp));
    }
done:
    spin_wait_while_eq( going, 2 );

    initialize();
}

bool queuing_rw_mutex::scoped_lock::downgrade_to_reader()
{
    __TBB_ASSERT( state==STATE_WRITER, "no sense to downgrade a reader" );

    ITT_NOTIFY(sync_releasing, mutex);

    // ensure acquire semantics of reading 'next'
    if( ! __TBB_load_with_acquire(next) ) {
        state = STATE_READER;
        if( this==mutex->q_tail ) {
            unsigned short old_state = state.compare_and_swap<tbb::release>(STATE_ACTIVEREADER, STATE_READER);
            if( old_state==STATE_READER ) {
                goto downgrade_done;
            }
        }
        /* wait for the next to register */
        spin_wait_while_eq( next, (void*)NULL );
    }
    __TBB_ASSERT( next, "still no successor at this point!" );
    if( next->state & STATE_COMBINED_WAITINGREADER )
        __TBB_store_with_release(next->going,1);
    else if( next->state==STATE_UPGRADE_WAITING )
        // the next waiting for upgrade means this writer was upgraded before.
        next->state = STATE_UPGRADE_LOSER;
    state = STATE_ACTIVEREADER;

downgrade_done:
    return true;
}

bool queuing_rw_mutex::scoped_lock::upgrade_to_writer()
{
    __TBB_ASSERT( state==STATE_ACTIVEREADER, "only active reader can be upgraded" );

    queuing_rw_mutex::scoped_lock * tmp;
    queuing_rw_mutex::scoped_lock * me = this;

    ITT_NOTIFY(sync_releasing, mutex);
    state = STATE_UPGRADE_REQUESTED;
requested:
    __TBB_ASSERT( !( tricky_pointer(next) & FLAG ), "use of corrupted pointer!" );
    acquire_internal_lock();
    if( this != mutex->q_tail.compare_and_swap<tbb::release>(tricky_pointer(me)|FLAG, this) ) {
        spin_wait_while_eq( next, (void*)NULL );
        queuing_rw_mutex::scoped_lock * n;
        n = tricky_pointer::fetch_and_add<tbb::acquire>(&(this->next), FLAG);
        unsigned short n_state = n->state;
        /* the next reader can be blocked by our state. the best thing to do is to unblock it */
        if( n_state & STATE_COMBINED_WAITINGREADER )
            __TBB_store_with_release(n->going,1);
        tmp = tricky_pointer::fetch_and_store<tbb::release>(&(n->prev), this);
        unblock_or_wait_on_internal_lock(get_flag(tmp));
        if( n_state & (STATE_COMBINED_READER | STATE_UPGRADE_REQUESTED) ) {
            // save n|FLAG for simplicity of following comparisons
            tmp = tricky_pointer(n)|FLAG;
            atomic_backoff backoff;
            while(next==tmp) {
                if( state & STATE_COMBINED_UPGRADING ) {
                    if( __TBB_load_with_acquire(next)==tmp )
                        next = n;
                    goto waiting;
                }
                backoff.pause();
            }
            __TBB_ASSERT(next!=(tricky_pointer(n)|FLAG), NULL);
            goto requested;
        } else {
            __TBB_ASSERT( n_state & (STATE_WRITER | STATE_UPGRADE_WAITING), "unexpected state");
            __TBB_ASSERT( (tricky_pointer(n)|FLAG)==next, NULL);
            next = n;
        }
    } else {
        /* We are in the tail; whoever comes next is blocked by q_tail&FLAG */
        release_internal_lock();
    } // if( this != mutex->q_tail... )
    state.compare_and_swap<tbb::acquire>(STATE_UPGRADE_WAITING, STATE_UPGRADE_REQUESTED);

waiting:
    __TBB_ASSERT( !( tricky_pointer(next) & FLAG ), "use of corrupted pointer!" );
    __TBB_ASSERT( state & STATE_COMBINED_UPGRADING, "wrong state at upgrade waiting_retry" );
    __TBB_ASSERT( me==this, NULL );
    ITT_NOTIFY(sync_prepare, mutex);
    /* if noone was blocked by the "corrupted" q_tail, turn it back */
    mutex->q_tail.compare_and_swap<tbb::release>( this, tricky_pointer(me)|FLAG );
    queuing_rw_mutex::scoped_lock * pred;
    pred = tricky_pointer::fetch_and_add<tbb::acquire>(&(this->prev), FLAG);
    if( pred ) {
        bool success = pred->try_acquire_internal_lock();
        pred->state.compare_and_swap<tbb::release>(STATE_UPGRADE_WAITING, STATE_UPGRADE_REQUESTED);
        if( !success ) {
            tmp = tricky_pointer::compare_and_swap<tbb::release>(&(this->prev), pred, tricky_pointer(pred)|FLAG );
            if( tricky_pointer(tmp)&FLAG ) {
                spin_wait_while_eq(this->prev, pred);
                pred = this->prev;
            } else {
                spin_wait_while_eq( this->prev, tricky_pointer(pred)|FLAG );
                pred->release_internal_lock();
            }
        } else {
            this->prev = pred;
            pred->release_internal_lock();
            spin_wait_while_eq(this->prev, pred);
            pred = this->prev;
        }
        if( pred )
            goto waiting;
    } else {
        // restore the corrupted prev field for possible further use (e.g. if downgrade back to reader)
        this->prev = pred;
    }
    __TBB_ASSERT( !pred && !this->prev, NULL );

    // additional lifetime issue prevention checks
    // wait for the successor to finish working with my fields
    wait_for_release_of_internal_lock();
    // now wait for the predecessor to finish working with my fields
    spin_wait_while_eq( going, 2 );
    // there is an acquire semantics statement in the end of spin_wait_while_eq.

    bool result = ( state != STATE_UPGRADE_LOSER );
    state = STATE_WRITER;
    going = 1;

    ITT_NOTIFY(sync_acquired, mutex);
    return result;
}

void queuing_rw_mutex::internal_construct() {
    ITT_SYNC_CREATE(this, _T("tbb::queuing_rw_mutex"), _T(""));
}

} // namespace tbb
