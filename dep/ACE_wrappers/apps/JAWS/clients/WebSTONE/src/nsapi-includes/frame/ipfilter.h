/*
 * $Id: ipfilter.h 80826 2008-03-04 14:51:23Z wotte $
 *
 * Copyright (c) 1994, 1995.  Netscape Communications Corporation.  All
 * rights reserved.
 *
 * Use of this software is governed by the terms of the license agreement for
 * the Netscape Communications or Netscape Comemrce Server between the
 * parties.
 */


/* ------------------------------------------------------------------------ */


#ifndef __ipfilter_h
#define __ipfilter_h

/* Define error codes */
#define IPFERR_MAX      (-1)    /* maximum error code value */
#define IPFERR_MALLOC   (-1)    /* insufficient memory */
#define IPFERR_FOPEN    (-2)    /* file open error */
#define IPFERR_FILEIO   (-3)    /* file I/O error */
#define IPFERR_DUPSPEC  (-4)    /* duplicate filter specification */
#define IPFERR_INTERR   (-5)    /* internal error (bug) */
#define IPFERR_SYNTAX   (-6)    /* syntax error in filter file */
#define IPFERR_CNFLICT  (-7)    /* conflicting filter specification */
#define IPFERR_MIN      (-7)    /* minimum error code value */

/* Define a scalar IP address value */
typedef unsigned long IPAddr_t;

/* Define structure for returning error information */
typedef struct IPFilterErr_s IPFilterErr_t;
struct IPFilterErr_s {
    int errNo;                  /* IPFERR_xxxx error code */
    int lineno;                 /* file line number, if applicable */
    char * filename;            /* filename, if applicable */
    char * errstr;              /* error text, if any */
};

/* Data and functions in ipfilter.c */
extern void * ipf_objndx;
extern void ip_filter_destroy(void * ipfptr);
extern int ip_filter_setup(pblock * client, IPFilterErr_t * reterr);
extern int ip_filter_check(pblock * client, IPAddr_t cip);

#endif /* __ipfilter_h */
