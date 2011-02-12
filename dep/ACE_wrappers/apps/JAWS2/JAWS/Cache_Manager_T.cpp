// $Id: Cache_Manager_T.cpp 91626 2010-09-07 10:59:20Z johnnyw $

#ifndef JAWS_CACHE_MANAGER_T_CPP
#define JAWS_CACHE_MANAGER_T_CPP

#include "JAWS/Cache_Manager_T.h"
#include "JAWS/Cache_Hash_T.h"
#include "JAWS/Cache_List_T.h"

// FUZZ: disable check_for_streams_include
#include "ace/streams.h"

class Cache_Manager;

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC>
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::JAWS_Cache_Manager (ACE_Allocator *alloc,
                     JAWS_Cache_Object_Factory *cof,
                     size_t hashsize,
                     size_t maxsize,
                     size_t maxobjsize,
                     size_t minobjsize,
                     size_t highwater,
                     size_t lowwater,
                     int timetolive,
                     int counted)
  : allocator_ (alloc),
    factory_ (cof),
    hashsize_ (hashsize),
    maxsize_ (maxsize),
    maxobjsize_ (maxobjsize),
    minobjsize_ (minobjsize),
    highwater_ (highwater),
    lowwater_ (lowwater),
    waterlevel_ (0),
    timetolive_ (timetolive),
    counted_ (counted),
    hash_ (0),
    heap_ (0)
{
  // Some sanity checking needed here --
  if (this->lowwater_ > this->highwater_)
    this->lowwater_ = this->highwater_ / 2;

  if (this->maxobjsize_ > (this->highwater_ - this->lowwater_) * 1024)
    this->maxobjsize_ = (this->highwater_ - this->lowwater_) * (1024/2);

  if (this->minobjsize_ > this->maxobjsize_)
    this->minobjsize_ = this->maxobjsize_ / 2;

  if (this->allocator_ == 0)
    this->allocator_ = ACE_Allocator::instance ();

  if (this->factory_ == 0)
    this->factory_ = Object_Factory::instance ();

  ACE_NEW_MALLOC (this->hash_,
                  (Cache_Hash *)
                  this->allocator_->malloc (sizeof (Cache_Hash)),
                  Cache_Hash (alloc, hashsize));

  if (this->hash_ == 0)
    {
      this->hashsize_ = 0;
      return;
    }

  ACE_NEW_MALLOC (this->heap_,
                  (Cache_Heap *)
                  this->allocator_->malloc (sizeof (Cache_Heap)),
                  Cache_Heap (alloc, maxsize));

  if (this->heap_ == 0)
    {
      this->maxsize_ = 0;


      ACE_DES_FREE_TEMPLATE3(this->hash_, this->allocator_->free,
                             JAWS_Cache_Hash,
                             KEY, HASH_FUNC, EQ_FUNC);



      this->hash_ = 0;
      this->hashsize_ = 0;
    }
}


