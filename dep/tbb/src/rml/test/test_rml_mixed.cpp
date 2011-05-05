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

#include "rml_tbb.h"
#include "rml_omp.h"
#include "tbb/atomic.h"
#include "tbb/tick_count.h"
#include "harness.h"

const int OMP_ParallelRegionSize = 16;
int TBB_MaxThread = 4;           // Includes master 
int OMP_MaxThread = int(~0u>>1); // Includes master

template<typename Client>
class ClientBase: public Client {
protected:
    typedef typename Client::version_type version_type;
    typedef typename Client::job job;
    typedef typename Client::policy_type policy_type;

private:
    /*override*/version_type version() const {
        return 0;
    }
    /*override*/size_t min_stack_size() const {
        return 1<<20;
    }
    /*override*/job* create_one_job() {
        return new rml::job;
    }
    /*override*/policy_type policy() const {
        return Client::turnaround;
    }
    /*override*/void acknowledge_close_connection() {
        delete this;
    }
    /*override*/void cleanup( job& j ) {delete &j;}
};

//! Represents a TBB or OpenMP run-time that uses RML.
template<typename Factory, typename Client>
class RunTime {
public:
    //! Factory that run-time uses to make servers.
    Factory factory;
    Client* client;
    typename Factory::server_type* server;
    RunTime() {
        factory.open();
    }
    ~RunTime() {
        factory.close();
    }
    //! Create server for this run-time
    void create_connection();

    //! Destroy server for this run-time
    void destroy_connection();
};

class ThreadLevelRecorder {
    tbb::atomic<int> level;
    struct record {
        tbb::tick_count time;
        int nthread;
    };
    tbb::atomic<unsigned> next;
    /** Must be power of two */
    static const unsigned max_record_count = 1<<20;
    record array[max_record_count];
public:
    void change_level( int delta );
    void dump();
};

void ThreadLevelRecorder::change_level( int delta ) {
    int x = level+=delta;
    tbb::tick_count t = tbb::tick_count::now();
    unsigned k = next++;
    if( k<max_record_count ) {
        record& r = array[k];
        r.time = t;
        r.nthread = x;
    } 
}

void ThreadLevelRecorder::dump() {
    FILE* f = fopen("time.txt","w");
    if( !f ) {
        perror("fopen(time.txt)\n");
        exit(1);
    }
    unsigned limit = next;
    if( limit>max_record_count ) {
        // Clip
        limit = next;
    }
    for( unsigned i=0; i<limit; ++i ) {
        fprintf(f,"%f\t%d\n",(array[i].time-array[0].time).seconds(),array[i].nthread);
    }
    fclose(f);
}

ThreadLevelRecorder TotalThreadLevel;

class TBB_Client: public ClientBase<tbb::internal::rml::tbb_client> {
    /*override*/void process( job& j );
    /*override*/size_type max_job_count() const {
        return TBB_MaxThread-1;
    }
};

class OMP_Client: public ClientBase<__kmp::rml::omp_client> {
    /*override*/void process( job&, void* cookie, omp_client::size_type );
    /*override*/size_type max_job_count() const {
        return OMP_MaxThread-1;
    }
};

RunTime<tbb::internal::rml::tbb_factory, TBB_Client> TBB_RunTime;
RunTime<__kmp::rml::omp_factory, OMP_Client> OMP_RunTime;

template<typename Factory, typename Client>
void RunTime<Factory,Client>::create_connection() {
    client = new Client;
    typename Factory::status_type status = factory.make_server( server, *client );
    ASSERT( status==Factory::st_success, NULL );
}

template<typename Factory, typename Client>
void RunTime<Factory,Client>::destroy_connection() {
    server->request_close_connection();
    server = NULL;
}

class OMP_Team {
public:
    OMP_Team( __kmp::rml::omp_server& ) {}
    tbb::atomic<unsigned> barrier;
};

tbb::atomic<int> AvailWork;
tbb::atomic<int> CompletionCount;
 
void OMPWork() {
    tbb::atomic<int> x;
    for( x=0; x<2000000; ++x ) {
        continue;
    }
}

void TBBWork() {
    if( AvailWork>=0 ) {
        int k = --AvailWork;
        if( k==-1 ) {
            TBB_RunTime.server->adjust_job_count_estimate(-(TBB_MaxThread-1));
            ++CompletionCount;
        } else if( k>=0 ) {
            for( int k=0; k<4; ++k ) {
                OMP_Team team( *OMP_RunTime.server );
                int n = OMP_RunTime.server->try_increase_load( OMP_ParallelRegionSize-1, /*strict=*/false );
                team.barrier = 0;
                ::rml::job* array[OMP_ParallelRegionSize-1];
                if( n>0)
                    OMP_RunTime.server->get_threads( n, &team, array );
                // Master does work inside parallel region too.
                OMPWork();
                // Master waits for workers to finish
                if( n>0 )
                    while( team.barrier!=unsigned(n) ) {
                        __TBB_Yield();
                    } 
            }
            ++CompletionCount;
        }
    }
}

/*override*/void TBB_Client::process( job& ) {
    TotalThreadLevel.change_level(1);
    TBBWork();
    TotalThreadLevel.change_level(-1);
}  

/*override*/void OMP_Client::process( job& /* j */, void* cookie, omp_client::size_type ) {
    TotalThreadLevel.change_level(1);
    ASSERT( OMP_RunTime.server, NULL );
    OMPWork();
    ASSERT( OMP_RunTime.server, NULL );
    static_cast<OMP_Team*>(cookie)->barrier+=1;
    TotalThreadLevel.change_level(-1);
}

void TBBOutSideOpenMPInside() {
    TotalThreadLevel.change_level(1);
    CompletionCount = 0;
    int tbbtasks = 32;
    AvailWork = tbbtasks;
    TBB_RunTime.server->adjust_job_count_estimate(TBB_MaxThread-1);
    while( CompletionCount!=tbbtasks+1 ) {
        TBBWork();
    }
    TotalThreadLevel.change_level(-1);
}  

int main( int argc, char* argv[] ) {
    // Set defaults
    MinThread = 4;
    MaxThread = 4;
    ParseCommandLine(argc,argv);
    for( int TBB_MaxThread=MinThread; TBB_MaxThread<=MaxThread; ++TBB_MaxThread ) {
        if( Verbose ) printf("Testing with TBB_MaxThread=%d\n", TBB_MaxThread);
        TBB_RunTime.create_connection();
        OMP_RunTime.create_connection();
        TBBOutSideOpenMPInside();
        OMP_RunTime.destroy_connection();
        TBB_RunTime.destroy_connection();
    }
    TotalThreadLevel.dump();
    printf("done\n");
    return 0;
}
