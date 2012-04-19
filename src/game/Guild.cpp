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

#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Player.h"
#include "Opcodes.h"
#include "ObjectMgr.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Chat.h"
#include "SocialMgr.h"
#include "Util.h"
#include "Language.h"
#include "World.h"

//// MemberSlot ////////////////////////////////////////////
void MemberSlot::SetMemberStats(Player* player)
{
    Name   = player->GetName();
    Level  = player->getLevel();
    Class  = player->getClass();
    ZoneId = player->IsInWorld() ? player->GetZoneId() : player->GetCachedZoneId();
}

void MemberSlot::UpdateLogoutTime()
{
    LogoutTime = time(NULL);
}

void MemberSlot::SetPNOTE(std::string pnote)
{
    Pnote = pnote;

    // pnote now can be used for encoding to DB
    CharacterDatabase.escape_string(pnote);
    CharacterDatabase.PExecute("UPDATE guild_member SET pnote = '%s' WHERE guid = '%u'", pnote.c_str(), guid.GetCounter());
}

void MemberSlot::SetOFFNOTE(std::string offnote)
{
    OFFnote = offnote;

    // offnote now can be used for encoding to DB
    CharacterDatabase.escape_string(offnote);
    CharacterDatabase.PExecute("UPDATE guild_member SET offnote = '%s' WHERE guid = '%u'", offnote.c_str(), guid.GetCounter());
}

void MemberSlot::ChangeRank(uint32 newRank)
{
    RankId = newRank;

    Player *player = sObjectMgr.GetPlayer(guid);
    // If player not online data in data field will be loaded from guild tabs no need to update it !!
    if (player)
        player->SetRank(newRank);

    CharacterDatabase.PExecute("UPDATE guild_member SET rank='%u' WHERE guid='%u'", newRank, guid.GetCounter());
}

//// Guild /////////////////////////////////////////////////

Guild::Guild()
{
    m_Id = 0;
    m_Name = "";
    GINFO = MOTD = "";
    m_EmblemStyle = 0;
    m_EmblemColor = 0;
    m_BorderStyle = 0;
    m_BorderColor = 0;
    m_BackgroundColor = 0;
    m_accountsNumber = 0;

    m_CreatedDate = 0;

    m_GuildBankMoney = 0;

    m_GuildEventLogNextGuid = 0;
    m_GuildBankEventLogNextGuid_Money = 0;
    for (uint8 i = 0; i < GUILD_BANK_MAX_TABS; ++i)
        m_GuildBankEventLogNextGuid_Item[i] = 0;
}

Guild::~Guild()
{
    DeleteGuildBankItems();
}

bool Guild::Create(Player* leader, std::string gname)
{
    if (sGuildMgr.GetGuildByName(gname))
        return false;

    WorldSession* lSession = leader->GetSession();
    if (!lSession)
        return false;

    m_LeaderGuid = leader->GetObjectGuid();
    m_Name = gname;
    GINFO = "";
    MOTD = "No message set.";
    m_GuildBankMoney = 0;
    m_Id = sObjectMgr.GenerateGuildId();
    m_CreatedDate = time(0);

    DEBUG_LOG("GUILD: creating guild %s to leader: %s", gname.c_str(), m_LeaderGuid.GetString().c_str());

    // gname already assigned to Guild::name, use it to encode string for DB
    CharacterDatabase.escape_string(gname);

    std::string dbGINFO = GINFO;
    std::string dbMOTD = MOTD;
    CharacterDatabase.escape_string(dbGINFO);
    CharacterDatabase.escape_string(dbMOTD);

    CharacterDatabase.BeginTransaction();
    // CharacterDatabase.PExecute("DELETE FROM guild WHERE guildid='%u'", Id); - MAX(guildid)+1 not exist
    CharacterDatabase.PExecute("DELETE FROM guild_member WHERE guildid='%u'", m_Id);
    CharacterDatabase.PExecute("INSERT INTO guild (guildid,name,leaderguid,info,motd,createdate,EmblemStyle,EmblemColor,BorderStyle,BorderColor,BackgroundColor,BankMoney) "
        "VALUES('%u','%s','%u', '%s', '%s','" UI64FMTD "','%u','%u','%u','%u','%u','" UI64FMTD "')",
        m_Id, gname.c_str(), m_LeaderGuid.GetCounter(), dbGINFO.c_str(), dbMOTD.c_str(), uint64(m_CreatedDate), m_EmblemStyle, m_EmblemColor, m_BorderStyle, m_BorderColor, m_BackgroundColor, m_GuildBankMoney);
    CharacterDatabase.CommitTransaction();

    CreateDefaultGuildRanks(lSession->GetSessionDbLocaleIndex());

    return AddMember(m_LeaderGuid, (uint32)GR_GUILDMASTER);
}

