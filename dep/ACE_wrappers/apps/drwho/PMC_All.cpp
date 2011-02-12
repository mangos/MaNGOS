// $Id: PMC_All.cpp 81993 2008-06-16 20:26:16Z sowayaa $

#include "global.h"
#include "Options.h"
#include "HT_Client.h"
#include "PMC_All.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_Memory.h"

// This function is pretty much a no-op that just sets up the
// appropriate lookup function to use.

int
PMC_All::encode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "in PMC_All::encode\n"));

  ACE_NEW_RETURN (this->ss,
                  HT_Client,
                  -1);

  SET_PACKET_TYPE (packet, Options::PROTO_ALL);

  char *buf_ptr = SKIP_PACKET_TYPE (packet);

  packet_length = buf_ptr - packet;
  return 1;
}

// This method is responsible for transforming the msg from the server
// back into a form usable by the client.  Note that it reads the
// REAL_NAME from the packet (since the server placed it there)...

int
PMC_All::decode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "in PMC_All::decode, packet_length = %d\n",
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
      // Skip over the LOGIN_NAME.

      char *login_name = cp;
      char *real_name = cp = (char *) ACE::strend (cp);

      for (cp = (char *) ACE::strend (cp);
           *(cp = this->handle_protocol_entries (cp, login_name, real_name)) != '\t';
           )
        continue;
    }

  return 1;
}

Protocol_Record *
PMC_All::insert_protocol_info (Protocol_Record &protocol_record)
{
  Protocol_Record *prp = PM_Client::insert_protocol_info (protocol_record);
  int length = ACE_OS::strlen (prp->set_real (ACE::strnew (protocol_record.get_real ())));

  if (length > this->max_key_length)
    this->max_key_length = length;

  return prp;
}

void
PMC_All::process (void)
{
  ACE_DEBUG ((LM_DEBUG,
              "remote users logged on\n"));
  PM_Client::process ();
}

PMC_All::PMC_All (void)
{
}
