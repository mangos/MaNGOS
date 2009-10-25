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

#ifndef __TBB_concurrent_vector_H
#define __TBB_concurrent_vector_H

#include "tbb_stddef.h"
#include <algorithm>
#include <iterator>
#include <new>
#include <cstring>
#include "atomic.h"
#include "cache_aligned_allocator.h"
#include "blocked_range.h"

#include "tbb_machine.h"

#if _MSC_VER==1500 && !__INTEL_COMPILER
    // VS2008/VC9 seems to have an issue; limits pull in math.h
    #pragma warning( push )
    #pragma warning( disable: 4985 )
#endif
#include <limits> /* std::numeric_limits */
#if _MSC_VER==1500 && !__INTEL_COMPILER
    #pragma warning( pop )
#endif

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER) && defined(_Wp64)
    // Workaround for overzealous compiler warnings in /Wp64 mode
    #pragma warning (push)
    #pragma warning (disable: 4267)
#endif

namespace tbb {

template<typename T, class A = cache_aligned_allocator<T> >
class concurrent_vector;


//! @cond INTERNAL
namespace internal {

    //! Bad allocation marker
    static void *const vector_allocation_error_flag = reinterpret_cast<void*>(size_t(63));
    //! Routine that loads pointer from location pointed to by src without any fence, without causing ITT to report a race.
    void* __TBB_EXPORTED_FUNC itt_load_pointer_v3( const void* src );

    //! Base class of concurrent vector implementation.
    /** @ingroup containers */
    class concurrent_vector_base_v3 {
    protected:

        // Basic types declarations
        typedef size_t segment_index_t;
        typedef size_t size_type;

        // Using enumerations due to Mac linking problems of static const variables
        enum {
            // Size constants
            default_initial_segments = 1, // 2 initial items
            //! Number of slots for segment's pointers inside the class
            pointers_per_short_table = 3, // to fit into 8 words of entire structure
            pointers_per_long_table = sizeof(segment_index_t) * 8 // one segment per bit
        };

        // Segment pointer. Can be zero-initialized
        struct segment_t {
            void* array;
#if TBB_USE_ASSERT
            ~segment_t() {
                __TBB_ASSERT( array <= internal::vector_allocation_error_flag, "should have been freed by clear" );
            }
#endif /* TBB_USE_ASSERT */
        };
 
        // Data fields

        //! allocator function pointer
        void* (*vector_allocator_ptr)(concurrent_vector_base_v3 &, size_t);

        //! count of segments in the first block
        atomic<size_type> my_first_block;

        //! Requested size of vector
        atomic<size_type> my_early_size;

        //! Pointer to the segments table
        atomic<segment_t*> my_segment;

        //! embedded storage of segment pointers
        segment_t my_storage[pointers_per_short_table];

        // Methods

        concurrent_vector_base_v3() {
            my_early_size = 0;
            my_first_block = 0; // here is not default_initial_segments
            for( segment_index_t i = 0; i < pointers_per_short_table; i++)
                my_storage[i].array = NULL;
            my_segment = my_storage;
        }
        __TBB_EXPORTED_METHOD ~concurrent_vector_base_v3();

        static segment_index_t segment_index_of( size_type index ) {
            return segment_index_t( __TBB_Log2( index|1 ) );
        }

        static segment_index_t segment_base( segment_index_t k ) {
            return (segment_index_t(1)<<k & ~segment_index_t(1));
        }

        static inline segment_index_t segment_base_index_of( segment_index_t &index ) {
            segment_index_t k = segment_index_of( index );
            index -= segment_base(k);
            return k;
        }

        static size_type segment_size( segment_index_t k ) {
            return segment_index_t(1)<<k; // fake value for k==0
        }

        //! An operation on an n-element array starting at begin.
        typedef void (__TBB_EXPORTED_FUNC *internal_array_op1)(void* begin, size_type n );

        //! An operation on n-element destination array and n-element source array.
        typedef void (__TBB_EXPORTED_FUNC *internal_array_op2)(void* dst, const void* src, size_type n );

        //! Internal structure for compact()
        struct internal_segments_table {
            segment_index_t first_block;
            void* table[pointers_per_long_table];
        };

        void __TBB_EXPORTED_METHOD internal_reserve( size_type n, size_type element_size, size_type max_size );
        size_type __TBB_EXPORTED_METHOD internal_capacity() const;
        void internal_grow( size_type start, size_type finish, size_type element_size, internal_array_op2 init, const void *src );
        size_type __TBB_EXPORTED_METHOD internal_grow_by( size_type delta, size_type element_size, internal_array_op2 init, const void *src );
        void* __TBB_EXPORTED_METHOD internal_push_back( size_type element_size, size_type& index );
        segment_index_t __TBB_EXPORTED_METHOD internal_clear( internal_array_op1 destroy );
        void* __TBB_EXPORTED_METHOD internal_compact( size_type element_size, void *table, internal_array_op1 destroy, internal_array_op2 copy );
        void __TBB_EXPORTED_METHOD internal_copy( const concurrent_vector_base_v3& src, size_type element_size, internal_array_op2 copy );
        void __TBB_EXPORTED_METHOD internal_assign( const concurrent_vector_base_v3& src, size_type element_size,
                              internal_array_op1 destroy, internal_array_op2 assign, internal_array_op2 copy );
        void __TBB_EXPORTED_METHOD internal_throw_exception(size_type) const;
        void __TBB_EXPORTED_METHOD internal_swap(concurrent_vector_base_v3& v);

