// $Id: Concrete_Connection_Handlers.cpp 91688 2010-09-09 11:21:50Z johnnyw $

#define ACE_BUILD_SVC_DLL

#include "ace/OS_NS_unistd.h"
#include "Event_Channel.h"
#include "Concrete_Connection_Handlers.h"

Consumer_Handler::Consumer_Handler (const Connection_Config_Info &pci)
  : Connection_Handler (pci)
{
  this->connection_role_ = 'C';
  this->msg_queue ()->high_water_mark (Options::instance ()->max_queue_size ());
}

// This method should be called only when the Consumer shuts down
// unexpectedly.  This method simply marks the Connection_Handler as
// having failed so that handle_close () can reconnect.

// Do not close handler when received data successfully.
// Consumer_Handler should could process received data.
// For example, Consumer could send reply-event to Supplier.
int
Consumer_Handler::handle_input (ACE_HANDLE)
{
  // Do not set FAILED state at here, just at real failed place.

  char buf[BUFSIZ];
  ssize_t received = this->peer ().recv (buf, sizeof buf);

  switch (received)
    {
    case -1:
      this->state (Connection_Handler::FAILED);
      ACE_ERROR_RETURN ((LM_ERROR,
                        "(%t) Peer has failed unexpectedly for Consumer_Handler %d\n",
                        this->connection_id ()),
                        -1);
      /* NOTREACHED */
    case 0:
      this->state (Connection_Handler::FAILED);
      ACE_ERROR_RETURN ((LM_ERROR,
                        "(%t) Peer has shutdown unexpectedly for Consumer_Handler %d\n",
                        this->connection_id ()),
                        -1);
      /* NOTREACHED */
    default:
      ACE_ERROR_RETURN ((LM_ERROR,
                        "(%t) IGNORED: Consumer is erroneously sending input to Consumer_Handler %d\n"
                        "data size = %d\n",
                        this->connection_id (),
                        received),
                        0); // Return 0 to identify received data successfully.
      /* NOTREACHED */
    }
}

// Perform a non-blocking put() of event.  If we are unable to send
// the entire event the remainder is re-queued at the *front* of the
// Event_List.

int
Consumer_Handler::nonblk_put (ACE_Message_Block *event)
{
  // Try to send the event.  If we don't send it all (e.g., due to
  // flow control), then re-queue the remainder at the head of the
  // Event_List and ask the ACE_Reactor to inform us (via
  // handle_output()) when it is possible to try again.

  ssize_t n = this->send (event);

  if (n == -1)
    {
      // -1 is returned only when things have really gone wrong (i.e.,
      // not when flow control occurs).  Thus, let's try to close down
      // and set up a new reconnection by calling handle_close().
      this->state (Connection_Handler::FAILED);
      this->handle_close ();
      return -1;
    }
  else if (errno == EWOULDBLOCK)
    {
      // We didn't manage to send everything, so we need to queue
      // things up.

      ACE_DEBUG ((LM_DEBUG,
                  "(%t) queueing activated on handle %d to routing id %d\n",
                  this->get_handle (),
                  this->connection_id ()));

      // ACE_Queue in *front* of the list to preserve order.
      if (this->msg_queue ()->enqueue_head
          (event, (ACE_Time_Value *) &ACE_Time_Value::zero) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           "(%t) %p\n",
                           "enqueue_head"),
                          -1);

      // Tell ACE_Reactor to call us back when we can send again.
      else if (ACE_Reactor::instance ()->schedule_wakeup
               (this, ACE_Event_Handler::WRITE_MASK) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           "(%t) %p\n",
                           "schedule_wakeup"),
                          -1);
      return 0;
    }
  else
    return n;
}

