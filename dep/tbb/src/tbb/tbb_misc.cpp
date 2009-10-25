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

// Source file for miscellaneous entities that are infrequently referenced by 
// an executing program.

#include "tbb/tbb_stddef.h"
// Out-of-line TBB assertion handling routines are instantiated here.
#include "tbb_assert_impl.h"

#include "tbb_misc.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(__EXCEPTIONS) || defined(_CPPUNWIND) || defined(__SUNPRO_CC)
    #include "tbb/tbb_exception.h"
    #include <string> // std::string is used to construct runtime_error
    #include <stdexcept>
#endif

using namespace std;

#include "tbb/tbb_machine.h"

namespace tbb {

namespace internal {

#if defined(__EXCEPTIONS) || defined(_CPPUNWIND) || defined(__SUNPRO_CC)
// The above preprocessor symbols are defined by compilers when exception handling is enabled.
// However, in some cases it could be disabled for this file.

void handle_perror( int error_code, const char* what ) {
    char buf[128];
    sprintf(buf,"%s: ",what);
    char* end = strchr(buf,0);
    size_t n = buf+sizeof(buf)-end;
    strncpy( end, strerror( error_code ), n );
    // Ensure that buffer ends in terminator.
    buf[sizeof(buf)-1] = 0; 
    throw runtime_error(buf);
}

void throw_bad_last_alloc_exception_v4() 
{
    throw bad_last_alloc();
}
#endif //__EXCEPTIONS || _CPPUNWIND

bool GetBoolEnvironmentVariable( const char * name ) {
    if( const char* s = getenv(name) )
        return strcmp(s,"0") != 0;
    return false;
}

#include "tbb_version.h"

/** The leading "\0" is here so that applying "strings" to the binary delivers a clean result. */
static const char VersionString[] = "\0" TBB_VERSION_STRINGS;

static bool PrintVersionFlag = false;

void PrintVersion() {
    PrintVersionFlag = true;
    fputs(VersionString+1,stderr);
}

void PrintExtraVersionInfo( const char* category, const char* description ) {
    if( PrintVersionFlag ) 
        fprintf(stderr, "%s: %s\t%s\n", "TBB", category, description );
}

void PrintRMLVersionInfo( void* arg, const char* server_info )
{
    PrintExtraVersionInfo( server_info, (const char *)arg );
}

} // namespace internal
 
extern "C" int TBB_runtime_interface_version() {
    return TBB_INTERFACE_VERSION;
}

} // namespace tbb

#if !__TBB_RML_STATIC
#if __TBB_x86_32

#include "tbb/atomic.h"

// in MSVC environment, int64_t defined in tbb::internal namespace only (see tbb_stddef.h)
#if _MSC_VER
using tbb::internal::int64_t;
#endif

//! Warn about 8-byte store that crosses a cache line.
extern "C" void __TBB_machine_store8_slow_perf_warning( volatile void *ptr ) {
    // Report run-time warning unless we have already recently reported warning for that address.
    const unsigned n = 4;
    static tbb::atomic<void*> cache[n];
    static tbb::atomic<unsigned> k;
    for( unsigned i=0; i<n; ++i ) 
        if( ptr==cache[i] ) 
            goto done;
    cache[(k++)%n] = const_cast<void*>(ptr);
    tbb::internal::runtime_warning( "atomic store on misaligned 8-byte location %p is slow", ptr );
done:;
}

//! Handle 8-byte store that crosses a cache line.
extern "C" void __TBB_machine_store8_slow( volatile void *ptr, int64_t value ) {
    for( tbb::internal::atomic_backoff b;; b.pause() ) {
        int64_t tmp = *(int64_t*)ptr;
        if( __TBB_machine_cmpswp8(ptr,value,tmp)==tmp ) 
            break;
    }
}

#endif /* __TBB_x86_32 */
#endif /* !__TBB_RML_STATIC */

#if __TBB_ipf
extern "C" intptr_t __TBB_machine_lockbyte( volatile unsigned char& flag ) {
    if ( !__TBB_TryLockByte(flag) ) {
        tbb::internal::atomic_backoff b;
        do {
            b.pause();
        } while ( !__TBB_TryLockByte(flag) );
    }
    return 0;
}
#endif
