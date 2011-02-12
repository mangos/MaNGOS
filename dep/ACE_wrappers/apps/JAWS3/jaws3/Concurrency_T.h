/* -*- c++ -*- */
// $Id: Concurrency_T.h 91730 2010-09-13 09:31:11Z johnnyw $

#ifndef JAWS_CONCURRENCY_T_H
#define JAWS_CONCURRENCY_T_H

#include "ace/config-lite.h"

class JAWS_Protocol_Handler;

template <class CONCURRENCY_IMPL>
class JAWS_Concurrency_Bridge
// = TITLE
//     The Concurrency Bridge uses the Concrete Bridge pattern.
//
// = DESCRIPTION
//     The Concurrency Bridge class plays the role of Abstraction
//     in the Bridge pattern.  If the CONCURRENCY_IMPL is the
//     base class JAWS_Concurrency_Impl, then it plays the classic
//     role.  If the CONCURRENCY_IMPL is one of the Concrete
//     Implementors, then its role is more that of an Adapter,
//     which is like Bridge, but without the polymorphism overhead.
//
{
public:

  JAWS_Concurrency_Bridge (CONCURRENCY_IMPL *impl = 0);

  int putq (JAWS_Protocol_Handler *ph);
  int getq (JAWS_Protocol_Handler *&ph);

  void shutdown (void);

protected:

  CONCURRENCY_IMPL *impl_;

};

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "jaws3/Concurrency_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("Concurrency_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */


#endif /* JAWS_CONCURRENCY_T_H */
