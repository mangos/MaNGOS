/* -*- C++ -*- */

//=============================================================================
/**
 *  @file    Server_Logging_Handler_T.h
 *
 *  $Id: Server_Logging_Handler_T.h 84378 2009-02-10 18:27:54Z johnnyw $
 *
 *  @author Doug Schmidt and Per Andersson
 */
//=============================================================================


#ifndef ACE_SERVER_LOGGING_HANDLER_T_H
#define ACE_SERVER_LOGGING_HANDLER_T_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/TLI_Acceptor.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Svc_Handler.h"
#include "ace/Acceptor.h"
#include "ace/SString.h"
#include "ace/Atomic_Op.h"


#if !defined (__GNUG__)
#include "Base_Optimizer.h"
#endif /* ! __GNUG__ */

/**
 * @class ACE_Server_Logging_Handler_T
 *
 * @brief Product object created by an <ACE_Server_Logging_Acceptor_T>.  An
 * <ACE_Server_Logging_Handler_T> receives, and frames logging
 * records. The logging record is then processed by the
 * <LOG_MESSAGE_RECEIVER>
 *
 * Defines the classes that perform server logging daemon
 * functionality.
 */
template <ACE_PEER_STREAM_1, class COUNTER, ACE_SYNCH_DECL, class LOG_MESSAGE_RECEIVER>
class ACE_Server_Logging_Handler_T : public ACE_Svc_Handler<ACE_PEER_STREAM_2, ACE_SYNCH_USE>
{
public:
  /// Constructor.
  ACE_Server_Logging_Handler_T (ACE_Thread_Manager *,
                                const LOG_MESSAGE_RECEIVER &receiver );


  /// Process remote logging records.
  virtual int handle_input (ACE_HANDLE = ACE_INVALID_HANDLE);

protected:
  /// Receive the logging record from a client.
  int handle_logging_record (void);

  /// Common parts of open function, sets hostname and diables NONBLOCK in peer
  /// called from derived classes open method.
  int open_common (void);

#if !defined (ACE_LACKS_STATIC_DATA_MEMBER_TEMPLATES)
  /// Count the number of logging records that arrive.
  static COUNTER request_count_;
#endif /* ACE_LACKS_STATIC_DATA_MEMBER_TEMPLATES */

#if !defined (__GNUG__)
  /**
   * Packs a LOG_MESSAGE_RECEIVER and ACE_CString attribute together
   * in a optimized fashion.  The LOG_MESSAGE_RECEIVER class is often
   * a class with no instance data.
   */
  Base_Optimizer<LOG_MESSAGE_RECEIVER, ACE_TString> receiver_;
#else
  LOG_MESSAGE_RECEIVER receiver_;
  ACE_TString host_name_;
#endif /* ! __GNUG__ */
  /// Name of the host we are connected to.
  const ACE_TCHAR *host_name (void);

  /// The receiver of log records
  LOG_MESSAGE_RECEIVER &receiver (void){ return receiver_; }
};

#if 1   //!defined (ACE_HAS_TLI)
#define LOGGING_PEER_ACCEPTOR ACE_SOCK_ACCEPTOR
#define LOGGING_PEER_STREAM ACE_SOCK_STREAM
#else /* use sockets */
#define LOGGING_PEER_ACCEPTOR ACE_TLI_ACCEPTOR
#define LOGGING_PEER_STREAM ACE_TLI_STREAM
#endif /* ACE_HAS_TLI */

/**
 * @class ACE_Server_Logging_Acceptor_T
 *
 * @brief Factory that creates <SERVER_LOGGING_HANDLER>s scheduled with
 * <SCHEDULE_STRATEGY> and logging records proccessed by a
 * <LOG_MESSAGE_RECEIVER>
 *
 * This class contains the service-specific methods that can't
 * easily be factored into the <ACE_Strategy_Acceptor>.
 */
template<class SERVER_LOGGING_HANDLER, class LOG_MESSAGE_RECEIVER, class SCHEDULE_STRATEGY>
class ACE_Server_Logging_Acceptor_T : public ACE_Strategy_Acceptor<SERVER_LOGGING_HANDLER, LOGGING_PEER_ACCEPTOR>
{
public:
  /// Dynamic linking hook.
  ACE_Server_Logging_Acceptor_T (void);
  virtual int init (int argc, ACE_TCHAR *argv[]);

protected:
  /// Parse svc.conf arguments.
  int parse_args (int argc, ACE_TCHAR *argv[]);

