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
#include "SharedDefines.h"
#include "Player.h"
#include "BattleGroundMgr.h"
#include "BattleGroundAV.h"
#include "BattleGroundAB.h"
#include "BattleGroundEY.h"
#include "BattleGroundWS.h"
#include "BattleGroundNA.h"
#include "BattleGroundBE.h"
#include "BattleGroundAA.h"
#include "BattleGroundRL.h"
#include "BattleGroundSA.h"
#include "BattleGroundDS.h"
#include "BattleGroundRV.h"
#include "BattleGroundIC.h"
#include "BattleGroundRB.h"
#include "MapManager.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "ProgressBar.h"
#include "Chat.h"
#include "ArenaTeam.h"
#include "World.h"
#include "WorldPacket.h"
#include "GameEventMgr.h"

#include "Policies/SingletonImp.h"

INSTANTIATE_SINGLETON_1(BattleGroundMgr);

/*********************************************************/
/***            BATTLEGROUND QUEUE SYSTEM              ***/
/*********************************************************/

BattleGroundQueue::BattleGroundQueue()
{
    for (uint8 i = 0; i < BG_TEAMS_COUNT; ++i)
    {
        for (uint8 j = 0; j < MAX_BATTLEGROUND_BRACKETS; ++j)
        {
            m_SumOfWaitTimes[i][j] = 0;
            m_WaitTimeLastPlayer[i][j] = 0;
            for (uint8 k = 0; k < COUNT_OF_PLAYERS_TO_AVERAGE_WAIT_TIME; ++k)
                m_WaitTimes[i][j][k] = 0;
        }
    }
}

BattleGroundQueue::~BattleGroundQueue()
{
    m_QueuedPlayers.clear();
    for (uint8 i = 0; i < MAX_BATTLEGROUND_BRACKETS; ++i)
    {
        for (uint8 j = 0; j < BG_QUEUE_GROUP_TYPES_COUNT; ++j)
        {
            for (GroupsQueueType::iterator itr = m_QueuedGroups[i][j].begin(); itr != m_QueuedGroups[i][j].end(); ++itr)
                delete(*itr);
            m_QueuedGroups[i][j].clear();
        }
    }
}

/*********************************************************/
/***      BATTLEGROUND QUEUE SELECTION POOLS           ***/
/*********************************************************/

// selection pool initialization, used to clean up from prev selection
void BattleGroundQueue::SelectionPool::Init()
{
    SelectedGroups.clear();
    PlayerCount = 0;
}

// remove group info from selection pool
// returns true when we need to try to add new group to selection pool
// returns false when selection pool is ok or when we kicked smaller group than we need to kick
// sometimes it can be called on empty selection pool
bool BattleGroundQueue::SelectionPool::KickGroup(uint32 size)
{
    // find maxgroup or LAST group with size == size and kick it
    bool found = false;
    GroupsQueueType::iterator groupToKick = SelectedGroups.begin();
    for (GroupsQueueType::iterator itr = groupToKick; itr != SelectedGroups.end(); ++itr)
    {
        if (abs((int32)((*itr)->Players.size() - size)) <= 1)
        {
            groupToKick = itr;
            found = true;
        }
        else if (!found && (*itr)->Players.size() >= (*groupToKick)->Players.size())
            groupToKick = itr;
    }
    // if pool is empty, do nothing
    if (GetPlayerCount())
    {
        // update player count
        GroupQueueInfo* ginfo = (*groupToKick);
        SelectedGroups.erase(groupToKick);
        PlayerCount -= ginfo->Players.size();
        // return false if we kicked smaller group or there are enough players in selection pool
        if (ginfo->Players.size() <= size + 1)
            return false;
    }
    return true;
}

// add group to selection pool
// used when building selection pools
// returns true if we can invite more players, or when we added group to selection pool
// returns false when selection pool is full
bool BattleGroundQueue::SelectionPool::AddGroup(GroupQueueInfo* ginfo, uint32 desiredCount)
{
    // if group is larger than desired count - don't allow to add it to pool
    if (!ginfo->IsInvitedToBGInstanceGUID && desiredCount >= PlayerCount + ginfo->Players.size())
    {
        SelectedGroups.push_back(ginfo);
        // increase selected players count
        PlayerCount += ginfo->Players.size();
        return true;
    }
    if (PlayerCount < desiredCount)
        return true;
    return false;
}

/*********************************************************/
/***               BATTLEGROUND QUEUES                 ***/
/*********************************************************/

// add group or player (grp == NULL) to bg queue with the given leader and bg specifications
GroupQueueInfo* BattleGroundQueue::AddGroup(Player* leader, Group* grp, BattleGroundTypeId BgTypeId, PvPDifficultyEntry const*  bracketEntry, ArenaType arenaType, bool isRated, bool isPremade, uint32 arenaRating, uint32 arenateamid)
{
    BattleGroundBracketId bracketId =  bracketEntry->GetBracketId();

    // create new ginfo
    GroupQueueInfo* ginfo = new GroupQueueInfo;
    ginfo->BgTypeId                  = BgTypeId;
    ginfo->arenaType                 = arenaType;
    ginfo->ArenaTeamId               = arenateamid;
    ginfo->IsRated                   = isRated;
    ginfo->IsInvitedToBGInstanceGUID = 0;
    ginfo->JoinTime                  = WorldTimer::getMSTime();
    ginfo->RemoveInviteTime          = 0;
    ginfo->GroupTeam                 = leader->GetTeam();
    ginfo->ArenaTeamRating           = arenaRating;
    ginfo->OpponentsTeamRating       = 0;

    ginfo->Players.clear();

    // compute index (if group is premade or joined a rated match) to queues
    uint32 index = 0;
    if (!isRated && !isPremade)
        index += BG_TEAMS_COUNT;                            // BG_QUEUE_PREMADE_* -> BG_QUEUE_NORMAL_*

    if (ginfo->GroupTeam == HORDE)
        ++index;                                            // BG_QUEUE_*_ALLIANCE -> BG_QUEUE_*_HORDE

    DEBUG_LOG("Adding Group to BattleGroundQueue bgTypeId : %u, bracket_id : %u, index : %u", BgTypeId, bracketId, index);

    uint32 lastOnlineTime = WorldTimer::getMSTime();

    // announce world (this don't need mutex)
    if (isRated && sWorld.getConfig(CONFIG_BOOL_ARENA_QUEUE_ANNOUNCER_JOIN))
    {
        sWorld.SendWorldText(LANG_ARENA_QUEUE_ANNOUNCE_WORLD_JOIN, ginfo->arenaType, ginfo->arenaType, ginfo->ArenaTeamRating);
    }

    // add players from group to ginfo
    {
        // ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_Lock);
        if (grp)
        {
            for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* member = itr->getSource();
                if (!member)
                    continue;   // this should never happen
                PlayerQueueInfo& pl_info = m_QueuedPlayers[member->GetObjectGuid()];
                pl_info.LastOnlineTime   = lastOnlineTime;
                pl_info.GroupInfo        = ginfo;
                // add the pinfo to ginfo's list
                ginfo->Players[member->GetObjectGuid()]  = &pl_info;
            }
        }
        else
        {
            PlayerQueueInfo& pl_info = m_QueuedPlayers[leader->GetObjectGuid()];
            pl_info.LastOnlineTime   = lastOnlineTime;
            pl_info.GroupInfo        = ginfo;
            ginfo->Players[leader->GetObjectGuid()]  = &pl_info;
        }

        // add GroupInfo to m_QueuedGroups
        m_QueuedGroups[bracketId][index].push_back(ginfo);

        // announce to world, this code needs mutex
        if (arenaType == ARENA_TYPE_NONE && !isRated && !isPremade && sWorld.getConfig(CONFIG_UINT32_BATTLEGROUND_QUEUE_ANNOUNCER_JOIN))
        {
            if (BattleGround* bg = sBattleGroundMgr.GetBattleGroundTemplate(ginfo->BgTypeId))
            {
                char const* bgName = bg->GetName();
                uint32 MinPlayers = bg->GetMinPlayersPerTeam();
                uint32 qHorde = 0;
                uint32 qAlliance = 0;
                uint32 q_min_level = bracketEntry->minLevel;
                uint32 q_max_level = bracketEntry->maxLevel;
                GroupsQueueType::const_iterator itr;
                for (itr = m_QueuedGroups[bracketId][BG_QUEUE_NORMAL_ALLIANCE].begin(); itr != m_QueuedGroups[bracketId][BG_QUEUE_NORMAL_ALLIANCE].end(); ++itr)
                    if (!(*itr)->IsInvitedToBGInstanceGUID)
                        qAlliance += (*itr)->Players.size();
                for (itr = m_QueuedGroups[bracketId][BG_QUEUE_NORMAL_HORDE].begin(); itr != m_QueuedGroups[bracketId][BG_QUEUE_NORMAL_HORDE].end(); ++itr)
                    if (!(*itr)->IsInvitedToBGInstanceGUID)
                        qHorde += (*itr)->Players.size();

                // Show queue status to player only (when joining queue)
                if (sWorld.getConfig(CONFIG_UINT32_BATTLEGROUND_QUEUE_ANNOUNCER_JOIN) == 1)
                {
                    ChatHandler(leader).PSendSysMessage(LANG_BG_QUEUE_ANNOUNCE_SELF, bgName, q_min_level, q_max_level,
                                                        qAlliance, (MinPlayers > qAlliance) ? MinPlayers - qAlliance : (uint32)0, qHorde, (MinPlayers > qHorde) ? MinPlayers - qHorde : (uint32)0);
                }
                // System message
                else
                {
                    sWorld.SendWorldText(LANG_BG_QUEUE_ANNOUNCE_WORLD, bgName, q_min_level, q_max_level,
                                         qAlliance, (MinPlayers > qAlliance) ? MinPlayers - qAlliance : (uint32)0, qHorde, (MinPlayers > qHorde) ? MinPlayers - qHorde : (uint32)0);
                }
            }
        }
        // release mutex
    }

    return ginfo;
}

void BattleGroundQueue::PlayerInvitedToBGUpdateAverageWaitTime(GroupQueueInfo* ginfo, BattleGroundBracketId bracket_id)
{
    uint32 timeInQueue = WorldTimer::getMSTimeDiff(ginfo->JoinTime, WorldTimer::getMSTime());
    uint8 team_index = BG_TEAM_ALLIANCE;                    // default set to BG_TEAM_ALLIANCE - or non rated arenas!
    if (ginfo->arenaType == ARENA_TYPE_NONE)
    {
        if (ginfo->GroupTeam == HORDE)
            team_index = BG_TEAM_HORDE;
    }
    else
    {
        if (ginfo->IsRated)
            team_index = BG_TEAM_HORDE;                     // for rated arenas use BG_TEAM_HORDE
    }

    // store pointer to arrayindex of player that was added first
    uint32* lastPlayerAddedPointer = &(m_WaitTimeLastPlayer[team_index][bracket_id]);
    // remove his time from sum
    m_SumOfWaitTimes[team_index][bracket_id] -= m_WaitTimes[team_index][bracket_id][(*lastPlayerAddedPointer)];
    // set average time to new
    m_WaitTimes[team_index][bracket_id][(*lastPlayerAddedPointer)] = timeInQueue;
    // add new time to sum
    m_SumOfWaitTimes[team_index][bracket_id] += timeInQueue;
    // set index of last player added to next one
    (*lastPlayerAddedPointer)++;
    (*lastPlayerAddedPointer) %= COUNT_OF_PLAYERS_TO_AVERAGE_WAIT_TIME;
}

uint32 BattleGroundQueue::GetAverageQueueWaitTime(GroupQueueInfo* ginfo, BattleGroundBracketId bracket_id)
{
    uint8 team_index = BG_TEAM_ALLIANCE;                    // default set to BG_TEAM_ALLIANCE - or non rated arenas!
    if (ginfo->arenaType == ARENA_TYPE_NONE)
    {
        if (ginfo->GroupTeam == HORDE)
            team_index = BG_TEAM_HORDE;
    }
    else
    {
        if (ginfo->IsRated)
            team_index = BG_TEAM_HORDE;                     // for rated arenas use BG_TEAM_HORDE
    }
    // check if there is enought values(we always add values > 0)
    if (m_WaitTimes[team_index][bracket_id][COUNT_OF_PLAYERS_TO_AVERAGE_WAIT_TIME - 1])
        return (m_SumOfWaitTimes[team_index][bracket_id] / COUNT_OF_PLAYERS_TO_AVERAGE_WAIT_TIME);
    else
        // if there aren't enough values return 0 - not available
        return 0;
}

