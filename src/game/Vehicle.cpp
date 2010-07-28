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

#include "Common.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Vehicle.h"
#include "Unit.h"
#include "Util.h"
#include "WorldPacket.h"
#include "InstanceData.h"
#include "ZoneScript.h"

VehicleKit::VehicleKit(Unit* base, VehicleEntry const* vehicleInfo) : m_vehicleInfo(vehicleInfo), m_pBase(base), m_uiNumFreeSeats(0)
{
    for (uint32 i = 0; i < MAX_VEHICLE_SEAT; ++i)
    {
        uint32 seatId = m_vehicleInfo->m_seatID[i];

        if (!seatId)
            continue;

        if (VehicleSeatEntry const *veSeat = sVehicleSeatStore.LookupEntry(seatId))
        {
            m_Seats.insert(std::make_pair(i, VehicleSeat(veSeat)));

            if (veSeat->IsUsable())
                ++m_uiNumFreeSeats;
        }
    }
}

VehicleKit::~VehicleKit()
{
}

void VehicleKit::RemoveAllPassengers()
{
    for (SeatMap::iterator itr = m_Seats.begin(); itr != m_Seats.end(); ++itr)
    {
        if (Unit *passenger = itr->second.passenger)
            passenger->ExitVehicle();
    }
}

bool VehicleKit::HasEmptySeat(int8 seatId) const
{
    SeatMap::const_iterator seat = m_Seats.find(seatId);

    if (seat == m_Seats.end())
        return false;

    return !seat->second.passenger;
}

Unit *VehicleKit::GetPassenger(int8 seatId) const
{
    SeatMap::const_iterator seat = m_Seats.find(seatId);

    if (seat == m_Seats.end())
        return NULL;

    return seat->second.passenger;
}

int8 VehicleKit::GetNextEmptySeat(int8 seatId, bool next) const
{
    SeatMap::const_iterator seat = m_Seats.find(seatId);

    if (seat == m_Seats.end())
        return -1;

    while (seat->second.passenger || !seat->second.seatInfo->IsUsable())
    {
        if (next)
        {
            ++seat;
            if (seat == m_Seats.end())
                seat = m_Seats.begin();
        }
        else
        {
            if (seat == m_Seats.begin())
                seat = m_Seats.end();
            --seat;
        }

        if (seat->first == seatId)
            return -1; // no available seat
    }

    return seat->first;
}

bool VehicleKit::AddPassenger(Unit *unit, int8 seatId)
{
    SeatMap::iterator seat;

    if (seatId < 0) // no specific seat requirement
    {
        for (seat = m_Seats.begin(); seat != m_Seats.end(); ++seat)
        {
            if (!seat->second.passenger && seat->second.seatInfo->IsUsable())
                break;
        }

        if (seat == m_Seats.end()) // no available seat
            return false;
    }
    else
    {
        seat = m_Seats.find(seatId);

        if (seat == m_Seats.end())
            return false;

        if (seat->second.passenger)
            seat->second.passenger->ExitVehicle();
    }

    seat->second.passenger = unit;

    if (seat->second.seatInfo->IsUsable())
    {
        ASSERT(m_uiNumFreeSeats);
        --m_uiNumFreeSeats;

        if (!m_uiNumFreeSeats)
        {
            if (m_pBase->GetTypeId() == TYPEID_PLAYER)
                m_pBase->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_PLAYER_VEHICLE);
            else
                m_pBase->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }
    }

    unit->addUnitState(UNIT_STAT_ON_VEHICLE);

    VehicleSeatEntry const *veSeat = seat->second.seatInfo;

    unit->m_movementInfo.AddMovementFlag(MOVEFLAG_ONTRANSPORT);
    unit->m_movementInfo.SetTransportData(m_pBase->GetGUID(),
        veSeat->m_attachmentOffsetX, veSeat->m_attachmentOffsetY, veSeat->m_attachmentOffsetZ,
        veSeat->m_passengerYaw, getMSTime(), seat->first, veSeat);

    if (unit->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)unit)->GetCamera().SetView(m_pBase);

        WorldPacket data(SMSG_FORCE_MOVE_ROOT, 8+4);
        data << unit->GetPackGUID();
        data << uint32((unit->m_movementInfo.GetVehicleSeatFlags() & SEAT_FLAG_CAN_CAST) ? 2 : 0);
        unit->SendMessageToSet(&data, true);
    }

    unit->SendMonsterMoveTransport(m_pBase, SPLINETYPE_FACINGANGLE, SPLINEFLAG_UNKNOWN5, 0, 0.0f);

    if (m_pBase->GetTypeId() == TYPEID_UNIT)
        RelocatePassengers(m_pBase->GetPositionX(), m_pBase->GetPositionY(), m_pBase->GetPositionZ(), m_pBase->GetOrientation());

    return true;
}

void VehicleKit::RemovePassenger(Unit *unit)
{
    SeatMap::iterator seat;
    for (seat = m_Seats.begin(); seat != m_Seats.end(); ++seat)
    {
        if (seat->second.passenger == unit)
            break;
    }

    if (seat == m_Seats.end())
        return;

    seat->second.passenger = NULL;

    if (seat->second.seatInfo->IsUsable())
    {
        if (!m_uiNumFreeSeats)
        {
            if (m_pBase->GetTypeId() == TYPEID_PLAYER)
                m_pBase->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_PLAYER_VEHICLE);
            else
                m_pBase->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }

        ++m_uiNumFreeSeats;
    }

    unit->clearUnitState(UNIT_STAT_ON_VEHICLE);

    unit->m_movementInfo.ClearTransportData();
    unit->m_movementInfo.RemoveMovementFlag(MOVEFLAG_ONTRANSPORT);

    if (unit->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)unit)->GetCamera().ResetView();

        WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 8+4);
        data << unit->GetPackGUID();
        data << uint32(0);
        unit->SendMessageToSet(&data, true);
    }
}

void VehicleKit::Reset()
{
    if (m_pBase->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_uiNumFreeSeats)
            m_pBase->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_PLAYER_VEHICLE);
    }
    else
    {
        if (m_uiNumFreeSeats)
            m_pBase->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    }
}

void VehicleKit::RelocatePassengers(float x, float y, float z, float ang)
{
    for (SeatMap::const_iterator itr = m_Seats.begin(); itr != m_Seats.end(); ++itr)
    {
        if (Unit *passenger = itr->second.passenger)
        {
            float px = x + passenger->m_movementInfo.GetTransportPos()->x;
            float py = y + passenger->m_movementInfo.GetTransportPos()->y;
            float pz = z + passenger->m_movementInfo.GetTransportPos()->z;
            float po = ang + passenger->m_movementInfo.GetTransportPos()->o;

            passenger->SetPosition(px, py, pz, po);
        }
    }
}

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

void Vehicle::setDeathState(DeathState s)                       // overwrite virtual Creature::setDeathState and Unit::setDeathState
{
    Creature::setDeathState(s);
}

void Vehicle::Update(uint32 diff)
{
    Creature::Update(diff);
}

bool Vehicle::Create(uint32 guidlow, Map *map, uint32 Entry, uint32 vehicleId, uint32 team)
{
    SetMap(map);

    Object::_Create(guidlow, Entry, HIGHGUID_VEHICLE);

    if(!InitEntry(Entry, team))
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
