/* -*- C++ -*- */
// $Id: BS_Client.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    BS_Client.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _BS_CLIENT_H
#define _BS_CLIENT_H

#include "Binary_Search.h"

class BS_Client : public Binary_Search
{
  // = TITLE
  //    Provides the client's binary search lookup table abstraction.
public:
  // = Initialization.
  BS_Client (void);
  // Constructor.

  virtual Protocol_Record *insert (const char *key_name,
                                   int max_len = MAXUSERIDNAMELEN);
  // This function is used to merge the <key_name> from server
  // <host_name> into the sorted list of userids kept on the client's
  // side.  Since we *know* we are going to find the name we use the
  // traditional binary search.

  virtual Protocol_Record *get_each_entry (void);
  // An iterator, similar to Binary_Search::get_next_friend, though in
  // this case the friend records are returned in the order they
  // appeared in the friend file, rather than in sorted order.  Also,
  // we skip over entries that don't have any hosts associated with
  // them.
};

#endif /* _BS_CLIENT_H */
