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
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "Player.h"
#include "Vehicle.h"
#include "ObjectMgr.h"

void WorldSession::HandleDismissControlledVehicle(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_DISMISS_CONTROLLED_VEHICLE");
    recv_data.hexlike();

    ObjectGuid guid;
    MovementInfo mi;

    recv_data >> guid.ReadAsPacked();
    recv_data >> mi;

    if(!GetPlayer()->GetVehicle())
        return;

    bool dismiss = true;

    if (GetPlayer()->GetVehicle()->GetVehicleInfo()->m_flags & (VEHICLE_FLAG_NOT_DISMISS | VEHICLE_FLAG_ACCESSORY))
        dismiss = false;

    GetPlayer()->m_movementInfo = mi;
    GetPlayer()->ExitVehicle();

    if (dismiss)
        if (Creature* vehicle = GetPlayer()->GetMap()->GetAnyTypeCreature(guid))
            vehicle->ForcedDespawn();

}

void WorldSession::HandleRequestVehicleExit(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_REQUEST_VEHICLE_EXIT");

    GetPlayer()->ExitVehicle();
}

void WorldSession::HandleRequestVehiclePrevSeat(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_REQUEST_VEHICLE_PREV_SEAT");

    GetPlayer()->ChangeSeat(-1, false);
}

void WorldSession::HandleRequestVehicleNextSeat(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_REQUEST_VEHICLE_NEXT_SEAT");

    GetPlayer()->ChangeSeat(-1, true);
}

void WorldSession::HandleRequestVehicleSwitchSeat(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_REQUEST_VEHICLE_SWITCH_SEAT");
    recv_data.hexlike();

    ObjectGuid guid;
    recv_data >> guid.ReadAsPacked();

    int8 seatId;
    recv_data >> seatId;

    VehicleKit* pVehicle = GetPlayer()->GetVehicle();

    if (!pVehicle)
        return;

    if (GetPlayer()->GetVehicle()->GetVehicleInfo()->m_flags & VEHICLE_FLAG_DISABLE_SWITCH)
        return;

    if (pVehicle->GetBase()->GetObjectGuid() == guid)
        GetPlayer()->ChangeSeat(seatId);
    else if (Unit *Vehicle2 = GetPlayer()->GetMap()->GetUnit(guid))
    {
        if (VehicleKit *pVehicle2 = Vehicle2->GetVehicleKit())
            if (pVehicle2->HasEmptySeat(seatId))
            {
                    GetPlayer()->ExitVehicle();
                    GetPlayer()->EnterVehicle(pVehicle2, seatId);
            }
    }
}

void WorldSession::HandleEnterPlayerVehicle(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_PLAYER_VEHICLE_ENTER");
    recv_data.hexlike();

    ObjectGuid guid;
    recv_data >> guid;

    Player* player = sObjectMgr.GetPlayer(guid);

    if (!player)
        return;

    if (!GetPlayer()->IsInSameRaidWith(player))
        return;

    if (!GetPlayer()->IsWithinDistInMap(player, INTERACTION_DISTANCE))
        return;

    if (player->GetTransport())
        return;

    if (VehicleKit* pVehicle = player->GetVehicleKit())
        GetPlayer()->EnterVehicle(pVehicle);
}

void WorldSession::HandleEjectPasenger(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_EJECT_PASSENGER");
    recv_data.hexlike();

    ObjectGuid guid;
    recv_data >> guid;

    Unit* passenger = ObjectAccessor::GetUnit(*GetPlayer(), guid);

    if (!passenger)
        return;

    if (!passenger->GetVehicle() || passenger->GetVehicle() != GetPlayer()->GetVehicleKit())
        return;

    passenger->ExitVehicle();

    // eject and remove creatures of player mounts
    if (passenger->GetTypeId() == TYPEID_UNIT)
        passenger->AddObjectToRemoveList();
}

void WorldSession::HandleChangeSeatsOnControlledVehicle(WorldPacket &recv_data)
{
    sLog.outDebug("WORLD: Recvd CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE");
    recv_data.hexlike();

    ObjectGuid guid, guid2;
    recv_data >> guid.ReadAsPacked();

    MovementInfo mi;
    recv_data >> mi;
    GetPlayer()->m_movementInfo = mi;

    recv_data >> guid2.ReadAsPacked(); //guid of vehicle or of vehicle in target seat

    int8 seatId;
    recv_data >> seatId;

    VehicleKit* pVehicle = GetPlayer()->GetVehicle();

    if (!pVehicle)
        return;

    if (GetPlayer()->GetVehicle()->GetVehicleInfo()->m_flags & VEHICLE_FLAG_DISABLE_SWITCH)
        return;

    if(guid.GetRawValue() == guid2.GetRawValue())
        GetPlayer()->ChangeSeat(seatId, false);

    else if (guid2.IsVehicle())
    {
        if (Creature* vehicle = GetPlayer()->GetMap()->GetAnyTypeCreature(guid2))
        {
            if (VehicleKit* pVehicle2 = vehicle->GetVehicleKit())
                if(pVehicle2->HasEmptySeat(seatId))
                {
                    GetPlayer()->ExitVehicle();
                    GetPlayer()->EnterVehicle(pVehicle2, seatId);
                }
        }
    }
}
