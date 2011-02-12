// $Id: Gateway.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#define ACE_BUILD_SVC_DLL

#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/Service_Config.h"
#include "ace/Signal.h"
#include "Config_Files.h"
#include "Event_Channel.h"
#include "Gateway.h"

class ACE_Svc_Export Gateway : public ACE_Service_Object
{
  // = TITLE
  //     Integrates the whole Gateway application.
  //
  // = DESCRIPTION
  //     This implementation uses the <Event_Channel> as the basis
  //     for the <Gateway> routing.
protected:
  // = Service configurator hooks.
  virtual int init (int argc, ACE_TCHAR *argv[]);
  // Perform initialization.

  virtual int fini (void);
  // Perform termination when unlinked dynamically.

  virtual int info (ACE_TCHAR **, size_t) const;
  // Return info about this service.

  // = Configuration methods.
  int parse_connection_config_file (void);
  // Parse the proxy configuration file.

  int parse_consumer_config_file (void);
  // Parse the consumer configuration file.

  // = Lifecycle management methods.
  int handle_input (ACE_HANDLE);
  // Shut down the Gateway when input comes in from the controlling
  // console.

  int handle_signal (int signum, siginfo_t * = 0, ucontext_t * = 0);
  // Shut down the Gateway when a signal arrives.

  Event_Channel event_channel_;
  // The Event Channel routes events from Supplier(s) to Consumer(s)
  // using <Supplier_Handler> and <Consumer_Handler> objects.

  Connection_Handler_Factory connection_handler_factory_;
  // Creates the appropriate type of <Connection_Handlers>.
};

int
Gateway::handle_signal (int signum, siginfo_t *, ucontext_t *)
{
  ACE_UNUSED_ARG (signum);

  // Shut down the main event loop.
  ACE_Reactor::end_event_loop ();
  return 0;
}

int
Gateway::handle_input (ACE_HANDLE h)
{
  char buf[BUFSIZ];
  // Consume the input...
  ACE_OS::read (h, buf, sizeof (buf));

  // Shut us down.
  return this->handle_signal ((int) h);
}

int
Gateway::init (int argc, ACE_TCHAR *argv[])
{
  // Parse the "command-line" arguments.
  Options::instance ()->parse_args (argc, argv);

  ACE_Sig_Set sig_set;
  sig_set.sig_add (SIGINT);
  sig_set.sig_add (SIGQUIT);

  // Register ourselves to receive signals so we can shut down
  // gracefully.

  if (ACE_Reactor::instance ()->register_handler (sig_set,
                                                  this) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("(%t) %p\n"),
                       ACE_TEXT ("register_handler")),
                      -1);

  // Register this handler to receive events on stdin.  We use this to
  // shutdown the Gateway gracefully.
  if (ACE_Event_Handler::register_stdin_handler (this,
                                                 ACE_Reactor::instance (),
                                                 ACE_Thread_Manager::instance ()) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("(%t) %p\n"),
                       ACE_TEXT ("register_stdin_handler")),
                      -1);

  // If this->performance_window_ > 0 start a timer.

  if (Options::instance ()->performance_window () > 0)
    {
      ACE_Time_Value const performance_time (Options::instance ()->performance_window ());
      if (ACE_Reactor::instance ()->schedule_timer
          (&this->event_channel_, 0,
           performance_time) == -1)
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT ("(%t) %p\n"),
                    ACE_TEXT ("schedule_timer")));
      else
        ACE_DEBUG ((LM_DEBUG,
                    ACE_TEXT ("starting timer for %d seconds...\n"),
                   Options::instance ()->performance_window ()));
    }

  // Are we running as a connector?
  if (Options::instance ()->enabled
      (Options::CONSUMER_CONNECTOR | Options::SUPPLIER_CONNECTOR))
    {
      // Parse the proxy configuration file.
      this->parse_connection_config_file ();

      // Parse the consumer config file and build the event forwarding
      // discriminator.
      this->parse_consumer_config_file ();
    }

  // Initialize the Event_Channel.
  return this->event_channel_.open ();
}

// This method is automatically called when the Gateway is shutdown.

int
Gateway::fini (void)
{
  // Remove the handler that receive events on stdin.  Otherwise, we
  // will crash on shutdown.
  ACE_Event_Handler::remove_stdin_handler (ACE_Reactor::instance (),
                                           ACE_Thread_Manager::instance ());

  // Close down the event channel.
  this->event_channel_.close ();

  // Need to make sure we cleanup this Singleton.
  delete Options::instance ();
  return 0;
}

// Returns information on the currently active service.

int
Gateway::info (ACE_TCHAR **strp, size_t length) const
{
  ACE_TCHAR buf[BUFSIZ];

  ACE_OS::strcpy
    (buf, ACE_TEXT ("Gateway daemon\t   # Application-level gateway\n"));

  if (*strp == 0 && (*strp = ACE_OS::strdup (buf)) == 0)
    return -1;
  else
    ACE_OS::strncpy (*strp, buf, length);
  return ACE_OS::strlen (buf);
}

// Parse and build the proxy table.

