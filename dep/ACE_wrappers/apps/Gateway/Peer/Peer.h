/* -*- C++ -*- */
// $Id: Peer.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    gateway
//
// = FILENAME
//    Peer.h
//
// = DESCRIPTION
//    These classes process Supplier/Consumer events sent from the
//    gateway (gatewayd) to its various peers (peerd).  The general
//    collaboration works as follows:
//
//    1. <Peer_Acceptor> creates a listener endpoint and waits
//       passively for gatewayd to connect with it.
//
//    2. When a gatewayd connects, <Peer_Acceptor> creates an
//       <Peer_Handler> object that sends/receives events from
//       gatewayd on that connection.
//
//    3. The <Peer_Handler> waits for gatewayd to inform it of its
//       connection ID, which is prepended to all subsequent outgoing
//       events sent from peerd.
//
//    4. Once the connection ID is set, peerd periodically sends events
//       to gatewayd.  Peerd also receives and "processes" events
//       forwarded to it from gatewayd.  In this program, peerd
//       "processes" the events sent to it by writing them to stdout.
//
//    Note that in the current peerd implementation, one Peer process
//    cannot serve as both a Consumer and Supplier of Events.  This is
//    because the gatewayd establishes a separate connection for
//    Suppliers and Consumers and the peerd only maintains a single
//    <Peer_Handler> object to handle this one connection.  Enhancing
//    this implementation to be both a Consumer and Supplier
//    simultaneously is straightforward, however.  In addition,
//    multiple peerd processes can already work together to play these
//    different roles.
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef PEER_H
#define PEER_H

#include "ace/Service_Config.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Connector.h"
#include "ace/Svc_Handler.h"
#include "ace/Connector.h"
#include "ace/Null_Condition.h"
#include "ace/Null_Mutex.h"
#include "Options.h"

ACE_SVC_FACTORY_DECLARE (Peer_Factory)

#if defined ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT
template class ACE_Svc_Export ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>;
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT */

class ACE_Svc_Export Peer_Handler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
  // = TITLE
  //     Handle Peer events arriving from a Gateway.
public:
  // = Initialization and termination methods.
  Peer_Handler (void);
  // Initialize the peer.

  ~Peer_Handler (void);
  // Shutdown the Peer.

  virtual int open (void * = 0);
  // Initialize the handler when called by
  // <ACE_Acceptor::handle_input>.

  virtual int handle_input (ACE_HANDLE);
  // Receive and process peer events.

  virtual int put (ACE_Message_Block *, ACE_Time_Value *tv = 0);
  // Send a event to a gateway (may be queued if necessary due to flow
  // control).

  virtual int handle_output (ACE_HANDLE);
  // Finish sending a event when flow control conditions abate.

  virtual int handle_timeout (const ACE_Time_Value &,
                              const void *arg);
  // Periodically send events via <ACE_Reactor> timer mechanism.

  virtual int handle_close (ACE_HANDLE = ACE_INVALID_HANDLE,
                            ACE_Reactor_Mask = ACE_Event_Handler::ALL_EVENTS_MASK);
  // Perform object termination.

protected:
  typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> inherited;

  int transmit (ACE_Message_Block *mb,
                size_t n,
                int event_type);
  // Transmit <mb> to the gatewayd.

  virtual int recv (ACE_Message_Block *&mb);
  // Receive an Peer event from gatewayd.

  virtual int send (ACE_Message_Block *mb);
  // Send an Peer event to gatewayd, using <nonblk_put>.

  virtual int nonblk_put (ACE_Message_Block *mb);
  // Perform a non-blocking <put>, which tries to send an event to the
  // gatewayd, but only if it isn't flow controlled.

  int subscribe (void);
  // Register Consumer subscriptions with the gateway.

  // = Event/state/action handlers.
  int transmit_stdin (void);
  // Receive a event from stdin and send it to the gateway.

  int await_connection_id (void);
  // Action that receives the route id.

  int await_events (void);
  // Action that receives events.

  int (Peer_Handler::*do_action_)(void);
  // Pointer-to-member-function for the current action to run in this
  // state.  This points to one of the preceding 3 methods.

  CONNECTION_ID connection_id_;
  // Connection ID of the peer, which is obtained from the gatewayd.

  ACE_Message_Block *msg_frag_;
  // Keep track of event fragments that arrive in non-blocking recv's
  // from the gatewayd.

  size_t total_bytes_;
  // The total number of bytes sent/received to the gatewayd thus far.

  int first_time_;
  // Used to call register_stdin_handle only once.  Otherwise, thread
  // leak will occur on Win32.
};

