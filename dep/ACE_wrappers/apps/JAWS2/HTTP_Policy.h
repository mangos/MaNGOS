/* -*- c++ -*- */
// $Id: HTTP_Policy.h 80826 2008-03-04 14:51:23Z wotte $

#ifndef HTTP_POLICY_H
#define HTTP_POLICY_H

#include "JAWS/Concurrency.h"
#include "JAWS/Policy.h"

/* create a policy */
class HTTP_Policy : public JAWS_Dispatch_Policy
{
public:
  HTTP_Policy (JAWS_Concurrency_Base *concurrency);
  virtual JAWS_Concurrency_Base * update (void *state = 0);

private:
  JAWS_Concurrency_Base *concurrency_;
};


#endif /* !defined (HTTP_POLICY_H) */
