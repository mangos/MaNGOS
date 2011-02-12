/*
 * $Id: systems.h 80826 2008-03-04 14:51:23Z wotte $
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
 * systems.h: Lists of defines for systems
 *
 * This sets what general flavor the system is (UNIX, etc.),
 * and defines what extra functions your particular system needs.
 */


#ifndef SYSTEMS_H
#define SYSTEMS_H

#include <string.h>


#define DAEMON_ANY
#define DAEMON_LISTEN_SIZE 128
#ifndef MCC_ADMSERV
#define DAEMON_STATS
#endif

/* Linux is not currently supported */
#ifdef linux

#define FILE_UNIX
#undef FILE_STDIO
#undef DAEMON_UNIX_FORK
#undef DAEMON_UNIX_POOL
#define DAEMON_UNIX_MOBRULE
#undef DAEMON_STATS
#define BSD_FLOCK
#define BSD_RLIMIT
#define NET_SOCKETS
#define FILE_UNIX_MMAP
#define FILE_MMAP_FLAGS (MAP_FILE | MAP_PRIVATE)
#undef BSD_SIGNALS

#undef NEED_CRYPT_PROTO
#undef AUTH_DBM
#define SEM_FLOCK


#define ZERO(ptr,len) memset(ptr,0,len)

#elif defined(BSDI)

#define FILE_UNIX
#define DAEMON_UNIX_MOBRULE
#define BSD_FLOCK
#define BSD_RLIMIT
#define NET_SOCKETS
#define FILE_UNIX_MMAP
#define FILE_MMAP_FLAGS (MAP_FILE | MAP_PRIVATE)
#define BSD_SIGNALS
#define BSD_TIME
#define BSD_MAIL
#undef NEED_CRYPT_PROTO
#define AUTH_DBM
#define SEM_FLOCK

#define ZERO(ptr,len) memset(ptr,0,len)

#elif defined(SOLARIS)

#define FILE_UNIX
#undef FILE_STDIO
#define DAEMON_UNIX_MOBRULE
#define DAEMON_NEEDS_SEMAPHORE
#define NET_SOCKETS
#define FILE_UNIX_MMAP
#define FILE_MMAP_FLAGS MAP_PRIVATE
#undef BSD_SIGNALS
#define BSD_RLIMIT
#define NEED_CRYPT_H
#define AUTH_DBM
/* The Solaris routines return ENOSPC when too many semaphores are SEM_UNDO. */
#define SEM_FLOCK
#define DLL_CAPABLE
#define DLL_DLOPEN

#define ZERO(ptr,len) memset(ptr,0,len)

#elif defined(SUNOS4)

#define BSD_SIGNALS
#define BSD_TIME
#define BSD_MAIL
#define BSD_FLOCK
#define BSD_RLIMIT
#define FILE_UNIX
#undef FILE_STDIO
#define DAEMON_UNIX_MOBRULE
#define NET_SOCKETS
#define FILE_UNIX_MMAP
#define FILE_MMAP_FLAGS MAP_PRIVATE
#undef NEED_CRYPT_H
#define NEED_CRYPT_PROTO
#define AUTH_DBM
#define SEM_FLOCK
#define ZERO(ptr,len) memset(ptr,0,len)
#define DLL_CAPABLE
#define DLL_DLOPEN

#elif defined(OSF1)

#undef BSD_SIGNALS
#define BSD_TIME
#define BSD_FLOCK
#define BSD_RLIMIT
#define FILE_UNIX
#undef FILE_STDIO
#define DAEMON_UNIX_MOBRULE
#define DAEMON_NEEDS_SEMAPHORE
#define NET_SOCKETS
#define FILE_UNIX_MMAP
#define FILE_MMAP_FLAGS MAP_PRIVATE
#define AUTH_DBM
#define SEM_FLOCK
#define ZERO(ptr,len) memset(ptr,0,len)
#define DLL_CAPABLE
#define DLL_DLOPEN

#elif defined(AIX)

#define FILE_UNIX
#undef FILE_STDIO
#undef DAEMON_UNIX_FORK
#undef DAEMON_UNIX_POOL
#define DAEMON_UNIX_MOBRULE
#define DAEMON_NEEDS_SEMAPHORE
#define NET_SOCKETS
#define FILE_UNIX_MMAP
#define FILE_MMAP_FLAGS MAP_PRIVATE
#undef BSD_SIGNALS
#define BSD_RLIMIT
#undef NEED_CRYPT_H
#define AUTH_DBM
#define SEM_FLOCK
#define ZERO(ptr,len) memset(ptr,0,len)
#define DLL_CAPABLE
#define DLL_DLOPEN

#elif defined(HPUX)

#define FILE_UNIX
#undef FILE_STDIO
#define DAEMON_UNIX_MOBRULE
#define DAEMON_NEEDS_SEMAPHORE
#define NET_SOCKETS
#define FILE_UNIX_MMAP
#define FILE_MMAP_FLAGS MAP_PRIVATE
#undef BSD_SIGNALS
#undef BSD_RLIMIT
#undef NEED_CRYPT_H
#define AUTH_DBM
#define SEM_FLOCK
#define ZERO(ptr,len) memset(ptr,0,len)
#define DLL_CAPABLE
#define DLL_HPSHL

#elif defined (IRIX)

#define FILE_UNIX
#undef FILE_STDIO
#undef DAEMON_UNIX_FORK
#undef DAEMON_UNIX_POOL
#define DAEMON_UNIX_MOBRULE
#define DLL_CAPABLE
#define DLL_DLOPEN
#define NET_SOCKETS
#define FILE_UNIX_MMAP
#define FILE_MMAP_FLAGS MAP_PRIVATE
#undef BSD_SIGNALS
#define BSD_RLIMIT
#define NEED_CRYPT_H
#define AUTH_DBM
#define SEM_FLOCK
#define ZERO(ptr,len) memset(ptr,0,len)

#else      /* Windows NT */

#include <wtypes.h>
#include <winbase.h>

typedef void* PASSWD;

#define FILE_WIN32
#define NET_SOCKETS
#define NET_WINSOCK
#define DAEMON_WIN32
#undef AUTH_DBM
#define ZERO(ptr, len) ZeroMemory(ptr, len)
#define SEM_WIN32
#define DLL_CAPABLE
#define DLL_WIN32
#define NO_NODELOCK /* aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaagh */

/* The stat call under NT doesn't define these macros */
#define S_ISDIR(mode)   ((mode&S_IFMT) == S_IFDIR)
#define S_ISREG(mode)   ((mode&S_IFMT) == S_IFREG)

#define strcasecmp util_strcasecmp
#define strncasecmp util_strncasecmp
int util_strcasecmp(const char *s1, const char *s2);
int util_strncasecmp(const char *s1, const char *s2, int n);
#endif  /* Windows NT */

#endif
