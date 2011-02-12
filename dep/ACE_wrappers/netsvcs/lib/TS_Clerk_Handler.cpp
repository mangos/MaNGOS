// $Id: TS_Clerk_Handler.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "ace/Get_Opt.h"
#include "TS_Clerk_Handler.h"
#include "ace/Signal.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_time.h"
#include "ace/os_include/os_netdb.h"



ACE_TS_Clerk_Handler::ACE_TS_Clerk_Handler (ACE_TS_Clerk_Processor *processor,
                                            ACE_INET_Addr &addr)
: state_ (ACE_TS_Clerk_Handler::IDLE),
  timeout_ (ACE_DEFAULT_TIMEOUT),
  max_timeout_ (ACE_TS_Clerk_Handler::MAX_RETRY_TIMEOUT),
  remote_addr_ (addr),
  processor_ (processor)
{
  ACE_TRACE ("ACE_TS_Clerk_Handler::ACE_TS_Clerk_Handler");
  this->time_info_.delta_time_ = 0;
  this->time_info_.sequence_num_ = 0;
}

// Set the connection state
void
ACE_TS_Clerk_Handler::state (ACE_TS_Clerk_Handler::State state)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::state"));
  this->state_ = state;
}

// Get the connection state
ACE_TS_Clerk_Handler::State
ACE_TS_Clerk_Handler::state (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::state"));
  return this->state_;
}

// Sets the timeout delay.
void
ACE_TS_Clerk_Handler::timeout (long to)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::timeout"));
  if (to > this->max_timeout_)
    to = this->max_timeout_;

  this->timeout_ = to;
}

// Recalculate the current retry timeout delay using exponential
// backoff.  Returns the original timeout (i.e., before the
// recalculation).
long
ACE_TS_Clerk_Handler::timeout (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::timeout"));
  long old_timeout = this->timeout_;
  this->timeout_ *= 2;

  if (this->timeout_ > this->max_timeout_)
    this->timeout_ = this->max_timeout_;

  return old_timeout;
}

// This is called when a <send> to the logging server fails...

int
ACE_TS_Clerk_Handler::handle_signal (int, siginfo_t *, ucontext_t *)
{
  return -1;
}

// Set the max timeout delay.
void
ACE_TS_Clerk_Handler::max_timeout (long mto)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::max_timeout"));
  this->max_timeout_ = mto;
}

// Gets the max timeout delay.
long
ACE_TS_Clerk_Handler::max_timeout (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::max_timeout"));
  return this->max_timeout_;
}

int
ACE_TS_Clerk_Handler::open (void *)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::open"));
  ACE_INET_Addr server_addr;

  // Set connection state as established
  this->state (ACE_TS_Clerk_Handler::ESTABLISHED);

  // Register ourselves to receive SIGPIPE so we can attempt
  // reconnections.
#if !defined (ACE_WIN32)
  if (ACE_Reactor::instance ()->register_handler (SIGPIPE, this) == -1)
    ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT ("%n: %p\n"),
                       ACE_TEXT ("register_handler (SIGPIPE)")), -1);
#endif /* ACE_WIN32 */

  // Register ourselves with the reactor to receive input
  if (ACE_Reactor::instance ()->register_handler (this->get_handle (),
                                                  this,
                                                        ACE_Event_Handler::READ_MASK |
                                                        ACE_Event_Handler::EXCEPT_MASK) == -1)
    ACE_ERROR ((LM_ERROR, ACE_TEXT ("%n: %p\n"),
                ACE_TEXT ("register_handler (this)")));

  // Figure out what remote port we're really bound to.
  else if (this->peer ().get_remote_addr (server_addr) == -1)
    ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT ("%p\n"),
                       ACE_TEXT ("get_remote_addr")),
                       -1);

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("TS Clerk Daemon connected to port %d on handle %d\n"),
              server_addr.get_port_number (),
              this->peer ().get_handle ()));

  return 0;
}

