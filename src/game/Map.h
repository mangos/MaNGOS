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

#ifndef MANGOS_MAP_H
#define MANGOS_MAP_H

#include "Common.h"
#include "Platform/Define.h"
#include "Policies/ThreadingModel.h"
#include "ace/RW_Thread_Mutex.h"
#include "ace/Thread_Mutex.h"

#include "DBCStructure.h"
#include "GridDefines.h"
#include "Cell.h"
#include "Object.h"
#include "Timer.h"
#include "SharedDefines.h"
#include "GridMap.h"
#include "GameSystem/GridRefManager.h"
#include "MapRefManager.h"
#include "Utilities/TypeList.h"
#include "ScriptMgr.h"

#include <bitset>
#include <list>

class Creature;
class Unit;
class WorldPacket;
class InstanceData;
class Group;
class InstanceSave;
struct ScriptInfo;
class BattleGround;
class GridMap;

// GCC have alternative #pragma pack(N) syntax and old gcc version not support pack(push,N), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

struct InstanceTemplate
{
    uint32 map;                                             // instance map
    uint32 parent;                                          // non-continent parent instance (for instance with entrance in another instances)
                                                            // or 0 (not related to continent 0 map id)
    uint32 levelMin;
    uint32 levelMax;
    uint32 script_id;
};

enum LevelRequirementVsMode
{
    LEVELREQUIREMENT_HEROIC = 70
};

#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

#define MIN_UNLOAD_DELAY      1                             // immediate unload

class MANGOS_DLL_SPEC Map : public GridRefManager<NGridType>
{
    friend class MapReference;
    friend class ObjectGridLoader;
    friend class ObjectWorldLoader;
    public:
        Map(uint32 id, time_t, uint32 InstanceId, uint8 SpawnMode);
        virtual ~Map();

        // currently unused for normal maps
        bool CanUnload(uint32 diff)
        {
            if(!m_unloadTimer) return false;
            if(m_unloadTimer <= diff) return true;
            m_unloadTimer -= diff;
            return false;
        }

        virtual bool Add(Player *);
        virtual void Remove(Player *, bool);
        template<class T> void Add(T *);
        template<class T> void Remove(T *, bool);

        static void DeleteFromWorld(Player* player);        // player object will deleted at call

        virtual void Update(const uint32&);

        void MessageBroadcast(Player *, WorldPacket *, bool to_self);
        void MessageBroadcast(WorldObject *, WorldPacket *);
        void MessageDistBroadcast(Player *, WorldPacket *, float dist, bool to_self, bool own_team_only = false);
        void MessageDistBroadcast(WorldObject *, WorldPacket *, float dist);

        float GetVisibilityDistance() const { return m_VisibleDistance; }
        //function for setting up visibility distance for maps on per-type/per-Id basis
        virtual void InitVisibilityDistance();

        void PlayerRelocation(Player *, float x, float y, float z, float angl);
        void CreatureRelocation(Creature *creature, float x, float y, float z, float orientation);

        template<class T, class CONTAINER> void Visit(const Cell& cell, TypeContainerVisitor<T, CONTAINER> &visitor);

        bool IsRemovalGrid(float x, float y) const
        {
            GridPair p = MaNGOS::ComputeGridPair(x, y);
            return( !getNGrid(p.x_coord, p.y_coord) || getNGrid(p.x_coord, p.y_coord)->GetGridState() == GRID_STATE_REMOVAL );
        }

        bool IsLoaded(float x, float y) const
        {
            GridPair p = MaNGOS::ComputeGridPair(x, y);
            return loaded(p);
        }

        bool GetUnloadLock(const GridPair &p) const { return getNGrid(p.x_coord, p.y_coord)->getUnloadLock(); }
        void SetUnloadLock(const GridPair &p, bool on) { getNGrid(p.x_coord, p.y_coord)->setUnloadExplicitLock(on); }
        void LoadGrid(const Cell& cell, bool no_unload = false);
        bool UnloadGrid(const uint32 &x, const uint32 &y, bool pForce);
        virtual void UnloadAll(bool pForce);

        void ResetGridExpiry(NGridType &grid, float factor = 1) const
        {
            grid.ResetTimeTracker((time_t)((float)i_gridExpiry*factor));
        }

        time_t GetGridExpiry(void) const { return i_gridExpiry; }
        uint32 GetId(void) const { return i_id; }

        // some calls like isInWater should not use vmaps due to processor power
        // can return INVALID_HEIGHT if under z+2 z coord not found height

        virtual void RemoveAllObjectsInRemoveList();

        bool CreatureRespawnRelocation(Creature *c);        // used only in CreatureRelocation and ObjectGridUnloader

        // ASSERT print helper
        bool CheckGridIntegrity(Creature* c, bool moved) const;

        uint32 GetInstanceId() const { return i_InstanceId; }
        virtual bool CanEnter(Player* /*player*/) { return true; }
        const char* GetMapName() const;

