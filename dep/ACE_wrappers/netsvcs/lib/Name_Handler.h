// -*- C++ -*-

//=============================================================================
/**
 *  @file    Name_Handler.h
 *
 *  $Id: Name_Handler.h 84498 2009-02-17 18:08:47Z johnnyw $
 *
 *  @author Prashant Jain
 *  @author Gerhard Lenzer
 *  @author and Douglas C. Schmidt
 */
//=============================================================================


#ifndef ACE_NAME_HANDLER_H
#define ACE_NAME_HANDLER_H

#include "ace/Acceptor.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/SOCK_Acceptor.h"
#include "ace/SString.h"
#include "ace/Svc_Handler.h"
#include "ace/Naming_Context.h"
#include "ace/Name_Request_Reply.h"
#include "ace/Null_Mutex.h"
#include "ace/svc_export.h"


#if defined ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT
template class ACE_Svc_Export ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>;
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT */

/**
 * @class ACE_Name_Handler
 *
 * @brief Product object created by <ACE_Name_Acceptor>.  An
 * <ACE_Name_Handler> exchanges messages with a <ACE_Name_Proxy>
 * object on the client-side.
 *
 * This class is the main workhorse of the <ACE_Name_Server>.  It
 * handles client requests to bind, rebind, resolve, and unbind
 * names.  It also schedules and handles timeouts that are used to
 * support "timed waits."  Clients used timed waits to bound the
 * amount of time they block trying to get a name.
 */
class ACE_Svc_Export ACE_Name_Handler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
public:
  /// Pointer to a member function of ACE_Name_Handler returning int
  typedef int (ACE_Name_Handler::*OPERATION) (void);

  /// Pointer to a member function of ACE_Naming_Context returning int
  typedef int (ACE_Naming_Context::*LIST_OP) (ACE_PWSTRING_SET &, const ACE_NS_WString &);

  /// Pointer to a member function of ACE_Name_Handler returning ACE_Name_Request
  typedef ACE_Name_Request (ACE_Name_Handler::*REQUEST) (ACE_NS_WString *);

  // = Initialization and termination.

  /// Default constructor.
  ACE_Name_Handler (ACE_Thread_Manager * = 0);

  /// Activate this instance of the <ACE_Name_Handler> (called by the
  /// <ACE_Strategy_Acceptor>).
  virtual int open (void * = 0);

protected:
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

  /// Create and send a reply to the client.
  virtual int send_reply (ACE_INT32 status,
                          ACE_UINT32 errnum = 0);

  /// Special kind of reply
  virtual int send_request (ACE_Name_Request &);

  // = Demultiplexing hooks.
  /// Return the underlying <ACE_HANDLE>.
  virtual ACE_HANDLE get_handle (void) const;

  /// Callback method invoked by the <ACE_Reactor> when client events
  /// arrive.
  virtual int handle_input (ACE_HANDLE);

  // = Timer hook.
  /// Enable clients to limit the amount of time they wait for a name.
  virtual int handle_timeout (const ACE_Time_Value &tv, const void *arg);

  /// Ensure dynamic allocation...
  ~ACE_Name_Handler (void);

private:

  /// Table of pointers to member functions
  OPERATION op_table_[ACE_Name_Request::MAX_ENUM];

  struct LIST_ENTRY
  {
    LIST_OP operation_;
    // A member function pointer that performs the appropriate
    // operation (e.g., LIST_NAMES, LIST_VALUES, or LIST_TYPES).

    REQUEST request_factory_;
    // A member function pointer that serves as a factory to create a
    // request that is passed back to the client.

    const char *description_;
    // Name of the operation we're dispatching (used for debugging).
  };

  /// This is the table of pointers to functions that we use to
  /// simplify the handling of list requests.
  LIST_ENTRY list_table_[ACE_Name_Request::MAX_LIST];

  /// Cache request from the client.
  ACE_Name_Request name_request_;

  /// Special kind of reply for resolve and listnames.
  ACE_Name_Request name_request_back_;

  /// Cache reply to the client.
  ACE_Name_Reply name_reply_;

  /// Address of client we are connected with.
  ACE_INET_Addr addr_;

  ///  Naming Context
  ACE_Naming_Context *naming_context_;

  ACE_Naming_Context *naming_context (void);

  /// Handle binds.
  int bind (void);

  /// Handle rebinds.
  int rebind (void);

  /// Handle binds and rebinds.
  int shared_bind (int rebind);

  /// Handle find requests.
  int resolve (void);

  /// Handle unbind requests.
  int unbind (void);

  /// Handle LIST_NAMES, LIST_VALUES, and LIST_TYPES requests.
  int lists (void);

  /// Handle LIST_NAME_ENTRIES, LIST_VALUE_ENTRIES, and
  /// LIST_TYPE_ENTRIES requests.
  int lists_entries (void);

  /// Create a name request.
  ACE_Name_Request name_request (ACE_NS_WString *one_name);

  /// Create a value request.
  ACE_Name_Request value_request (ACE_NS_WString *one_name);

  /// Create a type request.
  ACE_Name_Request type_request (ACE_NS_WString *one_name);
};

/**
 * @class ACE_Name_Acceptor
 *
 * @brief This class contains the service-specific methods that can't
 * easily be factored into the <ACE_Strategy_Acceptor>.
 */
class ACE_Name_Acceptor : public ACE_Strategy_Acceptor<ACE_Name_Handler, ACE_SOCK_ACCEPTOR>
{
public:
  /// Dynamic linking hook.
  virtual int init (int argc, ACE_TCHAR *argv[]);

  /// Parse svc.conf arguments.
  int parse_args (int argc, ACE_TCHAR *argv[]);

  /// Naming context for acceptor /for the listening port/
  ACE_Naming_Context *naming_context (void);

private:
  /// The scheduling strategy is designed for Reactive services.
  ACE_Schedule_All_Reactive_Strategy<ACE_Name_Handler> scheduling_strategy_;

  /// The Naming Context
  ACE_Naming_Context naming_context_;
};

ACE_SVC_FACTORY_DECLARE (ACE_Name_Acceptor)


#endif /* ACE_NAME_HANDLER_H */
