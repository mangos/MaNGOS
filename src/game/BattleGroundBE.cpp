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

#include "Object.h"
#include "Player.h"
#include "BattleGround.h"
#include "BattleGroundBE.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "Language.h"

BattleGroundBE::BattleGroundBE()
{
    m_StartDelayTimes[BG_STARTING_EVENT_FIRST]  = BG_START_DELAY_1M;
    m_StartDelayTimes[BG_STARTING_EVENT_SECOND] = BG_START_DELAY_30S;
    m_StartDelayTimes[BG_STARTING_EVENT_THIRD]  = BG_START_DELAY_15S;
    m_StartDelayTimes[BG_STARTING_EVENT_FOURTH] = BG_START_DELAY_NONE;
    //we must set messageIds
    m_StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_ARENA_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_ARENA_THIRTY_SECONDS;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_ARENA_FIFTEEN_SECONDS;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_ARENA_HAS_BEGUN;
}

BattleGroundBE::~BattleGroundBE()
{
}

void BattleGroundBE::Update(uint32 diff)
{
    BattleGround::Update(diff);

    /*if (GetStatus() == STATUS_IN_PROGRESS)
    {
        // update something
    }*/
}

void BattleGroundBE::StartingEventCloseDoors()
{
}

void BattleGroundBE::StartingEventOpenDoors()
{
    OpenDoorEvent(BG_EVENT_DOOR);
}

void BattleGroundBE::AddPlayer(Player *plr)
{
    BattleGround::AddPlayer(plr);
    //create score and add it to map, default values are set in constructor
    BattleGroundBEScore* sc = new BattleGroundBEScore;

    m_PlayerScores[plr->GetObjectGuid()] = sc;

    UpdateWorldState(0x9f1, GetAlivePlayersCountByTeam(ALLIANCE));
    UpdateWorldState(0x9f0, GetAlivePlayersCountByTeam(HORDE));
}

void BattleGroundBE::RemovePlayer(Player* /*plr*/, ObjectGuid /*guid*/)
{
    if (GetStatus() == STATUS_WAIT_LEAVE)
        return;

    UpdateWorldState(0x9f1, GetAlivePlayersCountByTeam(ALLIANCE));
    UpdateWorldState(0x9f0, GetAlivePlayersCountByTeam(HORDE));

    CheckArenaWinConditions();
}

void BattleGroundBE::HandleKillPlayer(Player *player, Player *killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (!killer)
    {
        sLog.outError("Killer player not found");
        return;
    }

    BattleGround::HandleKillPlayer(player,killer);

    UpdateWorldState(0x9f1, GetAlivePlayersCountByTeam(ALLIANCE));
    UpdateWorldState(0x9f0, GetAlivePlayersCountByTeam(HORDE));

    CheckArenaWinConditions();
}

bool BattleGroundBE::HandlePlayerUnderMap(Player *player)
{
    player->TeleportTo(GetMapId(),6238.930176f,262.963470f,0.889519f,player->GetOrientation(),false);
    return true;
}

void BattleGroundBE::HandleAreaTrigger(Player *Source, uint32 Trigger)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    //uint32 SpellId = 0;
    //uint64 buff_guid = 0;
    switch(Trigger)
    {
        case 4538:                                          // buff trigger?
            //buff_guid = m_BgObjects[BG_BE_OBJECT_BUFF_1];
            break;
        case 4539:                                          // buff trigger?
            //buff_guid = m_BgObjects[BG_BE_OBJECT_BUFF_2];
            break;
        default:
            sLog.outError("WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            Source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }

    //if (buff_guid)
    //    HandleTriggerBuff(buff_guid,Source);
}

void BattleGroundBE::FillInitialWorldStates(WorldPacket &data, uint32& count)
{
    FillInitialWorldState(data, count, 0x9f1, GetAlivePlayersCountByTeam(ALLIANCE));
    FillInitialWorldState(data, count, 0x9f0, GetAlivePlayersCountByTeam(HORDE));
    FillInitialWorldState(data, count, 0x9f3, 1);
}

void BattleGroundBE::Reset()
{
    //call parent's class reset
    BattleGround::Reset();
}

bool BattleGroundBE::SetupBattleGround()
{
    return true;
}

void BattleGroundBE::UpdatePlayerScore(Player* Source, uint32 type, uint32 value)
{

    BattleGroundScoreMap::iterator itr = m_PlayerScores.find(Source->GetObjectGuid());
    if(itr == m_PlayerScores.end())                         // player not found...
        return;

    //there is nothing special in this score
    BattleGround::UpdatePlayerScore(Source, type, value);

}

/*
21:45:46 id:231310 [S2C] SMSG_INIT_WORLD_STATES (706 = 0x02C2) len: 86
0000: 32 02 00 00 76 0e 00 00 00 00 00 00 09 00 f3 09  |  2...v...........
0010: 00 00 01 00 00 00 f1 09 00 00 01 00 00 00 f0 09  |  ................
0020: 00 00 02 00 00 00 d4 08 00 00 00 00 00 00 d8 08  |  ................
0030: 00 00 00 00 00 00 d7 08 00 00 00 00 00 00 d6 08  |  ................
0040: 00 00 00 00 00 00 d5 08 00 00 00 00 00 00 d3 08  |  ................
0050: 00 00 00 00 00 00                                |  ......

spell 32724 - Gold Team
spell 32725 - Green Team
35774 Gold Team
35775 Green Team
*/
