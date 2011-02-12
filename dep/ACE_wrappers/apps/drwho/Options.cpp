// $Id: Options.cpp 82722 2008-09-16 09:28:33Z johnnyw $

#include "ace/OS_NS_stdlib.h"
#include "ace/Get_Opt.h"
#include "ace/Log_Msg.h"
#include "Options.h"
#include "Multicast_Manager.h"

// Initialize all the static variables.

// Contains bit-mask for options.
u_int Options::option_word = 0;

// Which protocol are we using?
Options::Protocol_Types Options::protocol_type = Options::PROTO_FLO;

// User name for quick lookups.
char *Options::user_name = 0;

// Port number for client/server.
short Options::port_number = PORT_NUMBER;

// Maximum time the client waits for servers to timeout.
int Options::max_server_timeout = 5;

// Name of the program.
char *Options::program_name;

// Default name of file that stores friend info.
const char *Options::friend_file = FRIEND_FILE;

void
Options::print_usage_and_die (int long_msg)
{
  ACE_DEBUG ((LM_DEBUG,
              "usage: %s %s",
              program_name,
              long_msg
              ? "\n"
              "-?\tprints a short usage message\n"
              "-A\tappend the following hostname to the list of predefined hostnames.\n"
              "-a\treturn information on *all* users remotely logged in (uses yp passwd).\n"
              "-b\trun the server in the background (i.e., as a daemon).\n"
              "-d\tturn on debugging.\n"
              "-F\tuse the following file contents to initialize the host list.\n"
              "-f\tuse the following file contents to initialize the friends database.\n"
              "-H\tuse the following hostname as part of the new list of hostnames.\n"
              "\t(this option overwrites the existing default names).\n"
              "-h\tprint a long usage message.\n"
              "-L\tprint the login name rather than the real name (which is the default).\n"
              "-l\tprint information in long format (works for all protocols).\n"
              "-p\tset the port number (server must correspond).\n"
              "-r\tdo the remote lookups (i.e., local operations are the default).\n"
              "-R\tprint info using the rusers format.\n"
              "-s\tsort the output by login name.\n"
              "-S\tsort the output by real name.\n"
              "-t\tset the amount of time we wait for servers to timeout.\n"
              "-w\treturn information on just one user.\n"
              : "[-?haAbdfFHhLlpRrtw]\n"));
  ACE_OS::exit (1);
}

void
Options::set_opt (Option_Types opt)
{
  Options::option_word |= opt;
}

int
Options::get_opt (Option_Types opt)
{
  return (Options::option_word & opt) != 0;
}

void
Options::set_options (int argc, char *argv[])
{
  int c;
  int add_default_hosts = 1;

  Options::program_name = argv[0];

  //FUZZ: disable check_for_lack_ACE_OS
  ACE_Get_Opt getopt (argc, argv, ACE_TEXT("?aA:bdF:f:hH:Llp:rRsSt:w:"));

  while ((c = getopt ()) != -1)
    {
  //FUZZ: endable check_for_lack_ACE_OS
      switch (c)
        {
        case '?':
          Options::print_usage_and_die (0);
          /* NOTREACHED */
        case 'A':
          Multicast_Manager::add_host (getopt.opt_arg ());
          break;
        case 'a':
          Options::protocol_type = PROTO_ALL;
          break;
        case 'b':
          Options::set_opt (Options::BE_A_DAEMON);
          break;
        case 'd':
          Options::set_opt (Options::DEBUGGING);
          break;
        case 'f':
          Options::friend_file = getopt.opt_arg ();
          break;
        case 'F':
          if (Multicast_Manager::insert_hosts_from_file (getopt.opt_arg ()) < 0)
            ACE_DEBUG ((LM_DEBUG,
                        "%p%a\n",
                        Options::program_name,
                        1));
          add_default_hosts = 0;
          break;
        case 'H':
          Multicast_Manager::add_host (getopt.opt_arg ());
          add_default_hosts = 0;
          break;
        case 'h':
          Options::print_usage_and_die (1);
          /* NOTREACHED */
        case 'L':
          Options::set_opt (Options::PRINT_LOGIN_NAME);
          break;
        case 'l':
          Options::set_opt (Options::USE_VERBOSE_FORMAT);
          break;
        case 'p':
          Options::port_number = ACE_OS::atoi (getopt.opt_arg ());
          break;
        case 'R':
          Options::protocol_type = PROTO_RUSER;
          break;
        case 'r':
          Options::set_opt (Options::REMOTE_USAGE);
          break;
        case 's':
          Options::set_opt (Options::SORT_BY_LOGIN_NAME);
          break;
        case 'S':
          Options::set_opt (Options::SORT_BY_REAL_NAME);
          break;
        case 't':
          Options::max_server_timeout = ACE_OS::atoi (getopt.opt_arg ());
          break;
        case 'w':
          Options::user_name = getopt.opt_arg ();
          Options::protocol_type = PROTO_USR;
          break;
        default:
          break;
        }
    }

  if (Options::get_opt (Options::REMOTE_USAGE) && add_default_hosts)
    Multicast_Manager::insert_default_hosts ();
}
