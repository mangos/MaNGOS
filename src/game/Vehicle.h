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

#ifndef MANGOSSERVER_VEHICLE_H
#define MANGOSSERVER_VEHICLE_H

#include "ObjectGuid.h"
#include "Creature.h"
#include "Unit.h"

struct VehicleSeat
{
    VehicleSeat(VehicleSeatEntry const *pSeatInfo = NULL) : seatInfo(pSeatInfo), passenger(NULL) {}

    VehicleSeatEntry const *seatInfo;
    Unit* passenger;
};

typedef std::map<int8, VehicleSeat> SeatMap;

class VehicleKit
{
public:
    explicit VehicleKit(Unit* base, VehicleEntry const* vehicleInfo);
    ~VehicleKit();

    void Reset();

    bool HasEmptySeat(int8 seatId) const;
    Unit *GetPassenger(int8 seatId) const;
    int8 GetNextEmptySeat(int8 seatId, bool next) const;
    bool AddPassenger(Unit *passenger, int8 seatId = -1);
    void RemovePassenger(Unit *passenger);
    void RelocatePassengers(float x, float y, float z, float ang);
    void RemoveAllPassengers();

    uint32 GetVehicleId() const { return m_vehicleInfo->m_ID; }
    VehicleEntry const* GetVehicleInfo() const { return m_vehicleInfo; }
    Unit* GetBase() { return m_pBase; }
private:
    SeatMap m_Seats;
    uint32 m_uiNumFreeSeats;
    VehicleEntry const *m_vehicleInfo;
    Unit* m_pBase;
};

class Vehicle : public Creature
{
    public:
        explicit Vehicle();
        virtual ~Vehicle();

        void AddToWorld();
        void RemoveFromWorld();

        bool Create (uint32 guidlow, Map *map, uint32 Entry, uint32 vehicleId, uint32 team);

        void setDeathState(DeathState s);                   // overwrite virtual Creature::setDeathState and Unit::setDeathState
        void Update(uint32 diff);                           // overwrite virtual Creature::Update and Unit::Update

        uint32 GetVehicleId() { return m_vehicleId; }
        void SetVehicleId(uint32 vehicleid) { m_vehicleId = vehicleid; }

        void Dismiss();

    protected:
        uint32 m_vehicleId;

    private:
        void SaveToDB(uint32, uint8)                        // overwrited of Creature::SaveToDB     - don't must be called
        {
            ASSERT(false);
        }
        void DeleteFromDB()                                 // overwrited of Creature::DeleteFromDB - don't must be called
        {
            ASSERT(false);
        }
};
#endif
