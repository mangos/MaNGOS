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

#include "PoolManager.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "ProgressBar.h"
#include "Log.h"
#include "MapPersistentStateMgr.h"
#include "MapManager.h"
#include "World.h"
#include "Policies/SingletonImp.h"

INSTANTIATE_SINGLETON_1(PoolManager);

////////////////////////////////////////////////////////////
// template class SpawnedPoolData

// Method that tell amount spawned objects/subpools
uint32 SpawnedPoolData::GetSpawnedObjects(uint32 pool_id) const
{
    SpawnedPoolPools::const_iterator itr = mSpawnedPools.find(pool_id);
    return itr != mSpawnedPools.end() ? itr->second : 0;
}

// Method that tell if a creature is spawned currently
template<>
bool SpawnedPoolData::IsSpawnedObject<Creature>(uint32 db_guid) const
{
    return mSpawnedCreatures.find(db_guid) != mSpawnedCreatures.end();
}

// Method that tell if a gameobject is spawned currently
template<>
bool SpawnedPoolData::IsSpawnedObject<GameObject>(uint32 db_guid) const
{
    return mSpawnedGameobjects.find(db_guid) != mSpawnedGameobjects.end();
}

// Method that tell if a pool is spawned currently
template<>
bool SpawnedPoolData::IsSpawnedObject<Pool>(uint32 sub_pool_id) const
{
    return mSpawnedPools.find(sub_pool_id) != mSpawnedPools.end();
}

template<>
void SpawnedPoolData::AddSpawn<Creature>(uint32 db_guid, uint32 pool_id)
{
    mSpawnedCreatures.insert(db_guid);
    ++mSpawnedPools[pool_id];
}

template<>
void SpawnedPoolData::AddSpawn<GameObject>(uint32 db_guid, uint32 pool_id)
{
    mSpawnedGameobjects.insert(db_guid);
    ++mSpawnedPools[pool_id];
}

template<>
void SpawnedPoolData::AddSpawn<Pool>(uint32 sub_pool_id, uint32 pool_id)
{
    mSpawnedPools[sub_pool_id] = 0;
    ++mSpawnedPools[pool_id];
}

template<>
void SpawnedPoolData::RemoveSpawn<Creature>(uint32 db_guid, uint32 pool_id)
{
    mSpawnedCreatures.erase(db_guid);
    uint32& val = mSpawnedPools[pool_id];
    if (val > 0)
        --val;
}

template<>
void SpawnedPoolData::RemoveSpawn<GameObject>(uint32 db_guid, uint32 pool_id)
{
    mSpawnedGameobjects.erase(db_guid);
    uint32& val = mSpawnedPools[pool_id];
    if (val > 0)
        --val;
}

template<>
void SpawnedPoolData::RemoveSpawn<Pool>(uint32 sub_pool_id, uint32 pool_id)
{
    mSpawnedPools.erase(sub_pool_id);
    uint32& val = mSpawnedPools[pool_id];
    if (val > 0)
        --val;
}

////////////////////////////////////////////////////////////
// Methods of class PoolObject
template<>
void PoolObject::CheckEventLinkAndReport<Creature>(uint32 poolId, int16 event_id, std::map<uint32, int16> const& creature2event, std::map<uint32, int16> const& /*go2event*/) const
{
    std::map<uint32, int16>::const_iterator itr = creature2event.find(guid);
    if (itr == creature2event.end() || itr->second != event_id)
        sLog.outErrorDb("Creature (GUID: %u) expected to be listed in `game_event_creature` for event %u as part pool %u", guid, event_id, poolId);
}

template<>
void PoolObject::CheckEventLinkAndReport<GameObject>(uint32 poolId, int16 event_id, std::map<uint32, int16> const& /*creature2event*/, std::map<uint32, int16> const& go2event) const
{
    std::map<uint32, int16>::const_iterator itr = go2event.find(guid);
    if (itr == go2event.end() || itr->second != event_id)
        sLog.outErrorDb("Gameobject (GUID: %u) expected to be listed in `game_event_gameobject` for event %u as part pool %u", guid, event_id, poolId);
}

template<>
void PoolObject::CheckEventLinkAndReport<Pool>(uint32 poolId, int16 event_id, std::map<uint32, int16> const& creature2event, std::map<uint32, int16> const& go2event) const
{
    sPoolMgr.CheckEventLinkAndReport(guid, event_id, creature2event, go2event);
}

////////////////////////////////////////////////////////////
// Methods of template class PoolGroup

// Method to add a gameobject/creature guid to the proper list depending on pool type and chance value
template <class T>
void PoolGroup<T>::AddEntry(PoolObject& poolitem, uint32 maxentries)
{
    if (poolitem.chance != 0 && maxentries == 1)
        ExplicitlyChanced.push_back(poolitem);
    else
        EqualChanced.push_back(poolitem);
}

// Method to check the chances are proper in this object pool
template <class T>
bool PoolGroup<T>::CheckPool() const
{
    if (EqualChanced.size() == 0)
    {
        float chance = 0;
        for (uint32 i=0; i<ExplicitlyChanced.size(); ++i)
            chance += ExplicitlyChanced[i].chance;
        if (chance != 100 && chance != 0)
            return false;
    }
    return true;
}

// Method to check event linking
template <class T>
void PoolGroup<T>::CheckEventLinkAndReport(int16 event_id, std::map<uint32, int16> const& creature2event, std::map<uint32, int16> const& go2event) const
{
    for (uint32 i=0; i < EqualChanced.size(); ++i)
        EqualChanced[i].CheckEventLinkAndReport<T>(poolId, event_id, creature2event, go2event);

    for (uint32 i=0; i<ExplicitlyChanced.size(); ++i)
        ExplicitlyChanced[i].CheckEventLinkAndReport<T>(poolId, event_id, creature2event, go2event);
}

