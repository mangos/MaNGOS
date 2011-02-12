// $Id: Reactive_IO.cpp 85419 2009-05-22 10:52:11Z johnnyw $

#include "ace/ACE.h"
#include "ace/OS_NS_unistd.h"
#include "ace/Reactor.h"

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif

#include "jaws3/Jaws_IO.h"
#include "jaws3/Reactive_IO.h"
#include "jaws3/Reactive_IO_Helpers.h"
#include "jaws3/Event_Completer.h"

void
JAWS_Reactive_IO::send ( ACE_HANDLE handle
                       , ACE_Message_Block *mb
                       , JAWS_Event_Completer *completer
                       , const ACE_Time_Value &tv
                       , void *act
                       )
{
  if (mb->length () == 0)
    {
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_OK
                                  , JAWS_Event_Result::JE_SEND_OK
                                  );
      if (completer)
        completer->output_complete (io_result, act);

      return;
    }

  JAWS_IO_Reactive_Send *rs;
  rs = JAWS_IO_Reactive_Send::make (handle, mb, completer, tv, act);

  if (rs == 0)
    {
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_SEND_FAIL
                                  );
      if (completer)
        completer->output_complete (io_result, act);

      return;
    }

  rs->open ();
}

void
JAWS_Reactive_IO::send ( ACE_HANDLE handle
                       , ACE_Message_Block *mb
                       , JAWS_Event_Completer *completer
                       , void *act
                       )
{
  this->send (handle, mb, completer, ACE_Time_Value::zero, act);
}


void
JAWS_Reactive_IO::recv ( ACE_HANDLE handle
                       , ACE_Message_Block *mb
                       , JAWS_Event_Completer *completer
                       , const ACE_Time_Value &tv
                       , void *act
                       )
{

  JAWS_IO_Reactive_Recv *rr;
  rr = JAWS_IO_Reactive_Recv::make (handle, mb, completer, tv, act);

  if (rr == 0)
    {
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_RECV_FAIL
                                  );
      if (completer)
        completer->output_complete (io_result, act);

      return;
    }

  rr->open ();
}


void
JAWS_Reactive_IO::recv ( ACE_HANDLE handle
                       , ACE_Message_Block *mb
                       , JAWS_Event_Completer *completer
                       , void *act
                       )
{
  this->recv (handle, mb, completer, ACE_Time_Value::zero, act);
}


void
JAWS_Reactive_IO::transmit ( ACE_HANDLE handle
                           , ACE_HANDLE source
                           , JAWS_Event_Completer *completer
                           , const ACE_Time_Value &tv
                           , void *act
                           , ACE_Message_Block *header
                           , ACE_Message_Block *trailer
                           )
{
  JAWS_IO_Reactive_Transmit *rt;
  rt = JAWS_IO_Reactive_Transmit::make ( handle
                                       , source
                                       , completer
                                       , tv
                                       , header
                                       , trailer
                                       , act
                                       );

  if (rt == 0)
    {
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_TRANSMIT_FAIL
                                  );
      if (completer)
        completer->output_complete (io_result, act);

      return;
    }

  rt->open ();
}


void
JAWS_Reactive_IO::transmit ( ACE_HANDLE handle
                           , ACE_HANDLE source
                           , JAWS_Event_Completer *completer
                           , void *act
                           , ACE_Message_Block *header
                           , ACE_Message_Block *trailer
                           )
{
  this->transmit ( handle
                 , source
                 , completer
                 , ACE_Time_Value::zero
                 , act
                 , header
                 , trailer
                 );
}


void
JAWS_IO_Reactive_Handler::open (void)
{
  int result = ACE_Reactor::instance ()->notify (this);

  if (result < 0)
    this->close (result);
}

void
JAWS_IO_Reactive_Handler::close (int result)
{
  if (result < 0)
    {
      if (ACE_BIT_ENABLED (this->mask_, ACE_Event_Handler::WRITE_MASK))
        {
          JAWS_Event_Result io_result ( 0
                                      , JAWS_Event_Result::JE_ERROR
                                      , JAWS_Event_Result::JE_SEND_FAIL
                                      );
          this->io_result_ = io_result;
        }
      else if (ACE_BIT_ENABLED (this->mask_, ACE_Event_Handler::READ_MASK))
        {
          JAWS_Event_Result io_result ( 0
                                      , JAWS_Event_Result::JE_ERROR
                                      , JAWS_Event_Result::JE_RECV_FAIL
                                      );
          this->io_result_ = io_result;
        }

      this->handle_close (this->handle_, this->mask_);
    }
}

