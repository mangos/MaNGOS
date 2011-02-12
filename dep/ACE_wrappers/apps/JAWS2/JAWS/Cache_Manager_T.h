/* -*- c++ -*- */
// Hey Emacs!  This is a C++ file!
// $Id: Cache_Manager_T.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_CACHE_MANAGER_T_H
#define JAWS_CACHE_MANAGER_T_H

#include "ace/Singleton.h"

#include "JAWS/Cache_Object.h"

template <class KEY, class HASH_FUNC, class EQ_FUNC> class JAWS_Cache_Hash;
template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC>
class JAWS_Cache_Heap;
template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC>
class JAWS_Cache_List;

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC>
class JAWS_Cache_Manager
{

  friend class JAWS_Cache_Hash<KEY, HASH_FUNC, EQ_FUNC>;
  friend class JAWS_Cache_Heap<KEY, FACTORY, HASH_FUNC, EQ_FUNC>;
  friend class JAWS_Cache_List<KEY, FACTORY, HASH_FUNC, EQ_FUNC>;

public:

  typedef ACE_Singleton<FACTORY, ACE_SYNCH_MUTEX> Object_Factory;
  typedef JAWS_Cache_Hash<KEY, HASH_FUNC, EQ_FUNC> Cache_Hash;
  typedef JAWS_Cache_List<KEY, FACTORY, HASH_FUNC, EQ_FUNC> Cache_Heap;

  JAWS_Cache_Manager (ACE_Allocator *alloc = 0,
                     JAWS_Cache_Object_Factory *cof = 0,

                     size_t hashsize = 8192,   // number of hash buckets
                     size_t maxsize = 65535,   // max number of in memory
                                               // objects

                     size_t maxobjsize = 256,  // max cached object size in kB
                     size_t minobjsize = 0,    // min cached object size in kB

                     size_t highwater = 100,   // max size of cache in MB
                     size_t lowwater = 50,     // min size of cache when
                                               // expiring after highwater
                                               // has been reached

                     int timetolive = -1,      // amt of time the lowest
                                               // priority item is allowed to
                                               // remain in the cache

                     int counted = 0           // flag for whether to use
                                               // counts
                     );

  int open (ACE_Allocator *alloc = 0,
            JAWS_Cache_Object_Factory *cof = 0,

            size_t hashsize = 1024,   // number of hash buckets
            size_t maxsize = 4096,    // max number of in memory
                                      // objects

            size_t maxobjsize = 5120, // max cached object size in kB
            size_t minobjsize = 0,    // min cached object size in kB

            size_t highwater = 50,    // max size of cache in MB
            size_t lowwater = 30,     // min size of cache when
                                      // expiring after highwater
                                      // has been reached

            int timetolive = -1,      // amount of time the lowest
                                      // priority item is allowed to
                                      // remain in the cache

            int counted = 0           // flag for whether to use
                                      // counts
            );

  ~JAWS_Cache_Manager (void);

  int close (void);

  // Search Methods

  int GET (const KEY &key, JAWS_Cache_Object *&cobj);
  // Retrieve the object associated with key from cache.  Return 0 on
  // success, -1 on failure.

  int PUT (const KEY &key, const void *data, size_t size,
           JAWS_Cache_Object *&obj);
  // Inserts or replaces object associated with key into cache.
  // Return 0 on success, -1 on failure.

  int MAKE (const void *data, size_t size, JAWS_Cache_Object *&cobj);
  // Create a cached object, increment reference count.

  int TAKE (JAWS_Cache_Object *const &cobj);
  // Increment reference count.

  int DROP (JAWS_Cache_Object *&cobj);
  // Decrement reference count on cached object, perhaps delete.
  // Returns 0 if only decremented, 1 if deleted, -1 if error.

  int FLUSH (void);
  // Removes lowest priority object from cache.

protected:

  int GET_i (const KEY &key, JAWS_Cache_Object *&object);
  // Retrieve the object associated with key from cache.  Return 0 on
  // success, -1 on failure.

  int PUT_i (const KEY &key, const void *data, size_t size,
             JAWS_Cache_Object *&object);
  // Inserts or replaces object associated with key into cache.
  // Return 0 on success, -1 on failure.

  int FLUSH_i (void);
  // Removes lowest priority object from cache.

  int FLUSH_i (const KEY &key);
  // Removes object associated with key from cache.

  int DROP_i (JAWS_Cache_Object *&cobj);
  // Decrement reference count on cached object, perhaps delete.

private:

  ACE_Allocator *allocator_;
  JAWS_Cache_Object_Factory *factory_;

  size_t hashsize_;
  size_t maxsize_;
  size_t maxobjsize_;
  size_t minobjsize_;
  size_t highwater_;
  size_t lowwater_;
  size_t waterlevel_;
  int timetolive_;
  int counted_;

  Cache_Hash *hash_;
  Cache_Heap *heap_;

  ACE_SYNCH_RW_MUTEX lock_;

};


template <class KEY, class DATA, class CACHE_MANAGER>
class JAWS_Cache_Proxy
{
public:
  typedef CACHE_MANAGER Cache_Manager;
  typedef ACE_Singleton<Cache_Manager, ACE_SYNCH_MUTEX>
          Cache_Manager_Singleton;

  JAWS_Cache_Proxy (const KEY &, Cache_Manager * = 0);
  // Corresponds to a GET

  JAWS_Cache_Proxy (const KEY &, DATA *, size_t, Cache_Manager * = 0);
  // Corresponds to a U/PUT

  virtual ~JAWS_Cache_Proxy (void);

  DATA *data (void) const;
  operator DATA * (void) const;

  virtual int close (DATA *);

private:
  JAWS_Cache_Object *object_;
  Cache_Manager *manager_;
};

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "JAWS/Cache_Manager_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#endif /* JAWS_CACHE_MANAGER_T_H */
