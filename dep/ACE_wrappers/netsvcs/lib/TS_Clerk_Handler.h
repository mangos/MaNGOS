/* -*- C++ -*- */

//=============================================================================
/**
 *  @file    TS_Clerk_Handler.h
 *
 *  $Id: TS_Clerk_Handler.h 84498 2009-02-17 18:08:47Z johnnyw $
 *
 *  @author Prashant Jain
 */
//=============================================================================


#ifndef ACE_TS_CLERK_HANDLER_H
#define ACE_TS_CLERK_HANDLER_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/SOCK_Connector.h"
#include "ace/Svc_Handler.h"
#include "ace/Connector.h"
#include "ace/MMAP_Memory_Pool.h"
#include "ace/Malloc_T.h"
#include "ace/Null_Mutex.h"
#include "ace/svc_export.h"
#include "ace/os_include/os_dirent.h"

#include "Time_Request_Reply.h"

/**
 * @class ACE_Time_Info
 *
 * @brief A simple struct containing delta time and a sequence number.
 */
class ACE_Time_Info
{

public:
  time_t delta_time_;

  ACE_UINT32 sequence_num_;
};

class ACE_TS_Clerk_Processor;  // forward declaration

#if defined ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT
template class ACE_Svc_Export ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>;
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT */

/**
 * @class ACE_TS_Clerk_Handler
 *
 * @brief The Clerk Handler provides the interface that is used by the
 * Clerk Processor to send time update requests to all the
 * servers. It obtains these updates from the servers and passes
 * the updates to the Clerk Processor
 *
 * The Clerk Processor uses send_request() to send a request for
 * time update to a server. The Clerk Handler internally computes
 * the round trip delay for the reply to come back. Once it gets
 * the reply back from the server (handle_input), it adjusts the
 * system time using the round trip delay estimate and then
 * passes the delta time by reference back to the Clerk
 * Processor.
 */
class ACE_Svc_Export ACE_TS_Clerk_Handler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
public:
  /// Default constructor.
  ACE_TS_Clerk_Handler (ACE_TS_Clerk_Processor *processor = 0,
                        ACE_INET_Addr &addr = (ACE_INET_Addr &) ACE_Addr::sap_any);

  // = Set/get the current state
  enum State
  {
    IDLE = 1,      // Prior to initialization.
    CONNECTING,    // During connection establishment.
    ESTABLISHED,   // Connection is established and active.
    DISCONNECTING, // In the process of disconnecting.
    FAILED         // Connection has failed.
  };

  // = Set/get the current state.
  State state (void);
  void state (State);

  // = Set/get the current retry timeout delay.
  long timeout (void);
  void timeout (long);

  // = Set/get the maximum retry timeout delay.
  long max_timeout (void);
  void max_timeout (long);

  /// Activate this instance of the <ACE_TS_Clerk_Handler>
  /// (called by the <ACE_TS_Clerk_Processor>).
  virtual int open (void * = 0);

  /// Return the handle of the message_fifo_;
  virtual ACE_HANDLE get_handle (void) const;

  /// Called when object is removed from the ACE_Reactor
  virtual int handle_close (ACE_HANDLE = ACE_INVALID_HANDLE,
                            ACE_Reactor_Mask = ACE_Event_Handler::ALL_EVENTS_MASK);

  /// Receive time update from a server.
  virtual int handle_input (ACE_HANDLE);

  /// Restart connection asynchronously when timeout occurs.
  virtual int handle_timeout (const ACE_Time_Value &tv,
                              const void *arg);

  /// Get/Set remote addr
  void remote_addr (ACE_INET_Addr &addr);
  ACE_INET_Addr &remote_addr (void);

  /// Send request for time update to the server as well as return the
  /// current time info by reference.
  int send_request (ACE_UINT32 sequence_num,
                    ACE_Time_Info &time_info);

protected:
  /// Handle SIGPIPE.
  virtual int handle_signal (int signum,
                             siginfo_t *,
                             ucontext_t *);

  static void stderr_output (int = 0);

  enum
    {
      MAX_RETRY_TIMEOUT = 300
      // 5 minutes is the maximum timeout.
    };

