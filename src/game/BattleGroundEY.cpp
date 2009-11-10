/*
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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
#include "BattleGroundEY.h"
#include "Creature.h"
#include "ObjectMgr.h"
#include "BattleGroundMgr.h"
#include "Language.h"
#include "WorldPacket.h"
#include "Util.h"
#include "MapManager.h"

BattleGroundEY::BattleGroundEY()
{
    m_BuffChange = true;
    m_BgObjects.resize(BG_EY_OBJECT_MAX);

    m_Points_Trigger[FEL_REALVER] = TR_FEL_REALVER_BUFF;
    m_Points_Trigger[BLOOD_ELF] = TR_BLOOD_ELF_BUFF;
    m_Points_Trigger[DRAENEI_RUINS] = TR_DRAENEI_RUINS_BUFF;
    m_Points_Trigger[MAGE_TOWER] = TR_MAGE_TOWER_BUFF;

    m_StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_EY_START_TWO_MINUTES;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_EY_START_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_EY_START_HALF_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_EY_HAS_BEGUN;
}

BattleGroundEY::~BattleGroundEY()
{
}

void BattleGroundEY::Update(uint32 diff)
{
    BattleGround::Update(diff);

    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        m_PointAddingTimer -= diff;
        if (m_PointAddingTimer <= 0)
        {
            m_PointAddingTimer = BG_EY_FPOINTS_TICK_TIME;
            if (m_TeamPointsCount[BG_TEAM_ALLIANCE] > 0)
                AddPoints(ALLIANCE, BG_EY_TickPoints[m_TeamPointsCount[BG_TEAM_ALLIANCE] - 1]);
            if (m_TeamPointsCount[BG_TEAM_HORDE] > 0)
                AddPoints(HORDE, BG_EY_TickPoints[m_TeamPointsCount[BG_TEAM_HORDE] - 1]);
        }

        if (m_FlagState == BG_EY_FLAG_STATE_WAIT_RESPAWN || m_FlagState == BG_EY_FLAG_STATE_ON_GROUND)
        {
            m_FlagsTimer -= diff;

            if (m_FlagsTimer < 0)
            {
                m_FlagsTimer = 0;
                if (m_FlagState == BG_EY_FLAG_STATE_WAIT_RESPAWN)
                    RespawnFlag(true);
                else
                    RespawnFlagAfterDrop();
            }
        }

        m_TowerCapCheckTimer -= diff;
        if (m_TowerCapCheckTimer <= 0)
        {
            //check if player joined point
            /*I used this order of calls, because although we will check if one player is in gameobject's distance 2 times
              but we can count of players on current point in CheckSomeoneLeftPoint
            */
            CheckSomeoneJoinedPoint();
            //check if player left point
            CheckSomeoneLeftPoint();
            UpdatePointStatuses();
            m_TowerCapCheckTimer = BG_EY_FPOINTS_TICK_TIME;
        }
    }
}

void BattleGroundEY::StartingEventCloseDoors()
{
}

void BattleGroundEY::StartingEventOpenDoors()
{
    // eye-doors are despawned, not opened
    SpawnEvent(BG_EVENT_DOOR, 0, false);

    for(uint32 i = 0; i < BG_EY_NODES_MAX; ++i)
    {
        //randomly spawn buff
        uint8 buff = urand(0, 2);
        SpawnBGObject(m_BgObjects[BG_EY_OBJECT_SPEEDBUFF_FEL_REALVER + buff + i * 3], RESPAWN_IMMEDIATELY);
    }
}

void BattleGroundEY::AddPoints(uint32 Team, uint32 Points)
{
    BattleGroundTeamId team_index = GetTeamIndexByTeamId(Team);
    m_TeamScores[team_index] += Points;
    m_HonorScoreTics[team_index] += Points;
    if (m_HonorScoreTics[team_index] >= m_HonorTics )
    {
        RewardHonorToTeam(GetBonusHonorFromKill(1), Team);
        m_HonorScoreTics[team_index] -= m_HonorTics;
    }
    UpdateTeamScore(Team);
}

