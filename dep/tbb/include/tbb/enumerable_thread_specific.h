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

#ifndef __TBB_enumerable_thread_specific_H
#define __TBB_enumerable_thread_specific_H

#include "concurrent_vector.h"
#include "tbb_thread.h"
#include "concurrent_hash_map.h"
#include "cache_aligned_allocator.h"
#if __SUNPRO_CC
#include <string.h>  // for memcpy
#endif

#if _WIN32||_WIN64
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace tbb {

    //! enum for selecting between single key and key-per-instance versions
    enum ets_key_usage_type { ets_key_per_instance, ets_no_key };

    //! @cond
    namespace internal {
        
        //! Random access iterator for traversing the thread local copies.
        template< typename Container, typename Value >
        class enumerable_thread_specific_iterator 
#if defined(_WIN64) && defined(_MSC_VER) 
            // Ensure that Microsoft's internal template function _Val_type works correctly.
            : public std::iterator<std::random_access_iterator_tag,Value>
#endif /* defined(_WIN64) && defined(_MSC_VER) */
        {
            //! current position in the concurrent_vector 
        
            Container *my_container;
            typename Container::size_type my_index;
            mutable Value *my_value;
        
            template<typename C, typename T>
            friend enumerable_thread_specific_iterator<C,T> operator+( ptrdiff_t offset, 
                                                                       const enumerable_thread_specific_iterator<C,T>& v );
        
            template<typename C, typename T, typename U>
            friend bool operator==( const enumerable_thread_specific_iterator<C,T>& i, 
                                    const enumerable_thread_specific_iterator<C,U>& j );
        
            template<typename C, typename T, typename U>
            friend bool operator<( const enumerable_thread_specific_iterator<C,T>& i, 
                                   const enumerable_thread_specific_iterator<C,U>& j );
        
            template<typename C, typename T, typename U>
            friend ptrdiff_t operator-( const enumerable_thread_specific_iterator<C,T>& i, const enumerable_thread_specific_iterator<C,U>& j );
            
            template<typename C, typename U> 
            friend class enumerable_thread_specific_iterator;
        
            public:
        
            enumerable_thread_specific_iterator( const Container &container, typename Container::size_type index ) : 
                my_container(&const_cast<Container &>(container)), my_index(index), my_value(NULL) {}
        
            //! Default constructor
            enumerable_thread_specific_iterator() : my_container(NULL), my_index(0), my_value(NULL) {}
        
            template<typename U>
            enumerable_thread_specific_iterator( const enumerable_thread_specific_iterator<Container, U>& other ) :
                    my_container( other.my_container ), my_index( other.my_index), my_value( const_cast<Value *>(other.my_value) ) {}
        
            enumerable_thread_specific_iterator operator+( ptrdiff_t offset ) const {
                return enumerable_thread_specific_iterator(*my_container, my_index + offset);
            }
        
            enumerable_thread_specific_iterator &operator+=( ptrdiff_t offset ) {
                my_index += offset;
                my_value = NULL;
                return *this;
            }
        
            enumerable_thread_specific_iterator operator-( ptrdiff_t offset ) const {
                return enumerable_thread_specific_iterator( *my_container, my_index-offset );
            }
        
            enumerable_thread_specific_iterator &operator-=( ptrdiff_t offset ) {
                my_index -= offset;
                my_value = NULL;
                return *this;
            }
        
            Value& operator*() const {
                Value* value = my_value;
                if( !value ) {
                    value = my_value = &(*my_container)[my_index].value;
                }
                __TBB_ASSERT( value==&(*my_container)[my_index].value, "corrupt cache" );
                return *value;
            }
        
            Value& operator[]( ptrdiff_t k ) const {
               return (*my_container)[my_index + k].value;
            }
        
            Value* operator->() const {return &operator*();}
        
            enumerable_thread_specific_iterator& operator++() {
                ++my_index;
                my_value = NULL;
                return *this;
            }
        
            enumerable_thread_specific_iterator& operator--() {
                --my_index;
                my_value = NULL;
                return *this;
            }
        
            //! Post increment
            enumerable_thread_specific_iterator operator++(int) {
                enumerable_thread_specific_iterator result = *this;
                ++my_index;
                my_value = NULL;
                return result;
            }
        
            //! Post decrement
            enumerable_thread_specific_iterator operator--(int) {
                enumerable_thread_specific_iterator result = *this;
                --my_index;
                my_value = NULL;
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
        enumerable_thread_specific_iterator<Container,T> operator+( ptrdiff_t offset, 
                                                                    const enumerable_thread_specific_iterator<Container,T>& v ) {
            return enumerable_thread_specific_iterator<Container,T>( v.my_container, v.my_index + offset );
        }
        
        template<typename Container, typename T, typename U>
        bool operator==( const enumerable_thread_specific_iterator<Container,T>& i, 
                         const enumerable_thread_specific_iterator<Container,U>& j ) {
            return i.my_index==j.my_index && i.my_container == j.my_container;
        }
        
        template<typename Container, typename T, typename U>
        bool operator!=( const enumerable_thread_specific_iterator<Container,T>& i, 
                         const enumerable_thread_specific_iterator<Container,U>& j ) {
            return !(i==j);
        }
        
        template<typename Container, typename T, typename U>
        bool operator<( const enumerable_thread_specific_iterator<Container,T>& i, 
                        const enumerable_thread_specific_iterator<Container,U>& j ) {
            return i.my_index<j.my_index;
        }
        
        template<typename Container, typename T, typename U>
        bool operator>( const enumerable_thread_specific_iterator<Container,T>& i, 
                        const enumerable_thread_specific_iterator<Container,U>& j ) {
            return j<i;
        }
        
        template<typename Container, typename T, typename U>
        bool operator>=( const enumerable_thread_specific_iterator<Container,T>& i, 
                         const enumerable_thread_specific_iterator<Container,U>& j ) {
            return !(i<j);
        }
        
        template<typename Container, typename T, typename U>
        bool operator<=( const enumerable_thread_specific_iterator<Container,T>& i, 
                         const enumerable_thread_specific_iterator<Container,U>& j ) {
            return !(j<i);
        }
        
        template<typename Container, typename T, typename U>
        ptrdiff_t operator-( const enumerable_thread_specific_iterator<Container,T>& i, 
                             const enumerable_thread_specific_iterator<Container,U>& j ) {
            return i.my_index-j.my_index;
        }

    template<typename SegmentedContainer, typename Value >
        class segmented_iterator
#if defined(_WIN64) && defined(_MSC_VER)
        : public std::iterator<std::input_iterator_tag, Value>
#endif
        {
            template<typename C, typename T, typename U>
            friend bool operator==(const segmented_iterator<C,T>& i, const segmented_iterator<C,U>& j);

            template<typename C, typename T, typename U>
            friend bool operator!=(const segmented_iterator<C,T>& i, const segmented_iterator<C,U>& j);
            
            template<typename C, typename U> 
            friend class segmented_iterator;

            public:

                segmented_iterator() {my_segcont = NULL;}

                segmented_iterator( const SegmentedContainer& _segmented_container ) : 
                    my_segcont(const_cast<SegmentedContainer*>(&_segmented_container)),
                    outer_iter(my_segcont->end()) { }

                ~segmented_iterator() {}

                typedef typename SegmentedContainer::iterator outer_iterator;
                typedef typename SegmentedContainer::value_type InnerContainer;
                typedef typename InnerContainer::iterator inner_iterator;

                // STL support
                typedef ptrdiff_t difference_type;
                typedef Value value_type;
                typedef typename SegmentedContainer::size_type size_type;
                typedef Value* pointer;
                typedef Value& reference;
                typedef std::input_iterator_tag iterator_category;

                // Copy Constructor
                template<typename U>
                segmented_iterator(const segmented_iterator<SegmentedContainer, U>& other) :
                    my_segcont(other.my_segcont),
                    outer_iter(other.outer_iter),
                    // can we assign a default-constructed iterator to inner if we're at the end?
                    inner_iter(other.inner_iter)
                {}

                // assignment
                template<typename U>
                segmented_iterator& operator=( const segmented_iterator<SegmentedContainer, U>& other) {
                    if(this != &other) {
                        my_segcont = other.my_segcont;
                        outer_iter = other.outer_iter;
                        if(outer_iter != my_segcont->end()) inner_iter = other.inner_iter;
                    }
                    return *this;
                }

                // allow assignment of outer iterator to segmented iterator.  Once it is
                // assigned, move forward until a non-empty inner container is found or
                // the end of the outer container is reached.
                segmented_iterator& operator=(const outer_iterator& new_outer_iter) {
                    __TBB_ASSERT(my_segcont != NULL, NULL);
                    // check that this iterator points to something inside the segmented container
                    for(outer_iter = new_outer_iter ;outer_iter!=my_segcont->end(); ++outer_iter) {
                        if( !outer_iter->empty() ) {
                            inner_iter = outer_iter->begin();
                            break;
                        }
                    }
                    return *this;
                }

                // pre-increment
                segmented_iterator& operator++() {
                    advance_me();
                    return *this;
                }

                // post-increment
                segmented_iterator operator++(int) {
                    segmented_iterator tmp = *this;
                    operator++();
                    return tmp;
                }

                bool operator==(const outer_iterator& other_outer) const {
                    __TBB_ASSERT(my_segcont != NULL, NULL);
                    return (outer_iter == other_outer &&
                            (outer_iter == my_segcont->end() || inner_iter == outer_iter->begin()));
                }

                bool operator!=(const outer_iterator& other_outer) const {
                    return !operator==(other_outer);

                }

                // (i)* RHS
                reference operator*() const {
                    __TBB_ASSERT(my_segcont != NULL, NULL);
                    __TBB_ASSERT(outer_iter != my_segcont->end(), "Dereferencing a pointer at end of container");
                    __TBB_ASSERT(inner_iter != outer_iter->end(), NULL); // should never happen
                    return *inner_iter;
                }

                // i->
                pointer operator->() const { return &operator*();}

            private:
                SegmentedContainer*             my_segcont;
                outer_iterator outer_iter;
                inner_iterator inner_iter;

                void advance_me() {
                    __TBB_ASSERT(my_segcont != NULL, NULL);
                    __TBB_ASSERT(outer_iter != my_segcont->end(), NULL); // not true if there are no inner containers
                    __TBB_ASSERT(inner_iter != outer_iter->end(), NULL); // not true if the inner containers are all empty.
                    ++inner_iter;
                    while(inner_iter == outer_iter->end() && ++outer_iter != my_segcont->end()) {
                        inner_iter = outer_iter->begin();
                    }
                }
        };    // segmented_iterator

        template<typename SegmentedContainer, typename T, typename U>
        bool operator==( const segmented_iterator<SegmentedContainer,T>& i, 
                         const segmented_iterator<SegmentedContainer,U>& j ) {
            if(i.my_segcont != j.my_segcont) return false;
            if(i.my_segcont == NULL) return true;
            if(i.outer_iter != j.outer_iter) return false;
            if(i.outer_iter == i.my_segcont->end()) return true;
            return i.inner_iter == j.inner_iter;
        }

        // !=
        template<typename SegmentedContainer, typename T, typename U>
        bool operator!=( const segmented_iterator<SegmentedContainer,T>& i, 
                         const segmented_iterator<SegmentedContainer,U>& j ) {
            return !(i==j);
        }

        // empty template for following specializations
        template<ets_key_usage_type et>
        struct tls_manager {};
        
        //! Struct that doesn't use a key
        template <>
        struct tls_manager<ets_no_key> {
            typedef size_t tls_key_t;
            static inline void create_key( tls_key_t &) { }
            static inline void destroy_key( tls_key_t & ) { }
            static inline void set_tls( tls_key_t &, void *  ) { }
            static inline void * get_tls( tls_key_t & ) { return (size_t)0; }
        };

        //! Struct to use native TLS support directly
        template <>
        struct tls_manager <ets_key_per_instance> {
#if _WIN32||_WIN64
            typedef DWORD tls_key_t;
            static inline void create_key( tls_key_t &k) { k = TlsAlloc(); }
            static inline void destroy_key( tls_key_t &k) { TlsFree(k); }
            static inline void set_tls( tls_key_t &k, void * value) { TlsSetValue(k, (LPVOID)value); }
            static inline void * get_tls( tls_key_t &k ) { return (void *)TlsGetValue(k); }
#else
            typedef pthread_key_t tls_key_t;
            static inline void create_key( tls_key_t &k) { pthread_key_create(&k, NULL); }
            static inline void destroy_key( tls_key_t &k) { pthread_key_delete(k); }
            static inline void set_tls( tls_key_t &k, void * value) { pthread_setspecific(k, value); }
            static inline void * get_tls( tls_key_t &k ) { return pthread_getspecific(k); }
#endif
        };

        class thread_hash_compare {
        public:
            // using hack suggested by Arch to get value for thread id for hashing...
#if _WIN32||_WIN64
            typedef DWORD thread_key;
#else
            typedef pthread_t thread_key;
#endif
            static thread_key my_thread_key(const tbb_thread::id j) {
                thread_key key_val;
                memcpy(&key_val, &j, sizeof(thread_key));
                return key_val;
            }

            bool equal( const thread_key j, const thread_key k) const {
                return j == k;
            }
            unsigned long hash(const thread_key k) const {
                return (unsigned long)k;
            }
        };

        // storage for initialization function pointer
        template<typename T>
        struct callback_base {
            virtual T apply( ) = 0;
            virtual void destroy( ) = 0;
            // need to be able to create copies of callback_base for copy constructor
            virtual callback_base* make_copy() = 0;
            // need virtual destructor to satisfy GCC compiler warning
            virtual ~callback_base() { }
        };

        template <typename T, typename Functor>
        struct callback_leaf : public callback_base<T> {
            typedef Functor my_callback_type;
            typedef callback_leaf<T,Functor> my_type;
            typedef my_type* callback_pointer;
            typedef typename tbb::tbb_allocator<my_type> my_allocator_type;
            Functor f;
            callback_leaf( const Functor& f_) : f(f_) {
            }

            static callback_pointer new_callback(const Functor& f_ ) {
                void* new_void = my_allocator_type().allocate(1);
                callback_pointer new_cb = new (new_void) callback_leaf<T,Functor>(f_); // placement new
                return new_cb;
            }

            /* override */ callback_pointer make_copy() {
                return new_callback( f );
            }

             /* override */ void destroy( ) {
                 callback_pointer my_ptr = this;
                 my_allocator_type().destroy(my_ptr);
                 my_allocator_type().deallocate(my_ptr,1);
             }
            /* override */ T apply() { return f(); }  // does copy construction of returned value.
        };

        template<typename Key, typename T, typename HC, typename A>
        class ets_concurrent_hash_map : public tbb::concurrent_hash_map<Key, T, HC, A> {
        public:
            typedef tbb::concurrent_hash_map<Key, T, HC, A> base_type;
            typedef typename base_type::const_pointer const_pointer;
            typedef typename base_type::key_type key_type;
            const_pointer find( const key_type &k ) {
                return internal_fast_find( k );
            } // make public
        };
    
    } // namespace internal
    //! @endcond

    //! The thread local class template
    template <typename T, 
              typename Allocator=cache_aligned_allocator<T>, 
              ets_key_usage_type ETS_key_type=ets_no_key > 
    class enumerable_thread_specific { 

        template<typename U, typename A, ets_key_usage_type C> friend class enumerable_thread_specific;
    
        typedef internal::tls_manager< ETS_key_type > my_tls_manager;

        //! The padded elements; padded to avoid false sharing
        template<typename U>
        struct padded_element {
            U value;
            char padding[ ( (sizeof(U) - 1) / internal::NFS_MaxLineSize + 1 ) * internal::NFS_MaxLineSize - sizeof(U) ];
            padded_element(const U &v) : value(v) {}
            padded_element() {}
        };
    
        //! A generic range, used to create range objects from the iterators
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
    
        typedef typename Allocator::template rebind< padded_element<T> >::other padded_allocator_type;
        typedef tbb::concurrent_vector< padded_element<T>, padded_allocator_type > internal_collection_type;
        typedef typename internal_collection_type::size_type hash_table_index_type; // storing array indices rather than iterators to simplify
        // copying the hash table that correlates thread IDs with concurrent vector elements.
        
        typedef typename Allocator::template rebind< std::pair< typename internal::thread_hash_compare::thread_key, hash_table_index_type > >::other hash_element_allocator;
        typedef internal::ets_concurrent_hash_map< typename internal::thread_hash_compare::thread_key, hash_table_index_type, internal::thread_hash_compare, hash_element_allocator > thread_to_index_type;

        typename my_tls_manager::tls_key_t my_key;

        void reset_key() {
            my_tls_manager::destroy_key(my_key);
            my_tls_manager::create_key(my_key); 
        }

        internal::callback_base<T> *my_finit_callback;

        // need to use a pointed-to exemplar because T may not be assignable.
        // using tbb_allocator instead of padded_element_allocator because we may be
        // copying an exemplar from one instantiation of ETS to another with a different
        // allocator.
        typedef typename tbb::tbb_allocator<padded_element<T> > exemplar_allocator_type;
        static padded_element<T> * create_exemplar(const T& my_value) {
            padded_element<T> *new_exemplar = 0;
            // void *new_space = padded_allocator_type().allocate(1);
            void *new_space = exemplar_allocator_type().allocate(1);
            new_exemplar = new(new_space) padded_element<T>(my_value);
            return new_exemplar;
        }

        static padded_element<T> *create_exemplar( ) {
            // void *new_space = padded_allocator_type().allocate(1);
            void *new_space = exemplar_allocator_type().allocate(1);
            padded_element<T> *new_exemplar = new(new_space) padded_element<T>( );
            return new_exemplar;
        }

        static void free_exemplar(padded_element<T> *my_ptr) {
            // padded_allocator_type().destroy(my_ptr);
            // padded_allocator_type().deallocate(my_ptr,1);
            exemplar_allocator_type().destroy(my_ptr);
            exemplar_allocator_type().deallocate(my_ptr,1);
        }

        padded_element<T>* my_exemplar_ptr;

        internal_collection_type my_locals;
        thread_to_index_type my_hash_tbl;
    
    public:
    
        //! Basic types
        typedef Allocator allocator_type;
        typedef T value_type;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef typename internal_collection_type::size_type size_type;
        typedef typename internal_collection_type::difference_type difference_type;
    
        // Iterator types
        typedef typename internal::enumerable_thread_specific_iterator< internal_collection_type, value_type > iterator;
        typedef typename internal::enumerable_thread_specific_iterator< internal_collection_type, const value_type > const_iterator;

        // Parallel range types
        typedef generic_range_type< iterator > range_type;
        typedef generic_range_type< const_iterator > const_range_type;
    
        //! Default constructor, which leads to default construction of local copies
        enumerable_thread_specific() : my_finit_callback(0) { 
            my_exemplar_ptr = create_exemplar();
            my_tls_manager::create_key(my_key); 
        }

        //! construction with initializer method
        // Finit should be a function taking 0 parameters and returning a T
        template <typename Finit>
        enumerable_thread_specific( Finit _finit )
        {
            my_finit_callback = internal::callback_leaf<T,Finit>::new_callback( _finit );
            my_tls_manager::create_key(my_key);
            my_exemplar_ptr = 0; // don't need exemplar if function is provided
        }
    
        //! Constuction with exemplar, which leads to copy construction of local copies
        enumerable_thread_specific(const T &_exemplar) : my_finit_callback(0) {
            my_exemplar_ptr = create_exemplar(_exemplar);
            my_tls_manager::create_key(my_key); 
        }
    
        //! Destructor
        ~enumerable_thread_specific() { 
            my_tls_manager::destroy_key(my_key); 
            if(my_finit_callback) {
                my_finit_callback->destroy();
            }
            if(my_exemplar_ptr)
            {
                free_exemplar(my_exemplar_ptr);
            }
        }
      
        //! returns reference to local, discarding exists
        reference local() {
            bool exists;
            return local(exists);
        }

        //! Returns reference to calling thread's local copy, creating one if necessary
        reference local(bool& exists)  {
            if ( pointer local_ptr = static_cast<pointer>(my_tls_manager::get_tls(my_key)) ) {
                exists = true;
               return *local_ptr;
            }
            hash_table_index_type local_index;
            typename internal::thread_hash_compare::thread_key my_t_key = internal::thread_hash_compare::my_thread_key(tbb::this_tbb_thread::get_id());
            {
                typename thread_to_index_type::const_pointer my_existing_entry;
                my_existing_entry = my_hash_tbl.find(my_t_key);
                if(my_existing_entry) {
                    exists = true;
                    local_index = my_existing_entry->second;
                }
                else {

                    // see if the table entry can be found by accessor
                    typename thread_to_index_type::accessor a;
                    if(!my_hash_tbl.insert(a, my_t_key)) {
                        exists = true;
                        local_index = a->second;
                    }
                    else {
                        // create new entry
                        exists = false;
                        if(my_finit_callback) {
                            // convert iterator to array index
#if TBB_DEPRECATED
                            local_index = my_locals.push_back(my_finit_callback->apply());
#else
                            local_index = my_locals.push_back(my_finit_callback->apply()) - my_locals.begin();
#endif
                        }
                        else {
                            // convert iterator to array index
#if TBB_DEPRECATED
                            local_index = my_locals.push_back(*my_exemplar_ptr);
#else
                            local_index = my_locals.push_back(*my_exemplar_ptr) - my_locals.begin();
#endif
                        }
                        // insert into hash table
                        a->second = local_index;
                    }
                }
            }

            reference local_ref = (my_locals[local_index].value);
            my_tls_manager::set_tls( my_key, static_cast<void *>(&local_ref) );
            return local_ref;
        } // local

        //! Get the number of local copies
        size_type size() const { return my_locals.size(); }
    
        //! true if there have been no local copies created
        bool empty() const { return my_locals.empty(); }
    
        //! begin iterator
        iterator begin() { return iterator( my_locals, 0 ); }
        //! end iterator
        iterator end() { return iterator(my_locals, my_locals.size() ); }
    
        //! begin const iterator
        const_iterator begin() const { return const_iterator(my_locals, 0); }
    
        //! end const iterator
        const_iterator end() const { return const_iterator(my_locals, my_locals.size()); }

        //! Get range for parallel algorithms
        range_type range( size_t grainsize=1 ) { return range_type( begin(), end(), grainsize ); } 
        
        //! Get const range for parallel algorithms
        const_range_type range( size_t grainsize=1 ) const { return const_range_type( begin(), end(), grainsize ); }
    
        //! Destroys local copies
        void clear() {
            my_locals.clear();
            my_hash_tbl.clear();
            reset_key();
            // callback is not destroyed
            // exemplar is not destroyed
        }

        // STL container methods
        // copy constructor

    private:

        template<typename U, typename A2, ets_key_usage_type C2>
        void
        internal_copy_construct( const enumerable_thread_specific<U, A2, C2>& other) {
            typedef typename tbb::enumerable_thread_specific<U, A2, C2> other_type;
            for(typename other_type::const_iterator ci = other.begin(); ci != other.end(); ++ci) {
                my_locals.push_back(*ci);
            }
            if(other.my_finit_callback) {
                my_finit_callback = other.my_finit_callback->make_copy();
            }
            else {
                my_finit_callback = 0;
            }
            if(other.my_exemplar_ptr) {
                my_exemplar_ptr = create_exemplar(other.my_exemplar_ptr->value);
            }
            else {
                my_exemplar_ptr = 0;
            }
            my_tls_manager::create_key(my_key);
        }

    public:

        template<typename U, typename Alloc, ets_key_usage_type Cachetype>
        enumerable_thread_specific( const enumerable_thread_specific<U, Alloc, Cachetype>& other ) : my_hash_tbl(other.my_hash_tbl) 
        {   // Have to do push_back because the contained elements are not necessarily assignable.
            internal_copy_construct(other);
        }

        // non-templatized version
        enumerable_thread_specific( const enumerable_thread_specific& other ) : my_hash_tbl(other.my_hash_tbl) 
        {
            internal_copy_construct(other);
        }

    private:

        template<typename U, typename A2, ets_key_usage_type C2>
        enumerable_thread_specific &
        internal_assign(const enumerable_thread_specific<U, A2, C2>& other) {
            typedef typename tbb::enumerable_thread_specific<U, A2, C2> other_type;
            if(static_cast<void *>( this ) != static_cast<const void *>( &other )) {
                this->clear(); // resets TLS key
                my_hash_tbl = other.my_hash_tbl;
                // cannot use assign because T may not be assignable.
                for(typename other_type::const_iterator ci = other.begin(); ci != other.end(); ++ci) {
                    my_locals.push_back(*ci);
                }

                if(my_finit_callback) {
                    my_finit_callback->destroy();
                    my_finit_callback = 0;
                }
                if(my_exemplar_ptr) {
                    free_exemplar(my_exemplar_ptr);
                    my_exemplar_ptr = 0;
                }
                if(other.my_finit_callback) {
                    my_finit_callback = other.my_finit_callback->make_copy();
                }

                if(other.my_exemplar_ptr) {
                    my_exemplar_ptr = create_exemplar(other.my_exemplar_ptr->value);
                }
            }
            return *this;
        }

    public:

        // assignment
        enumerable_thread_specific& operator=(const enumerable_thread_specific& other) {
            return internal_assign(other);
        }

        template<typename U, typename Alloc, ets_key_usage_type Cachetype>
        enumerable_thread_specific& operator=(const enumerable_thread_specific<U, Alloc, Cachetype>& other)
        {
            return internal_assign(other);
        }

    private:

        // combine_func_t has signature T(T,T) or T(const T&, const T&)
        template <typename combine_func_t>
        T internal_combine(typename internal_collection_type::const_range_type r, combine_func_t f_combine) {
            if(r.is_divisible()) {
                typename internal_collection_type::const_range_type r2(r,split());
                return f_combine(internal_combine(r2, f_combine), internal_combine(r, f_combine));
            }
            if(r.size() == 1) {
                return r.begin()->value;
            }
            typename internal_collection_type::const_iterator i2 = r.begin();
            ++i2;
            return f_combine(r.begin()->value, i2->value);
        }

    public:

        // combine_func_t has signature T(T,T) or T(const T&, const T&)
        template <typename combine_func_t>
        T combine(combine_func_t f_combine) {
            if(my_locals.begin() == my_locals.end()) {
                if(my_finit_callback) {
                    return my_finit_callback->apply();
                }
                return (*my_exemplar_ptr).value;
            }
            typename internal_collection_type::const_range_type r(my_locals.begin(), my_locals.end(), (size_t)2);
            return internal_combine(r, f_combine);
        }

        // combine_func_t has signature void(T) or void(const T&)
        template <typename combine_func_t>
        void combine_each(combine_func_t f_combine) {
            for(const_iterator ci = begin(); ci != end(); ++ci) {
                f_combine( *ci );
            }
        }
    }; // enumerable_thread_specific

    template< typename Container >
    class flattened2d {

        // This intermediate typedef is to address issues with VC7.1 compilers
        typedef typename Container::value_type conval_type;

    public:

        //! Basic types
        typedef typename conval_type::size_type size_type;
        typedef typename conval_type::difference_type difference_type;
        typedef typename conval_type::allocator_type allocator_type;
        typedef typename conval_type::value_type value_type;
        typedef typename conval_type::reference reference;
        typedef typename conval_type::const_reference const_reference;
        typedef typename conval_type::pointer pointer;
        typedef typename conval_type::const_pointer const_pointer;

        typedef typename internal::segmented_iterator<Container, value_type> iterator;
        typedef typename internal::segmented_iterator<Container, const value_type> const_iterator;

        flattened2d( const Container &c, typename Container::const_iterator b, typename Container::const_iterator e ) : 
            my_container(const_cast<Container*>(&c)), my_begin(b), my_end(e) { }

        flattened2d( const Container &c ) : 
            my_container(const_cast<Container*>(&c)), my_begin(c.begin()), my_end(c.end()) { }

        iterator begin() { return iterator(*my_container) = my_begin; }
        iterator end() { return iterator(*my_container) = my_end; }
        const_iterator begin() const { return const_iterator(*my_container) = my_begin; }
        const_iterator end() const { return const_iterator(*my_container) = my_end; }

        size_type size() const {
            size_type tot_size = 0;
            for(typename Container::const_iterator i = my_begin; i != my_end; ++i) {
                tot_size += i->size();
            }
            return tot_size;
        }

    private:

        Container *my_container;
        typename Container::const_iterator my_begin;
        typename Container::const_iterator my_end;

    };

    template <typename Container>
    flattened2d<Container> flatten2d(const Container &c, const typename Container::const_iterator b, const typename Container::const_iterator e) {
        return flattened2d<Container>(c, b, e);
    }

    template <typename Container>
    flattened2d<Container> flatten2d(const Container &c) {
        return flattened2d<Container>(c);
    }

} // namespace tbb

#endif
