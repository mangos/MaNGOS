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

#include "Player.h"
#include "BattleGround.h"
#include "BattleGroundAV.h"
#include "BattleGroundMgr.h"
#include "Creature.h"
#include "GameObject.h"
#include "Language.h"
#include "WorldPacket.h"

BattleGroundAV::BattleGroundAV()
{
    m_StartMessageIds[BG_STARTING_EVENT_FIRST]  = 0;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_AV_START_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_AV_START_HALF_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_AV_HAS_BEGUN;
}

BattleGroundAV::~BattleGroundAV()
{
}

void BattleGroundAV::HandleKillPlayer(Player *player, Player *killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    BattleGround::HandleKillPlayer(player, killer);
    UpdateScore(GetTeamIndexByTeamId(player->GetTeam()), -1);
}

void BattleGroundAV::HandleKillUnit(Creature *creature, Player *killer)
{
    DEBUG_LOG("BattleGroundAV: HandleKillUnit %i", creature->GetEntry());
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;
    uint8 event1 = (sBattleGroundMgr.GetCreatureEventIndex(creature->GetDBTableGUIDLow())).event1;
    if (event1 == BG_EVENT_NONE)
        return;
    switch(event1)
    {
        case BG_AV_BOSS_A:
            CastSpellOnTeam(BG_AV_BOSS_KILL_QUEST_SPELL, HORDE);   // this is a spell which finishes a quest where a player has to kill the boss
            RewardReputationToTeam(BG_AV_FACTION_H, m_RepBoss, HORDE);
            RewardHonorToTeam(GetBonusHonorFromKill(BG_AV_KILL_BOSS), HORDE);
            SendYellToAll(LANG_BG_AV_A_GENERAL_DEAD, LANG_UNIVERSAL, GetSingleCreatureGuid(BG_AV_HERALD, 0));
            EndBattleGround(HORDE);
            break;
        case BG_AV_BOSS_H:
            CastSpellOnTeam(BG_AV_BOSS_KILL_QUEST_SPELL, ALLIANCE); // this is a spell which finishes a quest where a player has to kill the boss
            RewardReputationToTeam(BG_AV_FACTION_A, m_RepBoss, ALLIANCE);
            RewardHonorToTeam(GetBonusHonorFromKill(BG_AV_KILL_BOSS), ALLIANCE);
            SendYellToAll(LANG_BG_AV_H_GENERAL_DEAD, LANG_UNIVERSAL, GetSingleCreatureGuid(BG_AV_HERALD, 0));
            EndBattleGround(ALLIANCE);
            break;
        case BG_AV_CAPTAIN_A:
            if (IsActiveEvent(BG_AV_NodeEventCaptainDead_A, 0))
                return;
            RewardReputationToTeam(BG_AV_FACTION_H, m_RepCaptain, HORDE);
            RewardHonorToTeam(GetBonusHonorFromKill(BG_AV_KILL_CAPTAIN), HORDE);
            UpdateScore(BG_TEAM_ALLIANCE, (-1) * BG_AV_RES_CAPTAIN);
            // spawn destroyed aura
            SpawnEvent(BG_AV_NodeEventCaptainDead_A, 0, true);
            break;
        case BG_AV_CAPTAIN_H:
            if (IsActiveEvent(BG_AV_NodeEventCaptainDead_H, 0))
                return;
            RewardReputationToTeam(BG_AV_FACTION_A, m_RepCaptain, ALLIANCE);
            RewardHonorToTeam(GetBonusHonorFromKill(BG_AV_KILL_CAPTAIN), ALLIANCE);
            UpdateScore(BG_TEAM_HORDE, (-1) * BG_AV_RES_CAPTAIN);
            // spawn destroyed aura
            SpawnEvent(BG_AV_NodeEventCaptainDead_H, 0, true);
            break;
        case BG_AV_MINE_BOSSES_NORTH:
            ChangeMineOwner(BG_AV_NORTH_MINE, GetTeamIndexByTeamId(killer->GetTeam()));
            break;
        case BG_AV_MINE_BOSSES_SOUTH:
            ChangeMineOwner(BG_AV_SOUTH_MINE, GetTeamIndexByTeamId(killer->GetTeam()));
            break;
    }
}

