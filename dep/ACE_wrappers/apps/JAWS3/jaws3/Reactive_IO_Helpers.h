/* -*- c++ -*- */
// $Id: Reactive_IO_Helpers.h 85419 2009-05-22 10:52:11Z johnnyw $

#ifndef JAWS_REACTIVE_IO_HELPERS_H
#define JAWS_REACTIVE_IO_HELPERS_H

#include "ace/Event_Handler.h"
#include "ace/Message_Block.h"
#include "ace/Singleton.h"
#include "ace/Time_Value.h"

#include "jaws3/Export.h"
#include "jaws3/Jaws_IO.h"
#include "jaws3/Event_Result.h"
#include "jaws3/Event_Completer.h"

class JAWS_Reactive_IO;

class JAWS_IO_Reactive_Handler : public ACE_Event_Handler
{

  friend class JAWS_IO_Reactive_Send;
  friend class JAWS_IO_Reactive_Recv;
  friend class JAWS_IO_Reactive_Transmit;

public:

  virtual void open (void);

  int handle_timeout (const ACE_Time_Value &, const void *);

  int handle_close (ACE_HANDLE, ACE_Reactor_Mask);

  int handle_exception (ACE_HANDLE);

  ACE_HANDLE get_handle (void) const { return this->handle_; }

  void set_handle (ACE_HANDLE handle) { this->handle_ = handle; }

protected:

  virtual void close (int result);

private:

  JAWS_IO_Reactive_Handler ( ACE_HANDLE handle
                           , JAWS_Event_Completer *completer
                           , const ACE_Time_Value &tv
                           , void *act
                           , ACE_Reactor_Mask mask
                           )
    : bytes_ (0)
    , handle_ (handle)
    , completer_ (completer)
    , tv_ (tv)
    , act_ (act)
    , mask_ (mask)
    , timer_id_ (-1)
    , was_active_ (0)
  {
    if (ACE_Time_Value::zero < this->tv_)
      this->timer_id_ =
        ACE_Reactor::instance ()->schedule_timer (this, 0, this->tv_);
  }

public: // needed for destructor due to "aCC: HP ANSI C++ B3910B A.03.39" compiler bug

  ~JAWS_IO_Reactive_Handler (void)
  {
    if (this->timer_id_ != -1)
      ACE_Reactor::instance ()->cancel_timer (this->timer_id_);
  }

private:

  JAWS_Event_Result io_result_;

  size_t bytes_;
  ACE_HANDLE handle_;
  JAWS_Event_Completer *completer_;
  ACE_Time_Value tv_;
  void *act_;

  ACE_Reactor_Mask mask_;
  long timer_id_;

  int was_active_;

};


class JAWS_IO_Reactive_Send : public JAWS_IO_Reactive_Handler
{

  friend class JAWS_Reactive_IO;

public:

  int handle_output (ACE_HANDLE handle);

  static JAWS_IO_Reactive_Send * make ( ACE_HANDLE handle
                                      , ACE_Message_Block *mb
                                      , JAWS_Event_Completer *completer
                                      , void *act
                                      )
  {
    return new JAWS_IO_Reactive_Send ( handle
                                     , mb
                                     , completer
                                     , ACE_Time_Value::zero
                                     , act
                                     , ACE_Event_Handler::WRITE_MASK
                                     );
  }

  static JAWS_IO_Reactive_Send * make ( ACE_HANDLE handle
                                      , ACE_Message_Block *mb
                                      , JAWS_Event_Completer *completer
                                      , const ACE_Time_Value &tv
                                      , void *act
                                      )
  {
    return new JAWS_IO_Reactive_Send ( handle
                                     , mb
                                     , completer
                                     , tv
                                     , act
                                     , ACE_Event_Handler::WRITE_MASK
                                     );
  }

private:

  JAWS_IO_Reactive_Send ( ACE_HANDLE handle
                        , ACE_Message_Block *mb
                        , JAWS_Event_Completer *completer
                        , const ACE_Time_Value &tv
                        , void *act
                        , ACE_Reactor_Mask mask
                        )
    : JAWS_IO_Reactive_Handler (handle, completer, tv, act, mask)
    , mb_ (mb)
  {
  }

private:

  ACE_Message_Block *mb_;

};


