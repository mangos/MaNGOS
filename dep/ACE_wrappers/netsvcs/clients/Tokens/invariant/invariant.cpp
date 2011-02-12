// $Id: invariant.cpp 91671 2010-09-08 18:39:23Z johnnyw $

// ============================================================================
//
// = LIBRARY
//    examples
//
// = FILENAME
//    invariant.cpp
//
// = DESCRIPTION
//
// = AUTHOR
//    Tim Harrison
//
// ============================================================================

#include "ace/Get_Opt.h"
#include "ace/Singleton.h"
#include "ace/Thread_Manager.h"
#include "ace/Token_Invariants.h"

#if defined (ACE_HAS_THREADS) && defined (ACE_HAS_THREADS_LIBRARY)



typedef ACE_Token_Invariant_Manager ACE_TOKEN_INVARIANTS;

static const char *rwname = "reader/writer";
static const char *mutexname = "mutex";

static void *
run_reader_writer (void *)
{
  for (int x = 0; x < 50; x++)
    {
      int y = 0;
      for (; y < 5; y++)
        {
          if (ACE_TOKEN_INVARIANTS::instance ()->reader_acquired (rwname) == 0)
            ACE_ERROR_RETURN ((LM_ERROR, "reader acquire violated invariant.\n"), 0);

          ACE_DEBUG ((LM_DEBUG, "(%t) rlock acquired.\n"));
        }

      ACE_TOKEN_INVARIANTS::instance ()->rwlock_releasing (rwname);

      if (ACE_TOKEN_INVARIANTS::instance ()->reader_acquired (rwname) == 0)
        ACE_ERROR_RETURN ((LM_ERROR, "reader renew violated invariant.\n"), 0);

      ACE_DEBUG ((LM_DEBUG, "(%t) rlock renewed.\n"));

      for (; y > 0; y--)
        {
          ACE_TOKEN_INVARIANTS::instance ()->rwlock_releasing (rwname);
          ACE_DEBUG ((LM_DEBUG, "(%t) r-released.\n"));
        }

      if (ACE_TOKEN_INVARIANTS::instance ()->writer_acquired (rwname) == 0)
        ACE_ERROR_RETURN ((LM_ERROR, "writer acquire violated invariant.\n"), 0);

      ACE_DEBUG ((LM_DEBUG, "\t\t(%t) wlock acquired.\n"));

      ACE_TOKEN_INVARIANTS::instance ()->rwlock_releasing (rwname);

      if (ACE_TOKEN_INVARIANTS::instance ()->writer_acquired (rwname) == 0)
        ACE_ERROR_RETURN ((LM_ERROR, "writer renew violated invariant.\n"), 0);

      ACE_DEBUG ((LM_DEBUG, "(%t) rlock renewed.\n"));

      ACE_TOKEN_INVARIANTS::instance ()->rwlock_releasing (rwname);
    }

  ACE_DEBUG ((LM_DEBUG, "(%t) thread exiting.\n"));
  return 0;
}

static void *
run_mutex (void *)
{
  for (int x = 0; x < 50; x++)
    {
      if (ACE_TOKEN_INVARIANTS::instance ()->mutex_acquired (mutexname) == 0)
        ACE_ERROR_RETURN ((LM_ERROR, "mutex acquire violated invariant.\n"), 0);

      ACE_DEBUG ((LM_DEBUG, "(%t) mutex acquired.\n"));

      ACE_TOKEN_INVARIANTS::instance ()->mutex_releasing (mutexname);

      if (ACE_TOKEN_INVARIANTS::instance ()->mutex_acquired (mutexname) == 0)
        ACE_ERROR_RETURN ((LM_ERROR, "mutex renew violated invariant.\n"), 0);

      ACE_DEBUG ((LM_DEBUG, "(%t) mutex renewed.\n"));

      ACE_TOKEN_INVARIANTS::instance ()->mutex_releasing (mutexname);
      ACE_DEBUG ((LM_DEBUG, "(%t) mutex released.\n"));
    }

  ACE_DEBUG ((LM_DEBUG, "(%t) thread exiting.\n"));
  return 0;
}