ssize_t
Consumer_Handler::send (ACE_Message_Block *event)
{
  ACE_DEBUG ((LM_DEBUG,
              "(%t) sending %d bytes to Consumer %d\n",
              event->length (),
              this->connection_id ()));

  ssize_t len = event->length ();
  ssize_t n = this->peer ().send (event->rd_ptr (), len);

  if (n <= 0)
    return errno == EWOULDBLOCK ? 0 : n;
  else if (n < len)
    {
      // Re-adjust pointer to skip over the part we did send.
      event->rd_ptr (n);
      errno = EWOULDBLOCK;
    }
  else // if (n == length)
    {
      // The whole event is sent, we now decrement the reference count
      // (which deletes itself with it reaches 0).
      event->release ();
      errno = 0;
    }
  this->total_bytes (n);
  return n;
}

// Finish sending an event when flow control conditions abate.
// This method is automatically called by the ACE_Reactor.

int
Consumer_Handler::handle_output (ACE_HANDLE)
{
  ACE_Message_Block *event = 0;

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT("(%t) Receiver signalled 'resume transmission' %d\n"),
              this->get_handle ()));

  // WIN32 Notes: When the receiver blocked, we started adding to the
  // consumer handler's message Q. At this time, we registered a
  // callback with the reactor to tell us when the TCP layer signalled
  // that we could continue to send messages to the consumer. However,
  // Winsock only sends this notification ONCE, so we have to assume
  // at the application level, that we can continue to send until we
  // get any subsequent blocking signals from the receiver's buffer.

#if defined (ACE_WIN32)
  // Win32 Winsock doesn't trigger multiple "You can write now"
  // signals, so we have to assume that we can continue to write until
  // we get another EWOULDBLOCK.

  // We cancel the wakeup callback we set earlier.
  if (ACE_Reactor::instance ()->cancel_wakeup
      (this, ACE_Event_Handler::WRITE_MASK) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("(%t) %p\n"),
                       ACE_TEXT ("Error in ACE_Reactor::cancel_wakeup()")),
                      -1);

  // The list had better not be empty, otherwise there's a bug!
  while (this->msg_queue ()->dequeue_head
         (event, (ACE_Time_Value *) &ACE_Time_Value::zero) != -1)
    {
      switch (this->nonblk_put (event))
        {
        case -1:                // Error sending message to consumer.
          {
            // We are responsible for releasing an ACE_Message_Block if
            // failures occur.
            event->release ();

            ACE_ERROR ((LM_ERROR,
                        ACE_TEXT ("(%t) %p\n"),
                        ACE_TEXT ("transmission failure")));
            break;
          }
        case 0:                 // Partial Send - we got flow controlled by the receiver
          {
            ACE_DEBUG ((LM_DEBUG,
                        ACE_TEXT ("%D Partial Send due to flow control")
                        ACE_TEXT ("- scheduling new wakeup with reactor\n")));

            // Re-schedule a wakeup call from the reactor when the
            // flow control conditions abate.
            if (ACE_Reactor::instance ()->schedule_wakeup
                (this,
                 ACE_Event_Handler::WRITE_MASK) == -1)
              ACE_ERROR_RETURN ((LM_ERROR,
                                 ACE_TEXT ("(%t) %p\n"),
                                 ACE_TEXT ("Error in ACE_Reactor::schedule_wakeup()")),
                                -1);

            // Didn't write everything this time, come back later...
            return 0;
          }
        default:                // Sent the whole thing
          {
            ACE_DEBUG ((LM_DEBUG,
                        ACE_TEXT ("Sent message from message Q, Q size = %d\n"),
                        this->msg_queue()->message_count ()));
            break;
          }
        }
    }

  // If we drop out of the while loop, then the message Q should be
  // empty...or there's a problem in the dequeue_head() call...but
  // thats another story.
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("%D Sent all messages from consumers message Q\n")));

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("(%t) queueing deactivated on handle %d to routing id %d\n"),
              this->get_handle (),
              this->connection_id ()));
