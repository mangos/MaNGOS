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

#ifndef __TBB_parallel_invoke_H
#define __TBB_parallel_invoke_H

#include "task.h"

namespace tbb {

//! @cond INTERNAL
namespace internal {
    // Simple task object, executing user method
    template<typename function>
    class function_invoker : public task{
    public:
        function_invoker(function& _function) : my_function(_function) {}
    private:
        function &my_function;
        /*override*/
        task* execute()
        {
            my_function();
            return NULL;
        }
    };

    // The class spawns two or three child tasks
    template <size_t N, typename function1, typename function2, typename function3>
    class spawner : public task {
    private:
        function1& my_func1;
        function2& my_func2;
        function3& my_func3;
        bool is_recycled;

        task* execute (){
            if(is_recycled){
                return NULL;
            }else{
                __TBB_ASSERT(N==2 || N==3, "Number of arguments passed to spawner is wrong");
                set_ref_count(N);
                recycle_as_safe_continuation();
                internal::function_invoker<function2>* invoker2 = new (allocate_child()) internal::function_invoker<function2>(my_func2);
                __TBB_ASSERT(invoker2, "Child task allocation failed");
                spawn(*invoker2);
                size_t n = N; // To prevent compiler warnings
                if (n>2) {
                    internal::function_invoker<function3>* invoker3 = new (allocate_child()) internal::function_invoker<function3>(my_func3);
                    __TBB_ASSERT(invoker3, "Child task allocation failed");
                    spawn(*invoker3);
                }
                my_func1();
                is_recycled = true;
                return NULL;
            }
        } // execute

    public:
        spawner(function1& _func1, function2& _func2, function3& _func3) : my_func1(_func1), my_func2(_func2), my_func3(_func3), is_recycled(false) {}
    };

    // Creates and spawns child tasks
    class parallel_invoke_helper : public empty_task {
    public:
        // Dummy functor class
        class parallel_invoke_noop {
        public:
            void operator() () const {}
        };
        // Creates a helper object with user-defined number of children expected
        parallel_invoke_helper(int number_of_children)
        {
            set_ref_count(number_of_children + 1);
        }
        // Adds child task and spawns it
        template <typename function>
        void add_child (function &_func)
        {
            internal::function_invoker<function>* invoker = new (allocate_child()) internal::function_invoker<function>(_func);
            __TBB_ASSERT(invoker, "Child task allocation failed");
            spawn(*invoker);
        }

        // Adds a task with multiple child tasks and spawns it
        // two arguments
        template <typename function1, typename function2>
        void add_children (function1& _func1, function2& _func2)
        {
            // The third argument is dummy, it is ignored actually.
            parallel_invoke_noop noop;
            internal::spawner<2, function1, function2, parallel_invoke_noop>& sub_root = *new(allocate_child())internal::spawner<2, function1, function2, parallel_invoke_noop>(_func1, _func2, noop);
            spawn(sub_root);
        }
        // three arguments
        template <typename function1, typename function2, typename function3>
        void add_children (function1& _func1, function2& _func2, function3& _func3)
        {
            internal::spawner<3, function1, function2, function3>& sub_root = *new(allocate_child())internal::spawner<3, function1, function2, function3>(_func1, _func2, _func3);
            spawn(sub_root);
        }

