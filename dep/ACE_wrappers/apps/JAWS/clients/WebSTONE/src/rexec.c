/* $Id: rexec.c 91813 2010-09-17 07:52:52Z johnnyw $ */
/*
 * Copyright (c) 1994-1995 Ataman Software, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Ataman Software, Inc.
 * 4. The name of Ataman Software, Inc. may not may be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATAMAN SOFTWARE, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ATAMAN SOFTWARE, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


static char copyright[] =
    "Copyright (c) 1994-1995 Ataman Software, Inc.  All rights reserved.";


#pragma warning(disable: 4699)
/* Includes for Win32 systems go here. */
#define STRICT
#pragma warning(disable: 4201)
#include <windows.h>
#pragma warning(default: 4201)
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <signal.h>
#include "sysdep.h"
#include "bench.h"

#define RETVAL DWORD
#define IDENT HANDLE
#define STDINPUT hStdIn
#define STDOUTPUT hStdOut
#define STDERROR hStdErr
#define FILECOOKIE HANDLE

static void PassOutputThread(SOCKET);
static void PassErrorThread(SOCKET);

HANDLE hStdIn, hStdOut, hStdErr;



/*
Think carefully before enabling the -p option.  While it may be
convenient to have this option, it is for many (if not most) sites a
security hole.  Remember that the '-p password' used on the command
line is visible on most Unix systems to any user that is allow to run
the 'ps' command (normally ALL users can run this command).  While no
utility that comes by default with Windows NT at this time shows the
same information, it is unclear whether or not the information is
avaiable to all users.  Certainly privileged users would be be able to
see this information on any system.

If the security risk is acceptable at your site, you can enable the -p
option by uncommenting the #define below.
*/
/* #define ALLOWDASH_P */

static void MyOpenService(const char *remote_host);
static BOOL Send(SOCKET, const char *, size_t);
static BOOL SendZString(const char *);
static BOOL GetErrString(char *, size_t);

static IDENT PassInput(void);
static IDENT PassOutput(void);
static IDENT PassError(void);
static BOOL Close(FILECOOKIE);
static int Read(FILECOOKIE, char *, size_t);
static BOOL Write(FILECOOKIE, const char *, size_t);
static void Wait(IDENT, RETVAL *);

static SOCKET sIO = INVALID_SOCKET;
static SOCKET sErr = INVALID_SOCKET;

IDENT idIn = 0;
IDENT idOut, idErr;

SOCKET rexec(const char **hostname, NETPORT port, char *username, char *password,
             char *command, SOCKET *sockerr)
{

    MyOpenService(*hostname);

    SendZString(username);
    SendZString(password);
    SendZString(command);

    if (!GetErrString(command, sizeof command)) {
        errexit("Rexec: Remote aborted connection without initiating protocol: %s.\n",
                neterrstr());
    }

    if (*command != '\0') {
        char *p = command;
        if (*p == '\001') {
            p++;
        }
        errexit("Rexec: Remote aborted connection: %s\n", p);
    }

    hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hStdErr = GetStdHandle(STD_ERROR_HANDLE);

    *sockerr = sErr;
    return sIO;
}