        void __TBB_EXPORTED_METHOD internal_resize( size_type n, size_type element_size, size_type max_size, const void *src,
                                                    internal_array_op1 destroy, internal_array_op2 init );
        size_type __TBB_EXPORTED_METHOD internal_grow_to_at_least_with_result( size_type new_size, size_type element_size, internal_array_op2 init, const void *src );

        //! Deprecated entry point for backwards compatibility to TBB 2.1.
        void __TBB_EXPORTED_METHOD internal_grow_to_at_least( size_type new_size, size_type element_size, internal_array_op2 init, const void *src );
private:
        //! Private functionality
        class helper;
        friend class helper;
    };
    
    typedef concurrent_vector_base_v3 concurrent_vector_base;

    //! Meets requirements of a forward iterator for STL and a Value for a blocked_range.*/
    /** Value is either the T or const T type of the container.
        @ingroup containers */
    template<typename Container, typename Value>
    class vector_iterator 
    {
        //! concurrent_vector over which we are iterating.
        Container* my_vector;

        //! Index into the vector 
        size_t my_index;

        //! Caches my_vector-&gt;internal_subscript(my_index)
        /** NULL if cached value is not available */
        mutable Value* my_item;

        template<typename C, typename T>
        friend vector_iterator<C,T> operator+( ptrdiff_t offset, const vector_iterator<C,T>& v );

        template<typename C, typename T, typename U>
        friend bool operator==( const vector_iterator<C,T>& i, const vector_iterator<C,U>& j );

        template<typename C, typename T, typename U>
        friend bool operator<( const vector_iterator<C,T>& i, const vector_iterator<C,U>& j );

        template<typename C, typename T, typename U>
        friend ptrdiff_t operator-( const vector_iterator<C,T>& i, const vector_iterator<C,U>& j );
    
        template<typename C, typename U>
        friend class internal::vector_iterator;

#if !defined(_MSC_VER) || defined(__INTEL_COMPILER)
        template<typename T, class A>
        friend class tbb::concurrent_vector;
#else
public: // workaround for MSVC
#endif 

        vector_iterator( const Container& vector, size_t index, void *ptr = 0 ) : 
            my_vector(const_cast<Container*>(&vector)), 
            my_index(index), 
            my_item(static_cast<Value*>(ptr))
        {}

    public:
        //! Default constructor
        vector_iterator() : my_vector(NULL), my_index(~size_t(0)), my_item(NULL) {}

        vector_iterator( const vector_iterator<Container,typename Container::value_type>& other ) :
            my_vector(other.my_vector),
            my_index(other.my_index),
            my_item(other.my_item)
        {}

        vector_iterator operator+( ptrdiff_t offset ) const {
            return vector_iterator( *my_vector, my_index+offset );
        }
        vector_iterator &operator+=( ptrdiff_t offset ) {
            my_index+=offset;
            my_item = NULL;
            return *this;
        }
        vector_iterator operator-( ptrdiff_t offset ) const {
            return vector_iterator( *my_vector, my_index-offset );
        }
        vector_iterator &operator-=( ptrdiff_t offset ) {
            my_index-=offset;
            my_item = NULL;
            return *this;
        }
        Value& operator*() const {
            Value* item = my_item;
            if( !item ) {
                item = my_item = &my_vector->internal_subscript(my_index);
            }
            __TBB_ASSERT( item==&my_vector->internal_subscript(my_index), "corrupt cache" );
            return *item;
        }
        Value& operator[]( ptrdiff_t k ) const {
            return my_vector->internal_subscript(my_index+k);
        }
        Value* operator->() const {return &operator*();}

        //! Pre increment
        vector_iterator& operator++() {
            size_t k = ++my_index;
            if( my_item ) {
                // Following test uses 2's-complement wizardry
                if( (k& (k-2))==0 ) {
                    // k is a power of two that is at least k-2
                    my_item= NULL;
                } else {
                    ++my_item;
                }
            }
            return *this;
        }

        //! Pre decrement
        vector_iterator& operator--() {
            __TBB_ASSERT( my_index>0, "operator--() applied to iterator already at beginning of concurrent_vector" ); 
            size_t k = my_index--;
            if( my_item ) {
                // Following test uses 2's-complement wizardry
                if( (k& (k-2))==0 ) {
                    // k is a power of two that is at least k-2  
                    my_item= NULL;
                } else {
                    --my_item;
                }
            }
            return *this;
        }

