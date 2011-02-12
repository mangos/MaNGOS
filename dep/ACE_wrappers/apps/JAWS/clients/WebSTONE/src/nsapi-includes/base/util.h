/*
 * $Id: util.h 80826 2008-03-04 14:51:23Z wotte $
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
 * util.h: A hodge podge of utility functions and standard functions which
 *         are unavailable on certain systems
 *
 * Rob McCool
 */


#ifndef HTTPD_UTIL_H
#define HTTPD_UTIL_H

#include "buffer.h"    /* filebuf for getline */

#include <time.h>      /* struct tm */


/* ------------------------------ Prototypes ------------------------------ */


/*
 * getline scans in buf until it finds a LF or CRLF, storing the string in
 * l. It will terminate the string and return:
 *
 *  0 when done, with the scanned line (minus CR or LF) in l
 *  1 upon EOF, with the scanned line (minus CR or LF) in l
 * -1 on error with the error description in l (uses lineno for information)
 */

int util_getline(filebuf *buf, int lineno, int maxlen, char *l);


/*
 * can_exec returns 1 if you can execute the file described by finfo, and
 * 0 if you can't.
 */

#ifdef XP_UNIX
#include <sys/stat.h>
#include <sys/types.h>

int util_can_exec(struct stat *finfo, uid_t uid, gid_t gid);

#endif /* XP_UNIX */
/*
 * env_create creates a new environment with the given env, with n new
 * entries, and places the current position that you should add your
 * entries with at pos.
 *
 * If env is NULL, it will allocate a new one. If not, it will reallocate
 * that one.
 */

char **util_env_create(char **env, int n, int *pos);

/*
 * util_env_str allocates a string from the given name and value and
 * returns it. It does not check for things like = signs in name.
 */

char *util_env_str(char *name, char *value);

/*
 * env_replace replaces the occurrence of the given variable with the
 * value you give.
 */

void util_env_replace(char **env, char *name, char *value);

/*
 * util_env_free frees an environment.
 */

void util_env_free(char **env);

/*
 * util_env_find looks through env for the named string. Returns the
 * corresponding value if the named string is found, or NULL if not.
 */
char *util_env_find(char **env, char *name);


/*
 * hostname gets the local hostname. Returns NULL if it can't find a FQDN.
 * You are free to realloc or free this string.
 */

char *util_hostname(void);


/*
 * chdir2path changes the current directory to the one that the file
 * path is in. path should point to a file. Caveat: path must be a writable
 * string. It won't get modified permanently.
 */

int util_chdir2path(char *path);

/*
 * is_mozilla checks if the given user-agent is mozilla, of at least
 * the given major and minor revisions. These are strings to avoid
 * ambiguities like 1.56 > 1.5
 */

int util_is_mozilla(char *ua, char *major, char *minor);

/*
 * is_url will return 1 if the given string seems to be a URL, or will
 * return 0 otherwise.
 *
 * Because of stupid news URLs, this will return 1 if the string has
 * all alphabetic characters up to the first colon and will not check for
 * the double slash.
 */

int util_is_url(char *url);

/*
 * util_later_than checks the date in the string ims, and if that date is
 * later than or equal to the one in the tm struct lms, then it returns 1.
 *
 * Handles RFC 822, 850, and ctime formats.
 */

int util_later_than(struct tm *lms, char *ims);


/*
 * util_uri_is_evil returns 1 if a URL has ../ or // in it.
 */
int util_uri_is_evil(char *t);

/*
 * util_uri_parse gets rid of /../, /./, and //.
 *
 * Assumes that either the string starts with a /, or the string will
 * not .. right off of its beginning.  As such, ../foo.gif will
 * not be changed, although /../foo.gif will become /foo.gif.
 */

void util_uri_parse(char *uri);

/*
 * util_uri_unescape unescapes the given URI in place (% conversions only).
 */

void util_uri_unescape(char *s);

/*
 * util_uri_escape escapes any nasty chars in s and copies the string into d.
 * If d is NULL, it will allocate and return a properly sized string.
 * Warning: does not check bounds on a given d.
 *
 * util_url_escape does the same thing but does it for a url, i.e. ?:+ is
 * not escaped.
 */

char *util_uri_escape(char *d, char *s);
char *util_url_escape(char *d, char *s);

/*
 * util_sh_escape places a \ in front of any shell-special characters.
 * Returns a newly-allocated copy of the string.
 */

char *util_sh_escape(char *s);

/*
 * util_itoa converts the given integer to a string into a.
 */

int util_itoa(int i, char *a);

/*
 * util_vsprintf and util_sprintf are simplified clones of the System V
 * vsprintf and sprintf routines.
 *
 * Returns the number of characters printed. Only handles %d and %s,
 * does not handle any width or precision.
 */

#include <stdarg.h>

int util_vsprintf(char *s, register char *fmt, va_list args);
int util_sprintf(char *s, char *fmt, ...);

/* These routines perform bounds checks. */
int util_vsnprintf(char *s, int n, register char *fmt, va_list args);
int util_snprintf(char *s, int n, char *fmt, ...);

#endif
