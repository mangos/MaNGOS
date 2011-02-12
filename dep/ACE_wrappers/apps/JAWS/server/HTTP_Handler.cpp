// $Id: HTTP_Handler.cpp 91670 2010-09-08 18:02:26Z johnnyw $

// HTTP_Service.cpp -- simple implementation of the HTTP protocol

#include "ace/Message_Block.h"
#include "ace/Filecache.h"

#include "HTTP_Handler.h"
#include "HTTP_Helpers.h"
#include "JAWS_IO.h"
#include "ace/OS_NS_sys_socket.h"
#include "ace/OS_NS_stdio.h"

HTTP_Handler::HTTP_Handler (JAWS_IO &io,
                            HTTP_Handler_Factory &factory)
  : factory_ (factory),
    request_data_ (0),
    handle_ (ACE_INVALID_HANDLE),
    response_ (io, request_),
    io_ (io)
{
  this->io_.handler (this);
}

HTTP_Handler::~HTTP_Handler (void)
{
  this->request_data_->release ();
  this->request_data_ = 0;
}

void
HTTP_Handler::open (ACE_HANDLE handle,
                    ACE_Message_Block &initial_data)
{
  ACE_DEBUG ((LM_DEBUG, "(%t) New connection\n"));

  int sockbufsize = HTTP_Handler::MAX_SOCKBUFSIZE;
  int result = ACE_OS::setsockopt (handle,
                                   SOL_SOCKET,
                                   SO_RCVBUF,
                                   (char *) &sockbufsize,
                                   sizeof sockbufsize);

  if (result == -1)
    ACE_ERROR ((LM_ERROR, "%p\n", "SO_RCVBUF"));

  sockbufsize = HTTP_Handler::MAX_SOCKBUFSIZE;

  result = ACE_OS::setsockopt (handle,
                               SOL_SOCKET,
                               SO_SNDBUF,
                               (char *) &sockbufsize,
                               sizeof sockbufsize);
  if (result == -1)
    ACE_ERROR ((LM_ERROR, "%p\n", "SO_SNDBUF"));

  this->handle_ = handle;
  this->io_.handle (this->handle_);

  this->request_data_ = initial_data.duplicate ();
  this->read_complete (initial_data);
}

void
HTTP_Handler::read_complete (ACE_Message_Block &message_block)
{
  // This is actually a callback entry point.  The JAWS_IO framework
  // calls into this method after some data has been read in.

  switch (this->request_.parse_request (message_block))
    {
    case 0:
      do
        {
          int next_read_size
            = HTTP_Handler::MAX_REQUEST_SIZE - message_block.length ();

          if (next_read_size == 0)
            {
              this->request_too_long ();
              return;
            }

          this->io_.read (message_block, next_read_size);
        }
      while (0);
      break;

    default:
      // this->request_.respond ();
      this->response_.process_request ();
    }
}

void
HTTP_Handler::receive_file_complete (void)
{
  ACE_DEBUG ((LM_DEBUG, " (%t) %s received successfully\n",
              request_.uri ()));

  char buffer[BUFSIZ];
  int buflen =
    ACE_OS::sprintf (buffer,
                     "%s %d %s\r\n",
                     this->request_.version (),
                     HTTP_Status_Code::STATUS_OK,
                     "Successful");

  this->io_.send_confirmation_message (buffer, buflen);
}

void
HTTP_Handler::receive_file_error (int result)
{
  ACE_DEBUG ((LM_DEBUG, " (%t) %s error in receiving file\n",
              request_.uri ()));

  char buffer[BUFSIZ];

  int status_code;
  switch (result)
    {
    case ACE_Filecache_Handle::ACE_ACCESS_FAILED:
    case ACE_Filecache_Handle::ACE_WRITE_FAILED:
    case ACE_Filecache_Handle::ACE_OPEN_FAILED:
      status_code = HTTP_Status_Code::STATUS_NOT_FOUND;
      break;
    case ACE_Filecache_Handle::ACE_COPY_FAILED:
    case ACE_Filecache_Handle::ACE_STAT_FAILED:
    case ACE_Filecache_Handle::ACE_MEMMAP_FAILED:
      status_code = HTTP_Status_Code::STATUS_FORBIDDEN;
      break;
    default:
      status_code = HTTP_Status_Code::STATUS_INTERNAL_SERVER_ERROR;
      break;
    }
  int buflen =
    ACE_OS::sprintf (buffer,
                     "%s %d %s",
                     this->request_.version (),
                     status_code,
                     "Failed");

  this->io_.send_confirmation_message (buffer, buflen);
}

