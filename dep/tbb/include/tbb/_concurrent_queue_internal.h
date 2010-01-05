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

#ifndef __TBB_concurrent_queue_internal_H
#define __TBB_concurrent_queue_internal_H

#include "tbb_stddef.h"
#include "tbb_machine.h"
#include "atomic.h"
#include "spin_mutex.h"
#include "cache_aligned_allocator.h"
#include "tbb_exception.h"
#include <iterator>
#include <new>

namespace tbb {

#if !__TBB_TEMPLATE_FRIENDS_BROKEN

// forward declaration
namespace strict_ppl {
template<typename T, typename A> class concurrent_queue;
}

template<typename T, typename A> class concurrent_bounded_queue;

namespace deprecated {
template<typename T, typename A> class concurrent_queue;
}
#endif

//! For internal use only.
namespace strict_ppl {

//! @cond INTERNAL
namespace internal {

using namespace tbb::internal;

typedef size_t ticket;

static void* invalid_page;

template<typename T> class micro_queue ;
template<typename T> class micro_queue_pop_finalizer ;
template<typename T> class concurrent_queue_base_v3;

//! parts of concurrent_queue_rep that do not have references to micro_queue
/**
 * For internal use only.
 */
struct concurrent_queue_rep_base : no_copy {
    template<typename T> friend class micro_queue;
    template<typename T> friend class concurrent_queue_base_v3;

protected:
    //! Approximately n_queue/golden ratio
    static const size_t phi = 3;

public:
    // must be power of 2
    static const size_t n_queue = 8;

    //! Prefix on a page
    struct page {
        page* next;
        uintptr_t mask; 
    };

    atomic<ticket> head_counter;
    char pad1[NFS_MaxLineSize-sizeof(atomic<ticket>)];
    atomic<ticket> tail_counter;
    char pad2[NFS_MaxLineSize-sizeof(atomic<ticket>)];

    //! Always a power of 2
    size_t items_per_page;

    //! Size of an item
    size_t item_size;

    //! number of invalid entries in the queue
    atomic<size_t> n_invalid_entries;

    char pad3[NFS_MaxLineSize-sizeof(size_t)-sizeof(size_t)-sizeof(atomic<size_t>)];
} ;

//! Abstract class to define interface for page allocation/deallocation
/**
 * For internal use only.
 */
class concurrent_queue_page_allocator
{
    template<typename T> friend class micro_queue ;
    template<typename T> friend class micro_queue_pop_finalizer ;
protected:
    virtual ~concurrent_queue_page_allocator() {}
private:
    virtual concurrent_queue_rep_base::page* allocate_page() = 0;
    virtual void deallocate_page( concurrent_queue_rep_base::page* p ) = 0;
} ;

#if _MSC_VER && !defined(__INTEL_COMPILER)
// unary minus operator applied to unsigned type, result still unsigned
#pragma warning( push )
#pragma warning( disable: 4146 )
#endif

//! A queue using simple locking.
/** For efficient, this class has no constructor.  
    The caller is expected to zero-initialize it. */
template<typename T>
class micro_queue : no_copy {
    typedef concurrent_queue_rep_base::page page;

    //! Class used to ensure exception-safety of method "pop" 
    class destroyer: no_copy {
        T& my_value;
    public:
        destroyer( T& value ) : my_value(value) {}
        ~destroyer() {my_value.~T();}          
    };

    T& get_ref( page& page, size_t index ) {
        return static_cast<T*>(static_cast<void*>(&page+1))[index];
    }

    void copy_item( page& dst, size_t index, const void* src ) {
        new( &get_ref(dst,index) ) T(*static_cast<const T*>(src)); 
    }

    void copy_item( page& dst, size_t dindex, const page& src, size_t sindex ) {
        new( &get_ref(dst,dindex) ) T( static_cast<const T*>(static_cast<const void*>(&src+1))[sindex] );
    }

    void assign_and_destroy_item( void* dst, page& src, size_t index ) {
        T& from = get_ref(src,index);
        destroyer d(from);
        *static_cast<T*>(dst) = from;
    }

    void spin_wait_until_my_turn( atomic<ticket>& counter, ticket k, concurrent_queue_rep_base& rb ) const ;

public:
    friend class micro_queue_pop_finalizer<T>;

