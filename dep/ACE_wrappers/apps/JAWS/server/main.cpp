// $Id: main.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#include "ace/Service_Config.h"
#include "ace/Reactor.h"
#include "ace/Filecache.h"

#include "HTTP_Server.h"
#include "ace/OS_main.h"
#include "ace/OS_NS_signal.h"

ACE_STATIC_SVC_REQUIRE(HTTP_Server)

#ifdef ACE_HAS_SIG_C_FUNC
extern "C"
{
#endif /* ACE_HAS_SIG_C_FUNC */

  // call exit() so that static destructors get called
static void
handler (int)
{
  delete (ACE_Filecache *) ACE_Filecache::instance ();
  ACE_OS::exit (0);
}

#ifdef ACE_HAS_SIG_C_FUNC
}
#endif /* ACE_HAS_SIG_C_FUNC */

// This is the driver entry point into JAWS.  It is possible to use
// JAWS as an ACE Service, as well.

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  ACE_Service_Config daemon;

  ACE_OS::signal (SIGCHLD, SIG_IGN);

  // SigAction not needed since the handler will shutdown the server.
  ACE_OS::signal (SIGINT, (ACE_SignalHandler) handler);
  ACE_OS::signal (SIGUSR2, (ACE_SignalHandler) handler);

  if (daemon.open (argc, argv, ACE_DEFAULT_LOGGER_KEY, 0) != 0)
    ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "open"), 1);

  // The configured service creates threads, and the
  // server won't exit until the threads die.

  // Run forever, performing the configured services until we receive
  // a SIGINT.


  return 0;
}
