// $Id: Time_Request_Reply.cpp 91688 2010-09-09 11:21:50Z johnnyw $

#include "ace/Basic_Types.h"
#include "ace/CDR_Base.h"
#include "ace/Log_Msg.h"
#include "ace/Truncate.h"
#include "ace/os_include/netinet/os_in.h"
#include "ace/os_include/arpa/os_inet.h"

#include "Time_Request_Reply.h"



// Default "do nothing" constructor.

ACE_Time_Request::ACE_Time_Request (void)
{
  ACE_TRACE ("ACE_Time_Request::ACE_Time_Request");
}

// Create a ACE_Time_Request message.

ACE_Time_Request::ACE_Time_Request (ACE_INT32 t, // Type of request.
                                    const time_t time,
                                    ACE_Time_Value *timeout) // Max time waiting for request.
{
  ACE_TRACE ("ACE_Time_Request::ACE_Time_Request");
  this->msg_type (t);

  // If timeout is a NULL pointer, then block forever...
  if (timeout == 0)
    {
      this->transfer_.block_forever_ = 1;
      this->transfer_.sec_timeout_   = 0;
      this->transfer_.usec_timeout_  = 0;
    }
  else // Do a "timed wait."
    {
      this->block_forever (0);
      // Keep track of how long client is willing to wait.
      this->transfer_.sec_timeout_ = timeout->sec ();
      this->transfer_.usec_timeout_ = timeout->usec ();
    }

  // Copy time into request
  this->transfer_.time_ = this->time_ = time;
}

// Get the fixed size of message
ssize_t
ACE_Time_Request::size (void) const
{
  ACE_TRACE ("ACE_Time_Request::size");
  return sizeof (this->transfer_);
}

// = Set/get the type of the message.
ACE_INT32
ACE_Time_Request::msg_type (void) const
{
  ACE_TRACE ("ACE_Time_Request::msg_type");
  return this->transfer_.msg_type_;
}

void
ACE_Time_Request::msg_type (ACE_INT32 t)
{
  ACE_TRACE ("ACE_Time_Request::msg_type");
  this->transfer_.msg_type_ = t;
}

// = Set/get the blocking semantics.
ACE_UINT32
ACE_Time_Request::block_forever (void) const
{
  ACE_TRACE ("ACE_Time_Request::block_forever");
  return this->transfer_.block_forever_;
}

void
ACE_Time_Request::block_forever (ACE_UINT32 bs)
{
  ACE_TRACE ("ACE_Time_Request::block_forever");
  this->transfer_.block_forever_ = bs;
}

// = Set/get the timeout.
ACE_Time_Value
ACE_Time_Request::timeout (void) const
{
  ACE_TRACE ("ACE_Time_Request::timeout");
  time_t sec = ACE_Utils::truncate_cast<time_t> (this->transfer_.sec_timeout_);
  return ACE_Time_Value (sec, this->transfer_.usec_timeout_);
}

void
ACE_Time_Request::timeout (const ACE_Time_Value& timeout)
{
  ACE_TRACE ("ACE_Time_Request::timeout");
  this->transfer_.sec_timeout_  = timeout.sec ();
  this->transfer_.usec_timeout_ = timeout.usec ();
}

// = Set/get the time
time_t
ACE_Time_Request::time (void) const
{
  ACE_TRACE ("ACE_Time_Request::time");
  return this->time_;
}

void
ACE_Time_Request::time (time_t t)
{
  ACE_TRACE ("ACE_Time_Request::time");
  this->time_ = t;
}

// Encode the transfer buffer into network byte order
// so that it can be sent to the server.
int
ACE_Time_Request::encode (void *&buf)
{
  ACE_TRACE ("ACE_Time_Request::encode");
  // Compute the length *before* doing the marshaling.

  buf = (void *) &this->transfer_;
  this->transfer_.block_forever_ = ACE_HTONL (this->transfer_.block_forever_);
  this->transfer_.usec_timeout_  = ACE_HTONL (this->transfer_.usec_timeout_);
  this->transfer_.msg_type_      = ACE_HTONL (this->transfer_.msg_type_);
#if defined (ACE_LITTLE_ENDIAN)
  ACE_UINT64 secs = this->transfer_.sec_timeout_;
  ACE_CDR::swap_8 ((const char *)&secs, (char *)&this->transfer_.sec_timeout_);
  secs = this->transfer_.time_;
  ACE_CDR::swap_8 ((const char *)&secs, (char *)&this->transfer_.time_);
#endif

  return this->size ();  // Always fixed
}

// Decode the transfer buffer into host byte byte order
// so that it can be used by the server.
int
ACE_Time_Request::decode (void)
{
  ACE_TRACE ("ACE_Time_Request::decode");
  // Decode
  this->transfer_.block_forever_ = ACE_NTOHL (this->transfer_.block_forever_);
  this->transfer_.usec_timeout_  = ACE_NTOHL (this->transfer_.usec_timeout_);
  this->transfer_.msg_type_      = ACE_NTOHL (this->transfer_.msg_type_);
#if defined (ACE_LITTLE_ENDIAN)
  ACE_UINT64 secs = this->transfer_.sec_timeout_;
  ACE_CDR::swap_8 ((const char *)&secs, (char *)&this->transfer_.sec_timeout_);
  secs = this->transfer_.time_;
  ACE_CDR::swap_8 ((const char *)&secs, (char *)&this->transfer_.time_);
#endif

  this->time_ = ACE_Utils::truncate_cast<time_t> (this->transfer_.time_);
  return 0;
}

// Print out the current values of the ACE_Time_Request.

void
ACE_Time_Request::dump (void) const
{
#if defined (ACE_HAS_DUMP)
  ACE_TRACE ("ACE_Time_Request::dump");
  ACE_DEBUG ((LM_DEBUG,  ACE_TEXT ("*******\nlength = %d\n"),
              this->size ()));
  ACE_DEBUG ((LM_DEBUG,  ACE_TEXT ("message-type = ")));

  switch (this->msg_type ())
    {
    case ACE_Time_Request::TIME_UPDATE:
      ACE_DEBUG ((LM_DEBUG,  ACE_TEXT ("TIME_UPDATE\n")));
      break;
    default:
      ACE_DEBUG ((LM_DEBUG,  ACE_TEXT ("<unknown type> = %d\n"), this->msg_type ()));
      break;
    }

  if (this->block_forever ())
    ACE_DEBUG ((LM_DEBUG,  ACE_TEXT ("blocking forever\n")));
  else
    {
#if !defined (ACE_NLOGGING)
      ACE_Time_Value tv = this->timeout ();
#endif /* ! ACE_NLOGGING */
      ACE_DEBUG ((LM_DEBUG,  ACE_TEXT ("waiting for %d secs and %d usecs\n"),
                  (int)(tv.sec ()), tv.usec ()));
    }
  ACE_DEBUG ((LM_DEBUG,  ACE_TEXT ("*******\ntime = %d\n"),
              (int)(this->time ())));
  ACE_DEBUG ((LM_DEBUG,  ACE_TEXT ("+++++++\n")));
#endif /* ACE_HAS_DUMP */
}
