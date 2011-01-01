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

#ifndef MANGOS_GAMEEVENT_MGR_H
#define MANGOS_GAMEEVENT_MGR_H

#include "Common.h"
#include "SharedDefines.h"
#include "Platform/Define.h"
#include "Policies/Singleton.h"

#define max_ge_check_delay 86400                            // 1 day in seconds

class Creature;
class GameObject;

struct GameEventData
{
    GameEventData() : start(1),end(0),occurence(0),length(0), holiday_id(HOLIDAY_NONE) {}
    time_t start;
    time_t end;
    uint32 occurence;                                       // Delay in minutes between occurences of the event
    uint32 length;                                          // Length in minutes of the event
    HolidayIds holiday_id;
    std::string description;

    bool isValid() const { return length > 0; }
};

struct GameEventCreatureData
{
    uint32 entry_id;
    uint32 modelid;
    uint32 equipment_id;
    uint32 spell_id_start;
    uint32 spell_id_end;
};

typedef std::pair<uint32, GameEventCreatureData> GameEventCreatureDataPair;

class GameEventMgr
{
    public:
        GameEventMgr();
        ~GameEventMgr() {};
        typedef std::set<uint16> ActiveEvents;
        typedef std::vector<GameEventData> GameEventDataMap;
        ActiveEvents const& GetActiveEventList() const { return m_ActiveEvents; }
        GameEventDataMap const& GetEventMap() const { return mGameEvent; }
        bool CheckOneGameEvent(uint16 entry) const;
        uint32 NextCheck(uint16 entry) const;
        void LoadFromDB();
        uint32 Update();
        bool IsActiveEvent(uint16 event_id) const { return ( m_ActiveEvents.find(event_id)!=m_ActiveEvents.end()); }
        uint32 Initialize();
        void StartEvent(uint16 event_id, bool overwrite = false);
        void StopEvent(uint16 event_id, bool overwrite = false);
        template<typename T>
        int16 GetGameEventId(uint32 guid_or_poolid);

        GameEventCreatureData const* GetCreatureUpdateDataForActiveEvent(uint32 lowguid) const;
    private:
        void AddActiveEvent(uint16 event_id) { m_ActiveEvents.insert(event_id); }
        void RemoveActiveEvent(uint16 event_id) { m_ActiveEvents.erase(event_id); }
        void ApplyNewEvent(uint16 event_id);
        void UnApplyEvent(uint16 event_id);
        void GameEventSpawn(int16 event_id);
        void GameEventUnspawn(int16 event_id);
        void UpdateCreatureData(int16 event_id, bool activate);
        void UpdateEventQuests(uint16 event_id, bool Activate);
        void UpdateWorldStates(uint16 event_id, bool Activate);
    protected:
        typedef std::list<uint32> GuidList;
        typedef std::list<uint16> IdList;
        typedef std::vector<GuidList> GameEventGuidMap;
        typedef std::vector<IdList> GameEventIdMap;
        typedef std::list<GameEventCreatureDataPair> GameEventCreatureDataList;
        typedef std::vector<GameEventCreatureDataList> GameEventCreatureDataMap;
        typedef std::multimap<uint32, uint32> GameEventCreatureDataPerGuidMap;
        typedef std::pair<GameEventCreatureDataPerGuidMap::const_iterator,GameEventCreatureDataPerGuidMap::const_iterator> GameEventCreatureDataPerGuidBounds;

        typedef std::list<uint32> QuestList;
        typedef std::vector<QuestList> GameEventQuestMap;
        GameEventQuestMap mGameEventQuests;                 // events*2-1

        GameEventCreatureDataMap mGameEventCreatureData;    // events*2-1
        GameEventCreatureDataPerGuidMap mGameEventCreatureDataPerGuid;

        GameEventGuidMap  mGameEventCreatureGuids;          // events*2-1
        GameEventGuidMap  mGameEventGameobjectGuids;        // events*2-1
        GameEventIdMap    mGameEventSpawnPoolIds;           // events size, only positive event case
        GameEventDataMap  mGameEvent;
        ActiveEvents m_ActiveEvents;
        bool m_IsGameEventsInit;
};

#define sGameEventMgr MaNGOS::Singleton<GameEventMgr>::Instance()

MANGOS_DLL_SPEC bool IsHolidayActive(HolidayIds id);

#endif
