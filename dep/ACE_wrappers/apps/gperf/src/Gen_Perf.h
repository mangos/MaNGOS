// -*- C++ -*-

/**
 * $Id: Gen_Perf.h 91273 2010-08-04 16:24:59Z johnnyw $
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

#ifndef GEN_PERF_H
#define GEN_PERF_H

#include "Options.h"
#include "Key_List.h"
#include "Bool_Array.h"
#include "ace/Copy_Disabled.h"

#if defined (ACE_HAS_GPERF)

/*
 * Provides high-level routines to manipulate the keyword list
 * structures the code generation output.
 */
class Gen_Perf : private ACE_Copy_Disabled
{
public:
  /// Constructor.
  Gen_Perf (void);

  /// Destructor
  ~Gen_Perf (void);

  /// Attempt to generate a perfect hash function.
  int run (void);

private:
  int open (void);
  int change (List_Node *prior, List_Node *curr);
  int affects_prev (char c, List_Node *curr);
  int compute_perfect_hash (void);
  int compute_binary_search (void);
  int compute_linear_search (void);
  static int hash (List_Node *key_node);
  static int compute_disjoint_union (char *s1, char *s2, char *s3);
  static void sort_set (char *union_set, int len);

  /// Maximum possible hash value.
  int max_hash_value;

  /// Records fewest # of collisions for asso value.
  int fewest_collisions;

  /// Number of keywords processed without a collision.
  int num_done;

  /// Disjoint union.
  char *union_set;

  /// List of the keys we're trying to map into a perfect hash
  /// function.
  Key_List key_list;

  /// Table that keeps track of key collisions.
  Bool_Array char_search;
};

#endif /* ACE_HAS_GPERF */
#endif /* GEN_PERF_H */
