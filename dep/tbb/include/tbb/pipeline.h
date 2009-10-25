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

#ifndef __TBB_pipeline_H 
#define __TBB_pipeline_H 

#include "atomic.h"
#include "task.h"
#include <cstddef>

namespace tbb {

class pipeline;
class filter;

//! @cond INTERNAL
namespace internal {

// The argument for PIPELINE_VERSION should be an integer between 2 and 9
#define __TBB_PIPELINE_VERSION(x) (unsigned char)(x-2)<<1

typedef unsigned long Token;
typedef long tokendiff_t;
class stage_task;
class input_buffer;
class pipeline_root_task;
class pipeline_cleaner;

} // namespace internal
//! @endcond

//! A stage in a pipeline.
/** @ingroup algorithms */
class filter: internal::no_copy {
private:
    //! Value used to mark "not in pipeline"
    static filter* not_in_pipeline() {return reinterpret_cast<filter*>(internal::intptr(-1));}
    
    //! The lowest bit 0 is for parallel vs. serial
    static const unsigned char filter_is_serial = 0x1; 

    //! 4th bit distinguishes ordered vs unordered filters.
    /** The bit was not set for parallel filters in TBB 2.1 and earlier,
        but is_ordered() function always treats parallel filters as out of order. */
    static const unsigned char filter_is_out_of_order = 0x1<<4;  

    //! 5th bit distinguishes thread-bound and regular filters.
    static const unsigned char filter_is_bound = 0x1<<5;  

    static const unsigned char current_version = __TBB_PIPELINE_VERSION(5);
    static const unsigned char version_mask = 0x7<<1; // bits 1-3 are for version
public:
    enum mode {
        //! processes multiple items in parallel and in no particular order
        parallel = current_version | filter_is_out_of_order, 
        //! processes items one at a time; all such filters process items in the same order
        serial_in_order = current_version | filter_is_serial,
        //! processes items one at a time and in no particular order
        serial_out_of_order = current_version | filter_is_serial | filter_is_out_of_order,
        //! @deprecated use serial_in_order instead
        serial = serial_in_order
    };
protected:
    filter( bool is_serial_ ) : 
        next_filter_in_pipeline(not_in_pipeline()),
        my_input_buffer(NULL),
        my_filter_mode(static_cast<unsigned char>(is_serial_ ? serial : parallel)),
        prev_filter_in_pipeline(not_in_pipeline()),
        my_pipeline(NULL),
        next_segment(NULL)
    {}
    
    filter( mode filter_mode ) :
        next_filter_in_pipeline(not_in_pipeline()),
        my_input_buffer(NULL),
        my_filter_mode(static_cast<unsigned char>(filter_mode)),
        prev_filter_in_pipeline(not_in_pipeline()),
        my_pipeline(NULL),
        next_segment(NULL)
    {}

public:
    //! True if filter is serial.
    bool is_serial() const {
        return bool( my_filter_mode & filter_is_serial );
    }  
    
    //! True if filter must receive stream in order.
    bool is_ordered() const {
        return (my_filter_mode & (filter_is_out_of_order|filter_is_serial))==filter_is_serial;
    }

    //! True if filter is thread-bound.
    bool is_bound() const {
        return ( my_filter_mode & filter_is_bound )==filter_is_bound;
    }

    //! Operate on an item from the input stream, and return item for output stream.
    /** Returns NULL if filter is a sink. */
    virtual void* operator()( void* item ) = 0;

    //! Destroy filter.  
    /** If the filter was added to a pipeline, the pipeline must be destroyed first. */
    virtual __TBB_EXPORTED_METHOD ~filter();

#if __TBB_EXCEPTIONS
    //! Destroys item if pipeline was cancelled.
    /** Required to prevent memory leaks.
        Note it can be called concurrently even for serial filters.*/
    virtual void finalize( void* /*item*/ ) {};
#endif

private:
    //! Pointer to next filter in the pipeline.
    filter* next_filter_in_pipeline;

    //! Buffer for incoming tokens, or NULL if not required.
    /** The buffer is required if the filter is serial or follows a thread-bound one. */
    internal::input_buffer* my_input_buffer;

