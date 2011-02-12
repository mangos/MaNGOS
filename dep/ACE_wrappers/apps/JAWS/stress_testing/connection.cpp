// $Id: connection.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#include "connection.h"

// Make the connection to the WEB server

int connection::connect(char *hostname_opt_port, int tcp_nodelay, int sockbufsiz) {
  if(!hostname_opt_port) return 1;

  char *hostname_with_port;
  // Check to see if portnumber is specified in the hostnameport
  // If not, append :80
  if(!ACE_OS::strchr(hostname_opt_port,':')) {
    hostname_with_port = new char[ACE_OS::strlen(hostname_opt_port) + 3];
    ACE_OS::sprintf(hostname_with_port, "%s:%d", hostname_opt_port, 80);
  }
  else {
    hostname_with_port = hostname_opt_port;
  }

  // Beyond this point, hostname_with_port is of the form hostname:port

  ACE_INET_Addr server_addr(hostname_with_port);

  // Connect to server

  ACE_SOCK_Connector con;

  if(con.connect(stream_, server_addr) == -1) {
    ACE_OS::perror("ACE_SOCK_Connector::connect");
    return 1;
  }


  // tcp_nodelay processing.

  // turn off weird ack things
  if(tcp_nodelay) {
    struct protoent *p = ACE_OS::getprotobyname ("tcp");
    int one = 1;

    if (p && stream_.set_option (p->p_proto,
                                 TCP_NODELAY,
                                 (char *)& one,
                                 sizeof (one)))
    {
      ACE_OS::perror("tcp_nodelay");
      return 1;
    }
  }

  if(sockbufsiz)
    if (stream_.set_option (SOL_SOCKET,
                            SO_RCVBUF,
                            (char *) &sockbufsiz,
                            sizeof sockbufsiz) == -1)
    {
      ACE_OS::perror("socket_queue_size");
      return 1;
    }

  return 0;
}

int connection::read(void *buffer, size_t maxlen, unsigned int timeout_seconds) {
  ACE_UNUSED_ARG (timeout_seconds);
  return stream_.recv(buffer, maxlen);
}

int connection::write(const void *buffer, size_t maxlen, unsigned int timeout_seconds) {
  ACE_UNUSED_ARG (timeout_seconds);
  return stream_.send(buffer, maxlen);
}

int connection::write_n(const void *buffer, size_t len, unsigned int timeout_seconds) {
  ACE_UNUSED_ARG (timeout_seconds);
  if(stream_.send_n(buffer, len) == -1)
    ACE_ERROR_RETURN((LM_ERROR, "Write failed for %s", buffer),1);
  return 0;
}

int connection::read_n(void *buffer, size_t maxlen, unsigned int timeout_seconds) {
  ACE_UNUSED_ARG (timeout_seconds);
  if(stream_.recv_n(buffer, maxlen) == -1)
    ACE_ERROR_RETURN((LM_ERROR, "Read failed.."),1);
  return 1;
}

int connection::close(void) {
  stream_.close_reader();
  stream_.close_writer();
  stream_.close();
  return 0;
}

connection::~connection(void) {
  this->close();
}
