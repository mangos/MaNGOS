// $Id: direct_logging.cpp 91671 2010-09-08 18:39:23Z johnnyw $

// This program sends logging records directly to the server, rather
// than going through the client logging daemon.

#include "ace/SOCK_Connector.h"
#include "ace/Log_Record.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_time.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_unistd.h"
#include "ace/CDR_Stream.h"



static u_short LOGGER_PORT = ACE_DEFAULT_SERVER_PORT;
static const ACE_TCHAR *const LOGGER_HOST = ACE_DEFAULT_SERVER_HOST;
static const ACE_TCHAR *const DATA = ACE_TEXT ("hello world\n");

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  u_short logger_port  = argc > 1 ? ACE_OS::atoi (argv[1]) : LOGGER_PORT;
  const ACE_TCHAR *logger_host = argc > 2 ? argv[2] : LOGGER_HOST;

  ACE_SOCK_Stream logger;
  ACE_SOCK_Connector connector;
  ACE_INET_Addr addr (logger_port, logger_host);
  ACE_Log_Record log_record (LM_DEBUG,
                             ACE_OS::time ((time_t *) 0),
                             ACE_OS::getpid ());

  if (connector.connect (logger, addr) == -1)
    ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "open"), -1);

  log_record.msg_data (DATA);
  const size_t max_payload_size =
    4 // type()
    + 8 // timestamp
    + 4 // process id
    + 4 // data length
    + ACE_Log_Record::MAXLOGMSGLEN // data
    + ACE_CDR::MAX_ALIGNMENT; // padding;

  // Insert contents of <log_record> into payload stream.
  ACE_OutputCDR payload (max_payload_size);
  payload << log_record;

  // Get the number of bytes used by the CDR stream.
  ACE_CDR::ULong length = payload.total_length ();

  // Send a header so the receiver can determine the byte order and
  // size of the incoming CDR stream.
  ACE_OutputCDR header (ACE_CDR::MAX_ALIGNMENT + 8);
  header << ACE_OutputCDR::from_boolean (ACE_CDR_BYTE_ORDER);

  // Store the size of the payload that follows
  header << ACE_CDR::ULong (length);

  // Use an iovec to send both buffer and payload simultaneously.
  iovec iov[2];
  iov[0].iov_base = header.begin ()->rd_ptr ();
  iov[0].iov_len  = 8;
  iov[1].iov_base = payload.begin ()->rd_ptr ();
  iov[1].iov_len  = length;

  if (logger.sendv_n (iov, 2) == -1)
    ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "send"), -1);
  else if (logger.close () == -1)
    ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "close"), -1);

#if defined (ACE_WIN32)
  // !!Important, Winsock is broken in that if you don't close
  // down the connection before exiting main, you'll lose data.
  // More over, your server might get "Access Violation" from
  // within Winsock functions.

  // Here we close down the connection to Logger by redirecting
  // the logging destination back to stderr.
  ACE_LOG_MSG->open (0, ACE_Log_Msg::STDERR, 0);
#endif /* ACE_WIN32 */

  return 0;
}
