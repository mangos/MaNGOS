// $Id: Cache_List_T.cpp 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_CACHE_LIST_T_CPP
#define JAWS_CACHE_LIST_T_CPP

#include "JAWS/Cache_List_T.h"
#include "JAWS/Cache_Manager_T.h"

template <class EXT_ID, class FACT, class H_FN, class E_FN>
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::JAWS_Cache_List (ACE_Allocator *alloc,
                                                       size_t maxsize)
  : allocator_ (alloc),
    maxsize_ (maxsize),
    size_ (0),
    head_ (0),
    tail_ (0)
{
  if (this->allocator_ == 0)
    this->allocator_ = ACE_Allocator::instance ();
}

template <class EXT_ID, class FACT, class H_FN, class E_FN>
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::~JAWS_Cache_List (void)
{
  while (this->head_ != 0)
    this->remove (this->head_);

  this->allocator_ = 0;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::is_empty (void) const
{
  return (this->size_ == 0);
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::is_full (void) const
{
  return (this->size_ == this->maxsize_);
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> size_t
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::size (void) const
{
  return this->size_;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> size_t
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::maxsize (void) const
{
  return this->maxsize_;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::maxsize (Cache_Manager *,
                                                size_t new_maxsize)
{
  this->maxsize_ = new_maxsize;
  return 0;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> void
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::insert_i (Cache_List_Item *item)
{
  /* ASSERT: this->size_ < this->maxsize_ */
  item->next_ = 0;
  item->prev_ = 0;

  if (this->size_++ == 0)
    this->head_ = this->tail_ = item;
  else
    {
      this->tail_->next_ = item;
      item->prev_ = this->tail_;
      this->tail_ = item;
    }
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::insert (const EXT_ID &ext_id,
                                               JAWS_Cache_Object *const &int_id)
{
  if (this->is_full ())
    return -1;

  Cache_List_Item *item = 0;
  ACE_NEW_MALLOC_RETURN (item,
                         (Cache_List_Item *)
                         this->allocator_->malloc (sizeof (Cache_List_Item)),
                         Cache_List_Item (ext_id, int_id), -1);

  this->insert_i (item);

  return 0;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> void
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::remove_i (void)
{
  /* ASSERT: this->size_ > 0 */
  this->size_--;
  this->item_ = this->head_;
  if (this->head_ == this->tail_)
    {
      this->head_ = this->tail_ = 0;
      return;
    }

  this->head_ = this->head_->next_;
  this->head_->prev_ = 0;
  this->item_->next_ = 0;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> void
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::remove_i (Cache_List_Item *item)
{
  this->size_--;
  this->item_ = item;

  if (item->next_ == 0 && item->prev_ == 0)
    {
      this->head_ = this->tail_ = 0;
    }
  else if (item->next_ == 0)
    {
      this->tail_ = item->prev_;
      this->tail_->next_ = 0;
    }
  else if (item->prev_ == 0)
    {
      this->head_ = item->next_;
      this->head_->prev_ = 0;
    }
  else
    {
      item->next_->prev_ = item->prev_;
      item->prev_->next_ = item->next_;
    }

  item->next_ = 0;
  item->prev_ = 0;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::remove (EXT_ID &ext_id,
                                               JAWS_Cache_Object *&int_id)
{
  if (this->is_empty ())
    return -1;

  this->remove_i ();

  ext_id = this->item_->ext_id_;
  int_id = this->item_->int_id_;
  int_id->heap_item (0);


  ACE_DES_FREE_TEMPLATE4(this->item_, this->allocator_->free,
                         JAWS_Cache_List_Item,
                         EXT_ID, FACT, H_FN, E_FN);

  this->item_ = 0;
  return 0;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::remove (void *item)
{
  if (item == 0)
    return 0;

  this->remove_i ((Cache_List_Item *) item);
  this->item_->int_id_->heap_item (0);

  ACE_DES_FREE_TEMPLATE4(this->item_, this->allocator_->free,
                         JAWS_Cache_List_Item,
                         EXT_ID, FACT, H_FN, E_FN);

  this->item_ = 0;

  return 0;
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> int
JAWS_Cache_List<EXT_ID,FACT,H_FN,E_FN>::adjust (void *item)
{
  if (item == 0)
    return 0;

  Cache_List_Item *real_item = (Cache_List_Item *) item;

  this->remove_i (real_item);
  this->insert_i (real_item);
  this->item_ = 0;

  return 0;
}


template <class EXT_ID, class FACT, class H_FN, class E_FN>
JAWS_Cache_List_Item<EXT_ID,FACT,H_FN,E_FN>::
JAWS_Cache_List_Item (const EXT_ID &ext_id, JAWS_Cache_Object *const &int_id)
  : ext_id_ (ext_id),
    int_id_ (int_id)
{
  this->int_id_->heap_item (this);
}

template <class EXT_ID, class FACT, class H_FN, class E_FN> unsigned int
JAWS_Cache_List_Item<EXT_ID,FACT,H_FN,E_FN>::priority (void)
{
  return this->int_id_->priority ();
}


#endif /* JAWS_CACHE_LIST_T_CPP */
