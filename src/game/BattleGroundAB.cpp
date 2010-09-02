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

#include "Object.h"
#include "Player.h"
#include "BattleGround.h"
#include "BattleGroundAB.h"
#include "Creature.h"
#include "GameObject.h"
#include "BattleGroundMgr.h"
#include "Language.h"
#include "Util.h"
#include "WorldPacket.h"
#include "MapManager.h"

BattleGroundAB::BattleGroundAB()
{
    m_BuffChange = true;
    m_BgObjects.resize(BG_AB_OBJECT_MAX);

    m_StartMessageIds[BG_STARTING_EVENT_FIRST]  = 0;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_AB_START_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_AB_START_HALF_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_AB_HAS_BEGUN;
}

BattleGroundAB::~BattleGroundAB()
{
}

void BattleGroundAB::Update(uint32 diff)
{
    BattleGround::Update(diff);

    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        int team_points[BG_TEAMS_COUNT] = { 0, 0 };

        for (int node = 0; node < BG_AB_NODES_MAX; ++node)
        {
            // 3 sec delay to spawn new banner instead previous despawned one
            if (m_BannerTimers[node].timer)
            {
                if (m_BannerTimers[node].timer > diff)
                    m_BannerTimers[node].timer -= diff;
                else
                {
                    m_BannerTimers[node].timer = 0;
                    _CreateBanner(node, m_BannerTimers[node].type, m_BannerTimers[node].teamIndex, false);
                }
            }

            // 1-minute to occupy a node from contested state
            if (m_NodeTimers[node])
            {
                if (m_NodeTimers[node] > diff)
                    m_NodeTimers[node] -= diff;
                else
                {
                    m_NodeTimers[node] = 0;
                    // Change from contested to occupied !
                    uint8 teamIndex = m_Nodes[node]-1;
                    m_prevNodes[node] = m_Nodes[node];
                    m_Nodes[node] += 2;
                    // create new occupied banner
                    _CreateBanner(node, BG_AB_NODE_TYPE_OCCUPIED, teamIndex, true);
                    _SendNodeUpdate(node);
                    _NodeOccupied(node,(teamIndex == 0) ? ALLIANCE:HORDE);
                    // Message to chatlog

                    if (teamIndex == 0)
                    {
                        SendMessage2ToAll(LANG_BG_AB_NODE_TAKEN,CHAT_MSG_BG_SYSTEM_ALLIANCE,NULL,LANG_BG_ALLY,_GetNodeNameId(node));
                        PlaySoundToAll(BG_AB_SOUND_NODE_CAPTURED_ALLIANCE);
                    }
                    else
                    {
                        SendMessage2ToAll(LANG_BG_AB_NODE_TAKEN,CHAT_MSG_BG_SYSTEM_HORDE,NULL,LANG_BG_HORDE,_GetNodeNameId(node));
                        PlaySoundToAll(BG_AB_SOUND_NODE_CAPTURED_HORDE);
                    }
                }
            }

            for (int team = 0; team < BG_TEAMS_COUNT; ++team)
                if (m_Nodes[node] == team + BG_AB_NODE_TYPE_OCCUPIED)
                    ++team_points[team];
        }

        // Accumulate points
        for (int team = 0; team < BG_TEAMS_COUNT; ++team)
        {
            int points = team_points[team];
            if (!points)
                continue;
            m_lastTick[team] += diff;
            if (m_lastTick[team] > BG_AB_TickIntervals[points])
            {
                m_lastTick[team] -= BG_AB_TickIntervals[points];
                m_TeamScores[team] += BG_AB_TickPoints[points];
                m_HonorScoreTics[team] += BG_AB_TickPoints[points];
                m_ReputationScoreTics[team] += BG_AB_TickPoints[points];
                if (m_ReputationScoreTics[team] >= m_ReputationTics)
                {
                    (team == BG_TEAM_ALLIANCE) ? RewardReputationToTeam(509, 10, ALLIANCE) : RewardReputationToTeam(510, 10, HORDE);
                    m_ReputationScoreTics[team] -= m_ReputationTics;
                }
                if (m_HonorScoreTics[team] >= m_HonorTics)
                {
                    RewardHonorToTeam(GetBonusHonorFromKill(1), (team == BG_TEAM_ALLIANCE) ? ALLIANCE : HORDE);
                    m_HonorScoreTics[team] -= m_HonorTics;
                }
                if (!m_IsInformedNearVictory && m_TeamScores[team] > BG_AB_WARNING_NEAR_VICTORY_SCORE)
                {
                    if (team == BG_TEAM_ALLIANCE)
                        SendMessageToAll(LANG_BG_AB_A_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                    else
                        SendMessageToAll(LANG_BG_AB_H_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                    PlaySoundToAll(BG_AB_SOUND_NEAR_VICTORY);
                    m_IsInformedNearVictory = true;
                }

                if (m_TeamScores[team] > BG_AB_MAX_TEAM_SCORE)
                    m_TeamScores[team] = BG_AB_MAX_TEAM_SCORE;
                if (team == BG_TEAM_ALLIANCE)
                    UpdateWorldState(BG_AB_OP_RESOURCES_ALLY, m_TeamScores[team]);
                if (team == BG_TEAM_HORDE)
                    UpdateWorldState(BG_AB_OP_RESOURCES_HORDE, m_TeamScores[team]);

                // update achievement flags
                // we increased m_TeamScores[team] so we just need to check if it is 500 more than other teams resources
                // horde will be a bit disadvantaged, but we can assume that points aren't updated for both team in same Update() call
                uint8 otherTeam = (team + 1) % BG_TEAMS_COUNT;
                if (m_TeamScores[team] > m_TeamScores[otherTeam] + 500)
                    m_TeamScores500Disadvantage[otherTeam] = true;
            }
        }

        // Test win condition
        if (m_TeamScores[BG_TEAM_ALLIANCE] >= BG_AB_MAX_TEAM_SCORE)
            EndBattleGround(ALLIANCE);
        if (m_TeamScores[BG_TEAM_HORDE] >= BG_AB_MAX_TEAM_SCORE)
            EndBattleGround(HORDE);
    }
}

void BattleGroundAB::StartingEventCloseDoors()
{
    // despawn buffs
    for (int i = 0; i < BG_AB_NODES_MAX * 3; ++i)
        SpawnBGObject(m_BgObjects[BG_AB_OBJECT_SPEEDBUFF_STABLES + i], RESPAWN_ONE_DAY);
}

void BattleGroundAB::StartingEventOpenDoors()
{
    for (int i = 0; i < BG_AB_NODES_MAX; ++i)
    {
        //randomly select buff to spawn
        uint8 buff = urand(0, 2);
        SpawnBGObject(m_BgObjects[BG_AB_OBJECT_SPEEDBUFF_STABLES + buff + i * 3], RESPAWN_IMMEDIATELY);
    }
    OpenDoorEvent(BG_EVENT_DOOR);
}

void BattleGroundAB::AddPlayer(Player *plr)
{
    BattleGround::AddPlayer(plr);
    //create score and add it to map, default values are set in the constructor
    BattleGroundABScore* sc = new BattleGroundABScore;

    m_PlayerScores[plr->GetGUID()] = sc;
}

void BattleGroundAB::RemovePlayer(Player * /*plr*/, uint64 /*guid*/)
{

}

void BattleGroundAB::HandleAreaTrigger(Player *Source, uint32 Trigger)
{
    switch(Trigger)
    {
        case 3948:                                          // Arathi Basin Alliance Exit.
            if (Source->GetTeam() != ALLIANCE)
                Source->GetSession()->SendNotification(LANG_BATTLEGROUND_ONLY_ALLIANCE_USE);
            else
                Source->LeaveBattleground();
            break;
        case 3949:                                          // Arathi Basin Horde Exit.
            if (Source->GetTeam() != HORDE)
                Source->GetSession()->SendNotification(LANG_BATTLEGROUND_ONLY_HORDE_USE);
            else
                Source->LeaveBattleground();
            break;
        case 3866:                                          // Stables
        case 3869:                                          // Gold Mine
        case 3867:                                          // Farm
        case 3868:                                          // Lumber Mill
        case 3870:                                          // Black Smith
        case 4020:                                          // Unk1
        case 4021:                                          // Unk2
            //break;
        default:
            //sLog.outError("WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            //Source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }
}

/*  type: 0-neutral, 1-contested, 3-occupied
    teamIndex: 0-ally, 1-horde                        */
void BattleGroundAB::_CreateBanner(uint8 node, uint8 type, uint8 teamIndex, bool delay)
{
    // Just put it into the queue
    if (delay)
    {
        m_BannerTimers[node].timer = 2000;
        m_BannerTimers[node].type = type;
        m_BannerTimers[node].teamIndex = teamIndex;
        return;
    }

    // cause the node-type is in the generic form
    // please see in the headerfile for the ids
    if (type != BG_AB_NODE_TYPE_NEUTRAL)
        type += teamIndex;

    SpawnEvent(node, type, true);                           // will automaticly despawn other events
}

int32 BattleGroundAB::_GetNodeNameId(uint8 node)
{
    switch (node)
    {
        case BG_AB_NODE_STABLES:    return LANG_BG_AB_NODE_STABLES;
        case BG_AB_NODE_BLACKSMITH: return LANG_BG_AB_NODE_BLACKSMITH;
        case BG_AB_NODE_FARM:       return LANG_BG_AB_NODE_FARM;
        case BG_AB_NODE_LUMBER_MILL:return LANG_BG_AB_NODE_LUMBER_MILL;
        case BG_AB_NODE_GOLD_MINE:  return LANG_BG_AB_NODE_GOLD_MINE;
        default:
            MANGOS_ASSERT(0);
    }
    return 0;
}

void BattleGroundAB::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
    const uint8 plusArray[] = {0, 2, 3, 0, 1};

    // Node icons
    for (uint8 node = 0; node < BG_AB_NODES_MAX; ++node)
        FillInitialWorldState(data, count, BG_AB_OP_NODEICONS[node], m_Nodes[node]==0);

    // Node occupied states
    for (uint8 node = 0; node < BG_AB_NODES_MAX; ++node)
        for (uint8 i = 1; i < BG_AB_NODES_MAX; ++i)
            FillInitialWorldState(data, count, BG_AB_OP_NODESTATES[node] + plusArray[i], m_Nodes[node]==i);

    // How many bases each team owns
    uint8 ally = 0, horde = 0;
    for (uint8 node = 0; node < BG_AB_NODES_MAX; ++node)
        if (m_Nodes[node] == BG_AB_NODE_STATUS_ALLY_OCCUPIED)
            ++ally;
        else if (m_Nodes[node] == BG_AB_NODE_STATUS_HORDE_OCCUPIED)
            ++horde;

    FillInitialWorldState(data, count, BG_AB_OP_OCCUPIED_BASES_ALLY, ally);
    FillInitialWorldState(data, count, BG_AB_OP_OCCUPIED_BASES_HORDE, horde);

    // Team scores
    FillInitialWorldState(data, count, BG_AB_OP_RESOURCES_MAX,      BG_AB_MAX_TEAM_SCORE);
    FillInitialWorldState(data, count, BG_AB_OP_RESOURCES_WARNING,  BG_AB_WARNING_NEAR_VICTORY_SCORE);
    FillInitialWorldState(data, count, BG_AB_OP_RESOURCES_ALLY,     m_TeamScores[BG_TEAM_ALLIANCE]);
    FillInitialWorldState(data, count, BG_AB_OP_RESOURCES_HORDE,    m_TeamScores[BG_TEAM_HORDE]);

    // other unknown
    FillInitialWorldState(data, count, 0x745, 0x2);         // 37 1861 unk
}

void BattleGroundAB::_SendNodeUpdate(uint8 node)
{
    // Send node owner state update to refresh map icons on client
    const uint8 plusArray[] = {0, 2, 3, 0, 1};

    if (m_prevNodes[node])
        UpdateWorldState(BG_AB_OP_NODESTATES[node] + plusArray[m_prevNodes[node]], 0);
    else
        UpdateWorldState(BG_AB_OP_NODEICONS[node], 0);

    UpdateWorldState(BG_AB_OP_NODESTATES[node] + plusArray[m_Nodes[node]], 1);

    // How many bases each team owns
    uint8 ally = 0, horde = 0;
    for (uint8 i = 0; i < BG_AB_NODES_MAX; ++i)
        if (m_Nodes[i] == BG_AB_NODE_STATUS_ALLY_OCCUPIED)
            ++ally;
        else if (m_Nodes[i] == BG_AB_NODE_STATUS_HORDE_OCCUPIED)
            ++horde;

    UpdateWorldState(BG_AB_OP_OCCUPIED_BASES_ALLY, ally);
    UpdateWorldState(BG_AB_OP_OCCUPIED_BASES_HORDE, horde);
}

void BattleGroundAB::_NodeOccupied(uint8 node,Team team)
{
    uint8 capturedNodes = 0;
    for (uint8 i = 0; i < BG_AB_NODES_MAX; ++i)
    {
        if (m_Nodes[node] == GetTeamIndexByTeamId(team) + BG_AB_NODE_TYPE_OCCUPIED && !m_NodeTimers[i])
            ++capturedNodes;
    }
    if (capturedNodes >= 5)
        CastSpellOnTeam(SPELL_AB_QUEST_REWARD_5_BASES, team);
    if (capturedNodes >= 4)
        CastSpellOnTeam(SPELL_AB_QUEST_REWARD_4_BASES, team);
}

/* Invoked if a player used a banner as a gameobject */
void BattleGroundAB::EventPlayerClickedOnFlag(Player *source, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint8 event = (sBattleGroundMgr.GetGameObjectEventIndex(target_obj->GetDBTableGUIDLow())).event1;
    if (event >= BG_AB_NODES_MAX)                           // not a node
        return;
    BG_AB_Nodes node = BG_AB_Nodes(event);

    BattleGroundTeamId teamIndex = GetTeamIndexByTeamId(source->GetTeam());

    // Check if player really could use this banner, not cheated
    if (!(m_Nodes[node] == 0 || teamIndex == m_Nodes[node] % 2))
        return;

    source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    uint32 sound = 0;

    // TODO in the following code we should restructure a bit to avoid
    // duplication (or maybe write functions?)
    // If node is neutral, change to contested
    if (m_Nodes[node] == BG_AB_NODE_TYPE_NEUTRAL)
    {
        UpdatePlayerScore(source, SCORE_BASES_ASSAULTED, 1);
        m_prevNodes[node] = m_Nodes[node];
        m_Nodes[node] = teamIndex + 1;
        // create new contested banner
        _CreateBanner(node, BG_AB_NODE_TYPE_CONTESTED, teamIndex, true);
        _SendNodeUpdate(node);
        m_NodeTimers[node] = BG_AB_FLAG_CAPTURING_TIME;

        if (teamIndex == 0)
            SendMessage2ToAll(LANG_BG_AB_NODE_CLAIMED,CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node), LANG_BG_ALLY);
        else
            SendMessage2ToAll(LANG_BG_AB_NODE_CLAIMED,CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node), LANG_BG_HORDE);

        sound = BG_AB_SOUND_NODE_CLAIMED;
    }
    // If node is contested
    else if ((m_Nodes[node] == BG_AB_NODE_STATUS_ALLY_CONTESTED) || (m_Nodes[node] == BG_AB_NODE_STATUS_HORDE_CONTESTED))
    {
        // If last state is NOT occupied, change node to enemy-contested
        if (m_prevNodes[node] < BG_AB_NODE_TYPE_OCCUPIED)
        {
            UpdatePlayerScore(source, SCORE_BASES_ASSAULTED, 1);
            m_prevNodes[node] = m_Nodes[node];
            m_Nodes[node] = teamIndex + BG_AB_NODE_TYPE_CONTESTED;
            // create new contested banner
            _CreateBanner(node, BG_AB_NODE_TYPE_CONTESTED, teamIndex, true);
            _SendNodeUpdate(node);
            m_NodeTimers[node] = BG_AB_FLAG_CAPTURING_TIME;

            if (teamIndex == BG_TEAM_ALLIANCE)
                SendMessage2ToAll(LANG_BG_AB_NODE_ASSAULTED,CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
            else
                SendMessage2ToAll(LANG_BG_AB_NODE_ASSAULTED,CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));
        }
        // If contested, change back to occupied
        else
        {
            UpdatePlayerScore(source, SCORE_BASES_DEFENDED, 1);
            m_prevNodes[node] = m_Nodes[node];
            m_Nodes[node] = teamIndex + BG_AB_NODE_TYPE_OCCUPIED;
            // create new occupied banner
            _CreateBanner(node, BG_AB_NODE_TYPE_OCCUPIED, teamIndex, true);
            _SendNodeUpdate(node);
            m_NodeTimers[node] = 0;
            _NodeOccupied(node,(teamIndex == BG_TEAM_ALLIANCE) ? ALLIANCE:HORDE);

            if (teamIndex == BG_TEAM_ALLIANCE)
                SendMessage2ToAll(LANG_BG_AB_NODE_DEFENDED,CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
            else
                SendMessage2ToAll(LANG_BG_AB_NODE_DEFENDED,CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));
        }
        sound = (teamIndex == BG_TEAM_ALLIANCE) ? BG_AB_SOUND_NODE_ASSAULTED_ALLIANCE : BG_AB_SOUND_NODE_ASSAULTED_HORDE;
    }
    // If node is occupied, change to enemy-contested
    else
    {
        UpdatePlayerScore(source, SCORE_BASES_ASSAULTED, 1);
        m_prevNodes[node] = m_Nodes[node];
        m_Nodes[node] = teamIndex + BG_AB_NODE_TYPE_CONTESTED;
        // create new contested banner
        _CreateBanner(node, BG_AB_NODE_TYPE_CONTESTED, teamIndex, true);
        _SendNodeUpdate(node);
        m_NodeTimers[node] = BG_AB_FLAG_CAPTURING_TIME;

        if (teamIndex == BG_TEAM_ALLIANCE)
            SendMessage2ToAll(LANG_BG_AB_NODE_ASSAULTED,CHAT_MSG_BG_SYSTEM_ALLIANCE, source, _GetNodeNameId(node));
        else
            SendMessage2ToAll(LANG_BG_AB_NODE_ASSAULTED,CHAT_MSG_BG_SYSTEM_HORDE, source, _GetNodeNameId(node));

        sound = (teamIndex == BG_TEAM_ALLIANCE) ? BG_AB_SOUND_NODE_ASSAULTED_ALLIANCE : BG_AB_SOUND_NODE_ASSAULTED_HORDE;
    }

    // If node is occupied again, send "X has taken the Y" msg.
    if (m_Nodes[node] >= BG_AB_NODE_TYPE_OCCUPIED)
    {
        if (teamIndex == BG_TEAM_ALLIANCE)
            SendMessage2ToAll(LANG_BG_AB_NODE_TAKEN,CHAT_MSG_BG_SYSTEM_ALLIANCE, NULL, LANG_BG_ALLY, _GetNodeNameId(node));
        else
            SendMessage2ToAll(LANG_BG_AB_NODE_TAKEN,CHAT_MSG_BG_SYSTEM_HORDE, NULL, LANG_BG_HORDE, _GetNodeNameId(node));
    }
    PlaySoundToAll(sound);
}

