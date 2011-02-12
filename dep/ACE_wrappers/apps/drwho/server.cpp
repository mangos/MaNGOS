// $Id: server.cpp 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    server.cpp
//
// = DESCRIPTION
//   Driver program for the server.  Note that it is easy to reuse the
//   server for other distributed programs.  Pretty much all that must
//   change are the functions registered with the communciations
//   manager.
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#include "Options.h"
#include "SMR_Server.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_signal.h"
#include "ace/OS_NS_time.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_sys_socket.h"

static char *
time_stamp (void)
{
  time_t time_now;
  char *temp;

  time_now = ACE_OS::time (0);
  temp = ACE_OS::asctime (ACE_OS::localtime (&time_now));
  temp[12] = 0;
  return temp;
}

// Catch the obvious signals and die with dignity...

static void
exit_server (int sig)
{
  ACE_DEBUG ((LM_DEBUG,
              "%s exiting on signal %S\n",
              time_stamp (),
              sig));
  ACE_OS::exit (0);
}

// Returns TRUE if the program was started by INETD.

static int
started_by_inetd (void)
{
  sockaddr_in sin;
  int size = sizeof sin;

  return ACE_OS::getsockname (0,
                              reinterpret_cast<sockaddr *> (&sin),
                              &size) == 0;
}

// Does the drwho service.

static void
do_drwho (SMR_Server &smr_server)
{
  if (smr_server.receive () == -1)
    ACE_ERROR ((LM_ERROR,
                "%p\n",
                Options::program_name));

  if (smr_server.send () == -1)
    ACE_ERROR ((LM_ERROR,
                "%p\n",
                Options::program_name));
}

// If the server is started with any argument at all then it doesn't
// fork off a child process to do the work.  This is useful when
// debugging!

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  ACE_OS::signal (SIGTERM, (ACE_SignalHandler)exit_server);
  ACE_OS::signal (SIGINT, (ACE_SignalHandler)exit_server);
  ACE_OS::signal (SIGQUIT, (ACE_SignalHandler)exit_server);

  Options::set_options (argc, argv);
  Options::set_opt (Options::STAND_ALONE_SERVER);

  int inetd_controlled = started_by_inetd ();

  if (!inetd_controlled && Options::get_opt (Options::BE_A_DAEMON))
    ACE::daemonize ();

  SMR_Server smr_server (Options::port_number);

  if (inetd_controlled)
    do_drwho (smr_server);
  else
    {

      for (;;)
        do_drwho (smr_server);

      /* NOTREACHED */
    }

  return 0;
}
