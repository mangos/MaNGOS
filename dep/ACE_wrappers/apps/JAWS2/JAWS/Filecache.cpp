// $Id: Filecache.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "ace/FILE_Connector.h"
#include "ace/OS_NS_unistd.h"

#include "JAWS/Filecache.h"
#include "JAWS/Cache_List_T.h"

void
JAWS_Referenced_Filecache_Factory::destroy (JAWS_Cache_Object *object)
{
  JAWS_FILE *file = (JAWS_FILE *) object->data ();
  file->close ();
  if (file->map ())
    file->map ()->close ();
  delete file;
  this->JAWS_Referenced_Cache_Object_Factory::destroy (object);
}

void
JAWS_Counted_Filecache_Factory::destroy (JAWS_Cache_Object *object)
{
  JAWS_FILE *file = (JAWS_FILE *) object->data ();
  file->close ();
  if (file->map ())
    file->map ()->close ();
  delete file;
  this->JAWS_Counted_Cache_Object_Factory::destroy (object);
}

JAWS_Cached_FILE::JAWS_Cached_FILE (const char *const &filename,
                                    JAWS_Filecache_Proxy::Cache_Manager *cm)
  : JAWS_Filecache_Proxy (filename, cm)
{
  ACE_HANDLE handle = ACE_INVALID_HANDLE;

  if (this->data () != 0)
    {
      handle = ACE_OS::dup (this->data ()->get_handle ());
    }
  else
    {
      JAWS_FILE *file = new JAWS_FILE;
      ACE_FILE_Connector file_connector;

      int result = file_connector.connect (*file, ACE_FILE_Addr (filename));
      if (result == -1 || file->get_handle () == ACE_INVALID_HANDLE)
        {
          // TODO: do something here!
        }

      ACE_FILE_Info info;
      file->get_info (info);

      handle = ACE_OS::dup (file->get_handle ());

      {
        JAWS_Cached_FILE cf (filename, file, info.size_, cm);
        if (cf.data () != 0)
          {
            new (this) JAWS_Cached_FILE (filename, cm);
            return;
          }
      }
    }

  this->file_.set_handle (handle);
}

JAWS_Cached_FILE::JAWS_Cached_FILE (const char *const &filename,
                                    JAWS_FILE *&file,
                                    size_t size,
                                    JAWS_Filecache_Proxy::Cache_Manager *cm)
  : JAWS_Filecache_Proxy (filename, file, size, cm)
{
}

JAWS_Cached_FILE::~JAWS_Cached_FILE (void)
{
  this->file_.close ();
}

ACE_FILE_IO *
JAWS_Cached_FILE::file (void)
{
  return &(this->file_);
}

ACE_Mem_Map *
JAWS_Cached_FILE::mmap (void)
{
  return (this->data () == 0 ? 0 : this->data ()->mem_map ());
}

