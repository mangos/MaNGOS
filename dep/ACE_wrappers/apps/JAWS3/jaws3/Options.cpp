// $Id: Options.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "ace/OS_NS_stdlib.h"

#ifndef JAWS_BUILD_DLL
#define JAWS_BUILD_DLL
#endif /*JAWS_BUILD_DLL*/

#include "jaws3/Options.h"

JAWS_Options::JAWS_Options (void)
{
  this->cf_ = new JAWS_Config_File ("jaws.conf");
}

const char *
JAWS_Options::getenv (const char *key)
{
  const char *value = 0;
  if (this->cf_ == 0 || this->cf_->find (key, value) < 0)
    value = ACE_OS::getenv (key);

  return value;
}

