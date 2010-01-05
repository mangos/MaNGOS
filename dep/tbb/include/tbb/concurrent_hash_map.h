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

#ifndef __TBB_concurrent_hash_map_H
#define __TBB_concurrent_hash_map_H

#include <stdexcept>
#include <iterator>
#include <utility>      // Need std::pair
#include <cstring>      // Need std::memset
#include <string>
#include "tbb_stddef.h"
#include "cache_aligned_allocator.h"
#include "tbb_allocator.h"
#include "spin_rw_mutex.h"
#include "atomic.h"
#include "aligned_space.h"
#if TBB_USE_PERFORMANCE_WARNINGS
#include <typeinfo>
#endif

namespace tbb {

template<typename T> struct tbb_hash_compare;
template<typename Key, typename T, typename HashCompare = tbb_hash_compare<Key>, typename A = tbb_allocator<std::pair<Key, T> > >
class concurrent_hash_map;

//! @cond INTERNAL
namespace internal {
    //! ITT instrumented routine that loads pointer from location pointed to by src.
    void* __TBB_EXPORTED_FUNC itt_load_pointer_with_acquire_v3( const void* src );
    //! ITT instrumented routine that stores src into location pointed to by dst.
    void __TBB_EXPORTED_FUNC itt_store_pointer_with_release_v3( void* dst, void* src );
    //! Routine that loads pointer from location pointed to by src without causing ITT to report a race.
    void* __TBB_EXPORTED_FUNC itt_load_pointer_v3( const void* src );

    //! Type of a hash code.
    typedef size_t hashcode_t;
    //! Node base type
    struct hash_map_node_base : no_copy {
        //! Mutex type
        typedef spin_rw_mutex mutex_t;
        //! Scoped lock type for mutex
        typedef mutex_t::scoped_lock scoped_t;
        //! Next node in chain
        hash_map_node_base *next;
        mutex_t mutex;
    };
    //! Incompleteness flag value
    static hash_map_node_base *const rehash_req = reinterpret_cast<hash_map_node_base*>(size_t(3));
    //! Rehashed empty bucket flag
    static hash_map_node_base *const empty_rehashed = reinterpret_cast<hash_map_node_base*>(size_t(0));
    //! base class of concurrent_hash_map
    class hash_map_base {
    public:
        //! Size type
        typedef size_t size_type;
        //! Type of a hash code.
        typedef size_t hashcode_t;
        //! Segment index type
        typedef size_t segment_index_t;
        //! Node base type
        typedef hash_map_node_base node_base;
        //! Bucket type
        struct bucket : no_copy {
            //! Mutex type for buckets
            typedef spin_rw_mutex mutex_t;
            //! Scoped lock type for mutex
            typedef mutex_t::scoped_lock scoped_t;
            mutex_t mutex;
            node_base *node_list;
        };
        //! Count of segments in the first block
        static size_type const embedded_block = 1;
        //! Count of segments in the first block
        static size_type const embedded_buckets = 1<<embedded_block;
        //! Count of segments in the first block
        static size_type const first_block = 8; //including embedded_block. perfect with bucket size 16, so the allocations are power of 4096
        //! Size of a pointer / table size
        static size_type const pointers_per_table = sizeof(segment_index_t) * 8; // one segment per bit
        //! Segment pointer
        typedef bucket *segment_ptr_t;
        //! Segment pointers table type
        typedef segment_ptr_t segments_table_t[pointers_per_table];
        //! Hash mask = sum of allocated segments sizes - 1
        atomic<hashcode_t> my_mask;
        //! Segment pointers table. Also prevents false sharing between my_mask and my_size
        segments_table_t my_table;
        //! Size of container in stored items
        atomic<size_type> my_size; // It must be in separate cache line from my_mask due to performance effects
        //! Zero segment
        bucket my_embedded_segment[embedded_buckets];

        //! Constructor
        hash_map_base() {
            std::memset( this, 0, pointers_per_table*sizeof(segment_ptr_t) // 32*4=128   or 64*8=512
                + sizeof(my_size) + sizeof(my_mask)  // 4+4 or 8+8
                + embedded_buckets*sizeof(bucket) ); // n*8 or n*16
            for( size_type i = 0; i < embedded_block; i++ ) // fill the table
                my_table[i] = my_embedded_segment + segment_base(i);
            my_mask = embedded_buckets - 1;
            __TBB_ASSERT( embedded_block <= first_block, "The first block number must include embedded blocks");
        }

        //! @return segment index of given index in the array
        static segment_index_t segment_index_of( size_type index ) {
            return segment_index_t( __TBB_Log2( index|1 ) );
        }

        //! @return the first array index of given segment
        static segment_index_t segment_base( segment_index_t k ) {
            return (segment_index_t(1)<<k & ~segment_index_t(1));
        }

        //! @return segment size except for @arg k == 0
        static size_type segment_size( segment_index_t k ) {
            return size_type(1)<<k; // fake value for k==0
        }
        
        //! @return true if @arg ptr is valid pointer
        static bool is_valid( void *ptr ) {
            return reinterpret_cast<size_t>(ptr) > size_t(63);
        }

        //! Initialize buckets
        static void init_buckets( segment_ptr_t ptr, size_type sz, bool is_initial ) {
            if( is_initial ) std::memset(ptr, 0, sz*sizeof(bucket) );
            else for(size_type i = 0; i < sz; i++, ptr++) {
                    *reinterpret_cast<intptr_t*>(&ptr->mutex) = 0;
                    ptr->node_list = rehash_req;
                }
        }
        
        //! Add node @arg n to bucket @arg b
        static void add_to_bucket( bucket *b, node_base *n ) {
            __TBB_ASSERT(b->node_list != rehash_req, NULL);
            n->next = b->node_list;
            b->node_list = n; // its under lock and flag is set
        }

        //! Exception safety helper
        struct enable_segment_failsafe {
            segment_ptr_t *my_segment_ptr;
            enable_segment_failsafe(segments_table_t &table, segment_index_t k) : my_segment_ptr(&table[k]) {}
            ~enable_segment_failsafe() {
                if( my_segment_ptr ) *my_segment_ptr = 0; // indicate no allocation in progress
            }
        };

