// $Id: SS_State_ERROR.cpp 91730 2010-09-13 09:31:11Z johnnyw $

#include "jaws3/IO.h"

#include "SS_State_ERROR.h"
#include "SS_State_DONE.h"
#include "SS_Data.h"

int
TeraSS_State_ERROR::service (JAWS_Event_Completer *ec, void *data)
{
  // Send an error message.
  TeraSS_Data *tdata = (TeraSS_Data *) data;

  tdata->mb ().reset ();
  tdata->mb ().copy ("FAIL\r\n", 6);

  JAWS_IO::instance ()->send ( tdata->peer ().get_handle ()
                             , & tdata->mb ()
                             , ec
                             );

  return 0;
}

JAWS_Protocol_State *
TeraSS_State_ERROR::transition (const JAWS_Event_Result &, void *, void *)
{
  // In the ERROR state, always transition to DONE.

  return TeraSS_State_DONE::instance ();
}

