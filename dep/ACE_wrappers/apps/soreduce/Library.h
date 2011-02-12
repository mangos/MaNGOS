// -*- C++ -*-
#ifndef _LIBRARY_H_
#define _LIBRARY_H_

// -*- C++ -*-
// $Id: Library.h 80826 2008-03-04 14:51:23Z wotte $

// File: Library.h

// Author: Phil Mesnier

// A Library is a collection of Obj_Modules that define a single shared
// library. It is used to manipulate the list of unresolved references by
// removing those that are resolved and adding those brought in by new modules
// that are required to resolve references.  The Library is responsible
// for outputting a specialized mpc file to build the reduce footprint library.

#include "Obj_Module.h"

// FUZZ: disable check_for_streams_include
#include "ace/streams.h"

// The MPC generator class serves as the base class used to output the
// custom mpc files used to build the subsetted libraries.
// The base class will make libACE_subset.so

class MPC_Generator
{
public:
  MPC_Generator (const ACE_CString& libname);
  virtual ~MPC_Generator();

  void write_prolog (const ACE_CString& );
  void write_file (const ACE_CString& );
  void write_epilog ();

protected:
  virtual void write_baseprojects();
  virtual void write_projectinfo();

  ofstream mpcfile_;
  ACE_CString libname_;
  ACE_CString mpcfilename_;
};

// Generate mpc files for libraries dependant on ACE, that are not TAO.
class MPC_ACE_Dep_Lib : public MPC_Generator
{
public:
  MPC_ACE_Dep_Lib (const ACE_CString& libname);

protected:
  virtual void write_baseprojects();
  virtual void write_projectinfo();
};

// Generates mpc files for libTAO_subset.so
class MPC_TAO_Lib : public MPC_ACE_Dep_Lib
{
public:
  MPC_TAO_Lib (const ACE_CString& libname);

protected:
  virtual void write_baseprojects();
  virtual void write_projectinfo();
};

// Generates makefiles for libs dependant on TAO.  This has a problem when
// building libraries in the orbsvcs tree.
class MPC_TAO_Dep_Lib : public MPC_TAO_Lib
{
public:
  MPC_TAO_Dep_Lib (const ACE_CString& );

protected:
  virtual void write_baseprojects();
  virtual void write_projectinfo();
};

//----------------------------------------------------------------------------

class Library
{
public:

  Library (const ACE_TCHAR *name = 0 );
  /// Constructor is responsible for loading all of the modules related to the
  /// library
  ~Library ();

  // Resolve interates over the supplied list of undefined signatures to locate
  // modules that contain definitions. Any symbol defined in a module marked as
  // exported is simply removed from the undef list. Any symbol defined in a
  // module not yet exported removed from the undef list, the module is marked
  // as exported, and its unresolved symbols are added to the undef list.
  void resolve (Sig_List &undefs);

  // Outputs a list of files suitable for inclusion in an mpc file to produce
  // a subsetted library. If the argument is non-zero, reference countes for
  // each module are also listed.
  void write_export_list ( int );

  // set the path to find the .so files
  void set_path (const ACE_TCHAR *p );

  // Load the actual .so files from the path.
  void load_modules();

  // returns the library name
  const ACE_CString &name () const;

  // returns non-zero if the module count is > 0.
  int has_modules () const;

private:
  ACE_CString name_;
  ACE_CString path_;

  int num_modules_;
  int num_exports_;
  int num_extrefs_;

  Obj_Module **modules_;
  Sig_List exported_;
  MPC_Generator *mpcfile_;
};

#endif /* _LIBRARY_H_ */