        //! Enable segment
        void enable_segment( segment_index_t k, bool is_initial = false ) {
            __TBB_ASSERT( k, "Zero segment must be embedded" );
            enable_segment_failsafe watchdog( my_table, k );
            cache_aligned_allocator<bucket> alloc;
            size_type sz;
            __TBB_ASSERT( !is_valid(my_table[k]), "Wrong concurrent assignment");
            if( k >= first_block ) {
                sz = segment_size( k );
                segment_ptr_t ptr = alloc.allocate( sz );
                init_buckets( ptr, sz, is_initial );
#if TBB_USE_THREADING_TOOLS
                // TODO: actually, fence and notification are unnecessary here and below
                itt_store_pointer_with_release_v3( my_table + k, ptr );
#else
                my_table[k] = ptr;// my_mask has release fence
#endif
                sz <<= 1;// double it to get entire capacity of the container
            } else { // the first block
                __TBB_ASSERT( k == embedded_block, "Wrong segment index" );
                sz = segment_size( first_block );
                segment_ptr_t ptr = alloc.allocate( sz - embedded_buckets );
                init_buckets( ptr, sz - embedded_buckets, is_initial );
                ptr -= segment_base(embedded_block);
                for(segment_index_t i = embedded_block; i < first_block; i++) // calc the offsets
#if TBB_USE_THREADING_TOOLS
                    itt_store_pointer_with_release_v3( my_table + i, ptr + segment_base(i) );
#else
                    my_table[i] = ptr + segment_base(i);
#endif
            }
#if TBB_USE_THREADING_TOOLS
            itt_store_pointer_with_release_v3( &my_mask, (void*)(sz-1) );
#else
            my_mask = sz - 1;
#endif
            watchdog.my_segment_ptr = 0;
        }

        //! Get bucket by (masked) hashcode
        bucket *get_bucket( hashcode_t h ) const throw() { // TODO: add throw() everywhere?
            segment_index_t s = segment_index_of( h );
            h -= segment_base(s);
            segment_ptr_t seg = my_table[s];
            __TBB_ASSERT( is_valid(seg), "hashcode must be cut by valid mask for allocated segments" );
            return &seg[h];
        }

        //! Check for mask race
        // Splitting into two functions should help inlining
        inline bool check_mask_race( const hashcode_t h, hashcode_t &m ) const {
            hashcode_t m_now, m_old = m;
#if TBB_USE_THREADING_TOOLS
            m_now = (hashcode_t) itt_load_pointer_with_acquire_v3( &my_mask );
#else
            m_now = my_mask;
#endif
            if( m_old != m_now )
                return check_rehashing_collision( h, m_old, m = m_now );
            return false;
        }

        //! Process mask race, check for rehashing collision
        bool check_rehashing_collision( const hashcode_t h, hashcode_t m_old, hashcode_t m ) const {
            __TBB_ASSERT(m_old != m, NULL); // TODO?: m arg could be optimized out by passing h = h&m
            if( (h & m_old) != (h & m) ) { // mask changed for this hashcode, rare event
                // condition above proves that 'h' has some other bits set beside 'm_old'
                // find next applicable mask after m_old    //TODO: look at bsl instruction
                for( ++m_old; !(h & m_old); m_old <<= 1 ); // at maximum few rounds depending on the first block size
                m_old = (m_old<<1) - 1; // get full mask from a bit
                __TBB_ASSERT((m_old&(m_old+1))==0 && m_old <= m, NULL);
                // check whether it is rehashing/ed
#if TBB_USE_THREADING_TOOLS
                if( itt_load_pointer_with_acquire_v3(&( get_bucket(h & m_old)->node_list )) != rehash_req )
#else
                if( __TBB_load_with_acquire(get_bucket( h & m_old )->node_list) != rehash_req )
#endif
                    return true;
            }
            return false;
        }

        //! Insert a node and check for load factor. @return segment index to enable.
        segment_index_t insert_new_node( bucket *b, node_base *n, hashcode_t mask ) {
            size_type sz = ++my_size; // prefix form is to enforce allocation after the first item inserted
            add_to_bucket( b, n );
            // check load factor
            if( sz >= mask ) { // TODO: add custom load_factor 
                segment_index_t new_seg = segment_index_of( mask+1 );
                __TBB_ASSERT( is_valid(my_table[new_seg-1]), "new allocations must not publish new mask until segment has allocated");
#if TBB_USE_THREADING_TOOLS
                if( !itt_load_pointer_v3(my_table+new_seg)
#else
                if( !my_table[new_seg]
#endif
                  && __TBB_CompareAndSwapW(&my_table[new_seg], 2, 0) == 0 )
                    return new_seg; // The value must be processed
            }
            return 0;
        }

        //! Prepare enough segments for number of buckets
        void reserve(size_type buckets) {
            if( !buckets-- ) return;
            bool is_initial = !my_size;
            for( size_type m = my_mask; buckets > m; m = my_mask )
                enable_segment( segment_index_of( m+1 ), is_initial );
        }
        //! Swap hash_map_bases
        void internal_swap(hash_map_base &table) {
            std::swap(this->my_mask, table.my_mask);
            std::swap(this->my_size, table.my_size);
            for(size_type i = 0; i < embedded_buckets; i++)
                std::swap(this->my_embedded_segment[i].node_list, table.my_embedded_segment[i].node_list);
            for(size_type i = embedded_block; i < pointers_per_table; i++)
                std::swap(this->my_table[i], table.my_table[i]);
        }
    };

    template<typename Iterator>
    class hash_map_range;

