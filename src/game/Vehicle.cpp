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
 * - Board to board a passenger onto a vehicle (includes checks)
 * - Unboard to unboard a passenger from the vehicle
 * - SwitchSeat to switch to another seat of the same vehicle
 * - CanBoard to check if a passenger can board a vehicle
 * - Internal helper to set the controlling and spells for a vehicle's seat
 * - Internal helper to control the available seats of a vehicle
 */

#include "Common.h"
#include "SharedDefines.h"
#include "ObjectGuid.h"
#include "Log.h"
#include "Unit.h"
#include "Creature.h"
#include "ObjectMgr.h"
#include "SQLStorages.h"
#include "Vehicle.h"
#include "Util.h"
#include "movement/MoveSplineInit.h"
#include "movement/MoveSpline.h"
#include "MapManager.h"
#include "TemporarySummon.h"

void ObjectMgr::LoadVehicleAccessory()
{
    sVehicleAccessoryStorage.Load();

    sLog.outString(">> Loaded %u vehicle accessories", sVehicleAccessoryStorage.GetRecordCount());
    sLog.outString();

    // Check content
    for (SQLMultiStorage::SQLSIterator<VehicleAccessory> itr = sVehicleAccessoryStorage.getDataBegin<VehicleAccessory>(); itr < sVehicleAccessoryStorage.getDataEnd<VehicleAccessory>(); ++itr)
    {
        if (!sCreatureStorage.LookupEntry<CreatureInfo>(itr->vehicleEntry))
        {
            sLog.outErrorDb("Table `vehicle_accessory` has entry (vehicle entry: %u, seat %u, passenger %u) where vehicle_entry is invalid, skip vehicle.", itr->vehicleEntry, itr->seatId, itr->passengerEntry);
            sVehicleAccessoryStorage.EraseEntry(itr->vehicleEntry);
            continue;
        }
        if (!sCreatureStorage.LookupEntry<CreatureInfo>(itr->passengerEntry))
        {
            sLog.outErrorDb("Table `vehicle_accessory` has entry (vehicle entry: %u, seat %u, passenger %u) where accessory_entry is invalid, skip vehicle.", itr->vehicleEntry, itr->seatId, itr->passengerEntry);
            sVehicleAccessoryStorage.EraseEntry(itr->vehicleEntry);
            continue;
        }
        if (itr->seatId >= MAX_VEHICLE_SEAT)
        {
            sLog.outErrorDb("Table `vehicle_accessory` has entry (vehicle entry: %u, seat %u, passenger %u) where seat is invalid (must be between 0 and %u), skip vehicle.", itr->vehicleEntry, itr->seatId, itr->passengerEntry, MAX_VEHICLE_SEAT - 1);
            sVehicleAccessoryStorage.EraseEntry(itr->vehicleEntry);
            continue;
        }
    }
}

/**
 * Constructor of VehicleInfo
 *
 * @param owner         MUST be provided owner of the vehicle (type Unit)
 * @param vehicleEntry  MUST be provided dbc-entry of the vehicle
 * @param overwriteNpcEntry Use to overwrite the GetEntry() result for selecting associated passengers
 *
 * This function will initialise the VehicleInfo of the vehicle owner
 * Also the seat-map is created here
 */
VehicleInfo::VehicleInfo(Unit* owner, VehicleEntry const* vehicleEntry, uint32 overwriteNpcEntry) : TransportBase(owner),
    m_vehicleEntry(vehicleEntry),
    m_creatureSeats(0),
    m_playerSeats(0),
    m_overwriteNpcEntry(overwriteNpcEntry),
    m_isInitialized(false)
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

VehicleInfo::~VehicleInfo()
{
    ((Unit*)m_owner)->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE);

    RemoveAccessoriesFromMap();                             // Remove accessories (for example required with player vehicles)
}

