/*
 * $Id: conf.h 80826 2008-03-04 14:51:23Z wotte $
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
 * conf.h: Deals with the server configuration file.
 *
 * Object configuration is an entirely different matter. This deals with
 * things like what port the server runs on, how many children it spawns,
 * and other server-related issues. Information related configuration is
 * handled by the object conf.
 *
 * Rob McCool
 */


#ifndef CONF_H
#define CONF_H

#ifdef XP_WIN32
#include <nt/ntconf.h>
#else

#include "netsite.h"
#include "frame/objset.h"
#include "base/daemon.h"  /* daemon struct */

#include <pwd.h>      /* struct passwd */

/* ------------------------------ Constants ------------------------------- */


/* The longest line in the configuration file */
#define CONF_MAXLEN 16384

#define LF 10
#define CR 13


#if defined(DAEMON_ANY)
#ifdef MCC_PROXY
#define DEFAULT_PORT 8080
#else
#define DEFAULT_PORT 80
#endif
#endif

#if defined(DAEMON_UNIX_POOL) || defined(DAEMON_UNIX_MOBRULE)
#define DEFAULT_POOL_MAX 50
#endif



/* ------------------------------- Globals -------------------------------- */


#ifndef CONF_C

/*
 * These server parameters are made global because they really don't
 * belong anywhere else.
 */

#if defined(DAEMON_ANY)

#include <sys/types.h>

/* What port we listen to */
extern int port;

/* What address to bind to */
extern char *addr;

/* User to run as */
extern struct passwd *userpw;

/* Directory to chroot to */
extern char *chr;

/* Where to log our pid to */
extern char *pidfn;

#if defined(DAEMON_UNIX_POOL) || defined(DAEMON_UNIX_MOBRULE)
/* The maximum number of processes to keep in the pool */
extern int pool_max;
/* The minimum number of processes to keep in the pool */
extern int pool_min;
/* The maximum number of requests each process should handle. -1=default */
extern int pool_life;
#endif

#ifdef NET_SSL
extern char *secure_keyfn;
extern char *secure_certfn;
extern char *secure_dongle;
extern int security_active;
extern int secure_auth;
extern int security_session_timeout;
#endif

#endif

/* The server's hostname as should be reported in self-ref URLs */
extern char *server_hostname;

/* The main object from which all are derived */
extern char *root_object;

/* The object set the administrator has asked us to load */
extern httpd_objset *std_os;

/* The main error log, where all errors are logged */
extern char *master_error_log;

/* The e-mail address of someone to mail upon catastrophic error */
extern char *admin_email;

#endif


/* ------------------------------ Prototypes ------------------------------ */


/*
 * conf_init reads the given configuration file and sets any non-default
 * parameters to their given setting.
 */

char *conf_init(char *cfn);

/*
 * conf_terminate frees any data the conf routines may be holding.
 */

void conf_terminate(void);


/*
 * conf_vars2daemon transfers these globals to a daemon structure
 */
void conf_vars2daemon(daemon_s *d);

#endif
#endif
