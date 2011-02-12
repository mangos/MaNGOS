/* -*- c++ -*- */
// $Id: Symbol_Table.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_SYMBOL_TABLE_H
#define JAWS_SYMBOL_TABLE_H

#include "ace/Hash_Map_Manager.h"
#include "ace/Synch.h"

#include "jaws3/Export.h"

#define JAWS_SYMBOL_TABLE_ENTRY \
        ACE_Hash_Map_Entry <const ACE_TCHAR *, const ACE_TCHAR *>

#define JAWS_SYMBOL_TABLE_BASE \
        ACE_Hash_Map_Manager_Ex<const ACE_TCHAR *, const ACE_TCHAR *, \
                                ACE_Hash<const ACE_TCHAR *>, ACE_Equal_To<const ACE_TCHAR *>, \
                                ACE_SYNCH_NULL_MUTEX>

#define JAWS_SYMBOL_TABLE_ITERATOR_BASE \
        ACE_Hash_Map_Iterator_Base_Ex<const ACE_TCHAR *, const ACE_TCHAR *, \
                                      ACE_Hash<const ACE_TCHAR *>, \
                                      ACE_Equal_To<const ACE_TCHAR *>, \
                                      ACE_SYNCH_NULL_MUTEX>

#define JAWS_SYMBOL_TABLE_ITERATOR \
        ACE_Hash_Map_Iterator_Ex<const ACE_TCHAR *, const ACE_TCHAR *, \
                                 ACE_Hash<const ACE_TCHAR *>, ACE_Equal_To<const ACE_TCHAR *>, \
                                 ACE_SYNCH_NULL_MUTEX>

#define JAWS_SYMBOL_TABLE_REVERSE_ITERATOR \
        ACE_Hash_Map_Reverse_Iterator_Ex<const ACE_TCHAR *, const ACE_TCHAR *, \
                                         ACE_Hash<const ACE_TCHAR *>, \
                                         ACE_Equal_To<const ACE_TCHAR *>, \
                                         ACE_SYNCH_NULL_MUTEX>

class JAWS_Symbol_Table;

class JAWS_Export JAWS_Symbol_Table : public JAWS_SYMBOL_TABLE_BASE

// = TITLE
//     A class the associates a string with another string.
{
public:

  // = Initialization methods

  JAWS_Symbol_Table (size_t size = 211);
  // Hash table <size> should be a prime.

};

#endif /* JAWS_SYMBOL_TABLE_H */
