/*
 * $Id: httpact.h 80826 2008-03-04 14:51:23Z wotte $
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
 * httpact.h: Defines the API characteristics for HTTP servers
 *
 * Rob McCool
 */


#ifndef HTTPACT_H
#define HTTPACT_H

#include "frame/req.h"
#include "frame/object.h"


/* ------------------------------ Constants ------------------------------- */

#ifdef MCC_BATMAN
#define NUM_DIRECTIVES 5
#else
#ifdef MCC_PROXY
#define NUM_DIRECTIVES 10
#else
#define NUM_DIRECTIVES 7
#endif
#endif


/* -------------------------- Generic Prototypes -------------------------- */


/*
 * directive_name2num will return the position of the abbreviated directive
 * dir in the directive table.
 *
 * If dir does not exist in the table, it will return -1.
 */

int directive_name2num(char *dir);


/*
 * directive_num2name returns a string describing directive number num.
 */
const char *directive_num2name(int num);


/*
 * servact_finderror looks through the request's object set to find a
 * suitable error function to execute. Returns REQ_PROCEED if a function
 * was found and executed successfully, REQ_NOACTION otherwise.
 */
int servact_finderror(Session *sn, Request *rq);


/*
 * Read in and handle one request from the given session
 */
void servact_handle(Session *sn);


/* ----------------------- HTTP specific prototypes ----------------------- */


int servact_handle_processed(Session *sn, Request *rq);

/*
 * Returns the translated path (filename) for the given uri, NULL otherwise.
 * If authentication is required for the given uri, nothing is returned even
 * if the current user has authenticated to that area.
 */
char *servact_translate_uri(char *uri, Session *sn);



#ifdef MCC_PROXY

/* ----------------------- proxy specific prototypes ---------------------- */

/*
 * Resolves the given hostname, first trying to find a resolver
 * function from obj.conf, and if that fails, just calls gethostbyname().
 *
 */
struct hostent *servact_gethostbyname(char *host, Session *sn, Request *rq);


/*
 * Establishes a connection to the specified host and port using
 * a Connect class function from obj.conf.  Returns the sockect
 * descriptor that is connected (and which should be SSL_Import()'ed
 * by the caller).
 *
 * Returns -2 (REQ_NOACTION), if no such Connect class function exists.
 * The caller should use the native connect mechanism in that case.
 *
 * Returns -1 (REQ_ABORT) on failure to connect.  The caller should not
 * attempt to use the native connect.
 *
 */
int servact_connect(char *host, int port, Session *sn, Request *rq);


#endif  /* ! MCC_PROXY */

#endif
