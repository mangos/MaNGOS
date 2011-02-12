/* -*- c++ -*- */
// $Id: HTTP_Data.h 91730 2010-09-13 09:31:11Z johnnyw $

#ifndef JAWS_HTTP_DATA_H
#define JAWS_HTTP_DATA_H

#include "ace/FILE_IO.h"
#include "ace/SOCK_Stream.h"
#include "ace/Message_Block.h"

class JAWS_HTTP_Service_Handler;

class JAWS_HTTP_Data
{
public:

  JAWS_HTTP_Data (JAWS_HTTP_Service_Handler *sh);

  ACE_SOCK_Stream & peer (void);
  ACE_Message_Block & mb (void);
  ACE_FILE_IO & file_io (void);

private:

  JAWS_HTTP_Service_Handler *sh_;
  ACE_Message_Block mb_;
  ACE_FILE_IO file_io_;

};

#endif /* JAWS_HTTP_DATA_H */
