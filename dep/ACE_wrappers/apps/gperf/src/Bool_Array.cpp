// -*- C++ -*-

// Fast lookup table abstraction implemented as an Iteration Number Array

/**
 * $Id: Bool_Array.cpp 91671 2010-09-08 18:39:23Z johnnyw $
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

#include "Bool_Array.h"
#include "ace/OS_NS_string.h"



#if defined (ACE_HAS_GPERF)

#include "ace/OS_Memory.h"

// Prints out debugging diagnostics.

Bool_Array::~Bool_Array (void)
{
  if (option[DEBUGGING])
    ACE_DEBUG ((LM_DEBUG,
                "\ndumping boolean array information\n"
                "size = %u\niteration number = %u\nend of array dump\n",
                size_,
                generation_number_));
  delete [] this->storage_array_;
}

Bool_Array::Bool_Array (void)
  : storage_array_ (0),
    generation_number_ (0),
    size_ (0)
{
}

int
Bool_Array::open (u_long s)
{
  this->generation_number_ = 1;
  this->size_ = s;

  ACE_NEW_RETURN (storage_array_,
                  u_long[s],
                  -1);

  ACE_OS::memset (storage_array_,
                  0,
                  s * sizeof *storage_array_);

  if (option[DEBUGGING])
    ACE_DEBUG ((LM_DEBUG,
                "\nbool array size = %u, total bytes = %u\n",
                size_,
                size_ * (int) sizeof *storage_array_));
  return 0;
}

int
Bool_Array::find (u_long slot)
{
  if (storage_array_[slot] == generation_number_)
    return 1;
  else
    {
      storage_array_[slot] = generation_number_;
      return 0;
    }
}

void
Bool_Array::reset (void)
{
  if (++generation_number_ == 0)
    {
      if (option[DEBUGGING])
        ACE_DEBUG ((LM_DEBUG,
                    "(re-initializing bool_array)..."));

      this->generation_number_ = 1;
      ACE_OS::memset (storage_array_,
                      0,
                      size_ * sizeof *storage_array_);
      if (option[DEBUGGING])
        ACE_DEBUG ((LM_DEBUG,
                    "done\n"));
    }
}

#endif /* ACE_HAS_GPERF */
