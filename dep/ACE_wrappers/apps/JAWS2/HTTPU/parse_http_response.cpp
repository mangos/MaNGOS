// $Id: parse_http_response.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "HTTPU/parse_http_response.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_stdlib.h"

Parse_HTTP_Response::Parse_HTTP_Response (const char *response)
  : code_ (200),
    code_str_ (0),
    major_version_ (0),
    minor_version_ (9),
    version_ (0),
    response_ (0),
    error_ (0)
{
  if (response != 0)
    this->init (response);
}

Parse_HTTP_Response::~Parse_HTTP_Response (void)
{
  if (this->response_)
    ACE_OS::free (this->response_);
  this->response_ = 0;
  this->code_str_ = 0;
  this->version_ = 0;
}

void
Parse_HTTP_Response::init (const char *response)
{
  this->response_ = ACE_OS::strdup (response);
  if (this->response_ == 0)
    {
      this->error_ = NO_MEMORY;
      return;
    }

  int n = ::sscanf (this->response_, "HTTP/%d.%d %d %*s",
                    &(this->major_version_),
                    &(this->minor_version_),
                    &(this->code_));

  if (n == 3)
    {
      char *p = this->response_;

      while (*p == ' ' || *p == '\t')
        p++;

      this->version_ = p++;

      while (*p != ' ' && *p != '\t')
        p++;

      *p++ = '\0';

      while (*p == ' ' || *p == '\t')
        p++;

      this->code_str_ = p;

      while (*p && !ACE_OS::strchr (" \t\r\n", *p))
        p++;

      *p++ = '\0';
    }
  else
    this->error_ = BAD_RESPONSE;
}

#if !defined (ACE_HAS_INLINED_OSCALLS)
# include "HTTPU/parse_http_response.inl"
#endif /* ACE_HAS_INLINED_OSCALLS */
