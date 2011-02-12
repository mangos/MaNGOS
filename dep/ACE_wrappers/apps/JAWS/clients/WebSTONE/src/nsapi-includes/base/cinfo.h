/*
 * $Id: cinfo.h 80826 2008-03-04 14:51:23Z wotte $
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
 * cinfo.h: Content Information for a file, i.e. its type, etc.
 *
 * See cinfo.c for dependency information.
 *
 * Rob McCool
 */



#ifndef CINFO_H
#define CINFO_H



/* ------------------------------ Constants ------------------------------- */


/*
 * This will be the first string in the file, followed by x.x version
 * where x is an integer.
 *
 * Updated due to trendy name change
 */

#define MCC_MT_MAGIC "#--Mosaic Communications Corporation MIME Information"
#define MCC_MT_MAGIC_LEN 53
#define NCC_MT_MAGIC "#--Netscape Communications Corporation MIME Information"
#define NCC_MT_MAGIC_LEN 55

/* The character which separates extensions with cinfo_find */

#define CINFO_SEPARATOR '.'

/* The maximum length of a line in this file */

#define CINFO_MAX_LEN 1024

/* The hash function for the database. Hashed on extension. */
#include <ctype.h>
#define CINFO_HASH(s) (isalpha(s[0]) ? tolower(s[0]) - 'a' : 26)

/* The hash table size for that function */
#define CINFO_HASHSIZE 27


/* ------------------------------ Structures ------------------------------ */


/*
 * The ContentInfo structure.
 *
 * Currently, we support the following attributes:
 *
 * 1. Type: This identifies what kind of data is in the file.
 * 2. Encoding: Identifies any compression or otherwise content-independent
 *    transformation which has been applied to the file (uuencode, etc.)
 * 3. Language: Identifies the language a text document is in.
 * 4. Description: A text string describing the file.
 * 5. Viewer: The program to use to view the file.
 *
 * Multiple items are separated with a comma, e.g.
 * encoding="x-gzip, x-uuencode"
 */

typedef struct {
    char *type;
    char *encoding;
    char *language;
} cinfo;


/* ------------------------------ Prototypes ------------------------------ */


/*
 * cinfo_init initializes the content info system. Call this before
 * cinfo_merge.
 */

void cinfo_init(void);

/*
 * cinfo_terminate frees the database for shutdown.
 */

void cinfo_terminate(void);

/*
 * cinfo_merge merges the contents of the given filename with the current
 * cinfo database. It returns NULL upon success and a string (which you
 * must deallocate) upon error.
 */

char *cinfo_merge(char *fn);


/*
 * cinfo_find finds any content information for the given uri. The file name
 * is the string following the last / in the uri. Multiple extensions are
 * separated by CINFO_SEPARATOR. You may pass in a filename instead of uri.
 *
 * Returns a newly allocated cinfo structure with the information it
 * finds. The elements of this structure are coming right out of the types
 * database and so if you change it or want to keep it around for long you
 * should strdup it. You should free only the structure itself when finished
 * with it.
 *
 * If there is no information for any one of the extensions it
 * finds, it will ignore that extension. If it cannot find information for
 * any of the extensions, it will return NULL.
 */

cinfo *cinfo_find(char *uri);

/*
 * cinfo_lookup finds the information about the given content-type, and
 * returns a cinfo structure so you can look up description and icon.
 */

cinfo *cinfo_lookup(char *type);

/*
 * cinfo_dump_database dumps the current database to the given file desc.
 */

#include <stdio.h>
void cinfo_dump_database(FILE *dump);


#endif