void BattleGroundAV::HandleQuestComplete(uint32 questid, Player *player)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;
    uint8 team = GetTeamIndexByTeamId(player->GetTeam());
    uint32 reputation = 0;                                  // reputation for the whole team (other reputation must be done in db)
    // TODO add events (including quest not available anymore, next quest availabe, go/npc de/spawning)
    sLog.outError("BattleGroundAV: Quest %i completed", questid);
    switch(questid)
    {
        case BG_AV_QUEST_A_SCRAPS1:
        case BG_AV_QUEST_A_SCRAPS2:
        case BG_AV_QUEST_H_SCRAPS1:
        case BG_AV_QUEST_H_SCRAPS2:
            m_Team_QuestStatus[team][0] += 20;
            reputation = 1;
            if( m_Team_QuestStatus[team][0] == 500 || m_Team_QuestStatus[team][0] == 1000 || m_Team_QuestStatus[team][0] == 1500 ) //25,50,75 turn ins
            {
                DEBUG_LOG("BattleGroundAV: Quest %i completed starting with unit upgrading..", questid);
                for (BG_AV_Nodes i = BG_AV_NODES_FIRSTAID_STATION; i <= BG_AV_NODES_FROSTWOLF_HUT; ++i)
                    if (m_Nodes[i].Owner == team && m_Nodes[i].State == POINT_CONTROLLED)
                        PopulateNode(i);
            }
            break;
        case BG_AV_QUEST_A_COMMANDER1:
        case BG_AV_QUEST_H_COMMANDER1:
            m_Team_QuestStatus[team][1]++;
            reputation = 1;
            if (m_Team_QuestStatus[team][1] == 120)
                DEBUG_LOG("BattleGroundAV: Quest %i completed (need to implement some events here", questid);
            break;
        case BG_AV_QUEST_A_COMMANDER2:
        case BG_AV_QUEST_H_COMMANDER2:
            m_Team_QuestStatus[team][2]++;
            reputation = 2;
            if (m_Team_QuestStatus[team][2] == 60)
                DEBUG_LOG("BattleGroundAV: Quest %i completed (need to implement some events here", questid);
            break;
        case BG_AV_QUEST_A_COMMANDER3:
        case BG_AV_QUEST_H_COMMANDER3:
            m_Team_QuestStatus[team][3]++;
            reputation = 5;
            RewardReputationToTeam(team, 1, player->GetTeam());
            if (m_Team_QuestStatus[team][1] == 30)
                DEBUG_LOG("BattleGroundAV: Quest %i completed (need to implement some events here", questid);
            break;
        case BG_AV_QUEST_A_BOSS1:
        case BG_AV_QUEST_H_BOSS1:
            m_Team_QuestStatus[team][4] += 4;               // there are 2 quests where you can turn in 5 or 1 item.. ( + 4 cause +1 will be done some lines below)
            reputation = 4;
        case BG_AV_QUEST_A_BOSS2:
        case BG_AV_QUEST_H_BOSS2:
            m_Team_QuestStatus[team][4]++;
            reputation += 1;
            if (m_Team_QuestStatus[team][4] >= 200)
                DEBUG_LOG("BattleGroundAV: Quest %i completed (need to implement some events here", questid);
            break;
        case BG_AV_QUEST_A_NEAR_MINE:
        case BG_AV_QUEST_H_NEAR_MINE:
            m_Team_QuestStatus[team][5]++;
            reputation = 2;
            if (m_Team_QuestStatus[team][5] == 28)
            {
                DEBUG_LOG("BattleGroundAV: Quest %i completed (need to implement some events here", questid);
                if (m_Team_QuestStatus[team][6] == 7)
                    DEBUG_LOG("BattleGroundAV: Quest %i completed (need to implement some events here - ground assault ready", questid);
            }
            break;
        case BG_AV_QUEST_A_OTHER_MINE:
        case BG_AV_QUEST_H_OTHER_MINE:
            m_Team_QuestStatus[team][6]++;
            reputation = 3;
            if (m_Team_QuestStatus[team][6] == 7)
            {
                DEBUG_LOG("BattleGroundAV: Quest %i completed (need to implement some events here", questid);
                if (m_Team_QuestStatus[team][5] == 20)
                    DEBUG_LOG("BattleGroundAV: Quest %i completed (need to implement some events here - ground assault ready", questid);
            }
            break;
        case BG_AV_QUEST_A_RIDER_HIDE:
        case BG_AV_QUEST_H_RIDER_HIDE:
            m_Team_QuestStatus[team][7]++;
            reputation = 1;
            if (m_Team_QuestStatus[team][7] == 25)
            {
                DEBUG_LOG("BattleGroundAV: Quest %i completed (need to implement some events here", questid);
                if (m_Team_QuestStatus[team][8] == 25)
                    DEBUG_LOG("BattleGroundAV: Quest %i completed (need to implement some events here - rider assault ready", questid);
            }
            break;
        case BG_AV_QUEST_A_RIDER_TAME:
        case BG_AV_QUEST_H_RIDER_TAME:
            m_Team_QuestStatus[team][8]++;
            reputation = 1;
            if (m_Team_QuestStatus[team][8] == 25)
            {
                DEBUG_LOG("BattleGroundAV: Quest %i completed (need to implement some events here", questid);
                if (m_Team_QuestStatus[team][7] == 25)
                    DEBUG_LOG("BattleGroundAV: Quest %i completed (need to implement some events here - rider assault ready", questid);
            }
            break;
        default:
            DEBUG_LOG("BattleGroundAV: Quest %i completed but is not interesting for us", questid);
            return;
            break;
    }
    if (reputation)
        RewardReputationToTeam((player->GetTeam() == ALLIANCE) ? BG_AV_FACTION_A : BG_AV_FACTION_H, reputation, player->GetTeam());
}

void BattleGroundAV::UpdateScore(BattleGroundTeamId team, int32 points )
{
    // note: to remove reinforcements points must be negative, for adding reinforcements points must be positive
    MANGOS_ASSERT( team == BG_TEAM_ALLIANCE || team == BG_TEAM_HORDE);
    m_TeamScores[team] += points;                      // m_TeamScores is int32 - so no problems here

    if (points < 0)
    {
        if (m_TeamScores[team] < 1)
        {
            m_TeamScores[team] = 0;
            // other team will win:
            EndBattleGround((team == BG_TEAM_ALLIANCE)? HORDE : ALLIANCE);
        }
        else if (!m_IsInformedNearLose[team] && m_TeamScores[team] < BG_AV_SCORE_NEAR_LOSE)
        {
            SendMessageToAll((team == BG_TEAM_HORDE) ? LANG_BG_AV_H_NEAR_LOSE : LANG_BG_AV_A_NEAR_LOSE, CHAT_MSG_BG_SYSTEM_NEUTRAL);
            PlaySoundToAll(BG_AV_SOUND_NEAR_LOSE);
            m_IsInformedNearLose[team] = true;
        }
    }
    // must be called here, else it could display a negative value
    UpdateWorldState(((team == BG_TEAM_HORDE) ? BG_AV_Horde_Score : BG_AV_Alliance_Score), m_TeamScores[team]);
}