ACE_HANDLE
ACE_TS_Clerk_Handler::get_handle (void) const
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::get_handle"));
  return this->peer().get_handle ();
}

int
ACE_TS_Clerk_Handler::handle_close (ACE_HANDLE,
                                    ACE_Reactor_Mask mask)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::handle_close"));
  ACE_UNUSED_ARG (mask);

  ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%t) shutting down on handle %d\n"),
              this->get_handle ()));

  return this->reinitiate_connection ();
}

int
ACE_TS_Clerk_Handler::reinitiate_connection (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::reinitiate_connection"));
  // Skip over deactivated descriptors.

  // Set state to connecting so that we don't try to send anything
  // using this handler
  this->state (ACE_TS_Clerk_Handler::CONNECTING);
  if (this->get_handle () != ACE_INVALID_HANDLE)
    {
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("(%t) Scheduling reinitiation of connection\n")));

      // Reschedule ourselves to try and connect again.
      ACE_Time_Value const timeout (this->timeout ());
      if (ACE_Reactor::instance ()->schedule_timer (this, 0,
                                                    timeout) == -1)
        ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT ("(%t) %p\n"),
                           ACE_TEXT ("schedule_timer")), -1);
    }
  return 0;
}

// Receive a time update from a server
int
ACE_TS_Clerk_Handler::handle_input (ACE_HANDLE)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::handle_input"));
  // We're getting a time update message from a server
  ACE_Time_Request reply;
  if (this->recv_reply (reply) != 0)
    return -1;
  else
    {
      // Get current local time
      time_t local_time = ACE_OS::time (0);

      // Compure delta time (difference between current local time and
      // system time obtained from the server)
      time_t t = reply.time () - local_time;

      // Compute round trip delay and adjust time accordingly
      time_t one_way_time = (local_time - this->start_time_)/2;
      t += one_way_time;

      // Now update time info (to be retrieved by Clerk_Processor)
      this->time_info_.delta_time_ = t;
      this->time_info_.sequence_num_ = this->cur_sequence_num_;
    }
  return 0;
}

// Restart connection asynchronously when timeout occurs.
int
ACE_TS_Clerk_Handler::handle_timeout (const ACE_Time_Value &,
                                      const void *)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::handle_timeout"));
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("(%t) attempting to reconnect to server with timeout = %d\n"),
              this->timeout_));

  // Close down peer to reclaim descriptor if need be. Note this is
  // necessary to reconnect.
  this->peer ().close ();

  return this->processor_->initiate_connection (this, ACE_Synch_Options::asynch);
}

void
ACE_TS_Clerk_Handler::remote_addr (ACE_INET_Addr &addr)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::remote_addr"));
  this->remote_addr_ = addr;
}

ACE_INET_Addr &
ACE_TS_Clerk_Handler::remote_addr (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::remote_addr"));
  return this->remote_addr_;
}

int
ACE_TS_Clerk_Handler::recv_reply (ACE_Time_Request &reply)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::recv_reply"));
  const int bytes_expected = reply.size ();

  // Since Time_Request messages are fixed size, read the entire
  // message in one go.
  ssize_t n = this->peer ().recv ((void *) &reply, bytes_expected);

  if (n != bytes_expected)
    {
      switch (n)
        {
        case -1:
          // FALLTHROUGH
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("****************** recv_reply returned -1\n")));
        default:
          ACE_ERROR ((LM_ERROR, ACE_TEXT ("%p got %d bytes, expected %d bytes\n"),
                      ACE_TEXT ("recv failed"), n, bytes_expected));
          // FALLTHROUGH
        case 0:
          // We've shutdown unexpectedly
          return -1;
          // NOTREACHED
        }
    }
  else if (reply.decode () == -1)       // Decode the request into host byte order.
    ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT ("%p\n"),
                       ACE_TEXT ("decode failed")), -1);
  return 0;
}


