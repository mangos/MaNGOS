// $Id: Pipeline_Tasks.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "JAWS/Jaws_IO.h"
#include "JAWS/Pipeline_Tasks.h"
#include "JAWS/Pipeline_Handler_T.h"
#include "JAWS/Data_Block.h"
#include "JAWS/IO_Handler.h"
#include "JAWS/Policy.h"



JAWS_Pipeline_Handler::JAWS_Pipeline_Handler (void)
  : policy_ (0)
{
}

JAWS_Pipeline_Handler::~JAWS_Pipeline_Handler (void)
{
}

int
JAWS_Pipeline_Handler::put (ACE_Message_Block *mb, ACE_Time_Value *tv)
{
  JAWS_Data_Block *db = dynamic_cast<JAWS_Data_Block *> (mb);
  JAWS_IO_Handler *ioh = db->io_handler ();

  // guarantee the handler remains for the duration of this call
  ioh->acquire ();

  int status = this->handle_put (db, tv);

  if (status != -1 && status != 2)
    {
      JAWS_Pipeline_Handler *task = ioh->task ();
      JAWS_Pipeline_Handler *next
        = dynamic_cast<JAWS_Pipeline_Handler *> (task->next ());

      ioh->task (next);
    }

  ioh->release ();

  return status;
}

JAWS_Dispatch_Policy *
JAWS_Pipeline_Handler::policy (void)
{
  return this->policy_;
}

void
JAWS_Pipeline_Handler::policy (JAWS_Dispatch_Policy *policy)
{
  this->policy_ = policy;
}

int
JAWS_Pipeline_Accept_Task::put (ACE_Message_Block *mb, ACE_Time_Value *tv)
{
  JAWS_Data_Block *db = dynamic_cast<JAWS_Data_Block *> (mb);

  JAWS_Pipeline_Handler *task = db->task ();
  JAWS_Pipeline_Handler *next
    = dynamic_cast<JAWS_Pipeline_Handler *> (task->next ());

  JAWS_IO_Handler *ioh = this->new_handler (db);
  if (ioh == 0)
    {
      ACE_ERROR ((LM_ERROR, "%p\n", "JAWS_Pipeline_Accept_Task::put"));
      return -1;
    }

  ioh->acquire ();

  ioh->task (next);
  db->io_handler (ioh);

  int result = this->handle_put (ioh->message_block (), tv);

  ioh->release ();

  return result;
}

int
JAWS_Pipeline_Accept_Task::handle_put (JAWS_Data_Block *data,
                                       ACE_Time_Value *)
{
  int result = -1;

  // JAWS_Data_Block should contain an INET_Addr and an IO
  JAWS_IO_Handler *handler = data->io_handler ();
  JAWS_Dispatch_Policy *policy = this->policy ();

  if (policy == 0) policy = data->policy ();

  // data->policy ()->update (handler);

  JAWS_IO *io = policy->io ();
  io->accept (handler);

  // When accept returns, the resulting handle should be stored into
  // the JAWS_DATA_BLOCK somewhere.

  // Check the handler for status of the io call
  switch (handler->status ())
    {
    case JAWS_IO_Handler::ACCEPT_OK:
      {
        ACE_DEBUG ((LM_DEBUG, "(%t) ACCEPT_OK\n"));
        result = 0;
        JAWS_TRACE ("JAWS_Pipeline_Accept_Task::handle_put ACCEPT_OK");
        // Move on to next stage in pipeline
        break;
      }
    case JAWS_IO_Handler::ACCEPT_ERROR:
      {
        ACE_DEBUG ((LM_DEBUG, "(%t) ACCEPT_ERROR\n"));
        result = -1;
        JAWS_TRACE ("JAWS_Pipeline_Accept_Task::handle_put ACCEPT_ERROR");
        // Should recycle the thread
        break;
      }
    default:
      {
        result = 1;
        JAWS_TRACE ("JAWS_Pipeline_Accept_Task::handle_put ACCEPT_IDLE");
        // Should mean that the IO is asynchronous, and the word isn't out
        // yet.
        break;
      }
    }

  // In asynchronous and synchronous models, we can --
  //   have the io_handler set the new task in the data_block

  // In asynchronous model, we can --
  //   insert a wait task into the task queue

  ACE_DEBUG ((LM_DEBUG, "(%t) Returning %d\n", result));
  return result;
}

JAWS_IO_Handler *
JAWS_Pipeline_Accept_Task::new_handler (JAWS_Data_Block *data)
{
  // Create a new handler and message block
  JAWS_Data_Block *ndb = new JAWS_Data_Block (*data);
  if (ndb == 0)
    {
      JAWS_TRACE ("JAWS_Pipeline_Accept_Task::new_handler, failed DB");
      return 0;
    }

  JAWS_Dispatch_Policy *policy =
    (this->policy () == 0) ? data->policy () : this->policy ();
  JAWS_IO_Handler_Factory *ioh_factory = policy->ioh_factory ();

  JAWS_IO_Handler *nioh = ioh_factory->create_io_handler ();
  if (nioh == 0)
    {
      delete ndb;
      return 0;
    }

  ndb->io_handler (nioh);
  nioh->task (data->task ());
  nioh->message_block (ndb);

  return nioh;
}

int
JAWS_Pipeline_Done_Task::put (ACE_Message_Block *mb, ACE_Time_Value *)
{
  JAWS_TRACE ("JAWS_Pipeline_Done_Task::put");

  JAWS_Data_Block *data = dynamic_cast<JAWS_Data_Block *> (mb);

  JAWS_IO_Handler *handler = data->io_handler ();
  JAWS_Dispatch_Policy *policy = this->policy ();
  if (policy == 0) policy = data->policy ();

  // JAWS_IO *io = policy->io ();

  data->task (0);
  data->io_handler (0);

  if (handler)
    handler->done ();

  // hack, let Concurrency know we are done.
  return -2;
}

int
JAWS_Pipeline_Done_Task::handle_put (JAWS_Data_Block *, ACE_Time_Value *)
{
  return 0;
}

