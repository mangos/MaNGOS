// -*- C++ -*-

//=============================================================================
/**
 *  @file    Client_Logging_Handler.h
 *
 *  $Id: Client_Logging_Handler.h 84527 2009-02-19 14:01:42Z johnnyw $
 *
 *  @author Doug Schmidt <schmidt@.cs.wustl.edu>
 */
//=============================================================================


#ifndef ACE_CLIENT_LOGGER_H
#define ACE_CLIENT_LOGGER_H

#include "ace/SPIPE_Stream.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/SOCK_Stream.h"
#include "ace/Svc_Handler.h"
#include "ace/svc_export.h"

#if (ACE_HAS_STREAM_LOG_MSG_IPC == 1)
#define LOGGING_STREAM ACE_SPIPE_STREAM
#define LOGGING_ACCEPTOR ACE_SPIPE_ACCEPTOR
#define LOGGING_ADDR ACE_SPIPE_Addr
#else
#define LOGGING_STREAM ACE_SOCK_STREAM
#define LOGGING_ACCEPTOR ACE_SOCK_ACCEPTOR
#define LOGGING_ADDR ACE_INET_Addr
#endif /* ACE_HAS_STREAM_LOG_MSG_IPC == 1 */

#if defined ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT
template class ACE_Svc_Export ACE_Svc_Handler<LOGGING_STREAM, ACE_NULL_SYNCH>;
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT */

/**
 * @class ACE_Client_Logging_Handler
 *
 * @brief This client logging daemon is a mediator that receives logging
 * records from local applications processes and forwards them to
 * the server logging daemon running on another host.
 *
 * The default implementation uses an ACE_SPIPE_Stream to
 * receive the logging message from the application and an
 * ACE_SOCK_Stream to forward the logging message to the
 * server.  However, on platforms that don't support
 * <ACE_SPIPEs> (e.g., Win32) we use sockets instead.
 */
class ACE_Svc_Export ACE_Client_Logging_Handler :
  public ACE_Svc_Handler<LOGGING_STREAM, ACE_NULL_SYNCH>
{

public:
  // = Initialization and termination.

  /// Default constructor. @a handle is where the output is sent.
  ACE_Client_Logging_Handler (ACE_HANDLE handle = ACE_STDERR);

  /// Activate this instance of the ACE_Client_Logging_Handler
  /// (called by the ACE_Client_Logging_Acceptor).
  virtual int open (void * = 0);

  /// Return the handle of the IPC endpoint.
  virtual ACE_HANDLE get_handle (void) const;

  /// Called when object is removed from the ACE_Reactor.
  virtual int close (u_long);

private:
  /// Handle SIGPIPE.
  virtual int handle_signal (int signum,
                             siginfo_t *,
                             ucontext_t *);

  /// Receive logging records from applications.
  virtual int handle_input (ACE_HANDLE);

  /**
   * Receive logging records from applications.  This is necessary to
   * handle madness with UNIX select, which can't deal with MSG_BAND
   * data easily due to its overly simple interface...  This just
   * calls handle_input().
   */
  virtual int handle_exception (ACE_HANDLE);

  /// Called back when it's ok to send.
  virtual int handle_output (ACE_HANDLE);

  /// Send the @a log_record to the logging server.
  int send (ACE_Log_Record &log_record);

  /// This is either a SOCKET (if we're connected to a logging server)
  /// or ACE_STDERR.
  ACE_HANDLE logging_output_;
};

ACE_SVC_FACTORY_DECLARE (ACE_Client_Logging_Acceptor)

#endif /* ACE_CLIENT_LOGGER_H */
