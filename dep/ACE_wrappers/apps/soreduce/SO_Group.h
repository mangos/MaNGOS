// -*- C++ -*-
// $Id: SO_Group.h 91743 2010-09-13 18:24:51Z johnnyw $

// File: SO_Group.h

// Author: Phil Mesnier

#ifndef _SO_GROUP_H_
#define _SO_GROUP_H_

#include "Library.h"

// A shared object group is a wrapper around all of the information needed to
// analize a collection of applications so that common shared libraries can
// be reduced.

class SO_Group
{
public:
  SO_Group ();
  ~SO_Group ();

  // For each executable named, run ldd to get the dependances list, For each
  // library listed, see if there is a path to .shobj and add to the list of
  // libraries if found. Finally, add the undefined symbols from the executable
  // to the undefs collection.
  void add_executable(const char * );

  // Do the actual business of the program
  void analize ();

  // Output the results
  void write_results ();

  // load the object modules for the group
  void load_modules ();

  void list_libs();

private:
  Obj_Module undef_wrapper_;
  Sig_List &undefs_;
  Library **libs_;
  int max_libs_;
  int num_libs_;
};

#endif //_SO_GROUP_H_
