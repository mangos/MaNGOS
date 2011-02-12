/* -*- c++ -*- */
// $Id: Waiter.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_WAITER_H
#define JAWS_WAITER_H

#include "ace/Singleton.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Synch_Traits.h"
#include "JAWS/Assoc_Array.h"
#include "JAWS/Export.h"

class JAWS_IO_Handler;

typedef ACE_thread_t JAWS_Thread_ID;

typedef JAWS_Assoc_Array<JAWS_Thread_ID, JAWS_IO_Handler *> JAWS_Waiter_Base;
typedef JAWS_Assoc_Array_Iterator<JAWS_Thread_ID, JAWS_IO_Handler *>
        JAWS_Waiter_Base_Iterator;

class JAWS_Export JAWS_Waiter : public JAWS_Waiter_Base
{
public:
  JAWS_Waiter (void);
  ~JAWS_Waiter (void);

  JAWS_Waiter_Base_Iterator &iter (void);
  // Returns an iterator to the headers container.

  int index (void);
  // Returns the index into the table associated with calling thread.

  JAWS_IO_Handler * wait_for_completion (int i = -1);
  // The entry point for this class, handles outstanding asynchronous
  // events.  Can optionally accept a parameter that points to which
  // table entry to return.

private:
  JAWS_Waiter_Base_Iterator iter_;

};

typedef ACE_Singleton<JAWS_Waiter, ACE_SYNCH_MUTEX> JAWS_Waiter_Singleton;

#endif /* JAWS_WAITER_H */
