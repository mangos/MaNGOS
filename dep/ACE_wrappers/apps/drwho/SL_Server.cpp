// $Id: SL_Server.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "global.h"
#include "SL_Server.h"
#include "ace/OS_NS_string.h"

SL_Server::SL_Server (const char *usr_name)
  : Single_Lookup (usr_name)
{
}

Protocol_Record *
SL_Server::get_each_entry (void)
{
  Protocol_Record *prp = Single_Lookup::get_each_entry ();
  return prp->get_drwho_list () == 0 ? 0 : prp;
}

Protocol_Record *
SL_Server::insert (const char *key_name, int max_len)
{
  return ACE_OS::strncmp (key_name,
                          this->prp_->get_login (),
                          max_len) == 0 ? this->prp_ : 0;
}

