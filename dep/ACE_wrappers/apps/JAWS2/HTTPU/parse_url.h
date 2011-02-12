/* -*- c++ -*- */
// $Id: parse_url.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef HTTPU_HTTP_PARSE_H
#define HTTPU_HTTP_PARSE_H

#include "HTTPU/http_export.h"

class HTTPU_Export HTTP_Parse_URL
{
  // CAVEAT:

  // The user of the class is responsible for testing the difference
  // between a missing username versus an empty one.  Same goes for
  // password The RFC (1738) makes the differentiation for username
  // and password.  If the hostname is missing (or empty), this class
  // always returns a null value for the host.

public:
  HTTP_Parse_URL (const char *url = 0);
  ~HTTP_Parse_URL (void);

  void init (const char *url);

  enum URL_SCHEME { HTTP, FTP };

  const char *scheme (void) const;
  const char *user (void) const;
  const char *passwd (void) const;
  const char *host (void) const;
  int port (void) const;
  const char *url_path (void) const;

  enum URL_ERROR { URL_ERROR_NONE, URL_ERROR_STRDUP, URL_ERROR_SCHEME, URL_ERROR_SLASHSLASH };

  int error (void) const { return( error_ ); }

  int is_cgi (void) const;

private:
  void parse_url (void);
  void parse_scheme (char *&p);
  void parse_host (char *&p);
  void parse_url_path (char *&p);
  void is_cgi (const char *path);

  void set_port_from_scheme (void);

private:
  char *url_;

  const char *scheme_;
  const char *user_;
  const char *passwd_;
  const char *host_;
  int port_;
  const char *url_path_;

  int error_;
  int is_cgi_;
};

#endif /* !defined (HTTPU_HTTP_PARSE_H) */
