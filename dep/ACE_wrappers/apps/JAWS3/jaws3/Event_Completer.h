/* -*- c++ -*- */
// $Id: Event_Completer.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_EVENT_COMPLETER_H
#define JAWS_EVENT_COMPLETER_H

#include "jaws3/Export.h"
#include "jaws3/Event_Result.h"

class JAWS_Event_Completer;

class JAWS_Export JAWS_Event_Completer
// = TITLE
//     Event completion callback class.
{
public:

  virtual ~JAWS_Event_Completer (void);

  virtual void accept_complete (const JAWS_Event_Result &r, void *act = 0);
  // The callback entry point when input has been read.

  virtual void connect_complete (const JAWS_Event_Result &r, void *act = 0);
  // The callback entry point when input has been read.

  virtual void input_complete (const JAWS_Event_Result &r, void *act = 0);
  // The callback entry point when input has been read.

  virtual void output_complete (const JAWS_Event_Result &r, void *act = 0);
  // The callback entry point when output has been completely written.

  virtual void timer_complete (const JAWS_Event_Result &r, void *act = 0);
  // The callback entry point when timer has expired.

  virtual void lambda_complete (const JAWS_Event_Result &r, void *act = 0);
  // The callback entry point when a lambda event completes.

  virtual void default_complete (const JAWS_Event_Result &r, void *act = 0);
  // The defaul callback entry point when an event completes.

};

#endif /* JAWS_EVENT_COMPLETER_H */
