/* -*- C++ -*- */

//=============================================================================
/**
 *  @file    Base_Optimizer.h
 *
 *  $Id: Base_Optimizer.h 80826 2008-03-04 14:51:23Z wotte $
 *
 *  @author Per Andersson.
 */
//=============================================================================


#ifndef BASE_OPTIMIZER_H
#define BASE_OPTIMIZER_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

/**
 * @class Base_Optimizer
 *
 *
 * Thanks to Nathan Myers and Fergus Henderson for this little
 * beauty.
 */
template<class Base, class Member>
class Base_Optimizer : public Base
{

public:
  Base_Optimizer (void);
  Base_Optimizer (const Base &base);
  Base_Optimizer (const Base &base,
                  const Member &member);

  Member m_;
};

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "Base_Optimizer.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("Base_Optimizer.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#endif /* BASE_OPTIMIZER_H */
