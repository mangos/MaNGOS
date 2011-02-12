// $Id: Assoc_Array.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#ifndef JAWS_ASSOC_ARRAY_CPP
#define JAWS_ASSOC_ARRAY_CPP

#include "ace/config-all.h"
#include "JAWS/Assoc_Array.h"

template <class KEY, class DATA>
JAWS_Assoc_Array<KEY,DATA>::JAWS_Assoc_Array (int maxsize)
  : k_array_ (0),
    d_array_ (0),
    maxsize_ (maxsize)
{
  typedef void * ptr_type;

  this->k_array_ = reinterpret_cast<KEY **> (new ptr_type[this->maxsize_]);
  if (this->k_array_ == 0)
    {
      this->maxsize_ = 0;
      return;
    }

  this->d_array_ = reinterpret_cast<DATA **> (new ptr_type[this->maxsize_]);
  if (this->d_array_ == 0)
    {
      delete[] this->k_array_;
      this->maxsize_ = 0;
      return;
    }

  for (int i = 0; i < this->maxsize_; i++)
    {
      this->k_array_[i] = 0;
      this->d_array_[i] = 0;
    }
}

template <class KEY, class DATA>
JAWS_Assoc_Array<KEY,DATA>::~JAWS_Assoc_Array (void)
{
  this->clear ();

  delete[] reinterpret_cast<void **> (this->k_array_);
  delete[] reinterpret_cast<void **> (this->d_array_);

  this->k_array_ = 0;
  this->d_array_ = 0;
}

template <class KEY, class DATA> int
JAWS_Assoc_Array<KEY,DATA>::index (const KEY &k)
{
  return this->find_i (k);
}

template <class KEY, class DATA> DATA *
JAWS_Assoc_Array<KEY,DATA>::find (const KEY &k)
{
  int i = this->find_i (k);

  return (i < this->maxsize_) ? this->d_array_[i] : 0;
}

template <class KEY, class DATA> DATA *
JAWS_Assoc_Array<KEY,DATA>::find_by_index (int i)
{
  return ((0 <= i) && (i < this->maxsize_)) ? this->d_array_[i] : 0;
}

template <class KEY, class DATA> DATA *
JAWS_Assoc_Array<KEY,DATA>::insert (const KEY &k, const DATA &d)
{
  int i = this->find_i (k);

  if (i == this->maxsize_)
    return 0;

  KEY *&kk = this->k_array_[i];
  DATA *&dd = this->d_array_[i];

  if (kk == 0)
    {
      dd = new DATA (d);
      if (dd == 0)
        return 0;

      KEY *kkk = new KEY (k);
      if (kkk == 0)
        {
          delete dd;
          return 0;
        }
      kk = kkk;
    }

  return dd;
}

template <class KEY, class DATA> int
JAWS_Assoc_Array<KEY,DATA>::remove (const KEY &k)
{
  int i = this->find_i (k);

  if (i == this->maxsize_)
    return 0;

  KEY *&kk = this->k_array_[i];
  DATA *&dd = this->d_array_[i];

  if (kk != 0)
    {
      KEY *kkk = kk;
      kk = 0;
      delete kkk;
      delete dd;
      dd = 0;
      return 1;
    }

  return 0;
}

template <class KEY, class DATA> void
JAWS_Assoc_Array<KEY,DATA>::clear (void)
{
  for (int i = 0; i < this->maxsize_; i++)
    {
      if (this->k_array_[i] != 0)
        {
          delete this->k_array_[i];
          delete this->d_array_[i];

          this->k_array_[i] = 0;
          this->d_array_[i] = 0;
        }
    }
}

template <class KEY, class DATA> int
JAWS_Assoc_Array<KEY,DATA>::find_i (const KEY &k)
{
  int j = this->maxsize_;

  for (int i = 0; i < this->maxsize_; i++)
    {
      KEY *kk = this->k_array_[i];
      if (kk)
        {
          if (*kk == k)
            return i;
        }
      else if (j == this->maxsize_)
        j = i;
    }
  return j;
}

template <class KEY, class DATA>
JAWS_Assoc_Array_Iterator<KEY,DATA>::
JAWS_Assoc_Array_Iterator (const JAWS_Assoc_Array<KEY, DATA> &aa)
  : aa_ (aa),
    i_ (0),
    j_ (0)
{
}

template <class KEY, class DATA>
JAWS_Assoc_Array_Iterator<KEY,DATA>::~JAWS_Assoc_Array_Iterator (void)
{
}

template <class KEY, class DATA> KEY *
JAWS_Assoc_Array_Iterator<KEY,DATA>::key (void)
{
  return this->aa_.k_array_[this->i_];
}

template <class KEY, class DATA> DATA *
JAWS_Assoc_Array_Iterator<KEY,DATA>::data (void)
{
  return this->aa_.d_array_[this->i_];
}

template <class KEY, class DATA> int
JAWS_Assoc_Array_Iterator<KEY,DATA>::first (void)
{
  this->i_ = 0;
  for (this->j_ = 0; this->j_ < this->aa_.maxsize_; this->j_++)
    {
      if (this->aa_.k_array_[this->j_] != 0)
        break;
    }
  return this->next ();
}

template <class KEY, class DATA> int
JAWS_Assoc_Array_Iterator<KEY,DATA>::last (void)
{
  this->j_ = this->aa_.maxsize_;
  for (this->i_ = this->aa_.maxsize_; this->i_ > 0; this->i_--)
    {
      if (this->aa_.k_array_[this->i_-1] != 0)
        break;
    }

  return (this->i_-- > 0);
}

template <class KEY, class DATA> int
JAWS_Assoc_Array_Iterator<KEY,DATA>::next (void)
{
  if (this->j_ < this->aa_.maxsize_)
    {
      this->i_ = this->j_;
      for (this->j_++; this->j_ < this->aa_.maxsize_; this->j_++)
        {
          if (this->aa_.k_array_[this->j_] != 0)
            break;
        }
    }

  return (this->i_ < this->aa_.maxsize_);
}

template <class KEY, class DATA> int
JAWS_Assoc_Array_Iterator<KEY,DATA>::previous (void)
{
  if (this->i_ > 0)
    {
      for (this->j_ = this->i_; this->i_ > 0; this->i_--)
        {
          if (this->aa_.k_array_[this->i_-1] != 0)
            break;
        }
    }

  if (this->i_ == 0)
    this->first ();
  else
    this->i_--;

  return 1;
}

template <class KEY, class DATA> int
JAWS_Assoc_Array_Iterator<KEY,DATA>::is_done (void)
{
  return (this->j_ == this->aa_.maxsize_);
}

template <class KEY, class DATA>
JAWS_Assoc_Array_Iterator<KEY,DATA>::
JAWS_Assoc_Array_Iterator (const JAWS_Assoc_Array_Iterator<KEY, DATA> &aai)
  : aa_ (aai.aa_),
    i_ (aai.i_),
    j_ (aai.j_)
{
}

template <class KEY, class DATA> void
JAWS_Assoc_Array_Iterator<KEY,DATA>::
operator= (const JAWS_Assoc_Array_Iterator<KEY, DATA> &)
{
}

#endif /* !defined (JAWS_ASSOC_ARRAY_CPP) */
