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

#ifndef MANGOS_NGRID_H
#define MANGOS_NGRID_H

/** NGrid is nothing more than a wrapper of the Grid with an NxN cells
 */

#include "GameSystem/Grid.h"
#include "GameSystem/GridReference.h"
#include "Timer.h"

#include <cassert>

class GridInfo
{
    public:

        GridInfo()
            : i_timer(0), i_unloadActiveLockCount(0), i_unloadExplicitLock(false)
        {
        }

        GridInfo(time_t expiry, bool unload = true )
            : i_timer(expiry), i_unloadActiveLockCount(0), i_unloadExplicitLock(!unload)
        {
        }

        const TimeTracker& getTimeTracker() const { return i_timer; }

        bool getUnloadLock() const
        {
            return i_unloadActiveLockCount || i_unloadExplicitLock;
        }

        void setUnloadExplicitLock( bool on ) { i_unloadExplicitLock = on; }
        void incUnloadActiveLock() { ++i_unloadActiveLockCount; }
        void decUnloadActiveLock() { if (i_unloadActiveLockCount) --i_unloadActiveLockCount; }

        void setTimer(const TimeTracker& pTimer) { i_timer = pTimer; }
        void ResetTimeTracker(time_t interval) { i_timer.Reset(interval); }
        void UpdateTimeTracker(time_t diff) { i_timer.Update(diff); }

    private:

        TimeTracker i_timer;
        uint16 i_unloadActiveLockCount : 16;                    // lock from active object spawn points (prevent clone loading)
        bool i_unloadExplicitLock      : 1;                     // explicit manual lock or config setting
};

typedef enum
{
    GRID_STATE_INVALID = 0,
    GRID_STATE_ACTIVE = 1,
    GRID_STATE_IDLE = 2,
    GRID_STATE_REMOVAL = 3,
    MAX_GRID_STATE = 4
} grid_state_t;

template
<
uint32 N,
class ACTIVE_OBJECT,
class WORLD_OBJECT_TYPES,
class GRID_OBJECT_TYPES
>
class MANGOS_DLL_DECL NGrid
{
    public:

        typedef Grid<ACTIVE_OBJECT, WORLD_OBJECT_TYPES, GRID_OBJECT_TYPES> GridType;

        NGrid(uint32 id, uint32 x, uint32 y, time_t expiry, bool unload = true)
            : i_gridId(id), i_x(x), i_y(y), i_cellstate(GRID_STATE_INVALID), i_GridObjectDataLoaded(false)
        {
            i_GridInfo = GridInfo(expiry, unload);
        }

        const GridType& operator()(uint32 x, uint32 y) const
        {
            assert(x < N);
            assert(y < N);
            return i_cells[x][y];
        }

        GridType& operator()(uint32 x, uint32 y)
        {
            assert(x < N);
            assert(y < N);
            return i_cells[x][y];
        }

        const uint32& GetGridId() const { return i_gridId; }
        void SetGridId(const uint32 id) { i_gridId = id; }
        grid_state_t GetGridState() const { return i_cellstate; }
        void SetGridState(grid_state_t s) { i_cellstate = s; }
        uint32 getX() const { return i_x; }
        uint32 getY() const { return i_y; }

        void link(GridRefManager<NGrid<N, ACTIVE_OBJECT, WORLD_OBJECT_TYPES, GRID_OBJECT_TYPES> >* pTo)
        {
            i_Reference.link(pTo, this);
        }

        bool isGridObjectDataLoaded() const { return i_GridObjectDataLoaded; }
        void setGridObjectDataLoaded(bool pLoaded) { i_GridObjectDataLoaded = pLoaded; }

        GridInfo* getGridInfoRef() { return &i_GridInfo; }
        const TimeTracker& getTimeTracker() const { return i_GridInfo.getTimeTracker(); }
        bool getUnloadLock() const { return i_GridInfo.getUnloadLock(); }
        void setUnloadExplicitLock(bool on) { i_GridInfo.setUnloadExplicitLock(on); }
        void incUnloadActiveLock() { i_GridInfo.incUnloadActiveLock(); }
        void decUnloadActiveLock() { i_GridInfo.decUnloadActiveLock(); }
        void ResetTimeTracker(time_t interval) { i_GridInfo.ResetTimeTracker(interval); }
        void UpdateTimeTracker(time_t diff) { i_GridInfo.UpdateTimeTracker(diff); }

        template<class SPECIFIC_OBJECT>
        void AddWorldObject(const uint32 x, const uint32 y, SPECIFIC_OBJECT *obj)
        {
            getGridType(x, y).AddWorldObject(obj);
        }

        template<class SPECIFIC_OBJECT>
        void RemoveWorldObject(const uint32 x, const uint32 y, SPECIFIC_OBJECT *obj)
        {
            getGridType(x, y).RemoveWorldObject(obj);
        }

        template<class T, class TT>
        void Visit(TypeContainerVisitor<T, TypeMapContainer<TT> > &visitor)
        {
            for (uint32 x = 0; x < N; ++x)
                for (uint32 y = 0; y < N; ++y)
                    i_cells[x][y].Visit(visitor);
        }

        template<class T, class TT>
        void Visit(const uint32 &x, const uint32 &y, TypeContainerVisitor<T, TypeMapContainer<TT> > &visitor)
        {
            getGridType(x, y).Visit(visitor);
        }

        uint32 ActiveObjectsInGrid() const
        {
            uint32 count = 0;
            for (uint32 x = 0; x < N; ++x)
                for (uint32 y = 0; y < N; ++y)
                    count += i_cells[x][y].ActiveObjectsInGrid();

            return count;
        }

        template<class SPECIFIC_OBJECT>
        bool AddGridObject(const uint32 x, const uint32 y, SPECIFIC_OBJECT *obj)
        {
            return getGridType(x, y).AddGridObject(obj);
        }

        template<class SPECIFIC_OBJECT>
        bool RemoveGridObject(const uint32 x, const uint32 y, SPECIFIC_OBJECT *obj)
        {
            return getGridType(x, y).RemoveGridObject(obj);
        }

    private:

        GridType& getGridType(const uint32& x, const uint32& y)
        {
            assert(x < N);
            assert(y < N);
            return i_cells[x][y];
        }

        uint32 i_gridId;
        GridInfo i_GridInfo;
        GridReference<NGrid<N, ACTIVE_OBJECT, WORLD_OBJECT_TYPES, GRID_OBJECT_TYPES> > i_Reference;
        uint32 i_x;
        uint32 i_y;
        grid_state_t i_cellstate;
        GridType i_cells[N][N];
        bool i_GridObjectDataLoaded;
};

#endif
