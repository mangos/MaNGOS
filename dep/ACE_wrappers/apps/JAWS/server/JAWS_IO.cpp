// $Id: JAWS_IO.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#include "ace/OS_NS_string.h"
#include "ace/OS_NS_sys_uio.h"
#include "ace/OS_NS_sys_socket.h"
#include "ace/Message_Block.h"
#include "ace/Min_Max.h"
#include "ace/SOCK_Stream.h"
#include "ace/Filecache.h"
#include "JAWS_IO.h"
#include "HTTP_Helpers.h"

#include "ace/OS_NS_fcntl.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_sys_stat.h"
#include "ace/Auto_Ptr.h"
#include "ace/Basic_Types.h"

JAWS_IO::JAWS_IO (void)
  : handler_ (0)
{
}

JAWS_IO::~JAWS_IO (void)
{
}

void
JAWS_IO::handler (JAWS_IO_Handler *handler)
{
  this->handler_ = handler;
}

JAWS_IO_Handler::~JAWS_IO_Handler (void)
{
}

JAWS_Synch_IO::JAWS_Synch_IO (void)
  : handle_ (ACE_INVALID_HANDLE)
{
}

JAWS_Synch_IO::~JAWS_Synch_IO (void)
{
  ACE_OS::closesocket (this->handle_);
}

ACE_HANDLE
JAWS_Synch_IO::handle (void) const
{
  return this->handle_;
}

void
JAWS_Synch_IO::handle (ACE_HANDLE handle)
{
  this->handle_ = handle;
}

void
JAWS_Synch_IO::read (ACE_Message_Block &mb,
                     int size)
{
  ACE_SOCK_Stream stream;
  stream.set_handle (this->handle_);
  int result = stream.recv (mb.wr_ptr (), size);

  if (result <= 0)
    this->handler_->read_error ();
  else
    {
      mb.wr_ptr (result);
      this->handler_->read_complete (mb);
    }
}

void
JAWS_Synch_IO::receive_file (const char *filename,
                             void *initial_data,
                             int initial_data_length,
                             int entire_length)
{
  ACE_Filecache_Handle handle (ACE_TEXT_CHAR_TO_TCHAR (filename), entire_length);

  int result = handle.error ();

  if (result == ACE_Filecache_Handle::ACE_SUCCESS)
    {
      ACE_SOCK_Stream stream;
      stream.set_handle (this->handle_);

      int bytes_to_memcpy = ACE_MIN (entire_length, initial_data_length);
      ACE_OS::memcpy (handle.address (), initial_data, bytes_to_memcpy);

      int bytes_to_read = entire_length - bytes_to_memcpy;

      int bytes = stream.recv_n ((char *) handle.address () + initial_data_length,
                                 bytes_to_read);
      if (bytes == bytes_to_read)
        this->handler_->receive_file_complete ();
      else
        result = -1;
    }

  if (result != ACE_Filecache_Handle::ACE_SUCCESS)
    this->handler_->receive_file_error (result);
}

