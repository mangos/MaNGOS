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

#include "Map.h"
#include "MapManager.h"
#include "Player.h"
#include "Vehicle.h"
#include "GridNotifiers.h"
#include "Log.h"
#include "GridStates.h"
#include "CellImpl.h"
#include "InstanceData.h"
#include "GridNotifiersImpl.h"
#include "Transports.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "World.h"
#include "ScriptCalls.h"
#include "Group.h"
#include "MapRefManager.h"
#include "DBCEnums.h"
#include "MapInstanced.h"
#include "InstanceSaveMgr.h"
#include "VMapFactory.h"
#include "BattleGroundMgr.h"

struct ScriptAction
{
    uint64 sourceGUID;
    uint64 targetGUID;
    uint64 ownerGUID;                                       // owner of source if source is item
    ScriptInfo const* script;                               // pointer to static script data
};

Map::~Map()
{
    ObjectAccessor::DelinkMap(this);
    UnloadAll(true);

    if(!m_scriptSchedule.empty())
        sWorld.DecreaseScheduledScriptCount(m_scriptSchedule.size());

    if (m_instanceSave)
        m_instanceSave->SetUsedByMapState(false);           // field pointer can be deleted after this
}

void Map::LoadVMap(int gx,int gy)
{
                                                            // x and y are swapped !!
    VMAP::VMAPLoadResult vmapLoadResult = VMAP::VMapFactory::createOrGetVMapManager()->loadMap((sWorld.GetDataPath()+ "vmaps").c_str(),  GetId(), gx,gy);
    switch(vmapLoadResult)
    {
        case VMAP::VMAP_LOAD_RESULT_OK:
            DETAIL_LOG("VMAP loaded name:%s, id:%d, x:%d, y:%d (vmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx,gy,gx,gy);
            break;
        case VMAP::VMAP_LOAD_RESULT_ERROR:
            DETAIL_LOG("Could not load VMAP name:%s, id:%d, x:%d, y:%d (vmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx,gy,gx,gy);
            break;
        case VMAP::VMAP_LOAD_RESULT_IGNORED:
            DEBUG_LOG("Ignored VMAP name:%s, id:%d, x:%d, y:%d (vmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx,gy,gx,gy);
            break;
    }
}

void Map::LoadMap(int gx,int gy, bool reload)
{
    if( i_InstanceId != 0 )
    {
        if(GridMaps[gx][gy])
            return;

        // load grid map for base map
        if (!m_parentMap->GridMaps[gx][gy])
            m_parentMap->EnsureGridCreated(GridPair(63-gx,63-gy));

        ((MapInstanced*)(m_parentMap))->AddGridMapReference(GridPair(gx,gy));
        GridMaps[gx][gy] = m_parentMap->GridMaps[gx][gy];
        return;
    }

    if(GridMaps[gx][gy] && !reload)
        return;

    //map already load, delete it before reloading (Is it necessary? Do we really need the ability the reload maps during runtime?)
    if(GridMaps[gx][gy])
    {
        DETAIL_LOG("Unloading already loaded map %u before reloading.",i_id);
        delete (GridMaps[gx][gy]);
        GridMaps[gx][gy]=NULL;
    }

    // map file name
    char *tmp=NULL;
    int len = sWorld.GetDataPath().length()+strlen("maps/%03u%02u%02u.map")+1;
    tmp = new char[len];
    snprintf(tmp, len, (char *)(sWorld.GetDataPath()+"maps/%03u%02u%02u.map").c_str(),i_id,gx,gy);
    DETAIL_LOG("Loading map %s",tmp);
    // loading data
    GridMaps[gx][gy] = new GridMap();
    if (!GridMaps[gx][gy]->loadData(tmp))
    {
        sLog.outError("Error load map file: \n %s\n", tmp);
    }
    delete [] tmp;
}

void Map::LoadMapAndVMap(int gx,int gy)
{
    LoadMap(gx,gy);
    if(i_InstanceId == 0)
        LoadVMap(gx, gy);                                   // Only load the data for the base map
}

Map::Map(uint32 id, time_t expiry, uint32 InstanceId, uint8 SpawnMode, Map* _parent)
  : i_mapEntry (sMapStore.LookupEntry(id)), i_spawnMode(SpawnMode),
  i_id(id), i_InstanceId(InstanceId), m_unloadTimer(0),
  m_VisibleDistance(DEFAULT_VISIBILITY_DISTANCE), m_instanceSave(NULL),
  m_activeNonPlayersIter(m_activeNonPlayers.end()),
  i_gridExpiry(expiry), m_parentMap(_parent ? _parent : this)
{
    for(unsigned int idx=0; idx < MAX_NUMBER_OF_GRIDS; ++idx)
    {
        for(unsigned int j=0; j < MAX_NUMBER_OF_GRIDS; ++j)
        {
            //z code
            GridMaps[idx][j] =NULL;
            setNGrid(NULL, idx, j);
        }
    }
    ObjectAccessor::LinkMap(this);

    //lets initialize visibility distance for map
    Map::InitVisibilityDistance();
}

void Map::InitVisibilityDistance()
{
    //init visibility for continents
    m_VisibleDistance = World::GetMaxVisibleDistanceOnContinents();
}

// Template specialization of utility methods
template<class T>
void Map::AddToGrid(T* obj, NGridType *grid, Cell const& cell)
{
    (*grid)(cell.CellX(), cell.CellY()).template AddGridObject<T>(obj);
}

template<>
void Map::AddToGrid(Player* obj, NGridType *grid, Cell const& cell)
{
    (*grid)(cell.CellX(), cell.CellY()).AddWorldObject(obj);
}

template<>
void Map::AddToGrid(Corpse *obj, NGridType *grid, Cell const& cell)
{
    // add to world object registry in grid
    if(obj->GetType()!=CORPSE_BONES)
    {
        (*grid)(cell.CellX(), cell.CellY()).AddWorldObject(obj);
    }
    // add to grid object store
    else
    {
        (*grid)(cell.CellX(), cell.CellY()).AddGridObject(obj);
    }
}

template<>
void Map::AddToGrid(Creature* obj, NGridType *grid, Cell const& cell)
{
    // add to world object registry in grid
    if(obj->isPet() || obj->isVehicle())
    {
        (*grid)(cell.CellX(), cell.CellY()).AddWorldObject<Creature>(obj);
        obj->SetCurrentCell(cell);
    }
    // add to grid object store
    else
    {
        (*grid)(cell.CellX(), cell.CellY()).AddGridObject<Creature>(obj);
        obj->SetCurrentCell(cell);
    }
}

template<class T>
void Map::RemoveFromGrid(T* obj, NGridType *grid, Cell const& cell)
{
    (*grid)(cell.CellX(), cell.CellY()).template RemoveGridObject<T>(obj);
}

template<>
void Map::RemoveFromGrid(Player* obj, NGridType *grid, Cell const& cell)
{
    (*grid)(cell.CellX(), cell.CellY()).RemoveWorldObject(obj);
}

template<>
void Map::RemoveFromGrid(Corpse *obj, NGridType *grid, Cell const& cell)
{
    // remove from world object registry in grid
    if(obj->GetType()!=CORPSE_BONES)
    {
        (*grid)(cell.CellX(), cell.CellY()).RemoveWorldObject(obj);
    }
    // remove from grid object store
    else
    {
        (*grid)(cell.CellX(), cell.CellY()).RemoveGridObject(obj);
    }
}

template<>
void Map::RemoveFromGrid(Creature* obj, NGridType *grid, Cell const& cell)
{
    // remove from world object registry in grid
    if(obj->isPet() || obj->isVehicle())
    {
        (*grid)(cell.CellX(), cell.CellY()).RemoveWorldObject<Creature>(obj);
    }
    // remove from grid object store
    else
    {
        (*grid)(cell.CellX(), cell.CellY()).RemoveGridObject<Creature>(obj);
    }
}

template<class T>
void Map::DeleteFromWorld(T* obj)
{
    // Note: In case resurrectable corpse and pet its removed from global lists in own destructor
    delete obj;
}

template<>
void Map::DeleteFromWorld(Player* pl)
{
    sObjectAccessor.RemoveObject(pl);
    delete pl;
}

template<class T>
void Map::AddNotifier(T* , Cell const& , CellPair const& )
{
}

template<>
void Map::AddNotifier(Player* obj, Cell const& cell, CellPair const& cellpair)
{
    PlayerRelocationNotify(obj,cell,cellpair);
}

template<>
void Map::AddNotifier(Creature* obj, Cell const&, CellPair const&)
{
    obj->SetNeedNotify();
}

void
Map::EnsureGridCreated(const GridPair &p)
{
    if(!getNGrid(p.x_coord, p.y_coord))
    {
        Guard guard(*this);
        if(!getNGrid(p.x_coord, p.y_coord))
        {
            setNGrid(new NGridType(p.x_coord*MAX_NUMBER_OF_GRIDS + p.y_coord, p.x_coord, p.y_coord, i_gridExpiry, sWorld.getConfig(CONFIG_BOOL_GRID_UNLOAD)),
                p.x_coord, p.y_coord);

            // build a linkage between this map and NGridType
            buildNGridLinkage(getNGrid(p.x_coord, p.y_coord));

            getNGrid(p.x_coord, p.y_coord)->SetGridState(GRID_STATE_IDLE);

            //z coord
            int gx = (MAX_NUMBER_OF_GRIDS - 1) - p.x_coord;
            int gy = (MAX_NUMBER_OF_GRIDS - 1) - p.y_coord;

            if(!GridMaps[gx][gy])
                LoadMapAndVMap(gx,gy);
        }
    }
}

void
Map::EnsureGridLoadedAtEnter(const Cell &cell, Player *player)
{
    NGridType *grid;

    if(EnsureGridLoaded(cell))
    {
        grid = getNGrid(cell.GridX(), cell.GridY());

        if (player)
        {
            DEBUG_FILTER_LOG(LOG_FILTER_PLAYER_MOVES, "Player %s enter cell[%u,%u] triggers of loading grid[%u,%u] on map %u", player->GetName(), cell.CellX(), cell.CellY(), cell.GridX(), cell.GridY(), i_id);
        }
        else
        {
            DEBUG_FILTER_LOG(LOG_FILTER_PLAYER_MOVES, "Active object nearby triggers of loading grid [%u,%u] on map %u", cell.GridX(), cell.GridY(), i_id);
        }

        ResetGridExpiry(*getNGrid(cell.GridX(), cell.GridY()), 0.1f);
        grid->SetGridState(GRID_STATE_ACTIVE);
    }
    else
        grid = getNGrid(cell.GridX(), cell.GridY());

    if (player)
        AddToGrid(player,grid,cell);
}

bool Map::EnsureGridLoaded(const Cell &cell)
{
    EnsureGridCreated(GridPair(cell.GridX(), cell.GridY()));
    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());

    MANGOS_ASSERT(grid != NULL);
    if( !isGridObjectDataLoaded(cell.GridX(), cell.GridY()) )
    {
        ObjectGridLoader loader(*grid, this, cell);
        loader.LoadN();

        // Add resurrectable corpses to world object list in grid
        sObjectAccessor.AddCorpsesToGrid(GridPair(cell.GridX(),cell.GridY()),(*grid)(cell.CellX(), cell.CellY()), this);

        setGridObjectDataLoaded(true,cell.GridX(), cell.GridY());
        return true;
    }

    return false;
}

void Map::LoadGrid(const Cell& cell, bool no_unload)
{
    EnsureGridLoaded(cell);

    if(no_unload)
        getNGrid(cell.GridX(), cell.GridY())->setUnloadExplicitLock(true);
}

bool Map::Add(Player *player)
{
    player->GetMapRef().link(this, player);
    player->SetMap(this);

    // update player state for other player and visa-versa
    CellPair p = MaNGOS::ComputeCellPair(player->GetPositionX(), player->GetPositionY());
    Cell cell(p);
    EnsureGridLoadedAtEnter(cell, player);
    player->AddToWorld();

    SendInitSelf(player);
    SendInitTransports(player);

    NGridType* grid = getNGrid(cell.GridX(), cell.GridY());
    player->GetViewPoint().Event_AddedToWorld(&(*grid)(cell.CellX(), cell.CellY()));
    UpdateObjectVisibility(player,cell,p);

    AddNotifier(player,cell,p);
    return true;
}

template<class T>
void
Map::Add(T *obj)
{
    MANGOS_ASSERT(obj);

    CellPair p = MaNGOS::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP )
    {
        sLog.outError("Map::Add: Object (GUID: %u TypeId: %u) have invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUIDLow(), obj->GetTypeId(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    obj->SetMap(this);

    Cell cell(p);
    if(obj->isActiveObject())
        EnsureGridLoadedAtEnter(cell);
    else
        EnsureGridCreated(GridPair(cell.GridX(), cell.GridY()));

    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());
    MANGOS_ASSERT( grid != NULL );

    AddToGrid(obj,grid,cell);
    obj->AddToWorld();

    if(obj->isActiveObject())
        AddToActive(obj);

    DEBUG_LOG("Object %u enters grid[%u,%u]", GUID_LOPART(obj->GetGUID()), cell.GridX(), cell.GridY());

    obj->GetViewPoint().Event_AddedToWorld(&(*grid)(cell.CellX(), cell.CellY()));
    UpdateObjectVisibility(obj,cell,p);

    AddNotifier(obj,cell,p);
}

void Map::MessageBroadcast(Player *player, WorldPacket *msg, bool to_self)
{
    CellPair p = MaNGOS::ComputeCellPair(player->GetPositionX(), player->GetPositionY());

    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP )
    {
        sLog.outError("Map::MessageBroadcast: Player (GUID: %u) have invalid coordinates X:%f Y:%f grid cell [%u:%u]", player->GetGUIDLow(), player->GetPositionX(), player->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    if( !loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)) )
        return;

    MaNGOS::MessageDeliverer post_man(*player, msg, to_self);
    TypeContainerVisitor<MaNGOS::MessageDeliverer, WorldTypeMapContainer > message(post_man);
    cell.Visit(p, message, *this, *player, GetVisibilityDistance());
}

void Map::MessageBroadcast(WorldObject *obj, WorldPacket *msg)
{
    CellPair p = MaNGOS::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());

    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP )
    {
        sLog.outError("Map::MessageBroadcast: Object (GUID: %u TypeId: %u) have invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUIDLow(), obj->GetTypeId(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    if( !loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)) )
        return;

    //TODO: currently on continents when Visibility.Distance.InFlight > Visibility.Distance.Continents
    //we have alot of blinking mobs because monster move packet send is broken...
    MaNGOS::ObjectMessageDeliverer post_man(*obj,msg);
    TypeContainerVisitor<MaNGOS::ObjectMessageDeliverer, WorldTypeMapContainer > message(post_man);
    cell.Visit(p, message, *this, *obj, GetVisibilityDistance());
}

void Map::MessageDistBroadcast(Player *player, WorldPacket *msg, float dist, bool to_self, bool own_team_only)
{
    CellPair p = MaNGOS::ComputeCellPair(player->GetPositionX(), player->GetPositionY());

    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP )
    {
        sLog.outError("Map::MessageBroadcast: Player (GUID: %u) have invalid coordinates X:%f Y:%f grid cell [%u:%u]", player->GetGUIDLow(), player->GetPositionX(), player->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    if( !loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)) )
        return;

    MaNGOS::MessageDistDeliverer post_man(*player, msg, dist, to_self, own_team_only);
    TypeContainerVisitor<MaNGOS::MessageDistDeliverer , WorldTypeMapContainer > message(post_man);
    cell.Visit(p, message, *this, *player, dist);
}

