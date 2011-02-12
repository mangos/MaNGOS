/* -*- c++ -*- */

//=============================================================================
/**
 *  @file    HTTP_Request.h
 *
 *  $Id: HTTP_Request.h 80826 2008-03-04 14:51:23Z wotte $
 *
 *  @author James Hu
 */
//=============================================================================


#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Parse_Headers.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

/**
 * @class HTTP_Request
 *
 * @brief This parses the client request of an HTTP transaction.
 *
 */
class HTTP_Request
{
public:
  /// Default construction.
  HTTP_Request (void);

  /// Destructor.
  ~HTTP_Request (void);

  /// parse an incoming request
  int parse_request (ACE_Message_Block &mb);

  /// the first line of a request is the request line, which is of the
  /// form: METHOD URI VERSION.
  void parse_request_line (char *const request_line);

  /// Initialize the request object.  This will parse the buffer and
  /// prepare for the accessors.
  int init (char *const buffer,
            int buflen);

public:
  // = The Accessors.

  /// HTTP request method
  const char *method (void) const;

  /// HTTP request uri
  const char *uri (void) const;

  /// HTTP request version
  const char *version (void) const;

  /// The HTTP request uri translated into a server filename path
  const char *path (void) const;

  /// TRUE of the request is a cgi request
  int cgi (void) const;

  /// The arguments to the cgi request
  const char *cgi_args (void) const;

  /// The environment variables passed to the CGI request
  const char **cgi_env (void) const;

  /// The cgi request query string
  const char *query_string (void) const;

  /// The cgi request path information
  const char *path_info (void) const;

  /// The type of the HTTP request
  int type (void) const;

  /// The headers that were parsed from the request
  const Headers &headers (void) const;

  /// Header strings stored
  const char *header_strings (int index) const;

  /// Values associated with the header strings
  const char *header_values (int index) const;

  /// The buffer into which request data is read
  char *data (void);

  /// The length of the request data
  int data_length (void);

  /// The length of incoming content if any
  int content_length (void);

  /// Current status of the incoming request
  int status (void);

  /// A string describing the state of the incoming request
  const char *status_string (void);

  /// Dump the state of the request.
  void dump (void);

  enum
  {
    NO_TYPE = -1,
    GET = 0,
    HEAD,
    POST,
    PUT,
    NUM_METHOD_STRINGS
  };
  // Values for request type

  enum
  {
    DATE = 0,
    PRAGMA,
    AUTHORIZATION,
    FROM,
    IF_MODIFIED_SINCE,
    REFERRER,
    USER_AGENT,
    ALLOW,
    CONTENT_ENCODING,
    CONTENT_LENGTH,
    CONTENT_TYPE,
    EXPIRES,
    LAST_MODIFIED,
    NUM_HEADER_STRINGS
  };
  // Header strings

private:
  // = Private Accessors which can set values
  const char *method (const char *method_string);
  const char *uri (char *uri_string);
  const char *version (const char *version_string);
  const char *path (const char *uri_string);

  /// determine if the given URI is a CGI program.
  int cgi (char *uri_string);

  /// determine if the given URI resides in a cgi-bin directory
  int cgi_in_path (char *uri_string, char *&extra_path_info);

  /// determine if the given URI contains a cgi extension
  int cgi_in_extension (char *uri_string, char *&extra_path_info);

  /// set the arguments and environment for the cgi program
  void cgi_args_and_env (char *&extra_path_info);

  int type (const char *type_string);

private:
  int got_request_line (void) const;

private:
  int got_request_line_;
  Headers headers_;

  char *method_;
  char *uri_;
  char *version_;
  char *path_;

  int cgi_;
  char **cgi_env_;
  char *cgi_args_;

  char *query_string_;
  char *path_info_;

  const char * const *const header_strings_;
  static const char *const static_header_strings_[NUM_HEADER_STRINGS];

  const char * const *const method_strings_;
  static const char *const static_method_strings_[NUM_METHOD_STRINGS];

  char *data_;
  int datalen_;
  int content_length_;
  char *filename_;
  int status_;
  int type_;
};

#endif /* HTTP_REQUEST_H */