    atomic<page*> head_page;
    atomic<ticket> head_counter;

    atomic<page*> tail_page;
    atomic<ticket> tail_counter;

    spin_mutex page_mutex;
    
    void push( const void* item, ticket k, concurrent_queue_base_v3<T>& base ) ;

    bool pop( void* dst, ticket k, concurrent_queue_base_v3<T>& base ) ;

    micro_queue& assign( const micro_queue& src, concurrent_queue_base_v3<T>& base ) ;

    page* make_copy( concurrent_queue_base_v3<T>& base, const page* src_page, size_t begin_in_page, size_t end_in_page, ticket& g_index ) ;

    void make_invalid( ticket k ) ;
};

template<typename T>
void micro_queue<T>::spin_wait_until_my_turn( atomic<ticket>& counter, ticket k, concurrent_queue_rep_base& rb ) const {
    atomic_backoff backoff;
    do {
        backoff.pause();
        if( counter&0x1 ) {
            ++rb.n_invalid_entries;
            throw_bad_last_alloc_exception_v4();
        }
    } while( counter!=k ) ;
}

template<typename T>
void micro_queue<T>::push( const void* item, ticket k, concurrent_queue_base_v3<T>& base ) {
    k &= -concurrent_queue_rep_base::n_queue;
    page* p = NULL;
    size_t index = k/concurrent_queue_rep_base::n_queue & (base.my_rep->items_per_page-1);
    if( !index ) {
        try {
            concurrent_queue_page_allocator& pa = base;
            p = pa.allocate_page();
        } catch (...) {
            ++base.my_rep->n_invalid_entries;
            make_invalid( k );
        }
        p->mask = 0;
        p->next = NULL;
    }
    
    if( tail_counter!=k ) spin_wait_until_my_turn( tail_counter, k, *base.my_rep );
        
    if( p ) {
        spin_mutex::scoped_lock lock( page_mutex );
        if( page* q = tail_page )
            q->next = p;
        else
            head_page = p; 
        tail_page = p;
    } else {
        p = tail_page;
    }
   
    try {
        copy_item( *p, index, item );
        // If no exception was thrown, mark item as present.
        p->mask |= uintptr_t(1)<<index;
        tail_counter += concurrent_queue_rep_base::n_queue; 
    } catch (...) {
        ++base.my_rep->n_invalid_entries;
        tail_counter += concurrent_queue_rep_base::n_queue; 
        throw;
    }
}

template<typename T>
bool micro_queue<T>::pop( void* dst, ticket k, concurrent_queue_base_v3<T>& base ) {
    k &= -concurrent_queue_rep_base::n_queue;
    if( head_counter!=k ) spin_wait_until_eq( head_counter, k );
    if( tail_counter==k ) spin_wait_while_eq( tail_counter, k );
    page& p = *head_page;
    __TBB_ASSERT( &p, NULL );
    size_t index = k/concurrent_queue_rep_base::n_queue & (base.my_rep->items_per_page-1);
    bool success = false; 
    {
        micro_queue_pop_finalizer<T> finalizer( *this, base, k+concurrent_queue_rep_base::n_queue, index==base.my_rep->items_per_page-1 ? &p : NULL ); 
        if( p.mask & uintptr_t(1)<<index ) {
            success = true;
            assign_and_destroy_item( dst, p, index );
        } else {
            --base.my_rep->n_invalid_entries;
        }
    }
    return success;
}

template<typename T>
micro_queue<T>& micro_queue<T>::assign( const micro_queue<T>& src, concurrent_queue_base_v3<T>& base ) {
    head_counter = src.head_counter;
    tail_counter = src.tail_counter;
    page_mutex   = src.page_mutex;

    const page* srcp = src.head_page;
    if( srcp ) {
        ticket g_index = head_counter;
        try {
            size_t n_items  = (tail_counter-head_counter)/concurrent_queue_rep_base::n_queue;
            size_t index = head_counter/concurrent_queue_rep_base::n_queue & (base.my_rep->items_per_page-1);
            size_t end_in_first_page = (index+n_items<base.my_rep->items_per_page)?(index+n_items):base.my_rep->items_per_page;

            head_page = make_copy( base, srcp, index, end_in_first_page, g_index );
            page* cur_page = head_page;

            if( srcp != src.tail_page ) {
                for( srcp = srcp->next; srcp!=src.tail_page; srcp=srcp->next ) {
                    cur_page->next = make_copy( base, srcp, 0, base.my_rep->items_per_page, g_index );
                    cur_page = cur_page->next;
                }

                __TBB_ASSERT( srcp==src.tail_page, NULL );
                size_t last_index = tail_counter/concurrent_queue_rep_base::n_queue & (base.my_rep->items_per_page-1);
                if( last_index==0 ) last_index = base.my_rep->items_per_page;

                cur_page->next = make_copy( base, srcp, 0, last_index, g_index );
                cur_page = cur_page->next;
            }
            tail_page = cur_page;
        } catch (...) {
            make_invalid( g_index );
        }
    } else {
        head_page = tail_page = NULL;
    }
    return *this;
}

template<typename T>
void micro_queue<T>::make_invalid( ticket k ) {
    static page dummy = {static_cast<page*>((void*)1), 0};
    // mark it so that no more pushes are allowed.
    invalid_page = &dummy;
    {
        spin_mutex::scoped_lock lock( page_mutex );
        tail_counter = k+concurrent_queue_rep_base::n_queue+1;
        if( page* q = tail_page )
            q->next = static_cast<page*>(invalid_page);
        else
            head_page = static_cast<page*>(invalid_page); 
        tail_page = static_cast<page*>(invalid_page);
    }
    throw;
}

template<typename T>
concurrent_queue_rep_base::page* micro_queue<T>::make_copy( concurrent_queue_base_v3<T>& base, const concurrent_queue_rep_base::page* src_page, size_t begin_in_page, size_t end_in_page, ticket& g_index ) {
    concurrent_queue_page_allocator& pa = base;
    page* new_page = pa.allocate_page();
    new_page->next = NULL;
    new_page->mask = src_page->mask;
    for( ; begin_in_page!=end_in_page; ++begin_in_page, ++g_index )
        if( new_page->mask & uintptr_t(1)<<begin_in_page )
            copy_item( *new_page, begin_in_page, *src_page, begin_in_page );
    return new_page;
}

template<typename T>
class micro_queue_pop_finalizer: no_copy {
    typedef concurrent_queue_rep_base::page page;
    ticket my_ticket;
    micro_queue<T>& my_queue;
    page* my_page; 
    concurrent_queue_page_allocator& allocator;
public:
    micro_queue_pop_finalizer( micro_queue<T>& queue, concurrent_queue_base_v3<T>& b, ticket k, page* p ) :
        my_ticket(k), my_queue(queue), my_page(p), allocator(b)
    {}
    ~micro_queue_pop_finalizer() ;
};

template<typename T>
micro_queue_pop_finalizer<T>::~micro_queue_pop_finalizer() {
    page* p = my_page;
    if( p ) {
        spin_mutex::scoped_lock lock( my_queue.page_mutex );
        page* q = p->next;
        my_queue.head_page = q;
        if( !q ) {
            my_queue.tail_page = NULL;
        }
    }
    my_queue.head_counter = my_ticket;
    if( p ) {
        allocator.deallocate_page( p );
    }
}

#if _MSC_VER && !defined(__INTEL_COMPILER)
#pragma warning( pop )
#endif // warning 4146 is back

template<typename T> class concurrent_queue_iterator_rep ;
template<typename T> class concurrent_queue_iterator_base_v3;

//! representation of concurrent_queue_base
/**
 * the class inherits from concurrent_queue_rep_base and defines an array of micro_queue<T>'s
 */
template<typename T>
struct concurrent_queue_rep : public concurrent_queue_rep_base {
    micro_queue<T> array[n_queue];

