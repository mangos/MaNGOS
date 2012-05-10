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

/**
 * @addtogroup npc_linking System to link groups of NPCs together
 * This NPC-linking system in MaNGOS consists of 2 files:
 * - CreatureLinkingMgr.h
 * - CreatureLinkingMgr.cpp
 * as well of
 * - hooks in Creature.cpp, to trigger actions
 * - holder of the linked npcs for every map
 *
 * @{
 *
 * @file CreatureLinkingMgr.h
 * This file contains the the headers needed for MaNGOS to link NPCs together
 *
 */

#ifndef CREATURE_LINKING_MGR_H
#define CREATURE_LINKING_MGR_H

#include "Common.h"
#include "Policies/Singleton.h"
#include "ObjectGuid.h"
#include <functional>

class Unit;
class Creature;

// enum on which Events an action for linked NPCs can trigger
enum CreatureLinkingEvent
{
    LINKING_EVENT_AGGRO         = 0,
    LINKING_EVENT_EVADE         = 1,
    LINKING_EVENT_DIE           = 2,
    LINKING_EVENT_RESPAWN       = 3,
};

// enum describing possible flags action flags for NPCs linked to other NPCs
// These flags are actually put into the database
// FLAG_TO_ means, that in this case the linked NPC will also trigger an action for the NPC it is linked to
enum CreatureLinkingFlags
{
    // Dynamic behaviour, in combat
    FLAG_AGGRO_ON_AGGRO             = 0x0001,
    FLAG_TO_AGGRO_ON_AGGRO          = 0x0002,
    FLAG_RESPAWN_ON_EVADE           = 0x0004,
    FLAG_TO_RESPAWN_ON_EVADE        = 0x0008,
    FLAG_DESPAWN_ON_EVADE           = 0x1000,
    FLAG_DESPAWN_ON_DEATH           = 0x0010,
    FLAG_SELFKILL_ON_DEATH          = 0x0020,
    FLAG_RESPAWN_ON_DEATH           = 0x0040,
    FLAG_RESPAWN_ON_RESPAWN         = 0x0080,
    FLAG_DESPAWN_ON_RESPAWN         = 0x0100,

    // Dynamic behaviour, out of combat
    FLAG_FOLLOW                     = 0x0200,

    // Passive behaviour
    FLAG_CANT_SPAWN_IF_BOSS_DEAD    = 0x0400,
    FLAG_CANT_SPAWN_IF_BOSS_ALIVE   = 0x0800,

    LINKING_FLAG_INVALID            = 0x2000,               // TODO adjust when other flags are implemented
};

// Structure holding the information for an entry
struct CreatureLinkingInfo
{
    uint32 mapId;
    uint32 masterId;
    uint32 masterDBGuid;
    uint16 linkingFlag:16;
    uint16 searchRange:16;
};

/**
 * A class to represent the static information of linking NPCs together
 */

class CreatureLinkingMgr
{
    public:                                                 // Constructors
        CreatureLinkingMgr() {}

    public:                                                 // Initialisation
        void LoadFromDB();

    public:                                                 // Accessors
        // This functions checks if the NPC triggers actions for other NPCs
        bool IsLinkedEventTrigger(Creature* pCreature);

        // This function checks if the NPC is a master NPC.
        bool IsLinkedMaster(Creature* pCreature);

        // This function checks if the spawning of this NPC is dependend on other NPCs
        bool IsSpawnedByLinkedMob(Creature* pCreature);

        // This gives the information of a linked NPC (describes action when its ActionTrigger triggers)
        // Depends of the map
        CreatureLinkingInfo const* GetLinkedTriggerInformation(Creature* pCreature);

    private:
        typedef std::multimap<uint32 /*slaveEntry*/, CreatureLinkingInfo> CreatureLinkingMap;
        typedef std::pair<CreatureLinkingMap::const_iterator, CreatureLinkingMap::const_iterator> CreatureLinkingMapBounds;

        // Storage of Data: npc_entry_slave, (map, npc_entry_master, flag, master_db_guid[If Unique], search_range)
        CreatureLinkingMap m_creatureLinkingMap;
        // Storage of Data: npc_guid_slave, (map, npc_guid_master, flag, master_db_guid, search_range)
        CreatureLinkingMap m_creatureLinkingGuidMap;

        // Lookup Storage for fast access:
        UNORDERED_SET<uint32> m_eventTriggers;              // master by entry
        UNORDERED_SET<uint32> m_eventGuidTriggers;          // master by guid

        // Check-routine
        bool IsLinkingEntryValid(uint32 slaveEntry, CreatureLinkingInfo* pInfo, bool byEntry);
};

/**
 * A class to represent the dynamic information of linking NPCs together
 *
 * Every map has an instance of this class as member, in which the dynamic information (GUIDs) are stored
 */

class CreatureLinkingHolder
{
    public:                                                 // Constructors
        CreatureLinkingHolder() {}

    public:                                                 // Accessors
        // Function to add slave-NPCs to the holder
        void AddSlaveToHolder(Creature* pCreature);

        // Function to add master-NPCs to the holder
        void AddMasterToHolder(Creature* pCreature);

        // Function to process actions for linked NPCs
        void DoCreatureLinkingEvent(CreatureLinkingEvent eventType, Creature* pSource, Unit* pEnemy = NULL);

        // Function to check if a passive spawning condition is met
        bool CanSpawn(Creature* pCreature);

        // This function lets a slave refollow his master
        bool TryFollowMaster(Creature* pCreature);

    private:
        typedef std::list<ObjectGuid> GuidList;
        // Structure associated to a master (entry case)
        struct InfoAndGuids
        {
            uint16 linkingFlag:16;
            uint16 searchRange:16;
            GuidList linkedGuids;
        };
        // Structure associated to a master (guid case)
        struct InfoAndGuid
        {
            uint16 linkingFlag;
            ObjectGuid linkedGuid;
        };

        typedef std::multimap<uint32 /*masterEntryOrGuid*/, InfoAndGuids> HolderMap;
        typedef std::pair<HolderMap::iterator, HolderMap::iterator> HolderMapBounds;
        typedef std::multimap<uint32 /*Entry*/, ObjectGuid> BossGuidMap;
        typedef std::pair<BossGuidMap::iterator, BossGuidMap::iterator> BossGuidMapBounds;

        // Helper function, to process a slave list
        void ProcessSlaveGuidList(CreatureLinkingEvent eventType, Creature* pSource, uint32 flag, uint16 searchRange, GuidList& slaveGuidList, Unit* pEnemy);
        // Helper function, to process a single slave
        void ProcessSlave(CreatureLinkingEvent eventType, Creature* pSource, uint32 flag, Creature* pSlave, Unit* pEnemy);
        // Helper function to set following
        void SetFollowing(Creature* pWho, Creature* pWhom);
        // Helper function to return if a slave is in range of a boss
        bool IsSlaveInRangeOfBoss(Creature* pSlave, Creature* pBoss, uint16 searchRange);

        // Storage of Data (boss, flag, searchRange, GuidList) for action triggering
        HolderMap m_holderMap;
        // Storage of Data (boss, flag, slave-guid)
        HolderMap m_holderGuidMap;
        // boss_entry, guid for reverse action triggering and check alive
        BossGuidMap m_masterGuid;
};

#define sCreatureLinkingMgr MaNGOS::Singleton<CreatureLinkingMgr>::Instance()

#endif
/*! @} */