void BattleGroundAV::Update(uint32 diff)
{
    BattleGround::Update(diff);
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    // add points from mine owning, and look if the neutral team can reclaim the mine
    for(uint8 mine = 0; mine < BG_AV_MAX_MINES; mine++)
    {
        if (m_Mine_Owner[mine] == BG_TEAM_ALLIANCE || m_Mine_Owner[mine] == BG_TEAM_HORDE)
        {
            m_Mine_Timer[mine] -=diff;
            if (m_Mine_Timer[mine] <= 0)
            {
                UpdateScore(BattleGroundTeamId(m_Mine_Owner[mine]), 1);
                m_Mine_Timer[mine] = BG_AV_MINE_TICK_TIMER;
            }

            if (m_Mine_Reclaim_Timer[mine] > diff)
                m_Mine_Reclaim_Timer[mine] -= diff;
            else
                ChangeMineOwner(mine, BG_AV_NEUTRAL_TEAM);
        }
    }

    // looks for all timers of the nodes and destroy the building (for graveyards the building wont get destroyed, it goes just to the other team
    for(BG_AV_Nodes i = BG_AV_NODES_FIRSTAID_STATION; i < BG_AV_NODES_MAX; ++i)
    {
        if (m_Nodes[i].State == POINT_ASSAULTED)
        {
            if (m_Nodes[i].Timer > diff)
                m_Nodes[i].Timer -= diff;
            else
                EventPlayerDestroyedPoint(i);
        }
    }
}

void BattleGroundAV::StartingEventCloseDoors()
{
    DEBUG_LOG("BattleGroundAV: entering state STATUS_WAIT_JOIN ...");
}

void BattleGroundAV::StartingEventOpenDoors()
{
    UpdateWorldState(BG_AV_SHOW_H_SCORE, 1);
    UpdateWorldState(BG_AV_SHOW_A_SCORE, 1);

    OpenDoorEvent(BG_EVENT_DOOR);
}

void BattleGroundAV::AddPlayer(Player *plr)
{
    BattleGround::AddPlayer(plr);
    // create score and add it to map, default values are set in constructor
    BattleGroundAVScore* sc = new BattleGroundAVScore;
    m_PlayerScores[plr->GetGUID()] = sc;
}

void BattleGroundAV::EndBattleGround(uint32 winner)
{
    // calculate bonuskills for both teams:
    uint32 tower_survived[BG_TEAMS_COUNT]  = {0, 0};
    uint32 graves_owned[BG_TEAMS_COUNT]    = {0, 0};
    uint32 mines_owned[BG_TEAMS_COUNT]     = {0, 0};
    // towers all not destroyed:
    for(BG_AV_Nodes i = BG_AV_NODES_DUNBALDAR_SOUTH; i <= BG_AV_NODES_STONEHEART_BUNKER; ++i)
        if (m_Nodes[i].State == POINT_CONTROLLED)
            if (m_Nodes[i].TotalOwner == BG_TEAM_ALLIANCE)
                ++tower_survived[BG_TEAM_ALLIANCE];
    for(BG_AV_Nodes i = BG_AV_NODES_ICEBLOOD_TOWER; i <= BG_AV_NODES_FROSTWOLF_WTOWER; ++i)
        if (m_Nodes[i].State == POINT_CONTROLLED)
            if (m_Nodes[i].TotalOwner == BG_TEAM_HORDE)
                ++tower_survived[BG_TEAM_HORDE];

    // graves all controlled
    for(BG_AV_Nodes i = BG_AV_NODES_FIRSTAID_STATION; i < BG_AV_NODES_MAX; ++i)
        if (m_Nodes[i].State == POINT_CONTROLLED)
            ++graves_owned[m_Nodes[i].Owner];

    for (uint32 i = 0; i < BG_AV_MAX_MINES; ++i)
        if (m_Mine_Owner[i] != BG_AV_NEUTRAL_TEAM)
            ++mines_owned[m_Mine_Owner[i]];

    // now we have the values give the honor/reputation to the teams:
    uint32 team[BG_TEAMS_COUNT]      = { ALLIANCE, HORDE };
    uint32 faction[BG_TEAMS_COUNT]   = { BG_AV_FACTION_A, BG_AV_FACTION_H };
    for (uint32 i = 0; i < BG_TEAMS_COUNT; i++)
    {
        if (tower_survived[i])
        {
            RewardReputationToTeam(faction[i], tower_survived[i] * m_RepSurviveTower, team[i]);
            RewardHonorToTeam(GetBonusHonorFromKill(tower_survived[i] * BG_AV_KILL_SURVIVING_TOWER), team[i]);
        }
        DEBUG_LOG("BattleGroundAV: EndbattleGround: bgteam: %u towers:%u honor:%u rep:%u", i, tower_survived[i], GetBonusHonorFromKill(tower_survived[i] * BG_AV_KILL_SURVIVING_TOWER), tower_survived[i] * BG_AV_REP_SURVIVING_TOWER);
        if (graves_owned[i])
            RewardReputationToTeam(faction[i], graves_owned[i] * m_RepOwnedGrave, team[i]);
        if (mines_owned[i])
            RewardReputationToTeam(faction[i], mines_owned[i] * m_RepOwnedMine, team[i]);
        // captain survived?:
        if (!IsActiveEvent(BG_AV_NodeEventCaptainDead_A + GetTeamIndexByTeamId(team[i]), 0))
        {
            RewardReputationToTeam(faction[i], m_RepSurviveCaptain, team[i]);
            RewardHonorToTeam(GetBonusHonorFromKill(BG_AV_KILL_SURVIVING_CAPTAIN), team[i]);
        }
    }

    // both teams:
    if (m_HonorMapComplete)
    {
        RewardHonorToTeam(m_HonorMapComplete, ALLIANCE);
        RewardHonorToTeam(m_HonorMapComplete, HORDE);
    }
    BattleGround::EndBattleGround(winner);
}

