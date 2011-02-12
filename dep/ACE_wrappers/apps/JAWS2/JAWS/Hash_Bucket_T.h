/* -*- c++ -*- */
// Hey Emacs!  This is a C++ file!
// $Id: Hash_Bucket_T.h 81993 2008-06-16 20:26:16Z sowayaa $

#ifndef JAWS_HASH_BUCKET_T_H
#define JAWS_HASH_BUCKET_T_H

#include "ace/Containers.h"

#define JAWS_HASH_BUCKET_ITEM JAWS_Hash_Bucket_Item<EXT_ID, INT_ID>
#define JAWS_HASH_BUCKET_DLCSTACK JAWS_Hash_Bucket_DLCStack<EXT_ID, INT_ID>
#define JAWS_HASH_BUCKET_DLCSTACK_ITERATOR \
        JAWS_Hash_Bucket_DLCStack_Iterator<EXT_ID, INT_ID>


// Why Hash_Bucket?
//
// This is an attempt to simplify the creation of high-performance
// hash tables with respect to concurrent access by multiple threads.
// To this end, we attempt to raise the amount of concurrency through
// the use or readers/writer locks rather than through mutual
// exclusion.

template <class EXT_ID, class INT_ID>
class JAWS_Hash_Bucket_Item
{
public:
  JAWS_Hash_Bucket_Item (const EXT_ID &ext_id, const INT_ID &int_id,
                         JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *next = 0,
                         JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *prev = 0);
  JAWS_Hash_Bucket_Item (JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *next = 0,
                         JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *prev = 0);

  ~JAWS_Hash_Bucket_Item (void);
  // Destructor.

  EXT_ID ext_id_;
  // Key used to look up an entry.

  INT_ID int_id_;
  // The contents of the entry itself.

  JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *next_;
  // Pointer to the next item in the bucket of overflow nodes.

  JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *prev_;
  // Pointer to the prev item in the bucket of overflow nodes.

};


template <class EXT_ID, class INT_ID> class JAWS_Hash_Bucket_DLCStack_Iterator;

template <class EXT_ID, class INT_ID, class EQ_FUNC>
class JAWS_Hash_Bucket_Manager;

template <class EXT_ID, class INT_ID>
class JAWS_Hash_Bucket_DLCStack
// Create a doubly linked circular stack to be managed by the
// Hash_Bucket_Manager
{
  friend class JAWS_Hash_Bucket_DLCStack_Iterator<EXT_ID, INT_ID>;

public:

  JAWS_Hash_Bucket_DLCStack (ACE_Allocator *alloc = 0);
  ~JAWS_Hash_Bucket_DLCStack (void);

  int is_empty (void) const;
  // Returns 1 if the container is empty, otherwise returns 0.

  JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *push (const EXT_ID &ext_id,
                                              const INT_ID &int_id);
  // Adds <new_item> to the head of the list.
  // Returns the new item that was inserted.

  JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *pop (void);
  // Removes and returns the first <item> in the list.  Returns
  // internal node's address on success, 0 if the queue was empty.
  // This method will *not* free the internal node.

  void reset (void);
  // Reset the <JAWS_Hash_Bucket_DLCStack> to be empty.
  // Notice that since no one is interested in the items within,
  // This operation will delete all items.

  int remove (JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *item);
  // If item is still part of the CStack, it is removed.
  // In anycase, if there is no error, item is freed.
  // Returns 0 if ok, -1 on error.

  ACE_Allocator *allocator_;

private:

  JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *head_;
  JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *tail_;

};


template <class EXT_ID, class INT_ID>
class JAWS_Hash_Bucket_DLCStack_Iterator
{
public:

  JAWS_Hash_Bucket_DLCStack_Iterator (const JAWS_HASH_BUCKET_DLCSTACK &dlcstack);

  int first (void);
  // Moves to first element in the set, clears done flag.  Returns 0
  // if empty, 1 otherwise.

  int last (void);
  // Moves to last element in the set, clears done flag.  Returns 0 if
  // empty, 1 otherwise.

  int advance (void);
  // Move forward by one element of set.  Returns 0 if empty or we end
  // up being the first element in the set, 1 otherwise.  If advance
  // takes us to the first element, done is set to true.

  int revert (void);
  // Move backward by one element of set.  Returns 0 if empty or we
  // end up being the last element in the set, 1 otherwise.  If revert
  // takes us to the last element, done is set to true.

  int next (JAWS_HASH_BUCKET_ITEM *&item);
  int next (JAWS_HASH_BUCKET_ITEM *&item) const;
  // Pass back the next item.  Returns 0 if done is true, 1 otherwise.

  int prev (JAWS_HASH_BUCKET_ITEM *&item);
  int prev (JAWS_HASH_BUCKET_ITEM *&item) const;
  // Pass back the previous item.  Returns 0 if done is true, 1
  // otherwise.

  int done (void) const;
  // Returns 1 if done_ flag is set, 0 otherwise.  done_ flag is set
  // if next takes us to first element or prev takes us to last
  // element.

private:
  const JAWS_HASH_BUCKET_DLCSTACK &dlcstack_;
  JAWS_HASH_BUCKET_ITEM *next_;
  JAWS_HASH_BUCKET_ITEM *prev_;
  int done_;
};


template <class EXT_ID, class INT_ID, class EQ_FUNC>
class JAWS_Hash_Bucket_Manager
{
public:
  JAWS_Hash_Bucket_Manager (ACE_Allocator *alloc = 0);
  int open (ACE_Allocator *alloc = 0);

  ~JAWS_Hash_Bucket_Manager (void);
  int close (void);

  int find (const EXT_ID &ext_id) const;
  int find (const EXT_ID &ext_id, INT_ID &int_id) const;
  // Locate <ext_id> and pass out parameter via <int_id>.  If found,
  // return 0, returns -1 if not found.

  int bind (const EXT_ID &ext_id, const INT_ID &int_id);
  int trybind (const EXT_ID &ext_id, INT_ID &int_id);
  // Associate <ext_id> with <int_id> if and only if <ext_id> is not
  // in the map.  If <ext_id> is already in the map then the <int_id>
  // parameter is assigned the existing value in the map.  Returns 0
  // if a new entry is bound successfully, returns 1 if an attempt is
  // made to bind an existing entry, and returns -1 if failures occur.

  int rebind (const EXT_ID &ext_id, const INT_ID &int_id,
              EXT_ID &old_ext_id, INT_ID &old_int_id);
  // Associate <ext_id> with <int_id>.  If <ext_id> is not in the map
  // then behaves just like <bind>.  Otherwise, store the old values
  // of <ext_id> and <int_id> into the "out" parameters and rebind the
  // new parameters.  This is very useful if you need to have an
  // atomic way of updating <JAWS_Hash_Map_Entrys> and you also need full
  // control over memory allocation.  Returns 0 if a new entry is
  // bound successfully, returns 1 if an existing entry was rebound,
  // and returns -1 if failures occur.

  int unbind (const EXT_ID &ext_id);
  int unbind (const EXT_ID &ext_id, INT_ID &int_id);
  // Break any association of <ext_id>.  Returns the value of <int_id>
  // in case the caller needs to deallocate memory.  Return value is 0
  // if unbind succeeds, -1 otherwise.

protected:

  JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *find_i (const EXT_ID &ext_id) const;
  // Returns the item associated with ext_id if found in list.
  // Returns NULL if not found.

private:

  JAWS_Hash_Bucket_DLCStack<EXT_ID, INT_ID> dlcstack_;

};


#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "JAWS/Hash_Bucket_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#endif /* JAWS_HASH_BUCKET_T_H */