bool BattleGroundAB::SetupBattleGround()
{
    //buffs
    for (int i = 0; i < BG_AB_NODES_MAX; ++i)
    {
        if (!AddObject(BG_AB_OBJECT_SPEEDBUFF_STABLES + 3 * i, Buff_Entries[0], BG_AB_BuffPositions[i][0], BG_AB_BuffPositions[i][1], BG_AB_BuffPositions[i][2], BG_AB_BuffPositions[i][3], 0, 0, sin(BG_AB_BuffPositions[i][3]/2), cos(BG_AB_BuffPositions[i][3]/2), RESPAWN_ONE_DAY)
            || !AddObject(BG_AB_OBJECT_SPEEDBUFF_STABLES + 3 * i + 1, Buff_Entries[1], BG_AB_BuffPositions[i][0], BG_AB_BuffPositions[i][1], BG_AB_BuffPositions[i][2], BG_AB_BuffPositions[i][3], 0, 0, sin(BG_AB_BuffPositions[i][3]/2), cos(BG_AB_BuffPositions[i][3]/2), RESPAWN_ONE_DAY)
            || !AddObject(BG_AB_OBJECT_SPEEDBUFF_STABLES + 3 * i + 2, Buff_Entries[2], BG_AB_BuffPositions[i][0], BG_AB_BuffPositions[i][1], BG_AB_BuffPositions[i][2], BG_AB_BuffPositions[i][3], 0, 0, sin(BG_AB_BuffPositions[i][3]/2), cos(BG_AB_BuffPositions[i][3]/2), RESPAWN_ONE_DAY)
            )
            sLog.outErrorDb("BatteGroundAB: Failed to spawn buff object!");
    }

    return true;
}

