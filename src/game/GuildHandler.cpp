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
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "GossipDef.h"
#include "SocialMgr.h"

void WorldSession::HandleGuildQueryOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_QUERY");

    ObjectGuid guildGuid, playerGuid;
    recvPacket >> guildGuid >> playerGuid;

    if (Guild* guild = sGuildMgr.GetGuildByGuid(guildGuid))
    {
        if (guild->GetMemberSlot(playerGuid))
        {
            guild->Query(this);
            return;
        }
    }

    SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
}

void WorldSession::HandleGuildCreateOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_CREATE");

    std::string gname;
    recvPacket >> gname;

    if (GetPlayer()->GetGuildId())                          // already in guild
        return;

    Guild* guild = new Guild;
    if (!guild->Create(GetPlayer(), gname))
    {
        delete guild;
        return;
    }

    sGuildMgr.AddGuild(guild);
}

void WorldSession::HandleGuildInviteOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_INVITE");

    std::string Invitedname, plname;
    Player* player = NULL;

    Invitedname = recvPacket.ReadString(recvPacket.ReadBits(7));

    if (normalizePlayerName(Invitedname))
        player = ObjectAccessor::FindPlayerByName(Invitedname.c_str());

    if (!player)
    {
        SendGuildCommandResult(GUILD_INVITE_S, Invitedname, ERR_GUILD_PLAYER_NOT_FOUND_S);
        return;
    }

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    // OK result but not send invite
    if (player->GetSocial()->HasIgnore(GetPlayer()->GetObjectGuid()))
        return;

    // not let enemies sign guild charter
    if (!sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_GUILD) && player->GetTeam() != GetPlayer()->GetTeam())
    {
        SendGuildCommandResult(GUILD_INVITE_S, Invitedname, ERR_GUILD_NOT_ALLIED);
        return;
    }

    if (player->GetGuildId())
    {
        plname = player->GetName();
        SendGuildCommandResult(GUILD_INVITE_S, plname, ERR_ALREADY_IN_GUILD_S);
        return;
    }

    if (player->GetGuildIdInvited())
    {
        plname = player->GetName();
        SendGuildCommandResult(GUILD_INVITE_S, plname, ERR_ALREADY_INVITED_TO_GUILD_S);
        return;
    }

    if (!guild->HasRankRight(GetPlayer()->GetRank(), GR_RIGHT_INVITE))
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    DEBUG_LOG("Player %s Invited %s to Join his Guild", GetPlayer()->GetName(), Invitedname.c_str());

    player->SetGuildInvited(GetPlayer()->GetGuildId(), GetPlayer()->GetObjectGuid());
    // Put record into guildlog
    guild->LogGuildEvent(GUILD_EVENT_LOG_INVITE_PLAYER, GetPlayer()->GetObjectGuid(), player->GetObjectGuid());

    ObjectGuid oldGuild = player->GetGuildGuid();
    ObjectGuid newGuild = guild->GetObjectGuid();
    std::string oldGuildName = player->GetGuildName();
    std::string newGuildName = guild->GetName();

    WorldPacket data(SMSG_GUILD_INVITE, 4 * 6 + 10);          // guess size
    data << uint32(guild->GetLevel());
    data << uint32(guild->GetBorderStyle());
    data << uint32(guild->GetBorderColor());
    data << uint32(guild->GetEmblemStyle());
    data << uint32(guild->GetBackgroundColor());
    data << uint32(guild->GetEmblemColor());

    data.WriteGuidMask<3, 2>(newGuild);
    data.WriteBits(oldGuildName.length(), 8);
    data.WriteGuidMask<1>(newGuild);
    data.WriteGuidMask<6, 4, 1, 5, 7, 2>(oldGuild);
    data.WriteGuidMask<7, 0, 6>(newGuild);
    data.WriteBits(newGuildName.length(), 8);
    data.WriteGuidMask<3, 0>(oldGuild);
    data.WriteGuidMask<5>(newGuild);
    data.WriteBits(strlen(_player->GetName()), 7);
    data.WriteGuidMask<4>(newGuild);

    data.WriteGuidBytes<1>(newGuild);
    data.WriteGuidBytes<3>(oldGuild);
    data.WriteGuidBytes<6>(newGuild);
    data.WriteGuidBytes<2, 1>(oldGuild);
    data.WriteGuidBytes<0>(newGuild);

    data.WriteStringData(oldGuildName);

    data.WriteGuidBytes<7, 2>(newGuild);

    data.WriteStringData(_player->GetName());

    data.WriteGuidBytes<7, 6, 5, 0>(oldGuild);
    data.WriteGuidBytes<4>(newGuild);

    data.WriteStringData(newGuildName);

    data.WriteGuidBytes<5, 3>(newGuild);
    data.WriteGuidBytes<4>(oldGuild);

    player->GetSession()->SendPacket(&data);

    DEBUG_LOG("WORLD: Sent (SMSG_GUILD_INVITE)");
}

