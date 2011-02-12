// $Id: Hash_Bucket_T.cpp 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_HASH_BUCKET_T_CPP
#define JAWS_HASH_BUCKET_T_CPP

#include "JAWS/Hash_Bucket_T.h"

// -----------------
// Hash_Bucket_Item
// -----------------

template <class EXT_ID, class INT_ID>
JAWS_Hash_Bucket_Item<EXT_ID, INT_ID>
::JAWS_Hash_Bucket_Item (const EXT_ID &ext_id, const INT_ID &int_id,
                        JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *next,
                        JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *prev)
  : ext_id_ (ext_id),
    int_id_ (int_id),
    next_ (next),
    prev_ (prev)
{
}

template <class EXT_ID, class INT_ID>
JAWS_Hash_Bucket_Item<EXT_ID, INT_ID>
::JAWS_Hash_Bucket_Item (JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *next,
                        JAWS_Hash_Bucket_Item<EXT_ID, INT_ID> *prev)
  : next_ (next),
    prev_ (prev)
{
}

template <class EXT_ID, class INT_ID>
JAWS_Hash_Bucket_Item<EXT_ID, INT_ID>::~JAWS_Hash_Bucket_Item (void)
{
  this->next_ = 0;
  this->prev_ = 0;
}


// ---------------------
// Hash_Bucket_DLCStack
// ---------------------

template <class EXT_ID, class INT_ID>
JAWS_Hash_Bucket_DLCStack<EXT_ID, INT_ID>::
JAWS_Hash_Bucket_DLCStack (ACE_Allocator *alloc)
  : allocator_ (alloc),
    head_ (0),
    tail_ (0)
{
  if (this->allocator_ == 0)
    this->allocator_ = ACE_Allocator::instance ();
}

template <class EXT_ID, class INT_ID>
JAWS_Hash_Bucket_DLCStack<EXT_ID, INT_ID>::
~JAWS_Hash_Bucket_DLCStack (void)
{
  this->reset ();
}

template <class EXT_ID, class INT_ID> int
JAWS_Hash_Bucket_DLCStack<EXT_ID, INT_ID>::
is_empty (void) const
{
  return this->head_ == 0 && this->tail_ == 0;
}

template <class EXT_ID, class INT_ID> JAWS_HASH_BUCKET_ITEM *
JAWS_Hash_Bucket_DLCStack<EXT_ID, INT_ID>::
push (const EXT_ID &ext_id, const INT_ID &int_id)
{
  size_t malloc_size = sizeof (JAWS_HASH_BUCKET_ITEM);
  JAWS_HASH_BUCKET_ITEM *item;
  ACE_NEW_MALLOC_RETURN (item,
                         (JAWS_HASH_BUCKET_ITEM *)
                         this->allocator_->malloc (malloc_size),
                         JAWS_HASH_BUCKET_ITEM (ext_id, int_id), 0);

  if (item != 0)
    {
      if (this->is_empty ())
        {
          this->head_ = item;
          this->tail_ = item;
          item->next_ = this->head_;
          item->prev_ = this->tail_;
        }
      else
        {
          item->next_ = this->head_;
          item->prev_ = this->tail_;
          this->head_->prev_ = item;
          this->tail_->next_ = item;
          this->head_ = item;
        }
    }

  return item;
}

template <class EXT_ID, class INT_ID> JAWS_HASH_BUCKET_ITEM *
JAWS_Hash_Bucket_DLCStack<EXT_ID, INT_ID>::pop (void)
{
  JAWS_HASH_BUCKET_ITEM *item = 0;

  if (! this->is_empty ())
    {
      item = this->head_;
      if (this->head_ == this->tail_)
        {
          this->head_ = this->tail_ = 0;
        }
      else
        {
          this->head_ = this->head_->next_;
          this->head_->prev_ = this->tail_;
          this->tail_->next_ = this->head_;
        }
      item->next_ = 0;
      item->prev_ = 0;
    }

  return item;
}

template <class EXT_ID, class INT_ID> void
JAWS_Hash_Bucket_DLCStack<EXT_ID, INT_ID>::reset (void)
{
  JAWS_HASH_BUCKET_ITEM *item = 0;

  while ((item = this->pop ()) != 0)
    this->remove (item);
}

