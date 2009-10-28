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

#include "tbb/enumerable_thread_specific.h"
#include "tbb/concurrent_queue.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/atomic.h"
#include "tbb/spin_mutex.h"

namespace tbb {

    namespace internal {

        // Manages fake TLS keys and fake TLS space
        // Uses only a single native TLS key through use of an enumerable_thread_specific< ... , ets_key_per_instance >
        class tls_single_key_manager {

            // Typedefs
            typedef concurrent_vector<void *> local_vector_type;
            typedef enumerable_thread_specific< local_vector_type, cache_aligned_allocator<local_vector_type>, ets_key_per_instance > my_ets_type;
            typedef local_vector_type::size_type fake_key_t;

            // The fake TLS space
            my_ets_type my_vectors;

            // The next never-yet-assigned fake TLS key
            atomic< fake_key_t > next_key;

            // A Q of fake TLS keys that can be reused
            typedef spin_mutex free_mutex_t;
            free_mutex_t free_mutex;

            struct free_node_t {
                fake_key_t key;        
                free_node_t *next;
            };

            cache_aligned_allocator< free_node_t > my_allocator;
            free_node_t *free_stack;

            bool pop_if_present( fake_key_t &k ) { 
                free_node_t *n = NULL;
                {
                    free_mutex_t::scoped_lock(free_mutex);
                    n = free_stack;
                    if (n) free_stack = n->next;
                }
                if ( n ) {
                    k = n->key;
                    my_allocator.deallocate(n,1);
                    return true;
                }
                return false;
            }

            void push( fake_key_t &k ) { 
                free_node_t *n = my_allocator.allocate(1); 
                n->key = k;
                {
                    free_mutex_t::scoped_lock(free_mutex);
                    n->next = free_stack;
                    free_stack = n;
                }
            }

        public:

            tls_single_key_manager() : free_stack(NULL)  {
                next_key = 0;
            }

            ~tls_single_key_manager() {
                free_node_t *n = free_stack; 
                while (n != NULL) {
                    free_node_t *next = n->next;
                    my_allocator.deallocate(n,1);
                    n = next;
                }
            }

            // creates or finds an available fake TLS key
            inline void create_key( fake_key_t &k ) {
                if ( !(free_stack && pop_if_present( k )) ) {
                    k = next_key.fetch_and_add(1);     
                } 
            }

            // resets the fake TLS space associated with the key and then recycles the key
            inline void destroy_key( fake_key_t &k ) {
                for ( my_ets_type::iterator i = my_vectors.begin(); i != my_vectors.end(); ++i ) {
                    local_vector_type &ivec = *i;
                    if (ivec.size() > k) 
                        ivec[k] = NULL;
                } 
                push(k);
            }

            // sets the fake TLS space to point to the given value for this thread
            inline void set_tls( fake_key_t &k, void *value ) {
                local_vector_type &my_vector = my_vectors.local();
                local_vector_type::size_type size = my_vector.size();

                if ( size <= k ) { 
                    // We use grow_by so that we can initialize the pointers to NULL
                    my_vector.grow_by( k - size + 1, NULL );
                }
                my_vector[k] = value;
            }

            inline void *get_tls( fake_key_t &k ) {
                local_vector_type &my_vector = my_vectors.local();
                if (my_vector.size() > k) 
                    return my_vector[k];
                else
                    return NULL;
            }

        };

        // The single static instance of tls_single_key_manager
        static tls_single_key_manager tls_key_manager;

        // The EXPORTED functions
        void
        tls_single_key_manager_v4::create_key( tls_key_t &k) {
            tls_key_manager.create_key( k );
        }

        void
        tls_single_key_manager_v4::destroy_key( tls_key_t &k) {
            tls_key_manager.destroy_key( k );
        }

        void
        tls_single_key_manager_v4::set_tls( tls_key_t &k, void *value) {
             tls_key_manager.set_tls( k, value);
        }

        void *
        tls_single_key_manager_v4::get_tls( tls_key_t &k ) {
            return tls_key_manager.get_tls( k );
        }
 
    }

}

