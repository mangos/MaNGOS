// -*- c++ -*-
// $Id: parse_http_request.inl 80826 2008-03-04 14:51:23Z wotte $

#if !defined (ACE_HAS_INLINED_OSCALLS)
# undef ACE_INLINE
# define ACE_INLINE
#endif /* ACE_HAS_INLINED_OSCALLS */

ACE_INLINE int
Parse_HTTP_Request::method (void) const
{
  return (int) *this->method_;
}

ACE_INLINE const char *
Parse_HTTP_Request::method_str (void) const
{
  return (const char *) *this->method_;
}

ACE_INLINE int
Parse_HTTP_Request::major_version (void) const
{
  return this->major_version_;
}

ACE_INLINE int
Parse_HTTP_Request::minor_version (void) const
{
  return this->minor_version_;
}

ACE_INLINE const char *
Parse_HTTP_Request::version (void) const
{
  return this->version_ ? this->version_ : "HTTP/0.9";
}

ACE_INLINE const char *
Parse_HTTP_Request::url (void) const
{
  return this->url_;
}

ACE_INLINE int
Parse_HTTP_Request::error (void) const
{
  return this->error_;
}
