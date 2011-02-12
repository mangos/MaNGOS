// $Id: PMC_Usr.cpp 81993 2008-06-16 20:26:16Z sowayaa $

#include "Options.h"
#include "SL_Client.h"
#include "PMC_Usr.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_Memory.h"

int
PMC_Usr::encode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "in PMC_Usr::encode"));

  ACE_NEW_RETURN (this->ss,
                  SL_Client (this->usr_name),
                  -1);

  SET_PACKET_TYPE (packet, Options::PROTO_USR);

  char *buf_ptr = SKIP_PACKET_TYPE (packet);

  buf_ptr = ACE_OS::strecpy (buf_ptr,
                             this->get_next_friend ()->get_login ());

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

// This method is responsible for transforming the msg from the server
// back into a form usable by the client.

int
PMC_Usr::decode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "in PMC_Usr::decode, packet_length = %d\n",
                  packet_length));
      ACE_OS::write (ACE_STDERR, packet, packet_length);
      ACE_DEBUG ((LM_DEBUG,
                  "\n"));
    }

  char *cp = packet;

  if (*cp != '\n')
    {
      char *login_name = cp;

      for (cp = (char *) ACE::strend (cp);
           *(cp = this->handle_protocol_entries (cp, login_name)) != '\t';
           )
        continue;
    }

  return 1;
}

void
PMC_Usr::process (void)
{
  Protocol_Record *prp = this->get_each_friend ();
  Drwho_Node *np  = prp->get_drwho_list ();

  if (np == 0)
    ACE_DEBUG ((LM_DEBUG,
                "<unknown>"));
  else
    {
      // First try to get a login session that is active...

      for (; np != 0; np = np->next_)
        if (np->active_count_ > 0)
          {
            ACE_DEBUG ((LM_DEBUG,
                        "%s ",
                        np->get_host_name ()));

            if (Options::get_opt (Options::USE_VERBOSE_FORMAT) == 0)
              return;
          }

      for (np = prp->get_drwho_list ();
           np != 0;
           np = np->next_)
        if (np->active_count_ == 0)
          {
            ACE_DEBUG ((LM_DEBUG,
                        "%s ",
                        np->get_host_name ()));

            if (Options::get_opt (Options::USE_VERBOSE_FORMAT) == 0)
              return;
          }
    }
}

PMC_Usr::PMC_Usr (char *u_name)
  : usr_name (u_name)
{
}
