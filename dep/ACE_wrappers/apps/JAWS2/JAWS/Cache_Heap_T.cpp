// $Id: Cache_Heap_T.cpp 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_CACHE_HEAP_T_CPP
#define JAWS_CACHE_HEAP_T_CPP

#include "JAWS/Cache_Heap_T.h"
#include "JAWS/Cache_Manager_T.h"

template <class EXT_ID, class FACT, class H_FN, class E_FN>
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::JAWS_Cache_Heap (ACE_Allocator *alloc,
                                                       size_t maxsize)
  : allocator_ (alloc),
    maxsize_ (maxsize),
    size_ (0)
{
  if (this->allocator_ == 0)
    this->allocator_ = ACE_Allocator::instance ();

  size_t memsize
    = this->maxsize_ * sizeof (Cache_Heap_Item *);

  this->heap_ = (Cache_Heap_Item **) this->allocator_->malloc (memsize);
  if (this->heap_)
    {
      for (size_t i = 0; i < this->maxsize_; i++)
        this->heap_[i] = 0;
    }
  else
    {
      this->maxsize_ = 0;
      // should indicate something
    }
}

template <class EXT_ID, class FACT, class H_FN, class E_FN>
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::~JAWS_Cache_Heap (void)
{
  if (this->heap_ != 0)
    {
      for (size_t i = 0; i < this->maxsize_; i++)
        {
          if (this->heap_[i])
            {
              ACE_DES_FREE_TEMPLATE4(this->heap_[i], this->allocator_->free,
                                     JAWS_Cache_Heap_Item,
                                     EXT_ID, FACT, H_FN, E_FN);

              this->heap_[i] = 0;
            }
        }
      this->allocator_->free (this->heap_);
      this->heap_ = 0;
    }

  this->allocator_ = 0;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::is_empty (void) const
{
  return (this->size_ == 0);
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::is_full (void) const
{
  return (this->size_ == this->maxsize_);
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> size_t
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::size (void) const
{
  return this->size_;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> size_t
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::maxsize (void) const
{
  return this->maxsize_;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::maxsize (Cache_Manager *cm,
                                                size_t new_maxsize)
{
  int result = -1;

  size_t memsize
    = new_maxsize * sizeof (Cache_Heap_Item *);

  Cache_Heap_Item **new_heap
    = (Cache_Heap_Item **) this->allocator_->malloc (memsize);
  if (new_heap)
    {
      while (new_maxsize < this->size_)
        cm->FLUSH_i ();

      for (size_t i = 0; i < new_maxsize; i++)
        if (i < this->size_)
          new_heap[i] = this->heap_[i];
        else
          new_heap[i] = 0;

      Cache_Heap_Item ** volatile temp = this->heap_;
      this->heap_ = new_heap;
      this->maxsize_ = new_maxsize;
      this->allocator_->free (temp);
      result = 0;
    }

  return result;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> void
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::insert_i (Cache_Heap_Item *item)
{
  /* ASSERT: this->size_ < this->maxsize_ */

  size_t i;

  for (i = this->size_ + 1; i > 1; i /= 2)
    {
      if (item->priority () > this->heap_[i/2 - 1]->priority ())
        break;

      this->heap_[i-1] = this->heap_[i/2 - 1];
      this->heap_[i-1]->heap_idx_ = i-1;
    }

  this->heap_[i-1] = item;
  this->heap_[i-1]->heap_idx_ = i-1;
  this->size_++;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::insert (const EXT_ID &ext_id,
                                               JAWS_Cache_Object *const &int_id)
{
  if (this->is_full ())
    return -1;

  Cache_Heap_Item *item;
  ACE_NEW_MALLOC_RETURN (item,
                         (Cache_Heap_Item *)
                         this->allocator_->malloc (sizeof (Cache_Heap_Item)),
                         Cache_Heap_Item (ext_id, int_id), -1);

  this->insert_i (item);

  return 0;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> void
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::remove_i (void)
{
  /* ASSERT: this->size_ > 0 */
  this->size_--;
  Cache_Heap_Item *temp = this->heap_[this->size_];
  this->heap_[this->size_] = 0;

  size_t i = 1;
  while (2*i <= this->size_)
    {
      size_t child = 2*i;
      if ((child < this->size_)
          && (this->heap_[2*i]->priority ()
              < this->heap_[2*i - 1]->priority ()))
        child = 2*i + 1;

      if (temp->priority () < this->heap_[child-1]->priority ())
        break;

      this->heap_[i-1] = this->heap_[child-1];
      this->heap_[i-1]->heap_idx_ = i-1;
      i = child;
    }

  if (this->size_ > 0)
    {
      this->heap_[i-1] = temp;
      this->heap_[i-1]->heap_idx_ = i-1;
    }
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> void
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::remove_i (size_t pos)
{
  Cache_Heap_Item *item = this->heap_[pos];

  if (pos > 0)
    {
      int i = pos + 1;
      do
        {
          this->heap_[i-1] = this->heap_[i/2 - 1];
          this->heap_[i-1]->heap_idx_ = i-1;
          i /= 2;
        }
      while (i > 1);
    }

  this->heap_[0] = item;

  this->remove_i ();
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::remove (EXT_ID &ext_id,
                                               JAWS_Cache_Object *&int_id)
{
  if (this->is_empty ())
    return -1;

  Cache_Heap_Item *item = this->heap_[0];
  item->int_id_->heap_item (0);

  this->remove_i ();

  ext_id = item->ext_id_;
  int_id = item->int_id_;

  ACE_DES_FREE_TEMPLATE4(item, this->allocator_->free,
                         JAWS_Cache_Heap_Item,
                         EXT_ID, FACT, H_FN, E_FN);

  item = 0;
  return 0;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::remove (void *item)
{
  if (item == 0)
    return 0;

  Cache_Heap_Item *real_item = (Cache_Heap_Item *) item;

  // Make sure the item is where it thinks it is.
  if (this->heap_[real_item->heap_idx_] != real_item)
    return -1;

  real_item->int_id_->heap_item (0);
  this->remove_i (real_item->heap_idx_);

  ACE_DES_FREE_TEMPLATE4(real_item, this->allocator_->free,
                         JAWS_Cache_Heap_Item,
                         EXT_ID, FACT, H_FN, E_FN);

  real_item = 0;

  return 0;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_Heap<EXT_ID,FACT,H_FN,E_FN>::adjust (void *item)
{
  if (item == 0)
    return 0;

  Cache_Heap_Item *real_item = (Cache_Heap_Item *) item;

  // Make sure the item is where it thinks it is.
  if (this->heap_[real_item->heap_idx_] != real_item)
    return -1;

  this->remove_i (real_item->heap_idx_);
  this->insert_i (real_item);

  return 0;
}


template <class EXT_ID, class FACT, class H_FN, class E_FN>
JAWS_Cache_Heap_Item<EXT_ID,FACT,H_FN,E_FN>::
JAWS_Cache_Heap_Item (const EXT_ID &ext_id, JAWS_Cache_Object *const &int_id)
  : ext_id_ (ext_id),
    int_id_ (int_id),
    heap_idx_ (0)
{
  this->int_id_->heap_item (this);
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> unsigned int
JAWS_Cache_Heap_Item<EXT_ID,FACT,H_FN,E_FN>::priority (void)
{
  return this->int_id_->priority ();
}


#endif /* JAWS_CACHE_HEAP_T_CPP */
