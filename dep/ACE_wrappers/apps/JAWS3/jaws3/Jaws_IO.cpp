// $Id: Jaws_IO.cpp 85419 2009-05-22 10:52:11Z johnnyw $

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif /*JAWS_BUILD_DLL*/

#include "ace/OS_NS_strings.h"

#include "jaws3/Jaws_IO.h"
#include "jaws3/Asynch_IO.h"
#include "jaws3/Synch_IO.h"
#include "jaws3/Reactive_IO.h"
#include "jaws3/Options.h"

JAWS_IO::JAWS_IO (JAWS_IO_Impl *impl)
  : impl_ (impl)
{
  // Use synchronous IO by default.  It is the most efficient
  // in terms of calls needed, but provides the least amount
  // of support for concurrency.

  if (this->impl_ == 0)
    {
      const char *io_type = JAWS_Options::instance ()->getenv ("JAWS_IO");
      if (io_type == 0)
        io_type = JAWS_DEFAULT_IO;

      if (ACE_OS::strcasecmp (io_type, "SYNCH") == 0)
        this->impl_ = JAWS_Synch_IO::instance ();
      else if (ACE_OS::strcasecmp (io_type, "ASYNCH") == 0)
        this->impl_ = JAWS_Asynch_IO::instance ();
      else if (ACE_OS::strcasecmp (io_type, "REACTIVE") == 0)
        this->impl_ = JAWS_Reactive_IO::instance ();
      else
        this->impl_ = JAWS_Synch_IO::instance ();
    }
}

JAWS_IO::~JAWS_IO (void)
{
}


void
JAWS_IO::send ( ACE_HANDLE handle
              , ACE_Message_Block *mb
              , JAWS_Event_Completer *completer
              , void *act
              )
{
  this->impl_->send (handle, mb, completer, act);
}


void
JAWS_IO::recv ( ACE_HANDLE handle
              , ACE_Message_Block *mb
              , JAWS_Event_Completer *completer
              , void *act
              )
{
  this->impl_->recv (handle, mb, completer, act);
}


void
JAWS_IO::transmit ( ACE_HANDLE handle
                  , ACE_HANDLE source
                  , JAWS_Event_Completer *completer
                  , void *act
                  , ACE_Message_Block *header
                  , ACE_Message_Block *trailer
                  )
{
  this->impl_->transmit (handle, source, completer, act, header, trailer);
}


void
JAWS_IO::send ( ACE_HANDLE handle
              , ACE_Message_Block *mb
              , JAWS_Event_Completer *completer
              , const ACE_Time_Value &tv
              , void *act
              )
{
  this->impl_->send (handle, mb, completer, tv, act);
}


void
JAWS_IO::recv ( ACE_HANDLE handle
              , ACE_Message_Block *mb
              , JAWS_Event_Completer *completer
              , const ACE_Time_Value &tv
              , void *act
              )
{
  this->impl_->recv (handle, mb, completer, tv, act);
}


void
JAWS_IO::transmit ( ACE_HANDLE handle
                  , ACE_HANDLE source
                  , JAWS_Event_Completer *completer
                  , const ACE_Time_Value &tv
                  , void *act
                  , ACE_Message_Block *header
                  , ACE_Message_Block *trailer
                  )
{
  this->impl_->transmit (handle, source, completer, tv, act, header, trailer);
}