template <class EXT_ID, class INT_ID> int
JAWS_Hash_Bucket_DLCStack<EXT_ID, INT_ID>::remove (JAWS_HASH_BUCKET_ITEM *item)
{
  int result = 0;

  if (item != 0)
    {
      if (item->next_ != 0 && item->prev_ != 0)
        {
          if (item->next_ != item)
            {
              if (this->head_ == item)
                this->head_ = item->next_;
              if (this->tail_ == item)
                this->tail_ = item->prev_;
              item->next_->prev_ = item->prev_;
              item->prev_->next_ = item->next_;
            }
          else
            {
              this->head_ = this->tail_ = 0;
            }
          item->next_ = 0;
          item->prev_ = 0;
        }

      if (item->next_ == 0 && item->prev_ == 0)
        {
          ACE_DES_FREE_TEMPLATE2 (item, this->allocator_->free,
                                  JAWS_Hash_Bucket_Item, EXT_ID, INT_ID);
        }
      else
        result = -1;
    }

  return result;
}


// ------------------------------
// Hash_Bucket_DLCStack_Iterator
// ------------------------------

template <class EXT_ID, class INT_ID>
JAWS_Hash_Bucket_DLCStack_Iterator<EXT_ID, INT_ID>::
JAWS_Hash_Bucket_DLCStack_Iterator (const JAWS_HASH_BUCKET_DLCSTACK &dlcstack)
  : dlcstack_ (dlcstack),
    next_ (0),
    prev_ (0),
    done_ (0)
{
}

template <class EXT_ID, class INT_ID> int
JAWS_Hash_Bucket_DLCStack_Iterator<EXT_ID, INT_ID>::first (void)
{
  int result = 0;

  if (! this->dlcstack_.is_empty ())
    {
      result = 1;
      this->next_ = this->dlcstack_.head_;
      this->prev_ = this->dlcstack_.tail_;
      this->done_ = 0;
    }

  return result;
}

template <class EXT_ID, class INT_ID> int
JAWS_Hash_Bucket_DLCStack_Iterator<EXT_ID, INT_ID>::last (void)
{
  return this->first ();
}

template <class EXT_ID, class INT_ID> int
JAWS_Hash_Bucket_DLCStack_Iterator<EXT_ID, INT_ID>::advance (void)
{
  int result = 1;

  if (this->next_ != 0)
    {
      this->prev_ = this->next_;
      this->next_ = this->next_->next_;
      if (this->next_ == this->dlcstack_.head_)
        {
          this->done_ = 1;
          result = 0;
        }
    }
  else
    result = this->first ();

  return result;
}

template <class EXT_ID, class INT_ID> int
JAWS_Hash_Bucket_DLCStack_Iterator<EXT_ID, INT_ID>::revert (void)
{
  int result = 1;

  if (this->prev_ != 0)
    {
      this->next_ = this->prev_;
      this->prev_ = this->prev_->prev_;
      if (this->prev_ == this->dlcstack_.tail_)
        {
          this->done_ = 1;
          result = 0;
        }
    }
  else
    result = this->last ();

  return result;
}

template <class EXT_ID, class INT_ID> int
JAWS_Hash_Bucket_DLCStack_Iterator<EXT_ID, INT_ID>::
next (JAWS_HASH_BUCKET_ITEM *&item)
{
  if (this->next_ == 0)
    this->first ();

  item = this->next_;
  return ! this->done ();
}

template <class EXT_ID, class INT_ID> int
JAWS_Hash_Bucket_DLCStack_Iterator<EXT_ID, INT_ID>::
next (JAWS_HASH_BUCKET_ITEM *&item) const
{
  item = this->next_;
  return ! this->done ();
}

template <class EXT_ID, class INT_ID> int
JAWS_Hash_Bucket_DLCStack_Iterator<EXT_ID, INT_ID>::
prev (JAWS_HASH_BUCKET_ITEM *&item)
{
  if (this->prev_ == 0)
    this->last ();

  item = this->prev_;
  return ! this->done ();
}

template <class EXT_ID, class INT_ID> int
JAWS_Hash_Bucket_DLCStack_Iterator<EXT_ID, INT_ID>::
prev (JAWS_HASH_BUCKET_ITEM *&item) const
{
  item = this->prev_;
  return ! this->done ();
}

template <class EXT_ID, class INT_ID> int
JAWS_Hash_Bucket_DLCStack_Iterator<EXT_ID, INT_ID>::done (void) const
{
  return this->done_;
}


// --------------------
// Hash_Bucket_Manager
// --------------------

template <class EXT_ID, class INT_ID, class EQ_FUNC>
JAWS_Hash_Bucket_Manager<EXT_ID,INT_ID,EQ_FUNC>
::JAWS_Hash_Bucket_Manager (ACE_Allocator *alloc)
  : dlcstack_ (alloc)
{
  if (alloc == 0)
    this->dlcstack_.allocator_ = ACE_Allocator::instance ();
}

template <class EXT_ID, class INT_ID, class EQ_FUNC> int
JAWS_Hash_Bucket_Manager<EXT_ID,INT_ID,EQ_FUNC>::open (ACE_Allocator *alloc)
{
  this->dlcstack_.allocator_ = alloc;
  if (alloc == 0)
    this->dlcstack_.allocator_ = ACE_Allocator::instance ();

  return 0;
}

