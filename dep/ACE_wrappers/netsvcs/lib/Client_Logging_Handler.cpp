// $Id: Client_Logging_Handler.cpp 91673 2010-09-08 18:49:47Z johnnyw $

#include "ace/Get_Opt.h"
#include "ace/Acceptor.h"
#include "ace/SOCK_Connector.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SPIPE_Acceptor.h"
#include "ace/Log_Record.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_sys_socket.h"
#include "ace/OS_NS_unistd.h"
#include "ace/CDR_Stream.h"
#include "ace/Auto_Ptr.h"
#include "ace/SString.h"
#include "ace/INET_Addr.h"
#include "Client_Logging_Handler.h"

ACE_Client_Logging_Handler::ACE_Client_Logging_Handler (ACE_HANDLE output_handle)
  : logging_output_ (output_handle)
{
  // Register ourselves to receive SIGPIPE so we can attempt
  // reconnections.
#if !defined (ACE_LACKS_UNIX_SIGNALS)
  if (ACE_Reactor::instance ()->register_handler (SIGPIPE,
                                                  this) == -1)
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT ("%n: %p\n"),
                ACE_TEXT ("register_handler (SIGPIPE)")));
#endif /* !ACE_LACKS_UNIX_SIGNALS */
}

// This is called when a <send> to the logging server fails...

int
ACE_Client_Logging_Handler::handle_signal (int signum,
                                           siginfo_t *,
                                           ucontext_t *)
{
  if (signum == SIGPIPE)
    return 0;
  else
    return -1;
}

// This function is called every time a client connects to us.

int
ACE_Client_Logging_Handler::open (void *)
{
  LOGGING_ADDR server_addr;

  // Register ourselves to receive <handle_input> callbacks when
  // clients send us logging records.  Note that since we're really a
  // Singleton, this->peer() will change after each connect, so we
  // need to grab the value now.
  if (ACE_Reactor::instance ()->register_handler
      (this->peer ().get_handle (),
       this,
       ACE_Event_Handler::READ_MASK
       | ACE_Event_Handler::EXCEPT_MASK) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%n: %p\n"),
                       ACE_TEXT ("register_handler")),
                       -1);
  // Figure out what remote port we're really bound to.
  if (this->peer ().get_remote_addr (server_addr) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("get_remote_addr")),
                      -1);
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("Connected to client on handle %u\n"),
              this->peer ().get_handle ()));
  return 0;
}

/* VIRTUAL */ ACE_HANDLE
ACE_Client_Logging_Handler::get_handle (void) const
{
  ACE_TRACE ("ACE_Client_Logging_Handler::get_handle");

  ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("get_handle() shouldn't be called\n")));

  return ACE_INVALID_HANDLE;
}

// Receive a logging record from an application.

int
ACE_Client_Logging_Handler::handle_input (ACE_HANDLE handle)
{
#if 0
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("in ACE_Client_Logging_Handler::handle_input, handle = %u\n"),
              handle));
#endif /* 0 */

  if (handle == this->logging_output_)
    // We're getting a message from the logging server!
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("Received data from server!\n")),
                      -1);
  ACE_Log_Record log_record;

  // We need to use the old two-read trick here since TCP sockets
  // don't support framing natively.  Allocate a message block for the
  // payload; initially at least large enough to hold the header, but
  // needs some room for alignment.
  ACE_Message_Block *payload_p = 0;
  ACE_Message_Block *header_p = 0;
  ACE_NEW_RETURN (header_p,
                  ACE_Message_Block (ACE_DEFAULT_CDR_BUFSIZE),
                  -1);

  auto_ptr <ACE_Message_Block> header (header_p);

  // Align the Message Block for a CDR stream
  ACE_CDR::mb_align (header.get ());

#if (ACE_HAS_STREAM_LOG_MSG_IPC == 1)
  // We're getting a logging message from a local application using
  // STREAM pipes, which are nicely prioritized for us.
  ACE_Str_Buf header_msg (header->wr_ptr (),
                          0,
                          8);

  ACE_SPIPE_Stream spipe;
  spipe.set_handle (handle);
  int flags = 0;

  // We've got a framed IPC mechanism, so we can just to a <recv>.
  ssize_t result = spipe.recv (&header_msg,
                               (ACE_Str_Buf *) 0,
                               &flags);

  if (result < 0 || header_msg.len == 0)
    {
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("client closing down unexpectedly\n")));

      if (ACE_Reactor::instance ()->remove_handler
          (handle,
          ACE_Event_Handler::READ_MASK
          | ACE_Event_Handler::EXCEPT_MASK
          | ACE_Event_Handler::DONT_CALL) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%n: %p\n"),
                           ACE_TEXT ("remove_handler")),
                          -1);
      spipe.close ();
      return 0;
    }
