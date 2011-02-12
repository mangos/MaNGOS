// -*- C++ -*-

/**
 * $Id: Gen_Perf.cpp 91671 2010-09-08 18:39:23Z johnnyw $
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

#include "Gen_Perf.h"



#if defined (ACE_HAS_GPERF)

#include "Vectors.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_time.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_Memory.h"

/// Current release version.
extern const char *version_string;

/// Reads input keys, possibly applies the reordering heuristic, sets
/// the maximum associated value size (rounded up to the nearest power
/// of 2), may initialize the associated values array, and determines
/// the maximum hash table size.  Note: using the random numbers is
/// often helpful, though not as deterministic, of course!
Gen_Perf::Gen_Perf (void)
  : fewest_collisions (0),
    num_done (1)
{
}

/// Merge two disjoint hash key multisets to form the ordered disjoint
/// union of the sets.  (In a multiset, an element can occur multiple
/// times).  Precondition: both set1 and set2 must be
/// ordered. Returns the length of the combined set.
int
Gen_Perf::compute_disjoint_union (char *set1, char *set2, char *set3)
{
  char *base = set3;

  while (*set1 && *set2)
    if (*set1 == *set2)
      set1++, set2++;
    else
      {
        *set3 = *set1 < *set2 ? *set1++ : *set2++;
        if (set3 == base || *set3 != *(set3 - 1))
          set3++;
      }

  while (*set1)
    {
      *set3 = *set1++;
      if (set3 == base || *set3 != *(set3 - 1))
        set3++;
    }

  while (*set2)
    {
      *set3 = *set2++;
      if (set3 == base || *set3 != *(set3 - 1))
        set3++;
    }
  *set3 = '\0';
  return set3 - base;
}

/// Sort the UNION_SET in increasing frequency of occurrence.  This
/// speeds up later processing since we may assume the resulting set
/// (Set_3, in this case), is ordered. Uses insertion sort, since the
/// UNION_SET is typically short.
void
Gen_Perf::sort_set (char *union_set, int len)
{
  for (int i = 0, j = len - 1; i < j; i++)
    {
      int curr, tmp;

      for (curr = i + 1, tmp = (int) union_set[curr];
           curr > 0
           && Vectors::occurrences[tmp] < Vectors::occurrences[(int) union_set[curr - 1]];
           curr--)
        union_set[curr] = union_set[curr - 1];

      union_set[curr] = static_cast<char> (tmp);
    }
}

/// Generate a keysig's hash value.
int
Gen_Perf::hash (List_Node *key_node)
{
  int sum = option[NOLENGTH] ? 0 : key_node->length;

  for (char *ptr = key_node->keysig; *ptr; ptr++)
      sum += Vectors::asso_values[(int) *ptr];

  key_node->hash_value = sum;
  return sum;
}

/// Find out how character value change affects successfully hash
/// items.  Returns FALSE if no other hash values are affected, else
/// returns TRUE.  Note that because Option.Get_Asso_Max is a power of
/// two we can guarantee that all legal Vectors::Asso_Values are
/// visited without repetition since Option.Get_Jump was forced to be
/// an odd value!
inline int
Gen_Perf::affects_prev (char c, List_Node *curr)
{
  int const original_char = Vectors::asso_values[(int) c];
  int total_iterations = 0;

  if (!option[FAST])
    {
      total_iterations = option.asso_max ();
    }
  else
    {
      total_iterations = option.iterations ();

      if (total_iterations == 0)
        total_iterations = this->key_list.keyword_list_length ();
    }

  // Try all legal associated values.

  for (int i = total_iterations - 1; i >= 0; --i)
    {
      Vectors::asso_values[(int) c] =
        (Vectors::asso_values[(int) c]
         + (option.jump () ? option.jump () : ACE_OS::rand ()))
        & (option.asso_max () - 1);

      // Iteration Number array is a win, O(1) intialization time!
      this->char_search.reset ();

      int collisions = 0;

      // See how this asso_value change affects previous keywords.  If
      // it does better than before we'll take it!
      for (List_Node *ptr = this->key_list.head;
           this->char_search.find (this->hash (ptr)) == 0
           || ++collisions < fewest_collisions;
           ptr = ptr->next)
        {
          if (ptr == curr)
            {
              fewest_collisions = collisions;

              if (option[DEBUGGING])
                {
                  ACE_DEBUG ((LM_DEBUG,
                              "- resolved after %d iterations",
                              total_iterations - i));
                }

              return 0;
            }
        }
    }

  // Restore original values, no more tries.
  Vectors::asso_values[(int) c] = original_char;

  // If we're this far it's time to try the next character....
  return 1;
}

/// Change a character value, try least-used characters first.
int
Gen_Perf::change (List_Node *prior, List_Node *curr)
{
  if (option[DEBUGGING])
    ACE_DEBUG ((LM_DEBUG,
                "collision on keyword #%d, prior = \"%C\", curr = \"%C\" hash = %d\n",
                num_done,
                prior->key,
                curr->key,
                curr->hash_value));
  Gen_Perf::sort_set (this->union_set,
                      compute_disjoint_union (prior->keysig,
                                              curr->keysig,
                                              this->union_set));

  // Try changing some values, if change doesn't alter other values
  // continue normal action.
  ++fewest_collisions;

  for (char *temp = union_set; *temp != '\0'; temp++)
    if (affects_prev (*temp, curr) == 0)
      {
        if (option[DEBUGGING])
          ACE_DEBUG ((LM_DEBUG,
                      " by changing asso_value['%c'] (char #%d) to %d\n",
                      *temp,
                      temp - union_set + 1,
                      Vectors::asso_values[(int) *temp]));
        // Good, doesn't affect previous hash values, we'll take it.
        return 0;
      }

  for (List_Node *ptr = this->key_list.head;
       ptr != curr;
       ptr = ptr->next)
    this->hash (ptr);

  this->hash (curr);

  if (option[DEBUGGING])
    ACE_DEBUG ((LM_DEBUG,
                "** collision not resolved after %d iterations, %d duplicates remain, continuing...\n",
               !option[FAST] ? option.asso_max () : option.iterations () ? option.iterations () : this->key_list.keyword_list_length (),
                fewest_collisions + this->key_list.total_duplicates));
  return 0;
}

int
Gen_Perf::open (void)
{
  if (this->key_list.read_keys () == -1)
    return -1;

  if (option[ORDER])
    this->key_list.reorder ();

  int asso_value_max = option.asso_max ();
  int const non_linked_length = this->key_list.keyword_list_length ();

  if (asso_value_max == 0)
    asso_value_max = non_linked_length;
  else if (asso_value_max > 0)
    asso_value_max *= non_linked_length;
  else // if (asso_value_max < 0)
    asso_value_max = non_linked_length / -asso_value_max;

  option.asso_max (ACE_POW (asso_value_max));

  if (option[RANDOM])
    {
      ACE_OS::srand ((u_int) ACE_OS::time (0));

      for (int i = 0; i < ACE_STANDARD_CHARACTER_SET_SIZE; ++i)
        {
          Vectors::asso_values[i] = (ACE_OS::rand () & (asso_value_max - 1));
        }
    }
  else
    {
      int const asso_value = option.initial_value ();

      // Initialize array if user requests non-zero default.
      if (asso_value)
        {
          for (int i = ACE_STANDARD_CHARACTER_SET_SIZE - 1; i >= 0; --i)
            {
              Vectors::asso_values[i] = asso_value & (option.asso_max () - 1);
            }
        }
    }

  this->max_hash_value =
    this->key_list.max_key_length ()
    + option.asso_max ()
    * option.max_keysig_size ();

  ACE_NEW_RETURN (this->union_set,
                  char[2 * option.max_keysig_size () + 1],
                  -1);
  ACE_OS::printf ("/* ");

  if (option[C])
    ACE_OS::printf ("C");

  else if (option[CPLUSPLUS])
    ACE_OS::printf ("C++");

  ACE_OS::printf (" code produced by gperf version %s */\n",
                  version_string);
  Options::print_options ();

  if (option[DEBUGGING])
    ACE_DEBUG ((LM_DEBUG,
                "total non-linked keys = %d\n"
                "total duplicates = %d\n"
                "maximum associated value is %d\n"
                "maximum size of generated hash table is %d\n",
                non_linked_length,
                this->key_list.total_duplicates,
                asso_value_max,
                max_hash_value));
  if (this->char_search.open (max_hash_value + 1) == -1)
    return -1;
  return 0;
}

