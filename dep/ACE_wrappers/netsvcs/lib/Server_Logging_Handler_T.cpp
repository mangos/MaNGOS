// $Id: Server_Logging_Handler_T.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#ifndef ACE_SERVER_LOGGING_HANDLERT_C
#define ACE_SERVER_LOGGING_HANDLERT_C

#include "ace/config-all.h"
#include "ace/Get_Opt.h"
#include "ace/Log_Record.h"
#include "ace/CDR_Stream.h"
#include "Server_Logging_Handler_T.h"
#include "ace/Signal.h"



#if !defined (ACE_LACKS_STATIC_DATA_MEMBER_TEMPLATES)
// Track number of requests.
template <ACE_PEER_STREAM_1, class COUNTER, ACE_SYNCH_DECL, class LMR>
COUNTER ACE_Server_Logging_Handler_T<ACE_PEER_STREAM_2, COUNTER, ACE_SYNCH_USE, LMR>::request_count_ = (COUNTER) 0;
#endif /* ACE_LACKS_STATIC_DATA_MEMBER_TEMPLATES */

template <ACE_PEER_STREAM_1, class COUNTER, ACE_SYNCH_DECL, class LMR>
ACE_Server_Logging_Handler_T<ACE_PEER_STREAM_2, COUNTER, ACE_SYNCH_USE, LMR>::ACE_Server_Logging_Handler_T
  (ACE_Thread_Manager *,
   LMR const &receiver)
   // Initialize the CString to something that is not the empty string
   // to avoid problems when calling fast_rep()
#if !defined (__GNUG__)
  : receiver_ (receiver, ACE_TString (ACE_TEXT(" "), 1))
#else
  : receiver_ (receiver),
    host_name_ (ACE_TString (ACE_TEXT (" "), 1))
#endif /* ! __GNUG__ */
{
}

// Callback routine for handling the reception of remote logging
// transmissions.

template <ACE_PEER_STREAM_1, class COUNTER, ACE_SYNCH_DECL, class LMR> int
ACE_Server_Logging_Handler_T<ACE_PEER_STREAM_2, COUNTER, ACE_SYNCH_USE, LMR>::handle_input (ACE_HANDLE)
{
  int result = this->handle_logging_record ();
  return result >= 0 ? 0 : -1;
}

template <ACE_PEER_STREAM_1, class COUNTER, ACE_SYNCH_DECL, class LMR> const ACE_TCHAR *
ACE_Server_Logging_Handler_T<ACE_PEER_STREAM_2, COUNTER, ACE_SYNCH_USE, LMR>::host_name (void)
{
#if !defined (__GNUG__)
  return this->receiver_.m_.fast_rep ();
#else
  return this->host_name_.fast_rep ();
#endif /* ! __GNUG__ */
}

template <ACE_PEER_STREAM_1, class COUNTER, ACE_SYNCH_DECL, class LMR> int
ACE_Server_Logging_Handler_T<ACE_PEER_STREAM_2, COUNTER, ACE_SYNCH_USE, LMR>::handle_logging_record ()
{
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

  ACE_CDR::Boolean byte_order;
  ACE_CDR::ULong length;

  ssize_t count = ACE::recv_n (this->peer ().get_handle (),
                               header->wr_ptr (),
                               8);
  switch (count)
    {
      // Handle shutdown and error cases.
    default:
    case -1:
    case 0:
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("server logging daemon closing down at host %s\n"),
                  this->host_name ()));

      return -1;
      /* NOTREACHED */

    case 8:
      // Just fall through in this case..
      break;
    }

  header->wr_ptr (8); // Reflect addition of 8 bytes.

  // Create a CDR stream to parse the 8-byte header.
  ACE_InputCDR header_cdr (header.get ());

  // Extract the byte-order and use helper methods to disambiguate
  // octet, booleans, and chars.
  if (!(header_cdr >> ACE_InputCDR::to_boolean (byte_order)))
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("Can't extract byte_order\n")));
      return 0;
    }

  // Set the byte-order on the stream...
  header_cdr.reset_byte_order (byte_order);

  // Extract the length
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

  // Use <recv_n> to obtain the contents.
  if (ACE::recv_n (this->peer ().get_handle (),
                   payload->wr_ptr (),
                   length) <= 0)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("%p\n"),
                  ACE_TEXT ("recv_n()")));
      return -1;
    }

  payload->wr_ptr (length);   // Reflect additional bytes

  ACE_InputCDR payload_cdr (payload.get ());
  payload_cdr.reset_byte_order (byte_order);
  if (!(payload_cdr >> log_record))  // Finally extract the <ACE_log_record>.
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("Can't extract log_record\n")));
      return 0;
    }

  log_record.length (length);

  // Send the log record to the log message receiver for processing.
  if (ACE_BIT_ENABLED (ACE_Log_Msg::instance ()->flags (), ACE_Log_Msg::STDERR))
    receiver ().log_record (this->host_name (), log_record);

  ostream *orig_ostream = ACE_Log_Msg::instance ()->msg_ostream ();
  receiver ().log_output (this->host_name (),
                          log_record,
                          orig_ostream);
  return 0;

