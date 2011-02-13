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

// configuration:

// Size of input array
const int INPUT_SIZE = 2000000;
// Specify list of unique percents to test against. Max - 10
#define SOURCE_ARRAY UNIQUE_PERCENT(5); UNIQUE_PERCENT(10); UNIQUE_PERCENT(20); UNIQUE_PERCENT(40)

// enable/disable tests for:
#define BOX1 "TBB"
#define BOX1TEST ValuePerSecond<Uniques<tbb::concurrent_hash_map<int,int> >, 1000000/*ns*/>
#define BOX1HEADER "tbb/concurrent_hash_map.h"

// enable/disable tests for:
#define BOX2 "OLD"
#define BOX2TEST ValuePerSecond<Uniques<tbb::concurrent_hash_map<int,int> >, 1000000/*ns*/>
#define BOX2HEADER "tbb/concurrent_hash_map-5468.h"

#define TBB_USE_THREADING_TOOLS 0
//////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <math.h>
#include "tbb/tbb_stddef.h"
#include <vector>
#include <map>
// needed by hash_maps
#include <stdexcept>
#include <iterator>
#include <algorithm>                 // std::swap
#include <utility>      // Need std::pair
#include <cstring>      // Need std::memset
#include <typeinfo>
#include "tbb/cache_aligned_allocator.h"
#include "tbb/tbb_allocator.h"
#include "tbb/spin_rw_mutex.h"
#include "tbb/aligned_space.h"
#include "tbb/atomic.h"
// for test
#include "tbb/spin_mutex.h"
#include "time_framework.h"


using namespace tbb;
using namespace tbb::internal;

/////////////////////////////////////////////////////////////////////////////////////////
// Input data built for SOURCE_ARRAY settings
int Mixtures = 0;
int Percents[10];
int *Data[10];

// Main test class used to run the timing tests. All overridden methods are called by the framework
template<typename TableType>
struct Uniques : TesterBase {
    typedef typename TableType::accessor accessor;
    typedef typename TableType::const_accessor const_accessor;
    TableType *Table;
    int n_items;

    // Returns name of test mode specified by number
    /*override*/ std::string get_name(int testn) {
        return Format("%d%% uniques", Percents[testn]);
    }

    // Initializes base class with number of test modes
    Uniques() : TesterBase(Mixtures), Table(0) {}
    ~Uniques() { if(Table) delete Table; }
    
    // Informs the class that value and threads number become known
    /*override*/ void init() {
        n_items = value/threads_count;
    }

    // Informs the class that the test mode for specified thread is about to start
    /*override*/ void test_prefix(int testn, int t) {
        barrier->wait();
        if( t ) return;
        if(Table) delete Table;
        Table = new TableType(MaxThread*4);
    }

    // Executes test mode for a given thread. Return value is ignored when used with timing wrappers.
    /*override*/ double test(int testn, int t)
    {
        for(int i = t*n_items, e = (t+1)*n_items; i < e; i++) {
            Table->insert( std::make_pair(Data[testn][i],t) );
        }
        return 0;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////

// Using BOX declarations from configuration
#include "time_sandbox.h"

// Prepares the input data for given unique percent
inline void UNIQUE_PERCENT(int p) {
    Percents[Mixtures] = p;
    Data[Mixtures] = new int[INPUT_SIZE];
    int uniques = INPUT_SIZE/100*p;
    srand(10101);
    for(int i = 0; i < INPUT_SIZE; i++)
        Data[Mixtures][i] = rand()%uniques;
    Mixtures++;
}

int main(int argc, char* argv[]) {
    if(argc>1) Verbose = true;
    //if(argc>2) ExtraVerbose = true;
    MinThread = 1; MaxThread = task_scheduler_init::default_num_threads();
    ParseCommandLine( argc, argv );

    ASSERT(tbb_allocator<int>::allocator_type() == tbb_allocator<int>::scalable, "expecting scalable allocator library to be loaded. Please build it by:\n\t\tmake tbbmalloc");
    SOURCE_ARRAY; // prepare source array

    {
        // Declares test processor
        TEST_PROCESSOR_NAME the_test("time_hash_map_fill"/*, StatisticsCollector::ByThreads*/);
        for( int t=MinThread; t <= MaxThread; t++)
            the_test.factory(INPUT_SIZE, t); // executes the tests specified in BOX-es for given 'value' and threads
        the_test.report.SetTitle("Operations per nanosecond", INPUT_SIZE);
        the_test.report.Print(StatisticsCollector::HTMLFile|StatisticsCollector::ExcelXML); // Write files
    }
    return 0;
}

