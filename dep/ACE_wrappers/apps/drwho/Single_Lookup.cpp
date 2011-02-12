// $Id: Single_Lookup.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "Options.h"
#include "Single_Lookup.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS_Memory.h"

Single_Lookup::Single_Lookup (const char *usr_name)
{
  ACE_NEW (this->prp_,
           Protocol_Record (ACE::strnew (usr_name)));
}

Single_Lookup::~Single_Lookup (void)
{
  if (Options::get_opt (Options::DEBUGGING))
    ACE_DEBUG ((LM_DEBUG,
                "disposing Single_Lookup\n"));
}

Protocol_Record *
Single_Lookup::get_each_entry (void)
{
  return this->prp_;
}

Protocol_Record *
Single_Lookup::get_next_entry (void)
{
  return this->get_each_entry ();
}