void BattleGroundAV::RemovePlayer(Player* /*plr*/,uint64 /*guid*/)
{
}

void BattleGroundAV::HandleAreaTrigger(Player *Source, uint32 Trigger)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    switch(Trigger)
    {
        case 95:
        case 2608:
            if (Source->GetTeam() != ALLIANCE)
                Source->GetSession()->SendNotification(LANG_BATTLEGROUND_ONLY_ALLIANCE_USE);
            else
                Source->LeaveBattleground();
            break;
        case 2606:
            if (Source->GetTeam() != HORDE)
                Source->GetSession()->SendNotification(LANG_BATTLEGROUND_ONLY_HORDE_USE);
            else
                Source->LeaveBattleground();
            break;
        case 3326:
        case 3327:
        case 3328:
        case 3329:
        case 3330:
        case 3331:
            //Source->Unmount();
            break;
        default:
            DEBUG_LOG("BattleGroundAV: WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
//            Source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }
}

void BattleGroundAV::UpdatePlayerScore(Player* Source, uint32 type, uint32 value)
{

    BattleGroundScoreMap::iterator itr = m_PlayerScores.find(Source->GetGUID());
    if(itr == m_PlayerScores.end())                         // player not found...
        return;

    switch(type)
    {
        case SCORE_GRAVEYARDS_ASSAULTED:
            ((BattleGroundAVScore*)itr->second)->GraveyardsAssaulted += value;
            break;
        case SCORE_GRAVEYARDS_DEFENDED:
            ((BattleGroundAVScore*)itr->second)->GraveyardsDefended += value;
            break;
        case SCORE_TOWERS_ASSAULTED:
            ((BattleGroundAVScore*)itr->second)->TowersAssaulted += value;
            break;
        case SCORE_TOWERS_DEFENDED:
            ((BattleGroundAVScore*)itr->second)->TowersDefended += value;
            break;
        case SCORE_SECONDARY_OBJECTIVES:
            ((BattleGroundAVScore*)itr->second)->SecondaryObjectives += value;
            break;
        default:
            BattleGround::UpdatePlayerScore(Source, type, value);
            break;
    }
}

void BattleGroundAV::EventPlayerDestroyedPoint(BG_AV_Nodes node)
{

    DEBUG_LOG("BattleGroundAV: player destroyed point node %i", node);

    // despawn banner
    DestroyNode(node);
    PopulateNode(node);
    UpdateNodeWorldState(node);

    uint32 owner = m_Nodes[node].Owner;
    if (IsTower(node))
    {
        uint8 tmp = node - BG_AV_NODES_DUNBALDAR_SOUTH;
        // despawn marshal (one of those guys protecting the boss)
        SpawnEvent(BG_AV_MARSHAL_A_SOUTH + tmp, 0, false);

        UpdateScore(BattleGroundTeamId(owner^0x1), (-1) * BG_AV_RES_TOWER);
        RewardReputationToTeam((owner == BG_TEAM_ALLIANCE) ? BG_AV_FACTION_A : BG_AV_FACTION_H, m_RepTowerDestruction, owner);
        RewardHonorToTeam(GetBonusHonorFromKill(BG_AV_KILL_TOWER), owner);
        SendYell2ToAll(LANG_BG_AV_TOWER_TAKEN, LANG_UNIVERSAL, GetSingleCreatureGuid(BG_AV_HERALD, 0), GetNodeName(node), ( owner == BG_TEAM_ALLIANCE ) ? LANG_BG_ALLY : LANG_BG_HORDE);
    }
    else
    {
        SendYell2ToAll(LANG_BG_AV_GRAVE_TAKEN, LANG_UNIVERSAL, GetSingleCreatureGuid(BG_AV_HERALD, 0), GetNodeName(node), ( owner == BG_TEAM_ALLIANCE ) ? LANG_BG_ALLY : LANG_BG_HORDE);
    }
}

void BattleGroundAV::ChangeMineOwner(uint8 mine, uint32 team)
{
    m_Mine_Timer[mine] = BG_AV_MINE_TICK_TIMER;
    // TODO implement quest 7122
    // mine=0 northmine, mine=1 southmine
    // TODO changing the owner should result in setting respawntime to infinite for current creatures (they should fight the new ones), spawning new mine owners creatures and changing the chest - objects so that the current owning team can use them
    MANGOS_ASSERT(mine == BG_AV_NORTH_MINE || mine == BG_AV_SOUTH_MINE);
    if (m_Mine_Owner[mine] == int8(team))
        return;

    if (team != BG_TEAM_ALLIANCE && team != BG_TEAM_HORDE)
        team = BG_AV_NEUTRAL_TEAM;

    m_Mine_PrevOwner[mine] = m_Mine_Owner[mine];
    m_Mine_Owner[mine] = team;

    SendMineWorldStates(mine);

    SpawnEvent(BG_AV_MINE_EVENT + mine, team, true);
    SpawnEvent(BG_AV_MINE_BOSSES + mine, team, true);

    if (team == BG_TEAM_ALLIANCE || team == BG_TEAM_HORDE)
    {
        PlaySoundToAll((team == BG_TEAM_ALLIANCE) ? BG_AV_SOUND_ALLIANCE_GOOD : BG_AV_SOUND_HORDE_GOOD);
        m_Mine_Reclaim_Timer[mine] = BG_AV_MINE_RECLAIM_TIMER;
        SendYell2ToAll(LANG_BG_AV_MINE_TAKEN , LANG_UNIVERSAL, GetSingleCreatureGuid(BG_AV_HERALD, 0),
            (team == BG_TEAM_ALLIANCE ) ? LANG_BG_ALLY : LANG_BG_HORDE,
            (mine == BG_AV_NORTH_MINE) ? LANG_BG_AV_MINE_NORTH : LANG_BG_AV_MINE_SOUTH);
    }
}

bool BattleGroundAV::PlayerCanDoMineQuest(int32 GOId, uint32 team)
{
    if (GOId == BG_AV_OBJECTID_MINE_N)
        return (m_Mine_Owner[BG_AV_NORTH_MINE] == GetTeamIndexByTeamId(team));
    if (GOId == BG_AV_OBJECTID_MINE_S)
        return (m_Mine_Owner[BG_AV_SOUTH_MINE] == GetTeamIndexByTeamId(team));
    return true;                                            // cause it's no mine'object it is ok if this is true
}

/// will spawn and despawn creatures around a node
/// more a wrapper around spawnevent cause graveyards are special
void BattleGroundAV::PopulateNode(BG_AV_Nodes node)
{
    uint32 team = m_Nodes[node].Owner;
    if (IsGrave(node) && team != BG_AV_NEUTRAL_TEAM)
    {
        uint32 graveDefenderType;
        if (m_Team_QuestStatus[team][0] < 500 )
            graveDefenderType = 0;
        else if (m_Team_QuestStatus[team][0] < 1000 )
            graveDefenderType = 1;
        else if (m_Team_QuestStatus[team][0] < 1500 )
            graveDefenderType = 2;
        else
            graveDefenderType = 3;

        if (m_Nodes[node].State == POINT_CONTROLLED) // we can spawn the current owner event
            SpawnEvent(BG_AV_NODES_MAX + node, team * BG_AV_MAX_GRAVETYPES + graveDefenderType, true);
        else // we despawn the event from the prevowner
            SpawnEvent(BG_AV_NODES_MAX + node, m_Nodes[node].PrevOwner * BG_AV_MAX_GRAVETYPES + graveDefenderType, false);
    }
    SpawnEvent(node, (team * BG_AV_MAX_STATES) + m_Nodes[node].State, true);
}

/// called when using a banner
void BattleGroundAV::EventPlayerClickedOnFlag(Player *source, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;
    DEBUG_LOG("BattleGroundAV: using gameobject %i", target_obj->GetEntry());
    uint8 event = (sBattleGroundMgr.GetGameObjectEventIndex(target_obj->GetDBTableGUIDLow())).event1;
    if (event >= BG_AV_NODES_MAX)                           // not a node
        return;
    BG_AV_Nodes node = BG_AV_Nodes(event);
    switch ((sBattleGroundMgr.GetGameObjectEventIndex(target_obj->GetDBTableGUIDLow())).event2 % BG_AV_MAX_STATES)
    {
        case POINT_CONTROLLED:
            EventPlayerAssaultsPoint(source, node);
            break;
        case POINT_ASSAULTED:
            EventPlayerDefendsPoint(source, node);
            break;
        default:
            break;
    }
}

void BattleGroundAV::EventPlayerDefendsPoint(Player* player, BG_AV_Nodes node)
{
    MANGOS_ASSERT(GetStatus() == STATUS_IN_PROGRESS);

    uint32 team = GetTeamIndexByTeamId(player->GetTeam());

    if (m_Nodes[node].Owner == team || m_Nodes[node].State != POINT_ASSAULTED)
        return;
    if( m_Nodes[node].TotalOwner == BG_AV_NEUTRAL_TEAM )    // initial snowfall capture
    {
        // until snowfall doesn't belong to anyone it is better handled in assault - code (best would be to have a special function
        // for neutral nodes.. but doing this just for snowfall will be a bit to much i think
        MANGOS_ASSERT(node == BG_AV_NODES_SNOWFALL_GRAVE);  // currently the only neutral grave
        EventPlayerAssaultsPoint(player, node);
        return;
    }
    DEBUG_LOG("BattleGroundAV: player defends node: %i", node);
    if (m_Nodes[node].PrevOwner != team)
    {
        sLog.outError("BattleGroundAV: player defends point which doesn't belong to his team %i", node);
        return;
    }

    DefendNode(node, team);                                 // set the right variables for nodeinfo
    PopulateNode(node);                                     // spawn node-creatures (defender for example)
    UpdateNodeWorldState(node);                             // send new mapicon to the player

    if (IsTower(node))
    {
        SendYell2ToAll( LANG_BG_AV_TOWER_DEFENDED, LANG_UNIVERSAL, GetSingleCreatureGuid(BG_AV_HERALD, 0),
            GetNodeName(node),
            ( team == BG_TEAM_ALLIANCE ) ? LANG_BG_ALLY:LANG_BG_HORDE);
        UpdatePlayerScore(player, SCORE_TOWERS_DEFENDED, 1);
        PlaySoundToAll(BG_AV_SOUND_BOTH_TOWER_DEFEND);
    }
    else
    {
        SendYell2ToAll(LANG_BG_AV_GRAVE_DEFENDED, LANG_UNIVERSAL, GetSingleCreatureGuid(BG_AV_HERALD, 0),
            GetNodeName(node),
            ( team == BG_TEAM_ALLIANCE ) ? LANG_BG_ALLY:LANG_BG_HORDE);
        UpdatePlayerScore(player, SCORE_GRAVEYARDS_DEFENDED, 1);
    // update the statistic for the defending player
        PlaySoundToAll((team == BG_TEAM_ALLIANCE)?BG_AV_SOUND_ALLIANCE_GOOD:BG_AV_SOUND_HORDE_GOOD);
    }
}

void BattleGroundAV::EventPlayerAssaultsPoint(Player* player, BG_AV_Nodes node)
{
    // TODO implement quest 7101, 7081
    uint32 team  = GetTeamIndexByTeamId(player->GetTeam());
    DEBUG_LOG("BattleGroundAV: player assaults node %i", node);
    if (m_Nodes[node].Owner == team || team == m_Nodes[node].TotalOwner)
        return;

    AssaultNode(node, team);                                // update nodeinfo variables
    UpdateNodeWorldState(node);                             // send mapicon
    PopulateNode(node);

    if (IsTower(node))
    {
        SendYell2ToAll(LANG_BG_AV_TOWER_ASSAULTED, LANG_UNIVERSAL, GetSingleCreatureGuid(BG_AV_HERALD, 0),
            GetNodeName(node),
            ( team == BG_TEAM_ALLIANCE ) ? LANG_BG_ALLY:LANG_BG_HORDE);
        UpdatePlayerScore(player, SCORE_TOWERS_ASSAULTED, 1);
    }
    else
    {
        SendYell2ToAll(LANG_BG_AV_GRAVE_ASSAULTED, LANG_UNIVERSAL, GetSingleCreatureGuid(BG_AV_HERALD, 0),
            GetNodeName(node),
            ( team == BG_TEAM_ALLIANCE ) ? LANG_BG_ALLY:LANG_BG_HORDE);
        // update the statistic for the assaulting player
        UpdatePlayerScore(player, SCORE_GRAVEYARDS_ASSAULTED, 1);
    }

    PlaySoundToAll((team == BG_TEAM_ALLIANCE) ? BG_AV_SOUND_ALLIANCE_ASSAULTS : BG_AV_SOUND_HORDE_ASSAULTS);
}

void BattleGroundAV::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
    bool stateok;
    for (uint32 i = BG_AV_NODES_FIRSTAID_STATION; i < BG_AV_NODES_MAX; ++i)
    {
        for (uint8 j = 0; j < BG_AV_MAX_STATES; j++)
        {
            stateok = (m_Nodes[i].State == j);
            FillInitialWorldState(data, count, BG_AV_NodeWorldStates[i][GetWorldStateType(j, BG_TEAM_ALLIANCE)],
                m_Nodes[i].Owner == BG_TEAM_ALLIANCE && stateok);
            FillInitialWorldState(data, count, BG_AV_NodeWorldStates[i][GetWorldStateType(j, BG_TEAM_HORDE)],
                m_Nodes[i].Owner == BG_TEAM_HORDE && stateok);
        }
    }

    if( m_Nodes[BG_AV_NODES_SNOWFALL_GRAVE].Owner == BG_AV_NEUTRAL_TEAM )   // cause neutral teams aren't handled generic
        FillInitialWorldState(data, count, AV_SNOWFALL_N, 1);

    FillInitialWorldState(data, count, BG_AV_Alliance_Score, m_TeamScores[BG_TEAM_ALLIANCE]);
    FillInitialWorldState(data, count, BG_AV_Horde_Score,    m_TeamScores[BG_TEAM_HORDE]);
    if( GetStatus() == STATUS_IN_PROGRESS )                 // only if game is running the teamscores are displayed
    {
        FillInitialWorldState(data, count, BG_AV_SHOW_A_SCORE, 1);
        FillInitialWorldState(data, count, BG_AV_SHOW_H_SCORE, 1);
    }
    else
    {
        FillInitialWorldState(data, count, BG_AV_SHOW_A_SCORE, 0);
        FillInitialWorldState(data, count, BG_AV_SHOW_H_SCORE, 0);
    }

    FillInitialWorldState(data, count, BG_AV_MineWorldStates[BG_AV_NORTH_MINE][m_Mine_Owner[BG_AV_NORTH_MINE]], 1);
    if (m_Mine_Owner[BG_AV_NORTH_MINE] != m_Mine_PrevOwner[BG_AV_NORTH_MINE])
        FillInitialWorldState(data, count, BG_AV_MineWorldStates[BG_AV_NORTH_MINE][m_Mine_PrevOwner[BG_AV_NORTH_MINE]], 0);

    FillInitialWorldState(data, count, BG_AV_MineWorldStates[BG_AV_SOUTH_MINE][m_Mine_Owner[BG_AV_SOUTH_MINE]], 1);
    if (m_Mine_Owner[BG_AV_SOUTH_MINE] != m_Mine_PrevOwner[BG_AV_SOUTH_MINE])
        FillInitialWorldState(data, count, BG_AV_MineWorldStates[BG_AV_SOUTH_MINE][m_Mine_PrevOwner[BG_AV_SOUTH_MINE]], 0);
}

