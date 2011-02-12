/* -*- C++ -*- */
// $Id: Concrete_Connection_Handlers.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    gateway
//
// = FILENAME
//    Concrete_Connection_Handlers.h
//
// = DESCRIPTION
//    These are all the subclasses of Connection_Handler that define the
//    appropriate threaded/reactive Consumer/Supplier behavior.
//
// = AUTHOR
//    Doug Schmidt <schmidt@cs.wustl.edu>
//
// ============================================================================

#ifndef CONCRETE_CONNECTION_HANDLER
#define CONCRETE_CONNECTION_HANDLER

#include "Connection_Handler.h"

class Supplier_Handler : public Connection_Handler
{
  // = TITLE
  //     Handles reception of Events from Suppliers.
  //
  // = DESCRIPTION
  //     Performs framing and error checking on Events.  Intended to
  //     run reactively, i.e., in one thread of control using a
  //     Reactor for demuxing and dispatching.
public:
  // = Initialization method.
  Supplier_Handler (const Connection_Config_Info &);

protected:
  // = All the following methods are upcalls, so they can be protected.

  virtual int handle_input (ACE_HANDLE = ACE_INVALID_HANDLE);
  // Receive and process peer events.

  virtual int recv (ACE_Message_Block *&);
  // Receive an event from a Supplier.

  int process (ACE_Message_Block *event);
  // This delegates to the <Event_Channel> to do the actual
  // processing.  Typically, it forwards the <event> to its
  // appropriate Consumer.

  ACE_Message_Block *msg_frag_;
  // Keep track of event fragment to handle non-blocking recv's from
  // Suppliers.
};

class Consumer_Handler : public Connection_Handler
{
  // = TITLE
  //     Handles transmission of events to Consumers.
  //
  // = DESCRIPTION
  //     Performs queueing and error checking.  Intended to run
  //     reactively, i.e., in one thread of control using a Reactor
  //     for demuxing and dispatching.  Also uses a Reactor to handle
  //     flow controlled output connections.
public:
  // = Initialization method.
  Consumer_Handler (const Connection_Config_Info &);

  virtual int put (ACE_Message_Block *event,
                   ACE_Time_Value * = 0);
  // Send an event to a Consumer (may be queued if necessary).

protected:
  virtual int handle_output (ACE_HANDLE);
  // Finish sending event when flow control conditions abate.

  int nonblk_put (ACE_Message_Block *mb);
  // Perform a non-blocking put().

  virtual ssize_t send (ACE_Message_Block *);
  // Send an event to a Consumer.

  virtual int handle_input (ACE_HANDLE);
  // Receive and process shutdowns from a Consumer.
};

class Thr_Consumer_Handler : public Consumer_Handler
{
  // = TITLE
  //    Runs each <Consumer_Handler> in a separate thread.
public:
  Thr_Consumer_Handler (const Connection_Config_Info &);

  virtual int open (void *);
  // Initialize the threaded Consumer_Handler object and spawn a new
  // thread.

  virtual int put (ACE_Message_Block *, ACE_Time_Value * = 0);
  // Send a message to a peer.

protected:
  virtual int handle_input (ACE_HANDLE);
  // Called when Peer shutdown unexpectedly.

  virtual int svc (void);
  // Transmit peer messages.

  virtual int handle_close (ACE_HANDLE = ACE_INVALID_HANDLE,
                            ACE_Reactor_Mask = ACE_Event_Handler::ALL_EVENTS_MASK);
  // When thread started, connection become blocked, so no need to use
  // handle_close to reinitiate the connection_handler, so should
  // override this function to justify if controlling is in thread or
  // not. If yes, handle_close do nothing, otherwise, it call parent
  // handle_close().

private:
  int in_thread_;
  // If the controlling is in thread's svc() or not.
};

class Thr_Supplier_Handler : public Supplier_Handler
{
  // = TITLE
  //    Runs each <Supplier_Handler> in a separate thread.
public:
  Thr_Supplier_Handler (const Connection_Config_Info &pci);

  virtual int open (void *);
  // Initialize the object and spawn a new thread.

protected:
  virtual int handle_close (ACE_HANDLE = ACE_INVALID_HANDLE,
                            ACE_Reactor_Mask = ACE_Event_Handler::ALL_EVENTS_MASK);
  // When thread started, connection become blocked, so no need to use
  // handle_close to reinitiate the connection_handler, so should
  // override this function to justify if controlling is in thread or
  // not. If yes, handle_close do nothing, otherwise, it call parent
  // handle_close().

  virtual int svc (void);
  // Transmit peer messages.

private:
  int in_thread_;
  // If the controlling is in thread's svc() or not.
};

#endif /* CONCRETE_CONNECTION_HANDLER */
