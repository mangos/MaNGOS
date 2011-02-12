// $Id: PMS_Flo.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "Options.h"
#include "BS_Server.h"
#include "PMS_Flo.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_Memory.h"

// This function packs the located friends userids, plus the machines
// they are logged into (along with the inactive and active counts on
// each machine) into a buffer that is subsequently transmitted back
// to the client across the network.

int
PMS_Flo::encode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "in PMS_Flo::encode"));

  Protocol_Record *prp;
  char *buf_ptr = packet;

  ACE_OS::sprintf (buf_ptr,
                   "Users   %d",
                   this->get_total_users ());
  buf_ptr += ACE_OS::strlen (buf_ptr) + 1;

  // We only send back info on friends that we actually see logged in.

  for (;
       (prp = this->get_next_friend ()) != 0;
       *buf_ptr++ = '\t')
    buf_ptr = this->handle_protocol_entries (ACE_OS::strecpy (buf_ptr,
                                                              prp->get_login ()),
                                             prp->get_drwho_list ());
  *buf_ptr++ = '\n';
  packet_length = buf_ptr - packet;

  if (Options::get_opt (Options::DEBUGGING) != 0)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "packet_length = %d\n",
                  packet_length));
      ACE_OS::write (ACE_STDERR, packet, packet_length);
      ACE_DEBUG ((LM_DEBUG,
                  "\n"));
    }

  return 1;
}

// This function takes a packet received from the client and calls the
// appropriate Protocol_Manager routine to build the local table of
// friends.

int
PMS_Flo::decode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "in PMS_Flo::decode, packet_length = %d\n",
                packet_length));

  ACE_NEW_RETURN (this->ss,
                  BS_Server (packet),
                  -1);
  return 1;
}

PMS_Flo::PMS_Flo (void)
{
}