static int
run_final_test (void)
{
  ACE_DEBUG ((LM_DEBUG, "starting mutex tests 1 & 2\n"));

  // Mutex tests.
  if (ACE_TOKEN_INVARIANTS::instance ()->mutex_acquired ("testing mutex") == 0)
    ACE_ERROR_RETURN ((LM_ERROR, "mutex test 1 failed.\n"), 0);
  if (ACE_TOKEN_INVARIANTS::instance ()->mutex_acquired ("testing mutex2") == 0)
    ACE_ERROR_RETURN ((LM_ERROR, "mutex test 2 failed.\n"), 0);
  if (ACE_TOKEN_INVARIANTS::instance ()->mutex_acquired ("testing mutex") == 0)
    ACE_DEBUG ((LM_DEBUG, "mutex test 1 succeeded.\n"));
  else
    ACE_ERROR_RETURN ((LM_ERROR, "mutex test 1 failed..\n"), 0);

  if (ACE_TOKEN_INVARIANTS::instance ()->mutex_acquired ("testing mutex2") == 0)
    ACE_DEBUG ((LM_DEBUG, "mutex test 2 succeeded.\n"));
  else
    ACE_ERROR_RETURN ((LM_ERROR, "mutex test 2 failed..\n"), 0);

  // RW tests.
  ACE_DEBUG ((LM_DEBUG, "starting rwlock tests 1 & 2\n"));

  // Multiple readers.
  if (ACE_TOKEN_INVARIANTS::instance ()->reader_acquired ("testing rwlock") == 0)
    ACE_ERROR_RETURN ((LM_ERROR, "rwlock test 1 failed.\n"), 0);
  if (ACE_TOKEN_INVARIANTS::instance ()->reader_acquired ("testing rwlock 2") == 0)
    ACE_ERROR_RETURN ((LM_ERROR, "rwlock test 2 failed.\n"), 0);
  if (ACE_TOKEN_INVARIANTS::instance ()->reader_acquired ("testing rwlock") == 0)
    ACE_ERROR_RETURN ((LM_ERROR, "rwlock test 1 failed..\n"), 0);
  if (ACE_TOKEN_INVARIANTS::instance ()->reader_acquired ("testing rwlock 2") == 0)
    ACE_ERROR_RETURN ((LM_ERROR, "rwlock test 2 failed..\n"), 0);

  // Writer.
  if (ACE_TOKEN_INVARIANTS::instance ()->writer_acquired ("testing rwlock") == 0)
    ACE_DEBUG ((LM_ERROR, "rwlock test 1 succeded.\n"));
  else
    ACE_ERROR_RETURN ((LM_ERROR, "rwlock test 1 failed...\n"), 0);

  // Releasing reader.
  ACE_TOKEN_INVARIANTS::instance ()->rwlock_releasing ("testing rwlock 2");
  ACE_TOKEN_INVARIANTS::instance ()->rwlock_releasing ("testing rwlock 2");

  // Writer.
  if (ACE_TOKEN_INVARIANTS::instance ()->writer_acquired ("testing rwlock 2") == 0)
    ACE_ERROR_RETURN ((LM_ERROR, "rwlock test 2 failed....\n"), 0);

  // Reader.
  if (ACE_TOKEN_INVARIANTS::instance ()->reader_acquired ("testing rwlock 2") == 0)
    ACE_DEBUG ((LM_DEBUG, "rwlock test 2 succeeded.\n"));
  else
    ACE_ERROR_RETURN ((LM_ERROR, "rwlock test 2 failed.....\n"), 0);

  return 0;
}

int
ACE_TMAIN(int, ACE_TCHAR *[])
{
  ACE_Thread_Manager mgr;

  // Run reader/writer test
  if (mgr.spawn_n (2, ACE_THR_FUNC (run_reader_writer),
                   (void *) 0,
                   THR_NEW_LWP | THR_DETACHED) == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "spawn failed"), -1);

  mgr.wait ();

  ACE_OS::sleep (2);

  // Run mutex test.
  if (mgr.spawn_n (2, ACE_THR_FUNC (run_mutex),
                   (void *) 0,
                   THR_NEW_LWP | THR_DETACHED) == -1)
    ACE_ERROR_RETURN ((LM_DEBUG, "%p\n", "spawn failed"), -1);

  mgr.wait ();

  ACE_OS::sleep (2);

  run_final_test ();

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
