/* -*- c++ -*- */
// $Id: Cache_Manager.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_CACHE_MANAGER_H
#define JAWS_CACHE_MANAGER_H

#include "ace/ACE.h"

#include "JAWS/Cache_Manager_T.h"

class JAWS_String_Hash_Functor
{
public:
  JAWS_String_Hash_Functor (const char *s);
  operator unsigned long (void) const;

private:
  unsigned long i_;
};

class JAWS_String_Equal_Functor
{
public:
  JAWS_String_Equal_Functor (const char *s1, const char *s2);
  operator int (void) const;

private:
  int i_;
};

class JAWS_Strdup_String
{
public:

  JAWS_Strdup_String (void);
  JAWS_Strdup_String (const char *s);
  JAWS_Strdup_String (const JAWS_Strdup_String &s);
  ~JAWS_Strdup_String (void);

  operator const char * (void) const;
  void operator = (const char *s);
  void operator = (const JAWS_Strdup_String &s);

private:

  int *c_;
  char *s_;

};

typedef JAWS_Cache_Manager<JAWS_Strdup_String,
                          JAWS_Referenced_Cache_Object_Factory,
                          JAWS_String_Hash_Functor,
                          JAWS_String_Equal_Functor>
        JAWS_String_Referenced_Cache_Manager;

typedef JAWS_Cache_Manager<JAWS_Strdup_String,
                          JAWS_Counted_Cache_Object_Factory,
                          JAWS_String_Hash_Functor,
                          JAWS_String_Equal_Functor>
        JAWS_String_Counted_Cache_Manager;


#endif /* JAWS_CACHE_MANAGER_H */
