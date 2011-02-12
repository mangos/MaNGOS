// $Id: collection.cpp 91671 2010-09-08 18:39:23Z johnnyw $

// ============================================================================
//
// = LIBRARY
//    examples
//
// = FILENAME
//    collection.cpp
//
// = DESCRIPTION
//     Shows how applications can use the ACE_Token_Collection
//     utility.  This example creates three collections and spawns a
//     thread to operate on each.  The threads use the collective
//     acquire, renew, and release features of ACE_Token_Collection.
//
// = AUTHOR
//    Tim Harrison
//
// ============================================================================

#include "ace/Get_Opt.h"
#include "ace/Local_Tokens.h"
#include "ace/Token_Collection.h"
#include "ace/Remote_Tokens.h"
#include "ace/Thread_Manager.h"
#include "ace/Service_Config.h"



#if defined (ACE_HAS_THREADS) && defined (ACE_HAS_THREADS_LIBRARY)

static const char *server_host = ACE_DEFAULT_SERVER_HOST;
static int server_port = ACE_DEFAULT_SERVER_PORT;
// unused:  static int threads = 2;
static int iterations = 50;
static int debug = 0;
static int remote = 0;
// unused:  static int tokens = 5;

static void *
run_thread (void *vp)
{
  ACE_Token_Proxy *collection = (ACE_Token_Proxy *) vp;

  int count = iterations;
  while (count--)
    {
      if (collection->acquire () == -1)
        {
          if (ACE_OS::last_error () == EDEADLK)
            {
              ACE_DEBUG ((LM_DEBUG, "deadlock detected in acquire"));
              continue;
            }
          ACE_ERROR ((LM_ERROR, "(%t) %p acquire failed\n","run_thread"));
          return (void *) -1;
        }

      ACE_DEBUG ((LM_DEBUG, "(%t) %s acquired.\n", collection->name ()));

      if (collection->renew () == -1)
        {
          if (ACE_OS::last_error () == EDEADLK)
            {
              ACE_DEBUG ((LM_DEBUG, "deadlock detected"));
              goto deadlock;
            }
          ACE_ERROR ((LM_ERROR, "(%t) %p renew failed\n","run_thread"));
          return (void *) -1;
        }

      ACE_DEBUG ((LM_DEBUG, "(%t) %s renewed.\n", collection->name ()));

      deadlock:
        if (collection->release () == -1)
          {
            ACE_ERROR ((LM_ERROR, "(%t) %p release failed\n","run_thread"));
            return (void *) -1;
          }

      ACE_DEBUG ((LM_DEBUG, "(%t) %s released.\n", collection->name ()));
    }

  ACE_DEBUG ((LM_DEBUG, "(%t) thread exiting.\n"));
  return 0;
}

static int
parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_LOG_MSG->open (argv[0], ACE_Log_Msg::STDERR); // | ACE_Log_Msg::VERBOSE);

  ACE_Get_Opt get_opt (argc, argv, ACE_TEXT("un:dp:h:"), 1);

  for (int c; (c = get_opt ()) != -1; )
    {
      switch (c)
        {
        case 'h':  // specify the host machine on which the server is running
          server_host = get_opt.opt_arg ();
          remote = 1;
          break;
        case 'p':  // specify the port on which the server is running
          server_port = ACE_OS::atoi (get_opt.opt_arg ());
          remote = 1;
          break;
        case 'd':
          debug = 1;
          break;
        case 'n':
          iterations = ACE_OS::atoi (get_opt.opt_arg ());
          break;
        case 'u':
        // usage: fallthrough
        default:
          ACE_ERROR_RETURN ((LM_ERROR,
                             "%n:\n"
                             "[-h <remote host>]\n"
                             "[-p <remote port>]\n"
                             "[-n <iterations>]\n"
                             "[-d debug]\n", 1), -1);
          /* NOTREACHED */
        }
    }

  return 0;
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  if (parse_args (argc, argv) == -1)
    return -1;

  ACE_Token_Proxy *A;  // Mutex *A*.
  ACE_Token_Proxy *B;  // Mutex *B*.
  ACE_Token_Proxy *R;  // *R*eader Lock.
  ACE_Token_Proxy *W;  // *W*riter Lock.

  // Depending on the command line arguments, we will create local or
  // remote tokens.  The names of the tokens are not important as long
  // as they are unique.
  if (remote)
    {
      ACE_Remote_Mutex::set_server_address (ACE_INET_Addr (server_port, server_host));
      A = new ACE_Remote_Mutex ("R Mutex A", 0, debug);
      B = new ACE_Remote_Mutex ("R Mutex B", 0, debug);
      R = new ACE_Remote_RLock ("R Reader Lock", 0, debug);
      W = new ACE_Remote_WLock ("R Writer Lock", 0, debug);
    }
  else
    {
      A = new ACE_Local_Mutex ("L Mutex A", 0, debug);
      B = new ACE_Local_Mutex ("L Mutex B", 0, debug);
      R = new ACE_Local_RLock ("L Reader Lock", 0, debug);
      W = new ACE_Local_WLock ("L Writer Lock", 0, debug);
    }

  // These collections will be treated as Tokens by the threads.
  ACE_Token_Collection collectionAR (debug, "A and Reader");
  ACE_Token_Collection collectionAW (debug, "A and Writer");
  ACE_Token_Collection collectionBR (debug, "B and Reader");

  // AR and BR can run concurrently.  Neither AR or BR can run when AW
  // is running.
  collectionAR.insert (*A);
  collectionAR.insert (*R);

  collectionAW.insert (*A);
  collectionAW.insert (*W);

  collectionBR.insert (*B);
  collectionBR.insert (*R);

  // Spawn off three threads.
  ACE_Thread_Manager *mgr = ACE_Thread_Manager::instance ();

  if (mgr->spawn (ACE_THR_FUNC (run_thread),
                  (void *) &collectionAR, THR_BOUND | THR_SUSPENDED) == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "spawn 1 failed"), -1);

  if (mgr->spawn (ACE_THR_FUNC (run_thread),
                  (void *) &collectionAW, THR_BOUND | THR_SUSPENDED) == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "spawn 2 failed"), -1);

  if (mgr->spawn (ACE_THR_FUNC (run_thread),
                  (void *) &collectionBR, THR_BOUND | THR_SUSPENDED) == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "spawn 3 failed"), -1);

#if ! defined (ACE_HAS_PTHREADS)
  if (mgr->resume_all () == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "resume failed"), -1);
#endif

  // Wait for all threads to exit.
  mgr->wait ();

  return 0;
}

#else
int
ACE_TMAIN(int, ACE_TCHAR *[])
{
  ACE_ERROR_RETURN ((LM_ERROR,
                     "threads not supported on this platform\n"), -1);
}
#endif /* ACE_HAS_THREADS && ACE_HAS_TOKENS_LIBRARY */
