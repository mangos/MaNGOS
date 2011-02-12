// $Id: Event_Dispatcher.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "ace/Log_Msg.h"
#include "ace/Thread.h"
#include "ace/Reactor.h"
#include "ace/Proactor.h"
#include "ace/POSIX_Proactor.h"

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif /*JAWS_BUILD_DLL*/

#include "jaws3/Event_Dispatcher.h"

static ACE_THR_FUNC_RETURN
JAWS_Event_Dispatcher_Proactor_Event_Loop (void *)
{
  ACE_Proactor::run_event_loop ();
  return 0;
}


void
JAWS_Event_Dispatcher::end_event_loop (void)
{
  ACE_Reactor::end_event_loop ();
}


void
JAWS_Event_Dispatcher::run_event_loop (void)
{
  // First, initiate the proactor thread

  int result;
  result = ACE_Thread::spawn ( JAWS_Event_Dispatcher_Proactor_Event_Loop
                             , 0
                             , THR_BOUND
                             );
  if (result < 0)
    {
      ACE_ERROR ((LM_ERROR, "%p\n", "ACE_Thread::spawn"));
      return;
    }

  // Now, enter the reactor's event loop.
  ACE_Reactor::run_event_loop ();

  // End the proactor's event loop if reactor was interrupted.
  ACE_Proactor::end_event_loop ();
}

