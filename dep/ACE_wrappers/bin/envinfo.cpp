// $Id: envinfo.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS_main.h"
#include "ace/OS_NS_sys_utsname.h"



int
ACE_TMAIN (int, ACE_TCHAR *[])
{
  ACE_DEBUG ((LM_INFO, ACE_TEXT ("ACE: %u.%u.%u\n"),
              ACE::major_version(),
              ACE::minor_version(),
              ACE::beta_version()));

  ACE_utsname uname;
  ACE_OS::uname(&uname);
#if defined (ACE_LACKS_UTSNAME_T)
  ACE_DEBUG ((LM_INFO,
              ACE_TEXT ("OS: %s %s\n"),
              uname.sysname,
              uname.release));
#else
  ACE_DEBUG ((LM_INFO,
              ACE_TEXT ("OS: %C %C\n"),
              uname.sysname,
              uname.release));
#endif

  ACE_DEBUG ((LM_INFO, ACE_TEXT ("Compiler: %s %u.%u\n"),
              ACE::compiler_name(),
              ACE::compiler_major_version(),
              ACE::compiler_minor_version(),
              ACE::compiler_beta_version()));

  return 0;
}

