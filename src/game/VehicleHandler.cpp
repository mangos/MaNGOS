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

    GetPlayer()->m_movementInfo = mi;
    GetPlayer()->ExitVehicle();
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

    if (pVehicle->GetBase()->GetObjectGuid() == guid)
        GetPlayer()->ChangeSeat(seatId);
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
}
