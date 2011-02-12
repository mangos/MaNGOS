// $Id: test.cpp 91671 2010-09-08 18:39:23Z johnnyw $

// Tests the generated perfect hash function.

// The -v option prints diagnostics as to whether a word is in the set
// or not.  Without -v the program is useful for timing.

#include "ace/OS_NS_string.h"
#include "ace/OS_NS_stdio.h"



static const int MAX_LEN = 80;

// Lookup function.
const char *in_word_set (const char *str, unsigned int len);

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  int verbose = argc > 1 && ACE_OS::strcmp (argv[1], ACE_TEXT("-v")) == 0 ? 1 : 0;
  char buf[MAX_LEN];

  while (ACE_OS::fgets (buf, sizeof buf, stdin) != 0)
    {
      size_t len = ACE_OS::strlen (buf) - 1;
      buf[len] = '\0';
      if (in_word_set (buf, len) && verbose)
        ACE_OS::printf ("in word set %s\n", buf);
      else if (verbose)
        ACE_OS::printf ("NOT in word set %s\n", buf);
    }

  return 0;
}