void Map::MessageDistBroadcast(WorldObject *obj, WorldPacket *msg, float dist)
{
    CellPair p = MaNGOS::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());

    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP )
    {
        sLog.outError("Map::MessageBroadcast: Object (GUID: %u TypeId: %u) have invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUIDLow(), obj->GetTypeId(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    if( !loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)) )
        return;

    MaNGOS::ObjectMessageDistDeliverer post_man(*obj, msg, dist);
    TypeContainerVisitor<MaNGOS::ObjectMessageDistDeliverer, WorldTypeMapContainer > message(post_man);
    cell.Visit(p, message, *this, *obj, dist);
}

bool Map::loaded(const GridPair &p) const
{
    return ( getNGrid(p.x_coord, p.y_coord) && isGridObjectDataLoaded(p.x_coord, p.y_coord) );
}

void Map::Update(const uint32 &t_diff)
{
    /// update players at tick
    for(m_mapRefIter = m_mapRefManager.begin(); m_mapRefIter != m_mapRefManager.end(); ++m_mapRefIter)
    {
        Player* plr = m_mapRefIter->getSource();
        if(plr && plr->IsInWorld())
            plr->Update(t_diff);
    }

    /// update active cells around players and active objects
    resetMarkedCells();

    MaNGOS::ObjectUpdater updater(t_diff);
    // for creature
    TypeContainerVisitor<MaNGOS::ObjectUpdater, GridTypeMapContainer  > grid_object_update(updater);
    // for pets
    TypeContainerVisitor<MaNGOS::ObjectUpdater, WorldTypeMapContainer > world_object_update(updater);

    // the player iterator is stored in the map object
    // to make sure calls to Map::Remove don't invalidate it
    for(m_mapRefIter = m_mapRefManager.begin(); m_mapRefIter != m_mapRefManager.end(); ++m_mapRefIter)
    {
        Player* plr = m_mapRefIter->getSource();

        if(!plr->IsInWorld())
            continue;

        CellPair standing_cell(MaNGOS::ComputeCellPair(plr->GetPositionX(), plr->GetPositionY()));

        // Check for correctness of standing_cell, it also avoids problems with update_cell
        if (standing_cell.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || standing_cell.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
            continue;

        // the overloaded operators handle range checking
        // so ther's no need for range checking inside the loop
        CellPair begin_cell(standing_cell), end_cell(standing_cell);
        //lets update mobs/objects in ALL visible cells around player!
        CellArea area = Cell::CalculateCellArea(*plr, GetVisibilityDistance());
        area.ResizeBorders(begin_cell, end_cell);

        for(uint32 x = begin_cell.x_coord; x <= end_cell.x_coord; ++x)
        {
            for(uint32 y = begin_cell.y_coord; y <= end_cell.y_coord; ++y)
            {
                // marked cells are those that have been visited
                // don't visit the same cell twice
                uint32 cell_id = (y * TOTAL_NUMBER_OF_CELLS_PER_MAP) + x;
                if(!isCellMarked(cell_id))
                {
                    markCell(cell_id);
                    CellPair pair(x,y);
                    Cell cell(pair);
                    cell.data.Part.reserved = CENTER_DISTRICT;
                    cell.SetNoCreate();
                    cell.Visit(pair, grid_object_update,  *this);
                    cell.Visit(pair, world_object_update, *this);
                }
            }
        }
    }

    // non-player active objects
    if(!m_activeNonPlayers.empty())
    {
        for(m_activeNonPlayersIter = m_activeNonPlayers.begin(); m_activeNonPlayersIter != m_activeNonPlayers.end(); )
        {
            // skip not in world
            WorldObject* obj = *m_activeNonPlayersIter;

            // step before processing, in this case if Map::Remove remove next object we correctly
            // step to next-next, and if we step to end() then newly added objects can wait next update.
            ++m_activeNonPlayersIter;

            if(!obj->IsInWorld())
                continue;

            CellPair standing_cell(MaNGOS::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY()));

            // Check for correctness of standing_cell, it also avoids problems with update_cell
            if (standing_cell.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || standing_cell.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
                continue;

            // the overloaded operators handle range checking
            // so ther's no need for range checking inside the loop
            CellPair begin_cell(standing_cell), end_cell(standing_cell);
            begin_cell << 1; begin_cell -= 1;               // upper left
            end_cell >> 1; end_cell += 1;                   // lower right

            for(uint32 x = begin_cell.x_coord; x <= end_cell.x_coord; ++x)
            {
                for(uint32 y = begin_cell.y_coord; y <= end_cell.y_coord; ++y)
                {
                    // marked cells are those that have been visited
                    // don't visit the same cell twice
                    uint32 cell_id = (y * TOTAL_NUMBER_OF_CELLS_PER_MAP) + x;
                    if(!isCellMarked(cell_id))
                    {
                        markCell(cell_id);
                        CellPair pair(x,y);
                        Cell cell(pair);
                        cell.data.Part.reserved = CENTER_DISTRICT;
                        cell.SetNoCreate();
                        cell.Visit(pair, grid_object_update,  *this);
                        cell.Visit(pair, world_object_update, *this);
                    }
                }
            }
        }
    }

    // Send world objects and item update field changes
    SendObjectUpdates();

    // Don't unload grids if it's battleground, since we may have manually added GOs,creatures, those doesn't load from DB at grid re-load !
    // This isn't really bother us, since as soon as we have instanced BG-s, the whole map unloads as the BG gets ended
    if (!IsBattleGroundOrArena())
    {
        for (GridRefManager<NGridType>::iterator i = GridRefManager<NGridType>::begin(); i != GridRefManager<NGridType>::end(); )
        {
            NGridType *grid = i->getSource();
            GridInfo *info = i->getSource()->getGridInfoRef();
            ++i;                                                // The update might delete the map and we need the next map before the iterator gets invalid
            MANGOS_ASSERT(grid->GetGridState() >= 0 && grid->GetGridState() < MAX_GRID_STATE);
            sMapMgr.UpdateGridState(grid->GetGridState(), *this, *grid, *info, grid->getX(), grid->getY(), t_diff);
        }
    }

    ///- Process necessary scripts
    if (!m_scriptSchedule.empty())
        ScriptsProcess();
}

void Map::Remove(Player *player, bool remove)
{
    if(remove)
        player->CleanupsBeforeDelete();
    else
        player->RemoveFromWorld();

    // this may be called during Map::Update
    // after decrement+unlink, ++m_mapRefIter will continue correctly
    // when the first element of the list is being removed
    // nocheck_prev will return the padding element of the RefManager
    // instead of NULL in the case of prev
    if(m_mapRefIter == player->GetMapRef())
        m_mapRefIter = m_mapRefIter->nocheck_prev();
    player->GetMapRef().unlink();
    CellPair p = MaNGOS::ComputeCellPair(player->GetPositionX(), player->GetPositionY());
    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        // invalid coordinates
        player->ResetMap();

        if( remove )
            DeleteFromWorld(player);

        return;
    }

    Cell cell(p);

    if( !getNGrid(cell.data.Part.grid_x, cell.data.Part.grid_y) )
    {
        sLog.outError("Map::Remove() i_grids was NULL x:%d, y:%d",cell.data.Part.grid_x,cell.data.Part.grid_y);
        return;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_PLAYER_MOVES, "Remove player %s from grid[%u,%u]", player->GetName(), cell.GridX(), cell.GridY());
    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());
    MANGOS_ASSERT(grid != NULL);

    RemoveFromGrid(player,grid,cell);

    SendRemoveTransports(player);
    UpdateObjectVisibility(player,cell,p);

    player->ResetMap();
    if( remove )
        DeleteFromWorld(player);
}

