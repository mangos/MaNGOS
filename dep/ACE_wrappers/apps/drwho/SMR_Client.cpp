// $Id: SMR_Client.cpp 91813 2010-09-17 07:52:52Z johnnyw $

#include "Options.h"
#include "PMC_All.h"
#include "PMC_Flo.h"
#include "PMC_Usr.h"
#include "PMC_Ruser.h"
#include "SMR_Client.h"
#include "ace/Log_Msg.h"

SMR_Client::SMR_Client (short port_number)
{
  if (CM_Client::open (port_number) < 0)
    ACE_ERROR ((LM_ERROR,
                "%p\n%a",
                Options::program_name,
                1));
}

SMR_Client::~SMR_Client (void)
{
}
