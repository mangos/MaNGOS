/*
 * $Id: object.h 80826 2008-03-04 14:51:23Z wotte $
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
 * object.h: Handle httpd objects
 *
 * Manages information about a document from config. files. Called mainly
 * by objset.c.
 *
 * This module does not assume anything about the directives being parsed.
 * That is handled by objset.c.
 *
 * This module requires the pblock module from the base library.
 *
 * Rob McCool
 *
 */


#ifndef OBJECT_H
#define OBJECT_H


#include "netsite.h"
#include "base/pblock.h"
#include "base/session.h"




/* ------------------------------ Constants ------------------------------- */


/* The maximum directive length unabbreviated, plus one space */
#define MAX_DNAME_LEN 11
#define NUM_DIRECTIVES 7


/* ------------------------------ Structures ------------------------------ */



/*
 * Hierarchy of httpd_object
 *
 * An object contains dtables.
 *
 * Each dtable is a table of directives that were entered of a certain type.
 * There is one dtable for each unique type of directive.
 *
 * Each dtable contains an array of directives, each of which is equivalent
 * to one directive that occurred in a config. file.
 *
 * It is up to the caller to determine how many dtables will be allocated
 * and to keep track of which of their directive types maps to which dtable
 * number.
 */


/*
 * directive is a structure containing the protection and parameters to an
 * instance of a directive within an httpd_object.
 *
 * param is the parameters, client is the protection.
 */

typedef struct {
    pblock *param;
    pblock *client;
} directive;

/*
 * dtable is a structure for creating tables of directives
 */

typedef struct {
    int ni;
    directive *inst;
} dtable;

/*
 * The httpd_object structure.
 *
 * The name pblock array contains the names for this object, such as its
 * virtual location, its physical location, or its identifier.
 *
 * tmpl contains any templates allocated to this object.
 */

typedef struct {
    pblock *name;

    int nd;
    dtable *dt;
} httpd_object;




/* ------------------------------ Prototypes ------------------------------ */


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
 * object_create will create a new object and return a pointer to it.
 * It will allocate space for nd directive types and set name accordingly.
 */

httpd_object *object_create(int nd, pblock *name);

/*
 * object_free will free an object and any data associated with it.
 */

void object_free(httpd_object *obj);

/*
 * object_add_directive will add a new directive to the dtable for
 * the directive class at position dc.
 */

void object_add_directive(int dc, pblock *p, pblock *c, httpd_object *obj);


/*
 * object_findnext finds the object configured to follow the given object,
 * and stores the variables in rq->vars. It returns REQ_PROCEED if more
 * objects should be processed, or REQ_NOACTION if it did not find any
 * further objects to process. If something bad happens, REQ_ABORTED is
 * returned.
 *
 * Handles all DIRECTIVE_CONSTRUCT type directives such as NameTrans and
 * AuthType.
 */


/* --------- Prototype moved to req.h because of interdependency ---------- */

#endif
