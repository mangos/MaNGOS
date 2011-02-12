/* -*- C++ -*- */

//=============================================================================
/**
 *  @file    TS_Server_Handler.h
 *
 *  $Id: TS_Server_Handler.h 84498 2009-02-17 18:08:47Z johnnyw $
 *
 *  @author Prashant Jain
 */
//=============================================================================


#ifndef ACE_TS_SERVER_HANDLER_H
#define ACE_TS_SERVER_HANDLER_H

#include "ace/Acceptor.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/SOCK_Acceptor.h"
#include "ace/Svc_Handler.h"
#include "ace/svc_export.h"

#include "Time_Request_Reply.h"

#if defined ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT
template class ACE_Svc_Export ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>;
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT */

/**
 * @class ACE_TS_Server_Handler
 *
 * @brief Product object created by <ACE_TS_Server_Acceptor>.
 *
 */
class ACE_Svc_Export ACE_TS_Server_Handler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
public:
  // = Initialization and termination.

  /// Default constructor.
  ACE_TS_Server_Handler (ACE_Thread_Manager * = 0);

  /// Activate this instance of the <ACE_TS_Server_Handler> (called by the
  /// <ACE_Strategy_Acceptor>).
  virtual int open (void * = 0);

protected:
  /// Must be allocated dynamically.
  ~ACE_TS_Server_Handler (void);

  // = Helper routines for the operations exported to clients.

  /// Give up waiting (e.g., when a timeout occurs or a client shuts
  /// down unexpectedly).
  virtual int abandon (void);

  // = Low level routines for framing requests, dispatching
  // operations, and returning replies.

  /// Receive, frame, and decode the client's request.
  virtual int recv_request (void);

  /// Dispatch the appropriate operation to handle the client's
  /// request.
  virtual int dispatch (void);

  /// Special kind of reply
  virtual int send_request (ACE_Time_Request &);

  // = Demultiplexing hooks.
  /// Return the underlying <ACE_HANDLE>.
  virtual ACE_HANDLE get_handle (void) const;

  /// Callback method invoked by the <ACE_Reactor> when client events
  /// arrive.
  virtual int handle_input (ACE_HANDLE);

  // = Timer hook.
  /// Enable clients to limit the amount of time they wait.
  virtual int handle_timeout (const ACE_Time_Value &tv, const void *arg);

private:
  /// Cache request from the client.
  ACE_Time_Request time_request_;

  /// Address of client we are connected with.
  ACE_INET_Addr addr_;
};

/**
 * @class ACE_TS_Server_Acceptor
 *
 * @brief This class contains the service-specific methods that can't
 * easily be factored into the <ACE_Strategy_Acceptor>.
 */
class ACE_TS_Server_Acceptor : public ACE_Strategy_Acceptor<ACE_TS_Server_Handler, ACE_SOCK_ACCEPTOR>
{

public:
  /// Dynamic linking hook.
  virtual int init (int argc, ACE_TCHAR *argv[]);

  /// Parse svc.conf arguments.
  int parse_args (int argc, ACE_TCHAR *argv[]);

private:
  /// The scheduling strategy is designed for Reactive services.
  ACE_Schedule_All_Reactive_Strategy<ACE_TS_Server_Handler> scheduling_strategy_;
};

ACE_SVC_FACTORY_DECLARE (ACE_TS_Server_Acceptor)

#endif /* ACE_TS_SERVER_HANDLER_H */