#else /* !defined (ACE_WIN32) */
  // The list had better not be empty, otherwise there's a bug!
  if (this->msg_queue ()->dequeue_head
      (event, (ACE_Time_Value *) &ACE_Time_Value::zero) != -1)
    {
      switch (this->nonblk_put (event))
        {
        case 0:           // Partial send.
          ACE_ASSERT (errno == EWOULDBLOCK);
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT ("%D Partial Send\n")));

          // Didn't write everything this time, come back later...
          break;

        case -1:
          // We are responsible for releasing an ACE_Message_Block if
          // failures occur.
          event->release ();
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT ("(%t) %p\n"),
                      ACE_TEXT ("transmission failure")));

          /* FALLTHROUGH */
        default: // Sent the whole thing.

          // If we succeed in writing the entire event (or we did not
          // fail due to EWOULDBLOCK) then check if there are more
          // events on the Message_Queue.  If there aren't, tell the
          // ACE_Reactor not to notify us anymore (at least until
          // there are new events queued up).

          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT ("QQQ::Sent Message from consumer's Q\n")));

          if (this->msg_queue ()->is_empty ())
            {
              ACE_DEBUG ((LM_DEBUG,
                          ACE_TEXT ("(%t) queueing deactivated on handle %d to routing id %d\n"),
                          this->get_handle (),
                          this->connection_id ()));

              if (ACE_Reactor::instance ()->cancel_wakeup
                  (this, ACE_Event_Handler::WRITE_MASK) == -1)
                ACE_ERROR ((LM_ERROR,
                            ACE_TEXT ("(%t) %p\n"),
                            ACE_TEXT ("cancel_wakeup")));
            }
        }
    }
  else
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT ("(%t) %p\n"),
                ACE_TEXT ("dequeue_head - handle_output called by reactor but nothing in Q")));
#endif /* ACE_WIN32 */
  return 0;
}

// Send an event to a Consumer (may queue if necessary).

int
Consumer_Handler::put (ACE_Message_Block *event,
                       ACE_Time_Value *)
{
  if (this->msg_queue ()->is_empty ())
    // Try to send the event *without* blocking!
    return this->nonblk_put (event);
  else
    // If we have queued up events due to flow control then just
    // enqueue and return.
    return this->msg_queue ()->enqueue_tail
      (event, (ACE_Time_Value *) &ACE_Time_Value::zero);
}

Supplier_Handler::Supplier_Handler (const Connection_Config_Info &pci)
  : Connection_Handler (pci),
    msg_frag_ (0)
{
  this->connection_role_ = 'S';
  this->msg_queue ()->high_water_mark (0);
}

// Receive an Event from a Supplier.  Handles fragmentation.
//
// The event returned from recv consists of two parts:
//
// 1. The Address part, contains the "virtual" routing id.
//
// 2. The Data part, which contains the actual data to be forwarded.
//
// The reason for having two parts is to shield the higher layers
// of software from knowledge of the event structure.

