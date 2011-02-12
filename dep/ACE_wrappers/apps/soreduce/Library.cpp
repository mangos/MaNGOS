// -*- C++ -*-
// $Id: Library.cpp 91670 2010-09-08 18:02:26Z johnnyw $

// File: Library.cpp

// Author: Phil Mesnier

// This file contains the implementation of the classes responsible for
// generating specialized mpc files for individual libraries, as well as
// outputting usage metrics for the various object modules contained in the
// library.

#include "ace/OS_NS_dirent.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_sys_stat.h"
#include "ace/OS_NS_ctype.h"
#include "ace/Log_Msg.h"

#include "Library.h"

MPC_Generator::MPC_Generator (const ACE_CString& libname)
  : mpcfile_(),
    libname_(libname),
    mpcfilename_()
{
  mpcfilename_ = libname_ + "_subset.mpc";
}

MPC_Generator::~MPC_Generator ()
{
}

void
MPC_Generator::write_file (const ACE_CString& file)
{
  mpcfile_ << "    " << file << ".cpp" << endl;
}

void
MPC_Generator::write_prolog (const ACE_CString& path)
{
  ACE_CString fname (path + "/" + mpcfilename_);
  ACE_DEBUG ((LM_DEBUG, "writing file %s\n",fname.c_str()));
  mpcfile_.open(fname.c_str());
  if (!mpcfile_)
    ACE_DEBUG ((LM_DEBUG,"mpc file open failed\n"));

  mpcfile_
    << "// Generated mpc file for producing a subset of the "
    << libname_ << " library " << endl << endl
    << "project(" << libname_ << "_subset)";

  this->write_baseprojects ();

  mpcfile_
    << " {" << endl
    << "  sharedname   = " << libname_ << "_subset" << endl
    << "  pch_header   = " << endl
    << "  pch_source   = " << endl;

  this->write_projectinfo ();

  mpcfile_ << endl
           << "  Source_Files {" << endl;
}

void
MPC_Generator::write_epilog ()
{
  mpcfile_ << "  }" << endl
           << "}" << endl;
  mpcfile_.close();
}

void
MPC_Generator::write_baseprojects()
{
  mpcfile_ << ": acedefaults, core";
}

void
MPC_Generator::write_projectinfo()
{
  mpcfile_ << "  libout       = $(ACE_ROOT)/lib" << endl
           << "  dynamicflags = ACE_BUILD_DLL ACE_OS_BUILD_DLL" << endl;
}

//-----------------------------------------------------------------------------
MPC_ACE_Dep_Lib::MPC_ACE_Dep_Lib (const ACE_CString& libname)
  : MPC_Generator(libname)
{}

void
MPC_ACE_Dep_Lib::write_baseprojects()
{
  mpcfile_ << ": acedefaults, aceversion";
}

void
MPC_ACE_Dep_Lib::write_projectinfo()
{
  mpcfile_ << "  libout       = $(ACE_ROOT)/lib" << endl
           << "  libs        += ACE_subset" << endl
           << "  after       += ACE_subset" << endl;
}

//-----------------------------------------------------------------------------
MPC_TAO_Lib::MPC_TAO_Lib (const ACE_CString& libname)
  : MPC_ACE_Dep_Lib(libname)
{}

void
MPC_TAO_Lib::write_baseprojects()
{
  MPC_ACE_Dep_Lib::write_baseprojects ();
  mpcfile_ << ", core, tao_output, taodefaults";
}

void
MPC_TAO_Lib::write_projectinfo()
{
  MPC_ACE_Dep_Lib::write_projectinfo();
  mpcfile_ << "  dynamicflags = TAO_BUILD_DLL" << endl;
}

//-----------------------------------------------------------------------------
MPC_TAO_Dep_Lib::MPC_TAO_Dep_Lib (const ACE_CString& libname)
  : MPC_TAO_Lib(libname)
{}

void
MPC_TAO_Dep_Lib::write_baseprojects()
{
  MPC_TAO_Lib::write_baseprojects ();
  mpcfile_ << ", taoidldefaults";
}

void
MPC_TAO_Dep_Lib::write_projectinfo()
{
  // Try our best to generate the dynamicflags
  ACE_CString dflags;
  for(size_t i = 0; i < this->libname_.length (); ++i) {
    dflags += static_cast<char>(ACE_OS::ace_toupper (this->libname_[i]));
  }
  dflags += "_BUILD_DLL";

  MPC_ACE_Dep_Lib::write_projectinfo();
  mpcfile_ << "  dynamicflags = " << dflags.c_str () << endl
           << "  libs        += TAO_subset" << endl
           << "  after       += TAO_subset" << endl
           << "  includes    += $(TAO_ROOT)/orbsvcs" << endl
           << "  idlflags    += -I$(TAO_ROOT)/orbsvcs" << endl;
}

//-----------------------------------------------------------------------------

Library::Library (const char *name)
  : name_(name),
    path_(),
    num_modules_(0),
    num_exports_(0),
    num_extrefs_(0),
    modules_(0),
    exported_(0),
    mpcfile_(0)
{
  if (name_ == "ACE")
    mpcfile_ = new MPC_Generator(name_);
  else if (name_.find ("ACE_") == 0)
    mpcfile_ = new MPC_ACE_Dep_Lib (name_);
  else if (name_ == "TAO")
    mpcfile_ = new MPC_TAO_Lib (name_);
  else
    mpcfile_ = new MPC_TAO_Dep_Lib (name_);
}

