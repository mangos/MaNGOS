/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
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

#include "WorldSession.h"
#include "Log.h"
#include "Player.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "World.h"

void WorldSession::HandleLfgJoinOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("CMSG_LFG_JOIN");

    uint8 dungeonsCount, counter2;
    std::string comment;
    std::vector<uint32> dungeons;

    recv_data >> Unused<uint32>();                          // lfg roles
    recv_data >> Unused<uint8>();                           // lua: GetLFGInfoLocal
    recv_data >> Unused<uint8>();                           // lua: GetLFGInfoLocal

    recv_data >> dungeonsCount;

    dungeons.resize(dungeonsCount);

    for (uint8 i = 0; i < dungeonsCount; ++i)
        recv_data >> dungeons[i];                           // dungeons id/type

    recv_data >> counter2;                                  // const count = 3, lua: GetLFGInfoLocal

    for (uint8 i = 0; i < counter2; ++i)
        recv_data >> Unused<uint8>();                       // lua: GetLFGInfoLocal

    recv_data >> comment;                                   // lfg comment

    //SendLfgJoinResult(ERR_LFG_OK);
    //SendLfgUpdate(false, LFG_UPDATE_JOIN, dungeons[0]);
}

void WorldSession::HandleLfgLeaveOpcode( WorldPacket & /*recv_data*/ )
{
    DEBUG_LOG("CMSG_LFG_LEAVE");

    //SendLfgUpdate(false, LFG_UPDATE_LEAVE, 0);
}

void WorldSession::HandleSearchLfgJoinOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("CMSG_LFG_SEARCH_JOIN");

    uint32 temp, entry;
    recv_data >> temp;

    entry = (temp & 0x00FFFFFF);
    LfgType type = LfgType((temp >> 24) & 0x000000FF);

    //SendLfgSearchResults(type, entry);
}

void WorldSession::HandleSearchLfgLeaveOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("CMSG_LFG_SEARCH_LEAVE");

    recv_data >> Unused<uint32>();                          // join id?
}

void WorldSession::HandleSetLfgCommentOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("CMSG_SET_LFG_COMMENT");

    std::string comment;
    recv_data >> comment;
    DEBUG_LOG("LFG comment \"%s\"", comment.c_str());
}

void WorldSession::SendLfgSearchResults(LfgType type, uint32 entry)
{
    WorldPacket data(SMSG_LFG_SEARCH_RESULTS);
    data << uint32(type);                                   // type
    data << uint32(entry);                                  // entry from LFGDungeons.dbc

    uint8 isGuidsPresent = 0;
    data << uint8(isGuidsPresent);
    if(isGuidsPresent)
    {
        uint32 guids_count = 0;
        data << uint32(guids_count);
        for(uint32 i = 0; i < guids_count; ++i)
        {
            data << uint64(0);                              // player/group guid
        }
    }

    uint32 groups_count = 1;
    data << uint32(groups_count);                           // groups count
    data << uint32(groups_count);                           // groups count (total?)

    for(uint32 i = 0; i < groups_count; ++i)
    {
        data << uint64(1);                                  // group guid

        uint32 flags = 0x92;
        data << uint32(flags);                              // flags

        if(flags & 0x2)
        {
            data << uint8(0);                               // comment string, max len 256
        }

        if(flags & 0x10)
        {
            for(uint32 j = 0; j < 3; ++j)
                data << uint8(0);                           // roles
        }

        if(flags & 0x80)
        {
            data << uint64(0);                              // instance guid
            data << uint32(0);                              // completed encounters
        }
    }

    //TODO: Guard Player map
    HashMapHolder<Player>::MapType const& players = sObjectAccessor.GetPlayers();
    uint32 playersSize = players.size();
    data << uint32(playersSize);                            // players count
    data << uint32(playersSize);                            // players count (total?)

    for(HashMapHolder<Player>::MapType::const_iterator iter = players.begin(); iter != players.end(); ++iter)
    {
        Player *plr = iter->second;

        if(!plr || plr->GetTeam() != _player->GetTeam())
            continue;

        if(!plr->IsInWorld())
            continue;

        data << plr->GetObjectGuid();                       // guid

        uint32 flags = 0xFF;
        data << uint32(flags);                              // flags

        if(flags & 0x1)
        {
            data << uint8(plr->getLevel());
            data << uint8(plr->getClass());
            data << uint8(plr->getRace());

            for(uint32 i = 0; i < 3; ++i)
                data << uint8(0);                           // talent spec x/x/x

            data << uint32(0);                              // armor
            data << uint32(0);                              // spd/heal
            data << uint32(0);                              // spd/heal
            data << uint32(0);                              // HasteMelee
            data << uint32(0);                              // HasteRanged
            data << uint32(0);                              // HasteSpell
            data << float(0);                               // MP5
            data << float(0);                               // MP5 Combat
            data << uint32(0);                              // AttackPower
            data << uint32(0);                              // Agility
            data << uint32(0);                              // Health
            data << uint32(0);                              // Mana
            data << uint32(0);                              // Unk1
            data << float(0);                               // Unk2
            data << uint32(0);                              // Defence
            data << uint32(0);                              // Dodge
            data << uint32(0);                              // Block
            data << uint32(0);                              // Parry
            data << uint32(0);                              // Crit
            data << uint32(0);                              // Expertise
        }

        if(flags & 0x2)
            data << "";                                     // comment

        if(flags & 0x4)
            data << uint8(0);                               // group leader

        if(flags & 0x8)
            data << uint64(1);                              // group guid

        if(flags & 0x10)
            data << uint8(0);                               // roles

        if(flags & 0x20)
            data << uint32(plr->GetZoneId());               // areaid

        if(flags & 0x40)
            data << uint8(0);                               // status

        if(flags & 0x80)
        {
            data << uint64(0);                              // instance guid
            data << uint32(0);                              // completed encounters
        }
    }

    SendPacket(&data);
}

void WorldSession::SendLfgJoinResult(LfgJoinResult result)
{
    WorldPacket data(SMSG_LFG_JOIN_RESULT, 0);
    data << uint32(result);
    data << uint32(0); // ERR_LFG_ROLE_CHECK_FAILED_TIMEOUT = 3, ERR_LFG_ROLE_CHECK_FAILED_NOT_VIABLE = (value - 3 == result)

    if(result == ERR_LFG_NO_SLOTS_PARTY)
    {
        uint8 count1 = 0;
        data << uint8(count1);                              // players count?
        for(uint32 i = 0; i < count1; ++i)
        {
            data << uint64(0);                              // player guid?
            uint32 count2 = 0;
            for(uint32 j = 0; j < count2; ++j)
            {
                data << uint32(0);                          // dungeon id/type
                data << uint32(0);                          // lock status?
            }
        }
    }

    SendPacket(&data);
}

void WorldSession::SendLfgUpdate(bool isGroup, LfgUpdateType updateType, uint32 id)
{
    WorldPacket data(isGroup ? SMSG_LFG_UPDATE_PARTY : SMSG_LFG_UPDATE_PLAYER, 0);
    data << uint8(updateType);

    uint8 extra = updateType == LFG_UPDATE_JOIN ? 1 : 0;
    data << uint8(extra);

    if(extra)
    {
        data << uint8(0);
        data << uint8(0);
        data << uint8(0);

        if(isGroup)
        {
            data << uint8(0);
            for(uint32 i = 0; i < 3; ++i)
                data << uint8(0);
        }

        uint8 count = 1;
        data << uint8(count);
        for(uint32 i = 0; i < count; ++i)
            data << uint32(id);
        data << "";
    }
    SendPacket(&data);
}
