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

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"
#include <cmath>
#include <cstdlib>
#include <cerrno>
#include <cfloat>
#include <vector>
#include <algorithm>

#include "../src/test/harness.h"

#if  __linux__ || __APPLE__ || __FreeBSD__
    #include <sys/resource.h>
#endif /* __APPLE__ */

// The code, performance of which is to be measured, is surrounded by the StartSimpleTiming
// and StopSimpleTiming macros. It is called "target code" or "code of interest" hereafter.
//
// The target code is executed inside the nested loop. Nesting is necessary to allow
// measurements on arrays that fit cache of a particular level, while making the load
// big enough to eliminate the influence of random deviations.
//
// Macro StartSimpleTiming defines reduction variable "util::anchor", which may be modified (usually 
// by adding to) by the target code. This can be necessary to prevent optimizing compilers 
// from throwing out the code of interest. Besides, if the target code is complex enough, 
// make sure that all its branches contribute (directly or indirectly) to the value 
// being added to the "util::anchor" variable.
//
// To factor out overhead introduced by the measurement infra code it is recommended to make 
// a calibration run with target code replaced by a no-op (but still modifying "sum"), and
// store the resulting time in the "util::base" variable.
//
// A generally good approach is to make the target code use elements of a preliminary 
// initialized array. Then for calibration run you just need to add vector elements 
// to the "sum" variable. To get rid of memory access delays make the array small 
// enough to fit L2 or L1 cache (play with StartSimpleTiming arguments if necessary).
//
// Macro CalibrateSimpleTiming performs default calibration using "util::anchor += i;" operation.
//
// Macro ANCHOR_TYPE defines the type of the reduction variable. If it was not 
// defined  before including this header, it is defined as size_t. Depending on 
// the target code modern super scalar architectures may blend reduction operation
// and instructions of interest differently for different target alternatives. So
// you may play with the type to minimize out-of-order and parallel execution impact
// on the calibration time veracity. You may even end up with different reduction 
// variable types (and different calibration times) for different measurements.


namespace util {

typedef std::vector<double>    durations_t;

    void trace_histogram ( const durations_t& t, char* histogramFileName )
    {
        FILE* f = histogramFileName ? fopen(histogramFileName, "wt") : stdout;
        size_t  n = t.size();
        const size_t num_buckets = 100;
        double  min_val = *std::min_element(t.begin(), t.end()),
                max_val = *std::max_element(t.begin(), t.end()),
                bucket_size = (max_val - min_val) / num_buckets;
        std::vector<size_t> hist(num_buckets + 1, 0);
        for ( size_t i = 0; i < n; ++i )
            ++hist[size_t((t[i]-min_val)/bucket_size)];
        fprintf (f, "Histogram: nvals = %u, min = %g, max = %g, nbuckets = %u\n", (unsigned)n, min_val, max_val, (unsigned)num_buckets);
        double bucket = min_val;
        for ( size_t i = 0; i <= num_buckets; ++i, bucket+=bucket_size )
            fprintf (f, "%12g\t%u\n", bucket, (unsigned)hist[i]);
        fclose(f);
    }

    double average ( const durations_t& d, double& variation_percent, double& std_dev_percent )
    {
        durations_t t = d;
        if ( t.size() > 5 ) {
            t.erase(std::min_element(t.begin(), t.end()));
            t.erase(std::max_element(t.begin(), t.end()));
        }
        size_t  n = t.size();
        double  sum = 0,
                min_val = *std::min_element(t.begin(), t.end()),
                max_val = *std::max_element(t.begin(), t.end());
        for ( size_t i = 0; i < n; ++i )
            sum += t[i];
        double  avg = sum / n,
                std_dev = 0;
        for ( size_t i = 0; i < n; ++i ) {
            double    dev = fabs(t[i] - avg);
            std_dev += dev * dev;
        }
        std_dev = sqrt(std_dev / n);
        std_dev_percent = std_dev / avg * 100;
        variation_percent = 100 * (max_val - min_val) / avg;
        return avg;
    }

    static int num_threads;

    static double   base = 0,
                    base_dev = 0,
                    base_dev_percent = 0;

    static char *empty_fmt = "";
    static int rate_field_len = 11;

#if !defined(ANCHOR_TYPE)
    #define ANCHOR_TYPE size_t
#endif

    static ANCHOR_TYPE anchor = 0;
    
    static double sequential_time = 0;


#define StartSimpleTiming(nOuter, nInner) {             \
    tbb::tick_count t1, t0 = tbb::tick_count::now();    \
    for ( size_t j = 0; l < nOuter; ++l ) {             \
        for ( size_t i = 0; i < nInner; ++i ) {

#define StopSimpleTiming(res)                   \
        }                                       \
        util::anchor += (ANCHOR_TYPE)l;         \
    }                                           \
    t1 = tbb::tick_count::now();                \
    printf (util::empty_fmt, util::anchor);     \
    res = (t1-t0).seconds() - util::base;       \
}

#define CalibrateSimpleTiming(T, nOuter, nInner)    \
    StartSimpleTiming(nOuter, nInner);              \
        util::anchor += (ANCHOR_TYPE)i;             \
    StopSimpleTiming(util::base);


