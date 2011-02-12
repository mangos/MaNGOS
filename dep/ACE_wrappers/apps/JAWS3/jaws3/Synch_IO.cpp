// $Id: Synch_IO.cpp 85419 2009-05-22 10:52:11Z johnnyw $

#include "ace/ACE.h"

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif

#include "jaws3/Jaws_IO.h"
#include "jaws3/Synch_IO.h"
#include "jaws3/Event_Completer.h"

static JAWS_Event_Result
JAWS_synch_send ( ACE_HANDLE handle
                , ACE_Message_Block *mb
                , const ACE_Time_Value *tv = 0
                )
{
  JAWS_Event_Result io_result;

  ssize_t result = ACE::send_n (handle, mb->rd_ptr (), mb->length (), tv);
  if (result < 0)
    {
      if (errno == ETIME)
        {
          JAWS_Event_Result tmp_io_result ( 0
                                          , JAWS_Event_Result::JE_ERROR
                                          , JAWS_Event_Result::JE_SEND_TIMEOUT
                                          );
          io_result = tmp_io_result;
        }
      else
        {
          JAWS_Event_Result tmp_io_result ( 0
                                          , JAWS_Event_Result::JE_ERROR
                                          , JAWS_Event_Result::JE_SEND_FAIL
                                          );
          io_result = tmp_io_result;
        }
    }
  else if ((size_t) result < mb->length ())
    {
      if (result > 0)
        mb->rd_ptr (result);

      JAWS_Event_Result tmp_io_result ( result
                                      , JAWS_Event_Result::JE_ERROR
                                      , JAWS_Event_Result::JE_SEND_SHORT
                                      );
      io_result = tmp_io_result;
    }
  else
    {
      if (result > 0)
        mb->rd_ptr (result);

      JAWS_Event_Result tmp_io_result ( result
                                      , JAWS_Event_Result::JE_OK
                                      , JAWS_Event_Result::JE_SEND_OK
                                      );
      io_result = tmp_io_result;
    }

  return io_result;
}

void
JAWS_Synch_IO::send ( ACE_HANDLE handle
                    , ACE_Message_Block *mb
                    , JAWS_Event_Completer *completer
                    , const ACE_Time_Value &tv
                    , void *act
                    )
{
  JAWS_Event_Result io_result;
  const ACE_Time_Value *tvp = 0;

  if (ACE_Time_Value::zero < tv)
    tvp = &tv;

  io_result = JAWS_synch_send (handle, mb, tvp);

  if (completer)
    completer->output_complete (io_result, act);
}


void
JAWS_Synch_IO::send ( ACE_HANDLE handle
                    , ACE_Message_Block *mb
                    , JAWS_Event_Completer *completer
                    , void *act
                    )
{
  this->send (handle, mb, completer, ACE_Time_Value::zero, act);
}


void
JAWS_Synch_IO::recv ( ACE_HANDLE handle
                    , ACE_Message_Block *mb
                    , JAWS_Event_Completer *completer
                    , const ACE_Time_Value &tv
                    , void *act
                    )
{
  JAWS_Event_Result io_result;
  const ACE_Time_Value *tvp = 0;

  if (ACE_Time_Value::zero < tv)
    tvp = &tv;

  ssize_t result = ACE::recv (handle, mb->wr_ptr (), mb->space (), tvp);
  if (result < 0)
    {
      JAWS_Event_Result tmp_io_result ( 0
                                      , JAWS_Event_Result::JE_ERROR
                                      , JAWS_Event_Result::JE_RECV_FAIL
                                      );
      io_result = tmp_io_result;
    }
  else
    {
      if (result > 0)
        mb->wr_ptr (result);

      JAWS_Event_Result tmp_io_result ( result
                                      , JAWS_Event_Result::JE_OK
                                      , JAWS_Event_Result::JE_RECV_OK
                                      );
      io_result = tmp_io_result;
    }

  if (completer)
    completer->input_complete (io_result, act);
}


void
JAWS_Synch_IO::recv ( ACE_HANDLE handle
                    , ACE_Message_Block *mb
                    , JAWS_Event_Completer *completer
                    , void *act
                    )
{
  this->recv (handle, mb, completer, ACE_Time_Value::zero, act);
}


void
JAWS_Synch_IO::transmit ( ACE_HANDLE handle
                        , ACE_HANDLE source
                        , JAWS_Event_Completer *completer
                        , const ACE_Time_Value &tv
                        , void *act
                        , ACE_Message_Block *header
                        , ACE_Message_Block *trailer
                        )
{
  JAWS_Event_Result io_result;
  const ACE_Time_Value *tvp = 0;

  if (ACE_Time_Value::zero < tv)
    tvp = &tv;

  size_t bytes = 0;

  if (header)
    {
      io_result = JAWS_synch_send (handle, header, tvp);
      bytes += io_result.bytes ();
      if (io_result.status () != JAWS_Event_Result::JE_OK)
        {
          if (completer)
            completer->input_complete (io_result, act);

          return;
        }
    }

  ACE_Message_Block buf (8 * 1024);
  ssize_t len = 0;
  while ((len = ACE::recv (source, buf.wr_ptr (), buf.space (), tvp)) >= 0)
    {
      if (len == 0)
        break;

      buf.wr_ptr (len);
      io_result = JAWS_synch_send (handle, & buf);
      bytes += io_result.bytes ();
      if (io_result.status () != JAWS_Event_Result::JE_OK)
        {
          JAWS_Event_Result tmp_io_result ( bytes
                                          , JAWS_Event_Result::JE_ERROR
                                          , JAWS_Event_Result::JE_SEND_SHORT
                                          );

          if (completer)
            completer->input_complete (tmp_io_result, act);

          return;
        }

      buf.crunch ();
    }

  if (trailer)
    {
      io_result = JAWS_synch_send (handle, trailer, tvp);
      bytes += io_result.bytes ();
      if (io_result.status () != JAWS_Event_Result::JE_OK)
        {
          JAWS_Event_Result tmp_io_result ( bytes
                                          , JAWS_Event_Result::JE_ERROR
                                          , JAWS_Event_Result::JE_SEND_SHORT
                                          );

          if (completer)
            completer->input_complete (tmp_io_result, act);

          return;
        }
    }

  if (len == 0)
    {
      JAWS_Event_Result tmp_io_result ( bytes
                                      , JAWS_Event_Result::JE_OK
                                      , JAWS_Event_Result::JE_SEND_OK
                                      );
      io_result = tmp_io_result;
    }
  else
    {
      JAWS_Event_Result tmp_io_result ( bytes
                                      , JAWS_Event_Result::JE_ERROR
                                      , JAWS_Event_Result::JE_SEND_SHORT
                                      );
      io_result = tmp_io_result;
    }

  if (completer)
    completer->input_complete (io_result, act);
}


void
JAWS_Synch_IO::transmit ( ACE_HANDLE handle
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