int
Gateway::parse_connection_config_file (void)
{
  // File that contains the proxy configuration information.
  Connection_Config_File_Parser connection_file;
  int file_empty = 1;
  int line_number = 0;

  if (connection_file.open (Options::instance ()->connection_config_file ()) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("(%t) %p\n"),
                       Options::instance ()->connection_config_file ()),
                      -1);

  // Keep track of the previous connection id to make sure the
  // connection config file isn't corrupted.
  int previous_connection_id = 0;

  // Read config file one line at a time.

  for (Connection_Config_Info pci;
       connection_file.read_entry (pci, line_number) != FPRT::RT_EOFILE;
       )
    {
      file_empty = 0;

      // First time in check.
      if (previous_connection_id == 0)
        {
          previous_connection_id = 1;

          if (pci.connection_id_ != 1)
            ACE_DEBUG ((LM_DEBUG,
                        ACE_TEXT ("(%t) warning, the first connection id should be 1 not %d\n"),
                        pci.connection_id_));
        }
      else if (previous_connection_id + 1 != pci.connection_id_)
        ACE_DEBUG ((LM_DEBUG,
                    ACE_TEXT ("(%t) warning, connection ids should keep increasing by 1 and %d + 1 != %d\n"),
                    previous_connection_id,
                    pci.connection_id_));

      // Update the last connection id to ensure that we monotonically
      // increase by 1.
      previous_connection_id = pci.connection_id_;

      if (Options::instance ()->enabled (Options::DEBUGGING))
        ACE_DEBUG ((LM_DEBUG,
                    ACE_TEXT ("(%t) conn id = %d, ")
                    ACE_TEXT ("host = %s, ")
                    ACE_TEXT ("remote port = %d, ")
                    ACE_TEXT ("proxy role = %c, ")
                    ACE_TEXT ("max retry timeout = %d, ")
                    ACE_TEXT ("local port = %d, ")
                    ACE_TEXT ("priority = %d\n"),
                    pci.connection_id_,
                    pci.host_,
                    pci.remote_port_,
                    pci.connection_role_,
                    pci.max_retry_timeout_,
                    pci.local_port_,
                    pci.priority_));

      pci.event_channel_ = &this->event_channel_;

      // Create the appropriate type of Proxy.
      Connection_Handler *connection_handler;

      ACE_ALLOCATOR_RETURN (connection_handler,
                            this->connection_handler_factory_.make_connection_handler (pci),
                            -1);

      // Bind the new Connection_Handler to the connection ID.
      this->event_channel_.bind_proxy (connection_handler);
    }

  // Keep track of the next available connection id, which is
  // necessary for Peers that connect with us, rather than vice versa.
  Options::instance ()->connection_id () = previous_connection_id + 1;

  if (file_empty)
    ACE_ERROR ((LM_WARNING,
               ACE_TEXT ("warning: connection connection_handler configuration file was empty\n")));
  return 0;
}

int
Gateway::parse_consumer_config_file (void)
{
  // File that contains the consumer event forwarding information.
  Consumer_Config_File_Parser consumer_file;
  int file_empty = 1;
  int line_number = 0;

  if (consumer_file.open (Options::instance ()->consumer_config_file ()) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("(%t) %p\n"),
                       Options::instance ()->consumer_config_file ()),
                      -1);

  // Read config file line at a time.
  for (Consumer_Config_Info cci_entry;
       consumer_file.read_entry (cci_entry, line_number) != FPRT::RT_EOFILE;
       )
    {
      file_empty = 0;

      if (Options::instance ()->enabled (Options::DEBUGGING))
        {
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT ("(%t) connection id = %d, payload = %d, ")
                      ACE_TEXT ("number of consumers = %d\n"),
                      cci_entry.connection_id_,
                      cci_entry.type_,
                      cci_entry.total_consumers_));

          for (int i = 0; i < cci_entry.total_consumers_; i++)
            ACE_DEBUG ((LM_DEBUG,
                        ACE_TEXT ("(%t) destination[%d] = %d\n"),
                        i,
                        cci_entry.consumers_[i]));
        }

      Consumer_Dispatch_Set *dispatch_set;
      ACE_NEW_RETURN (dispatch_set,
                      Consumer_Dispatch_Set,
                      -1);

      Event_Key event_addr (cci_entry.connection_id_,
                            cci_entry.type_);

      // Add the Consumers to the Dispatch_Set.
      for (int i = 0; i < cci_entry.total_consumers_; i++)
        {
          Connection_Handler *connection_handler = 0;

          // Lookup destination and add to Consumer_Dispatch_Set set
          // if found.
          if (this->event_channel_.find_proxy (cci_entry.consumers_[i],
                                               connection_handler) != -1)
            dispatch_set->insert (connection_handler);
          else
            ACE_ERROR ((LM_ERROR,
                        ACE_TEXT ("(%t) not found: destination[%d] = %d\n"),
                        i,
                        cci_entry.consumers_[i]));
        }

      this->event_channel_.subscribe (event_addr, dispatch_set);
    }

  if (file_empty)
    ACE_ERROR ((LM_WARNING,
               ACE_TEXT ("warning: consumer map configuration file was empty\n")));
  return 0;
}

// The following is a "Factory" used by the ACE_Service_Config and
// svc.conf file to dynamically initialize the state of the Gateway.

ACE_SVC_FACTORY_DEFINE (Gateway)

