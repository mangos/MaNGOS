/*
 * $Id: objset.h 80826 2008-03-04 14:51:23Z wotte $
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
 * objset.h: Handles object sets
 *
 * Each object is produced by reading a config file of some form. See the
 * server documentation for descriptions of the directives that are
 * recognized, what they do, and how they are parsed.
 *
 * This module requires the pblock and buffer modules from the base library.
 *
 * Rob McCool
 */


#ifndef OBJSET_H
#define OBJSET_H

#ifdef XP_WIN32
#include <nt/ntobjset.h>
#else


#include "netsite.h"
#include "base/pblock.h"
#include "base/buffer.h"
#include "frame/object.h"



/* ------------------------------ Constants ------------------------------- */


/*
 * The default number of objects to leave room for in an object set,
 * and the number of new entries by which to increase the size when that
 * room is filled.
 */

#define OBJSET_INCSIZE 8

/*
 * When parsing config. files, httpd will put a limit on how long
 * the parameters to a directive can be (in characters).
 *
 * Currently set to 10 lines (80 chars/line).
 */

#define MAX_DIRECTIVE_LEN 800

/*
 * The size of the hash tables that store a directive's parameters
 */

#define PARAMETER_HASH_SIZE 3


/* ------------------------------ Structures ------------------------------ */


/*
 * httpd_objset is a container for a bunch of objects. obj is a
 * NULL-terminated array of objects. pos points to the entry after the last
 * one in the array. You should not mess with pos, but can read it to find
 * the last entry.
 */

typedef struct {
    int pos;
    httpd_object **obj;
} httpd_objset;


/* ------------------------------ Prototypes ------------------------------ */


/*
 * objset_scan_buffer will scan through buffer, looking for object
 * configuration information, and adding them to the object set os if it
 * finds any. If os is NULL it will allocate a new object set.
 *
 * If any error occurs (syntax error, premature EOF) this function will
 * free os, print an error message into errstr, and return NULL.
 * This is because a config. file error is viewed as a catastrophic error
 * from which httpd should not try to recover. If httpd were to continue
 * after an error, it would not behave as the admin. expected and he/she
 * may not notice until it's too late.
 *
 * Upon EOF the file will not be closed.
 */

httpd_objset *objset_scan_buffer(filebuf *buf, char *errstr, httpd_objset *os);

/*
 * objset_create creates a new object set and returns a pointer to it.
 */

httpd_objset *objset_create(void);

/*
 * objset_free will free an object set and any associated objects.
 */

void objset_free(httpd_objset *os);

/*
 * objset_free_setonly frees only the object set.
 */
void objset_free_setonly(httpd_objset *os);

/*
 * objset_new_object will add a new object to objset with the specified
 * name. It returns a pointer to the new object (which may be anywhere in
 * the objset).
 */

httpd_object *objset_new_object(pblock *name, httpd_objset *os);

/*
 * objset_add_object will add the existing object to os.
 */

void objset_add_object(httpd_object *obj, httpd_objset *os);

/*
 * objset_findbyname will find the object in objset having the given name,
 * and return the object if found, and NULL otherwise.
 * ign is a set of objects to ignore.
 */

httpd_object *objset_findbyname(char *name, httpd_objset *ign,
                                httpd_objset *os);

/*
 * objset_findbyppath will find the object in objset having the given
 * partial path entry. Returns object if found, NULL otherwise.
 * ign is a set of objects to ignore.
 */

httpd_object *objset_findbyppath(char *ppath, httpd_objset *ign,
                                 httpd_objset *os);


#endif
#endif
