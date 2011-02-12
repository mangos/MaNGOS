// $Id: Event_Channel.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#define ACE_BUILD_SVC_DLL

#include "Connection_Handler_Connector.h"
#include "Event_Channel.h"
#include "ace/OS_NS_sys_select.h"
#include "ace/Signal.h"

Event_Channel::~Event_Channel (void)
{
}

#if defined (ACE_WIN32_VC8)
#  pragma warning (push)
#  pragma warning (disable:4355)  /* Use of 'this' in initializer list */
#  endif
Event_Channel::Event_Channel (void)
  : supplier_acceptor_ (*this, 'S'),
    consumer_acceptor_ (*this, 'C')
{
}
#if defined (ACE_WIN32_VC8)
#  pragma warning (pop)
#endif

int
Event_Channel::compute_performance_statistics (void)
{
  ACE_DEBUG ((LM_DEBUG, "(%t) doing the performance timeout here...\n"));
  CONNECTION_MAP_ITERATOR cmi (this->connection_map_);

  // If we've got a <ACE_Thread_Manager> then use it to suspend all
  // the threads.  This will enable us to get an accurate count.

  if (Options::instance ()->threading_strategy ()
      != Options::REACTIVE)
    {
      if (ACE_Thread_Manager::instance ()->suspend_all () == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           "(%t) %p\n",
                           "suspend_all"),
                          -1);
      ACE_DEBUG ((LM_DEBUG,
                  "(%t) suspending all threads..."));
    }

  size_t total_bytes_in = 0;
  size_t total_bytes_out = 0;

  // Iterate through the connection map summing up the number of bytes
  // sent/received.

  for (CONNECTION_MAP_ENTRY *me = 0;
       cmi.next (me) != 0;
       cmi.advance ())
    {
      Connection_Handler *connection_handler = me->int_id_;

      if (connection_handler->connection_role () == 'C')
        total_bytes_out += connection_handler->total_bytes ();
      else // connection_handler->connection_role () == 'S'
        total_bytes_in += connection_handler->total_bytes ();
    }

  ACE_DEBUG ((LM_DEBUG,
              "(%t) after %d seconds, \ntotal_bytes_in = %d\ntotal_bytes_out = %d\n",
              Options::instance ()->performance_window (),
              total_bytes_in,
              total_bytes_out));

  ACE_DEBUG ((LM_DEBUG,
              "(%t) %f Mbits/sec received.\n",
              (float) (total_bytes_in * 8 /
                       (float) (1024 * 1024 * Options::instance ()->performance_window ()))));

  ACE_DEBUG ((LM_DEBUG,
              "(%t) %f Mbits/sec sent.\n",
              (float) (total_bytes_out * 8 /
                       (float) (1024 * 1024 * Options::instance ()->performance_window ()))));

  // Resume all the threads again.

  if (Options::instance ()->threading_strategy ()
      != Options::REACTIVE)
    {
      if (ACE_Thread_Manager::instance ()->resume_all () == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           "(%t) %p\n",
                           "resume_all"),
                          -1);
      ACE_DEBUG ((LM_DEBUG,
                  "(%t) resuming all threads..."));
    }


  return 0;
}

int
Event_Channel::handle_timeout (const ACE_Time_Value &,
                               const void *)
{
  // This is called periodically to compute performance statistics.
  return this->compute_performance_statistics ();
}

int
Event_Channel::put (ACE_Message_Block *event,
                    ACE_Time_Value *)
{
  // We got a valid event, so determine its type, which is stored in
  // the first of the two <ACE_Message_Block>s, which are chained
  // together by <ACE::recv>.
  Event_Key *event_key = (Event_Key *) event->rd_ptr ();

  // Skip over the address portion and get the data, which is in the
  // second <ACE_Message_Block>.
  ACE_Message_Block *data = event->cont ();

  switch (event_key->type_)
    {
    case ROUTING_EVENT:
      this->routing_event (event_key,
                           data);
      break;
    case SUBSCRIPTION_EVENT:
      this->subscription_event (data);
      break;
    }

  // Release the memory in the message block.
  event->release ();
  return 0;
}