void BattleGroundAV::UpdateNodeWorldState(BG_AV_Nodes node)
{
    UpdateWorldState(BG_AV_NodeWorldStates[node][GetWorldStateType(m_Nodes[node].State,m_Nodes[node].Owner)], 1);
    if( m_Nodes[node].PrevOwner == BG_AV_NEUTRAL_TEAM )     // currently only snowfall is supported as neutral node
        UpdateWorldState(AV_SNOWFALL_N, 0);
    else
        UpdateWorldState(BG_AV_NodeWorldStates[node][GetWorldStateType(m_Nodes[node].PrevState,m_Nodes[node].PrevOwner)], 0);
}

void BattleGroundAV::SendMineWorldStates(uint32 mine)
{
    MANGOS_ASSERT(mine == BG_AV_NORTH_MINE || mine == BG_AV_SOUTH_MINE);
    MANGOS_ASSERT(m_Mine_PrevOwner[mine] == BG_TEAM_ALLIANCE || m_Mine_PrevOwner[mine] == BG_TEAM_HORDE || m_Mine_PrevOwner[mine] == BG_AV_NEUTRAL_TEAM);
    MANGOS_ASSERT(m_Mine_Owner[mine] == BG_TEAM_ALLIANCE || m_Mine_Owner[mine] == BG_TEAM_HORDE || m_Mine_Owner[mine] == BG_AV_NEUTRAL_TEAM);

    UpdateWorldState(BG_AV_MineWorldStates[mine][m_Mine_Owner[mine]], 1);
    if (m_Mine_Owner[mine] != m_Mine_PrevOwner[mine])
        UpdateWorldState(BG_AV_MineWorldStates[mine][m_Mine_PrevOwner[mine]], 0);
}

