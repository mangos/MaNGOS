/* -*- C++ -*- */
// $Id: Event_Channel.h 82739 2008-09-16 12:20:46Z johnnyw $

// ============================================================================
//
// = LIBRARY
//    gateway
//
// = FILENAME
//    Event_Channel.h
//
// = AUTHOR
//    Doug Schmidt <schmidt@cs.wustl.edu>
//
// ============================================================================

#ifndef ACE_EVENT_CHANNEL
#define ACE_EVENT_CHANNEL

#include "Connection_Handler_Connector.h"
#include "Connection_Handler_Acceptor.h"
#include "Consumer_Dispatch_Set.h"
#include "Event_Forwarding_Discriminator.h"
#include "ace/svc_export.h"

typedef ACE_Null_Mutex MAP_MUTEX;

class ACE_Svc_Export Event_Channel : public ACE_Event_Handler
{
  // = TITLE
  //    Define a generic Event_Channel.
  //
  // = DESCRIPTION
  //    The inspiration for this class is derived from the CORBA COS
  //    Event Channel, though the design is simplified.
  //
  //    We inherit from <ACE_Event_Handler> so that we can be
  //    registered with an <ACE_Reactor> to handle timeouts.
public:
  // = Initialization and termination methods.
  Event_Channel (void);
  ~Event_Channel (void);

  virtual int open (void * = 0);
  // Open the channel.

  virtual int close (u_long = 0);
  // Close down the Channel.

  // = Proxy management methods.
  int initiate_connection_connection (Connection_Handler *, int sync_directly = 0);
  // Initiate the connection of the <Connection_Handler> to its peer.
  // Second paratemer is used for thread connection-handler which will
  // block the connecting procedure directly, need not care
  // Options::blocking_semantics().

  int complete_connection_connection (Connection_Handler *);
  // Complete the initialization of the <Connection_Handler> once it's
  // connected to its Peer.

  int reinitiate_connection_connection (Connection_Handler *);
  // Reinitiate a connection asynchronously when the Peer fails.
  int cancel_connection_connection (Connection_Handler *);
  // Cancel a asynchronous connection.

  int bind_proxy (Connection_Handler *);
  // Bind the <Connection_Handler> to the <connection_map_>.

  int find_proxy (ACE_INT32 connection_id,
                  Connection_Handler *&);
  // Locate the <Connection_Handler> with <connection_id>.

  int subscribe (const Event_Key &event_addr,
                 Consumer_Dispatch_Set *cds);
  // Subscribe the <Consumer_Dispatch_Set> to receive events that
  // match <Event_Key>.

  // = Event processing entry point.
  virtual int put (ACE_Message_Block *mb,
                   ACE_Time_Value * = 0);
  // Pass <mb> to the Event Channel so it can forward it to Consumers.

  void initiate_connector (void);
  // Actively initiate connections to the Peers.

  int initiate_acceptors (void);
  // Passively initiate the <Peer_Acceptor>s for Consumer and
  // Suppliers.

private:
  int parse_args (int argc, ACE_TCHAR *argv[]);
  // Parse the command-line arguments.

  // = Methods for handling events.
  void routing_event (Event_Key *event_key,
                    ACE_Message_Block *data);
  // Forwards the <data> to Consumer that have registered to receive
  // it, based on addressing information in the <event_key>.

  void subscription_event (ACE_Message_Block *data);
  // Add a Consumer subscription.

  int compute_performance_statistics (void);
  // Perform timer-based performance profiling.

  virtual int handle_timeout (const ACE_Time_Value &,
                              const void *arg);
  // Periodically callback to perform timer-based performance
  // profiling.

  Connection_Handler_Connector connector_;
  // Used to establish the connections actively.

  Connection_Handler_Acceptor supplier_acceptor_;
  // Used to establish connections passively and create Suppliers.

  Connection_Handler_Acceptor consumer_acceptor_;
  // Used to establish connections passively and create Consumers.

  // = Make life easier by defining typedefs.
  typedef ACE_Map_Manager<CONNECTION_ID, Connection_Handler *, MAP_MUTEX>
  CONNECTION_MAP;
  typedef ACE_Map_Iterator<CONNECTION_ID, Connection_Handler *, MAP_MUTEX>
  CONNECTION_MAP_ITERATOR;
  typedef ACE_Map_Entry<CONNECTION_ID, Connection_Handler *>
  CONNECTION_MAP_ENTRY;

  CONNECTION_MAP connection_map_;
  // Table that maps <CONNECTION_ID>s to <Connection_Handler> *'s.

  Event_Forwarding_Discriminator efd_;
  // Map that associates an event to a set of <Consumer_Handler> *'s.
};

#endif /* ACE_EVENT_CHANNEL */
