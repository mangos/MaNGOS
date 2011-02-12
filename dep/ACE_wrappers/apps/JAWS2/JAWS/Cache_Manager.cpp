// $Id: Cache_Manager.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "ace/ACE.h"
#include "ace/OS_NS_string.h"

#include "JAWS/Cache_Manager.h"
#include "JAWS/Cache_List_T.h"

JAWS_String_Hash_Functor::JAWS_String_Hash_Functor (const char *s)
  : i_ (0)
{
  this->i_ = ACE::hash_pjw (s);
}

JAWS_String_Hash_Functor::operator unsigned long (void) const
{
  return this->i_;
}

JAWS_String_Equal_Functor::JAWS_String_Equal_Functor (const char *s1,
                                                    const char *s2)
  : i_ (0)
{
  this->i_ = ACE_OS::strcmp (s1, s2);
}

JAWS_String_Equal_Functor::operator int (void) const
{
  return this->i_ == 0;
}

JAWS_Strdup_String::JAWS_Strdup_String (void)
  : c_ (0),
    s_ (0)
{
}

JAWS_Strdup_String::JAWS_Strdup_String (const char *s)
  : c_ (0),
    s_ (0)
{
  this->c_ = new int (1);
  this->s_ = ACE_OS::strdup (s);
}

JAWS_Strdup_String::JAWS_Strdup_String (const JAWS_Strdup_String &s)
  : c_ (s.c_),
    s_ (s.s_)
{
  ++*(this->c_);
}

JAWS_Strdup_String::~JAWS_Strdup_String (void)
{
  if (this->c_ && --*(this->c_) == 0)
    {
      if (this->s_)
        ACE_OS::free (this->s_);
      delete this->c_;
    }
  this->s_ = 0;
  this->c_ = 0;
}

JAWS_Strdup_String::operator const char * (void) const
{
  return this->s_;
}

void
JAWS_Strdup_String::operator = (const char *s)
{
  if (this->c_ && --*(this->c_) == 0)
    {
      if (this->s_)
        ACE_OS::free (this->s_);
      delete this->c_;
    }
  this->c_ = new int (1);
  this->s_ = ACE_OS::strdup (s);
}

void
JAWS_Strdup_String::operator = (const JAWS_Strdup_String &s)
{
  if (this == &s)
    return;

  if (this->c_ && --*(this->c_) == 0)
    {
      if (this->s_)
        ACE_OS::free (this->s_);
      delete this->c_;
    }
  this->c_ = s.c_;
  this->s_ = s.s_;
  ++*(this->c_);
}

