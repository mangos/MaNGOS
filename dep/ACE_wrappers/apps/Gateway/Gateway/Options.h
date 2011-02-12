/* -*- C++ -*- */
// $Id: Options.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    gateway
//
// = FILENAME
//    Options.h
//
// = AUTHOR
//    Douglas C. Schmidt <schmidt@cs.wustl.edu>
//
// ============================================================================

#ifndef OPTIONS_H
#define OPTIONS_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/svc_export.h"
#include "ace/Lock_Adapter_T.h"
#include "ace/Synch_Traits.h"
#include "ace/Thread_Mutex.h"

class ACE_Svc_Export Options
{
  // = TITLE
  //     Singleton that consolidates all Options for a gatewayd.
public:
  // = Options that can be enabled/disabled.
  enum
  {
    // = The types of threading strategies.
    REACTIVE = 0,
    OUTPUT_MT = 1,
    INPUT_MT = 2,

    VERBOSE = 01,
    DEBUGGING = 02,

    SUPPLIER_ACCEPTOR = 04,
    CONSUMER_ACCEPTOR = 010,
    SUPPLIER_CONNECTOR = 020,
    CONSUMER_CONNECTOR = 040
  };

  static Options *instance (void);
  // Return Singleton.

  ~Options (void);
  // Termination.

  int parse_args (int argc, ACE_TCHAR *argv[]);
  // Parse the arguments and set the options.

  void print_usage(void);
  // Print the gateway supported parameters.
  // = Accessor methods.
  int enabled (int option) const;
  // Determine if an option is enabled.

  ACE_Lock_Adapter<ACE_SYNCH_MUTEX> *locking_strategy (void) const;
  // Gets the locking strategy used for serializing access to the
  // reference count in <ACE_Message_Block>.  If it's 0, then there's
  // no locking strategy and we're using a REACTIVE concurrency
  // strategy.

  void locking_strategy (ACE_Lock_Adapter<ACE_SYNCH_MUTEX> *);
  // Set the locking strategy used for serializing access to the
  // reference count in <ACE_Message_Block>.

  long performance_window (void) const;
  // Number of seconds after connection establishment to report
  // throughput.

  int blocking_semantics (void) const;
  // 0 == blocking connects, ACE_NONBLOCK == non-blocking connects.

  int socket_queue_size (void) const;
  // Size of the socket queue (0 means "use default").

  u_long threading_strategy (void) const;
  // i.e., REACTIVE, OUTPUT_MT, and/or INPUT_MT.

  u_short supplier_acceptor_port (void) const;
  // Our acceptor port number, i.e., the one that we passively listen
  // on for connections to arrive from a gatewayd and create a
  // Supplier.

  u_short consumer_acceptor_port (void) const;
  // Our acceptor port number, i.e., the one that we passively listen
  // on for connections to arrive from a gatewayd and create a
  // Consumer.

  u_short supplier_connector_port (void) const;
  // The connector port number, i.e., the one that we use to actively
  // establish connections with a gatewayd and create a Supplier.

  u_short consumer_connector_port (void) const;
  // The connector port number, i.e., the one that we use to actively
  // establish connections with a gatewayd and create a Consumer.

  const ACE_TCHAR *connection_config_file (void) const;
  // Name of the connection configuration file.

  const ACE_TCHAR *consumer_config_file (void) const;
  // Name of the consumer map configuration file.

  long max_timeout (void) const;
  // The maximum retry timeout delay.

  long max_queue_size (void) const;
  // The maximum size of the queue.

  CONNECTION_ID &connection_id (void);
  // Returns a reference to the next available connection id;

private:
  enum
  {
    MAX_QUEUE_SIZE = 1024 * 1024 * 16,
    // We'll allow up to 16 megabytes to be queued per-output proxy.

    MAX_TIMEOUT = 32
    // The maximum timeout for trying to re-establish connections.
  };

  Options (void);
  // Initialization.

  static Options *instance_;
  // Options Singleton instance.

  ACE_Lock_Adapter<ACE_SYNCH_MUTEX> *locking_strategy_;
  // Points to the locking strategy used for serializing access to the
  // reference count in <ACE_Message_Block>.  If it's 0, then there's
  // no locking strategy and we're using a REACTIVE concurrency
  // strategy.

  long performance_window_;
  // Number of seconds after connection establishment to report
  // throughput.

  int blocking_semantics_;
  // 0 == blocking connects, ACE_NONBLOCK == non-blocking connects.

  int socket_queue_size_;
  // Size of the socket queue (0 means "use default").

  u_long threading_strategy_;
  // i.e., REACTIVE, OUTPUT_MT, and/or INPUT_MT.

  u_long options_;
  // Flag to indicate if we want verbose diagnostics.

  u_short supplier_acceptor_port_;
  // The acceptor port number, i.e., the one that we passively listen
  // on for connections to arrive from a gatewayd and create a
  // Supplier.

  u_short consumer_acceptor_port_;
  // The acceptor port number, i.e., the one that we passively listen
  // on for connections to arrive from a gatewayd and create a
  // Consumer.

  u_short supplier_connector_port_;
  // The connector port number, i.e., the one that we use to actively
  // establish connections with a gatewayd and create a Supplier.

  u_short consumer_connector_port_;
  // The connector port number, i.e., the one that we use to actively
  // establish connections with a gatewayd and create a Consumer.

  long max_timeout_;
  // The maximum retry timeout delay.

  long max_queue_size_;
  // The maximum size of the queue.

  CONNECTION_ID connection_id_;
  // The next available connection id.

  ACE_TCHAR connection_config_file_[MAXPATHLEN + 1];
  // Name of the connection configuration file.

  ACE_TCHAR consumer_config_file_[MAXPATHLEN + 1];
  // Name of the consumer map configuration file.
};

#endif /* OPTIONS_H */
