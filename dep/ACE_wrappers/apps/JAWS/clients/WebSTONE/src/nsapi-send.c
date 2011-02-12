/* $Id: nsapi-send.c 81993 2008-06-16 20:26:16Z sowayaa $ */
/*
 * Send random bits file
 * Once this service function is installed, any file with the extension
 * "dyn-send" will be serviced with this function.  An optional query
 * string may be passed to alter the amount of data in the response.
 *
 * For example:
 *      /file.dyn-send                  - returns a 10240 byte file
 *      /file.dyn-send?size=20          - returns a 20 byte file
 *      /file.dyn-send?size=1024        - returns a 1024 byte file
 *      etc.
 *
 * To install the service routine, compile it as per the makefile
 * included with your Netscape server distribution (serverroot/nsapi/examples)
 * and then add the following lines to your netscape server configuration:
 *
 * in magnus.conf
 *      Init fn=load-modules shlib=example.so funcs=nsapi-send
 *
 * in obj.conf
 *      Service method=(GET|HEAD) fn=nsapi-send type=magnus-internal/dyn-send
 *
 * in mime.types
 *      type=magnus-internal/dyn-send        exts=dyn-send
 *
 * Mike Belshe
 * mbelshe@netscape.com
 * 11-5-95
 *
 */

#ifndef WIN32
#include <stdio.h>
#include <stdlib.h>
#include "base/pblock.h"
#include "base/session.h"
#include "frame/protocol.h"
#include "base/util.h"
#include "frame/http.h"
#else
#include <windows.h>
#define FILE_STDIO 1
#endif
#include "frame/req.h"

#define FILE_SIZE       10240
#define HEADERS  "HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n"

#ifdef WIN32
__declspec(dllexport)
#endif

int nsapi_send(pblock *pb, Session *sn, Request *rq)
{
    char *query_string;
    char buffer[sizeof(HEADERS) + 204800 + 1];
    int filesize;
    unsigned int maxindex;
    unsigned int index;

    /* Get the query string, if any; check to see if an alternate
     * file size was specified.
     */
    if ( !(query_string = pblock_findval("query", rq->reqpb)) )
        filesize = FILE_SIZE;
    else {
        filesize = atoi(&(query_string[5]));
    }

    memcpy(&buffer, HEADERS, sizeof(HEADERS)-1);

    /* Generate the output */
    maxindex = sizeof(HEADERS) + filesize;
    for (index=sizeof(HEADERS); index < (maxindex); index++)
        /* generate random characters from A-Z */
#ifdef IRIX
        buffer[index] = rand_r() % 26 + 63;
#else
        buffer[index] = rand() %26 + 63;
#endif

    /* Send the output */
    if (net_write(sn->csd, buffer, sizeof(HEADERS)-1+filesize, 0) == IO_ERROR)
        return REQ_EXIT;

    return REQ_PROCEED;
}