        //! Post increment
        vector_iterator operator++(int) {
            vector_iterator result = *this;
            operator++();
            return result;
        }

        //! Post decrement
        vector_iterator operator--(int) {
            vector_iterator result = *this;
            operator--();
            return result;
        }

        // STL support

        typedef ptrdiff_t difference_type;
        typedef Value value_type;
        typedef Value* pointer;
        typedef Value& reference;
        typedef std::random_access_iterator_tag iterator_category;
    };

    template<typename Container, typename T>
    vector_iterator<Container,T> operator+( ptrdiff_t offset, const vector_iterator<Container,T>& v ) {
        return vector_iterator<Container,T>( *v.my_vector, v.my_index+offset );
    }

    template<typename Container, typename T, typename U>
    bool operator==( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
        return i.my_index==j.my_index && i.my_vector == j.my_vector;
    }

    template<typename Container, typename T, typename U>
    bool operator!=( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
        return !(i==j);
    }

    template<typename Container, typename T, typename U>
    bool operator<( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
        return i.my_index<j.my_index;
    }

    template<typename Container, typename T, typename U>
    bool operator>( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
        return j<i;
    }

    template<typename Container, typename T, typename U>
    bool operator>=( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
        return !(i<j);
    }

    template<typename Container, typename T, typename U>
    bool operator<=( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
        return !(j<i);
    }

    template<typename Container, typename T, typename U>
    ptrdiff_t operator-( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
        return ptrdiff_t(i.my_index)-ptrdiff_t(j.my_index);
    }

    template<typename T, class A>
    class allocator_base {
    public:
        typedef typename A::template
            rebind<T>::other allocator_type;
        allocator_type my_allocator;

