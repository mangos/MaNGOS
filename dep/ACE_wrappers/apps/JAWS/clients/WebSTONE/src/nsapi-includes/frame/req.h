/*
 * $Id: req.h 80826 2008-03-04 14:51:23Z wotte $
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
 * req.h: Request-specific data structures and functions
 *
 * Rob McCool
 */


#ifndef REQ_H
#define REQ_H


#include "netsite.h"
#include "base/pblock.h"
#include "base/session.h"
#include "frame/objset.h"

#include <sys/stat.h>



/* ------------------------------ Constants ------------------------------- */


#define REQ_HASHSIZE 10
#define REQ_MAX_LINE 4096


/*
 * The REQ_ return codes. These codes are used to determine what the server
 * should do after a particular module completes its task.
 *
 * Func type functions return these as do many internal functions.
 */

/* The function performed its task, proceed with the request */
#define REQ_PROCEED 0
/* The entire request should be aborted: An error occurred */
#define REQ_ABORTED -1
/* The function performed no task, but proceed anyway. */
#define REQ_NOACTION -2
/* Tear down the session and exit */
#define REQ_EXIT -3



/* ------------------------------ Structures ------------------------------ */


typedef struct {
    /* Server working variables */
    pblock *vars;

    /* The method, URI, and protocol revision of this request */
    pblock *reqpb;
    /* Protocol specific headers */
    int loadhdrs;
    pblock *headers;

    /* Server's response headers */
    pblock *srvhdrs;

    /* The object set constructed to fulfill this request */
    httpd_objset *os;
    /* Array of objects that were created from .nsconfig files */
    httpd_objset *tmpos;

    /* The stat last returned by request_stat_path */
    char *statpath;
    char *staterr;
    struct stat *finfo;

#ifdef MCC_PROXY
    /* SOCKS request data */
    void *socks_rq;
#endif

} Request;


/* ------------------------------ Prototypes ------------------------------ */


/*
 * request_create creates a new request structure.
 */

Request *request_create(void);

/*
 * request_free destroys a request structure.
 */

void request_free(Request *req);


/*
 * Restarts a request for a given URI internally. If rq is non-NULL, the
 * function will keep the old request's headers and protocol, but with a new
 * URI and method of GET. If the previous method was HEAD, this is preserved.
 * Any other method becomes GET. You may assume that if you give it a request
 * structure that it will use the same structure.
 *
 * Once you have this new Request, you must then do what you want with
 * it (e.g. send the object back, perform uri2path translation, etc.)
 */

Request *request_restart_internal(char *uri, Request *rq);


/*
 * request_translate_uri performs virtual to physical mapping on the given
 * uri and returns either a path string or NULL depending on whether it was
 * successful or not.
 */

char *request_translate_uri(char *uri, Session *sn);


/*
 * request_header finds the named header depending on the requesting
 * protocol. If possible, it will not load headers until the first is
 * requested. You have to watch out because this can return REQ_ABORTED.
 */

int request_header(char *name, char **value, Session *sn, Request *rq);

/*
 * request_loadheaders just makes sure the headers have been loaded.
 */

int request_loadheaders(Session *sn, Request *rq);


/*
 * request_stat_path tries to stat path. If path is NULL, it will look in
 * the vars pblock for "path". If the stat is successful, it returns the stat
 * structure. If not, returns NULL and leaves a message in rq->staterr. If a
 * previous call to this function was successful, and path is the same, the
 * function will simply return the previously found value.
 *
 * User functions should not free this structure.
 */

struct stat *request_stat_path(char *path, Request *rq);


/*
 * Parses the URI parameter in rq->vars and finds out what objects it
 * references (using NameTrans). Builds the request's object set.
 */

int request_uri2path(Session *sn, Request *rq);

/*
 * Performs any path checks needed for this request.
 */

int request_pathchecks(Session *sn, Request *rq);

/*
 * Does all the ObjectType directives for a request
 */

int request_fileinfo(Session *sn, Request *rq);


/*
 * request_handle_processed takes a Request structure with its reqpb
 * block filled in and handles the request.
 */

int request_handle_processed(Session *sn, Request *rq);


/*
 * Complete a request by finding the service function and using it. Returns
 * REQ_NOACTION if no matching function was found.
 */

int request_service(Session *sn, Request *rq);


/*
 * request_handle handles one request from the session's inbuf.
 */

void request_handle(Session *sn);

/*
 * Moved here due to problems with interdependency. See object.h for
 * description.
 */

int object_findnext(Session *sn, Request *rq, httpd_object *obj);
int object_pathcheck(Session *sn, Request *rq, httpd_object *obj);
int object_findinfo(Session *sn, Request *rq, httpd_object *obj);
int object_findservice(Session *sn, Request *rq, httpd_object *obj);
int object_finderror(Session *sn, Request *rq, httpd_object *obj);
int object_findlogs(Session *sn, Request *rq, httpd_object *obj);

#endif