int
Supplier_Handler::recv (ACE_Message_Block *&forward_addr)
{
  if (this->msg_frag_ == 0)
    // No existing fragment...
    ACE_NEW_RETURN (this->msg_frag_,
                    ACE_Message_Block (sizeof (Event),
                                       ACE_Message_Block::MB_DATA,
                                       0,
                                       0,
                                       0,
                                       Options::instance ()->locking_strategy ()),
                    -1);

  Event *event = (Event *) this->msg_frag_->rd_ptr ();
  ssize_t header_received = 0;

  const size_t HEADER_SIZE = sizeof (Event_Header);
  ssize_t header_bytes_left_to_read =
    HEADER_SIZE - this->msg_frag_->length ();

  if (header_bytes_left_to_read > 0)
    {
      header_received = this->peer ().recv
        (this->msg_frag_->wr_ptr (), header_bytes_left_to_read);

      if (header_received == -1 /* error */
          || header_received == 0  /* EOF */)
        {
          ACE_ERROR ((LM_ERROR, "%p\n",
                      "Recv error during header read "));
          ACE_DEBUG ((LM_DEBUG,
                      "attempted to read %d\n",
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
                      "Partial header received: only %d bytes\n",
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
                      "Data payload is too big (%d bytes)\n",
                      event->header_.len_));
          return -1;
        }

    }

  // At this point there is a complete, valid header in Event.  Now we
  // need to get the event payload.  Due to incomplete reads this may
  // not be the first time we've read in a fragment for this message.
  // We account for this here.  Note that the first time in here
  // msg_frag_->wr_ptr() will point to event->data_.  Every time we do
  // a successful fragment read, we advance wr_ptr().  Therefore, by
  // subtracting how much we've already read from the
  // event->header_.len_ we complete the data_bytes_left_to_read...

  ssize_t data_bytes_left_to_read =
    ssize_t (event->header_.len_ - (msg_frag_->wr_ptr () - event->data_));

  ssize_t data_received =
    !data_bytes_left_to_read
    ? 0 // peer().recv() should not be called when data_bytes_left_to_read is 0.
    : this->peer ().recv (this->msg_frag_->wr_ptr (), data_bytes_left_to_read);

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

          // Allocate an event forwarding header and chain the data
          // portion onto its continuation field.
          forward_addr = new ACE_Message_Block (sizeof (Event_Key),
                                                ACE_Message_Block::MB_PROTO,
                                                this->msg_frag_,
                                                0,
                                                0,
                                                Options::instance ()->locking_strategy ());
          if (forward_addr == 0)
            {
              this->msg_frag_ = this->msg_frag_->release ();
              errno = ENOMEM;
              return -1;
            }

          Event_Key event_addr (this->connection_id (),
                                event->header_.type_);
          // Copy the forwarding address from the Event_Key into
          // forward_addr.
          forward_addr->copy ((char *) &event_addr, sizeof (Event_Key));

          // Reset the pointer to indicate we've got an entire event.
          this->msg_frag_ = 0;
        }

      this->total_bytes (data_received + header_received);
      ACE_DEBUG ((LM_DEBUG,
                  "(%t) connection id = %d, cur len = %d, total bytes read = %d\n",
                  event->header_.connection_id_,
                  event->header_.len_,
                  data_received + header_received));
      if (Options::instance ()->enabled (Options::VERBOSE))
        ACE_DEBUG ((LM_DEBUG,
                    "data_ = %*s\n",
                    event->header_.len_ - 2,
                    event->data_));

      // Encode before returning so that we can set things out in
      // network byte order.
      event->header_.encode ();
      return data_received + header_received;
    }
}

// Receive various types of input (e.g., Peer event from the gatewayd,
// as well as stdio).

int
Supplier_Handler::handle_input (ACE_HANDLE)
{
  ACE_Message_Block *event_key = 0;

  switch (this->recv (event_key))
    {
    case 0:
      // Note that a peer shouldn't initiate a shutdown by closing the
      // connection.  Therefore, the peer must have crashed, so we'll
      // need to bail out here and let the higher layers reconnect.
      this->state (Connection_Handler::FAILED);
      ACE_ERROR_RETURN ((LM_ERROR,
                        "(%t) Peer has closed down unexpectedly for Input Connection_Handler %d\n",
                        this->connection_id ()),
                        -1);
      /* NOTREACHED */
    case -1:
      if (errno == EWOULDBLOCK)
        // A short-read, we'll come back and finish it up later on!
        return 0;
      else // A weird problem occurred, shut down and start again.
        {
          this->state (Connection_Handler::FAILED);
          ACE_ERROR_RETURN ((LM_ERROR, "(%t) %p for Input Connection_Handler %d\n",
                             "Peer has failed unexpectedly",
                             this->connection_id ()),
                            -1);
        }
      /* NOTREACHED */
    default:
      // Route messages to Consumers.
      return this->process (event_key);
    }
}

// This delegates to the <Event_Channel> to do the actual processing.
// Typically, this forwards the event to its appropriate Consumer(s).

int
Supplier_Handler::process (ACE_Message_Block *event_key)
{
  return this->event_channel_->put (event_key);
}

Thr_Consumer_Handler::Thr_Consumer_Handler (const Connection_Config_Info &pci)
  : Consumer_Handler (pci)
{
  // It is not in thread svc() now.
  in_thread_ = 0;
}