void BattleGroundAB::Reset()
{
    //call parent's class reset
    BattleGround::Reset();

    for (uint8 i = 0; i < BG_TEAMS_COUNT; ++i)
    {
        m_TeamScores[i]          = 0;
        m_lastTick[i]            = 0;
        m_HonorScoreTics[i]      = 0;
        m_ReputationScoreTics[i] = 0;
        m_TeamScores500Disadvantage[i] = false;
    }

    m_IsInformedNearVictory                 = false;
    bool isBGWeekend = BattleGroundMgr::IsBGWeekend(GetTypeID());
    m_HonorTics = (isBGWeekend) ? BG_AB_ABBGWeekendHonorTicks : BG_AB_NotABBGWeekendHonorTicks;
    m_ReputationTics = (isBGWeekend) ? BG_AB_ABBGWeekendReputationTicks : BG_AB_NotABBGWeekendReputationTicks;

    for (uint8 i = 0; i < BG_AB_NODES_MAX; ++i)
    {
        m_Nodes[i] = 0;
        m_prevNodes[i] = 0;
        m_NodeTimers[i] = 0;
        m_BannerTimers[i].timer = 0;

        // all nodes owned by neutral team at beginning
        m_ActiveEvents[i] = BG_AB_NODE_TYPE_NEUTRAL;
    }

}

