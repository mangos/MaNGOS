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
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "WorldSession.h"
#include "Object.h"
#include "Chat.h"
#include "BattleGroundMgr.h"
#include "BattleGroundWS.h"
#include "BattleGround.h"
#include "ArenaTeam.h"
#include "Language.h"
#include "ScriptMgr.h"
#include "World.h"

void WorldSession::HandleBattlemasterHelloOpcode(WorldPacket & recv_data)
{
    ObjectGuid guid;
    recv_data >> guid;

    DEBUG_LOG("WORLD: Recvd CMSG_BATTLEMASTER_HELLO Message from %s", guid.GetString().c_str());

    Creature *pCreature = GetPlayer()->GetMap()->GetCreature(guid);

    if (!pCreature)
        return;

    if (!pCreature->isBattleMaster())                       // it's not battlemaster
        return;

    // Stop the npc if moving
    if (!pCreature->IsStopped())
        pCreature->StopMoving();

    BattleGroundTypeId bgTypeId = sBattleGroundMgr.GetBattleMasterBG(pCreature->GetEntry());

    if (bgTypeId == BATTLEGROUND_TYPE_NONE)
        return;

    if (!_player->GetBGAccessByLevel(bgTypeId))
    {
        // temp, must be gossip message...
        SendNotification(LANG_YOUR_BG_LEVEL_REQ_ERROR);
        return;
    }

    SendBattlegGroundList(guid, bgTypeId);
}

void WorldSession::SendBattlegGroundList( ObjectGuid guid, BattleGroundTypeId bgTypeId )
{
    WorldPacket data;
    sBattleGroundMgr.BuildBattleGroundListPacket(&data, guid, _player, bgTypeId, 0);
    SendPacket( &data );
}

