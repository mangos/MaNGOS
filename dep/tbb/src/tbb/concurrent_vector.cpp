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

#include "tbb/concurrent_vector.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/tbb_exception.h"
#include "tbb_misc.h"
#include "itt_notify.h"
#include <cstring>

#if defined(_MSC_VER) && defined(_Wp64)
    // Workaround for overzealous compiler warnings in /Wp64 mode
    #pragma warning (disable: 4267)
#endif

using namespace std;

namespace tbb {

namespace internal {
    class concurrent_vector_base_v3::helper :no_assign {
public:
    //! memory page size
    static const size_type page_size = 4096;

    inline static bool incompact_predicate(size_type size) { // assert size != 0, see source/test/test_vector_layout.cpp
        return size < page_size || ((size-1)%page_size < page_size/2 && size < page_size * 128); // for more details
    }

    inline static size_type find_segment_end(const concurrent_vector_base_v3 &v) {
        segment_t *s = v.my_segment;
        segment_index_t u = s==v.my_storage? pointers_per_short_table : pointers_per_long_table;
        segment_index_t k = 0;
        while( k < u && s[k].array > internal::vector_allocation_error_flag )
            ++k;
        return k;
    }

    //! assign first segment size. k - is index of last segment to be allocated, not a count of segments
    inline static void assign_first_segment_if_neccessary(concurrent_vector_base_v3 &v, segment_index_t k) {
        if( !v.my_first_block ) {
            /* There was a suggestion to set first segment according to incompact_predicate:
            while( k && !helper::incompact_predicate(segment_size( k ) * element_size) )
                --k; // while previous vector size is compact, decrement
            // reasons to not do it:
            // * constructor(n) is not ready to accept fragmented segments
            // * backward compatibility due to that constructor
            // * current version gives additional guarantee and faster init.
            // * two calls to reserve() will give the same effect.
            */
            v.my_first_block.compare_and_swap(k+1, 0); // store number of segments
        }
    }

    inline static void *allocate_segment(concurrent_vector_base_v3 &v, size_type n) {
        void *ptr = v.vector_allocator_ptr(v, n);
        if(!ptr) throw bad_alloc(); // check for bad allocation, throw exception
        return ptr;
    }

    //! Publish segment so other threads can see it.
    inline static void publish_segment( segment_t& s, void* rhs ) {
    // see also itt_store_pointer_with_release_v3()
        ITT_NOTIFY( sync_releasing, &s.array );
        __TBB_store_with_release( s.array, rhs );
    }

    static size_type enable_segment(concurrent_vector_base_v3 &v, size_type k, size_type element_size) {
        segment_t* s = v.my_segment; // TODO: optimize out as argument? Optimize accesses to my_first_block
        __TBB_ASSERT( s[k].array <= internal::vector_allocation_error_flag, "concurrent operation during growth?" );
        if( !k ) {
            assign_first_segment_if_neccessary(v, default_initial_segments-1);
            try {
                publish_segment(s[0], allocate_segment(v, segment_size(v.my_first_block) ) );
            } catch(...) { // intercept exception here, assign internal::vector_allocation_error_flag value, re-throw exception
                publish_segment(s[0], internal::vector_allocation_error_flag); throw;
            }
            return 2;
        }
        size_type m = segment_size(k);
        if( !v.my_first_block ) // push_back only
            spin_wait_while_eq( v.my_first_block, segment_index_t(0) );
        if( k < v.my_first_block ) {
            // s[0].array is changed only once ( 0 -> !0 ) and points to uninitialized memory
            void *array0 = __TBB_load_with_acquire(s[0].array);
            if( !array0 ) {
                // sync_prepare called only if there is a wait
                ITT_NOTIFY(sync_prepare, &s[0].array );
                spin_wait_while_eq( s[0].array, (void*)0 );
                array0 = __TBB_load_with_acquire(s[0].array);
            }
            ITT_NOTIFY(sync_acquired, &s[0].array);
            if( array0 <= internal::vector_allocation_error_flag ) { // check for internal::vector_allocation_error_flag of initial segment
                publish_segment(s[k], internal::vector_allocation_error_flag); // and assign internal::vector_allocation_error_flag here
                throw bad_last_alloc(); // throw custom exception
            }
            publish_segment( s[k],
                    static_cast<void*>( static_cast<char*>(array0) + segment_base(k)*element_size )
            );
        } else {
            try {
                publish_segment(s[k], allocate_segment(v, m));
            } catch(...) { // intercept exception here, assign internal::vector_allocation_error_flag value, re-throw exception
                publish_segment(s[k], internal::vector_allocation_error_flag); throw;
            }
        }
        return m;
    }

