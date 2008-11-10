/*
 * Copyright (C) 2005-2008 MaNGOS <http://getmangos.com/>
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

#include "Common.h"
#include "Log.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Vehicle.h"
#include "MapManager.h"
#include "SpellAuras.h"
#include "Unit.h"
#include "Util.h"

Vehicle::Vehicle() : Creature()
{
    m_updateFlag = (UPDATEFLAG_LOWGUID | UPDATEFLAG_HIGHGUID | UPDATEFLAG_LIVING | UPDATEFLAG_HAS_POSITION | UPDATEFLAG_VEHICLE);
}

Vehicle::~Vehicle()
{
    if(m_uint32Values)                                      // only for fully created Object
        ObjectAccessor::Instance().RemoveObject(this);
}

void Vehicle::AddToWorld()
{
    ///- Register the vehicle for guid lookup
    if(!IsInWorld()) ObjectAccessor::Instance().AddObject(this);
    Unit::AddToWorld();
}

void Vehicle::RemoveFromWorld()
{
    ///- Remove the vehicle from the accessor
    if(IsInWorld()) ObjectAccessor::Instance().RemoveObject(this);
    ///- Don't call the function for Creature, normal mobs + totems go in a different storage
    Unit::RemoveFromWorld();
}

void Vehicle::setDeathState(DeathState s)                       // overwrite virtual Creature::setDeathState and Unit::setDeathState
{
    Creature::setDeathState(s);
}

void Vehicle::Update(uint32 diff)
{
    Creature::Update(diff);
}

bool Vehicle::Create(uint32 guidlow, Map *map, uint32 Entry)
{
    SetMapId(map->GetId());
    SetInstanceId(map->GetInstanceId());

    Object::_Create(guidlow, Entry, HIGHGUID_VEHICLE);

    if(!InitEntry(Entry))
        return false;

    return true;
}
