// $Id: deadlock_detection_test.cpp 91671 2010-09-08 18:39:23Z johnnyw $

// ============================================================================
//
// = LIBRARY
//    examples
//
// = FILENAME
//    deadlock_detection_test.cpp
//
// = DESCRIPTION
//
// = AUTHOR
//    Tim Harrison
//
// ============================================================================

#include "ace/Token_Manager.h"
#include "ace/Remote_Tokens.h"
#include "ace/Thread.h"
#include "ace/Thread_Manager.h"
#include "ace/Get_Opt.h"
#include "ace/Token_Invariants.h"

#if defined (ACE_HAS_THREADS) && defined (ACE_HAS_THREADS_LIBRARY)



typedef ACE_Token_Invariant_Manager ACE_TOKEN_INVARIANTS;

static ACE_Token_Proxy *global_mutex;

struct Two_Tokens
{
public:
  Two_Tokens (ACE_Thread_Manager *tm): thr_mgr_ (tm) {}
  ACE_Token_Proxy *first_;
  ACE_Token_Proxy *second_;
  ACE_Thread_Manager *thr_mgr_;
};

struct Four_Tokens
{
public:
  Four_Tokens (ACE_Thread_Manager *tm): thr_mgr_ (tm) {}
  ACE_Token_Proxy *first1_;
  ACE_Token_Proxy *first2_;
  ACE_Token_Proxy *first3_;
  ACE_Token_Proxy *second_;
  ACE_Thread_Manager *thr_mgr_;
};

static int ignore_deadlock = 0;
static int remote_mutexes = 0;
static const char *server_host = ACE_DEFAULT_SERVER_HOST;
static int server_port = ACE_DEFAULT_SERVER_PORT;
static int iterations = 100;
static int rwlocks = 0;

static void *
two_token_thread (void *vp)
{
  Two_Tokens* tm = (Two_Tokens *) vp;

  for (int x = 0; x < iterations; x++)
    {
      if (tm->first_->acquire () == -1)
        {
          ACE_DEBUG ((LM_DEBUG, "Deadlock detected\n"));
          continue;
        }

      if (ACE_TOKEN_INVARIANTS::instance ()->acquired (tm->first_) == 0)
        {
          tm->first_->dump ();
          ACE_ERROR_RETURN ((LM_ERROR, "violated invariant.\n"), 0);
        }

      if (tm->second_->acquire () == -1)
        {
          ACE_DEBUG ((LM_DEBUG, "Deadlock Detected\n"));
          goto G1;
        }

      if (ACE_TOKEN_INVARIANTS::instance ()->acquired (tm->second_) == 0)
        {
          tm->second_->dump ();
          ACE_ERROR_RETURN ((LM_ERROR, "violated invariant.\n"), 0);
        }

      ACE_TOKEN_INVARIANTS::instance ()->releasing (tm->second_);

      tm->second_->release ();
    G1:
      ACE_TOKEN_INVARIANTS::instance ()->releasing (tm->first_);

      tm->first_->release ();
    }

  ACE_DEBUG ((LM_DEBUG, "thread %t exiting\n"));
  return 0;
}

static void *
run_writer (void *vp)
{
  Four_Tokens *ft = (Four_Tokens *) vp;
  int acquire_number = 0;

  for (int x = 0; x < iterations; x++)
    {
      // Cycle through each of the first three tokens.
      ACE_Token_Proxy *t = 0;
      switch (acquire_number)
        {
        case 0:
          t = ft->first1_;
          break;
        case 1:
          t = ft->first2_;
          break;
        case 2:
          t = ft->first3_;
          break;
        }

      acquire_number = (acquire_number + 1) % 3;

      if (t->acquire () == -1)
        {
          ACE_ASSERT (errno == EDEADLK);
          ACE_DEBUG ((LM_DEBUG, "Deadlock detected.\n"));
          continue;
        }

      if (ACE_TOKEN_INVARIANTS::instance ()->acquired (t) == 0)
        {
          t->dump ();
          ACE_ERROR_RETURN ((LM_ERROR, "violated invariant.\n"), 0);
        }

      if (ft->second_->acquire () == -1)
        {
          ACE_ASSERT (errno == EDEADLK);
          ACE_DEBUG ((LM_DEBUG, "Deadlock Detected..\n"));
          goto G1;
        }

      if (ACE_TOKEN_INVARIANTS::instance ()->acquired (ft->second_) == 0)
        {
          ft->second_->dump ();
          ACE_ERROR_RETURN ((LM_ERROR, "violated invariant.\n"), 0);
        }

      ACE_TOKEN_INVARIANTS::instance ()->releasing (ft->second_);

      ft->second_->release ();

      G1:
        ACE_TOKEN_INVARIANTS::instance ()->releasing (t);

        t->release ();
    }

  ACE_DEBUG ((LM_DEBUG, "thread %t exiting\n"));
  return 0;
}

static int
parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_LOG_MSG->open (argv[0]);

  ACE_Get_Opt get_opt (argc, argv, ACE_TEXT("iuh:rp:n:"), 1);

  for (int c; (c = get_opt ()) != -1; )
    {
      switch (c)
        {
        case 'r':
          rwlocks = 1;
          break;
        case 'i':
          ignore_deadlock = 1;
          break;
        case 'h':
          server_host = get_opt.opt_arg ();
          remote_mutexes = 1;
          break;
        case 'p':
          server_port = ACE_OS::atoi (get_opt.opt_arg ());
          remote_mutexes = 1;
          break;
        case 'n':
          iterations = ACE_OS::atoi (get_opt.opt_arg ());
          break;
        case 'u':
        default:
          ACE_ERROR_RETURN ((LM_ERROR,
                             "%n:\n"
                             "[-r test readers/writer locks]\n"
                             "[-n <iterations>]\n"
                             "[-h <remote host>]\n"
                             "[-p <remote port>]\n"
                             "[-i ignore deadlock]\n%a", 1), -1);
        }
    }

  return 0;
}

