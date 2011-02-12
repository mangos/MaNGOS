// $Id: PMS_All.cpp 91813 2010-09-17 07:52:52Z johnnyw $

#include "Options.h"
#include "HT_Server.h"
#include "PMS_All.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_pwd.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_Memory.h"

// This function packs the located friends userids, plus the machines
// they are logged into (along with the inactive and active counts on
// each machine) into a buffer that is subsequently transmitted back
// to the client across the network.  Note that this function encodes
// the REAL_NAME of the user in the packet.

int
PMS_All::encode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "in PMS_All::encode"));

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
    buf_ptr =
      this->handle_protocol_entries (ACE_OS::strecpy
                                       (ACE_OS::strecpy (buf_ptr,
                                                         prp->get_login ()),
                                        prp->get_real ()),
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
PMS_All::decode (char *, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "in PMS_All::decode, packet_length = %d\n",
                packet_length));

  ACE_NEW_RETURN (this->ss,
                  HT_Server,
                  -1);
  return 1;
}

// We only want the user's real name, not the gecos junk after the
// first leading ','.  However, if the real-name is not in the
// password file, just return the login name instead.

Protocol_Record *
PMS_All::insert_protocol_info (Protocol_Record &protocol_record)
{
  Protocol_Record *prp = PM_Server::insert_protocol_info (protocol_record);
  passwd *pwent = ACE_OS::getpwnam (prp->get_login ());
  char *cp = (char *) ACE_OS::strchr (prp->set_real
                                      (pwent == 0
                                       ? prp->get_login () :
                                       ACE::strnew (pwent->pw_gecos)),
                                      ',');
  if (cp != 0)
    *cp = '\0';

  return prp;
}

PMS_All::PMS_All (void)
{
}
