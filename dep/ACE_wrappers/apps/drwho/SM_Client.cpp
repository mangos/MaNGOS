// $Id: SM_Client.cpp 91813 2010-09-17 07:52:52Z johnnyw $

#include "Options.h"
#include "PMC_All.h"
#include "PMC_Flo.h"
#include "PMC_Usr.h"
#include "PMC_Ruser.h"
#include "SM_Client.h"
#include "ace/Log_Msg.h"
#include "ace/OS_Memory.h"

// Call-back function that invokes the appropriate decode function.

int
SM_Client::demux (char *packet,
                  int &packet_length)
{
  if (this->pm_client->decode (packet, packet_length) < 0)
    return -1;
  else
    return 1;
}

// Call-back function that invokes the appropriate encode function.

int
SM_Client::mux (char *packet, int &packet_length)
{
  switch (Options::protocol_type)
    {
    case Options::PROTO_USR:
      ACE_NEW_RETURN (this->pm_client,
                      PMC_Usr (Options::user_name),
                      -1);
      break;
    case Options::PROTO_ALL:
      ACE_NEW_RETURN (this->pm_client,
                      PMC_All,
                      -1);
      break;
    case Options::PROTO_FLO:
      ACE_NEW_RETURN (this->pm_client,
                      PMC_Flo,
                      -1);
      break;
    case Options::PROTO_RUSER:
      ACE_NEW_RETURN (this->pm_client,
                      PMC_Ruser,
                      -1);
      break;
    default:
      ACE_DEBUG ((LM_DEBUG,
                  "%s: bad protocol\n",
                  Options::program_name));
      return -1;
    }

  if (this->pm_client->encode (packet, packet_length) < 0)
    return -1;
  return 1;
}

SM_Client::SM_Client (void)
{
}

SM_Client::~SM_Client (void)
{
}

void
SM_Client::process (void)
{
  this->pm_client->process ();
}