// remove player from queue and from group info, if group info is empty then remove it too
void BattleGroundQueue::RemovePlayer(ObjectGuid guid, bool decreaseInvitedCount)
{
    // Player *plr = sObjectMgr.GetPlayer(guid);
    // ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_Lock);

    int32 bracket_id = -1;                                  // signed for proper for-loop finish
    QueuedPlayersMap::iterator itr;

    // remove player from map, if he's there
    itr = m_QueuedPlayers.find(guid);
    if (itr == m_QueuedPlayers.end())
    {
        sLog.outError("BattleGroundQueue: couldn't find for remove: %s", guid.GetString().c_str());
        return;
    }

    GroupQueueInfo* group = itr->second.GroupInfo;
    GroupsQueueType::iterator group_itr, group_itr_tmp;
    // mostly people with the highest levels are in battlegrounds, thats why
    // we count from MAX_BATTLEGROUND_QUEUES - 1 to 0
    // variable index removes useless searching in other team's queue
    uint32 index = BattleGround::GetTeamIndexByTeamId(group->GroupTeam);

    for (int8 bracket_id_tmp = MAX_BATTLEGROUND_BRACKETS - 1; bracket_id_tmp >= 0 && bracket_id == -1; --bracket_id_tmp)
    {
        // we must check premade and normal team's queue - because when players from premade are joining bg,
        // they leave groupinfo so we can't use its players size to find out index
        for (uint8 j = index; j < BG_QUEUE_GROUP_TYPES_COUNT; j += BG_QUEUE_NORMAL_ALLIANCE)
        {
            for (group_itr_tmp = m_QueuedGroups[bracket_id_tmp][j].begin(); group_itr_tmp != m_QueuedGroups[bracket_id_tmp][j].end(); ++group_itr_tmp)
            {
                if ((*group_itr_tmp) == group)
                {
                    bracket_id = bracket_id_tmp;
                    group_itr = group_itr_tmp;
                    // we must store index to be able to erase iterator
                    index = j;
                    break;
                }
            }
        }
    }
    // player can't be in queue without group, but just in case
    if (bracket_id == -1)
    {
        sLog.outError("BattleGroundQueue: ERROR Cannot find groupinfo for %s", guid.GetString().c_str());
        return;
    }
    DEBUG_LOG("BattleGroundQueue: Removing %s, from bracket_id %u", guid.GetString().c_str(), (uint32)bracket_id);

    // ALL variables are correctly set
    // We can ignore leveling up in queue - it should not cause crash
    // remove player from group
    // if only one player there, remove group

    // remove player queue info from group queue info
    GroupQueueInfoPlayers::iterator pitr = group->Players.find(guid);
    if (pitr != group->Players.end())
        group->Players.erase(pitr);

    // if invited to bg, and should decrease invited count, then do it
    if (decreaseInvitedCount && group->IsInvitedToBGInstanceGUID)
    {
        BattleGround* bg = sBattleGroundMgr.GetBattleGround(group->IsInvitedToBGInstanceGUID, group->BgTypeId);
        if (bg)
            bg->DecreaseInvitedCount(group->GroupTeam);
    }

    // remove player queue info
    m_QueuedPlayers.erase(itr);

    // announce to world if arena team left queue for rated match, show only once
    if (group->arenaType != ARENA_TYPE_NONE && group->IsRated && group->Players.empty() && sWorld.getConfig(CONFIG_BOOL_ARENA_QUEUE_ANNOUNCER_EXIT))
        sWorld.SendWorldText(LANG_ARENA_QUEUE_ANNOUNCE_WORLD_EXIT, group->arenaType, group->arenaType, group->ArenaTeamRating);

    // if player leaves queue and he is invited to rated arena match, then he have to loose
    if (group->IsInvitedToBGInstanceGUID && group->IsRated && decreaseInvitedCount)
    {
        ArenaTeam* at = sObjectMgr.GetArenaTeamById(group->ArenaTeamId);
        if (at)
        {
            DEBUG_LOG("UPDATING memberLost's personal arena rating for %s by opponents rating: %u", guid.GetString().c_str(), group->OpponentsTeamRating);
            Player* plr = sObjectMgr.GetPlayer(guid);
            if (plr)
                at->MemberLost(plr, group->OpponentsTeamRating);
            else
                at->OfflineMemberLost(guid, group->OpponentsTeamRating);
            at->SaveToDB();
        }
    }

    // remove group queue info if needed
    if (group->Players.empty())
    {
        m_QueuedGroups[bracket_id][index].erase(group_itr);
        delete group;
    }
    // if group wasn't empty, so it wasn't deleted, and player have left a rated
    // queue -> everyone from the group should leave too
    // don't remove recursively if already invited to bg!
    else if (!group->IsInvitedToBGInstanceGUID && group->IsRated)
    {
        // remove next player, this is recursive
        // first send removal information
        if (Player* plr2 = sObjectMgr.GetPlayer(group->Players.begin()->first))
        {
            BattleGround* bg = sBattleGroundMgr.GetBattleGroundTemplate(group->BgTypeId);
            BattleGroundQueueTypeId bgQueueTypeId = BattleGroundMgr::BGQueueTypeId(group->BgTypeId, group->arenaType);
            uint32 queueSlot = plr2->GetBattleGroundQueueIndex(bgQueueTypeId);
            plr2->RemoveBattleGroundQueueId(bgQueueTypeId); // must be called this way, because if you move this call to
            // queue->removeplayer, it causes bugs
            WorldPacket data;
            sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, plr2, queueSlot, STATUS_NONE, 0, 0, ARENA_TYPE_NONE);
            plr2->GetSession()->SendPacket(&data);
        }
        // then actually delete, this may delete the group as well!
        RemovePlayer(group->Players.begin()->first, decreaseInvitedCount);
    }
}

// returns true when player pl_guid is in queue and is invited to bgInstanceGuid
bool BattleGroundQueue::IsPlayerInvited(ObjectGuid pl_guid, const uint32 bgInstanceGuid, const uint32 removeTime)
{
    // ACE_Guard<ACE_Recursive_Thread_Mutex> g(m_Lock);
    QueuedPlayersMap::const_iterator qItr = m_QueuedPlayers.find(pl_guid);
    return (qItr != m_QueuedPlayers.end()
            && qItr->second.GroupInfo->IsInvitedToBGInstanceGUID == bgInstanceGuid
            && qItr->second.GroupInfo->RemoveInviteTime == removeTime);
}

bool BattleGroundQueue::GetPlayerGroupInfoData(ObjectGuid guid, GroupQueueInfo* ginfo)
{
    // ACE_Guard<ACE_Recursive_Thread_Mutex> g(m_Lock);
    QueuedPlayersMap::const_iterator qItr = m_QueuedPlayers.find(guid);
    if (qItr == m_QueuedPlayers.end())
        return false;
    *ginfo = *(qItr->second.GroupInfo);
    return true;
}

bool BattleGroundQueue::InviteGroupToBG(GroupQueueInfo* ginfo, BattleGround* bg, Team side)
{
    // set side if needed
    if (side)
        ginfo->GroupTeam = side;

    if (!ginfo->IsInvitedToBGInstanceGUID)
    {
        // not yet invited
        // set invitation
        ginfo->IsInvitedToBGInstanceGUID = bg->GetInstanceID();
        BattleGroundTypeId bgTypeId = bg->GetTypeID();
        BattleGroundQueueTypeId bgQueueTypeId = BattleGroundMgr::BGQueueTypeId(bgTypeId, bg->GetArenaType());
        BattleGroundBracketId bracket_id = bg->GetBracketId();

        // set ArenaTeamId for rated matches
        if (bg->isArena() && bg->isRated())
            bg->SetArenaTeamIdForTeam(ginfo->GroupTeam, ginfo->ArenaTeamId);

        ginfo->RemoveInviteTime = WorldTimer::getMSTime() + INVITE_ACCEPT_WAIT_TIME;

        // loop through the players
        for (GroupQueueInfoPlayers::iterator itr = ginfo->Players.begin(); itr != ginfo->Players.end(); ++itr)
        {
            // get the player
            Player* plr = sObjectMgr.GetPlayer(itr->first);
            // if offline, skip him, this should not happen - player is removed from queue when he logs out
            if (!plr)
                continue;

            // invite the player
            PlayerInvitedToBGUpdateAverageWaitTime(ginfo, bracket_id);
            // sBattleGroundMgr.InvitePlayer(plr, bg, ginfo->Team);

            // set invited player counters
            bg->IncreaseInvitedCount(ginfo->GroupTeam);

            plr->SetInviteForBattleGroundQueueType(bgQueueTypeId, ginfo->IsInvitedToBGInstanceGUID);

            // create remind invite events
            BGQueueInviteEvent* inviteEvent = new BGQueueInviteEvent(plr->GetObjectGuid(), ginfo->IsInvitedToBGInstanceGUID, bgTypeId, ginfo->arenaType, ginfo->RemoveInviteTime);
            plr->m_Events.AddEvent(inviteEvent, plr->m_Events.CalculateTime(INVITATION_REMIND_TIME));
            // create automatic remove events
            BGQueueRemoveEvent* removeEvent = new BGQueueRemoveEvent(plr->GetObjectGuid(), ginfo->IsInvitedToBGInstanceGUID, bgTypeId, bgQueueTypeId, ginfo->RemoveInviteTime);
            plr->m_Events.AddEvent(removeEvent, plr->m_Events.CalculateTime(INVITE_ACCEPT_WAIT_TIME));

            WorldPacket data;

            uint32 queueSlot = plr->GetBattleGroundQueueIndex(bgQueueTypeId);

            DEBUG_LOG("Battleground: invited %s to BG instance %u queueindex %u bgtype %u, I can't help it if they don't press the enter battle button.",
                      plr->GetGuidStr().c_str(), bg->GetInstanceID(), queueSlot, bg->GetTypeID());

            // send status packet
            sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, plr, queueSlot, STATUS_WAIT_JOIN, INVITE_ACCEPT_WAIT_TIME, 0, ginfo->arenaType);
            plr->GetSession()->SendPacket(&data);
        }
        return true;
    }

    return false;
}

/*
This function is inviting players to already running battlegrounds
Invitation type is based on config file
large groups are disadvantageous, because they will be kicked first if invitation type = 1
*/
void BattleGroundQueue::FillPlayersToBG(BattleGround* bg, BattleGroundBracketId bracket_id)
{
    int32 hordeFree = bg->GetFreeSlotsForTeam(HORDE);
    int32 aliFree   = bg->GetFreeSlotsForTeam(ALLIANCE);

    // iterator for iterating through bg queue
    GroupsQueueType::const_iterator Ali_itr = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE].begin();
    // count of groups in queue - used to stop cycles
    uint32 aliCount = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE].size();
    // index to queue which group is current
    uint32 aliIndex = 0;
    for (; aliIndex < aliCount && m_SelectionPools[BG_TEAM_ALLIANCE].AddGroup((*Ali_itr), aliFree); ++aliIndex)
        ++Ali_itr;
    // the same thing for horde
    GroupsQueueType::const_iterator Horde_itr = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_HORDE].begin();
    uint32 hordeCount = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_HORDE].size();
    uint32 hordeIndex = 0;
    for (; hordeIndex < hordeCount && m_SelectionPools[BG_TEAM_HORDE].AddGroup((*Horde_itr), hordeFree); ++hordeIndex)
        ++Horde_itr;

    // if ofc like BG queue invitation is set in config, then we are happy
    if (sWorld.getConfig(CONFIG_UINT32_BATTLEGROUND_INVITATION_TYPE) == 0)
        return;

    /*
    if we reached this code, then we have to solve NP - complete problem called Subset sum problem
    So one solution is to check all possible invitation subgroups, or we can use these conditions:
    1. Last time when BattleGroundQueue::Update was executed we invited all possible players - so there is only small possibility
        that we will invite now whole queue, because only 1 change has been made to queues from the last BattleGroundQueue::Update call
    2. Other thing we should consider is group order in queue
    */

    // At first we need to compare free space in bg and our selection pool
    int32 diffAli   = aliFree   - int32(m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount());
    int32 diffHorde = hordeFree - int32(m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount());
    while (abs(diffAli - diffHorde) > 1 && (m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() > 0 || m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() > 0))
    {
        // each cycle execution we need to kick at least 1 group
        if (diffAli < diffHorde)
        {
            // kick alliance group, add to pool new group if needed
            if (m_SelectionPools[BG_TEAM_ALLIANCE].KickGroup(diffHorde - diffAli))
            {
                for (; aliIndex < aliCount && m_SelectionPools[BG_TEAM_ALLIANCE].AddGroup((*Ali_itr), (aliFree >= diffHorde) ? aliFree - diffHorde : 0); ++aliIndex)
                    ++Ali_itr;
            }
            // if ali selection is already empty, then kick horde group, but if there are less horde than ali in bg - break;
            if (!m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount())
            {
                if (aliFree <= diffHorde + 1)
                    break;
                m_SelectionPools[BG_TEAM_HORDE].KickGroup(diffHorde - diffAli);
            }
        }
        else
        {
            // kick horde group, add to pool new group if needed
            if (m_SelectionPools[BG_TEAM_HORDE].KickGroup(diffAli - diffHorde))
            {
                for (; hordeIndex < hordeCount && m_SelectionPools[BG_TEAM_HORDE].AddGroup((*Horde_itr), (hordeFree >= diffAli) ? hordeFree - diffAli : 0); ++hordeIndex)
                    ++Horde_itr;
            }
            if (!m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount())
            {
                if (hordeFree <= diffAli + 1)
                    break;
                m_SelectionPools[BG_TEAM_ALLIANCE].KickGroup(diffAli - diffHorde);
            }
        }
        // count diffs after small update
        diffAli   = aliFree   - int32(m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount());
        diffHorde = hordeFree - int32(m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount());
    }
}

