/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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

#include "MapUpdater.h"
#include "DelayExecutor.h"
#include "Map.h"
#include "Database/DatabaseEnv.h"

#include <ace/Guard_T.h>
#include <ace/Method_Request.h>

class WDBThreadStartReq1 : public ACE_Method_Request
{
    public:

        WDBThreadStartReq1()
        {
        }

        virtual int call()
        {
            WorldDatabase.ThreadStart();
            return 0;
        }
};

class WDBThreadEndReq1 : public ACE_Method_Request
{
    public:

        WDBThreadEndReq1()
        {
        }

        virtual int call()
        {
            WorldDatabase.ThreadEnd();
            return 0;
        }
};

class MapUpdateRequest : public ACE_Method_Request
{
    private:

        Map& m_map;
        MapUpdater& m_updater;
        ACE_UINT32 m_diff;

    public:

        MapUpdateRequest(Map& m, MapUpdater& u, ACE_UINT32 d)
            : m_map(m), m_updater(u), m_diff(d)
        {
        }

        virtual int call()
        {
            m_map.Update (m_diff);
            m_updater.update_finished ();
            return 0;
        }
};

MapUpdater::MapUpdater()
    : m_mutex(), m_condition(m_mutex), m_executor(), pending_requests(0)
{
}

MapUpdater::~MapUpdater()
{
    deactivate();
}

int MapUpdater::activate(size_t num_threads)
{
    return m_executor.activate((int)num_threads, new WDBThreadStartReq1, new WDBThreadEndReq1);
}

int MapUpdater::deactivate()
{
    wait();

    return m_executor.deactivate();
}

int MapUpdater::wait()
{
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_mutex, -1);

    while (pending_requests > 0)
        m_condition.wait();

    return 0;
}

int MapUpdater::schedule_update(Map& map, ACE_UINT32 diff)
{
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_mutex, -1);

    ++pending_requests;

    if (m_executor.execute(new MapUpdateRequest(map, *this, diff)) == -1)
    {
        ACE_DEBUG((LM_ERROR, ACE_TEXT("(%t) \n"), ACE_TEXT("Failed to schedule Map Update")));

        --pending_requests;
        return -1;
    }

    return 0;
}

bool MapUpdater::activated()
{
    return m_executor.activated();
}

void MapUpdater::update_finished()
{
    ACE_GUARD(ACE_Thread_Mutex, guard, m_mutex);

    if (pending_requests == 0)
    {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%t)\n"), ACE_TEXT("MapUpdater::update_finished BUG, report to devs")));
        return;
    }

    --pending_requests;

    m_condition.broadcast();
}
