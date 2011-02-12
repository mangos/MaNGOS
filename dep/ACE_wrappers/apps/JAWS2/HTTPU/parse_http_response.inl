// -*- c++ -*-
// $Id: parse_http_response.inl 80826 2008-03-04 14:51:23Z wotte $

#if !defined (ACE_HAS_INLINED_OSCALLS)
# undef ACE_INLINE
# define ACE_INLINE
#endif /* ACE_HAS_INLINED_OSCALLS */

ACE_INLINE int
Parse_HTTP_Response::code (void) const
{
  return this->code_;
}

ACE_INLINE const char *
Parse_HTTP_Response::code_str (void) const
{
  return this->code_str_ ? this->code_str_ : "200";
}

ACE_INLINE int
Parse_HTTP_Response::major_version (void) const
{
  return this->major_version_;
}

ACE_INLINE int
Parse_HTTP_Response::minor_version (void) const
{
  return this->minor_version_;
}

ACE_INLINE const char *
Parse_HTTP_Response::version (void) const
{
  return this->version_ ? this->version_ : "HTTP/0.9";
}

ACE_INLINE int
Parse_HTTP_Response::error (void) const
{
  return this->error_;
}
