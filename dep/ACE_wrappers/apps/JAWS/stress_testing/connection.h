// $Id: connection.h 80826 2008-03-04 14:51:23Z wotte $

#include "global.h"

#ifndef _D_connection
#define _D_connection
class connection {

public:
  int connect(char *hostname_opt_port, int tcp_nodelay, int sockbufsiz);
  int read(void *buffer, size_t maxlen, unsigned int timeout_seconds = 60);
  int write(const void *buffer, size_t maxlen, unsigned int timeout_seconds = 60);
  int write_n(const void *buffer, size_t len, unsigned int timeout_seconds = 60);
  int read_n(void *buffer, size_t maxlen, unsigned int timeout_seconds = 60);
  int close(void);
  ~connection(void);

private:
  ACE_SOCK_Stream stream_;
  char sockbuf[66000];
};
#endif