void BattleGroundEY::CheckSomeoneJoinedPoint()
{
    for (uint8 i = 0; i < BG_EY_NODES_MAX; ++i)
    {
        uint8 j = 0;
        while (j < m_PlayersNearPoint[BG_EY_NODES_MAX].size())
        {
            Player *plr = sObjectMgr.GetPlayer(m_PlayersNearPoint[BG_EY_NODES_MAX][j]);
            if (!plr)
            {
                sLog.outError("BattleGroundEY: Player (GUID: %u) not found!", GUID_LOPART(m_PlayersNearPoint[BG_EY_NODES_MAX][j]));
                ++j;
                continue;
            }
            if (plr->CanCaptureTowerPoint() &&
                plr->IsWithinDist3d(BG_EY_NodePositions[i][0], BG_EY_NodePositions[i][1], BG_EY_NodePositions[i][2], BG_EY_POINT_RADIUS))
            {
                //player joined point!
                //show progress bar
                UpdateWorldStateForPlayer(PROGRESS_BAR_PERCENT_GREY, BG_EY_PROGRESS_BAR_PERCENT_GREY, plr);
                UpdateWorldStateForPlayer(PROGRESS_BAR_STATUS, m_PointBarStatus[i], plr);
                UpdateWorldStateForPlayer(PROGRESS_BAR_SHOW, BG_EY_PROGRESS_BAR_SHOW, plr);
                //add player to point
                m_PlayersNearPoint[i].push_back(m_PlayersNearPoint[BG_EY_NODES_MAX][j]);
                //remove player from "free space"
                m_PlayersNearPoint[BG_EY_NODES_MAX].erase(m_PlayersNearPoint[BG_EY_NODES_MAX].begin() + j);
            }
            else
                ++j;
        }
    }
}

void BattleGroundEY::CheckSomeoneLeftPoint()
{
    //reset current point counts
    for (uint8 i = 0; i < 2*BG_EY_NODES_MAX; ++i)
        m_CurrentPointPlayersCount[i] = 0;
    for(uint8 i = 0; i < BG_EY_NODES_MAX; ++i)
    {
        uint8 j = 0;
        while (j < m_PlayersNearPoint[i].size())
        {
            Player *plr = sObjectMgr.GetPlayer(m_PlayersNearPoint[i][j]);
            if (!plr)
            {
                sLog.outError("BattleGroundEY: Player (GUID: %u) not found!", GUID_LOPART(m_PlayersNearPoint[i][j]));
                //move not existed player to "free space" - this will cause many error showing in log, but it is a very important bug
                m_PlayersNearPoint[BG_EY_NODES_MAX].push_back(m_PlayersNearPoint[i][j]);
                m_PlayersNearPoint[i].erase(m_PlayersNearPoint[i].begin() + j);
                ++j;
                continue;
            }
            if (!plr->CanCaptureTowerPoint() ||
                !plr->IsWithinDist3d(BG_EY_NodePositions[i][0], BG_EY_NodePositions[i][1], BG_EY_NodePositions[i][2], BG_EY_POINT_RADIUS))
                //move player out of point (add him to players that are out of points
            {
                m_PlayersNearPoint[BG_EY_NODES_MAX].push_back(m_PlayersNearPoint[i][j]);
                m_PlayersNearPoint[i].erase(m_PlayersNearPoint[i].begin() + j);
                UpdateWorldStateForPlayer(PROGRESS_BAR_SHOW, BG_EY_PROGRESS_BAR_DONT_SHOW, plr);
            }
            else
            {
                //player is neat flag, so update count:
                m_CurrentPointPlayersCount[2 * i + GetTeamIndexByTeamId(plr->GetTeam())]++;
                ++j;
            }
        }
    }
}