/// For binary search, do normal string sort on the keys, and then
/// assign hash values from 0 to N-1. Then go ahead with the normal
/// logic that is there for perfect hashing.
int
Gen_Perf::compute_binary_search (void)
{
  // Do a string sort.
  this->key_list.string_sort ();

  // Assign hash values.
  List_Node *curr = 0;
  int hash_value;
  for (hash_value = 0, curr = this->key_list.head;
       curr != 0;
       curr = curr->next, hash_value++)
    {
      curr->hash_value = hash_value;
    }

  return 0;
}

int
Gen_Perf::compute_linear_search (void)
{
  // Convert the list of keys to a linear list without
  // equivalence classes.
  this->key_list.string_sort ();

  // Assign hash values.
  List_Node *curr = 0;
  int hash_value = 0;
  for (hash_value = 0, curr = this->key_list.head;
       curr != 0;
       curr = curr->next, hash_value++)
    {
      curr->hash_value = hash_value;
    }
  return 0;
}

int
Gen_Perf::compute_perfect_hash (void)
{
  List_Node *curr = 0;

  for (curr = this->key_list.head;
       curr != 0;
       curr = curr->next)
    {
      this->hash (curr);

      for (List_Node *ptr = this->key_list.head;
           ptr != curr;
           ptr = ptr->next)
        if (ptr->hash_value == curr->hash_value)
          {
            if (this->change (ptr, curr) == -1)
              return -1;
            break;
          }
      num_done++;
    }

  // Make one final check, just to make sure nothing weird happened...

  this->char_search.reset ();

  for (curr = this->key_list.head;
       curr;
       curr = curr->next)
    {
      if (this->char_search.find (this->hash (curr)) != 0)
        {
          if (option[DUP])
            {
              // Keep track of the number of "dynamic" links (i.e., keys
              // that hash to the same value) so that we can use it later
              // when generating the output.
              this->key_list.total_duplicates++;
            }
          else
            {
              // Yow, big problems.  we're outta here!
              ACE_ERROR ((LM_ERROR,
                          "\nInternal error, duplicate value %d:\n"
                          "try options -D or -r, or use new key positions.\n\n",
                          this->hash (curr)));
              return -1;
            }
        }
    }

  return 0;
}