void WorldSession::HandleGuildRemoveOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_REMOVE");

    std::string plName;
    ObjectGuid targetGuid;
    recvPacket.ReadGuidMask<6, 5, 4, 0, 1, 3, 7, 2>(targetGuid);
    recvPacket.ReadGuidBytes<2, 6, 5, 7, 1, 4, 3, 0>(targetGuid);

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    if (!guild->HasRankRight(GetPlayer()->GetRank(), GR_RIGHT_REMOVE))
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    if (!sObjectMgr.GetPlayerNameByGUID(targetGuid, plName))
        return;

    MemberSlot* slot = guild->GetMemberSlot(targetGuid);
    if (!slot)
    {
        SendGuildCommandResult(GUILD_INVITE_S, plName, ERR_GUILD_PLAYER_NOT_IN_GUILD_S);
        return;
    }

    if (slot->RankId == GR_GUILDMASTER)
    {
        SendGuildCommandResult(GUILD_QUIT_S, "", ERR_GUILD_LEADER_LEAVE);
        return;
    }

    // do not allow to kick player with same or higher rights
    if (GetPlayer()->GetRank() >= slot->RankId)
    {
        SendGuildCommandResult(GUILD_QUIT_S, plName, ERR_GUILD_RANK_TOO_HIGH_S);
        return;
    }

    // do not delete guilds that level are higher than undeletable
    if (guild->GetMemberSize() == 1 && guild->GetLevel() >= sWorld.getConfig(CONGIG_UINT32_GUILD_UNDELETABLE_LEVEL))
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_UNDELETABLE_DUE_TO_LEVEL);
        return;
    }

    // possible last member removed, do cleanup, and no need events
    if (guild->DelMember(slot->guid))
    {
        guild->Disband();
        delete guild;
        return;
    }

    // Put record into guild log
    guild->LogGuildEvent(GUILD_EVENT_LOG_UNINVITE_PLAYER, GetPlayer()->GetObjectGuid(), slot->guid);

    guild->BroadcastEvent(GE_REMOVED, plName.c_str(), _player->GetName());
}

void WorldSession::HandleGuildAcceptOpcode(WorldPacket& /*recvPacket*/)
{
    Guild* guild;
    Player* player = GetPlayer();

    DEBUG_LOG("WORLD: Received CMSG_GUILD_ACCEPT");

    guild = sGuildMgr.GetGuildById(player->GetGuildIdInvited());
    if (!guild || player->GetGuildId())
        return;

    // not let enemies sign guild charter
    if (!sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_GUILD) && player->GetTeam() != sObjectMgr.GetPlayerTeamByGUID(guild->GetLeaderGuid()))
        return;

    if (!guild->AddMember(GetPlayer()->GetObjectGuid(), guild->GetLowestRank()))
        return;
    // Put record into guild log
    guild->LogGuildEvent(GUILD_EVENT_LOG_JOIN_GUILD, GetPlayer()->GetObjectGuid());

    guild->BroadcastEvent(GE_JOINED, player->GetObjectGuid(), player->GetName());
}

void WorldSession::HandleGuildDeclineOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received %s", LookupOpcodeName(recvPacket.GetOpcode()));

    if (Player* inviter = sObjectMgr.GetPlayer(GetPlayer()->GetGuildInviterGuid()))
        inviter->SendGuildDeclined(GetPlayer()->GetName(), recvPacket.GetOpcode() == CMSG_GUILD_AUTO_DECLINE);

    GetPlayer()->SetGuildInvited(0);
    GetPlayer()->SetInGuild(0);
    GetPlayer()->SetGuildLevel(0);
}

void WorldSession::HandleGuildInfoOpcode(WorldPacket& /*recvPacket*/)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_INFO");

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    WorldPacket data(SMSG_GUILD_INFO, (guild->GetName().size() + 4 + 4 + 4));
    data << guild->GetName();
    data << uint32(secsToTimeBitFields(guild->GetCreatedDate())); // 3.x (prev. day + month + year)
    data << uint32(guild->GetMemberSize());                 // amount of chars
    data << uint32(guild->GetAccountsNumber());             // amount of accounts
    SendPacket(&data);
}

void WorldSession::HandleGuildRosterOpcode(WorldPacket& recvPacket)
{
    ObjectGuid guid1, guid2;

    recvPacket.ReadGuidMask<2, 3>(guid2);
    recvPacket.ReadGuidMask<6, 0>(guid1);
    recvPacket.ReadGuidMask<7>(guid2);
    recvPacket.ReadGuidMask<2>(guid1);
    recvPacket.ReadGuidMask<6, 4>(guid2);

    recvPacket.ReadGuidMask<1>(guid1);
    recvPacket.ReadGuidMask<5>(guid2);
    recvPacket.ReadGuidMask<4, 3>(guid1);
    recvPacket.ReadGuidMask<0>(guid2);
    recvPacket.ReadGuidMask<5>(guid1);
    recvPacket.ReadGuidMask<1>(guid2);
    recvPacket.ReadGuidMask<7>(guid1);

    recvPacket.ReadGuidBytes<3>(guid1);
    recvPacket.ReadGuidBytes<4>(guid2);
    recvPacket.ReadGuidBytes<7, 2, 4, 0>(guid1);
    recvPacket.ReadGuidBytes<5>(guid2);
    recvPacket.ReadGuidBytes<1>(guid1);
    recvPacket.ReadGuidBytes<0, 6>(guid2);
    recvPacket.ReadGuidBytes<5>(guid1);
    recvPacket.ReadGuidBytes<7, 2, 3, 1>(guid2);
    recvPacket.ReadGuidBytes<6>(guid1);

    DEBUG_LOG("WORLD: Received CMSG_GUILD_ROSTER, guid1: %s raw: " UI64FMTD ", guid2: %s raw: " UI64FMTD "",
        guid1.GetString().c_str(), guid1.GetRawValue(), guid2.GetString().c_str(), guid2.GetRawValue());

    if (Guild* guild = sGuildMgr.GetGuildById(_player->GetGuildId()))
        guild->Roster(this);
}

