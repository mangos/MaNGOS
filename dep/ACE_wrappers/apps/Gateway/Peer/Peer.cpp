// $Id: Peer.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#define ACE_BUILD_SVC_DLL

#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/Signal.h"
#include "Peer.h"

Peer_Handler::Peer_Handler (void)
  : connection_id_ (-1),  // Maybe it's better than 0.
    msg_frag_ (0),
    total_bytes_ (0)
{
  // Set the high water mark of the <ACE_Message_Queue>.  This is used
  // to exert flow control.
  this->msg_queue ()->high_water_mark (Options::instance ()->max_queue_size ());
  first_time_ = 1;  // It will be first time to open Peer_Handler.
}

// Upcall from the <ACE_Acceptor::handle_input> that turns control
// over to our application-specific Gateway handler.

int
Peer_Handler::open (void *a)
{
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("handle = %d\n"),
              this->peer ().get_handle ()));

  // Call down to the base class to activate and register this handler
  // with an <ACE_Reactor>.
  if (this->inherited::open (a) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("open")),
                      -1);

  if (this->peer ().enable (ACE_NONBLOCK) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("enable")),
                      -1);

  ACE_Time_Value timeout (Options::instance ()->timeout ());

  // Schedule the time between disconnects.  This should really be a
  // "tunable" parameter.
  if (ACE_Reactor::instance ()->schedule_timer
      (this, 0, timeout) == -1)
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT ("%p\n"),
                ACE_TEXT ("schedule_timer")));

  // If there are events left in the queue, make sure we enable the
  // <ACE_Reactor> appropriately to get them sent out.
  if (this->msg_queue ()->is_empty () == 0
      && ACE_Reactor::instance ()->schedule_wakeup
          (this, ACE_Event_Handler::WRITE_MASK) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("schedule_wakeup")),
                      -1);

  // First action is to wait to be notified of our connection id.
  this->do_action_ = &Peer_Handler::await_connection_id;
  return 0;
}

int
Peer_Handler::transmit (ACE_Message_Block *mb,
                        size_t n,
                        int event_type)
{
  Event *event = (Event *) mb->rd_ptr ();

  // Initialize the header.
  new (&event->header_) Event_Header (n,
                                      this->connection_id_,
                                      event_type,
                                      0);

  // Convert all the fields into network byte order.
  event->header_.encode ();

  // Move the write pointer to the end of the event.
  mb->wr_ptr (sizeof (Event_Header) + n);

  if (this->put (mb) == -1)
    {
      if (errno == EWOULDBLOCK) // The queue has filled up!
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT ("%p\n"),
                    ACE_TEXT ("gateway is flow controlled, so we're dropping events")));
      else
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT ("%p\n"),
                    ACE_TEXT ("transmission failure in transmit()"))); // Function name fixed.
      // Caller is responsible for freeing a ACE_Message_Block
      // if failures occur.
      mb->release ();
      return -1;
    }
  return 0;
}

// Read events from stdin and send them to the gatewayd.

int
Peer_Handler::transmit_stdin (void)
{
  // If return value is -1, then first_time_ must be reset to 1.
  int result = 0;
  if (this->connection_id_ != -1)
    {
      ACE_Message_Block *mb = 0;

      ACE_NEW_RETURN (mb,
                      ACE_Message_Block (sizeof (Event)),
                      -1);

      // Cast the message block payload into an <Event> pointer.
      Event *event = (Event *) mb->rd_ptr ();

      ssize_t n = ACE_OS::read (ACE_STDIN,
                                event->data_,
                                sizeof event->data_);
      switch (n)
        {
        case 0:
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT ("stdin closing down\n")));

          // Take stdin out of the ACE_Reactor so we stop trying to
          // send events.
          ACE_Reactor::instance ()->remove_handler
            (ACE_STDIN,
             ACE_Event_Handler::DONT_CALL | ACE_Event_Handler::READ_MASK);
          mb->release ();
          result = 0; //
          break;
          /* NOTREACHED */
        case -1:
          mb->release ();
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT ("%p\n"),
                      ACE_TEXT ("read")));
          result = 0; //
          break;
          /* NOTREACHED */
        default:
          // Do not return directly, save the return value.
          result = this->transmit (mb, n, ROUTING_EVENT);
          break;
          /* NOTREACHED */
        }

      // Do not return at here, but at exit of function.
      /*return 0;*/
    }
  else
  {
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("Must transmit over an opened channel.\n")));
    result = -1; // Save return value at here, return at exit of function.
  }
  // If transmit error, the stdin-thread will be cancelled, so should
  // reset first_time_ to 1, which will register_stdin_handler again.
  if (result == -1)
    first_time_ = 1;

  return result;
}

