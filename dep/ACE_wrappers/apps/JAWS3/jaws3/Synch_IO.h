/* -*- c++ -*- */
// $Id: Synch_IO.h 85419 2009-05-22 10:52:11Z johnnyw $

#ifndef JAWS_SYNCH_IO_H
#define JAWS_SYNCH_IO_H

#include "ace/Singleton.h"

#include "jaws3/Jaws_IO.h"

class JAWS_Export JAWS_Synch_IO : public JAWS_IO_Impl
{
public:

  static JAWS_Synch_IO * instance (void)
  {
    return ACE_Singleton<JAWS_Synch_IO, ACE_SYNCH_MUTEX>::instance ();
  }

  void send ( ACE_HANDLE handle
            , ACE_Message_Block *mb
            , JAWS_Event_Completer *completer
            , void *act = 0
            );

  void recv ( ACE_HANDLE handle
            , ACE_Message_Block *mb
            , JAWS_Event_Completer *completer
            , void *act = 0
            );

  void transmit ( ACE_HANDLE handle
                , ACE_HANDLE source
                , JAWS_Event_Completer *completer
                , void *act = 0
                , ACE_Message_Block *header = 0
                , ACE_Message_Block *trailer = 0
                );

  void send ( ACE_HANDLE handle
            , ACE_Message_Block *mb
            , JAWS_Event_Completer *completer
            , const ACE_Time_Value &tv
            , void *act = 0
            );

  void recv ( ACE_HANDLE handle
            , ACE_Message_Block *mb
            , JAWS_Event_Completer *completer
            , const ACE_Time_Value &tv
            , void *act = 0
            );

  void transmit ( ACE_HANDLE handle
                , ACE_HANDLE source
                , JAWS_Event_Completer *completer
                , const ACE_Time_Value &tv
                , void *act = 0
                , ACE_Message_Block *header = 0
                , ACE_Message_Block *trailer = 0
                );

};

#endif /* JAWS_SYNCH_IO_H */