void
Event_Channel::subscription_event (ACE_Message_Block *data)
{
  Event *event = (Event *) data->rd_ptr ();

  ACE_DEBUG ((LM_DEBUG,
              "(%t) received a subscription with %d bytes from connection id %d\n",
              event->header_.len_,
              event->header_.connection_id_));
  Subscription *subscription = (Subscription *) event->data_;
  // Convert the subscription into host byte order so that we can
  // access it directly without having to repeatedly muck with it...
  subscription->decode ();

  ACE_DEBUG ((LM_DEBUG,
              "(%t) connection_id_ = %d, total_consumers_ = %d\n",
              subscription->connection_id_,
              subscription->total_consumers_));

  for (ACE_INT32 i = 0;
       i < subscription->total_consumers_;
       i++)
    ACE_DEBUG ((LM_DEBUG,
                "(%t) consumers_[%d] = %d\n",
                i,
                subscription->consumers_[i]));

}

void
Event_Channel::routing_event (Event_Key *forwarding_address,
                              ACE_Message_Block *data)
{
  Consumer_Dispatch_Set *dispatch_set = 0;

  // Initialize the <dispatch_set> to points to the set of Consumers
  // associated with this forwarding address.

  if (this->efd_.find (*forwarding_address,
                       dispatch_set) == -1)
    // Failure.
    ACE_ERROR ((LM_DEBUG,
                "(%t) find failed on connection id = %d, type = %d\n",
                forwarding_address->connection_id_,
                forwarding_address->type_));
  else
    {
      // Check to see if there are any consumers.
      if (dispatch_set->size () == 0)
        ACE_DEBUG ((LM_WARNING,
                    "there are no active consumers for this event currently\n"));

      else // There are consumers, so forward the event.
        {
          // Initialize the interator.
          Consumer_Dispatch_Set_Iterator dsi (*dispatch_set);

          // At this point, we should assign a thread-safe locking
          // strategy to the <ACE_Message_Block> is we're running in a
          // multi-threaded configuration.
          data->locking_strategy (Options::instance ()->locking_strategy ());

          for (Connection_Handler **connection_handler = 0;
               dsi.next (connection_handler) != 0;
               dsi.advance ())
            {
              // Only process active connection_handlers.
              if ((*connection_handler)->state () == Connection_Handler::ESTABLISHED)
                {
                  // Duplicate the event portion via reference
                  // counting.
                  ACE_Message_Block *dup_msg = data->duplicate ();

                  ACE_DEBUG ((LM_DEBUG,
                              "(%t) forwarding to Consumer %d\n",
                              (*connection_handler)->connection_id ()));

                  if ((*connection_handler)->put (dup_msg) == -1)
                    {
                      if (errno == EWOULDBLOCK) // The queue has filled up!
                        ACE_ERROR ((LM_ERROR,
                                    "(%t) %p\n",
                                    "gateway is flow controlled, so we're dropping events"));
                      else
                        ACE_ERROR ((LM_ERROR,
                                    "(%t) %p transmission error to peer %d\n",
                                    "put",
                                    (*connection_handler)->connection_id ()));

                      // We are responsible for releasing an
                      // ACE_Message_Block if failures occur.
                      dup_msg->release ();
                    }
                }
            }
        }
    }
}

int
Event_Channel::initiate_connection_connection (Connection_Handler *connection_handler,
                                               int sync_directly)
{
  ACE_Synch_Options synch_options;

  if (sync_directly)
    // In separated connection handler thread, connection can be
    // initiated by block mode (synch mode) directly.
    synch_options = ACE_Synch_Options::synch;
  else if (Options::instance ()->blocking_semantics () == ACE_NONBLOCK)
    synch_options = ACE_Synch_Options::asynch;
  else
    synch_options = ACE_Synch_Options::synch;

  return this->connector_.initiate_connection (connection_handler,
                                               synch_options);
}

int
Event_Channel::complete_connection_connection (Connection_Handler *connection_handler)
{
  int option = connection_handler->connection_role () == 'S'
    ? SO_RCVBUF
    : SO_SNDBUF;
  int socket_queue_size =
    Options::instance ()->socket_queue_size ();

  if (socket_queue_size > 0)
    if (connection_handler->peer ().set_option (SOL_SOCKET,
                                                option,
                                                &socket_queue_size,
                                                sizeof (int)) == -1)
      ACE_ERROR ((LM_ERROR,
                  "(%t) %p\n",
                  "set_option"));

  connection_handler->thr_mgr (ACE_Thread_Manager::instance ());

  // Our state is now "established."
  connection_handler->state (Connection_Handler::ESTABLISHED);

  // Restart the timeout to 1.
  connection_handler->timeout (1);

  ACE_INT32 id = htonl (connection_handler->connection_id ());

  // Send the connection id to the peerd.

  ssize_t n = connection_handler->peer ().send ((const void *) &id,
                                                sizeof id);

  if (n != sizeof id)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "(%t) %p\n",
                      n == 0 ? "peer has closed down unexpectedly" : "send"),
                      -1);
  return 0;
}

