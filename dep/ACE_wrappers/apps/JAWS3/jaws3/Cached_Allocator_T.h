/* -*- c++ -*- */
// $Id: Cached_Allocator_T.h 91743 2010-09-13 18:24:51Z johnnyw $

#ifndef JAWS_CACHED_ALLOCATOR_T_H
#define JAWS_CACHED_ALLOCATOR_T_H

#include "ace/ACE.h"
#include "ace/Synch.h"
#include "ace/Malloc.h"
#include "ace/Free_List.h"

#define JAWS_DEFAULT_ALLOCATOR_CHUNKS 10
#define JAWS_CACHED_ALLOCATOR(T) \
        JAWS_Cached_Allocator< T, ACE_SYNCH_NULL_MUTEX >

template <class T, class ACE_LOCK>
class JAWS_Cached_Allocator : public ACE_New_Allocator
// = TITLE
//   Create a cached memory pool with <n_chunks> chunks each with
//   sizeof (<TYPE>) size.
//
// = DESCRIPTION
//   This class enables caching of dynamically allocated,
//   fixed-sized classes.
{
public:

  JAWS_Cached_Allocator (size_t n_chunks = JAWS_DEFAULT_ALLOCATOR_CHUNKS);

  ~JAWS_Cached_Allocator (void);

  void* malloc (size_t);
  // get a chunk of memory from free store.

  void free (void *);
  // return a chunk of memory back to free store.

protected:

  char * get_next_pool (char *pool);

  void set_next_pool (char *pool, char *next_pool);

  void extend_pool (void);

private:

  size_t pool_size_;

  char *pool_head_;
  // Head of memory pool.

  char *pool_tail_;
  // Tail of memory pool.

  ACE_Locked_Free_List<ACE_Cached_Mem_Pool_Node<T>, ACE_LOCK> free_list_;
  // Maintain a cached memory free list.

};



template <class T>
class JAWS_TSS_Cached_Allocator : public ACE_New_Allocator
// = TITLE
//   Create a thread specific cached memory pool with <n_chunks>
//   chunks each with sizeof (<TYPE>) size.
//
// = DESCRIPTION
//   This class enables caching of dynamically allocated,
//   fixed-sized classes.
{
public:

  JAWS_TSS_Cached_Allocator (size_t n_chunks = JAWS_DEFAULT_ALLOCATOR_CHUNKS);

  ~JAWS_TSS_Cached_Allocator (void);

  void * malloc (size_t);
  // get a chunk of memory from free store.

  void free (void *);
  // return a chunk of memory back to free store.

protected:

  JAWS_Cached_Allocator<T, ACE_SYNCH_NULL_MUTEX> * ts_allocator (void);

private:

  size_t n_chunks_;

  ACE_TSS_TYPE (JAWS_CACHED_ALLOCATOR(T)) ts_allocator_;

};


#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "jaws3/Cached_Allocator_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("Cached_Allocator_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */


#endif /* JAWS_CACHED_ALLOCATOR_T_H */
