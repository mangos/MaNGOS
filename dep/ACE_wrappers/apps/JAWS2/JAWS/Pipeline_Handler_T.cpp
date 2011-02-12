// $Id: Pipeline_Handler_T.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#ifndef JAWS_PIPELINE_HANDLER_T_CPP
#define JAWS_PIPELINE_HANDLER_T_CPP

#include "JAWS/Pipeline_Handler_T.h"



template <class TYPE>
JAWS_Pipeline_Abstract_Handler<TYPE>::JAWS_Pipeline_Abstract_Handler (void)
{
}

template <class TYPE>
JAWS_Pipeline_Abstract_Handler<TYPE>::~JAWS_Pipeline_Abstract_Handler (void)
{
}

template <class TYPE> int
JAWS_Pipeline_Abstract_Handler<TYPE>::put (ACE_Message_Block *mb,
                                           ACE_Time_Value *tv)
{
  TYPE *data = reinterpret_cast <TYPE *> (mb->data_block ());

  int status = this->handle_put (data, tv);

  return status;
}

#endif /* !defined (JAWS_PIPELINE_HANDLER_T_CPP) */
