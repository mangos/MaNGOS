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

/**
 * @addtogroup TransportSystem
 * @{
 *
 * @file Vehicle.cpp
 * This file contains the code needed for CMaNGOS to support vehicles
 * Currently implemented
 * - TODO Board
 * - TODO Unboard
 * - TODO Switch
 * - CanBoard to check if a passenger can board a vehicle
 * - Internal helper to control the available seats of a vehicle
 */

#include "Common.h"
#include "SharedDefines.h"
#include "ObjectGuid.h"
#include "Log.h"
#include "Unit.h"
#include "Creature.h"
#include "ObjectMgr.h"
#include "Vehicle.h"
#include "Util.h"
#include "movement/MoveSplineInit.h"
#include "movement/MoveSpline.h"
#include "MapManager.h"

/**
 * Constructor of VehicleInfo
 *
 * @param owner         MUST be provided owner of the vehicle (type Unit)
 * @param vehicleEntry  MUST be provided dbc-entry of the vehicle
 *
 * This function will initialise the VehicleInfo of the vehicle owner
 * Also the seat-map is created here
 */
VehicleInfo::VehicleInfo(Unit* owner, VehicleEntry const* vehicleEntry) : TransportBase(owner),
    m_vehicleEntry(vehicleEntry),
    m_creatureSeats(0),
    m_playerSeats(0)
{
    MANGOS_ASSERT(vehicleEntry);

    // Initial fill of available seats for the vehicle
    for (uint8 i = 0; i < MAX_VEHICLE_SEAT; ++i)
    {
        if (uint32 seatId = vehicleEntry->m_seatID[i])
        {
            if (VehicleSeatEntry const* seatEntry = sVehicleSeatStore.LookupEntry(seatId))
            {
                m_vehicleSeats.insert(VehicleSeatMap::value_type(i, seatEntry));

                if (IsUsableSeatForCreature(seatEntry->m_flags))
                    m_creatureSeats |= 1 << i;

                if (IsUsableSeatForPlayer(seatEntry->m_flags))
                    m_playerSeats |= 1 << i;
            }
        }
    }
}

/**
 * This function will board a passenger onto a vehicle
 *
 * @param passenger MUST be provided. This Unit will be boarded onto the vehicles (if it checks out)
 * @param seat      Seat to which the passenger will be boarded (if can, elsewise an alternative will be selected if possible)
 */
void VehicleInfo::Board(Unit* passenger, uint8 seat)
{
    MANGOS_ASSERT(passenger);

    DEBUG_LOG("VehicleInfo::Board: Try to board passenger %s to seat %u", passenger->GetObjectGuid().GetString().c_str(), seat);
}

/**
 * This function will switch the seat of a passenger
 *
 * @param passenger MUST be provided. This Unit will change its seat on the vehicle
 * @param seat      Seat to which the passenger will be switched
 */
void VehicleInfo::SwitchSeat(Unit* passenger, uint8 seat)
{
    MANGOS_ASSERT(passenger);
}

/**
 * This function will Unboard a passenger
 *
 * @param passenger         MUST be provided. This Unit will be unboarded from the vehicle
 * @param changeVehicle     If set, the passenger is expected to be directly boarded to another vehicle,
 *                          and hence he will not be unboarded but only removed from this vehicle.
 */
void VehicleInfo::UnBoard(Unit* passenger, bool changeVehicle)
{
    MANGOS_ASSERT(passenger);

    DEBUG_LOG("VehicleInfo::Unboard: passenger: %s", passenger->GetObjectGuid().GetString().c_str());
}

/**
 * This function will check if a passenger can be boarded
 *
 * @param passenger         Unit that attempts to board onto a vehicle
 */
bool VehicleInfo::CanBoard(Unit* passenger) const
{
    if (!passenger)
        return false;

    // Passenger is this vehicle
    if (passenger == m_owner)
        return false;

    // Check if we have at least one empty seat
    if (!GetEmptySeats())
        return false;

    // Passenger is already boarded
    if (m_passengers.find(passenger) != m_passengers.end())
        return false;

    // Check for empty player seats
    if (passenger->GetTypeId() == TYPEID_PLAYER)
        return GetEmptySeatsMask() & m_playerSeats;

    // Check for empty creature seats
    return GetEmptySeatsMask() & m_creatureSeats;
}

// Helper function to undo the turning of the vehicle to calculate a relative position of the passenger when boarding
void VehicleInfo::CalculateBoardingPositionOf(float gx, float gy, float gz, float go, float &lx, float &ly, float &lz, float &lo)
{
    NormalizeRotatedPosition(gx - m_owner->GetPositionX(), gy - m_owner->GetPositionY(), lx, ly);

    lz = gz - m_owner->GetPositionZ();
    lo = NormalizeOrientation(go - m_owner->GetOrientation());
}

/* ************************************************************************************************
 *          Helper function for seat control
 * ***********************************************************************************************/

/// Get the Vehicle SeatEntry of a seat by position
VehicleSeatEntry const* VehicleInfo::GetSeatEntry(uint8 seat) const
{
    VehicleSeatMap::const_iterator itr = m_vehicleSeats.find(seat);
    return itr != m_vehicleSeats.end() ? itr->second : NULL;
}

/**
 * This function will get a usable seat for a passenger
 *
 * @param passenger         MUST be provided. Unit for which to try to get a free seat
 * @param seat              will contain an available seat if returned true
 * @return                  return TRUE if and only if an available seat was found. In this case @seat will contain the id
 */
bool VehicleInfo::GetUsableSeatFor(Unit* passenger, uint8& seat) const
{
    MANGOS_ASSERT(passenger);

    uint8 possibleSeats = (passenger->GetTypeId() == TYPEID_PLAYER) ? (GetEmptySeatsMask() & m_playerSeats) : (GetEmptySeatsMask() & m_creatureSeats);

    // No usable seats available
    if (!possibleSeats)
        return false;

    // Start with 0
    seat = 0;

    for (uint8 i = 1; seat < MAX_VEHICLE_SEAT; i <<= 1, ++seat)
        if (possibleSeats & i)
            return true;

    return false;
}

/// Returns if a @passenger could board onto @seat - @passenger MUST be provided
bool VehicleInfo::IsSeatAvailableFor(Unit* passenger, uint8 seat) const
{
    MANGOS_ASSERT(passenger);

    return seat < MAX_VEHICLE_SEAT &&
            (GetEmptySeatsMask() & (passenger->GetTypeId() == TYPEID_PLAYER ? m_playerSeats : m_creatureSeats) & (1 << seat));
}

/// Wrapper to collect all taken seats
uint8 VehicleInfo::GetTakenSeatsMask() const
{
    uint8 takenSeatsMask = 0;

    for (PassengerMap::const_iterator itr = m_passengers.begin(); itr != m_passengers.end(); ++itr)
        takenSeatsMask |= 1 << itr->second->GetTransportSeat();

    return takenSeatsMask;
}

bool VehicleInfo:: IsUsableSeatForPlayer(uint32 seatFlags) const
{
    return seatFlags & SEAT_FLAG_USABLE;
}

/// Add control and such modifiers to a passenger if required
void VehicleInfo::ApplySeatMods(Unit* passenger, uint32 seatFlags)
{
}

/// Remove control and such modifiers to a passenger if they were added
void VehicleInfo::RemoveSeatMods(Unit* passenger, uint32 seatFlags)
{
}

/*! @} */
