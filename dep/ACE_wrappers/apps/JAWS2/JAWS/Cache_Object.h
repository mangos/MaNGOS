// -*- C++ -*-

// $Id: Cache_Object.h 80826 2008-03-04 14:51:23Z wotte $


#ifndef JAWS_CACHE_OBJECT_H
#define JAWS_CACHE_OBJECT_H

#include "ace/Lock_Adapter_T.h"
#include "ace/Synch_Traits.h"
#include "ace/Thread_Mutex.h"
#include "ace/Malloc.h"
#include "ace/RW_Thread_Mutex.h"


ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Allocator;
ACE_END_VERSIONED_NAMESPACE_DECL


// Cache bucket -- use Hash_Bucket to hold cacheable objects.

class JAWS_Cache_Object
{
public:
  JAWS_Cache_Object (const void *, size_t);
  virtual ~JAWS_Cache_Object (void);

  void *internal (void) const;
  void internal (void *);

  const void *data (void) const;
  size_t size (void) const;
  unsigned int count (void) const;

  int acquire (void);
  int release (void);

  time_t last_access (void) const;
  time_t first_access (void) const;

  unsigned int priority (void) const;

  enum { ACE_CO_REFERENCED, ACE_CO_COUNTED };

  void *heap_item (void) const;
  void heap_item (void *item);

  virtual ACE_Lock & lock (void) = 0;

protected:

  virtual unsigned int count_i (void) const = 0;
  virtual int acquire_i (void) = 0;
  virtual int release_i (void) = 0;
  virtual unsigned int priority_i (void) const = 0;

private:

  void *internal_;
  const void *data_;
  size_t size_;

  time_t last_access_;
  time_t first_access_;
  time_t new_last_access_;

  void *heap_item_;

};

class JAWS_Referenced_Cache_Object : public JAWS_Cache_Object
{
public:
  JAWS_Referenced_Cache_Object (const void *, size_t);
  virtual ~JAWS_Referenced_Cache_Object (void);

  virtual ACE_Lock & lock (void);

protected:

  virtual unsigned int count_i (void) const;
  virtual int acquire_i (void);
  virtual int release_i (void);
  virtual unsigned int priority_i (void) const;

private:

  mutable ACE_SYNCH_RW_MUTEX count_;
  mutable ACE_Lock_Adapter<ACE_SYNCH_RW_MUTEX> lock_adapter_;

};

class JAWS_Counted_Cache_Object : public JAWS_Cache_Object
{
public:
  JAWS_Counted_Cache_Object (const void *, size_t);
  virtual ~JAWS_Counted_Cache_Object (void);

  virtual ACE_Lock & lock (void);

protected:

  virtual unsigned int count_i (void) const;
  virtual int acquire_i (void);
  virtual int release_i (void);
  virtual unsigned int priority_i (void) const;

private:

  unsigned int count_;
  unsigned int new_count_;
  mutable ACE_SYNCH_MUTEX lock_;
  mutable ACE_Lock_Adapter<ACE_SYNCH_MUTEX> lock_adapter_;

};

class JAWS_Cache_Object_Factory
{
public:

  JAWS_Cache_Object_Factory (ACE_Allocator *alloc = 0);
  virtual ~JAWS_Cache_Object_Factory (void);

  int open (ACE_Allocator *alloc = 0);

  virtual JAWS_Cache_Object * create (const void *, size_t) = 0;
  virtual void destroy (JAWS_Cache_Object *) = 0;

protected:

  ACE_Allocator *allocator_;

};

class JAWS_Referenced_Cache_Object_Factory : public JAWS_Cache_Object_Factory
{
public:
  JAWS_Referenced_Cache_Object_Factory (ACE_Allocator *alloc = 0);
  virtual ~JAWS_Referenced_Cache_Object_Factory (void);

  virtual JAWS_Cache_Object * create (const void *, size_t);
  virtual void destroy (JAWS_Cache_Object *);

};

class JAWS_Counted_Cache_Object_Factory : public JAWS_Cache_Object_Factory
{
public:
  JAWS_Counted_Cache_Object_Factory (ACE_Allocator *alloc = 0);
  virtual ~JAWS_Counted_Cache_Object_Factory (void);

  virtual JAWS_Cache_Object * create (const void *, size_t);
  virtual void destroy (JAWS_Cache_Object *);

};

#endif /* JAWS_CACHE_OBJECT_H */
