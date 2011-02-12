// -*- C++ -*-

/**
 * $Id: Iterator.h 86019 2009-07-14 12:13:09Z wotte $
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

#ifndef ITERATOR_H
#define ITERATOR_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Options.h"
#include "ace/Copy_Disabled.h"

#if defined (ACE_HAS_GPERF)

/**
 * Provides an Iterator that expands and decodes a control string
 * containing digits and ranges, returning an integer every time the
 * generator function is called.
 *
 * This class is used to decode the user's key position requests.
 * For example: "-k 1,2,5-10,$" will return 1, 2, 5, 6, 7, 8, 9,
 * 10, and 0 ( representing the abstract ``last character of the
 * key'' on successive calls to the member function operator ().
 * No errors are handled in these routines, they are passed back
 * to the calling routines via a user-supplied Error_Value
 */
class Iterator : private ACE_Copy_Disabled
{
public:
  Iterator (char *s,
            int lo,
            int hi,
            int word_end,
            int bad_val,
            int key_end);
  int operator () (void);

private:
  /// A pointer to the string provided by the user.
  char *str;

  /// Value returned after last key is processed.
  int end;

  /// A value marking the abstract ``end of word'' (usually '$').
  int end_word;

  /// Error value returned when input is syntactically erroneous.
  int error_value;

  /// Greatest possible value, inclusive.
  int hi_bound;

  /// Smallest possible value, inclusive.
  int lo_bound;
};

#endif /* ACE_HAS_GPERF */
#endif /* ITERATOR_H */
