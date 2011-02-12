// $Id: Concurrency.cpp 91813 2010-09-17 07:52:52Z johnnyw $

#include "ace/OS_NS_strings.h"
#include "ace/Message_Block.h"
#include "ace/Singleton.h"
#include "ace/Synch_Traits.h"
#include "ace/Null_Mutex.h"

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif

#include "jaws3/Concurrency.h"
#include "jaws3/TPOOL_Concurrency.h"
#include "jaws3/TPR_Concurrency.h"
#include "jaws3/THYBRID_Concurrency.h"
#include "jaws3/Options.h"


typedef ACE_Singleton<ACE_Message_Block, ACE_SYNCH_NULL_MUTEX>
        JAWS_Empty_Message_Block;


int
JAWS_Concurrency_Impl::svc (void)
{
  JAWS_Protocol_Handler *ph;

  while (this->getq (ph) != -1)
    {
      if (ph == 0)
        continue;

      if (ph->service () == -1)
        {
          // TODO: Do I really want to call close here, or
          //       let the protocol handler close itself?
          ph->dismiss ();
          continue;
        }
    }

  return 0;
}


JAWS_Concurrency_Bridge<JAWS_Concurrency_Impl>
::JAWS_Concurrency_Bridge (JAWS_Concurrency_Impl *impl)
  : impl_ (impl)
{
  if (this->impl_ == 0)
    {
      const char *concurrency;
      concurrency = JAWS_Options::instance ()->getenv ("JAWS_CONCURRENCY");
      if (concurrency == 0)
        concurrency = JAWS_DEFAULT_CONCURRENCY;

      if (ACE_OS::strcasecmp (concurrency, "TPR") == 0)
        this->impl_ = JAWS_TPR_Concurrency::instance ();
      else if (ACE_OS::strcasecmp (concurrency, "TPOOL") == 0)
        this->impl_ = JAWS_TPOOL_Concurrency::instance ();
      else if (ACE_OS::strcasecmp (concurrency, "THYBRID") == 0)
        this->impl_ = JAWS_THYBRID_Concurrency::instance ();
      else
        this->impl_ = JAWS_THYBRID_Concurrency::instance ();
        // Since synchronous IO is the default IO, need an aggressive
        // default concurrency mechanism.
    }
}


int
JAWS_Concurrency_Bridge<JAWS_Concurrency_Impl>
::putq (JAWS_Protocol_Handler *ph)
{
  return this->impl_->putq (ph);
}


int
JAWS_Concurrency_Bridge<JAWS_Concurrency_Impl>
::getq (JAWS_Protocol_Handler *&ph)
{
  return this->impl_->getq (ph);
}


void
JAWS_Concurrency_Bridge<JAWS_Concurrency_Impl>::shutdown (void)
{
  ACE_Message_Block *empty_mb = JAWS_Empty_Message_Block::instance ();
  JAWS_CONCURRENCY_TASK *task;

  task = JAWS_THYBRID_Concurrency::instance ();
  task->putq (empty_mb);
  task->wait ();

  task = JAWS_TPOOL_Concurrency::instance ();
  task->putq (empty_mb);
  task->wait ();

  task = JAWS_TPR_Concurrency::instance ();
  task->putq (empty_mb);
  task->wait ();
}