    //! Map ticket to an array index
    static size_t index( ticket k ) {
        return k*phi%n_queue;
    }

    micro_queue<T>& choose( ticket k ) {
        // The formula here approximates LRU in a cache-oblivious way.
        return array[index(k)];
    }
};

//! base class of concurrent_queue
/**
 * The class implements the interface defined by concurrent_queue_page_allocator
 * and has a pointer to an instance of concurrent_queue_rep.
 */
template<typename T>
class concurrent_queue_base_v3: public concurrent_queue_page_allocator {
    //! Internal representation
    concurrent_queue_rep<T>* my_rep;

    friend struct concurrent_queue_rep<T>;
    friend class micro_queue<T>;
    friend class concurrent_queue_iterator_rep<T>;
    friend class concurrent_queue_iterator_base_v3<T>;

protected:
    typedef typename concurrent_queue_rep<T>::page page;

private:
    /* override */ virtual page *allocate_page() {
        concurrent_queue_rep<T>& r = *my_rep;
        size_t n = sizeof(page) + r.items_per_page*r.item_size;
        return reinterpret_cast<page*>(allocate_block ( n ));
    }

    /* override */ virtual void deallocate_page( concurrent_queue_rep_base::page *p ) {
        concurrent_queue_rep<T>& r = *my_rep;
        size_t n = sizeof(page) + r.items_per_page*r.item_size;
        deallocate_block( reinterpret_cast<void*>(p), n );
    }