void WorldSession::HandleBattlemasterJoinOpcode( WorldPacket & recv_data )
{
    ObjectGuid guid;
    uint32 bgTypeId_;
    uint32 instanceId;
    uint8 joinAsGroup;
    bool isPremade = false;
    Group * grp;

    recv_data >> guid;                                      // battlemaster guid
    recv_data >> bgTypeId_;                                 // battleground type id (DBC id)
    recv_data >> instanceId;                                // instance id, 0 if First Available selected
    recv_data >> joinAsGroup;                               // join as group

    if (!sBattlemasterListStore.LookupEntry(bgTypeId_))
    {
        sLog.outError("Battleground: invalid bgtype (%u) received. possible cheater? player guid %u",bgTypeId_,_player->GetGUIDLow());
        return;
    }

    BattleGroundTypeId bgTypeId = BattleGroundTypeId(bgTypeId_);

    DEBUG_LOG( "WORLD: Recvd CMSG_BATTLEMASTER_JOIN Message from %s", guid.GetString().c_str());

    // can do this, since it's battleground, not arena
    BattleGroundQueueTypeId bgQueueTypeId = BattleGroundMgr::BGQueueTypeId(bgTypeId, ARENA_TYPE_NONE);

    // ignore if player is already in BG
    if (_player->InBattleGround())
        return;

    // get bg instance or bg template if instance not found
    BattleGround *bg = NULL;
    if (instanceId)
        bg = sBattleGroundMgr.GetBattleGroundThroughClientInstance(instanceId, bgTypeId);

    if (!bg && !(bg = sBattleGroundMgr.GetBattleGroundTemplate(bgTypeId)))
    {
        sLog.outError("Battleground: no available bg / template found");
        return;
    }

    // expected bracket entry
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(),_player->getLevel());
    if (!bracketEntry)
        return;

    GroupJoinBattlegroundResult err;

    // check queue conditions
    if (!joinAsGroup)
    {
        // check Deserter debuff
        if (!_player->CanJoinToBattleground())
        {
            WorldPacket data;
            sBattleGroundMgr.BuildGroupJoinedBattlegroundPacket(&data, ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS);
            _player->GetSession()->SendPacket(&data);
            return;
        }
        // check if already in queue
        if (_player->GetBattleGroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES)
            //player is already in this queue
            return;
        // check if has free queue slots
        if (!_player->HasFreeBattleGroundQueueId())
            return;
    }
    else
    {
        grp = _player->GetGroup();
        // no group found, error
        if (!grp)
            return;
        if (grp->GetLeaderGuid() != _player->GetObjectGuid())
            return;
        err = grp->CanJoinBattleGroundQueue(bg, bgQueueTypeId, 0, bg->GetMaxPlayersPerTeam(), false, 0);
        isPremade = sWorld.getConfig(CONFIG_UINT32_BATTLEGROUND_PREMADE_GROUP_WAIT_FOR_MATCH) &&
            (grp->GetMembersCount() >= bg->GetMinPlayersPerTeam());
    }
    // if we're here, then the conditions to join a bg are met. We can proceed in joining.

    // _player->GetGroup() was already checked, grp is already initialized
    BattleGroundQueue& bgQueue = sBattleGroundMgr.m_BattleGroundQueues[bgQueueTypeId];
    if (joinAsGroup)
    {
        GroupQueueInfo * ginfo;
        uint32 avgTime;

        if(err > 0)
        {
            DEBUG_LOG("Battleground: the following players are joining as group:");
            ginfo = bgQueue.AddGroup(_player, grp, bgTypeId, bracketEntry, ARENA_TYPE_NONE, false, isPremade, 0);
            avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
        }

        for(GroupReference *itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player *member = itr->getSource();
            if(!member)
                continue;                                   // this should never happen

            WorldPacket data;

            if(err <= 0)
            {
                sBattleGroundMgr.BuildGroupJoinedBattlegroundPacket(&data, err);
                member->GetSession()->SendPacket(&data);
                continue;
            }

            // add to queue
            uint32 queueSlot = member->AddBattleGroundQueueId(bgQueueTypeId);

            // send status packet (in queue)
            sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, queueSlot, STATUS_WAIT_QUEUE, avgTime, 0, ginfo->arenaType);
            member->GetSession()->SendPacket(&data);
            sBattleGroundMgr.BuildGroupJoinedBattlegroundPacket(&data, err);
            member->GetSession()->SendPacket(&data);
            DEBUG_LOG("Battleground: player joined queue for bg queue type %u bg type %u: GUID %u, NAME %s",bgQueueTypeId,bgTypeId,member->GetGUIDLow(), member->GetName());
        }
        DEBUG_LOG("Battleground: group end");
    }
    else
    {
        GroupQueueInfo * ginfo = bgQueue.AddGroup(_player, NULL, bgTypeId, bracketEntry, ARENA_TYPE_NONE, false, isPremade, 0);
        uint32 avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
        // already checked if queueSlot is valid, now just get it
        uint32 queueSlot = _player->AddBattleGroundQueueId(bgQueueTypeId);

        WorldPacket data;
                                                            // send status packet (in queue)
        sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, queueSlot, STATUS_WAIT_QUEUE, avgTime, 0, ginfo->arenaType);
        SendPacket(&data);
        DEBUG_LOG("Battleground: player joined queue for bg queue type %u bg type %u: GUID %u, NAME %s",bgQueueTypeId,bgTypeId,_player->GetGUIDLow(), _player->GetName());
    }
    sBattleGroundMgr.ScheduleQueueUpdate(0, ARENA_TYPE_NONE, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
}

