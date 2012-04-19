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
#include "Log.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "MapPersistentStateMgr.h"

void WorldSession::HandleCalendarGetCalendar(WorldPacket &/*recv_data*/)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_GET_CALENDAR");         // empty

    time_t cur_time = time(NULL);

    WorldPacket data(SMSG_CALENDAR_SEND_CALENDAR, 4+4*0+4+4*0+4+4);

    // TODO: calendar invite event output
    data << (uint32) 0;                                     // invite node count
    // TODO: calendar event output
    data << (uint32) 0;                                     // event count

    data << (uint32) 0;                                     // Current Unix Time?
    data << (uint32) secsToTimeBitFields(cur_time);         // current packed time

    uint32 counter = 0;
    size_t p_counter = data.wpos();
    data << uint32(counter);                                // instance state count

    for(int i = 0; i < MAX_DIFFICULTY; ++i)
    {
        for (Player::BoundInstancesMap::const_iterator itr = _player->m_boundInstances[i].begin(); itr != _player->m_boundInstances[i].end(); ++itr)
        {
            if(itr->second.perm)
            {
                DungeonPersistentState *state = itr->second.state;
                data << uint32(state->GetMapId());
                data << uint32(state->GetDifficulty());
                data << uint32(state->GetResetTime() - cur_time);
                data << ObjectGuid(state->GetInstanceGuid());
                ++counter;
            }
        }
    }
    data.put<uint32>(p_counter,counter);

    data << (uint32) 1135753200;                            // base date (28.12.2005 12:00)
    data << (uint32) 0;                                     // raid reset count
    data << (uint32) 0;                                     // holidays count
/*
    for(uint32 i = 0; i < holidays_count; ++i)
    {
        data << uint32(0);                                  // Holidays.dbc ID
        data << uint32(0);                                  // Holidays.dbc region
        data << uint32(0);                                  // Holidays.dbc looping
        data << uint32(0);                                  // Holidays.dbc priority
        data << uint32(0);                                  // Holidays.dbc calendarFilterType

        for(uint32 j = 0; j < 26; j++)
            data << uint32(0);                              // Holidays.dbc date

        for(uint32 j = 0; j < 10; j++)
            data << uint32(0);                              // Holidays.dbc duration

        for(uint32 j = 0; j < 10; j++)
            data << uint32(0);                              // Holidays.dbc calendarFlags

        data << "";                                         // Holidays.dbc textureFilename
    }
*/
    //DEBUG_LOG("Sending calendar");
    //data.hexlike();
    SendPacket(&data);
}

void WorldSession::HandleCalendarGetEvent(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_GET_EVENT");
    recv_data.hexlike();
    recv_data.read_skip<uint64>();                          // unk
}

void WorldSession::HandleCalendarGuildFilter(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_GUILD_FILTER");
    recv_data.hexlike();
    recv_data.read_skip<uint32>();                          // unk1
    recv_data.read_skip<uint32>();                          // unk2
    recv_data.read_skip<uint32>();                          // unk3
}

void WorldSession::HandleCalendarArenaTeam(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_ARENA_TEAM");
    recv_data.hexlike();
    recv_data.read_skip<uint32>();                          // unk
}

void WorldSession::HandleCalendarAddEvent(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_ADD_EVENT");
    recv_data.hexlike();
    recv_data.rpos(recv_data.wpos());                       // set to end to avoid warnings spam

    //std::string unk1, unk2;
    //recv_data >> (std::string)unk1;
    //recv_data >> (std::string)unk2;

    //uint8   unk3, unk4;
    //uint32  unk5, unk6, unk7, unk8, unk9, count = 0;
    //recv_data >> (uint8)unk3;
    //recv_data >> (uint8)unk4;
    //recv_data >> (uint32)unk5;
    //recv_data >> (uint32)unk6;
    //recv_data >> (uint32)unk7;
    //recv_data >> (uint32)unk8;
    //recv_data >> (uint32)unk9;
    //if (!((unk9 >> 6) & 1))
    //{
    //    recv_data >> (uint32)count;
    //    if (count)
    //    {
    //        uint8 unk12,unk13;
    //        ObjectGuid guid;
    //        for (int i=0;i<count;i++)
    //        {
    //            recv_data >> guid.ReadAsPacked();
    //            recv_data >> (uint8)unk12;
    //            recv_data >> (uint8)unk13;
    //        }
    //    }
    //}
}

