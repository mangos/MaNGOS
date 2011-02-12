/* -*- c++ -*- */
// $Id: SS_Data.h 91730 2010-09-13 09:31:11Z johnnyw $

#ifndef TERA_SS_DATA_H
#define TERA_SS_DATA_H

#include "ace/FILE_IO.h"
#include "ace/SOCK_Stream.h"
#include "ace/Message_Block.h"

class TeraSS_Service_Handler;

class TeraSS_Data
{
public:

  TeraSS_Data (TeraSS_Service_Handler *sh);

  ACE_SOCK_Stream & peer (void);
  ACE_Message_Block & mb (void);
  ACE_FILE_IO & file_io (void);

private:

  ACE_Message_Block mb_;
  TeraSS_Service_Handler *sh_;
  ACE_FILE_IO file_io_;

};

#endif /* TERA_SS_DATA_H */
