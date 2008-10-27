/*
 * Copyright (C) 2005-2008 MaNGOS <http://getmangos.com/>
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

#include "AchievementMgr.h"
#include "Common.h"
#include "Player.h"
#include "WorldPacket.h"

AchievementMgr::AchievementMgr(Player *player)
{
    m_player = player;
}

void AchievementMgr::SaveToDB()
{
    // TODO store achievements
}

void AchievementMgr::LoadFromDB()
{
    // TODO load achievements
}

void AchievementMgr::SendAchievementEarned(uint32 achievementId)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    data << uint8(CHAT_MSG_ACHIEVEMENT);
    data << uint32(LANG_UNIVERSAL);
    data << uint64(GetPlayer()->GetGUID());
    data << uint32(0);
    data << uint64(GetPlayer()->GetGUID());
    const char *msg = "|Hplayer:$N|h[$N]|h has earned the achievement $a!";
    data << uint32(strlen(msg));
    data << msg;
    data << uint8(0);
    data << uint32(achievementId);
    GetPlayer()->SendMessageToSet(&data, true);

    data.Initialize(SMSG_ACHIEVEMENT_EARNED, 8+4+8);
    data.append(GetPlayer()->GetPackGUID());
    data << uint32(achievementId);
    data << uint64(0x0000000); // magic number? same as in SMSG_CRITERIA_UPDATE. static for every player?
    GetPlayer()->SendMessageToSet(&data, true);
}

void AchievementMgr::SendCriteriaUpdate(uint32 criteriaId, uint32 counter)
{
    WorldPacket data(SMSG_CRITERIA_UPDATE, 8+4+8);
    data << uint32(criteriaId);
    data << uint8(counter);
    data << uint8(counter);// 2 times?
    data.append(GetPlayer()->GetPackGUID());
    data << uint64(0x0000000); // unknown, same as in SMSG_EARNED_ACHIEVEMENT, static for every player?
    data << uint32(0); // unknown, 0
    GetPlayer()->SendMessageToSet(&data, true);
}

