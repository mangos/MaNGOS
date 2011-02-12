// $Id: Server.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "ace/config-lite.h"
#include "ace/OS_NS_string.h"
#include "ace/Get_Opt.h"

#if (ACE_NTRACE != 1)
#include "ace/Trace.h"
#endif /* (ACE_NTRACE != 1) */

#include "JAWS/Server.h"
#include "JAWS/Data_Block.h"
#include "JAWS/Concurrency.h"
#include "JAWS/Jaws_IO.h"
#include "JAWS/IO_Handler.h"
#include "JAWS/IO_Acceptor.h"
#include "JAWS/Pipeline_Tasks.h"



JAWS_Server::JAWS_Server (void)
  : port_ (5432),
    concurrency_ (0),
    dispatch_ (0),
    nthreads_ (5),
    maxthreads_ (20),
    flags_ (THR_NEW_LWP)
{
}

JAWS_Server::JAWS_Server (int argc, char *argv[])
  : ratio_ (1),
    port_ (5432),
    concurrency_ (0),
    dispatch_ (0),
    nthreads_ (5),
    maxthreads_ (20),
    flags_ (THR_NEW_LWP)
{
  this->init (argc, argv);
}

void
JAWS_Server::init (int argc, char *argv[])
{
  this->parse_args (argc, argv);

  this->policy_.ratio (this->ratio_);

  if (this->concurrency_ == 1)
    {
      JAWS_Thread_Per_Singleton::instance ()->make (this->flags_,
                                                    this->maxthreads_);
      this->policy_.concurrency (JAWS_Thread_Per_Singleton::instance ());
    }
  else
    {
      JAWS_Thread_Pool_Singleton::instance ()->make (this->flags_,
                                                     this->nthreads_,
                                                     this->maxthreads_);
      this->policy_.concurrency (JAWS_Thread_Pool_Singleton::instance ());
    }

#if !(defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS))
  this->dispatch_ = 0;
#endif /* !(ACE_HAS_WIN32_OVERLAPPED_IO || ACE_HAS_AIO_CALLS) */

  if (this->dispatch_ == 1)
    {
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
      this->policy_.io (JAWS_Asynch_IO_Singleton::instance ());
      this->policy_.ioh_factory
        (JAWS_Asynch_IO_Handler_Factory_Singleton::instance ());
      this->policy_.acceptor (JAWS_IO_Asynch_Acceptor_Singleton::instance ());
#endif /* ACE_HAS_WIN32_OVERLAPPED_IO || ACE_HAS_AIO_CALLS */
    }
  else
    {
      this->policy_.io (JAWS_Synch_IO_Singleton::instance ());
      this->policy_.ioh_factory
        (JAWS_Synch_IO_Handler_Factory_Singleton::instance ());
      this->policy_.acceptor (JAWS_IO_Synch_Acceptor_Singleton::instance ());
    }

  //FUZZ: disable check_for_lack_ACE_OS
  ACE_INET_Addr inet_addr (this->port_);
  //FUZZ: enable check_for_lack_ACE_OS

  this->policy_.acceptor ()->open (inet_addr);
}

int
JAWS_Server::open (JAWS_Pipeline_Handler *protocol,
                   JAWS_Dispatch_Policy *policy)
{
  if (policy == 0)
    policy = &this->policy_;

  JAWS_Data_Block *db = new JAWS_Data_Block;
  if (db == 0)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "(%t) JAWS_Server::open, could not create Data_Block\n"));
      return -1;
    }

  // initialize data block

  db->task (JAWS_Pipeline_Accept_Task_Singleton::instance ());
  db->policy (policy);
  db->io_handler (0);

  db->task ()->next (protocol);

  // prime the acceptor if appropriate
  if (this->dispatch_ == 1)
    {
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)

      int n = this->nthreads_;
      if (this->concurrency_ == 1)
        n = 1;

      for (int i = 0; i < n * this->ratio_ - n; i++)
        db->task ()->put (db);

#endif /* ACE_HAS_WIN32_OVERLAPPED_IO || ACE_HAS_AIO_CALLS */
    }

  // The message block should contain an INET_Addr, and call the
  // io->accept (INET_Addr) method!

  policy->concurrency ()->put (db);

  ACE_Thread_Manager::instance ()->wait ();

  db->release ();

  return 0;
}

void
JAWS_Server::parse_args (int argc, ACE_TCHAR *argv[])
{
  int c;
  int t = 0;

  //FUZZ: disable check_for_lack_ACE_OS
  ACE_Get_Opt getopt (argc, argv, ACE_TEXT("t" "p:c:d:n:m:f:r:"));
  while ((c = getopt ()) != -1)
  //FUZZ: enable check_for_lack_ACE_OS
    switch (c)
      {
      case 't':
        t = !t;
        break;
      case 'p':
        this->port_ = ACE_OS::atoi (getopt.opt_arg ());
        break;
      case 'c':
        if (ACE_OS::strcmp (getopt.opt_arg (), "PER_REQUEST") == 0)
          this->concurrency_ = 1;
        else this->concurrency_ = 0;
        break;
      case 'd':
        if (ACE_OS::strcmp (getopt.opt_arg (), "ASYNCH") == 0)
          this->dispatch_ = 1;
        else this->dispatch_ = 0;
        break;
      case 'n':
        this->nthreads_ = ACE_OS::atoi (getopt.opt_arg ());
        break;
      case 'm':
        this->maxthreads_ = ACE_OS::atoi (getopt.opt_arg ());
        break;
      case 'f':
        if (ACE_OS::strcmp (getopt.opt_arg (), "THR_BOUND") == 0)
          this->flags_ |= THR_BOUND;
        else if (ACE_OS::strcmp (getopt.opt_arg (), "THR_DAEMON") == 0)
          this->flags_ |= THR_DAEMON;
        else if (ACE_OS::strcmp (getopt.opt_arg (), "THR_DETACHED") == 0)
          this->flags_ |= THR_DETACHED;
        break;
      case 'r':
        this->ratio_ = ACE_OS::atoi (getopt.opt_arg ());
        break;
      }

#if (ACE_NTRACE != 1)
  if (t)
    ACE_Trace::start_tracing ();
  else
    ACE_Trace::stop_tracing ();
#endif /* ACE_NTRACE != 1*/

  if (this->port_ == 0) this->port_ = 5432;
  if (this->nthreads_ == 0) this->nthreads_ = 5;
  if (this->maxthreads_ == 0) this->maxthreads_ = 20;
}
