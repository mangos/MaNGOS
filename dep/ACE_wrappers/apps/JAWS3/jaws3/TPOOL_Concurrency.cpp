// $Id: TPOOL_Concurrency.cpp 91813 2010-09-17 07:52:52Z johnnyw $

#include "ace/OS_NS_stdlib.h"
#include "ace/Message_Block.h"

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif

#include "jaws3/TPOOL_Concurrency.h"
#include "jaws3/Protocol_Handler.h"
#include "jaws3/Options.h"

JAWS_TPOOL_Concurrency::JAWS_TPOOL_Concurrency (void)
  : number_of_threads_ (5)
  , shutdown_task_ (0)
  , error_ (0)
{
  const char *value = JAWS_Options::instance ()->getenv ("JAWS_TPOOL_THREADS");
  if (value != 0)
    this->number_of_threads_ = ACE_OS::atoi (value);
  else
    this->number_of_threads_ = ACE_OS::atoi (JAWS_DEFAULT_TPOOL_THREADS);

  if (this->number_of_threads_ <= 0)
    this->number_of_threads_ = 5;

  int r;
  r = this->activate ( THR_BOUND | THR_JOINABLE, this->number_of_threads_);

  if (r < 0)
    {
      // ACE_ERROR
      this->error_ = 1;
      this->shutdown_task_ = 1;
    }
}

int
JAWS_TPOOL_Concurrency::putq (JAWS_Protocol_Handler *ph)
{
  if (this->error_)
    return -1;

  JAWS_CONCURRENCY_TASK *task = this;
  return task->putq (& ph->mb_);
}

int
JAWS_TPOOL_Concurrency::getq (JAWS_Protocol_Handler *&ph)
{
  ph = 0;

  JAWS_CONCURRENCY_TASK *task = this;

  if (this->shutdown_task_ && task->msg_queue ()->message_count () == 0)
    return -1;

  ACE_Message_Block *mb = 0;

  int result = task->getq (mb);

  if (result != -1)
    {
      ph = (JAWS_Protocol_Handler *) mb->base ();

      if (ph == 0)
        {
          // Shutdown this task;
          this->shutdown_task_ = 1;
          if (this->number_of_threads_ && this->number_of_threads_-- > 1)
            {
              task->putq (mb);
              result = -1;
            }
        }
    }

  return result;
}
