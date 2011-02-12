/* -*- c++ -*- */
// $Id: Event_Dispatcher.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_EVENT_DISPATCHER_H
#define JAWS_EVENT_DISPATCHER_H

#include "ace/Reactor.h"
#include "ace/Reactor_Token_T.h"
#include "ace/Select_Reactor.h"

#include "jaws3/Export.h"

// We are using the Reactor in a single threaded manner,
// so if we are using the Select Reactor, get rid of the
// big reactor lock.

typedef ACE_Reactor_Token_T<ACE_Noop_Token>
        ACE_Select_Reactor_Noop_Token;

typedef ACE_Select_Reactor_T<ACE_Select_Reactor_Noop_Token>
        ACE_Select_NULL_LOCK_Reactor;

#if !defined (ACE_WIN32) \
 || !defined (ACE_HAS_WINSOCK2) || (ACE_HAS_WINSOCK2 == 0) \
 || defined (ACE_USE_SELECT_REACTOR_FOR_REACTOR_IMPL) \
 || defined (ACE_USE_TP_REACTOR_FOR_REACTOR_IMPL)

#define ACE_REACTOR_INSTANCE_INIT \
        do { \
        ACE_Select_NULL_LOCK_Reactor *select_reactor; \
        select_reactor = new ACE_Select_NULL_LOCK_Reactor; \
        ACE_Reactor::instance (new ACE_Reactor (select_reactor, 1), 1); \
        } while (0)

#else

#define ACE_REACTOR_INSTANCE_INIT ACE_Reactor::instance ()

#endif /* ACE_WIN32 */

class JAWS_Event_Dispatcher;

class JAWS_Export JAWS_Event_Dispatcher
{
public:

  static void end_event_loop (void);
  static void run_event_loop (void);

};

#endif /* JAWS_EVENT_DISPATCHER_H */