void
HTTP_Handler::confirmation_message_complete (void)
{
  this->done ();
}

void
HTTP_Handler::error_message_complete (void)
{
  this->done ();
}

void
HTTP_Handler::transmit_file_complete (void)
{
  ACE_DEBUG ((LM_DEBUG, " (%t) %s transmitted successfully\n",
              request_.uri ()));

  this->done ();
}

void
HTTP_Handler::transmit_file_error (int result)
{
  ACE_DEBUG ((LM_DEBUG,
              " (%t) %s error in transmitting file\n",
              request_.uri ()));

  int status_code;

  switch (result)
    {
    case ACE_Filecache_Handle::ACE_ACCESS_FAILED:
    case ACE_Filecache_Handle::ACE_WRITE_FAILED:
    case ACE_Filecache_Handle::ACE_OPEN_FAILED:
      status_code = HTTP_Status_Code::STATUS_NOT_FOUND;
      break;
    case ACE_Filecache_Handle::ACE_COPY_FAILED:
    case ACE_Filecache_Handle::ACE_STAT_FAILED:
    case ACE_Filecache_Handle::ACE_MEMMAP_FAILED:
      status_code = HTTP_Status_Code::STATUS_FORBIDDEN;
      break;
    default:
      status_code = HTTP_Status_Code::STATUS_INTERNAL_SERVER_ERROR;
      break;
    }

  this->response_.error_response (status_code, "error in transmitting file");
}

void
HTTP_Handler::read_error (void)
{
  ACE_DEBUG ((LM_DEBUG, " (%t) error in reading request\n"));
  this->done ();
}

void
HTTP_Handler::write_error (void)
{
  ACE_DEBUG ((LM_DEBUG, " (%t) %s error in writing response\n",
              request_.uri ()));

  this->done ();
}

void
HTTP_Handler::timeout (void)
{
  ACE_DEBUG ((LM_DEBUG, " (%t) %s error in reading request\n",
              request_.uri ()));

  this->response_.
    error_response (HTTP_Status_Code::STATUS_INTERNAL_SERVER_ERROR,
                    "error in reading request");
}

void
HTTP_Handler::request_too_long (void)
{
  ACE_DEBUG ((LM_DEBUG, " (%t) request too long\n"));
  this->response_.
    error_response (HTTP_Status_Code::STATUS_BAD_REQUEST,
                    "request too long");
}

void
HTTP_Handler::done (void)
{
  this->factory_.destroy_http_handler (*this, this->io_);
}

HTTP_Handler_Factory::~HTTP_Handler_Factory (void)
{
}

HTTP_Handler *
Synch_HTTP_Handler_Factory::create_http_handler (void)
{
  JAWS_Synch_IO *io;
  ACE_NEW_RETURN (io, JAWS_Synch_IO, 0);
  HTTP_Handler *handler;
  ACE_NEW_RETURN (handler, HTTP_Handler (*io, *this), 0);

  return handler;
}

void
Synch_HTTP_Handler_Factory::destroy_http_handler (HTTP_Handler &handler,
                                                  JAWS_IO &io)
{
  delete &io;
  delete &handler;
}

//-------------SYNCH IO no Cache

HTTP_Handler *
No_Cache_Synch_HTTP_Handler_Factory::create_http_handler (void)
{
  JAWS_Synch_IO_No_Cache *io = 0;
  ACE_NEW_RETURN (io, JAWS_Synch_IO_No_Cache, 0);
  HTTP_Handler *handler = 0;
  ACE_NEW_RETURN (handler, HTTP_Handler (*io, *this), 0);

  return handler;
}

void
No_Cache_Synch_HTTP_Handler_Factory::destroy_http_handler (HTTP_Handler &handler,
                                                           JAWS_IO &io)
{
  delete &io;
  delete &handler;
}

//----------------

// This only works on Win32
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO)
void
Asynch_HTTP_Handler_Factory::open (ACE_HANDLE handle,
                                   ACE_Message_Block &mb)
{
  JAWS_Asynch_IO *io;
  ACE_NEW (io, JAWS_Asynch_IO);
  HTTP_Handler *handler;
  ACE_NEW (handler, HTTP_Handler (*io, *this));
  handler->open (handle, mb);
}

void
Asynch_HTTP_Handler_Factory::destroy_http_handler (HTTP_Handler &handler,
                                                   JAWS_IO &io)
{
  delete &handler;
  delete &io;
  delete this;
}

HTTP_Handler *
Asynch_HTTP_Handler_Factory::create_http_handler (void)
{
  return 0;
}
#endif /* ACE_HAS_WIN32_OVERLAPPED_IO */
