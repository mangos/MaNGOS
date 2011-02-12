// $Id: TS_Server_Handler.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "ace/SString.h"
#include "ace/Containers.h"
#include "ace/Get_Opt.h"
#include "TS_Server_Handler.h"
#include "ace/OS_NS_time.h"
#include "ace/Signal.h"



int
ACE_TS_Server_Acceptor::parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_TRACE ("ACE_TS_Server_Acceptor::parse_args");

  int service_port = ACE_DEFAULT_SERVER_PORT;

  ACE_LOG_MSG->open (ACE_TEXT ("Time Service"));

  ACE_Get_Opt get_opt (argc, argv, ACE_TEXT ("p:"), 0);

  for (int c; (c = get_opt ()) != -1; )
    {
      switch (c)
        {
        case 'p':
          service_port = ACE_OS::atoi (get_opt.opt_arg ());
          break;
        default:
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT ("%n:\n[-p server-port]\n"), 1),
                           -1);
        }
    }
  this->service_addr_.set (service_port);
  return 0;
}

int
ACE_TS_Server_Acceptor::init (int argc, ACE_TCHAR *argv[])
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Server_Acceptor::init"));

  // Use the options hook to parse the command line arguments and set
  // options.
  this->parse_args (argc, argv);

  // Set the acceptor endpoint into listen mode (use the Singleton
  // global Reactor...).
  if (this->open (this->service_addr_, ACE_Reactor::instance (),
                  0, 0, 0,
                  &this->scheduling_strategy_,
                  ACE_TEXT ("Time Server"),
                  ACE_TEXT ("ACE time service")) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%n: %p on port %d\n"),
                       ACE_TEXT ("acceptor::open failed"),
                       this->service_addr_.get_port_number ()),
                      -1);

  // Ignore SIGPIPE so that each <SVC_HANDLER> can handle this on its
  // own.
  ACE_Sig_Action sig ((ACE_SignalHandler) SIG_IGN, SIGPIPE);
  ACE_UNUSED_ARG (sig);

  ACE_INET_Addr server_addr;

  // Figure out what port we're really bound to.
  if (this->acceptor ().get_local_addr (server_addr) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("get_local_addr")),
                      -1);

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("starting up Time Server at port %d on handle %d\n"),
              server_addr.get_port_number (),
              this->acceptor ().get_handle ()));
  return 0;
}

// The following is a "Factory" used by the ACE_Service_Config and
// svc.conf file to dynamically initialize the state of the Time Server

ACE_SVC_FACTORY_DEFINE (ACE_TS_Server_Acceptor)

// Default constructor.
ACE_TS_Server_Handler::ACE_TS_Server_Handler (ACE_Thread_Manager *tm)
  : ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> (tm)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Server_Handler::ACE_TS_Server_Handler"));
}

// Activate this instance of the ACE_TS_Server_Handler (called by the
// ACE_TS_Server_Acceptor).

/* VIRTUAL */ int
ACE_TS_Server_Handler::open (void *)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Server_Handler::open"));

  ACE_INET_Addr client_addr;

  // Determine the address of the client and display it.
  if (this->peer ().get_remote_addr (client_addr) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("get_remote_addr")),
                      -1);

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("(%t) accepted connection from host %C on fd %d\n"),
              client_addr.get_host_name (),
              this->peer ().get_handle ()));

  // Call down to our parent to register ourselves with the Reactor.
  if (ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>::open (0) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("open")),
                      -1);
  return 0;
}

/* VIRTUAL */ int
ACE_TS_Server_Handler::send_request (ACE_Time_Request &request)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Server_Handler::send_request"));
  void *buffer;
  ssize_t length = request.encode (buffer);

  if (length == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("encode failed")),
                      -1);

  // Transmit request via a blocking send.

  if (this->peer ().send_n (buffer, length) != length)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("send_n failed")),
                      -1);
  return 0;
}

// Give up waiting (e.g., when a timeout occurs or a client shuts down
// unexpectedly).

/* VIRTUAL */ int
ACE_TS_Server_Handler::abandon (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Server_Handler::abandon"));

  // Note we are using the time field to report the errno in case of
  // failure.
  ACE_Time_Request rq (-1, errno);
  return this->send_request (rq);
}

// Enable clients to limit the amount of time they'll wait
/* VIRTUAL */ int
ACE_TS_Server_Handler::handle_timeout (const ACE_Time_Value &, const void *)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Server_Handler::handle_timeout"));
  return this->abandon ();
}

// Return the underlying ACE_HANDLE.

/* VIRTUAL */ ACE_HANDLE
ACE_TS_Server_Handler::get_handle (void) const
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Server_Handler::get_handle"));
  return this->peer ().get_handle ();
}

// Dispatch the appropriate operation to handle the client request.

/* VIRTUAL */ int
ACE_TS_Server_Handler::dispatch (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Server_Handler::dispatch"));
  // Get the system time and then create an ACE_Time_Request
  time_t t = ACE_OS::time (0);
  ACE_Time_Request rq (ACE_Time_Request::TIME_UPDATE, t);
  return this->send_request (rq);
}

// Receive, frame, and decode the client's request.  Note, this method
// should use non-blocking I/O.

/* VIRTUAL */ int
ACE_TS_Server_Handler::recv_request (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Server_Handler::recv_request"));
  ssize_t bytes_expected = this->time_request_.size ();

  // Since Time_Request messages are fixed size, read the entire
  // message in one go.
  ssize_t n = this->peer ().recv ((void *) &this->time_request_, bytes_expected);
  if (n != bytes_expected)
    {
      switch (n)
        {
        case -1:
          /* FALLTHROUGH */
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT ("****************** recv_request returned -1\n")));
        default:
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT ("%p got %d bytes, expected %d bytes\n"),
                      ACE_TEXT ("recv failed"),
                      n,
                      bytes_expected));
          /* FALLTHROUGH */
        case 0:
          // We've shutdown unexpectedly, let's abandon the
          // connection.
          this->abandon ();
          return -1;
          /* NOTREACHED */
        }
    }
  else
    {
      // Decode the request into host byte order.
      if (this->time_request_.decode () == -1)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT ("%p\n"),
                      ACE_TEXT ("decode failed")));
          return this->abandon ();
        }
    }
  return 0;
}

// Callback method invoked by the ACE_Reactor when events arrive from
// the client.

/* VIRTUAL */ int
ACE_TS_Server_Handler::handle_input (ACE_HANDLE)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Server_Handler::handle_input"));

  if (this->recv_request () == -1)
    return -1;
  else
    return this->dispatch ();
}

ACE_TS_Server_Handler::~ACE_TS_Server_Handler (void)
{
  ACE_TRACE (ACE_TEXT ("ACE_TS_Server_Handler::~ACE_TS_Server_Handler"));
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("closing down Handle %d\n"),
              this->get_handle ()));
}
