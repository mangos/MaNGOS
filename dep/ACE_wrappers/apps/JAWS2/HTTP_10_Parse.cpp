// $Id: HTTP_10_Parse.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "HTTP_10.h"

#include "JAWS/JAWS.h"
#include "JAWS/IO.h"
#include "JAWS/IO_Handler.h"
#include "JAWS/Policy.h"

#include "HTTP_10_Request.h"



// --------------- PARSE TASK ----------------------

JAWS_HTTP_10_Parse_Task::JAWS_HTTP_10_Parse_Task (void)
{
}

JAWS_HTTP_10_Parse_Task::~JAWS_HTTP_10_Parse_Task (void)
{
}

int
JAWS_HTTP_10_Parse_Task::handle_put (JAWS_Data_Block *data, ACE_Time_Value *)
{
  JAWS_TRACE ("JAWS_HTTP_10_Parse_Task::handle_put");

  JAWS_IO_Handler *handler = data->io_handler ();
  JAWS_Dispatch_Policy *policy = this->policy ();
  if (policy == 0) policy = data->policy ();
  JAWS_IO *io = policy->io ();

  JAWS_HTTP_10_Request *info;

  if (data->payload ())
    info = reinterpret_cast<JAWS_HTTP_10_Request *> (data->payload ());
  else
    {
      info = new JAWS_HTTP_10_Request;
      if (info == 0)
        {
          ACE_ERROR ((LM_ERROR, "%p\n", "JAWS_HTTP_10_Parse_Task::handle_put"));
          return -1;
        }
      data->payload (static_cast<void *> (info));
    }

  while (info->receive (*(ACE_Message_Block *)data) == 0)
    {
      int next_read_size = data->space ();

      if (next_read_size == 0)
        {
          // Set payload to reflect "request too long"
          break;
        }

      io->read (handler, data, next_read_size);
      switch (handler->status ())
        {
        case JAWS_IO_Handler::READ_OK:
          // Behaved synchronously, reiterate
          continue;
        case JAWS_IO_Handler::READ_ERROR:
        case JAWS_IO_Handler::READ_ERROR_A:
          return -1;
        default:
          // This needs to be a value that tells the framework that
          // the call is asynchronous, but that we should remain in
          // the current task state.
          return 2;
        }
    }

  // request completely parsed
  info->dump ();

  return 0;
}
