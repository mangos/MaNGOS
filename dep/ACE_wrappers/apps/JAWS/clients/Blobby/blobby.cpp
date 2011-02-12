// $Id: blobby.cpp 91670 2010-09-08 18:02:26Z johnnyw $

//============================================================================
//
// = LIBRARY
//    JAWS
//
// = FILENAME
//    blobby.c
//
// = DESCRIPTION
//     Simple client application to illustrate the use of the ACE_Blob class
//
//     It reads "length" number of bytes, after skipping offset "offset"
//     from hostname, port and filename as specified. (if -r specified)
//
//     It writes "length" number of bytes, after skipping offset "offset"
//     to hostname, port and filename as specified (if -w specified)
//
// = AUTHOR
//    Prashant Jain and Sumedh Mungee
//
//============================================================================

#include "Options.h"
#include "ace/OS_main.h"
#include "ace/OS_NS_fcntl.h"
#include "ace/OS_NS_unistd.h"

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  // Options is a singleton
  Options *options = Options::instance ();
  options->parse_args (argc, argv);

  // Explain what is going to happen
  if (options->debug_)
    ACE_DEBUG ((LM_DEBUG,
                ACE_TEXT ("hostname = %C, port = %d, filename = %s, ")
                ACE_TEXT ("length = %d, offset = %d, operation = %c\n"),
                options->hostname_,
                options->port_,
                options->filename_,
                options->length_,
                options->offset_,
                options->operation_));

  // Create a blob
  ACE_Blob blob;

  // User requested a read
  if (options->operation_ == 'r')
    {
      ACE_Message_Block mb (0, options->length_);

      // Open the blob
      if (blob.open (options->filename_,
                     options->hostname_,
                     options->port_) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("open error")),
                          -1);

      // Read from it
      if (blob.read (&mb, options->length_, options->offset_) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("read error")),
                          -1);

      // Write to STDOUT
      if (ACE_OS::write (ACE_STDOUT, mb.rd_ptr(), mb.length()) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("write error")),
                          -1);
    }
  else
    {
      int total = options->length_ + options->offset_;
      ACE_Message_Block mb (total);

      // Open the file to be sent
      ACE_HANDLE h = ACE_OS::open (options->filename_, O_RDONLY);
      if (h == ACE_INVALID_HANDLE)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("file open error")),
                          -1);

      // Open the blob
      if (blob.open (options->filename_, options->hostname_, options->port_) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("connection open error")),
                          -1);

      // Read from the file
      if (ACE_OS::read (h, mb.wr_ptr (), total) != total)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("file read error")),
                          -1);

      // Close the file
      ACE_OS::close (h);

      // Adjust the offset
      mb.wr_ptr (mb.size ());

      // Write to the blob
      if (blob.write (&mb, options->length_, options->offset_) == -1)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("%p\n"),
                           ACE_TEXT ("network write error")),
                          -1);
    }

  blob.close ();
  return 0;
}