// Perform a non-blocking <put> of event MB.  If we are unable to send
// the entire event the remainder is re-queue'd at the *front* of the
// Message_Queue.

int
Peer_Handler::nonblk_put (ACE_Message_Block *mb)
{
  // Try to send the event.  If we don't send it all (e.g., due to
  // flow control), then re-queue the remainder at the head of the
  // <ACE_Message_Queue> and ask the <ACE_Reactor> to inform us (via
  // <handle_output>) when it is possible to try again.

  ssize_t n = this->send (mb);

  if (n == -1)
    // -1 is returned only when things have really gone wrong (i.e.,
    // not when flow control occurs).
    return -1;
  else if (errno == EWOULDBLOCK)
    {
      // We didn't manage to send everything, so requeue.
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("queueing activated on handle %d to connection id %d\n"),
                 this->get_handle (),
                  this->connection_id_));

      // Re-queue in *front* of the list to preserve order.
      if (this->msg_queue ()->enqueue_head
          (mb,
           (ACE_Time_Value *) &ACE_Time_Value::zero) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("enqueue_head")),
                          -1);
      // Tell ACE_Reactor to call us back when we can send again.
      if (ACE_Reactor::instance ()->schedule_wakeup
          (this, ACE_Event_Handler::WRITE_MASK) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("schedule_wakeup")),
                          -1);
      return 0;
    }
  else
    return n;
}

// Finish sending a event when flow control conditions abate.  This
// method is automatically called by the ACE_Reactor.

int
Peer_Handler::handle_output (ACE_HANDLE)
{
  ACE_Message_Block *mb = 0;

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("in handle_output\n")));

  if (this->msg_queue ()->dequeue_head
      (mb,
       (ACE_Time_Value *) &ACE_Time_Value::zero) != -1)
    {
      switch (this->nonblk_put (mb))
        {
        case 0:           // Partial send.
          ACE_ASSERT (errno == EWOULDBLOCK);
          // Didn't write everything this time, come back later...
          break;
          /* NOTREACHED */
        case -1:
          // Caller is responsible for freeing a ACE_Message_Block if
          // failures occur.
          mb->release ();
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT ("%p\n"),
                      ACE_TEXT ("transmission failure in handle_output")));
          /* FALLTHROUGH */
        default: // Sent the whole thing.
          // If we succeed in writing the entire event (or we did not
          // fail due to EWOULDBLOCK) then check if there are more
          // events on the <ACE_Message_Queue>.  If there aren't, tell
          // the <ACE_Reactor> not to notify us anymore (at least
          // until there are new events queued up).

          if (this->msg_queue ()->is_empty ())
            {
              ACE_DEBUG ((LM_DEBUG,
                          ACE_TEXT ("queue now empty on handle %d to connection id %d\n"),
                          this->get_handle (),
                          this->connection_id_));

              if (ACE_Reactor::instance ()->cancel_wakeup
                  (this, ACE_Event_Handler::WRITE_MASK) == -1)
                ACE_ERROR ((LM_ERROR,
                            ACE_TEXT ("%p\n"),
                            ACE_TEXT ("cancel_wakeup")));
            }
        }
      return 0;
    }
  else
    // If the list is empty there's a bug!
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("dequeue_head")),
                      0);
}

// Send an event to a peer (may block if necessary).

int
Peer_Handler::put (ACE_Message_Block *mb, ACE_Time_Value *)
{
  if (this->msg_queue ()->is_empty ())
    // Try to send the event *without* blocking!
    return this->nonblk_put (mb);
  else
    // If we have queued up events due to flow control then just
    // enqueue and return.
    return this->msg_queue ()->enqueue_tail
      (mb, (ACE_Time_Value *) &ACE_Time_Value::zero);
}

