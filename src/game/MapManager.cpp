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

#include "MapManager.h"
#include "InstanceSaveMgr.h"
#include "Policies/SingletonImp.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "Transports.h"
#include "GridDefines.h"
#include "DestinationHolderImp.h"
#include "World.h"
#include "CellImpl.h"
#include "Corpse.h"
#include "ObjectMgr.h"

#define CLASS_LOCK MaNGOS::ClassLevelLockable<MapManager, ACE_Recursive_Thread_Mutex>
INSTANTIATE_SINGLETON_2(MapManager, CLASS_LOCK);
INSTANTIATE_CLASS_MUTEX(MapManager, ACE_Recursive_Thread_Mutex);

MapManager::MapManager()
    : i_gridCleanUpDelay(sWorld.getConfig(CONFIG_UINT32_INTERVAL_GRIDCLEAN))
{
    i_timer.SetInterval(sWorld.getConfig(CONFIG_UINT32_INTERVAL_MAPUPDATE));
}

MapManager::~MapManager()
{
    for(MapMapType::iterator iter=i_maps.begin(); iter != i_maps.end(); ++iter)
        delete iter->second;

    for(TransportSet::iterator i = m_Transports.begin(); i != m_Transports.end(); ++i)
        delete *i;

    DeleteStateMachine();
}

void
MapManager::Initialize()
{
    int num_threads(sWorld.getConfig(CONFIG_UINT32_NUMTHREADS));
    // Start mtmaps if needed.
    if (num_threads > 0 && m_updater.activate(num_threads) == -1)
        abort();

    InitStateMachine();
}

void MapManager::InitStateMachine()
{
    si_GridStates[GRID_STATE_INVALID] = new InvalidState;
    si_GridStates[GRID_STATE_ACTIVE] = new ActiveState;
    si_GridStates[GRID_STATE_IDLE] = new IdleState;
    si_GridStates[GRID_STATE_REMOVAL] = new RemovalState;
}

void MapManager::DeleteStateMachine()
{
    delete si_GridStates[GRID_STATE_INVALID];
    delete si_GridStates[GRID_STATE_ACTIVE];
    delete si_GridStates[GRID_STATE_IDLE];
    delete si_GridStates[GRID_STATE_REMOVAL];
}

void MapManager::UpdateGridState(grid_state_t state, Map& map, NGridType& ngrid, GridInfo& ginfo, const uint32 &x, const uint32 &y, const uint32 &t_diff)
{
    // TODO: The grid state array itself is static and therefore 100% safe, however, the data
    // the state classes in it accesses is not, since grids are shared across maps (for example
    // in instances), so some sort of locking will be necessary later.

    si_GridStates[state]->Update(map, ngrid, ginfo, x, y, t_diff);
}

void MapManager::InitializeVisibilityDistanceInfo()
{
    for(MapMapType::iterator iter=i_maps.begin(); iter != i_maps.end(); ++iter)
        (*iter).second->InitVisibilityDistance();
}

Map* MapManager::CreateMap(uint32 id, const WorldObject* obj)
{
    MANGOS_ASSERT(obj);
    //if(!obj->IsInWorld()) sLog.outError("GetMap: called for map %d with object (typeid %d, guid %d, mapid %d, instanceid %d) who is not in world!", id, obj->GetTypeId(), obj->GetGUIDLow(), obj->GetMapId(), obj->GetInstanceId());
    Guard _guard(*this);
    
    Map * m = NULL;

    const MapEntry* entry = sMapStore.LookupEntry(id);
    if(!entry)
        return NULL;

    if(entry->Instanceable())
    {
        MANGOS_ASSERT(obj->GetTypeId() == TYPEID_PLAYER);
        //create InstanceMap object
        if(obj->GetTypeId() == TYPEID_PLAYER)
            m = CreateInstance(id, (Player*)obj);
    }
    else
    {
        //create regular Continent map
        m = FindMap(id);
        if( m == NULL )
        {
            m = new Map(id, i_gridCleanUpDelay, 0, REGULAR_DIFFICULTY);
            //add map into container
            i_maps[MapID(id)] = m;
        }
    }

    return m;
}