class JAWS_IO_Reactive_Recv : public JAWS_IO_Reactive_Handler
{

  friend class JAWS_Reactive_IO;

public:

  int handle_input (ACE_HANDLE handle);

  static JAWS_IO_Reactive_Recv * make ( ACE_HANDLE handle
                                      , ACE_Message_Block *mb
                                      , JAWS_Event_Completer *completer
                                      , void *act
                                      )
  {
    return new JAWS_IO_Reactive_Recv ( handle
                                     , mb
                                     , completer
                                     , ACE_Time_Value::zero
                                     , act
                                     , ACE_Event_Handler::READ_MASK
                                     );
  }

  static JAWS_IO_Reactive_Recv * make ( ACE_HANDLE handle
                                      , ACE_Message_Block *mb
                                      , JAWS_Event_Completer *completer
                                      , const ACE_Time_Value &tv
                                      , void *act
                                      )
  {
    return new JAWS_IO_Reactive_Recv ( handle
                                     , mb
                                     , completer
                                     , tv
                                     , act
                                     , ACE_Event_Handler::READ_MASK
                                     );
  }

private:

  JAWS_IO_Reactive_Recv ( ACE_HANDLE handle
                        , ACE_Message_Block *mb
                        , JAWS_Event_Completer *completer
                        , const ACE_Time_Value &tv
                        , void *act
                        , ACE_Reactor_Mask mask
                        )
    : JAWS_IO_Reactive_Handler (handle, completer, tv, act, mask)
    , mb_ (mb)
  {
  }

private:

  ACE_Message_Block *mb_;

};


class JAWS_IO_Reactive_Transmit : public JAWS_IO_Reactive_Handler
{

  friend class JAWS_Reactive_IO;

public:

  int handle_timeout (const ACE_Time_Value &, const void *);

  int handle_output (ACE_HANDLE handle);

  int handle_exception (ACE_HANDLE handle);

  static JAWS_IO_Reactive_Transmit * make ( ACE_HANDLE handle
                                          , ACE_HANDLE source
                                          , JAWS_Event_Completer *completer
                                          , ACE_Message_Block *header
                                          , ACE_Message_Block *trailer
                                          , void *act
                                          )
  {
    return new JAWS_IO_Reactive_Transmit ( handle
                                         , source
                                         , completer
                                         , ACE_Time_Value::zero
                                         , header
                                         , trailer
                                         , act
                                         , ACE_Event_Handler::WRITE_MASK
                                         );
  }

  static JAWS_IO_Reactive_Transmit * make ( ACE_HANDLE handle
                                          , ACE_HANDLE source
                                          , JAWS_Event_Completer *completer
                                          , const ACE_Time_Value &tv
                                          , ACE_Message_Block *header
                                          , ACE_Message_Block *trailer
                                          , void *act
                                          )
  {
    return new JAWS_IO_Reactive_Transmit ( handle
                                         , source
                                         , completer
                                         , tv
                                         , header
                                         , trailer
                                         , act
                                         , ACE_Event_Handler::WRITE_MASK
                                         );
  }

protected:

  void close (int result);

  int handle_output_header (ACE_HANDLE handle);

  int handle_output_source (ACE_HANDLE handle);

  int handle_output_trailer (ACE_HANDLE handle);

  int handle_output_mb (ACE_HANDLE handle, ACE_Message_Block *&mb);

private:

  JAWS_IO_Reactive_Transmit ( ACE_HANDLE handle
                            , ACE_HANDLE source
                            , JAWS_Event_Completer *completer
                            , const ACE_Time_Value &tv
                            , ACE_Message_Block *header
                            , ACE_Message_Block *trailer
                            , void *act
                            , ACE_Reactor_Mask mask
                            )
    : JAWS_IO_Reactive_Handler (handle, completer, tv, act, mask)
    , source_ (source)
    , source_mb_ (8 * 1024)
    , source_buf_ (& this->source_mb_)
    , header_ (header)
    , trailer_ (trailer)
  {
  }

private:

  ACE_HANDLE source_;
  ACE_Message_Block source_mb_;
  ACE_Message_Block *source_buf_;
  ACE_Message_Block *header_;
  ACE_Message_Block *trailer_;

};

#endif /* JAWS_REACTIVE_IO_HELPERS_H */