// Send an Peer event to gatewayd.

int
Peer_Handler::send (ACE_Message_Block *mb)
{
  size_t len = mb->length ();

  ssize_t n = this->peer ().send (mb->rd_ptr (), len);

  if (n <= 0)
    return errno == EWOULDBLOCK ? 0 : n;
  else if (n < (ssize_t) len)
    {
      // Re-adjust pointer to skip over the part we did send.
      mb->rd_ptr (n);
      this->total_bytes_ += n;
    }
  else // if (n == length).
    {
      // The whole event is sent, we can now safely deallocate the
      // buffer.  Note that this should decrement a reference count...
      this->total_bytes_ += n;
      mb->release ();
      errno = 0;
    }

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("sent %d bytes, total bytes sent = %d\n"),
              n,
              this->total_bytes_));
  return n;
}

// Receive an Event from gatewayd.  Handles fragmentation.

int
Peer_Handler::recv (ACE_Message_Block *&mb)
{
  if (this->msg_frag_ == 0)
    // No existing fragment...
    ACE_NEW_RETURN (this->msg_frag_,
                    ACE_Message_Block (sizeof (Event)),
                    -1);

  Event *event = (Event *) this->msg_frag_->rd_ptr ();
  ssize_t header_received = 0;

  const size_t HEADER_SIZE = sizeof (Event_Header);
  ssize_t header_bytes_left_to_read =
    HEADER_SIZE - this->msg_frag_->length ();

  if (header_bytes_left_to_read > 0)
    {
      header_received = this->peer ().recv
        (this->msg_frag_->wr_ptr (),
         header_bytes_left_to_read);

      if (header_received == -1 /* error */
          || header_received == 0  /* EOF */)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT ("%p\n"),
                      ACE_TEXT ("Recv error during header read")));
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT ("attempted to read %d bytes\n"),
                      header_bytes_left_to_read));
          this->msg_frag_ = this->msg_frag_->release ();
          return header_received;
        }

      // Bump the write pointer by the amount read.
      this->msg_frag_->wr_ptr (header_received);

      // At this point we may or may not have the ENTIRE header.
      if (this->msg_frag_->length () < HEADER_SIZE)
        {
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT ("Partial header received: only %d bytes\n"),
                      this->msg_frag_->length ()));
          // Notify the caller that we didn't get an entire event.
          errno = EWOULDBLOCK;
          return -1;
        }

      // Convert the header into host byte order so that we can access
      // it directly without having to repeatedly muck with it...
      event->header_.decode ();

      if (event->header_.len_ > ACE_INT32 (sizeof event->data_))
        {
          // This data_ payload is too big!
          errno = EINVAL;
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT ("Data payload is too big (%d bytes)\n"),
                      event->header_.len_));
          return -1;
        }
    }

  // At this point there is a complete, valid header in Event.  Now we
  // need to get the event payload.  Due to incomplete reads this may
  // not be the first time we've read in a fragment for this message.
  // We account for this here.  Note that the first time in here
  // <msg_frag_->wr_ptr> will point to <event->data_>.  Every time we
  // do a successful fragment read, we advance <wr_ptr>.  Therefore,
  // by subtracting how much we've already read from the
  // <event->header_.len_> we complete the
  // <data_bytes_left_to_read>...

  ssize_t data_bytes_left_to_read =
    ssize_t (event->header_.len_ - (msg_frag_->wr_ptr () - event->data_));

  // peer().recv() should not be called when data_bytes_left_to_read is 0.
  ssize_t data_received = !data_bytes_left_to_read ? 0 :
    this->peer ().recv (this->msg_frag_->wr_ptr (),
                        data_bytes_left_to_read);

  // Try to receive the remainder of the event.

  switch (data_received)
    {
    case -1:
      if (errno == EWOULDBLOCK)
        // This might happen if only the header came through.
        return -1;
      /* FALLTHROUGH */;

    case 0: // Premature EOF.
      if (data_bytes_left_to_read)
      {
      this->msg_frag_ = this->msg_frag_->release ();
      return 0;
      }
      /* FALLTHROUGH */;

    default:
      // Set the write pointer at 1 past the end of the event.
      this->msg_frag_->wr_ptr (data_received);

      if (data_received != data_bytes_left_to_read)
        {
          errno = EWOULDBLOCK;
          // Inform caller that we didn't get the whole event.
          return -1;
        }
      else
        {
          // Set the read pointer to the beginning of the event.
          this->msg_frag_->rd_ptr (this->msg_frag_->base ());

          mb = this->msg_frag_;

          // Reset the pointer to indicate we've got an entire event.
          this->msg_frag_ = 0;
        }

      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("(%t) connection id = %d, cur len = %d, total bytes read = %d\n"),
                  event->header_.connection_id_,
                  event->header_.len_,
                  data_received + header_received));
      if (Options::instance ()->enabled (Options::VERBOSE))
        ACE_DEBUG ((LM_DEBUG,
                    ACE_TEXT ("data_ = %*s\n"),
                    event->header_.len_ - 2,
                    event->data_));
      return data_received + header_received;
    }
}

