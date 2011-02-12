// $Id: THYBRID_Concurrency.cpp 91813 2010-09-17 07:52:52Z johnnyw $

#include "ace/OS_NS_stdlib.h"
#include "ace/Message_Block.h"

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif

#include "jaws3/Concurrency.h"
#include "jaws3/THYBRID_Concurrency.h"
#include "jaws3/Protocol_Handler.h"
#include "jaws3/Options.h"


JAWS_THYBRID_Concurrency::JAWS_THYBRID_Concurrency (void)
  : getting_ (0)
  , min_number_of_threads_ (1)
  , max_number_of_threads_ (-1)
  , shutdown_task_ (0)
  , error_ (0)
{
  const char *value;

  value = JAWS_Options::instance ()->getenv ("JAWS_MIN_THYBRID_THREADS");
  if (value != 0)
    this->min_number_of_threads_ = ACE_OS::atoi (value);
  else
    this->min_number_of_threads_ =
      ACE_OS::atoi (JAWS_DEFAULT_MIN_THYBRID_THREADS);

  if (this->min_number_of_threads_ <= 0)
    this->min_number_of_threads_ = 1;

  value = JAWS_Options::instance ()->getenv ("JAWS_MAX_THYBRID_THREADS");
  if (value != 0)
    this->max_number_of_threads_ = ACE_OS::atoi (value);
  else
    this->max_number_of_threads_ =
      ACE_OS::atoi (JAWS_DEFAULT_MAX_THYBRID_THREADS);

  if (this->max_number_of_threads_ <= 0)
    this->max_number_of_threads_ = -1;
  else if (this->max_number_of_threads_ < this->min_number_of_threads_)
    this->max_number_of_threads_ = this->min_number_of_threads_;

  int r;
  r = this->activate (THR_BOUND | THR_JOINABLE, this->min_number_of_threads_);
  if (r < 0)
    {
      this->shutdown_task_ = 1;
      this->error_ = 1;
    }
}

int
JAWS_THYBRID_Concurrency::putq (JAWS_Protocol_Handler *ph)
{
  if (this->error_)
    return -1;

  JAWS_CONCURRENCY_TASK *task = this;
  int result = task->putq (& ph->mb_);

  if (result != -1)
    {
      if (this->getting_ < this->min_number_of_threads_
          && (this->max_number_of_threads_ < 0
              || this->thr_count () < (size_t) this->max_number_of_threads_))
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
    }

  return result;
}

int
JAWS_THYBRID_Concurrency::getq (JAWS_Protocol_Handler *&ph)
{
  ph = 0;

  JAWS_CONCURRENCY_TASK *task = this;

  if (this->shutdown_task_ && task->msg_queue ()->message_count () == 0)
    return -1;

  int getting = ++(this->getting_);

  if (getting > this->min_number_of_threads_)
    {
      if (task->msg_queue ()->message_count () == 0)
        {
          --(this->getting_);
          return -1;
        }
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
          if (this->getting_ > 1)
            {
              task->putq (mb);
              result = -1;
            }

        }
    }

  --(this->getting_);
  return result;
}