    //! Meets requirements of a forward iterator for STL */
    /** Value is either the T or const T type of the container.
        @ingroup containers */ 
    template<typename Container, typename Value>
    class hash_map_iterator
        : public std::iterator<std::forward_iterator_tag,Value>
    {
        typedef Container map_type;
        typedef typename Container::node node;
        typedef hash_map_base::node_base node_base;
        typedef hash_map_base::bucket bucket;

        template<typename C, typename T, typename U>
        friend bool operator==( const hash_map_iterator<C,T>& i, const hash_map_iterator<C,U>& j );

        template<typename C, typename T, typename U>
        friend bool operator!=( const hash_map_iterator<C,T>& i, const hash_map_iterator<C,U>& j );

        template<typename C, typename T, typename U>
        friend ptrdiff_t operator-( const hash_map_iterator<C,T>& i, const hash_map_iterator<C,U>& j );
    
        template<typename C, typename U>
        friend class internal::hash_map_iterator;

        template<typename I>
        friend class internal::hash_map_range;

        void advance_to_next_bucket() { // TODO?: refactor to iterator_base class
            size_t k = my_index+1;
            while( my_bucket && k <= my_map->my_mask ) {
                // Following test uses 2's-complement wizardry
                if( k& (k-2) ) // not the beginning of a segment
                    ++my_bucket;
                else my_bucket = my_map->get_bucket( k );
                my_node = static_cast<node*>( my_bucket->node_list );
                if( hash_map_base::is_valid(my_node) ) {
                    my_index = k; return;
                }
                ++k;
            }
            my_bucket = 0; my_node = 0; my_index = k; // the end
        }
#if !defined(_MSC_VER) || defined(__INTEL_COMPILER)
        template<typename Key, typename T, typename HashCompare, typename A>
        friend class tbb::concurrent_hash_map;
#else
    public: // workaround
#endif
        //! concurrent_hash_map over which we are iterating.
        const Container *my_map;

        //! Index in hash table for current item
        size_t my_index;

        //! Pointer to bucket
        const bucket *my_bucket;

        //! Pointer to node that has current item
        node *my_node;

        hash_map_iterator( const Container &map, size_t index, const bucket *b, node_base *n );

    public:
        //! Construct undefined iterator
        hash_map_iterator() {}
        hash_map_iterator( const hash_map_iterator<Container,typename Container::value_type> &other ) :
            my_map(other.my_map),
            my_index(other.my_index),
            my_bucket(other.my_bucket),
            my_node(other.my_node)
        {}
        Value& operator*() const {
            __TBB_ASSERT( hash_map_base::is_valid(my_node), "iterator uninitialized or at end of container?" );
            return my_node->item;
        }
        Value* operator->() const {return &operator*();}
        hash_map_iterator& operator++();
        
        //! Post increment
        Value* operator++(int) {
            Value* result = &operator*();
            operator++();
            return result;
        }
    };

    template<typename Container, typename Value>
    hash_map_iterator<Container,Value>::hash_map_iterator( const Container &map, size_t index, const bucket *b, node_base *n ) :
        my_map(&map),
        my_index(index),
        my_bucket(b),
        my_node( static_cast<node*>(n) )
    {
        if( b && !hash_map_base::is_valid(n) )
            advance_to_next_bucket();
    }

    template<typename Container, typename Value>
    hash_map_iterator<Container,Value>& hash_map_iterator<Container,Value>::operator++() {
        my_node = static_cast<node*>( my_node->next );
        if( !my_node ) advance_to_next_bucket();
        return *this;
    }

    template<typename Container, typename T, typename U>
    bool operator==( const hash_map_iterator<Container,T>& i, const hash_map_iterator<Container,U>& j ) {
        return i.my_node == j.my_node && i.my_map == j.my_map;
    }

    template<typename Container, typename T, typename U>
    bool operator!=( const hash_map_iterator<Container,T>& i, const hash_map_iterator<Container,U>& j ) {
        return i.my_node != j.my_node || i.my_map != j.my_map;
    }

    //! Range class used with concurrent_hash_map
    /** @ingroup containers */ 
    template<typename Iterator>
    class hash_map_range {
        typedef typename Iterator::map_type map_type;
        Iterator my_begin;
        Iterator my_end;
        mutable Iterator my_midpoint;
        size_t my_grainsize;
        //! Set my_midpoint to point approximately half way between my_begin and my_end.
        void set_midpoint() const;
        template<typename U> friend class hash_map_range;
    public:
        //! Type for size of a range
        typedef std::size_t size_type;
        typedef typename Iterator::value_type value_type;
        typedef typename Iterator::reference reference;
        typedef typename Iterator::difference_type difference_type;
        typedef Iterator iterator;

        //! True if range is empty.
        bool empty() const {return my_begin==my_end;}

        //! True if range can be partitioned into two subranges.
        bool is_divisible() const {
            return my_midpoint!=my_end;
        }
        //! Split range.
        hash_map_range( hash_map_range& r, split ) : 
            my_end(r.my_end),
            my_grainsize(r.my_grainsize)
        {
            r.my_end = my_begin = r.my_midpoint;
            __TBB_ASSERT( !empty(), "Splitting despite the range is not divisible" );
            __TBB_ASSERT( !r.empty(), "Splitting despite the range is not divisible" );
            set_midpoint();
            r.set_midpoint();
        }
        //! type conversion
        template<typename U>
        hash_map_range( hash_map_range<U>& r) : 
            my_begin(r.my_begin),
            my_end(r.my_end),
            my_midpoint(r.my_midpoint),
            my_grainsize(r.my_grainsize)
        {}
#if TBB_DEPRECATED
        //! Init range with iterators and grainsize specified
        hash_map_range( const Iterator& begin_, const Iterator& end_, size_type grainsize = 1 ) : 
            my_begin(begin_), 
            my_end(end_),
            my_grainsize(grainsize)
        {
            if(!my_end.my_index && !my_end.my_bucket) // end
                my_end.my_index = my_end.my_map->my_mask + 1;
            set_midpoint();
            __TBB_ASSERT( grainsize>0, "grainsize must be positive" );
        }
#endif
        //! Init range with container and grainsize specified
        hash_map_range( const map_type &map, size_type grainsize = 1 ) : 
            my_begin( Iterator( map, 0, map.my_embedded_segment, map.my_embedded_segment->node_list ) ),
            my_end( Iterator( map, map.my_mask + 1, 0, 0 ) ),
            my_grainsize( grainsize )
        {
            __TBB_ASSERT( grainsize>0, "grainsize must be positive" );
            set_midpoint();
        }
        const Iterator& begin() const {return my_begin;}
        const Iterator& end() const {return my_end;}
        //! The grain size for this range.
        size_type grainsize() const {return my_grainsize;}
    };

