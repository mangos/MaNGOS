// $Id: Sig_List.cpp 91670 2010-09-08 18:02:26Z johnnyw $

// File: Sig_List.cpp

// Author: Phil Mesnier

#include "ace/OS_NS_string.h"
#include "Sig_List.h"

//-----------------------------------------------------------------------------

Sig_List::Sig_List (int cap)
  : size_(0),
    capacity_(cap),
    index_(0),
    has_nulls_(0),
    modified_(0),
    array_(0)
{
  array_ = new Signature*[capacity_];
}

Sig_List::~Sig_List ()
{
  for (int i = 0; i < size_; i++)
    if (array_[i]) array_[i]->release();
  delete [] array_;
}

void
Sig_List::add (const ACE_CString &s)
{
  if (this->index_of (s) != -1)
    return;
  modified_ = 1;
  if (has_nulls_)
    for (int i = 0; i < size_; i++)
      if (array_[i] == 0) {
        array_[i] = new Signature (s);
        has_nulls_ --;
        return;
      }
  if (size_ == capacity_) {
    int ncap = capacity_ * 2;
    Signature ** narray = new Signature *[ncap];
    ACE_OS::memcpy (narray,array_,capacity_ * sizeof(Signature*));
    delete [] array_;
    array_ = narray;
    capacity_ = ncap;
  }
  array_[size_++] = new Signature(s);
}

void
Sig_List::add (const Sig_List &other)
{
  if (capacity_ < size_ + other.capacity_) {
    int ncap = size_ + other.capacity_ + 50;
    Signature ** narray = new Signature *[ncap];
    ACE_OS::memcpy (narray,array_,capacity_ * sizeof(Signature*));
    delete [] array_;
    array_ = narray;
    capacity_ = ncap;
  }
  modified_ = 1;
  for (int i = 0; i < other.size_; i++)
    if (other.array_[i] != 0 &&
        this->index_of (other.array_[i]->name()) == -1)
      {
        if (!has_nulls_)
          array_[size_++] = other.array_[i]->dup();
        else
          for (int i = 0; i < size_; i++)
            if (array_[i] == 0)
              {
                array_[i] = other.array_[i]->dup();
                has_nulls_ --;
                break;
              }
      }
}

void
Sig_List::remove (const Signature &s)
{
  for (int i = 0; i < size_; i++)
    if (array_[i] && array_[i]->name() == s.name()) {
      array_[i]->release();
      array_[i] = 0;
      modified_ = 1;
      if (i == size_ - 1)
        size_ --;
      else
        has_nulls_ ++;
      break;
    }
}

void
Sig_List::remove_current ()
{
  array_[index_]->release();
  array_[index_] = 0;
  modified_ = 1;
  if (index_ == size_ - 1)
    size_--;
  else
    has_nulls_++;
}

int
Sig_List::index_of (const Signature *s)
{
  for (int i = 0; i < size_; i++)
    if (array_[i] && array_[i]->name() == s->name()) {
      array_[i]->used();
      return i;
    }
  return -1;
}

int
Sig_List::index_of (const ACE_CString &s)
{
  for (int i = 0; i < size_; i++)
    if (array_[i] && array_[i]->name() == s) {
      return i;
    }
  return -1;
}


const Signature *
Sig_List::first()
{
  for (index_ = 0; index_ < size_; index_++)
    if (array_[index_] != 0)
      return array_[index_];
  return 0;
}

const Signature *
Sig_List::next()
{
  for (++index_; index_ < size_; index_++)
    if (array_[index_] != 0)
      return array_[index_];
  return 0;
}

int
Sig_List::hasmore ()
{
  return index_ < size_;
}

int
Sig_List::size()
{
  return size_;
}

int
Sig_List::modified()
{
  int rtn = modified_;
  modified_ = 0;
  int insert = 0;
  if (has_nulls_) {
    for (int i = 0; i < size_; i++)
      if (array_[i] != 0) {
        if (i != insert) {
          array_[insert] = array_[i];
          array_[i] = 0;
        }
        insert++;
      }
    size_ = insert+1;
    has_nulls_ = 0;
  }
  return rtn;
}
