/*
 * $Id: log.h 80826 2008-03-04 14:51:23Z wotte $
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
 * log.h: Records transactions, reports errors to administrators, etc.
 *
 * Rob McCool
 */


#ifndef LOG_H
#define LOG_H


#include "netsite.h"
#include "base/session.h"   /* Session structure */
#include "frame/req.h"      /* Request struct */
#include "base/ereport.h"   /* Error reporting, degrees */


#define ERROR_CUTOFF 128


/* ------------------------------ Prototypes ------------------------------ */


/*
 * log_error logs an error of the given degree from the function func
 * and formats the arguments with the printf() style fmt. Returns whether the
 * log was successful. Records the current date.
 *
 * sn and rq are optional parameters. If given, information about the client
 * will be reported.
 */

int log_error(int degree, char *func, Session *sn, Request *rq,
              char *fmt, ...);

#endif
