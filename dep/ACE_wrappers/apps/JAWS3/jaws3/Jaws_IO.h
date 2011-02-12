/* -*- c++ -*- */
// $Id: Jaws_IO.h 85419 2009-05-22 10:52:11Z johnnyw $

#ifndef JAWS_IO_H
#define JAWS_IO_H

#include "ace/Message_Block.h"
#include "ace/Singleton.h"
#include "ace/Synch_Traits.h"

#include "jaws3/Export.h"


class JAWS_IO;
class JAWS_Event_Completer;


class JAWS_Export JAWS_IO_Impl
// = TITLE
//     IO inteface for the implementation classes on the RHS of the
//     Bridge Pattern.
{
public:

  virtual ~JAWS_IO_Impl (void) {}

  // = Regular IO methods.

  virtual void send ( ACE_HANDLE handle
                    , ACE_Message_Block *mb
                    , JAWS_Event_Completer *completer
                    , void *act = 0
                    ) = 0;

  virtual void recv ( ACE_HANDLE handle
                    , ACE_Message_Block *mb
                    , JAWS_Event_Completer *completer
                    , void *act = 0
                    ) = 0;

  virtual void transmit ( ACE_HANDLE handle
                        , ACE_HANDLE source
                        , JAWS_Event_Completer *completer
                        , void *act = 0
                        , ACE_Message_Block *header = 0
                        , ACE_Message_Block *trailer = 0
                        ) = 0;

  // = Timed IO methods.

  virtual void send ( ACE_HANDLE handle
                    , ACE_Message_Block *mb
                    , JAWS_Event_Completer *completer
                    , const ACE_Time_Value &tv
                    , void *act = 0
                    ) = 0;

  virtual void recv ( ACE_HANDLE handle
                    , ACE_Message_Block *mb
                    , JAWS_Event_Completer *completer
                    , const ACE_Time_Value &tv
                    , void *act = 0
                    ) = 0;

  virtual void transmit ( ACE_HANDLE handle
                        , ACE_HANDLE source
                        , JAWS_Event_Completer *completer
                        , const ACE_Time_Value &tv
                        , void *act = 0
                        , ACE_Message_Block *header = 0
                        , ACE_Message_Block *trailer = 0
                        ) = 0;

};


class JAWS_Export JAWS_IO
{
public:

  JAWS_IO (JAWS_IO_Impl *impl = 0);

  ~JAWS_IO (void);

  static JAWS_IO * instance (void)
  {
    return ACE_Singleton<JAWS_IO, ACE_SYNCH_MUTEX>::instance ();
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


private:

  JAWS_IO_Impl *impl_;

};

#endif /* JAWS_IO_H */
