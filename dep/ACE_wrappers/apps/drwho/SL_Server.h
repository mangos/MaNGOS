/* -*- C++ -*- */
// $Id: SL_Server.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    SL_Server.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _SL_SERVER_H
#define _SL_SERVER_H

#include "Single_Lookup.h"

class SL_Server : public Single_Lookup
{
  // = TITLE
  //   Provides the server's single user lookup table abstraction.

public:
  SL_Server (const char *packet);
  virtual Protocol_Record *insert (const char *key_name,
                                   int max_len = MAXUSERIDNAMELEN);
  virtual Protocol_Record *get_each_entry (void);
};

#endif /* _SL_SERVER_H */