// Overriding handle_close() method.  If in thread svc(), no need to
// process handle_close() when call peer().close(), because the
// connection is blocked now.

int
Thr_Consumer_Handler::handle_close (ACE_HANDLE h, ACE_Reactor_Mask m)
{
  if (in_thread_)
    return 0;
  else
    return Consumer_Handler::handle_close (h, m);
}

// This method should be called only when the Consumer shuts down
// unexpectedly.  This method marks the Connection_Handler as having
// failed and deactivates the ACE_Message_Queue (to wake up the thread
// blocked on <dequeue_head> in svc()).
// Thr_Consumer_Handler::handle_close () will eventually try to
// reconnect...

// Let Consumer_Handler receive normal data.
int
Thr_Consumer_Handler::handle_input (ACE_HANDLE h)
{
  // Call down to the <Consumer_Handler> to handle this first.
  if (this->Consumer_Handler::handle_input (h) != 0)
    {
      // Only do such work when failed.

      ACE_Reactor::instance ()->remove_handler
        (h, ACE_Event_Handler::ALL_EVENTS_MASK | ACE_Event_Handler::DONT_CALL);

      // Deactivate the queue while we try to get reconnected.
      this->msg_queue ()->deactivate ();
      // Will call handle_close.
      return -1;
    }
  return 0;
}

// Initialize the threaded Consumer_Handler object and spawn a new
// thread.

int
Thr_Consumer_Handler::open (void *)
{
  // Turn off non-blocking I/O.
  if (this->peer ().disable (ACE_NONBLOCK) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "(%t) %p\n",
                       "disable"),
                      -1); // Incorrect info fixed.

  // Call back to the <Event_Channel> to complete our initialization.
  else if (this->event_channel_->complete_connection_connection (this) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "(%t) %p\n",
                       "complete_connection_connection"),
                      -1);

  // Register ourselves to receive input events (which indicate that
  // the Consumer has shut down unexpectedly).
  else if (ACE_Reactor::instance ()->register_handler
      (this, ACE_Event_Handler::READ_MASK) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "(%t) %p\n",
                       "register_handler"),
                      -1);

  // Reactivate message queue.  If it was active then this is the
  // first time in and we need to spawn a thread, otherwise the queue
  // was inactive due to some problem and we've already got a thread.
  else if (this->msg_queue ()->activate () == ACE_Message_Queue<ACE_SYNCH>::ACTIVATED)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "(%t) spawning new thread\n"));
      // Become an active object by spawning a new thread to transmit
      // events to Consumers.
      return this->activate (THR_NEW_LWP | THR_DETACHED);
    }
  else
    {
      ACE_DEBUG ((LM_DEBUG,
                  "(%t) reusing existing thread\n"));
      return 0;
    }
}

// Queue up an event for transmission (must not block since
// Supplier_Handlers may be single-threaded).

int
Thr_Consumer_Handler::put (ACE_Message_Block *mb, ACE_Time_Value *)
{
  // Perform non-blocking enqueue, i.e., if <msg_queue> is full
  // *don't* block!
  return this->msg_queue ()->enqueue_tail
    (mb, (ACE_Time_Value *) &ACE_Time_Value::zero);
}

// Transmit events to the peer.  Note the simplification resulting
// from the use of threads, compared with the Reactive solution.

