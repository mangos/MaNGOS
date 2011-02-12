// $Id: main.cpp 80826 2008-03-04 14:51:23Z wotte $
#include "ace/Log_Msg.h"
#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"
#include "ace/Service_Config.h"
#include "ace/Thread_Manager.h"

#include "jaws3/Event_Dispatcher.h"
#include "jaws3/Signal_Task.h"

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  ACE_REACTOR_INSTANCE_INIT;

  JAWS_Signal_Task::instance ();

  if (ACE_Service_Config::open (argc, argv) == -1
      && errno != ENOENT)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("open")),
                      1);

  // Run forever, performing the configured services until we
  // shutdown.

  JAWS_Event_Dispatcher::run_event_loop ();

  ACE_Thread_Manager::instance ()->wait ();

  return 0;
}
