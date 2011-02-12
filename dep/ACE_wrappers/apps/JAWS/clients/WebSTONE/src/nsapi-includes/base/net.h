/*
 * $Id: net.h 80826 2008-03-04 14:51:23Z wotte $
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
 * net.h: system specific networking definitions
 *
 * Rob McCool
 */


#ifndef NET_H
#define NET_H

#include "systems.h"

#include "file.h"       /* for client file descriptors */

#include "pblock.h"     /* for client data block */


/* This should be a user-given parameter later */
#define NET_BUFFERSIZE 8192
/* So should this. */
#define NET_READ_TIMEOUT 120
#define NET_WRITE_TIMEOUT 300

#define SSL_HANDSHAKE_TIMEOUT 300

#if defined(NET_SOCKETS) || defined(NET_SSL)

#ifdef NET_WINSOCK
#include <winsock.h>
#else /* XP_UNIX */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /* sockaddr and in_addr */
#include <arpa/inet.h>  /* inet_ntoa */
#include <netdb.h>      /* hostent stuff */
#endif /* NET_WINSOCK */

#ifdef NET_SSL
#include "minissl.h"
#endif


/* -------------------------------- Global -------------------------------- */

extern int net_enabledns;



/* ------------------------------ Data types ------------------------------ */


#ifdef NET_WINSOCK
typedef SOCKET SYS_NETFD;
#else /* XP_UNIX */
typedef int SYS_NETFD;
#endif /* NET_WINSOCK */

#define SYS_NET_ERRORFD -1


/* -------------------------------- Macros -------------------------------- */


/* These may be different for non-UNIX systems. */


#ifndef NET_SSL
#define net_socket socket
#define net_setsockopt setsockopt
#define net_getsockopt getsockopt
#define net_listen listen
#define net_select select
#define net_getpeername getpeername

#ifndef NET_WINSOCK
#define net_close(sd) close(sd)
#define net_bind bind
#else /* NET_WINSOCK */
#define net_close(sd) closesocket(sd)
#define system_netbind bind
int net_bind(SYS_NETFD s, const struct sockaddr *name, int namelen);
#endif /* NET_WINSOCK */

#ifdef DAEMON_NEEDS_SEMAPHORE
#define net_accept net_semaccept
#else  /* ! DAEMON_NEEDS_SEMAPHORE */
#define net_accept accept
#endif /* DAEMON_NEEDS_SEMAPHORE */

#else /* NET_SSL */
#define net_close(sd) SSL_Close(sd)
#define net_socket SSL_Socket
#define net_setsockopt SSL_SetSockOpt
#define net_getsockopt SSL_GetSockOpt

#ifdef XP_UNIX
#define net_bind SSL_Bind
#else /* WIN32 */
#define system_netbind SSL_Bind
int net_bind(SYS_NETFD s, const struct sockaddr *name, int namelen);
#endif /* XP_UNIX */

#define net_listen SSL_Listen
#define net_select select /* !!! */
#define net_getpeername SSL_GetPeerName
#define net_accept SSL_Accept
#endif /* ! NET_SSL */


/* Users should never call the system_net* functions.  */
#ifdef NET_SSL
#define system_netread(sd, buf, sz) SSL_Read(sd, buf, sz)
#define system_netwrite SSL_Write
#else  /* ! NET_SSL */

#if !defined(NET_WINSOCK)
#define system_netread(sd, buf, sz) read(sd, buf, sz)
#define system_netwrite write
#else /* NET_WINSOCK */
#define system_netread(sd, buf, sz) recv(sd, buf, sz, 0)
#define system_netwrite(sd, buf, sz) send(sd, buf, sz, 0)
#endif /* ! NET_WINSOCK */

#endif /* NET_SSL */

int net_read(SYS_NETFD sd, char *buf, int sz, int timeout);
int net_write(SYS_NETFD sd, char *buf, int sz);

#ifdef DAEMON_NEEDS_SEMAPHORE
int net_semaccept_init(int port);
int net_semaccept(int s, struct sockaddr *addr, int *addrlen);
void net_semaccept_terminate(void);
#endif


/* ------------------------------ Prototypes ------------------------------ */


/*
 * net_find_fqdn looks through the given hostent structure trying to find
 * a FQDN for the host. If it finds none, it returns NULL. Otherwise, it
 * returns a newly allocated copy of that string.
 */

char *net_find_fqdn(struct hostent *p);

/*
 * net_ip2host transforms the given textual IP number into a FQDN. If it
 * can't find a FQDN, it will return what it can get. Otherwise, NULL.
 *
 * verify is whether or not the function should verify the hostname it
 * gets. This takes an extra query but is safer for use in access control.
 */

char *net_ip2host(char *ip, int verify);

/*
 * net_sendmail sends mail to the specified recipient with the given subject
 * and message. Currently uses external programs.
 */

int net_sendmail(char *to, char *subject, char *msg);

#endif
#endif