void VehicleInfo::Initialize()
{
    if (!m_overwriteNpcEntry)
        m_overwriteNpcEntry = m_owner->GetEntry();

    // Loading passengers (rough version only!)
    SQLMultiStorage::SQLMSIteratorBounds<VehicleAccessory> bounds = sVehicleAccessoryStorage.getBounds<VehicleAccessory>(m_overwriteNpcEntry);
    for (SQLMultiStorage::SQLMultiSIterator<VehicleAccessory> itr = bounds.first; itr != bounds.second; ++itr)
    {
        if (Creature* summoned = m_owner->SummonCreature(itr->passengerEntry, m_owner->GetPositionX(), m_owner->GetPositionY(), m_owner->GetPositionZ(), m_owner->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0))
        {
            m_accessoryGuids.insert(summoned->GetObjectGuid());
            int32 basepoint0 = itr->seatId + 1;
            summoned->CastCustomSpell((Unit*)m_owner, SPELL_RIDE_VEHICLE_HARDCODED, &basepoint0, NULL, NULL, true);
        }
    }
    m_isInitialized = true;
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

    DEBUG_LOG("VehicleInfo::Board: Try to board passenger %s to seat %u", passenger->GetGuidStr().c_str(), seat);

    // This check is also called in Spell::CheckCast()
    if (!CanBoard(passenger))
        return;

    // Use the planned seat only if the seat is valid, possible to choose and empty
    if (!IsSeatAvailableFor(passenger, seat))
        if (!GetUsableSeatFor(passenger, seat))
            return;

    VehicleSeatEntry const* seatEntry = GetSeatEntry(seat);
    MANGOS_ASSERT(seatEntry);

    // ToDo: Unboard passenger from a MOTransport when they are properly implemented
    /*if (TransportInfo* transportInfo = passenger->GetTransportInfo())
    {
        WorldObject* transporter = transportInfo->GetTransport();

        // Must be a MO transporter
        MANGOS_ASSERT(transporter->GetObjectGuid().IsMOTransport());

        ((Transport*)transporter)->UnBoardPassenger(passenger);
    }*/

    DEBUG_LOG("VehicleInfo::Board: Board passenger: %s to seat %u", passenger->GetGuidStr().c_str(), seat);

    // Calculate passengers local position
    float lx, ly, lz, lo;
    CalculateBoardingPositionOf(passenger->GetPositionX(), passenger->GetPositionY(), passenger->GetPositionZ(), passenger->GetOrientation(), lx, ly, lz, lo);

    BoardPassenger(passenger, lx, ly, lz, lo, seat);        // Use TransportBase to store the passenger

    // Set data for createobject packets
    passenger->m_movementInfo.SetTransportData(m_owner->GetObjectGuid(), lx, ly, lz, lo, 0, seat);

    if (passenger->GetTypeId() == TYPEID_PLAYER)
    {
        Player* pPlayer = (Player*)passenger;
        pPlayer->RemovePet(PET_SAVE_AS_CURRENT);

        WorldPacket data(SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA);
        pPlayer->GetSession()->SendPacket(&data);

        // SMSG_BREAK_TARGET (?)
    }

    if (!passenger->IsRooted())
        passenger->SetRoot(true);

    Movement::MoveSplineInit init(*passenger);
    init.MoveTo(0.0f, 0.0f, 0.0f);                          // ToDo: Set correct local coords
    init.SetFacing(0.0f);                                   // local orientation ? ToDo: Set proper orientation!
    init.SetBoardVehicle();
    init.Launch();

    // Apply passenger modifications
    ApplySeatMods(passenger, seatEntry->m_flags);
}

/**
 * This function will switch the seat of a passenger on the same vehicle
 *
 * @param passenger MUST be provided. This Unit will change its seat on the vehicle
 * @param seat      Seat to which the passenger will be switched
 */