    inline static void extend_table_if_necessary(concurrent_vector_base_v3 &v, size_type k, size_type start ) {
        if(k >= pointers_per_short_table && v.my_segment == v.my_storage)
            extend_segment_table(v, start );
    }

    static void extend_segment_table(concurrent_vector_base_v3 &v, size_type start) {
        if( start > segment_size(pointers_per_short_table) ) start = segment_size(pointers_per_short_table);
        // If other threads are trying to set pointers in the short segment, wait for them to finish their
        // assigments before we copy the short segment to the long segment. Note: grow_to_at_least depends on it
        for( segment_index_t i = 0; segment_base(i) < start && v.my_segment == v.my_storage; i++ )
            if(!v.my_storage[i].array) {
                ITT_NOTIFY(sync_prepare, &v.my_storage[i].array);
                atomic_backoff backoff;
                do backoff.pause(); while( v.my_segment == v.my_storage && !v.my_storage[i].array );
                ITT_NOTIFY(sync_acquired, &v.my_storage[i].array);
            }
        if( v.my_segment != v.my_storage ) return;

        segment_t* s = (segment_t*)NFS_Allocate( pointers_per_long_table, sizeof(segment_t), NULL );
        // if( !s ) throw bad_alloc() -- implemented in NFS_Allocate
        memset( s, 0, pointers_per_long_table*sizeof(segment_t) );
        for( segment_index_t i = 0; i < pointers_per_short_table; i++)
            s[i] = v.my_storage[i];
        if( v.my_segment.compare_and_swap( s, v.my_storage ) != v.my_storage )
            NFS_Free( s );
    }

    inline static segment_t &acquire_segment(concurrent_vector_base_v3 &v, size_type index, size_type element_size, bool owner) {
        segment_t &s = v.my_segment[index]; // TODO: pass v.my_segment as arument
        if( !__TBB_load_with_acquire(s.array) ) { // do not check for internal::vector_allocation_error_flag 
            if( owner ) {
                enable_segment( v, index, element_size );
            } else {
                ITT_NOTIFY(sync_prepare, &s.array);
                spin_wait_while_eq( s.array, (void*)0 );
                ITT_NOTIFY(sync_acquired, &s.array);
            }
        } else {
            ITT_NOTIFY(sync_acquired, &s.array);
        }
        if( s.array <= internal::vector_allocation_error_flag ) // check for internal::vector_allocation_error_flag
            throw bad_last_alloc(); // throw custom exception, because it's hard to recover after internal::vector_allocation_error_flag correctly
        return s;
    }

