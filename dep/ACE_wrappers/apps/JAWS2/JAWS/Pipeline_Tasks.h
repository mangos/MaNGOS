/* -*- c++ -*- */
// $Id: Pipeline_Tasks.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_PIPELINE_TASKS_H
#define JAWS_PIPELINE_TASKS_H

#include "ace/Singleton.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "JAWS/Export.h"
#include "JAWS/Pipeline.h"
#include "JAWS/Pipeline_Handler_T.h"
#include "JAWS/Data_Block.h"

class JAWS_Export JAWS_Pipeline_Handler
  : public JAWS_Pipeline_Abstract_Handler<JAWS_Data_Block>
{
public:
  JAWS_Pipeline_Handler (void);
  virtual ~JAWS_Pipeline_Handler (void);
  virtual int put (ACE_Message_Block *mb, ACE_Time_Value *tv = 0);
  virtual int handle_put (JAWS_Data_Block *data, ACE_Time_Value *tv) = 0;

  virtual JAWS_Dispatch_Policy * policy (void);
  virtual void policy (JAWS_Dispatch_Policy *);

private:
  JAWS_Dispatch_Policy *policy_;
};

class JAWS_Pipeline_Accept_Task : public JAWS_Pipeline_Handler
{
public:
  virtual int put (ACE_Message_Block *mb, ACE_Time_Value *tv = 0);
  virtual int handle_put (JAWS_Data_Block *data, ACE_Time_Value *tv);

  virtual JAWS_IO_Handler * new_handler (JAWS_Data_Block *data);
};

class JAWS_Pipeline_Done_Task : public JAWS_Pipeline_Handler
{
public:
  virtual int put (ACE_Message_Block *mb, ACE_Time_Value *tv = 0);
  virtual int handle_put (JAWS_Data_Block *data, ACE_Time_Value *tv);
};

typedef ACE_Singleton<JAWS_Pipeline_Accept_Task, ACE_SYNCH_MUTEX>
        JAWS_Pipeline_Accept_Task_Singleton;

typedef ACE_Singleton<JAWS_Pipeline_Done_Task, ACE_SYNCH_NULL_MUTEX>
        JAWS_Pipeline_Done_Task_Singleton;

#endif /* !defined (JAWS_PIPELINE_TASKS_H) */
