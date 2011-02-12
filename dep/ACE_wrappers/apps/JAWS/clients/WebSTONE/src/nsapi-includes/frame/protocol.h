/*
 * $Id: protocol.h 80826 2008-03-04 14:51:23Z wotte $
 *
 * Copyright (c) 1994, 1995.  Netscape Communications Corporation.  All
 * rights reserved.
 *
 * Use of this software is governed by the terms of the license agreement for
 * the Netscape Communications or Netscape Comemrce Server between the
 * parties.
 */


/* ------------------------------------------------------------------------ */


/*
 * protocol.h: Switch which protocol we're speaking.
 *
 * Currently only HTTP, but expected to include MTP in the future.
 *
 * Rob McCool
 */


#ifdef MCC_HTTPD
#include "http.h"
#endif