// this method checks if premade versus premade battleground is possible
// then after 30 mins (default) in queue it moves premade group to normal queue
// it tries to invite as much players as it can - to MaxPlayersPerTeam, because premade groups have more than MinPlayersPerTeam players
bool BattleGroundQueue::CheckPremadeMatch(BattleGroundBracketId bracket_id, uint32 MinPlayersPerTeam, uint32 MaxPlayersPerTeam)
{
    // check match
    if (!m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].empty() && !m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].empty())
    {
        // start premade match
        // if groups aren't invited
        GroupsQueueType::const_iterator ali_group, horde_group;
        for (ali_group = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].begin(); ali_group != m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].end(); ++ali_group)
            if (!(*ali_group)->IsInvitedToBGInstanceGUID)
                break;
        for (horde_group = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].begin(); horde_group != m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].end(); ++horde_group)
            if (!(*horde_group)->IsInvitedToBGInstanceGUID)
                break;

        if (ali_group != m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].end() && horde_group != m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].end())
        {
            m_SelectionPools[BG_TEAM_ALLIANCE].AddGroup((*ali_group), MaxPlayersPerTeam);
            m_SelectionPools[BG_TEAM_HORDE].AddGroup((*horde_group), MaxPlayersPerTeam);
            // add groups/players from normal queue to size of bigger group
            uint32 maxPlayers = std::max(m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount(), m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount());
            GroupsQueueType::const_iterator itr;
            for (uint8 i = 0; i < BG_TEAMS_COUNT; ++i)
            {
                for (itr = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + i].begin(); itr != m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + i].end(); ++itr)
                {
                    // if itr can join BG and player count is less that maxPlayers, then add group to selectionpool
                    if (!(*itr)->IsInvitedToBGInstanceGUID && !m_SelectionPools[i].AddGroup((*itr), maxPlayers))
                        break;
                }
            }
            // premade selection pools are set
            return true;
        }
    }
    // now check if we can move group from Premade queue to normal queue (timer has expired) or group size lowered!!
    // this could be 2 cycles but i'm checking only first team in queue - it can cause problem -
    // if first is invited to BG and seconds timer expired, but we can ignore it, because players have only 80 seconds to click to enter bg
    // and when they click or after 80 seconds the queue info is removed from queue
    uint32 time_before = WorldTimer::getMSTime() - sWorld.getConfig(CONFIG_UINT32_BATTLEGROUND_PREMADE_GROUP_WAIT_FOR_MATCH);
    for (uint8 i = 0; i < BG_TEAMS_COUNT; ++i)
    {
        if (!m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE + i].empty())
        {
            GroupsQueueType::iterator itr = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE + i].begin();
            if (!(*itr)->IsInvitedToBGInstanceGUID && ((*itr)->JoinTime < time_before || (*itr)->Players.size() < MinPlayersPerTeam))
            {
                // we must insert group to normal queue and erase pointer from premade queue
                m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + i].push_front((*itr));
                m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE + i].erase(itr);
            }
        }
    }
    // selection pools are not set
    return false;
}

// this method tries to create battleground or arena with MinPlayersPerTeam against MinPlayersPerTeam
bool BattleGroundQueue::CheckNormalMatch(BattleGround* bg_template, BattleGroundBracketId bracket_id, uint32 minPlayers, uint32 maxPlayers)
{
    GroupsQueueType::const_iterator itr_team[BG_TEAMS_COUNT];
    for (uint8 i = 0; i < BG_TEAMS_COUNT; ++i)
    {
        itr_team[i] = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + i].begin();
        for (; itr_team[i] != m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + i].end(); ++(itr_team[i]))
        {
            if (!(*(itr_team[i]))->IsInvitedToBGInstanceGUID)
            {
                m_SelectionPools[i].AddGroup(*(itr_team[i]), maxPlayers);
                if (m_SelectionPools[i].GetPlayerCount() >= minPlayers)
                    break;
            }
        }
    }
    // try to invite same number of players - this cycle may cause longer wait time even if there are enough players in queue, but we want ballanced bg
    uint32 j = BG_TEAM_ALLIANCE;
    if (m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() < m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount())
        j = BG_TEAM_HORDE;
    if (sWorld.getConfig(CONFIG_UINT32_BATTLEGROUND_INVITATION_TYPE) != 0
            && m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() >= minPlayers && m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() >= minPlayers)
    {
        // we will try to invite more groups to team with less players indexed by j
        ++(itr_team[j]);                                    // this will not cause a crash, because for cycle above reached break;
        for (; itr_team[j] != m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + j].end(); ++(itr_team[j]))
        {
            if (!(*(itr_team[j]))->IsInvitedToBGInstanceGUID)
                if (!m_SelectionPools[j].AddGroup(*(itr_team[j]), m_SelectionPools[(j + 1) % BG_TEAMS_COUNT].GetPlayerCount()))
                    break;
        }
        // do not allow to start bg with more than 2 players more on 1 faction
        if (abs((int32)(m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() - m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount())) > 2)
            return false;
    }
    // allow 1v0 if debug bg
    if (sBattleGroundMgr.isTesting() && bg_template->isBattleGround() && (m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() || m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount()))
        return true;
    // return true if there are enough players in selection pools - enable to work .debug bg command correctly
    return m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() >= minPlayers && m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() >= minPlayers;
}

// this method will check if we can invite players to same faction skirmish match
bool BattleGroundQueue::CheckSkirmishForSameFaction(BattleGroundBracketId bracket_id, uint32 minPlayersPerTeam)
{
    if (m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() < minPlayersPerTeam && m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() < minPlayersPerTeam)
        return false;
    BattleGroundTeamIndex teamIdx = BG_TEAM_ALLIANCE;
    BattleGroundTeamIndex otherTeamIdx = BG_TEAM_HORDE;
    Team otherTeamId = HORDE;
    if (m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() == minPlayersPerTeam)
    {
        teamIdx = BG_TEAM_HORDE;
        otherTeamIdx = BG_TEAM_ALLIANCE;
        otherTeamId = ALLIANCE;
    }
    // clear other team's selection
    m_SelectionPools[otherTeamIdx].Init();
    // store last ginfo pointer
    GroupQueueInfo* ginfo = m_SelectionPools[teamIdx].SelectedGroups.back();
    // set itr_team to group that was added to selection pool latest
    GroupsQueueType::iterator itr_team = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + teamIdx].begin();
    for (; itr_team != m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + teamIdx].end(); ++itr_team)
        if (ginfo == *itr_team)
            break;
    if (itr_team == m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + teamIdx].end())
        return false;
    GroupsQueueType::iterator itr_team2 = itr_team;
    ++itr_team2;
    // invite players to other selection pool
    for (; itr_team2 != m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + teamIdx].end(); ++itr_team2)
    {
        // if selection pool is full then break;
        if (!(*itr_team2)->IsInvitedToBGInstanceGUID && !m_SelectionPools[otherTeamIdx].AddGroup(*itr_team2, minPlayersPerTeam))
            break;
    }
    if (m_SelectionPools[otherTeamIdx].GetPlayerCount() != minPlayersPerTeam)
        return false;

    // here we have correct 2 selections and we need to change one teams team and move selection pool teams to other team's queue
    for (GroupsQueueType::iterator itr = m_SelectionPools[otherTeamIdx].SelectedGroups.begin(); itr != m_SelectionPools[otherTeamIdx].SelectedGroups.end(); ++itr)
    {
        // set correct team
        (*itr)->GroupTeam = otherTeamId;
        // add team to other queue
        m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + otherTeamIdx].push_front(*itr);
        // remove team from old queue
        GroupsQueueType::iterator itr2 = itr_team;
        ++itr2;
        for (; itr2 != m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + teamIdx].end(); ++itr2)
        {
            if (*itr2 == *itr)
            {
                m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + teamIdx].erase(itr2);
                break;
            }
        }
    }
    return true;
}

