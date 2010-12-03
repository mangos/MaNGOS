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

#include "Common.h"
#include "ObjectGuid.h"
#include "Creature.h"
#include "Unit.h"
#include "SharedDefines.h"

struct VehicleSeat
{
    VehicleSeat(VehicleSeatEntry const *pSeatInfo = NULL) : seatInfo(pSeatInfo), passenger(NULL) {}

    VehicleSeatEntry const *seatInfo;
    Unit* passenger;
};

typedef std::map<int8, VehicleSeat> SeatMap;

struct VehicleAccessory
{
    explicit VehicleAccessory(uint32 _uiAccessory, int8 _uiSeat, bool _bMinion) : uiAccessory(_uiAccessory), uiSeat(_uiSeat), bMinion(_bMinion) {}
    uint32 uiAccessory;
    int8 uiSeat;
    uint32 bMinion;
};

typedef std::vector<VehicleAccessory> VehicleAccessoryList;
typedef std::map<uint32, VehicleAccessoryList> VehicleAccessoryMap;

class VehicleKit
{
public:
    explicit VehicleKit(Unit* base, VehicleEntry const* vehicleInfo);
    ~VehicleKit();

    void Reset();
    void InstallAllAccessories(uint32 entry);

    bool HasEmptySeat(int8 seatId) const;
    Unit *GetPassenger(int8 seatId) const;
    int8 GetNextEmptySeat(int8 seatId, bool next) const;
    bool AddPassenger(Unit *passenger, int8 seatId = -1);
    void RemovePassenger(Unit *passenger);
    void RelocatePassengers(float x, float y, float z, float ang);
    void RemoveAllPassengers();
    VehicleSeatEntry const* GetSeatInfo(Unit* passenger);

    uint32 GetVehicleId() const { return m_vehicleInfo->m_ID; }
    VehicleEntry const* GetVehicleInfo() const { return m_vehicleInfo; }
    Unit* GetBase() { return m_pBase; }
private:
    void UpdateFreeSeatCount();
    void InstallAccessory(uint32 entry, int8 seatId, bool minion = true);

    SeatMap m_Seats;
    uint32 m_uiNumFreeSeats;
    VehicleEntry const *m_vehicleInfo;
    Unit* m_pBase;
};

#endif
