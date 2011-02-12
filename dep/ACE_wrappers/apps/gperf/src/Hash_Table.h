// -*- C++ -*-

/**
 * $Id: Hash_Table.h 86019 2009-07-14 12:13:09Z wotte $
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

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Options.h"
#include "List_Node.h"
#include "ace/Copy_Disabled.h"

#if defined (ACE_HAS_GPERF)

/**
 * Hash table used to check for duplicate keyword entries.
 *
 * This implementation uses "double hashing."
 *
 * @todo This class should be replaced by something in ACE, e.g.,
 * ACE_Hash_Map_Manager.  Perhaps we should implement a new
 * ACE_Hash_Map that uses double hashing, however!
 */
class Hash_Table : private ACE_Copy_Disabled
{
public:
  /// Constructor
  Hash_Table (size_t s);

  /// Destructor
  ~Hash_Table (void);

  List_Node *find (List_Node *item, int ignore_length);

private:
  /// Vector of pointers to linked lists of List_Node's.
  List_Node **table_;

  /// Size of the vector.
  size_t size_;

  /// Find out how well our double hashing is working!
  int collisions_;
};

#endif /* ACE_HAS_GPERF */
#endif /* HASH_TABLE_H */
