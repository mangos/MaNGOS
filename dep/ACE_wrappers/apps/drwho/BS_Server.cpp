// $Id: BS_Server.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "BS_Server.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_Memory.h"
#include "ace/os_include/os_netdb.h"

// This constructor takes a message of sorted login names and loads up
// the symbol table on the server's side.  It assumes that the number
// of friends is stored in the first MAXUSERIDNAMELEN bytes of the
// packet.  Note that we assume that the client sends the login names
// in sorted order, so we don't bother sorting them!

BS_Server::BS_Server (const char *packet)
{
  const char *buf_ptr = packet + MAXUSERIDNAMELEN;

  this->count_ = ACE_OS::atoi (packet);
  this->buffer_ = buf_ptr;

  ACE_NEW (this->protocol_record_,
           Protocol_Record[this->count_]);
  ACE_NEW (this->sorted_record_,
           Protocol_Record *[this->count_]);

  for (int i = 0; i < this->count_; i++)
    {
      Protocol_Record *rec_ptr = &this->protocol_record_[i];

      this->sorted_record_[i] = rec_ptr;
      rec_ptr->key_name1_ = buf_ptr;

      // Skip forward to the start of the next login name.

      while (*buf_ptr++ != '\0')
        continue;
    }

}

// Insert the HOST_NAME into the appropriate DRWHO_LIST slot if the
// KEY_NAME happens to be one of our friends.  Binary search is used
// because the Protocol_Manager keeps a sorted representation of the
// friend names.
//
// Note that this binary search is tuned for unsuccessful searches,
// since most of the time we the KEY_NAME is *not* a friend (unless
// you've got *lots* of friends)!
//
// Note finally that we keep a cache of the last KEY_NAME that is
// looked up, as well as the result of the lookup.  This speeds things
// up because the whod files tend to cluster userids together. */

Protocol_Record *
BS_Server::insert (const char *key_name, int max_len)
{
  static char last_lookup[MAXHOSTNAMELEN];
  static int mid = 0;
  static int result = 0;
  Protocol_Record **buffer = this->sorted_record_;

  // First check the cache...
  if (ACE_OS::strncmp (last_lookup, key_name, max_len) == 0)
    {
      if (result == 0)
        return 0;
    }
  else
    {
      // Store this away in the cache for the next iteration.
      ACE_OS::strncpy (last_lookup, key_name, max_len);

      int hi = this->count_ - 1;
      int lo = 0;
      int cmp = 0;

      while (lo < hi)
        {
          mid = (hi + lo + 1) / 2;

          cmp = ACE_OS::strncmp (key_name,
                                 buffer[mid]->get_login (),
                                 max_len);
          if (cmp < 0)
            hi = mid - 1;
          else
            lo = mid;
        }

      // This line is very subtle... ;-)
      if (!(cmp == 0
            || ACE_OS::strncmp (key_name, buffer[--mid]->get_login (), max_len) == 0))
        {
          result = 0;
          return 0;
        }
    }

  // If we get here we've located a friend.

  result = 1;
  return buffer[mid];
}

// Returns the next friend in the sequence of sorted friends.  Skips
// over the entries that don't have any hosts associated with them
// (because these entries weren't on the server machine. */

Protocol_Record *
BS_Server::get_next_entry (void)
{
  for (Protocol_Record *prp = Binary_Search::get_next_entry ();
       prp != 0;
       prp = Binary_Search::get_next_entry ())
    if (prp->get_drwho_list () != 0)
      return prp;

  return 0;
}
