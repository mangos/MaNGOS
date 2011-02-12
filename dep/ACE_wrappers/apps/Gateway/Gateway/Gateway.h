// -*- C++ -*-
//
// $Id: Gateway.h 80826 2008-03-04 14:51:23Z wotte $

// ============================================================================
//
// = LIBRARY
//    gateway
//
// = FILENAME
//    Gateway.h
//
// = DESCRIPTION
//    Since the Gateway is an <ACE_Service_Object>, this file defines
//    the entry point into the Service Configurator framework.
//
// = AUTHOR
//    Doug Schmidt
//
// ============================================================================

#ifndef ACE_GATEWAY
#define ACE_GATEWAY

#include "ace/svc_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

ACE_SVC_FACTORY_DECLARE (Gateway)

#endif /* ACE_GATEWAY */
