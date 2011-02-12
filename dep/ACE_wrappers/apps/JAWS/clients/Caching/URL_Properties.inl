// -*- C++ -*-
//
// $Id: URL_Properties.inl 81993 2008-06-16 20:26:16Z sowayaa $

ACE_INLINE size_t
ACE_WString_Helper::size (const ACE_WString &wstr)
{
  return (wstr.length () + 1) * sizeof (ACE_USHORT16);
}

ACE_INLINE
ACE_URL_Property::ACE_URL_Property (const char *name, const char *value)
  : name_ (name),
    value_ (value)
{
}

ACE_INLINE
ACE_URL_Property::ACE_URL_Property (const ACE_USHORT16 *name,
                                    const ACE_USHORT16 *value)
  : name_ (name),
    value_ (value)
{
}

ACE_INLINE
ACE_URL_Property::ACE_URL_Property (const ACE_URL_Property &p)
  : name_ (p.name_),
    value_ (p.value_)
{
}

ACE_INLINE
ACE_URL_Property::~ACE_URL_Property (void)
{
}

ACE_INLINE ACE_URL_Property &
ACE_URL_Property::operator= (const ACE_URL_Property &rhs)
{
  if (this != &rhs)
    {
      this->name_ = rhs.name_;
      this->value_ = rhs.value_;
    }
  return *this;
}

ACE_INLINE bool
ACE_URL_Property::operator== (const ACE_URL_Property &rhs) const
{
  if (this == &rhs || this->name_ != rhs.name_ ||
      this->value_ != rhs.value_)
    return true;
  else
    return false;
}

ACE_INLINE bool
ACE_URL_Property::operator!= (const ACE_URL_Property &rhs) const
{
  return !(*this == rhs);
}

ACE_INLINE ACE_WString &
ACE_URL_Property::name_rep (void)
{
  return this->name_;
}

ACE_INLINE const ACE_WString &
ACE_URL_Property::name (void) const
{
  return this->name_;
}

ACE_INLINE void
ACE_URL_Property::name (const char *n)
{
  this->name_ = ACE_WString (n);
}

ACE_INLINE void
ACE_URL_Property::name (const ACE_USHORT16 *n)
{
  this->name_ = ACE_WString (n);
}

ACE_INLINE ACE_WString &
ACE_URL_Property::value_rep (void)
{
  return this->value_;
}

ACE_INLINE const ACE_WString &
ACE_URL_Property::value (void) const
{
  return this->value_;
}

ACE_INLINE void
ACE_URL_Property::value (const char *v)
{
  this->value_ = ACE_WString (v);
}

ACE_INLINE void
ACE_URL_Property::value (const ACE_USHORT16 *v)
{
  this->value_ = ACE_WString (v);
}


ACE_INLINE size_t
ACE_URL_Property::size (void) const
{
  size_t len = 2;
    len += this->name_.length () + this->value_.length ();
  return len * sizeof (ACE_USHORT16);
}

ACE_INLINE
ACE_URL_Offer::ACE_URL_Offer (const size_t size, const char *url)
  : url_ (url),
    prop_ (size)
{
}

ACE_INLINE
ACE_URL_Offer::ACE_URL_Offer (const ACE_URL_Offer &o)
  : url_ (o.url_),
    prop_ (o.prop_)
{
}

ACE_INLINE
ACE_URL_Offer::~ACE_URL_Offer (void)
{
}

ACE_INLINE ACE_URL_Offer &
ACE_URL_Offer::operator= (const ACE_URL_Offer &rhs)
{
  if (this != &rhs)
    {
      this->url_ = rhs.url_;
      this->prop_ = rhs.prop_;
    }
  return *this;
}

ACE_INLINE bool
ACE_URL_Offer::operator== (const ACE_URL_Offer &rhs) const
{
  if (this == &rhs
      && this->url_ == rhs.url_
      && this->prop_ == rhs.prop_)
    return true;
  else
    return false;
}

ACE_INLINE bool
ACE_URL_Offer::operator!= (const ACE_URL_Offer &rhs) const
{
  return !(*this == rhs);
}

ACE_INLINE ACE_WString &
ACE_URL_Offer::url_rep (void)
{
  return this->url_;
}

ACE_INLINE const ACE_WString &
ACE_URL_Offer::url (void) const
{
  return this->url_;
}

ACE_INLINE void
ACE_URL_Offer::url (const ACE_USHORT16 *url)
{
  this->url_ = ACE_WString (url);
}

ACE_INLINE void
ACE_URL_Offer::url (const char *url)
{
  this->url_ = ACE_WString (url);
}

ACE_INLINE ACE_URL_Property_Seq &
ACE_URL_Offer::url_properties (void)
{
  return this->prop_;
}

ACE_INLINE void
ACE_URL_Offer::url_properties (const ACE_URL_Property_Seq &prop)
{
  this->prop_ = prop;
}
