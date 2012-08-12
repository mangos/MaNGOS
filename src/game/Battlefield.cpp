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

#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include "DBCStores.h"
#include "SharedDefines.h"

Battlefield::Battlefield(uint8 BattleId)
{
    m_battleId = BattleId;

    for(uint8 i = 0; i < MAX_TEAM; ++i)
    {
        m_raidGroup[i] = new Group();
    }

    m_battleInProgress = false;
    m_invitationSent = false;
}
Battlefield::~Battlefield()
{
    for(uint8 i = 0; i < MAX_TEAM; ++i)
    {
        delete m_raidGroup[i];
    }
}

void Battlefield::Update(uint32 uiDiff)
{
    if(m_battleInProgress)
    {
        if(m_battleDurationTimer <= uiDiff)
        {
            BattleEnd();
            m_battleInProgress = false;
        }
        else
            m_battleDurationTimer -= uiDiff;
    }
    else
    {
        if(m_nextBattleTimer <= uiDiff)
        {
            BattleStart();
            m_battleInProgress = true;
        }
        else 
            m_nextBattleTimer -= uiDiff;

        if(m_preBattleTimer <= uiDiff && !m_invitationSent)
        {
            const Map::PlayerList players = m_map->GetPlayers();

            WorldPacket send_data(SMSG_BATTLEFIELD_MANAGER_QUEUE_INVITE,5);

            send_data << uint32(m_battleId);
            send_data << uint8(!m_battleInProgress);

            for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                if(itr->getSource()->GetZoneId() == m_zoneId && !sBattlefieldMgr.GetQueueForBattlefield(m_battleId)->HasPlayerInQueue(itr->getSource()))
                {
                    itr->getSource()->GetSession()->SendPacket(&send_data);
                }
            }
        }
        else
            m_preBattleTimer -= uiDiff;
    }

    OnUpdate(uiDiff);
}

void Battlefield::BattleStart()
{
    BeforeBattleStarted();
}

void Battlefield::BattleEnd()
{
    AfterBattleEnded();
}

bool Battlefield::AddPlayerToGroup(Player * player)
{
    Group* grp;
    if(player->GetTeam() == ALLIANCE)
        grp = m_raidGroup[0];
    else
        grp = m_raidGroup[1];

    if(!grp->IsCreated())
    {
        grp->Create(player->GetObjectGuid(),player->GetName());
        grp->ConvertToRaid();
    }
    else if(!grp->IsFull())
    {
        grp->AddMember(player->GetObjectGuid(),player->GetName());
    }
    else
        return false;

    return true;
}

void Battlefield::PlayerJoin(Player* player)
{
    OnPlayerEnter(player);
}

void Battlefield::PlayerLeave(Player* player)
{

}
