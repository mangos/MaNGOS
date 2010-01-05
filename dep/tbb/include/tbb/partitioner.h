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

#ifndef __TBB_partitioner_H
#define __TBB_partitioner_H

#include "task.h"

namespace tbb {
class affinity_partitioner;

//! @cond INTERNAL
namespace internal {
size_t __TBB_EXPORTED_FUNC get_initial_auto_partitioner_divisor();

//! Defines entry points into tbb run-time library;
/** The entry points are the constructor and destructor. */
class affinity_partitioner_base_v3: no_copy {
    friend class tbb::affinity_partitioner;
    //! Array that remembers affinities of tree positions to affinity_id.
    /** NULL if my_size==0. */
    affinity_id* my_array;
    //! Number of elements in my_array.
    size_t my_size;
    //! Zeros the fields.
    affinity_partitioner_base_v3() : my_array(NULL), my_size(0) {}
    //! Deallocates my_array.
    ~affinity_partitioner_base_v3() {resize(0);}
    //! Resize my_array.
    /** Retains values if resulting size is the same. */
    void __TBB_EXPORTED_METHOD resize( unsigned factor );
    friend class affinity_partition_type;
};

//! Provides default methods for partition objects without affinity.
class partition_type_base {
public:
    void set_affinity( task & ) {}
    void note_affinity( task::affinity_id ) {}
    task* continue_after_execute_range( task& ) {return NULL;}
    bool decide_whether_to_delay() {return false;}
    void spawn_or_delay( bool, task& a, task& b ) {
        a.spawn(b);
    }
};

class affinity_partition_type;

template<typename Range, typename Body, typename Partitioner> class start_for;
template<typename Range, typename Body, typename Partitioner> class start_reduce;
template<typename Range, typename Body> class start_reduce_with_affinity;
template<typename Range, typename Body, typename Partitioner> class start_scan;

} // namespace internal
//! @endcond

//! A simple partitioner 
/** Divides the range until the range is not divisible. 
    @ingroup algorithms */
class simple_partitioner {
public:
    simple_partitioner() {}
private:
    template<typename Range, typename Body, typename Partitioner> friend class internal::start_for;
    template<typename Range, typename Body, typename Partitioner> friend class internal::start_reduce;
    template<typename Range, typename Body, typename Partitioner> friend class internal::start_scan;

    class partition_type: public internal::partition_type_base {
    public:
        bool should_execute_range(const task& ) {return false;}
        partition_type( const simple_partitioner& ) {}
        partition_type( const partition_type&, split ) {}
    };
};

//! An auto partitioner 
/** The range is initial divided into several large chunks.
    Chunks are further subdivided into VICTIM_CHUNKS pieces if they are stolen and divisible.
    @ingroup algorithms */
class auto_partitioner {
public:
    auto_partitioner() {}

private:
    template<typename Range, typename Body, typename Partitioner> friend class internal::start_for;
    template<typename Range, typename Body, typename Partitioner> friend class internal::start_reduce;
    template<typename Range, typename Body, typename Partitioner> friend class internal::start_scan;

    class partition_type: public internal::partition_type_base {
        size_t num_chunks;
        static const size_t VICTIM_CHUNKS = 4;
public:
        bool should_execute_range(const task &t) {
            if( num_chunks<VICTIM_CHUNKS && t.is_stolen_task() )
                num_chunks = VICTIM_CHUNKS;
            return num_chunks==1;
        }
        partition_type( const auto_partitioner& ) : num_chunks(internal::get_initial_auto_partitioner_divisor()) {}
        partition_type( partition_type& pt, split ) {
            num_chunks = pt.num_chunks /= 2u;
        }
    };
};

//! An affinity partitioner
class affinity_partitioner: internal::affinity_partitioner_base_v3 {
public:
    affinity_partitioner() {}

private:
    template<typename Range, typename Body, typename Partitioner> friend class internal::start_for;
    template<typename Range, typename Body, typename Partitioner> friend class internal::start_reduce;
    template<typename Range, typename Body> friend class internal::start_reduce_with_affinity;
    template<typename Range, typename Body, typename Partitioner> friend class internal::start_scan;

    typedef internal::affinity_partition_type partition_type;
    friend class internal::affinity_partition_type;
};

//! @cond INTERNAL
namespace internal {

class affinity_partition_type: public no_copy {
    //! Must be power of two
    static const unsigned factor = 16;
    static const size_t VICTIM_CHUNKS = 4;

    internal::affinity_id* my_array;
    task_list delay_list;
    unsigned map_begin, map_end;
    size_t num_chunks;
public:
    affinity_partition_type( affinity_partitioner& ap ) {
        __TBB_ASSERT( (factor&(factor-1))==0, "factor must be power of two" ); 
        ap.resize(factor);
        my_array = ap.my_array;
        map_begin = 0;
        map_end = unsigned(ap.my_size);
        num_chunks = internal::get_initial_auto_partitioner_divisor();
    }
    affinity_partition_type(affinity_partition_type& p, split) : my_array(p.my_array) {
        __TBB_ASSERT( p.map_end-p.map_begin<factor || (p.map_end-p.map_begin)%factor==0, NULL );
        num_chunks = p.num_chunks /= 2;
        unsigned e = p.map_end;
        unsigned d = (e - p.map_begin)/2;
        if( d>factor ) 
            d &= 0u-factor;
        map_end = e;
        map_begin = p.map_end = e-d;
    }

    bool should_execute_range(const task &t) {
        if( num_chunks < VICTIM_CHUNKS && t.is_stolen_task() )
            num_chunks = VICTIM_CHUNKS;
        return num_chunks == 1;
    }

    void set_affinity( task &t ) {
        if( map_begin<map_end )
            t.set_affinity( my_array[map_begin] );
    }
    void note_affinity( task::affinity_id id ) {
        if( map_begin<map_end ) 
            my_array[map_begin] = id;
    }
    task* continue_after_execute_range( task& t ) {
        task* first = NULL;
        if( !delay_list.empty() ) {
            first = &delay_list.pop_front();
            while( !delay_list.empty() ) {
                t.spawn(*first);
                first = &delay_list.pop_front();
            }
        }
        return first;
    }
    bool decide_whether_to_delay() {
        // The possible underflow caused by "-1u" is deliberate
        return (map_begin&(factor-1))==0 && map_end-map_begin-1u<factor;
    }
    void spawn_or_delay( bool delay, task& a, task& b ) {
        if( delay )  
            delay_list.push_back(b);
        else 
            a.spawn(b);
    }

    ~affinity_partition_type() {
        // The delay_list can be non-empty if an exception is thrown.
        while( !delay_list.empty() ) {
            task& t = delay_list.pop_front();
            t.destroy(t);
        } 
    }
};

} // namespace internal
//! @endcond


} // namespace tbb

#endif /* __TBB_partitioner_H */
