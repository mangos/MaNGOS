/* -*- C++ -*- */
// $Id: HT_Client.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    HT_Client.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _HT_CLIENT_H
#define _HT_CLIENT_H

#include "Hash_Table.h"

class HT_Client : public Hash_Table
{
  // = TITLE
  //   Provides the client's hash table abstraction.
public:
  virtual Protocol_Record *insert (const char *key_name,
                                   int max_len = MAXUSERIDNAMELEN);
};

#endif /* _HT_CLIENT_H */
