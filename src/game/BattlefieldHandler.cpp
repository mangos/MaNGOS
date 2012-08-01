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
#include "SharedDefines.h"
#include "DBCStores.h"

void WorldSession::HandleBattlefieldExitRequest(WorldPacket& recv_data)
{
    uint32 battleId;

    recv_data >> battleId;
    sBattlefieldMgr.GetQueueForBattlefield(battleId)->RemovePlayerFromQueue(_player);
    sBattlefieldMgr.RemovePlayerFromBattlefield(_player,sBattlefieldMgr.FindBattlefield(battleId),BF_LEAVE_REASON_EXITED,false);
}

void WorldSession::HandleBattlefieldEntryInviteResponseOpcode( WorldPacket &recv_data )
{
    uint32 battleId;
    uint8 accepted;

    recv_data >> battleId >> accepted;

    Battlefield * battlefield = sBattlefieldMgr.FindBattlefield(battleId);

    if(accepted == 1)
    {
        WorldPacket send_data(SMSG_BATTLEFIELD_MANAGER_ENTERING,7);

        send_data << uint32(battleId);
        send_data << uint8(0);
        send_data << uint8(0);
        send_data << uint8(_player->isAFK() ? 1 : 0);

        if(battlefield->GetControllerTeam() == _player->GetTeam())
        {
            const WorldSafeLocsEntry * location = sWorldSafeLocsStore.LookupEntry(1474);
            _player->TeleportTo(location->map_id,location->x,location->y,location->z,_player->GetOrientation());
        }
        else if(_player->GetTeam() == ALLIANCE)
        {
            const WorldSafeLocsEntry * location = sWorldSafeLocsStore.LookupEntry(1332);
            _player->TeleportTo(location->map_id,location->x,location->y,location->z,_player->GetOrientation());
        }
        else if(_player->GetTeam() == HORDE)
        {
            const WorldSafeLocsEntry * location = sWorldSafeLocsStore.LookupEntry(1331);
            _player->TeleportTo(location->map_id,location->x,location->y,location->z,_player->GetOrientation());
        }

        SendPacket(&send_data);

        battlefield->AddPlayerToGroup(_player);
    }
    else
    {
        sBattlefieldMgr.RemovePlayerFromBattlefield(_player,battlefield,BF_LEAVE_REASON_EXITED,false);
    }
}

void WorldSession::HandleBattlefieldQueueInviteResponseOpcode( WorldPacket &recv_data )
{
    uint32 battleId;
    uint8 accepted;

    recv_data >> battleId >> accepted;

    if(accepted)
    {
        sBattlefieldMgr.GetQueueForBattlefield(battleId)->AddPlayerToQueue(_player);
    }
    else
    {
        sBattlefieldMgr.RemovePlayerFromBattlefield(_player,sBattlefieldMgr.FindBattlefield(battleId),BF_LEAVE_REASON_EXITED,false);
    }
}