void WorldSession::HandleBattleGroundPlayerPositionsOpcode( WorldPacket & /*recv_data*/ )
{
                                                            // empty opcode
    DEBUG_LOG("WORLD: Recvd MSG_BATTLEGROUND_PLAYER_POSITIONS Message");

    BattleGround *bg = _player->GetBattleGround();
    if(!bg)                                                 // can't be received if player not in battleground
        return;

    switch( bg->GetTypeID() )
    {
        case BATTLEGROUND_WS:
            {
                uint32 count1 = 0;                          // always constant zero?
                uint32 count2 = 0;                          // count of next fields

                Player *ali_plr = sObjectMgr.GetPlayer(((BattleGroundWS*)bg)->GetAllianceFlagPickerGuid());
                if (ali_plr)
                    ++count2;

                Player *horde_plr = sObjectMgr.GetPlayer(((BattleGroundWS*)bg)->GetHordeFlagPickerGuid());
                if (horde_plr)
                    ++count2;

                WorldPacket data(MSG_BATTLEGROUND_PLAYER_POSITIONS, (4+4+16*count1+16*count2));
                data << count1;                             // alliance flag holders count - obsolete, now always 0
                /*for(uint8 i = 0; i < count1; ++i)
                {
                    data << ObjectGuid(0);                  // guid
                    data << (float)0;                       // x
                    data << (float)0;                       // y
                }*/
                data << count2;                             // horde flag holders count - obsolete, now count of next fields
                if (ali_plr)
                {
                    data << ObjectGuid(ali_plr->GetObjectGuid());
                    data << float(ali_plr->GetPositionX());
                    data << float(ali_plr->GetPositionY());
                }
                if (horde_plr)
                {
                    data << ObjectGuid(horde_plr->GetObjectGuid());
                    data << float(horde_plr->GetPositionX());
                    data << float(horde_plr->GetPositionY());
                }

                SendPacket(&data);
            }
            break;
        case BATTLEGROUND_EY:
            //TODO : fix me!
            break;
        case BATTLEGROUND_AB:
        case BATTLEGROUND_AV:
            {
                //for other BG types - send default
                WorldPacket data(MSG_BATTLEGROUND_PLAYER_POSITIONS, (4+4));
                data << uint32(0);
                data << uint32(0);
                SendPacket(&data);
            }
            break;
        default:
            //maybe it is sent also in arena - do nothing
            break;
    }
}

void WorldSession::HandlePVPLogDataOpcode( WorldPacket & /*recv_data*/ )
{
    DEBUG_LOG( "WORLD: Recvd MSG_PVP_LOG_DATA Message");

    BattleGround *bg = _player->GetBattleGround();
    if (!bg)
        return;

    // arena finish version will send in BattleGround::EndBattleGround directly
    if (bg->isArena())
        return;

    WorldPacket data;
    sBattleGroundMgr.BuildPvpLogDataPacket(&data, bg);
    SendPacket(&data);

    DEBUG_LOG( "WORLD: Sent MSG_PVP_LOG_DATA Message");
}

void WorldSession::HandleBattlefieldListOpcode( WorldPacket &recv_data )
{
    DEBUG_LOG( "WORLD: Recvd CMSG_BATTLEFIELD_LIST Message");

    uint32 bgTypeId;
    recv_data >> bgTypeId;                                  // id from DBC

    uint8 fromWhere;
    recv_data >> fromWhere;                                 // 0 - battlemaster (lua: ShowBattlefieldList), 1 - UI (lua: RequestBattlegroundInstanceInfo)

    uint8 unk1;
    recv_data >> unk1;                                      // unknown 3.2.2

    BattlemasterListEntry const* bl = sBattlemasterListStore.LookupEntry(bgTypeId);
    if (!bl)
    {
        sLog.outError("Battleground: invalid bgtype received.");
        return;
    }

    WorldPacket data;
    sBattleGroundMgr.BuildBattleGroundListPacket(&data, ObjectGuid(), _player, BattleGroundTypeId(bgTypeId), fromWhere);
    SendPacket( &data );
}

