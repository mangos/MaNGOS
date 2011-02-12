/* -*- C++ -*- */

//=============================================================================
/**
 *  @file    Token_Handler.h
 *
 *  $Id: Token_Handler.h 84498 2009-02-17 18:08:47Z johnnyw $
 *
 *  @author Douglas C. Schmidt (schmidt@cs.wustl.edu)
 *          Tim Harrison (harrison@cs.wustl.edu)
 */
//=============================================================================

#ifndef ACE_TOKEN_HANDLER_H
#define ACE_TOKEN_HANDLER_H

#include "ace/Acceptor.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/SOCK_Acceptor.h"
#include "ace/Local_Tokens.h"
#include "ace/Token_Collection.h"
#include "ace/Token_Request_Reply.h"
#include "ace/svc_export.h"

#if defined (ACE_HAS_TOKENS_LIBRARY)

/**
 * @class ACE_Token_Handler
 *
 * @brief Product object created by an <ACE_Token_Acceptor>.  A
 * <Token_Handler> exchanges messages with a <Token_Proxy> object
 * on the client-side.
 *
 * This class is the main workhorse of the ACE Token service.  It
 * receives token operation requests from remote clients and turns
 * them into calls on local tokens (acquire, release, renew, and
 * remove).  In OMG CORBA terms, it is an object adapter. It also
 * schedules and handles timeouts that are used to support "timed
 * waits."  Clients used timed waits to bound the amount of time
 * they block trying to get a token.
 */
class ACE_Svc_Export ACE_Token_Handler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{

public:
  // = Initialization and termination.

  /// Default constructor.
  ACE_Token_Handler (ACE_Thread_Manager * = 0);

  // = Accessor and mutator methods.

  // = Remote operations "exported" to a client.
  /**
   * Try to acquire the token.
   * Precondition: client *may* hold the token already (i.e.,
   * supports recursive acquisitions).
   */
  virtual int acquire (ACE_Token_Proxy *proxy);

  /// Try to acquire the token.
  virtual int try_acquire (ACE_Token_Proxy *proxy);

  /// Release the token and allow the next client that is waiting to
  /// proceed.  Preconditions: client must hold the token.
  virtual int release (ACE_Token_Proxy *proxy);

  /// Yield the token if any clients are waiting, otherwise keep the
  /// token.  Preconditions: client must hold the token.
  virtual int renew (ACE_Token_Proxy *proxy);

  /**
   * Remove the specified token from the Token_Map.  Preconditions:
   * ACE_Token must exist.  @@ Any other preconditions, e.g., must
   * client hold token, must there be no waiters, etc.?
   */
  virtual int remove (ACE_Token_Proxy *proxy);

  /// Called by TS_[Mutex,RLock,WLock] when we hold the mutex and
  /// someone wants it.
  void sleep_hook (void);

  /// Called by TS_[Mutex,RLock,WLock] when we are waiting and acquire
  /// the mutex.
  void token_acquired (ACE_TPQ_Entry *);

protected:
  // = Low level routines for framing requests, dispatching
  // operations, and returning replies.

  /// Our connection has been closed.
  virtual int abandon (int send_error);

  /// Receive, frame, and decode the client's request.
  virtual int recv_request (void);

  /// Dispatch the appropriate operation to handle the client's
  /// request.
  virtual int dispatch (void);

  /// Create and send a reply to the client.
  virtual int send_reply (ACE_UINT32 errnum);

  // = Demultiplexing hooks.
  /// Callback method invoked by the <ACE_Reactor> when client events
  /// arrive.
  virtual int handle_input (ACE_HANDLE);

  // = Timer hook.
  /// Enable clients to limit the amount of time they wait for a token.
  virtual int handle_timeout (const ACE_Time_Value &tv, const void *arg);

  /// return a proxy for the calling client_id and token name.
  ACE_Token_Proxy *get_proxy (void);

private:

  /// Switches on the type of token_request_ and creates a new
  /// Token_Proxy.
  virtual ACE_Token_Proxy *create_proxy (void);

  /// Keeps track of the synchronization options (i.e., the timeout
  /// interval).
  ACE_Synch_Options request_options_;

  /// collection of the client's token proxies.
  ACE_Token_Collection collection_;

  /// ID returned by the Reactor that is used to kill registered timers
  /// when a token operation times out.
  long timeout_id_;

  /// Cache request from the client.
  ACE_Token_Request token_request_;

  /// Cache reply to the client.
  ACE_Token_Reply token_reply_;
};

