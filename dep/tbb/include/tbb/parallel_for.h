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

#ifndef __TBB_parallel_for_H
#define __TBB_parallel_for_H

#include "task.h"
#include "partitioner.h"
#include "blocked_range.h"
#include <new>
#include <stdexcept> // std::invalid_argument
#include <string> // std::invalid_argument text

namespace tbb {

//! @cond INTERNAL
namespace internal {

    //! Task type used in parallel_for
    /** @ingroup algorithms */
    template<typename Range, typename Body, typename Partitioner>
    class start_for: public task {
        Range my_range;
        const Body my_body;
        typename Partitioner::partition_type my_partition;
        /*override*/ task* execute();

        //! Constructor for root task.
        start_for( const Range& range, const Body& body, Partitioner& partitioner ) :
            my_range(range),    
            my_body(body),
            my_partition(partitioner)
        {
        }
        //! Splitting constructor used to generate children.
        /** this becomes left child.  Newly constructed object is right child. */
        start_for( start_for& parent, split ) :
            my_range(parent.my_range,split()),    
            my_body(parent.my_body),
            my_partition(parent.my_partition,split())
        {
            my_partition.set_affinity(*this);
        }
        //! Update affinity info, if any.
        /*override*/ void note_affinity( affinity_id id ) {
            my_partition.note_affinity( id );
        }
    public:
        static void run(  const Range& range, const Body& body, const Partitioner& partitioner ) {
            if( !range.empty() ) {
#if !__TBB_EXCEPTIONS || TBB_JOIN_OUTER_TASK_GROUP
                start_for& a = *new(task::allocate_root()) start_for(range,body,const_cast<Partitioner&>(partitioner));
#else
                // Bound context prevents exceptions from body to affect nesting or sibling algorithms,
                // and allows users to handle exceptions safely by wrapping parallel_for in the try-block.
                task_group_context context;
                start_for& a = *new(task::allocate_root(context)) start_for(range,body,const_cast<Partitioner&>(partitioner));
#endif /* __TBB_EXCEPTIONS && !TBB_JOIN_OUTER_TASK_GROUP */
                task::spawn_root_and_wait(a);
            }
        }
#if __TBB_EXCEPTIONS
        static void run(  const Range& range, const Body& body, const Partitioner& partitioner, task_group_context& context ) {
            if( !range.empty() ) {
                start_for& a = *new(task::allocate_root(context)) start_for(range,body,const_cast<Partitioner&>(partitioner));
                task::spawn_root_and_wait(a);
            }
        }
#endif /* __TBB_EXCEPTIONS */
    };

    template<typename Range, typename Body, typename Partitioner>
    task* start_for<Range,Body,Partitioner>::execute() {
        if( !my_range.is_divisible() || my_partition.should_execute_range(*this) ) {
            my_body( my_range );
            return my_partition.continue_after_execute_range(*this); 
        } else {
            empty_task& c = *new( this->allocate_continuation() ) empty_task;
            recycle_as_child_of(c);
            c.set_ref_count(2);
            bool delay = my_partition.decide_whether_to_delay();
            start_for& b = *new( c.allocate_child() ) start_for(*this,split());
            my_partition.spawn_or_delay(delay,*this,b);
            return this;
        }
    } 
} // namespace internal
//! @endcond


// Requirements on Range concept are documented in blocked_range.h

/** \page parallel_for_body_req Requirements on parallel_for body
    Class \c Body implementing the concept of parallel_for body must define:
    - \code Body::Body( const Body& ); \endcode                 Copy constructor
    - \code Body::~Body(); \endcode                             Destructor
    - \code void Body::operator()( Range& r ) const; \endcode   Function call operator applying the body to range \c r.
**/

/** \name parallel_for
    See also requirements on \ref range_req "Range" and \ref parallel_for_body_req "parallel_for Body". **/
//@{

//! Parallel iteration over range with default partitioner. 
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_for( const Range& range, const Body& body ) {
    internal::start_for<Range,Body,__TBB_DEFAULT_PARTITIONER>::run(range,body,__TBB_DEFAULT_PARTITIONER());
}

//! Parallel iteration over range with simple partitioner.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_for( const Range& range, const Body& body, const simple_partitioner& partitioner ) {
    internal::start_for<Range,Body,simple_partitioner>::run(range,body,partitioner);
}

