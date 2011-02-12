#ifndef __SYSDEP_H__
#define __SYSDEP_H__
/* $Id: sysdep.h 80826 2008-03-04 14:51:23Z wotte $ */
/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1995 Silicon Graphics, Inc.                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs were      *
 *  developed by SGI for public use.  If any changes are made to this code*
 *  please try to get the changes back to the author.  Feel free to make  *
 *  modifications and changes to the code and release it.                 *
 *                                                                        *
 **************************************************************************/

/* include config.h, output from autoconf */
#ifdef HAVE_CONFIG_H
#ifndef __CONFIG_H__
#define __CONFIG_H__
#include "config.h"
#endif
#endif

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#endif /* WIN32 */

/* MAXHOSTNAMELEN is undefined on some systems */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

//FUZZ: disable check_for_NULL
/* SunOS doesn't define NULL */
#ifndef NULL
#define NULL 0
#endif
//FUZZ: enable check_for_NULL

/* encapsulation of minor UNIX/WIN NT differences */
#ifdef WIN32
#define NETREAD(sock, buf, len)         recv(sock, buf, len, 0)
#define NETWRITE(sock, buf, len)        send(sock, buf, len, 0)
#define NETCLOSE(sock)                  closesocket(sock)
#define BADSOCKET(sock)                 ((sock) == INVALID_SOCKET)
#define BADSOCKET_ERRNO(sock)           BADSOCKET(sock)
#define BADSOCKET_VALUE                 INVALID_SOCKET
#define S_ADDR                          S_un.S_addr

#ifdef USE_TIMEZONE
#error  NT gettimeofday() doesn't support USE_TIMEZONE (yet)
#else
#define GETTIMEOFDAY(timeval, tz)       gettimeofday(timeval)
#endif /* USE_TIMEZONE */

typedef unsigned short                  NETPORT;
#define SRANDOM                         srand
#define RANDOM_R                        rand_r
#define RANDOM                          rand
#define THREAD                          __declspec ( thread )
#define PROGPATH                        "c:\\webstone\\webclient"
#define FILENAME_SIZE                   256
#define HAVE_VPRINTF                    1

#define SIGCHLD                         0       /* dummy value */
#define SIGALRM                         0       /* dummy value */
typedef int                             pid_t;
typedef unsigned short                  ushort;
#define MAXPATHLEN                      512



#else /* not WIN32 */
#define NETREAD(sock, buf, len)         read(sock, buf, len)
#define NETWRITE(sock, buf, len)        write(sock, buf, len)
#define NETCLOSE(sock)                  close(sock)
#define BADSOCKET(sock)                 ((sock) < 0)
#define BADSOCKET_ERRNO(sock)           (BADSOCKET(sock) || errno)
#define BADSOCKET_VALUE                 (-1)
#define S_ADDR                          s_addr

#ifdef USE_TIMEZONE
#define GETTIMEOFDAY(timeval,tz)        gettimeofday(timeval, 0)
#else
#define GETTIMEOFDAY(timeval,tz)        gettimeofday(timeval, tz)
#endif /* USE_TIMEZONE */

typedef unsigned short                  NETPORT;
#define SRANDOM                         srandom
#define RANDOM                          random
#define THREAD
#define PROGPATH                        "/tmp/webclient" /* "/usr/local/bin/webclient" */
#define FILENAME_SIZE                   1024
#define HAVE_VPRINTF                    1

typedef int                             SOCKET;
#define min(a,b)                        (((a) < (b)) ? a : b)
#define max(a,b)                        (((a) > (b)) ? a : b)
#endif /* WIN32 */


/* function prototypes */

#ifdef WIN32
int     getopt(int argc, char ** argv, char *opts);
int     getpid(void);
int     gettimeofday(struct timeval *curTimeP);
int     random_number(int max);
SOCKET  rexec(const char **hostname, NETPORT port, char *username, char *password,
                char *command, SOCKET *sockerr);
void    sleep(int secs);

#else
#ifdef NO_REXEC
extern int      rexec(char **, int, char *, char *, char *, int *);
#endif
#endif /* WIN32 */


#ifndef HAVE_STRERROR
/* strerror() is not available on SunOS 4.x and others */
char *strerror(int errnum);

#endif
/* strerror() */


#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif

/* !__SYSDEP_H__ */
#endif
