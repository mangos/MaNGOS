// $Id: Protocol_Handler.h 80826 2008-03-04 14:51:23Z wotte $
#ifndef JAWS_PROTOCOL_HANDLER_H
#define JAWS_PROTOCOL_HANDLER_H

#include "ace/Message_Block.h"

#include "jaws3/Export.h"
#include "jaws3/Event_Completer.h"


class JAWS_Export JAWS_Protocol_State
{

  friend class JAWS_Protocol_Handler;

public:

  virtual ~JAWS_Protocol_State (void);

protected:

  virtual int service (JAWS_Event_Completer *, void *data) = 0;

  virtual JAWS_Protocol_State * transition ( const JAWS_Event_Result &
                                           , void *data
                                           , void *act
                                           ) = 0;

};

class JAWS_Export JAWS_Protocol_Handler
  : public JAWS_Event_Completer
// = TITLE
//     Abstraction that interacts with concurrency dispatching.
{

  friend class JAWS_TPOOL_Concurrency;
  friend class JAWS_TPR_Concurrency;
  friend class JAWS_THYBRID_Concurrency;

public:

  JAWS_Protocol_Handler (JAWS_Protocol_State *state = 0, void *data = 0);

  virtual int service (void);

  virtual void dismiss (void)
  {
    delete this;
  }

protected:

  virtual ~JAWS_Protocol_Handler (void);
  // Try to guarantee this class will be created dynamically.

protected:

  void event_complete (const JAWS_Event_Result &result, void *act);
  // The event completion routine that triggers the transition
  // to the next Protocol State.

  void default_complete (const JAWS_Event_Result &result, void *act)
  {
    this->event_complete (result, act);
  }

private:

  JAWS_Protocol_State *state_;

  void *data_;

  ACE_Data_Block db_;
  ACE_Message_Block mb_;

};


#endif /* JAWS_PROTOCOL_HANDLER_H */
