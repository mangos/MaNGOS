// $Id: IO_Handler.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "ace/Proactor.h"
#include "ace/Filecache.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_sys_socket.h"

#include "JAWS/Jaws_IO.h"
#include "JAWS/IO_Handler.h"
#include "JAWS/IO_Acceptor.h"
#include "JAWS/Data_Block.h"
#include "JAWS/Policy.h"
#include "JAWS/Waiter.h"
#include "JAWS/Filecache.h"



JAWS_Abstract_IO_Handler::~JAWS_Abstract_IO_Handler (void)
{
}

JAWS_IO_Handler_Factory::~JAWS_IO_Handler_Factory (void)
{
}

JAWS_IO_Handler *
JAWS_IO_Handler_Factory::create_io_handler (void)
{
  JAWS_TRACE ("JAWS_IO_Handler_Factory::create");

  JAWS_IO_Handler *handler;
  handler = new JAWS_IO_Handler (this);

  return handler;
}

void
JAWS_IO_Handler_Factory::destroy_io_handler (JAWS_IO_Handler *handler)
{
  JAWS_TRACE ("JAWS_IO_Handler_Factory::destroy");
  if (handler != 0)
    {
      delete handler->message_block ();
      delete handler;
    }
}

JAWS_IO_Handler::JAWS_IO_Handler (JAWS_IO_Handler_Factory *factory)
  : status_ (0),
    mb_ (0),
    handle_ (ACE_INVALID_HANDLE),
    task_ (0),
    factory_ (factory)
{
}

JAWS_IO_Handler::~JAWS_IO_Handler (void)
{
  this->mb_ = 0;
  this->status_ = 0;
  this->task_ = 0;
  this->factory_ = 0;

  ACE_OS::closesocket (this->handle_);
  this->handle_ = ACE_INVALID_HANDLE;
}

void
JAWS_IO_Handler::accept_complete (ACE_HANDLE handle)
{
  // callback into pipeline task, notify that the accept has completed
  this->handle_ = handle;
  this->status_ |= ACCEPT_OK;
  this->status_ &= (ACCEPT_OK+1);

  JAWS_Dispatch_Policy *policy = this->mb_->policy ();

  // Do this so that Thread Per Request can spawn a new thread
  policy->concurrency ()->activate_hook ();
}

void
JAWS_IO_Handler::accept_error (void)
{
  // callback into pipeline task, notify that the accept has failed
  this->status_ |= ACCEPT_ERROR;
  this->status_ &= (ACCEPT_ERROR+1);
}

void
JAWS_IO_Handler::read_complete (ACE_Message_Block *data)
{
  ACE_UNUSED_ARG (data);
  // We can call back into the pipeline task at this point
  // this->pipeline_->read_complete (data);
  this->status_ |= READ_OK;
  this->status_ &= (READ_OK+1);
}

void
JAWS_IO_Handler::read_error (void)
{
  // this->pipeline_->read_error ();
  this->status_ |= READ_ERROR;
  this->status_ &= (READ_ERROR+1);
}

void
JAWS_IO_Handler::transmit_file_complete (void)
{
  JAWS_TRACE ("JAWS_IO_Handler::transmit_file_complete");
  // this->pipeline_->transmit_file_complete ();
  this->status_ |= TRANSMIT_OK;
  this->status_ &= (TRANSMIT_OK+1);
}

void
JAWS_IO_Handler::transmit_file_error (int result)
{
  JAWS_TRACE ("JAWS_IO_Handler::transmit_file_error");
  ACE_UNUSED_ARG (result);
  // this->pipeline_->transmit_file_complete (result);
  this->status_ |= TRANSMIT_ERROR;
  this->status_ &= (TRANSMIT_ERROR+1);
}

void
JAWS_IO_Handler::receive_file_complete (void)
{
  this->status_ |= RECEIVE_OK;
  this->status_ &= (RECEIVE_OK+1);
}

void
JAWS_IO_Handler::receive_file_error (int result)
{
  ACE_UNUSED_ARG(result);
  this->status_ |= RECEIVE_ERROR;
  this->status_ &= (RECEIVE_ERROR+1);
}

void
JAWS_IO_Handler::write_error (void)
{
  ACE_DEBUG ((LM_DEBUG, " (%t) error in writing response\n"));

  this->status_ |= WRITE_ERROR;
  this->status_ &= (WRITE_ERROR+1);
  this->done ();
}

void
JAWS_IO_Handler::confirmation_message_complete (void)
{
  this->status_ |= WRITE_OK;
  this->status_ &= (WRITE_OK+1);
}

