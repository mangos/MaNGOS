/* -*- c++ -*- */
// $Id: Task_Timer.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_TASK_TIMER_H
#define JAWS_TASK_TIMER_H

#include "ace/Singleton.h"
#include "ace/Timer_Wheel.h"
#include "ace/Timer_Queue_Adapters.h"

#include "jaws3/Export.h"
#include "jaws3/Timer.h"

class JAWS_Task_Timer;

class JAWS_Export JAWS_Task_Timer : public JAWS_Timer_Impl
{
public:

  JAWS_Task_Timer (void);

  static JAWS_Timer_Impl * instance (void)
  {
    return ACE_Singleton<JAWS_Task_Timer, ACE_SYNCH_MUTEX>::instance ();
  }

  void schedule_timer ( long *timer_id
                      , const ACE_Time_Value &delta
                      , JAWS_Event_Completer *completer
                      , void *act = 0
                      );

  void schedule_absolute_timer ( long *timer_id
                               , const ACE_Time_Value &tv
                               , JAWS_Event_Completer *completer
                               , void *act = 0
                               );

  void schedule_interval_timer ( long *timer_id
                               , const ACE_Time_Value &interval
                               , JAWS_Event_Completer *completer
                               , void *act = 0
                               );

  void cancel_timer (long timer_id);
  // NOTE: Cancelling the timer causes handle_close to be called, but
  // not handle_timeout (of couse, I mean in the ACE_Event_Handler that
  // is being used as the timer helper).

private:

  ACE_Thread_Timer_Queue_Adapter<ACE_Timer_Wheel> timer_queue_;

};

#endif /* JAWS_TASK_TIMER_H */