// Restart connection (blocking_semantics dicates whether we restart
// synchronously or asynchronously).

int
Event_Channel::reinitiate_connection_connection (Connection_Handler *connection_handler)
{
  // Cancel asynchronous connecting before re-initializing.  It will
  // close the peer and cancel the asynchronous connecting.
  this->cancel_connection_connection(connection_handler);

  if (connection_handler->state () != Connection_Handler::DISCONNECTING)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "(%t) scheduling reinitiation of Connection_Handler %d\n",
                  connection_handler->connection_id ()));

      // Reschedule ourselves to try and connect again.
      ACE_Time_Value const timeout (connection_handler->timeout ());
      if (ACE_Reactor::instance ()->schedule_timer
          (connection_handler,
           0,
           timeout) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           "(%t) %p\n",
                           "schedule_timer"),
                          -1);
    }
  return 0;
}

// It is useful to provide a separate method to cancel the
// asynchronous connecting.

int
Event_Channel::cancel_connection_connection (Connection_Handler *connection_handler)
{
  // Skip over proxies with deactivated handles.
  if (connection_handler->get_handle () != ACE_INVALID_HANDLE)
    {
      // Make sure to close down peer to reclaim descriptor.
      connection_handler->peer ().close ();
      // Cancel asynchronous connecting before re-initializing.
      return this->connector_.cancel(connection_handler);
    }
  return 0;
}

// Initiate active connections with the Consumer and Supplier Peers.

void
Event_Channel::initiate_connector (void)
{
  if (Options::instance ()->enabled
      (Options::CONSUMER_CONNECTOR | Options::SUPPLIER_CONNECTOR))
    {
      CONNECTION_MAP_ITERATOR cmi (this->connection_map_);

      // Iterate through the Consumer Map connecting all the
      // Connection_Handlers.

      for (CONNECTION_MAP_ENTRY *me = 0;
           cmi.next (me) != 0;
           cmi.advance ())
        {
          Connection_Handler *connection_handler = me->int_id_;

          if (this->initiate_connection_connection (connection_handler) == -1)
            continue; // Failures are handled elsewhere...
        }
    }
}

// Initiate passive acceptor to wait for Consumer and Supplier Peers
// to accept.

int
Event_Channel::initiate_acceptors (void)
{
  if (Options::instance ()->enabled (Options::CONSUMER_ACCEPTOR))
    {
      ACE_INET_Addr
        consumer_addr (Options::instance ()->consumer_acceptor_port ());
      if (this->consumer_acceptor_.open
          (consumer_addr,
           ACE_Reactor::instance (),
           Options::instance ()->blocking_semantics ()) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           "%p\n",
                           "cannot register acceptor"),
                          -1);
      else
        ACE_DEBUG ((LM_DEBUG,
                    "accepting Consumers at %d\n",
                    Options::instance ()->consumer_acceptor_port ()));
    }
  if (Options::instance ()->enabled (Options::SUPPLIER_ACCEPTOR))
    {
      ACE_INET_Addr
        supplier_addr (Options::instance ()->supplier_acceptor_port ());
      if (this->supplier_acceptor_.open
          (supplier_addr,
           ACE_Reactor::instance (),
           Options::instance ()->blocking_semantics ()) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           "%p\n",
                           "cannot register acceptor"),
                          -1);
      else
        ACE_DEBUG ((LM_DEBUG,
                    "accepting Suppliers at %d\n",
                    Options::instance ()->supplier_acceptor_port ()));
    }

  return 0;
}

// This method gracefully shuts down all the Handlers in the
// Connection_Handler Connection Map.

