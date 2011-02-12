/* -*- C++ -*- */
// $Id: global.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    global.h
//
// = DESCRIPTION
//    Here are all the declarations that are needed throughout the program. */
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _GLOBAL_H
#define _GLOBAL_H

#include "ace/config-all.h"
#include "ace/Basic_Types.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// These constants are used throughout drwho.

enum
{
  MAXUSERIDNAMELEN = 8,
  MAX_USER_TIMEOUT = 300,
  MAX_HOST_TIMEOUT = 300,
  UDP_PACKET_SIZE = 1024 * 8,
  PORT_NUMBER = 12344
};

// Default name of file where friends info is stored.
#define FRIEND_FILE ".friends.dta"

// Default name where rwho info is stored.
#define RWHODIR "/usr/spool/rwho"

// Macros for handling message types.
#define GET_PACKET_TYPE(P)      (ACE_NTOHS (*((short *) P)))
#define SET_PACKET_TYPE(P,T)    ((*(short *) P) = ACE_NTOHS (T))
#define SKIP_PACKET_TYPE(P)     ((P) + sizeof (short))
#define SUBTRACT_PACKET_TYPE(L) ((L) - sizeof (short))

#endif /* _GLOBAL_H */
