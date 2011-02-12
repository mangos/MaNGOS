// $Id: HT_Server.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "HT_Server.h"
#include "ace/ACE.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_Memory.h"

// Insert a KEY_NAME into the hash table, if it doesn't already exist
// there.  What gets returned is a pointer to the node inserted.  Note
// that we do our own memory allocation here...

Protocol_Record *
HT_Server::insert (const char *key_name, int max_len)
{
  Protocol_Record **prpp = 0;

  // This is tricky...

  for (prpp = &this->hash_table[ACE::hash_pjw (key_name) % this->hash_table_size];
       *prpp != 0 && ACE_OS::strncmp ((*prpp)->get_login (), key_name, max_len) != 0;
       prpp = &(*prpp)->next_)
    continue;

  if (*prpp == 0)
    {
      // Remember, the server must be very careful about stuff it
      // receives from the rwho manager, since it may not be
      // NUL-terminated.  That's why we use ACE::strnnew ()...

      ACE_NEW_RETURN (*prpp,
                      Protocol_Record (ACE::strnnew (key_name,
                                                     max_len),
                                       *prpp),
                      0);
      this->count_++;
    }

  return *prpp;
}
