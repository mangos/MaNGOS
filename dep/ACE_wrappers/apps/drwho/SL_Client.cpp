// $Id: SL_Client.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "Options.h"
#include "SL_Client.h"

SL_Client::SL_Client (const char *usr_name)
  : Single_Lookup (usr_name)
{
}

Protocol_Record *
SL_Client::insert (const char *, int)
{
  return this->prp_;
}
