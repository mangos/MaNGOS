/* -*- c++ -*- */
// $Id: Filecache.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_FILECACHE_H
#define JAWS_FILECACHE_H

#include "ace/FILE_IO.h"

#include "JAWS/Cache_Manager.h"
#include "JAWS/FILE.h"
#include "JAWS/Export.h"

class JAWS_Export JAWS_Referenced_Filecache_Factory
  : public JAWS_Referenced_Cache_Object_Factory
{
public:

  virtual void destroy (JAWS_Cache_Object *);

};

class JAWS_Export JAWS_Counted_Filecache_Factory
  : public JAWS_Counted_Cache_Object_Factory
{
public:

  virtual void destroy (JAWS_Cache_Object *);

};

typedef JAWS_Cache_Manager<JAWS_Strdup_String,
                           JAWS_Referenced_Filecache_Factory,
                           JAWS_String_Hash_Functor,
                           JAWS_String_Equal_Functor>
        JAWS_Referenced_Filecache_Manager;

typedef JAWS_Cache_Manager<JAWS_Strdup_String,
                           JAWS_Counted_Filecache_Factory,
                           JAWS_String_Hash_Functor,
                           JAWS_String_Equal_Functor>
        JAWS_Counted_Filecache_Manager;

typedef JAWS_Counted_Filecache_Manager JAWS_Filecache_Manager;

typedef JAWS_Cache_Proxy<const char *,
                         JAWS_FILE, JAWS_Filecache_Manager>
        JAWS_Filecache_Proxy;

class JAWS_Export JAWS_Cached_FILE : private JAWS_Filecache_Proxy
{
public:

  JAWS_Cached_FILE (const char *const &filename,
                    JAWS_Filecache_Proxy::Cache_Manager *cm = 0);
  JAWS_Cached_FILE (const char *const &filename,
                    JAWS_FILE *&file,
                    size_t size,
                    JAWS_Filecache_Proxy::Cache_Manager *cm = 0);

  ~JAWS_Cached_FILE (void);

  ACE_FILE_IO * file (void);
  ACE_Mem_Map * mmap (void);

private:

  ACE_FILE_IO file_;

};


#endif /* JAWS_FILECACHE_H */
