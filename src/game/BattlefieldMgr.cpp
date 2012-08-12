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

#include "pchdef.h"
#include "BattlefieldMgr.h"
#include "BattlefieldWG.h"
#include "Policies/SingletonImp.h"
#include "World.h"
#include "WorldPacket.h"

INSTANTIATE_SINGLETON_1(BattlefieldMgr);

BattlefieldMgr::BattlefieldMgr()
{
    
}

BattlefieldMgr::~BattlefieldMgr()
{
    for(BattlefieldMap::iterator itr = m_battlefieldList.begin(); itr != m_battlefieldList.end(); ++itr)
        delete itr->second;
}

void BattlefieldMgr::Initialize()
{
    sLog.outDebug("Creating Battlefields");

    Battlefield * WG = new BattlefieldWG();
    m_battlefieldList[WG->GetBattleId()] = WG;
    m_queueMap[WG->GetBattleId()] = new BattlefieldQueue(BATTLEFIELD_WG);
}

void BattlefieldMgr::Update(uint32 uiDiff)
{
    for(BattlefieldMap::iterator itr = m_battlefieldList.begin(); itr != m_battlefieldList.end(); ++itr)
    {
        (*itr).second->Update(uiDiff);
    }
}

void BattlefieldMgr::SendInvitePlayerToQueue(Player * player)
{
    WorldPacket send_data(SMSG_BATTLEFIELD_MANAGER_QUEUE_REQUEST_RESPONSE,7);

    Battlefield * battlefield = FindBattlefield(BATTLEFIELD_WG);

    send_data << uint32(battlefield->GetBattleId());
    send_data << uint32(battlefield->GetZoneId());
    if(player->InBattleGroundQueue() || player->InBattleGround() || player->InArena() || player->getLevel() < 75)
    {
        send_data << uint8(false); // accepted
    }
    else
    {
        send_data << uint8(true); // accepted
    }
    send_data << uint8(!GetQueueForBattlefield(BATTLEFIELD_WG)->HasEnoughSpace()); // can join
    send_data << uint8(!battlefield->IsBattleInProgress()); // in warmup

    GetQueueForBattlefield(BATTLEFIELD_WG)->AddPlayerToQueue(player);
    
    player->GetSession()->SendPacket(&send_data);
}

void BattlefieldMgr::ChangeState(Battlefield * battlefield)
{
    
}

Battlefield * BattlefieldMgr::FindBattlefield(uint8 battleId)
{
    BattlefieldMap::const_iterator itr = m_battlefieldList.find(battleId);
    if(itr != m_battlefieldList.end())
        return itr->second;
    else
        return NULL;
}

void BattlefieldMgr::UpdateWorldState(uint32 stateId, uint32 value)
{
    WorldPacket send_data(SMSG_UPDATE_WORLD_STATE, 4+4);
    send_data << uint32(stateId);
    send_data << uint32(value);
    sWorld.SendGlobalMessage(&send_data);
}


void BattlefieldMgr::SendInvitePlayersToWar(uint8 battleId)
{
    BattlefieldQueue * queue = GetQueueForBattlefield(battleId);

    WorldPacket send_data(SMSG_BATTLEFIELD_MANAGER_ENTRY_INVITE,12);

    send_data << uint32(battleId);
    send_data << uint32(FindBattlefield(battleId)->GetZoneId());
    send_data << uint32(time(NULL) + 20);

    for(PlayerQueue::iterator itr = queue->m_inQueue.begin(); itr != queue->m_inQueue.end(); ++itr)
    {
        ((Player * )*itr)->GetSession()->SendPacket(&send_data);
    }
}

void BattlefieldMgr::PlayerEnterZone(Player * player, uint32 zoneId)
{
    Battlefield * battlefield;

    if(zoneId == 4197)
    {
        battlefield = FindBattlefield(BATTLEFIELD_WG);
    }

    if(!player->IsFlying())
    {
        if(battlefield->IsBattleInProgress())
        {
            if(GetQueueForBattlefield(battlefield->GetBattleId())->HasEnoughSpace())
            {
                WorldPacket send_data(SMSG_BATTLEFIELD_MANAGER_ENTRY_INVITE,12);

                send_data << uint32(battlefield->GetBattleId());
                send_data << uint32(battlefield->GetZoneId());
                send_data << uint32(time(NULL) + 20);

                player->GetSession()->SendPacket(&send_data);
            }            
            else
            {
                //teleport player here
                WorldPacket send_data(SMSG_BATTLEFIELD_MANAGER_EJECT_PENDING,5);

                send_data << uint32(battlefield->GetBattleId());
                send_data << uint8(true);

                player->GetSession()->SendPacket(&send_data);
            }
        }
    }

    if(battlefield->m_map)
    {
        battlefield->m_map = player->GetMap();
    }

}

void BattlefieldMgr::PlayerLeftZone(Player * player, uint32 zoneId)
{

}

void BattlefieldMgr::RemovePlayerFromBattlefield(Player * player,Battlefield * battlefield,BFLeaveReason reason,bool relocated)
{
    WorldPacket send_data(SMSG_BATTLEFIELD_MANAGER_EJECTED,7);

    send_data << uint32(battlefield->GetBattleId());
    send_data << uint8(reason);
    send_data << uint8(battlefield->IsBattleInProgress() ? 2 : 0);
    send_data << uint8(false); //reloacted

    player->GetSession()->SendPacket(&send_data);
}