template <class T>
void PoolGroup<T>::SetExcludeObject(uint32 guid, bool state)
{
    for (uint32 i=0; i < EqualChanced.size(); ++i)
    {
        if (EqualChanced[i].guid == guid)
        {
            EqualChanced[i].exclude = state;
            return;
        }
    }

    for (uint32 i=0; i<ExplicitlyChanced.size(); ++i)
    {
        if (ExplicitlyChanced[i].guid == guid)
        {
            ExplicitlyChanced[i].exclude = state;
            return;
        }
    }
}


template <class T>
PoolObject* PoolGroup<T>::RollOne(SpawnedPoolData& spawns, uint32 triggerFrom)
{
    if (!ExplicitlyChanced.empty())
    {
        float roll = (float)rand_chance();

        for (uint32 i = 0; i < ExplicitlyChanced.size(); ++i)
        {
            roll -= ExplicitlyChanced[i].chance;
            // Triggering object is marked as spawned at this time and can be also rolled (respawn case)
            // so this need explicit check for this case
            if (roll < 0 && !ExplicitlyChanced[i].exclude && (ExplicitlyChanced[i].guid == triggerFrom || !spawns.IsSpawnedObject<T>(ExplicitlyChanced[i].guid)))
                return &ExplicitlyChanced[i];
        }
    }

    if (!EqualChanced.empty())
    {
        int32 index = irand(0, EqualChanced.size()-1);
        // Triggering object is marked as spawned at this time and can be also rolled (respawn case)
        // so this need explicit check for this case
        if (!EqualChanced[index].exclude && (EqualChanced[index].guid == triggerFrom || !spawns.IsSpawnedObject<T>(EqualChanced[index].guid)))
            return &EqualChanced[index];
    }

    return NULL;
}

// Main method to despawn a creature or gameobject in a pool
// If no guid is passed, the pool is just removed (event end case)
// If guid is filled, cache will be used and no removal will occur, it just fill the cache
template<class T>
void PoolGroup<T>::DespawnObject(MapPersistentState& mapState, uint32 guid)
{
    for (size_t i = 0; i < EqualChanced.size(); ++i)
    {
        // if spawned
        if (mapState.GetSpawnedPoolData().IsSpawnedObject<T>(EqualChanced[i].guid))
        {
            // any or specially requested
            if (!guid || EqualChanced[i].guid == guid)
            {
                Despawn1Object(mapState, EqualChanced[i].guid);
                mapState.GetSpawnedPoolData().RemoveSpawn<T>(EqualChanced[i].guid,poolId);
            }
        }
    }

    for (size_t i = 0; i < ExplicitlyChanced.size(); ++i)
    {
        // spawned
        if (mapState.GetSpawnedPoolData().IsSpawnedObject<T>(ExplicitlyChanced[i].guid))
        {
            // any or specially requested
            if (!guid || ExplicitlyChanced[i].guid == guid)
            {
                Despawn1Object(mapState, ExplicitlyChanced[i].guid);
                mapState.GetSpawnedPoolData().RemoveSpawn<T>(ExplicitlyChanced[i].guid,poolId);
            }
        }
    }
}

// Method that is actualy doing the removal job on one creature
template<>
void PoolGroup<Creature>::Despawn1Object(MapPersistentState& mapState, uint32 guid)
{
    if (CreatureData const* data = sObjectMgr.GetCreatureData(guid))
    {
        // for non-instanceable maps pool spawn can be at different map from provided mapState
        if (MapPersistentState* dataMapState = mapState.GetMapId() == data->mapid ? &mapState : sMapPersistentStateMgr.GetPersistentState(data->mapid, 0))
        {
            dataMapState->RemoveCreatureFromGrid(guid, data);

            if (Map* dataMap = dataMapState->GetMap())
                if (Creature* pCreature = dataMap->GetCreature(data->GetObjectGuid(guid)))
                    pCreature->AddObjectToRemoveList();
        }
    }
}

// Same on one gameobject
template<>
void PoolGroup<GameObject>::Despawn1Object(MapPersistentState& mapState, uint32 guid)
{
    if (GameObjectData const* data = sObjectMgr.GetGOData(guid))
    {
        // for non-instanceable maps pool spawn can be at different map from provided mapState
        if (MapPersistentState* dataMapState = mapState.GetMapId() == data->mapid ? &mapState : sMapPersistentStateMgr.GetPersistentState(data->mapid, 0))
        {
            dataMapState->RemoveGameobjectFromGrid(guid, data);

            if (Map* dataMap = dataMapState->GetMap())
                if (GameObject* pGameobject = dataMap->GetGameObject(ObjectGuid(HIGHGUID_GAMEOBJECT, data->id, guid)))
                    pGameobject->AddObjectToRemoveList();
        }
    }
}

// Same on one pool
template<>
void PoolGroup<Pool>::Despawn1Object(MapPersistentState& mapState, uint32 child_pool_id)
{
    sPoolMgr.DespawnPool(mapState, child_pool_id);
}

// Method for a pool only to remove any found record causing a circular dependency loop
template<>
void PoolGroup<Pool>::RemoveOneRelation(uint16 child_pool_id)
{
    for (PoolObjectList::iterator itr = ExplicitlyChanced.begin(); itr != ExplicitlyChanced.end(); ++itr)
    {
        if(itr->guid == child_pool_id)
        {
            ExplicitlyChanced.erase(itr);
            break;
        }
    }
    for (PoolObjectList::iterator itr = EqualChanced.begin(); itr != EqualChanced.end(); ++itr)
    {
        if(itr->guid == child_pool_id)
        {
            EqualChanced.erase(itr);
            break;
        }
    }
}