/*
this method is called when group is inserted, or player / group is removed from BG Queue - there is only one player's status changed, so we don't use while(true) cycles to invite whole queue
it must be called after fully adding the members of a group to ensure group joining
should be called from BattleGround::RemovePlayer function in some cases
*/
void BattleGroundQueue::Update(BattleGroundTypeId bgTypeId, BattleGroundBracketId bracket_id, ArenaType arenaType, bool isRated, uint32 arenaRating)
{
    // ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_Lock);
    // if no players in queue - do nothing
    if (m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].empty() &&
            m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].empty() &&
            m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE].empty() &&
            m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_HORDE].empty())
        return;

    // battleground with free slot for player should be always in the beggining of the queue
    // maybe it would be better to create bgfreeslotqueue for each bracket_id
    BGFreeSlotQueueType::iterator itr, next;
    for (itr = sBattleGroundMgr.BGFreeSlotQueue[bgTypeId].begin(); itr != sBattleGroundMgr.BGFreeSlotQueue[bgTypeId].end(); itr = next)
    {
        next = itr;
        ++next;
        // DO NOT allow queue manager to invite new player to arena
        if ((*itr)->isBattleGround() && (*itr)->GetTypeID() == bgTypeId && (*itr)->GetBracketId() == bracket_id &&
                (*itr)->GetStatus() > STATUS_WAIT_QUEUE && (*itr)->GetStatus() < STATUS_WAIT_LEAVE)
        {
            BattleGround* bg = *itr; // we have to store battleground pointer here, because when battleground is full, it is removed from free queue (not yet implemented!!)
            // and iterator is invalid

            // clear selection pools
            m_SelectionPools[BG_TEAM_ALLIANCE].Init();
            m_SelectionPools[BG_TEAM_HORDE].Init();

            // call a function that does the job for us
            FillPlayersToBG(bg, bracket_id);

            // now everything is set, invite players
            for (GroupsQueueType::const_iterator citr = m_SelectionPools[BG_TEAM_ALLIANCE].SelectedGroups.begin(); citr != m_SelectionPools[BG_TEAM_ALLIANCE].SelectedGroups.end(); ++citr)
                InviteGroupToBG((*citr), bg, (*citr)->GroupTeam);
            for (GroupsQueueType::const_iterator citr = m_SelectionPools[BG_TEAM_HORDE].SelectedGroups.begin(); citr != m_SelectionPools[BG_TEAM_HORDE].SelectedGroups.end(); ++citr)
                InviteGroupToBG((*citr), bg, (*citr)->GroupTeam);

            if (!bg->HasFreeSlots())
            {
                // remove BG from BGFreeSlotQueue
                bg->RemoveFromBGFreeSlotQueue();
            }
        }
    }

    // finished iterating through the bgs with free slots, maybe we need to create a new bg

    BattleGround* bg_template = sBattleGroundMgr.GetBattleGroundTemplate(bgTypeId);
    if (!bg_template)
    {
        sLog.outError("Battleground: Update: bg template not found for %u", bgTypeId);
        return;
    }

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketById(bg_template->GetMapId(), bracket_id);
    if (!bracketEntry)
    {
        sLog.outError("Battleground: Update: bg bracket entry not found for map %u bracket id %u", bg_template->GetMapId(), bracket_id);
        return;
    }

    // get the min. players per team, properly for larger arenas as well. (must have full teams for arena matches!)
    uint32 MinPlayersPerTeam = bg_template->GetMinPlayersPerTeam();
    uint32 MaxPlayersPerTeam = bg_template->GetMaxPlayersPerTeam();
    if (sBattleGroundMgr.isTesting())
        MinPlayersPerTeam = 1;
    if (bg_template->isArena())
    {
        if (sBattleGroundMgr.isArenaTesting())
        {
            MaxPlayersPerTeam = 1;
            MinPlayersPerTeam = 1;
        }
        else
        {
            // this switch can be much shorter
            MaxPlayersPerTeam = arenaType;
            MinPlayersPerTeam = arenaType;
            /*switch(arenaType)
            {
            case ARENA_TYPE_2v2:
                MaxPlayersPerTeam = 2;
                MinPlayersPerTeam = 2;
                break;
            case ARENA_TYPE_3v3:
                MaxPlayersPerTeam = 3;
                MinPlayersPerTeam = 3;
                break;
            case ARENA_TYPE_5v5:
                MaxPlayersPerTeam = 5;
                MinPlayersPerTeam = 5;
                break;
            }*/
        }
    }

    m_SelectionPools[BG_TEAM_ALLIANCE].Init();
    m_SelectionPools[BG_TEAM_HORDE].Init();

    if (bg_template->isBattleGround())
    {
        // check if there is premade against premade match
        if (CheckPremadeMatch(bracket_id, MinPlayersPerTeam, MaxPlayersPerTeam))
        {
            // create new battleground
            BattleGround* bg2 = sBattleGroundMgr.CreateNewBattleGround(bgTypeId, bracketEntry, ARENA_TYPE_NONE, false);
            if (!bg2)
            {
                sLog.outError("BattleGroundQueue::Update - Cannot create battleground: %u", bgTypeId);
                return;
            }
            // invite those selection pools
            for (uint8 i = 0; i < BG_TEAMS_COUNT; ++i)
                for (GroupsQueueType::const_iterator citr = m_SelectionPools[BG_TEAM_ALLIANCE + i].SelectedGroups.begin(); citr != m_SelectionPools[BG_TEAM_ALLIANCE + i].SelectedGroups.end(); ++citr)
                    InviteGroupToBG((*citr), bg2, (*citr)->GroupTeam);
            // start bg
            bg2->StartBattleGround();
            // clear structures
            m_SelectionPools[BG_TEAM_ALLIANCE].Init();
            m_SelectionPools[BG_TEAM_HORDE].Init();
        }
    }

    // now check if there are in queues enough players to start new game of (normal battleground, or non-rated arena)
    if (!isRated)
    {
        // if there are enough players in pools, start new battleground or non rated arena
        if (CheckNormalMatch(bg_template, bracket_id, MinPlayersPerTeam, MaxPlayersPerTeam)
                || (bg_template->isArena() && CheckSkirmishForSameFaction(bracket_id, MinPlayersPerTeam)))
        {
            // we successfully created a pool
            BattleGround* bg2 = sBattleGroundMgr.CreateNewBattleGround(bgTypeId, bracketEntry, arenaType, false);
            if (!bg2)
            {
                sLog.outError("BattleGroundQueue::Update - Cannot create battleground: %u", bgTypeId);
                return;
            }

            // invite those selection pools
            for (uint8 i = 0; i < BG_TEAMS_COUNT; ++i)
                for (GroupsQueueType::const_iterator citr = m_SelectionPools[BG_TEAM_ALLIANCE + i].SelectedGroups.begin(); citr != m_SelectionPools[BG_TEAM_ALLIANCE + i].SelectedGroups.end(); ++citr)
                    InviteGroupToBG((*citr), bg2, (*citr)->GroupTeam);
            // start bg
            bg2->StartBattleGround();
        }
    }
    else if (bg_template->isArena())
    {
        // found out the minimum and maximum ratings the newly added team should battle against
        // arenaRating is the rating of the latest joined team, or 0
        // 0 is on (automatic update call) and we must set it to team's with longest wait time
        if (!arenaRating)
        {
            GroupQueueInfo* front1 = NULL;
            GroupQueueInfo* front2 = NULL;
            if (!m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].empty())
            {
                front1 = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].front();
                arenaRating = front1->ArenaTeamRating;
            }
            if (!m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].empty())
            {
                front2 = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].front();
                arenaRating = front2->ArenaTeamRating;
            }
            if (front1 && front2)
            {
                if (front1->JoinTime < front2->JoinTime)
                    arenaRating = front1->ArenaTeamRating;
            }
            else if (!front1 && !front2)
                return; // queues are empty
        }

        // set rating range
        uint32 arenaMinRating = (arenaRating <= sBattleGroundMgr.GetMaxRatingDifference()) ? 0 : arenaRating - sBattleGroundMgr.GetMaxRatingDifference();
        uint32 arenaMaxRating = arenaRating + sBattleGroundMgr.GetMaxRatingDifference();
        // if max rating difference is set and the time past since server startup is greater than the rating discard time
        // (after what time the ratings aren't taken into account when making teams) then
        // the discard time is current_time - time_to_discard, teams that joined after that, will have their ratings taken into account
        // else leave the discard time on 0, this way all ratings will be discarded
        uint32 discardTime = WorldTimer::getMSTime() - sBattleGroundMgr.GetRatingDiscardTimer();

        // we need to find 2 teams which will play next game

        GroupsQueueType::iterator itr_team[BG_TEAMS_COUNT];

        // optimalization : --- we dont need to use selection_pools - each update we select max 2 groups

        for (uint8 i = BG_QUEUE_PREMADE_ALLIANCE; i < BG_QUEUE_NORMAL_ALLIANCE; ++i)
        {
            // take the group that joined first
            itr_team[i] = m_QueuedGroups[bracket_id][i].begin();
            for (; itr_team[i] != m_QueuedGroups[bracket_id][i].end(); ++(itr_team[i]))
            {
                // if group match conditions, then add it to pool
                if (!(*itr_team[i])->IsInvitedToBGInstanceGUID
                        && (((*itr_team[i])->ArenaTeamRating >= arenaMinRating && (*itr_team[i])->ArenaTeamRating <= arenaMaxRating)
                            || (*itr_team[i])->JoinTime < discardTime))
                {
                    m_SelectionPools[i].AddGroup((*itr_team[i]), MaxPlayersPerTeam);
                    // break for cycle to be able to start selecting another group from same faction queue
                    break;
                }
            }
        }
        // now we are done if we have 2 groups - ali vs horde!
        // if we don't have, we must try to continue search in same queue
        // tmp variables are correctly set
        // this code isn't much userfriendly - but it is supposed to continue search for mathing group in HORDE queue
        if (m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() == 0 && m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount())
        {
            itr_team[BG_TEAM_ALLIANCE] = itr_team[BG_TEAM_HORDE];
            ++itr_team[BG_TEAM_ALLIANCE];
            for (; itr_team[BG_TEAM_ALLIANCE] != m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].end(); ++(itr_team[BG_TEAM_ALLIANCE]))
            {
                if (!(*itr_team[BG_TEAM_ALLIANCE])->IsInvitedToBGInstanceGUID
                        && (((*itr_team[BG_TEAM_ALLIANCE])->ArenaTeamRating >= arenaMinRating && (*itr_team[BG_TEAM_ALLIANCE])->ArenaTeamRating <= arenaMaxRating)
                            || (*itr_team[BG_TEAM_ALLIANCE])->JoinTime < discardTime))
                {
                    m_SelectionPools[BG_TEAM_ALLIANCE].AddGroup((*itr_team[BG_TEAM_ALLIANCE]), MaxPlayersPerTeam);
                    break;
                }
            }
        }
        // this code isn't much userfriendly - but it is supposed to continue search for mathing group in ALLIANCE queue
        if (m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() == 0 && m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount())
        {
            itr_team[BG_TEAM_HORDE] = itr_team[BG_TEAM_ALLIANCE];
            ++itr_team[BG_TEAM_HORDE];
            for (; itr_team[BG_TEAM_HORDE] != m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].end(); ++(itr_team[BG_TEAM_HORDE]))
            {
                if (!(*itr_team[BG_TEAM_HORDE])->IsInvitedToBGInstanceGUID
                        && (((*itr_team[BG_TEAM_HORDE])->ArenaTeamRating >= arenaMinRating && (*itr_team[BG_TEAM_HORDE])->ArenaTeamRating <= arenaMaxRating)
                            || (*itr_team[BG_TEAM_HORDE])->JoinTime < discardTime))
                {
                    m_SelectionPools[BG_TEAM_HORDE].AddGroup((*itr_team[BG_TEAM_HORDE]), MaxPlayersPerTeam);
                    break;
                }
            }
        }

        // if we have 2 teams, then start new arena and invite players!
        if (m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() && m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount())
        {
            BattleGround* arena = sBattleGroundMgr.CreateNewBattleGround(bgTypeId, bracketEntry, arenaType, true);
            if (!arena)
            {
                sLog.outError("BattlegroundQueue::Update couldn't create arena instance for rated arena match!");
                return;
            }

            (*(itr_team[BG_TEAM_ALLIANCE]))->OpponentsTeamRating = (*(itr_team[BG_TEAM_HORDE]))->ArenaTeamRating;
            DEBUG_LOG("setting oposite teamrating for team %u to %u", (*(itr_team[BG_TEAM_ALLIANCE]))->ArenaTeamId, (*(itr_team[BG_TEAM_ALLIANCE]))->OpponentsTeamRating);
            (*(itr_team[BG_TEAM_HORDE]))->OpponentsTeamRating = (*(itr_team[BG_TEAM_ALLIANCE]))->ArenaTeamRating;
            DEBUG_LOG("setting oposite teamrating for team %u to %u", (*(itr_team[BG_TEAM_HORDE]))->ArenaTeamId, (*(itr_team[BG_TEAM_HORDE]))->OpponentsTeamRating);
            // now we must move team if we changed its faction to another faction queue, because then we will spam log by errors in Queue::RemovePlayer
            if ((*(itr_team[BG_TEAM_ALLIANCE]))->GroupTeam != ALLIANCE)
            {
                // add to alliance queue
                m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].push_front(*(itr_team[BG_TEAM_ALLIANCE]));
                // erase from horde queue
                m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].erase(itr_team[BG_TEAM_ALLIANCE]);
                itr_team[BG_TEAM_ALLIANCE] = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].begin();
            }
            if ((*(itr_team[BG_TEAM_HORDE]))->GroupTeam != HORDE)
            {
                m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].push_front(*(itr_team[BG_TEAM_HORDE]));
                m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].erase(itr_team[BG_TEAM_HORDE]);
                itr_team[BG_TEAM_HORDE] = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].begin();
            }

            InviteGroupToBG(*(itr_team[BG_TEAM_ALLIANCE]), arena, ALLIANCE);
            InviteGroupToBG(*(itr_team[BG_TEAM_HORDE]), arena, HORDE);

            DEBUG_LOG("Starting rated arena match!");

            arena->StartBattleGround();
        }
    }
}

/*********************************************************/
/***            BATTLEGROUND QUEUE EVENTS              ***/
/*********************************************************/

bool BGQueueInviteEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    Player* plr = sObjectMgr.GetPlayer(m_PlayerGuid);
    // player logged off (we should do nothing, he is correctly removed from queue in another procedure)
    if (!plr)
        return true;

    BattleGround* bg = sBattleGroundMgr.GetBattleGround(m_BgInstanceGUID, m_BgTypeId);
    // if battleground ended and its instance deleted - do nothing
    if (!bg)
        return true;

    BattleGroundQueueTypeId bgQueueTypeId = BattleGroundMgr::BGQueueTypeId(bg->GetTypeID(), bg->GetArenaType());
    uint32 queueSlot = plr->GetBattleGroundQueueIndex(bgQueueTypeId);
    if (queueSlot < PLAYER_MAX_BATTLEGROUND_QUEUES)         // player is in queue or in battleground
    {
        // check if player is invited to this bg
        BattleGroundQueue& bgQueue = sBattleGroundMgr.m_BattleGroundQueues[bgQueueTypeId];
        if (bgQueue.IsPlayerInvited(m_PlayerGuid, m_BgInstanceGUID, m_RemoveTime))
        {
            WorldPacket data;
            // we must send remaining time in queue
            sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, plr, queueSlot, STATUS_WAIT_JOIN, INVITE_ACCEPT_WAIT_TIME - INVITATION_REMIND_TIME, 0, m_ArenaType);
            plr->GetSession()->SendPacket(&data);
        }
    }
    return true;                                            // event will be deleted
}

void BGQueueInviteEvent::Abort(uint64 /*e_time*/)
{
    // do nothing
}

/*
    this event has many possibilities when it is executed:
    1. player is in battleground ( he clicked enter on invitation window )
    2. player left battleground queue and he isn't there any more
    3. player left battleground queue and he joined it again and IsInvitedToBGInstanceGUID = 0
    4. player left queue and he joined again and he has been invited to same battleground again -> we should not remove him from queue yet
    5. player is invited to bg and he didn't choose what to do and timer expired - only in this condition we should call queue::RemovePlayer
    we must remove player in the 5. case even if battleground object doesn't exist!
*/
bool BGQueueRemoveEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    Player* plr = sObjectMgr.GetPlayer(m_PlayerGuid);
    if (!plr)
        // player logged off (we should do nothing, he is correctly removed from queue in another procedure)
        return true;

    BattleGround* bg = sBattleGroundMgr.GetBattleGround(m_BgInstanceGUID, m_BgTypeId);
    // battleground can be deleted already when we are removing queue info
    // bg pointer can be NULL! so use it carefully!

    uint32 queueSlot = plr->GetBattleGroundQueueIndex(m_BgQueueTypeId);
    if (queueSlot < PLAYER_MAX_BATTLEGROUND_QUEUES)         // player is in queue, or in Battleground
    {
        // check if player is in queue for this BG and if we are removing his invite event
        BattleGroundQueue& bgQueue = sBattleGroundMgr.m_BattleGroundQueues[m_BgQueueTypeId];
        if (bgQueue.IsPlayerInvited(m_PlayerGuid, m_BgInstanceGUID, m_RemoveTime))
        {
            DEBUG_LOG("Battleground: removing player %u from bg queue for instance %u because of not pressing enter battle in time.", plr->GetGUIDLow(), m_BgInstanceGUID);

            plr->RemoveBattleGroundQueueId(m_BgQueueTypeId);
            bgQueue.RemovePlayer(m_PlayerGuid, true);
            // update queues if battleground isn't ended
            if (bg && bg->isBattleGround() && bg->GetStatus() != STATUS_WAIT_LEAVE)
                sBattleGroundMgr.ScheduleQueueUpdate(0, ARENA_TYPE_NONE, m_BgQueueTypeId, m_BgTypeId, bg->GetBracketId());

            WorldPacket data;
            sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, plr, queueSlot, STATUS_NONE, 0, 0, ARENA_TYPE_NONE);
            plr->GetSession()->SendPacket(&data);
        }
    }

    // event will be deleted
    return true;
}

void BGQueueRemoveEvent::Abort(uint64 /*e_time*/)
{
    // do nothing
}

/*********************************************************/
/***            BATTLEGROUND MANAGER                   ***/
/*********************************************************/

BattleGroundMgr::BattleGroundMgr() : m_ArenaTesting(false)
{
    for (uint8 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; ++i)
        m_BattleGrounds[i].clear();
    m_NextRatingDiscardUpdate = sWorld.getConfig(CONFIG_UINT32_ARENA_RATING_DISCARD_TIMER);
    m_Testing = false;
}

BattleGroundMgr::~BattleGroundMgr()
{
    DeleteAllBattleGrounds();
}

void BattleGroundMgr::DeleteAllBattleGrounds()
{
    // will also delete template bgs:
    for (uint8 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; ++i)
    {
        for (BattleGroundSet::iterator itr = m_BattleGrounds[i].begin(); itr != m_BattleGrounds[i].end();)
        {
            BattleGround* bg = itr->second;
            ++itr;                                          // step from invalidate iterator pos in result element remove in ~BattleGround call
            delete bg;
        }
    }
}

