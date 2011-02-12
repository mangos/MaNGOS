/*
 * $Id: shexp.h 91995 2010-09-24 12:45:24Z johnnyw $
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
 * shexp.h: Defines and prototypes for shell exp. match routines
 *
 *
 * This routine will match a string with a shell expression. The expressions
 * accepted are based loosely on the expressions accepted by zsh.
 *
 * o * matches anything
 * o ? matches one character
 * o \ will escape a special character
 * o $ matches the end of the string
 * o [abc] matches one occurrence of a, b, or c. The only character that needs
 *         to be escaped in this is ], all others are not special.
 * o [a-z] matches any character between a and z
 * o [^az] matches any character except a or z
 * o ~ followed by another shell expression will remove any pattern
 *     matching the shell expression from the match list
 * o (foo|bar) will match either the substring foo, or the substring bar.
 *             These can be shell expressions as well.
 *
 * The public interface to these routines is documented below.
 *
 * Rob McCool
 *
 */

#ifndef SHEXP_H
#define SHEXP_H

/*
 * Requires that the macro MALLOC be set to a "safe" malloc that will
 * exit if no memory is available. If not under MCC httpd, define MALLOC
 * to be the real malloc and play with fire, or make your own function.
 */

#include "../netsite.h"

#include <ctype.h>  /* isalnum */
#include <string.h> /* strlen */



/* --------------------------- Public routines ---------------------------- */


/*
 * shexp_valid takes a shell expression exp as input. It returns:
 *
 *  NON_SXP      if exp is a standard string
 *  INVALID_SXP  if exp is a shell expression, but invalid
 *  VALID_SXP    if exp is a valid shell expression
 */

#define NON_SXP -1
#define INVALID_SXP -2
#define VALID_SXP 1

int shexp_valid(char *exp);

/*
 * shexp_match
 *
 * Takes a prevalidated shell expression exp, and a string str.
 *
 * Returns 0 on match and 1 on non-match.
 */

int shexp_match(char *str, char *exp);


/*
 * shexp_cmp
 *
 * Same as above, but validates the exp first. 0 on match, 1 on non-match,
 * -1 on invalid exp. shexp_casecmp does the same thing but is case
 * insensitive.
 */

int shexp_cmp(char *str, char *exp);
int shexp_casecmp(char *str, char *exp);

#endif