template <class T>
void PoolGroup<T>::SpawnObject(MapPersistentState& mapState, uint32 limit, uint32 triggerFrom, bool instantly)
{
    SpawnedPoolData& spawns = mapState.GetSpawnedPoolData();

    uint32 lastDespawned = 0;
    int count = limit - spawns.GetSpawnedObjects(poolId);

    // If triggered from some object respawn this object is still marked as spawned
    // and also counted into m_SpawnedPoolAmount so we need increase count to be
    // spawned by 1
    if (triggerFrom)
    {
        if (spawns.IsSpawnedObject<T>(triggerFrom))
            ++count;
        else
            triggerFrom = 0;
    }

    // This will try to spawn the rest of pool, not guaranteed
    for (int i = 0; i < count; ++i)
    {
        PoolObject* obj = RollOne(spawns,triggerFrom);
        if (!obj)
            continue;
        if (obj->guid == lastDespawned)
            continue;

        if (obj->guid == triggerFrom)
        {
            MANGOS_ASSERT(spawns.IsSpawnedObject<T>(obj->guid));
            MANGOS_ASSERT(spawns.GetSpawnedObjects(poolId) > 0);
            ReSpawn1Object(mapState, obj);
            triggerFrom = 0;
            continue;
        }

        spawns.AddSpawn<T>(obj->guid,poolId);
        Spawn1Object(mapState, obj, instantly);

        if (triggerFrom)
        {
            // One spawn one despawn no count increase
            DespawnObject(mapState, triggerFrom);
            lastDespawned = triggerFrom;
            triggerFrom = 0;
        }
    }
}

// Method that is actualy doing the spawn job on 1 creature
template <>
void PoolGroup<Creature>::Spawn1Object(MapPersistentState& mapState, PoolObject* obj, bool instantly)
{
    if (CreatureData const* data = sObjectMgr.GetCreatureData(obj->guid))
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(data->mapid);

        // for non-instanceable maps pool spawn can be at different map from provided mapState
        if (MapPersistentState* dataMapState = mapState.GetMapId() == data->mapid ? &mapState : sMapPersistentStateMgr.GetPersistentState(data->mapid, 0))
        {
            dataMapState->AddCreatureToGrid(obj->guid, data);

            Map* dataMap = dataMapState->GetMap();

            // We use spawn coords to spawn
            if (dataMap && dataMap->IsLoaded(data->posX, data->posY))
            {
                Creature* pCreature = new Creature;
                //DEBUG_LOG("Spawning creature %u",obj->guid);
                if (!pCreature->LoadFromDB(obj->guid, dataMap))
                {
                    delete pCreature;
                    return;
                }
                else
                {
                    // if new spawn replaces a just despawned creature, not instantly spawn but set respawn timer
                    if(!instantly)
                    {
                        pCreature->SetRespawnTime( pCreature->GetRespawnDelay() );
                        if (sWorld.getConfig(CONFIG_BOOL_SAVE_RESPAWN_TIME_IMMEDIATELY) || pCreature->IsWorldBoss())
                            pCreature->SaveRespawnTime();
                    }
                    dataMap->Add(pCreature);
                }
            }
            // for not loaded grid just update respawn time (avoid work for instances until implemented support)
            else if(!instantly)
            {
                dataMapState->SaveCreatureRespawnTime(obj->guid, time(NULL) + data->spawntimesecs);
            }
        }
    }
}

// Same for 1 gameobject
template <>
void PoolGroup<GameObject>::Spawn1Object(MapPersistentState& mapState, PoolObject* obj, bool instantly)
{
    if (GameObjectData const* data = sObjectMgr.GetGOData(obj->guid))
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(data->mapid);

        // for non-instanceable maps pool spawn can be at different map from provided mapState
        if (MapPersistentState* dataMapState = mapState.GetMapId() == data->mapid ? &mapState : sMapPersistentStateMgr.GetPersistentState(data->mapid, 0))
        {
            dataMapState->AddGameobjectToGrid(obj->guid, data);

            Map* dataMap = dataMapState->GetMap();

            // We use spawn coords to spawn
            if (dataMap && dataMap->IsLoaded(data->posX, data->posY))
            {
                GameObject* pGameobject = new GameObject;
                //DEBUG_LOG("Spawning gameobject %u", obj->guid);
                if (!pGameobject->LoadFromDB(obj->guid, dataMap))
                {
                    delete pGameobject;
                    return;
                }
                else
                {
                    if (pGameobject->isSpawnedByDefault())
                    {
                        // if new spawn replaces a just despawned object, not instantly spawn but set respawn timer
                        if(!instantly)
                        {
                            pGameobject->SetRespawnTime( pGameobject->GetRespawnDelay() );
                            if (sWorld.getConfig(CONFIG_BOOL_SAVE_RESPAWN_TIME_IMMEDIATELY))
                                pGameobject->SaveRespawnTime();
                        }
                        dataMap->Add(pGameobject);
                    }
                }
            }
            // for not loaded grid just update respawn time (avoid work for instances until implemented support)
            else if(!instantly)
            {
                // for spawned by default object only
                if (data->spawntimesecs >= 0)
                    dataMapState->SaveGORespawnTime(obj->guid, time(NULL) + data->spawntimesecs);
            }
        }
    }
}

// Same for 1 pool
template <>
void PoolGroup<Pool>::Spawn1Object(MapPersistentState& mapState, PoolObject* obj, bool instantly)
{
    sPoolMgr.SpawnPool(mapState, obj->guid, instantly);
}

// Method that does the respawn job on the specified creature
template <>
void PoolGroup<Creature>::ReSpawn1Object(MapPersistentState& mapState, PoolObject* obj)
{
    if (CreatureData const* data = sObjectMgr.GetCreatureData(obj->guid))
    {
        // for non-instanceable maps pool spawn can be at different map from provided mapState
        if (MapPersistentState* dataMapState = mapState.GetMapId() == data->mapid ? &mapState : sMapPersistentStateMgr.GetPersistentState(data->mapid, 0))
            if (Map* dataMap = dataMapState->GetMap())
                if (Creature* pCreature = dataMap->GetCreature(data->GetObjectGuid(obj->guid)))
                    pCreature->GetMap()->Add(pCreature);
    }
}

