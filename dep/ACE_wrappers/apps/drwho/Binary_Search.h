/* -*- C++ -*- */
// $Id: Binary_Search.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    Binary_Search.h
//
// = DESCRIPTION
//    Defines a binary search abstraction for friend records.
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _BINARY_SEARCH_H
#define _BINARY_SEARCH_H

#include "Search_Struct.h"

class Binary_Search : public Search_Struct
{
  // = TITLE
  //    Defines a binary search abstraction for friend records.
public:
  // = Initialization and termination method.
  Binary_Search (void);
  // Initialize the values for the iterators...

  virtual ~Binary_Search (void);
  // Destructor.

  virtual Protocol_Record *get_next_entry (void);
  // Returns the next friend in the sequence of sorted friends.  Note
  // that this function would be simplified if we expanded the
  // iterator interface to include an "initialize" and "next"
  // function!

  virtual Protocol_Record *get_each_entry (void);
  // An iterator, similar to Binary_Search::get_next_friend, though in
  // this case the friend records are returned in the order they
  // appeared in the friend file, rather than in sorted order.  Also,
  // we skip over entries that don't have any hosts associated with
  // them.

  virtual Protocol_Record *insert (const char *key_name,
                                   int max_len = MAXUSERIDNAMELEN) = 0;
  // This function is used to merge the <key_name> from server
  // <host_name> into the sorted list of userids kept on the client's
  // side.

  static int name_compare (const void *, const void *);
  // This function is passed to qsort to perform the comparison
  // between login names for two friends.

protected:
  Protocol_Record **current_ptr_;
  int current_index_;

  Protocol_Record *protocol_record_;
  Protocol_Record **sorted_record_;

  const char *buffer_;
  int buffer_size_;
};

#endif /* _BINARY_SEARCH_H */