void BattleGroundEY::UpdatePointStatuses()
{
    for(uint8 point = 0; point < BG_EY_NODES_MAX; ++point)
    {
        if (m_PlayersNearPoint[point].empty())
            continue;
        //count new point bar status:
        m_PointBarStatus[point] += (m_CurrentPointPlayersCount[2 * point] - m_CurrentPointPlayersCount[2 * point + 1] < BG_EY_POINT_MAX_CAPTURERS_COUNT) ? m_CurrentPointPlayersCount[2 * point] - m_CurrentPointPlayersCount[2 * point + 1] : BG_EY_POINT_MAX_CAPTURERS_COUNT;

        if (m_PointBarStatus[point] > BG_EY_PROGRESS_BAR_ALI_CONTROLLED)
            //point is fully alliance's
            m_PointBarStatus[point] = BG_EY_PROGRESS_BAR_ALI_CONTROLLED;
        if (m_PointBarStatus[point] < BG_EY_PROGRESS_BAR_HORDE_CONTROLLED)
            //point is fully horde's
            m_PointBarStatus[point] = BG_EY_PROGRESS_BAR_HORDE_CONTROLLED;

        uint32 pointOwnerTeamId = 0;
        //find which team should own this point
        if (m_PointBarStatus[point] <= BG_EY_PROGRESS_BAR_NEUTRAL_LOW)
            pointOwnerTeamId = HORDE;
        else if (m_PointBarStatus[point] >= BG_EY_PROGRESS_BAR_NEUTRAL_HIGH)
            pointOwnerTeamId = ALLIANCE;
        else
            pointOwnerTeamId = EY_POINT_NO_OWNER;

        for (uint8 i = 0; i < m_PlayersNearPoint[point].size(); ++i)
        {
            Player *plr = sObjectMgr.GetPlayer(m_PlayersNearPoint[point][i]);
            if (plr)
            {
                UpdateWorldStateForPlayer(PROGRESS_BAR_STATUS, m_PointBarStatus[point], plr);
                                                            //if point owner changed we must evoke event!
                if (pointOwnerTeamId != m_PointOwnedByTeam[point])
                {
                    //point was uncontrolled and player is from team which captured point
                    if (m_PointState[point] == EY_POINT_STATE_UNCONTROLLED && plr->GetTeam() == pointOwnerTeamId)
                        EventTeamCapturedPoint(plr, point);

                    //point was under control and player isn't from team which controlled it
                    if (m_PointState[point] == EY_POINT_UNDER_CONTROL && plr->GetTeam() != m_PointOwnedByTeam[point])
                        EventTeamLostPoint(plr, point);
                }
            }
        }
    }
}

void BattleGroundEY::UpdateTeamScore(uint32 Team)
{
    uint32 score = GetTeamScore(Team);
    //TODO there should be some sound played when one team is near victory!! - and define variables
    /*if (!m_IsInformedNearVictory && score >= BG_EY_WARNING_NEAR_VICTORY_SCORE)
    {
        if (Team == ALLIANCE)
            SendMessageToAll(LANG_BG_EY_A_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
        else
            SendMessageToAll(LANG_BG_EY_H_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
        PlaySoundToAll(BG_EY_SOUND_NEAR_VICTORY);
        m_IsInformedNearVictory = true;
    }*/

    if (score >= BG_EY_MAX_TEAM_SCORE)
    {
        score = BG_EY_MAX_TEAM_SCORE;
        EndBattleGround(Team);
    }

    if (Team == ALLIANCE)
        UpdateWorldState(EY_ALLIANCE_RESOURCES, score);
    else
        UpdateWorldState(EY_HORDE_RESOURCES, score);
}

void BattleGroundEY::EndBattleGround(uint32 winner)
{
    //win reward
    if (winner == ALLIANCE)
        RewardHonorToTeam(GetBonusHonorFromKill(1), ALLIANCE);
    if (winner == HORDE)
        RewardHonorToTeam(GetBonusHonorFromKill(1), HORDE);
    //complete map reward
    RewardHonorToTeam(GetBonusHonorFromKill(1), ALLIANCE);
    RewardHonorToTeam(GetBonusHonorFromKill(1), HORDE);

    BattleGround::EndBattleGround(winner);
}

void BattleGroundEY::UpdatePointsCount(uint32 Team)
{
    if (Team == ALLIANCE)
        UpdateWorldState(EY_ALLIANCE_BASE, m_TeamPointsCount[BG_TEAM_ALLIANCE]);
    else
        UpdateWorldState(EY_HORDE_BASE, m_TeamPointsCount[BG_TEAM_HORDE]);
}

void BattleGroundEY::UpdatePointsIcons(uint32 Team, uint32 Point)
{
    //we MUST firstly send 0, after that we can send 1!!!
    if (m_PointState[Point] == EY_POINT_UNDER_CONTROL)
    {
        UpdateWorldState(PointsIconStruct[Point].WorldStateControlIndex, 0);
        if (Team == ALLIANCE)
            UpdateWorldState(PointsIconStruct[Point].WorldStateAllianceControlledIndex, 1);
        else
            UpdateWorldState(PointsIconStruct[Point].WorldStateHordeControlledIndex, 1);
    }
    else
    {
        if (Team == ALLIANCE)
            UpdateWorldState(PointsIconStruct[Point].WorldStateAllianceControlledIndex, 0);
        else
            UpdateWorldState(PointsIconStruct[Point].WorldStateHordeControlledIndex, 0);
        UpdateWorldState(PointsIconStruct[Point].WorldStateControlIndex, 1);
    }
}

void BattleGroundEY::AddPlayer(Player *plr)
{
    BattleGround::AddPlayer(plr);
    //create score and add it to map
    BattleGroundEYScore* sc = new BattleGroundEYScore;

    m_PlayersNearPoint[BG_EY_NODES_MAX].push_back(plr->GetGUID());

    m_PlayerScores[plr->GetGUID()] = sc;
}

