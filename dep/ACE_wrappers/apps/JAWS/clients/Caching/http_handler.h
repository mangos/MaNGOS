/* -*- c++ -*- */
// Hey, Emacs!  This is a C++ file.
// $Id: http_handler.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    apps/JAWS/clients/Caching
//
// = FILENAME
//    http_handler.h
//
// = AUTHOR
//    James Hu
//
// ============================================================================

#include "ace/SOCK_Connector.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Connector.h"
#include "ace/Svc_Handler.h"

class HTTP_Handler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
  // = TITLE
  //     A simple HTTP protocol handler for clients.
  //
  // = DESCRIPTION
  //     Checks to see if the requested file is already cached.  If
  //     so, it says so.  If not, the request is issued to the
  //     connection.  The fetched file is cached.
{
public:
  // = Initialization methods.
  HTTP_Handler (void);
  HTTP_Handler (const char * path);

  virtual int open (void *);
  // Open hook.

  virtual int svc (void);
  // Entry points defined by the abstract Svc_Handler.

  const char *filename (void) const;
  // Accessor to the file being fetched.

private:
  char request_[BUFSIZ];
  size_t request_size_;

  char filename_[BUFSIZ];
  size_t response_size_;
};

class HTTP_Connector
  // = TITLE
  //     A simple HTTP connector.
  //
  // = DESCRIPTION
  //     Creates an HTTP Handler based on the URL, and then delegates
  //     to to the SOCK_CONNECTOR.  Adapter pattern.
{
public:
  int connect (const char * url);
  // User entry point into the HTTP connector.

private:
  int parseurl (const char *url,
                char *host,
                u_short *port,
                char *path);
  // Helper function.

private:
  ACE_Connector<HTTP_Handler, ACE_SOCK_CONNECTOR> connector_;
  // Factory that actively establishes a connection with an HTTP
  // server.
};
