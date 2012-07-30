/*
 * Copyright (C) 2010-2012 Strawberry-Pr0jcts <http://strawberry-pr0jcts.com/>
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

#include "WorldSession.h"
#include "BattlefieldMgr.h"

void WorldSession::HandleBattlefieldExitRequest(WorldPacket& recv_data)
{
    uint32 battleId;

    recv_data >> battleId;
    sBattlefieldMgr.GetQueueForBattlefield(battleId)->RemovePlayerFromQueue(_player);

    WorldPacket send_data(SMSG_BATTLEFIELD_MANAGER_EJECTED,7);

    send_data << uint32(battleId);
    send_data << uint8(BF_LEAVE_REASON_EXITED);
    send_data << uint8(sBattlefieldMgr.FindBattlefield(battleId)->IsBattleInProgress() ? 2 : 0);
    send_data << uint8(false); //reloacted

    SendPacket(&send_data);
}

void WorldSession::HandleBattlefieldEntryInviteResponseOpcode( WorldPacket &recv_data )
{
    uint32 battleId;
    uint8 accepted;

    recv_data >> battleId >> accepted;

    if(accepted == 1)
    {
        WorldPacket send_data(SMSG_BATTLEFIELD_MANAGER_ENTERING,7);

        send_data << uint32(battleId);
        send_data << uint8(0);
        send_data << uint8(0);
        send_data << uint8(_player->isAFK() ? 1 : 0);

        //Teleport player here

        SendPacket(&send_data);
    }
    else
    {
        WorldPacket send_data(SMSG_BATTLEFIELD_MANAGER_EJECTED,7);

        send_data << uint32(battleId);
        send_data << uint8(BF_LEAVE_REASON_EXITED);
        send_data << uint8(sBattlefieldMgr.FindBattlefield(battleId)->IsBattleInProgress() ? 2 : 0);
        send_data << uint8(false); //reloacted

        SendPacket(&send_data);
    }
}