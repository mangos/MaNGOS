// -*- C++ -*-

/**
 * $Id: Options.h 91273 2010-08-04 16:24:59Z johnnyw $
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "ace/Log_Msg.h"
#include "ace/SString.h"
#include "ace/Copy_Disabled.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#if defined (ACE_HAS_GPERF)

/// Enumerate the potential debugging Options.
enum Option_Type
{
  DEBUGGING    = 01,            /* Enable debugging (prints diagnostics to stderr). */
  ORDER        = 02,            /**< Apply ordering heuristic to speed-up search time. */
  ANSI         = 04,            /**< Generate ANSI prototypes. */
  ALLCHARS     = 010,           /**< Use all characters in hash function. */
  INLINE       = 020,           /**< Generate code for inline functions. */
  TYPE         = 040,           /**< Handle user-defined type structured keyword input. */
  RANDOM       = 0100,          /**< Randomly initialize the associated values table. */
  DEFAULTCHARS = 0200,          /**< Make default char positions be 1,$ (end of keyword). */
  SWITCH       = 0400,          /**< Generate switch output to save space. */
  POINTER      = 01000,         /**< Have in_word_set function return pointer, not boolean. */
  NOLENGTH     = 02000,         /**< Don't include keyword length in hash computations. */
  LENTABLE     = 04000,         /**< Generate a length table for string comparison. */
  DUP          = 010000,        /**< Handle duplicate hash values for keywords. */
  FAST         = 020000,        /**< Generate the hash function ``fast.'' */
  NOTYPE       = 040000,        /**< Don't include user-defined type definition in output -- it's already defined elsewhere. */
  COMP         = 0100000,       /**< Generate strncmp rather than strcmp. */
  GLOBAL       = 0200000,       /**< Make the keyword table a global variable. */
  CONSTANT     = 0400000,       /**< Make the generated tables readonly (const). */
  CPLUSPLUS    = 01000000,      /**< Generate C++ code. */
  C            = 02000000,      /**< Generate C code. */
  ENUM         = 04000000,      /**< Use enum for constants. */
  STRCASECMP   = 010000000,     /**< Use the case insensitive comparison. */
  OPTIMIZE     = 020000000,     /**< Assume all input keywords are in the keyset. */
  ADA          = 040000000,     /**< Generate Ada code. */
  MUTE         = 0100000000,    /**< Dont print the warnings. */
  SKIPCLASS    = 0200000000,    /**< Skip the class definition part in the output while in C++ mode. */
  SKIPSTRINGH  = 0400000000,    /**< Skip including the header file ace/OS_NS_string.h. */
  BINARYSEARCH = 01000000000,   /**< Generates Binary Search code. */
  LINEARSEARCH = 02000000000    /**< Generates Linear Search code. */
};

// Define some useful constants (these don't really belong here, but
// I'm not sure where else to put them!).  These should be consts, but
// g++ doesn't seem to do the right thing with them at the
// moment... ;-(

// PharLap ETS defines EOS as well... so if building for ETS, clear out
// their EOS.
#if defined (ACE_HAS_PHARLAP) && defined (EOS)
# undef EOS
#endif /* ACE_HAS_PHARLAP && EOS */

enum
{
  MAX_KEY_POS = 128 - 1,    /**< Max size of each word's key set. */
  WORD_START = 1,           /**< Signals the start of a word. */
  WORD_END = 0,             /**< Signals the end of a word. */
  EOS = MAX_KEY_POS         /**< Signals end of the key list. */
};

/*
 * This class provides a uniform interface to the various options
 * available to a user of the gperf hash function generator.
 *
 * In addition to the run-time options, found in the <Option_Type>
 * there is also the hash table Size and the Keys to be used in
 * the hashing.  The overall design of this module was an
 * experiment in using C++ classes as a mechanism to enhance
 * centralization of option and and error handling.
 *
 * @todo The Options class should be changed to use the Singleton pattern.
 */
class Options : private ACE_Copy_Disabled
{
public:
  Options (void);
  ~Options (void);
  int operator[] (Option_Type option);
  int parse_args (int argc, ACE_TCHAR *argv[]);
  void operator= (enum Option_Type);
  bool operator!= (enum Option_Type);
  static void print_options (void);
  static void asso_max (int r);
  static int asso_max (void);
  static void reset (void);
  static int get (void);
  static int iterations (void);
  static u_int max_keysig_size (void);
  static void keysig_size (u_int);
  static int jump (void);
  static int initial_value (void);
  static int total_switches (void);
  static const char *function_name (void);
  static const char *fill_default (void);
  static const char *key_name (void);
  static const char *class_name (void);
  static const char *hash_name (void);
  static const char *delimiter (void);

private:
  /// Holds the user-specified Options.
  static int option_word_;

  /// Number of switch statements to generate.
  static int total_switches_;

  /// Total number of distinct key_positions.
  static u_int total_keysig_size_;

  /// Range of the hash table.
  static int size_;

  /// Tracks current key position for Iterator.
  static int key_pos_;

  /// Tracks current key position for Iterator.
  static int jump_;

  /// Initial value for asso_values table.
  static int initial_asso_value_;

  /// Amount to iterate when a collision occurs.
  static int iterations_;

  /// Records count of command-line arguments.
  static int argc_;

  /// Stores a pointer to command-line vector.
  static ACE_TCHAR **argv_;

  /// Names used for generated lookup function.
  static ACE_CString function_name_;

  /// Expression used to assign default values in keyword table.
  static ACE_CString fill_default_;

  /// Name used for keyword key.
  static ACE_CString key_name_;

  /// Name used for generated C++ class.
  static ACE_CString class_name_;

  /// Name used for generated hash function.
  static ACE_CString hash_name_;

  /// Separates keywords from other attributes.
  static ACE_CString delimiters_;

  /// Contains user-specified key choices.
  static char key_positions_[MAX_KEY_POS];

  /// Sorts key positions in REVERSE order.
  static int key_sort (char *base, int len);

  /// Prints proper program usage.
  static void usage (void);
};

/// Global option coordinator for the entire program.
extern Options option;

#endif /* ACE_HAS_GPERF */
#endif /* OPTIONS_H */