int
JAWS_IO_Reactive_Handler::handle_timeout (const ACE_Time_Value &, const void *)
{
  if (this->was_active_)
    {
      this->was_active_ = 0;

      this->timer_id_ =
        ACE_Reactor::instance ()->schedule_timer (this, 0, this->tv_);

      return 0;
    }

  ACE_Reactor::instance ()
  ->remove_handler ( this
                   , ACE_Event_Handler::RWE_MASK|ACE_Event_Handler::DONT_CALL
                   );

  this->timer_id_ = -1;

  if (ACE_BIT_ENABLED (this->mask_, ACE_Event_Handler::WRITE_MASK))
    {
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_SEND_TIMEOUT
                                  , ETIME
                                  );
      this->io_result_ = io_result;
    }
  else if (ACE_BIT_ENABLED (this->mask_, ACE_Event_Handler::READ_MASK))
    {
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_RECV_TIMEOUT
                                  , ETIME
                                  );
      this->io_result_ = io_result;
    }

  return -1;
}

int
JAWS_IO_Reactive_Handler::handle_close (ACE_HANDLE, ACE_Reactor_Mask)
{
  if (this->completer_)
    {
      if (ACE_BIT_ENABLED (this->mask_, ACE_Event_Handler::WRITE_MASK))
        this->completer_->output_complete (this->io_result_, this->act_);
      else if (ACE_BIT_ENABLED (this->mask_, ACE_Event_Handler::READ_MASK))
        this->completer_->input_complete (this->io_result_, this->act_);
    }

  ACE_Reactor::instance ()
  ->remove_handler ( this
                   , ACE_Event_Handler::RWE_MASK|ACE_Event_Handler::DONT_CALL
                   );

  delete this;
  return 0;
}

int
JAWS_IO_Reactive_Handler::handle_exception (ACE_HANDLE handle)
{
  if (handle == ACE_INVALID_HANDLE)
    {
      // We are being called back from a notify call.
      // This is our cue to register ourselves with the Reactor.

      int result;
      result =
        ACE_Reactor::instance ()
        ->register_handler (this, this->mask_|ACE_Event_Handler::EXCEPT_MASK);

      if (result < 0)
        this->close (result);

      return 0;
    }

  // back to our regularly scheduled except mask handling.

  if (ACE_BIT_ENABLED (this->mask_, ACE_Event_Handler::WRITE_MASK))
    {
      JAWS_Event_Result io_result ( this->bytes_
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_SEND_SHORT
                                  );
      this->io_result_ = io_result;
    }
  else if (ACE_BIT_ENABLED (this->mask_, ACE_Event_Handler::READ_MASK))
    {
      JAWS_Event_Result io_result ( this->bytes_
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_RECV_SHORT
                                  );
      this->io_result_ = io_result;
    }

  return -1;
}


int
JAWS_IO_Reactive_Send::handle_output (ACE_HANDLE handle)
{
  this->was_active_ = 1;

  ssize_t count = ACE::send ( handle
                            , this->mb_->rd_ptr ()
                            , this->mb_->length ()
                            );

  if (count <= 0 && this->bytes_ == 0)
    {
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_SEND_FAIL
                                  );
      this->io_result_ = io_result;
    }
  else if (count <= 0 && this->bytes_ > 0)
    {
      JAWS_Event_Result io_result ( this->bytes_
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_SEND_SHORT
                                  );
      this->io_result_ = io_result;
    }
  else
    {
      if (count > 0)
        this->mb_->rd_ptr (count);

      this->bytes_ += count;

      JAWS_Event_Result io_result ( this->bytes_
                                  , JAWS_Event_Result::JE_OK
                                  , JAWS_Event_Result::JE_SEND_OK
                                  );
      this->io_result_ = io_result;
    }

  if (count <= 0 || this->mb_->length () == 0)
    return -1;

  // Not done yet, so stay registered.
  return 0;
}


int
JAWS_IO_Reactive_Recv::handle_input (ACE_HANDLE handle)
{
  ssize_t count = ACE::recv ( handle
                            , this->mb_->wr_ptr ()
                            , this->mb_->space ()
                            );

  if (count < 0)
    {
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_RECV_FAIL
                                  );
      this->io_result_ = io_result;
    }
  else
    {
      if (count > 0)
        this->mb_->wr_ptr (count);

      JAWS_Event_Result io_result ( count
                                  , JAWS_Event_Result::JE_OK
                                  , JAWS_Event_Result::JE_RECV_OK
                                  );
      this->io_result_ = io_result;
    }

  return -1;
}


int
JAWS_IO_Reactive_Transmit::handle_output (ACE_HANDLE handle)
{
  this->was_active_ = 1;

  if (this->header_ && this->header_->length () > 0)
    return this->handle_output_header (handle);
  else
    this->header_ = 0;

  if (this->source_ != ACE_INVALID_HANDLE)
    return this->handle_output_source (handle);

  if (this->trailer_ && this->trailer_->length () > 0)
    return this->handle_output_trailer (handle);
  else
    this->trailer_ = 0;

  JAWS_Event_Result io_result ( this->bytes_
                              , JAWS_Event_Result::JE_OK
                              , JAWS_Event_Result::JE_TRANSMIT_OK
                              );
  this->io_result_ = io_result;

  return -1;
}

