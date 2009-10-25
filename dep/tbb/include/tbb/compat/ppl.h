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

#ifndef __TBB_compat_ppl_H
#define __TBB_compat_ppl_H

#include "../task_group.h"
#include "../parallel_invoke.h"
#include "../parallel_for_each.h"
#include "../parallel_for.h"

namespace Concurrency {

    using tbb::task_handle;
    using tbb::task_group_status;
    using tbb::task_group;
    using tbb::structured_task_group;
    using tbb::missing_wait;
    using tbb::make_task;

    using tbb::not_complete;
    using tbb::complete;
    using tbb::canceled;

    using tbb::is_current_task_group_canceling;

    using tbb::parallel_invoke;
    using tbb::strict_ppl::parallel_for;
    using tbb::parallel_for_each;

} // namespace Concurrency

#endif /* __TBB_compat_ppl_H */
