// $Id: Jaws_IO.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "ace/Message_Block.h"
#include "ace/SOCK_Stream.h"
#include "ace/Filecache.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_sys_uio.h"
#include "ace/OS_NS_sys_socket.h"
#include "ace/Min_Max.h"

#include "JAWS/Data_Block.h"
#include "JAWS/Policy.h"
#include "JAWS/Jaws_IO.h"
#include "JAWS/IO_Handler.h"
#include "JAWS/IO_Acceptor.h"
#include "JAWS/Filecache.h"

#include "ace/Asynch_IO.h"  //for ACE_Asynch_Write_Stream

// #include "HTTP_Helpers.h"



JAWS_IO::JAWS_IO (void)
  : handle_ (ACE_INVALID_HANDLE),
    handler_ (0),
    inet_addr_ (0),
    acceptor_ (0)
{
}

JAWS_IO::~JAWS_IO (void)
{
}

#if 0
ACE_HANDLE
JAWS_IO::handle (void)
{
  return this->handle_;
}

void
JAWS_IO::handle (ACE_HANDLE handle)
{
  this->handle_ = handle;
}

void
JAWS_IO::handler (JAWS_IO_Handler *handler)
{
  this->handler_ = handler;
}

void
JAWS_IO::acceptor (JAWS_IO_Acceptor *acceptor)
{
  this->acceptor_ = acceptor;
}
#endif /* 0 */

JAWS_Synch_IO::JAWS_Synch_IO (void)
{
  this->acceptor_ = JAWS_IO_Synch_Acceptor_Singleton::instance ();
}

JAWS_Synch_IO::~JAWS_Synch_IO (void)
{
  if (this->handle_ != ACE_INVALID_HANDLE)
    ACE_OS::closesocket (this->handle_);
}

void
JAWS_Synch_IO::accept (JAWS_IO_Handler *ioh,
                       ACE_Message_Block *,
                       unsigned int)
{
  ACE_SOCK_Stream new_stream;
  new_stream.set_handle (ACE_INVALID_HANDLE);
  if (this->acceptor_->accept (new_stream) == -1)
    ioh->accept_error ();
  else
    ioh->accept_complete (new_stream.get_handle ());
}

void
JAWS_Synch_IO::read (JAWS_IO_Handler *ioh,
                     ACE_Message_Block *mb,
                     unsigned int size)
{
  JAWS_TRACE ("JAWS_Synch_IO::read");

  ACE_SOCK_Stream stream;

  stream.set_handle (ioh->handle ());
  int result = stream.recv (mb->wr_ptr (), size);

  if (result <= 0)
    ioh->read_error ();
  else
    {
      JAWS_TRACE ("JAWS_Synch_IO::read success");
      mb->wr_ptr (result);
      ioh->read_complete (mb);
    }
}

void
JAWS_Synch_IO::receive_file (JAWS_IO_Handler *ioh,
                             const char *filename,
                             void *initial_data,
                             unsigned int initial_data_length,
                             unsigned int entire_length)
{
  ACE_Filecache_Handle handle (filename,
                               (int) entire_length);

  int result = handle.error ();

  if (result == ACE_Filecache_Handle::ACE_SUCCESS)
    {
      ACE_SOCK_Stream stream;
      stream.set_handle (ioh->handle ());

      int bytes_to_memcpy = ACE_MIN (entire_length, initial_data_length);
      ACE_OS::memcpy (handle.address (), initial_data, bytes_to_memcpy);

      int bytes_to_read = entire_length - bytes_to_memcpy;

      int bytes = stream.recv_n ((char *)
                                 handle.address () + initial_data_length,
                                 bytes_to_read);
      if (bytes == bytes_to_read)
        ioh->receive_file_complete ();
      else
        result = -1;
    }

  if (result != ACE_Filecache_Handle::ACE_SUCCESS)
    ioh->receive_file_error (result);
}

void
JAWS_Synch_IO::transmit_file (JAWS_IO_Handler *ioh,
                              ACE_HANDLE handle,
                              const char *header,
                              unsigned int header_size,
                              const char *trailer,
                              unsigned int trailer_size)
{
  int result = 0;

  if (handle != ACE_INVALID_HANDLE)
    {
      ACE_SOCK_Stream stream;
      stream.set_handle (ioh->handle ());

      if ((unsigned long) stream.send_n (header, header_size) < header_size)
        {
          result = -1;
        }
      else
        {
          int count;
          char buf[BUFSIZ];

          do
            {
              count = ACE_OS::read (handle, buf, sizeof (buf));
              if (count <= 0)
                break;

              if (stream.send_n (buf, count) < count)
                {
                result = -1;
                }
            }
          while (result == 0);

          if ((unsigned long) stream.send_n (trailer, trailer_size)
              < trailer_size)
            {
            result = -1;
            }
        }
    }

  if (result == 0)
    ioh->transmit_file_complete ();
  else
    ioh->transmit_file_error (result);
}

