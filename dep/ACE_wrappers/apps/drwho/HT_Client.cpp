// $Id: HT_Client.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "HT_Client.h"
#include "ace/ACE.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_Memory.h"

// Insert a KEY_NAME into the hash table, if it doesn't already exist
// there.  What gets returned is a pointer to the node inserted.  Note
// that we do our own memory allocation here...

Protocol_Record *
HT_Client::insert (const char *key_name, int max_len)
{
  Protocol_Record **prpp = 0;

  // This is tricky...

  for (prpp = &this->hash_table[ACE::hash_pjw (key_name) % this->hash_table_size];
       *prpp != 0
         && ACE_OS::strncmp ((*prpp)->get_login (),
                             key_name, max_len) != 0;
       prpp = &(*prpp)->next_)
    continue;

  if (*prpp == 0)
    {
      ACE_NEW_RETURN (*prpp,
                      Protocol_Record (ACE::strnew (key_name),
                                       *prpp),
                      0);
      this->count_++;
    }

  return *prpp;
}