void BattleGroundEY::RemovePlayer(Player *plr, uint64 guid)
{
    // sometimes flag aura not removed :(
    for (int j = BG_EY_NODES_MAX; j >= 0; --j)
    {
        for(size_t i = 0; i < m_PlayersNearPoint[j].size(); ++i)
            if (m_PlayersNearPoint[j][i] == guid)
                m_PlayersNearPoint[j].erase(m_PlayersNearPoint[j].begin() + i);
    }
    if (IsFlagPickedup())
    {
        if (m_FlagKeeper == guid)
        {
            if (plr)
                EventPlayerDroppedFlag(plr);
            else
            {
                SetFlagPicker(0);
                RespawnFlag(true);
            }
        }
    }
}

void BattleGroundEY::HandleAreaTrigger(Player *Source, uint32 Trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if(!Source->isAlive())                                  //hack code, must be removed later
        return;

    switch(Trigger)
    {
        case TR_BLOOD_ELF_POINT:
            if (m_PointState[BLOOD_ELF] == EY_POINT_UNDER_CONTROL && m_PointOwnedByTeam[BLOOD_ELF] == Source->GetTeam())
                if (m_FlagState && GetFlagPickerGUID() == Source->GetGUID())
                    EventPlayerCapturedFlag(Source, BG_EY_NODE_BLOOD_ELF);
            break;
        case TR_FEL_REALVER_POINT:
            if (m_PointState[FEL_REALVER] == EY_POINT_UNDER_CONTROL && m_PointOwnedByTeam[FEL_REALVER] == Source->GetTeam())
                if (m_FlagState && GetFlagPickerGUID() == Source->GetGUID())
                    EventPlayerCapturedFlag(Source, BG_EY_NODE_FEL_REALVER);
            break;
        case TR_MAGE_TOWER_POINT:
            if (m_PointState[MAGE_TOWER] == EY_POINT_UNDER_CONTROL && m_PointOwnedByTeam[MAGE_TOWER] == Source->GetTeam())
                if (m_FlagState && GetFlagPickerGUID() == Source->GetGUID())
                    EventPlayerCapturedFlag(Source, BG_EY_NODE_MAGE_TOWER);
            break;
        case TR_DRAENEI_RUINS_POINT:
            if (m_PointState[DRAENEI_RUINS] == EY_POINT_UNDER_CONTROL && m_PointOwnedByTeam[DRAENEI_RUINS] == Source->GetTeam())
                if (m_FlagState && GetFlagPickerGUID() == Source->GetGUID())
                    EventPlayerCapturedFlag(Source, BG_EY_NODE_DRAENEI_RUINS);
            break;
        case 4512:
        case 4515:
        case 4517:
        case 4519:
        case 4530:
        case 4531:
        case 4568:
        case 4569:
        case 4570:
        case 4571:
            break;
        default:
            sLog.outError("WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            Source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }
}

bool BattleGroundEY::SetupBattleGround()
{
    //buffs
    for (int i = 0; i < BG_EY_NODES_MAX; ++i)
    {
        AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(m_Points_Trigger[i]);
        if (!at)
        {
            sLog.outError("BattleGroundEY: Unknown trigger: %u", m_Points_Trigger[i]);
            continue;
        }
        if (!AddObject(BG_EY_OBJECT_SPEEDBUFF_FEL_REALVER + i * 3, Buff_Entries[0], at->x, at->y, at->z, 0.907571f, 0, 0, 0.438371f, 0.898794f, RESPAWN_ONE_DAY)
            || !AddObject(BG_EY_OBJECT_SPEEDBUFF_FEL_REALVER + i * 3 + 1, Buff_Entries[1], at->x, at->y, at->z, 0.907571f, 0, 0, 0.438371f, 0.898794f, RESPAWN_ONE_DAY)
            || !AddObject(BG_EY_OBJECT_SPEEDBUFF_FEL_REALVER + i * 3 + 2, Buff_Entries[2], at->x, at->y, at->z, 0.907571f, 0, 0, 0.438371f, 0.898794f, RESPAWN_ONE_DAY)
            )
            sLog.outError("BattleGroundEY: Cannot spawn buff");
    }

    return true;
}

void BattleGroundEY::Reset()
{
    //call parent's class reset
    BattleGround::Reset();

    m_TeamScores[BG_TEAM_ALLIANCE] = 0;
    m_TeamScores[BG_TEAM_HORDE] = 0;
    m_TeamPointsCount[BG_TEAM_ALLIANCE] = 0;
    m_TeamPointsCount[BG_TEAM_HORDE] = 0;
    m_HonorScoreTics[BG_TEAM_ALLIANCE] = 0;
    m_HonorScoreTics[BG_TEAM_HORDE] = 0;
    m_FlagState = BG_EY_FLAG_STATE_ON_BASE;
    m_FlagKeeper = 0;
    m_DroppedFlagGUID = 0;
    m_PointAddingTimer = 0;
    m_TowerCapCheckTimer = 0;
    bool isBGWeekend = BattleGroundMgr::IsBGWeekend(GetTypeID());
    m_HonorTics = (isBGWeekend) ? BG_EY_EYWeekendHonorTicks : BG_EY_NotEYWeekendHonorTicks;

    for(uint8 i = 0; i < BG_EY_NODES_MAX; ++i)
    {
        m_PointOwnedByTeam[i] = EY_POINT_NO_OWNER;
        m_PointState[i] = EY_POINT_STATE_UNCONTROLLED;
        m_PointBarStatus[i] = BG_EY_PROGRESS_BAR_STATE_MIDDLE;
        m_PlayersNearPoint[i].clear();
        m_PlayersNearPoint[i].reserve(15);                  //tip size
        m_ActiveEvents[i] = BG_EYE_NEUTRAL_TEAM;            // neutral team owns every node
    }
    // the flag in the middle is spawned at beginning
    m_ActiveEvents[BG_EY_EVENT_CAPTURE_FLAG] = BG_EY_EVENT2_FLAG_CENTER;

    m_PlayersNearPoint[EY_PLAYERS_OUT_OF_POINTS].clear();
    m_PlayersNearPoint[EY_PLAYERS_OUT_OF_POINTS].reserve(30);
}

void BattleGroundEY::RespawnFlag(bool send_message)
{
    m_FlagState = BG_EY_FLAG_STATE_ON_BASE;
    // will despawn captured flags at the node and spawn in center
    SpawnEvent(BG_EY_EVENT_CAPTURE_FLAG, BG_EY_EVENT2_FLAG_CENTER, true);

    if (send_message)
    {
        SendMessageToAll(LANG_BG_EY_RESETED_FLAG, CHAT_MSG_BG_SYSTEM_NEUTRAL);
        PlaySoundToAll(BG_EY_SOUND_FLAG_RESET);             // flags respawned sound...
    }

    UpdateWorldState(NETHERSTORM_FLAG, 1);
}

void BattleGroundEY::RespawnFlagAfterDrop()
{
    RespawnFlag(true);

    GameObject *obj = GetBgMap()->GetGameObject(GetDroppedFlagGUID());
    if (obj)
        obj->Delete();
    else
        sLog.outError("BattleGroundEY: Unknown dropped flag guid: %u",GUID_LOPART(GetDroppedFlagGUID()));

    SetDroppedFlagGUID(0);
}

void BattleGroundEY::HandleKillPlayer(Player *player, Player *killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    BattleGround::HandleKillPlayer(player, killer);
    EventPlayerDroppedFlag(player);
}

void BattleGroundEY::EventPlayerDroppedFlag(Player *Source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
    {
        // if not running, do not cast things at the dropper player, neither send unnecessary messages
        // just take off the aura
        if (IsFlagPickedup() && GetFlagPickerGUID() == Source->GetGUID())
        {
            SetFlagPicker(0);
            Source->RemoveAurasDueToSpell(BG_EY_NETHERSTORM_FLAG_SPELL);
        }
        return;
    }

    if (!IsFlagPickedup())
        return;

    if (GetFlagPickerGUID() != Source->GetGUID())
        return;

    SetFlagPicker(0);
    Source->RemoveAurasDueToSpell(BG_EY_NETHERSTORM_FLAG_SPELL);
    m_FlagState = BG_EY_FLAG_STATE_ON_GROUND;
    m_FlagsTimer = BG_EY_FLAG_RESPAWN_TIME;
    Source->CastSpell(Source, SPELL_RECENTLY_DROPPED_FLAG, true);
    Source->CastSpell(Source, BG_EY_PLAYER_DROPPED_FLAG_SPELL, true);
    //this does not work correctly :( (it should remove flag carrier name)
    UpdateWorldState(NETHERSTORM_FLAG_STATE_HORDE, BG_EY_FLAG_STATE_WAIT_RESPAWN);
    UpdateWorldState(NETHERSTORM_FLAG_STATE_ALLIANCE, BG_EY_FLAG_STATE_WAIT_RESPAWN);

    if (Source->GetTeam() == ALLIANCE)
        SendMessageToAll(LANG_BG_EY_DROPPED_FLAG, CHAT_MSG_BG_SYSTEM_ALLIANCE, NULL);
    else
        SendMessageToAll(LANG_BG_EY_DROPPED_FLAG, CHAT_MSG_BG_SYSTEM_HORDE, NULL);
}

void BattleGroundEY::EventPlayerClickedOnFlag(Player *Source, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS || IsFlagPickedup() || !Source->IsWithinDistInMap(target_obj, 10))
        return;

    if (Source->GetTeam() == ALLIANCE)
    {
        UpdateWorldState(NETHERSTORM_FLAG_STATE_ALLIANCE, BG_EY_FLAG_STATE_ON_PLAYER);
        PlaySoundToAll(BG_EY_SOUND_FLAG_PICKED_UP_ALLIANCE);
    }
    else
    {
        UpdateWorldState(NETHERSTORM_FLAG_STATE_HORDE, BG_EY_FLAG_STATE_ON_PLAYER);
        PlaySoundToAll(BG_EY_SOUND_FLAG_PICKED_UP_HORDE);
    }

    if (m_FlagState == BG_EY_FLAG_STATE_ON_BASE)
        UpdateWorldState(NETHERSTORM_FLAG, 0);
    m_FlagState = BG_EY_FLAG_STATE_ON_PLAYER;

    // despawn center-flag
    SpawnEvent(BG_EY_EVENT_CAPTURE_FLAG, BG_EY_EVENT2_FLAG_CENTER, false);

    SetFlagPicker(Source->GetGUID());
    //get flag aura on player
    Source->CastSpell(Source, BG_EY_NETHERSTORM_FLAG_SPELL, true);
    Source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);

    if (Source->GetTeam() == ALLIANCE)
        PSendMessageToAll(LANG_BG_EY_HAS_TAKEN_FLAG, CHAT_MSG_BG_SYSTEM_ALLIANCE, NULL, Source->GetName());
    else
        PSendMessageToAll(LANG_BG_EY_HAS_TAKEN_FLAG, CHAT_MSG_BG_SYSTEM_HORDE, NULL, Source->GetName());
}