void
JAWS_Synch_IO::transmit_file (JAWS_IO_Handler *ioh,
                              const char *filename,
                              const char *header,
                              unsigned int header_size,
                              const char *trailer,
                              unsigned int trailer_size)
{
  int result = 0;

  if (filename == 0)
    {
      ioh->transmit_file_error (-1);
      return;
    }

  JAWS_Cached_FILE cf (filename);

  if (cf.file ()->get_handle () != ACE_INVALID_HANDLE
      && cf.mmap () != 0)
    {
#if defined (ACE_JAWS_BASELINE) || defined (ACE_WIN32)
      ACE_FILE_Info info;
      cf.file ()->get_info (info);

      if (cf.file ()->get_info (info) == 0 && info.size_ > 0)
        {
          ACE_SOCK_Stream stream;
          stream.set_handle (ioh->handle ());
          if (((u_long) stream.send_n (header, header_size) == header_size)
              && (stream.send_n (cf.mmap ()->addr (), info.size_)
                  == info.size_)
              && ((u_long) stream.send_n (trailer, trailer_size)
                  == trailer_size))
            {
              ioh->transmit_file_complete ();
              return;
            }
          else
            {
              result = -1;
            }
        }
      else
        {
          result = -1;
        }
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

      ACE_FILE_Info info;

      if (cf.file ()->get_info (info) == 0 && info.size_ > 0)
        {
          iov[iovcnt].iov_base = (char *) cf.mmap ()->addr ();
          iov[iovcnt].iov_len = info.size_;
          iovcnt++;
        }
      if (trailer_size > 0)
        {
          iov[iovcnt].iov_base = const_cast<char*> (trailer);
          iov[iovcnt].iov_len = trailer_size;
          iovcnt++;
        }
      if (ACE_OS::writev (ioh->handle (), iov, iovcnt) < 0)
        {
          result = -1;
        }
      else
        {
          ioh->transmit_file_complete ();
          return;
        }
#endif /* ACE_JAWS_BASELINE */
    }
  else if (cf.file ()->get_handle () != ACE_INVALID_HANDLE
           && cf.mmap () == 0)
    {
      this->transmit_file (ioh,
                           cf.file ()->get_handle (),
                           header, header_size,
                           trailer, trailer_size);
      return;
    }
  else
    {
      result = -1;
    }

  if (result != 0)
    {
      ioh->transmit_file_error (result);
    }
}

void
JAWS_Synch_IO::send_confirmation_message (JAWS_IO_Handler *ioh,
                                          const char *buffer,
                                          unsigned int length)
{
  this->send_message (ioh, buffer, length);
  ioh->confirmation_message_complete ();
}

void
JAWS_Synch_IO::send_error_message (JAWS_IO_Handler *ioh,
                                   const char *buffer,
                                   unsigned int length)
{
  this->send_message (ioh, buffer, length);
  ioh->error_message_complete ();
}

void
JAWS_Synch_IO::send_message (JAWS_IO_Handler *ioh,
                             const char *buffer,
                             unsigned int length)
{
  ACE_SOCK_Stream stream;
  stream.set_handle (ioh->handle ());
  stream.send_n (buffer, length);
}

// This only works on asynch I/O-capable systems.
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)

JAWS_Asynch_IO::JAWS_Asynch_IO (void)
{
}

JAWS_Asynch_IO::~JAWS_Asynch_IO (void)
{
  if (this->handle_ != ACE_INVALID_HANDLE)
    ACE_OS::closesocket (this->handle_);
}

void
JAWS_Asynch_IO::accept (JAWS_IO_Handler *ioh,
                        ACE_Message_Block *,
                        unsigned int)
{
  JAWS_TRACE ("JAWS_Asynch_IO::accept");

  ioh->idle ();

  JAWS_Data_Block *db = ioh->message_block ();
  //ACE_HANDLE listen_handle = db->policy ()->acceptor ()->get_handle ();

  //JAWS_Asynch_IO_Handler *aioh =
  //  dynamic_cast<JAWS_Asynch_IO_Handler *> (ioh);

  size_t bytes_to_read = JAWS_Data_Block::JAWS_DATA_BLOCK_SIZE;

  if (db->policy ()->acceptor ()->accept (bytes_to_read, ioh) == -1)
    ioh->accept_error ();
}

void
JAWS_Asynch_IO::read (JAWS_IO_Handler *ioh,
                      ACE_Message_Block* mb,
                      unsigned int size)
{
  JAWS_TRACE ("JAWS_Asynch_IO::read");

  ioh->idle ();

  JAWS_Asynch_IO_Handler *aioh =
    dynamic_cast<JAWS_Asynch_IO_Handler *> (ioh);

  ACE_Asynch_Read_Stream ar;

  if (ar.open (*(aioh->handler ()), aioh->handle ()) == -1
      || ar.read (*mb, size) == -1)
    aioh->read_error ();
}

