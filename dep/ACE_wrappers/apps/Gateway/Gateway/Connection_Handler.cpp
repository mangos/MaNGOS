// $Id: Connection_Handler.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#define ACE_BUILD_SVC_DLL

#include "ace/OS_NS_string.h"
#include "Event_Channel.h"
#include "Concrete_Connection_Handlers.h"

Event_Channel *
Connection_Handler::event_channel (void) const
{
  return this->event_channel_;
}

void
Connection_Handler::event_channel (Event_Channel *ec)
{
  this->event_channel_ = ec;
}

void
Connection_Handler::connection_id (CONNECTION_ID id)
{
  this->connection_id_ = id;
}

CONNECTION_ID
Connection_Handler::connection_id (void) const
{
  return this->connection_id_;
}

// The total number of bytes sent/received on this Proxy.

size_t
Connection_Handler::total_bytes (void) const
{
  return this->total_bytes_;
}

void
Connection_Handler::total_bytes (size_t bytes)
{
  this->total_bytes_ += bytes;
}

Connection_Handler::Connection_Handler (void)
{
}

Connection_Handler::Connection_Handler (const Connection_Config_Info &pci)
  : local_addr_ (pci.local_port_),
    connection_id_ (pci.connection_id_),
    total_bytes_ (0),
    state_ (Connection_Handler::IDLE),
    timeout_ (1),
    max_timeout_ (pci.max_retry_timeout_),
    event_channel_ (pci.event_channel_)
{
  if (ACE_OS::strlen (pci.host_) > 0)
    this->remote_addr_.set (pci.remote_port_, pci.host_);
  else
    this->remote_addr_.set (pci.remote_port_, ACE_DEFAULT_SERVER_HOST);
  // Set the priority of the Proxy.
  this->priority (int (pci.priority_));
}

// Set the connection_role.

void
Connection_Handler::connection_role (char d)
{
  this->connection_role_ = d;
}

// Get the connection_role.

char
Connection_Handler::connection_role (void) const
{
  return this->connection_role_;
}

// Sets the timeout delay.

void
Connection_Handler::timeout (long to)
{
  if (to > this->max_timeout_)
    to = this->max_timeout_;

  this->timeout_ = to;
}

// Re-calculate the current retry timeout delay using exponential
// backoff.  Returns the original timeout (i.e., before the
// re-calculation).

long
Connection_Handler::timeout (void)
{
  long old_timeout = this->timeout_;
  this->timeout_ *= 2;

  if (this->timeout_ > this->max_timeout_)
    this->timeout_ = this->max_timeout_;

  return old_timeout;
}

// Sets the max timeout delay.

void
Connection_Handler::max_timeout (long mto)
{
  this->max_timeout_ = mto;
}

// Gets the max timeout delay.

long
Connection_Handler::max_timeout (void) const
{
  return this->max_timeout_;
}

// Restart connection asynchronously when timeout occurs.

int
Connection_Handler::handle_timeout (const ACE_Time_Value &,
                                    const void *)
{
  ACE_DEBUG ((LM_DEBUG,
              "(%t) attempting to reconnect Connection_Handler %d with timeout = %d\n",
              this->connection_id (),
              this->timeout_));

  // Delegate the re-connection attempt to the Event Channel.
  this->event_channel_->initiate_connection_connection (this);

  return 0;
}

// Handle shutdown of the Connection_Handler object.

int
Connection_Handler::handle_close (ACE_HANDLE, ACE_Reactor_Mask)
{
  ACE_DEBUG ((LM_DEBUG,
              "(%t) shutting down %s Connection_Handler %d on handle %d\n",
              this->connection_role () == 'C' ? "Consumer" : "Supplier",
              this->connection_id (),
              this->get_handle ()));

  // Restart the connection, if possible.
  return this->event_channel_->reinitiate_connection_connection (this);
}

// Set the state of the Proxy.

void
Connection_Handler::state (Connection_Handler::State s)
{
  this->state_ = s;
}

// Return the current state of the Proxy.

Connection_Handler::State
Connection_Handler::state (void) const
{
  return this->state_;
}

// Upcall from the <ACE_Acceptor> or <ACE_Connector> that delegates
// control to our Connection_Handler.

int
Connection_Handler::open (void *)
{
  ACE_DEBUG ((LM_DEBUG, "(%t) %s Connection_Handler's handle = %d\n",
              this->connection_role () == 'C' ? "Consumer" : "Supplier",
              this->peer ().get_handle ()));

  // Call back to the <Event_Channel> to complete our initialization.
  if (this->event_channel_->complete_connection_connection (this) == -1)
    ACE_ERROR_RETURN ((LM_ERROR, "(%t) %p\n", "complete_connection_connection"), -1);

  // Turn on non-blocking I/O.
  else if (this->peer ().enable (ACE_NONBLOCK) == -1)
    ACE_ERROR_RETURN ((LM_ERROR, "(%t) %p\n", "enable"), -1);

  // Register ourselves to receive input events.
  else if (ACE_Reactor::instance ()->register_handler
      (this, ACE_Event_Handler::READ_MASK) == -1)
    ACE_ERROR_RETURN ((LM_ERROR, "(%t) %p\n", "register_handler"), -1);
  else
    return 0;
}

const ACE_INET_Addr &
Connection_Handler::remote_addr (void) const
{
  return this->remote_addr_;
}

void
Connection_Handler::remote_addr (ACE_INET_Addr &ra)
{
  this->remote_addr_ = ra;
}

const ACE_INET_Addr &
Connection_Handler::local_addr (void) const
{
  return this->local_addr_;
}

void
Connection_Handler::local_addr (ACE_INET_Addr &la)
{
  this->local_addr_ = la;
}

// Make the appropriate type of <Connection_Handler> (i.e.,
// <Consumer_Handler>, <Supplier_Handler>, <Thr_Consumer_Handler>, or
// <Thr_Supplier_Handler>).

Connection_Handler *
Connection_Handler_Factory::make_connection_handler (const Connection_Config_Info &pci)
{
  Connection_Handler *connection_handler = 0;

  // The next few lines of code are dependent on whether we are making
  // a threaded/reactive Supplier_Handler/Consumer_Handler.

  if (pci.connection_role_ == 'C') // Configure a Consumer_Handler.
    {
      // Create a threaded Consumer_Handler.
      if (ACE_BIT_ENABLED (Options::instance ()->threading_strategy (),
                           Options::OUTPUT_MT))
        ACE_NEW_RETURN (connection_handler,
                        Thr_Consumer_Handler (pci),
                        0);

      // Create a reactive Consumer_Handler.
      else
        ACE_NEW_RETURN (connection_handler,
                        Consumer_Handler (pci),
                        0);
    }
  else // connection_role == 'S', so configure a Supplier_Handler.
    {
      // Create a threaded Supplier_Handler.
      if (ACE_BIT_ENABLED (Options::instance ()->threading_strategy (),
                           Options::INPUT_MT))
        ACE_NEW_RETURN (connection_handler,
                        Thr_Supplier_Handler (pci),
                        0);

      // Create a reactive Supplier_Handler.
      else
        ACE_NEW_RETURN (connection_handler,
                        Supplier_Handler (pci),
                        0);
    }

  return connection_handler;
}
