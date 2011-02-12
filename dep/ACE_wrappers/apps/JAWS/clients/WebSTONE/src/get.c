/* $Id: get.c 81994 2008-06-16 21:23:17Z sowayaa $ */
/**************************************************************************
 *
 *  Copyright (C) 1995 Silicon Graphics, Inc.
 *
 *  These coded instructions, statements, and computer programs were
 *  developed by SGI for public use.  If any changes are made to this code
 *  please try to get the changes back to the author.  Feel free to make
 *  modifications and changes to the code and release it.
 *
 **************************************************************************/

/* THIS IS WHERE WE GO OUT AND FETCH A URL */

#include <stdio.h>
#include <errno.h>

#ifndef WIN32
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <netdb.h>
#endif /* WIN32 */

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef SUNOS
#include <unistd.h>
#endif
#include <ctype.h>

#ifdef WIN32
#include <io.h>
#include <windows.h>
#include <winsock.h>
#endif /* WIN32 */

#include "sysdep.h"
#include "bench.h"

#define ACCEPT_COMMAND "Accept: */* HTTP/1.0\r\n\r\n"
#define ACCEPT_COMMAND_LEN ((int)strlen(ACCEPT_COMMAND))
#define MAXCOMMANDLEN 256
#define HEADERBUFSIZ (8*1024)

#define UPPER(c) (((c) >= 'a' && (c) <= 'z') ? (c) + 'A' - 'a' : (c))

/* compare two strings with max length, ignoring case */
int mystrincmp(const char *str1, const char *str2, int len) {
    register int diff;

    while (*str1 && *str2 && len--) {
        if (diff = UPPER(*str1) - UPPER(*str2))
            return diff < 0 ? -1 : 1;
        str1++;
        str2++;
    }
    return 0;
}

int
get(char *loc, NETPORT port, char *url, rqst_timer_t *timer)
{
    SOCKET sock = BADSOCKET_VALUE;
    int writelen;
    int bytesread;
    int totalbytesread;
    int headerlen;
    int bodylength;
    int contentlength = 0;
    int outputfile = -1;
    int status;
    char getcommand[MAXCOMMANDLEN];
    char headerbuffer[HEADERBUFSIZ+1];
    char *offset;
    char outputfilename[MAXPATHLEN];
    char version[100];
    int count;

/*#define ABORTIVE_CLOSE 1*/
#ifdef ABORTIVE_CLOSE
#error don't enable this option
    struct linger {
        int l_onoff;
        int l_linger;
    } linger_opt;
#endif /* ABORTIVE_CLOSE */

    /* can you really get an error from gettimeofday?? */
    if(GETTIMEOFDAY(&timer->entertime, &timer->entertimezone) != 0)
    {
        returnerr("Error retrieving entertime\n");
        goto error;
    }
    timer->valid = 1;

    if(GETTIMEOFDAY(&timer->beforeconnect, &timer->beforeconnectzone) != 0)
    {
        returnerr("Error retrieving beforeconnect\n");
        goto error;
    }

    sock = connectsock(loc, port, "tcp");
    if (BADSOCKET(sock))
    {
        D_PRINTF( "Call to connectsock returned %d (%s)\n", sock, neterrstr() );
        returnerr("Couldn't connect to WWW server: %s\n", neterrstr());
        goto error;
    }

#ifdef ABORTIVE_CLOSE
#error don't enable this option
    /* set up for abortive close */
    linger_opt.l_onoff = 1;
    linger_opt.l_linger = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *) &linger_opt, sizeof(linger_opt)) < 0) {
        fprintf(stderr, "Can't set sockopt SO_LINGER");
        returnerr("Couldn't set SO_LINGER = 0\n");
        goto error;
    }