// Receive various types of input (e.g., Peer event from the gatewayd,
// as well as stdio).

int
Peer_Handler::handle_input (ACE_HANDLE sd)
{
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("in handle_input, sd = %d\n"),
              sd));
  if (sd == ACE_STDIN) // Handle event from stdin.
    return this->transmit_stdin ();
  else
    // Perform the appropriate action depending on the state we are
    // in.
    return (this->*do_action_) ();
}

// Action that receives our connection id from the Gateway.

int
Peer_Handler::await_connection_id (void)
{
  ssize_t n = this->peer ().recv (&this->connection_id_,
                                  sizeof this->connection_id_);

  if (n != sizeof this->connection_id_)
    {
      if (n == 0)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("gatewayd has closed down unexpectedly\n")),
                          -1);
      else
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p, bytes received on handle %d = %d\n"),
                           ACE_TEXT ("recv"),
                           this->get_handle (),
                           n),
                          -1);
    }
  else
    {
      this->connection_id_ = ntohl (this->connection_id_);
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("assigned connection id %d\n"),
                  this->connection_id_));
    }

  // Subscribe for events if we're a Consumer.
  if (Options::instance ()->enabled (Options::CONSUMER_CONNECTOR))
    this->subscribe ();

  // No need to disconnect by timeout.
  ACE_Reactor::instance ()->cancel_timer(this);
  // Transition to the action that waits for Peer events.
  this->do_action_ = &Peer_Handler::await_events;

  // Reset standard input.
  ACE_OS::rewind (stdin);

  // Call register_stdin_handler only once, until the stdin-thread
  // closed which caused by transmit_stdin error.
  if (first_time_)
    {
      // Register this handler to receive test events on stdin.
      if (ACE_Event_Handler::register_stdin_handler
          (this,
           ACE_Reactor::instance (),
           ACE_Thread_Manager::instance ()) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("(%t) %p\n"),
                           ACE_TEXT ("register_stdin_handler")),
                          -1);

      // Next time in await_connection_id(), I'll don't call
      // register_stdin_handler().
      first_time_ = 0;
    }
  return 0;
}

int
Peer_Handler::subscribe (void)
{
  ACE_Message_Block *mb = 0;

  ACE_NEW_RETURN (mb,
                  ACE_Message_Block (sizeof (Event)),
                  -1);

  Subscription *subscription =
    (Subscription *) ((Event *) mb->rd_ptr ())->data_;
  subscription->connection_id_ =
    Options::instance ()->connection_id ();

  return this->transmit (mb, sizeof *subscription, SUBSCRIPTION_EVENT);
}

// Action that receives events from the Gateway.