void BattleGroundEY::EventTeamLostPoint(Player *Source, uint32 Point)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    // neutral node
    uint32 Team = m_PointOwnedByTeam[Point];

    if (!Team)
        return;

    if (Team == ALLIANCE)
        m_TeamPointsCount[BG_TEAM_ALLIANCE]--;
    else
        m_TeamPointsCount[BG_TEAM_HORDE]--;

    // it's important to set the OwnedBy before despawning spiritguides, else
    // player won't get teleported away
    m_PointOwnedByTeam[Point] = EY_POINT_NO_OWNER;
    m_PointState[Point] = EY_POINT_NO_OWNER;

    SpawnEvent(Point, BG_EYE_NEUTRAL_TEAM, true);           // will despawn alliance/horde

    //buff isn't despawned

    if (Team == ALLIANCE)
        SendMessageToAll(LoosingPointTypes[Point].MessageIdAlliance,CHAT_MSG_BG_SYSTEM_ALLIANCE, Source);
    else
        SendMessageToAll(LoosingPointTypes[Point].MessageIdHorde,CHAT_MSG_BG_SYSTEM_HORDE, Source);

    UpdatePointsIcons(Team, Point);
    UpdatePointsCount(Team);
}

void BattleGroundEY::EventTeamCapturedPoint(Player *Source, uint32 Point)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint32 Team = Source->GetTeam();

    m_TeamPointsCount[GetTeamIndexByTeamId(Team)]++;
    SpawnEvent(Point, GetTeamIndexByTeamId(Team), true);

    //buff isn't respawned

    m_PointOwnedByTeam[Point] = Team;
    m_PointState[Point] = EY_POINT_UNDER_CONTROL;

    if (Team == ALLIANCE)
        SendMessageToAll(CapturingPointTypes[Point].MessageIdAlliance,CHAT_MSG_BG_SYSTEM_ALLIANCE, Source);
    else
        SendMessageToAll(CapturingPointTypes[Point].MessageIdHorde,CHAT_MSG_BG_SYSTEM_HORDE, Source);

    UpdatePointsIcons(Team, Point);
    UpdatePointsCount(Team);
}

