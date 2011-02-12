// $Id: SS_State_WRITE.cpp 91730 2010-09-13 09:31:11Z johnnyw $

#include "ace/FILE_Connector.h"
#include "ace/FILE_Addr.h"
#include "ace/FILE_IO.h"

#include "jaws3/IO.h"

#include "SS_State_WRITE.h"
#include "SS_State_ERROR.h"
#include "SS_State_DONE.h"
#include "SS_Data.h"

int
TeraSS_State_WRITE::service (JAWS_Event_Completer *ec, void *data)
{
  JAWS_Event_Result fake_bad_result (0, JAWS_Event_Result::JE_ERROR);

  // Retrieve context
  TeraSS_Data *tdata = (TeraSS_Data *) data;

  ACE_FILE_Addr file_addr (tdata->mb ().rd_ptr ());
  ACE_FILE_Connector file_connector;

  if (file_connector.connect ( tdata->file_io ()
                             , file_addr
                             , 0
                             , ACE_Addr::sap_any
                             , 0
                             , O_RDONLY
                             ) < 0)
    {
      ec->output_complete (fake_bad_result, & tdata->file_io ());
      return 0;
    }


  JAWS_IO::instance ()->transmit ( tdata->peer ().get_handle ()
                                 , tdata->file_io ().get_handle ()
                                 , ec
                                 , & tdata->file_io ()
                                 );

  return 0;
}

JAWS_Protocol_State *
TeraSS_State_WRITE::transition ( const JAWS_Event_Result &result
                               , void *data
                               , void *act
                               )
{
  // Clean up FILE.

  ((ACE_FILE_IO *) act)->close ();

  // In the WRITE state, move to DONE state if success, ERROR if error.

  JAWS_Protocol_State *next_state = 0;

  switch (result.status ())
    {
    case JAWS_Event_Result::JE_OK:
      next_state = TeraSS_State_DONE::instance ();
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