void VehicleInfo::SwitchSeat(Unit* passenger, uint8 seat)
{
    MANGOS_ASSERT(passenger);

    DEBUG_LOG("VehicleInfo::SwitchSeat: passenger: %s try to switch to seat %u", passenger->GetGuidStr().c_str(), seat);

    // Switching seats is not possible
    if (m_vehicleEntry->m_flags & VEHICLE_FLAG_DISABLE_SWITCH)
        return;

    PassengerMap::const_iterator itr = m_passengers.find(passenger);
    MANGOS_ASSERT(itr != m_passengers.end());

    // We are already boarded to this seat
    if (itr->second->GetTransportSeat() == seat)
        return;

    // Check if it's a valid seat
    if (!IsSeatAvailableFor(passenger, seat))
        return;

    VehicleSeatEntry const* seatEntry = GetSeatEntry(itr->second->GetTransportSeat());
    MANGOS_ASSERT(seatEntry);

    // Switching seats is only allowed if this flag is set
    if (~seatEntry->m_flags & SEAT_FLAG_CAN_SWITCH)
        return;

    // Remove passenger modifications of the old seat
    RemoveSeatMods(passenger, seatEntry->m_flags);

    // Set to new seat
    itr->second->SetTransportSeat(seat);

    Movement::MoveSplineInit init(*passenger);
    init.MoveTo(0.0f, 0.0f, 0.0f);                          // ToDo: Set correct local coords
    //if (oldorientation != neworientation) (?)
    //init.SetFacing(0.0f);                                 // local orientation ? ToDo: Set proper orientation!
    // It seems that Seat switching is sent without SplineFlag BoardVehicle
    init.Launch();

    // Get seatEntry of new seat
    seatEntry = GetSeatEntry(seat);
    MANGOS_ASSERT(seatEntry);

    // Apply passenger modifications of the new seat
    ApplySeatMods(passenger, seatEntry->m_flags);
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

    DEBUG_LOG("VehicleInfo::Unboard: passenger: %s", passenger->GetGuidStr().c_str());

    PassengerMap::const_iterator itr = m_passengers.find(passenger);
    MANGOS_ASSERT(itr != m_passengers.end());

    VehicleSeatEntry const* seatEntry = GetSeatEntry(itr->second->GetTransportSeat());
    MANGOS_ASSERT(seatEntry);

    UnBoardPassenger(passenger);                            // Use TransportBase to remove the passenger from storage list

    if (!changeVehicle)                                     // Send expected unboarding packages
    {
        // Update movementInfo
        passenger->m_movementInfo.ClearTransportData();

        if (passenger->GetTypeId() == TYPEID_PLAYER)
        {
            Player* pPlayer = (Player*)passenger;
            pPlayer->ResummonPetTemporaryUnSummonedIfAny();

            // SMSG_PET_DISMISS_SOUND (?)
        }

        if (passenger->IsRooted())
            passenger->SetRoot(false);

        Movement::MoveSplineInit init(*passenger);
        // ToDo: Set proper unboard coordinates
        init.MoveTo(m_owner->GetPositionX(), m_owner->GetPositionY(), m_owner->GetPositionZ());
        init.SetExitVehicle();
        init.Launch();

        // Despawn if passenger was accessory
        if (passenger->GetTypeId() == TYPEID_UNIT && m_accessoryGuids.find(passenger->GetObjectGuid()) != m_accessoryGuids.end())
        {
            // TODO Same TODO as in VehicleInfo::RemoveAccessoriesFromMap
            ((Creature*)passenger)->ForcedDespawn(5000);
            m_accessoryGuids.erase(passenger->GetObjectGuid());
        }
    }

    // Remove passenger modifications
    RemoveSeatMods(passenger, seatEntry->m_flags);

    // Some creature vehicles get despawned after passenger unboarding
    if (m_owner->GetTypeId() == TYPEID_UNIT)
    {
        // TODO: Guesswork, but seems to be fairly near correct
        // Only if the passenger was on control seat? Also depending on some flags
        if ((seatEntry->m_flags & SEAT_FLAG_CAN_CONTROL) &&
            !(m_vehicleEntry->m_flags & (VEHICLE_FLAG_UNK4 | VEHICLE_FLAG_UNK20)))
        {
            if (((Creature*)m_owner)->IsTemporarySummon())
                ((Creature*)m_owner)->ForcedDespawn(1000);
        }
    }
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

    // Passenger is already on this vehicle (in this case switching seats is required)
    if (passenger->IsBoarded() && passenger->GetTransportInfo()->GetTransport() == m_owner)
        return false;

    // Prevent circular boarding: passenger (could only be vehicle) must not have m_owner on board
    if (passenger->IsVehicle() && passenger->GetVehicleInfo()->HasOnBoard(m_owner))
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
void VehicleInfo::CalculateBoardingPositionOf(float gx, float gy, float gz, float go, float& lx, float& ly, float& lz, float& lo) const
{
    NormalizeRotatedPosition(gx - m_owner->GetPositionX(), gy - m_owner->GetPositionY(), lx, ly);

    lz = gz - m_owner->GetPositionZ();
    lo = NormalizeOrientation(go - m_owner->GetOrientation());
}

void VehicleInfo::RemoveAccessoriesFromMap()
{
    // Remove all accessories
    for (GuidSet::const_iterator itr = m_accessoryGuids.begin(); itr != m_accessoryGuids.end(); ++itr)
    {
        if (Creature* pAccessory = m_owner->GetMap()->GetCreature(*itr))
        {
            // TODO - unclear how long to despawn, also maybe some flag etc depending
            pAccessory->ForcedDespawn(5000);
        }
    }
    m_accessoryGuids.clear();
    m_isInitialized = false;
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
    Unit* pVehicle = (Unit*)m_owner;                        // Vehicles are alawys Unit

    if (passenger->GetTypeId() == TYPEID_PLAYER)
    {
        Player* pPlayer = (Player*)passenger;

        if (seatFlags & SEAT_FLAG_CAN_CONTROL)
        {
            pPlayer->GetCamera().SetView(pVehicle);

            pPlayer->SetCharm(pVehicle);
            pVehicle->SetCharmerGuid(pPlayer->GetObjectGuid());

            pVehicle->addUnitState(UNIT_STAT_CONTROLLED);
            pVehicle->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

            pPlayer->SetClientControl(pVehicle, 1);
            pPlayer->SetMover(pVehicle);

            // Unconfirmed - default speed handling
            if (pVehicle->GetTypeId() == TYPEID_UNIT)
            {
                if (!pPlayer->IsWalking() && pVehicle->IsWalking())
                {
                    ((Creature*)pVehicle)->SetWalk(false);
                }
                else if (pPlayer->IsWalking() && !pVehicle->IsWalking())
                {
                    ((Creature*)pVehicle)->SetWalk(true);
                }
            }
        }

        if (seatFlags & (SEAT_FLAG_USABLE | SEAT_FLAG_CAN_CAST))
        {
            CharmInfo* charmInfo = pVehicle->InitCharmInfo(pVehicle);
            charmInfo->InitVehicleCreateSpells();

            pPlayer->PossessSpellInitialize();
        }
    }
    else if (passenger->GetTypeId() == TYPEID_UNIT)
    {
        if (seatFlags & SEAT_FLAG_CAN_CONTROL)
        {
            passenger->SetCharm(pVehicle);
            pVehicle->SetCharmerGuid(passenger->GetObjectGuid());
        }
    }
}

/// Remove control and such modifiers to a passenger if they were added
void VehicleInfo::RemoveSeatMods(Unit* passenger, uint32 seatFlags)
{
    Unit* pVehicle = (Unit*)m_owner;

    if (passenger->GetTypeId() == TYPEID_PLAYER)
    {
        Player* pPlayer = (Player*)passenger;

        if (seatFlags & SEAT_FLAG_CAN_CONTROL)
        {
            pPlayer->SetCharm(NULL);
            pVehicle->SetCharmerGuid(ObjectGuid());

            pPlayer->SetClientControl(pVehicle, 0);
            pPlayer->SetMover(NULL);

            pVehicle->clearUnitState(UNIT_STAT_CONTROLLED);
            pVehicle->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

            // must be called after movement control unapplying
            pPlayer->GetCamera().ResetView();
        }

        if (seatFlags & (SEAT_FLAG_USABLE | SEAT_FLAG_CAN_CAST))
            pPlayer->RemovePetActionBar();
    }
    else if (passenger->GetTypeId() == TYPEID_UNIT)
    {
        if (seatFlags & SEAT_FLAG_CAN_CONTROL)
        {
            passenger->SetCharm(NULL);
            pVehicle->SetCharmerGuid(ObjectGuid());
        }
    }
}

/*! @} */