    template<typename Iterator>
    void hash_map_range<Iterator>::set_midpoint() const {
        // Split by groups of nodes
        size_t m = my_end.my_index-my_begin.my_index;
        if( m > my_grainsize ) {
            m = my_begin.my_index + m/2u;
            hash_map_base::bucket *b = my_begin.my_map->get_bucket(m);
            my_midpoint = Iterator(*my_begin.my_map,m,b,b->node_list);
        } else {
            my_midpoint = my_end;
        }
        __TBB_ASSERT( my_begin.my_index <= my_midpoint.my_index,
            "my_begin is after my_midpoint" );
        __TBB_ASSERT( my_midpoint.my_index <= my_end.my_index,
            "my_midpoint is after my_end" );
        __TBB_ASSERT( my_begin != my_midpoint || my_begin == my_end,
            "[my_begin, my_midpoint) range should not be empty" );
    }
} // namespace internal
//! @endcond

//! Hash multiplier
static const size_t hash_multiplier = sizeof(size_t)==4? 2654435769U : 11400714819323198485ULL;
//! Hasher functions
template<typename T>
inline static size_t tbb_hasher( const T& t ) {
    return static_cast<size_t>( t ) * hash_multiplier;
}
template<typename P>
inline static size_t tbb_hasher( P* ptr ) {
    size_t const h = reinterpret_cast<size_t>( ptr );
    return (h >> 3) ^ h;
}
template<typename E, typename S, typename A>
inline static size_t tbb_hasher( const std::basic_string<E,S,A>& s ) {
    size_t h = 0;
    for( const E* c = s.c_str(); *c; c++ )
        h = static_cast<size_t>(*c) ^ (h * hash_multiplier);
    return h;
}
template<typename F, typename S>
inline static size_t tbb_hasher( const std::pair<F,S>& p ) {
    return tbb_hasher(p.first) ^ tbb_hasher(p.second);
}

//! hash_compare - default argument
template<typename T>
struct tbb_hash_compare {
    static size_t hash( const T& t ) { return tbb_hasher(t); }
    static bool equal( const T& a, const T& b ) { return a == b; }
};

//! Unordered map from Key to T.
/** concurrent_hash_map is associative container with concurrent access.

@par Compatibility
    The class meets all Container Requirements from C++ Standard (See ISO/IEC 14882:2003(E), clause 23.1).

@par Exception Safety
    - Hash function is not permitted to throw an exception. User-defined types Key and T are forbidden from throwing an exception in destructors.
    - If exception happens during insert() operations, it has no effect (unless exception raised by HashCompare::hash() function during grow_segment).
    - If exception happens during operator=() operation, the container can have a part of source items, and methods size() and empty() can return wrong results.

@par Changes since TBB 2.1
    - Replaced internal algorithm and data structure. Patent is pending.
    - Added buckets number argument for constructor

@par Changes since TBB 2.0
    - Fixed exception-safety
    - Added template argument for allocator
    - Added allocator argument in constructors
    - Added constructor from a range of iterators
    - Added several new overloaded insert() methods
    - Added get_allocator()
    - Added swap()
    - Added count()
    - Added overloaded erase(accessor &) and erase(const_accessor&)
    - Added equal_range() [const]
    - Added [const_]pointer, [const_]reference, and allocator_type types
    - Added global functions: operator==(), operator!=(), and swap() 

    @ingroup containers */
template<typename Key, typename T, typename HashCompare, typename Allocator>
class concurrent_hash_map : protected internal::hash_map_base {
    template<typename Container, typename Value>
    friend class internal::hash_map_iterator;

    template<typename I>
    friend class internal::hash_map_range;

public:
    typedef Key key_type;
    typedef T mapped_type;
    typedef std::pair<const Key,T> value_type;
    typedef internal::hash_map_base::size_type size_type;
    typedef ptrdiff_t difference_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef internal::hash_map_iterator<concurrent_hash_map,value_type> iterator;
    typedef internal::hash_map_iterator<concurrent_hash_map,const value_type> const_iterator;
    typedef internal::hash_map_range<iterator> range_type;
    typedef internal::hash_map_range<const_iterator> const_range_type;
    typedef Allocator allocator_type;

protected:
    friend class const_accessor;
    struct node;
    typedef typename Allocator::template rebind<node>::other node_allocator_type;
    node_allocator_type my_allocator;
    HashCompare my_hash_compare;

    struct node : public node_base {
        value_type item;
        node( const Key &key ) : item(key, T()) {}
        node( const Key &key, const T &t ) : item(key, t) {}
        // exception-safe allocation, see C++ Standard 2003, clause 5.3.4p17
        void *operator new( size_t /*size*/, node_allocator_type &a ) {
            void *ptr = a.allocate(1);
            if(!ptr) throw std::bad_alloc();
            return ptr;
        }
        // match placement-new form above to be called if exception thrown in constructor
        void operator delete( void *ptr, node_allocator_type &a ) {return a.deallocate(static_cast<node*>(ptr),1); }
    };

    void delete_node( node_base *n ) {
        my_allocator.destroy( static_cast<node*>(n) );
        my_allocator.deallocate( static_cast<node*>(n), 1);
    }

    node *search_bucket( const key_type &key, bucket *b ) const {
        node *n = static_cast<node*>( b->node_list );
        while( is_valid(n) && !my_hash_compare.equal(key, n->item.first) )
            n = static_cast<node*>( n->next );
        __TBB_ASSERT(n != internal::rehash_req, "Search can be executed only for rehashed bucket");
        return n;
    }