int
Event_Channel::close (u_long)
{
  if (Options::instance ()->threading_strategy () != Options::REACTIVE)
    {
      if (ACE_Thread_Manager::instance ()->suspend_all () == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           "(%t) %p\n",
                           "suspend_all"),
                          -1);
      ACE_DEBUG ((LM_DEBUG,
                  "(%t) suspending all threads\n"));
    }

  // First tell everyone that the spaceship is here...
  {
    CONNECTION_MAP_ITERATOR cmi (this->connection_map_);

    // Iterate over all the handlers and shut them down.

    for (CONNECTION_MAP_ENTRY *me = 0; // It's safe to reset me to 0.
         cmi.next (me) != 0;
         cmi.advance ())
      {
        Connection_Handler *connection_handler = me->int_id_;

        ACE_DEBUG ((LM_DEBUG,
                    "(%t) closing down connection %d\n",
                    connection_handler->connection_id ()));

        // If have no this statement, the gatewayd will abort when exiting
        // with some Consumer/Supplier not connected.
        if (connection_handler->state()==Connection_Handler::CONNECTING)
          this->cancel_connection_connection(connection_handler);
        // Mark Connection_Handler as DISCONNECTING so we don't try to
        // reconnect...
        connection_handler->state (Connection_Handler::DISCONNECTING);
      }
  }

  // Close down the connector
  this->connector_.close ();

  // Close down the supplier acceptor.
  this->supplier_acceptor_.close ();

  // Close down the consumer acceptor.
  this->consumer_acceptor_.close ();

  // Now tell everyone that it is now time to commit suicide.
  {
    CONNECTION_MAP_ITERATOR cmi (this->connection_map_);

    for (CONNECTION_MAP_ENTRY *me = 0; // It's safe to reset me to 0.
         cmi.next (me) != 0;
         cmi.advance ())
      {
        Connection_Handler *connection_handler = me->int_id_;

        // Deallocate Connection_Handler resources.
        connection_handler->destroy (); // Will trigger a delete.
      }
  }

  return 0;
}

int
Event_Channel::find_proxy (ACE_INT32 connection_id,
                               Connection_Handler *&connection_handler)
{
  return this->connection_map_.find (connection_id,
                                     connection_handler);
}

int
Event_Channel::bind_proxy (Connection_Handler *connection_handler)
{
  int result = this->connection_map_.bind (connection_handler->connection_id (),
                                           connection_handler);

  switch (result)
    {
    case -1:
      ACE_ERROR_RETURN ((LM_ERROR,
                         "(%t) bind failed for connection %d\n",
                         connection_handler->connection_id ()),
                        -1);
      /* NOTREACHED */
    case 1: // Oops, found a duplicate!
      ACE_ERROR_RETURN ((LM_ERROR,
                         "(%t) duplicate connection %d, already bound\n",
                         connection_handler->connection_id ()),
                        -1);
      /* NOTREACHED */
    case 0:
      // Success.
      return 0;
      /* NOTREACHED */
    default:
      ACE_ERROR_RETURN ((LM_DEBUG,
                         "(%t) invalid result %d\n",
                         result),
                        -1);
      /* NOTREACHED */
    }

  ACE_NOTREACHED (return 0);
}

int
Event_Channel::subscribe (const Event_Key &event_addr,
                          Consumer_Dispatch_Set *cds)
{
  int result = this->efd_.bind (event_addr, cds);

  // Bind with consumer map, keyed by peer address.
  switch (result)
    {
    case -1:
      ACE_ERROR_RETURN ((LM_ERROR,
                         "(%t) bind failed for connection %d\n",
                         event_addr.connection_id_),
                        -1);
      /* NOTREACHED */
    case 1: // Oops, found a duplicate!
      ACE_ERROR_RETURN ((LM_DEBUG,
                         "(%t) duplicate consumer map entry %d, "
                         "already bound\n",
                         event_addr.connection_id_),
                        -1);
      /* NOTREACHED */
    case 0:
      // Success.
      return 0;
    default:
      ACE_ERROR_RETURN ((LM_DEBUG,
                         "(%t) invalid result %d\n",
                         result),
                        -1);
      /* NOTREACHED */
    }

  ACE_NOTREACHED (return 0);
}

int
Event_Channel::open (void *)
{
  // Ignore <SIGPIPE> so each <Consumer_Handler> can handle it.
  ACE_Sig_Action sig ((ACE_SignalHandler) SIG_IGN, SIGPIPE);
  ACE_UNUSED_ARG (sig);

  // Actively initiate Peer connections.
  this->initiate_connector ();

  // Passively initiate Peer acceptor.
  if (this->initiate_acceptors () == -1)
    return -1;

  // If we're not running reactively, then we need to make sure that
  // <ACE_Message_Block> reference counting operations are
  // thread-safe.  Therefore, we create an <ACE_Lock_Adapter> that is
  // parameterized by <ACE_SYNCH_MUTEX> to prevent race conditions.
  if (Options::instance ()->threading_strategy ()
      != Options::REACTIVE)
    {
      ACE_Lock_Adapter<ACE_SYNCH_MUTEX> *la;

      ACE_NEW_RETURN (la,
                      ACE_Lock_Adapter<ACE_SYNCH_MUTEX>,
                      -1);

      Options::instance ()->locking_strategy (la);
    }

  return 0;
}

