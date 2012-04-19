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

#ifndef MANGOS_OBJECTGRIDLOADER_H
#define MANGOS_OBJECTGRIDLOADER_H

#include "Common.h"
#include "Utilities/TypeList.h"
#include "Platform/Define.h"
#include "GameSystem/GridLoader.h"
#include "GridDefines.h"
#include "Cell.h"

class ObjectWorldLoader;

class MANGOS_DLL_DECL ObjectGridLoader
{
    friend class ObjectWorldLoader;

    public:
        ObjectGridLoader(NGridType &grid, Map* map, const Cell &cell)
            : i_cell(cell), i_grid(grid), i_map(map), i_gameObjects(0), i_creatures(0), i_corpses (0)
            {}

        void Load(GridType &grid);
        void Visit(GameObjectMapType &m);
        void Visit(CreatureMapType &m);
        void Visit(CorpseMapType &) {}

        void Visit(DynamicObjectMapType&) { }

        void LoadN(void);

    private:
        Cell i_cell;
        NGridType &i_grid;
        Map* i_map;
        uint32 i_gameObjects;
        uint32 i_creatures;
        uint32 i_corpses;
};

class MANGOS_DLL_DECL ObjectGridUnloader
{
    public:
        ObjectGridUnloader(NGridType &grid) : i_grid(grid) {}

        void MoveToRespawnN();
        void UnloadN()
        {
            for(unsigned int x=0; x < MAX_NUMBER_OF_CELLS; ++x)
            {
                for(unsigned int y=0; y < MAX_NUMBER_OF_CELLS; ++y)
                {
                    GridLoader<Player, AllWorldObjectTypes, AllGridObjectTypes> loader;
                    loader.Unload(i_grid(x, y), *this);
                }
            }
        }

        void Unload(GridType &grid);
        template<class T> void Visit(GridRefManager<T> &m);
    private:
        NGridType &i_grid;
};

class MANGOS_DLL_DECL ObjectGridStoper
{
    public:
        ObjectGridStoper(NGridType &grid) : i_grid(grid) {}

        void MoveToRespawnN();
        void StopN()
        {
            for(unsigned int x=0; x < MAX_NUMBER_OF_CELLS; ++x)
            {
                for(unsigned int y=0; y < MAX_NUMBER_OF_CELLS; ++y)
                {
                    GridLoader<Player, AllWorldObjectTypes, AllGridObjectTypes> loader;
                    loader.Stop(i_grid(x, y), *this);
                }
            }
        }

        void Stop(GridType &grid);
        void Visit(CreatureMapType &m);

        template<class NONACTIVE> void Visit(GridRefManager<NONACTIVE> &) {}
    private:
        NGridType &i_grid;
};

typedef GridLoader<Player, AllWorldObjectTypes, AllGridObjectTypes> GridLoaderType;
#endif
