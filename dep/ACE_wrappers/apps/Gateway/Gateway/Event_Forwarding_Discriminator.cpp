// $Id: Event_Forwarding_Discriminator.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#if !defined (_CONSUMER_MAP_C)
#define _CONSUMER_MAP_C

#include "Event_Forwarding_Discriminator.h"

// Bind the Event_Key to the INT_ID.

int
Event_Forwarding_Discriminator::bind (Event_Key event_addr,
                Consumer_Dispatch_Set *cds)
{
  return this->map_.bind (event_addr, cds);
}

// Find the Consumer_Dispatch_Set corresponding to the Event_Key.

int
Event_Forwarding_Discriminator::find (Event_Key event_addr,
                Consumer_Dispatch_Set *&cds)
{
  return this->map_.find (event_addr, cds);
}

// Unbind (remove) the Event_Key from the map.

int
Event_Forwarding_Discriminator::unbind (Event_Key event_addr)
{
  Consumer_Dispatch_Set *cds = 0;
  int result = this->map_.unbind (event_addr, cds);
  delete cds;
  return result;
}

Event_Forwarding_Discriminator_Iterator::Event_Forwarding_Discriminator_Iterator
  (Event_Forwarding_Discriminator &rt)
    : map_iter_ (rt.map_)
{
}

int
Event_Forwarding_Discriminator_Iterator::next (Consumer_Dispatch_Set *&cds)
{
  ACE_Map_Entry<Event_Key, Consumer_Dispatch_Set *> *temp;

  if (this->map_iter_.next (temp) == 0)
    return 0;
  else
    {
      cds = temp->int_id_;
      return 1;
    }
}

int
Event_Forwarding_Discriminator_Iterator::advance (void)
{
  return this->map_iter_.advance ();
}
#endif /* _CONSUMER_MAP_C */
