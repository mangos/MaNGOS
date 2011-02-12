// $Id: SS_State_PARSE.cpp 91730 2010-09-13 09:31:11Z johnnyw $

#include "SS_State_READ.h"
#include "SS_State_PARSE.h"
#include "SS_State_WRITE.h"
#include "SS_State_ERROR.h"
#include "SS_State_DONE.h"
#include "SS_Data.h"

int
TeraSS_State_PARSE::service (JAWS_Event_Completer *ec, void *data)
{
  JAWS_Event_Result fake_good_result (0, JAWS_Event_Result::JE_OK);
  JAWS_Event_Result fake_bad_result (0, JAWS_Event_Result::JE_ERROR);

  // Parse the request.
  TeraSS_Data *tdata = (TeraSS_Data *) data;
  char *p = tdata->mb ().rd_ptr ();
  while (p < tdata->mb ().wr_ptr () && *p != '\r' && *p != '\n')
    p++;
  if (p == tdata->mb ().wr_ptr ())
    {
      // Return to the READ state.
      ec->input_complete (fake_bad_result, 0);
      return 0;
    }
  *p = '\0';

  // Make us transition into the WRITE state.
  ec->input_complete (fake_good_result, 0);
  return 0;
}

JAWS_Protocol_State *
TeraSS_State_PARSE::transition ( const JAWS_Event_Result &result
                               , void *
                               , void *
                               )
{
  // In the PARSE state, we transition to WRITE on success,
  // and to READ on failure.

  JAWS_Protocol_State *next_state = 0;

  switch (result.status ())
    {
    case JAWS_Event_Result::JE_OK:
      next_state = TeraSS_State_WRITE::instance ();
      break;
    case JAWS_Event_Result::JE_ERROR:
      next_state = TeraSS_State_READ::instance ();
      break;
    default:
      // Just bail unceremoniously.
      next_state = TeraSS_State_DONE::instance ();
      break;
    }

  return next_state;
}

