// $Id: main.cpp 91673 2010-09-08 18:49:47Z johnnyw $

// Test the client-side of the ACE Name Server...

#include "ace/Service_Config.h"
#include "ace/Naming_Context.h"
#include "ace/ARGV.h"
#include "ace/Log_Msg.h"
#include "ace/Reactor.h"

#include "Client_Test.h"

int
ACE_TMAIN (int, ACE_TCHAR *argv[])
{
  ACE_Service_Config daemon;
  ACE_ARGV new_args;

  // Load the existing <argv> into our new one.
  new_args.add (argv);
  // Enable loading of static services.
  new_args.add (ACE_TEXT ("-y"));
  // Enable debugging within dynamically linked services.
  new_args.add (ACE_TEXT ("-d"));

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("argc = %d\n"),
              new_args.argc ()));

  // Print the contents of the combined <ACE_ARGV>.
  for (int i = 0; i < new_args.argc (); i++)
    ACE_DEBUG ((LM_DEBUG,
                ACE_TEXT ("(%d) %s\n"),
                i,
                new_args.argv ()[i]));

  if (daemon.open (new_args.argc (),
                   new_args.argv ()) == -1)
    {
      if (errno != ENOENT)
        ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT ("%p\n"), ACE_TEXT ("open")),
                          1);
      else // Use static binding.
        {
          ACE_ARGV args;

          args.add (argv[0]);
          args.add (ACE_TEXT ("-p10011")); // Port number.
          ACE_Service_Object *so =
            ACE_SVC_INVOKE (ACE_Naming_Context);

          if (so->init (args.argc (),
                        args.argv ()) == -1)
            ACE_ERROR_RETURN ((LM_ERROR,
                               ACE_TEXT ("%p\n"),
                               ACE_TEXT ("ACE_Naming_Context")),
                              1);
        }
    }

  Client_Test test_body;
  if (test_body.open () == -1)
    return 1;

  // Run forever, performing the configured services until we are shut
  // down by a SIGINT/SIGQUIT signal.

  ACE_Reactor::instance ()->run_reactor_event_loop ();
  test_body.close ();

  return 0;
}
