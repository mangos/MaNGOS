/* -*- c++ -*- */
// $Id: IO_Acceptor.h 85415 2009-05-22 07:26:32Z johnnyw $

#ifndef JAWS_IO_ACCEPTOR_H
#define JAWS_IO_ACCEPTOR_H

// Use the Adapter pattern to encapsulate either a LOCK_SOCK_Acceptor or
// an ACE_Asynch_Acceptor

#include "ace/Asynch_Acceptor.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/LOCK_SOCK_Acceptor.h"
#include "ace/Singleton.h"

#include "JAWS/Export.h"
#include "JAWS/Jaws_IO.h"
#include "JAWS/IO_Handler.h"

// Forward declaration.
ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Proactor;
class ACE_Reactor;
ACE_END_VERSIONED_NAMESPACE_DECL

#if defined (JAWS_HAS_THREAD_SAFE_ACCEPT)
typedef ACE_LOCK_SOCK_Acceptor<ACE_SYNCH_NULL_MUTEX> JAWS_IO_SOCK_Acceptor;
#else
typedef ACE_LOCK_SOCK_Acceptor<ACE_SYNCH_MUTEX> JAWS_IO_SOCK_Acceptor;
#endif /* JAWS_HAS_THREAD_SAFE_ACCEPT */

class JAWS_Export JAWS_IO_Acceptor
{
public:

  JAWS_IO_Acceptor (void);
  virtual ~JAWS_IO_Acceptor (void);

  virtual int open (const ACE_INET_Addr &address, int backlog = 20);
  // Initiate a passive mode socket.

  virtual int open (const ACE_HANDLE &socket);
  // Initiate a passive mode socket.

  virtual int accept (ACE_SOCK_Stream &new_stream,
                      ACE_Addr *remote_addr = 0,
                      ACE_Time_Value *timeout = 0,
                      int restart = 1,
                      int reset_new_handle = 0) const;
  // Synchronously accept the connection

  virtual int accept (size_t bytes_to_read = 0, const void *act = 0);
  // This initiates a new asynchronous accept through the AcceptEx call.

  virtual ACE_HANDLE get_handle (void);
  // Get the listener's handle

  virtual void close (void);
  // Close the acceptor.

  enum { ASYNC = 0, SYNCH = 1 };
  // identify if this is being used for asynchronous or synchronous
  // accept calls

};

class JAWS_Export JAWS_IO_Synch_Acceptor : public JAWS_IO_Acceptor
{
public:

  virtual int open (const ACE_INET_Addr &local_sap, int backlog = 20);
  // Initiate a passive mode socket.

  virtual int open (const ACE_HANDLE &socket);
  // Initiate a passive mode socket.

  virtual int accept (ACE_SOCK_Stream &new_stream,
                      ACE_Addr *remote_addr = 0,
                      ACE_Time_Value *timeout = 0,
                      int restart = 1,
                      int reset_new_handle = 0) const;
  // Accept the connection

  virtual ACE_HANDLE get_handle (void);
  // Get the listener's handle

private:
  virtual int accept (size_t bytes_to_read = 0, const void *act = 0);

private:
  JAWS_IO_SOCK_Acceptor acceptor_;
};


class JAWS_Export JAWS_IO_Asynch_Acceptor : public JAWS_IO_Acceptor
{
public:

  JAWS_IO_Asynch_Acceptor (void);
  virtual ~JAWS_IO_Asynch_Acceptor (void);

  virtual int open (const ACE_INET_Addr &address, int backlog = 20);
  // Initiate an asynchronous passive connection

  virtual int open (const ACE_HANDLE &socket);
  // Initiate an asynchronous passive connection

  virtual int accept (size_t bytes_to_read = 0, const void *act = 0);
  // This initiates a new asynchronous accept through the AcceptEx call.

  virtual ACE_HANDLE get_handle (void);
  // Get the listener's handle

  virtual void close (void);

private:

  virtual int accept (ACE_SOCK_Stream &new_stream,
                      ACE_Addr *remote_addr = 0,
                      ACE_Time_Value *timeout = 0,
                      int restart = 1,
                      int reset_new_handle = 0) const;

private:
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
  ACE_Asynch_Acceptor<JAWS_Asynch_Handler> &acceptor_;
  ACE_Asynch_Acceptor<JAWS_Asynch_Handler> *acceptor_ptr_;
  ACE_HANDLE handle_;
#endif /* ACE_HAS_WIN32_OVERLAPPED_IO || ACE_HAS_AIO_CALLS */
};

typedef ACE_Singleton<JAWS_IO_Synch_Acceptor, ACE_SYNCH_MUTEX>
        JAWS_IO_Synch_Acceptor_Singleton;

typedef ACE_Singleton<JAWS_IO_Asynch_Acceptor, ACE_SYNCH_MUTEX>
        JAWS_IO_Asynch_Acceptor_Singleton;

#endif /* !defined (JAWS_IO_ACCEPTOR_H) */
