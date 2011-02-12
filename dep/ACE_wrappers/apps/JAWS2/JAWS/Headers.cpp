/* $Id: Headers.cpp 80826 2008-03-04 14:51:23Z wotte $ */

#include "JAWS/Headers.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_strings.h"
#include "ace/OS_NS_string.h"

// Header Data

JAWS_Header_Data::JAWS_Header_Data (const char *name, const char *value,
                                    int type)
  : header_name_ (name ? ACE_OS::strdup (name) : 0),
    header_value_ (value ? ACE_OS::strdup (value) : 0),
    header_type_ (type)
{
}

JAWS_Header_Data::JAWS_Header_Data (const char *name, int type,
                                    const char *value)
  : header_name_ (name ? ACE_OS::strdup (name) : 0),
    header_value_ (value ? ACE_OS::strdup (value) : 0),
    header_type_ (type)
{
}

JAWS_Header_Data::~JAWS_Header_Data (void)
{
  if ( this->header_name_ )
      ACE_OS::free ((void *)this->header_name_);
  if ( this->header_value_ )
  ACE_OS::free ((void *)this->header_value_);
  this->header_name_ = 0;
  this->header_value_ = 0;
}

const char *
JAWS_Header_Data::header_name (void) const
{
  return this->header_name_;
}

const char *
JAWS_Header_Data::header_value (void) const
{
  return this->header_value_;
}

int
JAWS_Header_Data::header_type (void) const
{
  return this->header_type_;
}

void
JAWS_Header_Data::header_name (const char *name)
{
  if (this->header_name_)
    ACE_OS::free ((void *)this->header_name_);
  this->header_name_ = name ? ACE_OS::strdup (name) : 0;
}

void
JAWS_Header_Data::header_value (const char *value)
{
  if (this->header_value_)
    ACE_OS::free ((void *)this->header_value_);
  this->header_value_ = value ? ACE_OS::strdup (value) : 0;
}

void
JAWS_Header_Data::header_type (int type)
{
  this->header_type_ = type;
}


// Header Table

JAWS_Headers::JAWS_Headers (void)
  : iter_ (*this)
{
}

JAWS_Headers::~JAWS_Headers (void)
{
}

JAWS_Header_Table_Iterator &
JAWS_Headers::iter (void)
{
  return this->iter_;
}

int
JAWS_Headers::insert (JAWS_Header_Data *new_data)
{
  // Since there may be duplicate header entries, we don't worry about
  // doing this find anymore.  Make the application developer figure
  // out how to interpret duplicate entries.

  return (JAWS_Header_Table::insert_tail (new_data) ? 0 : -1);
}

JAWS_Header_Data *
JAWS_Headers::find (const char *const &header_name)
{
  this->iter_.first ();
  return this->find_next (header_name);
}

JAWS_Header_Data *
JAWS_Headers::find_next (const char *const &header_name)
{
  JAWS_Header_Data *data = 0;
  JAWS_Header_Table_Iterator &i = this->iter_;

  while (! i.done ())
    {
      data = i.next ();
      if (data != 0)
        {
          if (ACE_OS::strcasecmp (data->header_name (), header_name) != 0)
            data = 0;
        }
      i.advance ();
      if (data != 0)
        break;
    }

  return data;
}

void
JAWS_Headers::remove_all (const char *const &header_name)
{
  JAWS_Header_Data *data;
  int done;

  do
    {
      JAWS_Header_Table_Iterator i (*this);
      i.first ();
      done = 1;
      while (! i.done ())
        {
          data = i.next ();
          if (data != 0
              && ACE_OS::strcasecmp (data->header_name (), header_name) == 0)
            {
              i.remove ();
              delete data;
              done = 0;
              break;
            }
          else
            i.advance ();
        }
    }
  while (! done);
}

