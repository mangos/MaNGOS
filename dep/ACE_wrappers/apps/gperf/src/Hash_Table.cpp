// -*- C++ -*-

/**
 * $Id: Hash_Table.cpp 91671 2010-09-08 18:39:23Z johnnyw $
 *
 * Copyright (C) 1989 Free Software Foundation, Inc.
 * written by Douglas C. Schmidt (schmidt@cs.wustl.edu)
 *
 * This file is part of GNU GPERF.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "Hash_Table.h"



#if defined (ACE_HAS_GPERF)

#include "ace/ACE.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_Memory.h"

// The size of the hash table is always the smallest power of 2 >= the
// size indicated by the user.  This allows several optimizations,
// including the use of double hashing and elimination of the mod
// instruction.  Note that the size had better be larger than the
// number of items in the hash table, else there's trouble!!!

Hash_Table::Hash_Table (size_t s)
  : size_ (ACE_POW (s)),
    collisions_ (0)
{
  if (this->size_ == 0)
    this->size_ = 1;
  ACE_NEW (this->table_,
           List_Node*[this->size_]);
  ACE_OS::memset ((char *) this->table_,
                  0,
                  this->size_ * sizeof *this->table_);
}

Hash_Table::~Hash_Table (void)
{
  if (option[DEBUGGING])
    {
      size_t keysig_width = option.max_keysig_size () > ACE_OS::strlen ("keysig")
        ? option.max_keysig_size ()
        : ACE_OS::strlen ("keysig");

      ACE_DEBUG ((LM_DEBUG,
                  "\ndumping the hash table\ntotal available table slots = %d, total bytes = %d, total collisions = %d\n"
                  "location, %*s, keyword\n",
                  this->size_,
                  this->size_ * (int) sizeof *this->table_,
                  this->collisions_,
                  keysig_width,
                  "keysig"));

      for (int i = static_cast<int> (this->size_ - 1); i >= 0; i--)
        if (this->table_[i])
          ACE_DEBUG ((LM_DEBUG,
                      "%8d, %*s, %s\n",
                      i,
                      keysig_width,
                      this->table_[i]->keysig,
                      this->table_[i]->key));
      ACE_DEBUG ((LM_DEBUG,
                  "end dumping hash table\n\n"));
    }

  delete [] this->table_;
}

// If the ITEM is already in the hash table return the item found in
// the table.  Otherwise inserts the ITEM, and returns FALSE.  Uses
// double hashing.

List_Node *
Hash_Table::find (List_Node *item,
                  int ignore_length)
{
  size_t hash_val = ACE::hash_pjw (item->keysig);
  // The following works since the hash table size_ is always a power
  // of 2...
  size_t size = this->size_ - 1;
  size_t probe;
  size_t increment = ((hash_val ^ (ignore_length == 0 ? item->length : 0)) | 1) & size;

  for (probe = hash_val & size;
       this->table_[probe]
         && (ACE_OS::strcmp (this->table_[probe]->keysig, item->keysig) != 0
             || (ignore_length == 0 && this->table_[probe]->length != item->length));
       probe = (probe + increment) & size)
    {
      ++this->collisions_;
    }

  if (this->table_[probe])
    {
      return this->table_[probe];
    }
  else
    {
      this->table_[probe] = item;
      return 0;
    }
}

#endif /* ACE_HAS_GPERF */