void WorldSession::HandleGuildPromoteOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_PROMOTE");

    std::string plName;
    ObjectGuid targetGuid;
    recvPacket.ReadGuidMask<7, 2, 5, 6, 1, 0, 3, 4>(targetGuid);
    recvPacket.ReadGuidBytes<0, 5, 2, 3, 6, 4, 1, 7>(targetGuid);

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }
    if (!guild->HasRankRight(GetPlayer()->GetRank(), GR_RIGHT_PROMOTE))
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    if (!sObjectMgr.GetPlayerNameByGUID(targetGuid, plName))
        return;

    MemberSlot* slot = guild->GetMemberSlot(targetGuid);
    if (!slot)
    {
        SendGuildCommandResult(GUILD_INVITE_S, plName, ERR_GUILD_PLAYER_NOT_IN_GUILD_S);
        return;
    }

    if (slot->guid == GetPlayer()->GetObjectGuid())
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_NAME_INVALID);
        return;
    }

    // allow to promote only to lower rank than member's rank
    // guildmaster's rank = 0
    // GetPlayer()->GetRank() + 1 is highest rank that current player can promote to
    if (GetPlayer()->GetRank() + 1 >= slot->RankId)
    {
        SendGuildCommandResult(GUILD_INVITE_S, plName, ERR_GUILD_RANK_TOO_HIGH_S);
        return;
    }

    uint32 newRankId = slot->RankId - 1;                    // when promoting player, rank is decreased

    slot->ChangeRank(newRankId);
    // Put record into guild log
    guild->LogGuildEvent(GUILD_EVENT_LOG_PROMOTE_PLAYER, GetPlayer()->GetObjectGuid(), slot->guid, newRankId);

    guild->BroadcastEvent(GE_PROMOTION, _player->GetName(), plName.c_str(), guild->GetRankName(newRankId).c_str());
}

void WorldSession::HandleGuildDemoteOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_DEMOTE");

    std::string plName;
    ObjectGuid targetGuid;
    recvPacket.ReadGuidMask<7, 1, 5, 6, 2, 3, 0, 4>(targetGuid);
    recvPacket.ReadGuidBytes<1, 2, 7, 5, 6, 0, 4, 3>(targetGuid);

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());

    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    if (!guild->HasRankRight(GetPlayer()->GetRank(), GR_RIGHT_DEMOTE))
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    if (!sObjectMgr.GetPlayerNameByGUID(targetGuid, plName))
        return;

    MemberSlot* slot = guild->GetMemberSlot(targetGuid);
    if (!slot)
    {
        SendGuildCommandResult(GUILD_INVITE_S, plName, ERR_GUILD_PLAYER_NOT_IN_GUILD_S);
        return;
    }

    if (slot->guid == GetPlayer()->GetObjectGuid())
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_NAME_INVALID);
        return;
    }

    // do not allow to demote same or higher rank
    if (GetPlayer()->GetRank() >= slot->RankId)
    {
        SendGuildCommandResult(GUILD_INVITE_S, plName, ERR_GUILD_RANK_TOO_HIGH_S);
        return;
    }

    // do not allow to demote lowest rank
    if (slot->RankId >= guild->GetLowestRank())
    {
        SendGuildCommandResult(GUILD_INVITE_S, plName, ERR_GUILD_RANK_TOO_LOW_S);
        return;
    }

    uint32 newRankId = slot->RankId + 1;                    // when demoting player, rank is increased

    slot->ChangeRank(newRankId);
    // Put record into guild log
    guild->LogGuildEvent(GUILD_EVENT_LOG_DEMOTE_PLAYER, GetPlayer()->GetObjectGuid(), slot->guid, newRankId);

    guild->BroadcastEvent(GE_DEMOTION, _player->GetName(), plName.c_str(), guild->GetRankName(slot->RankId).c_str());
}

void WorldSession::HandleGuildSetRankOpcode(WorldPacket& recvPacket)
{
    uint32 newRankId;
    std::string plName;
    ObjectGuid targetGuid, invokerGuid;

    recvPacket >> newRankId;
    recvPacket.ReadGuidMask<1, 7>(targetGuid);
    recvPacket.ReadGuidMask<4, 2>(invokerGuid);
    recvPacket.ReadGuidMask<4, 5, 6>(targetGuid);
    recvPacket.ReadGuidMask<1, 7>(invokerGuid);
    recvPacket.ReadGuidMask<2, 3, 0>(targetGuid);
    recvPacket.ReadGuidMask<6, 3, 0, 5>(invokerGuid);

    recvPacket.ReadGuidBytes<0>(targetGuid);
    recvPacket.ReadGuidBytes<1, 3, 5>(invokerGuid);
    recvPacket.ReadGuidBytes<7, 3>(targetGuid);
    recvPacket.ReadGuidBytes<0>(invokerGuid);
    recvPacket.ReadGuidBytes<1>(targetGuid);
    recvPacket.ReadGuidBytes<6>(invokerGuid);
    recvPacket.ReadGuidBytes<2, 5, 4>(targetGuid);
    recvPacket.ReadGuidBytes<2, 4>(invokerGuid);
    recvPacket.ReadGuidBytes<6>(targetGuid);
    recvPacket.ReadGuidBytes<7>(invokerGuid);

    DEBUG_LOG("WORLD: Received CMSG_GUILD_SET_RANK guid1 %s guid2 %s rank %u",
        targetGuid.GetString().c_str(), invokerGuid.GetString().c_str(), newRankId);

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    if (!sObjectMgr.GetPlayerNameByGUID(targetGuid, plName))
        return;

    MemberSlot* slot = guild->GetMemberSlot(targetGuid);
    if (!slot)
    {
        SendGuildCommandResult(GUILD_INVITE_S, plName, ERR_GUILD_PLAYER_NOT_IN_GUILD_S);
        return;
    }

    if (slot->guid == GetPlayer()->GetObjectGuid())
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_NAME_INVALID);
        return;
    }

    if (slot->RankId == newRankId || newRankId >= guild->GetRanksSize())
        return;

    bool promote = newRankId < slot->RankId;
    if (!guild->HasRankRight(GetPlayer()->GetRank(), promote ? GR_RIGHT_PROMOTE : GR_RIGHT_DEMOTE))
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    if (promote)
    {
        // allow to promote only to lower rank than member's rank
        // and only to rank not higher that invoker's 
        if (GetPlayer()->GetRank() + 1 >= slot->RankId || newRankId <=  GetPlayer()->GetRank())
        {
            SendGuildCommandResult(GUILD_INVITE_S, plName, ERR_GUILD_RANK_TOO_HIGH_S);
            return;
        }
    }
    else
    {
        // do not allow to demote same or higher rank
        if (GetPlayer()->GetRank() >= slot->RankId)
        {
            SendGuildCommandResult(GUILD_INVITE_S, plName, ERR_GUILD_RANK_TOO_HIGH_S);
            return;
        }
    }

    slot->ChangeRank(newRankId);
    // Put record into guild log
    guild->LogGuildEvent(promote ? GUILD_EVENT_LOG_PROMOTE_PLAYER : GUILD_EVENT_LOG_DEMOTE_PLAYER, GetPlayer()->GetObjectGuid(), slot->guid, newRankId);

    guild->BroadcastEvent(promote ? GE_PROMOTION : GE_DEMOTION, _player->GetName(), plName.c_str(), guild->GetRankName(newRankId).c_str());
}