    //! custom allocator
    virtual void *allocate_block( size_t n ) = 0;

    //! custom de-allocator
    virtual void deallocate_block( void *p, size_t n ) = 0;

protected:
    concurrent_queue_base_v3( size_t item_size ) ;

    /* override */ virtual ~concurrent_queue_base_v3() {
        size_t nq = my_rep->n_queue;
        for( size_t i=0; i<nq; i++ )
            __TBB_ASSERT( my_rep->array[i].tail_page==NULL, "pages were not freed properly" );
        cache_aligned_allocator<concurrent_queue_rep<T> >().deallocate(my_rep,1);
    }

    //! Enqueue item at tail of queue
    void internal_push( const void* src ) {
        concurrent_queue_rep<T>& r = *my_rep;
        ticket k = r.tail_counter++;
        r.choose(k).push( src, k, *this );
    }

    //! Attempt to dequeue item from queue.
    /** NULL if there was no item to dequeue. */
    bool internal_try_pop( void* dst ) ;

    //! Get size of queue; result may be invalid if queue is modified concurrently
    size_t internal_size() const ;

    //! check if the queue is empty; thread safe
    bool internal_empty() const ;

    //! free any remaining pages
    /* note that the name may be misleading, but it remains so due to a historical accident. */
    void internal_finish_clear() ;

    //! throw an exception
    void internal_throw_exception() const {
        throw std::bad_alloc();
    }

