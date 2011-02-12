// $Id: CM_Client.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "Options.h"
#include "Multicast_Manager.h"
#include "CM_Client.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_sys_socket.h"
#include "ace/OS_NS_sys_select.h"
#include "ace/OS_NS_netdb.h"
#include "ace/OS_NS_arpa_inet.h"
#include "ace/os_include/os_string.h"

// Creates and binds a UDP socket...

int
CM_Client::open (short port_number)
{
  Comm_Manager::sokfd_ = ACE_OS::socket (PF_INET, SOCK_DGRAM, 0);

  if (Comm_Manager::sokfd_ == ACE_INVALID_HANDLE)
    return -1;

  ACE_OS::memset ((char *) &this->sin_,
                  0,
                  sizeof this->sin_);
  this->sin_.sin_family = AF_INET;
  this->sin_.sin_port = htons (port_number);

  return 1;
}

int
CM_Client::receive (int timeout)
{
  FD_ZERO (&this->read_fd_);
  FD_SET (Comm_Manager::sokfd_, &this->read_fd_);

  if (timeout > 0)
    {
      this->time_out_.sec (timeout);
      this->time_out_.usec (0);
      this->top_ = &time_out_;
    }

  while (Multicast_Manager::outstanding_hosts_remain ())
    {
      if (ACE_OS::select (Comm_Manager::sokfd_ + 1,
                          &this->read_fd_,
                          0,
                          0,
                          this->top_) <= 0)
        break;
      else
        {
          int sin_len = sizeof this->sin_;
          int n = ACE_OS::recvfrom ((int)Comm_Manager::sokfd_,
                                    this->recv_packet_,
                                    UDP_PACKET_SIZE,
                                    0,
                                    reinterpret_cast<sockaddr *> (&this->sin_),
                                    &sin_len);
          if (n < 0)
            return -1;
          else
            {
              if (Options::get_opt (Options::DEBUGGING) != 0)
                {
                  hostent *np = ACE_OS::gethostbyaddr ((char *) &this->sin_.sin_addr,
                                                       sizeof this->sin_.sin_addr,
                                                       AF_INET);

                  ACE_DEBUG ((LM_DEBUG,
                              "receiving from server host %s (%s)\n",
                              np->h_name,
                              ACE_OS::inet_ntoa (this->sin_.sin_addr)));
                }

              if (this->demux (this->recv_packet_, n) < 0)
                return -1;

              Multicast_Manager::checkoff_host (this->sin_.sin_addr);
            }
        }
    }

  for (const char *host_name;
       Multicast_Manager::get_next_non_responding_host (host_name);
       )
    ACE_DEBUG ((LM_DEBUG,
                "%s did not respond\n",
                host_name));
  return 1;
}

int
CM_Client::send (void)
{
  int packet_length = 0;

  if (this->mux (this->send_packet_, packet_length) < 0)
    return -1;

  // Ship off the info to all the hosts.

  while (Multicast_Manager::get_next_host_addr (this->sin_.sin_addr) != 0)
    {
      if (Options::get_opt (Options::DEBUGGING) != 0)
        {
          hostent *np = ACE_OS::gethostbyaddr ((char *) &this->sin_.sin_addr,
                                               sizeof this->sin_.sin_addr,
                                               AF_INET);

          ACE_DEBUG ((LM_DEBUG,
                      "sending to server host %s (%s)\n",
                      np->h_name,
                      ACE_OS::inet_ntoa (this->sin_.sin_addr)));
        }

      if (ACE_OS::sendto (Comm_Manager::sokfd_,
                         this->send_packet_,
                         packet_length,
                         0,
                         reinterpret_cast<sockaddr *> (&this->sin_),
                         sizeof this->sin_) < 0)
        return -1;
    }
  return 1;
}

CM_Client::CM_Client (void)
  : top_ (0)
{
}

CM_Client::~CM_Client (void)
{
  if (Options::get_opt (Options::DEBUGGING))
    ACE_DEBUG ((LM_DEBUG,
                "disposing CM_Client\n"));

  ACE_OS::closesocket ((int)Comm_Manager::sokfd_);
}