        // have meaning only for instanced map (that have set real difficulty), NOT USE its for BaseMap
        // _currently_ spawnmode == difficulty, but this can be changes later, so use appropriate spawmmode/difficult functions
        // for simplify later code support
        // regular difficulty = continent/dungeon normal/first raid normal difficulty
        uint8 GetSpawnMode() const { return (i_spawnMode); }
        Difficulty GetDifficulty() const { return Difficulty(GetSpawnMode()); }
        bool IsRegularDifficulty() const { return GetDifficulty() == REGULAR_DIFFICULTY; }
        uint32 GetMaxPlayers() const;                       // dependent from map difficulty
        uint32 GetMaxResetDelay() const;                    // dependent from map difficulty
        MapDifficulty const* GetMapDifficulty() const;      // dependent from map difficulty

        bool Instanceable() const { return i_mapEntry && i_mapEntry->Instanceable(); }
        // NOTE: this duplicate of Instanceable(), but Instanceable() can be changed when BG also will be instanceable
        bool IsDungeon() const { return i_mapEntry && i_mapEntry->IsDungeon(); }
        bool IsRaid() const { return i_mapEntry && i_mapEntry->IsRaid(); }
        bool IsRaidOrHeroicDungeon() const { return IsRaid() || GetDifficulty() > DUNGEON_DIFFICULTY_NORMAL; }
        bool IsBattleGround() const { return i_mapEntry && i_mapEntry->IsBattleGround(); }
        bool IsBattleArena() const { return i_mapEntry && i_mapEntry->IsBattleArena(); }
        bool IsBattleGroundOrArena() const { return i_mapEntry && i_mapEntry->IsBattleGroundOrArena(); }

        InstanceSave* GetInstanceSave() const { return m_instanceSave; }

        void AddObjectToRemoveList(WorldObject *obj);

        void UpdateObjectVisibility(WorldObject* obj, Cell cell, CellPair cellpair);

        void resetMarkedCells() { marked_cells.reset(); }
        bool isCellMarked(uint32 pCellId) { return marked_cells.test(pCellId); }
        void markCell(uint32 pCellId) { marked_cells.set(pCellId); }

        bool HavePlayers() const { return !m_mapRefManager.isEmpty(); }
        uint32 GetPlayersCountExceptGMs() const;
        bool ActiveObjectsNearGrid(uint32 x,uint32 y) const;

        void SendToPlayers(WorldPacket const* data) const;

        typedef MapRefManager PlayerList;
        PlayerList const& GetPlayers() const { return m_mapRefManager; }

        //per-map script storage
        void ScriptsStart(std::map<uint32, std::multimap<uint32, ScriptInfo> > const& scripts, uint32 id, Object* source, Object* target);
        void ScriptCommandStart(ScriptInfo const& script, uint32 delay, Object* source, Object* target);

        // must called with AddToWorld
        void AddToActive(WorldObject* obj);
        // must called with RemoveFromWorld
        void RemoveFromActive(WorldObject* obj);

        Player* GetPlayer(ObjectGuid guid);
        Creature* GetCreature(ObjectGuid guid);
        Pet* GetPet(ObjectGuid guid);
        Creature* GetAnyTypeCreature(ObjectGuid guid);      // normal creature or pet or vehicle
        GameObject* GetGameObject(ObjectGuid guid);
        DynamicObject* GetDynamicObject(ObjectGuid guid);
        Corpse* GetCorpse(ObjectGuid guid);                 // !!! find corpse can be not in world
        Unit* GetUnit(ObjectGuid guid);                     // only use if sure that need objects at current map, specially for player case
        WorldObject* GetWorldObject(ObjectGuid guid);       // only use if sure that need objects at current map, specially for player case

        TypeUnorderedMapContainer<AllMapStoredObjectTypes>& GetObjectsStore() { return m_objectsStore; }

        void AddUpdateObject(Object *obj)
        {
            i_objectsToClientUpdateQueue.push(obj);
        }

        void RemoveUpdateObject(Object *obj)
        {
            i_objectsToClientNotUpdate.insert(obj);
        }

        // DynObjects currently
        uint32 GenerateLocalLowGuid(HighGuid guidhigh);

        //get corresponding TerrainData object for this particular map
        const TerrainInfo * GetTerrain() const { return m_TerrainData; }

    private:
        void LoadMapAndVMap(int gx, int gy);

        void SetTimer(uint32 t) { i_gridExpiry = t < MIN_GRID_DELAY ? MIN_GRID_DELAY : t; }

        void SendInitSelf( Player * player );

        void SendInitTransports( Player * player );
        void SendRemoveTransports( Player * player );

        void PlayerRelocationNotify(Player* player, Cell cell, CellPair cellpair);

        bool CreatureCellRelocation(Creature *creature, Cell new_cell);

        bool loaded(const GridPair &) const;
        void EnsureGridCreated(const GridPair &);
        bool EnsureGridLoaded(Cell const&);
        void EnsureGridLoadedAtEnter(Cell const&, Player* player = NULL);

        void buildNGridLinkage(NGridType* pNGridType) { pNGridType->link(this); }