Map* MapManager::CreateBgMap(uint32 mapid, BattleGround* bg)
{
    TerrainInfo * pData = sTerrainMgr.LoadTerrain(mapid);

    Guard _guard(*this);
    return CreateBattleGroundMap(mapid, sObjectMgr.GenerateLowGuid(HIGHGUID_INSTANCE), bg);
}

Map* MapManager::FindMap(uint32 mapid, uint32 instanceId) const
{
    Guard guard(*this);

    MapMapType::const_iterator iter = i_maps.find(MapID(mapid, instanceId));
    if(iter == i_maps.end())
        return NULL;
    
    //this is a small workaround for transports
    if(instanceId == 0 && iter->second->Instanceable())
    {
        assert(false);
        return NULL;
    }

    return iter->second;
}

/*
    checks that do not require a map to be created
    will send transfer error messages on fail
*/
bool MapManager::CanPlayerEnter(uint32 mapid, Player* player)
{
    const MapEntry *entry = sMapStore.LookupEntry(mapid);
    if(!entry)
        return false;

    const char *mapName = entry->name[player->GetSession()->GetSessionDbcLocale()];

    if(entry->IsDungeon())
    {
        if (entry->IsRaid())
        {
            // GMs can avoid raid limitations
            if(!player->isGameMaster() && !sWorld.getConfig(CONFIG_BOOL_INSTANCE_IGNORE_RAID))
            {
                // can only enter in a raid group
                Group* group = player->GetGroup();
                if (!group || !group->isRaidGroup())
                {
                    // probably there must be special opcode, because client has this string constant in GlobalStrings.lua
                    // TODO: this is not a good place to send the message
                    player->GetSession()->SendAreaTriggerMessage("You must be in a raid group to enter %s instance", mapName);
                    DEBUG_LOG("MAP: Player '%s' must be in a raid group to enter instance of '%s'", player->GetName(), mapName);
                    return false;
                }
            }
        }

        //The player has a heroic mode and tries to enter into instance which has no a heroic mode
        MapDifficulty const* mapDiff = GetMapDifficultyData(entry->MapID,player->GetDifficulty(entry->map_type == MAP_RAID));
        if (!mapDiff)
        {
            bool isRegularTargetMap = player->GetDifficulty(entry->IsRaid()) == REGULAR_DIFFICULTY;

            //Send aborted message
            // FIX ME: what about absent normal/heroic mode with specific players limit...
            player->SendTransferAborted(mapid, TRANSFER_ABORT_DIFFICULTY, isRegularTargetMap ? DUNGEON_DIFFICULTY_NORMAL : DUNGEON_DIFFICULTY_HEROIC);
            return false;
        }

        if (!player->isAlive())
        {
            if(Corpse *corpse = player->GetCorpse())
            {
                // let enter in ghost mode in instance that connected to inner instance with corpse
                uint32 instance_map = corpse->GetMapId();
                do
                {
                    if(instance_map==mapid)
                        break;

                    InstanceTemplate const* instance = ObjectMgr::GetInstanceTemplate(instance_map);
                    instance_map = instance ? instance->parent : 0;
                }
                while (instance_map);

                if (!instance_map)
                {
                    WorldPacket data(SMSG_CORPSE_IS_NOT_IN_INSTANCE);
                    player->GetSession()->SendPacket(&data);

                    DEBUG_LOG("MAP: Player '%s' doesn't has a corpse in instance '%s' and can't enter", player->GetName(), mapName);
                    return false;
                }
                DEBUG_LOG("MAP: Player '%s' has corpse in instance '%s' and can enter", player->GetName(), mapName);
            }
            else
            {
                DEBUG_LOG("Map::CanEnter - player '%s' is dead but doesn't have a corpse!", player->GetName());
            }
        }

        // TODO: move this to a map dependent location
        /*if(i_data && i_data->IsEncounterInProgress())
        {
            DEBUG_LOG("MAP: Player '%s' can't enter instance '%s' while an encounter is in progress.", player->GetName(), GetMapName());
            player->SendTransferAborted(GetId(), TRANSFER_ABORT_ZONE_IN_COMBAT);
            return(false);
        }*/
    }

    return true;
}

