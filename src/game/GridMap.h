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

#ifndef MANGOS_GRIDMAP_H
#define MANGOS_GRIDMAP_H

#include "Platform/Define.h"
#include "DBCStructure.h"
#include "GridDefines.h"
#include "Object.h"
#include "SharedDefines.h"

#include <bitset>
#include <list>

class Creature;
class Unit;
class WorldPacket;
class InstanceData;
class Group;
class InstanceSave;
struct ScriptInfo;
struct ScriptAction;
class BattleGround;

struct GridMapFileHeader
{
    uint32 mapMagic;
    uint32 versionMagic;
    uint32 buildMagic;
    uint32 areaMapOffset;
    uint32 areaMapSize;
    uint32 heightMapOffset;
    uint32 heightMapSize;
    uint32 liquidMapOffset;
    uint32 liquidMapSize;
};

#define MAP_AREA_NO_AREA      0x0001

struct GridMapAreaHeader
{
    uint32 fourcc;
    uint16 flags;
    uint16 gridArea;
};

#define MAP_HEIGHT_NO_HEIGHT  0x0001
#define MAP_HEIGHT_AS_INT16   0x0002
#define MAP_HEIGHT_AS_INT8    0x0004

struct GridMapHeightHeader
{
    uint32 fourcc;
    uint32 flags;
    float gridHeight;
    float gridMaxHeight;
};

#define MAP_LIQUID_NO_TYPE    0x0001
#define MAP_LIQUID_NO_HEIGHT  0x0002

struct GridMapLiquidHeader
{
    uint32 fourcc;
    uint16 flags;
    uint16 liquidType;
    uint8 offsetX;
    uint8 offsetY;
    uint8 width;
    uint8 height;
    float liquidLevel;
};

enum GridMapLiquidStatus
{
    LIQUID_MAP_NO_WATER     = 0x00000000,
    LIQUID_MAP_ABOVE_WATER  = 0x00000001,
    LIQUID_MAP_WATER_WALK   = 0x00000002,
    LIQUID_MAP_IN_WATER     = 0x00000004,
    LIQUID_MAP_UNDER_WATER  = 0x00000008
};

#define MAP_LIQUID_TYPE_NO_WATER    0x00
#define MAP_LIQUID_TYPE_WATER       0x01
#define MAP_LIQUID_TYPE_OCEAN       0x02
#define MAP_LIQUID_TYPE_MAGMA       0x04
#define MAP_LIQUID_TYPE_SLIME       0x08

#define MAP_ALL_LIQUIDS   (MAP_LIQUID_TYPE_WATER | MAP_LIQUID_TYPE_OCEAN | MAP_LIQUID_TYPE_MAGMA | MAP_LIQUID_TYPE_SLIME)

#define MAP_LIQUID_TYPE_DARK_WATER  0x10
#define MAP_LIQUID_TYPE_WMO_WATER   0x20

struct GridMapLiquidData
{
    uint32 type;
    float level;
    float depth_level;
};

class GridMap
{
    private:

        uint32 m_flags;

        // Area data
        uint16 m_gridArea;
        uint16 *m_area_map;

        // Height level data
        float m_gridHeight;
        float m_gridIntHeightMultiplier;
        union
        {
            float *m_V9;
            uint16 *m_uint16_V9;
            uint8 *m_uint8_V9;
        };
        union
        {
            float *m_V8;
            uint16 *m_uint16_V8;
            uint8 *m_uint8_V8;
        };

        // Liquid data
        uint16 m_liquidType;
        uint8 m_liquid_offX;
        uint8 m_liquid_offY;
        uint8 m_liquid_width;
        uint8 m_liquid_height;
        float m_liquidLevel;
        uint8 *m_liquid_type;
        float *m_liquid_map;

        bool loadAreaData(FILE *in, uint32 offset, uint32 size);
        bool loadHeightData(FILE *in, uint32 offset, uint32 size);
        bool loadGridMapLiquidData(FILE *in, uint32 offset, uint32 size);

        // Get height functions and pointers
        typedef float (GridMap::*pGetHeightPtr) (float x, float y) const;
        pGetHeightPtr m_gridGetHeight;
        float getHeightFromFloat(float x, float y) const;
        float getHeightFromUint16(float x, float y) const;
        float getHeightFromUint8(float x, float y) const;
        float getHeightFromFlat(float x, float y) const;

    public:

        GridMap();
        ~GridMap();

        bool loadData(char *filaname);
        void unloadData();

        static bool ExistMap(uint32 mapid, int gx, int gy);
        static bool ExistVMap(uint32 mapid, int gx, int gy);

        uint16 getArea(float x, float y);
        float getHeight(float x, float y) { return (this->*m_gridGetHeight)(x, y); }
        float getLiquidLevel(float x, float y);
        uint8 getTerrainType(float x, float y);
        GridMapLiquidStatus getLiquidStatus(float x, float y, float z, uint8 ReqLiquidType, GridMapLiquidData *data = 0);
};

#endif