void WorldSession::HandleGuildSwitchRankOpcode(WorldPacket& recvPacket)
{
    uint32 rankId;
    bool up;

    recvPacket >> rankId;
    up = recvPacket.ReadBit();

    DEBUG_LOG("WORLD: Received CMSG_GUILD_SWITCH_RANK rank %u up %u", rankId, up);

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    if (GetPlayer()->GetObjectGuid() != guild->GetLeaderGuid())
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    guild->SwitchRank(rankId, up);

    guild->QueryRanks(this);
    guild->Query(this);
    guild->Roster();                                        // broadcast for tab rights update
}

void WorldSession::HandleGuildLeaveOpcode(WorldPacket& /*recvPacket*/)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_LEAVE");

    Guild* guild = sGuildMgr.GetGuildById(_player->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    if (_player->GetObjectGuid() == guild->GetLeaderGuid() && guild->GetMemberSize() > 1)
    {
        SendGuildCommandResult(GUILD_QUIT_S, "", ERR_GUILD_LEADER_LEAVE);
        return;
    }

    // do not delete guilds that level are higher than undeletable
    if (guild->GetMemberSize() == 1 && guild->GetLevel() >= sWorld.getConfig(CONGIG_UINT32_GUILD_UNDELETABLE_LEVEL))
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_UNDELETABLE_DUE_TO_LEVEL);
        return;
    }

    if (_player->GetObjectGuid() == guild->GetLeaderGuid())
    {
        guild->Disband();
        delete guild;
        return;
    }

    SendGuildCommandResult(GUILD_QUIT_S, guild->GetName(), ERR_PLAYER_NO_MORE_IN_GUILD);

    if (guild->DelMember(_player->GetObjectGuid()))
    {
        guild->Disband();
        delete guild;
        return;
    }

    // Put record into guild log
    guild->LogGuildEvent(GUILD_EVENT_LOG_LEAVE_GUILD, _player->GetObjectGuid());

    guild->BroadcastEvent(GE_LEFT, _player->GetObjectGuid(), _player->GetName());
}

void WorldSession::HandleGuildDisbandOpcode(WorldPacket& /*recvPacket*/)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_DISBAND");

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    if (GetPlayer()->GetObjectGuid() != guild->GetLeaderGuid())
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    // do not delete guilds that level are higher than undeletable
    if (guild->GetMemberSize() == 1 && guild->GetLevel() >= sWorld.getConfig(CONGIG_UINT32_GUILD_UNDELETABLE_LEVEL))
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_UNDELETABLE_DUE_TO_LEVEL);
        return;
    }

    guild->Disband();
    delete guild;

    DEBUG_LOG("WORLD: Guild Successfully Disbanded");
}

void WorldSession::HandleGuildLeaderOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_LEADER");

    std::string name = recvPacket.ReadString(recvPacket.ReadBits(7));

    Player* oldLeader = GetPlayer();

    if (!normalizePlayerName(name))
        return;

    Guild* guild = sGuildMgr.GetGuildById(oldLeader->GetGuildId());

    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    if (oldLeader->GetObjectGuid() != guild->GetLeaderGuid())
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    MemberSlot* oldSlot = guild->GetMemberSlot(oldLeader->GetObjectGuid());
    if (!oldSlot)
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    MemberSlot* slot = guild->GetMemberSlot(name);
    if (!slot)
    {
        SendGuildCommandResult(GUILD_INVITE_S, name, ERR_GUILD_PLAYER_NOT_IN_GUILD_S);
        return;
    }

    if (slot->guid == oldLeader->GetObjectGuid())
        return;

    guild->SetLeader(slot->guid);
    // NOTE: GR_OFFICER might not actually be officer rank
    oldSlot->ChangeRank(GR_OFFICER);

    guild->BroadcastEvent(GE_LEADER_CHANGED, oldLeader->GetName(), name.c_str());
}

void WorldSession::HandleGuildMOTDOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_MOTD");

    std::string MOTD = recvPacket.ReadString(recvPacket.ReadBits(11));

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }
    if (!guild->HasRankRight(GetPlayer()->GetRank(), GR_RIGHT_SETMOTD))
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    guild->SetMOTD(MOTD);

    guild->BroadcastEvent(GE_MOTD, MOTD.c_str());
}

