// $Id: PMC_Ruser.cpp 81993 2008-06-16 20:26:16Z sowayaa $

#include "global.h"
#include "Options.h"
#include "HT_Client.h"
#include "PMC_Ruser.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_Memory.h"
#include "ace/os_include/os_netdb.h"

// This function is pretty much a no-op that just sets up the
// appropriate lookup function to use.

int
PMC_Ruser::encode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "in PMC_Ruser::encode\n"));

  ACE_NEW_RETURN (this->ss,
                  HT_Client,
                  -1);

  SET_PACKET_TYPE (packet, Options::PROTO_RUSER);

  char *buf_ptr = SKIP_PACKET_TYPE (packet);

  *buf_ptr++ = char (Options::get_opt (Options::PRINT_LOGIN_NAME));

  packet_length = buf_ptr - packet;
  return 1;
}

// This method is responsible for transforming the msg from the server
// back into a form usable by the client.  Note that it reads the
// REAL_NAME from the packet (since the server placed it there)...

int
PMC_Ruser::decode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "in PMC_Ruser::decode, packet_length = %d\n",
                  packet_length));
      ACE_OS::write (ACE_STDERR, packet, packet_length);
      ACE_DEBUG ((LM_DEBUG,
                  "\n"));
    }
  char *cp = packet;
  int remote_users = 0;

  sscanf (cp,
          "Users   %d",
          &remote_users);

  this->increment_total_users (remote_users);

  for (cp = (char *) ACE::strend (cp);
       *cp != '\n';
       cp++)
    {
      char *host_name = cp;

      for (cp = (char *) ACE::strend (cp);
           *(cp = this->handle_protocol_entries (cp, host_name)) != '\t'; )
        continue;
    }

  return 1;
}

Protocol_Record *
PMC_Ruser::insert_protocol_info (Protocol_Record &protocol_record)
{
  Protocol_Record *prp = this->ss->insert (protocol_record.get_host (),
                                           MAXHOSTNAMELEN);
  Drwho_Node *current_node = protocol_record.get_drwho_list ();
  Drwho_Node *np = this->get_drwho_node (ACE::strnnew (current_node->get_login_name (),
                                                       MAXUSERIDNAMELEN),
                                         prp->drwho_list_);
  int length = ACE_OS::strlen (prp->get_host ());

  np->set_real_name (ACE::strnew (current_node->get_real_name ()));

  if (np->get_active_count () < current_node->get_active_count ())
    np->set_active_count (current_node->get_active_count ());
  if (np->get_inactive_count () < current_node->get_inactive_count())
    np->set_inactive_count (current_node->get_inactive_count ());

  if (length > this->max_key_length)
    this->max_key_length = length;

  return prp;
}

char *
PMC_Ruser::handle_protocol_entries (const char *cp,
                                    const char *host_name,
                                    const char *)
{
  static Protocol_Record protocol_record (1);
  Drwho_Node *current_node = protocol_record.get_drwho_list ();

  protocol_record.set_host (host_name);
  current_node->set_inactive_count (ACE_OS::atoi (cp));
  current_node->set_active_count (ACE_OS::atoi (cp = ACE_OS::strchr (cp, ' ') + 1));
  current_node->set_login_name (cp = ACE_OS::strchr (cp, ' ') + 1);
  current_node->set_real_name (cp = ACE_OS::strchr (cp, '\0') + 1);

  this->insert_protocol_info (protocol_record);

  return (char *) ACE::strend (cp);
}

void
PMC_Ruser::process (void)
{
  const char *(Drwho_Node::*get_name)(void);

  if (Options::get_opt (Options::PRINT_LOGIN_NAME))
    get_name = &Drwho_Node::get_login_name;
  else
    get_name = &Drwho_Node::get_real_name;

  for (Protocol_Record *prp;
       (prp = this->Protocol_Manager::get_each_friend ()) != 0;
       )
    {
      ACE_DEBUG ((LM_DEBUG,
                  "%-*s ",
                  this->max_key_length,
                  prp->get_host ()));

      for (Drwho_Node *np = prp->get_drwho_list (); ;)
        {
          ACE_DEBUG ((LM_DEBUG,
                      "%s",
                      (np->*get_name) ()));

          if (np->get_inactive_count () != 0)
            {
              if (np->get_active_count () != 0)
                ACE_DEBUG ((LM_DEBUG,
                            "*(%d)",
                            np->get_active_count ()));
            }
          else if (np->get_active_count () > 1)
            ACE_DEBUG ((LM_DEBUG,
                        "*(%d)",
                        np->get_active_count ()));
          else if (np->get_active_count () == 1)
            ACE_DEBUG ((LM_DEBUG,
                        "*"));

          np = np->next_;
          if (np == 0)
            break;
          else if (Options::get_opt (Options::PRINT_LOGIN_NAME))
            ACE_DEBUG ((LM_DEBUG,
                        " "));
          else
            ACE_DEBUG ((LM_DEBUG,
                        ", "));
        }

      ACE_DEBUG ((LM_DEBUG,
                  "\n"));
    }
}

PMC_Ruser::PMC_Ruser (void)
{
}
