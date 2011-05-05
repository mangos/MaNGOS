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

#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_reduce.h"


#define NRUNS               10
#define ONE_TEST_DURATION   0.01

#include "perf_util.h"


#define NUM_CHILD_TASKS     128
#define NUM_ROOT_TASKS      16

#define N               1000000
#define FINE_GRAIN      50
#define MED_GRAIN       500
#define COARSE_GRAIN    10000


typedef ANCHOR_TYPE count_type;
typedef tbb::blocked_range<count_type> range_type;

const count_type NUM_leaf_tasks = NUM_CHILD_TASKS * NUM_ROOT_TASKS;

const count_type N_finest = (count_type)(N/log((double)N)/10);
const count_type N_fine = N_finest * 10;


class static_task_holder {
public:
    tbb::task   *my_simple_leaf_task_ptr;
    
    static_task_holder ();
};

static static_task_holder s_tasks;


static size_t s_num_iterations = 0;


class simple_leaf_task : public tbb::task
{
    task* execute () {
        for ( size_t i=0; i < s_num_iterations; ++i )
            util::anchor += i;
            //util::anchor += size_t(log10((double)util::anchor)*10);
        return NULL;
    }
};

class simple_root_task : public tbb::task
{
    task* execute () {
        set_ref_count(NUM_leaf_tasks + 1);
        for ( size_t i = 0; i < NUM_leaf_tasks; ++i ) {
            simple_leaf_task &t = *new( allocate_child() ) simple_leaf_task;
            spawn(t);
        }
        wait_for_all();
        return NULL;
    }
};

void Work1 () {
    for ( size_t i=0; i < NUM_leaf_tasks; ++i )
        s_tasks.my_simple_leaf_task_ptr->execute();
}

void Test1_1 () {
    tbb::empty_task &r = *new( tbb::task::allocate_root() ) tbb::empty_task;
    r.set_ref_count(NUM_leaf_tasks + 1);
    for ( size_t i = 0; i < NUM_leaf_tasks; ++i ) {
        simple_leaf_task &t = *new( r.allocate_child() ) simple_leaf_task;
        r.spawn(t);
    }
    r.wait_for_all();
    r.destroy(r);
}

void Test1_2 ()
{
    simple_root_task &r = *new( tbb::task::allocate_root() ) simple_root_task;
    tbb::task::spawn_root_and_wait(r);
}


class children_launcher_task : public tbb::task
{
    task* execute () {
        set_ref_count(NUM_CHILD_TASKS + 1);
        for ( size_t i = 0; i < NUM_CHILD_TASKS; ++i ) {
            simple_leaf_task &t = *new( allocate_child() ) simple_leaf_task;
            spawn(t);
        }
        wait_for_all();
        return NULL;
    }
};

class root_launcher_task : public tbb::task
{
    task* execute () {
        children_launcher_task &r = *new( allocate_root() ) children_launcher_task;
        spawn_root_and_wait(r);
        return NULL;
    }
};

class hierarchy_root_task : public tbb::task
{
    task* execute () {
        tbb::task_list  tl;
        for ( size_t i = 0; i < NUM_ROOT_TASKS; ++i ) {
            root_launcher_task &r = *new( allocate_root() ) root_launcher_task;
            tl.push_back(r);
        }
        spawn_root_and_wait(tl);
        return NULL;
    }
};

void Test1_3 ()
{
    hierarchy_root_task &r = *new( tbb::task::allocate_root() ) hierarchy_root_task;
    tbb::task::spawn_root_and_wait(r);
}


static size_t   s_range = N,
                s_grain = 1;

class simple_pfor_body {
public:
    void operator()( const range_type& r ) const {
        count_type end = r.end();
        for( count_type i = r.begin(); i < end; ++i )
            util::anchor += i;
    }
};

void Work2 () {
    simple_pfor_body body;
    range_type range(0, s_range, s_grain);
    body(range);
}

void Test2 () {
    tbb::parallel_for( range_type(0, s_range, s_grain), simple_pfor_body() );
}

void Test2_0 () {
    volatile count_type zero = 0;
    tbb::parallel_for( range_type(0, zero, 1), simple_pfor_body() );
}


class simple_preduce_body {
public:
    count_type my_sum;
    simple_preduce_body () : my_sum(0) {}
    simple_preduce_body ( simple_preduce_body&, tbb::split ) : my_sum(0) {}
    void join( simple_preduce_body& rhs ) { my_sum += rhs.my_sum;}
    void operator()( const range_type& r ) {
        count_type end = r.end();
        for( count_type i = r.begin(); i < end; ++i )
            util::anchor += i;
        my_sum = util::anchor;
    }
};

void Work3 () {
    simple_preduce_body body;
    range_type range(0, s_range, s_grain);
    body(range);
}

void Test3 () {
    simple_preduce_body body;
    tbb::parallel_reduce( range_type(0, s_range, s_grain), body );
}

void Test3_0 () {
    volatile count_type zero = 0;
    simple_preduce_body body;
    tbb::parallel_reduce( range_type(0, zero, 1), body );
}


static_task_holder::static_task_holder () {
    static simple_leaf_task s_t1;
    my_simple_leaf_task_ptr = &s_t1;
}

void Test () {
    const size_t num_task_tree_workloads = 4;
    size_t task_tree_workloads[num_task_tree_workloads] = {0, 50, 500, 10000};
    for (size_t i = 0; i < num_task_tree_workloads; ++i ) {
        size_t n = task_tree_workloads[i];
        s_num_iterations = n;
        CalcSequentialTime(Work1);
        RunTest ("Bunch of leaves: %d adds/task", n, Test1_1);
        RunTest ("Simple task tree: %d adds/task", n, Test1_2);
        RunTest ("Complex task tree: %d adds/task", n, Test1_3);
    }

    // Using N_fine constant in the body of this function results in incorrect code
    // generation by icl 10.1.014
    const size_t num_alg_workloads = 4;
    size_t alg_ranges[num_alg_workloads] = {N_fine/10, N_fine, N, N};
    size_t alg_grains[num_alg_workloads] = {1, FINE_GRAIN, MED_GRAIN, COARSE_GRAIN};
    
    //RunTest ("Empty pfor", 0, Test2_0);
    for (size_t i = 0; i < num_alg_workloads; ++i ) {
        s_range = alg_ranges[i];
        s_grain = alg_grains[i];
        CalcSequentialTime(Work2);
        RunTest ("pfor: %d adds/body", s_grain, Test2);
    }

    //RunTest ("Empty preduce", Test3_0);
    for (size_t i = 0; i < num_alg_workloads; ++i ) {
        s_range = alg_ranges[i];
        s_grain = alg_grains[i];
        CalcSequentialTime(Work3);
        RunTest ("preduce: %d adds/body", s_grain, Test3);
    }
}

int main( int argc, char* argv[] ) {
    test_main(argc, argv);
    return 0;
}