void WorldSession::HandleBattleFieldPortOpcode( WorldPacket &recv_data )
{
    DEBUG_LOG( "WORLD: Recvd CMSG_BATTLEFIELD_PORT Message");

    uint8 type;                                             // arenatype if arena
    uint8 unk2;                                             // unk, can be 0x0 (may be if was invited?) and 0x1
    uint32 bgTypeId_;                                       // type id from dbc
    uint16 unk;                                             // 0x1F90 constant?
    uint8 action;                                           // enter battle 0x1, leave queue 0x0

    recv_data >> type >> unk2 >> bgTypeId_ >> unk >> action;

    if (!sBattlemasterListStore.LookupEntry(bgTypeId_))
    {
        sLog.outError("BattlegroundHandler: invalid bgtype (%u) received.", bgTypeId_);
        return;
    }

    if (type && !IsArenaTypeValid(ArenaType(type)))
    {
        sLog.outError("BattlegroundHandler: Invalid CMSG_BATTLEFIELD_PORT received from player (%u), arena type wrong: %u.", _player->GetGUIDLow(), type);
        return;
    }

    if (!_player->InBattleGroundQueue())
    {
        sLog.outError("BattlegroundHandler: Invalid CMSG_BATTLEFIELD_PORT received from player (%u), he is not in bg_queue.", _player->GetGUIDLow());
        return;
    }

    //get GroupQueueInfo from BattleGroundQueue
    BattleGroundTypeId bgTypeId = BattleGroundTypeId(bgTypeId_);
    BattleGroundQueueTypeId bgQueueTypeId = BattleGroundMgr::BGQueueTypeId(bgTypeId, ArenaType(type));
    BattleGroundQueue& bgQueue = sBattleGroundMgr.m_BattleGroundQueues[bgQueueTypeId];
    //we must use temporary variable, because GroupQueueInfo pointer can be deleted in BattleGroundQueue::RemovePlayer() function
    GroupQueueInfo ginfo;
    if (!bgQueue.GetPlayerGroupInfoData(_player->GetObjectGuid(), &ginfo))
    {
        sLog.outError("BattlegroundHandler: itrplayerstatus not found.");
        return;
    }
    // if action == 1, then instanceId is required
    if (!ginfo.IsInvitedToBGInstanceGUID && action == 1)
    {
        sLog.outError("BattlegroundHandler: instance not found.");
        return;
    }

    BattleGround *bg = sBattleGroundMgr.GetBattleGround(ginfo.IsInvitedToBGInstanceGUID, bgTypeId);

    // bg template might and must be used in case of leaving queue, when instance is not created yet
    if (!bg && action == 0)
        bg = sBattleGroundMgr.GetBattleGroundTemplate(bgTypeId);
    if (!bg)
    {
        sLog.outError("BattlegroundHandler: bg_template not found for type id %u.", bgTypeId);
        return;
    }

    // expected bracket entry
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(),_player->getLevel());
    if (!bracketEntry)
        return;

    //some checks if player isn't cheating - it is not exactly cheating, but we cannot allow it
    if (action == 1 && ginfo.arenaType == ARENA_TYPE_NONE)
    {
        //if player is trying to enter battleground (not arena!) and he has deserter debuff, we must just remove him from queue
        if (!_player->CanJoinToBattleground())
        {
            //send bg command result to show nice message
            WorldPacket data2;
            sBattleGroundMgr.BuildGroupJoinedBattlegroundPacket(&data2, ERR_GROUP_JOIN_BATTLEGROUND_DESERTERS);
            _player->GetSession()->SendPacket(&data2);
            action = 0;
            DEBUG_LOG("Battleground: player %s (%u) has a deserter debuff, do not port him to battleground!", _player->GetName(), _player->GetGUIDLow());
        }
        //if player don't match battleground max level, then do not allow him to enter! (this might happen when player leveled up during his waiting in queue
        if (_player->getLevel() > bg->GetMaxLevel())
        {
            sLog.outError("Battleground: Player %s (%u) has level (%u) higher than maxlevel (%u) of battleground (%u)! Do not port him to battleground!",
                _player->GetName(), _player->GetGUIDLow(), _player->getLevel(), bg->GetMaxLevel(), bg->GetTypeID());
            action = 0;
        }
    }
    uint32 queueSlot = _player->GetBattleGroundQueueIndex(bgQueueTypeId);
    WorldPacket data;
    switch( action )
    {
        case 1:                                         // port to battleground
            if (!_player->IsInvitedForBattleGroundQueueType(bgQueueTypeId))
                return;                                 // cheating?

            if (!_player->InBattleGround())
                _player->SetBattleGroundEntryPoint();

            // resurrect the player
            if (!_player->isAlive())
            {
                _player->ResurrectPlayer(1.0f);
                _player->SpawnCorpseBones();
            }
            // stop taxi flight at port
            if (_player->IsTaxiFlying())
            {
                _player->GetMotionMaster()->MovementExpired();
                _player->m_taxi.ClearTaxiDestinations();
            }

            sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, queueSlot, STATUS_IN_PROGRESS, 0, bg->GetStartTime(), bg->GetArenaType());
            _player->GetSession()->SendPacket(&data);
            // remove battleground queue status from BGmgr
            bgQueue.RemovePlayer(_player->GetObjectGuid(), false);
            // this is still needed here if battleground "jumping" shouldn't add deserter debuff
            // also this is required to prevent stuck at old battleground after SetBattleGroundId set to new
            if (BattleGround *currentBg = _player->GetBattleGround())
                currentBg->RemovePlayerAtLeave(_player->GetObjectGuid(), false, true);

            // set the destination instance id
            _player->SetBattleGroundId(bg->GetInstanceID(), bgTypeId);
            // set the destination team
            _player->SetBGTeam(ginfo.GroupTeam);
            // bg->HandleBeforeTeleportToBattleGround(_player);
            sBattleGroundMgr.SendToBattleGround(_player, ginfo.IsInvitedToBGInstanceGUID, bgTypeId);
            // add only in HandleMoveWorldPortAck()
            // bg->AddPlayer(_player,team);
            DEBUG_LOG("Battleground: player %s (%u) joined battle for bg %u, bgtype %u, queue type %u.", _player->GetName(), _player->GetGUIDLow(), bg->GetInstanceID(), bg->GetTypeID(), bgQueueTypeId);
            break;
        case 0:                                         // leave queue
            // if player leaves rated arena match before match start, it is counted as he played but he lost
            if (ginfo.IsRated && ginfo.IsInvitedToBGInstanceGUID)
            {
                ArenaTeam * at = sObjectMgr.GetArenaTeamById(ginfo.ArenaTeamId);
                if (at)
                {
                    DEBUG_LOG("UPDATING memberLost's personal arena rating for %s by opponents rating: %u, because he has left queue!", _player->GetGuidStr().c_str(), ginfo.OpponentsTeamRating);
                    at->MemberLost(_player, ginfo.OpponentsTeamRating);
                    at->SaveToDB();
                }
            }
            _player->RemoveBattleGroundQueueId(bgQueueTypeId);  // must be called this way, because if you move this call to queue->removeplayer, it causes bugs
            sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, queueSlot, STATUS_NONE, 0, 0, ARENA_TYPE_NONE);
            bgQueue.RemovePlayer(_player->GetObjectGuid(), true);
            // player left queue, we should update it - do not update Arena Queue
            if (ginfo.arenaType == ARENA_TYPE_NONE)
                sBattleGroundMgr.ScheduleQueueUpdate(ginfo.ArenaTeamRating, ginfo.arenaType, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
            SendPacket(&data);
            DEBUG_LOG("Battleground: player %s (%u) left queue for bgtype %u, queue type %u.", _player->GetName(), _player->GetGUIDLow(), bg->GetTypeID(), bgQueueTypeId);
            break;
        default:
            sLog.outError("Battleground port: unknown action %u", action);
            break;
    }
}

