// $Id: stats.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#include "stats.h"

Stats::Stats(int size) {
  throughput_ = new float[size];
  latency_ = new float[size];
  thread_count_ = size;
  init_fini_ = new Init_Fini_t[2*size];
  for(int i = 0; i < size; i++)
    throughput_[i] = latency_[i] = 0;
}

void Stats::log(int id, float throughput, float latency) {
  throughput_[id] = throughput;
  latency_[id] = latency;
}

// Unused for now.
void Stats::print(char *message) {

  ACE_UNUSED_ARG (message);

  // char time_buf[64];
  // long ltime;
  // time(&ltime);

  // ACE_OS::ctime_r(&ltime, time_buf, sizeof time_buf);

  // if(ACE_OS::gettimeofday() == -1) {
  //   perror("gettimeofday");
  // }
  // time_buf[strlen(time_buf)-1] = 0;
  //   printf("%010ld%09ld \t %s %s\n", tp.tv_sec, tp.tv_usec, time_buf, message);
}


int comp(const void *a, const void *b) {

  Init_Fini_t *A = (Init_Fini_t *)a;
  Init_Fini_t *B = (Init_Fini_t *)b;

  return (A->timestamp < B->timestamp) ? -1 : (A->timestamp > B->timestamp);
}


void Stats::output() {
  int i;
  float tavg = 0, lavg = 0;

  ACE_OS::qsort(init_fini_, 2*thread_count_, sizeof(Init_Fini_t), comp);

  int max = 0,thread_peak = 0;

  for(i = 0; i < 2*thread_count_; i++) {
    //    cerr << " " << ((init_fini_[i].type == THREAD_START) ? "START": "END") << " " << init_fini_[i].timestamp.sec() << "." << init_fini_[i].timestamp.usec() << endl;
    if(init_fini_[i].type == THREAD_START) {
      if(++thread_peak > max)
        max = thread_peak;
    }
    else thread_peak--;
  }
  for(i = 0; i < thread_count_; i++) {
    tavg += throughput_[i];
    lavg += latency_[i];
  }
  cout << " " << tavg/thread_count_ << " " << lavg/thread_count_ << " " << max;
}


void Stats::i_have_started(int id) {

  init_fini_[2*id].type = THREAD_START;
  init_fini_[2*id].timestamp = ACE_OS::gettimeofday();

}

void Stats::i_am_done(int id) {

  init_fini_[(2*id)+1].type = THREAD_END;

  init_fini_[(2*id)+1].timestamp = ACE_OS::gettimeofday();

}