int
mutex_test (void)
{
  ACE_Thread_Manager thr_mgr;

  Two_Tokens one (&thr_mgr);
  Two_Tokens two (&thr_mgr);

  if (remote_mutexes == 0)
    {
      global_mutex = new ACE_Local_Mutex ("global proxy", ignore_deadlock, 1);
      one.first_ = new ACE_Local_Mutex ("local proxy", ignore_deadlock, 1);
      two.second_ = new ACE_Local_Mutex ("local proxy", ignore_deadlock, 1);
    }
  else
    {
      ACE_Remote_Mutex::set_server_address (ACE_INET_Addr (server_port, server_host));
      global_mutex = new ACE_Remote_Mutex ("global proxy", ignore_deadlock, 1);
      one.first_ = new ACE_Remote_Mutex ("local proxy", ignore_deadlock, 1);
      two.second_ = new ACE_Remote_Mutex ("local proxy", ignore_deadlock, 1);
    }

  one.second_ = global_mutex;
  two.first_ = global_mutex;

  // Tell the token manager to be verbose when reporting deadlock.
  ACE_Token_Manager::instance ()->debug (1);

  if (thr_mgr.spawn (ACE_THR_FUNC (two_token_thread),
                                   (void *) &one, THR_BOUND) == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "first spawn"), -1);

  if (thr_mgr.spawn (ACE_THR_FUNC (two_token_thread),
                                   (void *) &two, THR_BOUND) == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "second spawn"), -1);

  // Wait for all threads to exit.
  thr_mgr.wait ();

  return 0;
}

static int
rwlock_test (void)
{
  ACE_Thread_Manager thr_mgr;

  Two_Tokens reader1 (&thr_mgr);
  Two_Tokens reader2 (&thr_mgr);
  Two_Tokens reader3 (&thr_mgr);
  Four_Tokens writer (&thr_mgr);

  if (remote_mutexes == 0)
    {
      reader1.first_  = new ACE_Local_RLock ("reader first", ignore_deadlock, 1);
      reader1.second_ = new ACE_Local_RLock ("writer first 1", ignore_deadlock, 1);
      reader2.first_  = new ACE_Local_RLock ("reader first", ignore_deadlock, 1);
      reader2.second_ = new ACE_Local_RLock ("writer first 2", ignore_deadlock, 1);
      reader3.first_  = new ACE_Local_RLock ("reader first", ignore_deadlock, 1);
      reader3.second_ = new ACE_Local_RLock ("writer first 3", ignore_deadlock, 1);

      writer.first1_  = new ACE_Local_WLock ("writer first 1", ignore_deadlock, 1);
      writer.first2_  = new ACE_Local_WLock ("writer first 2", ignore_deadlock, 1);
      writer.first3_  = new ACE_Local_WLock ("writer first 3", ignore_deadlock, 1);
      writer.second_  = new ACE_Local_WLock ("reader first", ignore_deadlock, 1);
    }
  else
    {
      ACE_Remote_Mutex::set_server_address (ACE_INET_Addr (server_port, server_host));

      reader1.first_  = new ACE_Remote_RLock ("writer first 1", ignore_deadlock, 1);
      reader1.second_ = new ACE_Remote_RLock ("reader first", ignore_deadlock, 1);
      reader2.first_  = new ACE_Remote_RLock ("writer first 2", ignore_deadlock, 1);
      reader2.second_ = new ACE_Remote_RLock ("reader first", ignore_deadlock, 1);
      reader3.first_  = new ACE_Remote_RLock ("writer first 3", ignore_deadlock, 1);
      reader3.second_ = new ACE_Remote_RLock ("reader first", ignore_deadlock, 1);

      writer.first1_  = new ACE_Remote_WLock ("writer first 1", ignore_deadlock, 1);
      writer.first2_  = new ACE_Remote_WLock ("writer first 2", ignore_deadlock, 1);
      writer.first3_  = new ACE_Remote_WLock ("writer first 3", ignore_deadlock, 1);
      writer.second_  = new ACE_Remote_WLock ("reader first", ignore_deadlock, 1);
    }

  // Tell the token manager to be verbose when reporting deadlock.
  ACE_Token_Manager::instance ()->debug (1);

  if (thr_mgr.spawn (ACE_THR_FUNC (two_token_thread),
                                   (void *) &reader1, THR_BOUND) == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "first spawn"), -1);

  if (thr_mgr.spawn (ACE_THR_FUNC (two_token_thread),
                                   (void *) &reader2, THR_BOUND) == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "first spawn"), -1);

  if (thr_mgr.spawn (ACE_THR_FUNC (two_token_thread),
                                   (void *) &reader3, THR_BOUND) == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "first spawn"), -1);

  if (thr_mgr.spawn (ACE_THR_FUNC (run_writer),
                                   (void *) &writer, THR_BOUND) == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "second spawn"), -1);

  // Wait for all threads to exit.
  thr_mgr.wait ();

  return 0;
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  if (parse_args (argc, argv) == -1)
    return -1;

  if (rwlocks)
    rwlock_test ();
  else
    mutex_test ();

  ACE_DEBUG ((LM_DEBUG, "test exiting.\n"));
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