/// Does the hard stuff....  Initializes the Bool Array, and attempts
/// to find a perfect function that will hash all the key words without
/// getting any duplications.  This is made much easier since we aren't
/// attempting to generate *minimum* functions, only perfect ones.  If
/// we can't generate a perfect function in one pass *and* the user
/// hasn't enabled the DUP option, we'll inform the user to try the
/// randomization option, use -D, or choose alternative key positions.
/// The alternatives (e.g., back-tracking) are too time-consuming, i.e,
/// exponential in the number of keys.
int
Gen_Perf::run (void)
{
  if (this->open () == -1)
    return 1;

  if (option[BINARYSEARCH])
    {
      if (this->compute_binary_search () == -1)
        return 1;
    }
  else if (option[LINEARSEARCH])
    {
      if (this->compute_linear_search () == -1)
        return 1;
    }
  else
    {
      if (this->compute_perfect_hash () == -1)
        return 1;

      // Sorts the key word list by hash value, and then outputs the
      // list.  The generated hash table code is only output if the
      // early stage of processing turned out O.K.
      this->key_list.sort ();
    }

  this->key_list.output ();
  return 0;
}

/// Prints out some diagnostics upon completion.
Gen_Perf::~Gen_Perf (void)
{
  if (option[DEBUGGING])
    {
      ACE_DEBUG ((LM_DEBUG,
                  "\ndumping occurrence and associated values tables\n"));
      for (int i = 0; i < ACE_STANDARD_CHARACTER_SET_SIZE; i++)
        if (Vectors::occurrences[i])
          ACE_DEBUG ((LM_DEBUG,
                      "Vectors::asso_values[%c] = %6d, Vectors::occurrences[%c] = %6d\n",
                      i,
                      Vectors::asso_values[i],
                      i,
                      Vectors::occurrences[i]));
      ACE_DEBUG ((LM_DEBUG,
                  "end table dumping\n"));
    }

  delete [] this->union_set;
}

#endif /* ACE_HAS_GPERF */