void MapManager::DeleteInstance(uint32 mapid, uint32 instanceId)
{
    Guard _guard(*this);

    MapMapType::iterator iter = i_maps.find(MapID(mapid, instanceId));
    if(iter != i_maps.end())
    {
        Map * pMap = iter->second;
        if (pMap->Instanceable())
        {
            i_maps.erase(iter);

            pMap->UnloadAll(true);
            delete pMap;
        }
    }
}

void
MapManager::Update(uint32 diff)
{
    i_timer.Update(diff);
    if( !i_timer.Passed())
        return;

    for (MapMapType::iterator iter=i_maps.begin(); iter != i_maps.end(); ++iter)
    {
        if (m_updater.activated())
            m_updater.schedule_update(*iter->second, (uint32)i_timer.GetCurrent());
        else
            iter->second->Update((uint32)i_timer.GetCurrent());
    }

    if (m_updater.activated())
        m_updater.wait();

    for (TransportSet::iterator iter = m_Transports.begin(); iter != m_Transports.end(); ++iter)
    {
        WorldObject::UpdateHelper helper((*iter));
        helper.Update((uint32)i_timer.GetCurrent());
    }

    //remove all maps which can be unloaded
    MapMapType::iterator iter = i_maps.begin();
    while(iter != i_maps.end())
    {
        Map * pMap = iter->second;
        //check if map can be unloaded
        if(pMap->CanUnload((uint32)i_timer.GetCurrent()))
        {
            pMap->UnloadAll(true);
            delete pMap;

            i_maps.erase(iter++);
        }
        else
            ++iter;
    }

    i_timer.SetCurrent(0);
}

void MapManager::RemoveAllObjectsInRemoveList()
{
    for(MapMapType::iterator iter=i_maps.begin(); iter != i_maps.end(); ++iter)
        iter->second->RemoveAllObjectsInRemoveList();
}

bool MapManager::ExistMapAndVMap(uint32 mapid, float x,float y)
{
    GridPair p = MaNGOS::ComputeGridPair(x,y);

    int gx=63-p.x_coord;
    int gy=63-p.y_coord;

    return GridMap::ExistMap(mapid,gx,gy) && GridMap::ExistVMap(mapid,gx,gy);
}

bool MapManager::IsValidMAP(uint32 mapid)
{
    MapEntry const* mEntry = sMapStore.LookupEntry(mapid);
    return mEntry && (!mEntry->IsDungeon() || ObjectMgr::GetInstanceTemplate(mapid));
    // TODO: add check for battleground template
}

void MapManager::UnloadAll()
{
    for(MapMapType::iterator iter=i_maps.begin(); iter != i_maps.end(); ++iter)
        iter->second->UnloadAll(true);

    while(!i_maps.empty())
    {
        delete i_maps.begin()->second;
        i_maps.erase(i_maps.begin());
    }

    TerrainManager::Instance().UnloadAll();

    if (m_updater.activated())
        m_updater.deactivate();

}

uint32 MapManager::GetNumInstances()
{
    Guard guard(*this);

    uint32 ret = 0;
    for(MapMapType::iterator itr = i_maps.begin(); itr != i_maps.end(); ++itr)
    {
        Map *map = itr->second;
        if(!map->IsDungeon()) continue;
            ret += 1;
    }
    return ret;
}

uint32 MapManager::GetNumPlayersInInstances()
{
    Guard guard(*this);

    uint32 ret = 0;
    for(MapMapType::iterator itr = i_maps.begin(); itr != i_maps.end(); ++itr)
    {
        Map *map = itr->second;
        if(!map->IsDungeon()) continue;
            ret += map->GetPlayers().getSize();
    }
    return ret;
}

