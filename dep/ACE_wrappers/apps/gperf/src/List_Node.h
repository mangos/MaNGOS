// -*- C++ -*-

/**
 * $Id: List_Node.h 86019 2009-07-14 12:13:09Z wotte $
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

#ifndef LIST_NODE_H
#define LIST_NODE_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Options.h"
#include "ace/Copy_Disabled.h"

#if defined (ACE_HAS_GPERF)

/**
 * Data and function members for defining values and operations of
 * a list node.
 */
class List_Node : private ACE_Copy_Disabled
{
public:
  /// Constructor.
  List_Node (char *key, int len);

  /// Destructor.
  ~List_Node (void);

  static void sort (char *base, int len);

  /// TRUE if key has an identical KEY_SET as another key.
  List_Node *link;

  /// Points to next element on the list.
  List_Node *next;

  /// Each keyword string stored here.
  char *key;

  /// Additional information for building hash function.
  char *rest;

  /// Set of characters to hash, specified by user.
  char *keysig;

  /// Length of the key.
  int length;

  /// Hash value for the key.
  int hash_value;

  /// A metric for frequency of key set occurrences.
  int occurrence;

  /// Position of this node relative to other nodes.
  int slot;
};

#endif /* ACE_HAS_GPERF */
#endif /* LIST_NODE_H */
