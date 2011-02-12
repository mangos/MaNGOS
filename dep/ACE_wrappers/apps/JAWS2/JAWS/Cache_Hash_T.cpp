// $Id: Cache_Hash_T.cpp 91626 2010-09-07 10:59:20Z johnnyw $

#ifndef JAWS_CACHE_HASH_T_CPP
#define JAWS_CACHE_HASH_T_CPP

#include "JAWS/Cache_Hash_T.h"
#include "JAWS/Hash_Bucket_T.h"

template <class EXT_ID, class HASH_FUNC, class EQ_FUNC> unsigned long
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::hash (const EXT_ID &ext_id) const
{
  return HASH_FUNC (ext_id) % this->size_;
}

template <class EXT_ID, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::isprime (unsigned long number) const
{
  unsigned long d = 3;

  if (number <= 2) return (number == 2);

  if (number % 2 == 0) return 0;

  while (d <= number/d)
    {
      if (number % d == 0) return 0;
      d += 2;
    }

  return 1;
}

template <class EXT_ID, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::new_cachebucket (size_t hash_idx)
{
  if (this->hashtable_[hash_idx] == 0)
    {
      size_t alloc_size = sizeof (CACHE_BUCKET_MANAGER);
      ACE_NEW_MALLOC_RETURN (this->hashtable_[hash_idx],
                             (CACHE_BUCKET_MANAGER *)
                             this->allocator_->malloc (alloc_size),
                             CACHE_BUCKET_MANAGER (this->allocator_), -1);
    }

  return 0;
}

template <class EXT_ID, class HASH_FUNC, class EQ_FUNC>
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::JAWS_Cache_Hash (ACE_Allocator *alloc,
                                                          size_t size)
  : allocator_ (alloc),
    hashtable_ (0)
{
  while (!this->isprime (size))
    size++;

  this->size_ = size;

  if (this->allocator_ == 0)
    this->allocator_ = ACE_Allocator::instance ();

  size_t memsize = this->size_ * sizeof (CACHE_BUCKET_MANAGER *);

  this->hashtable_
    = (CACHE_BUCKET_MANAGER **) this->allocator_->malloc (memsize);

  if (this->hashtable_)
    {
      for (size_t i = 0; i < this->size_; i++)
        this->hashtable_[i] = 0;
    }
  else
    {
      this->size_ = 0;
      // should indicate something is wrong to the user.
    }
}

template <class EXT_ID, class HASH_FUNC, class EQ_FUNC>
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::~JAWS_Cache_Hash (void)
{
  if (this->hashtable_)
    {
      for (size_t i = 0; i < this->size_; i++)
        {
          if (this->hashtable_[i])
            {

              ACE_DES_FREE_TEMPLATE3(this->hashtable_[i],
                                     this->allocator_->free,
                                     JAWS_Hash_Bucket_Manager,
                                     EXT_ID,
                                     JAWS_Cache_Object *,
                                     EQ_FUNC);





              this->hashtable_[i] = 0;
            }
        }
      this->allocator_->free (this->hashtable_);
      this->hashtable_ = 0;
    }

  this->allocator_ = 0;
}

template <class EXT_ID, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::find (const EXT_ID &ext_id) const
{
  unsigned long hash_idx = this->hash (ext_id);

  if (this->hashtable_[hash_idx] == 0)
    return -1;

  return this->hashtable_[hash_idx]->find (ext_id);
}

template <class EXT_ID, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::find (const EXT_ID &ext_id,
                                                JAWS_Cache_Object *&int_id) const
{
  unsigned long hash_idx = this->hash (ext_id);

  if (this->hashtable_[hash_idx] == 0)
    return -1;

  return this->hashtable_[hash_idx]->find (ext_id, int_id);
}

template <class EXT_ID, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::bind (const EXT_ID &ext_id,
                                                JAWS_Cache_Object *const &int_id)
{
  int result;
  unsigned long hash_idx = this->hash (ext_id);

  if (this->hashtable_[hash_idx] == 0)
    {
      ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, g, this->lock_, -1);

      if (this->new_cachebucket (hash_idx) == -1)
        return -1;

      result = this->hashtable_[hash_idx]->bind (ext_id, int_id);
    }
  else
    result = this->hashtable_[hash_idx]->bind (ext_id, int_id);

  return result;
}

template <class EXT_ID, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::trybind (const EXT_ID &ext_id,
                                                   JAWS_Cache_Object *&int_id)
{
  int result;
  unsigned long hash_idx = this->hash (ext_id);

  if (this->hashtable_[hash_idx] == 0)
    {
      ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, g, this->lock_, -1);

      if (this->new_cachebucket (hash_idx) == -1)
        return -1;

      result = this->hashtable_[hash_idx]->trybind (ext_id, int_id);
    }
  else
    result = this->hashtable_[hash_idx]->trybind (ext_id, int_id);

  return result;
}

template <class EXT_ID, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::rebind (const EXT_ID &ext_id,
                                                  JAWS_Cache_Object *const &int_id,
                                                  EXT_ID &old_ext_id,
                                                  JAWS_Cache_Object *&old_int_id)
{
  int result;
  unsigned long hash_idx = this->hash (ext_id);

  if (this->hashtable_[hash_idx] == 0)
    {
      ACE_GUARD_RETURN (ACE_SYNCH_MUTEX, g, this->lock_, -1);

      if (this->new_cachebucket (hash_idx) == -1)
        return -1;

      result = this->hashtable_[hash_idx]->rebind (ext_id, int_id,
                                                   old_ext_id, old_int_id);
    }
  else
    result = this->hashtable_[hash_idx]->rebind (ext_id, int_id,
                                                 old_ext_id, old_int_id);

  return result;
}


template <class EXT_ID, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::unbind (const EXT_ID &ext_id)
{
  unsigned long hash_idx = this->hash (ext_id);

  if (this->hashtable_[hash_idx] == 0)
    return -1;

  return this->hashtable_[hash_idx]->unbind (ext_id);
}

template <class EXT_ID, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::unbind (const EXT_ID &ext_id,
                                                  JAWS_Cache_Object *&int_id)
{
  unsigned long hash_idx = this->hash (ext_id);

  if (this->hashtable_[hash_idx] == 0)
    return -1;

  return this->hashtable_[hash_idx]->unbind (ext_id, int_id);
}


template <class EXT_ID, class HASH_FUNC, class EQ_FUNC> size_t
JAWS_Cache_Hash<EXT_ID,HASH_FUNC,EQ_FUNC>::size (void) const
{
  return this->size_;
}




#endif /* JAWS_CACHEHASH_T_CPP */
