/* -*- c++ -*- */
// $Id: Asynch_IO_Helpers.h 85419 2009-05-22 10:52:11Z johnnyw $

#ifndef JAWS_ASYNCH_IO_HELPERS_H
#define JAWS_ASYNCH_IO_HELPERS_H

#include "ace/Asynch_IO.h"
#include "ace/Singleton.h"

#include "jaws3/Jaws_IO.h"
#include "jaws3/Event_Result.h"
#include "jaws3/Event_Completer.h"

class JAWS_Asynch_IO;

class JAWS_EC_AH_Adapter : public ACE_Handler
// = TITLE
//     A JAWS_Event_Completer - ACE_Handler adapter.
//
// = DESCRIPTION
//     This class will be conditionally #included by jaws3/Asynch_IO.cpp
//     if the given platform supports POSIX or WIN32 asynch IO.
{
public:

  static JAWS_EC_AH_Adapter * make (JAWS_Event_Completer *);

  void handle_read_stream (const ACE_Asynch_Read_Stream::Result &result);

  void handle_write_stream (const ACE_Asynch_Write_Stream::Result &result);

  void handle_transmit_file (const ACE_Asynch_Transmit_File::Result &result);

private:

  JAWS_EC_AH_Adapter (JAWS_Event_Completer *completer)
    : completer_ (completer)
  {
  }

protected:

  JAWS_Event_Result make_io_result ( const ACE_Asynch_Result &result
                                   , JAWS_Event_Result::JE_REASON reason_ok
                                   , JAWS_Event_Result::JE_REASON reason_fail
                                   );

private:

  JAWS_Event_Completer *completer_;

};

#endif /* JAWS_ASYNCH_IO_HELPERS_H */
