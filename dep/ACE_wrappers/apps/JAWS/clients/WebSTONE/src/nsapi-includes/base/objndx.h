/*
 * $Id: objndx.h 80826 2008-03-04 14:51:23Z wotte $
 *
 * Copyright (c) 1994, 1995.  Netscape Communications Corporation.  All
 * rights reserved.
 *
 * Use of this software is governed by the terms of the license agreement for
 * the Netscape Communications or Netscape Comemrce Server between the
 * parties.
 */


/* ------------------------------------------------------------------------ */


#ifndef __objndx_h
#define __objndx_h

/* Define maximum length of object name strings */
#define OBJNDXNAMLEN    16

/* Functions in objndx.c */
extern void * objndx_create(int size, void (*freefunc)(void *));
extern char * objndx_register(void * objndx, void * objptr, char * namebuf);
extern void * objndx_lookup(void * objndx, char * objname);
extern void * objndx_remove(void * objndx, char * objname);
extern void objndx_destroy(void * objndx);

#endif /* __objndx_h */
