// $Id: http_client.cpp 91670 2010-09-08 18:02:26Z johnnyw $

// ============================================================================
//
// = LIBRARY
//    apps/JAWS/clients/Caching
//
// = FILENAME
//    http_client.cpp
//
// = DESCRIPTION
//    This is a very simple client.  It accepts URLs from a prompt, and
//    will try to fetch them.  Also accepts shell escapes.
//
// = AUTHOR
//    James Hu
//
// ============================================================================

#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_ctype.h"
#include "http_handler.h"

int
ACE_TMAIN (int, ACE_TCHAR *[])
{
  // Present a command line.
  // * Accept a URL.
  //     Pass it to the HTTP_Connector.
  //     Connect.
  //     Report status.
  // * Accept shell escape character.

  char buf[BUFSIZ];

  ACE_DEBUG ((LM_DEBUG, "* "));

  while (ACE_OS::fgets (buf, sizeof (buf), stdin) != 0)
    {
      char *s = buf;

      // get rid of trailing '\n'
      int len = ACE_OS::strlen (s);

      if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = 0;

      while (ACE_OS::ace_isspace (*s))
        s++;

      if (*s == '!')
        {
          do
          s++;
          while (ACE_OS::ace_isspace (*s));

          // Shell command.
          if (ACE_OS::system (ACE_TEXT_CHAR_TO_TCHAR (s)) == -1)
            ACE_ERROR ((LM_ERROR, ACE_TEXT (" ! Error executing: %C\n"), s));
        }
      else if (ACE_OS::strncmp (s, "http://", 7) == 0)
        {
          // URL
          HTTP_Connector connector;
          connector.connect (s);
        }
      else
        ACE_ERROR ((LM_ERROR, ACE_TEXT (" ? I don't understand: %C\n"), s));

      ACE_ERROR ((LM_ERROR, ACE_TEXT ("* ")));
    }

  ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("\nBye!\n")));

  return 0;
}