int
ACE_TS_Clerk_Handler::send_request (ACE_UINT32 sequence_num, ACE_Time_Info &time_info)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Handler::send_request"));
  void    *buffer;
  ssize_t length;

  // Update current sequence number
  this->cur_sequence_num_ = sequence_num;

  // First update the current time info.
  time_info.delta_time_ = this->time_info_.delta_time_;
  time_info.sequence_num_ = this->time_info_.sequence_num_;

  // Now prepare a new time update request
  ACE_Time_Request request (ACE_Time_Request::TIME_UPDATE, 0, 0);

  if ((length = request.encode (buffer)) == -1)
    ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT ("%p\n"),
                       ACE_TEXT ("encode failed")), -1);

  // Compute start time of sending request (needed to compute
  // roundtrip delay)
  this->start_time_ = ACE_OS::time (0);

  // Send the request
  if (this->peer ().send_n (buffer, length) != length)
    ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT ("%p\n"),
                       ACE_TEXT ("send_n failed")),
                      -1);

  return 0;
}

ACE_TS_Clerk_Processor::ACE_TS_Clerk_Processor ()
: timeout_ (ACE_DEFAULT_TIMEOUT),
  blocking_semantics_ (0),
  cur_sequence_num_ (0)
{
#if defined (ACE_DEFAULT_BACKING_STORE)
  // Create a temporary file.
  ACE_OS::strcpy (this->poolname_,
                  ACE_DEFAULT_BACKING_STORE);
#else /* ACE_DEFAULT_BACKING_STORE */
  if (ACE::get_temp_dir (this->poolname_,
                         MAXPATHLEN - 17) == -1) // -17 for ace-malloc-XXXXXX
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("Temporary path too long, ")
                  ACE_TEXT ("defaulting to current directory\n")));
      this->poolname_[0] = 0;
    }

  // Add the filename to the end
  ACE_OS::strcat (this->poolname_, ACE_TEXT ("ace-malloc-XXXXXX"));

#endif /* ACE_DEFAULT_BACKING_STORE */
}

void
ACE_TS_Clerk_Processor::alloc (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Processor::alloc"));
  ACE_NEW (this->shmem_, ALLOCATOR (this->poolname_));

  void *temp = 0;

  // Only create the state if it doesn't already exist.
  if (this->shmem_->find (ACE_DEFAULT_TIME_SERVER_STR, temp) ==  -1)
    {
      // Allocate the space out of shared memory for the system time entry
      temp = (this->shmem_->malloc (2 * sizeof (time_t)));

      // Give it a name binding
      this->shmem_->bind (ACE_DEFAULT_TIME_SERVER_STR, temp);
    }

  // Set up pointers. Note that we add one to get to the second
  // field in the structure
  time_t *time_p = (time_t *)temp;
  this->system_time_.delta_time_ = time_p;
  this->system_time_.last_local_time_ = time_p + 1;

  // Initialize
  *(this->system_time_.delta_time_) = 0;
  *(this->system_time_.last_local_time_) = ACE_OS::time (0);
}

// Query the servers for the latest time
int
ACE_TS_Clerk_Processor::handle_timeout (const ACE_Time_Value &,
                                        const void *)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Processor::handle_timeout"));
  return this->update_time ();
}

