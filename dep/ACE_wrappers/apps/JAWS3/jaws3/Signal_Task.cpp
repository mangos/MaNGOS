// $Id: Signal_Task.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "ace/OS_NS_signal.h"
#include "ace/OS_NS_Thread.h"
#include "ace/Thread.h"

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif

#include "jaws3/Signal_Task.h"
#include "jaws3/Event_Dispatcher.h"
#include "jaws3/THYBRID_Concurrency.h"
#include "jaws3/TPOOL_Concurrency.h"
#include "jaws3/TPR_Concurrency.h"


ACE_THR_FUNC_RETURN
JAWS_Signal_Task_function (void *)
{
  int done = 0;

  while (! done)
    {
      int signo = ACE_OS::sigwait (JAWS_Signal_Task::instance ()->sigset ());

      switch (signo)
        {
        case SIGINT:
        case SIGTERM:

           JAWS_Concurrency::instance ()->shutdown ();
           JAWS_Event_Dispatcher::end_event_loop ();

           done = 1;

          break;
# if !defined (ACE_WIN32)
        case SIGHUP:
          // In the future, re-read jaws.conf and svc.conf,
          // and then reset the JAWS singletons.
          // For now, just ignore it.
          break;

        case SIGPIPE:
#endif // !defined (ACE_WIN32)
        default:
          break;

        }

    }

  return 0;
}


JAWS_Signal_Task::JAWS_Signal_Task (void)
{
  // Set our signal mask.
  this->sigset_.empty_set ();

  this->sigset_.sig_add (SIGINT);
  this->sigset_.sig_add (SIGTERM);
  this->sigset_.sig_add (SIGPIPE);

#if 0
  this->sigset_.fill_set ();
#endif

  ACE_OS::sigprocmask (SIG_BLOCK, this->sigset_, 0);

  int result;
  result = ACE_Thread::spawn ( JAWS_Signal_Task_function
                             , 0
                             , THR_BOUND
                             );
  if (result < 0)
    {
      ACE_ERROR ((LM_ERROR, "%p\n", "ACE_Thread::spawn"));
      return;
    }
}
