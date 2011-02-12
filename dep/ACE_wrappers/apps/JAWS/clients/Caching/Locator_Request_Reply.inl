/* -*- C++ -*- */

// $Id: Locator_Request_Reply.inl 80826 2008-03-04 14:51:23Z wotte $

#include "URL_Locator.h"

ACE_INLINE
ACE_URL_Locator_Request::ACE_URL_Locator_Request (void)
  : code_(ACE_URL_Locator::INVALID_OPERATION),
    seq1_ (0),
    seq2_ (0),
    offer_ (0),
    buffer_ (0)
{
}

ACE_INLINE
ACE_URL_Locator_Request::~ACE_URL_Locator_Request (void)
{
  delete this->seq1_;
  delete this->seq2_;
  delete this->offer_;
  delete [] this->buffer_;
}

ACE_INLINE const  int
ACE_URL_Locator_Request::how (void) const
{
  return this-> how_;
}

ACE_INLINE const  int
ACE_URL_Locator_Request::how_many (void) const
{
  return this->how_many_;
}

ACE_INLINE const u_int
ACE_URL_Locator_Request::opcode (void) const
{
  return this->code_;
}

ACE_INLINE const  ACE_URL_Property_Seq *
ACE_URL_Locator_Request::seq (void) const
{
  return this->seq1_;
}

ACE_INLINE const  ACE_URL_Property_Seq *
ACE_URL_Locator_Request::del (void) const
{
  return this->seq1_;
}

ACE_INLINE const  ACE_URL_Property_Seq *
ACE_URL_Locator_Request::modify (void) const
{
  return this->seq2_;
}

ACE_INLINE const ACE_URL_Offer *
ACE_URL_Locator_Request::offer (void) const
{
  return this->offer_;
}

ACE_INLINE const  ACE_WString &
ACE_URL_Locator_Request::id (void) const
{
  return this->id_;
}

ACE_INLINE const  ACE_WString &
ACE_URL_Locator_Request::url (void) const
{
  return this->url_;
}

ACE_INLINE const  char *
ACE_URL_Locator_Request::buffer (void) const
{
  return this->buffer_;
}

ACE_INLINE
ACE_URL_Locator_Reply::ACE_URL_Locator_Reply (void)
  : code_ (ACE_URL_Locator::INVALID_OPERATION),
    offer_ (0),
    offers_ (0),
    buffer_ (0)
{
}

ACE_INLINE
ACE_URL_Locator_Reply::~ACE_URL_Locator_Reply (void)
{
  delete this->offer_;
  delete this->offers_;
  delete [] this->buffer_;
}

ACE_INLINE const size_t
ACE_URL_Locator_Reply::num_offers (void) const
{
  return this->num_offers_;
}


ACE_INLINE const ACE_URL_Offer *
ACE_URL_Locator_Reply::offer (void) const
{
  return this->offer_;
}

ACE_INLINE const ACE_URL_Offer_Seq *
ACE_URL_Locator_Reply::offers (void) const
{
  return this->offers_;
}

ACE_INLINE const u_int
ACE_URL_Locator_Reply::opcode (void) const
{
  return this->code_;
}

ACE_INLINE const u_int
ACE_URL_Locator_Reply::status (void) const
{
  return this->status_;
}

ACE_INLINE const char *
ACE_URL_Locator_Reply::buffer (void) const
{
  return this->buffer_;
}
