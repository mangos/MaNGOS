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
#include "Database/DBCEnums.h"
#include "ObjectMgr.h"

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
    sLog.outString("AchievementMgr::SendAchievementEarned(%u)", achievementId);
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
    sLog.outString("AchievementMgr::SendCriteriaUpdate(%u, %u)", criteriaId, counter);
    WorldPacket data(SMSG_CRITERIA_UPDATE, 8+4+8);
    data << uint32(criteriaId);

    // the counter is packed like a packed Guid
    data.appendPackGUID(counter);

    data.append(GetPlayer()->GetPackGUID());
    /*
    data << uint32(counter);
    data << uint32(counter+1);//timer1
    data << uint32(counter+2);//timer2
    */
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    GetPlayer()->SendMessageToSet(&data, true);
}

/**
 * this function will be called whenever the user might have done a criteria relevant action
 */
void AchievementMgr::UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1, uint32 miscvalue2, uint32 time)
{
    sLog.outString("AchievementMgr::UpdateAchievementCriteria(%u, %u, %u, %u)", type, miscvalue1, miscvalue2, time);
    AchievementCriteriaEntryList const& achievementCriteriaList = objmgr.GetAchievementCriteriaByType(type);
    for(AchievementCriteriaEntryList::const_iterator i = achievementCriteriaList.begin(); i!=achievementCriteriaList.end(); ++i)
    {
        AchievementCriteriaEntry const *achievementCriteria = (*i);
        switch (type)
        {
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
            case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
                SetCriteriaProgress(achievementCriteria, miscvalue1);
            break;
            default:
                return;
        }
        if(IsCompletedCriteria(achievementCriteria))
            CompletedCriteria(achievementCriteria);
    }
}

bool AchievementMgr::IsCompletedCriteria(AchievementCriteriaEntry const* achievementCriteria)
{
    AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementCriteria->referredAchievement);
    if(!achievement)
        return false;
    // counter can never complete
    if(achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    switch(achievementCriteria->requiredType)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
            return m_criteriaProgress[achievementCriteria->ID] >= achievementCriteria->reach_level.level;
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
            return m_criteriaProgress[achievementCriteria->ID] >= achievementCriteria->buy_bank_slot.numberOfSlots;

    }
    return false;
}

void AchievementMgr::CompletedCriteria(AchievementCriteriaEntry const* criteria)
{
    AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->referredAchievement);
    if(!achievement)
        return;
    // counter can never complete
    if(achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return;

    if(criteria->completionFlag & ACHIEVEMENT_CRITERIA_COMPLETE_FLAG_ALL)
    {
        CompletedAchievement(achievement);
        return;
    }

    // Check if there are also other critiera which have to be fulfilled for that achievement
    for (uint32 entryId = 0; entryId<sAchievementCriteriaStore.GetNumRows(); entryId++)
    {
         AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(entryId);
         if(!criteria || criteria->referredAchievement!= achievement->ID)
             continue;

         // found an outstanding criteria, return
         if(!IsCompletedCriteria(criteria))
             return;
    }
    CompletedAchievement(achievement);
}

void AchievementMgr::SetCriteriaProgress(AchievementCriteriaEntry const* entry, uint32 newValue)
{
    sLog.outString("AchievementMgr::SetCriteriaProgress(%u, %u)", entry->ID, newValue);
    m_criteriaProgress[entry->ID] = newValue;
    SendCriteriaUpdate(entry->ID, newValue);
}

void AchievementMgr::CompletedAchievement(AchievementEntry const* achievement)
{
    sLog.outString("AchievementMgr::CompletedAchievement(%u)", achievement->ID);
    if(achievement->flags & ACHIEVEMENT_FLAG_COUNTER || m_completedAchievements.find(achievement->ID)!=m_completedAchievements.end())
        return;

    SendAchievementEarned(achievement->ID);
    m_completedAchievements.insert(achievement->ID);
    // TODO: reward titles and items
}

