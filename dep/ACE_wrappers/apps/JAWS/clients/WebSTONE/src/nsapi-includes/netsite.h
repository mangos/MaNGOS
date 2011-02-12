/*
 * $Id: netsite.h 80826 2008-03-04 14:51:23Z wotte $
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
 * Standard defs for NetSite servers.
 */


#ifndef NETSITE_H
#define NETSITE_H

#ifdef MCC_PROXY
#define MAGNUS_VERSION "1.1m"
#define MAGNUS_VERSION_STRING "Netscape-Proxy/1.1m"

#elif defined(MCC_ADMSERV)
#define MAGNUS_VERSION "1.1"
#define MAGNUS_VERSION_STRING "Netscape-Administrator/1.1"

#elif defined(MCC_HTTPD)
#define MAGNUS_VERSION "1.1"
#ifdef NET_SSL
#define MAGNUS_VERSION_STRING "Netscape-Commerce/1.12"
#else
#define MAGNUS_VERSION_STRING "Netscape-Communications/1.12"
#endif

#elif defined(MCC_NEWS)
#define MAGNUS_VERSION_STRING "Netscape 1.1"
#endif

/* Used in some places as a length limit on error messages */
#define MAGNUS_ERROR_LEN 8192

/* Carraige return and line feed */
#define CR 13
#define LF 10

/* -------------------------- Memory allocation --------------------------- */


/* Later change these to have catastrophic error handling */

#include <stdlib.h>

#define MALLOC(sz) malloc(sz)
#define FREE(ptr) free((void *)ptr)
#define STRDUP(str) strdup(str)
#define REALLOC(ptr,sz) realloc(ptr,sz)


/* Not sure where to put this. */
void magnus_atrestart(void (*fn)(void *), void *data);

#endif
