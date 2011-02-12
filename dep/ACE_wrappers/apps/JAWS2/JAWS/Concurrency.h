/* -*- c++ -*- */
// $Id: Concurrency.h 85415 2009-05-22 07:26:32Z johnnyw $

#ifndef JAWS_CONCURRENCY_H
#define JAWS_CONCURRENCY_H

#include "ace/Singleton.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Task.h"
#include "ace/Synch_Traits.h"

#include "JAWS/Export.h"
#include "JAWS/Jaws_IO.h"

class JAWS_Data_Block;
class JAWS_Dispatch_Policy;
class JAWS_Reaper;

class JAWS_Export JAWS_Concurrency_Base : public ACE_Task<ACE_SYNCH>
  // = TITLE
  //     Base class for different concurrency models
  //
  // = DESCRIPTION
  //     Provides a default implementaion of the virtual put() method
  //     which calls putq(), but can be overloaded to do something
  //     synchronously, such as call put_next().

{
public:
  JAWS_Concurrency_Base (void);
  ~JAWS_Concurrency_Base (void);

  virtual int put (ACE_Message_Block *mb, ACE_Time_Value *tv = 0);
  virtual int svc (void);

  virtual int svc_loop (JAWS_Data_Block *db);
  // in thread pool, this is an infinite loop
  // in thread per request, it is a single iteration

  virtual int svc_hook (JAWS_Data_Block *db);
  // does the work of following the pipeline tasks

  virtual int activate_hook (void);
  // callback for IO_Handler when accept completes

  virtual ACE_Message_Block *singleton_mb (void);

protected:
  int mb_acquired_;
  ACE_Message_Block *mb_;
  JAWS_Reaper *reaper_;
  ACE_SYNCH_MUTEX lock_;
};

class JAWS_Export JAWS_Dispatcher
  // = TITLE
  //     The class that is responsible to delivering events to the
  //     appropriate concurrency mechanism.
  //
  // = DESCRIPTION
  //     JAWS_IO_Handler calls into the dispatcher so that the completed
  //     IO can find a thread to take care of it.
{
public:
  JAWS_Dispatcher (void);

  int dispatch (ACE_Message_Block *mb);
  JAWS_Dispatch_Policy *policy (void);
  JAWS_Dispatch_Policy *policy (JAWS_Dispatch_Policy *p);

private:
  JAWS_Dispatch_Policy *policy_;
};

class JAWS_Export JAWS_Thread_Pool_Task : public JAWS_Concurrency_Base
  // = TITLE
  //     Used to implement Thread Pool Concurrency Strategy
  //
  // = DESCRIPTION
  //     This task is created to hold a pool of threads that receive
  //     requests through the message queue.
{
public:
  virtual int make (long flags, int nthreads, int maxthreads);
  // Initiate the thread_pool task

private:
  long flags_;
  int nthreads_;
  int maxthreads_;
};

class JAWS_Export JAWS_Thread_Per_Task : public JAWS_Concurrency_Base
  // = TITLE
  //     Used to implement Thread Per Request Concurrency Strategy
  //
  // = DESCRIPTION
  //     As each new message arrives from the queue, a new thread is
  //     spawned to handle it.  This is done by overloading put to call
  //     activate.
{
public:
  virtual int make (long flags, int maxthreads);
  // Initiate the thread_per task

  virtual int put (ACE_Message_Block *mb, ACE_Time_Value *tv = 0);

  virtual int svc_loop (JAWS_Data_Block *db);
  // a single iteration

  virtual int activate_hook (void);
  // callback for IO_Handler when accept completes

private:
  long flags_;
  int maxthreads_;
};

typedef ACE_Singleton<JAWS_Dispatcher, ACE_SYNCH_MUTEX>
        JAWS_Dispatcher_Singleton;

typedef ACE_Singleton<JAWS_Thread_Pool_Task, ACE_SYNCH_MUTEX>
        JAWS_Thread_Pool_Singleton;

typedef ACE_Singleton<JAWS_Thread_Per_Task, ACE_SYNCH_MUTEX>
        JAWS_Thread_Per_Singleton;

#endif /* !defined (JAWS_CONCURRENCY_H) */
