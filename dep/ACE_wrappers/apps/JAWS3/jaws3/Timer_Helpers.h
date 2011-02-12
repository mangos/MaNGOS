/* -*- c+ -*- */
// $Id: Timer_Helpers.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_TIMER_HELPERS_H
#define JAWS_TIMER_HELPERS_H

#include "ace/Singleton.h"
#include "ace/Event_Handler.h"

#include "jaws3/Timer.h"
#include "jaws3/Event_Completer.h"
#include "jaws3/Event_Result.h"

class JAWS_Timer_EH : public ACE_Event_Handler
{
public:

  JAWS_Timer_EH ( JAWS_Event_Completer *completer
                , void *act = 0
                , int interval = 0
                )
    : completer_ (completer)
    , act_ (act)
    , interval_ (interval)
    , expired_ (0)
  {
  }

  int handle_timeout (const ACE_Time_Value &tv, const void *act);
  // Called when timer expires.

  int handle_close (ACE_HANDLE h, ACE_Reactor_Mask m);
  // Called directly when timer is canceled.

private:

  JAWS_Event_Completer *completer_;
  void *act_;
  const int interval_;
  int expired_;

};

#endif /* JAWS_TIMER_HELPERS_H */
