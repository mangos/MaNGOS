// $Id: Blob_Handler.cpp 91670 2010-09-08 18:02:26Z johnnyw $

#include "Blob_Handler.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_strings.h"

// Empty constructor for compliance with new Connector behavior.
ACE_Blob_Handler::ACE_Blob_Handler (void)
{
}

// Always use this constructor
ACE_Blob_Handler::ACE_Blob_Handler (ACE_Message_Block * mb,
                                    size_t length,
                                    size_t offset,
                                    ACE_TCHAR *filename) :
  mb_ (mb),
  length_ (length),
  offset_ (offset),
  filename_ (ACE_OS::strdup (filename)),
  bytecount_ (0)
{
}

ACE_Blob_Handler::~ACE_Blob_Handler (void)
{
  if (filename_)
    {
      ACE_OS::free ((void *) filename_);
      filename_ = 0;
    }
}

// Called by Connector after connection is established
int
ACE_Blob_Handler::open (void *)
{
  if (this->send_request () != 0)
    ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "ACE_Blob_Handler::open():send_request failed"), -1);

  if (this->receive_reply () != 0)
    ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "ACE_Blob_Handler::open():receive_reply failed"), -1);
  return 0;

}

// No-op
int
ACE_Blob_Handler::close (u_long flags)
{
  ACE_UNUSED_ARG (flags);
  return 0;
}


// Always overridden by the derived classes
int
ACE_Blob_Handler::send_request (void)
{
  return -1;
}

// Always overridden by the derived classes
int
ACE_Blob_Handler::receive_reply (void)
{
  return -1;
}

// used to retrieve the number of bytes read/written by the
// last operation on the Blob
int
ACE_Blob_Handler::byte_count (void)
{
  return bytecount_;
}

// Reader **************************************************

ACE_Blob_Reader::ACE_Blob_Reader (ACE_Message_Block * mb,
                                  size_t length,
                                  size_t offset,
                                  ACE_TCHAR *filename,
                                  const char *request_prefix,
                                  const char *request_suffix) :
  ACE_Blob_Handler (mb, length, offset, filename),
  request_prefix_ (request_prefix),
  request_suffix_ (request_suffix)
{
}

// Send the HTTP request
int
ACE_Blob_Reader::send_request (void)
{
  char mesg [MAX_HEADER_SIZE];

  // Check to see if the request is too big
  if (MAX_HEADER_SIZE < (ACE_OS::strlen (request_prefix_)
                         + ACE_OS::strlen (filename_)
                         + ACE_OS::strlen (request_suffix_) + 4))
    ACE_ERROR_RETURN((LM_ERROR,"Request too large!"), -1);

  // Create a message to send to the server requesting retrieval of the file
  int len = ACE_OS::sprintf (mesg, "%s %s %s", request_prefix_, filename_, request_suffix_);

  // Send the message to server
  if (peer ().send_n (mesg, len) != len)
    ACE_ERROR_RETURN((LM_ERROR,"Error sending request"), -1);


  return 0;
}

// Recieve the HTTP Reply
int
ACE_Blob_Reader::receive_reply (void)
{
  ssize_t len;
  char buf [MAX_HEADER_SIZE + 1];
  char *buf_ptr;
  size_t bytes_read = 0;
  size_t bytes_left = this->length_;
  size_t offset_left = this->offset_;

  // Receive the first MAX_HEADER_SIZE bytes to be able to strip off the
  // header. Note that we assume that the header will fit into the
  // first MAX_HEADER_SIZE bytes of the transmitted data.
  if ((len = peer ().recv_n (buf, MAX_HEADER_SIZE)) >= 0)
    {
      buf[len] = '\0';

      // Search for the header termination string "\r\n\r\n", or "\n\n". If
      // found, move past it to get to the data portion.
      if ((buf_ptr = ACE_OS::strstr (buf,"\r\n\r\n")) != 0)
        buf_ptr += 4;
      else if ((buf_ptr = ACE_OS::strstr (buf, "\n\n")) != 0)
        buf_ptr += 2;
      else
        buf_ptr = buf;

      // Determine number of data bytes read. This is equal to the
      // total bytes read minus number of header bytes.
      bytes_read = (buf + len) - buf_ptr;
    }
  else
    ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "ACE_Blob_Reader::receiveReply():Error while reading header"), -1);

  // ***************************************************************
  // At this point, we have stripped off the header and are ready to
  // process data. buf_ptr points to the data

  // First adjust for offset. There are two cases:
  // (1) The first block of data encountered the offset. In this case
  // we simply increment the buf_ptr by offset.
  // (2) The first block of data did not encounter the offset. That
  // is, the offset needs to go past the number of data bytes already read.
  if (bytes_read > offset_left)
    {
      // The first case is true -- that is offset is less than the
      // data bytes we just read.
      buf_ptr += offset_left;

      // Determine how many data bytes are actually there. This is
      // basically the total number of data bytes we read minus any
      // offset we have.
      size_t data_bytes = bytes_read - offset_left;

      // Check for the case where the bytes read are enough to fulfill
      // our request (for length bytes). If this is the case, then we
      // don't need to do any extra recvs and can simply return with
      // the data.
      if (data_bytes >= bytes_left)
        {
          // The first block contains enough data to satisfy the
          // length. So copy the data into the message buffer.
          if (mb_->copy (buf_ptr, bytes_left) == -1)
            ACE_ERROR_RETURN ((LM_ERROR, "%p\n",
                               "ACE Blob_Reader::receiveReply():Error copying data into Message_Block"), -1);
          bytecount_ = length_;
          return 0;
        }

      // Copy over all the data bytes into our message buffer.
      if (mb_->copy (buf_ptr, data_bytes) == -1)
        ACE_ERROR_RETURN ((LM_ERROR, "%p\n",
                           "ACE_Blob_Reader::receiveReply():Error copying data into Message_Block" ), -1);

      // Adjust bytes left
      bytes_left -= data_bytes;

      // No more offset left. So set it to zero.
      offset_left = 0;
    }
  else
    {
      // The second case is true -- that is offset is greater than
      // the data bytes we just read.
     offset_left -= bytes_read;
    }

  // If we ad any offset left, take care of that.
  while (offset_left > 0)
    {
      // MAX_HEADER_SIZE in which case we should do a receive of
      // offset bytes into a temporary buffer. Otherwise, we should
      // receive MAX_HEADER_SIZE bytes into temporary buffer and
      // decrement offset_left.
      if (offset_left < (sizeof buf))
        len = offset_left;
      else
        len = sizeof buf;
      if (peer().recv_n (buf, len) != len)
        ACE_ERROR_RETURN ((LM_ERROR, "%p\n",
                           "ACE_Blob_Reader::receiveReply():Read error" ),
                          -1);
      offset_left -= len;
    }

  // *****************************************************************
  // At this point we are all set to receive the actual data which the
  // user wants. We have made adjustments for offset and are ready to
  // receive the actual data. Receive the data directly into the
  // message buffer.

  len = peer().recv_n (mb_->wr_ptr (), bytes_left);

  if (len < 0 || static_cast<size_t> (len) != bytes_left)
    ACE_ERROR_RETURN ((LM_ERROR, "%p\n",
                       "ACE_Blob_Reader::receiveReply():Read error" ),
                      -1);

  // Adjust the message buffer write pointer by number of bytes we
  // received.
  mb_->wr_ptr (len);

  // Set the byte count to number of bytes received
  this->bytecount_ = length_;

  return 0;
}

