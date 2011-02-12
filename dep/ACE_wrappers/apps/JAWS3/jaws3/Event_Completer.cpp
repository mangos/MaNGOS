// $Id: Event_Completer.cpp 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif

#include "jaws3/Event_Completer.h"

// Default do nothing completers.

JAWS_Event_Completer::~JAWS_Event_Completer (void)
{
}

void
JAWS_Event_Completer::default_complete (const JAWS_Event_Result &, void *)
{
}

void
JAWS_Event_Completer::accept_complete (const JAWS_Event_Result &r, void *d)
{
  this->default_complete (r, d);
}

void
JAWS_Event_Completer::connect_complete (const JAWS_Event_Result &r, void *d)
{
  this->default_complete (r, d);
}

void
JAWS_Event_Completer::input_complete (const JAWS_Event_Result &r, void *d)
{
  this->default_complete (r, d);
}

void
JAWS_Event_Completer::output_complete (const JAWS_Event_Result &r, void *d)
{
  this->default_complete (r, d);
}

void
JAWS_Event_Completer::timer_complete (const JAWS_Event_Result &r, void *d)
{
  this->default_complete (r, d);
}

void
JAWS_Event_Completer::lambda_complete (const JAWS_Event_Result &r, void *d)
{
  this->default_complete (r, d);
}