    //! bucket accessor is to find, rehash, acquire a lock, and access a bucket
    class bucket_accessor : public bucket::scoped_t {
        bool my_is_writer; // TODO: use it from base type
        bucket *my_b;
    public:
        bucket_accessor( concurrent_hash_map *base, const hashcode_t h, bool writer = false ) { acquire( base, h, writer ); }
        //! find a bucket by masked hashcode, optionally rehash, and acquire the lock
        inline void acquire( concurrent_hash_map *base, const hashcode_t h, bool writer = false ) {
            my_b = base->get_bucket( h );
#if TBB_USE_THREADING_TOOLS
            // TODO: actually, notification is unnecessary here, just hiding double-check
            if( itt_load_pointer_with_acquire_v3(&my_b->node_list) == internal::rehash_req
#else
            if( __TBB_load_with_acquire(my_b->node_list) == internal::rehash_req
#endif
                && try_acquire( my_b->mutex, /*write=*/true ) )
            {
                if( my_b->node_list == internal::rehash_req ) base->rehash_bucket( my_b, h ); //recursive rehashing
                my_is_writer = true;
            }
            else bucket::scoped_t::acquire( my_b->mutex, /*write=*/my_is_writer = writer );
            __TBB_ASSERT( my_b->node_list != internal::rehash_req, NULL);
        }
        //! check whether bucket is locked for write
        bool is_writer() { return my_is_writer; }
        //! get bucket pointer
        bucket *operator() () { return my_b; }
        // TODO: optimize out
        bool upgrade_to_writer() { my_is_writer = true; return bucket::scoped_t::upgrade_to_writer(); }
    };

    // TODO refactor to hash_base
    void rehash_bucket( bucket *b_new, const hashcode_t h ) {
        __TBB_ASSERT( *(intptr_t*)(&b_new->mutex), "b_new must be locked (for write)");
        __TBB_ASSERT( h > 1, "The lowermost buckets can't be rehashed" );
        __TBB_store_with_release(b_new->node_list, internal::empty_rehashed); // mark rehashed
        hashcode_t mask = ( 1u<<__TBB_Log2( h ) ) - 1; // get parent mask from the topmost bit

        bucket_accessor b_old( this, h & mask );

        mask = (mask<<1) | 1; // get full mask for new bucket
        __TBB_ASSERT( (mask&(mask+1))==0 && (h & mask) == h, NULL );
    restart:
        for( node_base **p = &b_old()->node_list, *n = __TBB_load_with_acquire(*p); is_valid(n); n = *p ) {
            hashcode_t c = my_hash_compare.hash( static_cast<node*>(n)->item.first );
            if( (c & mask) == h ) {
                if( !b_old.is_writer() )
                    if( !b_old.upgrade_to_writer() ) {
                        goto restart; // node ptr can be invalid due to concurrent erase
                    }
                *p = n->next; // exclude from b_old
                add_to_bucket( b_new, n );
            } else p = &n->next; // iterate to next item
        }
    }

public:
    
    class accessor;
    //! Combines data access, locking, and garbage collection.
    class const_accessor {
        friend class concurrent_hash_map<Key,T,HashCompare,Allocator>;
        friend class accessor;
        void operator=( const accessor & ) const; // Deny access
        const_accessor( const accessor & );       // Deny access
    public:
        //! Type of value
        typedef const typename concurrent_hash_map::value_type value_type;

        //! True if result is empty.
        bool empty() const {return !my_node;}

        //! Set to null
        void release() {
            if( my_node ) {
                my_lock.release();
                my_node = 0;
            }
        }

        //! Return reference to associated value in hash table.
        const_reference operator*() const {
            __TBB_ASSERT( my_node, "attempt to dereference empty accessor" );
            return my_node->item;
        }

        //! Return pointer to associated value in hash table.
        const_pointer operator->() const {
            return &operator*();
        }

        //! Create empty result
        const_accessor() : my_node(NULL) {}

        //! Destroy result after releasing the underlying reference.
        ~const_accessor() {
            my_node = NULL; // my_lock.release() is called in scoped_lock destructor
        }
    private:
        node *my_node;
        typename node::scoped_t my_lock;
        hashcode_t my_hash;
    };

    //! Allows write access to elements and combines data access, locking, and garbage collection.
    class accessor: public const_accessor {
    public:
        //! Type of value
        typedef typename concurrent_hash_map::value_type value_type;

        //! Return reference to associated value in hash table.
        reference operator*() const {
            __TBB_ASSERT( this->my_node, "attempt to dereference empty accessor" );
            return this->my_node->item;
        }

        //! Return pointer to associated value in hash table.
        pointer operator->() const {
            return &operator*();
        }
    };

    //! Construct empty table.
    concurrent_hash_map(const allocator_type &a = allocator_type())
        : my_allocator(a)
    {}

    //! Construct empty table with n preallocated buckets. This number serves also as initial concurrency level.
    concurrent_hash_map(size_type n, const allocator_type &a = allocator_type())
        : my_allocator(a)
    {
        reserve( n );
    }

    //! Copy constructor
    concurrent_hash_map( const concurrent_hash_map& table, const allocator_type &a = allocator_type())
        : my_allocator(a)
    {
        internal_copy(table);
    }

    //! Construction with copying iteration range and given allocator instance
    template<typename I>
    concurrent_hash_map(I first, I last, const allocator_type &a = allocator_type())
        : my_allocator(a)
    {
        reserve( std::distance(first, last) ); // TODO: load_factor?
        internal_copy(first, last);
    }

    //! Assignment
    concurrent_hash_map& operator=( const concurrent_hash_map& table ) {
        if( this!=&table ) {
            clear();
            internal_copy(table);
        } 
        return *this;
    }


    //! Clear table
    void clear();

    //! Clear table and destroy it.  
    ~concurrent_hash_map() { clear(); }

    //------------------------------------------------------------------------
    // Parallel algorithm support
    //------------------------------------------------------------------------
    range_type range( size_type grainsize=1 ) {
        return range_type( *this, grainsize );
    }
    const_range_type range( size_type grainsize=1 ) const {
        return const_range_type( *this, grainsize );
    }

    //------------------------------------------------------------------------
    // STL support - not thread-safe methods
    //------------------------------------------------------------------------
    iterator begin() {return iterator(*this,0,my_embedded_segment,my_embedded_segment->node_list);}
    iterator end() {return iterator(*this,0,0,0);}
    const_iterator begin() const {return const_iterator(*this,0,my_embedded_segment,my_embedded_segment->node_list);}
    const_iterator end() const {return const_iterator(*this,0,0,0);}
    std::pair<iterator, iterator> equal_range( const Key& key ) { return internal_equal_range(key, end()); }
    std::pair<const_iterator, const_iterator> equal_range( const Key& key ) const { return internal_equal_range(key, end()); }
    