void WorldSession::HandleGuildSetNoteOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_SET_NOTE");

    bool officer;
    uint32 noteLen;
    std::string name, note;
    ObjectGuid targetGuid;

    recvPacket.ReadGuidMask<1, 4, 5, 3, 0, 7>(targetGuid);
    officer = !recvPacket.ReadBit();
    recvPacket.ReadGuidMask<6>(targetGuid);
    noteLen = recvPacket.ReadBits(8);
    recvPacket.ReadGuidMask<2>(targetGuid);

    recvPacket.ReadGuidBytes<4, 5, 0, 3, 1, 6, 7>(targetGuid);
    note = recvPacket.ReadString(noteLen);
    recvPacket.ReadGuidBytes<2>(targetGuid);

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    if (!guild->HasRankRight(GetPlayer()->GetRank(), officer ? GR_RIGHT_EOFFNOTE : GR_RIGHT_EPNOTE))
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    if (!sObjectMgr.GetPlayerNameByGUID(targetGuid, name))
        return;

    MemberSlot* slot = guild->GetMemberSlot(targetGuid);
    if (!slot)
    {
        SendGuildCommandResult(GUILD_INVITE_S, name, ERR_GUILD_PLAYER_NOT_IN_GUILD_S);
        return;
    }

    if (officer)
        slot->SetOFFNOTE(note);
    else
        slot->SetPNOTE(note);

    guild->Roster(this);
}

void WorldSession::HandleGuildRankOpcode(WorldPacket& recvPacket)
{
    std::string rankname;
    uint32 rankId;
    uint32 rights, MoneyPerDay;

    DEBUG_LOG("WORLD: Received CMSG_GUILD_RANK");

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        recvPacket.rfinish();                               // set to end to avoid warnings spam
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    if (GetPlayer()->GetObjectGuid() != guild->GetLeaderGuid())
    {
        recvPacket.rfinish();                               // set to end to avoid warnings spam
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    recvPacket >> Unused<uint32>();     // unk
    recvPacket >> Unused<uint32>();     // old rights
    recvPacket >> rights;

    uint32 BankRights[GUILD_BANK_MAX_TABS];
    uint32 BankSlotPerDay[GUILD_BANK_MAX_TABS];
    for (int i = 0; i < GUILD_BANK_MAX_TABS; ++i)
    {
        recvPacket >> BankRights[i];
        recvPacket >> BankSlotPerDay[i];
    }

    recvPacket >> MoneyPerDay;
    recvPacket >> rankId;
    rankname = recvPacket.ReadString(recvPacket.ReadBits(7));

    DEBUG_LOG("WORLD: Changed RankName to %s , Rights to 0x%.4X", rankname.c_str(), rights);

    for (int i = 0; i < GUILD_BANK_MAX_TABS; ++i)
        guild->SetBankRightsAndSlots(rankId, uint8(i), uint16(BankRights[i] & 0xFF), uint16(BankSlotPerDay[i]), true);

    guild->SetBankMoneyPerDay(rankId, MoneyPerDay);
    guild->SetRankName(rankId, rankname);

    if (rankId == GR_GUILDMASTER)                           // prevent loss leader rights
        rights = GR_RIGHT_ALL;

    guild->SetRankRights(rankId, rights);

    guild->Query(this);
    guild->Roster();                                        // broadcast for tab rights update
}

void WorldSession::HandleGuildAddRankOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_ADD_RANK");

    recvPacket >> Unused<uint32>(); // rank id
    std::string rankname = recvPacket.ReadString(recvPacket.ReadBits(7));

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    if (GetPlayer()->GetObjectGuid() != guild->GetLeaderGuid())
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    if (guild->GetRanksSize() >= GUILD_RANKS_MAX_COUNT)     // client not let create more 10 than ranks
        return;

    guild->CreateRank(rankname, GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);

    guild->Query(this);
    guild->QueryRanks(this);
    guild->Roster();                                        // broadcast for tab rights update
}

void WorldSession::HandleGuildDelRankOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_DEL_RANK");

    uint32 rankId;
    recvPacket >> rankId;

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    if (GetPlayer()->GetObjectGuid() != guild->GetLeaderGuid())
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    // do not allow delete rank if there are still members using it
    if (guild->HasMembersWithRank(rankId))
    {
        SendGuildCommandResult(GUILD_INVITE_S, "", ERR_GUILD_RANK_IN_USE);
        return;
    }

    guild->DelRank(rankId);

    guild->Query(this);
    guild->QueryRanks(this);
    guild->Roster();                                        // broadcast for tab rights update
}

void WorldSession::SendGuildCommandResult(uint32 typecmd, const std::string& str, uint32 cmdresult)
{
    WorldPacket data(SMSG_GUILD_COMMAND_RESULT, 8 + str.size() + 1);
    data << typecmd;
    data << cmdresult;
    data.WriteBits(str.length(), 8);
    data.WriteStringData(str);
    SendPacket(&data);

    DEBUG_LOG("WORLD: Sent (SMSG_GUILD_COMMAND_RESULT)");
}

void WorldSession::HandleGuildChangeInfoTextOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_INFO_TEXT");

    std::string GINFO = recvPacket.ReadString(recvPacket.ReadBits(12));

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    if (!guild->HasRankRight(GetPlayer()->GetRank(), GR_RIGHT_MODIFY_GUILD_INFO))
    {
        SendGuildCommandResult(GUILD_CREATE_S, "", ERR_GUILD_PERMISSIONS);
        return;
    }

    guild->SetGINFO(GINFO);
}

void WorldSession::HandleSaveGuildEmblemOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received MSG_SAVE_GUILD_EMBLEM");

    ObjectGuid vendorGuid;
    uint32 EmblemStyle, EmblemColor, BorderStyle, BorderColor, BackgroundColor;

    recvPacket >> vendorGuid;
    recvPacket >> EmblemStyle >> EmblemColor >> BorderStyle >> BorderColor >> BackgroundColor;

    Creature* pCreature = GetPlayer()->GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_TABARDDESIGNER);
    if (!pCreature)
    {
        //"That's not an emblem vendor!"
        SendSaveGuildEmblem(ERR_GUILDEMBLEM_INVALIDVENDOR);
        DEBUG_LOG("WORLD: HandleSaveGuildEmblemOpcode - %s not found or you can't interact with him.", vendorGuid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId());
    if (!guild)
    {
        //"You are not part of a guild!";
        SendSaveGuildEmblem(ERR_GUILDEMBLEM_NOGUILD);
        return;
    }

    if (guild->GetLeaderGuid() != GetPlayer()->GetObjectGuid())
    {
        //"Only guild leaders can create emblems."
        SendSaveGuildEmblem(ERR_GUILDEMBLEM_NOTGUILDMASTER);
        return;
    }

    if (GetPlayer()->GetMoney() < 10 * GOLD)
    {
        //"You can't afford to do that."
        SendSaveGuildEmblem(ERR_GUILDEMBLEM_NOTENOUGHMONEY);
        return;
    }

    GetPlayer()->ModifyMoney(-10 * GOLD);
    guild->SetEmblem(EmblemStyle, EmblemColor, BorderStyle, BorderColor, BackgroundColor);

    //"Guild Emblem saved."
    SendSaveGuildEmblem(ERR_GUILDEMBLEM_SUCCESS);

    guild->Query(this);
}

void WorldSession::HandleGuildEventLogQueryOpcode(WorldPacket& /* recvPacket */)
{
    // empty
    DEBUG_LOG("WORLD: Received (CMSG_GUILD_EVENT_LOG_QUERY)");

    if (uint32 GuildId = GetPlayer()->GetGuildId())
        if (Guild* pGuild = sGuildMgr.GetGuildById(GuildId))
            pGuild->DisplayGuildEventLog(this);
}

/******  GUILD BANK  *******/

void WorldSession::HandleGuildBankMoneyWithdrawn(WorldPacket& /* recv_data */)
{
    DEBUG_LOG("WORLD: Received (CMSG_GUILD_BANK_MONEY_WITHDRAWN)");

    if (uint32 GuildId = GetPlayer()->GetGuildId())
        if (Guild* pGuild = sGuildMgr.GetGuildById(GuildId))
            pGuild->SendMoneyInfo(this, GetPlayer()->GetGUIDLow());
}

void WorldSession::HandleGuildPermissions(WorldPacket& /* recv_data */)
{
    DEBUG_LOG("WORLD: Received (CMSG_GUILD_PERMISSIONS)");

    if (uint32 GuildId = GetPlayer()->GetGuildId())
    {
        if (Guild* pGuild = sGuildMgr.GetGuildById(GuildId))
        {
            uint32 rankId = GetPlayer()->GetRank();

            WorldPacket data(SMSG_GUILD_PERMISSIONS, 4 * 15 + 2 * 8 + 3);
            data << uint32(rankId);                         // guild rank id
            data << uint32(pGuild->GetPurchasedTabs());     // tabs count
            data << uint32(pGuild->GetRankRights(rankId));  // rank rights
            // money per day left
            // WTF uint32?
            data << uint32(pGuild->GetMemberMoneyWithdrawRem(GetPlayer()->GetGUIDLow()));
            data.WriteBits(GUILD_BANK_MAX_TABS, 23);
            for (int i = 0; i < GUILD_BANK_MAX_TABS; ++i)
            {
                data << uint32(pGuild->GetBankRights(rankId, uint8(i)));
                data << uint32(pGuild->GetMemberSlotWithdrawRem(GetPlayer()->GetGUIDLow(), uint8(i)));
            }
            SendPacket(&data);
            DEBUG_LOG("WORLD: Sent (SMSG_GUILD_PERMISSIONS)");
        }
    }
}

