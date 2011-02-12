// $Id: HTTP_10_Read.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "HTTP_10.h"

#include "JAWS/JAWS.h"
#include "JAWS/IO.h"
#include "JAWS/IO_Handler.h"
#include "JAWS/Policy.h"



// --------------- READ TASK ----------------------

JAWS_HTTP_10_Read_Task::JAWS_HTTP_10_Read_Task (void)
{
}

JAWS_HTTP_10_Read_Task::~JAWS_HTTP_10_Read_Task (void)
{
}

int
JAWS_HTTP_10_Read_Task::handle_put (JAWS_Data_Block *data, ACE_Time_Value *)
{
  JAWS_TRACE ("JAWS_HTTP_10_Read_Task::handle_put");

  JAWS_IO_Handler *handler = data->io_handler ();
  JAWS_Dispatch_Policy *policy = this->policy ();
  if (policy == 0) policy = data->policy ();

  JAWS_IO *io = policy->io ();

  if (data->length () > 0)
    {
      if (data->rd_ptr ()[0] != '\0')
        {
          JAWS_TRACE ("JAWS_HTTP_10_Read_Task::handle_put, have data");
          return 0;
        }
    }
  data->rd_ptr (data->wr_ptr ());
  data->crunch ();

  io->read (handler, data, data->size ());
  switch (handler->status ())
    {
    case JAWS_IO_Handler::READ_OK:
      {
        JAWS_TRACE ("JAWS_HTTP_10_Read_Task::handle_put, READ_OK");
        return 0;
      }
      break;
    case JAWS_IO_Handler::READ_ERROR:
    case JAWS_IO_Handler::READ_ERROR_A:
      {
        JAWS_TRACE ("JAWS_HTTP_10_Read_Task::handle_put, READ_ERROR");
        return -1;
      }
    default:
      break;
    }

  return 1;
}