int
JAWS_IO_Reactive_Transmit::handle_output_header (ACE_HANDLE handle)
{
  return this->handle_output_mb (handle, this->header_);
}

int
JAWS_IO_Reactive_Transmit::handle_output_source (ACE_HANDLE handle)
{
  ACE_Message_Block *mb = this->source_buf_;

  // Try to read data into the mb if data is still available.
  if (mb->space () && this->source_ != ACE_INVALID_HANDLE)
    {
      ssize_t count;
      count = ACE_OS::read (this->source_, mb->wr_ptr (), mb->space ());

      if (count < 0)
        {
          this->source_ = ACE_INVALID_HANDLE;
          this->source_buf_ = 0;

          if (this->bytes_ == 0)
            {
              JAWS_Event_Result io_result ( 0
                                          , JAWS_Event_Result::JE_ERROR
                                          , JAWS_Event_Result::JE_TRANSMIT_FAIL
                                          );
              this->io_result_ = io_result;
            }
          else if (this->bytes_ > 0)
            {
              JAWS_Event_Result io_result ( this->bytes_
                                          , JAWS_Event_Result::JE_ERROR
                                          , JAWS_Event_Result::JE_TRANSMIT_SHORT
                                          );
              this->io_result_ = io_result;
            }

          return -1;
        }
      else if (count == 0)
        this->source_ = ACE_INVALID_HANDLE;
      else
        mb->wr_ptr (count);
    }

  int result = 0;

  if (mb->length () > 0)
    result = this->handle_output_mb (handle, mb);

  if (result < 0)
    {
      this->source_ = ACE_INVALID_HANDLE;
      this->source_buf_ = 0;
    }
  else if (mb == 0 && this->source_ == ACE_INVALID_HANDLE)
    this->source_buf_ = 0;
  else
    this->source_buf_->crunch ();

  return result;
}

int
JAWS_IO_Reactive_Transmit::handle_output_trailer (ACE_HANDLE handle)
{
  int result = this->handle_output_mb (handle, this->trailer_);

  if (result == 0 && this->trailer_ == 0)
    {
      JAWS_Event_Result io_result ( this->bytes_
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_TRANSMIT_SHORT
                                  );
      this->io_result_ = io_result;
      return -1;
    }

  return result;
}

int
JAWS_IO_Reactive_Transmit::handle_output_mb ( ACE_HANDLE handle
                                            , ACE_Message_Block *&mb
                                            )
{
  ssize_t count = ACE::send (handle, mb->rd_ptr (), mb->length ());

  if (count <= 0 && this->bytes_ == 0)
    {
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_TRANSMIT_FAIL
                                  );
      this->io_result_ = io_result;
    }
  else if (count <= 0 && this->bytes_ > 0)
    {
      JAWS_Event_Result io_result ( this->bytes_
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_TRANSMIT_SHORT
                                  );
      this->io_result_ = io_result;
    }
  else
    {
      mb->rd_ptr (count);
      this->bytes_ += count;
    }

  if (count <= 0)
    return -1;

  if (mb->length () == 0)
    mb = 0;

  return 0;
}

void
JAWS_IO_Reactive_Transmit::close (int result)
{
  if (result < 0)
    {
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_TRANSMIT_FAIL
                                  );
      this->io_result_ = io_result;

      this->handle_close (this->handle_, this->mask_);
    }
}

int
JAWS_IO_Reactive_Transmit::handle_timeout (const ACE_Time_Value &, const void *)
{
  if (this->was_active_)
    {
      this->was_active_ = 0;

      this->timer_id_ =
        ACE_Reactor::instance ()->schedule_timer (this, 0, this->tv_);

      return 0;
    }

  ACE_Reactor::instance ()
  ->remove_handler ( this
                   , ACE_Event_Handler::RWE_MASK|ACE_Event_Handler::DONT_CALL
                   );

  this->timer_id_ = -1;

  JAWS_Event_Result io_result ( 0
                              , JAWS_Event_Result::JE_ERROR
                              , JAWS_Event_Result::JE_TRANSMIT_TIMEOUT
                              , ETIME
                              );

  this->io_result_ = io_result;

  return -1;
}

int
JAWS_IO_Reactive_Transmit::handle_exception (ACE_HANDLE handle)
{
  if (handle == ACE_INVALID_HANDLE)
    {
      // We are being called back from a notify call.
      // This is our cue to register ourselves with the Reactor.

      int result;
      result =
        ACE_Reactor::instance ()
        ->register_handler (this, this->mask_|ACE_Event_Handler::EXCEPT_MASK);

      if (result < 0)
        this->close (result);

      return 0;
    }

  // back to our regularly scheduled except mask handling.

  JAWS_Event_Result io_result ( this->bytes_
                              , JAWS_Event_Result::JE_ERROR
                              , JAWS_Event_Result::JE_TRANSMIT_SHORT
                              );
  this->io_result_ = io_result;

  return -1;
}