    //! Number of items in table.
    size_type size() const { return my_size; }

    //! True if size()==0.
    bool empty() const { return my_size == 0; }

    //! Upper bound on size.
    size_type max_size() const {return (~size_type(0))/sizeof(node);}

    //! return allocator object
    allocator_type get_allocator() const { return this->my_allocator; }

    //! swap two instances. Iterators are invalidated
    void swap(concurrent_hash_map &table);

    //------------------------------------------------------------------------
    // concurrent map operations
    //------------------------------------------------------------------------

    //! Return count of items (0 or 1)
    size_type count( const Key &key ) const {
        return const_cast<concurrent_hash_map*>(this)->lookup(/*insert*/false, key, NULL, NULL, /*write=*/false );
    }

    //! Find item and acquire a read lock on the item.
    /** Return true if item is found, false otherwise. */
    bool find( const_accessor &result, const Key &key ) const {
        result.release();
        return const_cast<concurrent_hash_map*>(this)->lookup(/*insert*/false, key, NULL, &result, /*write=*/false );
    }

    //! Find item and acquire a write lock on the item.
    /** Return true if item is found, false otherwise. */
    bool find( accessor &result, const Key &key ) {
        result.release();
        return lookup(/*insert*/false, key, NULL, &result, /*write=*/true );
    }
        
    //! Insert item (if not already present) and acquire a read lock on the item.
    /** Returns true if item is new. */
    bool insert( const_accessor &result, const Key &key ) {
        result.release();
        return lookup(/*insert*/true, key, NULL, &result, /*write=*/false );
    }

    //! Insert item (if not already present) and acquire a write lock on the item.
    /** Returns true if item is new. */
    bool insert( accessor &result, const Key &key ) {
        result.release();
        return lookup(/*insert*/true, key, NULL, &result, /*write=*/true );
    }

    //! Insert item by copying if there is no such key present already and acquire a read lock on the item.
    /** Returns true if item is new. */
    bool insert( const_accessor &result, const value_type &value ) {
        result.release();
        return lookup(/*insert*/true, value.first, &value.second, &result, /*write=*/false );
    }

    //! Insert item by copying if there is no such key present already and acquire a write lock on the item.
    /** Returns true if item is new. */
    bool insert( accessor &result, const value_type &value ) {
        result.release();
        return lookup(/*insert*/true, value.first, &value.second, &result, /*write=*/true );
    }

    //! Insert item by copying if there is no such key present already
    /** Returns true if item is inserted. */
    bool insert( const value_type &value ) {
        return lookup(/*insert*/true, value.first, &value.second, NULL, /*write=*/false );
    }

    //! Insert range [first, last)
    template<typename I>
    void insert(I first, I last) {
        for(; first != last; ++first)
            insert( *first );
    }

    //! Erase item.
    /** Return true if item was erased by particularly this call. */
    bool erase( const Key& key );

    //! Erase item by const_accessor.
    /** Return true if item was erased by particularly this call. */
    bool erase( const_accessor& item_accessor ) {
        return exclude( item_accessor, /*readonly=*/ true );
    }

    //! Erase item by accessor.
    /** Return true if item was erased by particularly this call. */
    bool erase( accessor& item_accessor ) {
        return exclude( item_accessor, /*readonly=*/ false );
    }

protected:
    //! Insert or find item and optionally acquire a lock on the item.
    bool lookup( bool op_insert, const Key &key, const T *t, const_accessor *result, bool write );

    //! delete item by accessor
    bool exclude( const_accessor &item_accessor, bool readonly );

    //! Returns an iterator for an item defined by the key, or for the next item after it (if upper==true)
    template<typename I>
    std::pair<I, I> internal_equal_range( const Key& key, I end ) const;

    //! Copy "source" to *this, where *this must start out empty.
    void internal_copy( const concurrent_hash_map& source );

    template<typename I>
    void internal_copy(I first, I last);

