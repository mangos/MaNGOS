// $Id: Timer.cpp 91813 2010-09-17 07:52:52Z johnnyw $

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif

#include "jaws3/Timer.h"
#include "jaws3/Task_Timer.h"

JAWS_Timer::JAWS_Timer (JAWS_Timer_Impl *impl)
  : impl_ (impl)
{
  // TODO: Change this to use JAWS_Options after we have more than
  // one way of handling timers.
  if (this->impl_ == 0)
    this->impl_ = JAWS_Task_Timer::instance ();
}

void
JAWS_Timer::schedule_timer ( long *timer_id
                           , const ACE_Time_Value &delta
                           , JAWS_Event_Completer *completer
                           , void *act
                           )
{
  this->impl_->schedule_timer (timer_id, delta, completer, act);
}

void
JAWS_Timer::schedule_absolute_timer ( long *timer_id
                                    , const ACE_Time_Value &tv
                                    , JAWS_Event_Completer *completer
                                    , void *act
                                    )
{
  this->impl_->schedule_absolute_timer (timer_id, tv, completer, act);
}

void
JAWS_Timer::schedule_interval_timer ( long *timer_id
                                    , const ACE_Time_Value &interval
                                    , JAWS_Event_Completer *completer
                                    , void *act
                                    )
{
  this->impl_->schedule_interval_timer (timer_id, interval, completer, act);
}

void
JAWS_Timer::cancel_timer (long timer_id)
{
  this->impl_->cancel_timer (timer_id);
}
