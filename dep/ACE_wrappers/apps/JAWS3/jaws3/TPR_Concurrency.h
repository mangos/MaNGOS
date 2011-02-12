/* -*- c++ -*- */
// $Id: TPR_Concurrency.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_TPR_CONCURRENCY_H
#define JAWS_TPR_CONCURRENCY_H

#include "ace/Atomic_Op.h"

#include "jaws3/Concurrency.h"

class JAWS_Export JAWS_TPR_Concurrency : public JAWS_Concurrency_Impl
{
public:

  JAWS_TPR_Concurrency (void);

  int putq (JAWS_Protocol_Handler *ph);

  int getq (JAWS_Protocol_Handler *&ph);

  static JAWS_TPR_Concurrency * instance (void)
  {
    return ACE_Singleton<JAWS_TPR_Concurrency, ACE_SYNCH_MUTEX>::instance ();
  }

private:

  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> getting_;
  int min_number_of_threads_;
  int shutdown_task_;
  int error_;

};

#endif /* JAWS_TPR_CONCURRENCY_H */
