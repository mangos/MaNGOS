// $Id: Asynch_IO.cpp 85419 2009-05-22 10:52:11Z johnnyw $

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif /*JAWS_BUILD_DLL*/

#include "ace/config-lite.h"

#include "jaws3/Jaws_IO.h"
#include "jaws3/Asynch_IO.h"
#include "jaws3/Event_Completer.h"
#include "jaws3/Event_Result.h"

#if defined (ACE_HAS_AIO_CALLS) || defined (ACE_HAS_WIN32_OVERLAPPED_IO)

#include "jaws3/Asynch_IO_Helpers.h"

void
JAWS_Asynch_IO::send ( ACE_HANDLE handle
                     , ACE_Message_Block *mb
                     , JAWS_Event_Completer *completer
                     , void *act
                     )
{
  JAWS_EC_AH_Adapter *jecaha;
  jecaha = JAWS_EC_AH_Adapter::make (completer);

  ACE_Asynch_Write_Stream asynch_write_stream;

  if (jecaha == 0
      || asynch_write_stream.open (*jecaha, handle) == -1
      || asynch_write_stream.write (*mb, mb->length (), act) == -1)
    {
      delete jecaha;
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_SEND_FAIL
                                  );

      if (completer)
        completer->output_complete (io_result, act);
    }
}


void
JAWS_Asynch_IO::recv ( ACE_HANDLE handle
                     , ACE_Message_Block *mb
                     , JAWS_Event_Completer *completer
                     , void *act
                     )
{
  JAWS_EC_AH_Adapter *jecaha;
  jecaha = JAWS_EC_AH_Adapter::make (completer);

  ACE_Asynch_Read_Stream asynch_read_stream;

  if (jecaha == 0
      || asynch_read_stream.open (*jecaha, handle) == -1
      || asynch_read_stream.read (*mb, mb->space (), act) == -1)
    {
      delete jecaha;
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_RECV_FAIL
                                  );

      if (completer)
        completer->output_complete (io_result, act);
    }
}


void
JAWS_Asynch_IO::transmit ( ACE_HANDLE handle
                         , ACE_HANDLE source
                         , JAWS_Event_Completer *completer
                         , void *act
                         , ACE_Message_Block *header
                         , ACE_Message_Block *trailer
                         )
{
  JAWS_EC_AH_Adapter *jecaha;
  jecaha = JAWS_EC_AH_Adapter::make (completer);

  ACE_Asynch_Transmit_File::Header_And_Trailer *header_and_trailer = 0;
  header_and_trailer =
    new ACE_Asynch_Transmit_File::Header_And_Trailer ( header
                                                     , header->length ()
                                                     , trailer
                                                     , trailer->length ()
                                                     );

  ACE_Asynch_Transmit_File asynch_transmit_file;

  if (source == ACE_INVALID_HANDLE
      || jecaha == 0
      || header_and_trailer == 0
      || asynch_transmit_file.open (*jecaha, handle) == -1
      || asynch_transmit_file.transmit_file ( source
                                            , header_and_trailer
                                            , 0
                                            , 0
                                            , 0
                                            , 0
                                            , 0
                                            , act
                                            ) == -1)
    {
      delete jecaha;
      delete header_and_trailer;
      JAWS_Event_Result io_result ( 0
                                  , JAWS_Event_Result::JE_ERROR
                                  , JAWS_Event_Result::JE_TRANSMIT_FAIL
                                  );

      if (completer)
        completer->output_complete (io_result, act);
    }
}



JAWS_EC_AH_Adapter *
JAWS_EC_AH_Adapter::make (JAWS_Event_Completer *completer)
{
  return new JAWS_EC_AH_Adapter (completer);
}

void
JAWS_EC_AH_Adapter
::handle_read_stream (const ACE_Asynch_Read_Stream::Result &result)
{
  JAWS_Event_Result io_result;

  io_result = this->make_io_result ( result
                                   , JAWS_Event_Result::JE_RECV_OK
                                   , JAWS_Event_Result::JE_RECV_FAIL
                                   );
  // More useful diagnostics not implemented yet.

  void *act = const_cast<void *> (result.act ());

  this->completer_->input_complete (io_result, act);
  delete this;
}

