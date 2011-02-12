/* -*- c++ -*- */

//=============================================================================
/**
 *  @file    HTTP_Response.h
 *
 *  $Id: HTTP_Response.h 80826 2008-03-04 14:51:23Z wotte $
 *
 *  @author James Hu
 */
//=============================================================================


#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

class JAWS_IO;
class HTTP_Request;

/**
 * @class HTTP_Response
 *
 * @brief Abstraction for HTTP responses.
 *
 * Provides an encapsulation of responses to HTTP requests.
 * For instance, given an HTTP GET request, it will produce
 * header and body suitable for returning to the client who made
 * the request.
 */
class HTTP_Response
{
public:
  HTTP_Response (JAWS_IO &io,
                 HTTP_Request &request);
  HTTP_Response (HTTP_Request &request, JAWS_IO &io);
  ~HTTP_Response (void);

  /// This is called by the handler to initiate a response.
  void process_request (void);

  /// This returns an error response for cases where there is a problem
  /// with the request, logging the log_message.
  void error_response (int status,
                       const char *log_message);

private:

  /// Called by process_request when the request is a normal request.
  void normal_response (void);

  /// Called by process_request when the request is a cgi request.
  void cgi_response (void);

private:

  /// static version of process_request, just in case.
  static void process_request (HTTP_Response &response);

  /// creates the appropriate header information for responses.
  void build_headers (void);

private:
  /// The IO and Request objects associated with this re
  JAWS_IO &io_;
  HTTP_Request &request_;

#if defined (ACE_JAWS_BASELINE)
  char *HTTP_HEADER;
#else
  const char *HTTP_HEADER;
#endif
  /// HTTP Headers and trailers.
  const char *HTTP_TRAILER;
  int HTTP_HEADER_LENGTH;
  int HTTP_TRAILER_LENGTH;
};

#endif /* HTTP_RESPONSE_H */
