// -*- C++ -*-
// $Id: Signature.h 91743 2010-09-13 18:24:51Z johnnyw $

// File: Signature.h

// Author: Phil Mesnier


#ifndef _SIGNATURE_H_
#define _SIGNATURE_H_

// Signature class encapsulates a single line of nm output. This line may be
// either an "undefined" name to be resolved, or text or data which resolves
// the unknowns. Some of the features of the Signature class are currently
// unused, such as owner_, which is anticipation of analysis that may lead to
// further code reduction. The premise being that unresolved symbols that are
// defined within otherwise unused code should not be resolved. However this
// information is not available in the output of nm. Further research is
// required.
//
// Signatures are reference counted to avoid duplication.

#include <ace/SString.h>

class Signature {
public:

  enum Kind {
    text_,
    undef_
  };

  Signature (const ACE_CString &);
  void used ();
  int used_count() const;

  const ACE_CString &name() const;

  Signature *dup();
  void release();

private:
  ACE_CString name_;
  int ref_count_;
  int used_;
  Signature * owner_;
  Kind kind_;
};

#endif
