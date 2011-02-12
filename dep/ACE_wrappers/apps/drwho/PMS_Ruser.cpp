// $Id: PMS_Ruser.cpp 81993 2008-06-16 20:26:16Z sowayaa $

#include "Options.h"
#include "HT_Server.h"
#include "PMS_Ruser.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_pwd.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_Memory.h"
#include "ace/os_include/os_netdb.h"

// This function packs the located friends userids, plus the machines
// they are logged into (along with the inactive and active counts on
// each machine) into a buffer that is subsequently transmitted back
// to the client across the network.  Note that this function encodes
// the REAL_NAME of the user in the packet.

int
PMS_Ruser::encode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "in PMS_Ruser::encode"));

  Protocol_Record *prp;
  char *buf_ptr = packet;

  ACE_OS::sprintf (buf_ptr,
                   "Users   %d",
                   this->get_total_users ());
  buf_ptr += ACE_OS::strlen (buf_ptr) + 1;

  // We only send back info on hosts that we actually see.

  for (;
       (prp = this->get_next_friend ()) != 0;
       *buf_ptr++ = '\t')
    buf_ptr = this->handle_protocol_entries (ACE_OS::strecpy (buf_ptr,
                                                              prp->get_host ()),
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

// This function takes a packet received from the client and crusers
// the appropriate Protocol_Manager routine to build the local table
// of friends.

int
PMS_Ruser::decode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "in PMS_Ruser::decode, packet_length = %d\n",
                packet_length));

  if (*packet)
    Options::set_opt (Options::PRINT_LOGIN_NAME);

  ACE_NEW_RETURN (this->ss,
                  HT_Server,
                  -1);
  return 1;
}

Protocol_Record *
PMS_Ruser::insert_protocol_info (Protocol_Record &protocol_record)
{
  Drwho_Node *current_node = protocol_record.get_drwho_list ();
  Protocol_Record *prp = this->ss->insert (current_node->get_host_name (),
                                           MAXHOSTNAMELEN);
  Drwho_Node *np = this->get_drwho_node (ACE::strnnew (protocol_record.get_login (),
                                                       MAXUSERIDNAMELEN),
                                         prp->drwho_list_);

  if (Options::get_opt (Options::PRINT_LOGIN_NAME))
    np->set_real_name ("");
  else
    {
      passwd *pwent = ACE_OS::getpwnam (np->get_login_name ());
      char *cp =
        (char *) ACE_OS::strchr (np->set_real_name (pwent == 0
                                                    ? np->get_login_name ()
                                                    : ACE::strnew (pwent->pw_gecos)),
                                 ',');
      if (cp != 0)
        *cp = '\0';
    }

  if (current_node->get_idle_time () >= MAX_USER_TIMEOUT)
    np->inactive_count_++;
  else
    np->active_count_++;

  return prp;
}

char *
PMS_Ruser::handle_protocol_entries (char *buf_ptr,
                                    Drwho_Node *np)
{
  for (; np != 0; np = np->next_)
    {
      ACE_OS::sprintf (buf_ptr,
                       "%d %d ",
                       np->get_inactive_count (),
                       np->get_active_count ());
      buf_ptr += ACE_OS::strlen (buf_ptr);

      buf_ptr = ACE_OS::strecpy (ACE_OS::strecpy (buf_ptr,
                                                  np->get_login_name ()),
                                 np->get_real_name ());
    }

  return buf_ptr;
}

PMS_Ruser::PMS_Ruser (void)
{
}
