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

#include "ChannelMgr.h"
#include "Policies/SingletonImp.h"
#include "World.h"

INSTANTIATE_SINGLETON_1( AllianceChannelMgr );
INSTANTIATE_SINGLETON_1( HordeChannelMgr );

ChannelMgr* channelMgr(Team team)
{
    if (sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_CHANNEL))
        return &MaNGOS::Singleton<AllianceChannelMgr>::Instance();        // cross-faction

    if(team == ALLIANCE)
        return &MaNGOS::Singleton<AllianceChannelMgr>::Instance();
    if(team == HORDE)
        return &MaNGOS::Singleton<HordeChannelMgr>::Instance();

    return NULL;
}

ChannelMgr::~ChannelMgr()
{
    for(ChannelMap::iterator itr = channels.begin();itr!=channels.end(); ++itr)
        delete itr->second;

    channels.clear();
}

Channel *ChannelMgr::GetJoinChannel(std::string name, uint32 channel_id)
{
    std::wstring wname;
    Utf8toWStr(name,wname);
    wstrToLower(wname);

    if (channels.find(wname) == channels.end())
    {
        Channel *nchan = new Channel(name,channel_id);
        channels[wname] = nchan;
        return nchan;
    }

    return channels[wname];
}

Channel *ChannelMgr::GetChannel(std::string name, Player *p, bool pkt)
{
    std::wstring wname;
    Utf8toWStr(name,wname);
    wstrToLower(wname);

    ChannelMap::const_iterator i = channels.find(wname);

    if(i == channels.end())
    {
        if(pkt)
        {
            WorldPacket data;
            MakeNotOnPacket(&data,name);
            p->GetSession()->SendPacket(&data);
        }

        return NULL;
    }
    else
        return i->second;
}

void ChannelMgr::LeftChannel(std::string name)
{
    std::wstring wname;
    Utf8toWStr(name,wname);
    wstrToLower(wname);

    ChannelMap::const_iterator i = channels.find(wname);

    if(i == channels.end())
        return;

    Channel* channel = i->second;

    if(channel->GetNumPlayers() == 0 && !channel->IsConstant())
    {
        channels.erase(wname);
        delete channel;
    }
}

void ChannelMgr::MakeNotOnPacket(WorldPacket *data, std::string name)
{
    data->Initialize(SMSG_CHANNEL_NOTIFY, (1+10));  // we guess size
    (*data) << (uint8)CHAT_NOT_MEMBER_NOTICE << name;
}