void BattleGroundEY::EventPlayerCapturedFlag(Player *Source, BG_EY_Nodes node)
{
    if (GetStatus() != STATUS_IN_PROGRESS || GetFlagPickerGUID() != Source->GetGUID())
        return;

    SetFlagPicker(0);
    m_FlagState = BG_EY_FLAG_STATE_WAIT_RESPAWN;
    Source->RemoveAurasDueToSpell(BG_EY_NETHERSTORM_FLAG_SPELL);

    Source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);

    if (Source->GetTeam() == ALLIANCE)
        PlaySoundToAll(BG_EY_SOUND_FLAG_CAPTURED_ALLIANCE);
    else
        PlaySoundToAll(BG_EY_SOUND_FLAG_CAPTURED_HORDE);

    SpawnEvent(BG_EY_EVENT_CAPTURE_FLAG, node, true);

    m_FlagsTimer = BG_EY_FLAG_RESPAWN_TIME;

    uint8 team_id = 0;
    if (Source->GetTeam() == ALLIANCE)
    {
        team_id = BG_TEAM_ALLIANCE;
        SendMessageToAll(LANG_BG_EY_CAPTURED_FLAG_A, CHAT_MSG_BG_SYSTEM_ALLIANCE, Source);
    }
    else
    {
        team_id = BG_TEAM_HORDE;
        SendMessageToAll(LANG_BG_EY_CAPTURED_FLAG_H, CHAT_MSG_BG_SYSTEM_HORDE, Source);
    }

    if (m_TeamPointsCount[team_id] > 0)
        AddPoints(Source->GetTeam(), BG_EY_FlagPoints[m_TeamPointsCount[team_id] - 1]);

    UpdatePlayerScore(Source, SCORE_FLAG_CAPTURES, 1);
}

