// $Id: HTTP_Policy.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "HTTP_Policy.h"



HTTP_Policy::HTTP_Policy (JAWS_Concurrency_Base *concurrency)
  : concurrency_ (concurrency)
{
}

JAWS_Concurrency_Base *
HTTP_Policy::update (void *)
{
  /* for now, we always return the same concurrency strategy */
  return this->concurrency_;
}
