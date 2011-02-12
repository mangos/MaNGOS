// $Id: parse_http_response.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef HTTPU_PARSE_HTTP_RESPONSE_H
#define HTTPU_PARSE_HTTP_RESPONSE_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "HTTPU/http_export.h"

class HTTPU_Export Parse_HTTP_Response
{
public:
  Parse_HTTP_Response (const char *response = 0);
  ~Parse_HTTP_Response (void);

  void init (const char *response);

  int code (void) const;
  const char *code_str (void) const;

  int major_version (void) const;
  int minor_version (void) const;

  const char *version (void) const;

  enum { HTTPU_OK, NO_MEMORY, BAD_RESPONSE };

  int error (void) const;
  // 0 -> ok

private:

  int code_;
  char *code_str_;
  int major_version_;
  int minor_version_;
  char *version_;
  char *response_;
  int error_;

};

#if defined (ACE_HAS_INLINED_OSCALLS)
#   if defined (ACE_INLINE)
#     undef ACE_INLINE
#   endif /* ACE_INLINE */
#   define ACE_INLINE inline
#   include "HTTPU/parse_http_response.inl"
# endif /* ACE_HAS_INLINED_OSCALLS */


#endif /* !defined (HTTPU_PARSE_HTTP_RESPONSE_H) */
