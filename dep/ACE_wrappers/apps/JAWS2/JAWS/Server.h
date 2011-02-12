/* -*- c++ -*- */
// $Id: Server.h 82739 2008-09-16 12:20:46Z johnnyw $

#ifndef JAWS_SERVER_H
#define JAWS_SERVER_H

#include "JAWS/Export.h"
#include "JAWS/Data_Block.h"
#include "JAWS/Policy.h"

class JAWS_IO_Handler_Factory;

class JAWS_Export JAWS_Server
{
public:
  JAWS_Server (void);
  JAWS_Server (int argc, char *argv[]);

  void init (int argc, char *argv[]);
  int open (JAWS_Pipeline_Handler *ph, JAWS_Dispatch_Policy *dp = 0);

private:
  void parse_args (int argc, ACE_TCHAR *argv[]);
  // Parse arguments

private:
  int ratio_;           // ratio of asynch ops to threads
  int port_;            // port to listen on
  int concurrency_;     // 0 => pool, 1 => per request
  int dispatch_;        // 0 => synch, 1 => asynch
  int nthreads_;        // number of threads
  int maxthreads_;      // maximum number of threads
  long flags_;          // thread creation flags

  JAWS_Default_Dispatch_Policy policy_;
};


#endif /* !defined (JAWS_SERVER_H) */
