// $Id: TPR_Concurrency.cpp 91813 2010-09-17 07:52:52Z johnnyw $

#include "ace/Message_Block.h"

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif

#include "jaws3/Concurrency.h"
#include "jaws3/TPR_Concurrency.h"
#include "jaws3/Protocol_Handler.h"
#include "jaws3/Options.h"


JAWS_TPR_Concurrency::JAWS_TPR_Concurrency (void)
  : getting_ (0)
  , shutdown_task_ (0)
  , error_ (0)
{
  int r;
  r = this->activate (THR_BOUND | THR_JOINABLE);
  if (r < 0)
    {
      this->error_ = 1;
      this->shutdown_task_ = 1;
    }
}

int
JAWS_TPR_Concurrency::putq (JAWS_Protocol_Handler *ph)
{
  if (this->error_)
    return -1;

  JAWS_CONCURRENCY_TASK *task = this;
  int result = task->putq (& ph->mb_);

  if (result != -1)
    {
      int r;
      r = this->activate ( THR_BOUND | THR_JOINABLE
                         , 1 // number of threads
                         , 1 // force active
                         );
      if (r < 0)
        {
          // ACE_ERROR
          return -1;
        }
    }

  return result;
}

int
JAWS_TPR_Concurrency::getq (JAWS_Protocol_Handler *&ph)
{
  ph = 0;

  JAWS_CONCURRENCY_TASK *task = this;

  if (this->shutdown_task_ && task->msg_queue ()->message_count () == 0)
    return -1;

  int getting = ++(this->getting_);

  if (getting > 1 && task->msg_queue ()->message_count () == 0)
    {
      --(this->getting_);
      return -1;
    }

  ACE_Message_Block *mb = 0;
  int result = task->getq (mb);

  if (result != -1)
    {
      ph = (JAWS_Protocol_Handler *) mb->base ();

      if (ph == 0)
        {
          // Shutdown this task;
          this->shutdown_task_ = 1;
        }
    }

  --(this->getting_);
  return result;
}
