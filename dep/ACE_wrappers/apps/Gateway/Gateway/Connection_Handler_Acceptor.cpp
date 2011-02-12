// $Id: Connection_Handler_Acceptor.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#define ACE_BUILD_SVC_DLL

#include "Event_Channel.h"
#include "Connection_Handler_Acceptor.h"

int
Connection_Handler_Acceptor::make_svc_handler (Connection_Handler *&ch)
{
  ACE_ALLOCATOR_RETURN (ch,
                        this->connection_handler_factory_.make_connection_handler (this->connection_config_info_),
                        -1);
  return 0;
}

int
Connection_Handler_Acceptor::accept_svc_handler (Connection_Handler *ch)
{
  if (this->inherited::accept_svc_handler (ch) == -1)
    return -1;
  else
    {
      ch->connection_id (Options::instance ()->connection_id ());
      ACE_INET_Addr remote_addr;

      if (ch->peer ().get_remote_addr (remote_addr) == -1)
        return -1;

      // Set the remote address of our connected Peer.
      ch->remote_addr (remote_addr);

      // Set the Event_Channel pointer.
      ch->event_channel (&this->event_channel_);

      // Increment the connection ID by one.
      Options::instance ()->connection_id ()++;
      return 0;
    }
}

Connection_Handler_Acceptor::Connection_Handler_Acceptor (Event_Channel &ec,
                                                          char connection_role)
  : event_channel_ (ec)
{
  this->connection_config_info_.connection_id_ = 0;
  this->connection_config_info_.host_[0] = '\0';
  this->connection_config_info_.remote_port_ = 0;
  this->connection_config_info_.connection_role_ = connection_role;
  this->connection_config_info_.max_retry_timeout_ = Options::instance ()->max_timeout ();
  this->connection_config_info_.local_port_ = 0;
  this->connection_config_info_.priority_ = 1;
}

