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
#include "test_server.h"

typedef tbb::internal::rml::tbb_server MyServer;
typedef tbb::internal::rml::tbb_factory MyFactory;

class MyClient: public ClientBase<tbb::internal::rml::tbb_client> {
    tbb::atomic<int> counter;
    /*override*/void process( job& j ) {
        do_process(j);
    }
public:
    MyClient() {counter=1;}
    static const bool is_omp = false;
    bool is_strict() const {return false;}
};

void FireUpJobs( MyServer& server, MyClient& client, int n_thread, int n_extra, Checker* checker ) {
    if( Verbose ) 
        printf("client %d: calling adjust_job_count_estimate(%d)\n", client.client_id(),n_thread); 
    // Exercise independent_thread_number_changed, even for zero values.
    server.independent_thread_number_changed( n_extra );
    // Experiments indicate that when oversubscribing, the main thread should wait a little
    // while for the RML worker threads to do some work. 
    int delay = n_thread>int(server.default_concurrency()) ? 50 : 1;
    if( checker ) {
        // Give RML time to respond to change in number of threads.
        MilliSleep(delay);
        for( int k=0; k<n_thread; ++k )
            client.job_array[k].processing_count = 0;
    }
    server.adjust_job_count_estimate( n_thread );
    int n_used = 0;
    if( checker ) {
        MilliSleep(delay);
        for( int k=0; k<n_thread; ++k )
            if( client.job_array[k].processing_count )
                ++n_used;
    }
    // Logic further below presumes that jobs never starve, so undo previous call
    // to independent_thread_number_changed before waiting on those jobs.
    server.independent_thread_number_changed( -n_extra );
    if( Verbose ) 
        printf("client %d: wait for each job to be processed at least once\n",client.client_id());
    // Calculate the number of jobs that are expected to get threads.
    // Typically this is equal to n_thread.  But if nested, subtract 1 to account for the fact
    // that this thread itself cannot process the job.
    int expected = client.nesting.level==0 ? n_thread : n_thread-1;
    // Wait for expected number of jobs to be processed.
    if( client.nesting.level==0 ) {
        for(;;) {
            int n = 0;
            for( int k=0; k<n_thread; ++k ) 
                if( client.job_array[k].processing_count!=0 ) 
                    ++n;
            if( n>=expected ) break;
            server.yield();
        }
    } else {
        printf("testing of nested tbb execution is yet to be supported\n");
    }
    server.adjust_job_count_estimate(-n_thread);
    if( checker ) 
        checker->check_number_of_threads_delivered( n_used, n_thread, n_extra );
}

void DoClientSpecificVerification( MyServer&, int n_thread )
{
    MyClient* client = new MyClient;
    client->initialize( n_thread, Nesting(), ClientStackSize[0] );
    MyFactory factory;
    memset( &factory, 0, sizeof(factory) );
    MyFactory::status_type status = factory.open();
    ASSERT( status!=MyFactory::st_not_found, "could not find RML library" );
    ASSERT( status!=MyFactory::st_incompatible, NULL );
    ASSERT( status==MyFactory::st_success, NULL );
    MyFactory::server_type* server; 
    status = factory.make_server( server, *client );
    ASSERT( status==MyFactory::st_connection_exists, "Did the first connection get lost?" );
    factory.close();
    client->update(MyClient::destroyed, MyClient::live);
    delete client;
}

int main( int argc, char* argv[] ) {
    // Set defaults
    MinThread = 0;
    MaxThread = 4;
    ParseCommandLine(argc,argv);
    VerifyInitialization<MyFactory,MyClient>( MaxThread );
    SimpleTest<MyFactory,MyClient>();
    printf("done\n");
    return 0;
}