// used to update running battlegrounds, and delete finished ones
void BattleGroundMgr::Update(uint32 diff)
{
    // update scheduled queues
    if (!m_QueueUpdateScheduler.empty())
    {
        std::vector<uint64> scheduled;
        {
            // create mutex
            // ACE_Guard<ACE_Thread_Mutex> guard(SchedulerLock);
            // copy vector and clear the other
            scheduled = std::vector<uint64>(m_QueueUpdateScheduler);
            m_QueueUpdateScheduler.clear();
            // release lock
        }

        for (uint8 i = 0; i < scheduled.size(); ++i)
        {
            uint32 arenaRating = scheduled[i] >> 32;
            ArenaType arenaType = ArenaType(scheduled[i] >> 24 & 255);
            BattleGroundQueueTypeId bgQueueTypeId = BattleGroundQueueTypeId(scheduled[i] >> 16 & 255);
            BattleGroundTypeId bgTypeId = BattleGroundTypeId((scheduled[i] >> 8) & 255);
            BattleGroundBracketId bracket_id = BattleGroundBracketId(scheduled[i] & 255);
            m_BattleGroundQueues[bgQueueTypeId].Update(bgTypeId, bracket_id, arenaType, arenaRating > 0, arenaRating);
        }
    }

    // if rating difference counts, maybe force-update queues
    if (sWorld.getConfig(CONFIG_UINT32_ARENA_MAX_RATING_DIFFERENCE) && sWorld.getConfig(CONFIG_UINT32_ARENA_RATING_DISCARD_TIMER))
    {
        // it's time to force update
        if (m_NextRatingDiscardUpdate < diff)
        {
            // forced update for rated arenas (scan all, but skipped non rated)
            DEBUG_LOG("BattleGroundMgr: UPDATING ARENA QUEUES");
            for (uint8 qtype = BATTLEGROUND_QUEUE_2v2; qtype <= BATTLEGROUND_QUEUE_5v5; ++qtype)
                for (uint8 bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
                    m_BattleGroundQueues[qtype].Update(
                        BATTLEGROUND_AA, BattleGroundBracketId(bracket),
                        BattleGroundMgr::BGArenaType(BattleGroundQueueTypeId(qtype)), true, 0);

            m_NextRatingDiscardUpdate = sWorld.getConfig(CONFIG_UINT32_ARENA_RATING_DISCARD_TIMER);
        }
        else
            m_NextRatingDiscardUpdate -= diff;
    }
}

void BattleGroundMgr::BuildBattleGroundStatusPacket(WorldPacket* data, BattleGround* bg, Player* player, uint8 QueueSlot, uint8 StatusID, uint32 Time1, uint32 Time2, ArenaType arenatype)
{
    // we can be in 2 queues in same time...
    if (!bg)
        StatusID = STATUS_NONE;

    ObjectGuid playerGuid = player->GetObjectGuid();
    ObjectGuid bgGuid = bg ? bg->GetObjectGuid() : ObjectGuid();

    DEBUG_LOG("BattleGroundMgr::BuildBattleGroundStatusPacket bgGuid %s, playerguid %s, queueSlot %u, "
        "statusid %u time1 %u time2 %u arenatype %u",
        bgGuid.GetString().c_str(), playerGuid.GetString().c_str(), QueueSlot, StatusID, Time1, Time2, arenatype);

    switch (StatusID)
    {
        case STATUS_NONE:
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS);

            data->WriteGuidMask<0, 4, 7, 1, 6, 3, 5, 2>(playerGuid);
            data->WriteGuidBytes<5, 6, 7, 2>(playerGuid);
            *data << uint32(QueueSlot);                 // not queue slot
            data->WriteGuidBytes<3, 1>(playerGuid);
            *data << uint32(QueueSlot);                 // Queue slot
            *data << uint32(bg->GetStartTime());
            data->WriteGuidBytes<0, 4>(playerGuid);
            break;
        }
        case STATUS_WAIT_QUEUE:
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS_QUEUED);

            data->WriteGuidMask<3, 0>(playerGuid);
            data->WriteGuidMask<3>(bgGuid);
            data->WriteGuidMask<2>(playerGuid);
            data->WriteBit(1);                          // eligible in queue
            data->WriteBit(0);                          // Join Failed
            data->WriteGuidMask<2>(bgGuid);
            data->WriteGuidMask<1>(playerGuid);
            data->WriteGuidMask<0, 6, 4>(bgGuid);
            data->WriteGuidMask<6, 7>(playerGuid);
            data->WriteGuidMask<7, 5>(bgGuid);
            data->WriteGuidMask<4, 5>(playerGuid);
            data->WriteBit(bg->isRated());              // is rated bg or arena. if israted => cant exit (wtf?)
            data->WriteBit(0);                          // Waiting On Other Activity 4
            data->WriteGuidMask<1>(bgGuid);

            data->WriteGuidBytes<0>(playerGuid);
            *data << uint32(QueueSlot);                 // unk 5 - received in battlefield port then
            data->WriteGuidBytes<5>(bgGuid);
            data->WriteGuidBytes<3>(playerGuid);
            *data << uint32(Time1);                     // Estimated Wait Time 6 time1
            data->WriteGuidBytes<7, 1, 2>(bgGuid);
            *data << uint8(0);                          // unk param
            data->WriteGuidBytes<4>(bgGuid);
            data->WriteGuidBytes<2>(playerGuid);
            //if (...)
            //    *data << uint8(playerCount);          // player count (XvX) > 0 && rated: Rated XvX, !rated: Skirmish XvX. if ==0 >= Bg type
            //else
                *data << uint8(arenatype);
            data->WriteGuidBytes<6>(bgGuid);
            data->WriteGuidBytes<7>(playerGuid);
            data->WriteGuidBytes<3>(bgGuid);
            data->WriteGuidBytes<6>(playerGuid);
            data->WriteGuidBytes<0>(bgGuid);
            *data << uint32(0);                         // Time
            *data << uint32(QueueSlot);                 // queueslot
            *data << uint8(0);                          // maxlevel? seen 85 only in sniffs
            *data << uint32(Time2);                     // unk 10 Time since started time2
            data->WriteGuidBytes<1, 5>(playerGuid);
            *data << uint32(bg->GetClientInstanceID()); // client instance id
            data->WriteGuidBytes<4>(playerGuid);
            break;
        }
        case STATUS_WAIT_JOIN:
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS_NEEDCONFIRMATION, 44);

            *data << uint32(bg->GetClientInstanceID()); // Client Instance ID
            *data << uint32(Time1);                     // Time until closed
            //*data << uint8(bg->GetPlayersCountByTeam(bg->GetPlayerTeam(playerGuid)));
            *data << uint8(0);                          // unk 0
            *data << uint32(QueueSlot);                 // Queue slot
            *data << uint32(0);                         // Time
            *data << uint8(0);                          // Max Level
            *data << uint32(QueueSlot);                 // not queueslot
            *data << uint32(bg->GetMapId());            // Map Id
            //if (...)
            //    *data << uint8(playerCount);          // player count (XvX) > 0 && rated: Rated XvX, !rated: Skirmish XvX. if ==0 >= Bg type
            //else
                *data << uint8(arenatype);

            data->WriteGuidMask<5, 2, 1>(playerGuid);
            data->WriteGuidMask<2>(bgGuid);
            data->WriteGuidMask<4>(playerGuid);
            data->WriteGuidMask<6, 3>(bgGuid);
            data->WriteBit(bg->isRated());              // Is Rated
            data->WriteGuidMask<7, 3>(playerGuid);
            data->WriteGuidMask<7, 0, 4>(bgGuid);
            data->WriteGuidMask<6>(playerGuid);
            data->WriteGuidMask<5, 1>(bgGuid);
            data->WriteGuidMask<0>(playerGuid);

            data->WriteGuidBytes<6, 5, 7, 2>(bgGuid);
            data->WriteGuidBytes<0, 7>(playerGuid);
            data->WriteGuidBytes<4>(bgGuid);
            data->WriteGuidBytes<1>(playerGuid);
            data->WriteGuidBytes<0>(bgGuid);
            data->WriteGuidBytes<4>(playerGuid);
            data->WriteGuidBytes<1>(bgGuid);
            data->WriteGuidBytes<5>(playerGuid);
            data->WriteGuidBytes<3>(bgGuid);
            data->WriteGuidBytes<6, 2, 3>(playerGuid);
            break;
        }
        case STATUS_IN_PROGRESS:
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS_ACTIVE, 49);

            data->WriteGuidMask<2, 7>(playerGuid);
            data->WriteGuidMask<7, 1>(bgGuid);
            data->WriteGuidMask<5>(playerGuid);
            data->WriteBit(bg->GetPlayerTeam(playerGuid) == ALLIANCE);  // Bg faction bit
            data->WriteGuidMask<0>(bgGuid);
            data->WriteGuidMask<1>(playerGuid);
            data->WriteGuidMask<3>(bgGuid);
            data->WriteGuidMask<6>(playerGuid);
            data->WriteGuidMask<5>(bgGuid);
            data->WriteBit(bg->isRated());              // Unk Bit 64
            data->WriteGuidMask<4>(playerGuid);
            data->WriteGuidMask<6, 4, 2>(bgGuid);
            data->WriteGuidMask<3, 0>(playerGuid);

            data->WriteGuidBytes<4, 5>(bgGuid);
            data->WriteGuidBytes<5>(playerGuid);
            data->WriteGuidBytes<1, 6, 3, 7>(bgGuid);
            data->WriteGuidBytes<6>(playerGuid);

            *data << uint32(0);                         // Time
            *data << uint8(0);                          // unk

            data->WriteGuidBytes<4, 1>(playerGuid);

            *data << uint32(QueueSlot);                 // Queue slot
            //if (...)
            //    *data << uint8(playerCount);          // player count (XvX) > 0 && rated: Rated XvX, !rated: Skirmish XvX. if ==0 >= Bg type
            //else
                *data << uint8(arenatype);

            *data << uint32(QueueSlot);                 // not queue slot
            *data << uint32(bg->GetMapId());            // Map Id
            *data << uint8(0);                          // Max Level? seen 85
            *data << uint32(Time2);                     // Time since started

            data->WriteGuidBytes<2>(playerGuid);

            *data << uint32(Time1);                     // Time until closed

            data->WriteGuidBytes<0, 3>(playerGuid);
            data->WriteGuidBytes<2>(bgGuid);

            *data << uint32(bg->GetClientInstanceID()); // Client Instance ID

            data->WriteGuidBytes<0>(bgGuid);
            data->WriteGuidBytes<7>(playerGuid);
            break;
        }
        case STATUS_WAIT_LEAVE:
        {
            // not used currently and not checked
            data->Initialize(SMSG_BATTLEFIELD_STATUS_WAITFORGROUPS, 48);

            *data << uint8(0);                          // unk
            *data << uint32(QueueSlot);                 // not queueSlot
            *data << uint32(QueueSlot);                 // Queue slot
            *data << uint32(bg->GetEndTime());          // Time until closed
            *data << uint32(0);                         // unk
            *data << uint8(0);                          // unk
            *data << uint8(0);                          // unk
            *data << uint8(bg->GetMinLevel());          // Min Level
            *data << uint8(0);                          // unk
            *data << uint8(0);                          // unk
            *data << uint32(bg->GetMapId());            // Map Id
            *data << uint32(0);                         // Time
            *data << uint8(0);                          // unk

            data->WriteGuidMask<0, 1, 7>(bgGuid);
            data->WriteGuidMask<7, 0>(playerGuid);
            data->WriteGuidMask<4>(bgGuid);
            data->WriteGuidMask<6, 2, 3>(playerGuid);
            data->WriteGuidMask<3>(bgGuid);
            data->WriteGuidMask<4>(playerGuid);
            data->WriteGuidMask<5>(bgGuid);
            data->WriteGuidMask<5>(playerGuid);
            data->WriteGuidMask<2>(bgGuid);
            data->WriteBit(bg->isRated());              // Is Rated
            data->WriteGuidMask<1>(playerGuid);
            data->WriteGuidMask<6>(bgGuid);

            data->WriteGuidBytes<0>(playerGuid);
            data->WriteGuidBytes<4>(bgGuid);
            data->WriteGuidBytes<3>(playerGuid);
            data->WriteGuidBytes<1, 0, 2>(bgGuid);
            data->WriteGuidBytes<2>(playerGuid);
            data->WriteGuidBytes<7>(bgGuid);
            data->WriteGuidBytes<1, 6>(playerGuid);
            data->WriteGuidBytes<6, 5>(bgGuid);
            data->WriteGuidBytes<5, 4, 7>(playerGuid);
            data->WriteGuidBytes<3>(bgGuid);
            break;
        }
        default:
            sLog.outError("Unknown BG status %u!", StatusID);
            break;
    }
}

