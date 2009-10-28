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

#ifndef __TBB_task_scheduler_observer_H
#define __TBB_task_scheduler_observer_H

#include "atomic.h"

#if __TBB_SCHEDULER_OBSERVER

namespace tbb {

namespace internal {

class observer_proxy;

class task_scheduler_observer_v3 {
    friend class observer_proxy;
    observer_proxy* my_proxy;
    atomic<intptr> my_busy_count;
public:
    //! Enable or disable observation
    void __TBB_EXPORTED_METHOD observe( bool state=true );

    //! True if observation is enables; false otherwise.
    bool is_observing() const {return my_proxy!=NULL;}

    //! Construct observer with observation disabled.
    task_scheduler_observer_v3() : my_proxy(NULL) {my_busy_count=0;}

    //! Called by thread before first steal since observation became enabled
    virtual void on_scheduler_entry( bool /*is_worker*/ ) {} 

    //! Called by thread when it no longer takes part in task stealing.
    virtual void on_scheduler_exit( bool /*is_worker*/ ) {}

    //! Destructor
    virtual ~task_scheduler_observer_v3() {observe(false);}
};

} // namespace internal

typedef internal::task_scheduler_observer_v3 task_scheduler_observer;

} // namespace tbb

#endif /* __TBB_SCHEDULER_OBSERVER */

#endif /* __TBB_task_scheduler_observer_H */