int
Thr_Consumer_Handler::svc (void)
{
  for (in_thread_ = 1;;)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "(%t) Thr_Consumer_Handler's handle = %d\n",
                  this->peer ().get_handle ()));

      // Since this method runs in its own thread it is OK to block on
      // output.

      for (ACE_Message_Block *mb = 0;
           this->msg_queue ()->dequeue_head (mb) != -1;
           )
        if (this->send (mb) == -1)
          ACE_ERROR ((LM_ERROR,
                      "(%t) %p\n",
                      "send failed"));

      ACE_ASSERT (errno == ESHUTDOWN);

      ACE_DEBUG ((LM_DEBUG,
                  "(%t) shutting down threaded Consumer_Handler %d on handle %d\n",
                  this->connection_id (),
                  this->get_handle ()));

      this->peer ().close ();

      // Re-establish the connection, using exponential backoff.
      for (this->timeout (1);
           // Default is to reconnect synchronously.
           this->event_channel_->initiate_connection_connection (this, 1) == -1;
           // Second parameter '1' means using sync mode directly,
           // don't care Options::blocking_semantics().  If don't do
           // so, async mode will be used to connect which won't
           // satisfy original design.
           )
        {
          ACE_Time_Value tv (this->timeout ());

          ACE_ERROR ((LM_ERROR,
                      "(%t) reattempting connection, sec = %d\n",
                      tv.sec ()));

          ACE_OS::sleep (tv);
        }
    }

  ACE_NOTREACHED (return 0;)
}

Thr_Supplier_Handler::Thr_Supplier_Handler (const Connection_Config_Info &pci)
  : Supplier_Handler (pci)
{
  // It is not in thread svc() now.
  in_thread_ = 0;
}

// Overriding handle_close() method.  If in thread svc(), no need to
// process handle_close() when call peer().close(), because the
// connection is blocked now.

int
Thr_Supplier_Handler::handle_close (ACE_HANDLE h, ACE_Reactor_Mask m)
{
  if (in_thread_)
    return 0;
  else
    return Supplier_Handler::handle_close (h, m);
}

int
Thr_Supplier_Handler::open (void *)
{
  // Turn off non-blocking I/O.
  if (this->peer ().disable (ACE_NONBLOCK) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "(%t) %p\n",
                       "disable"),
                      -1); // Incorrect info fixed.

  // Call back to the <Event_Channel> to complete our initialization.
  else if (this->event_channel_->complete_connection_connection (this) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       "(%t) %p\n",
                       "complete_connection_connection"),
                      -1);

  // Reactivate message queue.  If it was active then this is the
  // first time in and we need to spawn a thread, otherwise the queue
  // was inactive due to some problem and we've already got a thread.
  else if (this->msg_queue ()->activate () == ACE_Message_Queue<ACE_SYNCH>::ACTIVATED)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "(%t) spawning new thread\n"));
      // Become an active object by spawning a new thread to transmit
      // events to peers.
      return this->activate (THR_NEW_LWP | THR_DETACHED);
    }
  else
    {
      ACE_DEBUG ((LM_DEBUG, "(%t) reusing existing thread\n"));
      return 0;
    }
}

// Receive events from a Peer in a separate thread (note reuse of
// existing code!).

int
Thr_Supplier_Handler::svc (void)
{
  for (in_thread_ = 1;;)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "(%t) Thr_Supplier_Handler's handle = %d\n",
                 this->peer ().get_handle ()));

      // Since this method runs in its own thread and processes events
      // for one connection it is OK to call down to the
      // <Supplier_Handler::handle_input> method, which blocks on
      // input.

      while (this->Supplier_Handler::handle_input () != -1)
        continue;

      ACE_DEBUG ((LM_DEBUG,
                  "(%t) shutting down threaded Supplier_Handler %d on handle %d\n",
                  this->connection_id (),
                  this->get_handle ()));

      this->peer ().close ();

      // Deactivate the queue while we try to get reconnected.
      this->msg_queue ()->deactivate ();

      // Re-establish the connection, using expoential backoff.
      for (this->timeout (1);
           // Default is to reconnect synchronously.
           this->event_channel_->initiate_connection_connection (this, 1) == -1;
           // Second parameter '1' means using sync mode directly,
           // don't care Options::blocking_semantics().  If don't do
           // so, async mode will be used to connect which won't
           // satisfy original design.
           )
        {
          ACE_Time_Value tv (this->timeout ());
          ACE_ERROR ((LM_ERROR,
                      "(%t) reattempting connection, sec = %d\n",
                      tv.sec ()));
          ACE_OS::sleep (tv);
        }
    }
  ACE_NOTREACHED(return 0);
}
