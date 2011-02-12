/*
 * $Id: file.h 80826 2008-03-04 14:51:23Z wotte $
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
 * file.h: system specific functions for reading/writing files
 *
 * Rob McCool
 */


#ifndef FILE_H
#define FILE_H

#ifdef XP_WIN32
#include <nt/ntfile.h>
#else


#include "netsite.h"
#include "systems.h"


/*
 * I cheat: These are set up such that system_read can be a macro for read
 * under UNIX. IO_OKAY is anything positive.
 */

#define IO_OKAY 1
#define IO_ERROR -1
#define IO_EOF 0


#ifdef FILE_STDIO
#include <stdio.h>

#elif defined(FILE_UNIX)
#include <sys/types.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif


/* -------------------------- File related defs --------------------------- */


/* The disk page size on this machine. */
#define FILE_BUFFERSIZE 4096


/*
 * The fd data type for this system.
 */

#if defined(FILE_STDIO)
typedef FILE* SYS_FILE;
#define SYS_ERROR_FD 0
#define SYS_STDERR stderr

#elif defined(FILE_UNIX)
typedef int SYS_FILE;
#define SYS_ERROR_FD -1
#define SYS_STDERR STDERR_FILENO

#else
#error "undefined file typing for current system"
#endif

#ifdef XP_UNIX
#define FILE_PATHSEP '/'
#define FILE_PARENT "../"

#define system_chdir chdir
#endif


/*
 * system_fread reads sz bytes from fd into to buf, return number of bytes
 * read, or IO_EOF if EOF, or IO_ERROR if error.
 */

#if defined(FILE_STDIO)
int system_fread(SYS_FILE fd, char *buf, int sz);

#elif defined(FILE_UNIX)
#define system_fread(fd,buf,sz) read(fd,buf,sz)

#endif

/*
 * system_fopenRO opens a given file for reading only
 * system_fopenWA opens a given file for writing, appending new output
 */

#if defined(FILE_STDIO)
#define system_fopenRO(path) fopen(path,"r")
#define system_fopenWA(path) fopen(path,"a")
#define system_fopenRW(path) fopen(path,"w")

#elif defined(FILE_UNIX)
#define system_fopenRO(path) open(path, O_RDONLY)
#define system_fopenWA(path) \
               open(path, O_RDWR | O_CREAT | O_APPEND, 0644)
#define system_fopenRW(path) \
               open(path, O_RDWR | O_CREAT, 0644)

#endif


/*
 * system_fclose closes the file fd
 */

#if defined(FILE_STDIO)
#define system_fclose(fd) fclose(fd)

#elif defined(FILE_UNIX)
#define system_fclose(fd) close(fd)
#endif

/*
 * This call stops core dumps in a portable way. Returns -1 on error.
 */

int system_nocoredumps(void);


#if defined(FILE_STDIO)
#define system_lseek fseek

#elif defined(FILE_UNIX)
#define system_lseek lseek

#endif

/*
 * system_write writes sz bytes from buf to fd. The handler function should
 * handle partial writes and anything else like that. Returns IO_*
 */

int system_fwrite(SYS_FILE fd,char *buf,int sz);

/*
 * system_fwrite_atomic locks the given fd before writing to it. This avoids
 * interference between simultaneous writes. Returns IO_*
 */

int system_fwrite_atomic(SYS_FILE fd, char *buf, int sz);

/*
 * system_errmsg returns the last error that occured while processing file
 * descriptor fd. fd does not have to be specified (if the error is a global
 * such as in UNIX systems). PPS: Rob is a halfwit. This parameter is useless.
 */

#ifndef FILE_WIN32
#include <errno.h>

extern char *sys_errlist[];
#define file_notfound() (errno == ENOENT)
#define system_errmsg(fd) (sys_errlist[errno])
#endif


/*
 * flock locks a file against interference from other processes
 * ulock unlocks it.
 */
#ifdef BSD_FLOCK
#include <sys/file.h>
#define system_initlock(fd) (0)
#define system_flock(fd) flock(fd, LOCK_EX)
#define system_ulock(fd) flock(fd, LOCK_UN)

#elif defined(FILE_UNIX)
#include <unistd.h>
#define system_initlock(fd) (0)
#define system_flock(fd) lockf(fd, F_LOCK, 0)
#define system_ulock(fd) lockf(fd, F_ULOCK, 0)

#endif


/*
 * unix2local converts a unix-style pathname to a local one
 */

#ifdef XP_UNIX
#define file_unix2local(path,p2) strcpy(p2,path)
#endif

/* -------------------------- Dir related defs ---------------------------- */


#ifdef XP_UNIX
#include <dirent.h>
typedef DIR* SYS_DIR;
typedef struct dirent SYS_DIRENT;
#define dir_open opendir
#define dir_read readdir
#define dir_close closedir

#endif
#endif
#endif