//! Parallel iteration over range with auto_partitioner.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_for( const Range& range, const Body& body, const auto_partitioner& partitioner ) {
    internal::start_for<Range,Body,auto_partitioner>::run(range,body,partitioner);
}

//! Parallel iteration over range with affinity_partitioner.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_for( const Range& range, const Body& body, affinity_partitioner& partitioner ) {
    internal::start_for<Range,Body,affinity_partitioner>::run(range,body,partitioner);
}

#if __TBB_EXCEPTIONS
//! Parallel iteration over range with simple partitioner and user-supplied context.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_for( const Range& range, const Body& body, const simple_partitioner& partitioner, task_group_context& context ) {
    internal::start_for<Range,Body,simple_partitioner>::run(range, body, partitioner, context);
}

//! Parallel iteration over range with auto_partitioner and user-supplied context.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_for( const Range& range, const Body& body, const auto_partitioner& partitioner, task_group_context& context ) {
    internal::start_for<Range,Body,auto_partitioner>::run(range, body, partitioner, context);
}

//! Parallel iteration over range with affinity_partitioner and user-supplied context.
/** @ingroup algorithms **/
template<typename Range, typename Body>
void parallel_for( const Range& range, const Body& body, affinity_partitioner& partitioner, task_group_context& context ) {
    internal::start_for<Range,Body,affinity_partitioner>::run(range,body,partitioner, context);
}
#endif /* __TBB_EXCEPTIONS */
//@}

//! @cond INTERNAL
namespace internal {
    //! Calls the function with values from range [begin, end) with a step provided
template<typename Function, typename Index>
class parallel_for_body : internal::no_assign {
    const Function &my_func;
    const Index my_begin;
    const Index my_step; 
public:
    parallel_for_body( const Function& _func, Index& _begin, Index& _step) 
        : my_func(_func), my_begin(_begin), my_step(_step) {}
    
    void operator()( tbb::blocked_range<Index>& r ) const {
        for( Index i = r.begin(),  k = my_begin + i * my_step; i < r.end(); i++, k = k + my_step)
            my_func( k );
    }
};
} // namespace internal
//! @endcond

namespace strict_ppl {

//@{
//! Parallel iteration over a range of integers with a step provided
template <typename Index, typename Function>
void parallel_for(Index first, Index last, Index step, const Function& f) {
    tbb::task_group_context context;
    parallel_for(first, last, step, f, context);
}
template <typename Index, typename Function>
void parallel_for(Index first, Index last, Index step, const Function& f, tbb::task_group_context &context) {
    if (step <= 0 ) throw std::invalid_argument("step should be positive");

    if (last > first) {
        Index end = (last - first) / step;
        if (first + end * step < last) end++;
        tbb::blocked_range<Index> range(static_cast<Index>(0), end);
        internal::parallel_for_body<Function, Index> body(f, first, step);
        tbb::parallel_for(range, body, tbb::auto_partitioner(), context);
    }
}
//! Parallel iteration over a range of integers with a default step value
template <typename Index, typename Function>
void parallel_for(Index first, Index last, const Function& f) {
    tbb::task_group_context context;
    parallel_for(first, last, static_cast<Index>(1), f, context);
}
template <typename Index, typename Function>
void parallel_for(Index first, Index last, const Function& f, tbb::task_group_context &context) {
    parallel_for(first, last, static_cast<Index>(1), f, context);
}

//@}

} // namespace strict_ppl

using strict_ppl::parallel_for;

} // namespace tbb

#endif /* __TBB_parallel_for_H */