void BattleGroundEY::UpdatePlayerScore(Player *Source, uint32 type, uint32 value)
{
    BattleGroundScoreMap::iterator itr = m_PlayerScores.find(Source->GetGUID());
    if(itr == m_PlayerScores.end())                         // player not found
        return;

    switch(type)
    {
        case SCORE_FLAG_CAPTURES:                           // flags captured
            ((BattleGroundEYScore*)itr->second)->FlagCaptures += value;
            break;
        default:
            BattleGround::UpdatePlayerScore(Source, type, value);
            break;
    }
}

void BattleGroundEY::FillInitialWorldStates(WorldPacket& data)
{
    data << uint32(EY_HORDE_BASE) << uint32(m_TeamPointsCount[BG_TEAM_HORDE]);
    data << uint32(EY_ALLIANCE_BASE) << uint32(m_TeamPointsCount[BG_TEAM_ALLIANCE]);
    data << uint32(0xab6) << uint32(0x0);
    data << uint32(0xab5) << uint32(0x0);
    data << uint32(0xab4) << uint32(0x0);
    data << uint32(0xab3) << uint32(0x0);
    data << uint32(0xab2) << uint32(0x0);
    data << uint32(0xab1) << uint32(0x0);
    data << uint32(0xab0) << uint32(0x0);
    data << uint32(0xaaf) << uint32(0x0);

    data << uint32(DRAENEI_RUINS_HORDE_CONTROL)     << uint32(m_PointOwnedByTeam[DRAENEI_RUINS] == HORDE && m_PointState[DRAENEI_RUINS] == EY_POINT_UNDER_CONTROL);

    data << uint32(DRAENEI_RUINS_ALLIANCE_CONTROL)  << uint32(m_PointOwnedByTeam[DRAENEI_RUINS] == ALLIANCE && m_PointState[DRAENEI_RUINS] == EY_POINT_UNDER_CONTROL);

    data << uint32(DRAENEI_RUINS_UNCONTROL)         << uint32(m_PointState[DRAENEI_RUINS] != EY_POINT_UNDER_CONTROL);

    data << uint32(MAGE_TOWER_ALLIANCE_CONTROL)     << uint32(m_PointOwnedByTeam[MAGE_TOWER] == ALLIANCE && m_PointState[MAGE_TOWER] == EY_POINT_UNDER_CONTROL);

    data << uint32(MAGE_TOWER_HORDE_CONTROL)        << uint32(m_PointOwnedByTeam[MAGE_TOWER] == HORDE && m_PointState[MAGE_TOWER] == EY_POINT_UNDER_CONTROL);

    data << uint32(MAGE_TOWER_UNCONTROL)            << uint32(m_PointState[MAGE_TOWER] != EY_POINT_UNDER_CONTROL);

    data << uint32(FEL_REAVER_HORDE_CONTROL)        << uint32(m_PointOwnedByTeam[FEL_REALVER] == HORDE && m_PointState[FEL_REALVER] == EY_POINT_UNDER_CONTROL);

    data << uint32(FEL_REAVER_ALLIANCE_CONTROL)     << uint32(m_PointOwnedByTeam[FEL_REALVER] == ALLIANCE && m_PointState[FEL_REALVER] == EY_POINT_UNDER_CONTROL);

    data << uint32(FEL_REAVER_UNCONTROL)            << uint32(m_PointState[FEL_REALVER] != EY_POINT_UNDER_CONTROL);

    data << uint32(BLOOD_ELF_HORDE_CONTROL)         << uint32(m_PointOwnedByTeam[BLOOD_ELF] == HORDE && m_PointState[BLOOD_ELF] == EY_POINT_UNDER_CONTROL);

    data << uint32(BLOOD_ELF_ALLIANCE_CONTROL)      << uint32(m_PointOwnedByTeam[BLOOD_ELF] == ALLIANCE && m_PointState[BLOOD_ELF] == EY_POINT_UNDER_CONTROL);

    data << uint32(BLOOD_ELF_UNCONTROL)             << uint32(m_PointState[BLOOD_ELF] != EY_POINT_UNDER_CONTROL);

    data << uint32(NETHERSTORM_FLAG)                << uint32(m_FlagState == BG_EY_FLAG_STATE_ON_BASE);

    data << uint32(0xad2) << uint32(0x1);
    data << uint32(0xad1) << uint32(0x1);
    data << uint32(0xabe) << uint32(GetTeamScore(HORDE));
    data << uint32(0xabd) << uint32(GetTeamScore(ALLIANCE));
    data << uint32(0xa05) << uint32(0x8e);
    data << uint32(0xaa0) << uint32(0x0);
    data << uint32(0xa9f) << uint32(0x0);
    data << uint32(0xa9e) << uint32(0x0);
    data << uint32(0xc0d) << uint32(0x17b);
}

