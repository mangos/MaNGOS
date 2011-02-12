/* -*- C++ -*- */
// $Id: Event_Forwarding_Discriminator.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    gateway
//
// = FILENAME
//    Event_Forwarding_Discriminator.h
//
// = AUTHOR
//    Doug Schmidt
//
// ============================================================================

#ifndef _CONSUMER_MAP_H
#define _CONSUMER_MAP_H

#include "ace/Map_Manager.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Null_Mutex.h"
#include "Event.h"
#include "Consumer_Dispatch_Set.h"

class Event_Forwarding_Discriminator
{
  // = TITLE
  //    Map events to the set of Consumer_Proxies that have subscribed
  //    to receive the event.
public:
  int bind (Event_Key event, Consumer_Dispatch_Set *cds);
  // Associate Event with the Consumer_Dispatch_Set.

  int unbind (Event_Key event);
  // Locate EXID and pass out parameter via INID.  If found,
  // return 0, else -1.

  int find (Event_Key event, Consumer_Dispatch_Set *&cds);
  // Break any association of EXID.

public:
  ACE_Map_Manager<Event_Key, Consumer_Dispatch_Set *, ACE_Null_Mutex> map_;
  // Map that associates <Event_Key>s (external ids) with
  // <Consumer_Dispatch_Set> *'s <internal IDs>.
};

class Event_Forwarding_Discriminator_Iterator
{
  // = TITLE
  //    Define an iterator for the Consumer Map.
public:
  Event_Forwarding_Discriminator_Iterator (Event_Forwarding_Discriminator &mm);
  int next (Consumer_Dispatch_Set *&);
  int advance (void);

private:
  ACE_Map_Iterator<Event_Key, Consumer_Dispatch_Set *, ACE_Null_Mutex> map_iter_;
  // Map we are iterating over.
};
#endif /* _CONSUMER_MAP_H */