        template<class T> void AddType(T *obj);
        template<class T> void RemoveType(T *obj, bool);

        NGridType* getNGrid(uint32 x, uint32 y) const
        {
            MANGOS_ASSERT(x < MAX_NUMBER_OF_GRIDS);
            MANGOS_ASSERT(y < MAX_NUMBER_OF_GRIDS);
            return i_grids[x][y];
        }

        bool isGridObjectDataLoaded(uint32 x, uint32 y) const { return getNGrid(x,y)->isGridObjectDataLoaded(); }
        void setGridObjectDataLoaded(bool pLoaded, uint32 x, uint32 y) { getNGrid(x,y)->setGridObjectDataLoaded(pLoaded); }

        void setNGrid(NGridType* grid, uint32 x, uint32 y);
        void ScriptsProcess();

        void SendObjectUpdates();
        std::set<Object *> i_objectsToClientUpdate;
        std::set<Object *> i_objectsToClientNotUpdate;
        std::queue<Object*> i_objectsToClientUpdateQueue;

    protected:

        MapEntry const* i_mapEntry;
        uint8 i_spawnMode;
        uint32 i_id;
        uint32 i_InstanceId;
        uint32 m_unloadTimer;
        float m_VisibleDistance;
        InstanceSave* m_instanceSave;                       // can be NULL for non dungeons...

        MapRefManager m_mapRefManager;
        MapRefManager::iterator m_mapRefIter;

        typedef std::set<WorldObject*> ActiveNonPlayers;
        ActiveNonPlayers m_activeNonPlayers;
        ActiveNonPlayers::iterator m_activeNonPlayersIter;
        TypeUnorderedMapContainer<AllMapStoredObjectTypes> m_objectsStore;
    private:
        time_t i_gridExpiry;

        NGridType* i_grids[MAX_NUMBER_OF_GRIDS][MAX_NUMBER_OF_GRIDS];

        //Shared geodata object with map coord info...
        TerrainInfo * const m_TerrainData;
        bool m_bLoadedGrids[MAX_NUMBER_OF_GRIDS][MAX_NUMBER_OF_GRIDS];

        std::bitset<TOTAL_NUMBER_OF_CELLS_PER_MAP*TOTAL_NUMBER_OF_CELLS_PER_MAP> marked_cells;

        std::set<WorldObject *> i_objectsToRemove;
        std::multimap<time_t, ScriptAction> m_scriptSchedule;

        // Map local low guid counters
        ObjectGuidGenerator<HIGHGUID_DYNAMICOBJECT> m_DynObjectGuids;
        ObjectGuidGenerator<HIGHGUID_PET> m_PetGuids;

        // Type specific code for add/remove to/from grid
        template<class T>
            void AddToGrid(T*, NGridType *, Cell const&);

        template<class T>
            void AddNotifier(T*, Cell const&, CellPair const&);

        template<class T>
            void RemoveFromGrid(T*, NGridType *, Cell const&);
};

class MANGOS_DLL_SPEC InstanceMap : public Map
{
    public:
        InstanceMap(uint32 id, time_t, uint32 InstanceId, uint8 SpawnMode);
        ~InstanceMap();
        bool Add(Player *);
        void Remove(Player *, bool);
        void Update(const uint32&);
        void CreateInstanceData(bool load);
        bool Reset(InstanceResetMethod method);
        uint32 GetScriptId() const { return i_script_id; }
        InstanceData* GetInstanceData() { return i_data; }
        void PermBindAllPlayers(Player *player);
        void UnloadAll(bool pForce);
        bool CanEnter(Player* player);
        void SendResetWarnings(uint32 timeLeft) const;
        void SetResetSchedule(bool on);

        virtual void InitVisibilityDistance();
    private:
        bool m_resetAfterUnload;
        bool m_unloadWhenEmpty;
        InstanceData* i_data;
        uint32 i_script_id;
};

class MANGOS_DLL_SPEC BattleGroundMap : public Map
{
    public:
        BattleGroundMap(uint32 id, time_t, uint32 InstanceId, uint8 spawnMode);
        ~BattleGroundMap();

        void Update(const uint32&);
        bool Add(Player *);
        void Remove(Player *, bool);
        bool CanEnter(Player* player);
        void SetUnload();
        void UnloadAll(bool pForce);

        virtual void InitVisibilityDistance();
        BattleGround* GetBG() { return m_bg; }
        void SetBG(BattleGround* bg) { m_bg = bg; }
    private:
        BattleGround* m_bg;
};

template<class T, class CONTAINER>
inline void
Map::Visit(const Cell& cell, TypeContainerVisitor<T, CONTAINER> &visitor)
{
    const uint32 x = cell.GridX();
    const uint32 y = cell.GridY();
    const uint32 cell_x = cell.CellX();
    const uint32 cell_y = cell.CellY();

    if( !cell.NoCreate() || loaded(GridPair(x,y)) )
    {
        EnsureGridLoaded(cell);
        getNGrid(x, y)->Visit(cell_x, cell_y, visitor);
    }
}

#endif