void BattleGroundMgr::BuildPvpLogDataPacket(WorldPacket* data, BattleGround* bg)
{
    ByteBuffer buffer;

    // last check on 4.3.4
    data->Initialize(SMSG_PVP_LOG_DATA, (1 + 1 + 4 + 40 * bg->GetPlayerScoresSize()));
    data->WriteBit(bg->isArena());
    data->WriteBit(bg->isRated());

    if (bg->isArena())
    {
        // it seems this must be according to BG_WINNER_A/H and _NOT_ BG_TEAM_A/H
        for (int8 i = 0; i < BG_TEAMS_COUNT; ++i)
        {
            if (ArenaTeam* at = sObjectMgr.GetArenaTeamById(bg->m_ArenaTeamIds[i]))
                data->WriteBits(at->GetName().length(), 8);
            else
                data->WriteBits(0, 8);
        }
    }

    data->WriteBits(bg->GetPlayerScoresSize(), 21);
    for (BattleGround::BattleGroundScoreMap::const_iterator itr = bg->GetPlayerScoresBegin(); itr != bg->GetPlayerScoresEnd(); ++itr)
    {
        ObjectGuid memberGuid = itr->first;
        Player* player = sObjectMgr.GetPlayer(itr->first);

        data->WriteBit(0);                  // unk1
        data->WriteBit(0);                  // unk2
        data->WriteGuidMask<2>(memberGuid);
        data->WriteBit(!bg->isArena());
        data->WriteBit(0);                  // unk4
        data->WriteBit(0);                  // unk5
        data->WriteBit(0);                  // unk6
        data->WriteGuidMask<3, 0, 5, 1, 6>(memberGuid);
        Team team = bg->GetPlayerTeam(itr->first);
        if (!team && player)
            team = player->GetTeam();
        data->WriteBit(team == ALLIANCE);   // unk7
        data->WriteGuidMask<7>(memberGuid);

        buffer << uint32(itr->second->HealingDone);         // healing done
        buffer << uint32(itr->second->DamageDone);          // damage done

        if (!bg->isArena())
        {
            buffer << uint32(itr->second->BonusHonor);
            buffer << uint32(itr->second->Deaths);
            buffer << uint32(itr->second->HonorableKills);
        }

        buffer.WriteGuidBytes<4>(memberGuid);
        buffer << uint32(itr->second->KillingBlows);
        // if (unk5) << uint32() unk
        buffer.WriteGuidBytes<5>(memberGuid);
        // if (unk6) << uint32() unk
        // if (unk2) << uint32() unk
        buffer.WriteGuidBytes<1, 6>(memberGuid);
        // TODO: store this in player score
        if (player)
            buffer << uint32(player->GetPrimaryTalentTree(player->GetActiveSpec()));
        else
            buffer << uint32(0);

        switch (bg->GetTypeID())                            // battleground specific things
        {
            case BATTLEGROUND_AV:
                data->WriteBits(5, 24);                     // count of next fields
                buffer << uint32(((BattleGroundAVScore*)itr->second)->GraveyardsAssaulted); // GraveyardsAssaulted
                buffer << uint32(((BattleGroundAVScore*)itr->second)->GraveyardsDefended);  // GraveyardsDefended
                buffer << uint32(((BattleGroundAVScore*)itr->second)->TowersAssaulted);     // TowersAssaulted
                buffer << uint32(((BattleGroundAVScore*)itr->second)->TowersDefended);      // TowersDefended
                buffer << uint32(((BattleGroundAVScore*)itr->second)->SecondaryObjectives); // SecondaryObjectives - free some of the Lieutnants
                break;
            case BATTLEGROUND_WS:
                data->WriteBits(2, 24);                     // count of next fields
                buffer << uint32(((BattleGroundWGScore*)itr->second)->FlagCaptures);        // flag captures
                buffer << uint32(((BattleGroundWGScore*)itr->second)->FlagReturns);         // flag returns
                break;
            case BATTLEGROUND_AB:
                data->WriteBits(2, 24);                     // count of next fields
                buffer << uint32(((BattleGroundABScore*)itr->second)->BasesAssaulted);      // bases asssulted
                buffer << uint32(((BattleGroundABScore*)itr->second)->BasesDefended);       // bases defended
                break;
            case BATTLEGROUND_EY:
                data->WriteBits(1, 24);                     // count of next fields
                buffer << uint32(((BattleGroundEYScore*)itr->second)->FlagCaptures);        // flag captures
                break;
            case BATTLEGROUND_NA:
            case BATTLEGROUND_BE:
            case BATTLEGROUND_AA:
            case BATTLEGROUND_RL:
            case BATTLEGROUND_SA:                           // wotlk
            case BATTLEGROUND_DS:                           // wotlk
            case BATTLEGROUND_RV:                           // wotlk
            case BATTLEGROUND_IC:                           // wotlk
            case BATTLEGROUND_RB:                           // wotlk
                data->WriteBits(0, 24);                     // count of next fields
                break;
             default:
                sLog.outError("Unhandled SMSG_PVP_LOG_DATA for BG id %u", bg->GetTypeID());
                data->WriteBits(0, 24);                     // count of next fields
                break;
        }

        data->WriteGuidMask<4>(memberGuid);

        buffer.WriteGuidBytes<0, 3>(memberGuid);
        // if (unk4) << uint32() unk
        buffer.WriteGuidBytes<7, 2>(memberGuid);
    }

    data->WriteBit(bg->GetStatus() == STATUS_WAIT_LEAVE);   // If Ended

    if (bg->isRated())                                      // arena
    {
        for (int8 i = 0; i < BG_TEAMS_COUNT; ++i)
        {
            uint32 pointsLost = bg->m_ArenaTeamRatingChanges[i] < 0 ? abs(bg->m_ArenaTeamRatingChanges[i]) : 0;
            uint32 pointsGained = bg->m_ArenaTeamRatingChanges[i] > 0 ? bg->m_ArenaTeamRatingChanges[i] : 0;

            *data << uint32(0);                             // Matchmaking Value
            *data << uint32(pointsLost);                    // Rating Lost
            *data << uint32(pointsGained);                  // Rating gained
            DEBUG_LOG("rating change: %d", bg->m_ArenaTeamRatingChanges[i]);
        }
    }

    data->FlushBits();
    data->append(buffer);

    if (bg->isArena())
    {
        for (int8 i = 0; i < BG_TEAMS_COUNT; ++i)
        {
            if (ArenaTeam* at = sObjectMgr.GetArenaTeamById(bg->m_ArenaTeamIds[i]))
                data->append(at->GetName().data(), at->GetName().length());
        }
    }

    *data << uint8(bg->GetPlayersCountByTeam(HORDE));

    if (bg->GetStatus() == STATUS_WAIT_LEAVE)
        *data << uint8(bg->GetWinner() == ALLIANCE);        // who win

    *data << uint8(bg->GetPlayersCountByTeam(ALLIANCE));
}

void BattleGroundMgr::BuildBattleGroundStatusFailedPacket(WorldPacket* data, BattleGround* bg, Player* player, uint8 QueueSlot, GroupJoinBattlegroundResult result)
{
    ObjectGuid bgGuid = bg->GetObjectGuid();
    ObjectGuid unkGuid2 = ObjectGuid();
    ObjectGuid playerGuid = player ? player->GetObjectGuid() : ObjectGuid(); // player who caused the error

    DEBUG_LOG("BattleGroundMgr::BuildBattleGroundStatusFailedPacket slot %u result %u bgstatus %u player %s bg %s",
        QueueSlot, result, bg->GetStatus(), playerGuid.GetString().c_str(), bgGuid.GetString().c_str());

    data->Initialize(SMSG_BATTLEFIELD_STATUS_FAILED);

    data->WriteGuidMask<3>(bgGuid);
    data->WriteGuidMask<3>(playerGuid);
    data->WriteGuidMask<3>(unkGuid2);
    data->WriteGuidMask<0>(playerGuid);
    data->WriteGuidMask<6>(bgGuid);
    data->WriteGuidMask<5, 6, 4, 2>(unkGuid2);

    data->WriteGuidMask<1>(playerGuid);
    data->WriteGuidMask<1>(bgGuid);
    data->WriteGuidMask<5, 6>(playerGuid);
    data->WriteGuidMask<1>(unkGuid2);
    data->WriteGuidMask<7>(bgGuid);
    data->WriteGuidMask<4>(playerGuid);

    data->WriteGuidMask<2, 5>(bgGuid);
    data->WriteGuidMask<7>(playerGuid);
    data->WriteGuidMask<4, 0>(bgGuid);
    data->WriteGuidMask<0>(unkGuid2);
    data->WriteGuidMask<2>(playerGuid);
    data->WriteGuidMask<7>(unkGuid2);

    data->WriteGuidBytes<1>(bgGuid);

    *data << uint32(QueueSlot);         // not queue slot
    *data << uint32(QueueSlot);         // Queue slot

    data->WriteGuidBytes<6, 3, 7, 4>(unkGuid2);
    data->WriteGuidBytes<0>(bgGuid);
    data->WriteGuidBytes<5>(unkGuid2);
    data->WriteGuidBytes<7, 6, 2>(bgGuid);
    data->WriteGuidBytes<6, 3>(playerGuid);
    data->WriteGuidBytes<1>(unkGuid2);
    data->WriteGuidBytes<3>(bgGuid);
    data->WriteGuidBytes<0, 1, 4>(playerGuid);
    data->WriteGuidBytes<0>(unkGuid2);
    data->WriteGuidBytes<5>(bgGuid);
    data->WriteGuidBytes<7>(playerGuid);
    data->WriteGuidBytes<4>(bgGuid);
    data->WriteGuidBytes<2>(unkGuid2);

    *data << uint32(result);            // Result

    data->WriteGuidBytes<2>(playerGuid);

    *data << uint32(0);                 // unk Time

    data->WriteGuidBytes<5>(playerGuid);
}

void BattleGroundMgr::BuildUpdateWorldStatePacket(WorldPacket* data, uint32 field, uint32 value)
{
    data->Initialize(SMSG_UPDATE_WORLD_STATE, 4 + 4);
    *data << uint32(field);
    *data << uint32(value);
}

void BattleGroundMgr::BuildPlaySoundPacket(WorldPacket* data, uint32 soundid)
{
    data->Initialize(SMSG_PLAY_SOUND, 4);
    *data << uint32(soundid);
}

void BattleGroundMgr::BuildPlayerLeftBattleGroundPacket(WorldPacket* data, ObjectGuid guid)
{
    data->Initialize(SMSG_BATTLEGROUND_PLAYER_LEFT, 8);
    data->WriteGuidMask<7, 6, 2, 4, 5, 1, 3, 0>(guid);
    data->WriteGuidBytes<4, 2, 5, 7, 0, 6, 1, 3>(guid);
}

void BattleGroundMgr::BuildPlayerJoinedBattleGroundPacket(WorldPacket* data, Player* plr)
{
    data->Initialize(SMSG_BATTLEGROUND_PLAYER_JOINED, 8);
    data->WriteGuidMask<0, 4, 3, 5, 7, 6, 2, 1>(plr->GetObjectGuid());
    data->WriteGuidBytes<1, 5, 3, 2, 0, 7, 4, 6>(plr->GetObjectGuid());
}

BattleGround* BattleGroundMgr::GetBattleGroundThroughClientInstance(uint32 instanceId, BattleGroundTypeId bgTypeId)
{
    // cause at HandleBattleGroundJoinOpcode the clients sends the instanceid he gets from
    // SMSG_BATTLEFIELD_LIST we need to find the battleground with this clientinstance-id
    BattleGround* bg = GetBattleGroundTemplate(bgTypeId);
    if (!bg)
        return NULL;

    if (bg->isArena())
        return GetBattleGround(instanceId, bgTypeId);

    for (BattleGroundSet::iterator itr = m_BattleGrounds[bgTypeId].begin(); itr != m_BattleGrounds[bgTypeId].end(); ++itr)
    {
        if (itr->second->GetClientInstanceID() == instanceId)
            return itr->second;
    }
    return NULL;
}

BattleGround* BattleGroundMgr::GetBattleGround(uint32 InstanceID, BattleGroundTypeId bgTypeId)
{
    // search if needed
    BattleGroundSet::iterator itr;
    if (bgTypeId == BATTLEGROUND_TYPE_NONE)
    {
        for (uint8 i = BATTLEGROUND_AV; i < MAX_BATTLEGROUND_TYPE_ID; ++i)
        {
            itr = m_BattleGrounds[i].find(InstanceID);
            if (itr != m_BattleGrounds[i].end())
                return itr->second;
        }
        return NULL;
    }
    itr = m_BattleGrounds[bgTypeId].find(InstanceID);
    return ((itr != m_BattleGrounds[bgTypeId].end()) ? itr->second : NULL);
}

BattleGround* BattleGroundMgr::GetBattleGroundTemplate(BattleGroundTypeId bgTypeId)
{
    // map is sorted and we can be sure that lowest instance id has only BG template
    return m_BattleGrounds[bgTypeId].empty() ? NULL : m_BattleGrounds[bgTypeId].begin()->second;
}

uint32 BattleGroundMgr::CreateClientVisibleInstanceId(BattleGroundTypeId bgTypeId, BattleGroundBracketId bracket_id)
{
    if (IsArenaType(bgTypeId))
        return 0;                                           // arenas don't have client-instanceids

    // we create here an instanceid, which is just for
    // displaying this to the client and without any other use..
    // the client-instanceIds are unique for each battleground-type
    // the instance-id just needs to be as low as possible, beginning with 1
    // the following works, because std::set is default ordered with "<"
    // the optimalization would be to use as bitmask std::vector<uint32> - but that would only make code unreadable
    uint32 lastId = 0;
    ClientBattleGroundIdSet& ids = m_ClientBattleGroundIds[bgTypeId][bracket_id];
    for (ClientBattleGroundIdSet::const_iterator itr = ids.begin(); itr != ids.end();)
    {
        if ((++lastId) != *itr)                             // if there is a gap between the ids, we will break..
            break;
        lastId = *itr;
    }
    ids.insert(lastId + 1);
    return lastId + 1;
}

