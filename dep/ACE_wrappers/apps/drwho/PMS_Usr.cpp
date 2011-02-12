// $Id: PMS_Usr.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "Options.h"
#include "SL_Server.h"
#include "PMS_Usr.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_Memory.h"

// This function "encodes" a list of friends by putting the userid's in
// a contiguous block.  This block can then be transmitted over to the
// network to servers on other subnets.  Several things are added to
// make decoding easier on the other end:
//
// * A count of the number of friends is prepended (assumption: there
//   are no more than 9999999 friends... ;-))
// * The login userids are separated by a single space. */

int
PMS_Usr::encode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    ACE_DEBUG ((LM_DEBUG,
                "in PMS_Usr::encode"));

  char *buf_ptr = packet;

  // We only send back info on friend that is actually logged in.

  Protocol_Record *prp = this->get_next_friend ();

  if (prp)
    {
      buf_ptr = this->handle_protocol_entries (ACE_OS::strecpy (buf_ptr,
                                                                prp->get_login ()),
                                               prp->get_drwho_list ());
      *buf_ptr++ = '\t';
    }

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
PMS_Usr::decode (char *packet, int &packet_length)
{
  if (Options::get_opt (Options::DEBUGGING) != 0)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "in PMS_Usr::decode, packet_length = %d\n",
                  packet_length));
      ACE_OS::write (ACE_STDERR, packet, packet_length);
      ACE_DEBUG ((LM_DEBUG,
                  "\n"));
    }

  ACE_NEW_RETURN (this->ss,
                  SL_Server (packet),
                  -1);
  return 1;
}

PMS_Usr::PMS_Usr (void)
{
}
