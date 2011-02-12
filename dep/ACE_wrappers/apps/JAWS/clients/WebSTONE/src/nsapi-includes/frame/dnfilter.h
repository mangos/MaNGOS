/*
 * $Id: dnfilter.h 80826 2008-03-04 14:51:23Z wotte $
 *
 * Copyright (c) 1994, 1995.  Netscape Communications Corporation.  All
 * rights reserved.
 *
 * Use of this software is governed by the terms of the license agreement for
 * the Netscape Communications or Netscape Comemrce Server between the
 * parties.
 */


/* ------------------------------------------------------------------------ */


#ifndef __dnfilter_h
#define __dnfilter_h

/* Error codes */
#define DNFERR_MAX      (-1)    /* maximum error code */
#define DNFERR_MALLOC   (-1)    /* insufficient memory */
#define DNFERR_FOPEN    (-2)    /* file open error */
#define DNFERR_FILEIO   (-3)    /* file I/O error */
#define DNFERR_DUPSPEC  (-4)    /* duplicate filter specification */
#define DNFERR_INTERR   (-5)    /* internal error (bug) */
#define DNFERR_SYNTAX   (-6)    /* syntax error in filter file */
#define DNFERR_MIN      (-6)    /* minimum error code */

/* This is used to return error information from dns_filter_setup() */
typedef struct DNSFilterErr_s DNSFilterErr_t;
struct DNSFilterErr_s {
    int errNo;                  /* DNFERR_xxxx error code */
    int lineno;                 /* file line number, if applicable */
    char * filename;            /* filename, if applicable */
    char * errstr;              /* error text, if any */
};

/* Data and functions in dnfilter.c */
extern void * dnf_objndx;
extern void dns_filter_destroy(void * dnfptr);
extern int dns_filter_setup(pblock * client, DNSFilterErr_t * reterr);
extern int dns_filter_check(pblock * client, char * cdns);

#endif /* __dnfilter_h */
