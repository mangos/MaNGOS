/*
 * $Id: pblock.h 80826 2008-03-04 14:51:23Z wotte $
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
 * pblock.h: Header for Parameter Block handling functions
 *
 *
 * A parameter block is a set of name=value pairs which are generally used
 * as parameters, but can be anything. They are kept in a hash table for
 * reasonable speed, but if you are doing any intensive modification or
 * access of them you should probably make a local copy of each parameter
 * while working.
 *
 * When creating a pblock, you specify the hash table size for that pblock.
 * You should set this size larger if you know that many items will be in
 * that pblock, and smaller if only a few will be used or if speed is not
 * a concern.
 *
 * The hashing function is very simple right now, and only looks at the
 * first character of name.
 *
 * Rob McCool
 *
 */

#ifndef PBLOCK_H
#define PBLOCK_H

/*
 * Requires that the macros MALLOC and STRDUP be set to "safe" versions that
 * will exit if no memory is available. If not under MCC httpd, define
 * them to be the real functions and play with fire, or make your own
 * function.
 */

#include "../netsite.h"

#include <ctype.h>  /* isspace */
#include <stdio.h>  /* sprintf */
#include <string.h> /* strlen, strcmp */


/* ------------------------------ Structures ------------------------------ */


typedef struct {
    char *name,*value;
} pb_param;

struct pb_entry {
    pb_param *param;
    struct pb_entry *next;
};

typedef struct {
    int hsize;
    struct pb_entry **ht;
} pblock;


/* ------------------------------ Prototypes ------------------------------ */


/*
 * param_create creates a parameter with the given name and value. If name
 * and value are non-NULL, they are copied and placed into the new pb_param
 * struct.
 */

pb_param *param_create(char *name, char *value);

/*
 * param_free frees a given parameter if it's non-NULL, and returns 1 if
 * p was non-NULL, and 0 if p was NULL.
 *
 * Useful for error checking pblock_remove.
 */

int param_free(pb_param *pp);

/*
 * pblock_create creates a new pblock with hash table size n.
 *
 * It returns the newly allocated pblock.
 */

pblock *pblock_create(int n);

/*
 * pblock_free frees the given pblock and any entries inside it.
 *
 * If you want to save anything in a pblock, remove its entities with
 * pblock_remove first and save the pointers you get.
 */

void pblock_free(pblock *pb);

/*
 * pblock_find finds the entry with the given name in pblock pb.
 *
 * If it is successful, it returns the param block. If not, it returns NULL.
 */

#define pblock_find(name, pb) (_pblock_fr(name,pb,0))

/*
 * pblock_findval finds the entry with the given name in pblock pb, and
 * returns its value, otherwise returns NULL.
 */

char *pblock_findval(char *name, pblock *pb);

/*
 * pblock_remove behaves exactly like pblock_find, but removes the given
 * entry from pb.
 */

#define pblock_remove(name, pb) (_pblock_fr(name,pb,1))

/*
 * pblock_nvinsert creates a new parameter with the given name and value
 * and inserts it into pblock pb. The name and value in the parameter are
 * also newly allocated. Returns the pb_param it allocated (in case you
 * need it).
 *
 * pblock_nninsert inserts a numerical value.
 */

pb_param *pblock_nvinsert(char *name, char *value, pblock *pb);
pb_param *pblock_nninsert(char *name, int value, pblock *pb);

/*
 * pblock_pinsert inserts a pb_param into a pblock.
 */

void pblock_pinsert(pb_param *pp, pblock *pb);

/*
 * pblock_str2pblock scans the given string str for parameter pairs
 * name=value, or name="value". Any \ must be followed by a literal
 * character. If a string value is found, with no unescaped = signs, it
 * will be added with the name 1, 2, 3, etc. depending on whether it was
 * first, second, third, etc. in the stream (zero doesn't count).
 *
 * Returns the number of parameters added to the table, or -1 upon error.
 */

int pblock_str2pblock(char *str, pblock *pb);

/*
 * pblock_pblock2str places all of the parameters in the given pblock
 * into the given string (NULL if it needs creation). It will re-allocate
 * more space for the string. Each parameter is separated by a space and of
 * the form name="value"
 */

char *pblock_pblock2str(pblock *pb, char *str);

/*
 * pblock_copy copies the entries in the given source pblock to the
 * destination one. The entries are newly allocated so that the original
 * pblock may be freed or the new one changed without affecting the other.
 */

void pblock_copy(pblock *src, pblock *dst);

/*
 * pblock_pb2env copies the given pblock into the given environment, with
 * one new env entry for each name/value pair in the pblock.
 */

char **pblock_pb2env(pblock *pb, char **env);


/* --------------------------- Internal things ---------------------------- */


pb_param *_pblock_fr(char *name, pblock *pb, int remove);


#endif
