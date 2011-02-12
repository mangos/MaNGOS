// $Id: Data_Block.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "JAWS/Data_Block.h"
#include "JAWS/Policy.h"



JAWS_Data_Block::JAWS_Data_Block (void)
  : ACE_Message_Block (JAWS_DATA_BLOCK_SIZE),
    io_handler_ (0),
    policy_ (0),
    task_ (0),
    payload_ (0)
{
}

JAWS_Data_Block::JAWS_Data_Block (JAWS_Data_Block &db)
  : ACE_Message_Block (JAWS_DATA_BLOCK_SIZE),
    io_handler_ (db.io_handler_),
    policy_ (db.policy_),
    task_ (db.task_),
    payload_ (db.payload_)
{
}

JAWS_Data_Block::~JAWS_Data_Block (void)
{
}

JAWS_Pipeline_Handler *
JAWS_Data_Block::task (void)
{
  return this->task_;
}

JAWS_IO_Handler *
JAWS_Data_Block::io_handler (void)
{
  return this->io_handler_;
}

JAWS_Dispatch_Policy *
JAWS_Data_Block::policy (void)
{
  return this->policy_;
}

void *
JAWS_Data_Block::payload (void)
{
  return this->payload_;
}

void
JAWS_Data_Block::task (JAWS_Pipeline_Handler *taskp)
{
  this->task_ = taskp;
}

void
JAWS_Data_Block::io_handler (JAWS_IO_Handler *handlerp)
{
  this->io_handler_ = handlerp;
}

void
JAWS_Data_Block::policy (JAWS_Dispatch_Policy *policyp)
{
  this->policy_ = policyp;
}

void
JAWS_Data_Block::payload (void *payloadp)
{
  this->payload_ = payloadp;
}