template <class EXT_ID, class INT_ID, class EQ_FUNC>
JAWS_Hash_Bucket_Manager<EXT_ID,INT_ID,EQ_FUNC>::~JAWS_Hash_Bucket_Manager (void)
{
}

template <class EXT_ID, class INT_ID, class EQ_FUNC> int
JAWS_Hash_Bucket_Manager<EXT_ID,INT_ID,EQ_FUNC>::close (void)
{
  this->dlcstack_.reset ();
  return 0;
}

template <class EXT_ID, class INT_ID, class EQ_FUNC> JAWS_HASH_BUCKET_ITEM *
JAWS_Hash_Bucket_Manager<EXT_ID,INT_ID,EQ_FUNC>
::find_i (const EXT_ID &ext_id) const
{
  JAWS_HASH_BUCKET_DLCSTACK_ITERATOR iter (this->dlcstack_);
  JAWS_HASH_BUCKET_ITEM *item = 0;

  if (iter.first ())
    while (!iter.done ())
      {
        iter.next (item);
        if (item && EQ_FUNC (item->ext_id_, ext_id))
          break;
        iter.advance ();
      }

  return (item && EQ_FUNC (item->ext_id_, ext_id)) ? item : 0;
}

template <class EXT_ID, class INT_ID, class EQ_FUNC> int
JAWS_Hash_Bucket_Manager<EXT_ID,INT_ID,EQ_FUNC>::find (const EXT_ID &ext_id,
                                                      INT_ID &int_id) const
{
  int result = -1;
  JAWS_HASH_BUCKET_ITEM *item = this->find_i (ext_id);

  if (item)
    {
      int_id = item->int_id_;
      result = 0;
    }

  return result;
}

template <class EXT_ID, class INT_ID, class EQ_FUNC> int
JAWS_Hash_Bucket_Manager<EXT_ID,INT_ID,EQ_FUNC>
::find (const EXT_ID &ext_id) const
{
  INT_ID dummy_id;
  return this->find (ext_id, dummy_id);
}

template <class EXT_ID, class INT_ID, class EQ_FUNC> int
JAWS_Hash_Bucket_Manager<EXT_ID,INT_ID,EQ_FUNC>::bind (const EXT_ID &ext_id,
                                                      const INT_ID &int_id)
{
  int result = 0;

  if (this->find (ext_id) == 0)
    {
      result = 1;
    }
  else
    {
      if (this->dlcstack_.push (ext_id, int_id) == 0)
        result = -1;
    }

  return result;
}

template <class EXT_ID, class INT_ID, class EQ_FUNC> int
JAWS_Hash_Bucket_Manager<EXT_ID,INT_ID,EQ_FUNC>::trybind (const EXT_ID &ext_id,
                                                         INT_ID &int_id)
{
  int result = 0;

  if (this->find (ext_id, int_id) == 0)
    {
      result = 1;
    }
  else
    {
      if (this->dlcstack_.push (ext_id, int_id) == 0)
        result = -1;
    }

  return result;
}

template <class EXT_ID, class INT_ID, class EQ_FUNC> int
JAWS_Hash_Bucket_Manager<EXT_ID,INT_ID,EQ_FUNC>::rebind (const EXT_ID &ext_id,
                                                        const INT_ID &int_id,
                                                        EXT_ID &old_ext_id,
                                                        INT_ID &old_int_id)
{
  int result = 0;
  JAWS_HASH_BUCKET_ITEM *item = this->find_i (ext_id);

  if (item)
    {
      result = 1;
      old_ext_id = item->ext_id_;
      old_int_id = item->int_id_;
      this->dlcstack_.remove (item);
    }

  if (this->dlcstack_.push (ext_id, int_id) == 0)
    result = -1;

  return result;
}

template <class EXT_ID, class INT_ID, class EQ_FUNC> int
JAWS_Hash_Bucket_Manager<EXT_ID,INT_ID,EQ_FUNC>::unbind (const EXT_ID &ext_id,
                                                        INT_ID &int_id)
{
  int result = -1;
  JAWS_HASH_BUCKET_ITEM *item = this->find_i (ext_id);

  if (item)
    {
      result = 0;
      int_id = item->int_id_;
      this->dlcstack_.remove (item);
    }

  return result;
}

template <class EXT_ID, class INT_ID, class EQ_FUNC> int
JAWS_Hash_Bucket_Manager<EXT_ID,INT_ID,EQ_FUNC>::unbind (const EXT_ID &ext_id)
{
  INT_ID dummy_id;
  return this->unbind (ext_id, dummy_id);
}

#endif /* JAWS_HASH_BUCKET_T_CPP */
