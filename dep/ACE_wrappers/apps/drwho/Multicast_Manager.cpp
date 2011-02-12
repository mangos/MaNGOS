// $Id: Multicast_Manager.cpp 80826 2008-03-04 14:51:23Z wotte $

#include "Multicast_Manager.h"
#include "ace/Mem_Map.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_arpa_inet.h"
#include "ace/OS_NS_netdb.h"
#include "ace/OS_Memory.h"
#include "ace/OS_NS_ctype.h"

// Initialize all the static member vars.
int Multicast_Manager::received_host_count = 0;
Host_Elem *Multicast_Manager::drwho_list = 0;
Host_Elem *Multicast_Manager::current_ptr = 0;

// Names of hosts to query for friend info.
const char *Multicast_Manager::host_names[] =
{
  "tango.cs.wustl.edu",
  0 // The NULL entry...
};

void
Multicast_Manager::insert_default_hosts (void)
{
  // Enter the static list of hosts into the dynamic table!

  for (const char **np = host_names;
       *np != 0;
       np++)
    Multicast_Manager::add_host (*np);
}

// Inserts all the names in FILENAME into the list of hosts to
// contact.

int
Multicast_Manager::insert_hosts_from_file (const char *filename)
{
  //FUZZ: disable check_for_lack_ACE_OS
  ACE_Mem_Map mmap (filename);
  //FUZZ: enable check_for_lack_ACE_OS

  char *host_ptr = (char *) mmap.addr ();

  if (host_ptr == 0)
    return -1;
  else
    {
      for (char *end_ptr = host_ptr + mmap.size ();
           host_ptr < end_ptr;
           )
        {
          Multicast_Manager::add_host (host_ptr);

          while (*host_ptr != '\n')
            host_ptr++;

          *host_ptr++ = '\0';
        }

      return 0;
    }
}

// Returns the IP host address for the next unexamined host in the
// list.  If no more unexamined hosts remain a 0 is returned, else a
// 1.

int
Multicast_Manager::get_next_host_addr (in_addr &host_addr)
{
  for (Multicast_Manager::current_ptr = Multicast_Manager::current_ptr == 0 ? Multicast_Manager::drwho_list : Multicast_Manager::current_ptr->next;

       Multicast_Manager::current_ptr != 0;
       Multicast_Manager::current_ptr = Multicast_Manager::current_ptr->next)
    {
      const char *host_name = Multicast_Manager::current_ptr->host_name;
      hostent *hp = Multicast_Manager::get_host_entry (host_name);

      if (hp == 0)
        {
          ACE_ERROR ((LM_ERROR,
                      "%s: host unknown.\n",
                      host_name));
          continue;
        }

      Multicast_Manager::received_host_count++;
      ACE_OS::memcpy (&host_addr,
                      hp->h_addr,
                      sizeof host_addr);
      ACE_OS::memcpy (&Multicast_Manager::current_ptr->host_addr,
                      hp->h_addr,
                      sizeof host_addr);
      return 1;
    }

  return 0;
}

// This function attempts to get the internet address for either a
// hostname or hostnumber.  The function makes the simplifying
// assumption that hostnames begin with an alphabetic character!

hostent *
Multicast_Manager::get_host_entry (const char *host)
{
  static hostent host_entry;
  hostent *hp;

  if (ACE_OS::ace_isdigit (*host)) // IP address.
    {
      u_long ia = ACE_OS::inet_addr (host);

      if (ia == (u_long) -1)
        hp = 0;
      else
        hp = ACE_OS::gethostbyaddr ((char *) &ia,
                                    sizeof ia,
                                    AF_INET);
    }
  else
    // Host name.
    hp = ACE_OS::gethostbyname (host);


  return hp == 0 ? 0 : (hostent *) ACE_OS::memcpy (&host_entry, hp, sizeof *hp);
}

// Adds an additional new host to the list of host machines.

void
Multicast_Manager::add_host (const char *host_name)
{
  ACE_NEW (Multicast_Manager::drwho_list,
           Host_Elem (host_name,
                      Multicast_Manager::drwho_list));
}

void
Multicast_Manager::checkoff_host (in_addr host_addr)
{
  for (Host_Elem *tmp = Multicast_Manager::drwho_list;
       tmp != 0;
       tmp = tmp->next)
    if (ACE_OS::memcmp (&tmp->host_addr.s_addr,
                        &host_addr.s_addr,
                        sizeof host_addr.s_addr) == 0)
      {
        tmp->checked_off = 1;
        Multicast_Manager::received_host_count--;
        return;
      }
}

int
Multicast_Manager::get_next_non_responding_host (const char *&host_name)
{
  for (Multicast_Manager::current_ptr = Multicast_Manager::current_ptr == 0 ? Multicast_Manager::drwho_list : Multicast_Manager::current_ptr->next;
       Multicast_Manager::current_ptr != 0;
       Multicast_Manager::current_ptr = Multicast_Manager::current_ptr->next)
    if (Multicast_Manager::current_ptr->checked_off == 0)
      {
        host_name = Multicast_Manager::current_ptr->host_name;
        return 1;
      }

  return 0;
}

Host_Elem::Host_Elem (const char *h_name,
                      Host_Elem *n)
  : host_name (h_name),
    checked_off (0),
    next (n)
{
}

int
Multicast_Manager::outstanding_hosts_remain (void)
{
  return Multicast_Manager::received_host_count > 0;
}
