// $Id: stats.h 80826 2008-03-04 14:51:23Z wotte $

#include "global.h"

#ifndef _D_Stats
#define _D_Stats

#define THREAD_START 42
#define THREAD_END 43

class Init_Fini_t {
public:
  int type; // 0 is start, 1 is end
  ACE_Time_Value timestamp;
};

class Stats {
public:
  Stats(int);
  void log(int, float, float);
  void i_have_started(int);
  void i_am_done(int);
  void print (char *);
  void output();
private:
  float *throughput_;
  float *latency_;
  Init_Fini_t *init_fini_; // Array (2n deep) to count peak no. of active threads
  int thread_count_;
};
#endif
