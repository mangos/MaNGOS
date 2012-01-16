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
#include "BattleGroundNA.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "Language.h"

BattleGroundNA::BattleGroundNA()
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

BattleGroundNA::~BattleGroundNA()
{

}

void BattleGroundNA::Update(uint32 diff)
{
    BattleGround::Update(diff);

    /*if (GetStatus() == STATUS_IN_PROGRESS)
    {
        // update something
    }*/
}

void BattleGroundNA::StartingEventCloseDoors()
{
}

void BattleGroundNA::StartingEventOpenDoors()
{
    OpenDoorEvent(BG_EVENT_DOOR);
}

void BattleGroundNA::AddPlayer(Player *plr)
{
    BattleGround::AddPlayer(plr);
    //create score and add it to map, default values are set in constructor
    BattleGroundNAScore* sc = new BattleGroundNAScore;

    m_PlayerScores[plr->GetObjectGuid()] = sc;

    UpdateWorldState(0xa0f, GetAlivePlayersCountByTeam(ALLIANCE));
    UpdateWorldState(0xa10, GetAlivePlayersCountByTeam(HORDE));
}

void BattleGroundNA::RemovePlayer(Player* /*plr*/, ObjectGuid /*guid*/)
{
    if (GetStatus() == STATUS_WAIT_LEAVE)
        return;

    UpdateWorldState(0xa0f, GetAlivePlayersCountByTeam(ALLIANCE));
    UpdateWorldState(0xa10, GetAlivePlayersCountByTeam(HORDE));

    CheckArenaWinConditions();
}

void BattleGroundNA::HandleKillPlayer(Player *player, Player *killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (!killer)
    {
        sLog.outError("BattleGroundNA: Killer player not found");
        return;
    }

    BattleGround::HandleKillPlayer(player,killer);

    UpdateWorldState(0xa0f, GetAlivePlayersCountByTeam(ALLIANCE));
    UpdateWorldState(0xa10, GetAlivePlayersCountByTeam(HORDE));

    CheckArenaWinConditions();
}

bool BattleGroundNA::HandlePlayerUnderMap(Player *player)
{
    player->TeleportTo(GetMapId(),4055.504395f,2919.660645f,13.611241f,player->GetOrientation(),false);
    return true;
}

void BattleGroundNA::HandleAreaTrigger(Player *Source, uint32 Trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    //uint32 SpellId = 0;
    //uint64 buff_guid = 0;
    switch(Trigger)
    {
        case 4536:                                          // buff trigger?
        case 4537:                                          // buff trigger?
            break;
        default:
            sLog.outError("WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            Source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }

    //if (buff_guid)
    //    HandleTriggerBuff(buff_guid,Source);
}

void BattleGroundNA::FillInitialWorldStates(WorldPacket &data, uint32& count)
{
    FillInitialWorldState(data, count, 0xa0f, GetAlivePlayersCountByTeam(ALLIANCE));
    FillInitialWorldState(data, count, 0xa10, GetAlivePlayersCountByTeam(HORDE));
    FillInitialWorldState(data, count, 0xa11, 1);
}

void BattleGroundNA::Reset()
{
    //call parent's class reset
    BattleGround::Reset();
}

bool BattleGroundNA::SetupBattleGround()
{
    return true;
}

/*
20:12:14 id:036668 [S2C] SMSG_INIT_WORLD_STATES (706 = 0x02C2) len: 86
0000: 2f 02 00 00 72 0e 00 00 00 00 00 00 09 00 11 0a  |  /...r...........
0010: 00 00 01 00 00 00 0f 0a 00 00 00 00 00 00 10 0a  |  ................
0020: 00 00 00 00 00 00 d4 08 00 00 00 00 00 00 d8 08  |  ................
0030: 00 00 00 00 00 00 d7 08 00 00 00 00 00 00 d6 08  |  ................
0040: 00 00 00 00 00 00 d5 08 00 00 00 00 00 00 d3 08  |  ................
0050: 00 00 00 00 00 00                                |  ......
*/