#if defined ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT
template class ACE_Svc_Export ACE_Acceptor<Peer_Handler, ACE_SOCK_ACCEPTOR>;
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT */

class ACE_Svc_Export Peer_Acceptor : public ACE_Acceptor<Peer_Handler, ACE_SOCK_ACCEPTOR>
{
  // = TITLE
  //     Passively accept connections from gatewayd and dynamically
  //     create a new <Peer_Handler> object to communicate with the
  //     gatewayd.
public:
  // = Initialization and termination methods.
  Peer_Acceptor (void);
  // Default initialization.

  int start (u_short);
  //  the <Peer_Acceptor>.

  int close (void);
  // Terminate the <Peer_Acceptor>.

  virtual int make_svc_handler (Peer_Handler *&);
  // Factory method that creates a <Peer_Handler> just once.

private:
  int open_acceptor (u_short port);
  // Factor out common code for initializing the <Peer_Acceptor>.

  Peer_Handler *peer_handler_;
  // Pointer to <Peer_Handler> allocated just once.

  ACE_INET_Addr addr_;
  // Our acceptor addr.

  typedef ACE_Acceptor<Peer_Handler, ACE_SOCK_ACCEPTOR> inherited;
};

#if defined ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT
template class ACE_Svc_Export ACE_Connector<Peer_Handler, ACE_SOCK_CONNECTOR>;
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT */

class ACE_Svc_Export Peer_Connector : public ACE_Connector<Peer_Handler, ACE_SOCK_CONNECTOR>
{
  // = TITLE
  //     Actively establish connections with gatewayd and dynamically
  //     create a new <Peer_Handler> object to communicate with the
  //     gatewayd.
public:
  // = Initialization method.
  int open (ACE_Reactor * = 0, int = 0);
  // Initialize the <Peer_Connector>.  NOTE:  the arguments are
  // ignored.  They are only provided to avoid a compiler warning
  // about hiding the virtual function ACE_Connector<Peer_Handler,
  // ACE_SOCK_CONNECTOR>::open(ACE_Reactor*, int).

private:
  int open_connector (Peer_Handler *&ph, u_short port);
  // Factor out common code for initializing the <Peer_Connector>.

  Peer_Handler *consumer_peer_handler_;
  // Consumer <Peer_Handler> that is connected to a gatewayd.

  Peer_Handler *supplier_peer_handler_;
  // Supplier <Peer_Handler> that is connected to a gatewayd.
};

class ACE_Svc_Export Peer_Factory : public ACE_Service_Object
{
  // = TITLE
  //     A factory class that actively and/or passively establishes
  //     connections with the gatewayd.
public:
  // = Dynamic initialization and termination hooks from <ACE_Service_Object>.

  virtual int init (int argc, ACE_TCHAR *argv[]);
  // Initialize the acceptor and connector.

  virtual int fini (void);
  // Perform termination activities.

  virtual int info (ACE_TCHAR **, size_t) const;
  // Return info about this service.

  virtual int handle_signal (int signum, siginfo_t *, ucontext_t *);
  // Handle various signals (e.g., SIGPIPE, SIGINT, and SIGQUIT).

private:
  Peer_Acceptor consumer_acceptor_;
  // Pointer to an instance of our <Peer_Acceptor> that's used to
  // accept connections and create Consumers.

  Peer_Acceptor supplier_acceptor_;
  // Pointer to an instance of our <Peer_Acceptor> that's used to
  // accept connections and create Suppliers.

  Peer_Connector connector_;
  // An instance of our <Peer_Connector>.  Note that one
  // <Peer_Connector> is used to establish <Peer_Handler>s for both
  // Consumers and Suppliers.
};

#endif /* PEER_H */
