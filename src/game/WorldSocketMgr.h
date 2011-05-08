/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
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

/** \addtogroup u2w User to World Communication
 *  @{
 *  \file WorldSocketMgr.h
 *  \author Derex <derex101@gmail.com>
 */

#ifndef __WORLDSOCKETMGR_H
#define __WORLDSOCKETMGR_H

#include <ace/Basic_Types.h>
#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>

#include <string>

class WorldSocket;
class ReactorRunnable;
class ACE_Event_Handler;

/// Manages all sockets connected to peers and network threads
class WorldSocketMgr
{
    public:
        friend class WorldSocket;
        friend class ACE_Singleton<WorldSocketMgr, ACE_Thread_Mutex>;

        /// Start network, listen at address:port .
        int StartNetwork(ACE_UINT16 port, std::string& address);

        /// Stops all network threads, It will wait for all running threads .
        void StopNetwork();

        /// Wait untill all network threads have "joined" .
        void Wait();

        std::string& GetBindAddress() { return m_addr; }
        ACE_UINT16 GetBindPort() { return m_port; }

        /// Make this class singleton .
        static WorldSocketMgr* Instance();

    private:
        int OnSocketOpen(WorldSocket* sock);
        int StartReactiveIO(ACE_UINT16 port, const char* address);

        WorldSocketMgr();
        virtual ~WorldSocketMgr();

        ReactorRunnable* m_NetThreads;
        size_t m_NetThreadsCount;

        int m_SockOutKBuff;
        int m_SockOutUBuff;
        bool m_UseNoDelay;

        std::string m_addr;
        ACE_UINT16 m_port;

        ACE_Event_Handler* m_Acceptor;
};

#define sWorldSocketMgr WorldSocketMgr::Instance()

#endif
/// @}