// Method that does the respawn job on the specified gameobject
template <>
void PoolGroup<GameObject>::ReSpawn1Object(MapPersistentState& mapState, PoolObject* obj)
{
    if (GameObjectData const* data = sObjectMgr.GetGOData(obj->guid))
    {
        // for non-instanceable maps pool spawn can be at different map from provided mapState
        if (MapPersistentState* dataMapState = mapState.GetMapId() == data->mapid ? &mapState : sMapPersistentStateMgr.GetPersistentState(data->mapid, 0))
            if (Map* dataMap = dataMapState->GetMap())
                if (GameObject* pGameobject = dataMap->GetGameObject(ObjectGuid(HIGHGUID_GAMEOBJECT, data->id, obj->guid)))
                    pGameobject->GetMap()->Add(pGameobject);
    }
}

// Nothing to do for a child Pool
template <>
void PoolGroup<Pool>::ReSpawn1Object(MapPersistentState& /*mapState*/, PoolObject* /*obj*/)
{
}


////////////////////////////////////////////////////////////
// Methods of class PoolManager

PoolManager::PoolManager()
{
}

// Check listing all pool spawns in single instanceable map or only in non-instanceable maps
// This applied to all pools have common mother pool
struct PoolMapChecker
{
    PoolManager::PoolTemplateDataMap& m_poolTemplates;

    explicit PoolMapChecker(PoolManager::PoolTemplateDataMap& poolTemplates) : m_poolTemplates(poolTemplates) {}

    bool CheckAndRemember(uint32 mapid, uint32 pool_id, char const* tableName, char const* elementName)
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(mapid);
        if (!mapEntry)
            return false;

        MapEntry const* poolMapEntry = m_poolTemplates[pool_id].mapEntry;

        // if not listed then just remember
        if (!poolMapEntry)
        {
            m_poolTemplates[pool_id].mapEntry = mapEntry;
            return true;
        }

        // if at same map, then all ok
        if (poolMapEntry == mapEntry)
            return true;

        // pool spawns must be at single instanceable map
        if (mapEntry->Instanceable())
        {
            sLog.outErrorDb("`%s` has %s spawned at instanceable map %u when one or several other spawned at different map %u in pool id %i, skipped.",
                tableName, elementName, mapid, poolMapEntry->MapID, pool_id);
            return false;
        }

        // pool spawns must be at single instanceable map
        if (poolMapEntry->Instanceable())
        {
            sLog.outErrorDb("`%s` has %s spawned at map %u when one or several other spawned at different instanceable map %u in pool id %i, skipped.",
                tableName, elementName, mapid, poolMapEntry->MapID, pool_id);
            return false;
        }

        // pool spawns can be at different non-instanceable maps
        return true;
    }
};

