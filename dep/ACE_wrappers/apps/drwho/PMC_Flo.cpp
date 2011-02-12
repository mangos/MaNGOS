// $Id: PMC_Flo.cpp 81993 2008-06-16 20:26:16Z sowayaa $

#include "Options.h"
#include "BS_Client.h"
#include "PMC_Flo.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_Memory.h"

// This function "encodes" a list of friends by putting the userid's
// in a contiguous block.  This block can then be transmitted over to
// the network to servers on other subnets.  Several things are added
// to make decoding easier on the other end:
//
// * A count of the number of friends is prepended (assumption: there
//   are no more than 9999999 friends... ;-))
// * The login userids are separated by a single space. */

int
PMC_Flo::encode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "in PMC_Flo::encode"));

  ACE_NEW_RETURN (this->ss,
                  BS_Client,
                  -1);

  SET_PACKET_TYPE (packet, Options::PROTO_FLO);
  char *buf_ptr = SKIP_PACKET_TYPE (packet);

  ACE_OS::sprintf (buf_ptr,
                   "%d",
                   this->friend_count ());

  buf_ptr += MAXUSERIDNAMELEN;

  // Iterate through all the friends, copying them into the packet
  // buffer.

  for (Protocol_Record *prp; (prp = this->get_next_friend ()) != 0; )
    buf_ptr = ACE_OS::strecpy (buf_ptr,
                               prp->get_login ());

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
PMC_Flo::decode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "in PMC_Flo::decode, packet_length = %d\n",
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
      char *login_name = cp;

      for (cp = (char *) ACE::strend (cp);
           *(cp = this->handle_protocol_entries (cp, login_name)) != '\t';
          )
        continue;
    }

  return 1;
}

Protocol_Record *
PMC_Flo::insert_protocol_info (Protocol_Record &protocol_record)
{
  Protocol_Record *prp = PM_Client::insert_protocol_info (protocol_record);
  int length = ACE_OS::strlen (prp->get_real ());

  if (length > this->max_key_length)
    this->max_key_length = length;

  return prp;
}

void
PMC_Flo::process (void)
{
  ACE_DEBUG ((LM_DEBUG,
              "remote friends logged on\n"));
  PM_Client::process ();
}

PMC_Flo::PMC_Flo (void)
{
}
