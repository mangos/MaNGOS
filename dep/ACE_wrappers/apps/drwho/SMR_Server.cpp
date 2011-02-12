// $Id: SMR_Server.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "Options.h"
#include "SMR_Server.h"
#include "ace/Log_Msg.h"

SMR_Server::SMR_Server (short port_number)
{
  if (CM_Server::open (port_number) < 0)
    ACE_ERROR ((LM_ERROR,
                "%p\n%a",
                Options::program_name,
                1));
}

SMR_Server::~SMR_Server (void)
{
}