void PoolManager::LoadFromDB()
{
    QueryResult *result = WorldDatabase.Query("SELECT MAX(entry) FROM pool_template");
    if (!result)
    {
        sLog.outString(">> Table pool_template is empty.");
        sLog.outString();
        return;
    }
    else
    {
        Field *fields = result->Fetch();
        max_pool_id = fields[0].GetUInt16();
        delete result;
    }

    mPoolTemplate.resize(max_pool_id + 1);

    result = WorldDatabase.Query("SELECT entry, max_limit, description FROM pool_template");
    if (!result)
    {
        mPoolTemplate.clear();
        sLog.outString(">> Table pool_template is empty:");
        sLog.outString();
        return;
    }

    uint32 count = 0;

    BarGoLink bar(result->GetRowCount());
    do
    {
        ++count;
        Field *fields = result->Fetch();

        bar.step();

        uint16 pool_id = fields[0].GetUInt16();

        PoolTemplateData& pPoolTemplate = mPoolTemplate[pool_id];
        pPoolTemplate.MaxLimit    = fields[1].GetUInt32();
        pPoolTemplate.description = fields[2].GetCppString();
        pPoolTemplate.AutoSpawn = true;          // will update and later data loading

    } while (result->NextRow());

    sLog.outString();
    sLog.outString( ">> Loaded %u objects pools", count );
    delete result;

    PoolMapChecker mapChecker(mPoolTemplate);

    // Creatures (guids and entries)

    mPoolCreatureGroups.resize(max_pool_id + 1);
    mCreatureSearchMap.clear();
    //                                   1     2           3
    result = WorldDatabase.Query("SELECT guid, pool_entry, chance FROM pool_creature");

    count = 0;
    if (!result)
    {
        BarGoLink bar2(1);
        bar2.step();

        sLog.outString();
        sLog.outString(">> Loaded %u creatures in pools from `pool_creature`", count );
    }
    else
    {

        BarGoLink bar2(result->GetRowCount());
        do
        {
            Field *fields = result->Fetch();

            bar2.step();

            uint32 guid    = fields[0].GetUInt32();
            uint16 pool_id = fields[1].GetUInt16();
            float chance   = fields[2].GetFloat();

            CreatureData const* data = sObjectMgr.GetCreatureData(guid);
            if (!data)
            {
                sLog.outErrorDb("`pool_creature` has a non existing creature spawn (GUID: %u) defined for pool id (%u), skipped.", guid, pool_id );
                continue;
            }
            if (pool_id > max_pool_id)
            {
                sLog.outErrorDb("`pool_creature` pool id (%i) is out of range compared to max pool id in `pool_template`, skipped.",pool_id);
                continue;
            }
            if (chance < 0 || chance > 100)
            {
                sLog.outErrorDb("`pool_creature` has an invalid chance (%f) for creature guid (%u) in pool id (%i), skipped.", chance, guid, pool_id);
                continue;
            }

            if (!mapChecker.CheckAndRemember(data->mapid, pool_id, "pool_creature", "creature guid"))
                continue;

            PoolTemplateData *pPoolTemplate = &mPoolTemplate[pool_id];

            ++count;

            PoolObject plObject = PoolObject(guid, chance);
            PoolGroup<Creature>& cregroup = mPoolCreatureGroups[pool_id];
            cregroup.SetPoolId(pool_id);
            cregroup.AddEntry(plObject, pPoolTemplate->MaxLimit);
            SearchPair p(guid, pool_id);
            mCreatureSearchMap.insert(p);

        } while (result->NextRow());
        sLog.outString();
        sLog.outString( ">> Loaded %u creatures in pools from `pool_creature`", count );
        delete result;
    }

    result = WorldDatabase.Query("SELECT guid, pool_entry, chance, pool_creature_template.id FROM pool_creature_template LEFT JOIN creature ON creature.id = pool_creature_template.id");

    count = 0;
    if (!result)
    {
        BarGoLink bar2(1);
        bar2.step();

        sLog.outString();
        sLog.outString(">> Loaded %u creatures in pools from `pool_creature_template`", count );
    }
    else
    {
        BarGoLink bar2(result->GetRowCount());
        do
        {
            Field *fields = result->Fetch();

            bar2.step();

            uint32 guid    = fields[0].GetUInt32();
            uint16 pool_id = fields[1].GetUInt16();
            float chance   = fields[2].GetFloat();
            uint16 entry_id = fields[3].GetUInt32();        // for errors output only

            CreatureData const* data = sObjectMgr.GetCreatureData(guid);
            if (!data)
            {
                sLog.outErrorDb("`pool_creature_template` has a non existing creature spawn (GUID: %u Entry: %u) defined for pool id (%u), skipped.", guid, entry_id, pool_id );
                continue;
            }
            if (pool_id > max_pool_id)
            {
                sLog.outErrorDb("`pool_creature_template` pool id (%i) is out of range compared to max pool id in `pool_template`, skipped.",pool_id);
                continue;
            }
            if (chance < 0 || chance > 100)
            {
                sLog.outErrorDb("`pool_creature_template` has an invalid chance (%f) for creature (Guid %u Entry %u) in pool id (%i), skipped.", chance, guid, entry_id, pool_id);
                continue;
            }

            // `pool_creature` and `pool_creature_template` can't have guids duplicates (in second case because entries also unique)
            // So if guid already listed in pools then this duplicate from alt.table
            // Also note: for added guid not important what case we skip from 2 tables
            if (uint16 alt_pool_id = IsPartOfAPool<Creature>(guid))
            {
                sLog.outErrorDb("`pool_creature` has guid %u for pool %u that already added to pool %u from `pool_creature_template` for creature entry %u, skipped.",
                    guid, pool_id, alt_pool_id, entry_id);
                continue;
            }

            if (!mapChecker.CheckAndRemember(data->mapid, pool_id, "pool_creature_template", "creature guid"))
                continue;

            PoolTemplateData *pPoolTemplate = &mPoolTemplate[pool_id];

            ++count;

            PoolObject plObject = PoolObject(guid, chance);
            PoolGroup<Creature>& cregroup = mPoolCreatureGroups[pool_id];
            cregroup.SetPoolId(pool_id);
            cregroup.AddEntry(plObject, pPoolTemplate->MaxLimit);
            SearchPair p(guid, pool_id);
            mCreatureSearchMap.insert(p);

        } while (result->NextRow());
        sLog.outString();
        sLog.outString(">> Loaded %u creatures in pools from `pool_creature_template`", count );
        delete result;
    }

    // Gameobjects (guids and entries)

    mPoolGameobjectGroups.resize(max_pool_id + 1);
    mGameobjectSearchMap.clear();
    //                                   1     2           3
    result = WorldDatabase.Query("SELECT guid, pool_entry, chance FROM pool_gameobject");

    count = 0;
    if (!result)
    {
        BarGoLink bar2(1);
        bar2.step();

        sLog.outString();
        sLog.outString(">> Loaded %u gameobject in pools from `pool_gameobject`", count );
    }
    else
    {

        BarGoLink bar2(result->GetRowCount());
        do
        {
            Field *fields = result->Fetch();

            bar2.step();

            uint32 guid    = fields[0].GetUInt32();
            uint16 pool_id = fields[1].GetUInt16();
            float chance   = fields[2].GetFloat();

            GameObjectData const* data = sObjectMgr.GetGOData(guid);
            if (!data)
            {
                sLog.outErrorDb("`pool_gameobject` has a non existing gameobject spawn (GUID: %u) defined for pool id (%u), skipped.", guid, pool_id );
                continue;
            }
            GameObjectInfo const* goinfo = ObjectMgr::GetGameObjectInfo(data->id);
            if (goinfo->type != GAMEOBJECT_TYPE_CHEST &&
                goinfo->type != GAMEOBJECT_TYPE_GOOBER &&
                goinfo->type != GAMEOBJECT_TYPE_FISHINGHOLE)
            {
                sLog.outErrorDb("`pool_gameobject` has a not lootable gameobject spawn (GUID: %u, type: %u) defined for pool id (%u), skipped.", guid, goinfo->type, pool_id );
                continue;
            }
            if (pool_id > max_pool_id)
            {
                sLog.outErrorDb("`pool_gameobject` pool id (%i) is out of range compared to max pool id in `pool_template`, skipped.",pool_id);
                continue;
            }
            if (chance < 0 || chance > 100)
            {
                sLog.outErrorDb("`pool_gameobject` has an invalid chance (%f) for gameobject guid (%u) in pool id (%i), skipped.", chance, guid, pool_id);
                continue;
            }

            if (!mapChecker.CheckAndRemember(data->mapid, pool_id, "pool_gameobject", "gameobject guid"))
                continue;

            PoolTemplateData *pPoolTemplate = &mPoolTemplate[pool_id];

            ++count;

            PoolObject plObject = PoolObject(guid, chance);
            PoolGroup<GameObject>& gogroup = mPoolGameobjectGroups[pool_id];
            gogroup.SetPoolId(pool_id);
            gogroup.AddEntry(plObject, pPoolTemplate->MaxLimit);
            SearchPair p(guid, pool_id);
            mGameobjectSearchMap.insert(p);

        } while( result->NextRow() );
        sLog.outString();
        sLog.outString(">> Loaded %u gameobject in pools from `pool_gameobject`", count );
        delete result;
    }

    //                                   1     2           3
    result = WorldDatabase.Query("SELECT guid, pool_entry, chance, pool_gameobject_template.id FROM pool_gameobject_template LEFT JOIN gameobject ON gameobject.id = pool_gameobject_template.id");

    count = 0;
    if (!result)
    {
        BarGoLink bar2(1);
        bar2.step();

        sLog.outString();
        sLog.outString(">> Loaded %u gameobject in pools from `pool_gameobject_template`", count );
    }
    else
    {

        BarGoLink bar2(result->GetRowCount());
        do
        {
            Field *fields = result->Fetch();

            bar2.step();

            uint32 guid    = fields[0].GetUInt32();
            uint16 pool_id = fields[1].GetUInt16();
            float chance   = fields[2].GetFloat();
            uint16 entry_id = fields[3].GetUInt32();        // for errors output only

            GameObjectData const* data = sObjectMgr.GetGOData(guid);
            if (!data)
            {
                sLog.outErrorDb("`pool_gameobject_template` has a non existing gameobject spawn (GUID: %u Entry %u) defined for pool id (%u), skipped.", guid, entry_id, pool_id);
                continue;
            }
            GameObjectInfo const* goinfo = ObjectMgr::GetGameObjectInfo(data->id);
            if (goinfo->type != GAMEOBJECT_TYPE_CHEST &&
                goinfo->type != GAMEOBJECT_TYPE_GOOBER &&
                goinfo->type != GAMEOBJECT_TYPE_FISHINGHOLE)
            {
                sLog.outErrorDb("`pool_gameobject_template` has a not lootable gameobject spawn (GUID: %u Entry % Type: %u) defined for pool id (%u), skipped.", guid, entry_id, goinfo->type, pool_id );
                continue;
            }
            if (pool_id > max_pool_id)
            {
                sLog.outErrorDb("`pool_gameobject_template` pool id (%i) is out of range compared to max pool id in `pool_template`, skipped.",pool_id);
                continue;
            }
            if (chance < 0 || chance > 100)
            {
                sLog.outErrorDb("`pool_gameobject_template` has an invalid chance (%f) for gameobject (Guid %u Entry %u) in pool id (%i), skipped.", chance, guid, entry_id, pool_id);
                continue;
            }

            // `pool_gameobject` and `pool_gameobject_template` can't have guids duplicates (in second case because entries also unique)
            // So if guid already listed in pools then this duplicate from alt.table
            // Also note: for added guid not important what case we skip from 2 tables
            if (uint16 alt_pool_id = IsPartOfAPool<GameObject>(guid))
            {
                sLog.outErrorDb("`pool_gameobject` has guid %u for pool %u that already added to pool %u from `pool_gameobject_template` for gameobject entry %u, skipped.",
                    guid, pool_id, alt_pool_id, entry_id);
                continue;
            }

            if (!mapChecker.CheckAndRemember(data->mapid, pool_id, "pool_gameobject_template", "gameobject guid"))
                continue;

            PoolTemplateData *pPoolTemplate = &mPoolTemplate[pool_id];

            ++count;

            PoolObject plObject = PoolObject(guid, chance);
            PoolGroup<GameObject>& gogroup = mPoolGameobjectGroups[pool_id];
            gogroup.SetPoolId(pool_id);
            gogroup.AddEntry(plObject, pPoolTemplate->MaxLimit);
            SearchPair p(guid, pool_id);
            mGameobjectSearchMap.insert(p);

        } while( result->NextRow() );
        sLog.outString();
        sLog.outString(">> Loaded %u gameobject in pools from `pool_gameobject_template`", count );
        delete result;
    }

    // Pool of pools
    mPoolPoolGroups.resize(max_pool_id + 1);
    //                                   1        2            3
    result = WorldDatabase.Query("SELECT pool_id, mother_pool, chance FROM pool_pool");

    count = 0;
    if( !result )
    {
        BarGoLink bar2(1);
        bar2.step();

        sLog.outString();
        sLog.outString(">> Loaded %u pools in pools", count );
    }
    else
    {

        BarGoLink bar2(result->GetRowCount());
        do
        {
            Field *fields = result->Fetch();

            bar2.step();

            uint16 child_pool_id  = fields[0].GetUInt16();
            uint16 mother_pool_id = fields[1].GetUInt16();
            float chance          = fields[2].GetFloat();

            if (mother_pool_id > max_pool_id)
            {
                sLog.outErrorDb("`pool_pool` mother_pool id (%i) is out of range compared to max pool id in `pool_template`, skipped.",mother_pool_id);
                continue;
            }
            if (child_pool_id > max_pool_id)
            {
                sLog.outErrorDb("`pool_pool` included pool_id (%i) is out of range compared to max pool id in `pool_template`, skipped.",child_pool_id);
                continue;
            }
            if (mother_pool_id == child_pool_id)
            {
                sLog.outErrorDb("`pool_pool` pool_id (%i) includes itself, dead-lock detected, skipped.",child_pool_id);
                continue;
            }
            if (chance < 0 || chance > 100)
            {
                sLog.outErrorDb("`pool_pool` has an invalid chance (%f) for pool id (%u) in mother pool id (%i), skipped.", chance, child_pool_id, mother_pool_id);
                continue;
            }

            PoolTemplateData *pPoolTemplateMother = &mPoolTemplate[mother_pool_id];

            ++count;

            PoolObject plObject = PoolObject(child_pool_id, chance);
            PoolGroup<Pool>& plgroup = mPoolPoolGroups[mother_pool_id];
            plgroup.SetPoolId(mother_pool_id);
            plgroup.AddEntry(plObject, pPoolTemplateMother->MaxLimit);
            SearchPair p(child_pool_id, mother_pool_id);
            mPoolSearchMap.insert(p);

            // update top independent pool flag
            mPoolTemplate[child_pool_id].AutoSpawn = false;

        } while( result->NextRow() );

        // Now check for circular reference
        for(uint16 i=0; i<max_pool_id; ++i)
        {
            std::set<uint16> checkedPools;
            for(SearchMap::iterator poolItr = mPoolSearchMap.find(i); poolItr != mPoolSearchMap.end(); poolItr = mPoolSearchMap.find(poolItr->second))
            {
                // if child pool not have map data then it empty or have not checked child then will checked and all line later
                if (MapEntry const* childMapEntry = mPoolTemplate[poolItr->first].mapEntry)
                {
                    if (!mapChecker.CheckAndRemember(childMapEntry->MapID, poolItr->second, "pool_pool", "pool with creature/gameobject"))
                    {
                        mPoolPoolGroups[poolItr->second].RemoveOneRelation(poolItr->first);
                        mPoolSearchMap.erase(poolItr);
                        --count;
                        break;
                    }
                }

                checkedPools.insert(poolItr->first);
                if(checkedPools.find(poolItr->second) != checkedPools.end())
                {
                    std::ostringstream ss;
                    ss<< "The pool(s) ";
                    for (std::set<uint16>::const_iterator itr=checkedPools.begin(); itr!=checkedPools.end(); ++itr)
                        ss << *itr << " ";
                    ss << "create(s) a circular reference, which can cause the server to freeze.\nRemoving the last link between mother pool "
                        << poolItr->first << " and child pool " << poolItr->second;
                    sLog.outErrorDb("%s", ss.str().c_str());
                    mPoolPoolGroups[poolItr->second].RemoveOneRelation(poolItr->first);
                    mPoolSearchMap.erase(poolItr);
                    --count;
                    break;
                }
            }
        }

        sLog.outString();
        sLog.outString( ">> Loaded %u pools in mother pools", count );
        delete result;
    }

    // check chances integrity
    for(uint16 pool_entry = 0; pool_entry < mPoolTemplate.size(); ++pool_entry)
    {
        if (mPoolTemplate[pool_entry].AutoSpawn)
        {
            if (!CheckPool(pool_entry))
            {
                sLog.outErrorDb("Pool Id (%u) has all creatures or gameobjects with explicit chance sum <>100 and no equal chance defined. The pool system cannot pick one to spawn.", pool_entry);
                mPoolTemplate[pool_entry].AutoSpawn = false;
            }
        }
    }
}