void BattleGroundAB::EndBattleGround(uint32 winner)
{
    //win reward
    if (winner == ALLIANCE)
        RewardHonorToTeam(GetBonusHonorFromKill(1), ALLIANCE);
    if (winner == HORDE)
        RewardHonorToTeam(GetBonusHonorFromKill(1), HORDE);
    //complete map_end rewards (even if no team wins)
    RewardHonorToTeam(GetBonusHonorFromKill(1), HORDE);
    RewardHonorToTeam(GetBonusHonorFromKill(1), ALLIANCE);

    BattleGround::EndBattleGround(winner);
}

WorldSafeLocsEntry const* BattleGroundAB::GetClosestGraveYard(Player* player)
{
    BattleGroundTeamId teamIndex = GetTeamIndexByTeamId(player->GetTeam());

    // Is there any occupied node for this team?
    std::vector<uint8> nodes;
    for (uint8 i = 0; i < BG_AB_NODES_MAX; ++i)
        if (m_Nodes[i] == teamIndex + 3)
            nodes.push_back(i);

    WorldSafeLocsEntry const* good_entry = NULL;
    // If so, select the closest node to place ghost on
    if (!nodes.empty())
    {
        float plr_x = player->GetPositionX();
        float plr_y = player->GetPositionY();

        float mindist = 999999.0f;
        for (uint8 i = 0; i < nodes.size(); ++i)
        {
            WorldSafeLocsEntry const*entry = sWorldSafeLocsStore.LookupEntry( BG_AB_GraveyardIds[nodes[i]] );
            if (!entry)
                continue;
            float dist = (entry->x - plr_x)*(entry->x - plr_x)+(entry->y - plr_y)*(entry->y - plr_y);
            if (mindist > dist)
            {
                mindist = dist;
                good_entry = entry;
            }
        }
        nodes.clear();
    }
    // If not, place ghost on starting location
    if (!good_entry)
        good_entry = sWorldSafeLocsStore.LookupEntry( BG_AB_GraveyardIds[teamIndex+5] );

    return good_entry;
}

void BattleGroundAB::UpdatePlayerScore(Player *Source, uint32 type, uint32 value)
{
    BattleGroundScoreMap::iterator itr = m_PlayerScores.find(Source->GetGUID());
    if( itr == m_PlayerScores.end() )                         // player not found...
        return;

    switch(type)
    {
        case SCORE_BASES_ASSAULTED:
            ((BattleGroundABScore*)itr->second)->BasesAssaulted += value;
            break;
        case SCORE_BASES_DEFENDED:
            ((BattleGroundABScore*)itr->second)->BasesDefended += value;
            break;
        default:
            BattleGround::UpdatePlayerScore(Source,type,value);
            break;
    }
}

bool BattleGroundAB::IsAllNodesConrolledByTeam(uint32 team) const
{
    uint32 count = 0;
    for (int i = 0; i < BG_AB_NODES_MAX; ++i)
        if ((team == ALLIANCE && m_Nodes[i] == BG_AB_NODE_STATUS_ALLY_OCCUPIED) ||
            (team == HORDE    && m_Nodes[i] == BG_AB_NODE_STATUS_HORDE_OCCUPIED))
            ++count;

    return count == BG_AB_NODES_MAX;
}
