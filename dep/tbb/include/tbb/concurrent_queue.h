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

#ifndef __TBB_concurrent_queue_H
#define __TBB_concurrent_queue_H

#include "_concurrent_queue_internal.h"

namespace tbb {

namespace strict_ppl {

//! A high-performance thread-safe non-blocking concurrent queue.
/** Multiple threads may each push and pop concurrently.
    Assignment construction is not allowed.
    @ingroup containers */
template<typename T, typename A = cache_aligned_allocator<T> > 
class concurrent_queue: public internal::concurrent_queue_base_v3<T> {
    template<typename Container, typename Value> friend class internal::concurrent_queue_iterator;

    //! Allocator type
    typedef typename A::template rebind<char>::other page_allocator_type;
    page_allocator_type my_allocator;

    //! Allocates a block of size n (bytes)
    /*overide*/ virtual void *allocate_block( size_t n ) {
        void *b = reinterpret_cast<void*>(my_allocator.allocate( n ));
        if( !b ) this->internal_throw_exception(); 
        return b;
    }

    //! Returns a block of size n (bytes)
    /*override*/ virtual void deallocate_block( void *b, size_t n ) {
        my_allocator.deallocate( reinterpret_cast<char*>(b), n );
    }

public:
    //! Element type in the queue.
    typedef T value_type;

    //! Reference type
    typedef T& reference;

    //! Const reference type
    typedef const T& const_reference;

    //! Integral type for representing size of the queue.
    typedef size_t size_type;

    //! Difference type for iterator
    typedef ptrdiff_t difference_type;

    //! Allocator type
    typedef A allocator_type;

    //! Construct empty queue
    explicit concurrent_queue(const allocator_type& a = allocator_type()) : 
        internal::concurrent_queue_base_v3<T>( sizeof(T) ), my_allocator( a )
    {
    }

    //! [begin,end) constructor
    template<typename InputIterator>
    concurrent_queue( InputIterator begin, InputIterator end, const allocator_type& a = allocator_type()) :
        internal::concurrent_queue_base_v3<T>( sizeof(T) ), my_allocator( a )
    {
        for( ; begin != end; ++begin )
            internal_push(&*begin);
    }
    
    //! Copy constructor
    concurrent_queue( const concurrent_queue& src, const allocator_type& a = allocator_type()) : 
        internal::concurrent_queue_base_v3<T>( sizeof(T) ), my_allocator( a )
    {
        assign( src );
    }
    
    //! Destroy queue
    ~concurrent_queue();

    //! Enqueue an item at tail of queue.
    void push( const T& source ) {
        internal_push( &source );
    }

    //! Attempt to dequeue an item from head of queue.
    /** Does not wait for item to become available.
        Returns true if successful; false otherwise. */
    bool try_pop( T& result ) {
        return internal_try_pop( &result );
    }

    //! Return the number of items in the queue; thread unsafe
    size_type unsafe_size() const {return this->internal_size();}

    //! Equivalent to size()==0.
    bool empty() const {return this->internal_empty();}

    //! Clear the queue. not thread-safe.
    void clear() ;

    //! Return allocator object
    allocator_type get_allocator() const { return this->my_allocator; }

    typedef internal::concurrent_queue_iterator<concurrent_queue,T> iterator;
    typedef internal::concurrent_queue_iterator<concurrent_queue,const T> const_iterator;

    //------------------------------------------------------------------------
    // The iterators are intended only for debugging.  They are slow and not thread safe.
    //------------------------------------------------------------------------
    iterator unsafe_begin() {return iterator(*this);}
    iterator unsafe_end() {return iterator();}
    const_iterator unsafe_begin() const {return const_iterator(*this);}
    const_iterator unsafe_end() const {return const_iterator();}
} ;

template<typename T, class A>
concurrent_queue<T,A>::~concurrent_queue() {
    clear();
    this->internal_finish_clear();
}

template<typename T, class A>
void concurrent_queue<T,A>::clear() {
    while( !empty() ) {
        T value;
        internal_try_pop(&value);
    }
}

} // namespace strict_ppl
    
//! A high-performance thread-safe blocking concurrent bounded queue.
/** This is the pre-PPL TBB concurrent queue which supports boundedness and blocking semantics.
    Note that method names agree with the PPL-style concurrent queue.
    Multiple threads may each push and pop concurrently.
    Assignment construction is not allowed.
    @ingroup containers */
template<typename T, class A = cache_aligned_allocator<T> >
class concurrent_bounded_queue: public internal::concurrent_queue_base_v3 {
    template<typename Container, typename Value> friend class internal::concurrent_queue_iterator;