// Writer **************************************************

ACE_Blob_Writer::ACE_Blob_Writer (ACE_Message_Block * mb,
                                  size_t length,
                                  size_t offset,
                                  ACE_TCHAR *filename,
                                  const char *request_prefix,
                                  const char *request_suffix) :
  ACE_Blob_Handler (mb, length, offset, filename),
  request_prefix_ (request_prefix),
  request_suffix_ (request_suffix)
{
}

int
ACE_Blob_Writer::send_request (void)
{
  // Check for sanity -- check if we have any data to send.
  if (offset_+ length_ > mb_->length ())
    ACE_ERROR_RETURN((LM_ERROR, "%p\n",
                      "ACE_Blob_Writer::sendRequest():Invalid offset/length"), -1);

  // Determine the length of the header message we will be sending to
  // the server. Note that we add 32 for safety -- this corresponds to
  // the number of bytes needed for the length field.
  size_t mesglen =
    ACE_OS::strlen (request_prefix_)
    + ACE_OS::strlen (filename_)
    + ACE_OS::strlen (request_suffix_)
    + 32; // safety

  // Allocate a buffer to hold the header
  char *mesg = 0;
  ACE_NEW_RETURN (mesg, char [mesglen], -1);

  // Create the header, store the actual length in mesglen.
  mesglen = ACE_OS::sprintf (mesg, "%s /%s %s " ACE_SIZE_T_FORMAT_SPECIFIER_ASCII "\n\n",
                             request_prefix_, filename_, request_suffix_, length_);

  // Send the header followed by the data

  // First send the header
  if (peer ().send_n (mesg, mesglen) == -1)
    ACE_ERROR_RETURN((LM_ERROR, "%p\n", "Error sending request"), -1);

  // "Consume" the offset by moving the read pointer of the message
  // buffer
  mb_->rd_ptr (offset_);

  // Now send the data
  if (peer ().send_n (mb_->rd_ptr (), length_) != (int)length_)
    ACE_ERROR_RETURN((LM_ERROR, "%p\n", "Error sending file"), -1);

  // Adjust the read pointer of the mesage buffer
  mb_->rd_ptr (length_);

  return 0;
}

int
ACE_Blob_Writer::receive_reply (void)
{
  // Allocate a buffer big enough to hold the header
  char buf[MAX_HEADER_SIZE];

  // Receive the reply from the server
  size_t num_recvd = 0;
  ssize_t len = peer ().recv_n (buf, sizeof buf - 1, 0, &num_recvd); // reserve one byte to store the \0
  if (len ==-1)
    ACE_ERROR_RETURN((LM_ERROR, "%p\n", "Error reading header"), -1);

  buf [num_recvd] = 0;

  // Parse the header
  char *lasts = 0;

  // First check if this was a valid header -- HTTP/1.0
  char *token = ACE_OS::strtok_r (buf, " \t", &lasts);

  if ( (token == 0) || (ACE_OS::strcasecmp (token, "HTTP/1.0") != 0))
    ACE_ERROR_RETURN((LM_ERROR, "%p\n", "Did not receive a HTTP/1.0 response"), -1);

  // Get the return code.
  int return_code = ACE_OS::atoi (ACE_OS::strtok_r (0, " \t", &lasts));

  // Check if the transaction succeeded. The only success codes are in
  // the range of 200-299 (HTTP specification).
  if (return_code >= 200 && return_code < 300)
    return 0;
  else
    {
      // Something went wrong!
      // Get the description from the header message of what went wrong.
      char *description = ACE_OS::strtok_r (0, "\n\r", &lasts);
      ACE_ERROR_RETURN((LM_ERROR, "%p\n", description), -1);
    }
  ACE_NOTREACHED(return 0);
}

