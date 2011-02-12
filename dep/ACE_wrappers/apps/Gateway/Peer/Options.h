// -*- C++ -*-
//
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
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef OPTIONS_H
#define OPTIONS_H

#include "../Gateway/Event.h"
#include "ace/svc_export.h"

class ACE_Svc_Export Options
  // = TITLE
  //     Singleton that consolidates all Options for a peerd.
{
public:
  // = Options that can be enabled/disabled.
  enum
  {
    VERBOSE = 01,
    SUPPLIER_ACCEPTOR = 02,
    CONSUMER_ACCEPTOR = 04,
    SUPPLIER_CONNECTOR = 010,
    CONSUMER_CONNECTOR = 020
  };

  static Options *instance (void);
  // Return Singleton.

  void parse_args (int argc, ACE_TCHAR *argv[]);
  // Parse the arguments and set the options.

  // = Accessor methods.
  int enabled (int option) const;
  // Determine if an option is enabled.

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

  const ACE_TCHAR *connector_host (void) const;
  // Our connector port host, i.e., the host running the gatewayd
  // process.

  long timeout (void) const;
  // Duration between disconnects.

  long max_queue_size (void) const;
  // The maximum size of the queue.

  CONNECTION_ID &connection_id (void);
  // Returns a reference to the connection id.

private:
  enum
  {
    MAX_QUEUE_SIZE = 1024 * 1024 * 16,
    // We'll allow up to 16 megabytes to be queued per-output
    // channel!!!!  This is clearly a policy in search of
    // refinement...

    DEFAULT_TIMEOUT = 60
    // By default, disconnect the peer every minute.
  };

  Options (void);
  // Ensure Singleton.

  void print_usage_and_die (void);
  // Explain usage and exit.

  static Options *instance_;
  // Singleton.

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

  const ACE_TCHAR *connector_host_;
  // Our connector host, i.e., where the gatewayd process is running.

  long timeout_;
  // The amount of time to wait before disconnecting from the Peerd.

  long max_queue_size_;
  // The maximum size that the queue can grow to.

  CONNECTION_ID connection_id_;
  // The connection id.
};

#endif /* OPTIONS_H */
