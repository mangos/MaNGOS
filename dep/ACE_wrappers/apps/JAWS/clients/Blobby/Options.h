/* -*- C++ -*- */
// $Id: Options.h 80826 2008-03-04 14:51:23Z wotte $

//============================================================================
//
// = LIBRARY
//    JAWS
//
// = FILENAME
//    Options.h
//
// = DESCRIPTION
//     Options is an Singleton for blobby
//
// = AUTHOR
//    Prashant Jain and Sumedh Mungee
//
//============================================================================

#ifndef ACE_BLOBBY_OPTIONS_H
#define ACE_BLOBBY_OPTIONS_H

#include "Blob.h"
#include "Blob_Handler.h"
#include "ace/Get_Opt.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/ARGV.h"

class Options
{
public:
  static Options *instance (void);
  // Returns the singleton instance

  void parse_args (int argc, ACE_TCHAR *argv[]);
  // parses commandline arguments

  ACE_TCHAR *hostname_;
  // Hostname to connect to

  u_short port_;
  // Port number to use

  ACE_TCHAR *filename_;
  // Filename to upload/download

  int length_;
  // number of bytes to read/write

  int offset_;
  // offset to read/write

  char operation_;
  // "r" means download (read), and "w" means upload (write).

  int debug_;
  // turns on verbosity

protected:
  Options (void);
  // protected constructor, singleton

  static Options *instance_;
  // the singleton
};

#endif /* ACE_BLOBBY_OPTIONS_H */
