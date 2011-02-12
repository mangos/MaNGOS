/*
 * $Id: func.h 80826 2008-03-04 14:51:23Z wotte $
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
 * func.h: Handles the function hash table
 *
 * httpd uses a table of internal functions hashed by a name string such that
 * users can reference specific functions from the configuration files.
 *
 * Any function referenced by configuration files will be passed a
 * parameter, a Request structure. The functions do not return anything.
 *
 * Rob McCool
 */

#ifndef FUNC_H
#define FUNC_H


#include "netsite.h"
#include "base/pblock.h"
#include "base/session.h"   /* Session structure */
#include "frame/req.h"      /* Request structure */


/* -------------------------- Structure and Type -------------------------- */


/*
 * FuncPtr is a pointer to our kind of functions
 */

typedef int Func(pblock *, Session *, Request *);
typedef Func *FuncPtr;

/*
 * FuncStruct is a structure used in the static declaration of the
 * functions. This static declaration is parsed into a hash table at
 * startup. You should initialize the next entry to NULL.
 */

struct FuncStruct {
    char *name;
    FuncPtr func;
    struct FuncStruct *next;
};


/* --------------------------- Hash definitions --------------------------- */


/*
 * This is a primitive hash function. Once more is known about the names of
 * the functions, this will be optimized.
 */

#define NUM_HASH 20
#define FUNC_HASH(s) (s[0] % NUM_HASH)


/* ------------------------------ Prototypes ------------------------------ */


/*
 * func_init reads the static FuncStruct arrays and creates the global
 * function table from them.
 *
 * func_init will only read from the static arrays defined in func.c.
 */

void func_init(void);

/*
 * func_find returns a pointer to the function named name, or NULL if none
 * exists.
 */

FuncPtr func_find(char *name);

/*
 * func_exec will try to execute the function whose name is the "fn" entry
 * in the given pblock. If name is not found, it will log a misconfig of
 * missing fn parameter. If it can't find it, it will log that. In these
 * cases it will return REQ_ABORTED. Otherwise, it will return what the
 * function being executed returns.
 */

int func_exec(pblock *pb, Session *sn, Request *rq);

/*
 * func_insert dynamically inserts a named function into the server's
 * table of functions. Returns the FuncStruct it keeps in internal
 * databases, because on server restart you are responsible for freeing
 * (or not) its contents.
 */

struct FuncStruct *func_insert(char *name, FuncPtr fn);

#endif