    //! copy internal representation
    void assign( const concurrent_queue_base_v3& src ) ;
};

template<typename T>
concurrent_queue_base_v3<T>::concurrent_queue_base_v3( size_t item_size ) {
    my_rep = cache_aligned_allocator<concurrent_queue_rep<T> >().allocate(1);
    __TBB_ASSERT( (size_t)my_rep % NFS_GetLineSize()==0, "alignment error" );
    __TBB_ASSERT( (size_t)&my_rep->head_counter % NFS_GetLineSize()==0, "alignment error" );
    __TBB_ASSERT( (size_t)&my_rep->tail_counter % NFS_GetLineSize()==0, "alignment error" );
    __TBB_ASSERT( (size_t)&my_rep->array % NFS_GetLineSize()==0, "alignment error" );
    memset(my_rep,0,sizeof(concurrent_queue_rep<T>));
    my_rep->item_size = item_size;
    my_rep->items_per_page = item_size<=8 ? 32 :
                             item_size<=16 ? 16 : 
                             item_size<=32 ? 8 :
                             item_size<=64 ? 4 :
                             item_size<=128 ? 2 :
                             1;
}

template<typename T>
bool concurrent_queue_base_v3<T>::internal_try_pop( void* dst ) {
    concurrent_queue_rep<T>& r = *my_rep;
    ticket k;
    do {
        k = r.head_counter;
        for(;;) {
            if( r.tail_counter<=k ) {
                // Queue is empty 
                return false;
            }
            // Queue had item with ticket k when we looked.  Attempt to get that item.
            ticket tk=k;
#if defined(_MSC_VER) && defined(_Wp64)
    #pragma warning (push)
    #pragma warning (disable: 4267)
#endif
            k = r.head_counter.compare_and_swap( tk+1, tk );
#if defined(_MSC_VER) && defined(_Wp64)
    #pragma warning (pop)
#endif
            if( k==tk )
                break;
            // Another thread snatched the item, retry.
        }
    } while( !r.choose( k ).pop( dst, k, *this ) );
    return true;
}

template<typename T>
size_t concurrent_queue_base_v3<T>::internal_size() const {
    concurrent_queue_rep<T>& r = *my_rep;
    __TBB_ASSERT( sizeof(ptrdiff_t)<=sizeof(size_t), NULL );
    ticket hc = r.head_counter;
    size_t nie = r.n_invalid_entries;
    ticket tc = r.tail_counter;
    __TBB_ASSERT( hc!=tc || !nie, NULL );
    ptrdiff_t sz = tc-hc-nie;
    return sz<0 ? 0 :  size_t(sz);
}

template<typename T>
bool concurrent_queue_base_v3<T>::internal_empty() const {
    concurrent_queue_rep<T>& r = *my_rep;
    ticket tc = r.tail_counter;
    ticket hc = r.head_counter;
    // if tc!=r.tail_counter, the queue was not empty at some point between the two reads.
    return tc==r.tail_counter && tc==hc+r.n_invalid_entries ;
}

template<typename T>
void concurrent_queue_base_v3<T>::internal_finish_clear() {
    concurrent_queue_rep<T>& r = *my_rep;
    size_t nq = r.n_queue;
    for( size_t i=0; i<nq; ++i ) {
        page* tp = r.array[i].tail_page;
        __TBB_ASSERT( r.array[i].head_page==tp, "at most one page should remain" );
        if( tp!=NULL) {
            if( tp!=invalid_page ) deallocate_page( tp );
            r.array[i].tail_page = NULL;
        }
    }
}

template<typename T>
void concurrent_queue_base_v3<T>::assign( const concurrent_queue_base_v3& src ) {
    concurrent_queue_rep<T>& r = *my_rep;
    r.items_per_page = src.my_rep->items_per_page;

    // copy concurrent_queue_rep.
    r.head_counter = src.my_rep->head_counter;
    r.tail_counter = src.my_rep->tail_counter;
    r.n_invalid_entries = src.my_rep->n_invalid_entries;

    // copy micro_queues
    for( size_t i = 0; i<r.n_queue; ++i )
        r.array[i].assign( src.my_rep->array[i], *this);

    __TBB_ASSERT( r.head_counter==src.my_rep->head_counter && r.tail_counter==src.my_rep->tail_counter, 
            "the source concurrent queue should not be concurrently modified." );
}

template<typename Container, typename Value> class concurrent_queue_iterator;

template<typename T>
class concurrent_queue_iterator_rep: no_assign {
public:
    ticket head_counter;
    const concurrent_queue_base_v3<T>& my_queue;
    typename concurrent_queue_base_v3<T>::page* array[concurrent_queue_rep<T>::n_queue];
    concurrent_queue_iterator_rep( const concurrent_queue_base_v3<T>& queue ) :
        head_counter(queue.my_rep->head_counter),
        my_queue(queue)
    {
        for( size_t k=0; k<concurrent_queue_rep<T>::n_queue; ++k )
            array[k] = queue.my_rep->array[k].head_page;
    }

    //! Set item to point to kth element.  Return true if at end of queue or item is marked valid; false otherwise.
    bool get_item( void*& item, size_t k ) ;
};

template<typename T>
bool concurrent_queue_iterator_rep<T>::get_item( void*& item, size_t k ) {
    if( k==my_queue.my_rep->tail_counter ) {
        item = NULL;
        return true;
    } else {
        typename concurrent_queue_base_v3<T>::page* p = array[concurrent_queue_rep<T>::index(k)];
        __TBB_ASSERT(p,NULL);
        size_t i = k/concurrent_queue_rep<T>::n_queue & (my_queue.my_rep->items_per_page-1);
        item = static_cast<unsigned char*>(static_cast<void*>(p+1)) + my_queue.my_rep->item_size*i;
        return (p->mask & uintptr_t(1)<<i)!=0;
    }
}

//! Type-independent portion of concurrent_queue_iterator.
/** @ingroup containers */
template<typename Value>
class concurrent_queue_iterator_base_v3 : no_assign {
    //! Concurrentconcurrent_queue over which we are iterating.
    /** NULL if one past last element in queue. */
    concurrent_queue_iterator_rep<Value>* my_rep;

    template<typename C, typename T, typename U>
    friend bool operator==( const concurrent_queue_iterator<C,T>& i, const concurrent_queue_iterator<C,U>& j );