void WorldSession::HandleCalendarUpdateEvent(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_UPDATE_EVENT");
    recv_data.hexlike();
    recv_data.rpos(recv_data.wpos());                       // set to end to avoid warnings spam

    //recv_data >> uint64
    //recv_data >> uint64
    //recv_data >> std::string
    //recv_data >> std::string
    //recv_data >> uint8
    //recv_data >> uint8
    //recv_data >> uint32
    //recv_data >> uint32
    //recv_data >> uint32
    //recv_data >> uint32
    //recv_data >> uint32
}

void WorldSession::HandleCalendarRemoveEvent(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_REMOVE_EVENT");
    recv_data.hexlike();
    recv_data.rpos(recv_data.wpos());                       // set to end to avoid warnings spam

    //recv_data >> uint64
    //recv_data >> uint64
    //recv_data >> uint32

}

void WorldSession::HandleCalendarCopyEvent(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_COPY_EVENT");
    recv_data.hexlike();
    recv_data.rpos(recv_data.wpos());                       // set to end to avoid warnings spam

    //recv_data >> uint64
    //recv_data >> uint64
    //recv_data >> uint32

}

void WorldSession::HandleCalendarEventInvite(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_EVENT_INVITE");
    recv_data.hexlike();
    recv_data.rpos(recv_data.wpos());                       // set to end to avoid warnings spam

    //recv_data >> uint64
    //recv_data >> uint64
    //recv_data >> std::string
    //recv_data >> uint8
    //recv_data >> uint8

}

void WorldSession::HandleCalendarEventRsvp(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_EVENT_RSVP");
    recv_data.hexlike();
    recv_data.rpos(recv_data.wpos());                       // set to end to avoid warnings spam

    //recv_data >> uint64
    //recv_data >> uint64
    //recv_data >> uint32
}

void WorldSession::HandleCalendarEventRemoveInvite(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_EVENT_REMOVE_INVITE");
    recv_data.hexlike();
    recv_data.rpos(recv_data.wpos());                       // set to end to avoid warnings spam

    //recv_data.readPackGUID(guid)
    //recv_data >> uint64
    //recv_data >> uint64
    //recv_data >> uint64
}

void WorldSession::HandleCalendarEventStatus(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_EVENT_STATUS");
    recv_data.hexlike();
    recv_data.rpos(recv_data.wpos());                       // set to end to avoid warnings spam

    //recv_data.readPackGUID(guid)
    //recv_data >> uint64
    //recv_data >> uint64
    //recv_data >> uint64
    //recv_data >> uint32
}

void WorldSession::HandleCalendarEventModeratorStatus(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_EVENT_MODERATOR_STATUS");
    recv_data.hexlike();
    recv_data.rpos(recv_data.wpos());                       // set to end to avoid warnings spam

    //recv_data.readPackGUID(guid)
    //recv_data >> uint64
    //recv_data >> uint64
    //recv_data >> uint64
    //recv_data >> uint32
}

void WorldSession::HandleCalendarComplain(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_COMPLAIN");
    recv_data.hexlike();
    recv_data.rpos(recv_data.wpos());                       // set to end to avoid warnings spam

    //recv_data >> uint64
    //recv_data >> uint64
    //recv_data >> uint64
}

void WorldSession::HandleCalendarGetNumPending(WorldPacket & /*recv_data*/)
{
    DEBUG_LOG("WORLD: CMSG_CALENDAR_GET_NUM_PENDING");      // empty

    WorldPacket data(SMSG_CALENDAR_SEND_NUM_PENDING, 4);
    data << uint32(0);                                      // 0 - no pending invites, 1 - some pending invites
    SendPacket(&data);
}
