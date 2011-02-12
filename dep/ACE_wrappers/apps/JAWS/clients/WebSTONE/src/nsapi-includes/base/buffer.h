/*
 * $Id: buffer.h 80826 2008-03-04 14:51:23Z wotte $
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
 * buffer.h: For performing buffered I/O on a file or socket descriptor.
 *
 * This is an abstraction to allow I/O to be performed regardless of the
 * current system. That way, an integer file descriptor can be used under
 * UNIX but a stdio FILE structure could be used on systems which don't
 * support that or don't support it as efficiently.
 *
 * Two abstractions are defined: A file buffer, and a network buffer. A
 * distinction is made so that mmap() can be used on files (but is not
 * required). Also, the file buffer takes a file name as the object to
 * open instead of a file descriptor. A lot of the network buffering
 * is almost an exact duplicate of the non-mmap file buffering.
 *
 * If an error occurs, system-independent means to obtain an error string
 * are also provided. However, if the underlying system is UNIX the error
 * may not be accurate in a threaded environment.
 *
 * Rob McCool
 *
 */


#ifndef BUFFER_H
#define BUFFER_H

#ifdef XP_WIN32
#include <nt/ntbuffer.h>
#else


/*
 * We need certain system specific functions and symbols.
 */

#include "file.h"
#include "net.h"

/*
 * Requires that the macro MALLOC be set to a "safe" malloc that will
 * exit if no memory is available. If not under MCC httpd, define MALLOC
 * to be the real malloc and play with fire, or make your own function.
 */

#include "../netsite.h"

#ifdef FILE_UNIX_MMAP
#include <sys/types.h>          /* caddr_t */
#endif


/* ------------------------------ Structures ------------------------------ */

#ifdef FILE_UNIX_MMAP
typedef struct {
    SYS_FILE fd;
    caddr_t fp;
    int len;

    char *inbuf;   /* for buffer_grab */
    int cursize;

    int pos;
    char *errmsg;
} filebuf;

#else

typedef struct {
    SYS_FILE fd;

    int pos, cursize, maxsize;
    char *inbuf;
    char *errmsg;
} filebuf;

#endif

typedef struct {
    SYS_NETFD sd;

    int pos, cursize, maxsize, rdtimeout;
    char *inbuf;
    char *errmsg;
} netbuf;


/* -------------------------------- Macros -------------------------------- */


/*
 * netbuf_getc gets a character from the given network buffer and returns
 * it. (as an integer).
 *
 * It will return (int) IO_ERROR for an error and (int) IO_EOF for
 * an error condition or EOF respectively.
 */

#define netbuf_getc(b) \
 ((b)->pos != (b)->cursize ? (int)((b)->inbuf[(b)->pos++]) : netbuf_next(b,1))

#ifdef FILE_UNIX_MMAP
#define filebuf_getc(b) ((b)->pos == (b)->len ? IO_EOF : (b)->fp[(b)->pos++])
#else
#define filebuf_getc(b) \
 ((b)->pos != (b)->cursize ? (int)((b)->inbuf[(b)->pos++]) : filebuf_next(b,1))
#endif


/*
 * buffer_error returns the last error that occurred with buffer. Don't use
 * this unless you know an error occurred. Independent of network/file type.
 */

#define buffer_error(b) ((b)->errmsg)

/*
 * buffer_flush flushes any data after the current pos to the file
 * descriptor fd. Regardless of buffer type.
 */

#define buffer_flush(buf,fd) \
    system_write(fd,&(buf)->inbuf[(buf)->pos], (buf)->cursize - (buf)->pos)


/* ------------------------------ Prototypes ------------------------------ */


/*
 * buffer_open opens a new buffer reading the specified file, with an I/O
 * buffer of size sz, and returns a new buffer structure which will hold
 * the data.
 *
 * If FILE_UNIX_MMAP is defined, this may return NULL. If it does, check
 * system_errmsg to get a message about the error.
 */

filebuf *filebuf_open(SYS_FILE fd, int sz);
netbuf *netbuf_open(SYS_NETFD sd, int sz);

/*
 * filebuf_open_nostat is a convenience function for mmap() buffer opens,
 * if you happen to have the stat structure already.
 */

#ifdef FILE_UNIX_MMAP
#include <sys/stat.h>
filebuf *filebuf_open_nostat(SYS_FILE fd, int sz, struct stat *finfo);

#else
#define filebuf_open_nostat(fd,sz,finfo) filebuf_open(fd,sz)
#endif

/*
 * buffer_next loads size more bytes into the given buffer and returns the
 * first one, or BUFFER_EOF on EOF and BUFFER_ERROR on error.
 */

int filebuf_next(filebuf *buf, int advance);
int netbuf_next(netbuf *buf, int advance);

/*
 * buffer_close deallocates a buffer and closes its associated files
 * (does not close a network socket).
 */

void filebuf_close(filebuf *buf);
void netbuf_close(netbuf *buf);

/*
 * buffer_grab will set the buffer's inbuf array to an array of sz bytes
 * from the buffer's associated object. It returns the number of bytes
 * actually read (between 1 and sz). It returns IO_EOF upon EOF or IO_ERROR
 * upon error. The cursize entry of the structure will reflect the size
 * of the iobuf array.
 *
 * The buffer will take care of allocation and deallocation of this array.
 */

int filebuf_grab(filebuf *buf, int sz);
int netbuf_grab(netbuf *buf, int sz);


/*
 * netbuf_buf2sd will send n bytes from the (probably previously read)
 * buffer and send them to sd. If sd is -1, they are discarded. If n is
 * -1, it will continue until EOF is recieved. Returns IO_ERROR on error
 * and the number of bytes sent any other time.
 */

int netbuf_buf2sd(netbuf *buf, SYS_NETFD sd, int len);

/*
 * filebuf_buf2sd assumes that nothing has been read from the filebuf,
 * and just sends the file out to the given socket. Returns IO_ERROR on error
 * and the number of bytes sent otherwise.
 *
 * Does not currently support you having read from the buffer previously. This
 * can be changed transparently.
 */

int filebuf_buf2sd(filebuf *buf, SYS_NETFD sd);

#endif
#endif