WorldSafeLocsEntry const* BattleGroundAV::GetClosestGraveYard(Player *plr)
{
    float x = plr->GetPositionX();
    float y = plr->GetPositionY();
    uint32 team = GetTeamIndexByTeamId(plr->GetTeam());
    WorldSafeLocsEntry const* good_entry = NULL;
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        // Is there any occupied node for this team?
        float mindist = 9999999.0f;
        for(uint8 i = BG_AV_NODES_FIRSTAID_STATION; i <= BG_AV_NODES_FROSTWOLF_HUT; ++i)
        {
            if (m_Nodes[i].Owner != team || m_Nodes[i].State != POINT_CONTROLLED)
                continue;
            WorldSafeLocsEntry const * entry = sWorldSafeLocsStore.LookupEntry( BG_AV_GraveyardIds[i] );
            if (!entry)
                continue;
            float dist = (entry->x - x) * (entry->x - x) + (entry->y - y) * (entry->y - y);
            if (mindist > dist)
            {
                mindist = dist;
                good_entry = entry;
            }
        }
    }
    // If not, place ghost in the starting-cave
    if (!good_entry)
        good_entry = sWorldSafeLocsStore.LookupEntry( BG_AV_GraveyardIds[team + 7] );

    return good_entry;
}

uint32 BattleGroundAV::GetNodeName(BG_AV_Nodes node)
{
    switch (node)
    {
        case BG_AV_NODES_FIRSTAID_STATION:  return LANG_BG_AV_NODE_GRAVE_STORM_AID;
        case BG_AV_NODES_DUNBALDAR_SOUTH:   return LANG_BG_AV_NODE_TOWER_DUN_S;
        case BG_AV_NODES_DUNBALDAR_NORTH:   return LANG_BG_AV_NODE_TOWER_DUN_N;
        case BG_AV_NODES_STORMPIKE_GRAVE:   return LANG_BG_AV_NODE_GRAVE_STORMPIKE;
        case BG_AV_NODES_ICEWING_BUNKER:    return LANG_BG_AV_NODE_TOWER_ICEWING;
        case BG_AV_NODES_STONEHEART_GRAVE:  return LANG_BG_AV_NODE_GRAVE_STONE;
        case BG_AV_NODES_STONEHEART_BUNKER: return LANG_BG_AV_NODE_TOWER_STONE;
        case BG_AV_NODES_SNOWFALL_GRAVE:    return LANG_BG_AV_NODE_GRAVE_SNOW;
        case BG_AV_NODES_ICEBLOOD_TOWER:    return LANG_BG_AV_NODE_TOWER_ICE;
        case BG_AV_NODES_ICEBLOOD_GRAVE:    return LANG_BG_AV_NODE_GRAVE_ICE;
        case BG_AV_NODES_TOWER_POINT:       return LANG_BG_AV_NODE_TOWER_POINT;
        case BG_AV_NODES_FROSTWOLF_GRAVE:   return LANG_BG_AV_NODE_GRAVE_FROST;
        case BG_AV_NODES_FROSTWOLF_ETOWER:  return LANG_BG_AV_NODE_TOWER_FROST_E;
        case BG_AV_NODES_FROSTWOLF_WTOWER:  return LANG_BG_AV_NODE_TOWER_FROST_W;
        case BG_AV_NODES_FROSTWOLF_HUT:     return LANG_BG_AV_NODE_GRAVE_FROST_HUT;
        default: return 0; break;
    }
}

