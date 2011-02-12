// $Id: Protocol_Handler.cpp 91813 2010-09-17 07:52:52Z johnnyw $

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif

#include "jaws3/Protocol_Handler.h"
#include "jaws3/Concurrency.h"

JAWS_Protocol_State::~JAWS_Protocol_State (void)
{
}


JAWS_Protocol_Handler::JAWS_Protocol_Handler ( JAWS_Protocol_State *state
                                             , void *data
                                             )
  : state_ (state)
  , data_ (data)
  , mb_ (& this->db_)
{
  this->db_.base ((char *) this, 0 /* an infinite queue */);
}


JAWS_Protocol_Handler::~JAWS_Protocol_Handler (void)
{
  this->mb_.replace_data_block (0);
}


int
JAWS_Protocol_Handler::service (void)
{
  if (this->state_ == 0)
    return -1;

  return this->state_->service (this, this->data_);
}


void
JAWS_Protocol_Handler::event_complete ( const JAWS_Event_Result &result
                                      , void *act
                                      )
{
  // This call is done in the context of the dispatching
  // thread (e.g., by the Reactor thread, or by one of the
  // threads in the Proactor, or by the invoker of the IO
  // if the IO is synchronous).

  this->state_ = this->state_->transition (result, this->data_, act);

  // At this point, we need to cue this Handler back into
  // the concurrency mechanism.  This probably means the
  // Message Queue of some Concurrency Task.

  JAWS_Concurrency::instance ()->putq (this);

  // Doing it this way is less efficient than calling into
  // the service() method of the next state directly from
  // here, but it gains the flexibility of a more modular
  // concurrency mechanism.
}

