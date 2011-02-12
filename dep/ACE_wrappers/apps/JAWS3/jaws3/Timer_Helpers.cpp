// $Id: Timer_Helpers.cpp 91813 2010-09-17 07:52:52Z johnnyw $

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif

#include "jaws3/Timer_Helpers.h"

int
JAWS_Timer_EH::handle_timeout (const ACE_Time_Value &, const void *)
{
  JAWS_Event_Result timer_result ( 0
                                 , JAWS_Event_Result::JE_OK
                                 , JAWS_Event_Result::JE_TIMER_OK
                                 );

  if (this->completer_)
    this->completer_->timer_complete (timer_result, this->act_);

  if (this->interval_)
    return 0;

  this->expired_ = 1;
  return -1;
}

int
JAWS_Timer_EH::handle_close (ACE_HANDLE, ACE_Reactor_Mask)
{
  JAWS_Event_Result timer_result ( 0
                                 , JAWS_Event_Result::JE_CANCEL
                                 , JAWS_Event_Result::JE_TIMER_CANCEL
                                 );

  if (! this->expired_ && this->completer_)
    this->completer_->timer_complete (timer_result, this->act_);

  return 0;
}

