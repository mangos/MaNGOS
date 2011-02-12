/* -*- C++ -*- */
// $Id: Search_Struct.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    drwho
//
// = FILENAME
//    Search_Struct.h
//
// = AUTHOR
//    Douglas C. Schmidt
//
// ============================================================================

#ifndef _SEARCH_STRUCT_H
#define _SEARCH_STRUCT_H

#include "Protocol_Record.h"

class Search_Struct
{
  // = TITLE
  //   Provides an "Abstract Base Class" lookup table abstraction that
  //   stores and manipulates friend records.
public:
  Search_Struct (void);
  virtual ~Search_Struct (void);
  virtual int n_elems (void);

  virtual Protocol_Record *insert (const char *key_name,
                                   int max_len = MAXUSERIDNAMELEN) = 0;
  virtual Protocol_Record *get_next_entry (void) = 0;
  virtual Protocol_Record *get_each_entry (void) = 0;

protected:
  int count_;
};

#endif /* _SEARCH_STRUCT_H */
