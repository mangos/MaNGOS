// $Id: SM_Server.cpp 91813 2010-09-17 07:52:52Z johnnyw $

#include "Options.h"
#include "PMS_All.h"
#include "PMS_Flo.h"
#include "PMS_Usr.h"
#include "PMS_Ruser.h"
#include "SM_Server.h"
#include "ace/Log_Msg.h"
#include "ace/OS_Memory.h"

int
SM_Server::demux (char *packet, int &packet_length)
{
  switch (GET_PACKET_TYPE (packet))
    {
    case Options::PROTO_USR:
      ACE_NEW_RETURN (this->pm_server,
                      PMS_Usr,
                      -1);
      break;
    case Options::PROTO_ALL:
      ACE_NEW_RETURN (this->pm_server,
                      PMS_All,
                      -1);
      break;
    case Options::PROTO_FLO:
      ACE_NEW_RETURN (this->pm_server,
                      PMS_Flo,
                      -1);
      break;
    case Options::PROTO_RUSER:
      ACE_NEW_RETURN (this->pm_server,
                      PMS_Ruser,
                      -1);
      break;
    default:
      ACE_DEBUG ((LM_DEBUG,
                  "%s: bad protocol\n",
                  Options::program_name));
      return -1;
    }

  packet_length = SUBTRACT_PACKET_TYPE (packet_length);

  if (pm_server->decode (SKIP_PACKET_TYPE (packet),
                         packet_length) < 0)
    return -1;

  if (pm_server->process () < 0)
    return -1;

  return 1;
}

int
SM_Server::mux (char *packet,
                int &packet_length)
{
  return pm_server->encode (packet, packet_length);
}

SM_Server::SM_Server (void)
{
}

SM_Server::~SM_Server (void)
{
}
