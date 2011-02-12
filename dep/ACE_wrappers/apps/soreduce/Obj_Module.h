// -*- C++ -*-
// $Id: Obj_Module.h 91743 2010-09-13 18:24:51Z johnnyw $

// File: Obj_Module.h

// Author: Phil Mesnier

#ifndef _OBJ_MODULE_H_
#define _OBJ_MODULE_H_

// Obj_Module encapsulates the result of applying nm to a single object module
// in a shared library.  Currently an object module consists of two types of
// signatures, those that are exported, able to resolve references from others,
// and those that are imported, needing resolution.
//
// Obj_Modules keep track of external references. In the end, any module that
// has one or more external references to it must be included in the resulting
// library. While the means exists to remove external references, perhaps
// through further analysis of undefined signatures and their usage, this is
// not currently done. Once a technique is discovered to allow for easy
// determination that reference is truly unneeded this code may be useful.

#include "Sig_List.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

class Obj_Module {
public:
  Obj_Module ( const ACE_CString &, int = 500);

  // Returns the list of exported signatures, ie. those that are defined in
  // this module
  Sig_List & exports();

  // Returns the list of signatures used by this module but not defined within
  Sig_List & imports();

  // Returns the name of the object module.
  ACE_CString &name();

  // Add_source invokes GNU nm on the supplied file and parses the output to
  // build the list of imported and exported signatures.  When replacing GNU
  // nm to use a different tool, this method must be modified.  In the future
  // this could be a virtual to allow for specialization based on toolset.
  void add_source (const char *, int = 0);

  // Get the number of external references to this object module. At the end
  // of processing, if the number of external references is 0, the module is
  // not included in the final library.
  int extref ();

  // add a new external reference to this module.
  void add_extref ();

  // remove an exterenal reference. Currently, this function is not used.
  void remove_extref();

private:
  void populate_sig_list (Sig_List &, int ,  ACE_Message_Block *);
  int read_line (ACE_HANDLE src, ACE_Message_Block **buf);

  ACE_CString name_;
  Sig_List imports_;
  Sig_List exports_;
  int extrefs_;
};

#endif /* _OBJ_MODULE_H_ */
