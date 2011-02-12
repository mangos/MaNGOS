/* -*- C++ -*- */
// $Id: File_Parser.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    gateway
//
// = FILENAME
//    File_Parser.h
//
// = AUTHOR
//    Doug Schmidt
//
// ============================================================================

#ifndef _FILE_PARSER
#define _FILE_PARSER

#include "ace/Basic_Types.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class FPRT
{
  // = TITLE
  //     This class serves as a namespace for the <Return_Type>.
public:
  enum Return_Type
  {
    RT_EOLINE,
    RT_EOFILE,
    RT_SUCCESS,
    RT_COMMENT,
    RT_DEFAULT,
    RT_PARSE_ERROR
  };
};

template <class ENTRY>
class File_Parser
{
  // = TITLE
  //     Class used to parse the configuration file for the
  //     <Consumer_Map>.
public:

  /// Destructor.
  virtual ~File_Parser (void);

  // = Open and Close the file specified
  int open (const ACE_TCHAR filename[]);
  int close (void);

  virtual FPRT::Return_Type read_entry (ENTRY &entry,
                                        int &line_number) = 0;
  // Pure virtual hook that subclasses override and use the protected
  // methods to fill in the <entry>.

protected:
  FPRT::Return_Type getword (char buf[]);
  // Read the next ASCII word.

  FPRT::Return_Type getint (ACE_INT32 &value);
  // Read the next integer.

  FPRT::Return_Type readword (char buf[]);
  // Read the next "word," which is demarcated by <delimiter>s.
  //
  // @@ This function is inherently flawed since it doesn't take a
  // count of the size of <buf>...

  int delimiter (char ch);
  // Returns true if <ch> is a delimiter, i.e., ' ', ',', or '\t'.

  int comments (char ch);
  // Returns true if <ch> is the comment character, i.e., '#'.

  int skipline (void);
  // Skips to the remainder of a line, e.g., when we find a comment
  // character.

  FILE *infile_;
  // Pointer to the file we're reading.
};

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "File_Parser.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("File_Parser.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#endif /* _FILE_PARSER */