    template<typename C, typename T, typename U>
    friend bool operator!=( const concurrent_queue_iterator<C,T>& i, const concurrent_queue_iterator<C,U>& j );
protected:
    //! Pointer to current item
    mutable void* my_item;

public:
    //! Default constructor
    concurrent_queue_iterator_base_v3() : my_rep(NULL), my_item(NULL) {
#if __GNUC__==4&&__GNUC_MINOR__==3
        // to get around a possible gcc 4.3 bug
        __asm__ __volatile__("": : :"memory");
#endif
    }

    //! Copy constructor
    concurrent_queue_iterator_base_v3( const concurrent_queue_iterator_base_v3& i ) : my_rep(NULL), my_item(NULL) {
        assign(i);
    }

    //! Construct iterator pointing to head of queue.
    concurrent_queue_iterator_base_v3( const concurrent_queue_base_v3<Value>& queue ) ;

protected:
    //! Assignment
    void assign( const concurrent_queue_iterator_base_v3<Value>& other ) ;

    //! Advance iterator one step towards tail of queue.
    void advance() ;

    //! Destructor
    ~concurrent_queue_iterator_base_v3() {
        cache_aligned_allocator<concurrent_queue_iterator_rep<Value> >().deallocate(my_rep, 1);
        my_rep = NULL;
    }
};

template<typename Value>
concurrent_queue_iterator_base_v3<Value>::concurrent_queue_iterator_base_v3( const concurrent_queue_base_v3<Value>& queue ) {
    my_rep = cache_aligned_allocator<concurrent_queue_iterator_rep<Value> >().allocate(1);
    new( my_rep ) concurrent_queue_iterator_rep<Value>(queue);
    size_t k = my_rep->head_counter;
    if( !my_rep->get_item(my_item, k) ) advance();
}

template<typename Value>
void concurrent_queue_iterator_base_v3<Value>::assign( const concurrent_queue_iterator_base_v3<Value>& other ) {
    if( my_rep!=other.my_rep ) {
        if( my_rep ) {
            cache_aligned_allocator<concurrent_queue_iterator_rep<Value> >().deallocate(my_rep, 1);
            my_rep = NULL;
        }
        if( other.my_rep ) {
            my_rep = cache_aligned_allocator<concurrent_queue_iterator_rep<Value> >().allocate(1);
            new( my_rep ) concurrent_queue_iterator_rep<Value>( *other.my_rep );
        }
    }
    my_item = other.my_item;
}

template<typename Value>
void concurrent_queue_iterator_base_v3<Value>::advance() {
    __TBB_ASSERT( my_item, "attempt to increment iterator past end of queue" );  
    size_t k = my_rep->head_counter;
    const concurrent_queue_base_v3<Value>& queue = my_rep->my_queue;
#if TBB_USE_ASSERT
    void* tmp;
    my_rep->get_item(tmp,k);
    __TBB_ASSERT( my_item==tmp, NULL );
#endif /* TBB_USE_ASSERT */
    size_t i = k/concurrent_queue_rep<Value>::n_queue & (queue.my_rep->items_per_page-1);
    if( i==queue.my_rep->items_per_page-1 ) {
        typename concurrent_queue_base_v3<Value>::page*& root = my_rep->array[concurrent_queue_rep<Value>::index(k)];
        root = root->next;
    }
    // advance k
    my_rep->head_counter = ++k;
    if( !my_rep->get_item(my_item, k) ) advance();
}

template<typename T>
static inline const concurrent_queue_iterator_base_v3<const T>& add_constness( const concurrent_queue_iterator_base_v3<T>& q )
{
    return *reinterpret_cast<const concurrent_queue_iterator_base_v3<const T> *>(&q) ;
}

//! Meets requirements of a forward iterator for STL.
/** Value is either the T or const T type of the container.
    @ingroup containers */
template<typename Container, typename Value>
class concurrent_queue_iterator: public concurrent_queue_iterator_base_v3<Value>,
        public std::iterator<std::forward_iterator_tag,Value> {
#if !__TBB_TEMPLATE_FRIENDS_BROKEN
    template<typename T, class A>
    friend class ::tbb::strict_ppl::concurrent_queue;
#else
public: // workaround for MSVC
#endif 
    //! Construct iterator pointing to head of queue.
    concurrent_queue_iterator( const concurrent_queue_base_v3<Value>& queue ) :
        concurrent_queue_iterator_base_v3<Value>(queue)
    {
    }

public:
    concurrent_queue_iterator() {}

    //! Copy constructor
    concurrent_queue_iterator( const concurrent_queue_iterator<Container,Value>& other ) :
        concurrent_queue_iterator_base_v3<Value>(other)
    {
    }

    template<typename T>
    concurrent_queue_iterator( const concurrent_queue_iterator<Container,T>& other ) :
        concurrent_queue_iterator_base_v3<Value>(add_constness(other))
    {
    }

    //! Iterator assignment
    concurrent_queue_iterator& operator=( const concurrent_queue_iterator& other ) {
        assign(other);
        return *this;
    }

    //! Reference to current item 
    Value& operator*() const {
        return *static_cast<Value*>(this->my_item);
    }

    Value* operator->() const {return &operator*();}

    //! Advance to next item in queue
    concurrent_queue_iterator& operator++() {
        this->advance();
        return *this;
    }

    //! Post increment
    Value* operator++(int) {
        Value* result = &operator*();
        operator++();
        return result;
    }
}; // concurrent_queue_iterator


template<typename C, typename T, typename U>
bool operator==( const concurrent_queue_iterator<C,T>& i, const concurrent_queue_iterator<C,U>& j ) {
    return i.my_item==j.my_item;
}

template<typename C, typename T, typename U>
bool operator!=( const concurrent_queue_iterator<C,T>& i, const concurrent_queue_iterator<C,U>& j ) {
    return i.my_item!=j.my_item;
}

} // namespace internal

//! @endcond

} // namespace strict_ppl

//! @cond INTERNAL
namespace internal {

class concurrent_queue_rep;
class concurrent_queue_iterator_rep;
class concurrent_queue_iterator_base_v3;
template<typename Container, typename Value> class concurrent_queue_iterator;

//! For internal use only.
/** Type-independent portion of concurrent_queue.
    @ingroup containers */
class concurrent_queue_base_v3: no_copy {
    //! Internal representation
    concurrent_queue_rep* my_rep;

