/* -*- c++ -*- */
// $Id: Assoc_Array.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_ASSOC_ARRAY_H
#define JAWS_ASSOC_ARRAY_H

template <class KEY, class DATA> class JAWS_Assoc_Array_Iterator;

template <class KEY, class DATA>
class JAWS_Assoc_Array
{

friend class JAWS_Assoc_Array_Iterator<KEY, DATA>;

public:
  JAWS_Assoc_Array (int maxsize = 1024);
  ~JAWS_Assoc_Array (void);

  int index (const KEY &k);
  // Returns the index into the array associated with key k
  // Returns -1 if not found.

  DATA * find (const KEY &k);
  // Returns the data associated with key k.  0 if not found.

  DATA * find_by_index (int i);
  // Returns the data associated with array index i.  Returns 0 if the
  // index is invalid.

  DATA * insert (const KEY &k, const DATA &d);
  // Inserts a *copy* of the key and data into the associated array.
  // Both KEY and DATA must have well defined semantics for copy
  // construction.  This method returns a pointer to the inserted item
  // copy, or 0 if an error occurred.  NOTE: if an identical key
  // already exists in the tree, no new item is created, and the
  // returned pointer addresses the existing item associated with the
  // existing key.

  int remove (const KEY &k);
  // Removes the item associated with the given key from the
  // tree and destroys it.  Returns 1 if it found the item
  // and successfully destroyed it, 0 if it did not find the
  // item, or -1 if an error occurred.

  void clear (void);
  // Destroys all keys and associated data.

protected:

  int find_i (const KEY &k);
  // If k points to an associated data item, then this function
  // returns the index into the arrays that hold it.  Otherwise, it
  // returns an index suitable to insert the item.  If the item is not
  // found and the table is full, maxsize_ is returned.

private:
  KEY **k_array_;
  DATA **d_array_;
  int maxsize_;
};

template <class KEY, class DATA>
class JAWS_Assoc_Array_Iterator
{
public:

  JAWS_Assoc_Array_Iterator (const JAWS_Assoc_Array<KEY, DATA> &aa);
  ~JAWS_Assoc_Array_Iterator (void);

  KEY * key (void);
  DATA * data (void);

  int first (void);
  int last (void);
  int next (void);
  int previous (void);
  int is_done (void);

private:

  // declare private and do not define: explicitly
  // prevent assignment and copy construction of iterators
  JAWS_Assoc_Array_Iterator (const JAWS_Assoc_Array_Iterator<KEY, DATA> &);
  void operator= (const JAWS_Assoc_Array_Iterator<KEY, DATA> &);

private:

  const JAWS_Assoc_Array<KEY, DATA> &aa_;

  int i_;
  // The current item pointed by iterator.

  int j_;
  // The next item to be pointed to by iterator.

};

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "JAWS/Assoc_Array.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#endif /* !defined (JAWS_ASSOC_ARRAY_H) */
