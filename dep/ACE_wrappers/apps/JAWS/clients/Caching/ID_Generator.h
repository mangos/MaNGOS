/* -*- C++ -*- */

// $Id: ID_Generator.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    none
//
// = FILENAME
//    ID_Generator.h
//
// = AUTHOR
//    Nanbor Wang
//
// ============================================================================

#ifndef ACE_ID_GENERATOR_H
#define ACE_ID_GENERATOR_h

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#define ACE_OFFER_ID_LENGTH 21

class ACE_ID_Generator
  // = TITLE
  //     An unique ID generator.
  //
  // = DESCRIPTION

  //     Generate an offer ID according to current time and avoid
  //     duplicate ID.  It guarantees ID uniqueness within a process,
  //     i.e. no two threads may get the same ID string.  Using a
  //     similar method like the backery algorithm.
{
public:
  static char *get_new_id (char *id);
  // allocate a new ID string and point <id> to it.

private:
  static void get_serial_id (time_t &t, size_t &s);
  // Atomically get info required to generate an offer ID.

  static ACE_SYNCH_MUTEX *get_lock (void);
  // Get the lock instance.

  static time_t last_time_;
  // Record the time last offer ID generated.

  static size_t last_number_;
  // Record serial number of last offer ID with same
  // generation time.

  static ACE_SYNCH_MUTEX *lock_;
  // mutex to access private member.
};

#endif /* ACE_ID_GENERATOR_H */