// The initialize method will spawn all pools not in an event and not in another pool
void PoolManager::Initialize(MapPersistentState* state)
{
    // spawn pools for expected map or for not initialized shared pools state for non-instanceable maps
    for(uint16 pool_entry = 0; pool_entry < mPoolTemplate.size(); ++pool_entry)
        if (mPoolTemplate[pool_entry].AutoSpawn)
            InitSpawnPool(*state, pool_entry);
}

// Call to spawn a pool, if cache if true the method will spawn only if cached entry is different
// If it's same, the creature is respawned only (added back to map)
template<>
void PoolManager::SpawnPoolGroup<Creature>(MapPersistentState& mapState, uint16 pool_id, uint32 db_guid, bool instantly)
{
    if (!mPoolCreatureGroups[pool_id].isEmpty())
        mPoolCreatureGroups[pool_id].SpawnObject(mapState, mPoolTemplate[pool_id].MaxLimit, db_guid, instantly);
}

// Call to spawn a pool, if cache if true the method will spawn only if cached entry is different
// If it's same, the gameobject is respawned only (added back to map)
template<>
void PoolManager::SpawnPoolGroup<GameObject>(MapPersistentState& mapState, uint16 pool_id, uint32 db_guid, bool instantly)
{
    if (!mPoolGameobjectGroups[pool_id].isEmpty())
        mPoolGameobjectGroups[pool_id].SpawnObject(mapState, mPoolTemplate[pool_id].MaxLimit, db_guid, instantly);
}

