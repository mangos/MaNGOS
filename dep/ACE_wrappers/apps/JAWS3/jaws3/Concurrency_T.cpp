// $Id: Concurrency_T.cpp 91813 2010-09-17 07:52:52Z johnnyw $

#include "ace/Message_Block.h"
#include "ace/Singleton.h"
#include "ace/Synch_Traits.h"
#include "ace/Null_Mutex.h"

#include "jaws3/Concurrency_T.h"

typedef ACE_Singleton<ACE_Message_Block, ACE_SYNCH_NULL_MUTEX>
        JAWS_Empty_Message_Block;


template <class CONCURRENCY_IMPL>
JAWS_Concurrency_Bridge<CONCURRENCY_IMPL>
::JAWS_Concurrency_Bridge (CONCURRENCY_IMPL *impl)
  : impl_ (impl)
{
  if (this->impl_ == 0)
    this->impl_ = CONCURRENCY_IMPL::instance ();
}


template <class CONCURRENCY_IMPL> int
JAWS_Concurrency_Bridge<CONCURRENCY_IMPL>::putq (JAWS_Protocol_Handler *ph)
{
  return this->impl_->putq (ph);
}


template <class CONCURRENCY_IMPL> int
JAWS_Concurrency_Bridge<CONCURRENCY_IMPL>::getq (JAWS_Protocol_Handler *&ph)
{
  return this->impl_->getq (ph);
}


template <class CONCURRENCY_IMPL> void
JAWS_Concurrency_Bridge<CONCURRENCY_IMPL>::shutdown (void)
{
  ACE_Message_Block *empty_mb = JAWS_Empty_Message_Block::instance ();
  JAWS_CONCURRENCY_TASK *task;

  task = this->impl_;
  task->putq (empty_mb);
  task->wait ();
}