    friend class concurrent_queue_rep;
    friend struct micro_queue;
    friend class micro_queue_pop_finalizer;
    friend class concurrent_queue_iterator_rep;
    friend class concurrent_queue_iterator_base_v3;
protected:
    //! Prefix on a page
    struct page {
        page* next;
        uintptr_t mask; 
    };

    //! Capacity of the queue
    ptrdiff_t my_capacity;
   
    //! Always a power of 2
    size_t items_per_page;

    //! Size of an item
    size_t item_size;

private:
    virtual void copy_item( page& dst, size_t index, const void* src ) = 0;
    virtual void assign_and_destroy_item( void* dst, page& src, size_t index ) = 0;
protected:
    __TBB_EXPORTED_METHOD concurrent_queue_base_v3( size_t item_size );
    virtual __TBB_EXPORTED_METHOD ~concurrent_queue_base_v3();

    //! Enqueue item at tail of queue
    void __TBB_EXPORTED_METHOD internal_push( const void* src );

    //! Dequeue item from head of queue
    void __TBB_EXPORTED_METHOD internal_pop( void* dst );

    //! Attempt to enqueue item onto queue.
    bool __TBB_EXPORTED_METHOD internal_push_if_not_full( const void* src );

    //! Attempt to dequeue item from queue.
    /** NULL if there was no item to dequeue. */
    bool __TBB_EXPORTED_METHOD internal_pop_if_present( void* dst );

    //! Get size of queue
    ptrdiff_t __TBB_EXPORTED_METHOD internal_size() const;

    //! Check if the queue is emtpy
    bool __TBB_EXPORTED_METHOD internal_empty() const;

    //! Set the queue capacity
    void __TBB_EXPORTED_METHOD internal_set_capacity( ptrdiff_t capacity, size_t element_size );

    //! custom allocator
    virtual page *allocate_page() = 0;

    //! custom de-allocator
    virtual void deallocate_page( page *p ) = 0;

    //! free any remaining pages
    /* note that the name may be misleading, but it remains so due to a historical accident. */
    void __TBB_EXPORTED_METHOD internal_finish_clear() ;