    //! Fast find when no concurrent erasure is used. For internal use inside TBB only!
    /** Return pointer to item with given key, or NULL if no such item exists.
        Must not be called concurrently with erasure operations. */
    const_pointer internal_fast_find( const Key& key ) const {
        hashcode_t h = my_hash_compare.hash( key );
#if TBB_USE_THREADING_TOOLS
        hashcode_t m = (hashcode_t) itt_load_pointer_with_acquire_v3( &my_mask );
#else
        hashcode_t m = my_mask;
#endif
        node *n;
    restart:
        __TBB_ASSERT((m&(m+1))==0, NULL);
        bucket *b = get_bucket( h & m );
#if TBB_USE_THREADING_TOOLS
        // TODO: actually, notification is unnecessary here, just hiding double-check
        if( itt_load_pointer_with_acquire_v3(&b->node_list) == internal::rehash_req )
#else
        if( __TBB_load_with_acquire(b->node_list) == internal::rehash_req )
#endif
        {
            bucket::scoped_t lock;
            if( lock.try_acquire( b->mutex, /*write=*/true ) ) {
                if( b->node_list == internal::rehash_req)
                    const_cast<concurrent_hash_map*>(this)->rehash_bucket( b, h & m ); //recursive rehashing
            }
            else lock.acquire( b->mutex, /*write=*/false );
            __TBB_ASSERT(b->node_list!=internal::rehash_req,NULL);
        }
        n = search_bucket( key, b );
        if( n )
            return &n->item;
        else if( check_mask_race( h, m ) )
            goto restart;
        return 0;
    }
};

#if _MSC_VER && !defined(__INTEL_COMPILER)
    // Suppress "conditional expression is constant" warning.
    #pragma warning( push )
    #pragma warning( disable: 4127 )
#endif

template<typename Key, typename T, typename HashCompare, typename A>
bool concurrent_hash_map<Key,T,HashCompare,A>::lookup( bool op_insert, const Key &key, const T *t, const_accessor *result, bool write ) {
    __TBB_ASSERT( !result || !result->my_node, NULL );
    segment_index_t grow_segment;
    bool return_value;
    node *n, *tmp_n = 0;
    hashcode_t const h = my_hash_compare.hash( key );
#if TBB_USE_THREADING_TOOLS
    hashcode_t m = (hashcode_t) itt_load_pointer_with_acquire_v3( &my_mask );
#else
    hashcode_t m = my_mask;
#endif
    restart:
    {//lock scope
        __TBB_ASSERT((m&(m+1))==0, NULL);
        return_value = false;
        // get bucket
        bucket_accessor b( this, h & m );

        // find a node
        n = search_bucket( key, b() );
        if( op_insert ) {
            // [opt] insert a key
            if( !n ) {
                if( !tmp_n ) {
                    if(t) tmp_n = new( my_allocator ) node(key, *t);
                    else  tmp_n = new( my_allocator ) node(key);
                }
                if( !b.is_writer() && !b.upgrade_to_writer() ) { // TODO: improved insertion
                    // Rerun search_list, in case another thread inserted the item during the upgrade.
                    n = search_bucket( key, b() );
                    if( is_valid(n) ) { // unfortunately, it did
                        b.downgrade_to_reader();
                        goto exists;
                    }
                }
                if( check_mask_race(h, m) )
                    goto restart; // b.release() is done in ~b().
                // insert and set flag to grow the container
                grow_segment = insert_new_node( b(), n = tmp_n, m );
                tmp_n = 0;
                return_value = true;
            } else {
    exists:     grow_segment = 0;
            }
        } else { // find or count
            if( !n ) {
                if( check_mask_race( h, m ) )
                    goto restart; // b.release() is done in ~b(). TODO: replace by continue
                return false;
            }
            return_value = true;
            grow_segment = 0;
        }
        if( !result ) goto check_growth;
        // TODO: the following seems as generic/regular operation
        // acquire the item
        if( !result->my_lock.try_acquire( n->mutex, write ) ) {
            // we are unlucky, prepare for longer wait
            internal::atomic_backoff trials;
            do {
                if( !trials.bounded_pause() ) {
                    // the wait takes really long, restart the operation
                    b.release();
                    __TBB_ASSERT( !op_insert || !return_value, "Can't acquire new item in locked bucket?" );
                    __TBB_Yield();
                    m = my_mask;
                    goto restart;
                }
            } while( !result->my_lock.try_acquire( n->mutex, write ) );
        }
    }//lock scope
    result->my_node = n;
    result->my_hash = h;
check_growth:
    // [opt] grow the container
    if( grow_segment )
        enable_segment( grow_segment );
    if( tmp_n ) // if op_insert only
        delete_node( tmp_n );
    return return_value;
}

template<typename Key, typename T, typename HashCompare, typename A>
template<typename I>
std::pair<I, I> concurrent_hash_map<Key,T,HashCompare,A>::internal_equal_range( const Key& key, I end ) const {
    hashcode_t h = my_hash_compare.hash( key );
    hashcode_t m = my_mask;
    __TBB_ASSERT((m&(m+1))==0, NULL);
    h &= m;
    bucket *b = get_bucket( h );
    while( b->node_list == internal::rehash_req ) {
        m = ( 1u<<__TBB_Log2( h ) ) - 1; // get parent mask from the topmost bit
        b = get_bucket( h &= m );
    }
    node *n = search_bucket( key, b );
    if( !n )
        return std::make_pair(end, end);
    iterator lower(*this, h, b, n), upper(lower);
    return std::make_pair(lower, ++upper);
}

template<typename Key, typename T, typename HashCompare, typename A>
bool concurrent_hash_map<Key,T,HashCompare,A>::exclude( const_accessor &item_accessor, bool readonly ) {
    __TBB_ASSERT( item_accessor.my_node, NULL );
    node_base *const n = item_accessor.my_node;
    item_accessor.my_node = NULL; // we ought release accessor anyway
    hashcode_t const h = item_accessor.my_hash;
    hashcode_t m = my_mask;
    do {
        // get bucket
        bucket_accessor b( this, h & m, /*writer=*/true );
        node_base **p = &b()->node_list;
        while( *p && *p != n )
            p = &(*p)->next;
        if( !*p ) { // someone else was the first
            if( check_mask_race( h, m ) )
                continue;
            item_accessor.my_lock.release();
            return false;
        }
        __TBB_ASSERT( *p == n, NULL );
        *p = n->next; // remove from container
        my_size--;
        break;
    } while(true);
    if( readonly ) // need to get exclusive lock
        item_accessor.my_lock.upgrade_to_writer(); // return value means nothing here
    item_accessor.my_lock.release();
    delete_node( n ); // Only one thread can delete it due to write lock on the chain_mutex
    return true;
}

template<typename Key, typename T, typename HashCompare, typename A>
bool concurrent_hash_map<Key,T,HashCompare,A>::erase( const Key &key ) {
    node_base *n;
    hashcode_t const h = my_hash_compare.hash( key );
    hashcode_t m = my_mask;
restart:
    {//lock scope
        // get bucket
        bucket_accessor b( this, h & m );
    search:
        node_base **p = &b()->node_list;
        n = *p;
        while( is_valid(n) && !my_hash_compare.equal(key, static_cast<node*>(n)->item.first ) ) {
            p = &n->next;
            n = *p;
        }
        if( !n ) { // not found, but mask could be changed
            if( check_mask_race( h, m ) )
                goto restart;
            return false;
        }
        else if( !b.is_writer() && !b.upgrade_to_writer() ) {
            if( check_mask_race( h, m ) ) // contended upgrade, check mask
                goto restart;
            goto search;
        }
        *p = n->next;
        my_size--;
    }
    {
        typename node::scoped_t item_locker( n->mutex, /*write=*/true );
    }
    // note: there should be no threads pretending to acquire this mutex again, do not try to upgrade const_accessor!
    delete_node( n ); // Only one thread can delete it due to write lock on the bucket
    return true;
}

template<typename Key, typename T, typename HashCompare, typename A>
void concurrent_hash_map<Key,T,HashCompare,A>::swap(concurrent_hash_map<Key,T,HashCompare,A> &table) {
    std::swap(this->my_allocator, table.my_allocator);
    std::swap(this->my_hash_compare, table.my_hash_compare);
    internal_swap(table);
}

template<typename Key, typename T, typename HashCompare, typename A>
void concurrent_hash_map<Key,T,HashCompare,A>::clear() {
    hashcode_t m = my_mask;
    __TBB_ASSERT((m&(m+1))==0, NULL);
#if TBB_USE_DEBUG || TBB_USE_PERFORMANCE_WARNINGS
#if TBB_USE_PERFORMANCE_WARNINGS
    int size = int(my_size), buckets = int(m)+1, empty_buckets = 0, overpopulated_buckets = 0; // usage statistics
    static bool reported = false;
#endif
    // check consistency
    for( segment_index_t b = 0; b <= m; b++ ) {
        node_base *n = get_bucket(b)->node_list;
#if TBB_USE_PERFORMANCE_WARNINGS
        if( n == internal::empty_rehashed ) empty_buckets++;
        else if( n == internal::rehash_req ) buckets--;
        else if( n->next ) overpopulated_buckets++;
#endif
        for(; is_valid(n); n = n->next ) {
            hashcode_t h = my_hash_compare.hash( static_cast<node*>(n)->item.first );
            h &= m;
            __TBB_ASSERT( h == b || get_bucket(h)->node_list == internal::rehash_req, "Rehashing is not finished until serial stage due to concurrent or unexpectedly terminated operation" );
        }
    }
#if TBB_USE_PERFORMANCE_WARNINGS
    if( buckets > size) empty_buckets -= buckets - size;
    else overpopulated_buckets -= size - buckets; // TODO: load_factor?
    if( !reported && buckets >= 512 && ( 2*empty_buckets >= size || 2*overpopulated_buckets > size ) ) {
        internal::runtime_warning(
            "Performance is not optimal because the hash function produces bad randomness in lower bits in %s.\nSize: %d  Empties: %d  Overlaps: %d",
            typeid(*this).name(), size, empty_buckets, overpopulated_buckets );
        reported = true;
    }
#endif
#endif//TBB_USE_DEBUG || TBB_USE_PERFORMANCE_WARNINGS
    my_size = 0;
    segment_index_t s = segment_index_of( m );
    __TBB_ASSERT( s+1 == pointers_per_table || !my_table[s+1], "wrong mask or concurrent grow" );
    cache_aligned_allocator<bucket> alloc;
    do {
        __TBB_ASSERT( is_valid( my_table[s] ), "wrong mask or concurrent grow" );
        segment_ptr_t buckets = my_table[s];
        size_type sz = segment_size( s ? s : 1 );
        for( segment_index_t i = 0; i < sz; i++ )
            for( node_base *n = buckets[i].node_list; is_valid(n); n = buckets[i].node_list ) {
                buckets[i].node_list = n->next;
                delete_node( n );
            }
        if( s >= first_block) // the first segment or the next
            alloc.deallocate( buckets, sz );
        else if( s == embedded_block && embedded_block != first_block )
            alloc.deallocate( buckets, segment_size(first_block)-embedded_buckets );
        if( s >= embedded_block ) my_table[s] = 0;
    } while(s-- > 0);
    my_mask = embedded_buckets - 1;
}

template<typename Key, typename T, typename HashCompare, typename A>
void concurrent_hash_map<Key,T,HashCompare,A>::internal_copy( const concurrent_hash_map& source ) {
    reserve( source.my_size ); // TODO: load_factor?
    hashcode_t mask = source.my_mask;
    if( my_mask == mask ) { // optimized version
        bucket *dst = 0, *src = 0;
        for( hashcode_t k = 0; k <= mask; k++ ) {
            if( k & (k-2) ) ++dst,src++; // not the beginning of a segment
            else { dst = get_bucket( k ); src = source.get_bucket( k ); }
            __TBB_ASSERT( dst->node_list != internal::rehash_req, "Invalid bucket in destination table");
            node *n = static_cast<node*>( src->node_list );
            if( n == internal::rehash_req ) { // source is not rehashed, items are in previous buckets
                bucket_accessor b( this, k );
                rehash_bucket( b(), k ); // TODO: use without synchronization
            } else for(; n; n = static_cast<node*>( n->next ) ) {
                add_to_bucket( dst, new( my_allocator ) node(n->item.first, n->item.second) );
                ++my_size; // TODO: replace by non-atomic op
            }
        }
    } else internal_copy( source.begin(), source.end() );
}

template<typename Key, typename T, typename HashCompare, typename A>
template<typename I>
void concurrent_hash_map<Key,T,HashCompare,A>::internal_copy(I first, I last) {
    hashcode_t m = my_mask;
    for(; first != last; ++first) {
        hashcode_t h = my_hash_compare.hash( first->first );
        bucket *b = get_bucket( h & m );
        __TBB_ASSERT( b->node_list != internal::rehash_req, "Invalid bucket in destination table");
        node *n = new( my_allocator ) node(first->first, first->second);
        add_to_bucket( b, n );
        ++my_size; // TODO: replace by non-atomic op
    }
}

template<typename Key, typename T, typename HashCompare, typename A1, typename A2>
inline bool operator==(const concurrent_hash_map<Key, T, HashCompare, A1> &a, const concurrent_hash_map<Key, T, HashCompare, A2> &b) {
    if(a.size() != b.size()) return false;
    typename concurrent_hash_map<Key, T, HashCompare, A1>::const_iterator i(a.begin()), i_end(a.end());
    typename concurrent_hash_map<Key, T, HashCompare, A2>::const_iterator j, j_end(b.end());
    for(; i != i_end; ++i) {
        j = b.equal_range(i->first).first;
        if( j == j_end || !(i->second == j->second) ) return false;
    }
    return true;
}

template<typename Key, typename T, typename HashCompare, typename A1, typename A2>
inline bool operator!=(const concurrent_hash_map<Key, T, HashCompare, A1> &a, const concurrent_hash_map<Key, T, HashCompare, A2> &b)
{    return !(a == b); }

template<typename Key, typename T, typename HashCompare, typename A>
inline void swap(concurrent_hash_map<Key, T, HashCompare, A> &a, concurrent_hash_map<Key, T, HashCompare, A> &b)
{    a.swap( b ); }

#if _MSC_VER && !defined(__INTEL_COMPILER)
    #pragma warning( pop )
#endif // warning 4127 is back

} // namespace tbb

#endif /* __TBB_concurrent_hash_map_H */