#else
  // We're getting a logging message from a local application using
  // sockets pipes, which are NOT prioritized for us.

  ssize_t const count = ACE::recv_n (handle,
                                     header->wr_ptr (),
                                     8);
  switch (count)
    {
      // Handle shutdown and error cases.
    default:
    case -1:
    case 0:
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("client closing down\n")));

      if (ACE_Reactor::instance ()->remove_handler
          (handle,
           ACE_Event_Handler::READ_MASK
           | ACE_Event_Handler::EXCEPT_MASK
           | ACE_Event_Handler::DONT_CALL) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%n: %p\n"),
                           ACE_TEXT ("remove_handler")),
                          0);
      if (handle == this->peer ().get_handle ())
        this->peer ().close ();
      else
        ACE_OS::closesocket (handle);
      // Release the memory to prevent a leak.
      return 0;
      /* NOTREACHED */

    case 8:
      // Just fall through in this case..
      break;
    }
#endif /* ACE_HAS_STREAM_LOG_MSG_IPC == 1 */

  // Reflect addition of 8 bytes for the header.
  header->wr_ptr (8);

  // Create a CDR stream to parse the 8-byte header.
  ACE_InputCDR header_cdr (header.get ());

  // Extract the byte-order and use helper methods to disambiguate
  // octet, booleans, and chars.
  ACE_CDR::Boolean byte_order;
  if (!(header_cdr >> ACE_InputCDR::to_boolean (byte_order)))
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("Can't extract byte_order\n")));
      return 0;
    }

  // Set the byte-order on the stream...
  header_cdr.reset_byte_order (byte_order);

  // Extract the length
  ACE_CDR::ULong length;
  if (!(header_cdr >> length))
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("Can't extract length\n")));
      return 0;
    }

  ACE_NEW_RETURN (payload_p,
                  ACE_Message_Block (length),
                  -1);
  auto_ptr <ACE_Message_Block> payload (payload_p);

  // Ensure there's sufficient room for log record payload.
  ACE_CDR::grow (payload.get (), 8 + ACE_CDR::MAX_ALIGNMENT + length);

#if (ACE_HAS_STREAM_LOG_MSG_IPC == 1)
  ACE_Str_Buf payload_msg (payload->wr_ptr (),
                           0,
                           length);

  // We've got a framed IPC mechanism, so we can just do a <recv>.
  result = spipe.recv ((ACE_Str_Buf *) 0,
                       &payload_msg,
                       &flags);

  if (result < 0 || payload_msg.len != (int)length)
    {
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("%p\n"),
                  ACE_TEXT ("client closing down due to error\n")));

      if (ACE_Reactor::instance ()->remove_handler
          (handle,
           ACE_Event_Handler::READ_MASK
           | ACE_Event_Handler::EXCEPT_MASK
           | ACE_Event_Handler::DONT_CALL) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%n: result %d, length %d %p\n"),
                           result,
                           payload_msg.len,
                           ACE_TEXT ("remove_handler")),
                          -1);
      spipe.close ();
      return 0;
    }
#else
  // Use <recv_n> to obtain the contents.
  if (ACE::recv_n (handle, payload->wr_ptr (), length) <= 0)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("%p\n"),
                  ACE_TEXT ("recv_n()")));

      if (ACE_Reactor::instance ()->remove_handler
          (handle,
           ACE_Event_Handler::READ_MASK
           | ACE_Event_Handler::EXCEPT_MASK
           | ACE_Event_Handler::DONT_CALL) == -1)
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT ("%n: %p\n"),
                    ACE_TEXT ("remove_handler")));

      ACE_OS::closesocket (handle);
      return 0;
    }