#if 0
  ACE_INT32 length;

  // We need to use the ol' two-read trick here since TCP sockets
  // don't support framing natively.  Note that the first call is just
  // a "peek" -- we don't actually remove the data until the second
  // call.  Note that this code is portable as long as ACE_UNIT32 is
  // always 32 bits on both the sender and receiver side.

  switch (this->peer ().recv ((void *) &length,
                              sizeof length,
                              MSG_PEEK))
    {
    default:
    case -1:
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT ("%p at host %s\n"),
                         ACE_TEXT ("server logger"),
                         this->host_name ()),
                        -1);
      /* NOTREACHED */
    case 0:
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT ("closing log daemon at host %C\n"),
                         this->host_name ()),
                        -1);
      /* NOTREACHED */
    case sizeof length:
      {
        ACE_Log_Record lp;

        // Use ACE_NTOHL to get around bug in egcs 2.91.6x.
        length = ACE_NTOHL (length);

#if !defined (ACE_LACKS_STATIC_DATA_MEMBER_TEMPLATES)
        ++this->request_count_;

        u_long count = this->request_count_;
        ACE_DEBUG ((LM_DEBUG,
                    ACE_TEXT ("request count = %d, length = %d\n"),
                    count,
                    length));

#endif /* ACE_LACKS_STATIC_DATA_MEMBER_TEMPLATES */

        // Perform the actual <recv> this time.
        ssize_t n = this->peer ().recv_n ((void *) &lp,
                                          length);
        if (n != length)
          ACE_ERROR_RETURN ((LM_ERROR,
                             ACE_TEXT ("%d != %d, %p at host %C\n"),
                             n,
                             length,
                             ACE_TEXT ("server logger"),
                             this->host_name ()),
                            -1);

        lp.decode ();

        if (lp.length () == n)
          {
            // Send the log record to the log message receiver for
            // processing.
            if (ACE_BIT_ENABLED (ACE_Log_Msg::instance ()->flags (),
                                 ACE_Log_Msg::STDERR))
              receiver ().log_record (this->host_name (),
                                      lp);
            ostream *orig_ostream = ACE_Log_Msg::instance ()->msg_ostream ();
            receiver ().log_output (this->host_name (),
                                    lp,
                                    orig_ostream);
          }
        else
          ACERROR ((LM_ERROR,
                    ACE_TEXT ("error, lp.length = %d, n = %d\n"),
                    lp.length (),
                    n));
        return n;
      }
    }
#endif

  ACE_NOTREACHED (return -1;)
}

// Hook called by Server_Logging_Acceptor when connection is
// established.

template <ACE_PEER_STREAM_1, class COUNTER, ACE_SYNCH_DECL, class LMR> int
ACE_Server_Logging_Handler_T<ACE_PEER_STREAM_2, COUNTER, ACE_SYNCH_USE, LMR>::open_common (void)
{
  // Shut off non-blocking IO if it was enabled...
  if (this->peer ().disable (ACE_NONBLOCK) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("disable")),
                      -1);
  ACE_PEER_STREAM_ADDR client_addr;

  // Determine the address of the client and display it.
  if (this->peer ().get_remote_addr (client_addr) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("get_remote_addr")),
                      -1);

#if !defined (__GNUG__)
  this->receiver_.m_ =
    ACE_TString (ACE_TEXT_CHAR_TO_TCHAR (client_addr.get_host_name ()));
#else
  this->host_name_ =
    ACE_TString (ACE_TEXT_CHAR_TO_TCHAR (client_addr.get_host_name ()));
#endif /* ! __GNUG__ */

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("(%t) accepted connection from host %C on fd %d\n"),
              client_addr.get_host_name (),
              this->peer ().get_handle ()));

  return 0;
}

template<class SLH, class LMR, class SST>
ACE_Server_Logging_Acceptor_T<SLH, LMR, SST>::ACE_Server_Logging_Acceptor_T (void)
{
}

template<class SLH, class LMR, class SST> LMR &
ACE_Server_Logging_Acceptor_T<SLH, LMR, SST>::receiver (void)
{
  return receiver_;
}

template<class SLH, class LMR, class SST> SST &
ACE_Server_Logging_Acceptor_T<SLH, LMR, SST>::scheduling_strategy (void)
{
#if !defined (__GNUG__)
  return receiver_.m_;
#else
  return schedule_strategy_;
#endif /* ! __GNUG__ */
}

template<class SLH, class LMR, class SST> int
ACE_Server_Logging_Acceptor_T<SLH, LMR, SST>::init (int argc, ACE_TCHAR *argv[])
{
  ACE_TRACE (ACE_TEXT ("ACE_Server_Logging_Acceptor_T<SLH, LMR, SST>::init"));

  // Use the options hook to parse the command line arguments and set
  // options.
  this->parse_args (argc, argv);

  // Set the acceptor endpoint into listen mode (use the Singleton
  // global Reactor...).
  if (this->open (this->service_addr_,
                  ACE_Reactor::instance (),
                  0, 0, 0,
                  &this->scheduling_strategy(),
                  ACE_TEXT ("Logging Server"),
                  ACE_TEXT ("ACE logging service")) == -1)
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
              ACE_TEXT ("starting up Logging Server at port %d on handle %d\n"),
              server_addr.get_port_number (),
              this->acceptor ().get_handle ()));
  return 0;
}