void
JAWS_IO_Handler::error_message_complete (void)
{
  this->status_ |= WRITE_OK;
  this->status_ &= (WRITE_OK+1);
}

JAWS_IO_Handler_Factory *
JAWS_IO_Handler::factory (void)
{
  return this->factory_;
}

ACE_HANDLE
JAWS_IO_Handler::handle (void) const
{
  return this->handle_;
}

void
JAWS_IO_Handler::task (JAWS_Pipeline_Handler *ph)
{
  this->task_ = ph;
}

JAWS_Pipeline_Handler *
JAWS_IO_Handler::task (void)
{
  return this->task_;
}

void
JAWS_IO_Handler::message_block (JAWS_Data_Block *mb)
{
  this->mb_ = mb;
}

JAWS_Data_Block *
JAWS_IO_Handler::message_block (void)
{
  return this->mb_;
}

void
JAWS_IO_Handler::done (void)
{
  this->factory ()->destroy_io_handler (this);
}

int
JAWS_IO_Handler::status (void)
{
  return this->status_;
}

void
JAWS_IO_Handler::idle (void)
{
  this->status_ &= (IDLE+1);
}

void
JAWS_IO_Handler::acquire (void)
{
}

void
JAWS_IO_Handler::lock (void)
{
}

void
JAWS_IO_Handler::release (void)
{
}

#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)

JAWS_Asynch_IO_Handler_Factory::~JAWS_Asynch_IO_Handler_Factory (void)
{
}

JAWS_IO_Handler *
JAWS_Asynch_IO_Handler_Factory::create_io_handler (void)
{
  JAWS_TRACE ("JAWS_Asynch_IO_Handler_Factory::create");

  JAWS_Asynch_IO_Handler *handler = 0;
  handler = new JAWS_Asynch_IO_Handler (this);

  return handler;
}

void
JAWS_Asynch_IO_Handler_Factory::destroy_io_handler (JAWS_IO_Handler *handler)
{
  JAWS_TRACE ("JAWS_IO_Handler_Factory::destroy");

  if (handler != 0)
    {
      //cerr << "(" << thr_self () << ") locking for destruction: " << handler << endl;
      handler->lock ();
      delete handler->message_block ();
      handler->message_block (0);
      delete handler;
    }
}


JAWS_Asynch_IO_Handler::JAWS_Asynch_IO_Handler (JAWS_Asynch_IO_Handler_Factory *factory)
  : JAWS_IO_Handler (factory),
    handler_ (0)
{
  this->status_ = 1;
}

JAWS_Asynch_IO_Handler::~JAWS_Asynch_IO_Handler (void)
{
  delete this->handler_;
  this->handler_ = 0;
}

ACE_Handler *
JAWS_Asynch_IO_Handler::handler (void)
{
  return this->handler_;
}

void
JAWS_Asynch_IO_Handler::acquire (void)
{
  //cerr << "(" << thr_self () << ") acquire handler: " << this << endl;
  this->count_.acquire_read ();
}

void
JAWS_Asynch_IO_Handler::lock (void)
{
  //cerr << "(" << thr_self () << ") locking handler: " << this << endl;
  this->count_.acquire_write ();
}

void
JAWS_Asynch_IO_Handler::release (void)
{
  //cerr << "(" << thr_self () << ") release handler: " << this << endl;
  this->count_.release ();
}

JAWS_Asynch_Handler::JAWS_Asynch_Handler (void)
  : ioh_ (0)
{
  this->proactor (ACE_Proactor::instance ());
}

JAWS_Asynch_Handler::~JAWS_Asynch_Handler (void)
{
}

void
JAWS_Asynch_Handler::open (ACE_HANDLE h,
                           ACE_Message_Block &mb)
{
  JAWS_TRACE ("JAWS_Asynch_Handler::open");

  // This currently does nothing, but just in case.
  ACE_Service_Handler::open (h, mb);

  // ioh_ set from the ACT hopefully
  //this->dispatch_handler ();

#if !defined (ACE_WIN32)
  // Assume at this point there is no data.
  mb.rd_ptr (mb.wr_ptr ());
  mb.crunch ();
#else
  // AcceptEx reads some initial data from the socket.
  this->handler ()->message_block ()->copy (mb.rd_ptr (), mb.length ());
#endif

  ACE_Asynch_Accept_Result_Impl *fake_result
    = ACE_Proactor::instance ()->create_asynch_accept_result
      (this->proxy (), JAWS_IO_Asynch_Acceptor_Singleton::instance ()->get_handle (),
       h, mb, JAWS_Data_Block::JAWS_DATA_BLOCK_SIZE,
       this->ioh_, ACE_INVALID_HANDLE, 0);

  this->handler ()->handler_ = this;

  fake_result->complete (0, 1, 0);
}

