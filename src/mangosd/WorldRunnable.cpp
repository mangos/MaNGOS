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

/** \file
    \ingroup mangosd
*/

#include "WorldSocketMgr.h"
#include "Common.h"
#include "World.h"
#include "WorldRunnable.h"
#include "Timer.h"
#include "MapManager.h"
#include "BattleGroundMgr.h"

#include "Database/DatabaseEnv.h"

#define WORLD_SLEEP_CONST 50

#ifdef WIN32
#include "ServiceWin32.h"
extern int m_ServiceStatus;
#endif

/// Heartbeat for the World
void WorldRunnable::run()
{
    ///- Init new SQL thread for the world database
    WorldDatabase.ThreadStart();                                // let thread do safe mySQL requests (one connection call enough)
    sWorld.InitResultQueue();

    uint32 realCurrTime = 0;
    uint32 realPrevTime = WorldTimer::tick();

    uint32 prevSleepTime = 0;                               // used for balanced full tick time length near WORLD_SLEEP_CONST

    ///- While we have not World::m_stopEvent, update the world
    while (!World::IsStopped())
    {
        ++World::m_worldLoopCounter;
        realCurrTime = WorldTimer::getMSTime();

        uint32 diff = WorldTimer::tick();

        sWorld.Update( diff );
        realPrevTime = realCurrTime;

        // diff (D0) include time of previous sleep (d0) + tick time (t0)
        // we want that next d1 + t1 == WORLD_SLEEP_CONST
        // we can't know next t1 and then can use (t0 + d1) == WORLD_SLEEP_CONST requirement
        // d1 = WORLD_SLEEP_CONST - t0 = WORLD_SLEEP_CONST - (D0 - d0) = WORLD_SLEEP_CONST + d0 - D0
        if (diff <= WORLD_SLEEP_CONST+prevSleepTime)
        {
            prevSleepTime = WORLD_SLEEP_CONST+prevSleepTime-diff;
            ACE_Based::Thread::Sleep(prevSleepTime);
        }
        else
            prevSleepTime = 0;

        #ifdef WIN32
            if (m_ServiceStatus == 0) World::StopNow(SHUTDOWN_EXIT_CODE);
            while (m_ServiceStatus == 2) Sleep(1000);
        #endif
    }

    sWorld.KickAll();                                       // save and kick all players
    sWorld.UpdateSessions( 1 );                             // real players unload required UpdateSessions call

    // unload battleground templates before different singletons destroyed
    sBattleGroundMgr.DeleteAllBattleGrounds();

    sWorldSocketMgr->StopNetwork();

    MapManager::Instance().UnloadAll();                     // unload all grids (including locked in memory)

    ///- End the database thread
    WorldDatabase.ThreadEnd();                                  // free mySQL thread resources
}