// Call to spawn a pool, if cache if true the method will spawn only if cached entry is different
// If it's same, the pool is respawned only
template<>
void PoolManager::SpawnPoolGroup<Pool>(MapPersistentState& mapState, uint16 pool_id, uint32 sub_pool_id, bool instantly)
{
    if (!mPoolPoolGroups[pool_id].isEmpty())
        mPoolPoolGroups[pool_id].SpawnObject(mapState, mPoolTemplate[pool_id].MaxLimit, sub_pool_id, instantly);
}

/*!
    \param instantly defines if (leaf-)objects are spawned instantly or with fresh respawn timer */
void PoolManager::SpawnPool(MapPersistentState& mapState, uint16 pool_id, bool instantly)
{
    SpawnPoolGroup<Pool>(mapState, pool_id, 0, instantly);
    SpawnPoolGroup<GameObject>(mapState, pool_id, 0, instantly);
    SpawnPoolGroup<Creature>(mapState, pool_id, 0, instantly);
}

// Call to despawn a pool, all gameobjects/creatures in this pool are removed
void PoolManager::DespawnPool(MapPersistentState& mapState, uint16 pool_id)
{
    if (!mPoolCreatureGroups[pool_id].isEmpty())
        mPoolCreatureGroups[pool_id].DespawnObject(mapState);

    if (!mPoolGameobjectGroups[pool_id].isEmpty())
        mPoolGameobjectGroups[pool_id].DespawnObject(mapState);

    if (!mPoolPoolGroups[pool_id].isEmpty())
        mPoolPoolGroups[pool_id].DespawnObject(mapState);
}

// Method that check chance integrity of the creatures and gameobjects in this pool
bool PoolManager::CheckPool(uint16 pool_id) const
{
    return pool_id <= max_pool_id &&
        mPoolGameobjectGroups[pool_id].CheckPool() &&
        mPoolCreatureGroups[pool_id].CheckPool() &&
        mPoolPoolGroups[pool_id].CheckPool();
}

