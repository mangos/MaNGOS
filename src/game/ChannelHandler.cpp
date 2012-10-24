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

#include "ObjectMgr.h"                                      // for normalizePlayerName
#include "ChannelMgr.h"

void WorldSession::HandleJoinChannelOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());

    uint32 channel_id;
    std::string channelname, pass;

    recvPacket >> channel_id;
    recvPacket.ReadBit();       // has voice
    recvPacket.ReadBit();       // zone update

    uint8 channelLength = recvPacket.ReadBits(8);
    uint8 passwordLength = recvPacket.ReadBits(8);
    channelname = recvPacket.ReadString(channelLength);
    pass = recvPacket.ReadString(passwordLength);

    if (channelname.empty())
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetJoinChannel(channelname, channel_id))
            chn->Join(_player->GetObjectGuid(), pass.c_str());
}

void WorldSession::HandleLeaveChannelOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();

    uint32 unk;
    std::string channelname;
    recvPacket >> unk;                                      // channel id?
    channelname = recvPacket.ReadString(recvPacket.ReadBits(8));

    if (channelname.empty())
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Leave(_player->GetObjectGuid(), true);
        cMgr->LeftChannel(channelname);
    }
}

void WorldSession::HandleChannelListOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    std::string channelname = recvPacket.ReadString(recvPacket.ReadBits(8));

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->List(_player);
}

void WorldSession::HandleChannelPasswordOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 nameLen, passLen;
    std::string channelname, pass;

    nameLen = recvPacket.ReadBits(8);
    passLen = recvPacket.ReadBits(7);
    channelname = recvPacket.ReadString(nameLen);
    pass = recvPacket.ReadString(passLen);

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Password(_player->GetObjectGuid(), pass.c_str());
}

void WorldSession::HandleChannelSetOwnerOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();

    uint32 channelLen, nameLen;
    std::string channelname, newp;

    channelLen = recvPacket.ReadBits(8);
    nameLen = recvPacket.ReadBits(7);
    newp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(newp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->SetOwner(_player->GetObjectGuid(), newp.c_str());
}

void WorldSession::HandleChannelOwnerOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    std::string channelname = recvPacket.ReadString(recvPacket.ReadBits(8));

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->SendWhoOwner(_player->GetObjectGuid());
}

void WorldSession::HandleChannelModeratorOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    channelLen = recvPacket.ReadBits(8);
    nameLen = recvPacket.ReadBits(7);
    otp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->SetModerator(_player->GetObjectGuid(), otp.c_str());
}

void WorldSession::HandleChannelUnmoderatorOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    nameLen = recvPacket.ReadBits(7);
    channelLen = recvPacket.ReadBits(8);
    channelname = recvPacket.ReadString(channelLen);
    otp = recvPacket.ReadString(nameLen);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->UnsetModerator(_player->GetObjectGuid(), otp.c_str());
}

void WorldSession::HandleChannelMuteOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    channelLen = recvPacket.ReadBits(8);
    nameLen = recvPacket.ReadBits(7);
    channelname = recvPacket.ReadString(channelLen);
    otp = recvPacket.ReadString(nameLen);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->SetMute(_player->GetObjectGuid(), otp.c_str());
}

void WorldSession::HandleChannelUnmuteOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    channelLen = recvPacket.ReadBits(8);
    nameLen = recvPacket.ReadBits(7);
    otp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->UnsetMute(_player->GetObjectGuid(), otp.c_str());
}

void WorldSession::HandleChannelInviteOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    nameLen = recvPacket.ReadBits(7);
    channelLen = recvPacket.ReadBits(8);
    otp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Invite(_player->GetObjectGuid(), otp.c_str());
}

void WorldSession::HandleChannelKickOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    channelLen = recvPacket.ReadBits(8);
    nameLen = recvPacket.ReadBits(7);
    channelname = recvPacket.ReadString(channelLen);
    otp = recvPacket.ReadString(nameLen);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Kick(_player->GetObjectGuid(), otp.c_str());
}

void WorldSession::HandleChannelBanOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    channelLen = recvPacket.ReadBits(8);
    nameLen = recvPacket.ReadBits(7);
    otp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Ban(_player->GetObjectGuid(), otp.c_str());
}

void WorldSession::HandleChannelUnbanOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    nameLen = recvPacket.ReadBits(7);
    channelLen = recvPacket.ReadBits(8);
    otp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->UnBan(_player->GetObjectGuid(), otp.c_str());
}

void WorldSession::HandleChannelAnnouncementsOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();
    std::string channelname = recvPacket.ReadString(recvPacket.ReadBits(8));
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Announce(_player->GetObjectGuid());
}

void WorldSession::HandleChannelModerateOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    std::string channelname;
    recvPacket >> channelname;
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Moderate(_player->GetObjectGuid());
}

void WorldSession::HandleChannelDisplayListQueryOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    std::string channelname = recvPacket.ReadString(recvPacket.ReadBits(8));
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->List(_player);
}

void WorldSession::HandleGetChannelMemberCountOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    std::string channelname;
    recvPacket >> channelname;
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            WorldPacket data(SMSG_CHANNEL_MEMBER_COUNT, chn->GetName().size() + 1 + 1 + 4);
            data << chn->GetName();
            data << uint8(chn->GetFlags());
            data << uint32(chn->GetNumPlayers());
            SendPacket(&data);
        }
    }
}

void WorldSession::HandleSetChannelWatchOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("Opcode %u", recvPacket.GetOpcode());
    // recvPacket.hexlike();
    std::string channelname;
    recvPacket >> channelname;
    /*if(ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if(Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->JoinNotify(_player->GetGUID());*/
}
