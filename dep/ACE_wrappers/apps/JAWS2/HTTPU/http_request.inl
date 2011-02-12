// -*- c++ -*-
// $Id: http_request.inl 80826 2008-03-04 14:51:23Z wotte $

#if !defined (ACE_HAS_INLINED_OSCALLS)
# undef ACE_INLINE
# define ACE_INLINE
#endif /* ACE_HAS_INLINED_OSCALLS */

ACE_INLINE
HTTP_Request::HTTP_Request (void)
{
}

ACE_INLINE
HTTP_Request::~HTTP_Request (void)
{
}

ACE_INLINE Parse_HTTP_Request *
HTTP_Request::request_line (void)
{
  return &(this->request_);
}

ACE_INLINE const Parse_HTTP_Request *
HTTP_Request::request_line (void) const
{
  return &(this->request_);
}

ACE_INLINE HTTP_Parse_URL *
HTTP_Request::url (void)
{
  return &(this->url_);
}

ACE_INLINE void
HTTP_Request::set_status (int s)
{
  this->status_ = s;
}
