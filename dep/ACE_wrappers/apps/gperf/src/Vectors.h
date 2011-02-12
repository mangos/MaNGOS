// -*- C++ -*-

/**
 * $Id: Vectors.h 86019 2009-07-14 12:13:09Z wotte $
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

#ifndef VECTORS_H
#define VECTORS_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#if defined (ACE_HAS_GPERF)

// Do not change these values wantonly since GPERF depends on them..
#define ACE_ASCII_SIZE 128
#define ACE_EBCDIC_SIZE 256

#if 'a' < 'A'
#define ACE_HAS_EBCDIC
#define ACE_STANDARD_CHARACTER_SET_SIZE 256
#else
#define ACE_HAS_ASCII
#define ACE_STANDARD_CHARACTER_SET_SIZE 128
#endif /* 'a' < 'A' */

/*
 * Static class data members that are shared between several
 * classes via inheritance.
 */
class Vectors
{
public:
  /// Counts occurrences of each key set character.
  static int occurrences[ACE_STANDARD_CHARACTER_SET_SIZE];

  /// Value associated with each character.
  static int asso_values[ACE_STANDARD_CHARACTER_SET_SIZE];
};

#endif /* ACE_HAS_GPERF */
#endif /* VECTORS_H */
