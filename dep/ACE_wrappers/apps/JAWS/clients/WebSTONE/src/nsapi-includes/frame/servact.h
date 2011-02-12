/*
 * $Id: servact.h 80826 2008-03-04 14:51:23Z wotte $
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
 * servact.h: Define the server's actions, depending on what type of
 * server it is.
 *
 * Rob McCool
 */


#if defined(MCC_HTTPD) || defined(MCC_PROXY) || defined(MCC_NEWS)
#include "httpact.h"
#endif