#endif /* ACE_HAS_STREAM_LOG_MSG_IPC == 1 */

  // Reflect additional bytes for the message.
  payload->wr_ptr (length);

  ACE_InputCDR payload_cdr (payload.get ());
  payload_cdr.reset_byte_order (byte_order);
  if (!(payload_cdr >> log_record))  // Finally extract the ACE_log_record.
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("Can't extract log_record\n")));
      return 0;
    }

  log_record.length (length);

  // Forward the logging record to the server.
  if (this->send (log_record) == -1)
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT ("%p\n"),
                ACE_TEXT ("send")));
  return 0;
}

// Receive a logging record from an application send via a non-0
// MSG_BAND...  This just calls handle_input().

int
ACE_Client_Logging_Handler::handle_exception (ACE_HANDLE handle)
{
  return this->handle_input (handle);
}

// Called when object is removed from the ACE_Reactor

int
ACE_Client_Logging_Handler::close (u_long)
{
  if (this->logging_output_ != ACE_STDERR)
    ACE_OS::closesocket (this->logging_output_);

  this->destroy ();
  return 0;
}

int
ACE_Client_Logging_Handler::handle_output (ACE_HANDLE)
{
  return 0;
}

// Encodes the contents of log_record object using network byte-order
// and sends it to the logging server.

int
ACE_Client_Logging_Handler::send (ACE_Log_Record &log_record)
{
  ostream *orig_ostream = ACE_Log_Msg::instance ()->msg_ostream ();

  // This logic must occur before we do the encode() on <log_record>
  // since otherwise the values of the <log_record> fields will be in
  // network byte order.

  if (orig_ostream)
    log_record.print (ACE_TEXT ("<localhost>"),
                      ACE_Log_Msg::instance ()->flags (),
                      *orig_ostream);

  if (this->logging_output_ == ACE_STDERR)
    {
      log_record.print (ACE_TEXT ("<localhost>"),
                        ACE_Log_Msg::instance ()->flags (),
                        stderr);
    }
  else
    {
      // Serialize the log record using a CDR stream, allocate enough
      // space for the complete <ACE_Log_Record>.
      size_t const max_payload_size =
        4 // type()
        + 8 // timestamp
        + 4 // process id
        + 4 // data length
        + ACE_Log_Record::MAXLOGMSGLEN // data
        + ACE_CDR::MAX_ALIGNMENT; // padding;

      // Insert contents of <log_record> into payload stream.
      ACE_OutputCDR payload (max_payload_size);
      if (!(payload << log_record))
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT ("Can't insert log_record\n")));
          return -1;
        }

      // Get the number of bytes used by the CDR stream.
      ACE_CDR::ULong const length = payload.total_length ();

      // Send a header so the receiver can determine the byte order and
      // size of the incoming CDR stream.
      ACE_OutputCDR header (ACE_CDR::MAX_ALIGNMENT + 8);
      if (!(header << ACE_OutputCDR::from_boolean (ACE_CDR_BYTE_ORDER)))
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT ("Can't insert byte order\n")));
          return -1;
        }

      // Store the size of the payload that follows
      if (!(header << ACE_CDR::ULong (length)))
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT ("Can't insert length\n")));
          return -1;
        }

      // Use an iovec to send both buffer and payload simultaneously.
      iovec iov[2];
      iov[0].iov_base = header.begin ()->rd_ptr ();
      iov[0].iov_len  = 8;
      iov[1].iov_base = payload.begin ()->rd_ptr ();
      iov[1].iov_len  = length;

      // We're running over sockets, so send header and payload
      // efficiently using "gather-write".
      if (ACE::sendv_n (this->logging_output_,iov, 2) == -1)
        {
          ACE_DEBUG ((LM_DEBUG,
                      "Something about the sendv_n() failed, so switch to stderr\n"));

          if (ACE_Log_Msg::instance ()->msg_ostream () == 0)
            // Switch over to logging to stderr for now.  At some
            // point, we'll improve the implementation to queue up the
            // message, try to reestablish a connection, and then send
            // the queued data once we've reconnect to the logging
            // server.  If you'd like to implement this functionality
            // and contribute it back to ACE that would be great!
            this->logging_output_ = ACE_STDERR;
        }
      else
        ACE_DEBUG ((LM_DEBUG,
                    "Sent logging message %s successfully to Server Logging Daemon!\n",
                    log_record.priority_name (ACE_Log_Priority (log_record.type ()))));
    }

  return 0;
}