void
JAWS_Asynch_IO::receive_file (JAWS_IO_Handler *ioh,
                              const char *filename,
                              void *initial_data,
                              unsigned int initial_data_length,
                              unsigned int entire_length)
{
  JAWS_TRACE ("JAWS_Asynch_IO::receive_file");

  ioh->idle ();

  JAWS_Asynch_IO_Handler *aioh =
    dynamic_cast<JAWS_Asynch_IO_Handler *> (ioh);

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

          if (ar.open (*(aioh->handler ()), aioh->handle ()) == -1
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
JAWS_Asynch_IO::transmit_file (JAWS_IO_Handler *ioh,
                               ACE_HANDLE handle,
                               const char *header,
                               unsigned int header_size,
                               const char *trailer,
                               unsigned int trailer_size)
{
  JAWS_TRACE ("JAWS_Asynch_IO::transmit_file");

  ioh->idle ();

  JAWS_Asynch_IO_Handler *aioh =
    dynamic_cast<JAWS_Asynch_IO_Handler *> (ioh);

  ACE_Asynch_Transmit_File::Header_And_Trailer *header_and_trailer = 0;

  int result = 0;

  if (handle != ACE_INVALID_HANDLE)
    {
      ACE_Message_Block hdr_mb (header, header_size);
      ACE_Message_Block trl_mb (trailer, trailer_size);

      header_and_trailer =
        new ACE_Asynch_Transmit_File::Header_And_Trailer (hdr_mb.duplicate (),
                                                          header_size,
                                                          trl_mb.duplicate (),
                                                          trailer_size);

      ACE_Asynch_Transmit_File tf;

      if (tf.open (*(aioh->handler ()), aioh->handle ()) == -1
          || tf.transmit_file (handle, // file handle
                               header_and_trailer, // header and trailer data
                               0,  // bytes_to_write
                               0,  // offset
                               0,  // offset_high
                               0,  // bytes_per_send
                               0,  // flags
                               0 // act
                               ) == -1)
        result = -1;
    }

  if (result != 0)
    {
      ioh->transmit_file_error (result);
      delete header_and_trailer;
    }
}

void
JAWS_Asynch_IO::transmit_file (JAWS_IO_Handler *ioh,
                               const char *filename,
                               const char *header,
                               unsigned int header_size,
                               const char *trailer,
                               unsigned int trailer_size)
{
  int result = 0;

  JAWS_TRACE ("JAWS_Asynch_IO::transmit_file");

  ioh->idle ();

  JAWS_Asynch_IO_Handler *aioh =
    dynamic_cast<JAWS_Asynch_IO_Handler *> (ioh);

  ACE_Asynch_Transmit_File::Header_And_Trailer *header_and_trailer = 0;
  JAWS_Cached_FILE *cf = new JAWS_Cached_FILE (filename);

  if (cf->file ()->get_handle () != ACE_INVALID_HANDLE)
    {
      ACE_Message_Block hdr_mb (header, header_size);
      ACE_Message_Block trl_mb (trailer, trailer_size);

      header_and_trailer = new ACE_Asynch_Transmit_File::Header_And_Trailer
        (hdr_mb.duplicate (), header_size, trl_mb.duplicate (), trailer_size);

      ACE_Asynch_Transmit_File tf;

      if (tf.open (*(aioh->handler ()), aioh->handle ()) == -1
          || tf.transmit_file (cf->file ()->get_handle (), // file handle
                               header_and_trailer, // header and trailer data
                               0,  // bytes_to_write
                               0,  // offset
                               0,  // offset_high
                               0,  // bytes_per_send
                               0,  // flags
                               cf // act
                               ) == -1)
        result = -1;
    }

  if (result != 0)
    {
      ioh->transmit_file_error (result);
      delete header_and_trailer;
      delete cf;
    }
}

void
JAWS_Asynch_IO::send_confirmation_message (JAWS_IO_Handler *ioh,
                                           const char *buffer,
                                           unsigned int length)
{
  this->send_message (ioh, buffer, length, CONFIRMATION);
}

void
JAWS_Asynch_IO::send_error_message (JAWS_IO_Handler *ioh,
                                    const char *buffer,
                                    unsigned int length)
{
  this->send_message (ioh, buffer, length, ERROR_MESSAGE);
}

void
JAWS_Asynch_IO::send_message (JAWS_IO_Handler *ioh,
                              const char *buffer,
                              unsigned int length,
                              long act)
{
  ioh->idle ();

  JAWS_Asynch_IO_Handler *aioh =
    dynamic_cast<JAWS_Asynch_IO_Handler *> (ioh);

  ACE_Message_Block *mb = 0;
  ACE_NEW (mb, ACE_Message_Block (buffer, length));

  if (mb == 0)
    {
      this->handler_->error_message_complete ();
      return;
    }

  ACE_Asynch_Write_Stream aw;
  if (aw.open (*(aioh->handler ()), aioh->handle ()) == -1
      || aw.write (*mb, length, (void *) static_cast<intptr_t> (act)) == -1)
    {
      mb->release ();

      if (act == CONFIRMATION)
        ioh->confirmation_message_complete ();
      else
        ioh->error_message_complete ();
    }
}

void
JAWS_Asynch2_IO::accept (JAWS_IO_Handler *,
                         ACE_Message_Block *,
                         unsigned int)
{
}

#endif /* ACE_HAS_WIN32_OVERLAPPED_IO || ACE_HAS_AIO_CALLS */

