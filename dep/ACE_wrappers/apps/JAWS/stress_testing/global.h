// $Id: global.h 80826 2008-03-04 14:51:23Z wotte $

#include "ace/ACE.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Connector.h"
#include "ace/Pipe.h"
#include "ace/SOCK_Stream.h"
#include "ace/INET_Addr.h"
#include "ace/Profile_Timer.h"
#include "ace/Thread.h"
#include "ace/Thread_Manager.h"
#include "ace/Service_Config.h"

// FUZZ: disable check_for_math_include
#include <math.h>











