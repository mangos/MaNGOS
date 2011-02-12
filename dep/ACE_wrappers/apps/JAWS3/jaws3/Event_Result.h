/* -*- c++ -*- */
// $Id: Event_Result.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_EVENT_RESULT_H
#define JAWS_EVENT_RESULT_H

#include "ace/OS_NS_errno.h"
#include "ace/os_include/os_stddef.h"   /* Needed for size_t */

#include "jaws3/Export.h"

class JAWS_Event_Result;

class JAWS_Export JAWS_Event_Result
{
public:

  enum JE_STATUS
  { JE_IDLE
  , JE_OK
  , JE_PENDING
  , JE_CANCEL
  , JE_ERROR
  };

  enum JE_REASON
  { JE_NO_REASON
  , JE_ACCEPT_OK
  , JE_ACCEPT_PENDING
  , JE_ACCEPT_TIMEOUT
  , JE_ACCEPT_FAIL
  , JE_CONNECT_OK
  , JE_CONNECT_PENDING
  , JE_CONNECT_TIMEOUT
  , JE_CONNECT_FAIL
  , JE_SEND_OK
  , JE_SEND_PENDING
  , JE_SEND_BLOCKED
  , JE_SEND_SHORT
  , JE_SEND_TIMEOUT
  , JE_SEND_FAIL
  , JE_RECV_OK
  , JE_RECV_PENDING
  , JE_RECV_BLOCKED
  , JE_RECV_SHORT
  , JE_RECV_TIMEOUT
  , JE_RECV_FAIL
  , JE_TRANSMIT_OK
  , JE_TRANSMIT_PENDING
  , JE_TRANSMIT_BLOCKED
  , JE_TRANSMIT_SHORT
  , JE_TRANSMIT_TIMEOUT
  , JE_TRANSMIT_FAIL
  , JE_TIMER_OK
  , JE_TIMER_CANCEL
  , JE_TIMER_PENDING
  , JE_TIMER_FAIL
  , JE_LAMBDA_OK
  , JE_LAMBDA_CANCEL
  , JE_LAMBDA_PENDING
  , JE_LAMBDA_SHORT
  , JE_LAMBDA_TIMEOUT
  , JE_LAMBDA_FAIL
  };

  JAWS_Event_Result ( size_t bytes = 0
                    , JE_STATUS status = JE_IDLE
                    , JE_REASON reason = JE_NO_REASON
                    , int error_number = -1
                    , void * data = 0
                    )
    : bytes_ (bytes)
    , status_ (status)
    , reason_ (reason)
    , error_number_ (error_number)
    , data_ (data)
  {
    if (this->error_number_ == -1)
      this->error_number_ = errno;
  }

  size_t bytes (void) const { return this->bytes_; }

  int status (void) const { return this->status_; }
  int reason (void) const { return this->reason_; }

  int error_number (void) const { return (errno = this->error_number_); }

  void * data (void) const { return this->data_; }

private:

  size_t bytes_;

  JE_STATUS status_;
  JE_REASON reason_;

  int error_number_;

  void *data_;

};

#endif /* JAWS_EVENT_RESULT_H */