void
JAWS_Synch_IO::transmit_file (const char *filename,
                              const char *header,
                              int header_size,
                              const char *trailer,
                              int trailer_size)
{
  ACE_Filecache_Handle handle (ACE_TEXT_CHAR_TO_TCHAR (filename));

  int result = handle.error ();

  if (result == ACE_Filecache_Handle::ACE_SUCCESS)
    {
#if defined (ACE_JAWS_BASELINE) || defined (ACE_WIN32)
      ACE_SOCK_Stream stream;
      stream.set_handle (this->handle_);

      if ((stream.send_n (header, header_size) == header_size)
          && (stream.send_n (handle.address (), handle.size ())
              == handle.size ())
          && (stream.send_n (trailer, trailer_size) == trailer_size))
        this->handler_->transmit_file_complete ();
      else
        result = -1;
#else
      // Attempting to use writev
      // Is this faster?
      iovec iov[3];
      int iovcnt = 0;
      if (header_size > 0)
        {
          iov[iovcnt].iov_base = const_cast<char*> (header);
          iov[iovcnt].iov_len =  header_size;
          iovcnt++;
        }
      if (handle.size () > 0)
        {
          iov[iovcnt].iov_base =  reinterpret_cast<char*> (handle.address ());
          iov[iovcnt].iov_len = handle.size ();
          iovcnt++;
        }
      if (trailer_size > 0)
        {
          iov[iovcnt].iov_base = const_cast<char*> (trailer);
          iov[iovcnt].iov_len = trailer_size;
          iovcnt++;
        }
      if (ACE_OS::writev (this->handle_, iov, iovcnt) < 0)
        result = -1;
      else
        this->handler_->transmit_file_complete ();
#endif /* ACE_JAWS_BASELINE */
    }

  if (result != ACE_Filecache_Handle::ACE_SUCCESS)
    this->handler_->transmit_file_error (result);
}

void
JAWS_Synch_IO::send_confirmation_message (const char *buffer,
                                          int length)
{
  this->send_message (buffer, length);
  this->handler_->confirmation_message_complete ();
}

void
JAWS_Synch_IO::send_error_message (const char *buffer,
                                   int length)
{
  this->send_message (buffer, length);
  this->handler_->error_message_complete ();
}

void
JAWS_Synch_IO::send_message (const char *buffer,
                             int length)
{
  ACE_SOCK_Stream stream;
  stream.set_handle (this->handle_);
  stream.send_n (buffer, length);
}

// This only works on Win32
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO)

JAWS_Asynch_IO::JAWS_Asynch_IO (void)
{
}

JAWS_Asynch_IO::~JAWS_Asynch_IO (void)
{
  ACE_OS::closesocket (this->handle_);
}

void
JAWS_Asynch_IO::read (ACE_Message_Block& mb,
                      int size)
{
  ACE_Asynch_Read_Stream ar;

  if (ar.open (*this, this->handle_) == -1
      || ar.read (mb, size) == -1)
    this->handler_->read_error ();
}

// This method will be called when an asynchronous read completes on a
// stream.

void
JAWS_Asynch_IO::handle_read_stream (const ACE_Asynch_Read_Stream::Result &result)
{
  // This callback is for this->receive_file()
  if (result.act () != 0)
    {
      int code = 0;
      if (result.success () && result.bytes_transferred () != 0)
        {
          if (result.message_block ().length () == result.message_block ().size ())
            code = ACE_Filecache_Handle::ACE_SUCCESS;
          else
            {
              ACE_Asynch_Read_Stream ar;
              if (ar.open (*this, this->handle_) == -1
                  || ar.read (result.message_block (),
                              result.message_block ().size () - result.message_block ().length (),
                              result.act ()) == -1)
                code = -1;
              else
                return;
            }
        }
      else
        code = -1;

      if (code == ACE_Filecache_Handle::ACE_SUCCESS)
        this->handler_->receive_file_complete ();
      else
        this->handler_->receive_file_error (code);

      delete &result.message_block ();
      delete (ACE_Filecache_Handle *) result.act ();
    }
  else
    {
      // This callback is for this->read()
      if (result.success ()
          && result.bytes_transferred () != 0)
        this->handler_->read_complete (result.message_block ());
      else
        this->handler_->read_error ();
    }
}

