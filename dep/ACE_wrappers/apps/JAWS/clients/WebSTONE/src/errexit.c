/* $Id: errexit.c 91813 2010-09-17 07:52:52Z johnnyw $ */
/**************************************************************************
 *
 *     Copyright (C) 1995 Silicon Graphics, Inc.
 *
 *  These coded instructions, statements, and computer programs were
 *  developed by SGI for public use.  If any changes are made to this code
 *  please try to get the changes back to the author.  Feel free to make
 *  modifications and changes to the code and release it.
 *
 **************************************************************************/

/* errexit call for general error handling */

#include <stdio.h>
#ifndef WIN32
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#endif /* WIN32 */
#include <stdarg.h>
#include <sys/types.h>

#include "sysdep.h"
#include "bench.h"

#ifdef HAVE_VPRINTF
#define VPRINTF(stderr, format, args) vfprintf((stderr), (format), (args))
#else
#ifdef HAVE_DOPRNT
#define VPRINTF(stderr, format, args) _doprnt((format), (args), (stderr))
#endif /* HAVE_DOPRNT */
#endif /* HAVE_VPRINTF */

/* print an error message and exit 1 */
void
errexit(const char *format, ...)
{
va_list args;
char hostname[64] = "";
pid_t PID;

    PID = getpid();
    gethostname(hostname, sizeof(hostname));
    fprintf(stderr, "%s PID %d: ", hostname, PID);

    va_start(args, format);
    VPRINTF(stderr, format, args);
    debug && VPRINTF(debugfile, format, args);
    va_end(args);
    fflush(stderr);
    exit(1);
}
/* that's it */

/* print an error message and return -1 */
int
returnerr(const char *format, ...)
{
va_list args;
char hostname[64] = "";
pid_t PID;

    PID = getpid();
    gethostname(hostname, sizeof(hostname));
    fprintf(stderr, "%s PID %d: ", hostname, PID);

    va_start(args, format);
    VPRINTF(stderr, format, args);
    debug && VPRINTF(debugfile, format, args);
    va_end(args);
    fflush(stderr);
    debug && fflush(debugfile);
    return(-1);
}
/* that's it */

/* print a debug message and then flush */
int
d_printf(const char *format, ...)
{
va_list args;

    va_start(args, format);
    VPRINTF(debugfile, format, args);
    va_end(args);

    fflush(debugfile);
    return 0;
}
/* that's it */

/* returns the last network error as a string */
char *neterrstr(void) {
static char buf[200];

#ifdef WIN32
    sprintf(buf, "WSAGetLastError() = %d", WSAGetLastError());
    WSASetLastError(0);
#else
    sprintf(buf, "errno = %d: %s", errno, strerror(errno));
    errno = 0;
#endif /* WIN32 */

    return buf;
}