    //! Allocator type
    typedef typename A::template rebind<char>::other page_allocator_type;
    page_allocator_type my_allocator;

    //! Class used to ensure exception-safety of method "pop" 
    class destroyer: internal::no_copy {
        T& my_value;
    public:
        destroyer( T& value ) : my_value(value) {}
        ~destroyer() {my_value.~T();}          
    };

    T& get_ref( page& page, size_t index ) {
        __TBB_ASSERT( index<items_per_page, NULL );
        return static_cast<T*>(static_cast<void*>(&page+1))[index];
    }

    /*override*/ virtual void copy_item( page& dst, size_t index, const void* src ) {
        new( &get_ref(dst,index) ) T(*static_cast<const T*>(src)); 
    }

    /*override*/ virtual void copy_page_item( page& dst, size_t dindex, const page& src, size_t sindex ) {
        new( &get_ref(dst,dindex) ) T( static_cast<const T*>(static_cast<const void*>(&src+1))[sindex] );
    }

    /*override*/ virtual void assign_and_destroy_item( void* dst, page& src, size_t index ) {
        T& from = get_ref(src,index);
        destroyer d(from);
        *static_cast<T*>(dst) = from;
    }

    /*overide*/ virtual page *allocate_page() {
        size_t n = sizeof(page) + items_per_page*item_size;
        page *p = reinterpret_cast<page*>(my_allocator.allocate( n ));
        if( !p ) internal_throw_exception(); 
        return p;
    }

    /*override*/ virtual void deallocate_page( page *p ) {
        size_t n = sizeof(page) + items_per_page*item_size;
        my_allocator.deallocate( reinterpret_cast<char*>(p), n );
    }

public:
    //! Element type in the queue.
    typedef T value_type;

    //! Allocator type
    typedef A allocator_type;

    //! Reference type
    typedef T& reference;

    //! Const reference type
    typedef const T& const_reference;

    //! Integral type for representing size of the queue.
    /** Notice that the size_type is a signed integral type.
        This is because the size can be negative if there are pending pops without corresponding pushes. */
    typedef std::ptrdiff_t size_type;

    //! Difference type for iterator
    typedef std::ptrdiff_t difference_type;

    //! Construct empty queue
    explicit concurrent_bounded_queue(const allocator_type& a = allocator_type()) : 
        concurrent_queue_base_v3( sizeof(T) ), my_allocator( a )
    {
    }

    //! Copy constructor
    concurrent_bounded_queue( const concurrent_bounded_queue& src, const allocator_type& a = allocator_type()) : 
        concurrent_queue_base_v3( sizeof(T) ), my_allocator( a )
    {
        assign( src );
    }

    //! [begin,end) constructor
    template<typename InputIterator>
    concurrent_bounded_queue( InputIterator begin, InputIterator end, const allocator_type& a = allocator_type()) :
        concurrent_queue_base_v3( sizeof(T) ), my_allocator( a )
    {
        for( ; begin != end; ++begin )
            internal_push_if_not_full(&*begin);
    }

    //! Destroy queue
    ~concurrent_bounded_queue();

    //! Enqueue an item at tail of queue.
    void push( const T& source ) {
        internal_push( &source );
    }

    //! Dequeue item from head of queue.
    /** Block until an item becomes available, and then dequeue it. */
    void pop( T& destination ) {
        internal_pop( &destination );
    }

    //! Enqueue an item at tail of queue if queue is not already full.
    /** Does not wait for queue to become not full.
        Returns true if item is pushed; false if queue was already full. */
    bool try_push( const T& source ) {
        return internal_push_if_not_full( &source );
    }

    //! Attempt to dequeue an item from head of queue.
    /** Does not wait for item to become available.
        Returns true if successful; false otherwise. */
    bool try_pop( T& destination ) {
        return internal_pop_if_present( &destination );
    }