void Guild::CreateDefaultGuildRanks(int locale_idx)
{
    CharacterDatabase.PExecute("DELETE FROM guild_rank WHERE guildid='%u'", m_Id);
    CharacterDatabase.PExecute("DELETE FROM guild_bank_right WHERE guildid = '%u'", m_Id);

    CreateRank(sObjectMgr.GetMangosString(LANG_GUILD_MASTER, locale_idx),   GR_RIGHT_ALL);
    CreateRank(sObjectMgr.GetMangosString(LANG_GUILD_OFFICER, locale_idx),  GR_RIGHT_ALL);
    CreateRank(sObjectMgr.GetMangosString(LANG_GUILD_VETERAN, locale_idx),  GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
    CreateRank(sObjectMgr.GetMangosString(LANG_GUILD_MEMBER, locale_idx),   GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
    CreateRank(sObjectMgr.GetMangosString(LANG_GUILD_INITIATE, locale_idx), GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);

    SetBankMoneyPerDay((uint32)GR_GUILDMASTER, WITHDRAW_MONEY_UNLIMITED);
}

bool Guild::AddMember(ObjectGuid plGuid, uint32 plRank)
{
    Player* pl = sObjectMgr.GetPlayer(plGuid);
    if (pl)
    {
        if (pl->GetGuildId() != 0)
            return false;
    }
    else
    {
        if (Player::GetGuildIdFromDB(plGuid) != 0)          // player already in guild
            return false;
    }

    // remove all player signs from another petitions
    // this will be prevent attempt joining player to many guilds and corrupt guild data integrity
    Player::RemovePetitionsAndSigns(plGuid, 9);

    uint32 lowguid = plGuid.GetCounter();

    // fill player data
    MemberSlot newmember;

    newmember.guid = plGuid;

    if (pl)
    {
        newmember.accountId = pl->GetSession()->GetAccountId();
        newmember.Name   = pl->GetName();
        newmember.Level  = pl->getLevel();
        newmember.Class  = pl->getClass();
        newmember.ZoneId = pl->GetZoneId();
    }
    else
    {
        //                                                     0    1     2     3    4
        QueryResult *result = CharacterDatabase.PQuery("SELECT name,level,class,zone,account FROM characters WHERE guid = '%u'", lowguid);
        if (!result)
            return false;                                   // player doesn't exist

        Field *fields    = result->Fetch();
        newmember.Name   = fields[0].GetCppString();
        newmember.Level  = fields[1].GetUInt8();
        newmember.Class  = fields[2].GetUInt8();
        newmember.ZoneId = fields[3].GetUInt32();
        newmember.accountId = fields[4].GetInt32();
        delete result;
        if (newmember.Level < 1 || newmember.Level > STRONG_MAX_LEVEL ||
            !((1 << (newmember.Class-1)) & CLASSMASK_ALL_PLAYABLE))
        {
            sLog.outError("%s has a broken data in field `characters` table, cannot add him to guild.", plGuid.GetString().c_str());
            return false;
        }
    }

    newmember.RankId  = plRank;
    newmember.OFFnote = (std::string)"";
    newmember.Pnote   = (std::string)"";
    newmember.LogoutTime = time(NULL);
    newmember.BankResetTimeMoney = 0;                       // this will force update at first query
    for (int i = 0; i < GUILD_BANK_MAX_TABS; ++i)
        newmember.BankResetTimeTab[i] = 0;
    members[lowguid] = newmember;

    std::string dbPnote   = newmember.Pnote;
    std::string dbOFFnote = newmember.OFFnote;
    CharacterDatabase.escape_string(dbPnote);
    CharacterDatabase.escape_string(dbOFFnote);

    CharacterDatabase.PExecute("INSERT INTO guild_member (guildid,guid,rank,pnote,offnote) VALUES ('%u', '%u', '%u','%s','%s')",
        m_Id, lowguid, newmember.RankId, dbPnote.c_str(), dbOFFnote.c_str());

    // If player not in game data in data field will be loaded from guild tables, no need to update it!!
    if (pl)
    {
        pl->SetInGuild(m_Id);
        pl->SetRank(newmember.RankId);
        pl->SetGuildIdInvited(0);
    }

    UpdateAccountsNumber();

    return true;
}

void Guild::SetMOTD(std::string motd)
{
    MOTD = motd;

    // motd now can be used for encoding to DB
    CharacterDatabase.escape_string(motd);
    CharacterDatabase.PExecute("UPDATE guild SET motd='%s' WHERE guildid='%u'", motd.c_str(), m_Id);
}

void Guild::SetGINFO(std::string ginfo)
{
    GINFO = ginfo;

    // ginfo now can be used for encoding to DB
    CharacterDatabase.escape_string(ginfo);
    CharacterDatabase.PExecute("UPDATE guild SET info='%s' WHERE guildid='%u'", ginfo.c_str(), m_Id);
}

bool Guild::LoadGuildFromDB(QueryResult *guildDataResult)
{
    if (!guildDataResult)
        return false;

    Field *fields = guildDataResult->Fetch();

    m_Id              = fields[0].GetUInt32();
    m_Name            = fields[1].GetCppString();
    m_LeaderGuid      = ObjectGuid(HIGHGUID_PLAYER, fields[2].GetUInt32());
    m_EmblemStyle     = fields[3].GetUInt32();
    m_EmblemColor     = fields[4].GetUInt32();
    m_BorderStyle     = fields[5].GetUInt32();
    m_BorderColor     = fields[6].GetUInt32();
    m_BackgroundColor = fields[7].GetUInt32();
    GINFO             = fields[8].GetCppString();
    MOTD              = fields[9].GetCppString();
    m_CreatedDate     = time_t(fields[10].GetUInt64());
    m_GuildBankMoney  = fields[11].GetUInt64();

    uint32 purchasedTabs   = fields[12].GetUInt32();

    if (purchasedTabs > GUILD_BANK_MAX_TABS)
        purchasedTabs = GUILD_BANK_MAX_TABS;

    m_TabListMap.resize(purchasedTabs);

    for (uint8 i = 0; i < purchasedTabs; ++i)
        m_TabListMap[i] = new GuildBankTab;

    return true;
}

bool Guild::CheckGuildStructure()
{
    // Repair the structure of guild
    // If the guildmaster doesn't exist or isn't the member of guild
    // attempt to promote another member
    int32 GM_rights = GetRank(m_LeaderGuid);
    if (GM_rights == -1)
    {
        if (DelMember(m_LeaderGuid))
            return false;                                   // guild will disbanded and deleted in caller
    }
    else if (GM_rights != GR_GUILDMASTER)
        SetLeader(m_LeaderGuid);

    // Allow only 1 guildmaster, set other to officer
    for (MemberList::iterator itr = members.begin(); itr != members.end(); ++itr)
        if (itr->second.RankId == GR_GUILDMASTER && m_LeaderGuid != itr->second.guid)
            itr->second.ChangeRank(GR_OFFICER);

    return true;
}

bool Guild::LoadRanksFromDB(QueryResult *guildRanksResult)
{
    if (!guildRanksResult)
    {
        sLog.outError("Guild %u has broken `guild_rank` data, creating new...",m_Id);
        CreateDefaultGuildRanks(0);
        return true;
    }

    Field *fields;
    bool broken_ranks = false;

    // GUILD RANKS are sequence starting from 0 = GUILD_MASTER (ALL PRIVILEGES) to max 9 (lowest privileges)
    // the lower rank id is considered higher rank - so promotion does rank-- and demotion does rank++
    // between ranks in sequence cannot be gaps - so 0,1,2,4 cannot be
    // min ranks count is 5 and max is 10.

    do
    {
        fields = guildRanksResult->Fetch();
        //condition that would be true when all ranks in QueryResult will be processed and guild without ranks is being processed
        if (!fields)
            break;

        uint32 guildId       = fields[0].GetUInt32();
        if (guildId < m_Id)
        {
            //there is in table guild_rank record which doesn't have guildid in guild table, report error
            sLog.outErrorDb("Guild %u does not exist but it has a record in guild_rank table, deleting it!", guildId);
            CharacterDatabase.PExecute("DELETE FROM guild_rank WHERE guildid = '%u'", guildId);
            continue;
        }

        if (guildId > m_Id)                                 //we loaded all ranks for this guild already, break cycle
            break;

        uint32 rankID        = fields[1].GetUInt32();
        std::string rankName = fields[2].GetCppString();
        uint32 rankRights    = fields[3].GetUInt32();
        uint32 rankMoney     = fields[4].GetUInt32();

        if (rankID != m_Ranks.size())                       // guild_rank.ids are sequence 0,1,2,3..
            broken_ranks =  true;

        // first rank is guildmaster, prevent loss leader rights
        if (m_Ranks.empty())
            rankRights |= GR_RIGHT_ALL;

        AddRank(rankName, rankRights, rankMoney);
    } while( guildRanksResult->NextRow() );

    if (m_Ranks.size() < GUILD_RANKS_MIN_COUNT)             // if too few ranks, renew them
    {
        m_Ranks.clear();
        sLog.outError("Guild %u has broken `guild_rank` data, creating new...", m_Id);
        CreateDefaultGuildRanks(0);                         // 0 is default locale_idx
        broken_ranks = false;
    }
    // guild_rank have wrong numbered ranks, repair
    if (broken_ranks)
    {
        sLog.outError("Guild %u has broken `guild_rank` data, repairing...", m_Id);
        CharacterDatabase.BeginTransaction();
        CharacterDatabase.PExecute("DELETE FROM guild_rank WHERE guildid='%u'", m_Id);
        for(size_t i = 0; i < m_Ranks.size(); ++i)
        {
            std::string name = m_Ranks[i].Name;
            uint32 rights = m_Ranks[i].Rights;
            CharacterDatabase.escape_string(name);
            CharacterDatabase.PExecute( "INSERT INTO guild_rank (guildid,rid,rname,rights) VALUES ('%u', '%u', '%s', '%u')", m_Id, uint32(i), name.c_str(), rights);
        }
        CharacterDatabase.CommitTransaction();
    }

    return true;
}

bool Guild::LoadMembersFromDB(QueryResult *guildMembersResult)
{
    if (!guildMembersResult)
        return false;

    do
    {
        Field *fields = guildMembersResult->Fetch();
        // this condition will be true when all rows in QueryResult are processed and new guild without members is going to be loaded - prevent crash
        if (!fields)
            break;
        uint32 guildId       = fields[0].GetUInt32();
        if (guildId < m_Id)
        {
            // there is in table guild_member record which doesn't have guildid in guild table, report error
            sLog.outErrorDb("Guild %u does not exist but it has a record in guild_member table, deleting it!", guildId);
            CharacterDatabase.PExecute("DELETE FROM guild_member WHERE guildid = '%u'", guildId);
            continue;
        }

        if (guildId > m_Id)
            // we loaded all members for this guild already, break cycle
            break;

        MemberSlot newmember;
        uint32 lowguid = fields[1].GetUInt32();
        newmember.guid = ObjectGuid(HIGHGUID_PLAYER, lowguid);
        newmember.RankId = fields[2].GetUInt32();
        // don't allow member to have not existing rank!
        if (newmember.RankId >= m_Ranks.size())
            newmember.RankId = GetLowestRank();

        newmember.Pnote                 = fields[3].GetCppString();
        newmember.OFFnote               = fields[4].GetCppString();
        newmember.BankResetTimeMoney    = fields[5].GetUInt32();
        newmember.BankRemMoney          = fields[6].GetUInt32();
        for (int i = 0; i < GUILD_BANK_MAX_TABS; ++i)
        {
            newmember.BankResetTimeTab[i] = fields[7+(2*i)].GetUInt32();
            newmember.BankRemSlotsTab[i]  = fields[8+(2*i)].GetUInt32();
        }

        newmember.Name                  = fields[19].GetCppString();
        newmember.Level                 = fields[20].GetUInt8();
        newmember.Class                 = fields[21].GetUInt8();
        newmember.ZoneId                = fields[22].GetUInt32();
        newmember.LogoutTime            = fields[23].GetUInt64();
        newmember.accountId             = fields[24].GetInt32();

        // this code will remove not existing character guids from guild
        if (newmember.Level < 1 || newmember.Level > STRONG_MAX_LEVEL) // can be at broken `data` field
        {
            sLog.outError("%s has a broken data in field `characters`.`data`, deleting him from guild!", newmember.guid.GetString().c_str());
            CharacterDatabase.PExecute("DELETE FROM guild_member WHERE guid = '%u'", lowguid);
            continue;
        }
        if (!newmember.ZoneId)
        {
            sLog.outError("%s has broken zone-data", newmember.guid.GetString().c_str());
            // here it will also try the same, to get the zone from characters-table, but additional it tries to find
            // the zone through xy coords .. this is a bit redundant, but shouldn't be called often
            newmember.ZoneId = Player::GetZoneIdFromDB(newmember.guid);
        }
        if (!((1 << (newmember.Class-1)) & CLASSMASK_ALL_PLAYABLE)) // can be at broken `class` field
        {
            sLog.outError("%s has a broken data in field `characters`.`class`, deleting him from guild!", newmember.guid.GetString().c_str());
            CharacterDatabase.PExecute("DELETE FROM guild_member WHERE guid = '%u'", lowguid);
            continue;
        }

        members[lowguid]      = newmember;

    } while (guildMembersResult->NextRow());

    if (members.empty())
        return false;

    UpdateAccountsNumber();

    return true;
}

void Guild::SetLeader(ObjectGuid guid)
{
    MemberSlot* slot = GetMemberSlot(guid);
    if (!slot)
        return;

    m_LeaderGuid = guid;
    slot->ChangeRank(GR_GUILDMASTER);

    CharacterDatabase.PExecute("UPDATE guild SET leaderguid='%u' WHERE guildid='%u'", guid.GetCounter(), m_Id);
}

/**
 * Remove character from guild
 *
 * @param guid          Character that removed from guild
 * @param isDisbanding  Flag set if function called from Guild::Disband, so not need update DB in per-member mode only or leader update
 *
 * @return true, if guild need to be disband and erase (no members or can't setup leader)
 */
bool Guild::DelMember(ObjectGuid guid, bool isDisbanding)
{
    uint32 lowguid = guid.GetCounter();

    // guild master can be deleted when loading guild and guid doesn't exist in characters table
    // or when he is removed from guild by gm command
    if (m_LeaderGuid == guid && !isDisbanding)
    {
        MemberSlot* oldLeader = NULL;
        MemberSlot* best = NULL;
        ObjectGuid newLeaderGUID;
        for (Guild::MemberList::iterator i = members.begin(); i != members.end(); ++i)
        {
            if (i->first == lowguid)
            {
                oldLeader = &(i->second);
                continue;
            }

            if (!best || best->RankId > i->second.RankId)
            {
                best = &(i->second);
                newLeaderGUID = ObjectGuid(HIGHGUID_PLAYER, i->first);
            }
        }

        if (!best)
            return true;

        SetLeader(newLeaderGUID);

        // If player not online data in data field will be loaded from guild tabs no need to update it !!
        if (Player *newLeader = sObjectMgr.GetPlayer(newLeaderGUID))
            newLeader->SetRank(GR_GUILDMASTER);

        // when leader non-exist (at guild load with deleted leader only) not send broadcasts
        if (oldLeader)
        {
            BroadcastEvent(GE_LEADER_CHANGED, oldLeader->Name.c_str(), best->Name.c_str());
            BroadcastEvent(GE_LEFT, guid, oldLeader->Name.c_str());
        }
    }

    members.erase(lowguid);

    Player *player = sObjectMgr.GetPlayer(guid);
    // If player not online data in data field will be loaded from guild tabs no need to update it !!
    if (player)
    {
        player->SetInGuild(0);
        player->SetRank(0);
    }

    CharacterDatabase.PExecute("DELETE FROM guild_member WHERE guid = '%u'", lowguid);

    if (!isDisbanding)
        UpdateAccountsNumber();

    return members.empty();
}

void Guild::BroadcastToGuild(WorldSession *session, const std::string& msg, uint32 language)
{
    if (session && session->GetPlayer() && HasRankRight(session->GetPlayer()->GetRank(),GR_RIGHT_GCHATSPEAK))
    {
        WorldPacket data;
        ChatHandler::FillMessageData(&data, session, CHAT_MSG_GUILD, language, msg.c_str());

        for (MemberList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
        {
            Player *pl = ObjectAccessor::FindPlayer(ObjectGuid(HIGHGUID_PLAYER, itr->first));

            if (pl && pl->GetSession() && HasRankRight(pl->GetRank(),GR_RIGHT_GCHATLISTEN) && !pl->GetSocial()->HasIgnore(session->GetPlayer()->GetObjectGuid()) )
                pl->GetSession()->SendPacket(&data);
        }
    }
}

void Guild::BroadcastToOfficers(WorldSession *session, const std::string& msg, uint32 language)
{
    if (session && session->GetPlayer() && HasRankRight(session->GetPlayer()->GetRank(), GR_RIGHT_OFFCHATSPEAK))
    {
        for(MemberList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
        {
            WorldPacket data;
            ChatHandler::FillMessageData(&data, session, CHAT_MSG_OFFICER, language, msg.c_str());

            Player *pl = ObjectAccessor::FindPlayer(ObjectGuid(HIGHGUID_PLAYER, itr->first));

            if (pl && pl->GetSession() && HasRankRight(pl->GetRank(),GR_RIGHT_OFFCHATLISTEN) && !pl->GetSocial()->HasIgnore(session->GetPlayer()->GetObjectGuid()))
                pl->GetSession()->SendPacket(&data);
        }
    }
}

void Guild::BroadcastPacket(WorldPacket *packet)
{
    for(MemberList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
    {
        Player *player = ObjectAccessor::FindPlayer(ObjectGuid(HIGHGUID_PLAYER, itr->first));
        if (player)
            player->GetSession()->SendPacket(packet);
    }
}

void Guild::BroadcastPacketToRank(WorldPacket *packet, uint32 rankId)
{
    for(MemberList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
    {
        if (itr->second.RankId == rankId)
        {
            Player *player = ObjectAccessor::FindPlayer(ObjectGuid(HIGHGUID_PLAYER, itr->first));
            if (player)
                player->GetSession()->SendPacket(packet);
        }
    }
}

void Guild::CreateRank(std::string name_,uint32 rights)
{
    if (m_Ranks.size() >= GUILD_RANKS_MAX_COUNT)
        return;

    // ranks are sequence 0,1,2,... where 0 means guildmaster
    uint32 new_rank_id = m_Ranks.size();

    AddRank(name_, rights, 0);

    // existing records in db should be deleted before calling this procedure and m_PurchasedTabs must be loaded already

    for (uint32 i = 0; i < uint32(GetPurchasedTabs()); ++i)
    {
        // create bank rights with 0
        CharacterDatabase.PExecute("INSERT INTO guild_bank_right (guildid,TabId,rid) VALUES ('%u','%u','%u')", m_Id, i, new_rank_id);
    }
    // name now can be used for encoding to DB
    CharacterDatabase.escape_string(name_);
    CharacterDatabase.PExecute( "INSERT INTO guild_rank (guildid,rid,rname,rights) VALUES ('%u', '%u', '%s', '%u')", m_Id, new_rank_id, name_.c_str(), rights );
}

void Guild::AddRank(const std::string& name_,uint32 rights, uint32 money)
{
    m_Ranks.push_back(RankInfo(name_,rights,money));
}

void Guild::DelRank()
{
    // client won't allow to have less than GUILD_RANKS_MIN_COUNT ranks in guild
    if (m_Ranks.size() <= GUILD_RANKS_MIN_COUNT)
        return;

    // delete lowest guild_rank
    uint32 rank = GetLowestRank();
    CharacterDatabase.PExecute("DELETE FROM guild_rank WHERE rid>='%u' AND guildid='%u'", rank, m_Id);
    CharacterDatabase.PExecute("DELETE FROM guild_bank_right WHERE rid>='%u' AND guildid='%u'", rank, m_Id);

    m_Ranks.pop_back();
}

std::string Guild::GetRankName(uint32 rankId)
{
    if (rankId >= m_Ranks.size())
        return "<unknown>";

    return m_Ranks[rankId].Name;
}

uint32 Guild::GetRankRights(uint32 rankId)
{
    if (rankId >= m_Ranks.size())
        return 0;

    return m_Ranks[rankId].Rights;
}

void Guild::SetRankName(uint32 rankId, std::string name_)
{
    if (rankId >= m_Ranks.size())
        return;

    m_Ranks[rankId].Name = name_;

    // name now can be used for encoding to DB
    CharacterDatabase.escape_string(name_);
    CharacterDatabase.PExecute("UPDATE guild_rank SET rname='%s' WHERE rid='%u' AND guildid='%u'", name_.c_str(), rankId, m_Id);
}

void Guild::SetRankRights(uint32 rankId, uint32 rights)
{
    if (rankId >= m_Ranks.size())
        return;

    m_Ranks[rankId].Rights = rights;

    CharacterDatabase.PExecute("UPDATE guild_rank SET rights='%u' WHERE rid='%u' AND guildid='%u'", rights, rankId, m_Id);
}

/**
 * Disband guild including cleanup structures and DB
 *
 * Note: guild object need deleted after this in caller code.
 */
void Guild::Disband()
{
    BroadcastEvent(GE_DISBANDED);

    while (!members.empty())
    {
        MemberList::const_iterator itr = members.begin();
        DelMember(ObjectGuid(HIGHGUID_PLAYER, itr->first), true);
    }

    CharacterDatabase.BeginTransaction();
    CharacterDatabase.PExecute("DELETE FROM guild WHERE guildid = '%u'", m_Id);
    CharacterDatabase.PExecute("DELETE FROM guild_rank WHERE guildid = '%u'", m_Id);
    CharacterDatabase.PExecute("DELETE FROM guild_bank_tab WHERE guildid = '%u'", m_Id);

    //Free bank tab used memory and delete items stored in them
    DeleteGuildBankItems(true);

    CharacterDatabase.PExecute("DELETE FROM guild_bank_item WHERE guildid = '%u'", m_Id);
    CharacterDatabase.PExecute("DELETE FROM guild_bank_right WHERE guildid = '%u'", m_Id);
    CharacterDatabase.PExecute("DELETE FROM guild_bank_eventlog WHERE guildid = '%u'", m_Id);
    CharacterDatabase.PExecute("DELETE FROM guild_eventlog WHERE guildid = '%u'", m_Id);
    CharacterDatabase.CommitTransaction();
    sGuildMgr.RemoveGuild(m_Id);
}

void Guild::Roster(WorldSession *session /*= NULL*/)
{
                                                            // we can only guess size
    WorldPacket data(SMSG_GUILD_ROSTER, (4+MOTD.length()+1+GINFO.length()+1+4+m_Ranks.size()*(4+4+GUILD_BANK_MAX_TABS*(4+4))+members.size()*50));
    data << uint32(members.size());
    data << MOTD;
    data << GINFO;

    data << uint32(m_Ranks.size());
    for (RankList::const_iterator ritr = m_Ranks.begin(); ritr != m_Ranks.end(); ++ritr)
    {
        data << uint32(ritr->Rights);
        data << uint32(ritr->BankMoneyPerDay);              // count of: withdraw gold(gold/day) Note: in game set gold, in packet set bronze.
        for (int i = 0; i < GUILD_BANK_MAX_TABS; ++i)
        {
            data << uint32(ritr->TabRight[i]);              // for TAB_i rights: view tabs = 0x01, deposit items =0x02
            data << uint32(ritr->TabSlotPerDay[i]);         // for TAB_i count of: withdraw items(stack/day)
        }
    }
    for (MemberList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
    {
        if (Player *pl = ObjectAccessor::FindPlayer(ObjectGuid(HIGHGUID_PLAYER, itr->first)))
        {
            data << pl->GetObjectGuid();
            data << uint8(1);
            data << pl->GetName();
            data << uint32(itr->second.RankId);
            data << uint8(pl->getLevel());
            data << uint8(pl->getClass());
            data << uint8(0);                               // new 2.4.0
            data << uint32(pl->GetZoneId());
            data << itr->second.Pnote;
            data << itr->second.OFFnote;
        }
        else
        {
            data << ObjectGuid(HIGHGUID_PLAYER, itr->first);
            data << uint8(0);
            data << itr->second.Name;
            data << uint32(itr->second.RankId);
            data << uint8(itr->second.Level);
            data << uint8(itr->second.Class);
            data << uint8(0);                               // new 2.4.0
            data << uint32(itr->second.ZoneId);
            data << float(float(time(NULL)-itr->second.LogoutTime) / DAY);
            data << itr->second.Pnote;
            data << itr->second.OFFnote;
        }
    }
    if (session)
        session->SendPacket(&data);
    else
        BroadcastPacket(&data);
    DEBUG_LOG( "WORLD: Sent (SMSG_GUILD_ROSTER)" );
}

void Guild::Query(WorldSession *session)
{
    WorldPacket data(SMSG_GUILD_QUERY_RESPONSE, (8*32+200));// we can only guess size

    data << uint32(m_Id);
    data << m_Name;

    for (size_t i = 0 ; i < GUILD_RANKS_MAX_COUNT; ++i)     // show always 10 ranks
    {
        if (i < m_Ranks.size())
            data << m_Ranks[i].Name;
        else
            data << uint8(0);                               // null string
    }

    data << uint32(m_EmblemStyle);
    data << uint32(m_EmblemColor);
    data << uint32(m_BorderStyle);
    data << uint32(m_BorderColor);
    data << uint32(m_BackgroundColor);
    data << uint32(0);                                      // probably real ranks count

    session->SendPacket( &data );
    DEBUG_LOG( "WORLD: Sent (SMSG_GUILD_QUERY_RESPONSE)" );
}

void Guild::SetEmblem(uint32 emblemStyle, uint32 emblemColor, uint32 borderStyle, uint32 borderColor, uint32 backgroundColor)
{
    m_EmblemStyle = emblemStyle;
    m_EmblemColor = emblemColor;
    m_BorderStyle = borderStyle;
    m_BorderColor = borderColor;
    m_BackgroundColor = backgroundColor;

    CharacterDatabase.PExecute("UPDATE guild SET EmblemStyle=%u, EmblemColor=%u, BorderStyle=%u, BorderColor=%u, BackgroundColor=%u WHERE guildid = %u", m_EmblemStyle, m_EmblemColor, m_BorderStyle, m_BorderColor, m_BackgroundColor, m_Id);
}

/**
 * Return the number of accounts that are in the guild after possible update if required
 * A player may have many characters in the guild, but with the same account
 */
uint32 Guild::GetAccountsNumber()
{
    // not need recalculation
    if (m_accountsNumber)
        return m_accountsNumber;

    //We use a set to be sure each element will be unique
    std::set<uint32> accountsIdSet;
    for (MemberList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
        accountsIdSet.insert(itr->second.accountId);

    m_accountsNumber = accountsIdSet.size();

    return m_accountsNumber;
}

// *************************************************
// Guild Eventlog part
// *************************************************
// Display guild eventlog
void Guild::DisplayGuildEventLog(WorldSession *session)
{
    // Sending result
    WorldPacket data(MSG_GUILD_EVENT_LOG_QUERY, 0);
    // count, max count == 100
    data << uint8(m_GuildEventLog.size());
    for (GuildEventLog::const_iterator itr = m_GuildEventLog.begin(); itr != m_GuildEventLog.end(); ++itr)
    {
        // Event type
        data << uint8(itr->EventType);
        // Player 1
        data << ObjectGuid(HIGHGUID_PLAYER, itr->PlayerGuid1);
        // Player 2 not for left/join guild events
        if (itr->EventType != GUILD_EVENT_LOG_JOIN_GUILD && itr->EventType != GUILD_EVENT_LOG_LEAVE_GUILD)
            data << ObjectGuid(HIGHGUID_PLAYER, itr->PlayerGuid2);
        // New Rank - only for promote/demote guild events
        if (itr->EventType == GUILD_EVENT_LOG_PROMOTE_PLAYER || itr->EventType == GUILD_EVENT_LOG_DEMOTE_PLAYER)
            data << uint8(itr->NewRank);
        // Event timestamp
        data << uint32(time(NULL)-itr->TimeStamp);
    }
    session->SendPacket(&data);
    DEBUG_LOG("WORLD: Sent (MSG_GUILD_EVENT_LOG_QUERY)");
}

// Load guild eventlog from DB
void Guild::LoadGuildEventLogFromDB()
{
    //                                                     0        1          2            3            4        5
    QueryResult *result = CharacterDatabase.PQuery("SELECT LogGuid, EventType, PlayerGuid1, PlayerGuid2, NewRank, TimeStamp FROM guild_eventlog WHERE guildid=%u ORDER BY TimeStamp DESC,LogGuid DESC LIMIT %u", m_Id, GUILD_EVENTLOG_MAX_RECORDS);
    if (!result)
        return;
    bool isNextLogGuidSet = false;
    //uint32 configCount = sWorld.getConfig(CONFIG_UINT32_GUILD_EVENT_LOG_COUNT);
    // First event in list will be the oldest and the latest event is last event in list
    do
    {
        Field *fields = result->Fetch();
        if (!isNextLogGuidSet)
        {
            m_GuildEventLogNextGuid = fields[0].GetUInt32();
            isNextLogGuidSet = true;
        }
        // Fill entry
        GuildEventLogEntry NewEvent;
        NewEvent.EventType = fields[1].GetUInt8();
        NewEvent.PlayerGuid1 = fields[2].GetUInt32();
        NewEvent.PlayerGuid2 = fields[3].GetUInt32();
        NewEvent.NewRank = fields[4].GetUInt8();
        NewEvent.TimeStamp = fields[5].GetUInt64();

        // There can be a problem if more events have same TimeStamp the ORDER can be broken when fields[0].GetUInt32() == configCount, but
        // events with same timestamp can appear when there is lag, and we naively suppose that mangos isn't laggy
        // but if problem appears, player will see set of guild events that have same timestamp in bad order

        // Add entry to list
        m_GuildEventLog.push_front(NewEvent);

    } while( result->NextRow() );
    delete result;
}

// Add entry to guild eventlog
void Guild::LogGuildEvent(uint8 EventType, ObjectGuid playerGuid1, ObjectGuid playerGuid2, uint8 newRank)
{
    GuildEventLogEntry NewEvent;
    // Create event
    NewEvent.EventType = EventType;
    NewEvent.PlayerGuid1 = playerGuid1.GetCounter();
    NewEvent.PlayerGuid2 = playerGuid2.GetCounter();
    NewEvent.NewRank = newRank;
    NewEvent.TimeStamp = uint32(time(NULL));
    // Count new LogGuid
    m_GuildEventLogNextGuid = (m_GuildEventLogNextGuid + 1) % sWorld.getConfig(CONFIG_UINT32_GUILD_EVENT_LOG_COUNT);
    // Check max records limit
    if (m_GuildEventLog.size() >= GUILD_EVENTLOG_MAX_RECORDS)
        m_GuildEventLog.pop_front();
    // Add event to list
    m_GuildEventLog.push_back(NewEvent);
    // Save event to DB
    CharacterDatabase.PExecute("DELETE FROM guild_eventlog WHERE guildid='%u' AND LogGuid='%u'", m_Id, m_GuildEventLogNextGuid);
    CharacterDatabase.PExecute("INSERT INTO guild_eventlog (guildid, LogGuid, EventType, PlayerGuid1, PlayerGuid2, NewRank, TimeStamp) VALUES ('%u','%u','%u','%u','%u','%u','" UI64FMTD "')",
        m_Id, m_GuildEventLogNextGuid, uint32(NewEvent.EventType), NewEvent.PlayerGuid1, NewEvent.PlayerGuid2, uint32(NewEvent.NewRank), NewEvent.TimeStamp);
}

// *************************************************
// Guild Bank part
// *************************************************
// Bank content related
void Guild::DisplayGuildBankContent(WorldSession *session, uint8 TabId)
{
    GuildBankTab const* tab = m_TabListMap[TabId];

    if (!IsMemberHaveRights(session->GetPlayer()->GetGUIDLow(), TabId, GUILD_BANK_RIGHT_VIEW_TAB))
        return;

    WorldPacket data(SMSG_GUILD_BANK_LIST, 1200);

    data << uint64(GetGuildBankMoney());
    data << uint8(TabId);
                                                            // remaining slots for today
    data << uint32(GetMemberSlotWithdrawRem(session->GetPlayer()->GetGUIDLow(), TabId));
    data << uint8(0);                                       // Tell client that there's no tab info in this packet

    data << uint8(GUILD_BANK_MAX_SLOTS);

    for (int i = 0; i < GUILD_BANK_MAX_SLOTS; ++i)
        AppendDisplayGuildBankSlot(data, tab, i);

    session->SendPacket(&data);

    DEBUG_LOG("WORLD: Sent (SMSG_GUILD_BANK_LIST)");
}

void Guild::DisplayGuildBankMoneyUpdate(WorldSession *session)
{
    WorldPacket data(SMSG_GUILD_BANK_LIST, 8+1+4+1+1);

    data << uint64(GetGuildBankMoney());
    data << uint8(0);                                       // TabId, default 0
    data << uint32(GetMemberSlotWithdrawRem(session->GetPlayer()->GetGUIDLow(), 0));
    data << uint8(0);                                       // Tell that there's no tab info in this packet
    data << uint8(0);                                       // not send items
    BroadcastPacket(&data);

    DEBUG_LOG("WORLD: Sent (SMSG_GUILD_BANK_LIST)");
}

void Guild::DisplayGuildBankContentUpdate(uint8 TabId, int32 slot1, int32 slot2)
{
    GuildBankTab const* tab = m_TabListMap[TabId];

    WorldPacket data(SMSG_GUILD_BANK_LIST, 1200);

    data << uint64(GetGuildBankMoney());
    data << uint8(TabId);

    size_t rempos = data.wpos();
    data << uint32(0);                                      // item withdraw amount, will be filled later
    data << uint8(0);                                       // Tell client that there's no tab info in this packet

    if (slot2 == -1)                                        // single item in slot1
    {
        data << uint8(1);                                   // item count

        AppendDisplayGuildBankSlot(data, tab, slot1);
    }
    else                                                    // 2 items (in slot1 and slot2)
    {
        data << uint8(2);                                   // item count

        if (slot1 > slot2)
            std::swap(slot1, slot2);

        AppendDisplayGuildBankSlot(data, tab, slot1);
        AppendDisplayGuildBankSlot(data, tab, slot2);
    }

    for (MemberList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
    {
        Player *player = ObjectAccessor::FindPlayer(ObjectGuid(HIGHGUID_PLAYER, itr->first));
        if (!player)
            continue;

        if (!IsMemberHaveRights(itr->first,TabId,GUILD_BANK_RIGHT_VIEW_TAB))
            continue;

        data.put<uint32>(rempos, uint32(GetMemberSlotWithdrawRem(player->GetGUIDLow(), TabId)));

        player->GetSession()->SendPacket(&data);
    }

    DEBUG_LOG("WORLD: Sent (SMSG_GUILD_BANK_LIST)");
}

void Guild::DisplayGuildBankContentUpdate(uint8 TabId, GuildItemPosCountVec const& slots)
{
    GuildBankTab const* tab = m_TabListMap[TabId];

    WorldPacket data(SMSG_GUILD_BANK_LIST, 1200);

    data << uint64(GetGuildBankMoney());
    data << uint8(TabId);

    size_t rempos = data.wpos();
    data << uint32(0);                                      // item withdraw amount, will be filled later
    data << uint8(0);                                       // Tell client that there's no tab info in this packet

    data << uint8(slots.size());                            // updates count

    for (GuildItemPosCountVec::const_iterator itr = slots.begin(); itr != slots.end(); ++itr)
        AppendDisplayGuildBankSlot(data, tab, itr->Slot);

    for (MemberList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
    {
        Player *player = ObjectAccessor::FindPlayer(ObjectGuid(HIGHGUID_PLAYER, itr->first));
        if (!player)
            continue;

        if (!IsMemberHaveRights(itr->first,TabId,GUILD_BANK_RIGHT_VIEW_TAB))
            continue;

        data.put<uint32>(rempos,uint32(GetMemberSlotWithdrawRem(player->GetGUIDLow(), TabId)));

        player->GetSession()->SendPacket(&data);
    }

    DEBUG_LOG("WORLD: Sent (SMSG_GUILD_BANK_LIST)");
}

Item* Guild::GetItem(uint8 TabId, uint8 SlotId)
{
    if (TabId >= GetPurchasedTabs() || SlotId >= GUILD_BANK_MAX_SLOTS)
        return NULL;
    return m_TabListMap[TabId]->Slots[SlotId];
}

// *************************************************
// Tab related

void Guild::DisplayGuildBankTabsInfo(WorldSession *session)
{
    WorldPacket data(SMSG_GUILD_BANK_LIST, 500);

    data << uint64(GetGuildBankMoney());
    data << uint8(0);                                       // TabInfo packet must be for TabId 0
    data << uint32(GetMemberSlotWithdrawRem(session->GetPlayer()->GetGUIDLow(), 0));
    data << uint8(1);                                       // Tell client that this packet includes tab info

    data << uint8(GetPurchasedTabs());                      // here is the number of tabs

    for (uint8 i = 0; i < GetPurchasedTabs(); ++i)
    {
        data << m_TabListMap[i]->Name.c_str();
        data << m_TabListMap[i]->Icon.c_str();
    }
    data << uint8(0);                                       // Do not send tab content
    session->SendPacket(&data);

    DEBUG_LOG("WORLD: Sent (SMSG_GUILD_BANK_LIST)");
}

void Guild::CreateNewBankTab()
{
    if (GetPurchasedTabs() >= GUILD_BANK_MAX_TABS)
        return;

    uint32 tabId = GetPurchasedTabs();                      // next free id
    m_TabListMap.push_back(new GuildBankTab);

    CharacterDatabase.BeginTransaction();
    CharacterDatabase.PExecute("DELETE FROM guild_bank_tab WHERE guildid='%u' AND TabId='%u'", m_Id, tabId);
    CharacterDatabase.PExecute("INSERT INTO guild_bank_tab (guildid,TabId) VALUES ('%u','%u')", m_Id, tabId);
    CharacterDatabase.CommitTransaction();
}

void Guild::SetGuildBankTabInfo(uint8 TabId, std::string Name, std::string Icon)
{
    if (m_TabListMap[TabId]->Name == Name && m_TabListMap[TabId]->Icon == Icon)
        return;

    m_TabListMap[TabId]->Name = Name;
    m_TabListMap[TabId]->Icon = Icon;

    CharacterDatabase.escape_string(Name);
    CharacterDatabase.escape_string(Icon);
    CharacterDatabase.PExecute("UPDATE guild_bank_tab SET TabName='%s',TabIcon='%s' WHERE guildid='%u' AND TabId='%u'", Name.c_str(), Icon.c_str(), m_Id, uint32(TabId));
}

uint32 Guild::GetBankRights(uint32 rankId, uint8 TabId) const
{
    if (rankId >= m_Ranks.size() || TabId >= GUILD_BANK_MAX_TABS)
        return 0;

    return m_Ranks[rankId].TabRight[TabId];
}

// *************************************************
// Guild bank loading related

// This load should be called on startup only
void Guild::LoadGuildBankFromDB()
{
    //                                                     0      1        2        3
    QueryResult *result = CharacterDatabase.PQuery("SELECT TabId, TabName, TabIcon, TabText FROM guild_bank_tab WHERE guildid='%u' ORDER BY TabId", m_Id);
    if (!result)
    {
        m_TabListMap.clear();
        return;
    }

    do
    {
        Field *fields = result->Fetch();
        uint8 tabId = fields[0].GetUInt8();
        if (tabId >= GetPurchasedTabs())
        {
            sLog.outError("Table `guild_bank_tab` have not purchased tab %u for guild %u, skipped", tabId, m_Id);
            continue;
        }

        GuildBankTab *NewTab = new GuildBankTab;

        NewTab->Name = fields[1].GetCppString();
        NewTab->Icon = fields[2].GetCppString();
        NewTab->Text = fields[3].GetCppString();

        m_TabListMap[tabId] = NewTab;
    } while (result->NextRow());

    delete result;

    // data needs to be at first place for Item::LoadFromDB
    //                                        0     1     2      3       4          5
    result = CharacterDatabase.PQuery("SELECT data, text, TabId, SlotId, item_guid, item_entry FROM guild_bank_item JOIN item_instance ON item_guid = guid WHERE guildid='%u' ORDER BY TabId", m_Id);
    if (!result)
        return;

    do
    {
        Field *fields = result->Fetch();
        uint8 TabId = fields[2].GetUInt8();
        uint8 SlotId = fields[3].GetUInt8();
        uint32 ItemGuid = fields[4].GetUInt32();
        uint32 ItemEntry = fields[5].GetUInt32();

        if (TabId >= GetPurchasedTabs())
        {
            sLog.outError( "Guild::LoadGuildBankFromDB: Invalid tab for item (GUID: %u id: #%u) in guild bank, skipped.", ItemGuid,ItemEntry);
            continue;
        }

        if (SlotId >= GUILD_BANK_MAX_SLOTS)
        {
            sLog.outError( "Guild::LoadGuildBankFromDB: Invalid slot for item (GUID: %u id: #%u) in guild bank, skipped.", ItemGuid,ItemEntry);
            continue;
        }

        ItemPrototype const *proto = ObjectMgr::GetItemPrototype(ItemEntry);

        if (!proto)
        {
            sLog.outError( "Guild::LoadGuildBankFromDB: Unknown item (GUID: %u id: #%u) in guild bank, skipped.", ItemGuid,ItemEntry);
            continue;
        }

        Item *pItem = NewItemOrBag(proto);
        if (!pItem->LoadFromDB(ItemGuid, fields))
        {
            CharacterDatabase.PExecute("DELETE FROM guild_bank_item WHERE guildid='%u' AND TabId='%u' AND SlotId='%u'", m_Id, uint32(TabId), uint32(SlotId));
            sLog.outError("Item GUID %u not found in item_instance, deleting from Guild Bank!", ItemGuid);
            delete pItem;
            continue;
        }

        pItem->AddToWorld();
        m_TabListMap[TabId]->Slots[SlotId] = pItem;
    } while (result->NextRow());

    delete result;
}

// *************************************************
// Money deposit/withdraw related

void Guild::SendMoneyInfo(WorldSession *session, uint32 LowGuid)
{
    WorldPacket data(MSG_GUILD_BANK_MONEY_WITHDRAWN, 4);
    data << uint32(GetMemberMoneyWithdrawRem(LowGuid));
    session->SendPacket(&data);
    DEBUG_LOG("WORLD: Sent MSG_GUILD_BANK_MONEY_WITHDRAWN");
}

bool Guild::MemberMoneyWithdraw(uint32 amount, uint32 LowGuid)
{
    uint32 MoneyWithDrawRight = GetMemberMoneyWithdrawRem(LowGuid);

    if (MoneyWithDrawRight < amount || GetGuildBankMoney() < amount)
        return false;

    SetBankMoney(GetGuildBankMoney()-amount);

    if (MoneyWithDrawRight < WITHDRAW_MONEY_UNLIMITED)
    {
        MemberList::iterator itr = members.find(LowGuid);
        if (itr == members.end() )
            return false;
        itr->second.BankRemMoney -= amount;
        CharacterDatabase.PExecute("UPDATE guild_member SET BankRemMoney='%u' WHERE guildid='%u' AND guid='%u'",
            itr->second.BankRemMoney, m_Id, LowGuid);
    }
    return true;
}

void Guild::SetBankMoney(int64 money)
{
    if (money < 0)                                          // I don't know how this happens, it does!!
        money = 0;
    m_GuildBankMoney = money;

    CharacterDatabase.PExecute("UPDATE guild SET BankMoney='" UI64FMTD "' WHERE guildid='%u'", money, m_Id);
}

// *************************************************
// Item per day and money per day related

bool Guild::MemberItemWithdraw(uint8 TabId, uint32 LowGuid)
{
    uint32 SlotsWithDrawRight = GetMemberSlotWithdrawRem(LowGuid, TabId);

    if (SlotsWithDrawRight == 0)
        return false;

    if (SlotsWithDrawRight < WITHDRAW_SLOT_UNLIMITED)
    {
        MemberList::iterator itr = members.find(LowGuid);
        if (itr == members.end() )
            return false;
        --itr->second.BankRemSlotsTab[TabId];
        CharacterDatabase.PExecute("UPDATE guild_member SET BankRemSlotsTab%u='%u' WHERE guildid='%u' AND guid='%u'",
            uint32(TabId), itr->second.BankRemSlotsTab[TabId], m_Id, LowGuid);
    }
    return true;
}

bool Guild::IsMemberHaveRights(uint32 LowGuid, uint8 TabId, uint32 rights) const
{
    MemberList::const_iterator itr = members.find(LowGuid);
    if (itr == members.end() )
        return false;

    if (itr->second.RankId == GR_GUILDMASTER)
        return true;

    return (GetBankRights(itr->second.RankId, TabId) & rights) == rights;
}

uint32 Guild::GetMemberSlotWithdrawRem(uint32 LowGuid, uint8 TabId)
{
    MemberList::iterator itr = members.find(LowGuid);
    if (itr == members.end() )
        return 0;

    if (itr->second.RankId == GR_GUILDMASTER)
        return WITHDRAW_SLOT_UNLIMITED;

    if ((GetBankRights(itr->second.RankId,TabId) & GUILD_BANK_RIGHT_VIEW_TAB) != GUILD_BANK_RIGHT_VIEW_TAB)
        return 0;

    uint32 curTime = uint32(time(NULL)/MINUTE);
    if (curTime - itr->second.BankResetTimeTab[TabId] >= 24*HOUR/MINUTE)
    {
        itr->second.BankResetTimeTab[TabId] = curTime;
        itr->second.BankRemSlotsTab[TabId] = GetBankSlotPerDay(itr->second.RankId, TabId);
        CharacterDatabase.PExecute("UPDATE guild_member SET BankResetTimeTab%u='%u', BankRemSlotsTab%u='%u' WHERE guildid='%u' AND guid='%u'",
            uint32(TabId), itr->second.BankResetTimeTab[TabId], uint32(TabId), itr->second.BankRemSlotsTab[TabId], m_Id, LowGuid);
    }
    return itr->second.BankRemSlotsTab[TabId];
}

uint32 Guild::GetMemberMoneyWithdrawRem(uint32 LowGuid)
{
    MemberList::iterator itr = members.find(LowGuid);
    if (itr == members.end() )
        return 0;

    if (itr->second.RankId == GR_GUILDMASTER)
        return WITHDRAW_MONEY_UNLIMITED;

    uint32 curTime = uint32(time(NULL)/MINUTE);             // minutes
                                                            // 24 hours
    if (curTime > itr->second.BankResetTimeMoney + 24*HOUR/MINUTE)
    {
        itr->second.BankResetTimeMoney = curTime;
        itr->second.BankRemMoney = GetBankMoneyPerDay(itr->second.RankId);
        CharacterDatabase.PExecute("UPDATE guild_member SET BankResetTimeMoney='%u', BankRemMoney='%u' WHERE guildid='%u' AND guid='%u'",
            itr->second.BankResetTimeMoney, itr->second.BankRemMoney, m_Id, LowGuid);
    }
    return itr->second.BankRemMoney;
}

void Guild::SetBankMoneyPerDay(uint32 rankId, uint32 money)
{
    if (rankId >= m_Ranks.size())
        return;

    if (rankId == GR_GUILDMASTER)
        money = WITHDRAW_MONEY_UNLIMITED;

    m_Ranks[rankId].BankMoneyPerDay = money;

    for (MemberList::iterator itr = members.begin(); itr != members.end(); ++itr)
        if (itr->second.RankId == rankId)
            itr->second.BankResetTimeMoney = 0;

    CharacterDatabase.PExecute("UPDATE guild_rank SET BankMoneyPerDay='%u' WHERE rid='%u' AND guildid='%u'", money, rankId, m_Id);
    CharacterDatabase.PExecute("UPDATE guild_member SET BankResetTimeMoney='0' WHERE guildid='%u' AND rank='%u'", m_Id, rankId);
}

void Guild::SetBankRightsAndSlots(uint32 rankId, uint8 TabId, uint32 right, uint32 nbSlots, bool db)
{
    if (rankId >= m_Ranks.size() || TabId >= GetPurchasedTabs())
    {
        // TODO remove next line, It is there just to repair existing bug in deleting guild rank
        CharacterDatabase.PExecute("DELETE FROM guild_bank_right WHERE guildid='%u' AND rid='%u' AND TabId='%u'", m_Id, rankId, TabId);
        return;
    }

    if (rankId == GR_GUILDMASTER)
    {
        nbSlots = WITHDRAW_SLOT_UNLIMITED;
        right = GUILD_BANK_RIGHT_FULL;
    }

    m_Ranks[rankId].TabSlotPerDay[TabId] = nbSlots;
    m_Ranks[rankId].TabRight[TabId] = right;

    if (db)
    {
        for (MemberList::iterator itr = members.begin(); itr != members.end(); ++itr)
            if (itr->second.RankId == rankId)
                for (int i = 0; i < GUILD_BANK_MAX_TABS; ++i)
                    itr->second.BankResetTimeTab[i] = 0;

        CharacterDatabase.PExecute("DELETE FROM guild_bank_right WHERE guildid='%u' AND TabId='%u' AND rid='%u'", m_Id, uint32(TabId), rankId);
        CharacterDatabase.PExecute("INSERT INTO guild_bank_right (guildid,TabId,rid,gbright,SlotPerDay) VALUES "
            "('%u','%u','%u','%u','%u')", m_Id, uint32(TabId), rankId, m_Ranks[rankId].TabRight[TabId], m_Ranks[rankId].TabSlotPerDay[TabId]);
        CharacterDatabase.PExecute("UPDATE guild_member SET BankResetTimeTab%u='0' WHERE guildid='%u' AND rank='%u'", uint32(TabId), m_Id, rankId);
    }
}

uint32 Guild::GetBankMoneyPerDay(uint32 rankId)
{
    if (rankId >= m_Ranks.size())
        return 0;

    if (rankId == GR_GUILDMASTER)
        return WITHDRAW_MONEY_UNLIMITED;
    return m_Ranks[rankId].BankMoneyPerDay;
}

uint32 Guild::GetBankSlotPerDay(uint32 rankId, uint8 TabId)
{
    if (rankId >= m_Ranks.size() || TabId >= GUILD_BANK_MAX_TABS)
        return 0;

    if (rankId == GR_GUILDMASTER)
        return WITHDRAW_SLOT_UNLIMITED;
    return m_Ranks[rankId].TabSlotPerDay[TabId];
}

// *************************************************
// Rights per day related

bool Guild::LoadBankRightsFromDB(QueryResult *guildBankTabRightsResult)
{
    if (!guildBankTabRightsResult)
        return true;

    do
    {
        Field *fields      = guildBankTabRightsResult->Fetch();
        // prevent crash when all rights in result are already processed
        if (!fields)
            break;
        uint32 guildId     = fields[0].GetUInt32();
        if (guildId < m_Id)
        {
            // there is in table guild_bank_right record which doesn't have guildid in guild table, report error
            sLog.outErrorDb("Guild %u does not exist but it has a record in guild_bank_right table, deleting it!", guildId);
            CharacterDatabase.PExecute("DELETE FROM guild_bank_right WHERE guildid = '%u'", guildId);
            continue;
        }

        if (guildId > m_Id)
            // we loaded all ranks for this guild bank already, break cycle
            break;
        uint8 TabId        = fields[1].GetUInt8();
        uint32 rankId      = fields[2].GetUInt32();
        uint16 right       = fields[3].GetUInt16();
        uint16 SlotPerDay  = fields[4].GetUInt16();

        SetBankRightsAndSlots(rankId, TabId, right, SlotPerDay, false);

    } while (guildBankTabRightsResult->NextRow());

    return true;
}

// *************************************************
// Bank log related

void Guild::LoadGuildBankEventLogFromDB()
{
    // Money log is in TabId = GUILD_BANK_MONEY_LOGS_TAB

    // uint32 configCount = sWorld.getConfig(CONFIG_UINT32_GUILD_BANK_EVENT_LOG_COUNT);
    // cycle through all purchased guild bank item tabs
    for (uint32 tabId = 0; tabId < uint32(GetPurchasedTabs()); ++tabId)
    {
        //                                                     0        1          2           3            4               5          6
        QueryResult *result = CharacterDatabase.PQuery("SELECT LogGuid, EventType, PlayerGuid, ItemOrMoney, ItemStackCount, DestTabId, TimeStamp FROM guild_bank_eventlog WHERE guildid='%u' AND TabId='%u' ORDER BY TimeStamp DESC,LogGuid DESC LIMIT %u", m_Id, tabId, GUILD_BANK_MAX_LOGS);
        if (!result)
            continue;

        bool isNextLogGuidSet = false;
        do
        {
            Field *fields = result->Fetch();

            GuildBankEventLogEntry NewEvent;
            NewEvent.EventType = fields[1].GetUInt8();
            NewEvent.PlayerGuid = fields[2].GetUInt32();
            NewEvent.ItemOrMoney = fields[3].GetUInt32();
            NewEvent.ItemStackCount = fields[4].GetUInt8();
            NewEvent.DestTabId = fields[5].GetUInt8();
            NewEvent.TimeStamp = fields[6].GetUInt64();

            // if newEvent is moneyEvent, move it to moneyEventTab in DB and report error
            if (NewEvent.isMoneyEvent())
            {
                uint32 logGuid = fields[0].GetUInt32();
                CharacterDatabase.PExecute("UPDATE guild_bank_eventlog SET TabId='%u' WHERE guildid='%u' AND TabId='%u' AND LogGuid='%u'", GUILD_BANK_MONEY_LOGS_TAB, m_Id, tabId, logGuid);
                sLog.outError("GuildBankEventLog ERROR: MoneyEvent LogGuid %u for Guild %u had incorrectly set its TabId to %u, correcting it to %u TabId", logGuid, m_Id, tabId, GUILD_BANK_MONEY_LOGS_TAB);
                continue;
            }
            else
                // add event to list
                // events are ordered from oldest (in beginning) to latest (in the end)
                m_GuildBankEventLog_Item[tabId].push_front(NewEvent);

            if (!isNextLogGuidSet)
            {
                m_GuildBankEventLogNextGuid_Item[tabId] = fields[0].GetUInt32();
                // we don't have to do m_GuildBankEventLogNextGuid_Item[tabId] %= configCount; - it will be done when creating new record
                isNextLogGuidSet = true;
            }
        } while (result->NextRow());
        delete result;
    }

    // special handle for guild bank money log
    //                                                     0        1          2           3            4               5          6
    QueryResult *result = CharacterDatabase.PQuery("SELECT LogGuid, EventType, PlayerGuid, ItemOrMoney, ItemStackCount, DestTabId, TimeStamp FROM guild_bank_eventlog WHERE guildid='%u' AND TabId='%u' ORDER BY TimeStamp DESC,LogGuid DESC LIMIT %u", m_Id, GUILD_BANK_MONEY_LOGS_TAB, GUILD_BANK_MAX_LOGS);
    if (!result)
        return;

    bool isNextMoneyLogGuidSet = false;
    do
    {
        Field *fields = result->Fetch();
        if (!isNextMoneyLogGuidSet)
        {
            m_GuildBankEventLogNextGuid_Money = fields[0].GetUInt32();
            // we don't have to do m_GuildBankEventLogNextGuid_Money %= configCount; - it will be done when creating new record
            isNextMoneyLogGuidSet = true;
        }
        GuildBankEventLogEntry NewEvent;

        NewEvent.EventType = fields[1].GetUInt8();
        NewEvent.PlayerGuid = fields[2].GetUInt32();
        NewEvent.ItemOrMoney = fields[3].GetUInt32();
        NewEvent.ItemStackCount = fields[4].GetUInt8();
        NewEvent.DestTabId = fields[5].GetUInt8();
        NewEvent.TimeStamp = fields[6].GetUInt64();

        // if newEvent is not moneyEvent, then report error
        if (!NewEvent.isMoneyEvent())
            sLog.outError("GuildBankEventLog ERROR: MoneyEvent LogGuid %u for Guild %u is not MoneyEvent - ignoring...", fields[0].GetUInt32(), m_Id);
        else
            // add event to list
            // events are ordered from oldest (in beginning) to latest (in the end)
            m_GuildBankEventLog_Money.push_front(NewEvent);

    } while (result->NextRow());
    delete result;
}

void Guild::DisplayGuildBankLogs(WorldSession *session, uint8 TabId)
{
    if (TabId > GUILD_BANK_MAX_TABS)
        return;

    if (TabId == GUILD_BANK_MAX_TABS)
    {
        // Here we display money logs
        WorldPacket data(MSG_GUILD_BANK_LOG_QUERY, m_GuildBankEventLog_Money.size()*(4*4+1)+1+1);
        data << uint8(TabId);                               // Here GUILD_BANK_MAX_TABS
        data << uint8(m_GuildBankEventLog_Money.size());    // number of log entries
        for (GuildBankEventLog::const_iterator itr = m_GuildBankEventLog_Money.begin(); itr != m_GuildBankEventLog_Money.end(); ++itr)
        {
            data << uint8(itr->EventType);
            data << ObjectGuid(HIGHGUID_PLAYER, itr->PlayerGuid);
            if (itr->EventType == GUILD_BANK_LOG_DEPOSIT_MONEY ||
                itr->EventType == GUILD_BANK_LOG_WITHDRAW_MONEY ||
                itr->EventType == GUILD_BANK_LOG_REPAIR_MONEY ||
                itr->EventType == GUILD_BANK_LOG_UNK1 ||
                itr->EventType == GUILD_BANK_LOG_UNK2)
            {
                data << uint32(itr->ItemOrMoney);
            }
            else
            {
                data << uint32(itr->ItemOrMoney);
                data << uint32(itr->ItemStackCount);
                if (itr->EventType == GUILD_BANK_LOG_MOVE_ITEM || itr->EventType == GUILD_BANK_LOG_MOVE_ITEM2)
                    data << uint8(itr->DestTabId);          // moved tab
            }
            data << uint32(time(NULL) - itr->TimeStamp);
        }
        session->SendPacket(&data);
    }
    else
    {
        // here we display current tab logs
        WorldPacket data(MSG_GUILD_BANK_LOG_QUERY, m_GuildBankEventLog_Item[TabId].size()*(4*4+1+1)+1+1);
        data << uint8(TabId);                               // Here a real Tab Id
                                                            // number of log entries
        data << uint8(m_GuildBankEventLog_Item[TabId].size());
        for (GuildBankEventLog::const_iterator itr = m_GuildBankEventLog_Item[TabId].begin(); itr != m_GuildBankEventLog_Item[TabId].end(); ++itr)
        {
            data << uint8(itr->EventType);
            data << ObjectGuid(HIGHGUID_PLAYER, itr->PlayerGuid);
            if (itr->EventType == GUILD_BANK_LOG_DEPOSIT_MONEY ||
                itr->EventType == GUILD_BANK_LOG_WITHDRAW_MONEY ||
                itr->EventType == GUILD_BANK_LOG_REPAIR_MONEY ||
                itr->EventType == GUILD_BANK_LOG_UNK1 ||
                itr->EventType == GUILD_BANK_LOG_UNK2)
            {
                data << uint32(itr->ItemOrMoney);
            }
            else
            {
                data << uint32(itr->ItemOrMoney);
                data << uint32(itr->ItemStackCount);
                if (itr->EventType == GUILD_BANK_LOG_MOVE_ITEM || itr->EventType == GUILD_BANK_LOG_MOVE_ITEM2)
                    data << uint8(itr->DestTabId);          // moved tab
            }
            data << uint32(time(NULL) - itr->TimeStamp);
        }
        session->SendPacket(&data);
    }
    DEBUG_LOG("WORLD: Sent (MSG_GUILD_BANK_LOG_QUERY)");
}

void Guild::LogBankEvent(uint8 EventType, uint8 TabId, uint32 PlayerGuidLow, uint32 ItemOrMoney, uint8 ItemStackCount, uint8 DestTabId)
{
    // create Event
    GuildBankEventLogEntry NewEvent;
    NewEvent.EventType = EventType;
    NewEvent.PlayerGuid = PlayerGuidLow;
    NewEvent.ItemOrMoney = ItemOrMoney;
    NewEvent.ItemStackCount = ItemStackCount;
    NewEvent.DestTabId = DestTabId;
    NewEvent.TimeStamp = uint32(time(NULL));

    // add new event to the end of event list
    uint32 currentTabId = TabId;
    uint32 currentLogGuid = 0;
    if (NewEvent.isMoneyEvent())
    {
        m_GuildBankEventLogNextGuid_Money = (m_GuildBankEventLogNextGuid_Money + 1) % sWorld.getConfig(CONFIG_UINT32_GUILD_BANK_EVENT_LOG_COUNT);
        currentLogGuid = m_GuildBankEventLogNextGuid_Money;
        currentTabId = GUILD_BANK_MONEY_LOGS_TAB;
        if (m_GuildBankEventLog_Money.size() >= GUILD_BANK_MAX_LOGS)
            m_GuildBankEventLog_Money.pop_front();

        m_GuildBankEventLog_Money.push_back(NewEvent);
    }
    else
    {
        m_GuildBankEventLogNextGuid_Item[TabId] = ((m_GuildBankEventLogNextGuid_Item[TabId]) + 1) % sWorld.getConfig(CONFIG_UINT32_GUILD_BANK_EVENT_LOG_COUNT);
        currentLogGuid = m_GuildBankEventLogNextGuid_Item[TabId];
        if (m_GuildBankEventLog_Item[TabId].size() >= GUILD_BANK_MAX_LOGS)
            m_GuildBankEventLog_Item[TabId].pop_front();

        m_GuildBankEventLog_Item[TabId].push_back(NewEvent);
    }

    // save event to database
    CharacterDatabase.PExecute("DELETE FROM guild_bank_eventlog WHERE guildid='%u' AND LogGuid='%u' AND TabId='%u'", m_Id, currentLogGuid, currentTabId);

    CharacterDatabase.PExecute("INSERT INTO guild_bank_eventlog (guildid,LogGuid,TabId,EventType,PlayerGuid,ItemOrMoney,ItemStackCount,DestTabId,TimeStamp) VALUES ('%u','%u','%u','%u','%u','%u','%u','%u','" UI64FMTD "')",
        m_Id, currentLogGuid, currentTabId, uint32(NewEvent.EventType), NewEvent.PlayerGuid, NewEvent.ItemOrMoney, uint32(NewEvent.ItemStackCount), uint32(NewEvent.DestTabId), NewEvent.TimeStamp);
}

bool Guild::AddGBankItemToDB(uint32 GuildId, uint32 BankTab , uint32 BankTabSlot , uint32 GUIDLow, uint32 Entry )
{
    CharacterDatabase.PExecute("DELETE FROM guild_bank_item WHERE guildid = '%u' AND TabId = '%u'AND SlotId = '%u'", GuildId, BankTab, BankTabSlot);
    CharacterDatabase.PExecute("INSERT INTO guild_bank_item (guildid,TabId,SlotId,item_guid,item_entry) "
        "VALUES ('%u', '%u', '%u', '%u', '%u')", GuildId, BankTab, BankTabSlot, GUIDLow, Entry);
    return true;
}

void Guild::AppendDisplayGuildBankSlot( WorldPacket& data, GuildBankTab const *tab, int slot )
{
    Item *pItem = tab->Slots[slot];
    uint32 entry = pItem ? pItem->GetEntry() : 0;

    data << uint8(slot);
    data << uint32(entry);
    if (entry)
    {
        data << uint32(0);                                  // 3.3.0 (0x8000, 0x8020)
        data << uint32(pItem->GetItemRandomPropertyId());   // random item property id + 8

        if (pItem->GetItemRandomPropertyId())
            data << uint32(pItem->GetItemSuffixFactor());   // SuffixFactor + 4

        data << uint32(pItem->GetCount());                  // +12 ITEM_FIELD_STACK_COUNT
        data << uint32(0);                                  // +16 Unknown value
        data << uint8(0);                                   // +20

        uint8 enchCount = 0;
        size_t enchCountPos = data.wpos();

        data << uint8(enchCount);                           // number of enchantments
        for(uint32 i = PERM_ENCHANTMENT_SLOT; i < MAX_ENCHANTMENT_SLOT; ++i)
        {
            if(uint32 enchId = pItem->GetEnchantmentId(EnchantmentSlot(i)))
            {
                data << uint8(i);
                data << uint32(enchId);
                ++enchCount;
            }
        }
        data.put<uint8>(enchCountPos, enchCount);
    }
}

Item* Guild::StoreItem(uint8 tabId, GuildItemPosCountVec const& dest, Item* pItem )
{
    if (!pItem)
        return NULL;

    Item* lastItem = pItem;

    for (GuildItemPosCountVec::const_iterator itr = dest.begin(); itr != dest.end(); )
    {
        uint8 slot = itr->Slot;
        uint32 count = itr->Count;

        ++itr;

        if (itr == dest.end())
        {
            lastItem = _StoreItem(tabId, slot, pItem, count, false);
            break;
        }

        lastItem = _StoreItem(tabId, slot, pItem, count, true);
    }

    return lastItem;
}

// Return stored item (if stored to stack, it can diff. from pItem). And pItem ca be deleted in this case.
Item* Guild::_StoreItem( uint8 tab, uint8 slot, Item *pItem, uint32 count, bool clone )
{
    if (!pItem)
        return NULL;

    DEBUG_LOG( "GUILD STORAGE: StoreItem tab = %u, slot = %u, item = %u, count = %u", tab, slot, pItem->GetEntry(), count);

    Item* pItem2 = m_TabListMap[tab]->Slots[slot];

    if (!pItem2)
    {
        if (clone)
            pItem = pItem->CloneItem(count);
        else
            pItem->SetCount(count);

        if (!pItem)
            return NULL;

        m_TabListMap[tab]->Slots[slot] = pItem;

        pItem->SetGuidValue(ITEM_FIELD_CONTAINED, ObjectGuid());
        pItem->SetGuidValue(ITEM_FIELD_OWNER, ObjectGuid());
        AddGBankItemToDB(GetId(), tab, slot, pItem->GetGUIDLow(), pItem->GetEntry());
        pItem->FSetState(ITEM_NEW);
        pItem->SaveToDB();                                  // not in inventory and can be save standalone

        return pItem;
    }
    else
    {
        pItem2->SetCount( pItem2->GetCount() + count );
        pItem2->FSetState(ITEM_CHANGED);
        pItem2->SaveToDB();                                 // not in inventory and can be save standalone

        if (!clone)
        {
            pItem->RemoveFromWorld();
            pItem->DeleteFromDB();
            delete pItem;
        }

        return pItem2;
    }
}

void Guild::RemoveItem(uint8 tab, uint8 slot )
{
    m_TabListMap[tab]->Slots[slot] = NULL;
    CharacterDatabase.PExecute("DELETE FROM guild_bank_item WHERE guildid='%u' AND TabId='%u' AND SlotId='%u'",
        GetId(), uint32(tab), uint32(slot));
}

InventoryResult Guild::_CanStoreItem_InSpecificSlot( uint8 tab, uint8 slot, GuildItemPosCountVec &dest, uint32& count, bool swap, Item* pSrcItem ) const
{
    Item* pItem2 = m_TabListMap[tab]->Slots[slot];

    // ignore move item (this slot will be empty at move)
    if (pItem2 == pSrcItem)
        pItem2 = NULL;

    uint32 need_space;

    // empty specific slot - check item fit to slot
    if (!pItem2 || swap)
    {
        // non empty stack with space
        need_space = pSrcItem->GetMaxStackCount();
    }
    // non empty slot, check item type
    else
    {
        // check item type
        if (pItem2->GetEntry() != pSrcItem->GetEntry())
            return EQUIP_ERR_ITEM_CANT_STACK;

        // check free space
        if (pItem2->GetCount() >= pSrcItem->GetMaxStackCount())
            return EQUIP_ERR_ITEM_CANT_STACK;

        need_space = pSrcItem->GetMaxStackCount() - pItem2->GetCount();
    }

    if (need_space > count)
        need_space = count;

    GuildItemPosCount newPosition = GuildItemPosCount(slot, need_space);
    if (!newPosition.isContainedIn(dest))
    {
        dest.push_back(newPosition);
        count -= need_space;
    }

    return EQUIP_ERR_OK;
}

InventoryResult Guild::_CanStoreItem_InTab( uint8 tab, GuildItemPosCountVec &dest, uint32& count, bool merge, Item* pSrcItem, uint8 skip_slot ) const
{
    for (uint32 j = 0; j < GUILD_BANK_MAX_SLOTS; ++j)
    {
        // skip specific slot already processed in first called _CanStoreItem_InSpecificSlot
        if (j == skip_slot)
            continue;

        Item* pItem2 = m_TabListMap[tab]->Slots[j];

        // ignore move item (this slot will be empty at move)
        if (pItem2 == pSrcItem)
            pItem2 = NULL;

        // if merge skip empty, if !merge skip non-empty
        if ((pItem2 != NULL) != merge)
            continue;

        if (pItem2)
        {
            if (pItem2->GetEntry() == pSrcItem->GetEntry() && pItem2->GetCount() < pSrcItem->GetMaxStackCount())
            {
                uint32 need_space = pSrcItem->GetMaxStackCount() - pItem2->GetCount();
                if (need_space > count)
                    need_space = count;

                GuildItemPosCount newPosition = GuildItemPosCount(j, need_space);
                if (!newPosition.isContainedIn(dest))
                {
                    dest.push_back(newPosition);
                    count -= need_space;

                    if (count == 0)
                        return EQUIP_ERR_OK;
                }
            }
        }
        else
        {
            uint32 need_space = pSrcItem->GetMaxStackCount();
            if (need_space > count)
                need_space = count;

            GuildItemPosCount newPosition = GuildItemPosCount(j, need_space);
            if (!newPosition.isContainedIn(dest))
            {
                dest.push_back(newPosition);
                count -= need_space;

                if (count == 0)
                    return EQUIP_ERR_OK;
            }
        }
    }
    return EQUIP_ERR_OK;
}

InventoryResult Guild::CanStoreItem( uint8 tab, uint8 slot, GuildItemPosCountVec &dest, uint32 count, Item *pItem, bool swap ) const
{
    DEBUG_LOG( "GUILD STORAGE: CanStoreItem tab = %u, slot = %u, item = %u, count = %u", tab, slot, pItem->GetEntry(), count);

    if (count > pItem->GetCount())
        return EQUIP_ERR_COULDNT_SPLIT_ITEMS;

    if (pItem->IsSoulBound())
        return EQUIP_ERR_CANT_DROP_SOULBOUND;

    // in specific slot
    if (slot != NULL_SLOT)
    {
        InventoryResult res = _CanStoreItem_InSpecificSlot(tab,slot,dest,count,swap,pItem);
        if (res != EQUIP_ERR_OK)
            return res;

        if (count == 0)
            return EQUIP_ERR_OK;
    }

    // not specific slot or have space for partly store only in specific slot

    // search stack in tab for merge to
    if (pItem->GetMaxStackCount() > 1)
    {
        InventoryResult res = _CanStoreItem_InTab(tab, dest, count, true, pItem, slot);
        if (res != EQUIP_ERR_OK)
            return res;

        if (count == 0)
            return EQUIP_ERR_OK;
    }

    // search free slot in bag for place to
    InventoryResult res = _CanStoreItem_InTab(tab, dest, count, false, pItem, slot);
    if (res != EQUIP_ERR_OK)
        return res;

    if (count == 0)
        return EQUIP_ERR_OK;

    return EQUIP_ERR_BANK_FULL;
}

void Guild::SetGuildBankTabText(uint8 TabId, std::string text)
{
    if (TabId >= GetPurchasedTabs())
        return;

    if (!m_TabListMap[TabId])
        return;

    if (m_TabListMap[TabId]->Text == text)
        return;

    utf8truncate(text, 500);                                // DB and client size limitation

    m_TabListMap[TabId]->Text = text;

    CharacterDatabase.escape_string(text);
    CharacterDatabase.PExecute("UPDATE guild_bank_tab SET TabText='%s' WHERE guildid='%u' AND TabId='%u'", text.c_str(), m_Id, uint32(TabId));

    // announce
    SendGuildBankTabText(NULL,TabId);
}

void Guild::SendGuildBankTabText(WorldSession *session, uint8 TabId)
{
    GuildBankTab const* tab = m_TabListMap[TabId];

    WorldPacket data(MSG_QUERY_GUILD_BANK_TEXT, 1+tab->Text.size()+1);
    data << uint8(TabId);
    data << tab->Text;

    if (session)
        session->SendPacket(&data);
    else
        BroadcastPacket(&data);
}

void Guild::SwapItems(Player * pl, uint8 BankTab, uint8 BankTabSlot, uint8 BankTabDst, uint8 BankTabSlotDst, uint32 SplitedAmount )
{
    // empty operation
    if (BankTab == BankTabDst && BankTabSlot == BankTabSlotDst)
        return;

    Item *pItemSrc = GetItem(BankTab, BankTabSlot);
    if (!pItemSrc)                                      // may prevent crash
        return;

    if (SplitedAmount > pItemSrc->GetCount())
        return;                                         // cheating?
    else if (SplitedAmount == pItemSrc->GetCount())
        SplitedAmount = 0;                              // no split

    Item *pItemDst = GetItem(BankTabDst, BankTabSlotDst);

    if (BankTab != BankTabDst)
    {
        // check dest pos rights (if different tabs)
        if (!IsMemberHaveRights(pl->GetGUIDLow(), BankTabDst, GUILD_BANK_RIGHT_DEPOSIT_ITEM))
            return;

        // check source pos rights (if different tabs)
        uint32 remRight = GetMemberSlotWithdrawRem(pl->GetGUIDLow(), BankTab);
        if (remRight <= 0)
            return;
    }

    if (SplitedAmount)
    {                                                   // Bank -> Bank item split (in empty or non empty slot
        GuildItemPosCountVec dest;
        InventoryResult msg = CanStoreItem(BankTabDst, BankTabSlotDst, dest, SplitedAmount, pItemSrc, false);
        if (msg != EQUIP_ERR_OK)
        {
            pl->SendEquipError( msg, pItemSrc, NULL );
            return;
        }

        Item *pNewItem = pItemSrc->CloneItem( SplitedAmount );
        if (!pNewItem)
        {
            pl->SendEquipError( EQUIP_ERR_ITEM_NOT_FOUND, pItemSrc, NULL );
            return;
        }

        CharacterDatabase.BeginTransaction();

        if (BankTab != BankTabDst)
            LogBankEvent(GUILD_BANK_LOG_MOVE_ITEM, BankTab, pl->GetGUIDLow(), pItemSrc->GetEntry(), SplitedAmount, BankTabDst);

        pl->ItemRemovedQuestCheck( pItemSrc->GetEntry(), SplitedAmount );
        pItemSrc->SetCount( pItemSrc->GetCount() - SplitedAmount );
        pItemSrc->FSetState(ITEM_CHANGED);
        pItemSrc->SaveToDB();                               // not in inventory and can be save standalone
        StoreItem(BankTabDst, dest, pNewItem);
        CharacterDatabase.CommitTransaction();
    }
    else                                                    // non split
    {
        GuildItemPosCountVec gDest;
        InventoryResult msg = CanStoreItem(BankTabDst,BankTabSlotDst,gDest,pItemSrc->GetCount(), pItemSrc, false);
        if (msg == EQUIP_ERR_OK)                            // merge to
        {
            CharacterDatabase.BeginTransaction();

            if (BankTab != BankTabDst)
                LogBankEvent(GUILD_BANK_LOG_MOVE_ITEM, BankTab, pl->GetGUIDLow(), pItemSrc->GetEntry(), pItemSrc->GetCount(), BankTabDst);

            RemoveItem(BankTab, BankTabSlot);
            StoreItem(BankTabDst, gDest, pItemSrc);
            CharacterDatabase.CommitTransaction();
        }
        else                                                // swap
        {
            gDest.clear();
            msg = CanStoreItem(BankTabDst, BankTabSlotDst, gDest, pItemSrc->GetCount(), pItemSrc, true);
            if (msg != EQUIP_ERR_OK)
            {
                pl->SendEquipError( msg, pItemSrc, NULL );
                return;
            }

            GuildItemPosCountVec gSrc;
            msg = CanStoreItem(BankTab, BankTabSlot, gSrc, pItemDst->GetCount(), pItemDst, true);
            if (msg != EQUIP_ERR_OK)
            {
                pl->SendEquipError( msg, pItemDst, NULL );
                return;
            }

            if (BankTab != BankTabDst)
            {
                // check source pos rights (item swapped to src)
                if (!IsMemberHaveRights(pl->GetGUIDLow(), BankTab, GUILD_BANK_RIGHT_DEPOSIT_ITEM))
                    return;

                // check dest pos rights (item swapped to src)
                uint32 remRightDst = GetMemberSlotWithdrawRem(pl->GetGUIDLow(), BankTabDst);
                if (remRightDst <= 0)
                    return;
            }

            CharacterDatabase.BeginTransaction();

            if (BankTab != BankTabDst)
            {
                LogBankEvent(GUILD_BANK_LOG_MOVE_ITEM, BankTab,    pl->GetGUIDLow(), pItemSrc->GetEntry(), pItemSrc->GetCount(), BankTabDst);
                LogBankEvent(GUILD_BANK_LOG_MOVE_ITEM, BankTabDst, pl->GetGUIDLow(), pItemDst->GetEntry(), pItemDst->GetCount(), BankTab);
            }

            RemoveItem(BankTab, BankTabSlot);
            RemoveItem(BankTabDst, BankTabSlotDst);
            StoreItem(BankTab, gSrc, pItemDst);
            StoreItem(BankTabDst, gDest, pItemSrc);
            CharacterDatabase.CommitTransaction();
        }
    }
    DisplayGuildBankContentUpdate(BankTab, BankTabSlot, BankTab == BankTabDst ? BankTabSlotDst : -1);
    if (BankTab != BankTabDst)
        DisplayGuildBankContentUpdate(BankTabDst, BankTabSlotDst);
}


void Guild::MoveFromBankToChar( Player * pl, uint8 BankTab, uint8 BankTabSlot, uint8 PlayerBag, uint8 PlayerSlot, uint32 SplitedAmount)
{
    Item *pItemBank = GetItem(BankTab, BankTabSlot);
    Item *pItemChar = pl->GetItemByPos(PlayerBag, PlayerSlot);

    if (!pItemBank)                                     // Problem to get bank item
        return;

    if (SplitedAmount > pItemBank->GetCount())
        return;                                         // cheating?
    else if (SplitedAmount == pItemBank->GetCount())
        SplitedAmount = 0;                              // no split

    if (SplitedAmount)
    {                                                   // Bank -> Char split to slot (patly move)
        Item *pNewItem = pItemBank->CloneItem( SplitedAmount );
        if (!pNewItem)
        {
            pl->SendEquipError( EQUIP_ERR_ITEM_NOT_FOUND, pItemBank, NULL );
            return;
        }

        ItemPosCountVec dest;
        InventoryResult msg = pl->CanStoreItem(PlayerBag, PlayerSlot, dest, pNewItem, false);
        if (msg != EQUIP_ERR_OK)
        {
            pl->SendEquipError( msg, pNewItem, NULL );
            delete pNewItem;
            return;
        }

        // check source pos rights (item moved to inventory)
        uint32 remRight = GetMemberSlotWithdrawRem(pl->GetGUIDLow(), BankTab);
        if (remRight <= 0)
        {
            delete pNewItem;
            return;
        }

        CharacterDatabase.BeginTransaction();
        LogBankEvent(GUILD_BANK_LOG_WITHDRAW_ITEM, BankTab, pl->GetGUIDLow(), pItemBank->GetEntry(), SplitedAmount);

        pItemBank->SetCount(pItemBank->GetCount()-SplitedAmount);
        pItemBank->FSetState(ITEM_CHANGED);
        pItemBank->SaveToDB();                              // not in inventory and can be save standalone
        pl->MoveItemToInventory(dest, pNewItem, true);
        pl->SaveInventoryAndGoldToDB();

        MemberItemWithdraw(BankTab, pl->GetGUIDLow());
        CharacterDatabase.CommitTransaction();
    }
    else                                                    // Bank -> Char swap with slot (move)
    {
        ItemPosCountVec dest;
        InventoryResult msg = pl->CanStoreItem(PlayerBag, PlayerSlot, dest, pItemBank, false);
        if (msg == EQUIP_ERR_OK)                            // merge case
        {
            // check source pos rights (item moved to inventory)
            uint32 remRight = GetMemberSlotWithdrawRem(pl->GetGUIDLow(), BankTab);
            if (remRight <= 0)
                return;

            CharacterDatabase.BeginTransaction();
            LogBankEvent(GUILD_BANK_LOG_WITHDRAW_ITEM, BankTab, pl->GetGUIDLow(), pItemBank->GetEntry(), pItemBank->GetCount());

            RemoveItem(BankTab, BankTabSlot);
            pl->MoveItemToInventory(dest, pItemBank, true);
            pl->SaveInventoryAndGoldToDB();

            MemberItemWithdraw(BankTab, pl->GetGUIDLow());
            CharacterDatabase.CommitTransaction();
        }
        else                                                // Bank <-> Char swap items
        {
            // check source pos rights (item swapped to bank)
            if (!IsMemberHaveRights(pl->GetGUIDLow(), BankTab, GUILD_BANK_RIGHT_DEPOSIT_ITEM))
                return;

            if (pItemChar)
            {
                if (!pItemChar->CanBeTraded())
                {
                    pl->SendEquipError( EQUIP_ERR_ITEMS_CANT_BE_SWAPPED, pItemChar, NULL );
                    return;
                }
            }

            ItemPosCountVec iDest;
            msg = pl->CanStoreItem(PlayerBag, PlayerSlot, iDest, pItemBank, true);
            if (msg != EQUIP_ERR_OK)
            {
                pl->SendEquipError( msg, pItemBank, NULL );
                return;
            }

            GuildItemPosCountVec gDest;
            if (pItemChar)
            {
                msg = CanStoreItem(BankTab,BankTabSlot,gDest,pItemChar->GetCount(),pItemChar,true);
                if (msg != EQUIP_ERR_OK)
                {
                    pl->SendEquipError( msg, pItemChar, NULL );
                    return;
                }
            }

            // check source pos rights (item moved to inventory)
            uint32 remRight = GetMemberSlotWithdrawRem(pl->GetGUIDLow(), BankTab);
            if (remRight <= 0)
                return;

            if (pItemChar)
            {
                // logging item move to bank
                if (pl->GetSession()->GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
                {
                    sLog.outCommand(pl->GetSession()->GetAccountId(),"GM %s (Account: %u) deposit item: %s (Entry: %d Count: %u) to guild bank (Guild ID: %u )",
                        pl->GetName(),pl->GetSession()->GetAccountId(),
                        pItemChar->GetProto()->Name1, pItemChar->GetEntry(), pItemChar->GetCount(),
                        m_Id);
                }
            }

            CharacterDatabase.BeginTransaction();
            LogBankEvent(GUILD_BANK_LOG_WITHDRAW_ITEM, BankTab, pl->GetGUIDLow(), pItemBank->GetEntry(), pItemBank->GetCount());
            if (pItemChar)
                LogBankEvent(GUILD_BANK_LOG_DEPOSIT_ITEM, BankTab, pl->GetGUIDLow(), pItemChar->GetEntry(), pItemChar->GetCount());

            RemoveItem(BankTab, BankTabSlot);
            if (pItemChar)
            {
                pl->MoveItemFromInventory(PlayerBag, PlayerSlot, true);
                pItemChar->DeleteFromInventoryDB();
            }

            if (pItemChar)
                StoreItem(BankTab, gDest, pItemChar);
            pl->MoveItemToInventory(iDest, pItemBank, true);
            pl->SaveInventoryAndGoldToDB();

            MemberItemWithdraw(BankTab, pl->GetGUIDLow());
            CharacterDatabase.CommitTransaction();
        }
    }
    DisplayGuildBankContentUpdate(BankTab, BankTabSlot);
}


void Guild::MoveFromCharToBank( Player * pl, uint8 PlayerBag, uint8 PlayerSlot, uint8 BankTab, uint8 BankTabSlot, uint32 SplitedAmount )
{
    Item *pItemBank = GetItem(BankTab, BankTabSlot);
    Item *pItemChar = pl->GetItemByPos(PlayerBag, PlayerSlot);

    if (!pItemChar)                                         // Problem to get item from player
        return;

    if (!pItemChar->CanBeTraded())
    {
        pl->SendEquipError( EQUIP_ERR_ITEMS_CANT_BE_SWAPPED, pItemChar, NULL );
        return;
    }

    // check source pos rights (item moved to bank)
    if (!IsMemberHaveRights(pl->GetGUIDLow(), BankTab, GUILD_BANK_RIGHT_DEPOSIT_ITEM))
        return;

    if (SplitedAmount > pItemChar->GetCount())
        return;                                             // cheating?
    else if (SplitedAmount == pItemChar->GetCount())
        SplitedAmount = 0;                                  // no split

    if (SplitedAmount)
    {                                                       // Char -> Bank split to empty or non-empty slot (partly move)
        GuildItemPosCountVec dest;
        InventoryResult msg = CanStoreItem(BankTab, BankTabSlot, dest, SplitedAmount, pItemChar, false);
        if (msg != EQUIP_ERR_OK)
        {
            pl->SendEquipError( msg, pItemChar, NULL );
            return;
        }

        Item *pNewItem = pItemChar->CloneItem( SplitedAmount );
        if (!pNewItem)
        {
            pl->SendEquipError( EQUIP_ERR_ITEM_NOT_FOUND, pItemChar, NULL );
            return;
        }

        // logging item move to bank (before items merge
        if (pl->GetSession()->GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
        {
            sLog.outCommand(pl->GetSession()->GetAccountId(),"GM %s (Account: %u) deposit item: %s (Entry: %d Count: %u) to guild bank (Guild ID: %u )",
                pl->GetName(),pl->GetSession()->GetAccountId(),
                pItemChar->GetProto()->Name1, pItemChar->GetEntry(), SplitedAmount, m_Id);
        }

        CharacterDatabase.BeginTransaction();
        LogBankEvent(GUILD_BANK_LOG_DEPOSIT_ITEM, BankTab, pl->GetGUIDLow(), pItemChar->GetEntry(), SplitedAmount);

        pl->ItemRemovedQuestCheck( pItemChar->GetEntry(), SplitedAmount );
        pItemChar->SetCount(pItemChar->GetCount()-SplitedAmount);
        pItemChar->SetState(ITEM_CHANGED);
        pl->SaveInventoryAndGoldToDB();
        StoreItem(BankTab, dest, pNewItem);
        CharacterDatabase.CommitTransaction();

        DisplayGuildBankContentUpdate(BankTab, dest);
    }
    else                                                    // Char -> Bank swap with empty or non-empty (move)
    {
        GuildItemPosCountVec dest;
        InventoryResult msg = CanStoreItem(BankTab, BankTabSlot, dest, pItemChar->GetCount(), pItemChar, false);
        if (msg == EQUIP_ERR_OK)                            // merge
        {
            // logging item move to bank
            if (pl->GetSession()->GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
            {
                sLog.outCommand(pl->GetSession()->GetAccountId(),"GM %s (Account: %u) deposit item: %s (Entry: %d Count: %u) to guild bank (Guild ID: %u )",
                    pl->GetName(),pl->GetSession()->GetAccountId(),
                    pItemChar->GetProto()->Name1, pItemChar->GetEntry(), pItemChar->GetCount(),
                    m_Id);
            }

            CharacterDatabase.BeginTransaction();
            LogBankEvent(GUILD_BANK_LOG_DEPOSIT_ITEM, BankTab, pl->GetGUIDLow(), pItemChar->GetEntry(), pItemChar->GetCount());

            pl->MoveItemFromInventory(PlayerBag, PlayerSlot, true);
            pItemChar->DeleteFromInventoryDB();

            StoreItem(BankTab, dest, pItemChar);
            pl->SaveInventoryAndGoldToDB();
            CharacterDatabase.CommitTransaction();

            DisplayGuildBankContentUpdate(BankTab, dest);
        }
        else                                                // Char <-> Bank swap items (posible NULL bank item)
        {
            ItemPosCountVec iDest;
            if (pItemBank)
            {
                msg = pl->CanStoreItem(PlayerBag, PlayerSlot, iDest, pItemBank, true);
                if (msg != EQUIP_ERR_OK)
                {
                    pl->SendEquipError( msg, pItemBank, NULL );
                    return;
                }
            }

            GuildItemPosCountVec gDest;
            msg = CanStoreItem(BankTab, BankTabSlot, gDest, pItemChar->GetCount(), pItemChar, true);
            if (msg != EQUIP_ERR_OK)
            {
                pl->SendEquipError( msg, pItemChar, NULL );
                return;
            }

            if (pItemBank)
            {
                // check bank pos rights (item swapped with inventory)
                uint32 remRight = GetMemberSlotWithdrawRem(pl->GetGUIDLow(), BankTab);
                if (remRight <= 0)
                    return;
            }

            // logging item move to bank
            if (pl->GetSession()->GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
            {
                sLog.outCommand(pl->GetSession()->GetAccountId(), "GM %s (Account: %u) deposit item: %s (Entry: %d Count: %u) to guild bank (Guild ID: %u )",
                    pl->GetName(), pl->GetSession()->GetAccountId(),
                    pItemChar->GetProto()->Name1, pItemChar->GetEntry(), pItemChar->GetCount(),
                    m_Id);
            }

            CharacterDatabase.BeginTransaction();
            if (pItemBank)
                LogBankEvent(GUILD_BANK_LOG_WITHDRAW_ITEM, BankTab, pl->GetGUIDLow(), pItemBank->GetEntry(), pItemBank->GetCount());
            LogBankEvent(GUILD_BANK_LOG_DEPOSIT_ITEM, BankTab, pl->GetGUIDLow(), pItemChar->GetEntry(), pItemChar->GetCount());

            pl->MoveItemFromInventory(PlayerBag, PlayerSlot, true);
            pItemChar->DeleteFromInventoryDB();
            if (pItemBank)
                RemoveItem(BankTab, BankTabSlot);

            StoreItem(BankTab, gDest, pItemChar);
            if (pItemBank)
                pl->MoveItemToInventory(iDest, pItemBank, true);
            pl->SaveInventoryAndGoldToDB();
            if (pItemBank)
                MemberItemWithdraw(BankTab, pl->GetGUIDLow());
            CharacterDatabase.CommitTransaction();

            DisplayGuildBankContentUpdate(BankTab, gDest);
        }
    }
}

void Guild::BroadcastEvent(GuildEvents event, ObjectGuid guid, char const* str1 /*=NULL*/, char const* str2 /*=NULL*/, char const* str3 /*=NULL*/)
{
    uint8 strCount = !str1 ? 0 : (!str2 ? 1 : (!str3 ? 2 : 3));

    WorldPacket data(SMSG_GUILD_EVENT, 1 + 1 + 1*strCount + (!guid ? 0 : 8));
    data << uint8(event);
    data << uint8(strCount);

    if (str3)
    {
        data << str1;
        data << str2;
        data << str3;
    }
    else if (str2)
    {
        data << str1;
        data << str2;
    }
    else if (str1)
        data << str1;

    if (guid)
        data << ObjectGuid(guid);

    BroadcastPacket(&data);

    DEBUG_LOG("WORLD: Sent SMSG_GUILD_EVENT");
}

void Guild::DeleteGuildBankItems( bool alsoInDB /*= false*/ )
{
    for (size_t i = 0; i < m_TabListMap.size(); ++i)
    {
        for (uint8 j = 0; j < GUILD_BANK_MAX_SLOTS; ++j)
        {
            if (Item* pItem = m_TabListMap[i]->Slots[j])
            {
                pItem->RemoveFromWorld();

                if (alsoInDB)
                    pItem->DeleteFromDB();

                delete pItem;
            }
        }
        delete m_TabListMap[i];
    }
    m_TabListMap.clear();
}

bool GuildItemPosCount::isContainedIn(GuildItemPosCountVec const &vec) const
{
    for(GuildItemPosCountVec::const_iterator itr = vec.begin(); itr != vec.end(); ++itr)
        if (itr->Slot == this->Slot)
            return true;

    return false;
}
