/*
 * $Id: daemon.h 80826 2008-03-04 14:51:23Z wotte $
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
 * daemon.h: Things related to the accepting connections
 *
 * Rob McCool
 */


#ifndef DAEMON_H
#define DAEMON_H

#ifdef XP_WIN32
#include <nt/ntdaemon.h>
#else

#include "net.h"
#include "session.h"

#include <pwd.h>    /* struct passwd */


/* ------------------------------- Defines -------------------------------- */


#define child_exit exit


/* Codes for child_status */
#define CHILD_EMPTY_SLOT 0xfe
#define CHILD_AWAIT_CONNECT 0xff
#define CHILD_PROCESSING 0x00
#define CHILD_READING 0x01
#define CHILD_WRITING 0x02
#define CHILD_RESOLVING 0x03


typedef struct {
    char *ipstr;
    int port;
    struct passwd *pw;
    char *chr;
    char *pidfn;
    void (*rcback)(int);
#if defined(DAEMON_UNIX_POOL) || defined(DAEMON_UNIX_MOBRULE)
    int maxprocs, minprocs, proclife;
#endif
#ifdef NET_SSL
    char *secure_keyfn;
    char *secure_certfn;
    char *secure_dongle;
    int secure_auth;
    int secure_session_timeout;
    int security;
#endif
} daemon_s;


/* ------------------------------ Prototypes ------------------------------ */

#ifdef MCC_PROXY
/* A unique serial number assigned to each child. */
extern int child_serial;
#endif

/*
 * daemon_run accepts whether or not it should detach from its parent process,
 * and a daemon structure as its arguments. The daemon structure contains
 * a port number, a root directory to chroot to (can be NULL), a filename to
 * log the daemon pid to (can be NULL). daemon_run never returns.
 *
 * child_callback is a function which will be called every time a new
 * connection is recieved. Session is a new session ID.
 *
 * rcback is a function which is a restart function: When SIGHUP is received,
 * this function will be called. You may give SIG_DFL if you don't want to
 * support restarting. The rcback will be passed SIGHUP.
 *
 * pw is the passwd entry to run the daemon as. If the effective user id is
 * root, daemon_run will try to set its uid and gid to the user pointed
 * to by this structure. You may pass NULL.
 */

void daemon_run(int det, void (*child_callback)(Session *), daemon_s *d);

/*
 * fork is a wrapper for the system's fork function. This closes the listen
 * socket for the mob. This also makes sure that a threaded daemon only gets
 * the calling thread and not all of them.
 */

pid_t child_fork(void);


/*
 * Set status to the given code for statistics reporting
 */

#ifdef DAEMON_STATS
void child_status(int code);
#else
#define child_status(code) (void)(code)
#endif


#endif
#endif