int
ACE_TS_Clerk_Processor::update_time ()
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Processor::update_time"));
  ACE_UINT32 expected_sequence_num = this->cur_sequence_num_;

  // Increment sequence number
  this->cur_sequence_num_++;

  int count = 0;
  time_t total_delta = 0;
  ACE_Time_Info time_info;

  //  Call send_request() on all handlers
  ACE_TS_Clerk_Handler **handler = 0;

  for (HANDLER_SET_ITERATOR set_iterator (this->handler_set_);
       set_iterator.next (handler) != 0;
       set_iterator.advance ())
    {
      if ((*handler)->state () == ACE_TS_Clerk_Handler::ESTABLISHED)
        {
          if ((*handler)->send_request (this->cur_sequence_num_, time_info) == -1)
            return -1;
          // Check if sequence numbers match; otherwise discard
          else if (expected_sequence_num != 0 &&
                   time_info.sequence_num_ == expected_sequence_num)
            {
              count++;
              ACE_DEBUG ((LM_DEBUG,
                          ACE_TEXT ("[%d] Delta time: %d\n"),
                          count, time_info.delta_time_));

              // #### Can check here if delta value falls within a threshold ####
              total_delta += time_info.delta_time_;
            }
        }
    }
  // Update system_time_ using average of times obtained from all the servers.
  // Note that we are keeping two things in shared memory: the delta
  // time (difference between our system clock and the local clock),
  // and the last local time
  if (count > 0)
    {
      // At least one server is out there
      *(this->system_time_.delta_time_) = total_delta/count;
    }
  else
    {
      // No servers are out there (or this is the first time around
      // computing the time) so set delta time to zero. This
      // would mean that clients would use the actual local system time.
    *(this->system_time_.delta_time_) = 0;
  }
  // Update the last local time
  *(this->system_time_.last_local_time_) = ACE_OS::time (0);

  ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("Average delta time: %d\n"),
              (int)(*(this->system_time_.delta_time_))));
  return 0;
}


int
ACE_TS_Clerk_Processor::fini (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Processor::fini"));

  // Cancel the timer
  if (this->timer_id_ != -1)
    ACE_Reactor::instance ()->cancel_timer (this->timer_id_);

  // Destroy all the handlers
  ACE_TS_Clerk_Handler **handler = 0;

  for (HANDLER_SET_ITERATOR set_iterator (this->handler_set_);
       set_iterator.next (handler) != 0;
       set_iterator.advance ())
    {
      if ((*handler)->state () != ACE_TS_Clerk_Handler::IDLE)
        // Mark state as DISCONNECTING so we don't try to reconnect...
        (*handler)->state (ACE_TS_Clerk_Handler::DISCONNECTING);

      // Deallocate resources.
      (*handler)->destroy (); // Will trigger a delete
    }

  // Remove the backing store
  this->shmem_->remove ();

  ACE_Connector <ACE_TS_Clerk_Handler, ACE_SOCK_CONNECTOR>::fini ();

  return 0;
}

int
ACE_TS_Clerk_Processor::info (ACE_TCHAR **, size_t) const
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Processor::info"));
  return 0;
}

int
ACE_TS_Clerk_Processor::init (int argc, ACE_TCHAR *argv[])
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Processor::init"));
  // Use the options hook to parse the command line arguments and set
  // options.
  this->parse_args (argc, argv);

  this->alloc ();

#if !defined (ACE_WIN32)
  // Ignore SIPPIPE so each Output_Channel can handle it.
  ACE_Sig_Action sig ((ACE_SignalHandler) SIG_IGN, SIGPIPE);
  ACE_UNUSED_ARG (sig);
#endif /* ACE_WIN32 */

  ACE_Synch_Options &synch_options = this->blocking_semantics_ == 0
    ? ACE_Synch_Options::asynch : ACE_Synch_Options::synch;

  // Now set up connections to all servers
  ACE_TS_Clerk_Handler **handler = 0;

  for (HANDLER_SET_ITERATOR set_iterator (this->handler_set_);
       set_iterator.next (handler) != 0;
       set_iterator.advance ())
    {
      this->initiate_connection (*handler, synch_options);
    }
  // Now set up timer to receive updates from server
  // set the timer to go off after timeout value
  this->timer_id_ = ACE_Reactor::instance ()->schedule_timer (this,
                                                              0,
                                                              ACE_Time_Value (this->timeout_),
                                                              ACE_Time_Value (this->timeout_));
  return 0;
}

