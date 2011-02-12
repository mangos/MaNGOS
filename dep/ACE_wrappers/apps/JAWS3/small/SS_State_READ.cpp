// $Id: SS_State_READ.cpp 91730 2010-09-13 09:31:11Z johnnyw $

#include "jaws3/IO.h"
#include "jaws3/Event_Completer.h"

#include "SS_State_READ.h"
#include "SS_State_PARSE.h"
#include "SS_State_ERROR.h"
#include "SS_State_DONE.h"
#include "SS_Data.h"

int
TeraSS_State_READ::service (JAWS_Event_Completer *ec, void *data)
{
  // Recover state.
  TeraSS_Data *tdata = (TeraSS_Data *) data;

  // Read until we see a newline.
  JAWS_IO::instance ()->recv ( tdata->peer ().get_handle ()
                             , & tdata->mb ()
                             , ec
                             );

  return 0;
}

JAWS_Protocol_State *
TeraSS_State_READ::transition ( const JAWS_Event_Result &result
                              , void *
                              , void *
                              )
{
  // In the READ state, move to PARSE if success.
  // Otherwise ERROR.

  JAWS_Protocol_State *next_state = 0;

  switch (result.status ())
    {
    case JAWS_Event_Result::JE_OK:
      next_state = TeraSS_State_PARSE::instance ();
      break;
    case JAWS_Event_Result::JE_ERROR:
      next_state = TeraSS_State_ERROR::instance ();
      break;
    default:
      // Just bail unceremoniously.
      next_state = TeraSS_State_DONE::instance ();
      break;
    }

  return next_state;
}