WorldSafeLocsEntry const *BattleGroundEY::GetClosestGraveYard(Player* player)
{
    uint32 g_id = 0;

    switch(player->GetTeam())
    {
        case ALLIANCE: g_id = EY_GRAVEYARD_MAIN_ALLIANCE; break;
        case HORDE:    g_id = EY_GRAVEYARD_MAIN_HORDE;    break;
        default:       return NULL;
    }

    float distance, nearestDistance;

    WorldSafeLocsEntry const* entry = NULL;
    WorldSafeLocsEntry const* nearestEntry = NULL;
    entry = sWorldSafeLocsStore.LookupEntry(g_id);
    nearestEntry = entry;

    if (!entry)
    {
        sLog.outError("BattleGroundEY: Not found the main team graveyard. Graveyard system isn't working!");
        return NULL;
    }

    float plr_x = player->GetPositionX();
    float plr_y = player->GetPositionY();
    float plr_z = player->GetPositionZ();


    distance = (entry->x - plr_x)*(entry->x - plr_x) + (entry->y - plr_y)*(entry->y - plr_y) + (entry->z - plr_z)*(entry->z - plr_z);
    nearestDistance = distance;

    for(uint8 i = 0; i < BG_EY_NODES_MAX; ++i)
    {
        if (m_PointOwnedByTeam[i]==player->GetTeam() && m_PointState[i]==EY_POINT_UNDER_CONTROL)
        {
            entry = sWorldSafeLocsStore.LookupEntry(CapturingPointTypes[i].GraveYardId);
            if (!entry)
                sLog.outError("BattleGroundEY: Not found graveyard: %u",CapturingPointTypes[i].GraveYardId);
            else
            {
                distance = (entry->x - plr_x)*(entry->x - plr_x) + (entry->y - plr_y)*(entry->y - plr_y) + (entry->z - plr_z)*(entry->z - plr_z);
                if (distance < nearestDistance)
                {
                    nearestDistance = distance;
                    nearestEntry = entry;
                }
            }
        }
    }

    return nearestEntry;
}

bool BattleGroundEY::IsAllNodesConrolledByTeam(uint32 team) const
{
    uint32 count = 0;
    for(int i = 0; i < BG_EY_NODES_MAX; ++i)
        if (m_PointOwnedByTeam[i] == team && m_PointState[i] == EY_POINT_UNDER_CONTROL)
            ++count;

    return count == BG_EY_NODES_MAX;
}