#endif /* ABORTIVE_CLOSE */

    if(GETTIMEOFDAY(&timer->afterconnect, &timer->afterconnectzone) != 0)
    {
        NETCLOSE(sock);
        GETTIMEOFDAY(&timer->exittime, &timer->exittimezone);
        returnerr("Error retrieving afterconnect\n");
        goto error;
    }

    /*
     * SEND THE GET AND THE ACCEPT.
     */
    sprintf(getcommand, "GET %s HTTP/1.0\r\n%s", url,
            ACCEPT_COMMAND);
    D_PRINTF( "Writing to server: %s\n", getcommand );
    writelen = strlen(getcommand);
    status = NETWRITE(sock, getcommand, writelen);
    if(status != writelen)
    {
        returnerr("Error sending command line to server: %s\n",
                  neterrstr());
        goto error;
    }
    /*
     * WE HAVE NOW SENT THE REQUEST SUCCESSFULLY.
     * WAIT FOR THE REPLY AND FIND THE HEADER
     */

    if(GETTIMEOFDAY(&timer->beforeheader, &timer->beforeheaderzone) != 0)
    {
        returnerr("Error retrieving beforeheader\n");
        goto error;
    }

    /* read the header and part of the file */
    totalbytesread = 0;
    headerlen = 0;
    while (totalbytesread < HEADERBUFSIZ)
    {
        bytesread = NETREAD(sock, headerbuffer+totalbytesread, HEADERBUFSIZ-totalbytesread);
        if (BADSOCKET(bytesread))
        {
            D_PRINTF( "Did not receive full header\n" );
            D_PRINTF( "NETREAD returned %d\n", bytesread );
            returnerr("Did not receive full header: %s\n",
                      neterrstr());
            goto error;
        }
        totalbytesread += bytesread;

        /* search for end of header */
        headerbuffer[totalbytesread] = 0;
        if (offset = strstr(headerbuffer, "\n\n")) {
            headerlen = offset - headerbuffer + 2;
            break;
        } else if (offset = strstr(headerbuffer, "\n\r\n")) {
            headerlen = offset - headerbuffer + 3;
            break;
        }
    }

    if (headerlen == 0) {
        returnerr("Can't find the end of the header in \"%s\"\n", headerbuffer);
        goto error;
    }

    /* get and check status code from the first line of the header */
    count = sscanf(headerbuffer, "HTTP/%s %d", version, &status);
    if (count != 2) {
        returnerr("Bad status line in get(): %s\n", headerbuffer);
        goto error;
    }
    if (status < 200 || status > 300) {
        returnerr("Bad status (%d) in get() for url %s\n", status, url);
        goto error;
    }

    /* get the content length line from the header */
    offset = headerbuffer;
    while (offset < headerbuffer+headerlen && *offset) {
        if (*offset++ != '\n')
            continue;

        if (mystrincmp(offset, CONTENT_LENGTH_STRING, strlen( CONTENT_LENGTH_STRING)) == 0) {
            sscanf(offset+strlen(CONTENT_LENGTH_STRING), "%d", &contentlength);
            D_PRINTF( "Content-Length: %d\n", contentlength );
        }
    }

    if(GETTIMEOFDAY(&timer->afterheader, &timer->afterheaderzone) != 0)
    {
        returnerr("Error retrieving afterheader\n");
        goto error;
    }

    if(savefile)
    {
        sprintf(outputfilename,"/tmp/webstone.data.%d", (int)getpid());
        if((outputfile = open(outputfilename,(O_WRONLY|O_CREAT),0777)) < 0)
        {
            D_PRINTF( "outputfile %d %d\n", outputfile, errno );
            returnerr("Error saving file: %s\n", strerror(errno));
            goto error;
        }
        lseek(outputfile,1,SEEK_END); /* this is odd... JEF */

        /* if we have part of the file already, save that part */
        if(totalbytesread > headerlen)
        {
            write(outputfile, headerbuffer+headerlen, totalbytesread-headerlen);
        }
    }

    /* read the body of the file */
    do
    {
        bytesread = NETREAD(sock, headerbuffer, HEADERBUFSIZ);
        D_PRINTF( "Read %d bytes from socket %d\n", bytesread, sock );

        if (BADSOCKET(bytesread))
        {
            D_PRINTF("Read returns %d, error: %s\n", bytesread,
                     neterrstr() );
            returnerr("Error during read of page body. Read "
                      "returns %d on socket %d, error: %s\n",
                      bytesread, sock, neterrstr());
            goto error;
        }

        totalbytesread += bytesread;

        if (outputfile != -1 && bytesread)
        {
            write(outputfile, headerbuffer, bytesread);
        }
    } while (bytesread);

    /* done reading body */
    if ( contentlength && (totalbytesread - headerlen) != contentlength)
    {
        D_PRINTF( "Warning: file length (%d) doesn't match Content-length (%d)\n",
        totalbytesread - headerlen, contentlength);
    }

    bodylength = totalbytesread - headerlen;

    if(GETTIMEOFDAY(&timer->afterbody, &timer->afterbodyzone) != 0)
    {
        returnerr("Error retrieving afterbody\n");
        goto error;
    }

    NETCLOSE(sock);
    if (outputfile != -1)
    {
        close(outputfile);
    }

    D_PRINTF("Read %d bytes, %d of that being body\n",
             totalbytesread, bodylength );

    if(GETTIMEOFDAY(&timer->exittime, &timer->exittimezone) != 0)
    {
        D_PRINTF( "Error retrieving exit time: %s\n", strerror(errno) );
        returnerr("Error retrieving exit time\n");
        goto error;
    }
    timer->valid = 2;
    timer->totalbytes = totalbytesread;
    timer->bodybytes = bodylength;

    D_PRINTF("get returning totalbytes %d body %d valid %d\n",
             timer->totalbytes, timer->bodybytes, timer->valid );

    D_PRINTF("get returning start %d, end %d\n",
             timer->entertime.tv_sec, timer->exittime.tv_sec );

    D_PRINTF("get returning connect %d, request %d, header %d, body %d\n",
             timer->afterconnect.tv_sec, timer->beforeheader.tv_sec,
             timer->afterheader.tv_sec, timer->afterbody.tv_sec );

    return 0;

error:
    if (!BADSOCKET(sock))
        NETCLOSE(sock);
    if (outputfile != -1)
        close(outputfile);
    GETTIMEOFDAY(&timer->exittime, &timer->exittimezone);   /* needed? */
    return -1;
}