void BattleGroundAV::AssaultNode(BG_AV_Nodes node, uint32 team)
{
    MANGOS_ASSERT(team < 3);                                // alliance:0, horde:1, neutral:2
    MANGOS_ASSERT(m_Nodes[node].TotalOwner != team);
    MANGOS_ASSERT(m_Nodes[node].Owner != team);
    // only assault an assaulted node if no totalowner exists:
    MANGOS_ASSERT(m_Nodes[node].State != POINT_ASSAULTED || m_Nodes[node].TotalOwner == BG_AV_NEUTRAL_TEAM);
    // the timer gets another time, if the previous owner was 0 == Neutral
    m_Nodes[node].Timer      = (m_Nodes[node].PrevOwner != BG_AV_NEUTRAL_TEAM) ? BG_AV_CAPTIME : BG_AV_SNOWFALL_FIRSTCAP;
    m_Nodes[node].PrevOwner  = m_Nodes[node].Owner;
    m_Nodes[node].Owner      = team;
    m_Nodes[node].PrevState  = m_Nodes[node].State;
    m_Nodes[node].State      = POINT_ASSAULTED;
}

void BattleGroundAV::DestroyNode(BG_AV_Nodes node)
{
    MANGOS_ASSERT(m_Nodes[node].State == POINT_ASSAULTED);

    m_Nodes[node].TotalOwner = m_Nodes[node].Owner;
    m_Nodes[node].PrevOwner  = m_Nodes[node].Owner;
    m_Nodes[node].PrevState  = m_Nodes[node].State;
    m_Nodes[node].State      = POINT_CONTROLLED;
    m_Nodes[node].Timer      = 0;
}

void BattleGroundAV::InitNode(BG_AV_Nodes node, uint32 team, bool tower)
{
    MANGOS_ASSERT(team < 3);                                // alliance:0, horde:1, neutral:2
    m_Nodes[node].TotalOwner = team;
    m_Nodes[node].Owner      = team;
    m_Nodes[node].PrevOwner  = team;
    m_Nodes[node].State      = POINT_CONTROLLED;
    m_Nodes[node].PrevState  = m_Nodes[node].State;
    m_Nodes[node].State      = POINT_CONTROLLED;
    m_Nodes[node].Timer      = 0;
    m_Nodes[node].Tower      = tower;
    m_ActiveEvents[node] = team * BG_AV_MAX_STATES + m_Nodes[node].State;
    if (IsGrave(node))                                      // grave-creatures are special cause of a quest
        m_ActiveEvents[node + BG_AV_NODES_MAX]  = team * BG_AV_MAX_GRAVETYPES;
}

