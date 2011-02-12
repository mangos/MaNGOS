// $Id: CM_Server.cpp 85282 2009-05-05 13:26:00Z olli $

#include "global.h"
#include "Options.h"
#include "CM_Server.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_sys_socket.h"
#include "ace/OS_NS_arpa_inet.h"

// Creates and binds a UDP socket...

int
CM_Server::open (short port_number)
{
  int max_packet_size = UDP_PACKET_SIZE;

  this->sokfd_ = ACE_OS::socket (PF_INET, SOCK_DGRAM, 0);

  if (this->sokfd_ < 0)
    return -1;

  ACE_OS::memset (&this->sin_, 0, sizeof this->sin_);
  this->sin_.sin_family = AF_INET;
  this->sin_.sin_port = htons (port_number);
  this->sin_.sin_addr.s_addr = INADDR_ANY;

  // This call fails if an rflo daemon is already running.
  if (ACE_OS::bind (this->sokfd_,
                    reinterpret_cast<sockaddr *> (&this->sin_),
                    sizeof this->sin_) < 0)
    return -1;

  if (ACE_OS::setsockopt (this->sokfd_,
                          SOL_SOCKET,
                          SO_SNDBUF,
                          (char *) &max_packet_size,
                          sizeof max_packet_size) < 0)
    return -1;

  return 1;
}

int
CM_Server::receive (int)
{
  int sin_len = sizeof this->sin_;

  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG, "waiting for client to send...\n"));

  int n = ACE_OS::recvfrom (this->sokfd_,
                            this->recv_packet_,
                            UDP_PACKET_SIZE,
                            0,
                            reinterpret_cast<sockaddr *> (&this->sin_),
                            (int *) &sin_len);
  if (n == -1)
    return -1;

  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "receiving from client host %s\n",
                ACE_OS::inet_ntoa (this->sin_.sin_addr)));

  if (this->demux (this->recv_packet_, n) < 0)
    return -1;

  return 1;
}

int
CM_Server::send (void)
{
  int  packet_length = 0;

  if (this->mux (this->send_packet_,
                 packet_length) < 0)
    return -1;

  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "sending to client host %s\n",
                ACE_OS::inet_ntoa (this->sin_.sin_addr)));

  if (ACE_OS::sendto (this->sokfd_,
                      this->send_packet_,
                      packet_length,
                      0,
                      reinterpret_cast<sockaddr *> (&this->sin_),
                      sizeof this->sin_) < 0)
    return -1;

  return 1;
}

CM_Server::CM_Server (void)
{
}

CM_Server::~CM_Server (void)
{
  if (Options::get_opt (Options::DEBUGGING))
    ACE_DEBUG ((LM_DEBUG,
                "CM_Server\n"));

  ACE_OS::closesocket (this->sokfd_);
}
