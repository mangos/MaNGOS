/*
 * Copyright (C) 2005-2013 MaNGOS <http://getmangos.com/>
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

/**
 * @addtogroup TransportSystem
 * @{
 *
 * @file TransportSystem.cpp
 * This file contains the code needed for MaNGOS to provide abstract support for transported entities
 * Currently implemented
 * - Calculating between local and global coords
 * - Abstract storage of passengers (added by BoardPassenger, UnboardPassenger)
 */

#include "TransportSystem.h"
#include "Unit.h"
#include "Vehicle.h"
#include "MapManager.h"

/* **************************************** TransportBase ****************************************/

TransportBase::TransportBase(WorldObject* owner) :
    m_owner(owner),
    m_lastPosition(owner->GetPositionX(), owner->GetPositionY(), owner->GetPositionZ(), owner->GetOrientation()),
    m_sinO(sin(m_lastPosition.o)),
    m_cosO(cos(m_lastPosition.o)),
    m_updatePositionsTimer(500)
{
    MANGOS_ASSERT(m_owner);
}

TransportBase::~TransportBase()
{
    MANGOS_ASSERT(m_passengers.size() == 0);
}

// Update every now and then (after some change of transporter's position)
// This is used to calculate global positions (which don't have to be exact, they are only required for some server-side calculations
void TransportBase::Update(uint32 diff)
{
    if (m_updatePositionsTimer < diff)
    {
        if (fabs(m_owner->GetPositionX() - m_lastPosition.x) +
                fabs(m_owner->GetPositionY() - m_lastPosition.y) +
                fabs(m_owner->GetPositionZ() - m_lastPosition.z) > 1.0f ||
                NormalizeOrientation(m_owner->GetOrientation() - m_lastPosition.o) > 0.01f)
            UpdateGlobalPositions();

        m_updatePositionsTimer = 500;
    }
    else
        m_updatePositionsTimer -= diff;
}

// Update the global positions of all passengers
void TransportBase::UpdateGlobalPositions()
{
    Position pos(m_owner->GetPositionX(), m_owner->GetPositionY(),
                 m_owner->GetPositionZ(), m_owner->GetOrientation());

    // Calculate new direction multipliers
    if (NormalizeOrientation(pos.o - m_lastPosition.o) > 0.01f)
    {
        m_sinO = sin(pos.o);
        m_cosO = cos(pos.o);
    }

    // Update global positions
    for (PassengerMap::const_iterator itr = m_passengers.begin(); itr != m_passengers.end(); ++itr)
        UpdateGlobalPositionOf(itr->first, itr->second->GetLocalPositionX(), itr->second->GetLocalPositionY(),
                               itr->second->GetLocalPositionZ(), itr->second->GetLocalOrientation());

    m_lastPosition = pos;
}

// Update the global position of a passenger
void TransportBase::UpdateGlobalPositionOf(WorldObject* passenger, float lx, float ly, float lz, float lo) const
{
    float gx, gy, gz, go;
    CalculateGlobalPositionOf(lx, ly, lz, lo, gx, gy, gz, go);

    if (passenger->GetTypeId() == TYPEID_PLAYER || passenger->GetTypeId() == TYPEID_UNIT)
    {
        if (passenger->GetTypeId() == TYPEID_PLAYER)
        {
            m_owner->GetMap()->PlayerRelocation((Player*)passenger, gx, gy, gz, go);
        }
        else
            m_owner->GetMap()->CreatureRelocation((Creature*)passenger, gx, gy, gz, go);

        // If passenger is vehicle
        if (((Unit*)passenger)->IsVehicle())
            ((Unit*)passenger)->GetVehicleInfo()->UpdateGlobalPositions();
    }
    // ToDo: Add gameobject relocation
    // ToDo: Add passenger relocation for MO transports
}

// This rotates the vector (lx, ly) by transporter->orientation
void TransportBase::RotateLocalPosition(float lx, float ly, float& rx, float& ry) const
{
    rx = lx * m_cosO - ly * m_sinO;
    ry = lx * m_sinO + ly * m_cosO;
}

// This rotates the vector (rx, ry) by -transporter->orientation
void TransportBase::NormalizeRotatedPosition(float rx, float ry, float& lx, float& ly) const
{
    lx = rx * -m_cosO - ry * -m_sinO;
    ly = rx * -m_sinO + ry * -m_cosO;
}

// Calculate a global position of local positions based on this transporter
void TransportBase::CalculateGlobalPositionOf(float lx, float ly, float lz, float lo, float& gx, float& gy, float& gz, float& go) const
{
    RotateLocalPosition(lx, ly, gx, gy);
    gx += m_owner->GetPositionX();
    gy += m_owner->GetPositionY();

    gz = lz + m_owner->GetPositionZ();
    go = NormalizeOrientation(lo + m_owner->GetOrientation());
}

//  Helper function to check if a unit is boarded onto this transporter (or a transporter boarded onto this) recursively
bool TransportBase::HasOnBoard(WorldObject const* passenger) const
{
    MANGOS_ASSERT(passenger);

    // For efficiency we go down from the (possible) passenger until we reached our owner, or until we reached no passenger
    // Note, this will not catch, if self and passenger are boarded onto the same transporter (as it should not)
    while (passenger->IsBoarded())
    {
        // pasenger is boarded onto this
        if (passenger->GetTransportInfo()->GetTransport() == m_owner)
            return true;
        else
            passenger = passenger->GetTransportInfo()->GetTransport();
    }

    return false;
}

void TransportBase::BoardPassenger(WorldObject* passenger, float lx, float ly, float lz, float lo, uint8 seat)
{
    TransportInfo* transportInfo = new TransportInfo(passenger, this, lx, ly, lz, lo, seat);

    // Insert our new passenger
    m_passengers.insert(PassengerMap::value_type(passenger, transportInfo));

    // The passenger needs fast access to transportInfo
    passenger->SetTransportInfo(transportInfo);
}

void TransportBase::UnBoardPassenger(WorldObject* passenger)
{
    PassengerMap::iterator itr = m_passengers.find(passenger);

    if (itr == m_passengers.end())
        return;

    // Set passengers transportInfo to NULL
    passenger->SetTransportInfo(NULL);

    // Delete transportInfo
    delete itr->second;

    // Unboard finally
    m_passengers.erase(itr);
}

/* **************************************** TransportInfo ****************************************/

TransportInfo::TransportInfo(WorldObject* owner, TransportBase* transport, float lx, float ly, float lz, float lo, uint8 seat) :
    m_owner(owner),
    m_transport(transport),
    m_localPosition(lx, ly, lz, lo),
    m_seat(seat)
{
    MANGOS_ASSERT(owner && m_transport);
}

void TransportInfo::SetLocalPosition(float lx, float ly, float lz, float lo)
{
    m_localPosition.x = lx;
    m_localPosition.y = ly;
    m_localPosition.z = lz;
    m_localPosition.o = lo;

    // Update global position
    m_transport->UpdateGlobalPositionOf(m_owner, lx, ly, lz, lo);
}

/*! @} */
