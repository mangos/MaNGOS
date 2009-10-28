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

#ifndef __TBB_combinable_H
#define __TBB_combinable_H

#include "tbb/enumerable_thread_specific.h"
#include "tbb/cache_aligned_allocator.h"

namespace tbb {
/** \name combinable
    **/
//@{
//! Thread-local storage with optional reduction
/** @ingroup containers */
    template <typename T>
        class combinable {
    private:
        typedef typename tbb::cache_aligned_allocator<T> my_alloc;

        typedef typename tbb::enumerable_thread_specific<T, my_alloc, ets_no_key> my_ets_type;
        my_ets_type my_ets; 
 
    public:

        combinable() { }

        template <typename finit>
        combinable( finit _finit) : my_ets(_finit) { }

        //! destructor
        ~combinable() { 
        }

        combinable(const combinable& other) : my_ets(other.my_ets) { }

        combinable & operator=( const combinable & other) { my_ets = other.my_ets; return *this; }

        void clear() { my_ets.clear(); }

        T& local() { return my_ets.local(); }

        T& local(bool & exists) { return my_ets.local(exists); }

        template< typename FCombine>
        T combine(FCombine fcombine) { return my_ets.combine(fcombine); }

        template<typename FCombine>
        void combine_each(FCombine fcombine) { my_ets.combine_each(fcombine); }

    };
} // namespace tbb
#endif /* __TBB_combinable_H */