// Method that check linking all elements to event
void PoolManager::CheckEventLinkAndReport(uint16 pool_id, int16 event_id, std::map<uint32, int16> const& creature2event, std::map<uint32, int16> const& go2event) const
{
    mPoolGameobjectGroups[pool_id].CheckEventLinkAndReport(event_id, creature2event, go2event);
    mPoolCreatureGroups[pool_id].CheckEventLinkAndReport(event_id, creature2event, go2event);
    mPoolPoolGroups[pool_id].CheckEventLinkAndReport(event_id, creature2event, go2event);
}

// Method that exclude some elements from next spawn
template<>
void PoolManager::SetExcludeObject<Creature>(uint16 pool_id, uint32 db_guid_or_pool_id, bool state)
{
    mPoolCreatureGroups[pool_id].SetExcludeObject(db_guid_or_pool_id, state);
}

template<>
void PoolManager::SetExcludeObject<GameObject>(uint16 pool_id, uint32 db_guid_or_pool_id, bool state)
{
    mPoolGameobjectGroups[pool_id].SetExcludeObject(db_guid_or_pool_id, state);
}

// Call to update the pool when a gameobject/creature part of pool [pool_id] is ready to respawn
// Here we cache only the creature/gameobject whose guid is passed as parameter
// Then the spawn pool call will use this cache to decide
template<typename T>
void PoolManager::UpdatePool(MapPersistentState& mapState, uint16 pool_id, uint32 db_guid_or_pool_id)
{
    if (uint16 motherpoolid = IsPartOfAPool<Pool>(pool_id))
        SpawnPoolGroup<Pool>(mapState, motherpoolid, pool_id, false);
    else
        SpawnPoolGroup<T>(mapState, pool_id, db_guid_or_pool_id, false);
}

template void PoolManager::UpdatePool<Pool>(MapPersistentState& mapState, uint16 pool_id, uint32 db_guid_or_pool_id);
template void PoolManager::UpdatePool<GameObject>(MapPersistentState& mapState, uint16 pool_id, uint32 db_guid_or_pool_id);
template void PoolManager::UpdatePool<Creature>(MapPersistentState& mapState, uint16 pool_id, uint32 db_guid_or_pool_id);

struct SpawnPoolInMapsWorker
{
    explicit SpawnPoolInMapsWorker(PoolManager& mgr, uint32 pool_id, bool instantly)
        : i_mgr(mgr), i_pool_id(pool_id), i_instantly(instantly) {}

    void operator() (MapPersistentState* state)
    {
        i_mgr.SpawnPool(*state, i_pool_id, i_instantly);
    }

    PoolManager& i_mgr;
    uint32 i_pool_id;
    bool i_instantly;
};

// used for calling from global systems when need spawn pool in all appropriate map persistent states
void PoolManager::SpawnPoolInMaps(uint16 pool_id, bool instantly)
{
    PoolTemplateData& poolTemplate = mPoolTemplate[pool_id];

    // pool no have spawns (base at loading algo
    if (!poolTemplate.mapEntry)
        return;

    SpawnPoolInMapsWorker worker(*this, pool_id, instantly);
    sMapPersistentStateMgr.DoForAllStatesWithMapId(poolTemplate.mapEntry->MapID, worker);
}

struct DespawnPoolInMapsWorker
{
    explicit DespawnPoolInMapsWorker(PoolManager& mgr, uint32 pool_id)
        : i_mgr(mgr), i_pool_id(pool_id) {}

    void operator() (MapPersistentState* state)
    {
        i_mgr.DespawnPool(*state, i_pool_id);
    }

    PoolManager& i_mgr;
    uint32 i_pool_id;
};

// used for calling from global systems when need spawn pool in all appropriate map persistent states
void PoolManager::DespawnPoolInMaps(uint16 pool_id)
{
    PoolTemplateData& poolTemplate = mPoolTemplate[pool_id];

    // pool no have spawns (base at loading algo
    if (!poolTemplate.mapEntry)
        return;

    DespawnPoolInMapsWorker worker(*this, pool_id);
    sMapPersistentStateMgr.DoForAllStatesWithMapId(poolTemplate.mapEntry->MapID, worker);
}

void PoolManager::InitSpawnPool(MapPersistentState& mapState, uint16 pool_id)
{
    // spawn pool for expected map or for not initialized shared pools state for non-instanceable maps
    if (mPoolTemplate[pool_id].CanBeSpawnedAtMap(mapState.GetMapEntry()))
        SpawnPool(mapState, pool_id, true);
}

template<typename T>
struct UpdatePoolInMapsWorker
{
    explicit UpdatePoolInMapsWorker(PoolManager& mgr, uint32 pool_id, uint32 db_guid_or_pool_id)
        : i_mgr(mgr), i_pool_id(pool_id), i_db_guid_or_pool_id(db_guid_or_pool_id) {}

    void operator() (MapPersistentState* state)
    {
        i_mgr.UpdatePool<T>(*state, i_pool_id, i_db_guid_or_pool_id);
    }

    PoolManager& i_mgr;
    uint32 i_pool_id;
    uint32 i_db_guid_or_pool_id;
};

template<typename T>
void PoolManager::UpdatePoolInMaps(uint16 pool_id, uint32 db_guid_or_pool_id)
{
    PoolTemplateData& poolTemplate = mPoolTemplate[pool_id];

    // pool no have spawns (base at loading algo
    if (!poolTemplate.mapEntry)
        return;

    UpdatePoolInMapsWorker<T> worker(*this, pool_id, db_guid_or_pool_id);
    sMapPersistentStateMgr.DoForAllStatesWithMapId(poolTemplate.mapEntry->MapID, worker);
}

template void PoolManager::UpdatePoolInMaps<Pool>(uint16 pool_id, uint32 db_guid_or_pool_id);
template void PoolManager::UpdatePoolInMaps<GameObject>(uint16 pool_id, uint32 db_guid_or_pool_id);
template void PoolManager::UpdatePoolInMaps<Creature>(uint16 pool_id, uint32 db_guid_or_pool_id);