    ///// non-static fields of helper for exception-safe iteration across segments
    segment_t *table;// TODO: review all segment_index_t as just short type
    size_type first_block, k, sz, start, finish, element_size;
    helper(segment_t *segments, size_type fb, size_type esize, size_type index, size_type s, size_type f) throw()
        : table(segments), first_block(fb), k(index), sz(0), start(s), finish(f), element_size(esize) {}
    inline void first_segment() throw() {
        __TBB_ASSERT( start <= finish, NULL );
        __TBB_ASSERT( first_block || !finish, NULL );
        if( k < first_block ) k = 0; // process solid segment at a time
        size_type base = segment_base( k );
        __TBB_ASSERT( base <= start, NULL );
        finish -= base; start -= base; // rebase as offsets from segment k
        sz = k ? base : segment_size( first_block ); // sz==base for k>0
    }
    inline void next_segment() throw() {
        finish -= sz; start = 0; // offsets from next segment
        if( !k ) k = first_block;
        else { ++k; sz <<= 1; }
    }
    template<typename F>
    inline size_type apply(const F &func) {
        first_segment();
        while( sz < finish ) { // work for more than one segment
            func( table[k], static_cast<char*>(table[k].array)+element_size*start, sz-start );
            next_segment();
        }
        func( table[k], static_cast<char*>(table[k].array)+element_size*start, finish-start );
        return k;
    }
    inline void *get_segment_ptr(size_type index, bool wait) {
        segment_t &s = table[index];
        if( !__TBB_load_with_acquire(s.array) && wait ) {
            ITT_NOTIFY(sync_prepare, &s.array);
            spin_wait_while_eq( s.array, (void*)0 );
            ITT_NOTIFY(sync_acquired, &s.array);
        }
        return s.array;
    }
    ~helper() {
        if( sz >= finish ) return; // the work is done correctly
        if( !sz ) { // allocation failed, restore the table
            segment_index_t k_start = k, k_end = segment_index_of(finish-1);
            if( segment_base( k_start ) < start )
                get_segment_ptr(k_start++, true); // wait
            if( k_start < first_block ) {
                void *array0 = get_segment_ptr(0, start>0); // wait if necessary
                if( array0 && !k_start ) ++k_start;
                if( array0 <= internal::vector_allocation_error_flag )
                    for(; k_start < first_block && k_start <= k_end; ++k_start )
                        publish_segment(table[k_start], internal::vector_allocation_error_flag);
                else for(; k_start < first_block && k_start <= k_end; ++k_start )
                        publish_segment(table[k_start], static_cast<void*>(
                            static_cast<char*>(array0) + segment_base(k_start)*element_size) );
            }
            for(; k_start <= k_end; ++k_start ) // not in first block
                if( !__TBB_load_with_acquire(table[k_start].array) )
                    publish_segment(table[k_start], internal::vector_allocation_error_flag);
            // fill alocated items
            first_segment();
            goto recover;
        }
        while( sz <= finish ) { // there is still work for at least one segment
            next_segment();
recover:
            void *array = table[k].array;
            if( array > internal::vector_allocation_error_flag )
                std::memset( static_cast<char*>(array)+element_size*start, 0, ((sz<finish?sz:finish) - start)*element_size );
            else __TBB_ASSERT( array == internal::vector_allocation_error_flag, NULL );
        }
    }
    struct init_body {
        internal_array_op2 func;
        const void *arg;
        init_body(internal_array_op2 init, const void *src) : func(init), arg(src) {}
        void operator()(segment_t &, void *begin, size_type n) const {
            func( begin, arg, n );
        }
    };
    struct safe_init_body {
        internal_array_op2 func;
        const void *arg;
        safe_init_body(internal_array_op2 init, const void *src) : func(init), arg(src) {}
        void operator()(segment_t &s, void *begin, size_type n) const {
            if( s.array <= internal::vector_allocation_error_flag )
                throw bad_last_alloc(); // throw custom exception
            func( begin, arg, n );
        }
    };
    struct destroy_body {
        internal_array_op1 func;
        destroy_body(internal_array_op1 destroy) : func(destroy) {}
        void operator()(segment_t &s, void *begin, size_type n) const {
            if( s.array > internal::vector_allocation_error_flag )
                func( begin, n );
        }
    };
};

concurrent_vector_base_v3::~concurrent_vector_base_v3() {
    segment_t* s = my_segment;
    if( s != my_storage ) {
        // Clear short segment.
        for( segment_index_t i = 0; i < pointers_per_short_table; i++)
            my_storage[i].array = NULL;
#if TBB_USE_DEBUG
        for( segment_index_t i = 0; i < pointers_per_long_table; i++)
            __TBB_ASSERT( my_segment[i].array <= internal::vector_allocation_error_flag, "Segment should have been freed. Please recompile with new TBB before using exceptions.");
#endif
        my_segment = my_storage;
        NFS_Free( s );
    }
}

concurrent_vector_base_v3::size_type concurrent_vector_base_v3::internal_capacity() const {
    return segment_base( helper::find_segment_end(*this) );
}

void concurrent_vector_base_v3::internal_throw_exception(size_type t) const {
    switch(t) {
        case 0: throw out_of_range("Index out of requested size range");
        case 1: throw range_error ("Index out of allocated segment slots");
        case 2: throw range_error ("Index is not allocated");
    }
}

void concurrent_vector_base_v3::internal_reserve( size_type n, size_type element_size, size_type max_size ) {
    if( n>max_size ) {
        throw length_error("argument to concurrent_vector::reserve exceeds concurrent_vector::max_size()");
    }
    __TBB_ASSERT( n, NULL );
    helper::assign_first_segment_if_neccessary(*this, segment_index_of(n-1));
    segment_index_t k = helper::find_segment_end(*this);
    try {
        for( ; segment_base(k)<n; ++k ) {
            helper::extend_table_if_necessary(*this, k, 0);
            if(my_segment[k].array <= internal::vector_allocation_error_flag)
                helper::enable_segment(*this, k, element_size);
        }
    } catch(...) {
        my_segment[k].array = NULL; throw; // repair and rethrow
    }
}

void concurrent_vector_base_v3::internal_copy( const concurrent_vector_base_v3& src, size_type element_size, internal_array_op2 copy ) {
    size_type n = src.my_early_size;
    __TBB_ASSERT( my_segment == my_storage, NULL);
    if( n ) {
        helper::assign_first_segment_if_neccessary(*this, segment_index_of(n-1));
        size_type b;
        for( segment_index_t k=0; (b=segment_base(k))<n; ++k ) {
            if( (src.my_segment == (segment_t*)src.my_storage && k >= pointers_per_short_table)
                || src.my_segment[k].array <= internal::vector_allocation_error_flag ) {
                my_early_size = b; break;
            }
            helper::extend_table_if_necessary(*this, k, 0);
            size_type m = helper::enable_segment(*this, k, element_size);
            if( m > n-b ) m = n-b;
            my_early_size = b+m;
            copy( my_segment[k].array, src.my_segment[k].array, m );
        }
    }
}

void concurrent_vector_base_v3::internal_assign( const concurrent_vector_base_v3& src, size_type element_size, internal_array_op1 destroy, internal_array_op2 assign, internal_array_op2 copy ) {
    size_type n = src.my_early_size;
    while( my_early_size>n ) { // TODO: improve
        segment_index_t k = segment_index_of( my_early_size-1 );
        size_type b=segment_base(k);
        size_type new_end = b>=n ? b : n;
        __TBB_ASSERT( my_early_size>new_end, NULL );
        if( my_segment[k].array <= internal::vector_allocation_error_flag) // check vector was broken before
            throw bad_last_alloc(); // throw custom exception
        // destructors are supposed to not throw any exceptions
        destroy( (char*)my_segment[k].array+element_size*(new_end-b), my_early_size-new_end );
        my_early_size = new_end;
    }
    size_type dst_initialized_size = my_early_size;
    my_early_size = n;
    helper::assign_first_segment_if_neccessary(*this, segment_index_of(n));
    size_type b;
    for( segment_index_t k=0; (b=segment_base(k))<n; ++k ) {
        if( (src.my_segment == (segment_t*)src.my_storage && k >= pointers_per_short_table)
            || src.my_segment[k].array <= internal::vector_allocation_error_flag ) { // if source is damaged
                my_early_size = b; break; // TODO: it may cause undestructed items
        }
        helper::extend_table_if_necessary(*this, k, 0);
        if( !my_segment[k].array )
            helper::enable_segment(*this, k, element_size);
        else if( my_segment[k].array <= internal::vector_allocation_error_flag )
            throw bad_last_alloc(); // throw custom exception
        size_type m = k? segment_size(k) : 2;
        if( m > n-b ) m = n-b;
        size_type a = 0;
        if( dst_initialized_size>b ) {
            a = dst_initialized_size-b;
            if( a>m ) a = m;
            assign( my_segment[k].array, src.my_segment[k].array, a );
            m -= a;
            a *= element_size;
        }
        if( m>0 )
            copy( (char*)my_segment[k].array+a, (char*)src.my_segment[k].array+a, m );
    }
    __TBB_ASSERT( src.my_early_size==n, "detected use of concurrent_vector::operator= with right side that was concurrently modified" );
}

void* concurrent_vector_base_v3::internal_push_back( size_type element_size, size_type& index ) {
    __TBB_ASSERT( sizeof(my_early_size)==sizeof(uintptr), NULL );
    size_type tmp = __TBB_FetchAndIncrementWacquire(&my_early_size);
    index = tmp;
    segment_index_t k_old = segment_index_of( tmp );
    size_type base = segment_base(k_old);
    helper::extend_table_if_necessary(*this, k_old, tmp);
    segment_t& s = helper::acquire_segment(*this, k_old, element_size, base==tmp);
    size_type j_begin = tmp-base;
    return (void*)((char*)s.array+element_size*j_begin);
}

void concurrent_vector_base_v3::internal_grow_to_at_least( size_type new_size, size_type element_size, internal_array_op2 init, const void *src ) {
    internal_grow_to_at_least_with_result( new_size, element_size, init, src );
}

concurrent_vector_base_v3::size_type concurrent_vector_base_v3::internal_grow_to_at_least_with_result( size_type new_size, size_type element_size, internal_array_op2 init, const void *src ) {
    size_type e = my_early_size;
    while( e<new_size ) {
        size_type f = my_early_size.compare_and_swap(new_size,e);
        if( f==e ) {
            internal_grow( e, new_size, element_size, init, src );
            break;
        }
        e = f;
    }
    // Check/wait for segments allocation completes
    segment_index_t i, k_old = segment_index_of( new_size-1 );
    if( k_old >= pointers_per_short_table && my_segment == my_storage ) {
        spin_wait_while_eq( my_segment, my_storage );
    }
    for( i = 0; i <= k_old; ++i ) {
        segment_t &s = my_segment[i];
        if(!s.array) {
            ITT_NOTIFY(sync_prepare, &s.array);
            atomic_backoff backoff;
            do backoff.pause();
            while( !__TBB_load_with_acquire(my_segment[i].array) ); // my_segment may change concurrently
            ITT_NOTIFY(sync_acquired, &s.array);
        }
        if( my_segment[i].array <= internal::vector_allocation_error_flag )
            throw bad_last_alloc();
    }
#if TBB_USE_DEBUG
    size_type capacity = internal_capacity();
    __TBB_ASSERT( capacity >= new_size, NULL);
#endif
    return e;
}

concurrent_vector_base_v3::size_type concurrent_vector_base_v3::internal_grow_by( size_type delta, size_type element_size, internal_array_op2 init, const void *src ) {
    size_type result = my_early_size.fetch_and_add(delta);
    internal_grow( result, result+delta, element_size, init, src );
    return result;
}

void concurrent_vector_base_v3::internal_grow( const size_type start, size_type finish, size_type element_size, internal_array_op2 init, const void *src ) {
    __TBB_ASSERT( start<finish, "start must be less than finish" );
    segment_index_t k_start = segment_index_of(start), k_end = segment_index_of(finish-1);
    helper::assign_first_segment_if_neccessary(*this, k_end);
    helper::extend_table_if_necessary(*this, k_end, start);
    helper range(my_segment, my_first_block, element_size, k_start, start, finish);
    for(; k_end > k_start && k_end >= range.first_block; --k_end ) // allocate segments in reverse order
        helper::acquire_segment(*this, k_end, element_size, true/*for k_end>k_start*/);
    for(; k_start <= k_end; ++k_start ) // but allocate first block in straight order
        helper::acquire_segment(*this, k_start, element_size, segment_base( k_start ) >= start );
    range.apply( helper::init_body(init, src) );
}

void concurrent_vector_base_v3::internal_resize( size_type n, size_type element_size, size_type max_size, const void *src,
                                                internal_array_op1 destroy, internal_array_op2 init ) {
    size_type j = my_early_size;
    if( n > j ) { // construct items
        internal_reserve(n, element_size, max_size);
        my_early_size = n;
        helper for_each(my_segment, my_first_block, element_size, segment_index_of(j), j, n);
        for_each.apply( helper::safe_init_body(init, src) );
    } else {
        my_early_size = n;
        helper for_each(my_segment, my_first_block, element_size, segment_index_of(n), n, j);
        for_each.apply( helper::destroy_body(destroy) );
    }
}

concurrent_vector_base_v3::segment_index_t concurrent_vector_base_v3::internal_clear( internal_array_op1 destroy ) {
    __TBB_ASSERT( my_segment, NULL );
    size_type j = my_early_size;
    my_early_size = 0;
    helper for_each(my_segment, my_first_block, 0, 0, 0, j); // element_size is safe to be zero if 'start' is zero
    j = for_each.apply( helper::destroy_body(destroy) );
    size_type i = helper::find_segment_end(*this);
    return j < i? i : j+1;
}

void *concurrent_vector_base_v3::internal_compact( size_type element_size, void *table, internal_array_op1 destroy, internal_array_op2 copy )
{
    const size_type my_size = my_early_size;
    const segment_index_t k_end = helper::find_segment_end(*this); // allocated segments
    const segment_index_t k_stop = my_size? segment_index_of(my_size-1) + 1 : 0; // number of segments to store existing items: 0=>0; 1,2=>1; 3,4=>2; [5-8]=>3;..
    const segment_index_t first_block = my_first_block; // number of merged segments, getting values from atomics

    segment_index_t k = first_block;
    if(k_stop < first_block)
        k = k_stop;
    else
        while (k < k_stop && helper::incompact_predicate(segment_size( k ) * element_size) ) k++;
    if(k_stop == k_end && k == first_block)
        return NULL;

    segment_t *const segment_table = my_segment;
    internal_segments_table &old = *static_cast<internal_segments_table*>( table );
    memset(&old, 0, sizeof(old));

    if ( k != first_block && k ) // first segment optimization
    {
        // exception can occur here
        void *seg = old.table[0] = helper::allocate_segment( *this, segment_size(k) );
        old.first_block = k; // fill info for freeing new segment if exception occurs
        // copy items to the new segment
        size_type my_segment_size = segment_size( first_block );
        for (segment_index_t i = 0, j = 0; i < k && j < my_size; j = my_segment_size) {
            __TBB_ASSERT( segment_table[i].array > internal::vector_allocation_error_flag, NULL);
            void *s = static_cast<void*>(
                static_cast<char*>(seg) + segment_base(i)*element_size );
            if(j + my_segment_size >= my_size) my_segment_size = my_size - j;
            try { // exception can occur here
                copy( s, segment_table[i].array, my_segment_size );
            } catch(...) { // destroy all the already copied items
                helper for_each(reinterpret_cast<segment_t*>(&old.table[0]), old.first_block, element_size,
                    0, 0, segment_base(i)+my_segment_size);
                for_each.apply( helper::destroy_body(destroy) );
                throw;
            }
            my_segment_size = i? segment_size( ++i ) : segment_size( i = first_block );
        }
        // commit the changes
        memcpy(old.table, segment_table, k * sizeof(segment_t));
        for (segment_index_t i = 0; i < k; i++) {
            segment_table[i].array = static_cast<void*>(
                static_cast<char*>(seg) + segment_base(i)*element_size );
        }
        old.first_block = first_block; my_first_block = k; // now, first_block != my_first_block
        // destroy original copies
        my_segment_size = segment_size( first_block ); // old.first_block actually
        for (segment_index_t i = 0, j = 0; i < k && j < my_size; j = my_segment_size) {
            if(j + my_segment_size >= my_size) my_segment_size = my_size - j;
            // destructors are supposed to not throw any exceptions
            destroy( old.table[i], my_segment_size );
            my_segment_size = i? segment_size( ++i ) : segment_size( i = first_block );
        }
    }
    // free unnecessary segments allocated by reserve() call
    if ( k_stop < k_end ) {
        old.first_block = first_block;
        memcpy(old.table+k_stop, segment_table+k_stop, (k_end-k_stop) * sizeof(segment_t));
        memset(segment_table+k_stop, 0, (k_end-k_stop) * sizeof(segment_t));
        if( !k ) my_first_block = 0;
    }
    return table;
}

void concurrent_vector_base_v3::internal_swap(concurrent_vector_base_v3& v)
{
    size_type my_sz = my_early_size, v_sz = v.my_early_size;
    if(!my_sz && !v_sz) return;
    size_type tmp = my_first_block; my_first_block = v.my_first_block; v.my_first_block = tmp;
    bool my_short = (my_segment == my_storage), v_short  = (v.my_segment == v.my_storage);
    if ( my_short && v_short ) { // swap both tables
        char tbl[pointers_per_short_table * sizeof(segment_t)];
        memcpy(tbl, my_storage, pointers_per_short_table * sizeof(segment_t));
        memcpy(my_storage, v.my_storage, pointers_per_short_table * sizeof(segment_t));
        memcpy(v.my_storage, tbl, pointers_per_short_table * sizeof(segment_t));
    }
    else if ( my_short ) { // my -> v
        memcpy(v.my_storage, my_storage, pointers_per_short_table * sizeof(segment_t));
        my_segment = v.my_segment; v.my_segment = v.my_storage;
    }
    else if ( v_short ) { // v -> my
        memcpy(my_storage, v.my_storage, pointers_per_short_table * sizeof(segment_t));
        v.my_segment = my_segment; my_segment = my_storage;
    } else {
        segment_t *ptr = my_segment; my_segment = v.my_segment; v.my_segment = ptr;
    }
    my_early_size = v_sz; v.my_early_size = my_sz;
}

} // namespace internal

} // tbb