private:
  /// Receive a reply from a server containing time update
  int recv_reply (ACE_Time_Request &reply);

  /// Reinitiate connection with the server
  int reinitiate_connection (void);

  /// The current state of the connection
  State state_;

  /// Amount of time to wait between reconnection attempts
  long timeout_;

  /// Maximum amount of time to wait between reconnection attempts
  long max_timeout_;

  /// Remote Addr used for connecting to the server
  ACE_INET_Addr remote_addr_;

  /// Instance of Clerk Processor used for re-establishing connections
  ACE_TS_Clerk_Processor *processor_;

  /// Time at which request was sent (used to compute round trip delay)
  time_t start_time_;

  /// Next sequence number of time request (waiting for this update from
  /// the server).
  ACE_UINT32 cur_sequence_num_;

  /// Record of current delta time and current sequence number
  ACE_Time_Info time_info_;
};

/**
 * @class ACE_TS_Clerk_Processor
 *
 * @brief This class manages all the connections to the servers along
 * with querying them periodically for time updates.
 *
 * The Clerk Processor creates connections to all the servers and
 * creates an ACE_TS_Clerk_Handler for each connection to handle
 * the requests and replies. It periodically sends a request for
 * time update through each of the handlers and uses the replies
 * for computing a synchronized system time.
 */
class ACE_TS_Clerk_Processor : public ACE_Connector <ACE_TS_Clerk_Handler, ACE_SOCK_CONNECTOR>
{
public:
  /// Default constructor
  ACE_TS_Clerk_Processor (void);

  /// Query servers for time periodically (timeout value)
  virtual int handle_timeout (const ACE_Time_Value &tv,
                              const void *arg);

  /// Set up connections to all servers
  int initiate_connection (ACE_TS_Clerk_Handler *,
                           ACE_Synch_Options &);

protected:
  // = Dynamic linking hooks.
  /// Called when service is linked.
  virtual int init (int argc, ACE_TCHAR *argv[]);

  /// Called when service is unlinked.
  virtual int fini (void);

  /// Called to determine info about the service.
  virtual int info (ACE_TCHAR **strp, size_t length) const;

  // = Scheduling hooks.
  virtual int suspend (void);
  virtual int resume (void);

private:
  /// Parse svc.conf arguments.
  int parse_args (int argc, ACE_TCHAR *argv[]);

  /// Allocate entry in shared memory for system time
  void alloc (void);

  /// Update delta_time using times obtained from all servers
  int update_time ();

  /// Allocator (used for reading/writing system time from/to shared memory)
  typedef ACE_Malloc <ACE_MMAP_MEMORY_POOL, ACE_Null_Mutex> MALLOC;
  typedef ACE_Allocator_Adapter<MALLOC> ALLOCATOR;
  ALLOCATOR *shmem_;

  /// Set of TS_Clerk_Handlers and iterator over the set.
  typedef ACE_Unbounded_Set <ACE_TS_Clerk_Handler *> HANDLER_SET;
  typedef ACE_Unbounded_Set_Iterator <ACE_TS_Clerk_Handler *> HANDLER_SET_ITERATOR;
  HANDLER_SET handler_set_;

  struct System_Time
    {
      time_t *delta_time_;       // Diff between system time and local time
      time_t *last_local_time_;  // Last local time
    };

  /// Clerk system time containing pointers to entries in shared memory
  System_Time system_time_;

  /// Timer id returned by Reactor
  long timer_id_;

  /// Time period for updating system time
  long timeout_;

  /// Pool name for backing store
  ACE_TCHAR poolname_[MAXNAMLEN + 1];

  /// Do a blocking/non-blocking connect
  int blocking_semantics_;

  /// Sequence number of next expected update from servers
  ACE_UINT32 cur_sequence_num_;
};

ACE_SVC_FACTORY_DECLARE (ACE_TS_Clerk_Processor)

#endif /* ACE_TS_CLERK_HANDLER_H */
