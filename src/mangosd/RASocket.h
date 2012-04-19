/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/// \addtogroup mangosd
/// @{
/// \file

#ifndef _RASOCKET_H
#define _RASOCKET_H

#include "Common.h"
#include <ace/Synch_Traits.h>
#include <ace/Svc_Handler.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/Acceptor.h>
#include <ace/Thread_Mutex.h>
#include <ace/Semaphore.h>

#define RA_BUFF_SIZE 8192


/// Remote Administration socket
typedef ACE_Svc_Handler < ACE_SOCK_STREAM, ACE_NULL_SYNCH> RAHandler;
class RASocket: protected RAHandler
{
    public:
        ACE_Semaphore pendingCommands;
        typedef ACE_Acceptor<RASocket, ACE_SOCK_ACCEPTOR > Acceptor;
        friend class ACE_Acceptor<RASocket, ACE_SOCK_ACCEPTOR >;

        int sendf(const char*);

    protected:
        /// things called by ACE framework.
        RASocket(void);
        virtual ~RASocket(void);

        /// Called on open ,the void* is the acceptor.
        virtual int open (void *);

        /// Called on failures inside of the acceptor, don't call from your code.
        virtual int close (int);

        /// Called when we can read from the socket.
        virtual int handle_input (ACE_HANDLE = ACE_INVALID_HANDLE);

        /// Called when the socket can write.
        virtual int handle_output (ACE_HANDLE = ACE_INVALID_HANDLE);

        /// Called when connection is closed or error happens.
        virtual int handle_close (ACE_HANDLE = ACE_INVALID_HANDLE,
            ACE_Reactor_Mask = ACE_Event_Handler::ALL_EVENTS_MASK);

    private:
        bool outActive;

        char inputBuffer[RA_BUFF_SIZE];
        uint32 inputBufferLen;

        ACE_Thread_Mutex outBufferLock;
        char outputBuffer[RA_BUFF_SIZE];
        uint32 outputBufferLen;

        uint32 accId;
        AccountTypes accAccessLevel;
        bool bSecure;                                       //kick on wrong pass, non exist. user OR user with no priv
        //will protect from DOS, bruteforce attacks
        bool bStricted;                                     // not allow execute console only commands (SEC_CONSOLE) remotly
        AccountTypes iMinLevel;
        enum
        {
            NONE,                                           //initial value
            LG,                                             //only login was entered
            OK,                                             //both login and pass were given, they were correct and user has enough priv.
        }stage;

        static void zprint(void* callbackArg, const char * szText );
        static void commandFinished(void* callbackArg, bool success);
};
#endif
/// @}
