// -*- C++ -*-
//
// $Id: Local_Locator.inl 80826 2008-03-04 14:51:23Z wotte $

ACE_INLINE
ACE_URL_Record::ACE_URL_Record (void)
  : id_ (0),
    offer_ (0)
{
}

ACE_INLINE
ACE_URL_Record::ACE_URL_Record (ACE_URL_Offer *offer)
  : offer_ (offer)
{
  char buf[ACE_OFFER_ID_LENGTH];

  ACE_NEW (this->id_, ACE_WString (ACE_ID_Generator::get_new_id (buf)));
}

ACE_INLINE
ACE_URL_Record::~ACE_URL_Record (void)
{
  delete this->id_;
  delete this->offer_;
}

ACE_INLINE bool
ACE_URL_Record::operator== (const ACE_URL_Record &rhs) const
{
  return this == &rhs || *this->id_ == *rhs.id_;
}

ACE_INLINE bool
ACE_URL_Record::operator!= (const ACE_URL_Record &rhs) const
{
  return !(*this == rhs);
}

ACE_INLINE
ACE_URL_Local_Locator::~ACE_URL_Local_Locator (void)
{
}
