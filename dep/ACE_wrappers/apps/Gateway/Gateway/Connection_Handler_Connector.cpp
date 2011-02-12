// $Id: Connection_Handler_Connector.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#include "Connection_Handler_Connector.h"
#include "ace/os_include/os_netdb.h"

Connection_Handler_Connector::Connection_Handler_Connector (void)
{
}

// Initiate (or reinitiate) a connection to the Connection_Handler.

int
Connection_Handler_Connector::initiate_connection (Connection_Handler *connection_handler,
                                                   ACE_Synch_Options &synch_options)
{
  ACE_TCHAR addr_buf[MAXHOSTNAMELEN];

  // Mark ourselves as idle so that the various iterators will ignore
  // us until we are reconnected.
  connection_handler->state (Connection_Handler::IDLE);

  // We check the remote addr second so that it remains in the
  // addr_buf.
  if (connection_handler->local_addr ().addr_to_string (addr_buf, sizeof addr_buf) == -1
      || connection_handler->remote_addr ().addr_to_string (addr_buf, sizeof addr_buf) == -1)
    ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT ("(%t) %p\n"),
                      ACE_TEXT ("can't obtain peer's address")), -1);

  // Try to connect to the Peer.
  if (this->connect (connection_handler,
                     connection_handler->remote_addr (),
                     synch_options,
                     connection_handler->local_addr ()) == -1)
    {
      if (errno != EWOULDBLOCK)
        {
          connection_handler->state (Connection_Handler::FAILED);
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%t) %p on address %s\n"),
                      ACE_TEXT ("connect"), addr_buf));
          return -1;
        }
      else
        {
          connection_handler->state (Connection_Handler::CONNECTING);
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT ("(%t) in the process of connecting to %s\n"),
                      addr_buf));
        }
    }
  else
    {
      connection_handler->state (Connection_Handler::ESTABLISHED);
      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%t) connected to %s on %d\n"),
                  addr_buf, connection_handler->get_handle ()));
    }
  return 0;
}

