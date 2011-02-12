// $Id: ID_Generator.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#if !defined (ACE_ID_GENERATOR_C)
#define ACE_ID_GENERATOR_C

#include "ace/Object_Manager.h"
#include "ID_Generator.h"

time_t ACE_ID_Generator::last_time_ = 0;

size_t ACE_ID_Generator::last_number_ = 0;

ACE_SYNCH_MUTEX *ACE_ID_Generator::lock_ = 0;

char *
ACE_ID_Generator::get_new_id (char *id)
{
  time_t t;
  size_t sn;

  ACE_ID_Generator::get_serial_id (t, sn);
  ACE_NEW_RETURN (id, char [ACE_OFFER_ID_LENGTH], 0);

  ACE_OS::sprintf (id, "%014d%06d", t, sn);
  return id;
}

void
ACE_ID_Generator::get_serial_id (time_t &t, size_t &s)
{
  ACE_MT (ACE_GUARD (ACE_SYNCH_MUTEX, ace_mon, *ACE_ID_Generator::get_lock ()));
  ACE_OS::time (&t);

  if (t != ACE_ID_Generator::last_time_)
    {
      ACE_ID_Generator::last_time_ = t;
      s = ACE_ID_Generator::last_number_ = 0;
    }
  else
      s = ACE_ID_Generator::last_number_++;
}

ACE_SYNCH_MUTEX *
ACE_ID_Generator::get_lock (void)
{
#if defined (ACE_HAS_THREADS)
  if (ACE_ID_Generator::lock_ == 0)
    {
      ACE_MT (ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, ace_mon, *ACE_Static_Object_Lock::instance (), 0));

      // Double-checked Locking Optimization.
      if (ACE_ID_Generator::lock_ == 0)
        ACE_NEW_RETURN (ACE_ID_Generator::lock_, ACE_SYNCH_MUTEX, 0);
    }
#endif /* ACE_HAS_THREADS */
  return ACE_ID_Generator::lock_;
}

#endif /* ACE_ID_GENERATOR_C */
