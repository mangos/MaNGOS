// $Id: Reaper.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "JAWS/Reaper.h"
#include "JAWS/Concurrency.h"
#include "JAWS/IO_Acceptor.h"



JAWS_Reaper::JAWS_Reaper (JAWS_Concurrency_Base *concurrency)
  : concurrency_ (concurrency),
    waiting_ (0)
{
}

JAWS_Reaper::~JAWS_Reaper (void)
{
}

int
JAWS_Reaper::open (void *)
{
  if (this->waiting_ == 0)
    {
      ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, g ,this->lock_, -1);
      if (this->waiting_ == 0)
        {
          if (this->activate () == -1)
            ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "JAWS_Reaper::activate"),
                              -1);
          this->waiting_ = 1;
        }
    }
  return 0;
}

int
JAWS_Reaper::svc (void)
{
  ACE_TRACE ("JAWS_Reaper::svc");
  int result = this->concurrency_->thr_mgr ()->wait ();
  JAWS_IO_Synch_Acceptor_Singleton::instance ()->close ();
  JAWS_IO_Asynch_Acceptor_Singleton::instance ()->close ();
  ACE_DEBUG ((LM_DEBUG, "(%t) Leaving REAPER\n"));
  return result;
}