template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::open (ACE_Allocator *alloc,
        JAWS_Cache_Object_Factory *cof,
        size_t hashsize,
        size_t maxsize,
        size_t maxobjsize,
        size_t minobjsize,
        size_t highwater,
        size_t lowwater,
        int timetolive,
        int counted)
{
  this->close ();

  this->allocator_ = alloc;
  this->factory_ = cof;
  this->hashsize_ = hashsize;
  this->maxsize_ = maxsize;
  this->maxobjsize_ = maxobjsize;
  this->minobjsize_ = minobjsize;
  this->highwater_ = highwater;
  this->lowwater_ = lowwater;
  this->waterlevel_ = 0;
  this->timetolive_ = timetolive;
  this->counted_ = counted;

  // Some sanity checking needed here --
  if (this->lowwater_ > this->highwater_)
    this->lowwater_ = this->highwater_ / 2;

  if (this->maxobjsize_ > (this->highwater_ - this->lowwater_) * 1024)
    this->maxobjsize_ = (this->highwater_ - this->lowwater_) * (1024/2);

  if (this->minobjsize_ > this->maxobjsize_)
    this->minobjsize_ = this->maxobjsize_ / 2;

  if (this->allocator_ == 0)
    this->allocator_ = ACE_Allocator::instance ();

  if (this->factory_ == 0)
    this->factory_ = Object_Factory::instance ();

  this->hash_ = (Cache_Hash *) this->allocator_->malloc (sizeof (Cache_Hash));
  if (this->hash_ == 0)
    {
      errno = ENOMEM;
      this->hashsize_ = 0;

      return -1;
    }
  new (this->hash_) Cache_Hash (alloc, hashsize);

  this->heap_ = (Cache_Heap *) this->allocator_->malloc (sizeof (Cache_Heap));
  if (this->heap_ == 0)
    {
      errno = ENOMEM;
      this->maxsize_ = 0;


      ACE_DES_FREE_TEMPLATE3(this->hash_, this->allocator_->free,
                             JAWS_Cache_Hash,
                             KEY, HASH_FUNC, EQ_FUNC);



      this->hash_ = 0;
      this->hashsize_ = 0;

      return -1;
    }
  new (this->heap_) Cache_Heap (alloc, maxsize);

  return 0;
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC>
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>::~JAWS_Cache_Manager (void)
{
  this->close ();
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>::close (void)
{
  while (this->waterlevel_ > 0)
    this->FLUSH_i ();

  if (this->hash_)
    {

      ACE_DES_FREE_TEMPLATE3(this->hash_, this->allocator_->free,
                             JAWS_Cache_Hash,
                             KEY, HASH_FUNC, EQ_FUNC);



      this->hash_ = 0;
    }

  if (this->heap_)
    {

      ACE_DES_FREE_TEMPLATE4(this->heap_, this->allocator_->free,
                             JAWS_Cache_List,
                             KEY, FACTORY, HASH_FUNC, EQ_FUNC);



      this->heap_ = 0;
    }

  return 0;
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::GET_i (const KEY &key, JAWS_Cache_Object *&object)
{
  int const result = this->hash_->find (key, object);

  if (result == 0)
    this->TAKE (object);
  else
    object = 0;

  return result;
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::PUT_i (const KEY &key, const void *data, size_t size, JAWS_Cache_Object *&obj)
{
  int result = 0;

  if (data == 0)
    {
      this->FLUSH_i (key);
      obj = 0;
      return 0;
    }

  result = this->MAKE (data, size, obj);
  if (result == -1)
    {
      if (size/1024 <= this->maxobjsize_)
        cerr << "MAKE failed.  Bummer!" << endl;
      else
        this->DROP_i (obj);
      return -1;
    }

  obj->internal (new KEY (key));

  KEY old_key;
  JAWS_Cache_Object *old_obj;

  result = this->hash_->rebind (key, obj, old_key, old_obj);
  if (result == -1)
    {
      cerr << "*** hash bind error: " << key << endl;
      obj->release ();
      this->DROP_i (obj);
      return -1;
    }
  else if (result == 1)
    {
      this->heap_->remove (old_obj->heap_item ());
      this->waterlevel_ -= old_obj->size ();
      old_obj->release ();
      this->DROP_i (old_obj);
    }

  result = this->heap_->insert (key, obj);
  if (result == -1)
    {
      cerr << "*** heap insertion error: " << key << endl;
      this->hash_->unbind (key);
      obj->release ();
      this->DROP_i (obj);
      return -1;
    }

  this->waterlevel_ += size;

  // Acquire this one for the putter.
  this->TAKE (obj);

  return 0;
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::FLUSH_i (const KEY &key)
{
  JAWS_Cache_Object *temp_object;

#ifdef ENTERA_VERBOSE_TRACE
  cerr << "*** flush key unbinding: " << key << endl;
#endif
  int result = this->hash_->unbind (key, temp_object);
  if (result == 0)
    {
      this->waterlevel_ -= temp_object->size ();
      if (this->heap_->remove (temp_object->heap_item ()) == -1)
        cerr << "*** flush key heap remove failed: " << endl;
      temp_object->release ();
      this->DROP_i (temp_object);
    }
  else
    cerr << "*** flush key hash unbind failed: " << key << endl;

  return result;
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::FLUSH_i (void)
{
  KEY temp_key;
  JAWS_Cache_Object *temp_object;

  int result = this->heap_->remove (temp_key, temp_object);
  if (result == 0)
    {
#ifdef ENTERA_VERBOSE_TRACE
      cerr << "*** flush unbinding: " << temp_key << endl;
#endif
      result = this->hash_->unbind (temp_key);
      if (result == -1)
        cerr << "*** flush hash unbind failed: " << temp_key << endl;
      result = 0;
      this->waterlevel_ -= temp_object->size ();
      temp_object->release ();
      this->DROP_i (temp_object);
    }
  else
    {
      cerr << "*** flush heap remove failed" << endl;
    }

  return result;
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::DROP_i (JAWS_Cache_Object *&obj)
{
  int result = 0;

  if (obj->count () == 0)
    {
      KEY *key = (KEY *) obj->internal ();
      this->factory_->destroy (obj);
      delete key;
      obj = 0;
      result = 1;
    }
  else
    result = this->heap_->adjust (obj->heap_item ());

  return result;
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::GET (const KEY &key, JAWS_Cache_Object *&object)
{
  ACE_READ_GUARD_RETURN (ACE_SYNCH_RW_MUTEX, g,this->lock_, -1);

  return this->GET_i (key, object);
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::PUT (const KEY &key, const void *data, size_t size, JAWS_Cache_Object *&obj)
{
  ACE_WRITE_GUARD_RETURN (ACE_SYNCH_RW_MUTEX, g,this->lock_, -1);

  return this->PUT_i (key, data, size, obj);
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::MAKE (const void *data, size_t size, JAWS_Cache_Object *&obj)
{
  // verify object is within cacheable range
  if (size/1024 > this->maxobjsize_)
    {
#if 0
      // What we do is cache it anyway, but remove it as soon as the
      // requester returns it.
      obj = this->factory_->create (data, size);
      return 0;
#else
      // The above is a little tricky to implement.  Think about it
      // some more.
      obj = this->factory_->create (data, size);
      return -1;

#endif /* 0 */
    }

  if (size/1024 < this->minobjsize_)

    {
      // Don't bother to cache this.
      cerr << "*** " << static_cast<unsigned int>(size) << " is too small to cache" << endl;
      return -1;
    }

  // make sure we have sufficient memory
  if (this->waterlevel_ + size > this->highwater_ * (1024 * 1024))
    {
      do
        {
          if (this->FLUSH_i () == -1)
            {
              cerr << "*** cache flooded, flush error" << endl;
              return -1;
            }
        }
      while (this->waterlevel_ > this->lowwater_ * (1024 * 1024));
    }

  // make sure heap has enough room
  if (this->heap_->is_full ())
    {
      cerr << "*** heap full, flushing" << endl;
      if (this->FLUSH_i () == -1)
        {
          cerr << "*** heap full, flush error" << endl;
          return -1;
        }
    }

  obj = this->factory_->create (data, size);
  if (this->TAKE (obj) == -1)
    {
      cerr << "*** take error" << endl;
      this->factory_->destroy (obj);
      obj = 0;
      return -1;
    }

  return 0;
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::TAKE (JAWS_Cache_Object *const &obj)
{
  if (obj == 0)
    return -1;

  return obj->acquire ();
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::DROP (JAWS_Cache_Object *&obj)
{
  if (obj == 0)
    return -1;

  ACE_WRITE_GUARD_RETURN (ACE_SYNCH_RW_MUTEX, g, this->lock_, -1);

  int result = obj->release ();

  if (result == 0)
    {
      if (obj->count () == 0)
        {
          KEY *key = (KEY *) obj->internal ();
          this->factory_->destroy (obj);
          delete key;
          obj = 0;
          result = 1;
        }
      else
        {
          result = this->DROP_i (obj);
        }
    }

  return result;
}

template <class KEY, class FACTORY, class HASH_FUNC, class EQ_FUNC> int
JAWS_Cache_Manager<KEY,FACTORY,HASH_FUNC,EQ_FUNC>
::FLUSH (void)
{
  ACE_WRITE_GUARD_RETURN (ACE_SYNCH_RW_MUTEX, g, this->lock_, -1);

  return this->FLUSH_i ();
}


template <class KEY, class DATA, class CACHE_MANAGER>
JAWS_Cache_Proxy<KEY, DATA, CACHE_MANAGER>
::JAWS_Cache_Proxy (const KEY &key, Cache_Manager *manager)
  : object_ (0),
    manager_ (manager)
{
  if (this->manager_ == 0)
    this->manager_ = Cache_Manager_Singleton::instance ();

  int const result = this->manager_->GET (key, this->object_);
  if (result == -1)
    this->object_ = 0;
}

template <class KEY, class DATA, class CACHE_MANAGER>
JAWS_Cache_Proxy<KEY, DATA, CACHE_MANAGER>
::JAWS_Cache_Proxy (const KEY &key, DATA *data, size_t size,
                   Cache_Manager *manager)
  : object_ (0),
    manager_ (manager)
{
  if (this->manager_ == 0)
    this->manager_ = Cache_Manager_Singleton::instance ();

  int result = this->manager_->PUT (key, data, size, this->object_);
  if (result == -1)
    this->object_ = 0;
}

template <class KEY, class DATA, class CACHE_MANAGER>
JAWS_Cache_Proxy<KEY, DATA, CACHE_MANAGER>::~JAWS_Cache_Proxy (void)
{
  DATA *data = this->data ();
  this->manager_->DROP (this->object_);
  if (this->object_ == 0)
    this->close (data);
}

template <class KEY, class DATA, class CACHE_MANAGER> DATA *
JAWS_Cache_Proxy<KEY, DATA, CACHE_MANAGER>::data (void) const
{
  return this->object_ ? (DATA *) this->object_->data () : 0;
}

template <class KEY, class DATA, class CACHE_MANAGER>
JAWS_Cache_Proxy<KEY, DATA, CACHE_MANAGER>::operator DATA * (void) const
{
  return this->data ();
}

template <class KEY, class DATA, class CACHE_MANAGER> int
JAWS_Cache_Proxy<KEY, DATA, CACHE_MANAGER>::close (DATA *)
{
  return 0;
}


#endif /* JAWS_CACHE_MANAGER_T_CPP */