// create a new battleground that will really be used to play
BattleGround* BattleGroundMgr::CreateNewBattleGround(BattleGroundTypeId bgTypeId, PvPDifficultyEntry const* bracketEntry, ArenaType arenaType, bool isRated)
{
    // get the template BG
    BattleGround* bg_template = GetBattleGroundTemplate(bgTypeId);
    if (!bg_template)
    {
        sLog.outError("BattleGround: CreateNewBattleGround - bg template not found for %u", bgTypeId);
        return NULL;
    }

    // for arenas there is random map used
    if (bg_template->isArena())
    {
        BattleGroundTypeId arenas[] = { BATTLEGROUND_NA, BATTLEGROUND_BE, BATTLEGROUND_RL/*, BATTLEGROUND_DS, BATTLEGROUND_RV*/ };
        bgTypeId = arenas[urand(0, countof(arenas) - 1)];
        bg_template = GetBattleGroundTemplate(bgTypeId);
        if (!bg_template)
        {
            sLog.outError("BattleGround: CreateNewBattleGround - bg template not found for %u", bgTypeId);
            return NULL;
        }
    }

    BattleGround* bg = NULL;
    // create a copy of the BG template
    switch (bgTypeId)
    {
        case BATTLEGROUND_AV:
            bg = new BattleGroundAV(*(BattleGroundAV*)bg_template);
            break;
        case BATTLEGROUND_WS:
            bg = new BattleGroundWS(*(BattleGroundWS*)bg_template);
            break;
        case BATTLEGROUND_AB:
            bg = new BattleGroundAB(*(BattleGroundAB*)bg_template);
            break;
        case BATTLEGROUND_NA:
            bg = new BattleGroundNA(*(BattleGroundNA*)bg_template);
            break;
        case BATTLEGROUND_BE:
            bg = new BattleGroundBE(*(BattleGroundBE*)bg_template);
            break;
        case BATTLEGROUND_AA:
            bg = new BattleGroundAA(*(BattleGroundAA*)bg_template);
            break;
        case BATTLEGROUND_EY:
            bg = new BattleGroundEY(*(BattleGroundEY*)bg_template);
            break;
        case BATTLEGROUND_RL:
            bg = new BattleGroundRL(*(BattleGroundRL*)bg_template);
            break;
        case BATTLEGROUND_SA:
            bg = new BattleGroundSA(*(BattleGroundSA*)bg_template);
            break;
        case BATTLEGROUND_DS:
            bg = new BattleGroundDS(*(BattleGroundDS*)bg_template);
            break;
        case BATTLEGROUND_RV:
            bg = new BattleGroundRV(*(BattleGroundRV*)bg_template);
            break;
        case BATTLEGROUND_IC:
            bg = new BattleGroundIC(*(BattleGroundIC*)bg_template);
            break;
        case BATTLEGROUND_RB:
            bg = new BattleGroundRB(*(BattleGroundRB*)bg_template);
            break;
        default:
            // error, but it is handled few lines above
            return 0;
    }

    // set before Map creating for let use proper difficulty
    bg->SetBracket(bracketEntry);

    // will also set m_bgMap, instanceid
    sMapMgr.CreateBgMap(bg->GetMapId(), bg);

    bg->SetClientInstanceID(CreateClientVisibleInstanceId(bgTypeId, bracketEntry->GetBracketId()));

    // reset the new bg (set status to status_wait_queue from status_none)
    bg->Reset();

    // start the joining of the bg
    bg->SetStatus(STATUS_WAIT_JOIN);
    bg->SetArenaType(arenaType);
    bg->SetRated(isRated);

    return bg;
}

// used to create the BG templates
uint32 BattleGroundMgr::CreateBattleGround(BattleGroundTypeId bgTypeId, bool IsArena, uint32 MinPlayersPerTeam, uint32 MaxPlayersPerTeam, uint32 LevelMin, uint32 LevelMax, char const* BattleGroundName, uint32 MapID, float Team1StartLocX, float Team1StartLocY, float Team1StartLocZ, float Team1StartLocO, float Team2StartLocX, float Team2StartLocY, float Team2StartLocZ, float Team2StartLocO)
{
    // Create the BG
    BattleGround* bg = NULL;
    switch (bgTypeId)
    {
        case BATTLEGROUND_AV: bg = new BattleGroundAV; break;
        case BATTLEGROUND_WS: bg = new BattleGroundWS; break;
        case BATTLEGROUND_AB: bg = new BattleGroundAB; break;
        case BATTLEGROUND_NA: bg = new BattleGroundNA; break;
        case BATTLEGROUND_BE: bg = new BattleGroundBE; break;
        case BATTLEGROUND_AA: bg = new BattleGroundAA; break;
        case BATTLEGROUND_EY: bg = new BattleGroundEY; break;
        case BATTLEGROUND_RL: bg = new BattleGroundRL; break;
        case BATTLEGROUND_SA: bg = new BattleGroundSA; break;
        case BATTLEGROUND_DS: bg = new BattleGroundDS; break;
        case BATTLEGROUND_RV: bg = new BattleGroundRV; break;
        case BATTLEGROUND_IC: bg = new BattleGroundIC; break;
        case BATTLEGROUND_RB: bg = new BattleGroundRB; break;
        default:              bg = new BattleGround;   break;                           // placeholder for non implemented BG
    }

    bg->SetMapId(MapID);
    bg->SetTypeID(bgTypeId);
    bg->SetArenaorBGType(IsArena);
    bg->SetMinPlayersPerTeam(MinPlayersPerTeam);
    bg->SetMaxPlayersPerTeam(MaxPlayersPerTeam);
    bg->SetMinPlayers(MinPlayersPerTeam * 2);
    bg->SetMaxPlayers(MaxPlayersPerTeam * 2);
    bg->SetName(BattleGroundName);
    bg->SetTeamStartLoc(ALLIANCE, Team1StartLocX, Team1StartLocY, Team1StartLocZ, Team1StartLocO);
    bg->SetTeamStartLoc(HORDE,    Team2StartLocX, Team2StartLocY, Team2StartLocZ, Team2StartLocO);
    bg->SetLevelRange(LevelMin, LevelMax);

    // add bg to update list
    AddBattleGround(bg->GetInstanceID(), bg->GetTypeID(), bg);

    // return some not-null value, bgTypeId is good enough for me
    return bgTypeId;
}

void BattleGroundMgr::CreateInitialBattleGrounds()
{
    uint32 count = 0;

    //                                                0   1                 2                 3                4              5             6
    QueryResult* result = WorldDatabase.Query("SELECT id, MinPlayersPerTeam,MaxPlayersPerTeam,AllianceStartLoc,AllianceStartO,HordeStartLoc,HordeStartO FROM battleground_template");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 battlegrounds. DB table `battleground_template` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 bgTypeID_ = fields[0].GetUInt32();

        // can be overwrite by values from DB
        BattlemasterListEntry const* bl = sBattlemasterListStore.LookupEntry(bgTypeID_);
        if (!bl)
        {
            sLog.outError("Battleground ID %u not found in BattlemasterList.dbc. Battleground not created.", bgTypeID_);
            continue;
        }

        BattleGroundTypeId bgTypeID = BattleGroundTypeId(bgTypeID_);

        bool IsArena = (bl->type == TYPE_ARENA);
        uint32 MinPlayersPerTeam = fields[1].GetUInt32();
        uint32 MaxPlayersPerTeam = fields[2].GetUInt32();

        // check values from DB
        if (MaxPlayersPerTeam == 0 || MinPlayersPerTeam == 0)
        {
            sLog.outErrorDb("Table `battleground_template` for id %u have wrong min/max players per team settings. BG not created.", bgTypeID);
            continue;
        }

        if (MinPlayersPerTeam > MaxPlayersPerTeam)
            MinPlayersPerTeam = MaxPlayersPerTeam;

        float AStartLoc[4];
        float HStartLoc[4];

        uint32 start1 = fields[3].GetUInt32();

        WorldSafeLocsEntry const* start = sWorldSafeLocsStore.LookupEntry(start1);
        if (start)
        {
            AStartLoc[0] = start->x;
            AStartLoc[1] = start->y;
            AStartLoc[2] = start->z;
            AStartLoc[3] = fields[4].GetFloat();
        }
        else if (bgTypeID == BATTLEGROUND_AA || bgTypeID == BATTLEGROUND_RB)
        {
            AStartLoc[0] = 0;
            AStartLoc[1] = 0;
            AStartLoc[2] = 0;
            AStartLoc[3] = fields[4].GetFloat();
        }
        else
        {
            sLog.outErrorDb("Table `battleground_template` for id %u have nonexistent WorldSafeLocs.dbc id %u in field `AllianceStartLoc`. BG not created.", bgTypeID, start1);
            continue;
        }

        uint32 start2 = fields[5].GetUInt32();

        start = sWorldSafeLocsStore.LookupEntry(start2);
        if (start)
        {
            HStartLoc[0] = start->x;
            HStartLoc[1] = start->y;
            HStartLoc[2] = start->z;
            HStartLoc[3] = fields[6].GetFloat();
        }
        else if (bgTypeID == BATTLEGROUND_AA || bgTypeID == BATTLEGROUND_RB)
        {
            HStartLoc[0] = 0;
            HStartLoc[1] = 0;
            HStartLoc[2] = 0;
            HStartLoc[3] = fields[6].GetFloat();
        }
        else
        {
            sLog.outErrorDb("Table `battleground_template` for id %u have nonexistent WorldSafeLocs.dbc id %u in field `HordeStartLoc`. BG not created.", bgTypeID, start2);
            continue;
        }

        // sLog.outDetail("Creating battleground %s, %u-%u", bl->name[sWorld.GetDBClang()], MinLvl, MaxLvl);
        if (!CreateBattleGround(bgTypeID, IsArena, MinPlayersPerTeam, MaxPlayersPerTeam, bl->minLevel, bl->maxLevel, bl->name[sWorld.GetDefaultDbcLocale()], bl->mapid[0], AStartLoc[0], AStartLoc[1], AStartLoc[2], AStartLoc[3], HStartLoc[0], HStartLoc[1], HStartLoc[2], HStartLoc[3]))
            continue;

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u battlegrounds", count);
}

void BattleGroundMgr::BuildBattleGroundListPacket(WorldPacket* data, ObjectGuid guid, Player* plr, BattleGroundTypeId bgTypeId)
{
    if (!plr)
        return;

    BattleGround* bgTemplate = sBattleGroundMgr.GetBattleGroundTemplate(bgTypeId);

    data->Initialize(SMSG_BATTLEFIELD_LIST);
    *data << uint32(0);                     // 4.3.4 winConquest weekend
    *data << uint32(0);                     // 4.3.4 winConquest random
    *data << uint32(0);                     // 4.3.4 lossHonor weekend
    *data << uint32(bgTypeId);              // battleground id
    *data << uint32(0);                     // 4.3.4 lossHonor random
    *data << uint32(0);                     // 4.3.4 winHonor random
    *data << uint32(0);                     // 4.3.4 winHonor weekend
    *data << uint8(0);                      // max level
    *data << uint8(0);                      // min level
    data->WriteGuidMask<0, 1, 7>(guid);
    data->WriteBit(false);                  // has holiday bg currency bonus ??
    data->WriteBit(false);                  // has random bg currency bonus ??

    uint32 count = 0;
    ByteBuffer buf;
    if (bgTemplate)
    {
        // expected bracket entry
        if (PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bgTemplate->GetMapId(), plr->getLevel()))
        {
            BattleGroundBracketId bracketId = bracketEntry->GetBracketId();
            ClientBattleGroundIdSet const& ids = m_ClientBattleGroundIds[bgTypeId][bracketId];
            for (ClientBattleGroundIdSet::const_iterator itr = ids.begin(); itr != ids.end(); ++itr)
            {
                buf << uint32(*itr);
                ++count;
            }
        }
    }

    data->WriteBits(count, 24);
    data->WriteGuidMask<6, 4, 2, 3>(guid);
    data->WriteBit(true);                   // unk
    data->WriteGuidMask<5>(guid);
    data->WriteBit(true);                   // signals EVENT_PVPQUEUE_ANYWHERE_SHOW if set

    data->WriteGuidBytes<6, 1, 7, 5>(guid);
    data->FlushBits();
    if (count)
        data->append(buf);
    data->WriteGuidBytes<0, 2, 4, 3>(guid);
}

void BattleGroundMgr::SendToBattleGround(Player* pl, uint32 instanceId, BattleGroundTypeId bgTypeId)
{
    BattleGround* bg = GetBattleGround(instanceId, bgTypeId);
    if (bg)
    {
        uint32 mapid = bg->GetMapId();
        float x, y, z, O;
        Team team = pl->GetBGTeam();
        if (team == 0)
            team = pl->GetTeam();
        bg->GetTeamStartLoc(team, x, y, z, O);

        DETAIL_LOG("BATTLEGROUND: Sending %s to map %u, X %f, Y %f, Z %f, O %f", pl->GetName(), mapid, x, y, z, O);
        pl->TeleportTo(mapid, x, y, z, O);
    }
    else
    {
        sLog.outError("player %u trying to port to nonexistent bg instance %u", pl->GetGUIDLow(), instanceId);
    }
}

bool BattleGroundMgr::IsArenaType(BattleGroundTypeId bgTypeId)
{
    switch (bgTypeId)
    {
        case BATTLEGROUND_NA:
        case BATTLEGROUND_BE:
        case BATTLEGROUND_RL:
        case BATTLEGROUND_DS:
        case BATTLEGROUND_RV:
        case BATTLEGROUND_AA:
            return true;
        default:
            return false;
    }
}

