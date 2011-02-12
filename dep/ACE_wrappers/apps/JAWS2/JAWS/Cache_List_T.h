/* -*- c++ -*- */
// $Id: Cache_List_T.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_CACHE_LIST_T_H
#define JAWS_CACHE_LIST_T_H

#include "ace/Malloc.h"
#include "JAWS/Cache_Object.h"

// Forward declarations
template <class EXT_ID, class FACTORY, class HASH_FUNC, class EQ_FUNC>
class JAWS_Cache_Manager;

template <class EXT_ID, class FACTORY, class HASH_FUNC, class EQ_FUNC>
class JAWS_Cache_List_Item;


template <class EXT_ID, class FACT, class H_FN, class E_FN>
class JAWS_Cache_List
// Roll my own heap here.  Eventually, a heap should be its own
// standalone data structure.
//
// This version is not a heap, but a doubly linked list.  We are
// trying to simplify all the heap operations to be O(1).
{
public:

  typedef JAWS_Cache_Manager<EXT_ID, FACT, H_FN, E_FN> Cache_Manager;
  typedef JAWS_Cache_List_Item<EXT_ID, FACT, H_FN, E_FN> Cache_List_Item;

  JAWS_Cache_List (ACE_Allocator *alloc = 0, size_t maxsize = 8192);
  // maxsize is the total number of objects the in memory cache is
  // willing to manage

  ~JAWS_Cache_List (void);

  int is_empty (void) const;
  int is_full (void) const;

  size_t size (void) const;
  size_t maxsize (void) const;

  int maxsize (Cache_Manager *cm, size_t new_maxsize);
  // attempt to grow (or shrink) the heap.  Return 0 on success, -1 on
  // error.

  int insert (const EXT_ID &ext_id, JAWS_Cache_Object *const &int_id);
  // attempt to insert int_id into heap.

  int remove (EXT_ID &ext_id, JAWS_Cache_Object *&int_id);
  // attempt to remove the top element of heap.

  int remove (void *item);
  // treat item as a Cache_List_Item, and remove it from the heap

  int adjust (void *item);
  // treat item as a Cache_List_Item, and alter its heap position

protected:

  void insert_i (Cache_List_Item *item);
  // insert item into heap.

  void remove_i (Cache_List_Item *item);
  // remove the element residing at pos, but do not delete it.

  void remove_i (void);
  // remove the element residing at the top of heap, but do not delete it.

private:

  ACE_Allocator *allocator_;

  size_t maxsize_;
  size_t size_;

  Cache_List_Item *item_;

  Cache_List_Item *head_;
  Cache_List_Item *tail_;

};


template <class EXT_ID, class FACT, class H_FN, class E_FN>
class JAWS_Cache_List_Item
{

  friend class JAWS_Cache_List<EXT_ID, FACT, H_FN, E_FN>;

public:

  typedef JAWS_Cache_List<EXT_ID, FACT, H_FN, E_FN> Cache_List;

  JAWS_Cache_List_Item (const EXT_ID &ext_id, JAWS_Cache_Object *const &int_id);
  unsigned int priority (void);

private:

  EXT_ID ext_id_;
  JAWS_Cache_Object *int_id_;

  JAWS_Cache_List_Item *next_;
  JAWS_Cache_List_Item *prev_;
};

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "JAWS/Cache_List_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#endif /* JAWS_CACHE_LIST_T_H */