    //! Return number of pushes minus number of pops.
    /** Note that the result can be negative if there are pops waiting for the 
        corresponding pushes.  The result can also exceed capacity() if there 
        are push operations in flight. */
    size_type size() const {return internal_size();}

    //! Equivalent to size()<=0.
    bool empty() const {return internal_empty();}

    //! Maximum number of allowed elements
    size_type capacity() const {
        return my_capacity;
    }

    //! Set the capacity
    /** Setting the capacity to 0 causes subsequent try_push operations to always fail,
        and subsequent push operations to block forever. */
    void set_capacity( size_type capacity ) {
        internal_set_capacity( capacity, sizeof(T) );
    }

    //! return allocator object
    allocator_type get_allocator() const { return this->my_allocator; }

    //! clear the queue. not thread-safe.
    void clear() ;

    typedef internal::concurrent_queue_iterator<concurrent_bounded_queue,T> iterator;
    typedef internal::concurrent_queue_iterator<concurrent_bounded_queue,const T> const_iterator;

    //------------------------------------------------------------------------
    // The iterators are intended only for debugging.  They are slow and not thread safe.
    //------------------------------------------------------------------------
    iterator unsafe_begin() {return iterator(*this);}
    iterator unsafe_end() {return iterator();}
    const_iterator unsafe_begin() const {return const_iterator(*this);}
    const_iterator unsafe_end() const {return const_iterator();}

}; 

template<typename T, class A>
concurrent_bounded_queue<T,A>::~concurrent_bounded_queue() {
    clear();
    internal_finish_clear();
}

template<typename T, class A>
void concurrent_bounded_queue<T,A>::clear() {
    while( !empty() ) {
        T value;
        internal_pop_if_present(&value);
    }
}

namespace deprecated {

//! A high-performance thread-safe blocking concurrent bounded queue.
/** This is the pre-PPL TBB concurrent queue which support boundedness and blocking semantics.
    Note that method names agree with the PPL-style concurrent queue.
    Multiple threads may each push and pop concurrently.
    Assignment construction is not allowed.
    @ingroup containers */
template<typename T, class A = cache_aligned_allocator<T> > 
class concurrent_queue: public concurrent_bounded_queue<T,A> {
#if !__TBB_TEMPLATE_FRIENDS_BROKEN
    template<typename Container, typename Value> friend class internal::concurrent_queue_iterator;
#endif 

public:
    //! Construct empty queue
    explicit concurrent_queue(const A& a = A()) : 
        concurrent_bounded_queue<T,A>( a )
    {
    }

    //! Copy constructor
    concurrent_queue( const concurrent_queue& src, const A& a = A()) : 
        concurrent_bounded_queue<T,A>( src, a )
    {
    }

    //! [begin,end) constructor
    template<typename InputIterator>
    concurrent_queue( InputIterator begin, InputIterator end, const A& a = A()) :
        concurrent_bounded_queue<T,A>( begin, end, a )
    {
    }

    //! Enqueue an item at tail of queue if queue is not already full.
    /** Does not wait for queue to become not full.
        Returns true if item is pushed; false if queue was already full. */
    bool push_if_not_full( const T& source ) {
        return try_push( source );
    }

    //! Attempt to dequeue an item from head of queue.
    /** Does not wait for item to become available.
        Returns true if successful; false otherwise. 
        @deprecated Use try_pop()
        */
    bool pop_if_present( T& destination ) {
        return try_pop( destination );
    }

    typedef typename concurrent_bounded_queue<T,A>::iterator iterator;
    typedef typename concurrent_bounded_queue<T,A>::const_iterator const_iterator;
    //
    //------------------------------------------------------------------------
    // The iterators are intended only for debugging.  They are slow and not thread safe.
    //------------------------------------------------------------------------
    iterator begin() {return this->unsafe_begin();}
    iterator end() {return this->unsafe_end();}
    const_iterator begin() const {return this->unsafe_begin();}
    const_iterator end() const {return this->unsafe_end();}
}; 

}
    

#if TBB_DEPRECATED
using deprecated::concurrent_queue;
#else
using strict_ppl::concurrent_queue;    
#endif

} // namespace tbb

#endif /* __TBB_concurrent_queue_H */