void WorldSession::HandleLeaveBattlefieldOpcode( WorldPacket& recv_data )
{
    DEBUG_LOG( "WORLD: Recvd CMSG_LEAVE_BATTLEFIELD Message");

    recv_data.read_skip<uint8>();                           // unk1
    recv_data.read_skip<uint8>();                           // unk2
    recv_data.read_skip<uint32>();                          // BattleGroundTypeId
    recv_data.read_skip<uint16>();                          // unk3

    //if(bgTypeId >= MAX_BATTLEGROUND_TYPES)                  // cheating? but not important in this case
    //    return;

    // not allow leave battleground in combat
    if (_player->isInCombat())
        if (BattleGround* bg = _player->GetBattleGround())
            if (bg->GetStatus() != STATUS_WAIT_LEAVE)
                return;

    _player->LeaveBattleground();
}

void WorldSession::HandleBattlefieldStatusOpcode( WorldPacket & /*recv_data*/ )
{
    // empty opcode
    DEBUG_LOG( "WORLD: Battleground status" );

    WorldPacket data;
    // we must update all queues here
    BattleGround *bg = NULL;
    for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
    {
        BattleGroundQueueTypeId bgQueueTypeId = _player->GetBattleGroundQueueTypeId(i);
        if (!bgQueueTypeId)
            continue;
        BattleGroundTypeId bgTypeId = BattleGroundMgr::BGTemplateId(bgQueueTypeId);
        ArenaType arenaType = BattleGroundMgr::BGArenaType(bgQueueTypeId);
        if (bgTypeId == _player->GetBattleGroundTypeId())
        {
            bg = _player->GetBattleGround();
            //i cannot check any variable from player class because player class doesn't know if player is in 2v2 / 3v3 or 5v5 arena
            //so i must use bg pointer to get that information
            if (bg && bg->GetArenaType() == arenaType)
            {
                // this line is checked, i only don't know if GetStartTime is changing itself after bg end!
                // send status in BattleGround
                sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, i, STATUS_IN_PROGRESS, bg->GetEndTime(), bg->GetStartTime(), arenaType);
                SendPacket(&data);
                continue;
            }
        }
        //we are sending update to player about queue - he can be invited there!
        //get GroupQueueInfo for queue status
        BattleGroundQueue& bgQueue = sBattleGroundMgr.m_BattleGroundQueues[bgQueueTypeId];
        GroupQueueInfo ginfo;
        if (!bgQueue.GetPlayerGroupInfoData(_player->GetObjectGuid(), &ginfo))
            continue;
        if (ginfo.IsInvitedToBGInstanceGUID)
        {
            bg = sBattleGroundMgr.GetBattleGround(ginfo.IsInvitedToBGInstanceGUID, bgTypeId);
            if (!bg)
                continue;
            uint32 remainingTime = WorldTimer::getMSTimeDiff(WorldTimer::getMSTime(), ginfo.RemoveInviteTime);
            // send status invited to BattleGround
            sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, i, STATUS_WAIT_JOIN, remainingTime, 0, arenaType);
            SendPacket(&data);
        }
        else
        {
            bg = sBattleGroundMgr.GetBattleGroundTemplate(bgTypeId);
            if (!bg)
                continue;

            // expected bracket entry
            PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(),_player->getLevel());
            if (!bracketEntry)
                continue;

            uint32 avgTime = bgQueue.GetAverageQueueWaitTime(&ginfo, bracketEntry->GetBracketId());
            // send status in BattleGround Queue
            sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, i, STATUS_WAIT_QUEUE, avgTime, WorldTimer::getMSTimeDiff(ginfo.JoinTime, WorldTimer::getMSTime()), arenaType);
            SendPacket(&data);
        }
    }
}