int
Peer_Handler::await_events (void)
{
  ACE_Message_Block *mb = 0;

  ssize_t n = this->recv (mb);

  switch (n)
    {
    case 0:
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT ("gatewayd has closed down\n")),
                        -1);
      /* NOTREACHED */
    case -1:
      if (errno == EWOULDBLOCK)
        // A short-read, we'll come back and finish it up later on!
        return 0;
      else
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("recv")),
                          -1);
      /* NOTREACHED */
    default:
      {
        // We got a valid event, so let's process it now!  At the
        // moment, we just print out the event contents...

        Event *event = (Event *) mb->rd_ptr ();
        this->total_bytes_ += mb->length ();

        ACE_DEBUG ((LM_DEBUG,
                    ACE_TEXT ("route id = %d, cur len = %d, total len = %d\n"),
                    event->header_.connection_id_,
                    event->header_.len_,
                    this->total_bytes_));
        if (Options::instance ()->enabled (Options::VERBOSE))
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT ("data_ = %*s\n"),
                      event->header_.len_ - 2,
                      event->data_));
        mb->release ();
        return 0;
      }
    }
}

// Periodically send events via ACE_Reactor timer mechanism.

int
Peer_Handler::handle_timeout (const ACE_Time_Value &,
                              const void *)
{
  // Shut down the handler.
  return this->handle_close ();
}

Peer_Handler::~Peer_Handler (void)
{
  // Shut down the handler.
  this->handle_close ();
}

// Handle shutdown of the Peer object.

int
Peer_Handler::handle_close (ACE_HANDLE,
                            ACE_Reactor_Mask)
{
  if (this->get_handle () != ACE_INVALID_HANDLE)
    {
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("shutting down Peer on handle %d\n"),
                  this->get_handle ()));

      ACE_Reactor_Mask mask =
        ACE_Event_Handler::DONT_CALL | ACE_Event_Handler::READ_MASK;

      // Explicitly remove ourselves for ACE_STDIN (the <ACE_Reactor>
      // removes the HANDLE.  Note that <ACE_Event_Handler::DONT_CALL>
      // instructs the ACE_Reactor *not* to call <handle_close>, which
      // would otherwise lead to infinite recursion!).
      ACE_Reactor::instance ()->remove_handler
        (ACE_STDIN, mask);

      // Deregister this handler with the ACE_Reactor.
      if (ACE_Reactor::instance ()->remove_handler
          (this, mask) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("handle = %d: %p\n"),
                           this->get_handle (),
                           ACE_TEXT ("remove_handler")),
                          -1);
      // Close down the peer.
      this->peer ().close ();
    }
  return 0;
}

int
Peer_Acceptor::start (u_short port)
{
  // This object only gets allocated once and is just recycled
  // forever.
  ACE_NEW_RETURN (peer_handler_, Peer_Handler, -1);

  this->addr_.set (port);

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("opening acceptor at port %d\n"),
              port));

  // Call down to the <Acceptor::open> method.
  if (this->inherited::open (this->addr_) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("open")),
                      -1);
  else if (this->acceptor ().get_local_addr (this->addr_) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("get_local_addr")),
                      -1);
  else
    ACE_DEBUG ((LM_DEBUG,
                ACE_TEXT ("accepting at port %d\n"),
                this->addr_.get_port_number ()));
  return 0;
}

Peer_Acceptor::Peer_Acceptor (void)
  : peer_handler_ (0)
{
}

int
Peer_Acceptor::close (void)
{
  // Will trigger a delete.
  if (this->peer_handler_ != 0)
    this->peer_handler_->destroy ();

  // Close down the base class.
  return this->inherited::close ();
}

// Note how this method just passes back the pre-allocated
// <Peer_Handler> instead of having the <ACE_Acceptor> allocate a new
// one each time!

int
Peer_Acceptor::make_svc_handler (Peer_Handler *&sh)
{
  sh = this->peer_handler_;
  return 0;
}

int
Peer_Connector::open_connector (Peer_Handler *&peer_handler,
                                u_short port)
{
  // This object only gets allocated once and is just recycled
  // forever.
  ACE_NEW_RETURN (peer_handler,
                  Peer_Handler,
                  -1);

  ACE_INET_Addr addr (port,
                      Options::instance ()->connector_host ());

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("connecting to %s:%d\n"),
              addr.get_host_name (),
              addr.get_port_number ()));

  if (this->connect (peer_handler, addr) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("connect")),
                      -1);
  else
    ACE_DEBUG ((LM_DEBUG,
                ACE_TEXT ("connected to %C:%d\n"),
                addr.get_host_name (),
                addr.get_port_number ()));
  return 0;
}

