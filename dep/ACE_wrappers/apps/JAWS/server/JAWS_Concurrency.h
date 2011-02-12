/* -*- c++ -*- */
// $Id: JAWS_Concurrency.h 85430 2009-05-25 11:26:46Z johnnyw $

#ifndef JAWS_CONCURRENCY_H
#define JAWS_CONCURRENCY_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Task.h"
#include "JAWS_IO.h"

class JAWS_Concurrency_Base : public ACE_Task<ACE_SYNCH>
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
  virtual int put (ACE_Message_Block *mb, ACE_Time_Value *tv = 0);
  virtual int svc (void);
};

class JAWS_Dispatch_Policy
  // = TITLE
  //     Policy mechanism for choosing different concurrency models.
  //
  // = DESCRIPTION
  //     Given some (unspecified) state, decides what the concurrency
  //     model should be.  (For now, we always return the same model.)
{
public:
  JAWS_Dispatch_Policy (void);
  virtual ~JAWS_Dispatch_Policy (void);
  virtual JAWS_Concurrency_Base * update (void *state = 0) = 0;
};

class JAWS_Dispatcher
  // = TITLE
  //     The class that is responsible to delivering events to the
  //     appropriate concurrency mechanism.
  //
  // = DESCRIPTION
  //     JAWS_IO_Handler calls into the dispatcher so that the completed
  //     IO can find a thread to take care of it.
{
public:
  JAWS_Dispatcher (JAWS_Dispatch_Policy *policy);

  int dispatch (JAWS_IO_Handler *ioh);

private:
  JAWS_Dispatch_Policy *policy_;
};

class JAWS_Thread_Pool_Task : public JAWS_Concurrency_Base
  // = TITLE
  //     Used to implement Thread Pool Concurrency Strategy
  //
  // = DESCRIPTION
  //     This task is created to hold a pool of threads that receive
  //     requests through the message queue.
{
public:
  JAWS_Thread_Pool_Task (long flags = THR_NEW_LWP,
                         int nthreads = 5,
                         int maxthreads = 20);

private:
  int nthreads_;
  int maxthreads_;
};

class JAWS_Thread_Per_Task : public JAWS_Concurrency_Base
  // = TITLE
  //     Used to implement Thread Per Request Concurrency Strategy
  //
  // = DESCRIPTION
  //     As each new message arrives from the queue, a new thread is
  //     spawned to handle it.  This is done by overloading put to call
  //     activate.
{
public:
  JAWS_Thread_Per_Task (long flags = THR_NEW_LWP, int maxthreads = 20);

  virtual int put (ACE_Message_Block *mb, ACE_Time_Value *tv = 0);

private:
  long flags_;
  int maxthreads_;
};

#endif /* !defined (JAWS_CONCURRENCY_H) */