void WorldSession::HandleAreaSpiritHealerQueryOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_AREA_SPIRIT_HEALER_QUERY");

    BattleGround *bg = _player->GetBattleGround();
    if (!bg)
        return;

    ObjectGuid guid;
    recv_data >> guid;

    Creature *unit = GetPlayer()->GetMap()->GetCreature(guid);
    if (!unit)
        return;

    if(!unit->isSpiritService())                            // it's not spirit service
        return;

    unit->SendAreaSpiritHealerQueryOpcode(GetPlayer());
}

void WorldSession::HandleAreaSpiritHealerQueueOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_AREA_SPIRIT_HEALER_QUEUE");

    BattleGround *bg = _player->GetBattleGround();
    if (!bg)
        return;

    ObjectGuid guid;
    recv_data >> guid;

    Creature *unit = GetPlayer()->GetMap()->GetCreature(guid);
    if (!unit)
        return;

    if(!unit->isSpiritService())                            // it's not spirit service
        return;

    sScriptMgr.OnGossipHello(GetPlayer(), unit);
}

void WorldSession::HandleBattlemasterJoinArena( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_BATTLEMASTER_JOIN_ARENA");
    //recv_data.hexlike();

    ObjectGuid guid;                                        // arena Battlemaster guid
    uint8 arenaslot;                                        // 2v2, 3v3 or 5v5
    uint8 asGroup;                                          // asGroup
    uint8 isRated;                                          // isRated

    recv_data >> guid >> arenaslot >> asGroup >> isRated;

    // ignore if we already in BG or BG queue
    if (_player->InBattleGround())
        return;

    Creature *unit = GetPlayer()->GetMap()->GetCreature(guid);
    if (!unit)
        return;

    if(!unit->isBattleMaster())                             // it's not battle master
        return;

    ArenaType arenatype;
    uint32 arenaRating = 0;

    switch(arenaslot)
    {
        case 0:
            arenatype = ARENA_TYPE_2v2;
            break;
        case 1:
            arenatype = ARENA_TYPE_3v3;
            break;
        case 2:
            arenatype = ARENA_TYPE_5v5;
            break;
        default:
            sLog.outError("Unknown arena slot %u at HandleBattlemasterJoinArena()", arenaslot);
            return;
    }

    // check existence
    BattleGround* bg = sBattleGroundMgr.GetBattleGroundTemplate(BATTLEGROUND_AA);
    if (!bg)
    {
        sLog.outError("Battleground: template bg (all arenas) not found");
        return;
    }

    BattleGroundTypeId bgTypeId = bg->GetTypeID();
    BattleGroundQueueTypeId bgQueueTypeId = BattleGroundMgr::BGQueueTypeId(bgTypeId, arenatype);
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(),_player->getLevel());
    if (!bracketEntry)
        return;

    GroupJoinBattlegroundResult err;

    Group * grp = NULL;

    // check queue conditions
    if (!asGroup)
    {
        // you can't join in this way by client
        if (isRated)
            return;

        // check if already in queue
        if (_player->GetBattleGroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES)
            //player is already in this queue
            return;
        // check if has free queue slots
        if (!_player->HasFreeBattleGroundQueueId())
            return;
    }
    else
    {
        grp = _player->GetGroup();
        // no group found, error
        if (!grp)
            return;
        if (grp->GetLeaderGuid() != _player->GetObjectGuid())
            return;
        // may be Group::CanJoinBattleGroundQueue should be moved to player class...
        err = grp->CanJoinBattleGroundQueue(bg, bgQueueTypeId, arenatype, arenatype, (bool)isRated, arenaslot);
    }

    uint32 ateamId = 0;

    if (isRated)
    {
        ateamId = _player->GetArenaTeamId(arenaslot);
        // check real arena team existence only here (if it was moved to group->CanJoin .. () then we would have to get it twice)
        ArenaTeam * at = sObjectMgr.GetArenaTeamById(ateamId);
        if (!at)
        {
            _player->GetSession()->SendNotInArenaTeamPacket(arenatype);
            return;
        }
        // get the team rating for queue
        arenaRating = at->GetRating();
        // the arena team id must match for everyone in the group
        // get the personal ratings for queue
        uint32 avg_pers_rating = 0;

        for(Group::member_citerator citr = grp->GetMemberSlots().begin(); citr != grp->GetMemberSlots().end(); ++citr)
        {
            ArenaTeamMember const* at_member = at->GetMember(citr->guid);
            if (!at_member)                                 // group member joining to arena must be in leader arena team
                return;

            // calc avg personal rating
            avg_pers_rating += at_member->personal_rating;
        }

        avg_pers_rating /= grp->GetMembersCount();

        // if avg personal rating is more than 150 points below the teams rating, the team will be queued against an opponent matching or similar to the average personal rating
        if (avg_pers_rating + 150 < arenaRating)
            arenaRating = avg_pers_rating;
    }

    BattleGroundQueue &bgQueue = sBattleGroundMgr.m_BattleGroundQueues[bgQueueTypeId];
    if (asGroup)
    {
        uint32 avgTime;

        if(err > 0)
        {
            DEBUG_LOG("Battleground: arena join as group start");
            if (isRated)
                DEBUG_LOG("Battleground: arena team id %u, leader %s queued with rating %u for type %u",_player->GetArenaTeamId(arenaslot),_player->GetName(),arenaRating,arenatype);

            GroupQueueInfo * ginfo = bgQueue.AddGroup(_player, grp, bgTypeId, bracketEntry, arenatype, isRated, false, arenaRating, ateamId);
            avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
        }

        for(GroupReference *itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player *member = itr->getSource();
            if(!member)
                continue;

            WorldPacket data;

            if(err <= 0)
            {
                sBattleGroundMgr.BuildGroupJoinedBattlegroundPacket(&data, err);
                member->GetSession()->SendPacket(&data);
                continue;
            }

            // add to queue
            uint32 queueSlot = member->AddBattleGroundQueueId(bgQueueTypeId);

            // send status packet (in queue)
            sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, queueSlot, STATUS_WAIT_QUEUE, avgTime, 0, arenatype);
            member->GetSession()->SendPacket(&data);
            sBattleGroundMgr.BuildGroupJoinedBattlegroundPacket(&data, err);
            member->GetSession()->SendPacket(&data);
            DEBUG_LOG("Battleground: player joined queue for arena as group bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, bgTypeId, member->GetGUIDLow(), member->GetName());
        }
        DEBUG_LOG("Battleground: arena join as group end");
    }
    else
    {
        GroupQueueInfo * ginfo = bgQueue.AddGroup(_player, NULL, bgTypeId, bracketEntry, arenatype, isRated, false, arenaRating, ateamId);
        uint32 avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
        uint32 queueSlot = _player->AddBattleGroundQueueId(bgQueueTypeId);

        WorldPacket data;
        // send status packet (in queue)
        sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, queueSlot, STATUS_WAIT_QUEUE, avgTime, 0, arenatype);
        SendPacket(&data);
        DEBUG_LOG("Battleground: player joined queue for arena, skirmish, bg queue type %u bg type %u: GUID %u, NAME %s",bgQueueTypeId,bgTypeId,_player->GetGUIDLow(), _player->GetName());
    }
    sBattleGroundMgr.ScheduleQueueUpdate(arenaRating, arenatype, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());
}

void WorldSession::HandleReportPvPAFK( WorldPacket & recv_data )
{
    ObjectGuid playerGuid;
    recv_data >> playerGuid;
    Player *reportedPlayer = sObjectMgr.GetPlayer(playerGuid);

    if (!reportedPlayer)
    {
        DEBUG_LOG("WorldSession::HandleReportPvPAFK: player not found");
        return;
    }

    DEBUG_LOG("WorldSession::HandleReportPvPAFK: %s reported %s", _player->GetName(), reportedPlayer->GetName());

    reportedPlayer->ReportedAfkBy(_player);
}
