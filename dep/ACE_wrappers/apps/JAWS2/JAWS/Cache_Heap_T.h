/* -*- c++ -*- */
// $Id: Cache_Heap_T.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_CACHE_HEAP_T_H
#define JAWS_CACHE_HEAP_T_H

#include "ace/Malloc.h"
#include "JAWS/Cache_Object.h"

// Forward declarations
template <class EXT_ID, class FACTORY, class HASH_FUNC, class EQ_FUNC>
class JAWS_Cache_Manager;

template <class EXT_ID, class FACTORY, class HASH_FUNC, class EQ_FUNC>
class JAWS_Cache_Heap_Item;


template <class EXT_ID, class FACT, class H_FN, class E_FN>
class JAWS_Cache_Heap
// Roll my own heap here.  Eventually, a heap should be its own
// standalone data structure.
{
public:

  typedef JAWS_Cache_Manager<EXT_ID, FACT, H_FN, E_FN> Cache_Manager;
  typedef JAWS_Cache_Heap_Item<EXT_ID, FACT, H_FN, E_FN> Cache_Heap_Item;

  JAWS_Cache_Heap (ACE_Allocator *alloc = 0, size_t maxsize = 8192);
  // maxsize is the total number of objects the in memory cache is
  // willing to manage

  ~JAWS_Cache_Heap (void);

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
  // treat item as a Cache_Heap_Item, and remove it from the heap

  int adjust (void *item);
  // treat item as a Cache_Heap_Item, and alter its heap position

protected:

  void insert_i (Cache_Heap_Item *item);
  // insert item into heap.

  void remove_i (size_t pos);
  // remove the element residing at pos, but do not delete it.

  void remove_i (void);
  // remove the element residing at the top of heap, but do not delete it.

private:

  ACE_Allocator *allocator_;

  size_t maxsize_;
  size_t size_;

  Cache_Heap_Item **heap_;

};


template <class EXT_ID, class FACT, class H_FN, class E_FN>
class JAWS_Cache_Heap_Item
{

  friend class JAWS_Cache_Heap<EXT_ID, FACT, H_FN, E_FN>;

public:

  JAWS_Cache_Heap_Item (const EXT_ID &ext_id, JAWS_Cache_Object *const &int_id);
  unsigned int priority (void);

private:

  EXT_ID ext_id_;
  JAWS_Cache_Object *int_id_;

  size_t heap_idx_;

};

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "JAWS/Cache_Heap_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#endif /* JAWS_CACHE_HEAP_T_H */