    //! throw an exception
    void __TBB_EXPORTED_METHOD internal_throw_exception() const;

    //! copy internal representation
    void __TBB_EXPORTED_METHOD assign( const concurrent_queue_base_v3& src ) ;

private:
    virtual void copy_page_item( page& dst, size_t dindex, const page& src, size_t sindex ) = 0;
};

//! Type-independent portion of concurrent_queue_iterator.
/** @ingroup containers */
class concurrent_queue_iterator_base_v3 {
    //! Concurrentconcurrent_queue over which we are iterating.
    /** NULL if one past last element in queue. */
    concurrent_queue_iterator_rep* my_rep;

    template<typename C, typename T, typename U>
    friend bool operator==( const concurrent_queue_iterator<C,T>& i, const concurrent_queue_iterator<C,U>& j );

    template<typename C, typename T, typename U>
    friend bool operator!=( const concurrent_queue_iterator<C,T>& i, const concurrent_queue_iterator<C,U>& j );
protected:
    //! Pointer to current item
    mutable void* my_item;

    //! Default constructor
    concurrent_queue_iterator_base_v3() : my_rep(NULL), my_item(NULL) {}

    //! Copy constructor
    concurrent_queue_iterator_base_v3( const concurrent_queue_iterator_base_v3& i ) : my_rep(NULL), my_item(NULL) {
        assign(i);
    }

    //! Construct iterator pointing to head of queue.
    __TBB_EXPORTED_METHOD concurrent_queue_iterator_base_v3( const concurrent_queue_base_v3& queue );

    //! Assignment
    void __TBB_EXPORTED_METHOD assign( const concurrent_queue_iterator_base_v3& i );

    //! Advance iterator one step towards tail of queue.
    void __TBB_EXPORTED_METHOD advance();

    //! Destructor
    __TBB_EXPORTED_METHOD ~concurrent_queue_iterator_base_v3();
};

typedef concurrent_queue_iterator_base_v3 concurrent_queue_iterator_base;

//! Meets requirements of a forward iterator for STL.
/** Value is either the T or const T type of the container.
    @ingroup containers */
template<typename Container, typename Value>
class concurrent_queue_iterator: public concurrent_queue_iterator_base,
        public std::iterator<std::forward_iterator_tag,Value> {
#if !defined(_MSC_VER) || defined(__INTEL_COMPILER)
    template<typename T, class A>
    friend class ::tbb::concurrent_bounded_queue;

    template<typename T, class A>
    friend class ::tbb::deprecated::concurrent_queue;
#else
public: // workaround for MSVC
#endif 
    //! Construct iterator pointing to head of queue.
    concurrent_queue_iterator( const concurrent_queue_base_v3& queue ) :
        concurrent_queue_iterator_base_v3(queue)
    {
    }

public:
    concurrent_queue_iterator() {}

    /** If Value==Container::value_type, then this routine is the copy constructor. 
        If Value==const Container::value_type, then this routine is a conversion constructor. */
    concurrent_queue_iterator( const concurrent_queue_iterator<Container,typename Container::value_type>& other ) :
        concurrent_queue_iterator_base_v3(other)
    {}

    //! Iterator assignment
    concurrent_queue_iterator& operator=( const concurrent_queue_iterator& other ) {
        assign(other);
        return *this;
    }

    //! Reference to current item 
    Value& operator*() const {
        return *static_cast<Value*>(my_item);
    }

    Value* operator->() const {return &operator*();}

    //! Advance to next item in queue
    concurrent_queue_iterator& operator++() {
        advance();
        return *this;
    }

    //! Post increment
    Value* operator++(int) {
        Value* result = &operator*();
        operator++();
        return result;
    }
}; // concurrent_queue_iterator


template<typename C, typename T, typename U>
bool operator==( const concurrent_queue_iterator<C,T>& i, const concurrent_queue_iterator<C,U>& j ) {
    return i.my_item==j.my_item;
}

template<typename C, typename T, typename U>
bool operator!=( const concurrent_queue_iterator<C,T>& i, const concurrent_queue_iterator<C,U>& j ) {
    return i.my_item!=j.my_item;
}

} // namespace internal;

//! @endcond

} // namespace tbb

#endif /* __TBB_concurrent_queue_internal_H */
