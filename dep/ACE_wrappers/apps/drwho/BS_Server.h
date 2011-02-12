/* -*- C++ -*- */
// $Id: BS_Server.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    BS_Server.h
//
// = DESCRIPTION
//    Provides the server's binary search lookup table abstraction.
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _BS_SERVER_H
#define _BS_SERVER_H

#include "Binary_Search.h"

class BS_Server : public Binary_Search
{
  // = TITLE
  //    Provides the server's binary search lookup table abstraction.
public:
  // = Initialization.
  BS_Server (const char *packet);

  virtual Protocol_Record *insert (const char *key_name,
                                   int max_len = MAXUSERIDNAMELEN);
  // This function is used to merge the <key_name> from server
  // <host_name> into the sorted list of userids kept on the client's
  // side.

  virtual Protocol_Record *get_next_entry (void);
  // An iterator, similar to Binary_Search::get_next_friend, though in
  // this case the friend records are returned in the order they
  // appeared in the friend file, rather than in sorted order.  Also,
  // we skip over entries that don't have any hosts associated with
  // them.
};

#endif /* _BS_SERVER_H */