int
Peer_Connector::open (ACE_Reactor *, int)
{
  this->supplier_peer_handler_ = 0;
  this->consumer_peer_handler_ = 0;

  if (Options::instance ()->enabled (Options::SUPPLIER_CONNECTOR)
      && this->open_connector (this->supplier_peer_handler_,
                               Options::instance ()->supplier_connector_port ()) == -1)
    return -1;

  if (Options::instance ()->enabled (Options::CONSUMER_CONNECTOR)
      && this->open_connector (this->consumer_peer_handler_,
                               Options::instance ()->consumer_connector_port ()) == -1)
    return -1;

  return 0;
}

int
Peer_Factory::handle_signal (int signum, siginfo_t *, ucontext_t *)
{
  if (signum != SIGPIPE)
  {
    // Shut down the main event loop.
    ACE_DEBUG((LM_NOTICE, ACE_TEXT ("Exit case signal\n"))); // Why do I exit?
    ACE_Reactor::instance ()->end_reactor_event_loop();
  }

  return 0;
}

// Returns information on the currently active service.

int
Peer_Factory::info (ACE_TCHAR **strp, size_t length) const
{
  ACE_TCHAR buf[BUFSIZ];
  ACE_TCHAR consumer_addr_str[BUFSIZ];
  ACE_TCHAR supplier_addr_str[BUFSIZ];

  ACE_INET_Addr addr;

  if (this->consumer_acceptor_.acceptor ().get_local_addr (addr) == -1)
    return -1;
  else if (addr.addr_to_string (consumer_addr_str,
                                sizeof addr) == -1)
    return -1;
  else if (this->supplier_acceptor_.acceptor ().get_local_addr (addr) == -1)
    return -1;
  else if (addr.addr_to_string (supplier_addr_str,
                                sizeof addr) == -1)
    return -1;

  ACE_OS::strcpy (buf, ACE_TEXT ("peerd\t C:"));
  ACE_OS::strcat (buf, consumer_addr_str);
  ACE_OS::strcat (buf, ACE_TEXT ("|S:"));
  ACE_OS::strcat (buf, supplier_addr_str);
  ACE_OS::strcat
    (buf, ACE_TEXT ("/tcp # Gateway traffic generator and data sink\n"));

  if (*strp == 0 && (*strp = ACE_OS::strdup (buf)) == 0)
    return -1;
  else
    ACE_OS::strncpy (*strp, buf, length);
  return ACE_OS::strlen (buf);
}

// Hook called by the explicit dynamic linking facility to terminate
// the peer.

int
Peer_Factory::fini (void)
{
  this->consumer_acceptor_.close ();
  this->supplier_acceptor_.close ();
  return 0;
}

// Hook called by the explicit dynamic linking facility to initialize
// the peer.

int
Peer_Factory::init (int argc, ACE_TCHAR *argv[])
{
  Options::instance ()->parse_args (argc, argv);

  ACE_Sig_Set sig_set;

  sig_set.sig_add (SIGINT);
  sig_set.sig_add (SIGQUIT);
  sig_set.sig_add (SIGPIPE);

  // Register ourselves to receive signals so we can shut down
  // gracefully.

  if (ACE_Reactor::instance ()->register_handler (sig_set,
                                                  this) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("register_handler")),
                      -1);

  if (Options::instance ()->enabled (Options::SUPPLIER_ACCEPTOR)
      && this->supplier_acceptor_.start
      (Options::instance ()->supplier_acceptor_port ()) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("Acceptor::open")),
                      -1);
  else if (Options::instance ()->enabled (Options::CONSUMER_ACCEPTOR)
           && this->consumer_acceptor_.start
           (Options::instance ()->consumer_acceptor_port ()) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("Acceptor::open")),
                      -1);
  else if (this->connector_.open () == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("Connector::open")),
                      -1);
  return 0;
}

// The following is a "Factory" used by the <ACE_Service_Config> and
// svc.conf file to dynamically initialize the <Peer_Acceptor> and
// <Peer_Connector>.

ACE_SVC_FACTORY_DEFINE (Peer_Factory)