class ACE_Client_Logging_Acceptor : public ACE_Acceptor<ACE_Client_Logging_Handler, LOGGING_ACCEPTOR>
{
  // = TITLE
  //     This factory creates connections with the
  //     <Server_Logging_Acceptor>.
  //
  // = DESCRIPTION
  //     This class contains the service-specific methods that can't
  //     easily be factored into the <ACE_Acceptor>.
public:
  // = Initialization method.
  ACE_Client_Logging_Acceptor (void);
  // Default constructor.

protected:
  // = Dynamic linking hooks.
  virtual int init (int argc, ACE_TCHAR *argv[]);
  // Called when service is linked.

  virtual int fini (void);
  // Called when service is unlinked.

  virtual int info (ACE_TCHAR **strp, size_t length) const;
  // Called to determine info about the service.

  virtual int make_svc_handler (ACE_Client_Logging_Handler *&sh);
  // Factory that always returns the <handler_>.

  // = Scheduling hooks.
  virtual int suspend (void);
  virtual int resume (void);

private:
  int parse_args (int argc, ACE_TCHAR *argv[]);
  // Parse svc.conf arguments.

  const ACE_TCHAR *server_host_;
  // Host where the logging server is located.

  u_short server_port_;
  // Port number where the logging server is listening for
  // connections.

  ACE_INET_Addr server_addr_;
  // Address to connect to the server logging daemon.

  ACE_INET_Addr local_addr_;
  // Local IP/port number to use for the connection to the server logging
  // daemon.

  const ACE_TCHAR *logger_key_;
  // Communication endpoint where the client logging daemon will
  // listen for connections from clients.

  ACE_Client_Logging_Handler *handler_;
  // Pointer to the singleton handler that receives messages from
  // clients and forwards to the server.
};

int
ACE_Client_Logging_Acceptor::fini (void)
{
  this->close ();

  if (this->handler_ != 0)
    this->handler_->close (0);

  // Try to unlink the logger key so weird things don't happen if
  // we're using STREAM pipes.
  ACE_OS::unlink (this->logger_key_);

  // This memory was allocated by <ACE_OS::strdup>.
  ACE_OS::free ((void *) this->logger_key_);

  ACE_OS::free ((void *) this->server_host_);

  return 0;
}

int
ACE_Client_Logging_Acceptor::make_svc_handler (ACE_Client_Logging_Handler *&sh)
{
  // Always return a pointer to the Singleton handler.
  sh = this->handler_;
  return 0;
}

int
ACE_Client_Logging_Acceptor::info (ACE_TCHAR **strp, size_t length) const
{
  ACE_TCHAR buf[BUFSIZ];

  ACE_OS::sprintf (buf, ACE_TEXT ("%d/%s %s"),
                   this->server_addr_.get_port_number (), "tcp",
                   "# client logging daemon\n");

  if (*strp == 0 && (*strp = ACE_OS::strdup (buf)) == 0)
    return -1;
  else
    ACE_OS::strncpy (*strp, buf, length);
  return ACE_OS::strlen (buf);
}

ACE_Client_Logging_Acceptor::ACE_Client_Logging_Acceptor (void)
  : server_host_ (ACE_OS::strdup (ACE_DEFAULT_SERVER_HOST)),
    server_port_ (ACE_DEFAULT_LOGGING_SERVER_PORT),
    logger_key_ (ACE_OS::strdup (ACE_DEFAULT_LOGGER_KEY)),
    handler_ (0)
{
}

int
ACE_Client_Logging_Acceptor::init (int argc, ACE_TCHAR *argv[])
{
  // We'll log *our* error and debug messages to stderr!
  if (ACE_LOG_MSG->open (ACE_TEXT ("Client Logging Service")) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("Can't open ACE_Log_Msg\n")),
                      -1);

  // Use the options hook to parse the command line arguments and set
  // options.
  this->parse_args (argc, argv);

  // Try to unlink the logger key so weird things don't happen if
  // we're using STREAM pipes.
  ACE_OS::unlink (this->logger_key_);

  // Initialize the acceptor endpoint.
  if (this->open (LOGGING_ADDR (this->logger_key_)) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       this->logger_key_),
                      -1);

  // Establish connection with the server.
  ACE_SOCK_Connector con;
  ACE_SOCK_Stream stream;
  ACE_INET_Addr server_addr;

