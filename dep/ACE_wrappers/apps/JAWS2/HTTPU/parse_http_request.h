// $Id: parse_http_request.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef HTTPU_PARSE_HTTP_REQUEST_H
#define HTTPU_PARSE_HTTP_REQUEST_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "HTTPU/http_headers.h"

class HTTPU_Export Parse_HTTP_Request : public HTTP_HCodes
{
public:
  Parse_HTTP_Request (const char *request = 0);
  ~Parse_HTTP_Request (void);

  void init (const char *request);

  int method (void) const;
  const char *method_str (void) const;

  int major_version (void) const;
  int minor_version (void) const;

  const char *version (void) const;

  const char *url (void) const;

  enum { HTTPU_OK, NO_MEMORY, BAD_REQUEST, NOT_IMPLEMENTED };

  int error (void) const;
  // 0 -> ok

  void dump (void);

private:

  HTTP_Hdr_Node *method_;
  int major_version_;
  int minor_version_;
  char *version_;
  char *url_;
  char *request_;
  int error_;

};

#if defined (ACE_HAS_INLINED_OSCALLS)
#   if defined (ACE_INLINE)
#     undef ACE_INLINE
#   endif /* ACE_INLINE */
#   define ACE_INLINE inline
#   include "HTTPU/parse_http_request.inl"
# endif /* ACE_HAS_INLINED_OSCALLS */

#endif /* !defined (HTTPU_PARSE_HTTP_REQUEST_H) */