void
JAWS_Asynch_IO::receive_file (const char *filename,
                              void *initial_data,
                              int initial_data_length,
                              int entire_length)
{
  ACE_Message_Block *mb = 0;
  ACE_Filecache_Handle *handle;

  ACE_NEW (handle, ACE_Filecache_Handle (filename, entire_length, ACE_NOMAP));

  int result = handle->error ();

  if (result == ACE_Filecache_Handle::ACE_SUCCESS)
    {
      ACE_OS::memcpy (handle->address (),
                      initial_data,
                      initial_data_length);

      int bytes_to_read = entire_length - initial_data_length;

      ACE_NEW (mb, ACE_Message_Block ((char *)handle->address ()
                                      + initial_data_length, bytes_to_read));

      if (mb == 0)
        {
          errno = ENOMEM;
          result = -1;
        }
      else
        {
          ACE_Asynch_Read_Stream ar;

          if (ar.open (*this, this->handle_) == -1
              || ar.read (*mb, mb->size () - mb->length (), handle) == -1)
            result = -1;
        }
    }

  if (result != ACE_Filecache_Handle::ACE_SUCCESS)
    {
      this->handler_->receive_file_error (result);
      delete mb;
      delete handle;
    }
}

void
JAWS_Asynch_IO::transmit_file (const char *filename,
                               const char *header,
                               int header_size,
                               const char *trailer,
                               int trailer_size)
{
  ACE_Asynch_Transmit_File::Header_And_Trailer *header_and_trailer = 0;
  ACE_Filecache_Handle *handle = new ACE_Filecache_Handle (filename, ACE_NOMAP);

  int result = handle->error ();

  if (result == ACE_Filecache_Handle::ACE_SUCCESS)
    {
      ACE_Message_Block header_mb (header, header_size);
      ACE_Message_Block trailer_mb (trailer, trailer_size);

      header_and_trailer = new ACE_Asynch_Transmit_File::Header_And_Trailer
        (&header_mb, header_size, &trailer_mb, trailer_size);

      ACE_Asynch_Transmit_File tf;

      if (tf.open (*this, this->handle_) == -1
          || tf.transmit_file (handle->handle (), // file handle
                               header_and_trailer, // header and trailer data
                               0,  // bytes_to_write
                               0,  // offset
                               0,  // offset_high
                               0,  // bytes_per_send
                               0,  // flags
                               handle // act
                               ) == -1)
        result = -1;
    }

  if (result != ACE_Filecache_Handle::ACE_SUCCESS)
    {
      this->handler_->transmit_file_error (result);
      delete header_and_trailer;
      delete handle;
    }
}


// This method will be called when an asynchronous transmit file completes.
void
JAWS_Asynch_IO::handle_transmit_file (const ACE_Asynch_Transmit_File::Result &result)
{
  if (result.success ())
    this->handler_->transmit_file_complete ();
  else
    this->handler_->transmit_file_error (-1);

  delete result.header_and_trailer ();
  delete (ACE_Filecache_Handle *) result.act ();
}

void
JAWS_Asynch_IO::send_confirmation_message (const char *buffer,
                                           int length)
{
  this->send_message (buffer, length, CONFORMATION);
}

void
JAWS_Asynch_IO::send_error_message (const char *buffer,
                                    int length)
{
  this->send_message (buffer, length, ERROR_MESSAGE);
}

void
JAWS_Asynch_IO::send_message (const char *buffer,
                              int length,
                              int act)
{
  ACE_Message_Block *mb = 0;
  ACE_NEW (mb, ACE_Message_Block (buffer, length));

  if (mb == 0)
    {
      this->handler_->error_message_complete ();
      return;
    }

  ACE_Asynch_Write_Stream aw;
  if (aw.open (*this, this->handle_) == -1
      || aw.write (*mb, length, (void *) static_cast<intptr_t> (act)) == -1)
    {
      mb->release ();

      if (act == CONFORMATION)
        this->handler_->confirmation_message_complete ();
      else
        this->handler_->error_message_complete ();
    }
}

void
JAWS_Asynch_IO::handle_write_stream (const ACE_Asynch_Write_Stream::Result &result)
{
  result.message_block ().release ();

  if (result.act () == (void *) CONFORMATION)
    this->handler_->confirmation_message_complete ();
  else
    this->handler_->error_message_complete ();
}

#endif /* ACE_HAS_WIN32_OVERLAPPED_IO */

//-------------------Adding SYNCH IO no Caching