#define StartTimingImpl(nRuns, nOuter, nInner)      \
    tbb::tick_count t1, t0;                         \
    for ( size_t k = 0; k < nRuns; ++k )  {         \
        t0 = tbb::tick_count::now();                \
        for ( size_t l = 0; l < nOuter; ++l ) {     \
            for ( size_t i = 0; i < nInner; ++i ) {

#define StartTiming(nRuns, nOuter, nInner) {        \
    util::durations_t  t_(nRuns);                   \
    StartTimingImpl(nRuns, nOuter, nInner)

#define StartTimingEx(vDurations, nRuns, nOuter, nInner) {  \
    util::durations_t  &t_ = vDurations;                    \
    vDurations.resize(nRuns);                               \
    StartTimingImpl(nRuns, nOuter, nInner)

#define StopTiming(Avg, StdDev, StdDevPercent)      \
            }                                       \
            util::anchor += (ANCHOR_TYPE)l;         \
        }                                           \
        t1 = tbb::tick_count::now();                \
        t_[k] = (t1 - t0).seconds()/nrep;           \
    }                                               \
    printf (util::empty_fmt, util::anchor);         \
    Avg = util::average(t_, StdDev, StdDevPercent); \
}

#define CalibrateTiming(nRuns, nOuter, nInner)      \
    StartTiming(nRuns, nOuter, nInner);             \
        util::anchor += (ANCHOR_TYPE)i;             \
    StopTiming(util::base, util::base_dev, util::base_dev_percent);

} // namespace util


#ifndef NRUNS
    #define NRUNS               7
#endif

#ifndef ONE_TEST_DURATION
    #define ONE_TEST_DURATION   0.01
#endif

#define no_histogram  ((char*)-1)

inline 
double RunTestImpl ( const char* title, void (*pfn)(), char* histogramFileName = no_histogram ) {
    double  time = 0, variation = 0, deviation = 0;
    size_t nrep = 1;
    while (true) {
        CalibrateTiming(NRUNS, 1, nrep);
        StartTiming(NRUNS, 1, nrep);
        pfn();
        StopTiming(time, variation, deviation);
        time -= util::base;
        if ( time > 1e-6 )
            break;
        nrep *= 2;
    }
    nrep *= (size_t)ceil(ONE_TEST_DURATION/time);
    CalibrateTiming(NRUNS, 1, nrep);    // sets util::base
    util::durations_t  t;
    StartTimingEx(t, NRUNS, 1, nrep);
        pfn();
    StopTiming(time, variation, deviation);
    if ( histogramFileName != (char*)-1 )
        util::trace_histogram(t, histogramFileName);
    double clean_time = time - util::base;
    if ( title ) {
        // Deviation (in percent) is calulated for the Gross time
        printf ("\n%-34s %.2e  %5.1f      ", title, clean_time, deviation);
        if ( util::sequential_time != 0  )
            //printf ("% .2e  ", clean_time - util::sequential_time);
            printf ("% 10.1f      ", 100*(clean_time - util::sequential_time)/util::sequential_time);
        else
            printf ("%*s ", util::rate_field_len, "");
        printf ("%-9u %1.6f    |", (unsigned)nrep, time * nrep);
    }
    return clean_time;
}


/// Runs the test function, does statistical processing, and, if title is nonzero, prints results.
/** If histogramFileName is a string, the histogram of individual runs is generated and stored
    in a file with the given name. If it is NULL then the histogram is printed on the console.
    By default no histogram is generated. 
    The histogram format is: "rate bucket start" "number of tests in this bucket". **/
inline 
void RunTest ( const char* title_fmt, size_t workload_param, void (*pfn_test)(), char* histogramFileName = no_histogram ) {
    char title[1024];
    sprintf(title, title_fmt, (long)workload_param);
    RunTestImpl(title, pfn_test, histogramFileName);
}

inline 
void CalcSequentialTime ( void (*pfn)() ) {
    util::sequential_time = RunTestImpl(NULL, pfn) / util::num_threads;
}

inline 
void ResetSequentialTime () {
    util::sequential_time = 0;
}


inline void PrintTitle() {
    //printf ("%-32s %-*s Std Dev,%%  %-*s  Repeats   Gross time  Infra time  | NRUNS = %u", 
    //        "Test name", util::rate_field_len, "Rate", util::rate_field_len, "Overhead", NRUNS);
    printf ("%-34s %-*s Std Dev,%%  Par.overhead,%%  Repeats   Gross time  | Nruns %u, Nthreads %d", 
            "Test name", util::rate_field_len, "Rate", NRUNS, util::num_threads);
}

void Test();

inline
int test_main( int argc, char* argv[] ) {
    ParseCommandLine( argc, argv );
    ASSERT (MinThread>=2, "Minimal number of threads must be 2 or more");
    char buf[128];
    util::rate_field_len = 2 + sprintf(buf, "%.1e", 1.1);
    for ( int i = MinThread; i <= MaxThread; ++i ) {
        tbb::task_scheduler_init init (i);
        util::num_threads = i;
        PrintTitle();
        Test();
        printf("\n");
    }
    printf("done\n");
    return 0;
}
