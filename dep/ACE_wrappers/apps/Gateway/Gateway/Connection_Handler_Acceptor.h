/* -*- C++ -*- */
// $Id: Connection_Handler_Acceptor.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    gateway
//
// = FILENAME
//    Connection_Handler_acceptor.h
//
// = AUTHOR
//    Doug Schmidt
//
// ============================================================================

#ifndef _CONNECTION_HANDLER_ACCEPTOR
#define _CONNECTION_HANDLER_ACCEPTOR

#include "ace/Acceptor.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/SOCK_Acceptor.h"
#include "Connection_Handler.h"

// Forward declaration
class Event_Channel;

class Connection_Handler_Acceptor : public ACE_Acceptor<Connection_Handler, ACE_SOCK_ACCEPTOR>
{
  // = TITLE
  //     A concrete factory class that setups connections to peerds
  //     and produces a new Connection_Handler object to do the dirty
  //     work...
public:
  Connection_Handler_Acceptor (Event_Channel &,
                               char connection_role);
  // Constructor.

  virtual int make_svc_handler (Connection_Handler *&ch);
  // Hook method for creating an appropriate <Connection_Handler>.

  virtual int accept_svc_handler (Connection_Handler *ch);
  // Hook method for accepting a connection into the
  // <Connection_Handler>.

protected:
  typedef ACE_Acceptor<Connection_Handler, ACE_SOCK_ACCEPTOR>
          inherited;
  // Make life easier later on.

  Event_Channel &event_channel_;
  // Reference to the event channel.

  Connection_Config_Info connection_config_info_;
  // Keeps track of what type of proxy we need to create.

  Connection_Handler_Factory connection_handler_factory_;
  // Make the appropriate type of <Connection_Handler>.
};

#endif /* _CONNECTION_HANDLER_ACCEPTOR */