JAWS_Synch_IO_No_Cache::JAWS_Synch_IO_No_Cache (void)
  : handle_ (ACE_INVALID_HANDLE)
{
}

JAWS_Synch_IO_No_Cache::~JAWS_Synch_IO_No_Cache (void)
{
  ACE_OS::closesocket (this->handle_);
}

ACE_HANDLE
JAWS_Synch_IO_No_Cache::handle (void) const
{
  return this->handle_;
}

void
JAWS_Synch_IO_No_Cache::handle (ACE_HANDLE handle)
{
  this->handle_ = handle;
}

void
JAWS_Synch_IO_No_Cache::read (ACE_Message_Block &mb, int size)
{
  ACE_SOCK_Stream stream;
  stream.set_handle (this->handle_);
  int result = stream.recv (mb.wr_ptr (), size);

  if (result <= 0)
    this->handler_->read_error ();
  else
    {
      mb.wr_ptr (result);
      this->handler_->read_complete (mb);
    }
}

void
JAWS_Synch_IO_No_Cache::receive_file (const char *,
                                      void *,
                                      int,
                                      int)
{
  //ugly hack to send HTTP_Status_Code::STATUS_FORBIDDEN
  this->handler_->receive_file_error (5);
}

void
JAWS_Synch_IO_No_Cache::transmit_file (const char *filename,
                                       const char *header,
                                       int header_size,
                                       const char *trailer,
                                       int trailer_size)
{
  int result = 0;

  // Can we access the file?
  if (ACE_OS::access (filename, R_OK) == -1)
  {
    //ugly hack to send in HTTP_Status_Code::STATUS_NOT_FOUND
    result = ACE_Filecache_Handle::ACE_ACCESS_FAILED;
    this->handler_->transmit_file_error (result);
    return;
  }

  ACE_stat stat;

  // Can we stat the file?
  if (ACE_OS::stat (filename, &stat) == -1)
  {
    //ugly hack to send HTTP_Status_Code::STATUS_FORBIDDEN
    result = ACE_Filecache_Handle::ACE_STAT_FAILED;
    this->handler_->transmit_file_error (result);
    return;
  }

  ACE_OFF_T size = stat.st_size;

  // Can we open the file?
  ACE_HANDLE handle = ACE_OS::open (filename, O_RDONLY);
  if (handle == ACE_INVALID_HANDLE)
  {
    //ugly hack to send HTTP_Status_Code::STATUS_FORBIDDEN
    result = ACE_Filecache_Handle::ACE_OPEN_FAILED;
    this->handler_->transmit_file_error (result);
    return;
  }

  char* f = new char[size];
  ACE_Auto_Basic_Array_Ptr<char> file (f);

  ACE_OS::read_n (handle, f, size);

  ACE_SOCK_Stream stream;
  stream.set_handle (this->handle_);

  if ((stream.send_n (header, header_size) == header_size)
      && (stream.send_n (f, size) == size)
      && (stream.send_n (trailer, trailer_size) == trailer_size))
  {
    this->handler_->transmit_file_complete ();
  }
  else
  {
    //ugly hack to default to HTTP_Status_Code::STATUS_INTERNAL_SERVER_ERROR
    result = -1;
    this->handler_->transmit_file_error (result);
  }

  ACE_OS::close (handle);
}

void
JAWS_Synch_IO_No_Cache::send_confirmation_message (const char *buffer,
                                                   int length)
{
  this->send_message (buffer, length);
  this->handler_->confirmation_message_complete ();
}

void
JAWS_Synch_IO_No_Cache::send_error_message (const char *buffer, int length)
{
  this->send_message (buffer, length);
  this->handler_->error_message_complete ();
}

void
JAWS_Synch_IO_No_Cache::send_message (const char *buffer, int length)
{
  ACE_SOCK_Stream stream;
  stream.set_handle (this->handle_);
  stream.send_n (buffer, length);
}