  /**
   * Factory that creates a new <SERVER_LOGGING_HANDLER>.  We need to
   * specialize this since the <LOG_MESSAGE_RECEIVER> held by this Acceptor must be
   * passed into the <SERVER_LOGGING_HANDLER>.
   */
  virtual int make_svc_handler (SERVER_LOGGING_HANDLER *&);

private:
  // At the moment each ACE_Server_Logging_Acceptor_T contains
  // a <LOG_MESSAGE_RECEIVER> attribute that is passed to the
  // <SERVER_LOGGING_HANDLER> at construction. A better idea might
  // be to have accessor class as template argument. The accessor
  // should be a factory/strategy that hands the
  // ACE_Server_Logging_Acceptor_T instance references
  // to a <LOG_MESSAGE_RECEIVER>. This makes it possible
  // to change how <LOG_MESSAGE_RECEIVER> are created without chaning the
  // ACE_Server_Logging_Acceptor_T code.

#if !defined (__GNUG__)
  /**
   * Packs a LOG_MESSAGE_RECEIVER and ACE_CString attribute together
   * in a optimized fashion. The LOG_MESSAGE_RECEIVER class is often a
   * class with no instance data.
   */
  Base_Optimizer<LOG_MESSAGE_RECEIVER, SCHEDULE_STRATEGY> receiver_;
#else
  LOG_MESSAGE_RECEIVER receiver_;
  SCHEDULE_STRATEGY schedule_strategy_;
#endif /* ! __GNUG__ */

  /// The scheduling strategy for the service.
  SCHEDULE_STRATEGY &scheduling_strategy (void);

  /// The receiver of log records
  LOG_MESSAGE_RECEIVER &receiver (void);
};

/**
 * @class ACE_Server_Logging_Handler
 *
 * @brief Product object created by a
 * <ACE_Server_Logging_Acceptor_T<ACE_Server_Logging_Handler> >.  An
 * ACE_Server_Logging_Handler receives, frames. The logging record
 * is then processed by the <LOG_MESSAGE_RECEIVER>
 *
 * All clients are handled in the same thread.
 */
template<class LOG_MESSAGE_RECEIVER>
class ACE_Server_Logging_Handler : public ACE_Server_Logging_Handler_T<LOGGING_PEER_STREAM, u_long, ACE_NULL_SYNCH, LOG_MESSAGE_RECEIVER>
{

public:
  ACE_Server_Logging_Handler (ACE_Thread_Manager * = 0);
  ACE_Server_Logging_Handler (ACE_Thread_Manager *,
                              const LOG_MESSAGE_RECEIVER &receiver);

  virtual int open (void* = 0);
};

#if defined (ACE_HAS_THREADS)
typedef ACE_Atomic_Op <ACE_Thread_Mutex, u_long> ACE_LOGGER_COUNTER;
#define ACE_LOGGER_SYNCH ACE_MT_SYNCH
#else
typedef u_long ACE_LOGGER_COUNTER;
#define ACE_LOGGER_SYNCH ACE_NULL_SYNCH
#endif /* ACE_HAS_THREADS */

/**
 * @class ACE_Thr_Server_Logging_Handler
 *
 * @brief Product object created by a
 * <ACE_Server_Logging_Acceptor_T<ACE_Thr_Server_Logging_Handler>
 * >.  An ACE_Thr_Server_Logging_Handler receives, frames. The
 * logging record is then processed by the <LOG_MESSAGE_RECEIVER>
 *
 * Each client is handled in its own separate thread.
 */
template<class LOG_MESSAGE_RECEIVER>
class ACE_Thr_Server_Logging_Handler : public ACE_Server_Logging_Handler_T<LOGGING_PEER_STREAM, ACE_LOGGER_COUNTER, ACE_LOGGER_SYNCH, LOG_MESSAGE_RECEIVER>
{

public:
  ACE_Thr_Server_Logging_Handler (ACE_Thread_Manager * = 0);
  ACE_Thr_Server_Logging_Handler (ACE_Thread_Manager *,
                                  const LOG_MESSAGE_RECEIVER &receiver);
  virtual int open (void * = 0);
  virtual int svc (void);
};

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "Server_Logging_Handler_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("Server_Logging_Handler_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#endif /* ACE_SERVER_LOGGING_HANDLER_T_H */