Library::~Library ()
{
  delete mpcfile_;
  int i;

  for (i = 0; i < num_modules_; delete modules_[i++])
    {
      // No action.
    }

  delete [] modules_;
}

void
Library::set_path (const char *p)
{
  char abspath[1000];
  ACE_OS::memset (abspath,0,1000);
  ssize_t abspathlen = ACE_OS::readlink(p,abspath,999);
  ACE_CString path (p);
  if (abspathlen > 0) {
    abspath[abspathlen] = 0;
    path = abspath;
  }

  ACE_CString::size_type pathsep = path.rfind('/');

  if (pathsep == ACE_CString::npos) {
    path_ = ".";
  } else {
    path_ = path.substr(0,pathsep);
  }
}

const ACE_CString &
Library::name () const
{
  return name_;
}

int
Library::has_modules () const
{
  return num_modules_ > 0;
}

extern "C" {

static int
selector (const dirent *d)
{
  return ACE_OS::strstr (d->d_name, ACE_TEXT (".o")) != 0;
}

static int
comparator (const dirent **d1, const dirent **d2)
{
  return ACE_OS::strcmp ((*d1)->d_name, (*d2)->d_name);
}

} /* extern "C" */

void
Library::load_modules ()
{
  ACE_CString subdir = path_ + "/.shobj";

  struct dirent **dent;
  num_modules_ = ACE_OS::scandir(ACE_TEXT_CHAR_TO_TCHAR (subdir.c_str()),
                                 &dent,selector,comparator);

  if (num_modules_ > 0) {
    modules_ = new Obj_Module * [num_modules_];
    for (int i = 0; i < num_modules_; i++) {
      ACE_CString ent_name (ACE_TEXT_ALWAYS_CHAR (dent[i]->d_name));
      modules_[i] = new Obj_Module(ent_name);
      modules_[i]->add_source (ACE_CString(subdir + "/" + ent_name).c_str());
      ACE_OS::free(dent[i]);
    };
  }

  if (num_modules_ > -1)
    ACE_OS::free(dent);
}

void
Library::resolve (Sig_List &undefs)
{
  if (num_modules_ < 1)
    return;

  for (const Signature *uname = undefs.first();
       undefs.hasmore();
       uname = undefs.next()) {
    if (exported_.index_of(uname) != -1) {
      undefs.remove_current();
    }
    else
      for (int i = 0; i < num_modules_; i++)
        if (modules_[i]->extref() == 0 &&
            modules_[i]->exports().index_of(uname) != -1)
          {
            undefs.remove_current();
            exported_.add (modules_[i]->exports());
            for (const Signature *needed = modules_[i]->imports().first();
                 modules_[i]->imports().hasmore();
                 needed = modules_[i]->imports().next())
              if (exported_.index_of(needed) == -1)
                undefs.add (needed->name());
            modules_[i]->add_extref();
            num_extrefs_++;
            break;
          }
  }
}

void
Library::write_export_list (int show_ref_counts)
{
  if (num_modules_ < 1)
    return;

  ACE_CString excludedfilename = path_ + "/excluded_modules";
  ACE_CString rcpath = path_ + "/usage_metrics";

  ofstream exclusions (excludedfilename.c_str());
  if (!exclusions) {
    ACE_ERROR ((LM_ERROR, "%p\n", "open exclusions list"));
  }

  if (show_ref_counts) {
    ACE_DEBUG ((LM_DEBUG, "Making directory %s\n",rcpath.c_str()));
    if (ACE_OS::mkdir(ACE_TEXT_CHAR_TO_TCHAR (rcpath.c_str())) == -1 &&
        errno != EEXIST)
      ACE_ERROR ((LM_ERROR, "%p\n", "mkdir"));
  }

  ACE_DEBUG ((LM_DEBUG,"%s: %d out of %d modules required\n",
              name_.c_str(), num_extrefs_, num_modules_));

  mpcfile_->write_prolog(path_);

  for (int i = 0; i < num_modules_ ; i++)
    if (modules_[i]->extref()) {
      if (show_ref_counts) {
        ACE_CString fname = rcpath + "/" +  modules_[i]->name();
        ofstream countfile (fname.c_str());
        countfile << "Exported symbols:" << endl;
        for (const Signature *sig = modules_[i]->exports().first();
             modules_[i]->exports().hasmore();
             sig = modules_[i]->exports().next())
          {
            countfile.width(5);
            countfile << sig->used_count() << " " << sig->name() << endl;
          }
        countfile << "\nImported symbols:" << endl;
        for (const Signature *n_sig = modules_[i]->imports().first();
             modules_[i]->imports().hasmore();
             n_sig = modules_[i]->imports().next())
          countfile << n_sig->name() << endl;
      }
      mpcfile_->write_file(modules_[i]->name().substring(0,modules_[i]->name().length()-2));
    } else {
      //      const char * modname = modules_[i]->name().c_str();
      exclusions
        << modules_[i]->name().substring(0,modules_[i]->name().length()-2)
        << endl;
    }

  mpcfile_->write_epilog();
}