        // Waits for all child tasks
        template <typename F0>
        void run_and_finish(F0& f0)
        {
            internal::function_invoker<F0>* invoker = new (allocate_child()) internal::function_invoker<F0>(f0);
            __TBB_ASSERT(invoker, "Child task allocation failed");
            spawn_and_wait_for_all(*invoker);
        }
    };
    // The class destroys root if exception occured as well as in normal case
    class parallel_invoke_cleaner: internal::no_copy { 
    public:
        parallel_invoke_cleaner(int number_of_children, tbb::task_group_context& context) : root(*new(task::allocate_root(context)) internal::parallel_invoke_helper(number_of_children))
        {}
        ~parallel_invoke_cleaner(){
            root.destroy(root);
        }
        internal::parallel_invoke_helper& root;
    };
} // namespace internal
//! @endcond

/** \name parallel_invoke
    **/
//@{
//! Executes a list of tasks in parallel and waits for all tasks to complete.
/** @ingroup algorithms */

// parallel_invoke with user-defined context
// two arguments
template<typename F0, typename F1 >
void parallel_invoke(F0 f0, F1 f1, tbb::task_group_context& context) {
    internal::parallel_invoke_cleaner cleaner(2, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_child(f1);

    root.run_and_finish(f0);
}

// three arguments
template<typename F0, typename F1, typename F2 >
void parallel_invoke(F0 f0, F1 f1, F2 f2, tbb::task_group_context& context) {
    internal::parallel_invoke_cleaner cleaner(3, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_child(f2);
    root.add_child(f1);

    root.run_and_finish(f0);
}

// four arguments
template<typename F0, typename F1, typename F2, typename F3>
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, tbb::task_group_context& context) {
    internal::parallel_invoke_cleaner cleaner(4, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_child(f3);
    root.add_child(f2);
    root.add_child(f1);

    root.run_and_finish(f0);
}

// five arguments
template<typename F0, typename F1, typename F2, typename F3, typename F4 >
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, F4 f4, tbb::task_group_context& context) {
    internal::parallel_invoke_cleaner cleaner(3, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_children(f4, f3);
    root.add_children(f2, f1);

    root.run_and_finish(f0);
}

// six arguments
template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5 >
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, F4 f4, F5 f5, tbb::task_group_context& context) {
    internal::parallel_invoke_cleaner cleaner(3, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_children(f5, f4, f3);
    root.add_children(f2, f1);

    root.run_and_finish(f0);
}

// seven arguments
template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6 >
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, F4 f4, F5 f5, F6 f6, tbb::task_group_context& context) {
    internal::parallel_invoke_cleaner cleaner(3, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_children(f6, f5, f4);
    root.add_children(f3, f2, f1);

    root.run_and_finish(f0);
}

// eight arguments
template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6,
    typename F7>
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, F4 f4, F5 f5, F6 f6, F7 f7, tbb::task_group_context& context) {
    internal::parallel_invoke_cleaner cleaner(4, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_children(f7, f6, f5);
    root.add_children(f4, f3);
    root.add_children(f2, f1);

    root.run_and_finish(f0);
}

// nine arguments
template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6,
        typename F7, typename F8>
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, F4 f4, F5 f5, F6 f6, F7 f7, F8 f8, tbb::task_group_context& context) {
    internal::parallel_invoke_cleaner cleaner(4, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_children(f8, f7, f6);
    root.add_children(f5, f4, f3);
    root.add_children(f2, f1);

    root.run_and_finish(f0);
}

// ten arguments
template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6,
        typename F7, typename F8, typename F9>
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, F4 f4, F5 f5, F6 f6, F7 f7, F8 f8, F9 f9, tbb::task_group_context& context) {
    internal::parallel_invoke_cleaner cleaner(4, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_children(f9, f8, f7);
    root.add_children(f6, f5, f4);
    root.add_children(f3, f2, f1);

    root.run_and_finish(f0);
}

// two arguments
template<typename F0, typename F1>
void parallel_invoke(F0 f0, F1 f1) {
    task_group_context context;
    parallel_invoke<F0, F1>(f0, f1, context);
}
// three arguments
template<typename F0, typename F1, typename F2>
void parallel_invoke(F0 f0, F1 f1, F2 f2) {
    task_group_context context;
    parallel_invoke<F0, F1, F2>(f0, f1, f2, context);
}
// four arguments
template<typename F0, typename F1, typename F2, typename F3 >
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3) {
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3>(f0, f1, f2, f3, context);
}
// five arguments
template<typename F0, typename F1, typename F2, typename F3, typename F4>
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, F4 f4) {
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3, F4>(f0, f1, f2, f3, f4, context);
}
// six arguments
template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5>
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, F4 f4, F5 f5) {
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3, F4, F5>(f0, f1, f2, f3, f4, f5, context);
}
// seven arguments
template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6>
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, F4 f4, F5 f5, F6 f6) {
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3, F4, F5, F6>(f0, f1, f2, f3, f4, f5, f6, context);
}
// eigth arguments
template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6,
        typename F7>
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, F4 f4, F5 f5, F6 f6, F7 f7) {
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3, F4, F5, F6, F7>(f0, f1, f2, f3, f4, f5, f6, f7, context);
}
// nine arguments
template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6,
        typename F7, typename F8>
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, F4 f4, F5 f5, F6 f6, F7 f7, F8 f8) {
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3, F4, F5, F6, F7, F8>(f0, f1, f2, f3, f4, f5, f6, f7, f8, context);
}
// ten arguments
template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6,
        typename F7, typename F8, typename F9>
void parallel_invoke(F0 f0, F1 f1, F2 f2, F3 f3, F4 f4, F5 f5, F6 f6, F7 f7, F8 f8, F9 f9) {
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3, F4, F5, F6, F7, F8, F9>(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, context);
}

//@}

} // namespace

#endif /* __TBB_parallel_invoke_H */