static void MyOpenService(const char *remote_host)
{
    struct sockaddr_in server_addr, my_err_addr, junk_addr;
    struct servent *sv;
    static char portbuf[30];
    SOCKET sTmp;
    int addr_len;

    if (remote_host[0] >= '0' && remote_host[0] <= '9') {
        unsigned long addr;

        addr = inet_addr(remote_host);
        if (addr == INADDR_NONE) {
            returnerr("Invalid IP address %s\n", remote_host);
            return;
        }

        server_addr.sin_addr.S_un.S_addr = addr;
    }
    else {
        struct hostent *hent;

        hent = gethostbyname(remote_host);
        if (hent == 0)
        {
            D_PRINTF( "Can't get %s host entry\n", remote_host );
            D_PRINTF( "Gethostbyname failed: %d", WSAGetLastError() );
            errexit("Rexec: gethostbyname(%s) failed: %s\n",
                    remote_host, neterrstr());
        }
    memcpy((char *)&server_addr.sin_addr, hent->h_addr, hent->h_length);
}

#ifdef OMIT
    hent = gethostbyname(remote_host);
    if(!hent) {
        errexit("Rexec: Lookup of server hostname failed: %s.\n",
                neterrstr());
    }
#endif /* OMIT */

    sv=getservbyname("exec", "tcp");
    if (!sv) {
        errexit("Rexec: Lookup of port number for rexec service failed: %s.\n",
                neterrstr());
    }

    server_addr.sin_family = PF_INET;
    server_addr.sin_port = htons(sv->s_port);

    if((sIO=socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        errexit("Rexec: I/O socket creation failed: %s.\n",
                neterrstr());
    }

    if(connect(sIO,
               (struct sockaddr *)&server_addr, sizeof server_addr) == SOCKET_ERROR) {
        errexit("Rexec: I/O socket connection failed: %s.\n",
                neterrstr());
    }

    memset(&my_err_addr, '\0', sizeof my_err_addr);
    my_err_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_err_addr.sin_family = AF_INET;
    my_err_addr.sin_port = 0;

    if ((sTmp=socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        errexit("Rexec: Error socket creation failed: error=%s.\n",
                neterrstr());
    }

    if (bind(sTmp,
             (struct sockaddr *)&my_err_addr, sizeof my_err_addr) == SOCKET_ERROR) {
        errexit("Rexec: Error socket bind failed: %s.\n",
                neterrstr());
        (void) closesocket(sTmp);
    }

    if (listen(sTmp, 1) == SOCKET_ERROR) {
        errexit("Rexec: Error socket listen failed: %s.\n",
                neterrstr());
        (void) closesocket(sTmp);
    }

    addr_len = sizeof my_err_addr;
    if (getsockname(sTmp,
                    (struct sockaddr *)&my_err_addr, &addr_len) == SOCKET_ERROR) {
        errexit("Rexec: Error socket bind failed: %s.\n",
                neterrstr());
        (void) closesocket(sTmp);
    }

    sprintf(portbuf, "%hu", ntohs(my_err_addr.sin_port));
    SendZString(portbuf);

    addr_len = sizeof junk_addr;
    if ((sErr = accept(sTmp, (struct sockaddr *)&junk_addr, &addr_len)) == INVALID_SOCKET) {
        errexit("Rexec: Error socket accept failed: %s.\n",
                neterrstr());
        (void) closesocket(sTmp);
    }

    (void) closesocket(sTmp);
}

static BOOL Send(SOCKET s, const char *buf, size_t nbuf)
{
    int cnt;
    size_t sent = 0;

    while (sent < nbuf) {
        cnt = send(s, &buf[sent], nbuf-sent, 0);
        if (cnt == -1) {
            return FALSE;
        }
        sent += cnt;
    }
    return TRUE;
}


static BOOL SendZString(const char *str)
{
    return Send(sIO, str, strlen(str)+1);
}


static BOOL GetErrString(char *str, size_t len)
{
    size_t pos = 0;

    while (pos < len) {
        char ch;
        if (recv(sIO, &ch, 1, 0) != 1) {
            return FALSE;
        }
        str[pos++] = ch;
        if (ch == '\0') {
            return TRUE;
        }
        if (ch == '\n') {
            return TRUE;
        }
    }
    return FALSE;
}


static IDENT PassOutput()
{
    IDENT id;
    id = (IDENT)_beginthread(PassOutputThread, 4096, (void *)sIO);
    if ((long)id == -1) {
        errexit("Rexec: Could not start output passing thread: error = %lu\n",
                GetLastError());
    }
    return id;
}

static void PassOutputThread(SOCKET sIO)
{
    RETVAL retval = 0;
    int count;
    char buf[4096];

    while ((count=recv(sIO, buf, sizeof buf, 0)) > 0) {
        if (!Write(STDOUTPUT, buf, count)) {
            fprintf(stderr,
                    "Error writing to standard output: error = %lu.\n",
                    GetLastError());
            retval = 1;
            break;
        }
    }

    _endthread();
}


static IDENT PassError()
{
    IDENT id;
    id = (IDENT)_beginthread(PassErrorThread, 4096, (void *)sErr);
    if ((long)id == -1) {
        errexit("Rexec: Could not start error passing thread: error = %lu\n",
                GetLastError());
    }
    return id;
}

static void PassErrorThread(SOCKET sErr)
{
    RETVAL retval = 0;
    int count;
    char buf[4096];

    while ((count=recv(sErr, buf, sizeof buf, 0)) > 0) {
        if (!Write(STDERROR, buf, count)) {
            fprintf(stderr,
                    "Error writing to standard error: error = %lu.\n",
            GetLastError());
            retval = 1;
            break;
        }
    }
    _endthread();
}

static BOOL Close(FILECOOKIE fc)
{
    return CloseHandle(fc);
}

static int Read(FILECOOKIE fc, char *buf, size_t nbuf)
{
    DWORD cbRead;
    if (!ReadFile(fc, buf, nbuf, &cbRead, 0)) {
        return -1;
    }
    return (int)cbRead;
}


static BOOL Write(FILECOOKIE fc, const char *buf, size_t nbuf)
{
    DWORD cbWritten;

    if (!WriteFile(fc, buf, nbuf, &cbWritten, 0)) {
        return FALSE;
    }
    if (cbWritten != nbuf) {
        return FALSE;
    }
    return TRUE;
}


static void
Wait(IDENT id, RETVAL *prv)
{
    if (!WaitForSingleObject(id, INFINITE)) {
        *prv = 2;
    } else {
        if (!GetExitCodeThread(id, prv)) {
            *prv = 4;
        }
    }
}