template<class T>
void
Map::Remove(T *obj, bool remove)
{
    CellPair p = MaNGOS::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
    if(p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP )
    {
        sLog.outError("Map::Remove: Object (GUID: %u TypeId:%u) have invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUIDLow(), obj->GetTypeId(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    if( !loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)) )
        return;

    DEBUG_LOG("Remove object (GUID: %u TypeId:%u) from grid[%u,%u]", obj->GetGUIDLow(), obj->GetTypeId(), cell.data.Part.grid_x, cell.data.Part.grid_y);
    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());
    MANGOS_ASSERT( grid != NULL );

    if(obj->isActiveObject())
        RemoveFromActive(obj);

    if(remove)
        obj->CleanupsBeforeDelete();
    else
        obj->RemoveFromWorld();

    UpdateObjectVisibility(obj,cell,p); // i think will be better to call this function while object still in grid, this changes nothing but logically is better(as for me)
    RemoveFromGrid(obj,grid,cell);

    obj->ResetMap();
    if( remove )
    {
        // if option set then object already saved at this moment
        if(!sWorld.getConfig(CONFIG_BOOL_SAVE_RESPAWN_TIME_IMMEDIATLY))
            obj->SaveRespawnTime();
        DeleteFromWorld(obj);
    }
}

void
Map::PlayerRelocation(Player *player, float x, float y, float z, float orientation)
{
    MANGOS_ASSERT(player);

    CellPair old_val = MaNGOS::ComputeCellPair(player->GetPositionX(), player->GetPositionY());
    CellPair new_val = MaNGOS::ComputeCellPair(x, y);

    Cell old_cell(old_val);
    Cell new_cell(new_val);
    new_cell |= old_cell;
    bool same_cell = (new_cell == old_cell);

    player->Relocate(x, y, z, orientation);

    if( old_cell.DiffGrid(new_cell) || old_cell.DiffCell(new_cell) )
    {
        DEBUG_FILTER_LOG(LOG_FILTER_PLAYER_MOVES, "Player %s relocation grid[%u,%u]cell[%u,%u]->grid[%u,%u]cell[%u,%u]", player->GetName(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());

        // update player position for group at taxi flight
        if(player->GetGroup() && player->IsTaxiFlying())
            player->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_POSITION);

        NGridType* oldGrid = getNGrid(old_cell.GridX(), old_cell.GridY());
        RemoveFromGrid(player, oldGrid,old_cell);
        if( !old_cell.DiffGrid(new_cell) )
            AddToGrid(player, oldGrid,new_cell);
        else
            EnsureGridLoadedAtEnter(new_cell, player);

        NGridType* newGrid = getNGrid(new_cell.GridX(), new_cell.GridY());
        player->GetViewPoint().Event_GridChanged(&(*newGrid)(new_cell.CellX(),new_cell.CellY()));
    }

    player->GetViewPoint().Call_UpdateVisibilityForOwner();
    // if move then update what player see and who seen
    UpdateObjectVisibility(player, new_cell, new_val);
    PlayerRelocationNotify(player,new_cell,new_val);

    NGridType* newGrid = getNGrid(new_cell.GridX(), new_cell.GridY());
    if( !same_cell && newGrid->GetGridState()!= GRID_STATE_ACTIVE )
    {
        ResetGridExpiry(*newGrid, 0.1f);
        newGrid->SetGridState(GRID_STATE_ACTIVE);
    }
}

void
Map::CreatureRelocation(Creature *creature, float x, float y, float z, float ang)
{
    MANGOS_ASSERT(CheckGridIntegrity(creature,false));

    Cell old_cell = creature->GetCurrentCell();

    CellPair new_val = MaNGOS::ComputeCellPair(x, y);
    Cell new_cell(new_val);

    // delay creature move for grid/cell to grid/cell moves
    if (old_cell.DiffCell(new_cell) || old_cell.DiffGrid(new_cell))
    {
        DEBUG_FILTER_LOG(LOG_FILTER_CREATURE_MOVES, "Creature (GUID: %u Entry: %u) added to moving list from grid[%u,%u]cell[%u,%u] to grid[%u,%u]cell[%u,%u].", creature->GetGUIDLow(), creature->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());

        // do move or do move to respawn or remove creature if previous all fail
        if(CreatureCellRelocation(creature,new_cell))
        {
            // update pos
            creature->Relocate(x, y, z, ang);

            // in diffcell/diffgrid case notifiers called in Creature::Update
            creature->SetNeedNotify();
        }
        else
        {
            // if creature can't be move in new cell/grid (not loaded) move it to repawn cell/grid
            // creature coordinates will be updated and notifiers send
            if(!CreatureRespawnRelocation(creature))
            {
                // ... or unload (if respawn grid also not loaded)
                DEBUG_FILTER_LOG(LOG_FILTER_CREATURE_MOVES, "Creature (GUID: %u Entry: %u ) can't be move to unloaded respawn grid.",creature->GetGUIDLow(),creature->GetEntry());
                creature->SetNeedNotify();
            }
        }
    }
    else
    {
        creature->Relocate(x, y, z, ang);
        creature->SetNeedNotify();
    }

    creature->GetViewPoint().Call_UpdateVisibilityForOwner();
    MANGOS_ASSERT(CheckGridIntegrity(creature,true));
}

bool Map::CreatureCellRelocation(Creature *c, Cell new_cell)
{
    Cell const& old_cell = c->GetCurrentCell();
    if(!old_cell.DiffGrid(new_cell) )                       // in same grid
    {
        // if in same cell then none do
        if(old_cell.DiffCell(new_cell))
        {
            DEBUG_FILTER_LOG(LOG_FILTER_CREATURE_MOVES, "Creature (GUID: %u Entry: %u) moved in grid[%u,%u] from cell[%u,%u] to cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.CellX(), new_cell.CellY());

            RemoveFromGrid(c,getNGrid(old_cell.GridX(), old_cell.GridY()),old_cell);

            NGridType* new_grid = getNGrid(new_cell.GridX(), new_cell.GridY());
            AddToGrid(c,new_grid,new_cell);

            c->GetViewPoint().Event_GridChanged( &(*new_grid)(new_cell.CellX(),new_cell.CellY()) );
        }
        else
        {
            DEBUG_FILTER_LOG(LOG_FILTER_CREATURE_MOVES, "Creature (GUID: %u Entry: %u) move in same grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY());
        }

        return true;
    }

    // in diff. grids but active creature
    if(c->isActiveObject())
    {
        EnsureGridLoadedAtEnter(new_cell);

        DEBUG_FILTER_LOG(LOG_FILTER_CREATURE_MOVES, "Active creature (GUID: %u Entry: %u) moved from grid[%u,%u]cell[%u,%u] to grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());

        RemoveFromGrid(c,getNGrid(old_cell.GridX(), old_cell.GridY()),old_cell);

        NGridType* new_grid = getNGrid(new_cell.GridX(), new_cell.GridY());
        AddToGrid(c,new_grid,new_cell);
        c->GetViewPoint().Event_GridChanged( &(*new_grid)(new_cell.CellX(),new_cell.CellY()) );

        return true;
    }

    // in diff. loaded grid normal creature
    if(loaded(GridPair(new_cell.GridX(), new_cell.GridY())))
    {
        DEBUG_FILTER_LOG(LOG_FILTER_CREATURE_MOVES, "Creature (GUID: %u Entry: %u) moved from grid[%u,%u]cell[%u,%u] to grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());

        RemoveFromGrid(c,getNGrid(old_cell.GridX(), old_cell.GridY()),old_cell);
        {
            EnsureGridCreated(GridPair(new_cell.GridX(), new_cell.GridY()));
            NGridType* new_grid = getNGrid(new_cell.GridX(), new_cell.GridY());
            AddToGrid(c,new_grid,new_cell);
            c->GetViewPoint().Event_GridChanged( &(*new_grid)(new_cell.CellX(),new_cell.CellY()) );
        }

        return true;
    }

    // fail to move: normal creature attempt move to unloaded grid
    DEBUG_FILTER_LOG(LOG_FILTER_CREATURE_MOVES, "Creature (GUID: %u Entry: %u) attempt move from grid[%u,%u]cell[%u,%u] to unloaded grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
    return false;
}

bool Map::CreatureRespawnRelocation(Creature *c)
{
    float resp_x, resp_y, resp_z, resp_o;
    c->GetRespawnCoord(resp_x, resp_y, resp_z, &resp_o);

    CellPair resp_val = MaNGOS::ComputeCellPair(resp_x, resp_y);
    Cell resp_cell(resp_val);

    c->CombatStop();
    c->GetMotionMaster()->Clear();

    DEBUG_FILTER_LOG(LOG_FILTER_CREATURE_MOVES, "Creature (GUID: %u Entry: %u) will moved from grid[%u,%u]cell[%u,%u] to respawn grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), c->GetCurrentCell().GridX(), c->GetCurrentCell().GridY(), c->GetCurrentCell().CellX(), c->GetCurrentCell().CellY(), resp_cell.GridX(), resp_cell.GridY(), resp_cell.CellX(), resp_cell.CellY());

    // teleport it to respawn point (like normal respawn if player see)
    if(CreatureCellRelocation(c,resp_cell))
    {
        c->Relocate(resp_x, resp_y, resp_z, resp_o);
        c->GetMotionMaster()->Initialize();                 // prevent possible problems with default move generators
        c->SetNeedNotify();
        return true;
    }
    else
        return false;
}

bool Map::UnloadGrid(const uint32 &x, const uint32 &y, bool pForce)
{
    NGridType *grid = getNGrid(x, y);
    MANGOS_ASSERT( grid != NULL);

    {
        if(!pForce && ActiveObjectsNearGrid(x, y) )
            return false;

        DEBUG_LOG("Unloading grid[%u,%u] for map %u", x,y, i_id);
        ObjectGridUnloader unloader(*grid);

        // Finish remove and delete all creatures with delayed remove before moving to respawn grids
        // Must know real mob position before move
        RemoveAllObjectsInRemoveList();

        // move creatures to respawn grids if this is diff.grid or to remove list
        unloader.MoveToRespawnN();

        // Finish remove and delete all creatures with delayed remove before unload
        RemoveAllObjectsInRemoveList();

        unloader.UnloadN();
        delete getNGrid(x, y);
        setNGrid(NULL, x, y);
    }

    int gx = (MAX_NUMBER_OF_GRIDS - 1) - x;
    int gy = (MAX_NUMBER_OF_GRIDS - 1) - y;

    // delete grid map, but don't delete if it is from parent map (and thus only reference)
    //+++if (GridMaps[gx][gy]) don't check for GridMaps[gx][gy], we might have to unload vmaps
    {
        if (i_InstanceId == 0)
        {
            if(GridMaps[gx][gy])
            {
                GridMaps[gx][gy]->unloadData();
                delete GridMaps[gx][gy];
            }
            VMAP::VMapFactory::createOrGetVMapManager()->unloadMap(GetId(), gx, gy);
        }
        else
            ((MapInstanced*)m_parentMap)->RemoveGridMapReference(GridPair(gx, gy));

        GridMaps[gx][gy] = NULL;
    }
    DEBUG_LOG("Unloading grid[%u,%u] for map %u finished", x,y, i_id);
    return true;
}

void Map::UnloadAll(bool pForce)
{
    for (GridRefManager<NGridType>::iterator i = GridRefManager<NGridType>::begin(); i != GridRefManager<NGridType>::end(); )
    {
        NGridType &grid(*i->getSource());
        ++i;
        UnloadGrid(grid.getX(), grid.getY(), pForce);       // deletes the grid and removes it from the GridRefManager
    }
}

MapDifficulty const* Map::GetMapDifficulty() const
{
    return GetMapDifficultyData(GetId(),GetDifficulty());
}

uint32 Map::GetMaxPlayers() const
{
    if(MapDifficulty const* mapDiff = GetMapDifficulty())
    {
        if(mapDiff->maxPlayers || IsRegularDifficulty())    // Normal case (expect that regular difficulty always have correct maxplayers)
            return mapDiff->maxPlayers;
        else                                                // DBC have 0 maxplayers for heroic instances with expansion < 2
        {                                                   // The heroic entry exists, so we don't have to check anything, simply return normal max players
            MapDifficulty const* normalDiff = GetMapDifficultyData(i_id, REGULAR_DIFFICULTY);
            return normalDiff ? normalDiff->maxPlayers : 0;
        }
    }
    else                                                    // I'd rather ASSERT(false);
        return 0;
}

uint32 Map::GetMaxResetDelay() const
{
    return InstanceResetScheduler::GetMaxResetTimeFor(GetMapDifficulty());
}

inline GridMap *Map::GetGrid(float x, float y)
{
    // half opt method
    int gx=(int)(32-x/SIZE_OF_GRIDS);                       //grid x
    int gy=(int)(32-y/SIZE_OF_GRIDS);                       //grid y

    // ensure GridMap is loaded
    EnsureGridCreated(GridPair(63-gx,63-gy));

    return GridMaps[gx][gy];
}

float Map::GetHeight(float x, float y, float z, bool pUseVmaps, float maxSearchDist) const
{
    // find raw .map surface under Z coordinates
    float mapHeight;
    float z2 = z + 2.f;
    if (GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
    {
        float _mapheight = gmap->getHeight(x,y);

        // look from a bit higher pos to find the floor, ignore under surface case
        if (z2 > _mapheight)
            mapHeight = _mapheight;
        else
            mapHeight = VMAP_INVALID_HEIGHT_VALUE;
    }
    else
        mapHeight = VMAP_INVALID_HEIGHT_VALUE;

    float vmapHeight;
    if (pUseVmaps)
    {
        VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
        if (vmgr->isHeightCalcEnabled())
        {
            // if mapHeight has been found search vmap height at least until mapHeight point
            // this prevent case when original Z "too high above ground and vmap height search fail"
            // this will not affect most normal cases (no map in instance, or stay at ground at continent)
            if (mapHeight > INVALID_HEIGHT && z2 - mapHeight > maxSearchDist)
                maxSearchDist = z2 - mapHeight + 1.0f;      // 1.0 make sure that we not fail for case when map height near but above for vamp height

            // look from a bit higher pos to find the floor
            vmapHeight = vmgr->getHeight(GetId(), x, y, z2, maxSearchDist);
        }
        else
            vmapHeight = VMAP_INVALID_HEIGHT_VALUE;
    }
    else
        vmapHeight = VMAP_INVALID_HEIGHT_VALUE;

    // mapHeight set for any above raw ground Z or <= INVALID_HEIGHT
    // vmapheight set for any under Z value or <= INVALID_HEIGHT

    if (vmapHeight > INVALID_HEIGHT)
    {
        if (mapHeight > INVALID_HEIGHT)
        {
            // we have mapheight and vmapheight and must select more appropriate

            // we are already under the surface or vmap height above map heigt
            // or if the distance of the vmap height is less the land height distance
            if (z < mapHeight || vmapHeight > mapHeight || fabs(mapHeight-z) > fabs(vmapHeight-z))
                return vmapHeight;
            else
                return mapHeight;                           // better use .map surface height

        }
        else
            return vmapHeight;                              // we have only vmapHeight (if have)
    }

    return mapHeight;
}

inline bool IsOutdoorWMO(uint32 mogpFlags, int32 adtId, int32 rootId, int32 groupId,
                              WMOAreaTableEntry const* wmoEntry, AreaTableEntry const* atEntry)
{
    bool outdoor = true;

    if(wmoEntry && atEntry)
    {
        if(atEntry->flags & AREA_FLAG_OUTSIDE)
            return true;
        if(atEntry->flags & AREA_FLAG_INSIDE)
            return false;
    }

    outdoor = mogpFlags&0x8;

    if(wmoEntry)
    {
        if(wmoEntry->Flags & 4)
            return true;

        if((wmoEntry->Flags & 2)!=0)
            outdoor = false;
    }
    return outdoor;
}

bool Map::IsOutdoors(float x, float y, float z) const
{
    uint32 mogpFlags;
    int32 adtId, rootId, groupId;

    // no wmo found? -> outside by default
    if(!GetAreaInfo(x, y, z, mogpFlags, adtId, rootId, groupId))
        return true;

    AreaTableEntry const* atEntry = 0;
    WMOAreaTableEntry const* wmoEntry= GetWMOAreaTableEntryByTripple(rootId, adtId, groupId);
    if(wmoEntry)
    {
        DEBUG_LOG("Got WMOAreaTableEntry! flag %u, areaid %u", wmoEntry->Flags, wmoEntry->areaId);

        atEntry = GetAreaEntryByAreaID(wmoEntry->areaId);
    }

    return IsOutdoorWMO(mogpFlags, adtId, rootId, groupId, wmoEntry, atEntry);
}

bool Map::GetAreaInfo(float x, float y, float z, uint32 &flags, int32 &adtId, int32 &rootId, int32 &groupId) const
{
    float vmap_z = z;
    VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
    if (vmgr->getAreaInfo(GetId(), x, y, vmap_z, flags, adtId, rootId, groupId))
    {
        // check if there's terrain between player height and object height
        if(GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
        {
            float _mapheight = gmap->getHeight(x,y);
            // z + 2.0f condition taken from GetHeight(), not sure if it's such a great choice...
            if(z + 2.0f > _mapheight &&  _mapheight > vmap_z)
                return false;
        }
        return true;
    }
    return false;
}

uint16 Map::GetAreaFlag(float x, float y, float z, bool *isOutdoors) const
{
    uint32 mogpFlags;
    int32 adtId, rootId, groupId;
    WMOAreaTableEntry const* wmoEntry = 0;
    AreaTableEntry const* atEntry = 0;
    bool haveAreaInfo = false;

    if(GetAreaInfo(x, y, z, mogpFlags, adtId, rootId, groupId))
    {
        haveAreaInfo = true;
        if(wmoEntry = GetWMOAreaTableEntryByTripple(rootId, adtId, groupId))
            atEntry = GetAreaEntryByAreaID(wmoEntry->areaId);
    }

    uint16 areaflag;
    if (atEntry)
        areaflag = atEntry->exploreFlag;
    else
    {
        if(GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
            areaflag = gmap->getArea(x, y);
        // this used while not all *.map files generated (instances)
        else
            areaflag = GetAreaFlagByMapId(i_id);
    }

    if (isOutdoors)
    {
        if (haveAreaInfo)
            *isOutdoors = IsOutdoorWMO(mogpFlags, adtId, rootId, groupId, wmoEntry, atEntry);
        else
            *isOutdoors = true;
    }
    return areaflag;
}

uint8 Map::GetTerrainType(float x, float y ) const
{
    if(GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
        return gmap->getTerrainType(x, y);
    else
        return 0;
}

GridMapLiquidStatus Map::getLiquidStatus(float x, float y, float z, uint8 ReqLiquidType, GridMapLiquidData *data) const
{
    GridMapLiquidStatus result = LIQUID_MAP_NO_WATER;
    VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
    float liquid_level, ground_level = INVALID_HEIGHT;
    uint32 liquid_type;
    if (vmgr->GetLiquidLevel(GetId(), x, y, z, ReqLiquidType, liquid_level, ground_level, liquid_type))
    {
        DEBUG_LOG("getLiquidStatus(): vmap liquid level: %f ground: %f type: %u", liquid_level, ground_level, liquid_type);
        // Check water level and ground level
        if (liquid_level > ground_level && z > ground_level - 2)
        {
            // All ok in water -> store data
            if (data)
            {
                data->type  = liquid_type;
                data->level = liquid_level;
                data->depth_level = ground_level;
            }

            // For speed check as int values
            int delta = int((liquid_level - z) * 10);

            // Get position delta
            if (delta > 20)                   // Under water
                return LIQUID_MAP_UNDER_WATER;
            if (delta > 0 )                   // In water
                return LIQUID_MAP_IN_WATER;
            if (delta > -1)                   // Walk on water
                return LIQUID_MAP_WATER_WALK;
            result = LIQUID_MAP_ABOVE_WATER;
        }
    }
    if(GridMap* gmap = const_cast<Map*>(this)->GetGrid(x, y))
    {
        GridMapLiquidData map_data;
        GridMapLiquidStatus map_result = gmap->getLiquidStatus(x, y, z, ReqLiquidType, &map_data);
        // Not override LIQUID_MAP_ABOVE_WATER with LIQUID_MAP_NO_WATER:
        if (map_result != LIQUID_MAP_NO_WATER && (map_data.level > ground_level))
        {
            if (data)
                *data = map_data;
            return map_result;
        }
    }
    return result;
}

float Map::GetWaterLevel(float x, float y ) const
{
    if(GridMap* gmap = const_cast<Map*>(this)->GetGrid(x, y))
        return gmap->getLiquidLevel(x, y);
    else
        return 0;
}

uint32 Map::GetAreaIdByAreaFlag(uint16 areaflag,uint32 map_id)
{
    AreaTableEntry const *entry = GetAreaEntryByAreaFlagAndMap(areaflag,map_id);

    if (entry)
        return entry->ID;
    else
        return 0;
}

uint32 Map::GetZoneIdByAreaFlag(uint16 areaflag,uint32 map_id)
{
    AreaTableEntry const *entry = GetAreaEntryByAreaFlagAndMap(areaflag,map_id);

    if( entry )
        return ( entry->zone != 0 ) ? entry->zone : entry->ID;
    else
        return 0;
}

void Map::GetZoneAndAreaIdByAreaFlag(uint32& zoneid, uint32& areaid, uint16 areaflag,uint32 map_id)
{
    AreaTableEntry const *entry = GetAreaEntryByAreaFlagAndMap(areaflag,map_id);

    areaid = entry ? entry->ID : 0;
    zoneid = entry ? (( entry->zone != 0 ) ? entry->zone : entry->ID) : 0;
}

bool Map::IsInWater(float x, float y, float pZ, GridMapLiquidData *data) const
{
    // Check surface in x, y point for liquid
    if (const_cast<Map*>(this)->GetGrid(x, y))
    {
        GridMapLiquidData liquid_status;
        GridMapLiquidData *liquid_ptr = data ? data : &liquid_status;
        if (getLiquidStatus(x, y, pZ, MAP_ALL_LIQUIDS, liquid_ptr))
        {
            //if (liquid_prt->level - liquid_prt->depth_level > 2) //???
                return true;
        }
    }
    return false;
}

bool Map::IsUnderWater(float x, float y, float z) const
{
    if (const_cast<Map*>(this)->GetGrid(x, y))
    {
        if (getLiquidStatus(x, y, z, MAP_LIQUID_TYPE_WATER|MAP_LIQUID_TYPE_OCEAN)&LIQUID_MAP_UNDER_WATER)
            return true;
    }
    return false;
}

bool Map::CheckGridIntegrity(Creature* c, bool moved) const
{
    Cell const& cur_cell = c->GetCurrentCell();

    CellPair xy_val = MaNGOS::ComputeCellPair(c->GetPositionX(), c->GetPositionY());
    Cell xy_cell(xy_val);
    if(xy_cell != cur_cell)
    {
        sLog.outError("Creature (GUIDLow: %u) X: %f Y: %f (%s) in grid[%u,%u]cell[%u,%u] instead grid[%u,%u]cell[%u,%u]",
            c->GetGUIDLow(),
            c->GetPositionX(),c->GetPositionY(),(moved ? "final" : "original"),
            cur_cell.GridX(), cur_cell.GridY(), cur_cell.CellX(), cur_cell.CellY(),
            xy_cell.GridX(),  xy_cell.GridY(),  xy_cell.CellX(),  xy_cell.CellY());
        return true;                                        // not crash at error, just output error in debug mode
    }

    return true;
}

const char* Map::GetMapName() const
{
    return i_mapEntry ? i_mapEntry->name[sWorld.GetDefaultDbcLocale()] : "UNNAMEDMAP\x0";
}

void Map::UpdateObjectVisibility( WorldObject* obj, Cell cell, CellPair cellpair)
{
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();
    MaNGOS::VisibleChangesNotifier notifier(*obj);
    TypeContainerVisitor<MaNGOS::VisibleChangesNotifier, WorldTypeMapContainer > player_notifier(notifier);
    cell.Visit(cellpair, player_notifier, *this, *obj, GetVisibilityDistance());
}

void Map::PlayerRelocationNotify( Player* player, Cell cell, CellPair cellpair )
{
    MaNGOS::PlayerRelocationNotifier relocationNotifier(*player);
    cell.data.Part.reserved = ALL_DISTRICT;

    TypeContainerVisitor<MaNGOS::PlayerRelocationNotifier, GridTypeMapContainer >  p2grid_relocation(relocationNotifier);
    TypeContainerVisitor<MaNGOS::PlayerRelocationNotifier, WorldTypeMapContainer > p2world_relocation(relocationNotifier);

    float radius = MAX_CREATURE_ATTACK_RADIUS * sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO);

    cell.Visit(cellpair, p2grid_relocation, *this, *player, radius);
    cell.Visit(cellpair, p2world_relocation, *this, *player, radius);
}

void Map::SendInitSelf( Player * player )
{
    DETAIL_LOG("Creating player data for himself %u", player->GetGUIDLow());

    UpdateData data;

    // attach to player data current transport data
    if(Transport* transport = player->GetTransport())
    {
        transport->BuildCreateUpdateBlockForPlayer(&data, player);
    }

    // build data for self presence in world at own client (one time for map)
    player->BuildCreateUpdateBlockForPlayer(&data, player);

    // build other passengers at transport also (they always visible and marked as visible and will not send at visibility update at add to map
    if(Transport* transport = player->GetTransport())
    {
        for(Transport::PlayerSet::const_iterator itr = transport->GetPassengers().begin();itr!=transport->GetPassengers().end();++itr)
        {
            if(player!=(*itr) && player->HaveAtClient(*itr))
            {
                (*itr)->BuildCreateUpdateBlockForPlayer(&data, player);
            }
        }
    }

    WorldPacket packet;
    data.BuildPacket(&packet);
    player->GetSession()->SendPacket(&packet);
}

void Map::SendInitTransports( Player * player )
{
    // Hack to send out transports
    MapManager::TransportMap& tmap = sMapMgr.m_TransportsByMap;

    // no transports at map
    if (tmap.find(player->GetMapId()) == tmap.end())
        return;

    UpdateData transData;

    MapManager::TransportSet& tset = tmap[player->GetMapId()];

    for (MapManager::TransportSet::const_iterator i = tset.begin(); i != tset.end(); ++i)
    {
        // send data for current transport in other place
        if((*i) != player->GetTransport() && (*i)->GetMapId()==i_id)
        {
            (*i)->BuildCreateUpdateBlockForPlayer(&transData, player);
        }
    }

    WorldPacket packet;
    transData.BuildPacket(&packet);
    player->GetSession()->SendPacket(&packet);
}

void Map::SendRemoveTransports( Player * player )
{
    // Hack to send out transports
    MapManager::TransportMap& tmap = sMapMgr.m_TransportsByMap;

    // no transports at map
    if (tmap.find(player->GetMapId()) == tmap.end())
        return;

    UpdateData transData;

    MapManager::TransportSet& tset = tmap[player->GetMapId()];

    // except used transport
    for (MapManager::TransportSet::const_iterator i = tset.begin(); i != tset.end(); ++i)
        if((*i) != player->GetTransport() && (*i)->GetMapId()!=i_id)
            (*i)->BuildOutOfRangeUpdateBlock(&transData);

    WorldPacket packet;
    transData.BuildPacket(&packet);
    player->GetSession()->SendPacket(&packet);
}

inline void Map::setNGrid(NGridType *grid, uint32 x, uint32 y)
{
    if(x >= MAX_NUMBER_OF_GRIDS || y >= MAX_NUMBER_OF_GRIDS)
    {
        sLog.outError("map::setNGrid() Invalid grid coordinates found: %d, %d!",x,y);
        MANGOS_ASSERT(false);
    }
    i_grids[x][y] = grid;
}

void Map::AddObjectToRemoveList(WorldObject *obj)
{
    MANGOS_ASSERT(obj->GetMapId()==GetId() && obj->GetInstanceId()==GetInstanceId());

    obj->CleanupsBeforeDelete();                            // remove or simplify at least cross referenced links

    i_objectsToRemove.insert(obj);
    //DEBUG_LOG("Object (GUID: %u TypeId: %u ) added to removing list.",obj->GetGUIDLow(),obj->GetTypeId());
}

void Map::RemoveAllObjectsInRemoveList()
{
    if(i_objectsToRemove.empty())
        return;

    //DEBUG_LOG("Object remover 1 check.");
    while(!i_objectsToRemove.empty())
    {
        WorldObject* obj = *i_objectsToRemove.begin();
        i_objectsToRemove.erase(i_objectsToRemove.begin());

        switch(obj->GetTypeId())
        {
            case TYPEID_CORPSE:
            {
                // ??? WTF
                Corpse* corpse = GetCorpse(obj->GetGUID());
                if (!corpse)
                    sLog.outError("Try delete corpse/bones %u that not in map", obj->GetGUIDLow());
                else
                    Remove(corpse,true);
                break;
            }
            case TYPEID_DYNAMICOBJECT:
                Remove((DynamicObject*)obj,true);
                break;
            case TYPEID_GAMEOBJECT:
                Remove((GameObject*)obj,true);
                break;
            case TYPEID_UNIT:
                Remove((Creature*)obj,true);
                break;
            default:
                sLog.outError("Non-grid object (TypeId: %u) in grid object removing list, ignored.",obj->GetTypeId());
                break;
        }
    }
    //DEBUG_LOG("Object remover 2 check.");
}

uint32 Map::GetPlayersCountExceptGMs() const
{
    uint32 count = 0;
    for(MapRefManager::const_iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
        if(!itr->getSource()->isGameMaster())
            ++count;
    return count;
}

void Map::SendToPlayers(WorldPacket const* data) const
{
    for(MapRefManager::const_iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
        itr->getSource()->GetSession()->SendPacket(data);
}

bool Map::ActiveObjectsNearGrid(uint32 x, uint32 y) const
{
    MANGOS_ASSERT(x < MAX_NUMBER_OF_GRIDS);
    MANGOS_ASSERT(y < MAX_NUMBER_OF_GRIDS);

    CellPair cell_min(x*MAX_NUMBER_OF_CELLS, y*MAX_NUMBER_OF_CELLS);
    CellPair cell_max(cell_min.x_coord + MAX_NUMBER_OF_CELLS, cell_min.y_coord+MAX_NUMBER_OF_CELLS);

    //we must find visible range in cells so we unload only non-visible cells...
    float viewDist = GetVisibilityDistance();
    int cell_range = (int)ceilf(viewDist / SIZE_OF_GRID_CELL) + 1;

    cell_min << cell_range;
    cell_min -= cell_range;
    cell_max >> cell_range;
    cell_max += cell_range;

    for(MapRefManager::const_iterator iter = m_mapRefManager.begin(); iter != m_mapRefManager.end(); ++iter)
    {
        Player* plr = iter->getSource();

        CellPair p = MaNGOS::ComputeCellPair(plr->GetPositionX(), plr->GetPositionY());
        if( (cell_min.x_coord <= p.x_coord && p.x_coord <= cell_max.x_coord) &&
            (cell_min.y_coord <= p.y_coord && p.y_coord <= cell_max.y_coord) )
            return true;
    }

    for(ActiveNonPlayers::const_iterator iter = m_activeNonPlayers.begin(); iter != m_activeNonPlayers.end(); ++iter)
    {
        WorldObject* obj = *iter;

        CellPair p = MaNGOS::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
        if( (cell_min.x_coord <= p.x_coord && p.x_coord <= cell_max.x_coord) &&
            (cell_min.y_coord <= p.y_coord && p.y_coord <= cell_max.y_coord) )
            return true;
    }

    return false;
}

void Map::AddToActive( WorldObject* obj )
{
    m_activeNonPlayers.insert(obj);

    // also not allow unloading spawn grid to prevent creating creature clone at load
    if (obj->GetTypeId()==TYPEID_UNIT)
    {
        Creature* c= (Creature*)obj;

        if (!c->isPet() && c->GetDBTableGUIDLow())
        {
            float x,y,z;
            c->GetRespawnCoord(x,y,z);
            GridPair p = MaNGOS::ComputeGridPair(x, y);
            if(getNGrid(p.x_coord, p.y_coord))
                getNGrid(p.x_coord, p.y_coord)->incUnloadActiveLock();
            else
            {
                GridPair p2 = MaNGOS::ComputeGridPair(c->GetPositionX(), c->GetPositionY());
                sLog.outError("Active creature (GUID: %u Entry: %u) added to grid[%u,%u] but spawn grid[%u,%u] not loaded.",
                    c->GetGUIDLow(), c->GetEntry(), p.x_coord, p.y_coord, p2.x_coord, p2.y_coord);
            }
        }
    }
}

void Map::RemoveFromActive( WorldObject* obj )
{
    // Map::Update for active object in proccess
    if(m_activeNonPlayersIter != m_activeNonPlayers.end())
    {
        ActiveNonPlayers::iterator itr = m_activeNonPlayers.find(obj);
        if(itr==m_activeNonPlayersIter)
            ++m_activeNonPlayersIter;
        m_activeNonPlayers.erase(itr);
    }
    else
        m_activeNonPlayers.erase(obj);

    // also allow unloading spawn grid
    if (obj->GetTypeId()==TYPEID_UNIT)
    {
        Creature* c= (Creature*)obj;

        if(!c->isPet() && c->GetDBTableGUIDLow())
        {
            float x,y,z;
            c->GetRespawnCoord(x,y,z);
            GridPair p = MaNGOS::ComputeGridPair(x, y);
            if(getNGrid(p.x_coord, p.y_coord))
                getNGrid(p.x_coord, p.y_coord)->decUnloadActiveLock();
            else
            {
                GridPair p2 = MaNGOS::ComputeGridPair(c->GetPositionX(), c->GetPositionY());
                sLog.outError("Active creature (GUID: %u Entry: %u) removed from grid[%u,%u] but spawn grid[%u,%u] not loaded.",
                    c->GetGUIDLow(), c->GetEntry(), p.x_coord, p.y_coord, p2.x_coord, p2.y_coord);
            }
        }
    }
}

template void Map::Add(Corpse *);
template void Map::Add(Creature *);
template void Map::Add(GameObject *);
template void Map::Add(DynamicObject *);

template void Map::Remove(Corpse *,bool);
template void Map::Remove(Creature *,bool);
template void Map::Remove(GameObject *, bool);
template void Map::Remove(DynamicObject *, bool);

/* ******* Dungeon Instance Maps ******* */

InstanceMap::InstanceMap(uint32 id, time_t expiry, uint32 InstanceId, uint8 SpawnMode, Map* _parent)
  : Map(id, expiry, InstanceId, SpawnMode, _parent),
    m_resetAfterUnload(false), m_unloadWhenEmpty(false),
    i_data(NULL), i_script_id(0)
{
    //lets initialize visibility distance for dungeons
    InstanceMap::InitVisibilityDistance();

    // the timer is started by default, and stopped when the first player joins
    // this make sure it gets unloaded if for some reason no player joins
    m_unloadTimer = std::max(sWorld.getConfig(CONFIG_UINT32_INSTANCE_UNLOAD_DELAY), (uint32)MIN_UNLOAD_DELAY);

    // Dungeon only code
    if(IsDungeon())
    {
        m_instanceSave = sInstanceSaveMgr.AddInstanceSave(GetId(), GetInstanceId(), Difficulty(GetSpawnMode()), 0, true);
        m_instanceSave->SetUsedByMapState(true);
    }
}

InstanceMap::~InstanceMap()
{
    if(i_data)
    {
        delete i_data;
        i_data = NULL;
    }
}

void InstanceMap::InitVisibilityDistance()
{
    //init visibility distance for instances
    m_VisibleDistance = World::GetMaxVisibleDistanceInInstances();
}

/*
    Do map specific checks to see if the player can enter
*/
bool InstanceMap::CanEnter(Player *player)
{
    if(player->GetMapRef().getTarget() == this)
    {
        sLog.outError("InstanceMap::CanEnter - player %s(%u) already in map %d,%d,%d!", player->GetName(), player->GetGUIDLow(), GetId(), GetInstanceId(), GetSpawnMode());
        MANGOS_ASSERT(false);
        return false;
    }

    // cannot enter if the instance is full (player cap), GMs don't count
    uint32 maxPlayers = GetMaxPlayers();
    if (!player->isGameMaster() && GetPlayersCountExceptGMs() >= maxPlayers)
    {
        DETAIL_LOG("MAP: Instance '%u' of map '%s' cannot have more than '%u' players. Player '%s' rejected", GetInstanceId(), GetMapName(), maxPlayers, player->GetName());
        player->SendTransferAborted(GetId(), TRANSFER_ABORT_MAX_PLAYERS);
        return false;
    }

    // cannot enter while players in the instance are in combat
    Group *pGroup = player->GetGroup();
    if(pGroup && pGroup->InCombatToInstance(GetInstanceId()) && player->isAlive() && player->GetMapId() != GetId())
    {
        player->SendTransferAborted(GetId(), TRANSFER_ABORT_ZONE_IN_COMBAT);
        return false;
    }

    return Map::CanEnter(player);
}

/*
    Do map specific checks and add the player to the map if successful.
*/
bool InstanceMap::Add(Player *player)
{
    // TODO: Not sure about checking player level: already done in HandleAreaTriggerOpcode
    // GMs still can teleport player in instance.
    // Is it needed?

    {
        Guard guard(*this);
        if (!CanEnter(player))
            return false;

        // Dungeon only code
        if (IsDungeon())
        {
            // check for existing instance binds
            InstancePlayerBind *playerBind = player->GetBoundInstance(GetId(), Difficulty(GetSpawnMode()));
            if (playerBind && playerBind->perm)
            {
                // cannot enter other instances if bound permanently
                if (playerBind->save != GetInstanceSave())
                {
                    sLog.outError("InstanceMap::Add: player %s(%d) is permanently bound to instance %d,%d,%d,%d,%d,%d but he is being put in instance %d,%d,%d,%d,%d,%d",
                        player->GetName(), player->GetGUIDLow(), playerBind->save->GetMapId(),
                        playerBind->save->GetInstanceId(), playerBind->save->GetDifficulty(),
                        playerBind->save->GetPlayerCount(), playerBind->save->GetGroupCount(),
                        playerBind->save->CanReset(),
                        GetInstanceSave()->GetMapId(), GetInstanceSave()->GetInstanceId(),
                        GetInstanceSave()->GetDifficulty(), GetInstanceSave()->GetPlayerCount(),
                        GetInstanceSave()->GetGroupCount(), GetInstanceSave()->CanReset());
                    MANGOS_ASSERT(false);
                }
            }
            else
            {
                Group *pGroup = player->GetGroup();
                if (pGroup)
                {
                    // solo saves should be reset when entering a group
                    InstanceGroupBind *groupBind = pGroup->GetBoundInstance(this,GetDifficulty());
                    if (playerBind)
                    {
                        sLog.outError("InstanceMap::Add: %s is being put in instance %d,%d,%d,%d,%d,%d but he is in group (Id: %d) and is bound to instance %d,%d,%d,%d,%d,%d!",
                            player->GetObjectGuid().GetString().c_str(), GetInstanceSave()->GetMapId(), GetInstanceSave()->GetInstanceId(),
                            GetInstanceSave()->GetDifficulty(), GetInstanceSave()->GetPlayerCount(), GetInstanceSave()->GetGroupCount(),
                            GetInstanceSave()->CanReset(), pGroup->GetId(),
                            playerBind->save->GetMapId(), playerBind->save->GetInstanceId(), playerBind->save->GetDifficulty(),
                            playerBind->save->GetPlayerCount(), playerBind->save->GetGroupCount(), playerBind->save->CanReset());

                        if (groupBind)
                            sLog.outError("InstanceMap::Add: the group (Id: %d) is bound to instance %d,%d,%d,%d,%d,%d",
                                pGroup->GetId(),
                                groupBind->save->GetMapId(), groupBind->save->GetInstanceId(), groupBind->save->GetDifficulty(),
                                groupBind->save->GetPlayerCount(), groupBind->save->GetGroupCount(), groupBind->save->CanReset());
                        MANGOS_ASSERT(false);
                    }
                    // bind to the group or keep using the group save
                    if (!groupBind)
                        pGroup->BindToInstance(GetInstanceSave(), false);
                    else
                    {
                        // cannot jump to a different instance without resetting it
                        if (groupBind->save != GetInstanceSave())
                        {
                            sLog.outError("InstanceMap::Add: %s is being put in instance %d,%d,%d but he is in group (Id: %d) which is bound to instance %d,%d,%d!",
                                player->GetObjectGuid().GetString().c_str(), GetInstanceSave()->GetMapId(),
                                GetInstanceSave()->GetInstanceId(), GetInstanceSave()->GetDifficulty(),
                                pGroup->GetId(), groupBind->save->GetMapId(),
                                groupBind->save->GetInstanceId(), groupBind->save->GetDifficulty());

                            if (GetInstanceSave())
                                sLog.outError("MapSave players: %d, group count: %d",
                                    GetInstanceSave()->GetPlayerCount(), GetInstanceSave()->GetGroupCount());
                            else
                                sLog.outError("MapSave NULL");

                            if (groupBind->save)
                                sLog.outError("GroupBind save players: %d, group count: %d", groupBind->save->GetPlayerCount(), groupBind->save->GetGroupCount());
                            else
                                sLog.outError("GroupBind save NULL");
                            MANGOS_ASSERT(false);
                        }
                        // if the group/leader is permanently bound to the instance
                        // players also become permanently bound when they enter
                        if (groupBind->perm)
                        {
                            WorldPacket data(SMSG_INSTANCE_SAVE_CREATED, 4);
                            data << uint32(0);
                            player->GetSession()->SendPacket(&data);
                            player->BindToInstance(GetInstanceSave(), true);
                        }
                    }
                }
                else
                {
                    // set up a solo bind or continue using it
                    if(!playerBind)
                        player->BindToInstance(GetInstanceSave(), false);
                    else
                        // cannot jump to a different instance without resetting it
                        MANGOS_ASSERT(playerBind->save == GetInstanceSave());
                }
            }
        }

        // for normal instances cancel the reset schedule when the
        // first player enters (no players yet)
        SetResetSchedule(false);

        DETAIL_LOG("MAP: Player '%s' is entering instance '%u' of map '%s'", player->GetName(), GetInstanceId(), GetMapName());
        // initialize unload state
        m_unloadTimer = 0;
        m_resetAfterUnload = false;
        m_unloadWhenEmpty = false;
    }

    // this will acquire the same mutex so it cannot be in the previous block
    Map::Add(player);

    if (i_data)
        i_data->OnPlayerEnter(player);

    return true;
}

void InstanceMap::Update(const uint32& t_diff)
{
    Map::Update(t_diff);

    if(i_data)
        i_data->Update(t_diff);
}

void BattleGroundMap::Update(const uint32& diff)
{
    Map::Update(diff);

    GetBG()->Update(diff);
}

void InstanceMap::Remove(Player *player, bool remove)
{
    DETAIL_LOG("MAP: Removing player '%s' from instance '%u' of map '%s' before relocating to other map", player->GetName(), GetInstanceId(), GetMapName());

    //if last player set unload timer
    if(!m_unloadTimer && m_mapRefManager.getSize() == 1)
        m_unloadTimer = m_unloadWhenEmpty ? MIN_UNLOAD_DELAY : std::max(sWorld.getConfig(CONFIG_UINT32_INSTANCE_UNLOAD_DELAY), (uint32)MIN_UNLOAD_DELAY);

    if (i_data)
        i_data->OnPlayerLeave(player);

    Map::Remove(player, remove);

    // for normal instances schedule the reset after all players have left
    SetResetSchedule(true);
}

void InstanceMap::CreateInstanceData(bool load)
{
    if(i_data != NULL)
        return;

    InstanceTemplate const* mInstance = ObjectMgr::GetInstanceTemplate(GetId());
    if (mInstance)
    {
        i_script_id = mInstance->script_id;
        i_data = Script->CreateInstanceData(this);
    }

    if(!i_data)
        return;

    if(load)
    {
        // TODO: make a global storage for this
        QueryResult* result = CharacterDatabase.PQuery("SELECT data FROM instance WHERE map = '%u' AND id = '%u'", GetId(), i_InstanceId);
        if (result)
        {
            Field* fields = result->Fetch();
            const char* data = fields[0].GetString();
            if(data)
            {
                DEBUG_LOG("Loading instance data for `%s` with id %u", sObjectMgr.GetScriptName(i_script_id), i_InstanceId);
                i_data->Load(data);
            }
            delete result;
        }
    }
    else
    {
        DEBUG_LOG("New instance data, \"%s\" ,initialized!", sObjectMgr.GetScriptName(i_script_id));
        i_data->Initialize();
    }
}

/*
    Returns true if there are no players in the instance
*/
bool InstanceMap::Reset(uint8 method)
{
    // note: since the map may not be loaded when the instance needs to be reset
    // the instance must be deleted from the DB by InstanceSaveManager

    if(HavePlayers())
    {
        if(method == INSTANCE_RESET_ALL)
        {
            // notify the players to leave the instance so it can be reset
            for(MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
                itr->getSource()->SendResetFailedNotify(GetId());
        }
        else
        {
            if(method == INSTANCE_RESET_GLOBAL)
            {
                // set the homebind timer for players inside (1 minute)
                for(MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
                    itr->getSource()->m_InstanceValid = false;
            }

            // the unload timer is not started
            // instead the map will unload immediately after the players have left
            m_unloadWhenEmpty = true;
            m_resetAfterUnload = true;
        }
    }
    else
    {
        // unloaded at next update
        m_unloadTimer = MIN_UNLOAD_DELAY;
        m_resetAfterUnload = true;
    }

    return m_mapRefManager.isEmpty();
}

void InstanceMap::PermBindAllPlayers(Player *player)
{
    if (!IsDungeon())
        return;

    Group *group = player->GetGroup();
    // group members outside the instance group don't get bound
    for(MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
    {
        Player* plr = itr->getSource();
        // players inside an instance cannot be bound to other instances
        // some players may already be permanently bound, in this case nothing happens
        InstancePlayerBind *bind = plr->GetBoundInstance(GetId(), GetDifficulty());
        if (!bind || !bind->perm)
        {
            plr->BindToInstance(GetInstanceSave(), true);
            WorldPacket data(SMSG_INSTANCE_SAVE_CREATED, 4);
            data << uint32(0);
            plr->GetSession()->SendPacket(&data);
        }

        // if the leader is not in the instance the group will not get a perm bind
        if (group && group->GetLeaderGuid() == plr->GetObjectGuid())
            group->BindToInstance(GetInstanceSave(), true);
    }
}

void InstanceMap::UnloadAll(bool pForce)
{
    if(HavePlayers())
    {
        sLog.outError("InstanceMap::UnloadAll: there are still players in the instance at unload, should not happen!");
        for(MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
        {
            Player* plr = itr->getSource();
            plr->TeleportToHomebind();
        }
    }

    if(m_resetAfterUnload == true)
        sObjectMgr.DeleteRespawnTimeForInstance(GetInstanceId());

    Map::UnloadAll(pForce);
}

void InstanceMap::SendResetWarnings(uint32 timeLeft) const
{
    for(MapRefManager::const_iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
        itr->getSource()->SendInstanceResetWarning(GetId(), itr->getSource()->GetDifficulty(IsRaid()), timeLeft);
}

void InstanceMap::SetResetSchedule(bool on)
{
    // only for normal instances
    // the reset time is only scheduled when there are no payers inside
    // it is assumed that the reset time will rarely (if ever) change while the reset is scheduled
    if(IsDungeon() && !HavePlayers() && !IsRaidOrHeroicDungeon())
        sInstanceSaveMgr.GetScheduler().ScheduleReset(on, GetInstanceSave()->GetResetTime(), InstanceResetEvent(RESET_EVENT_DUNGEON, GetId(), Difficulty(GetSpawnMode()), GetInstanceId()));
}

/* ******* Battleground Instance Maps ******* */

BattleGroundMap::BattleGroundMap(uint32 id, time_t expiry, uint32 InstanceId, Map* _parent, uint8 spawnMode)
  : Map(id, expiry, InstanceId, spawnMode, _parent)
{
    //lets initialize visibility distance for BG/Arenas
    BattleGroundMap::InitVisibilityDistance();
}

BattleGroundMap::~BattleGroundMap()
{
}

void BattleGroundMap::InitVisibilityDistance()
{
    //init visibility distance for BG/Arenas
    m_VisibleDistance = World::GetMaxVisibleDistanceInBGArenas();
}

bool BattleGroundMap::CanEnter(Player * player)
{
    if(player->GetMapRef().getTarget() == this)
    {
        sLog.outError("BGMap::CanEnter - player %u already in map!", player->GetGUIDLow());
        MANGOS_ASSERT(false);
        return false;
    }

    if(player->GetBattleGroundId() != GetInstanceId())
        return false;

    // player number limit is checked in bgmgr, no need to do it here

    return Map::CanEnter(player);
}

bool BattleGroundMap::Add(Player * player)
{
    {
        Guard guard(*this);
        if(!CanEnter(player))
            return false;
        // reset instance validity, battleground maps do not homebind
        player->m_InstanceValid = true;
    }
    return Map::Add(player);
}

void BattleGroundMap::Remove(Player *player, bool remove)
{
    DETAIL_LOG("MAP: Removing player '%s' from bg '%u' of map '%s' before relocating to other map", player->GetName(), GetInstanceId(), GetMapName());
    Map::Remove(player, remove);
}

void BattleGroundMap::SetUnload()
{
    m_unloadTimer = MIN_UNLOAD_DELAY;
}

void BattleGroundMap::UnloadAll(bool pForce)
{
    while(HavePlayers())
    {
        if(Player * plr = m_mapRefManager.getFirst()->getSource())
        {
            plr->TeleportTo(plr->GetBattleGroundEntryPoint());
            // TeleportTo removes the player from this map (if the map exists) -> calls BattleGroundMap::Remove -> invalidates the iterator.
            // just in case, remove the player from the list explicitly here as well to prevent a possible infinite loop
            // note that this remove is not needed if the code works well in other places
            plr->GetMapRef().unlink();
        }
    }

    Map::UnloadAll(pForce);
}

/// Put scripts in the execution queue
void Map::ScriptsStart(ScriptMapMap const& scripts, uint32 id, Object* source, Object* target)
{
    ///- Find the script map
    ScriptMapMap::const_iterator s = scripts.find(id);
    if (s == scripts.end())
        return;

    // prepare static data
    uint64 sourceGUID = source->GetGUID();
    uint64 targetGUID = target ? target->GetGUID() : (uint64)0;
    uint64 ownerGUID  = (source->GetTypeId()==TYPEID_ITEM) ? ((Item*)source)->GetOwnerGUID() : (uint64)0;

    ///- Schedule script execution for all scripts in the script map
    ScriptMap const *s2 = &(s->second);
    bool immedScript = false;
    for (ScriptMap::const_iterator iter = s2->begin(); iter != s2->end(); ++iter)
    {
        ScriptAction sa;
        sa.sourceGUID = sourceGUID;
        sa.targetGUID = targetGUID;
        sa.ownerGUID  = ownerGUID;

        sa.script = &iter->second;
        m_scriptSchedule.insert(std::pair<time_t, ScriptAction>(time_t(sWorld.GetGameTime() + iter->first), sa));
        if (iter->first == 0)
            immedScript = true;

        sWorld.IncreaseScheduledScriptsCount();
    }
    ///- If one of the effects should be immediate, launch the script execution
    if (immedScript)
        ScriptsProcess();
}

void Map::ScriptCommandStart(ScriptInfo const& script, uint32 delay, Object* source, Object* target)
{
    // NOTE: script record _must_ exist until command executed

    // prepare static data
    uint64 sourceGUID = source->GetGUID();
    uint64 targetGUID = target ? target->GetGUID() : (uint64)0;
    uint64 ownerGUID  = (source->GetTypeId()==TYPEID_ITEM) ? ((Item*)source)->GetOwnerGUID() : (uint64)0;

    ScriptAction sa;
    sa.sourceGUID = sourceGUID;
    sa.targetGUID = targetGUID;
    sa.ownerGUID  = ownerGUID;

    sa.script = &script;
    m_scriptSchedule.insert(std::pair<time_t, ScriptAction>(time_t(sWorld.GetGameTime() + delay), sa));

    sWorld.IncreaseScheduledScriptsCount();

    ///- If effects should be immediate, launch the script execution
    if(delay == 0)
        ScriptsProcess();
}

/// Process queued scripts
void Map::ScriptsProcess()
{
    if (m_scriptSchedule.empty())
        return;

    ///- Process overdue queued scripts
    std::multimap<time_t, ScriptAction>::iterator iter = m_scriptSchedule.begin();
    // ok as multimap is a *sorted* associative container
    while (!m_scriptSchedule.empty() && (iter->first <= sWorld.GetGameTime()))
    {
        ScriptAction const& step = iter->second;

        Object* source = NULL;

        if (step.sourceGUID)
        {
            switch(GUID_HIPART(step.sourceGUID))
            {
                case HIGHGUID_ITEM:
                // case HIGHGUID_CONTAINER: ==HIGHGUID_ITEM
                {
                    Player* player = HashMapHolder<Player>::Find(step.ownerGUID);
                    if(player)
                        source = player->GetItemByGuid(step.sourceGUID);
                    break;
                }
                case HIGHGUID_UNIT:
                    source = GetCreature(step.sourceGUID);
                    break;
                case HIGHGUID_PET:
                    source = GetPet(step.sourceGUID);
                    break;
                case HIGHGUID_VEHICLE:
                    source = GetVehicle(step.sourceGUID);
                    break;
                case HIGHGUID_PLAYER:
                    source = HashMapHolder<Player>::Find(step.sourceGUID);
                    break;
                case HIGHGUID_GAMEOBJECT:
                    source = GetGameObject(step.sourceGUID);
                    break;
                case HIGHGUID_CORPSE:
                    source = HashMapHolder<Corpse>::Find(step.sourceGUID);
                    break;
                default:
                    sLog.outError("*_script source with unsupported high guid value %u",GUID_HIPART(step.sourceGUID));
                    break;
            }
        }

        if (source && !source->IsInWorld())
            source = NULL;

        Object* target = NULL;

        if (step.targetGUID)
        {
            switch(GUID_HIPART(step.targetGUID))
            {
                case HIGHGUID_UNIT:
                    target = GetCreature(step.targetGUID);
                    break;
                case HIGHGUID_PET:
                    target = GetPet(step.targetGUID);
                    break;
                case HIGHGUID_VEHICLE:
                    target = GetVehicle(step.targetGUID);
                    break;
                case HIGHGUID_PLAYER:                       // empty GUID case also
                    target = HashMapHolder<Player>::Find(step.targetGUID);
                    break;
                case HIGHGUID_GAMEOBJECT:
                    target = GetGameObject(step.targetGUID);
                    break;
                case HIGHGUID_CORPSE:
                    target = HashMapHolder<Corpse>::Find(step.targetGUID);
                    break;
                default:
                    sLog.outError("*_script source with unsupported high guid value %u",GUID_HIPART(step.targetGUID));
                    break;
            }
        }

        if (target && !target->IsInWorld())
            target = NULL;

        switch(step.script->command)
        {
            case SCRIPT_COMMAND_TALK:
            {
                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_TALK (script id %u) call for NULL source.", step.script->id);
                    break;
                }

                if (!source->isType(TYPEMASK_WORLDOBJECT))
                {
                    sLog.outError("SCRIPT_COMMAND_TALK (script id %u) call for unsupported non-worldobject (TypeId: %u), skipping.", step.script->id, source->GetTypeId());
                    break;
                }

                WorldObject* pSource = (WorldObject*)source;
                Creature* pBuddy = NULL;

                // flag_target_player_as_source     0x01
                // flag_original_source_as_target   0x02
                // flag_buddy_as_target             0x04

                // If target is player (and not already the source) but should be the source
                if (target && target->GetTypeId() == TYPEID_PLAYER && step.script->data_flags & 0x01)
                {
                    if (source->GetTypeId() != TYPEID_PLAYER)
                        pSource = (WorldObject*)target;
                }

                // If step has a buddy entry defined, search for it.
                if (step.script->datalong2)
                {
                    MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck u_check(*pSource, step.script->datalong2, true, step.script->datalong3);
                    MaNGOS::CreatureLastSearcher<MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(pSource, pBuddy, u_check);

                    Cell::VisitGridObjects(pSource, searcher, step.script->datalong3);
                }

                // If buddy found, then use it
                if (pBuddy)
                {
                    // pBuddy can be target of talk
                    if (step.script->data_flags & 0x04)
                    {
                        target = (Object*)pBuddy;
                    }
                    else
                    {
                        // If not target of talk, then set pBuddy as source
                        // Useless when source is already flagged to be player, and should maybe produce error.
                        if (!(step.script->data_flags & 0x01))
                            pSource = (WorldObject*)pBuddy;
                    }
                }

                // If we should talk to the original source instead of target
                if (step.script->data_flags & 0x02)
                    target = source;

                uint64 unit_target = target ? target->GetGUID() : 0;

                switch(step.script->datalong)
                {
                    case CHAT_TYPE_SAY:
                        pSource->MonsterSay(step.script->dataint, LANG_UNIVERSAL, unit_target);
                        break;
                    case CHAT_TYPE_YELL:
                        pSource->MonsterYell(step.script->dataint, LANG_UNIVERSAL, unit_target);
                        break;
                    case CHAT_TYPE_TEXT_EMOTE:
                        pSource->MonsterTextEmote(step.script->dataint, unit_target);
                        break;
                    case CHAT_TYPE_BOSS_EMOTE:
                        pSource->MonsterTextEmote(step.script->dataint, unit_target, true);
                        break;
                    case CHAT_TYPE_WHISPER:
                        if (!unit_target || !IS_PLAYER_GUID(unit_target))
                        {
                            sLog.outError("SCRIPT_COMMAND_TALK (script id %u) attempt to whisper (%u) 0-guid or non-player, skipping.", step.script->id, step.script->datalong);
                            break;
                        }
                        pSource->MonsterWhisper(step.script->dataint, unit_target);
                        break;
                    case CHAT_TYPE_BOSS_WHISPER:
                        if (!unit_target || !IS_PLAYER_GUID(unit_target))
                        {
                            sLog.outError("SCRIPT_COMMAND_TALK (script id %u) attempt to whisper (%u) 0-guid or non-player, skipping.", step.script->id, step.script->datalong);
                            break;
                        }
                        pSource->MonsterWhisper(step.script->dataint, unit_target, true);
                        break;
                    case CHAT_TYPE_ZONE_YELL:
                        pSource->MonsterYellToZone(step.script->dataint, LANG_UNIVERSAL, unit_target);
                        break;
                    default:
                        break;                              // must be already checked at load
                }
                break;
            }
            case SCRIPT_COMMAND_EMOTE:
                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_EMOTE (script id %u) call for NULL creature.", step.script->id);
                    break;
                }

                if (source->GetTypeId()!=TYPEID_UNIT)
                {
                    sLog.outError("SCRIPT_COMMAND_EMOTE (script id %u) call for non-creature (TypeId: %u), skipping.", step.script->id, source->GetTypeId());
                    break;
                }

                ((Creature *)source)->HandleEmote(step.script->datalong);
                break;
            case SCRIPT_COMMAND_FIELD_SET:
                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_FIELD_SET (script id %u) call for NULL object.", step.script->id);
                    break;
                }

                if (step.script->datalong <= OBJECT_FIELD_ENTRY || step.script->datalong >= source->GetValuesCount())
                {
                    sLog.outError("SCRIPT_COMMAND_FIELD_SET (script id %u) call for wrong field %u (max count: %u) in object (TypeId: %u).",
                        step.script->id, step.script->datalong, source->GetValuesCount(), source->GetTypeId());
                    break;
                }

                source->SetUInt32Value(step.script->datalong, step.script->datalong2);
                break;
            case SCRIPT_COMMAND_MOVE_TO:
                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_MOVE_TO (script id %u) call for NULL creature.", step.script->id);
                    break;
                }

                if (source->GetTypeId() != TYPEID_UNIT)
                {
                    sLog.outError("SCRIPT_COMMAND_MOVE_TO (script id %u) call for non-creature (TypeId: %u), skipping.", step.script->id, source->GetTypeId());
                    break;
                }

                ((Unit*)source)->MonsterMoveWithSpeed(step.script->x, step.script->y, step.script->z, step.script->datalong2 );
                break;
            case SCRIPT_COMMAND_FLAG_SET:
                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_FLAG_SET (script id %u) call for NULL object.", step.script->id);
                    break;
                }
                if (step.script->datalong <= OBJECT_FIELD_ENTRY || step.script->datalong >= source->GetValuesCount())
                {
                    sLog.outError("SCRIPT_COMMAND_FLAG_SET (script id %u) call for wrong field %u (max count: %u) in object (TypeId: %u).",
                        step.script->id, step.script->datalong, source->GetValuesCount(), source->GetTypeId());
                    break;
                }

                source->SetFlag(step.script->datalong, step.script->datalong2);
                break;
            case SCRIPT_COMMAND_FLAG_REMOVE:
                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_FLAG_REMOVE (script id %u) call for NULL object.", step.script->id);
                    break;
                }
                if (step.script->datalong <= OBJECT_FIELD_ENTRY || step.script->datalong >= source->GetValuesCount())
                {
                    sLog.outError("SCRIPT_COMMAND_FLAG_REMOVE (script id %u) call for wrong field %u (max count: %u) in object (TypeId: %u).",
                        step.script->id, step.script->datalong, source->GetValuesCount(), source->GetTypeId());
                    break;
                }

                source->RemoveFlag(step.script->datalong, step.script->datalong2);
                break;
            case SCRIPT_COMMAND_TELEPORT_TO:
            {
                // accept player in any one from target/source arg
                if (!target && !source)
                {
                    sLog.outError("SCRIPT_COMMAND_TELEPORT_TO (script id %u) call for NULL object.", step.script->id);
                    break;
                }

                // must be only Player
                if ((!target || target->GetTypeId() != TYPEID_PLAYER) && (!source || source->GetTypeId() != TYPEID_PLAYER))
                {
                    sLog.outError("SCRIPT_COMMAND_TELEPORT_TO (script id %u) call for non-player (TypeIdSource: %u)(TypeIdTarget: %u), skipping.", step.script->id, source ? source->GetTypeId() : 0, target ? target->GetTypeId() : 0);
                    break;
                }

                Player* pSource = target && target->GetTypeId() == TYPEID_PLAYER ? (Player*)target : (Player*)source;

                pSource->TeleportTo(step.script->datalong, step.script->x, step.script->y, step.script->z, step.script->o);
                break;
            }
            case SCRIPT_COMMAND_KILL_CREDIT:
            {
                // accept player in any one from target/source arg
                if (!target && !source)
                {
                    sLog.outError("SCRIPT_COMMAND_KILL_CREDIT (script id %u) call for NULL object.", step.script->id);
                    break;
                }

                // must be only Player
                if ((!target || target->GetTypeId() != TYPEID_PLAYER) && (!source || source->GetTypeId() != TYPEID_PLAYER))
                {
                    sLog.outError("SCRIPT_COMMAND_KILL_CREDIT (script id %u) call for non-player (TypeIdSource: %u)(TypeIdTarget: %u), skipping.", step.script->id, source ? source->GetTypeId() : 0, target ? target->GetTypeId() : 0);
                    break;
                }

                Player* pSource = target && target->GetTypeId() == TYPEID_PLAYER ? (Player*)target : (Player*)source;

                if (step.script->datalong2)
                {
                    pSource->RewardPlayerAndGroupAtEvent(step.script->datalong, pSource);
                }
                else
                {
                    pSource->KilledMonsterCredit(step.script->datalong);
                }

                break;
            }
            case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:
            {
                if (!step.script->datalong)                 // creature not specified
                {
                    sLog.outError("SCRIPT_COMMAND_TEMP_SUMMON_CREATURE (script id %u) call for NULL creature.", step.script->id);
                    break;
                }

                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_TEMP_SUMMON_CREATURE (script id %u) call for NULL world object.", step.script->id);
                    break;
                }

                if (!source->isType(TYPEMASK_WORLDOBJECT))
                {
                    sLog.outError("SCRIPT_COMMAND_TEMP_SUMMON_CREATURE (script id %u) call for non-WorldObject (TypeId: %u), skipping.", step.script->id, source->GetTypeId());
                    break;
                }

                WorldObject* summoner = (WorldObject*)source;

                float x = step.script->x;
                float y = step.script->y;
                float z = step.script->z;
                float o = step.script->o;

                Creature* pCreature = summoner->SummonCreature(step.script->datalong, x, y, z, o, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, step.script->datalong2);
                if (!pCreature)
                {
                    sLog.outError("SCRIPT_COMMAND_TEMP_SUMMON (script id %u) failed for creature (entry: %u).", step.script->id, step.script->datalong);
                    break;
                }

                break;
            }
            case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT:
            {
                if (!step.script->datalong)                 // gameobject not specified
                {
                    sLog.outError("SCRIPT_COMMAND_RESPAWN_GAMEOBJECT (script id %u) call for NULL gameobject.", step.script->id);
                    break;
                }

                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_RESPAWN_GAMEOBJECT (script id %u) call for NULL world object.", step.script->id);
                    break;
                }

                if (!source->isType(TYPEMASK_WORLDOBJECT))
                {
                    sLog.outError("SCRIPT_COMMAND_RESPAWN_GAMEOBJECT (script id %u) call for non-WorldObject (TypeId: %u), skipping.", step.script->id, source->GetTypeId());
                    break;
                }

                WorldObject* summoner = (WorldObject*)source;

                GameObject *go = NULL;
                int32 time_to_despawn = step.script->datalong2<5 ? 5 : (int32)step.script->datalong2;

                MaNGOS::GameObjectWithDbGUIDCheck go_check(*summoner,step.script->datalong);
                MaNGOS::GameObjectSearcher<MaNGOS::GameObjectWithDbGUIDCheck> checker(summoner, go, go_check);
                Cell::VisitGridObjects(summoner, checker, GetVisibilityDistance());

                if (!go)
                {
                    sLog.outError("SCRIPT_COMMAND_RESPAWN_GAMEOBJECT (script id %u) failed for gameobject(guid: %u).", step.script->id, step.script->datalong);
                    break;
                }

                if (go->GetGoType()==GAMEOBJECT_TYPE_FISHINGNODE ||
                    go->GetGoType()==GAMEOBJECT_TYPE_DOOR        ||
                    go->GetGoType()==GAMEOBJECT_TYPE_BUTTON      ||
                    go->GetGoType()==GAMEOBJECT_TYPE_TRAP)
                {
                    sLog.outError("SCRIPT_COMMAND_RESPAWN_GAMEOBJECT (script id %u) can not be used with gameobject of type %u (guid: %u).", step.script->id, uint32(go->GetGoType()), step.script->datalong);
                    break;
                }

                if (go->isSpawned())
                    break;                                  //gameobject already spawned

                go->SetLootState(GO_READY);
                go->SetRespawnTime(time_to_despawn);        //despawn object in ? seconds

                go->GetMap()->Add(go);
                break;
            }
            case SCRIPT_COMMAND_OPEN_DOOR:
            {
                if (!step.script->datalong)                 // door not specified
                {
                    sLog.outError("SCRIPT_COMMAND_OPEN_DOOR (script id %u) call for NULL door.", step.script->id);
                    break;
                }

                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_OPEN_DOOR (script id %u) call for NULL unit.", step.script->id);
                    break;
                }

                if (!source->isType(TYPEMASK_UNIT))         // must be any Unit (creature or player)
                {
                    sLog.outError("SCRIPT_COMMAND_OPEN_DOOR (script id %u) call for non-unit (TypeId: %u), skipping.", step.script->id, source->GetTypeId());
                    break;
                }

                Unit* caster = (Unit*)source;

                GameObject *door = NULL;
                int32 time_to_close = step.script->datalong2 < 15 ? 15 : (int32)step.script->datalong2;

                MaNGOS::GameObjectWithDbGUIDCheck go_check(*caster, step.script->datalong);
                MaNGOS::GameObjectSearcher<MaNGOS::GameObjectWithDbGUIDCheck> checker(caster, door, go_check);
                Cell::VisitGridObjects(caster, checker, GetVisibilityDistance());

                if (!door)
                {
                    sLog.outError("SCRIPT_COMMAND_OPEN_DOOR (script id %u) failed for gameobject(guid: %u).", step.script->id, step.script->datalong);
                    break;
                }

                if (door->GetGoType() != GAMEOBJECT_TYPE_DOOR)
                {
                    sLog.outError("SCRIPT_COMMAND_OPEN_DOOR (script id %u) failed for non-door(GoType: %u).", step.script->id, door->GetGoType());
                    break;
                }

                if (door->GetGoState() != GO_STATE_READY)
                    break;                                  //door already  open

                door->UseDoorOrButton(time_to_close);

                if (target && target->isType(TYPEMASK_GAMEOBJECT) && ((GameObject*)target)->GetGoType()==GAMEOBJECT_TYPE_BUTTON)
                    ((GameObject*)target)->UseDoorOrButton(time_to_close);

                break;
            }
            case SCRIPT_COMMAND_CLOSE_DOOR:
            {
                if (!step.script->datalong)                 // guid for door not specified
                {
                    sLog.outError("SCRIPT_COMMAND_CLOSE_DOOR (script id %u) call for NULL door.", step.script->id);
                    break;
                }

                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_CLOSE_DOOR (script id %u) call for NULL unit.", step.script->id);
                    break;
                }

                if (!source->isType(TYPEMASK_UNIT))         // must be any Unit (creature or player)
                {
                    sLog.outError("SCRIPT_COMMAND_CLOSE_DOOR (script id %u) call for non-unit (TypeId: %u), skipping.", step.script->id, source->GetTypeId());
                    break;
                }

                Unit* caster = (Unit*)source;

                GameObject *door = NULL;
                int32 time_to_open = step.script->datalong2 < 15 ? 15 : (int32)step.script->datalong2;

                MaNGOS::GameObjectWithDbGUIDCheck go_check(*caster, step.script->datalong);
                MaNGOS::GameObjectSearcher<MaNGOS::GameObjectWithDbGUIDCheck> checker(caster, door, go_check);
                Cell::VisitGridObjects(caster, checker, GetVisibilityDistance());

                if (!door)
                {
                    sLog.outError("SCRIPT_COMMAND_CLOSE_DOOR (script id %u) failed for gameobject(guid: %u).", step.script->id, step.script->datalong);
                    break;
                }
                if (door->GetGoType() != GAMEOBJECT_TYPE_DOOR)
                {
                    sLog.outError("SCRIPT_COMMAND_CLOSE_DOOR (script id %u) failed for non-door(GoType: %u).", step.script->id, door->GetGoType());
                    break;
                }

                if (door->GetGoState() == GO_STATE_READY)
                    break;                                  //door already closed

                door->UseDoorOrButton(time_to_open);

                if (target && target->isType(TYPEMASK_GAMEOBJECT) && ((GameObject*)target)->GetGoType()==GAMEOBJECT_TYPE_BUTTON)
                    ((GameObject*)target)->UseDoorOrButton(time_to_open);

                break;
            }
            case SCRIPT_COMMAND_QUEST_EXPLORED:
            {
                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_QUEST_EXPLORED (script id %u) call for NULL source.", step.script->id);
                    break;
                }

                if (!target)
                {
                    sLog.outError("SCRIPT_COMMAND_QUEST_EXPLORED (script id %u) call for NULL target.", step.script->id);
                    break;
                }

                // when script called for item spell casting then target == (unit or GO) and source is player
                WorldObject* worldObject;
                Player* player;

                if (target->GetTypeId() == TYPEID_PLAYER)
                {
                    if (source->GetTypeId() != TYPEID_UNIT && source->GetTypeId() != TYPEID_GAMEOBJECT && source->GetTypeId() != TYPEID_PLAYER)
                    {
                        sLog.outError("SCRIPT_COMMAND_QUEST_EXPLORED (script id %u) call for non-creature, non-gameobject or non-player (TypeId: %u), skipping.", step.script->id, source->GetTypeId());
                        break;
                    }

                    worldObject = (WorldObject*)source;
                    player = (Player*)target;
                }
                else
                {
                    if (target->GetTypeId() != TYPEID_UNIT && target->GetTypeId() != TYPEID_GAMEOBJECT && target->GetTypeId() != TYPEID_PLAYER)
                    {
                        sLog.outError("SCRIPT_COMMAND_QUEST_EXPLORED (script id %u) call for non-creature, non-gameobject or non-player (TypeId: %u), skipping.", step.script->id, target->GetTypeId());
                        break;
                    }

                    if (source->GetTypeId() != TYPEID_PLAYER)
                    {
                        sLog.outError("SCRIPT_COMMAND_QUEST_EXPLORED (script id %u) call for non-player (TypeId: %u), skipping.", step.script->id, source->GetTypeId());
                        break;
                    }

                    worldObject = (WorldObject*)target;
                    player = (Player*)source;
                }

                // quest id and flags checked at script loading
                if ((worldObject->GetTypeId() != TYPEID_UNIT || ((Unit*)worldObject)->isAlive()) &&
                    (step.script->datalong2 == 0 || worldObject->IsWithinDistInMap(player, float(step.script->datalong2))))
                    player->AreaExploredOrEventHappens(step.script->datalong);
                else
                    player->FailQuest(step.script->datalong);

                break;
            }
            case SCRIPT_COMMAND_ACTIVATE_OBJECT:
            {
                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_ACTIVATE_OBJECT must have source caster.");
                    break;
                }

                if (!source->isType(TYPEMASK_UNIT))
                {
                    sLog.outError("SCRIPT_COMMAND_ACTIVATE_OBJECT source caster isn't unit (TypeId: %u), skipping.",source->GetTypeId());
                    break;
                }

                if (!target)
                {
                    sLog.outError("SCRIPT_COMMAND_ACTIVATE_OBJECT call for NULL gameobject.");
                    break;
                }

                if (target->GetTypeId()!=TYPEID_GAMEOBJECT)
                {
                    sLog.outError("SCRIPT_COMMAND_ACTIVATE_OBJECT call for non-gameobject (TypeId: %u), skipping.",target->GetTypeId());
                    break;
                }

                Unit* caster = (Unit*)source;

                GameObject *go = (GameObject*)target;

                go->Use(caster);
                break;
            }
            case SCRIPT_COMMAND_REMOVE_AURA:
            {
                Object* cmdTarget = step.script->datalong2 ? source : target;

                if (!cmdTarget)
                {
                    sLog.outError("SCRIPT_COMMAND_REMOVE_AURA (script id %u) call for NULL %s.", step.script->id, step.script->datalong2 ? "source" : "target");
                    break;
                }

                if (!cmdTarget->isType(TYPEMASK_UNIT))
                {
                    sLog.outError("SCRIPT_COMMAND_REMOVE_AURA (script id %u) %s isn't unit (TypeId: %u), skipping.", step.script->id, step.script->datalong2 ? "source" : "target",cmdTarget->GetTypeId());
                    break;
                }

                ((Unit*)cmdTarget)->RemoveAurasDueToSpell(step.script->datalong);
                break;
            }
            case SCRIPT_COMMAND_CAST_SPELL:
            {
                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_CAST_SPELL (script id %u) must have source caster.", step.script->id);
                    break;
                }

                Object* cmdTarget = step.script->datalong2 & 0x01 ? source : target;

                if (!cmdTarget)
                {
                    sLog.outError("SCRIPT_COMMAND_CAST_SPELL (script id %u) call for NULL %s.", step.script->id, step.script->datalong2 & 0x01 ? "source" : "target");
                    break;
                }

                if (!cmdTarget->isType(TYPEMASK_UNIT))
                {
                    sLog.outError("SCRIPT_COMMAND_CAST_SPELL (script id %u) %s isn't unit (TypeId: %u), skipping.", step.script->id, step.script->datalong2 & 0x01 ? "source" : "target",cmdTarget->GetTypeId());
                    break;
                }

                Unit* spellTarget = (Unit*)cmdTarget;

                Object* cmdSource = step.script->datalong2 & 0x02 ? target : source;

                if (!cmdSource)
                {
                    sLog.outError("SCRIPT_COMMAND_CAST_SPELL (script id %u) call for NULL %s.", step.script->id, step.script->datalong2 & 0x02 ? "target" : "source");
                    break;
                }

                if (!cmdSource->isType(TYPEMASK_UNIT))
                {
                    sLog.outError("SCRIPT_COMMAND_CAST_SPELL (script id %u) %s isn't unit (TypeId: %u), skipping.", step.script->id, step.script->datalong2 & 0x02 ? "target" : "source", cmdSource->GetTypeId());
                    break;
                }

                Unit* spellSource = (Unit*)cmdSource;

                //TODO: when GO cast implemented, code below must be updated accordingly to also allow GO spell cast
                spellSource->CastSpell(spellTarget,step.script->datalong,false);

                break;
            }
            case SCRIPT_COMMAND_PLAY_SOUND:
            {
                if (!source)
                {
                    sLog.outError("SCRIPT_COMMAND_PLAY_SOUND (script id %u) call for NULL creature.", step.script->id);
                    break;
                }

                if (!source->isType(TYPEMASK_WORLDOBJECT))
                {
                    sLog.outError("SCRIPT_COMMAND_PLAY_SOUND (script id %u) call for non-world object (TypeId: %u), skipping.", step.script->id, source->GetTypeId());
                    break;
                }

                WorldObject* pSource = (WorldObject*)source;

                // bitmask: 0/1=anyone/target, 0/2=with distance dependent
                Player* pTarget = NULL;

                if (step.script->datalong2 & 1)
                {
                    if (!target)
                    {
                        sLog.outError("SCRIPT_COMMAND_PLAY_SOUND (script id %u) in targeted mode call for NULL target.", step.script->id);
                        break;
                    }

                    if (target->GetTypeId() != TYPEID_PLAYER)
                    {
                        sLog.outError("SCRIPT_COMMAND_PLAY_SOUND (script id %u) in targeted mode call for non-player (TypeId: %u), skipping.", step.script->id, target->GetTypeId());
                        break;
                    }

                    pTarget = (Player*)target;
                }

                // bitmask: 0/1=anyone/target, 0/2=with distance dependent
                if (step.script->datalong2 & 2)
                    pSource->PlayDistanceSound(step.script->datalong, pTarget);
                else
                    pSource->PlayDirectSound(step.script->datalong, pTarget);

                break;
            }
            case SCRIPT_COMMAND_CREATE_ITEM:
            {
                if (!target && !source)
                {
                    sLog.outError("SCRIPT_COMMAND_CREATE_ITEM (script id %u) call for NULL object.", step.script->id);
                    break;
                }

                // only Player
                if ((!target || target->GetTypeId() != TYPEID_PLAYER) && (!source || source->GetTypeId() != TYPEID_PLAYER))
                {
                    sLog.outError("SCRIPT_COMMAND_CREATE_ITEM (script id %u) call for non-player (TypeIdSource: %u)(TypeIdTarget: %u), skipping.", step.script->id, source ? source->GetTypeId() : 0, target ? target->GetTypeId() : 0);
                    break;
                }

                Player* pReceiver = target && target->GetTypeId() == TYPEID_PLAYER ? (Player*)target : (Player*)source;

                if (Item* pItem = pReceiver->StoreNewItemInInventorySlot(step.script->datalong, step.script->datalong2))
                    pReceiver->SendNewItem(pItem, step.script->datalong2, true, false);

                break;
            }
            case SCRIPT_COMMAND_DESPAWN_SELF:
            {
                if (!target && !source)
                {
                    sLog.outError("SCRIPT_COMMAND_DESPAWN_SELF (script id %u) call for NULL object.", step.script->id);
                    break;
                }

                // only creature
                if ((!target || target->GetTypeId() != TYPEID_UNIT) && (!source || source->GetTypeId() != TYPEID_UNIT))
                {
                    sLog.outError("SCRIPT_COMMAND_DESPAWN_SELF (script id %u) call for non-creature (TypeIdSource: %u)(TypeIdTarget: %u), skipping.", step.script->id, source ? source->GetTypeId() : 0, target ? target->GetTypeId() : 0);
                    break;
                }

                Creature* pCreature = target && target->GetTypeId() == TYPEID_UNIT ? (Creature*)target : (Creature*)source;

                pCreature->ForcedDespawn(step.script->datalong);

                break;
            }
            case SCRIPT_COMMAND_PLAY_MOVIE:
            {
                if (!target && !source)
                {
                    sLog.outError("SCRIPT_COMMAND_PLAY_MOVIE (script id %u) call for NULL object.", step.script->id);
                    break;
                }

                // only Player
                if ((!target || target->GetTypeId() != TYPEID_PLAYER) && (!source || source->GetTypeId() != TYPEID_PLAYER))
                {
                    sLog.outError("SCRIPT_COMMAND_PLAY_MOVIE (script id %u) call for non-player (TypeIdSource: %u)(TypeIdTarget: %u), skipping.", step.script->id, source ? source->GetTypeId() : 0, target ? target->GetTypeId() : 0);
                    break;
                }

                Player* pReceiver = target && target->GetTypeId() == TYPEID_PLAYER ? (Player*)target : (Player*)source;

                pReceiver->SendMovieStart(step.script->datalong);

                break;
            }
            default:
                sLog.outError("Unknown SCRIPT_COMMAND_ %u called for script id %u.",step.script->command, step.script->id);
                break;
        }

        m_scriptSchedule.erase(iter);
        sWorld.DecreaseScheduledScriptCount();

        iter = m_scriptSchedule.begin();
    }
}

/**
 * Function return player that in world at CURRENT map
 *
 * Note: This is function preferred if you sure that need player only placed at specific map
 *       This is not true for some spell cast targeting and most packet handlers
 *
 * @param guid must be player guid (HIGHGUID_PLAYER)
 */
Player* Map::GetPlayer(ObjectGuid guid)
{
    Player* plr = ObjectAccessor::FindPlayer(guid);         // return only in world players
    return plr && plr->GetMap() == this ? plr : NULL;
}

/**
 * Function return creature (non-pet and then most summoned by spell creatures, and not vehicle) that in world at CURRENT map
 *
 * @param guid must be creature guid (HIGHGUID_UNIT)
 */
Creature* Map::GetCreature(ObjectGuid guid)
{
    return m_objectsStore.find<Creature>(guid.GetRawValue(), (Creature*)NULL);
}

/**
 * Function return vehicle that in world at CURRENT map
 *
 * @param guid must be vehicle guid (HIGHGUID_VEHICLE)
 */
Vehicle* Map::GetVehicle(ObjectGuid guid)
{
    return m_objectsStore.find<Vehicle>(guid.GetRawValue(), (Vehicle*)NULL);
}

/**
 * Function return pet that in world at CURRENT map
 *
 * @param guid must be pet guid (HIGHGUID_PET)
 */
Pet* Map::GetPet(ObjectGuid guid)
{
    return m_objectsStore.find<Pet>(guid.GetRawValue(), (Pet*)NULL);
}

/**
 * Function return corpse that at CURRENT map
 *
 * Note: corpse can be NOT IN WORLD, so can't be used corspe->GetMap() without pre-check corpse->isInWorld()
 *
 * @param guid must be corpse guid (HIGHGUID_CORPSE)
 */
Corpse* Map::GetCorpse(ObjectGuid guid)
{
    Corpse * ret = ObjectAccessor::GetCorpseInMap(guid,GetId());
    return ret && ret->GetInstanceId() == GetInstanceId() ? ret : NULL;
}

/**
 * Function return non-player unit object that in world at CURRENT map, so creature, or pet, or vehicle
 *
 * @param guid must be non-player unit guid (HIGHGUID_PET HIGHGUID_UNIT HIGHGUID_VEHICLE)
 */
Creature* Map::GetAnyTypeCreature(ObjectGuid guid)
{
    switch(guid.GetHigh())
    {
        case HIGHGUID_UNIT:         return GetCreature(guid);
        case HIGHGUID_PET:          return GetPet(guid);
        case HIGHGUID_VEHICLE:      return GetVehicle(guid);
        default:                    break;
    }

    return NULL;
}

/**
 * Function return gameobject that in world at CURRENT map
 *
 * @param guid must be gameobject guid (HIGHGUID_GAMEOBJECT)
 */
GameObject* Map::GetGameObject(ObjectGuid guid)
{
    return m_objectsStore.find<GameObject>(guid.GetRawValue(), (GameObject*)NULL);
}

/**
 * Function return dynamic object that in world at CURRENT map
 *
 * @param guid must be dynamic object guid (HIGHGUID_DYNAMICOBJECT)
 */
DynamicObject* Map::GetDynamicObject(ObjectGuid guid)
{
    return m_objectsStore.find<DynamicObject>(guid.GetRawValue(), (DynamicObject*)NULL);
}

/**
 * Function return unit in world at CURRENT map
 *
 * Note: in case player guid not always expected need player at current map only.
 *       For example in spell casting can be expected any in world player targeting in some cases
 *
 * @param guid must be unit guid (HIGHGUID_PLAYER HIGHGUID_PET HIGHGUID_UNIT HIGHGUID_VEHICLE)
 */
Unit* Map::GetUnit(ObjectGuid guid)
{
    if (guid.IsPlayer())
        return GetPlayer(guid);

    return GetAnyTypeCreature(guid);
}

/**
 * Function return world object in world at CURRENT map, so any except transports
 */
WorldObject* Map::GetWorldObject(ObjectGuid guid)
{
    switch(guid.GetHigh())
    {
        case HIGHGUID_PLAYER:       return GetPlayer(guid);
        case HIGHGUID_GAMEOBJECT:   return GetGameObject(guid);
        case HIGHGUID_UNIT:         return GetCreature(guid);
        case HIGHGUID_PET:          return GetPet(guid);
        case HIGHGUID_VEHICLE:      return GetVehicle(guid);
        case HIGHGUID_DYNAMICOBJECT:return GetDynamicObject(guid);
        case HIGHGUID_CORPSE:
        {
            // corpse special case, it can be not in world
            Corpse* corpse = GetCorpse(guid);
            return corpse && corpse->IsInWorld() ? corpse : NULL;
        }
        case HIGHGUID_MO_TRANSPORT:
        case HIGHGUID_TRANSPORT:
        default:                    break;
    }

    return NULL;
}

void Map::SendObjectUpdates()
{
    UpdateDataMapType update_players;

    while(!i_objectsToClientUpdate.empty())
    {
        Object* obj = *i_objectsToClientUpdate.begin();
        i_objectsToClientUpdate.erase(i_objectsToClientUpdate.begin());
        obj->BuildUpdateData(update_players);
    }

    WorldPacket packet;                                     // here we allocate a std::vector with a size of 0x10000
    for(UpdateDataMapType::iterator iter = update_players.begin(); iter != update_players.end(); ++iter)
    {
        iter->second.BuildPacket(&packet);
        iter->first->GetSession()->SendPacket(&packet);
        packet.clear();                                     // clean the string
    }
}

uint32 Map::GenerateLocalLowGuid(HighGuid guidhigh)
{
    // TODO: for map local guid counters possible force reload map instead shutdown server at guid counter overflow
    switch(guidhigh)
    {
        case HIGHGUID_DYNAMICOBJECT:
            return m_DynObjectGuids.Generate();
        case HIGHGUID_PET:
            return m_PetGuids.Generate();
        case HIGHGUID_VEHICLE:
            return m_VehicleGuids.Generate();
        default:
            MANGOS_ASSERT(0);
    }

    MANGOS_ASSERT(0);
    return 0;
}
