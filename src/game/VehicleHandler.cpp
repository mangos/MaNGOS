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

#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "ObjectGuid.h"
#include "Log.h"
#include "Player.h"
#include "Vehicle.h"
#include "ObjectMgr.h"

void WorldSession::HandleDismissControlledVehicle(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Recvd CMSG_DISMISS_CONTROLLED_VEHICLE");
    recv_data.hexlike();

    ObjectGuid guid;
    MovementInfo mi;

    recv_data >> guid.ReadAsPacked();
    recv_data >> mi;

    ObjectGuid vehicleGUID = _player->GetCharmGuid();
    if (!vehicleGUID)                                       // something wrong here...
        return;

    _player->m_movementInfo = mi;
}
