/* -*- c++ -*- */
// $Id: Data_Block.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_DATA_BLOCK_H
#define JAWS_DATA_BLOCK_H

#include "ace/Message_Block.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "JAWS/Export.h"
#include "JAWS/Pipeline.h"

class JAWS_IO_Handler;
class JAWS_Dispatch_Policy;
class JAWS_Data_Block;
class JAWS_Pipeline_Handler;

class JAWS_Export JAWS_Data_Block : public ACE_Message_Block
// = TITLE
//   Defines the communication unit between pipeline components
{
public:
  JAWS_Data_Block (void);
  JAWS_Data_Block (JAWS_Data_Block &db);
  ~JAWS_Data_Block (void);

  JAWS_Pipeline_Handler *task (void);
  JAWS_IO_Handler *io_handler (void);
  JAWS_Dispatch_Policy *policy (void);
  void *payload (void);

  void task (JAWS_Pipeline_Handler *taskp);
  void io_handler (JAWS_IO_Handler *handlerp);
  void policy (JAWS_Dispatch_Policy *policyp);
  void payload (void *payloadp);

  enum { JAWS_DATA_BLOCK_SIZE = 8192 };

private:
  JAWS_IO_Handler *io_handler_;
  JAWS_Dispatch_Policy *policy_;
  JAWS_Pipeline_Handler *task_;

  void *payload_;
};

#endif /* !defined (JAWS_DATA_BLOCK_H) */
