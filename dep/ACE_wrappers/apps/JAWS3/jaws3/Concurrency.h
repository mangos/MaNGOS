// -*- C++ -*-
//
// $Id: Concurrency.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_CONCURRENCY_H
#define JAWS_CONCURRENCY_H

#include "ace/Task.h"
#include "ace/Singleton.h"
#include "ace/Synch_Traits.h"

#include "jaws3/Export.h"
#include "jaws3/Protocol_Handler.h"


typedef ACE_Task<ACE_SYNCH> JAWS_CONCURRENCY_TASK;

class JAWS_Export JAWS_Concurrency_Impl : public JAWS_CONCURRENCY_TASK
{
public:

  virtual ~JAWS_Concurrency_Impl (void) {};

  virtual int putq (JAWS_Protocol_Handler *ph) = 0;

  virtual int getq (JAWS_Protocol_Handler *&ph) = 0;

  int svc (void);

};


// Include the templates here.
#include "jaws3/Concurrency_T.h"


template<>
class JAWS_Export JAWS_Concurrency_Bridge<JAWS_Concurrency_Impl>
{
public:

  JAWS_Concurrency_Bridge (JAWS_Concurrency_Impl *impl = 0);

  int putq (JAWS_Protocol_Handler *ph);
  int getq (JAWS_Protocol_Handler *&ph);

  void shutdown (void);

protected:

  JAWS_Concurrency_Impl *impl_;

};


#ifndef JAWS_CONCURRENCY_CONCRETE_IMPL
#define JAWS_CONCURRENCY_CONCRETE_IMPL JAWS_Concurrency_Impl
#endif /* JAWS_CONCURRENCY_CONCRETE_IMPL */


class JAWS_Export JAWS_Concurrency
  : public JAWS_Concurrency_Bridge<JAWS_CONCURRENCY_CONCRETE_IMPL>
{
public:

  static JAWS_Concurrency * instance (void)
  {
    return ACE_Singleton<JAWS_Concurrency, ACE_SYNCH_MUTEX>::instance ();
  }

};


#endif /* JAWS_CONCURRENCY_H */
