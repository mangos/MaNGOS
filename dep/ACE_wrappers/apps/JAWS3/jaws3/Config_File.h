/* -*- c++ -*- */
// $Id: Config_File.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_CONFIG_FILE_H
#define JAWS_CONFIG_FILE_H

#include "jaws3/Export.h"

class JAWS_Config_File;
class JAWS_Config_File_Impl;

class JAWS_Export JAWS_Config_File
// = TITLE
//     A simple configuration file manager.
//
// = DESCRIPTION
//     Reads in a configuration file.  The syntax of the configuration
//     file is:
// = BEGIN<CODE>
//     <configuration-file>   := <configuration-seq>
//
//     <configuration-seq>    := <configuration-line> [<configuration-seq>]
//
//     <configuration-line>   := <name-value-pair-line>
//                             | <comment-line>
//
//     <name-value-pair-line> := <name> '=' <value-line>
//
//     <name>                 := <char-seq>
//
//     <value-line>           := [<char-seq>] <LF> [<continuation-seq>]
//
//     <continuation-seq>     := <continuation-line> [<continuation-seq>]
//
//     <continuation-line>    := <LWSP> [<char-seq>] <LF>
//
//     <comment-line>         := [<comment>] <LF>
//
//     <comment>              := '#' <char-seq>
//
//     <char-seq>             := <char> [<char-seq>]
// = END<CODE>
//     No <configuration-line> is to exceed 4094 characters.
{
public:

  // = Initialization

  JAWS_Config_File (const ACE_TCHAR *config_file,
                    const ACE_TCHAR *config_dir = "./");
  // Parse the specified <config_file> in the <config_dir>.

  // = Searching

  int find (const ACE_TCHAR *key, const ACE_TCHAR *&value);
  // Find the <value> associated with <key>.

public:

  void reset (void);
  // Re-read the configuration file.

  void dump (void);
  // Dump the values of all configuration variables.

private:

  JAWS_Config_File_Impl *impl_;
  // Opaque implementation.

};

#endif /* JAWS_CONFIG_FILE_H */