#if (ACE_HAS_STREAM_LOG_MSG_IPC == 1)
  ACE_SPIPE_Addr lserver_addr;

  // Figure out what local port we're really bound to.
  if (this->acceptor ().get_local_addr (lserver_addr) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("get_local_addr")),
                      -1);

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("Starting up Client Logging Daemon, ")
              ACE_TEXT ("bounded to STREAM addr %s on handle %u\n"),
              lserver_addr.get_path_name (),
              this->acceptor ().get_handle ()));
#else
  ACE_INET_Addr lserver_addr;

  // Figure out what local port we're really bound to.
  if (this->acceptor ().get_local_addr (lserver_addr) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("get_local_addr")),
                      -1);

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("Starting up Client Logging Daemon, ")
              ACE_TEXT ("bounded to local port %d on handle %u\n"),
              lserver_addr.get_port_number (),
              this->acceptor ().get_handle ()));
#endif /* ACE_HAS_STREAM_LOG_MSG_IPC == 1 */

  if (con.connect (stream,
                   this->server_addr_,
                   0,
                   this->local_addr_) == -1)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("Can't connect to logging server %C on port %d: ")
                  ACE_TEXT ("%m, using stderr\n"),
                  this->server_addr_.get_host_name (),
                  this->server_addr_.get_port_number (),
                  ACE_ERRNO_GET));

      if (ACE_Log_Msg::instance ()->msg_ostream () == 0)
        // If we can't connect to the server then we'll send the logging
        // messages to stderr.
        stream.set_handle (ACE_STDERR);
    }
  else
    {
      // Figure out what remote port we're really bound to.
      if (stream.get_remote_addr (server_addr) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("get_remote_addr")),
                          -1);
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("Client Logging Daemon is connected to Server ")
                  ACE_TEXT ("Logging Daemon %C on port %d on handle %u\n"),
                  server_addr.get_host_name (),
                  server_addr.get_port_number (),
                  stream.get_handle ()));
    }

  // Create the Singleton <Client_Logging_Handler>.
  ACE_NEW_RETURN (this->handler_,
                  ACE_Client_Logging_Handler (stream.get_handle ()),
                  -1);
  return 0;
}

int
ACE_Client_Logging_Acceptor::parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opt (argc, argv, ACE_TEXT ("h:k:p:l:"), 0);
  ACE_TString local_addr_str;

  for (int c; (c = get_opt ()) != -1; )
    {
      switch (c)
       {
        case 'h':
          ACE_OS::free ((void *) this->server_host_);
          this->server_host_ = ACE_OS::strdup (get_opt.opt_arg ());
          break;
        case 'k':
          ACE_OS::free ((void *) this->logger_key_);
          this->logger_key_ = ACE_OS::strdup (get_opt.opt_arg ());
          break;
        case 'p':
          this->server_port_ = ACE_OS::atoi (get_opt.opt_arg ());
          break;
        case 'l':
          local_addr_str = get_opt.opt_arg ();
          break;
        default:
          ACE_ERROR_RETURN ((LM_ERROR,
                             ACE_TEXT ("%n:\n[-p server-port]\n")
                             ACE_TEXT ("[-l local-ip[:local-port]]\n")),
                            -1);
        }
    }

  this->local_addr_.set ((u_short)0);   // "any"
  if (local_addr_str.length () > 0)
    {
      if (local_addr_str.rfind (ACE_TCHAR(':')) == ACE_TString::npos)
        local_addr_str += ACE_TEXT (":0");
      ACE_TCHAR *local_addr_cstr = local_addr_str.rep ();
      if (-1 == local_addr_.string_to_addr (ACE_TEXT_ALWAYS_CHAR (local_addr_cstr)))
        ACE_ERROR ((LM_ERROR, ACE_TEXT ("%p\n"), local_addr_cstr));
      delete local_addr_cstr;
    }

  if (this->server_addr_.set (this->server_port_,
                              this->server_host_) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       this->server_host_),
                      -1);
  return 0;
}

int
ACE_Client_Logging_Acceptor::suspend (void)
{
  // To be done...
  return 0;
}

int
ACE_Client_Logging_Acceptor::resume (void)
{
  // To be done...
  return 0;
}

// The following is a "Factory" used by the ACE_Service_Config and
// svc.conf file to dynamically initialize the state of the
// single-threaded logging server.

ACE_SVC_FACTORY_DEFINE (ACE_Client_Logging_Acceptor)

