// $Id: Options.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#define ACE_BUILD_SVC_DLL

#include "ace/Get_Opt.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_strings.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_Memory.h"
#include "Options.h"

// Static initialization.
Options *Options::instance_ = 0;

void
Options::print_usage_and_die (void)
{
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("%n [-a {C|S}:acceptor-port] [-c {C|S}:connector-port] [-C connection-id] [-h gateway-host] [-q max-queue-size] [-t timeout] [-v]\n")));
  ACE_OS::exit (1);
}

Options::Options (void)
  : options_ (0),
    supplier_acceptor_port_ (DEFAULT_PEER_SUPPLIER_PORT),
    consumer_acceptor_port_ (DEFAULT_PEER_CONSUMER_PORT),
    supplier_connector_port_ (DEFAULT_GATEWAY_SUPPLIER_PORT),
    consumer_connector_port_ (DEFAULT_GATEWAY_CONSUMER_PORT),
    connector_host_ (ACE_DEFAULT_SERVER_HOST),
    timeout_ (0),
    max_queue_size_ (MAX_QUEUE_SIZE),
    connection_id_ (0)
{
  char *timeout = ACE_OS::getenv ("TIMEOUT");

  if (timeout == 0)
    this->timeout_ = Options::DEFAULT_TIMEOUT;
  else
    this->timeout_ = ACE_OS::atoi (timeout);
}

Options *
Options::instance (void)
{
  if (Options::instance_ == 0)
    ACE_NEW_RETURN (Options::instance_, Options, 0);

  return Options::instance_;
}

long
Options::timeout (void) const
{
  return this->timeout_;
}

CONNECTION_ID &
Options::connection_id (void)
{
  return this->connection_id_;
}

long
Options::max_queue_size (void) const
{
  return this->max_queue_size_;
}

u_short
Options::consumer_acceptor_port (void) const
{
  return this->consumer_acceptor_port_;
}

u_short
Options::supplier_acceptor_port (void) const
{
  return this->supplier_acceptor_port_;
}

u_short
Options::consumer_connector_port (void) const
{
  return this->consumer_connector_port_;
}

u_short
Options::supplier_connector_port (void) const
{
  return this->supplier_connector_port_;
}

const ACE_TCHAR *
Options::connector_host (void) const
{
  return this->connector_host_;
}

int
Options::enabled (int option) const
{
  return ACE_BIT_ENABLED (this->options_, option);
}

void
Options::parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opt (argc, argv, ACE_TEXT ("a:c:C:h:m:t:v"), 0);

  for (int c; (c = get_opt ()) != -1; )
    {
      switch (c)
        {
        case 'a':
          {
            // Become an Acceptor.

            for (ACE_TCHAR *flag = ACE_OS::strtok (get_opt.opt_arg (),
                                                   ACE_TEXT ("|"));
                 flag != 0;
                 flag = ACE_OS::strtok (0, ACE_TEXT ("|")))
              if (ACE_OS::strncasecmp (flag, ACE_TEXT ("C"), 1) == 0)
                {
                  ACE_SET_BITS (this->options_,
                                Options::CONSUMER_ACCEPTOR);
                  if (ACE_OS::strlen (flag) > 1)
                    // Set the Consumer Acceptor port number.
                    this->consumer_acceptor_port_ = ACE_OS::atoi (flag + 2);
                }
              else if (ACE_OS::strncasecmp (flag, ACE_TEXT ("S"), 1) == 0)
                {
                  ACE_SET_BITS (this->options_,
                                Options::SUPPLIER_ACCEPTOR);
                  if (ACE_OS::strlen (flag) > 1)
                    // Set the Supplier Acceptor port number.
                    this->supplier_acceptor_port_ = ACE_OS::atoi (flag + 2);
                }
          }
          break;
          /* NOTREACHED */
        case 'c':
          {
            // Become a Connector.

            for (ACE_TCHAR *flag = ACE_OS::strtok (get_opt.opt_arg (),
                                                   ACE_TEXT ("|"));
                 flag != 0;
                 flag = ACE_OS::strtok (0, ACE_TEXT ("|")))
              if (ACE_OS::strncasecmp (flag, ACE_TEXT ("C"), 1) == 0)
                {
                  ACE_SET_BITS (this->options_,
                                Options::CONSUMER_CONNECTOR);
                  if (ACE_OS::strlen (flag) > 1)
                    // Set the Consumer Connector port number.
                    this->consumer_connector_port_ = ACE_OS::atoi (flag + 2);
                }
              else if (ACE_OS::strncasecmp (flag, ACE_TEXT ("S"), 1) == 0)
                {
                  ACE_SET_BITS (this->options_,
                                Options::SUPPLIER_CONNECTOR);
                  if (ACE_OS::strlen (flag) > 1)
                    // Set the Supplier Connector port number.
                    this->supplier_connector_port_ = ACE_OS::atoi (flag + 2);
                }
          }
          break;
          /* NOTREACHED */
        case 'C':
          this->connection_id_ = ACE_OS::atoi (get_opt.opt_arg ());
          break;
          /* NOTREACHED */
        case 'h':
          // connector host
          this->connector_host_ = get_opt.opt_arg ();
          break;
          /* NOTREACHED */
        case 'm':
          // max queue size.
          this->max_queue_size_ = ACE_OS::atoi (get_opt.opt_arg ());
          break;
          /* NOTREACHED */
        case 't':
          // Timeout
          this->timeout_ = ACE_OS::atoi (get_opt.opt_arg ());
          break;
          /* NOTREACHED */
        case 'v':
          // Verbose mode.
          ACE_SET_BITS (this->options_, Options::VERBOSE);
          break;
          /* NOTREACHED */
        default:
          this->print_usage_and_die ();
          /* NOTREACHED */
        }
    }
}

