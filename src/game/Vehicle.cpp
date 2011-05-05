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

#include "Common.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Vehicle.h"
#include "Unit.h"
#include "Util.h"

Vehicle::Vehicle() : Creature(CREATURE_SUBTYPE_VEHICLE), m_vehicleId(0)
{
    m_updateFlag = (UPDATEFLAG_LIVING | UPDATEFLAG_HAS_POSITION | UPDATEFLAG_VEHICLE);
}

Vehicle::~Vehicle()
{
}

void Vehicle::AddToWorld()
{
    ///- Register the vehicle for guid lookup
    if(!IsInWorld())
        GetMap()->GetObjectsStore().insert<Vehicle>(GetGUID(), (Vehicle*)this);

    Unit::AddToWorld();
}

void Vehicle::RemoveFromWorld()
{
    ///- Remove the vehicle from the accessor
    if(IsInWorld())
        GetMap()->GetObjectsStore().erase<Vehicle>(GetGUID(), (Vehicle*)NULL);

    ///- Don't call the function for Creature, normal mobs + totems go in a different storage
    Unit::RemoveFromWorld();
}

void Vehicle::SetDeathState(DeathState s)                       // overwrite virtual Creature::SetDeathState and Unit::SetDeathState
{
    Creature::SetDeathState(s);
}

void Vehicle::Update( uint32 update_diff, uint32 diff)
{
    Creature::Update(update_diff, diff);
}

bool Vehicle::Create(uint32 guidlow, Map *map, uint32 Entry, uint32 vehicleId, Team team)
{
    SetMap(map);

    Object::_Create(guidlow, Entry, HIGHGUID_VEHICLE);

    if(!InitEntry(Entry))
        return false;

    m_defaultMovementType = IDLE_MOTION_TYPE;

    AIM_Initialize();

    SetVehicleId(vehicleId);

    SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    SetFloatValue(UNIT_FIELD_HOVERHEIGHT, 1.0f);

    CreatureInfo const *ci = GetCreatureInfo();
    setFaction(team == ALLIANCE ? ci->faction_A : ci->faction_H);

    SelectLevel(ci);

    return true;
}

void Vehicle::Dismiss()
{
    SendObjectDeSpawnAnim(GetGUID());
    CombatStop();
    AddObjectToRemoveList();
}