int
ACE_TS_Clerk_Processor::initiate_connection (ACE_TS_Clerk_Handler *handler,
                                             ACE_Synch_Options &synch_options)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Processor::initiate_connection"));
  ACE_TCHAR buf[MAXHOSTNAMELEN + 1];

  // Mark ourselves as idle so that the various iterators will ignore
  // us until we are connected/reconnected.
  handler->state (ACE_TS_Clerk_Handler::IDLE);

  if (handler->remote_addr ().addr_to_string (buf, MAXHOSTNAMELEN) == -1)
    ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT ("(%t) %p\n"),
                      ACE_TEXT ("can't obtain peer's address")), -1);

  // Establish connection with the server.
  if (this->connect (handler,
                     handler->remote_addr (),
                     synch_options) == -1)
    {
      if (errno != EWOULDBLOCK)
        {
          handler->state (ACE_TS_Clerk_Handler::FAILED);
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%t) %p on address %s\n"),
                      ACE_TEXT ("connect"), buf));

          // Reschedule ourselves to try and connect again.
          if (synch_options[ACE_Synch_Options::USE_REACTOR])
            {
              ACE_Time_Value const handler_timeout (handler->timeout ());
              if (ACE_Reactor::instance ()->schedule_timer (handler,
                                                            0,
                                                            handler_timeout) == -1)
                ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT ("(%t) %p\n"),
                                   ACE_TEXT ("schedule_timer")), -1);
            }
          else
            // Failures on synchronous connects are reported as errors
            // so that the caller can decide how to proceed.
            return -1;
        }
      else
        {
          handler->state (ACE_TS_Clerk_Handler::CONNECTING);
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT ("(%t) in the process of connecting %s to %s\n"),
                      synch_options[ACE_Synch_Options::USE_REACTOR] ?
                       ACE_TEXT ("asynchronously") : ACE_TEXT ("synchronously"),
                      buf));
        }
    }
  else
    {
      handler->state (ACE_TS_Clerk_Handler::ESTABLISHED);
      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%t) connected to %s on %d\n"),
                  buf, handler->get_handle ()));
    }
  return 0;
}

int
ACE_TS_Clerk_Processor::parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Processor::parse_args"));
  ACE_INET_Addr server_addr;
  ACE_TS_Clerk_Handler *handler;

  ACE_Get_Opt get_opt (argc, argv, ACE_TEXT ("h:t:p:b"), 0);

  for (int c; (c = get_opt ()) != -1; )
    {
      switch (c)
        {
        case 'h':
          // Get the hostname:port and create an ADDR
          server_addr.set (get_opt.opt_arg ());

          // Create a new handler
          ACE_NEW_RETURN (handler,
                          ACE_TS_Clerk_Handler (this, server_addr),
                          -1);

          // Cache the handler
          this->handler_set_.insert (handler);
          break;
        case 't':
          // Get the timeout value
          this->timeout_ = ACE_OS::atoi (get_opt.opt_arg ());
          break;
        case 'p':
          // Get the poolname
          ACE_OS::strncpy (this->poolname_,
                           get_opt.opt_arg (),
                           sizeof this->poolname_ / sizeof (ACE_TCHAR));
          break;
        case 'b':
          // Blocking semantics
          this->blocking_semantics_ = 1;
          break;
        default:
          ACE_ERROR_RETURN ((LM_ERROR,
                             ACE_TEXT ("%n:\n[-h hostname:port] [-t timeout] [-p poolname]\n")),
                            -1);
        }
    }
  return 0;
}

int
ACE_TS_Clerk_Processor::suspend (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Processor::suspend"));
  return 0;
}

int
ACE_TS_Clerk_Processor::resume (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Clerk_Processor::resume"));
  return 0;
}

// The following is a "Factory" used by the ACE_Service_Config and
// svc.conf file to dynamically initialize the state of the TS_Clerk.

ACE_SVC_FACTORY_DEFINE (ACE_TS_Clerk_Processor)