BattleGroundQueueTypeId BattleGroundMgr::BGQueueTypeId(BattleGroundTypeId bgTypeId, ArenaType arenaType)
{
    switch (bgTypeId)
    {
        case BATTLEGROUND_WS:
            return BATTLEGROUND_QUEUE_WS;
        case BATTLEGROUND_AB:
            return BATTLEGROUND_QUEUE_AB;
        case BATTLEGROUND_AV:
            return BATTLEGROUND_QUEUE_AV;
        case BATTLEGROUND_EY:
            return BATTLEGROUND_QUEUE_EY;
        case BATTLEGROUND_SA:
            return BATTLEGROUND_QUEUE_SA;
        case BATTLEGROUND_IC:
            return BATTLEGROUND_QUEUE_IC;
        case BATTLEGROUND_RB:
            return BATTLEGROUND_QUEUE_NONE;
        case BATTLEGROUND_TP:
            return BATTLEGROUND_QUEUE_TP;
        case BATTLEGROUND_BG:
            return BATTLEGROUND_QUEUE_BG;
        case BATTLEGROUND_AA:
        case BATTLEGROUND_NA:
        case BATTLEGROUND_RL:
        case BATTLEGROUND_BE:
        case BATTLEGROUND_DS:
        case BATTLEGROUND_RV:
            switch (arenaType)
            {
                case ARENA_TYPE_2v2:
                    return BATTLEGROUND_QUEUE_2v2;
                case ARENA_TYPE_3v3:
                    return BATTLEGROUND_QUEUE_3v3;
                case ARENA_TYPE_5v5:
                    return BATTLEGROUND_QUEUE_5v5;
                default:
                    return BATTLEGROUND_QUEUE_NONE;
            }
        default:
            return BATTLEGROUND_QUEUE_NONE;
    }
}

BattleGroundTypeId BattleGroundMgr::BGTemplateId(BattleGroundQueueTypeId bgQueueTypeId)
{
    switch (bgQueueTypeId)
    {
        case BATTLEGROUND_QUEUE_WS:
            return BATTLEGROUND_WS;
        case BATTLEGROUND_QUEUE_AB:
            return BATTLEGROUND_AB;
        case BATTLEGROUND_QUEUE_AV:
            return BATTLEGROUND_AV;
        case BATTLEGROUND_QUEUE_EY:
            return BATTLEGROUND_EY;
        case BATTLEGROUND_QUEUE_SA:
            return BATTLEGROUND_SA;
        case BATTLEGROUND_QUEUE_IC:
            return BATTLEGROUND_IC;
        case BATTLEGROUND_QUEUE_TP:
            return BATTLEGROUND_TP;
        case BATTLEGROUND_QUEUE_BG:
            return BATTLEGROUND_BG;
        case BATTLEGROUND_QUEUE_2v2:
        case BATTLEGROUND_QUEUE_3v3:
        case BATTLEGROUND_QUEUE_5v5:
            return BATTLEGROUND_AA;
        default:
            return BattleGroundTypeId(0);                   // used for unknown template (it exist and do nothing)
    }
}

ArenaType BattleGroundMgr::BGArenaType(BattleGroundQueueTypeId bgQueueTypeId)
{
    switch (bgQueueTypeId)
    {
        case BATTLEGROUND_QUEUE_2v2:
            return ARENA_TYPE_2v2;
        case BATTLEGROUND_QUEUE_3v3:
            return ARENA_TYPE_3v3;
        case BATTLEGROUND_QUEUE_5v5:
            return ARENA_TYPE_5v5;
        default:
            return ARENA_TYPE_NONE;
    }
}

void BattleGroundMgr::ToggleTesting()
{
    m_Testing = !m_Testing;
    if (m_Testing)
        sWorld.SendWorldText(LANG_DEBUG_BG_ON);
    else
        sWorld.SendWorldText(LANG_DEBUG_BG_OFF);
}

void BattleGroundMgr::ToggleArenaTesting()
{
    m_ArenaTesting = !m_ArenaTesting;
    if (m_ArenaTesting)
        sWorld.SendWorldText(LANG_DEBUG_ARENA_ON);
    else
        sWorld.SendWorldText(LANG_DEBUG_ARENA_OFF);
}

void BattleGroundMgr::ScheduleQueueUpdate(uint32 arenaRating, ArenaType arenaType, BattleGroundQueueTypeId bgQueueTypeId, BattleGroundTypeId bgTypeId, BattleGroundBracketId bracket_id)
{
    // ACE_Guard<ACE_Thread_Mutex> guard(SchedulerLock);
    // we will use only 1 number created of bgTypeId and bracket_id
    uint64 schedule_id = ((uint64)arenaRating << 32) | (arenaType << 24) | (bgQueueTypeId << 16) | (bgTypeId << 8) | bracket_id;
    bool found = false;
    for (uint8 i = 0; i < m_QueueUpdateScheduler.size(); ++i)
    {
        if (m_QueueUpdateScheduler[i] == schedule_id)
        {
            found = true;
            break;
        }
    }
    if (!found)
        m_QueueUpdateScheduler.push_back(schedule_id);
}

uint32 BattleGroundMgr::GetMaxRatingDifference() const
{
    // this is for stupid people who can't use brain and set max rating difference to 0
    uint32 diff = sWorld.getConfig(CONFIG_UINT32_ARENA_MAX_RATING_DIFFERENCE);
    if (diff == 0)
        diff = 5000;
    return diff;
}

uint32 BattleGroundMgr::GetRatingDiscardTimer() const
{
    return sWorld.getConfig(CONFIG_UINT32_ARENA_RATING_DISCARD_TIMER);
}

uint32 BattleGroundMgr::GetPrematureFinishTime() const
{
    return sWorld.getConfig(CONFIG_UINT32_BATTLEGROUND_PREMATURE_FINISH_TIMER);
}

void BattleGroundMgr::LoadBattleMastersEntry()
{
    mBattleMastersMap.clear();                              // need for reload case

    QueryResult* result = WorldDatabase.Query("SELECT entry,bg_template FROM battlemaster_entry");

    uint32 count = 0;

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded 0 battlemaster entries - table is empty!");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        ++count;
        bar.step();

        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();
        uint32 bgTypeId  = fields[1].GetUInt32();
        if (!sBattlemasterListStore.LookupEntry(bgTypeId))
        {
            sLog.outErrorDb("Table `battlemaster_entry` contain entry %u for nonexistent battleground type %u, ignored.", entry, bgTypeId);
            continue;
        }

        mBattleMastersMap[entry] = BattleGroundTypeId(bgTypeId);

    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u battlemaster entries", count);
}

HolidayIds BattleGroundMgr::BGTypeToWeekendHolidayId(BattleGroundTypeId bgTypeId)
{
    switch (bgTypeId)
    {
        case BATTLEGROUND_AV: return HOLIDAY_CALL_TO_ARMS_AV;
        case BATTLEGROUND_WS: return HOLIDAY_CALL_TO_ARMS_WS;
        case BATTLEGROUND_AB: return HOLIDAY_CALL_TO_ARMS_AB;
        case BATTLEGROUND_EY: return HOLIDAY_CALL_TO_ARMS_EY;
        case BATTLEGROUND_SA: return HOLIDAY_CALL_TO_ARMS_SA;
        case BATTLEGROUND_IC: return HOLIDAY_CALL_TO_ARMS_IC;
        case BATTLEGROUND_TP: return HOLIDAY_CALL_TO_ARMS_TP;
        case BATTLEGROUND_BG: return HOLIDAY_CALL_TO_ARMS_BG;
        default: return HOLIDAY_NONE;
    }
}

BattleGroundTypeId BattleGroundMgr::WeekendHolidayIdToBGType(HolidayIds holiday)
{
    switch (holiday)
    {
        case HOLIDAY_CALL_TO_ARMS_AV: return BATTLEGROUND_AV;
        case HOLIDAY_CALL_TO_ARMS_WS: return BATTLEGROUND_WS;
        case HOLIDAY_CALL_TO_ARMS_AB: return BATTLEGROUND_AB;
        case HOLIDAY_CALL_TO_ARMS_EY: return BATTLEGROUND_EY;
        case HOLIDAY_CALL_TO_ARMS_SA: return BATTLEGROUND_SA;
        case HOLIDAY_CALL_TO_ARMS_IC: return BATTLEGROUND_IC;
        case HOLIDAY_CALL_TO_ARMS_TP: return BATTLEGROUND_TP;
        case HOLIDAY_CALL_TO_ARMS_BG: return BATTLEGROUND_BG;
        default: return BATTLEGROUND_TYPE_NONE;
    }
}

bool BattleGroundMgr::IsBGWeekend(BattleGroundTypeId bgTypeId)
{
    return sGameEventMgr.IsActiveHoliday(BGTypeToWeekendHolidayId(bgTypeId));
}

void BattleGroundMgr::LoadBattleEventIndexes()
{
    BattleGroundEventIdx events;
    events.event1 = BG_EVENT_NONE;
    events.event2 = BG_EVENT_NONE;
    m_GameObjectBattleEventIndexMap.clear();             // need for reload case
    m_GameObjectBattleEventIndexMap[-1] = events;
    m_CreatureBattleEventIndexMap.clear();               // need for reload case
    m_CreatureBattleEventIndexMap[-1] = events;

    uint32 count = 0;

    QueryResult* result =
        //                           0         1           2                3                4              5           6
        WorldDatabase.Query("SELECT data.typ, data.guid1, data.ev1 AS ev1, data.ev2 AS ev2, data.map AS m, data.guid2, description.map, "
                            //                              7                  8                   9
                            "description.event1, description.event2, description.description "
                            "FROM "
                            "(SELECT '1' AS typ, a.guid AS guid1, a.event1 AS ev1, a.event2 AS ev2, b.map AS map, b.guid AS guid2 "
                            "FROM gameobject_battleground AS a "
                            "LEFT OUTER JOIN gameobject AS b ON a.guid = b.guid "
                            "UNION "
                            "SELECT '2' AS typ, a.guid AS guid1, a.event1 AS ev1, a.event2 AS ev2, b.map AS map, b.guid AS guid2 "
                            "FROM creature_battleground AS a "
                            "LEFT OUTER JOIN creature AS b ON a.guid = b.guid "
                            ") data "
                            "RIGHT OUTER JOIN battleground_events AS description ON data.map = description.map "
                            "AND data.ev1 = description.event1 AND data.ev2 = description.event2 "
                            // full outer join doesn't work in mysql :-/ so just UNION-select the same again and add a left outer join
                            "UNION "
                            "SELECT data.typ, data.guid1, data.ev1, data.ev2, data.map, data.guid2, description.map, "
                            "description.event1, description.event2, description.description "
                            "FROM "
                            "(SELECT '1' AS typ, a.guid AS guid1, a.event1 AS ev1, a.event2 AS ev2, b.map AS map, b.guid AS guid2 "
                            "FROM gameobject_battleground AS a "
                            "LEFT OUTER JOIN gameobject AS b ON a.guid = b.guid "
                            "UNION "
                            "SELECT '2' AS typ, a.guid AS guid1, a.event1 AS ev1, a.event2 AS ev2, b.map AS map, b.guid AS guid2 "
                            "FROM creature_battleground AS a "
                            "LEFT OUTER JOIN creature AS b ON a.guid = b.guid "
                            ") data "
                            "LEFT OUTER JOIN battleground_events AS description ON data.map = description.map "
                            "AND data.ev1 = description.event1 AND data.ev2 = description.event2 "
                            "ORDER BY m, ev1, ev2");
    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 battleground eventindexes.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        bar.step();
        Field* fields = result->Fetch();
        if (fields[2].GetUInt8() == BG_EVENT_NONE || fields[3].GetUInt8() == BG_EVENT_NONE)
            continue;                                       // we don't need to add those to the eventmap

        bool gameobject         = (fields[0].GetUInt8() == 1);
        uint32 dbTableGuidLow   = fields[1].GetUInt32();
        events.event1           = fields[2].GetUInt8();
        events.event2           = fields[3].GetUInt8();
        uint32 map              = fields[4].GetUInt32();

        uint32 desc_map = fields[6].GetUInt32();
        uint8 desc_event1 = fields[7].GetUInt8();
        uint8 desc_event2 = fields[8].GetUInt8();
        const char* description = fields[9].GetString();

        // checking for NULL - through right outer join this will mean following:
        if (fields[5].GetUInt32() != dbTableGuidLow)
        {
            sLog.outErrorDb("BattleGroundEvent: %s with nonexistent guid %u for event: map:%u, event1:%u, event2:%u (\"%s\")",
                            (gameobject) ? "gameobject" : "creature", dbTableGuidLow, map, events.event1, events.event2, description);
            continue;
        }

        // checking for NULL - through full outer join this can mean 2 things:
        if (desc_map != map)
        {
            // there is an event missing
            if (dbTableGuidLow == 0)
            {
                sLog.outErrorDb("BattleGroundEvent: missing db-data for map:%u, event1:%u, event2:%u (\"%s\")", desc_map, desc_event1, desc_event2, description);
                continue;
            }
            // we have an event which shouldn't exist
            else
            {
                sLog.outErrorDb("BattleGroundEvent: %s with guid %u is registered, for a nonexistent event: map:%u, event1:%u, event2:%u",
                                (gameobject) ? "gameobject" : "creature", dbTableGuidLow, map, events.event1, events.event2);
                continue;
            }
        }

        if (gameobject)
            m_GameObjectBattleEventIndexMap[dbTableGuidLow] = events;
        else
            m_CreatureBattleEventIndexMap[dbTableGuidLow] = events;

        ++count;

    }
    while (result->NextRow());

    sLog.outString();
    sLog.outString(">> Loaded %u battleground eventindexes", count);
    delete result;
}