void BattleGroundAV::DefendNode(BG_AV_Nodes node, uint32 team)
{
    MANGOS_ASSERT(team < 3);                                // alliance:0, horde:1, neutral:2
    MANGOS_ASSERT(m_Nodes[node].TotalOwner == team);
    MANGOS_ASSERT(m_Nodes[node].Owner != team);
    MANGOS_ASSERT(m_Nodes[node].State != POINT_CONTROLLED);
    m_Nodes[node].PrevOwner  = m_Nodes[node].Owner;
    m_Nodes[node].Owner      = team;
    m_Nodes[node].PrevState  = m_Nodes[node].State;
    m_Nodes[node].State      = POINT_CONTROLLED;
    m_Nodes[node].Timer      = 0;
}

void BattleGroundAV::Reset()
{
    BattleGround::Reset();
    // set the reputation and honor variables:
    bool isBGWeekend = sBattleGroundMgr.IsBGWeekend(GetTypeID());

    m_HonorMapComplete    = (isBGWeekend) ? BG_AV_KILL_MAP_COMPLETE_HOLIDAY : BG_AV_KILL_MAP_COMPLETE;
    m_RepTowerDestruction = (isBGWeekend) ? BG_AV_REP_TOWER_HOLIDAY         : BG_AV_REP_TOWER;
    m_RepCaptain          = (isBGWeekend) ? BG_AV_REP_CAPTAIN_HOLIDAY       : BG_AV_REP_CAPTAIN;
    m_RepBoss             = (isBGWeekend) ? BG_AV_REP_BOSS_HOLIDAY          : BG_AV_REP_BOSS;
    m_RepOwnedGrave       = (isBGWeekend) ? BG_AV_REP_OWNED_GRAVE_HOLIDAY   : BG_AV_REP_OWNED_GRAVE;
    m_RepSurviveCaptain   = (isBGWeekend) ? BG_AV_REP_SURVIVING_CAPTAIN_HOLIDAY : BG_AV_REP_SURVIVING_CAPTAIN;
    m_RepSurviveTower     = (isBGWeekend) ? BG_AV_REP_SURVIVING_TOWER_HOLIDAY : BG_AV_REP_SURVIVING_TOWER;
    m_RepOwnedMine        = (isBGWeekend) ? BG_AV_REP_OWNED_MINE_HOLIDAY    : BG_AV_REP_OWNED_MINE;

    for(uint8 i = 0; i < BG_TEAMS_COUNT; i++)
    {
        for(uint8 j = 0; j < 9; j++)                        // 9 quests getting tracked
            m_Team_QuestStatus[i][j] = 0;
        m_TeamScores[i]         = BG_AV_SCORE_INITIAL_POINTS;
        m_IsInformedNearLose[i] = false;
        m_ActiveEvents[BG_AV_NodeEventCaptainDead_A + i] = BG_EVENT_NONE;
    }

    for(uint8 i = 0; i < BG_AV_MAX_MINES; i++)
    {
        m_Mine_Owner[i] = BG_AV_NEUTRAL_TEAM;
        m_Mine_PrevOwner[i] = m_Mine_Owner[i];
        m_ActiveEvents[BG_AV_MINE_BOSSES+ i] = BG_AV_NEUTRAL_TEAM;
        m_ActiveEvents[BG_AV_MINE_EVENT + i] = BG_AV_NEUTRAL_TEAM;
        m_Mine_Timer[i] = BG_AV_MINE_TICK_TIMER;
    }

    m_ActiveEvents[BG_AV_CAPTAIN_A] = 0;
    m_ActiveEvents[BG_AV_CAPTAIN_H] = 0;
    m_ActiveEvents[BG_AV_HERALD] = 0;
    m_ActiveEvents[BG_AV_BOSS_A] = 0;
    m_ActiveEvents[BG_AV_BOSS_H] = 0;
    for(BG_AV_Nodes i = BG_AV_NODES_DUNBALDAR_SOUTH; i <= BG_AV_NODES_FROSTWOLF_WTOWER; ++i)   // towers
        m_ActiveEvents[BG_AV_MARSHAL_A_SOUTH + i - BG_AV_NODES_DUNBALDAR_SOUTH] = 0;

    for(BG_AV_Nodes i = BG_AV_NODES_FIRSTAID_STATION; i <= BG_AV_NODES_STONEHEART_GRAVE; ++i)   // alliance graves
        InitNode(i, BG_TEAM_ALLIANCE, false);
    for(BG_AV_Nodes i = BG_AV_NODES_DUNBALDAR_SOUTH; i <= BG_AV_NODES_STONEHEART_BUNKER; ++i)   // alliance towers
        InitNode(i, BG_TEAM_ALLIANCE, true);

    for(BG_AV_Nodes i = BG_AV_NODES_ICEBLOOD_GRAVE; i <= BG_AV_NODES_FROSTWOLF_HUT; ++i)        // horde graves
        InitNode(i, BG_TEAM_HORDE, false);
    for(BG_AV_Nodes i = BG_AV_NODES_ICEBLOOD_TOWER; i <= BG_AV_NODES_FROSTWOLF_WTOWER; ++i)     // horde towers
        InitNode(i, BG_TEAM_HORDE, true);

    InitNode(BG_AV_NODES_SNOWFALL_GRAVE, BG_AV_NEUTRAL_TEAM, false);                            // give snowfall neutral owner

}
