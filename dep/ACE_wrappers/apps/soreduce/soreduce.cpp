// $Id: soreduce.cpp 91670 2010-09-08 18:02:26Z johnnyw $

// File: soreduce.cpp

// Author: Phil Mesnier

// theory of operation:
//  1. Build a complete set of applications
//  2. apply "nm" to each of the .o files that make up the libraries to subset
//     filter the results into two files for each, one with exported names, the
//     other with imported names.
//  3. apply "nm" to all of the elements which use ace & tao. build a list of
//     imported names
//  4. Repeat the following steps until no entries remain in the list of
//     imports
//  4.1 Take a name from the list of imports, locate the module containing the
//      export of that name
//  4.2 Add the exporting module to the list of required modules, add its list
//      of exports to the list of resolved exports, add its imported names to
//      the list of imports.
//  4.4 Traverse the list of imported names to eliminate any found in the list
//      of exported names.
//  4.5 go to step 4.1
//  5. construct a new makefile for all required modules.
//
//  Currently works only with GNU nm

#include <ace/Log_Msg.h>

#include "SO_Group.h"

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  SO_Group group;

  ACE_DEBUG ((LM_DEBUG, "discovering libraries\n"));

  for (int i = 1; i < argc; group.add_executable (argv[i++]))
    {
      // No action.
    }

  ACE_DEBUG ((LM_DEBUG, "loading object modules\n"));
  group.load_modules ();
  group.list_libs ();

  ACE_DEBUG ((LM_DEBUG, "Starting analysis\n"));
  group.analize ();

  ACE_DEBUG ((LM_DEBUG, "Writing results\n"));
  group.write_results ();

  ACE_DEBUG ((LM_DEBUG, "Done.\n"));
  return 0;
}