    friend class internal::stage_task;
    friend class internal::pipeline_root_task;
    friend class pipeline;
    friend class thread_bound_filter;

    //! Storage for filter mode and dynamically checked implementation version.
    const unsigned char my_filter_mode;

    //! Pointer to previous filter in the pipeline.
    filter* prev_filter_in_pipeline;

    //! Pointer to the pipeline.
    pipeline* my_pipeline;

    //! Pointer to the next "segment" of filters, or NULL if not required.
    /** In each segment, the first filter is not thread-bound but follows a thread-bound one. */
    filter* next_segment;
};

//! A stage in a pipeline served by a user thread.
/** @ingroup algorithms */
class thread_bound_filter: public filter {
public:
    enum result_type {
        // item was processed
        success,
        // item is currently not available
        item_not_available,
        // there are no more items to process
        end_of_stream
    };
protected:
    thread_bound_filter(mode filter_mode): 
         filter(static_cast<mode>(filter_mode | filter::filter_is_bound))
    {}
public:
    //! If a data item is available, invoke operator() on that item.  
    /** This interface is non-blocking.
        Returns 'success' if an item was processed.
        Returns 'item_not_available' if no item can be processed now 
        but more may arrive in the future, or if token limit is reached. 
        Returns 'end_of_stream' if there are no more items to process. */
    result_type __TBB_EXPORTED_METHOD try_process_item(); 

    //! Wait until a data item becomes available, and invoke operator() on that item.
    /** This interface is blocking.
        Returns 'success' if an item was processed.
        Returns 'end_of_stream' if there are no more items to process.
        Never returns 'item_not_available', as it blocks until another return condition applies. */
    result_type __TBB_EXPORTED_METHOD process_item();

private:
    //! Internal routine for item processing
    result_type internal_process_item(bool is_blocking);
};

//! A processing pipeling that applies filters to items.
/** @ingroup algorithms */
class pipeline {
public:
    //! Construct empty pipeline.
    __TBB_EXPORTED_METHOD pipeline();

    /** Though the current implementation declares the destructor virtual, do not rely on this 
        detail.  The virtualness is deprecated and may disappear in future versions of TBB. */
    virtual __TBB_EXPORTED_METHOD ~pipeline();

    //! Add filter to end of pipeline.
    void __TBB_EXPORTED_METHOD add_filter( filter& filter_ );

    //! Run the pipeline to completion.
    void __TBB_EXPORTED_METHOD run( size_t max_number_of_live_tokens );

#if __TBB_EXCEPTIONS
    //! Run the pipeline to completion with user-supplied context.
    void __TBB_EXPORTED_METHOD run( size_t max_number_of_live_tokens, tbb::task_group_context& context );
#endif

    //! Remove all filters from the pipeline.
    void __TBB_EXPORTED_METHOD clear();

private:
    friend class internal::stage_task;
    friend class internal::pipeline_root_task;
    friend class filter;
    friend class thread_bound_filter;
    friend class internal::pipeline_cleaner;

    //! Pointer to first filter in the pipeline.
    filter* filter_list;

    //! Pointer to location where address of next filter to be added should be stored.
    filter* filter_end;

    //! task who's reference count is used to determine when all stages are done.
    task* end_counter;

    //! Number of idle tokens waiting for input stage.
    atomic<internal::Token> input_tokens;

    //! Global counter of tokens 
    atomic<internal::Token> token_counter;

    //! False until fetch_input returns NULL.
    bool end_of_input;

    //! True if the pipeline contains a thread-bound filter; false otherwise.
    bool has_thread_bound_filters;

    //! Remove filter from pipeline.
    void remove_filter( filter& filter_ );

    //! Not used, but retained to satisfy old export files.
    void __TBB_EXPORTED_METHOD inject_token( task& self );

#if __TBB_EXCEPTIONS
    //! Does clean up if pipeline is cancelled or exception occured
    void clear_filters();
#endif
};

} // tbb

#endif /* __TBB_pipeline_H */