template<class SLH, class LMR, class SST> int
ACE_Server_Logging_Acceptor_T<SLH, LMR, SST>::parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_TRACE (ACE_TEXT ("ACE_Server_Logging_Acceptor_T<SLH, LMR, SST>::parse_args"));

  int service_port = ACE_DEFAULT_SERVER_PORT;

  ACE_LOG_MSG->open (ACE_TEXT ("Logging Service"), ACE_LOG_MSG->flags ());

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
                             ACE_TEXT ("%n:\n[-p server-port]\n")),
                            -1);
        }
    }

  this->service_addr_.set (service_port);
  return 0;
}

template<class SERVER_LOGGING_HANDLER, class LOG_MESSAGE_RECEIVER, class SCHEDULE_STRATEGY> int
ACE_Server_Logging_Acceptor_T<SERVER_LOGGING_HANDLER,
                              LOG_MESSAGE_RECEIVER,
                              SCHEDULE_STRATEGY>
    ::make_svc_handler (SERVER_LOGGING_HANDLER *&handler)
{
  ACE_NEW_RETURN (handler,
                  SERVER_LOGGING_HANDLER (ACE_Thread_Manager::instance (),
                                          this->receiver()),
                  -1);
  return 0;
}

template<class LOG_MESSAGE_RECEIVER>
ACE_Server_Logging_Handler<LOG_MESSAGE_RECEIVER>::ACE_Server_Logging_Handler (ACE_Thread_Manager * tm,
                                                             LOG_MESSAGE_RECEIVER const& receiver)
  : ACE_Server_Logging_Handler_T<LOGGING_PEER_STREAM, u_long, ACE_NULL_SYNCH, LOG_MESSAGE_RECEIVER>(tm,
                                                                                   receiver)
{
}

template<class LOG_MESSAGE_RECEIVER>
ACE_Server_Logging_Handler<LOG_MESSAGE_RECEIVER>::ACE_Server_Logging_Handler(ACE_Thread_Manager * tm)
  : ACE_Server_Logging_Handler_T<LOGGING_PEER_STREAM, u_long, ACE_NULL_SYNCH, LOG_MESSAGE_RECEIVER>(tm, LOG_MESSAGE_RECEIVER())
{
}

template<class LOG_MESSAGE_RECEIVER>  int
ACE_Server_Logging_Handler<LOG_MESSAGE_RECEIVER>::open (void *)
{
  // call base class open_common
  if (this->open_common () != 0)
    return -1;

  // Register ourselves with the Reactor to enable subsequent
  // dispatching.
  if (ACE_Reactor::instance ()->register_handler
      (this, ACE_Event_Handler::READ_MASK) == -1)
    return -1;
  return 0;
}

template<class LOG_MESSAGE_RECEIVER>
ACE_Thr_Server_Logging_Handler<LOG_MESSAGE_RECEIVER>::ACE_Thr_Server_Logging_Handler (ACE_Thread_Manager *tm, LOG_MESSAGE_RECEIVER const &receiver)
  : ACE_Server_Logging_Handler_T<LOGGING_PEER_STREAM, ACE_LOGGER_COUNTER, ACE_LOGGER_SYNCH, LOG_MESSAGE_RECEIVER>(tm, receiver)
{
}

template<class LOG_MESSAGE_RECEIVER>
ACE_Thr_Server_Logging_Handler<LOG_MESSAGE_RECEIVER>::ACE_Thr_Server_Logging_Handler (ACE_Thread_Manager *tm)
  : ACE_Server_Logging_Handler_T<LOGGING_PEER_STREAM, ACE_LOGGER_COUNTER, ACE_LOGGER_SYNCH, LOG_MESSAGE_RECEIVER>(tm, LOG_MESSAGE_RECEIVER ())
{
}

template<class LOG_MESSAGE_RECEIVER> int
ACE_Thr_Server_Logging_Handler<LOG_MESSAGE_RECEIVER>::open (void *)
{
  // call base class open_common
  if (this->open_common () != 0)
    return -1;

  // Spawn a new thread of control to handle logging records with the
  // client using a thread-per-connection concurrency model.  Note
  // that this implicitly uses the <ACE_Thread_Manager::instance> to
  // control all the threads.
  if (this->activate (THR_BOUND | THR_DETACHED) == -1)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("%p\n"),
                       ACE_TEXT ("spawn")),
                      -1);
  return 0;
}

// Process remote logging records.

template<class LOG_MESSAGE_RECEIVER> int
ACE_Thr_Server_Logging_Handler<LOG_MESSAGE_RECEIVER>::svc (void)
{
  int result = 0;

  // Loop until the client terminates the connection or an error occurs.

  while ((result = this->handle_input ()) == 0)
    continue;

  return result;
}
#endif /* ACE_SERVER_LOGGING_HANDLER_TT_C */