void
JAWS_Asynch_Handler::act (const void *act_ref)
{
  JAWS_TRACE ("JAWS_Asynch_Handler::act");

  // Set the ioh from the act
  this->ioh_ = (JAWS_Asynch_IO_Handler *) act_ref;
}

#if 0
ACE_HANDLE
JAWS_Asynch_Handler::handle (void) const
{
  return this->ioh_->handle ();
}
#endif

void
JAWS_Asynch_Handler::dispatch_handler (void)
{
#if 0
  // A future version of ACE will support this.
  ACE_Thread_ID tid = ACE_OS::thr_self ();
#else
  // Do it this way for now
  ACE_thread_t thr_name;
  thr_name = ACE_OS::thr_self ();

  JAWS_Thread_ID tid (thr_name);
#endif /* 0 */

  JAWS_IO_Handler **iohref = JAWS_Waiter_Singleton::instance ()->find (tid);

  *iohref = this->handler ();
}

void
JAWS_Asynch_Handler::handle_read_stream (const ACE_Asynch_Read_Stream::Result
                                         &result)
{
  JAWS_TRACE ("JAWS_Asynch_Handler::handle_read_stream");

  this->dispatch_handler ();

  if (result.act () != 0)
    {
      // This callback is for io->receive_file()
      JAWS_TRACE ("JAWS_Asynch_Handler::handle_read_stream (recv_file)");

      int code = 0;
      if (result.success () && result.bytes_transferred () != 0)
        {
          if (result.message_block ().length ()
              == result.message_block ().size ())
            code = ACE_Filecache_Handle::ACE_SUCCESS;
          else
            {
              ACE_Asynch_Read_Stream ar;
              if (ar.open (*this, this->handler ()->handle ()) == -1
                  || ar.read (result.message_block (),
                              result.message_block ().size ()
                              - result.message_block ().length (),
                              result.act ()) == -1)
                code = -1;
              else
                return;
            }
        }
      else
        code = -1;

      if (code == ACE_Filecache_Handle::ACE_SUCCESS)
        this->handler ()->receive_file_complete ();
      else
        this->handler ()->receive_file_error (code);

      result.message_block ().release ();
      delete (ACE_Filecache_Handle *) result.act ();
    }
  else
    {
      // This callback is for this->read()
      JAWS_TRACE ("JAWS_Asynch_Handler::handle_read_stream (read)");

      if (result.success ()
          && result.bytes_transferred () != 0)
        this->handler ()->read_complete (&result.message_block ());
      else
        this->handler ()->read_error ();
    }
}

void
JAWS_Asynch_Handler::handle_write_stream (const ACE_Asynch_Write_Stream::Result
                                          &result)
{
  this->dispatch_handler ();

  result.message_block ().release ();

  if (result.act () == (void *) JAWS_Asynch_IO::CONFIRMATION)
    this->handler ()->confirmation_message_complete ();
  else
    this->handler ()->error_message_complete ();
}

void
JAWS_Asynch_Handler::handle_transmit_file (const
                                           ACE_Asynch_Transmit_File::Result
                                           &result)
{
  this->dispatch_handler ();

  if (result.success ())
    this->handler ()->transmit_file_complete ();
  else
    this->handler ()->transmit_file_error (-1);

  result.header_and_trailer ()->header ()->release ();
  result.header_and_trailer ()->trailer ()->release ();
  delete result.header_and_trailer ();
  delete (JAWS_Cached_FILE *) result.act ();
}

void
JAWS_Asynch_Handler::handle_accept (const ACE_Asynch_Accept::Result &result)
{
  JAWS_TRACE ("JAWS_Asynch_Handler::handle_accept");
  this->dispatch_handler ();

  if (result.success ())
    {
      JAWS_TRACE ("JAWS_Asynch_Handler::handle_accept, success");
      this->handler ()->accept_complete (result.accept_handle ());
    }
  else
    this->handler ()->accept_error ();

}

void
JAWS_Asynch_Handler::handler (JAWS_Asynch_IO_Handler *ioh)
{
  this->ioh_ = ioh;
}

JAWS_Asynch_IO_Handler *
JAWS_Asynch_Handler::handler (void)
{
  return this->ioh_;
}

#endif /* ACE_HAS_WIN32_OVERLAPPED_IO || ACE_HAS_AIO_CALLS */
