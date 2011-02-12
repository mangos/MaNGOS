// $Id: parse_http_request.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "HTTPU/parse_http_request.h"

Parse_HTTP_Request::Parse_HTTP_Request (const char *request)
  : method_ (0),
    major_version_ (-1),
    minor_version_ (-1),
    version_ (0),
    url_ (0),
    request_ (0),
    error_ (0)
{
  if (request != 0)
    this->init (request);
}

Parse_HTTP_Request::~Parse_HTTP_Request (void)
{
  if (this->request_)
    ACE_OS::free (this->request_);
  this->request_ = 0;
  this->version_ = 0;
  this->url_ = 0;
}

void
Parse_HTTP_Request::dump (void)
{
  ACE_DEBUG ((LM_DEBUG, "%s %s %s\n",
              this->method_str (), this->url (), this->version ()));
}

void
Parse_HTTP_Request::init (const char *request)
{
  char *method;

  this->request_ = ACE_OS::strdup (request);
  if (this->request_ == 0)
    {
      this->error_ = NO_MEMORY;
      return;
    }

  char buf[BUFSIZ];
  int n = ::sscanf (this->request_, "%s %*s HTTP/%d.%d",
                    buf,
                    &(this->major_version_),
                    &(this->minor_version_));

  if (n == 1 || n == 3)
    {
      char *p = this->request_;

      while (*p == ' ' || *p == '\t')
        p++;

      method = p++;

      while (*p != ' ' && *p != '\t')
        p++;

      *p++ = '\0';

      while (*p == ' ' || *p == '\t')
        p++;

      this->url_ = p;

      while (*p && !ACE_OS::strchr (" \t\r\n", *p))
        p++;

      *p++ = '\0';

      if (n == 1)
        {
          this->major_version_ = 0;
          this->minor_version_ = 9;
        }
      else
        {
          while (*p == ' ' || *p == '\t')
            p++;

          this->version_ = p;

          while (*p && !ACE_OS::strchr (" \t\r\n", *p))
            p++;

          *p++ = '\0';
        }

      if (ACE_OS::strcmp (method, "GET") == 0)
        this->method_ = &GET;
      else if (ACE_OS::strcmp (method, "HEAD") == 0)
        this->method_ = &HEAD;
      else if (ACE_OS::strcmp (method, "POST") == 0)
        this->method_ = &POST;
      else if (ACE_OS::strcmp (method, "PUT") == 0)
        this->method_ = &PUT;
      else if (ACE_OS::strcmp (method, "QUIT") == 0)
        this->method_ = &QUIT;
      else
        {
          this->method_ = &DUNNO;
          this->error_ = NOT_IMPLEMENTED;
        }
    }
  else
    this->error_ = BAD_REQUEST;
}

#if !defined (ACE_HAS_INLINED_OSCALLS)
# include "HTTPU/parse_http_request.inl"
#endif /* ACE_HAS_INLINED_OSCALLS */
