/*
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#ifndef MANGOS_OBJECTACCESSOR_H
#define MANGOS_OBJECTACCESSOR_H

#include "Platform/Define.h"
#include "Policies/Singleton.h"
#include <ace/Thread_Mutex.h>
#include "Utilities/UnorderedMap.h"
#include "Policies/ThreadingModel.h"

#include "UpdateData.h"

#include "GridDefines.h"
#include "Object.h"
#include "Player.h"
#include "Corpse.h"

#include <set>
#include <list>

class Creature;
class Unit;
class GameObject;
class WorldObject;
class Map;

template <class T>
class HashMapHolder
{
    public:

        typedef UNORDERED_MAP< uint64, T* >   MapType;
        typedef ACE_Thread_Mutex LockType;
        typedef MaNGOS::GeneralLock<LockType > Guard;

        static void Insert(T* o) { m_objectMap[o->GetGUID()] = o; }

        static void Remove(T* o)
        {
            Guard guard(i_lock);
            m_objectMap.erase(o->GetGUID());
        }

        static T* Find(uint64 guid)
        {
            typename MapType::iterator itr = m_objectMap.find(guid);
            return (itr != m_objectMap.end()) ? itr->second : NULL;
        }

        static MapType& GetContainer() { return m_objectMap; }

        static LockType* GetLock() { return &i_lock; }
    private:

        //Non instanceable only static
        HashMapHolder() {}

        static LockType i_lock;
        static MapType  m_objectMap;
};

class MANGOS_DLL_DECL ObjectAccessor : public MaNGOS::Singleton<ObjectAccessor, MaNGOS::ClassLevelLockable<ObjectAccessor, ACE_Thread_Mutex> >
{

    friend class MaNGOS::OperatorNew<ObjectAccessor>;
    ObjectAccessor();
    ~ObjectAccessor();
    ObjectAccessor(const ObjectAccessor &);
    ObjectAccessor& operator=(const ObjectAccessor &);

    public:
        typedef UNORDERED_MAP<uint64, Corpse* >      Player2CorpsesMapType;

        // global (obj used for map only location local guid objects (pets currently)
        static Unit*   GetUnitInWorld(WorldObject const& obj, uint64 guid);

        // FIXME: map local object with global search
        static Creature*   GetCreatureInWorld(uint64 guid)   { return FindHelper<Creature>(guid); }
        static GameObject* GetGameObjectInWorld(uint64 guid) { return FindHelper<GameObject>(guid); }

        // possible local search for specific object map
        static Object* GetObjectByTypeMask(WorldObject const &, uint64, uint32 typemask);
        static Unit* GetUnit(WorldObject const &, uint64);

        // Player access
        static Player* FindPlayer(uint64 guid);
        static Player* FindPlayerByName(const char *name);
        static void KickPlayer(uint64 guid);

        HashMapHolder<Player>::MapType& GetPlayers()
        {
            return HashMapHolder<Player>::GetContainer();
        }

        void SaveAllPlayers();

        // Corpse access
        Corpse* GetCorpseForPlayerGUID(uint64 guid);
        static Corpse* GetCorpseInMap(uint64 guid, uint32 mapid);
        void RemoveCorpse(Corpse *corpse);
        void AddCorpse(Corpse* corpse);
        void AddCorpsesToGrid(GridPair const& gridpair,GridType& grid,Map* map);
        Corpse* ConvertCorpseForPlayer(uint64 player_guid, bool insignia = false);

        // For call from Player/Corpse AddToWorld/RemoveFromWorld only
        void AddObject(Corpse *object) { HashMapHolder<Corpse>::Insert(object); }
        void AddObject(Player *object) { HashMapHolder<Player>::Insert(object); }
        void RemoveObject(Corpse *object) { HashMapHolder<Corpse>::Remove(object); }
        void RemoveObject(Player *object) { HashMapHolder<Player>::Remove(object); }

        // TODO: This methods will need lock in MT environment
        static void LinkMap(Map* map)   { i_mapList.push_back(map); }
        static void DelinkMap(Map* map) { i_mapList.remove(map); }
    private:
        // TODO: This methods will need lock in MT environment
        // Theoreticaly multiple threads can enter and search in this method but
        // in that case linking/delinking other map should be guarded
        template <class OBJECT> static OBJECT* FindHelper(uint64 guid)
        {
            for (std::list<Map*>::const_iterator i = i_mapList.begin() ; i != i_mapList.end(); ++i)
            {
                if (OBJECT* ret = (*i)->GetObjectsStore().find(guid, (OBJECT*)NULL))
                    return ret;
            }

            return NULL;
        }

        static std::list<Map*> i_mapList;

        Player2CorpsesMapType   i_player2corpse;

        typedef ACE_Thread_Mutex LockType;
        typedef MaNGOS::GeneralLock<LockType > Guard;

        LockType i_playerGuard;
        LockType i_corpseGuard;
};

inline Unit* ObjectAccessor::GetUnitInWorld(WorldObject const& obj, uint64 guid)
{
    if(!guid)
        return NULL;

    if (IS_PLAYER_GUID(guid))
        return FindPlayer(guid);

    if (IS_PET_GUID(guid))
        return obj.IsInWorld() ? obj.GetMap()->GetPet(guid) : NULL;

    return GetCreatureInWorld(guid);
}

#define sObjectAccessor ObjectAccessor::Instance()

#endif