void
JAWS_EC_AH_Adapter
::handle_write_stream (const ACE_Asynch_Write_Stream::Result &result)
{
  JAWS_Event_Result io_result;

  io_result = this->make_io_result ( result
                                   , JAWS_Event_Result::JE_SEND_OK
                                   , JAWS_Event_Result::JE_SEND_FAIL
                                   );
  // More useful diagnostics not implemented yet.

  void *act = const_cast<void *> (result.act ());

  this->completer_->output_complete (io_result, act);
  delete this;
}

void
JAWS_EC_AH_Adapter
::handle_transmit_file (const ACE_Asynch_Transmit_File::Result &result)
{
  JAWS_Event_Result io_result;

  io_result = this->make_io_result ( result
                                   , JAWS_Event_Result::JE_TRANSMIT_OK
                                   , JAWS_Event_Result::JE_TRANSMIT_FAIL
                                   );
  // More useful diagnostics not implemented yet.
  // Watch out for files not opened in overlapped IO mode.

  void *act = const_cast<void *> (result.act ());

  this->completer_->output_complete (io_result, act);
  delete this;
}

JAWS_Event_Result
JAWS_EC_AH_Adapter
::make_io_result ( const ACE_Asynch_Result &result
                 , JAWS_Event_Result::JE_REASON reason_ok
                 , JAWS_Event_Result::JE_REASON reason_fail
                 )
{
  size_t bytes = result.bytes_transferred ();

  JAWS_Event_Result::JE_STATUS status;
  JAWS_Event_Result::JE_REASON reason;

  if (result.success ())
    {
      status = JAWS_Event_Result::JE_OK;
      reason = reason_ok;
    }
  else
    {
      status = JAWS_Event_Result::JE_ERROR;
      reason = reason_fail;
    }

  JAWS_Event_Result io_result (bytes, status, reason);

  return io_result;
}

#else /* EMULATE AIO WITH REACTOR */

#include "jaws3/Reactive_IO.h"

void
JAWS_Asynch_IO::send ( ACE_HANDLE handle
                     , ACE_Message_Block *mb
                     , JAWS_Event_Completer *completer
                     , void *act
                     )
{
  JAWS_Reactive_IO::instance ()->send (handle, mb, completer, act);
}


void
JAWS_Asynch_IO::recv ( ACE_HANDLE handle
                     , ACE_Message_Block *mb
                     , JAWS_Event_Completer *completer
                     , void *act
                     )
{
  JAWS_Reactive_IO::instance ()->recv (handle, mb, completer, act);
}


void
JAWS_Asynch_IO::transmit ( ACE_HANDLE handle
                         , ACE_HANDLE source
                         , JAWS_Event_Completer *completer
                         , void *act
                         , ACE_Message_Block *header
                         , ACE_Message_Block *trailer
                         )
{
  JAWS_Reactive_IO::instance ()->transmit ( handle
                                          , source
                                          , completer
                                          , act
                                          , header
                                          , trailer
                                          );
}

#endif /* ACE_HAS_AIO_CALLS || ACE_HAS_WIN32_OVERLAPPED_IO */

// For now, we will simulate timed Asynch IO with timed Reactive IO.
// In the future, we will implement the timed Asynch IO with timers
// and Asynch IO cancelation.

#include "jaws3/Reactive_IO.h"

void
JAWS_Asynch_IO::send ( ACE_HANDLE handle
                     , ACE_Message_Block *mb
                     , JAWS_Event_Completer *completer
                     , const ACE_Time_Value &tv
                     , void *act
                     )
{
  JAWS_Reactive_IO::instance ()->send (handle, mb, completer, tv, act);
}


void
JAWS_Asynch_IO::recv ( ACE_HANDLE handle
                     , ACE_Message_Block *mb
                     , JAWS_Event_Completer *completer
                     , const ACE_Time_Value &tv
                     , void *act
                     )
{
  JAWS_Reactive_IO::instance ()->recv (handle, mb, completer, tv, act);
}


void
JAWS_Asynch_IO::transmit ( ACE_HANDLE handle
                         , ACE_HANDLE source
                         , JAWS_Event_Completer *completer
                         , const ACE_Time_Value &tv
                         , void *act
                         , ACE_Message_Block *header
                         , ACE_Message_Block *trailer
                         )
{
  JAWS_Reactive_IO::instance ()->transmit ( handle
                                          , source
                                          , completer
                                          , tv
                                          , act
                                          , header
                                          , trailer
                                          );
}

