/* -*- C++ -*- */
// $Id: URL_Array_Helper.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    none
//
// = FILENAME
//    URL_Array_Helper.h
//
// = AUTHOR
//    Nanbor Wang
//
// ============================================================================

#ifndef ACE_URL_ARRAY_HELPER_H
#define ACE_URL_ARRAY_HELPER_H

// ### These template functions are probably named improperly.
// You should find some way to avoid name space polution.

template <class T>
size_t ace_array_size (const T &x);

template <class T>
size_t ace_array_encode (void *buf, const T &x);

template <class T>
size_t ace_array_decode (void *buf, T &x);

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "URL_Array_Helper.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("URL_Array_Helper.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#endif /* ACE_URL_ARRAY_HELPER_H */
