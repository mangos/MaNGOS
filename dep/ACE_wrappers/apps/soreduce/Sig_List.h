// -*- C++ -*-
// $Id: Sig_List.h 91743 2010-09-13 18:24:51Z johnnyw $

// File: Sig_List.h

// Author: Phil Mesnier


#ifndef _SIG_LIST_H_
#define _SIG_LIST_H_

// A Sig_List is a specialized container of signatures. The initial use of a
// Sig_List was to manage a variable length of undefined Signatures, so the
// program could know when all possible resolutions were determined. As the
// program grows in complexity, Sig_Lists are used to store other groups as
// well.  The methods provide simple list traversal, as well as efficient use
// of space.

#include "Signature.h"

class Sig_List {
public:
  Sig_List (int cap = 500);
  ~Sig_List ();
  void add (const ACE_CString &s);
  void add (const Sig_List &other);
  void remove (const Signature &s);
  void remove_current ();

  int index_of (const Signature *s);
  int index_of (const ACE_CString &s);
  int hasmore();
  const Signature *first();
  const Signature *next();

  int modified ();
  int size();

private:
  int size_;
  int capacity_;
  int index_;
  int has_nulls_;
  int modified_;
  Signature ** array_;
};


#endif /* _SIG_LIST_H_ */