        allocator_base(const allocator_type &a = allocator_type() ) : my_allocator(a) {}
    };

} // namespace internal
//! @endcond

//! Concurrent vector container
/** concurrent_vector is a container having the following main properties:
    - It provides random indexed access to its elements. The index of the first element is 0.
    - It ensures safe concurrent growing its size (different threads can safely append new elements).
    - Adding new elements does not invalidate existing iterators and does not change indices of existing items.

@par Compatibility
    The class meets all Container Requirements and Reversible Container Requirements from
    C++ Standard (See ISO/IEC 14882:2003(E), clause 23.1). But it doesn't meet
    Sequence Requirements due to absence of insert() and erase() methods.

@par Exception Safety
    Methods working with memory allocation and/or new elements construction can throw an
    exception if allocator fails to allocate memory or element's default constructor throws one.
    Concurrent vector's element of type T must conform to the following requirements:
    - Throwing an exception is forbidden for destructor of T.
    - Default constructor of T must not throw an exception OR its non-virtual destructor must safely work when its object memory is zero-initialized.
    .
    Otherwise, the program's behavior is undefined.
@par
    If an exception happens inside growth or assignment operation, an instance of the vector becomes invalid unless it is stated otherwise in the method documentation.
    Invalid state means:
    - There are no guaranties that all items were initialized by a constructor. The rest of items is zero-filled, including item where exception happens.
    - An invalid vector instance cannot be repaired; it is unable to grow anymore.
    - Size and capacity reported by the vector are incorrect, and calculated as if the failed operation were successful.
    - Attempt to access not allocated elements using operator[] or iterators results in access violation or segmentation fault exception, and in case of using at() method a C++ exception is thrown.
    .
    If a concurrent grow operation successfully completes, all the elements it has added to the vector will remain valid and accessible even if one of subsequent grow operations fails.

@par Fragmentation
    Unlike an STL vector, a concurrent_vector does not move existing elements if it needs
    to allocate more memory. The container is divided into a series of contiguous arrays of
    elements. The first reservation, growth, or assignment operation determines the size of
    the first array. Using small number of elements as initial size incurs fragmentation that
    may increase element access time. Internal layout can be optimized by method compact() that
    merges several smaller arrays into one solid.

@par Changes since TBB 2.1
    - Fixed guarantees of concurrent_vector::size() and grow_to_at_least() methods to assure elements are allocated.
    - Methods end()/rbegin()/back() are partly thread-safe since they use size() to get the end of vector
    - Added resize() methods (not thread-safe)
    - Added cbegin/cend/crbegin/crend methods
    - Changed return type of methods grow* and push_back to iterator

@par Changes since TBB 2.0
    - Implemented exception-safety guaranties
    - Added template argument for allocator
    - Added allocator argument in constructors
    - Faster index calculation
    - First growth call specifies a number of segments to be merged in the first allocation.
    - Fixed memory blow up for swarm of vector's instances of small size
    - Added grow_by(size_type n, const_reference t) growth using copying constructor to init new items. 
    - Added STL-like constructors.
    - Added operators ==, < and derivatives
    - Added at() method, approved for using after an exception was thrown inside the vector
    - Added get_allocator() method.
    - Added assign() methods
    - Added compact() method to defragment first segments
    - Added swap() method
    - range() defaults on grainsize = 1 supporting auto grainsize algorithms. 

    @ingroup containers */
template<typename T, class A>
class concurrent_vector: protected internal::allocator_base<T, A>,
                         private internal::concurrent_vector_base {
private:
    template<typename I>
    class generic_range_type: public blocked_range<I> {
    public:
        typedef T value_type;
        typedef T& reference;
        typedef const T& const_reference;
        typedef I iterator;
        typedef ptrdiff_t difference_type;
        generic_range_type( I begin_, I end_, size_t grainsize = 1) : blocked_range<I>(begin_,end_,grainsize) {} 
        template<typename U>
        generic_range_type( const generic_range_type<U>& r) : blocked_range<I>(r.begin(),r.end(),r.grainsize()) {} 
        generic_range_type( generic_range_type& r, split ) : blocked_range<I>(r,split()) {}
    };

    template<typename C, typename U>
    friend class internal::vector_iterator;
public:
    //------------------------------------------------------------------------
    // STL compatible types
    //------------------------------------------------------------------------
    typedef internal::concurrent_vector_base_v3::size_type size_type;
    typedef typename internal::allocator_base<T, A>::allocator_type allocator_type;

    typedef T value_type;
    typedef ptrdiff_t difference_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T *pointer;
    typedef const T *const_pointer;

    typedef internal::vector_iterator<concurrent_vector,T> iterator;
    typedef internal::vector_iterator<concurrent_vector,const T> const_iterator;

#if !defined(_MSC_VER) || _CPPLIB_VER>=300 
    // Assume ISO standard definition of std::reverse_iterator
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
#else
    // Use non-standard std::reverse_iterator
    typedef std::reverse_iterator<iterator,T,T&,T*> reverse_iterator;
    typedef std::reverse_iterator<const_iterator,T,const T&,const T*> const_reverse_iterator;
#endif /* defined(_MSC_VER) && (_MSC_VER<1300) */

    //------------------------------------------------------------------------
    // Parallel algorithm support
    //------------------------------------------------------------------------
    typedef generic_range_type<iterator> range_type;
    typedef generic_range_type<const_iterator> const_range_type;

    //------------------------------------------------------------------------
    // STL compatible constructors & destructors
    //------------------------------------------------------------------------

    //! Construct empty vector.
    explicit concurrent_vector(const allocator_type &a = allocator_type())
        : internal::allocator_base<T, A>(a)
    {
        vector_allocator_ptr = &internal_allocator;
    }

    //! Copying constructor
    concurrent_vector( const concurrent_vector& vector, const allocator_type& a = allocator_type() )
        : internal::allocator_base<T, A>(a)
    {
        vector_allocator_ptr = &internal_allocator;
        try {
            internal_copy(vector, sizeof(T), &copy_array);
        } catch(...) {
            segment_t *table = my_segment;
            internal_free_segments( reinterpret_cast<void**>(table), internal_clear(&destroy_array), my_first_block );
            throw;
        }
    }

    //! Copying constructor for vector with different allocator type
    template<class M>
    concurrent_vector( const concurrent_vector<T, M>& vector, const allocator_type& a = allocator_type() )
        : internal::allocator_base<T, A>(a)
    {
        vector_allocator_ptr = &internal_allocator;
        try {
            internal_copy(vector.internal_vector_base(), sizeof(T), &copy_array);
        } catch(...) {
            segment_t *table = my_segment;
            internal_free_segments( reinterpret_cast<void**>(table), internal_clear(&destroy_array), my_first_block );
            throw;
        }
    }

    //! Construction with initial size specified by argument n
    explicit concurrent_vector(size_type n)
    {
        vector_allocator_ptr = &internal_allocator;
        try {
            internal_resize( n, sizeof(T), max_size(), NULL, &destroy_array, &initialize_array );
        } catch(...) {
            segment_t *table = my_segment;
            internal_free_segments( reinterpret_cast<void**>(table), internal_clear(&destroy_array), my_first_block );
            throw;
        }
    }

    //! Construction with initial size specified by argument n, initialization by copying of t, and given allocator instance
    concurrent_vector(size_type n, const_reference t, const allocator_type& a = allocator_type())
        : internal::allocator_base<T, A>(a)
    {
        vector_allocator_ptr = &internal_allocator;
        try {
            internal_resize( n, sizeof(T), max_size(), static_cast<const void*>(&t), &destroy_array, &initialize_array_by );
        } catch(...) {
            segment_t *table = my_segment;
            internal_free_segments( reinterpret_cast<void**>(table), internal_clear(&destroy_array), my_first_block );
            throw;
        }
    }

    //! Construction with copying iteration range and given allocator instance
    template<class I>
    concurrent_vector(I first, I last, const allocator_type &a = allocator_type())
        : internal::allocator_base<T, A>(a)
    {
        vector_allocator_ptr = &internal_allocator;
        try {
            internal_assign_range(first, last, static_cast<is_integer_tag<std::numeric_limits<I>::is_integer> *>(0) );
        } catch(...) {
            segment_t *table = my_segment;
            internal_free_segments( reinterpret_cast<void**>(table), internal_clear(&destroy_array), my_first_block );
            throw;
        }
    }

    //! Assignment
    concurrent_vector& operator=( const concurrent_vector& vector ) {
        if( this != &vector )
            internal_assign(vector, sizeof(T), &destroy_array, &assign_array, &copy_array);
        return *this;
    }

    //! Assignment for vector with different allocator type
    template<class M>
    concurrent_vector& operator=( const concurrent_vector<T, M>& vector ) {
        if( static_cast<void*>( this ) != static_cast<const void*>( &vector ) )
            internal_assign(vector.internal_vector_base(),
                sizeof(T), &destroy_array, &assign_array, &copy_array);
        return *this;
    }

    //------------------------------------------------------------------------
    // Concurrent operations
    //------------------------------------------------------------------------
    //! Grow by "delta" elements.
#if TBB_DEPRECATED
    /** Returns old size. */
    size_type grow_by( size_type delta ) {
        return delta ? internal_grow_by( delta, sizeof(T), &initialize_array, NULL ) : my_early_size;
    }
#else
    /** Returns iterator pointing to the first new element. */
    iterator grow_by( size_type delta ) {
        return iterator(*this, delta ? internal_grow_by( delta, sizeof(T), &initialize_array, NULL ) : my_early_size);
    }
#endif

    //! Grow by "delta" elements using copying constuctor.
#if TBB_DEPRECATED
    /** Returns old size. */
    size_type grow_by( size_type delta, const_reference t ) {
        return delta ? internal_grow_by( delta, sizeof(T), &initialize_array_by, static_cast<const void*>(&t) ) : my_early_size;
    }
#else
    /** Returns iterator pointing to the first new element. */
    iterator grow_by( size_type delta, const_reference t ) {
        return iterator(*this, delta ? internal_grow_by( delta, sizeof(T), &initialize_array_by, static_cast<const void*>(&t) ) : my_early_size);
    }
#endif

    //! Append minimal sequence of elements such that size()>=n.  
#if TBB_DEPRECATED
    /** The new elements are default constructed.  Blocks until all elements in range [0..n) are allocated.
        May return while other elements are being constructed by other threads. */
    void grow_to_at_least( size_type n ) {
        if( n ) internal_grow_to_at_least_with_result( n, sizeof(T), &initialize_array, NULL );
    };
#else
    /** The new elements are default constructed.  Blocks until all elements in range [0..n) are allocated.
        May return while other elements are being constructed by other threads.
        Returns iterator that points to beginning of appended sequence.
        If no elements were appended, returns iterator pointing to nth element. */
    iterator grow_to_at_least( size_type n ) {
        size_type m=0;
        if( n ) {
            m = internal_grow_to_at_least_with_result( n, sizeof(T), &initialize_array, NULL );
            if( m>n ) m=n;
        }
        return iterator(*this, m);
    };
#endif

    //! Push item 
#if TBB_DEPRECATED
    size_type push_back( const_reference item )
#else
    /** Returns iterator pointing to the new element. */
    iterator push_back( const_reference item )
#endif
    {
        size_type k;
        void *ptr = internal_push_back(sizeof(T),k);
        internal_loop_guide loop(1, ptr);
        loop.init(&item);
#if TBB_DEPRECATED
        return k;
#else
        return iterator(*this, k, ptr);
#endif
    }

    //! Get reference to element at given index.
    /** This method is thread-safe for concurrent reads, and also while growing the vector,
        as long as the calling thread has checked that index&lt;size(). */
    reference operator[]( size_type index ) {
        return internal_subscript(index);
    }

    //! Get const reference to element at given index.
    const_reference operator[]( size_type index ) const {
        return internal_subscript(index);
    }

    //! Get reference to element at given index. Throws exceptions on errors.
    reference at( size_type index ) {
        return internal_subscript_with_exceptions(index);
    }

    //! Get const reference to element at given index. Throws exceptions on errors.
    const_reference at( size_type index ) const {
        return internal_subscript_with_exceptions(index);
    }

    //! Get range for iterating with parallel algorithms
    range_type range( size_t grainsize = 1) {
        return range_type( begin(), end(), grainsize );
    }

    //! Get const range for iterating with parallel algorithms
    const_range_type range( size_t grainsize = 1 ) const {
        return const_range_type( begin(), end(), grainsize );
    }
    //------------------------------------------------------------------------
    // Capacity
    //------------------------------------------------------------------------
    //! Return size of vector. It may include elements under construction
    size_type size() const {
        size_type sz = my_early_size, cp = internal_capacity();
        return cp < sz ? cp : sz;
    }

    //! Return true if vector is not empty or has elements under construction at least.
    bool empty() const {return !my_early_size;}

    //! Maximum size to which array can grow without allocating more memory. Concurrent allocations are not included in the value.
    size_type capacity() const {return internal_capacity();}

    //! Allocate enough space to grow to size n without having to allocate more memory later.
    /** Like most of the methods provided for STL compatibility, this method is *not* thread safe. 
        The capacity afterwards may be bigger than the requested reservation. */
    void reserve( size_type n ) {
        if( n )
            internal_reserve(n, sizeof(T), max_size());
    }

    //! Resize the vector. Not thread-safe.
    void resize( size_type n ) {
        internal_resize( n, sizeof(T), max_size(), NULL, &destroy_array, &initialize_array );
    }
    
    //! Resize the vector, copy t for new elements. Not thread-safe.
    void resize( size_type n, const_reference t ) {
        internal_resize( n, sizeof(T), max_size(), static_cast<const void*>(&t), &destroy_array, &initialize_array_by );
    }
   
#if TBB_DEPRECATED 
    //! An alias for shrink_to_fit()
    void compact() {shrink_to_fit();}
#endif /* TBB_DEPRECATED */

    //! Optimize memory usage and fragmentation.
    void shrink_to_fit();

    //! Upper bound on argument to reserve.
    size_type max_size() const {return (~size_type(0))/sizeof(T);}

    //------------------------------------------------------------------------
    // STL support
    //------------------------------------------------------------------------

    //! start iterator
    iterator begin() {return iterator(*this,0);}
    //! end iterator
    iterator end() {return iterator(*this,size());}
    //! start const iterator
    const_iterator begin() const {return const_iterator(*this,0);}
    //! end const iterator
    const_iterator end() const {return const_iterator(*this,size());}
    //! start const iterator
    const_iterator cbegin() const {return const_iterator(*this,0);}
    //! end const iterator
    const_iterator cend() const {return const_iterator(*this,size());}
    //! reverse start iterator
    reverse_iterator rbegin() {return reverse_iterator(end());}
    //! reverse end iterator
    reverse_iterator rend() {return reverse_iterator(begin());}
    //! reverse start const iterator
    const_reverse_iterator rbegin() const {return const_reverse_iterator(end());}
    //! reverse end const iterator
    const_reverse_iterator rend() const {return const_reverse_iterator(begin());}
    //! reverse start const iterator
    const_reverse_iterator crbegin() const {return const_reverse_iterator(end());}
    //! reverse end const iterator
    const_reverse_iterator crend() const {return const_reverse_iterator(begin());}
    //! the first item
    reference front() {
        __TBB_ASSERT( size()>0, NULL);
        return static_cast<T*>(my_segment[0].array)[0];
    }
    //! the first item const
    const_reference front() const {
        __TBB_ASSERT( size()>0, NULL);
        return static_cast<const T*>(my_segment[0].array)[0];
    }
    //! the last item
    reference back() {
        __TBB_ASSERT( size()>0, NULL);
        return internal_subscript( size()-1 );
    }
    //! the last item const
    const_reference back() const {
        __TBB_ASSERT( size()>0, NULL);
        return internal_subscript( size()-1 );
    }
    //! return allocator object
    allocator_type get_allocator() const { return this->my_allocator; }

    //! assign n items by copying t item
    void assign(size_type n, const_reference t) {
        clear();
        internal_resize( n, sizeof(T), max_size(), static_cast<const void*>(&t), &destroy_array, &initialize_array_by );
    }

    //! assign range [first, last)
    template<class I>
    void assign(I first, I last) {
        clear(); internal_assign_range( first, last, static_cast<is_integer_tag<std::numeric_limits<I>::is_integer> *>(0) );
    }

    //! swap two instances
    void swap(concurrent_vector &vector) {
        if( this != &vector ) {
            concurrent_vector_base_v3::internal_swap(static_cast<concurrent_vector_base_v3&>(vector));
            std::swap(this->my_allocator, vector.my_allocator);
        }
    }

    //! Clear container while keeping memory allocated.
    /** To free up the memory, use in conjunction with method compact(). Not thread safe **/
    void clear() {
        internal_clear(&destroy_array);
    }

    //! Clear and destroy vector.
    ~concurrent_vector() {
        segment_t *table = my_segment;
        internal_free_segments( reinterpret_cast<void**>(table), internal_clear(&destroy_array), my_first_block );
        // base class destructor call should be then
    }

    const internal::concurrent_vector_base_v3 &internal_vector_base() const { return *this; }
private:
    //! Allocate k items
    static void *internal_allocator(internal::concurrent_vector_base_v3 &vb, size_t k) {
        return static_cast<concurrent_vector<T, A>&>(vb).my_allocator.allocate(k);
    }
    //! Free k segments from table
    void internal_free_segments(void *table[], segment_index_t k, segment_index_t first_block);

    //! Get reference to element at given index.
    T& internal_subscript( size_type index ) const;

    //! Get reference to element at given index with errors checks
    T& internal_subscript_with_exceptions( size_type index ) const;

    //! assign n items by copying t
    void internal_assign_n(size_type n, const_pointer p) {
        internal_resize( n, sizeof(T), max_size(), static_cast<const void*>(p), &destroy_array, p? &initialize_array_by : &initialize_array );
    }

    //! helper class
    template<bool B> class is_integer_tag;

    //! assign integer items by copying when arguments are treated as iterators. See C++ Standard 2003 23.1.1p9
    template<class I>
    void internal_assign_range(I first, I last, is_integer_tag<true> *) {
        internal_assign_n(static_cast<size_type>(first), &static_cast<T&>(last));
    }
    //! inline proxy assign by iterators
    template<class I>
    void internal_assign_range(I first, I last, is_integer_tag<false> *) {
        internal_assign_iterators(first, last);
    }
    //! assign by iterators
    template<class I>
    void internal_assign_iterators(I first, I last);

    //! Construct n instances of T, starting at "begin".
    static void __TBB_EXPORTED_FUNC initialize_array( void* begin, const void*, size_type n );

    //! Construct n instances of T, starting at "begin".
    static void __TBB_EXPORTED_FUNC initialize_array_by( void* begin, const void* src, size_type n );

    //! Construct n instances of T, starting at "begin".
    static void __TBB_EXPORTED_FUNC copy_array( void* dst, const void* src, size_type n );

    //! Assign n instances of T, starting at "begin".
    static void __TBB_EXPORTED_FUNC assign_array( void* dst, const void* src, size_type n );

    //! Destroy n instances of T, starting at "begin".
    static void __TBB_EXPORTED_FUNC destroy_array( void* begin, size_type n );

    //! Exception-aware helper class for filling a segment by exception-danger operators of user class
    class internal_loop_guide : internal::no_copy {
    public:
        const pointer array;
        const size_type n;
        size_type i;
        internal_loop_guide(size_type ntrials, void *ptr)
            : array(static_cast<pointer>(ptr)), n(ntrials), i(0) {}
        void init() {   for(; i < n; ++i) new( &array[i] ) T(); }
        void init(const void *src) { for(; i < n; ++i) new( &array[i] ) T(*static_cast<const T*>(src)); }
        void copy(const void *src) { for(; i < n; ++i) new( &array[i] ) T(static_cast<const T*>(src)[i]); }
        void assign(const void *src) { for(; i < n; ++i) array[i] = static_cast<const T*>(src)[i]; }
        template<class I> void iterate(I &src) { for(; i < n; ++i, ++src) new( &array[i] ) T( *src ); }
        ~internal_loop_guide() {
            if(i < n) // if exception raised, do zerroing on the rest of items
                std::memset(array+i, 0, (n-i)*sizeof(value_type));
        }
    };
};

template<typename T, class A>
void concurrent_vector<T, A>::shrink_to_fit() {
    internal_segments_table old;
    try {
        if( internal_compact( sizeof(T), &old, &destroy_array, &copy_array ) )
            internal_free_segments( old.table, pointers_per_long_table, old.first_block ); // free joined and unnecessary segments
    } catch(...) {
        if( old.first_block ) // free segment allocated for compacting. Only for support of exceptions in ctor of user T[ype]
            internal_free_segments( old.table, 1, old.first_block );
        throw;
    }
}

template<typename T, class A>
void concurrent_vector<T, A>::internal_free_segments(void *table[], segment_index_t k, segment_index_t first_block) {
    // Free the arrays
    while( k > first_block ) {
        --k;
        T* array = static_cast<T*>(table[k]);
        table[k] = NULL;
        if( array > internal::vector_allocation_error_flag ) // check for correct segment pointer
            this->my_allocator.deallocate( array, segment_size(k) );
    }
    T* array = static_cast<T*>(table[0]);
    if( array > internal::vector_allocation_error_flag ) {
        __TBB_ASSERT( first_block > 0, NULL );
        while(k > 0) table[--k] = NULL;
        this->my_allocator.deallocate( array, segment_size(first_block) );
    }
}

template<typename T, class A>
T& concurrent_vector<T, A>::internal_subscript( size_type index ) const {
    __TBB_ASSERT( index < my_early_size, "index out of bounds" );
    size_type j = index;
    segment_index_t k = segment_base_index_of( j );
    __TBB_ASSERT( my_segment != (segment_t*)my_storage || k < pointers_per_short_table, "index is being allocated" );
    // no need in __TBB_load_with_acquire since thread works in own space or gets 
#if TBB_USE_THREADING_TOOLS
    T* array = static_cast<T*>( tbb::internal::itt_load_pointer_v3(&my_segment[k].array));
#else
    T* array = static_cast<T*>(my_segment[k].array);
#endif /* TBB_USE_THREADING_TOOLS */
    __TBB_ASSERT( array != internal::vector_allocation_error_flag, "the instance is broken by bad allocation. Use at() instead" );
    __TBB_ASSERT( array, "index is being allocated" );
    return array[j];
}

template<typename T, class A>
T& concurrent_vector<T, A>::internal_subscript_with_exceptions( size_type index ) const {
    if( index >= my_early_size )
        internal_throw_exception(0); // throw std::out_of_range
    size_type j = index;
    segment_index_t k = segment_base_index_of( j );
    if( my_segment == (segment_t*)my_storage && k >= pointers_per_short_table )
        internal_throw_exception(1); // throw std::range_error
    void *array = my_segment[k].array; // no need in __TBB_load_with_acquire
    if( array <= internal::vector_allocation_error_flag ) // check for correct segment pointer
        internal_throw_exception(2); // throw std::range_error
    return static_cast<T*>(array)[j];
}

template<typename T, class A> template<class I>
void concurrent_vector<T, A>::internal_assign_iterators(I first, I last) {
    __TBB_ASSERT(my_early_size == 0, NULL);
    size_type n = std::distance(first, last);
    if( !n ) return;
    internal_reserve(n, sizeof(T), max_size());
    my_early_size = n;
    segment_index_t k = 0;
    size_type sz = segment_size( my_first_block );
    while( sz < n ) {
        internal_loop_guide loop(sz, my_segment[k].array);
        loop.iterate(first);
        n -= sz;
        if( !k ) k = my_first_block;
        else { ++k; sz <<= 1; }
    }
    internal_loop_guide loop(n, my_segment[k].array);
    loop.iterate(first);
}

template<typename T, class A>
void concurrent_vector<T, A>::initialize_array( void* begin, const void *, size_type n ) {
    internal_loop_guide loop(n, begin); loop.init();
}

template<typename T, class A>
void concurrent_vector<T, A>::initialize_array_by( void* begin, const void *src, size_type n ) {
    internal_loop_guide loop(n, begin); loop.init(src);
}

template<typename T, class A>
void concurrent_vector<T, A>::copy_array( void* dst, const void* src, size_type n ) {
    internal_loop_guide loop(n, dst); loop.copy(src);
}

template<typename T, class A>
void concurrent_vector<T, A>::assign_array( void* dst, const void* src, size_type n ) {
    internal_loop_guide loop(n, dst); loop.assign(src);
}

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER) 
    // Workaround for overzealous compiler warning
    #pragma warning (push)
    #pragma warning (disable: 4189)