///// returns a new or existing Instance
///// in case of battlegrounds it will only return an existing map, those maps are created by bg-system
Map* MapManager::CreateInstance(uint32 id, Player * player)
{
    Map* map = NULL;
    Map * pNewMap = NULL;
    uint32 NewInstanceId = 0;                                   // instanceId of the resulting map
    const MapEntry* entry = sMapStore.LookupEntry(id);

    if(entry->IsBattleGroundOrArena())
    {
        // find existing bg map for player
        NewInstanceId = player->GetBattleGroundId();
        MANGOS_ASSERT(NewInstanceId);
        map = FindMap(id, NewInstanceId);
        MANGOS_ASSERT(map);
    }
    else if (InstanceSave* pSave = player->GetBoundInstanceSaveForSelfOrGroup(id))
    {
        // solo/perm/group
        NewInstanceId = pSave->GetInstanceId();
        map = FindMap(id, NewInstanceId);
        // it is possible that the save exists but the map doesn't
        if (!map)
            pNewMap = CreateInstanceMap(id, NewInstanceId, pSave->GetDifficulty(), pSave);
    }
    else
    {
        // if no instanceId via group members or instance saves is found
        // the instance will be created for the first time
        NewInstanceId = sObjectMgr.GenerateLowGuid(HIGHGUID_INSTANCE);

        Difficulty diff = player->GetGroup() ? player->GetGroup()->GetDifficulty(entry->IsRaid()) : player->GetDifficulty(entry->IsRaid());
        pNewMap = CreateInstanceMap(id, NewInstanceId, diff);
    }

    //add a new map object into the registry
    if(pNewMap)
    {
        i_maps[MapID(id, NewInstanceId)] = pNewMap;
        map = pNewMap;
    }

    return map;
}

InstanceMap* MapManager::CreateInstanceMap(uint32 id, uint32 InstanceId, Difficulty difficulty, InstanceSave *save)
{
    // make sure we have a valid map id
    if (!sMapStore.LookupEntry(id))
    {
        sLog.outError("CreateInstanceMap: no entry for map %d", id);
        MANGOS_ASSERT(false);
    }
    if (!ObjectMgr::GetInstanceTemplate(id))
    {
        sLog.outError("CreateInstanceMap: no instance template for map %d", id);
        MANGOS_ASSERT(false);
    }

    // some instances only have one difficulty
    if (!GetMapDifficultyData(id, difficulty))
        difficulty = DUNGEON_DIFFICULTY_NORMAL;

    DEBUG_LOG("MapInstanced::CreateInstanceMap: %s map instance %d for %d created with difficulty %d", save?"":"new ", InstanceId, id, difficulty);

    InstanceMap *map = new InstanceMap(id, i_gridCleanUpDelay, InstanceId, difficulty);
    MANGOS_ASSERT(map->IsDungeon());

    bool load_data = save != NULL;
    map->CreateInstanceData(load_data);

    return map;
}

BattleGroundMap* MapManager::CreateBattleGroundMap(uint32 id, uint32 InstanceId, BattleGround* bg)
{
    DEBUG_LOG("MapInstanced::CreateBattleGroundMap: instance:%d for map:%d and bgType:%d created.", InstanceId, id, bg->GetTypeID());

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(),bg->GetMinLevel());

    uint8 spawnMode = bracketEntry ? bracketEntry->difficulty : REGULAR_DIFFICULTY;

    BattleGroundMap *map = new BattleGroundMap(id, i_gridCleanUpDelay, InstanceId, spawnMode);
    MANGOS_ASSERT(map->IsBattleGroundOrArena());
    map->SetBG(bg);
    bg->SetBgMap(map);

    //add map into map container
    i_maps[MapID(id, InstanceId)] = map;

    return map;
}

