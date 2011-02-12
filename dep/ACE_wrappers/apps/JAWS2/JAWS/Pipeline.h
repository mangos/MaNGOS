/* -*- c++ -*- */
// $Id: Pipeline.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef JAWS_PIPELINE_H
#define JAWS_PIPELINE_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Service_Config.h"
#include "ace/Stream.h"
#include "ace/Module.h"
#include "ace/Task.h"

#include "JAWS/Export.h"

typedef ACE_Stream<ACE_NULL_SYNCH> JAWS_Pipeline_Stream;
typedef ACE_Module<ACE_NULL_SYNCH> JAWS_Pipeline_Module;
typedef ACE_Task<ACE_NULL_SYNCH> JAWS_Pipeline_Task;

class JAWS_IO_Handler;
class JAWS_Dispatch_Policy;

class JAWS_Export JAWS_Pipeline : public JAWS_Pipeline_Task
  // = TITLE
  //   Methods that are common to pipeline components
{
public:
  JAWS_Pipeline (void);
  // ACE_Task hooks

  virtual int open (void * = 0);
  virtual int close (u_long = 0);
};

#endif /* !defined (JAWS_PIPELINE_H) */
