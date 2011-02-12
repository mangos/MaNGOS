// -*- C++ -*-

/**
 * $Id: List_Node.cpp 91671 2010-09-08 18:39:23Z johnnyw $
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

#include "List_Node.h"



#if defined (ACE_HAS_GPERF)

#include "Vectors.h"
#include "ace/OS_NS_ctype.h"

/// Sorts the key set alphabetically to speed up subsequent operation
/// Uses insertion sort since the set is probably quite small.
inline void
List_Node::sort (char *base, int len)
{
  int i, j;

  for (i = 0, j = len - 1; i < j; ++i)
    {
      char tmp;
      int curr;

      for (curr = i + 1, tmp = base[curr];
           curr > 0 && tmp < base[curr - 1];
           --curr)
        {
          base[curr] = base[curr - 1];
        }

      base[curr] = tmp;
    }
}

/// Initializes a List_Node.  This requires obtaining memory for the
/// CHAR_SET initializing them using the information stored in the
/// KEY_POSITIONS array in Options, and checking for simple errors.
/// It's important to note that KEY and REST are both pointers to the
/// different offsets into the same block of dynamic memory pointed to
/// by parameter K. The data member REST is used to store any
/// additional fields of the input file (it is set to the "" string if
/// Option[TYPE] is not enabled).  This is useful if the user wishes to
/// incorporate a lookup structure, rather than just an array of keys.
/// Finally, KEY_NUMBER contains a count of the total number of keys
/// seen so far.  This is used to initialize the INDEX field to some
/// useful value.
List_Node::List_Node (char *k, int len)
  : link (0),
    next (0),
    key (k),
    rest (option[TYPE] ? k + len + 1 : const_cast<char*> ("")),
    length (len),
    slot (0)
{
  char *ptr = new char[(option[ALLCHARS] ? len : option.max_keysig_size ()) + 1];
  keysig = ptr;
  k[len] = '\0';             // Null terminate KEY to separate it from REST.

  // Lower case if STRCASECMP option is enabled.
  if (option[STRCASECMP])
    for (char *p = k; *p; p++)
      if (ACE_OS::ace_isupper (*p))
        *p = static_cast<char> (ACE_OS::ace_tolower (*p));

  if (option[ALLCHARS])         // Use all the character position in the KEY.
    for (; *k; k++, ptr++)
      {
        *ptr = *k;
        int i = (int) *ptr;
        ++Vectors::occurrences[i];
      }
  else
    {
      // Only use those character positions specified by the user.
      option.reset ();

      // Iterate thru the list of key_positions, initializing
      // occurrences table and keysig (via char * pointer ptr).
      for (int i; (i = option.get ()) != EOS; )
        {
          if (i == WORD_END) // Special notation for last KEY position, i.e. '$'.
            *ptr = key[len - 1];
          else if (i <= len) // Within range of KEY length, so we'll keep it.
            *ptr = key[i - 1];
          else // Out of range of KEY length, so we'll just skip it.
            continue;
          ++Vectors::occurrences[(int) *ptr++];
        }

      // Didn't get any hits and user doesn't want to consider the
      // keylength, so there are essentially no usable hash positions!
      if (ptr == keysig && option[NOLENGTH])
        ACE_ERROR ((LM_ERROR,
                    "Can't hash keyword %s with chosen key positions.\n%a",
                    key,
                    1));
    }
  // Terminate this string.
  *ptr = '\0';

  // Sort the KEYSIG items alphabetically.
  sort (keysig, ptr - keysig);
}

List_Node::~List_Node (void)
{
  delete [] this->key;
  delete [] this->keysig;
}

#endif /* ACE_HAS_GPERF */