#endif
template<typename T, class A>
void concurrent_vector<T, A>::destroy_array( void* begin, size_type n ) {
    T* array = static_cast<T*>(begin);
    for( size_type j=n; j>0; --j )
        array[j-1].~T(); // destructors are supposed to not throw any exceptions
}
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER) 
    #pragma warning (pop)
#endif // warning 4189 is back 

// concurrent_vector's template functions
template<typename T, class A1, class A2>
inline bool operator==(const concurrent_vector<T, A1> &a, const concurrent_vector<T, A2> &b) {
    // Simply:    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
    if(a.size() != b.size()) return false;
    typename concurrent_vector<T, A1>::const_iterator i(a.begin());
    typename concurrent_vector<T, A2>::const_iterator j(b.begin());
    for(; i != a.end(); ++i, ++j)
        if( !(*i == *j) ) return false;
    return true;
}

template<typename T, class A1, class A2>
inline bool operator!=(const concurrent_vector<T, A1> &a, const concurrent_vector<T, A2> &b)
{    return !(a == b); }

template<typename T, class A1, class A2>
inline bool operator<(const concurrent_vector<T, A1> &a, const concurrent_vector<T, A2> &b)
{    return (std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end())); }

template<typename T, class A1, class A2>
inline bool operator>(const concurrent_vector<T, A1> &a, const concurrent_vector<T, A2> &b)
{    return b < a; }

template<typename T, class A1, class A2>
inline bool operator<=(const concurrent_vector<T, A1> &a, const concurrent_vector<T, A2> &b)
{    return !(b < a); }

template<typename T, class A1, class A2>
inline bool operator>=(const concurrent_vector<T, A1> &a, const concurrent_vector<T, A2> &b)
{    return !(a < b); }

template<typename T, class A>
inline void swap(concurrent_vector<T, A> &a, concurrent_vector<T, A> &b)
{    a.swap( b ); }

} // namespace tbb

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER) && defined(_Wp64)
    #pragma warning (pop)
#endif // warning 4267 is back

#endif /* __TBB_concurrent_vector_H */
