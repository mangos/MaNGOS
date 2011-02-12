/*
 * $Id: minissl.h 80826 2008-03-04 14:51:23Z wotte $
 *
 * Copyright (c) 1994, 1995.  Netscape Communications Corporation.  All
 * rights reserved.
 *
 * Use of this software is governed by the terms of the license agreement for
 * the Netscape Communications or Netscape Comemrce Server between the
 * parties.
 */


/* ------------------------------------------------------------------------ */



/* Prototypes for SSL I/O functions */
extern int SSL_Close(int);
extern int SSL_Socket(int, int, int);
extern int SSL_GetSockOpt(int, int, int, void *, int *);
extern int SSL_SetSockOpt(int, int, int, const void *, int);
extern int SSL_Bind(int, const void *, int);
extern int SSL_Listen(int, int);
extern int SSL_Accept(int, void *, int *);
extern int SSL_Read(int, void *, int);
extern int SSL_Write(int, const void *, int);
extern int SSL_GetPeerName(int, void *, int *);
