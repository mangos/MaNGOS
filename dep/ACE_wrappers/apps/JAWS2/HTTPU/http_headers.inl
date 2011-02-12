// -*- c++ -*-
// $Id: http_headers.inl 80826 2008-03-04 14:51:23Z wotte $

#if !defined (ACE_HAS_INLINED_OSCALLS)
# undef ACE_INLINE
# define ACE_INLINE
#endif /* ACE_HAS_INLINED_OSCALLS */

ACE_INLINE
HTTP_Hdr_Node::operator int (void) const
{
  return this->index_;
}

ACE_INLINE
HTTP_Hdr_Node::operator const char * (void) const
{
  return this->token_;
}

ACE_INLINE const char *
HTTP_Hdr_Node::format (void) const
{
  return this->format_;
}

ACE_INLINE const HTTP_Hdr_Node &
HTTP_HCodes::hcode (int type) const
{
  const HTTP_Hdr_Node **hn = this->header_nodes_->find (type);

  // No error checking!
  return **hn;
}

ACE_INLINE const char *
HTTP_Headers::header_token (int name) const
{
  const HTTP_Hdr_Node **hn = this->header_nodes_->find (name);
  return ((hn && *hn) ? (const char *)**hn : 0);
}

ACE_INLINE const char *
HTTP_Headers::header_strings (int name) const
{
  const HTTP_Hdr_Node **hn = this->header_nodes_->find (name);
  return ((hn && *hn) ? (*hn)->format () : 0);
}
