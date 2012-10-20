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
 * @addtogroup TransportSystem to provide abstract support for transported entities
 * The Transport System in MaNGOS consists of these files:
 * - TransportSystem.h to provide the basic classes TransportBase and TransportInfo
 * - TransportSystem.cpp which implements these classes
 * - Vehicle.h as a vehicle is a transporter it will inherit itr transporter-information from TransportBase
 * - Transports.h to implement the MOTransporter (subclas of gameobject) - Remains TODO
 * as well of
 * - impacts to various files
 *
 * @{
 *
 * @file Vehicle.h
 * This file contains the headers for the functionality required by Vehicles
 *
 */

#ifndef MANGOSSERVER_VEHICLE_H
#define MANGOSSERVER_VEHICLE_H

#include "Common.h"
#include "TransportSystem.h"

class Unit;

struct VehicleEntry;
struct VehicleSeatEntry;

struct VehicleAccessory
{
    uint32 vehicleEntry;
    uint32 seatId;
    uint32 passengerEntry;
};

typedef std::map<uint8 /*seatPosition*/, VehicleSeatEntry const*> VehicleSeatMap;

/**
 * A class to provide support for each vehicle. This includes
 * - Boarding and unboarding of passengers, including support to switch vehicles
 * - Basic checks if a passenger can board
 */
class VehicleInfo : public TransportBase
{
    public:
        explicit VehicleInfo(Unit* owner, VehicleEntry const* vehicleEntry, uint32 overwriteNpcEntry);
        void Initialize();                                  ///< Initializes the accessories
        bool IsInitialized() const { return m_isInitialized; }

        ~VehicleInfo();

        VehicleEntry const* GetVehicleEntry() const { return m_vehicleEntry; }

        void Board(Unit* passenger, uint8 seat);            // Board a passenger to a vehicle
        void SwitchSeat(Unit* passenger, uint8 seat);       // Used to switch seats of a passenger
        void UnBoard(Unit* passenger, bool changeVehicle);  // Used to Unboard a passenger from a vehicle

        bool CanBoard(Unit* passenger) const;               // Used to check if a Unit can board a vehicle
        Unit* GetPassenger(uint8 seat) const;

        void RemoveAccessoriesFromMap();                    ///< Unsummones accessory in case of far-teleport or death

    private:
        // Internal use to calculate the boarding position
        void CalculateBoardingPositionOf(float gx, float gy, float gz, float go, float& lx, float& ly, float& lz, float& lo) const;

        // Seat information
        VehicleSeatEntry const* GetSeatEntry(uint8 seat) const;
        bool GetUsableSeatFor(Unit* passenger, uint8& seat) const;
        bool IsSeatAvailableFor(Unit* passenger, uint8 seat) const;

        uint8 GetTakenSeatsMask() const;
        uint8 GetEmptySeatsMask() const { return ~GetTakenSeatsMask(); }
        uint8 GetEmptySeats() const { return m_vehicleSeats.size() - m_passengers.size(); }

        bool IsUsableSeatForPlayer(uint32 seatFlags) const;
        bool IsUsableSeatForCreature(uint32 seatFlags) const { return true; } // special flag?, !IsUsableSeatForPlayer(seatFlags)?

        // Apply/ Remove Controlling of the vehicle
        void ApplySeatMods(Unit* passenger, uint32 seatFlags);
        void RemoveSeatMods(Unit* passenger, uint32 seatFlags);

        VehicleEntry const* m_vehicleEntry;
        VehicleSeatMap m_vehicleSeats;                      ///< Stores the available seats of the vehicle (filled in constructor)
        uint8 m_creatureSeats;                              ///< Mask that stores which seats are avaiable for creatures
        uint8 m_playerSeats;                                ///< Mask that stores which seats are avaiable for players

        uint32 m_overwriteNpcEntry;                         // Internal use to store the entry with which the vehicle-accessories are fetched
        bool m_isInitialized;                               // Internal use to store if the accessory is initialized
        GuidSet m_accessoryGuids;                           ///< Stores the summoned accessories of this vehicle
};

#endif

/*! @} */
