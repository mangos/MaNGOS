// $Id: URL_Array_Helper.cpp 91670 2010-09-08 18:02:26Z johnnyw $

// URL_Array_Helper.cpp

#ifndef ACE_URL_ARRAY_HELPER_C
#define ACE_URL_ARRAY_HELPER_C

#include "URL_Array_Helper.h"

// Some helper functions for encoding/decoding

template <class T>
size_t ace_array_size (const T &x)
{
  size_t sum = sizeof (ACE_UINT32);
  for (size_t i = 0; i < x.size (); i++)
    sum += x[i].size ();
  return sum;
}

template <class T>
size_t ace_array_encode (void *buf, const T &x)
{
  size_t len = 0;
  for (size_t i = 0; i < x.size (); i++)
      len+= x[i].encode ((void *) ((char *) buf + len));
  return len ;
}

template <class T>
size_t ace_array_decode (void *buf, T &x)
{
  size_t len = 0;
  for (size_t i = 0; i < x.size (); i++)
      len += x[i].decode ((void *) ((char *) buf + len));
  return len;
}



#endif /* ACE_URL_ARRAY_HELPER_C */