/* Called when clicking on Guild bank gameobject */
void WorldSession::HandleGuildBankerActivate(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received (CMSG_GUILD_BANKER_ACTIVATE)");

    ObjectGuid goGuid;
    uint8 unk;
    recv_data >> goGuid >> unk;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(goGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        return;

    if (uint32 GuildId = GetPlayer()->GetGuildId())
    {
        if (Guild* pGuild = sGuildMgr.GetGuildById(GuildId))
        {
            pGuild->DisplayGuildBankTabsInfo(this);         // this also will load guild bank if not yet
            return;
        }
    }

    SendGuildCommandResult(GUILD_UNK1, "", ERR_GUILD_PLAYER_NOT_IN_GUILD);
}

/* Called when opening guild bank tab only (first one) */
void WorldSession::HandleGuildBankQueryTab(WorldPacket& recv_data)
{
    ObjectGuid goGuid;
    uint8 TabId, unk1;
    recv_data >> goGuid >> TabId >> unk1;

    DEBUG_LOG("WORLD: Received (CMSG_GUILD_BANK_QUERY_TAB) TabId %u", TabId);

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(goGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        return;

    uint32 GuildId = GetPlayer()->GetGuildId();
    if (!GuildId)
        return;

    Guild* pGuild = sGuildMgr.GetGuildById(GuildId);
    if (!pGuild)
        return;

    if (TabId >= pGuild->GetPurchasedTabs())
        return;

    pGuild->DisplayGuildBankContent(this, TabId);
}

void WorldSession::HandleGuildBankDepositMoney(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received (CMSG_GUILD_BANK_DEPOSIT_MONEY)");

    ObjectGuid goGuid;
    uint64 money;
    recv_data >> goGuid >> money;

    if (!money)
        return;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(goGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        return;

    if (GetPlayer()->GetMoney() < money)
        return;

    uint32 GuildId = GetPlayer()->GetGuildId();
    if (!GuildId)
        return;

    Guild* pGuild = sGuildMgr.GetGuildById(GuildId);
    if (!pGuild)
        return;

    CharacterDatabase.BeginTransaction();

    pGuild->SetBankMoney(pGuild->GetGuildBankMoney() + money);
    GetPlayer()->ModifyMoney(-int64(money));
    GetPlayer()->SaveGoldToDB();

    CharacterDatabase.CommitTransaction();

    // logging money
    if (_player->GetSession()->GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
    {
        sLog.outCommand(_player->GetSession()->GetAccountId(), "GM %s (Account: %u) deposit money (Amount: %u) to guild bank (Guild ID %u)",
                        _player->GetName(), _player->GetSession()->GetAccountId(), money, GuildId);
    }

    // log
    pGuild->LogBankEvent(GUILD_BANK_LOG_DEPOSIT_MONEY, uint8(0), GetPlayer()->GetGUIDLow(), money);

    pGuild->DisplayGuildBankTabsInfo(this);
    pGuild->DisplayGuildBankContent(this, 0);
    pGuild->DisplayGuildBankMoneyUpdate(this);
}

void WorldSession::HandleGuildBankWithdrawMoney(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received (CMSG_GUILD_BANK_WITHDRAW_MONEY)");

    ObjectGuid goGuid;
    uint64 money;
    recv_data >> goGuid >> money;

    if (!money)
        return;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(goGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        return;

    uint32 GuildId = GetPlayer()->GetGuildId();
    if (GuildId == 0)
        return;

    Guild* pGuild = sGuildMgr.GetGuildById(GuildId);
    if (!pGuild)
        return;

    if (pGuild->GetGuildBankMoney() < money)                // not enough money in bank
        return;

    if (!pGuild->HasRankRight(GetPlayer()->GetRank(), GR_RIGHT_WITHDRAW_GOLD))
        return;

    CharacterDatabase.BeginTransaction();

    if (!pGuild->MemberMoneyWithdraw(money, GetPlayer()->GetGUIDLow()))
    {
        CharacterDatabase.RollbackTransaction();
        return;
    }

    GetPlayer()->ModifyMoney(money);
    GetPlayer()->SaveGoldToDB();

    CharacterDatabase.CommitTransaction();

    // Log
    pGuild->LogBankEvent(GUILD_BANK_LOG_WITHDRAW_MONEY, uint8(0), GetPlayer()->GetGUIDLow(), money);

    pGuild->SendMoneyInfo(this, GetPlayer()->GetGUIDLow());
    pGuild->DisplayGuildBankTabsInfo(this);
    pGuild->DisplayGuildBankContent(this, 0);
    pGuild->DisplayGuildBankMoneyUpdate(this);
}

void WorldSession::HandleGuildBankSwapItems(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received (CMSG_GUILD_BANK_SWAP_ITEMS)");

    ObjectGuid goGuid;
    uint8 BankToBank;

    uint8 BankTab, BankTabSlot, AutoStore;
    uint8 PlayerSlot = NULL_SLOT;
    uint8 PlayerBag = NULL_BAG;
    uint8 BankTabDst, BankTabSlotDst, unk2;
    uint8 ToChar = 1;
    uint32 ItemEntry, unk1;
    uint32 AutoStoreCount = 0;
    uint32 SplitedAmount = 0;

    recv_data >> goGuid >> BankToBank;

    uint32 GuildId = GetPlayer()->GetGuildId();
    if (!GuildId)
    {
        recv_data.rfinish();                                // prevent additional spam at rejected packet
        return;
    }

    Guild* pGuild = sGuildMgr.GetGuildById(GuildId);
    if (!pGuild)
    {
        recv_data.rfinish();                                // prevent additional spam at rejected packet
        return;
    }

    if (BankToBank)
    {
        recv_data >> BankTabDst;
        recv_data >> BankTabSlotDst;
        recv_data >> unk1;                                  // always 0
        recv_data >> BankTab;
        recv_data >> BankTabSlot;
        recv_data >> ItemEntry;
        recv_data >> unk2;                                  // always 0
        recv_data >> SplitedAmount;

        if (BankTabSlotDst >= GUILD_BANK_MAX_SLOTS ||
                (BankTabDst == BankTab && BankTabSlotDst == BankTabSlot) ||
                BankTab >= pGuild->GetPurchasedTabs() ||
                BankTabDst >= pGuild->GetPurchasedTabs())
        {
            recv_data.rfinish();                            // prevent additional spam at rejected packet
            return;
        }
    }
    else
    {
        recv_data >> BankTab;
        recv_data >> BankTabSlot;
        recv_data >> ItemEntry;
        recv_data >> AutoStore;
        if (AutoStore)
        {
            recv_data >> AutoStoreCount;
            recv_data.read_skip<uint8>();                   // ToChar (?), always and expected to be 1 (autostore only triggered in guild->ToChar)
            recv_data.read_skip<uint32>();                  // unknown, always 0
        }
        else
        {
            recv_data >> PlayerBag;
            recv_data >> PlayerSlot;
            recv_data >> ToChar;
            recv_data >> SplitedAmount;
        }

        if ((BankTabSlot >= GUILD_BANK_MAX_SLOTS && BankTabSlot != 0xFF) ||
                BankTab >= pGuild->GetPurchasedTabs())
        {
            recv_data.rfinish();                            // prevent additional spam at rejected packet
            return;
        }
    }

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(goGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        return;

    // Bank <-> Bank
    if (BankToBank)
    {
        pGuild->SwapItems(_player, BankTab, BankTabSlot, BankTabDst, BankTabSlotDst, SplitedAmount);
        return;
    }

    // Player <-> Bank

    // allow work with inventory only
    if (!Player::IsInventoryPos(PlayerBag, PlayerSlot) && !(PlayerBag == NULL_BAG && PlayerSlot == NULL_SLOT))
    {
        _player->SendEquipError(EQUIP_ERR_NONE, NULL, NULL);
        return;
    }

    // BankToChar swap or char to bank remaining
    if (ToChar)                                             // Bank -> Char cases
        pGuild->MoveFromBankToChar(_player, BankTab, BankTabSlot, PlayerBag, PlayerSlot, SplitedAmount);
    else                                                    // Char -> Bank cases
        pGuild->MoveFromCharToBank(_player, PlayerBag, PlayerSlot, BankTab, BankTabSlot, SplitedAmount);
}

void WorldSession::HandleGuildBankBuyTab(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received (CMSG_GUILD_BANK_BUY_TAB)");

    ObjectGuid goGuid;
    uint8 TabId;

    recv_data >> goGuid;
    recv_data >> TabId;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(goGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        return;

    uint32 GuildId = GetPlayer()->GetGuildId();
    if (!GuildId)
        return;

    Guild* pGuild = sGuildMgr.GetGuildById(GuildId);
    if (!pGuild)
        return;

    // m_PurchasedTabs = 0 when buying Tab 0, that is why this check can be made
    // also don't allow buy tabs that are obtained through guild perks
    if (TabId != pGuild->GetPurchasedTabs() || TabId >= GUILD_BANK_MAX_BOUGHT_TABS)
        return;

    uint64 TabCost = GetGuildBankTabPrice(TabId) * GOLD;
    if (!TabCost)
        return;

    if (GetPlayer()->GetMoney() < TabCost)                  // Should not happen, this is checked by client
        return;

    // Go on with creating tab
    pGuild->CreateNewBankTab();
    GetPlayer()->ModifyMoney(-int64(TabCost));
    pGuild->SetBankRightsAndSlots(GetPlayer()->GetRank(), TabId, GUILD_BANK_RIGHT_FULL, WITHDRAW_SLOT_UNLIMITED, true);
    pGuild->Roster();                                       // broadcast for tab rights update
    pGuild->DisplayGuildBankTabsInfo(this);
}

void WorldSession::HandleGuildBankUpdateTab(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received (CMSG_GUILD_BANK_UPDATE_TAB)");

    ObjectGuid goGuid;
    uint8 TabId;
    std::string Name;
    std::string IconIndex;

    recv_data >> goGuid;
    recv_data >> TabId;
    recv_data >> Name;
    recv_data >> IconIndex;

    if (Name.empty())
        return;

    if (IconIndex.empty())
        return;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(goGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        return;

    uint32 GuildId = GetPlayer()->GetGuildId();
    if (!GuildId)
        return;

    Guild* pGuild = sGuildMgr.GetGuildById(GuildId);
    if (!pGuild)
        return;

    if (TabId >= pGuild->GetPurchasedTabs())
        return;

    if (!pGuild->HasRankRight(GetPlayer()->GetRank(), GR_RIGHT_MODIFY_BANK_TABS))
        return;

    pGuild->SetGuildBankTabInfo(TabId, Name, IconIndex);
    pGuild->DisplayGuildBankTabsInfo(this);
    pGuild->DisplayGuildBankContent(this, TabId);
}

void WorldSession::HandleGuildBankLogQuery(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received (CMSG_GUILD_BANK_LOG_QUERY)");

    uint32 TabId;
    recv_data >> TabId;

    uint32 GuildId = GetPlayer()->GetGuildId();
    if (!GuildId)
        return;

    Guild* pGuild = sGuildMgr.GetGuildById(GuildId);
    if (!pGuild)
        return;

    // GUILD_BANK_MAX_TABS send by client for money log
    if (TabId >= pGuild->GetPurchasedTabs() && TabId != GUILD_BANK_MAX_TABS)
        return;

    pGuild->DisplayGuildBankLogs(this, TabId);
}

void WorldSession::HandleQueryGuildBankTabText(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_QUERY_GUILD_BANK_TEXT");

    uint32 TabId;
    recv_data >> TabId;

    uint32 GuildId = GetPlayer()->GetGuildId();
    if (!GuildId)
        return;

    Guild* pGuild = sGuildMgr.GetGuildById(GuildId);
    if (!pGuild)
        return;

    if (TabId >= pGuild->GetPurchasedTabs())
        return;

    pGuild->SendGuildBankTabText(this, TabId);
}

void WorldSession::HandleSetGuildBankTabText(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_SET_GUILD_BANK_TEXT");

    uint32 TabId;
    std::string Text;

    recv_data >> TabId;
    Text = recv_data.ReadString(recv_data.ReadBits(14));

    uint32 GuildId = GetPlayer()->GetGuildId();
    if (!GuildId)
        return;

    Guild* pGuild = sGuildMgr.GetGuildById(GuildId);
    if (!pGuild)
        return;

    if (TabId >= pGuild->GetPurchasedTabs())
        return;

    pGuild->SetGuildBankTabText(TabId, Text);
}

void WorldSession::SendSaveGuildEmblem(uint32 msg)
{
    WorldPacket data(MSG_SAVE_GUILD_EMBLEM, 4);
    data << uint32(msg);                                    // not part of guild
    SendPacket(&data);
}

void WorldSession::HandleGuildQueryRanksOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_QUERY_RANKS");

    ObjectGuid guildGuid;

    recv_data.ReadGuidMask<2, 3, 0, 6, 4, 7, 5, 1>(guildGuid);
    recv_data.ReadGuidBytes<3, 4, 5, 7, 1, 0, 6, 2>(guildGuid);
    if (Guild* guild = sGuildMgr.GetGuildByGuid(guildGuid))
        guild->QueryRanks(this);
}

void WorldSession::HandleGuildAutoDeclineToggleOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_GUILD_AUTO_DECLINE_TOGGLE");

    bool apply;
    recv_data >> apply;

    GetPlayer()->ApplyModFlag(PLAYER_FLAGS, PLAYER_FLAGS_AUTO_DECLINE_GUILDS, apply);
}