// = DESCRIPTION of ACE_TS_* classes:
//     When Tokens are released, waiting token proxies are notified
//     when the releasing thread calls token_acquired on the waiting
//     proxy.  The Token Server specializes ACE_Token_Proxy to
//     redefine the implementation of token_acquired.  When
//     token_acquired is called, the Token_Handler can then send the
//     response back over the socket connection to unblock the
//     client side.
//     Since only the Token_Handler uses ACE_TS_Mutex, we've moved
//     the definition to the .cpp file.

/**
 * @class ACE_TS_Mutex
 *
 * @brief ACE_TS_Mutex  -- ACE_*T*oken_*S*erver_Mutex
 */
class ACE_TS_Mutex : public ACE_Local_Mutex
{

public:
  /// Creation.
  ACE_TS_Mutex (const ACE_TCHAR *name,
                ACE_Token_Handler *th);

protected:
  /// Somebody wants our token!
  virtual void sleep_hook (void);

  /**
   * We've been taken off the waiters list and given the token!  Call
   * the Token_Handler associated at construction, so it can tell the
   * remote client.
   */
  virtual void token_acquired (ACE_TPQ_Entry *);

  /// Duplication.
  ACE_TS_Mutex (const ACE_TS_Mutex &);

  /// Return a deep copy.
  virtual ACE_Token_Proxy *clone (void) const;

private:
  /// The Token Handler associated with this proxy.  Set at
  /// construction and notified when blocking acquires succeed.
  ACE_Token_Handler* th_;
};

/**
 * @class ACE_TS_RLock
 *
 * @brief ACE_TS_RLock  -- ACE_*T*oken_*S*erver_RLock
 */
class ACE_TS_RLock : public ACE_Local_RLock
{
public:
  /// Creation.
  ACE_TS_RLock (const ACE_TCHAR *name,
                ACE_Token_Handler *th);

protected:
  /// Somebody wants our token!
  virtual void sleep_hook (void);

  /**
   * We've been taken off the waiters list and given the token!  Call
   * the Token_Handler associated at construction, so it can tell the
   * remote client.
   */
  virtual void token_acquired (ACE_TPQ_Entry *);

  /// Duplication.
  ACE_TS_RLock (const ACE_TS_RLock&);

  /// Return a deep copy.
  virtual ACE_Token_Proxy *clone (void) const;

private:
  /// the Token Handler associated with this proxy.  Set at
  /// construction and notified when blocking acquires succeed.
  ACE_Token_Handler* th_;
};

/**
 * @class ACE_TS_WLock
 *
 * @brief ACE_TS_WLock  -- ACE_*T*oken_*S*erver_WLock
 */
class ACE_TS_WLock : public ACE_Local_WLock
{
public:
  /// Creation.
  ACE_TS_WLock (const ACE_TCHAR *name,
                ACE_Token_Handler *th);

protected:
  /// Somebody wants our token!
  virtual void sleep_hook (void);

  /**
   * We've been taken off the waiters list and given the token!  Call
   * the Token_Handler associated at construction, so it can tell the
   * remote client.
   */
  virtual void token_acquired (ACE_TPQ_Entry *);

  /// Duplication.
  ACE_TS_WLock (const ACE_TS_WLock&);

  /// Return a deep copy.
  virtual ACE_Token_Proxy *clone (void) const;

private:
  /// the Token Handler associated with this proxy.  Set at
  /// construction and notified when blocking acquires succeed.
  ACE_Token_Handler* th_;
};

/**
 * @class ACE_Token_Acceptor
 *
 * @brief This class contains the service-specific methods that can't
 * easily be factored into the <ACE_Strategy_Acceptor>.
 */
class ACE_Token_Acceptor : public ACE_Strategy_Acceptor<ACE_Token_Handler, ACE_SOCK_ACCEPTOR>
{
public:
  /// Dynamic linking hook.
  virtual int init (int argc, ACE_TCHAR *argv[]);

  /// Parse svc.conf arguments.
  int parse_args (int argc, ACE_TCHAR *argv[]);

private:
  /// The scheduling strategy is designed for Reactive services.
  ACE_Schedule_All_Reactive_Strategy<ACE_Token_Handler> scheduling_strategy_;
};

ACE_SVC_FACTORY_DECLARE (ACE_Token_Acceptor)

#endif /* ACE_HAS_TOKENS_LIBRARY */
#endif /* ACE_TOKEN_HANDLER_H */
