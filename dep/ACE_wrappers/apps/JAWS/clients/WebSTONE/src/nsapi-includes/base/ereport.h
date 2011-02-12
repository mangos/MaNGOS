/*
 * $Id: ereport.h 80826 2008-03-04 14:51:23Z wotte $
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
 * ereport.h: Records transactions, reports errors to administrators, etc.
 *
 * Rob McCool
 */


#ifndef EREPORT_H
#define EREPORT_H


#include "../base/session.h"   /* Session structure */
#ifdef XP_UNIX
#include <pwd.h>      /* struct passwd */
#endif /* XP_UNIX */


/* ------------------------------ Constants ------------------------------- */


/*
 * The maximum length of an error message. NOT RUN-TIME CHECKED
 */

#define MAX_ERROR_LEN 8192

/* A warning is a minor mishap, such as a 404 being issued. */
#define LOG_WARN 0

/*
 * A misconfig is when there is a syntax error or permission violation in
 * a config. file.
 */
#define LOG_MISCONFIG 1

/*
 * Security warnings are issued when authentication fails, or a host is
 * given a 403 return code.
 */
#define LOG_SECURITY 2

/*
 * A failure is when a request could not be fulfilled due to an internal
 * problem, such as a CGI script exiting prematurely, or a filesystem
 * permissions problem.
 */
#define LOG_FAILURE 3

/*
 * A catastrophe is a fatal server error such as running out of
 * memory or processes, or a system call failing, or even a server crash.
 * The server child cannot recover from a catastrophe.
 */
#define LOG_CATASTROPHE 4

/*
 * Informational message, of no concern.
 */
#define LOG_INFORM 5

/*
 * The time format to use in the error log
 */

#define ERR_TIMEFMT "[%d/%b/%Y:%H:%M:%S]"


/* The fd you will get if you are reporting errors to SYSLOG */

#define ERRORS_TO_SYSLOG -1



/* ------------------------------ Prototypes ------------------------------ */


/*
 * ereport logs an error of the given degree and formats the arguments with
 * the printf() style fmt. Returns whether the log was successful. Records
 * the current date.
 */

int ereport(int degree, char *fmt, ...);

/*
 * ereport_init initializes the error logging subsystem and opens the static
 * file descriptors. It returns NULL upon success and an error string upon
 * error. If a userpw is given, the logs will be chowned to that user.
 *
 * email is the address of a person to mail upon catastrophic error. It
 * can be NULL if no e-mail is desired. ereport_init will not duplicate
 * its own copy of this string; you must make sure it stays around and free
 * it when you shut down the server.
 */

char *ereport_init(char *err_fn, char *email, struct passwd *pw);

/*
 * log_terminate closes the error and common log file descriptors.
 */
void ereport_terminate(void);

/* For restarts */
SYS_FILE ereport_getfd(void);

#endif
