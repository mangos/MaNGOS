// $Id: rw_locks.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "ace/Get_Opt.h"
#include "ace/Local_Tokens.h"
#include "ace/Remote_Tokens.h"
#include "ace/Thread_Manager.h"

#if defined (ACE_HAS_THREADS) && defined (ACE_HAS_THREADS_LIBRARY)



static ACE_Token_Proxy *global_rlock;
static ACE_Token_Proxy *global_wlock;

static char *server_host = ACE_DEFAULT_SERVER_HOST;
static int server_port = ACE_DEFAULT_SERVER_PORT;
static int ignore_deadlock = 0;
static int threads = 2;
static int iterations = 50;
static int debug = 0;
static int remote = 0;
static int reads = 4;
static int write_sleep = 0;

static void *
run_thread (void *vp)
{
  for (int x = 0; x < iterations; x++)
    {
      int y = 0;
      for (; y < reads; y++)
        {
          if (global_rlock->acquire () == -1)
          {
            if (ACE_Log_Msg::instance ()->errnum () == EDEADLK)
              {
                ACE_DEBUG ((LM_DEBUG, "rlock deadlock detected\n"));
                goto READ_DEADLOCK;
              }
            else
              return 0;
          }

          ACE_DEBUG ((LM_DEBUG, "(%t) rlock acquired.\n"));
        }

      READ_DEADLOCK:
        for (; y > 0; y--)
          {
            if (global_rlock->release () == 0)
              ACE_DEBUG ((LM_DEBUG, "(%t) r-released.\n"));
          }

        if (global_wlock->acquire () == -1)
          {
            ACE_DEBUG ((LM_DEBUG, "wlock deadlock detected\n"));
          }
        else
          {
            if (write_sleep)
              ACE_OS::sleep (1);
            ACE_DEBUG ((LM_DEBUG, "\t\t(%t) wlock acquired.\n"));
            if (global_wlock->release () == 0)
              ACE_DEBUG ((LM_DEBUG, "\t\t(%t) w-released.\n"));
          }
    }

  ACE_DEBUG ((LM_DEBUG, "(%t) thread exiting.\n"));
  return 0;
}

static int
parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_LOG_MSG->open (argv[0], ACE_Log_Msg::STDERR); // | ACE_Log_Msg::VERBOSE);

  ACE_Get_Opt get_opt (argc, argv, ACE_TEXT("t:iun:drR:sp:h:"), 1);

  for (int c; (c = get_opt ()) != -1; )
    {
      switch (c)
        {
        case 'h':  // specify the host machine on which the server is running
          server_host = get_opt.opt_arg ();
          break;
        case 'p':  // specify the port on which the server is running
          server_port = ACE_OS::atoi (get_opt.opt_arg ());
          break;
        case 't':
          threads = ACE_OS::atoi (get_opt.opt_arg ());
          break;
        case 'R':
          reads = ACE_OS::atoi (get_opt.opt_arg ());
          break;
        case 'd':
          debug = 1;
          break;
        case 'r':
          remote = 1;
          break;
        case 's':
          write_sleep = 1;
          break;
        case 'n':
          iterations = ACE_OS::atoi (get_opt.opt_arg ());
          break;
        case 'i':
          ignore_deadlock = 1;
          break;
        case 'u':
        // usage: fallthrough
        default:
          ACE_ERROR_RETURN ((LM_ERROR,
                             "%n:\n"
                             "[-i ignore deadlock]\n"
                             "[-n <iterations>]\n"
                             "[-R <reads>]\n"
                             "[-r use remote locks]\n"
                             "[-d debug]\n"
                             "[-s sleep during writes]\n"
                             "[-t <threads>\n", 1), -1);
          break;
        }
    }

  return 0;
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  if (parse_args (argc, argv) == -1)
    return -1;

  if (remote)
    {
      ACE_Remote_Mutex::set_server_address (ACE_INET_Addr (server_port, server_host));
      global_rlock = (ACE_Token_Proxy *) new
                        ACE_Remote_RLock ("THE_TOKEN", ignore_deadlock, debug);
      global_wlock = (ACE_Token_Proxy *) new
                        ACE_Remote_WLock ("THE_TOKEN", ignore_deadlock, debug);
    }
  else
    {
      global_rlock = (ACE_Token_Proxy *) new
                        ACE_Local_RLock ("THE_TOKEN", ignore_deadlock, debug);
      global_wlock = (ACE_Token_Proxy *) new
                        ACE_Local_WLock ("THE_TOKEN", ignore_deadlock, debug);
    }

  ACE_Thread_Manager mgr;

  if (mgr.spawn_n (threads, ACE_THR_FUNC (run_thread),
                   (void *) 0,
                    THR_BOUND | THR_SUSPENDED) == -1)
                    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "spawn failed"), -1);

  if (mgr.resume_all () == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "resume failed"), -1);

  mgr.wait ();

  return 0;
}

#else
int
ACE_TMAIN(int, ACE_TCHAR *[])
{
  ACE_ERROR_RETURN ((LM_ERROR,
                     "threads not supported on this platform\n"), -1);
}
#endif /* ACE_HAS_THREADS */
