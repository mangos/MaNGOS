/*
 * $Id: session.h 80826 2008-03-04 14:51:23Z wotte $
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
 * session.h: Deals with virtual sessions
 *
 * A session is the time between when a client connects and when it
 * disconnects. Several requests may be handled in one session.
 *
 * Rob McCool
 */


#ifndef SESSION_H
#define SESSION_H


#include "../netsite.h"  /* MALLOC etc */
#include "net.h"          /* dns-related stuff */
#include "buffer.h"       /* netbuf */


/* ------------------------------ Structures ------------------------------ */



#define SESSION_HASHSIZE 5


typedef struct {
    /* Client-specific information */
    pblock *client;

    SYS_NETFD csd;
    netbuf *inbuf;

    struct in_addr iaddr;
} Session;


/* ------------------------------ Prototypes ------------------------------ */


/*
 * session_create creates a new request structure for the client with the
 * given socket descriptor and sockaddr.
 */

Session *session_create(SYS_NETFD csd, struct sockaddr_in *sac);

/*
 * session_free frees the given session
 */

void session_free(Session *sn);

/*
 * session_dns returns the DNS hostname of the client of this session,
 * and inserts it into the client pblock. Returns NULL if unavailable.
 */

#define session_dns(sn) session_dns_lookup(sn, 0)

/*
 * session_maxdns looks up a hostname from an IP address, and then verifies
 * that the host is really who they claim to be.
 */

#define session_maxdns(sn) session_dns_lookup(sn, 1)

char *session_dns_lookup(Session *sn, int verify);

#endif
