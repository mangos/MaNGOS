/* $Id: sysdep.c 81994 2008-06-16 21:23:17Z sowayaa $ */
/**************************************************************************
 *
 *  Copyright (C) 1995 Silicon Graphics, Inc.
 *
 *  These coded instructions, statements, and computer programs were
 *  developed by SGI for public use.  If any changes are made to this code
 *  please try to get the changes back to the author.  Feel free to make
 *  modifications and changes to the code and release it.
 *
 **************************************************************************/

#include "sysdep.h"
/* strerror() */
#ifndef HAVE_STRERROR
/* strerror is not available on SunOS 4.1.3 and others */
extern int sys_nerr;
extern char *sys_errlist[];
extern int errno;

char *strerror(int errnum)
{

    if (errnum<sys_nerr)
    {
        return(sys_errlist[errnum]);
    }

    return(0);
}

#endif /* strerror() */


/* stub routines for NT */

#ifdef WIN32
#include <winsock.h>
#include <process.h>

int getpid(void) {

    return GetCurrentThreadId();
}

void sleep(int sec) {

    Sleep(sec*1000);
}
#endif /* WIN32 */

