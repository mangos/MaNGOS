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
#include "Guild.h"
#include "Database/DatabaseEnv.h"

AchievementMgr::AchievementMgr(Player *player)
{
    m_player = player;
}

AchievementMgr::~AchievementMgr()
{
    for(CriteriaProgressMap::iterator iter = m_criteriaProgress.begin(); iter!=m_criteriaProgress.end(); ++iter)
        delete iter->second;
    m_criteriaProgress.clear();
}

void AchievementMgr::SaveToDB()
{
    if(!m_completedAchievements.empty())
    {
        CharacterDatabase.PExecute("DELETE FROM character_achievement WHERE guid = %u", GetPlayer()->GetGUIDLow());

        std::ostringstream ss;
        ss << "INSERT INTO character_achievement (guid, achievement, date) VALUES ";
        for(CompletedAchievementMap::iterator iter = m_completedAchievements.begin(); iter!=m_completedAchievements.end(); iter++)
        {
            if(iter != m_completedAchievements.begin())
                ss << ", ";
            ss << "("<<GetPlayer()->GetGUIDLow() << ", " << iter->first << ", " << iter->second << ")";
        }
        CharacterDatabase.Execute( ss.str().c_str() );
    }

    if(!m_criteriaProgress.empty())
    {
        CharacterDatabase.PExecute("DELETE FROM character_achievement_progress WHERE guid = %u", GetPlayer()->GetGUIDLow());

        std::ostringstream ss;
        ss << "INSERT INTO character_achievement_progress (guid, criteria, counter, date) VALUES ";
        for(CriteriaProgressMap::iterator iter = m_criteriaProgress.begin(); iter!=m_criteriaProgress.end(); ++iter)
        {
            if(iter != m_criteriaProgress.begin())
                ss << ", ";
            ss << "(" << GetPlayer()->GetGUIDLow() << ", " << iter->first << ", " << iter->second->counter << ", " << iter->second->date << ")";
        }
        CharacterDatabase.Execute( ss.str().c_str() );
    }
}

void AchievementMgr::LoadFromDB(QueryResult *achievementResult, QueryResult *criteriaResult)
{
    if(achievementResult)
    {
        do
        {
            Field *fields = achievementResult->Fetch();
            m_completedAchievements[fields[0].GetUInt32()] = fields[1].GetUInt32();
        } while(achievementResult->NextRow());
        delete achievementResult;
    }

    if(criteriaResult)
    {
        do
        {
            Field *fields = criteriaResult->Fetch();
            CriteriaProgress *progress = new CriteriaProgress(fields[0].GetUInt32(), fields[1].GetUInt32(), fields[2].GetUInt64());
            m_criteriaProgress[progress->id] = progress;
        } while(criteriaResult->NextRow());
        delete criteriaResult;
    }

}

void AchievementMgr::SendAchievementEarned(uint32 achievementId)
{
    sLog.outString("AchievementMgr::SendAchievementEarned(%u)", achievementId);

    WorldPacket data(SMSG_MESSAGECHAT, 200);
    data << uint8(CHAT_MSG_ACHIEVEMENT);
    data << uint32(LANG_UNIVERSAL);
    data << uint64(GetPlayer()->GetGUID());
    data << uint32(5);
    data << uint64(GetPlayer()->GetGUID());
    const char *msg = "|Hplayer:$N|h[$N]|h has earned the achievement $a!";
    data << uint32(strlen(msg)+1);
    data << msg;
    data << uint8(0);
    data << uint32(achievementId);
    GetPlayer()->SendMessageToSet(&data, true);

    if(Guild* guild = objmgr.GetGuildById(GetPlayer()->GetGuildId()))
    {
        data.Initialize(SMSG_MESSAGECHAT, 200);
        data << uint8(CHAT_MSG_GUILD_ACHIEVEMENT);
        data << uint32(LANG_UNIVERSAL);
        data << uint64(GetPlayer()->GetGUID());
        data << uint32(5);
        data << uint64(GetPlayer()->GetGUID());
        data << uint32(strlen(msg)+1);
        data << msg;
        data << uint8(0);
        data << uint32(achievementId);
        guild->BroadcastPacket(&data);
    }

    data.Initialize(SMSG_ACHIEVEMENT_EARNED, 8+4+8);
    data.append(GetPlayer()->GetPackGUID());
    data << uint32(achievementId);
    data << uint32(secsToTimeBitFields(time(NULL)));
    data << uint32(0);
    GetPlayer()->SendMessageToSet(&data, true);
}

void AchievementMgr::SendCriteriaUpdate(CriteriaProgress *progress)
{
    WorldPacket data(SMSG_CRITERIA_UPDATE, 8+4+8);
    data << uint32(progress->id);

    // the counter is packed like a packed Guid
    data.appendPackGUID(progress->counter);

    data.append(GetPlayer()->GetPackGUID());
    data << uint32(0);
    data << uint32(secsToTimeBitFields(progress->date));
    data << uint32(0);  // timer 1
    data << uint32(0);  // timer 2
    GetPlayer()->SendMessageToSet(&data, true);
}

/**
 * called at player login. The player might have fulfilled some achievements when the achievement system wasn't working yet
 */
void AchievementMgr::CheckAllAchievementCriteria()
{
    // suppress sending packets
    for(uint32 i=0; i<ACHIEVEMENT_CRITERIA_TYPE_TOTAL; i++)
        UpdateAchievementCriteria(AchievementCriteriaTypes(i));
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

        // don't update already completed criteria
        if(IsCompletedCriteria(achievementCriteria))
            continue;

        if(achievementCriteria->groupFlag & ACHIEVEMENT_CRITERIA_GROUP_NOT_IN_GROUP && GetPlayer()->GetGroup())
            continue;

        switch (type)
        {
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
                SetCriteriaProgress(achievementCriteria, GetPlayer()->getLevel());
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
                SetCriteriaProgress(achievementCriteria, GetPlayer()->GetByteValue(PLAYER_BYTES_2, 2)+1);
                break;
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

    if(achievement->flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
    {
        // someone on this realm has already completed that achievement
        if(objmgr.allCompletedAchievements.find(achievement->ID)!=objmgr.allCompletedAchievements.end())
            return false;
    }

    CriteriaProgressMap::iterator itr = m_criteriaProgress.find(achievementCriteria->ID);
    if(itr == m_criteriaProgress.end())
        return false;

    CriteriaProgress *progress = itr->second;

    switch(achievementCriteria->requiredType)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
            if(achievement->ID == 467 && GetPlayer()->getClass() != CLASS_SHAMAN ||
                    achievement->ID == 466 && GetPlayer()->getClass() != CLASS_DRUID ||
                    achievement->ID == 465 && GetPlayer()->getClass() != CLASS_PALADIN ||
                    achievement->ID == 464 && GetPlayer()->getClass() != CLASS_PRIEST ||
                    achievement->ID == 463 && GetPlayer()->getClass() != CLASS_WARLOCK ||
                    achievement->ID == 462 && GetPlayer()->getClass() != CLASS_HUNTER ||
                    achievement->ID == 461 && GetPlayer()->getClass() != CLASS_DEATH_KNIGHT ||
                    achievement->ID == 460 && GetPlayer()->getClass() != CLASS_MAGE ||
                    achievement->ID == 459 && GetPlayer()->getClass() != CLASS_WARRIOR ||
                    achievement->ID == 458 && GetPlayer()->getClass() != CLASS_ROGUE ||

                    achievement->ID == 1404 && GetPlayer()->getRace() != RACE_GNOME ||
                    achievement->ID == 1405 && GetPlayer()->getRace() != RACE_BLOODELF ||
                    achievement->ID == 1406 && GetPlayer()->getRace() != RACE_DRAENEI ||
                    achievement->ID == 1407 && GetPlayer()->getRace() != RACE_DWARF ||
                    achievement->ID == 1408 && GetPlayer()->getRace() != RACE_HUMAN ||
                    achievement->ID == 1409 && GetPlayer()->getRace() != RACE_NIGHTELF ||
                    achievement->ID == 1410 && GetPlayer()->getRace() != RACE_ORC ||
                    achievement->ID == 1411 && GetPlayer()->getRace() != RACE_TAUREN ||
                    achievement->ID == 1412 && GetPlayer()->getRace() != RACE_TROLL ||
                    achievement->ID == 1413 && GetPlayer()->getRace() != RACE_UNDEAD_PLAYER )
                return false;
            return progress->counter >= achievementCriteria->reach_level.level;
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
            return progress->counter >= achievementCriteria->buy_bank_slot.numberOfSlots;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            return m_completedAchievements.find(achievementCriteria->complete_achievement.linkedAchievement) != m_completedAchievements.end();
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

    if(criteria->completionFlag & ACHIEVEMENT_CRITERIA_COMPLETE_FLAG_ALL || GetAchievementCompletionState(achievement)==ACHIEVEMENT_COMPLETED_COMPLETED_NOT_STORED)
    {
        CompletedAchievement(achievement);
    }
}

AchievementCompletionState AchievementMgr::GetAchievementCompletionState(AchievementEntry const* entry)
{
    if(m_completedAchievements.find(entry->ID)!=m_completedAchievements.end())
        return ACHIEVEMENT_COMPLETED_COMPLETED_STORED;

    bool foundOutstanding = false;
    for (uint32 entryId = 0; entryId<sAchievementCriteriaStore.GetNumRows(); entryId++)
    {
         AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(entryId);
         if(!criteria || criteria->referredAchievement!= entry->ID)
             continue;

         if(IsCompletedCriteria(criteria) && criteria->completionFlag & ACHIEVEMENT_CRITERIA_COMPLETE_FLAG_ALL)
             return ACHIEVEMENT_COMPLETED_COMPLETED_NOT_STORED;

         // found an umcompleted criteria, but DONT return false yet - there might be a completed criteria with ACHIEVEMENT_CRITERIA_COMPLETE_FLAG_ALL
         if(!IsCompletedCriteria(criteria))
             foundOutstanding = true;
    }
    if(foundOutstanding)
        return ACHIEVEMENT_COMPLETED_NONE;
    else
        return ACHIEVEMENT_COMPLETED_COMPLETED_NOT_STORED;
}

void AchievementMgr::SetCriteriaProgress(AchievementCriteriaEntry const* entry, uint32 newValue)
{
    sLog.outString("AchievementMgr::SetCriteriaProgress(%u, %u)", entry->ID, newValue);
    CriteriaProgress *progress = NULL;

    if(m_criteriaProgress.find(entry->ID) == m_criteriaProgress.end())
    {
        progress = new CriteriaProgress(entry->ID, newValue);
        m_criteriaProgress[entry->ID]=progress;
    }
    else
    {
        progress = m_criteriaProgress[entry->ID];
        if(progress->counter == newValue)
            return;
        progress->counter = newValue;
    }
    SendCriteriaUpdate(progress);
}

void AchievementMgr::CompletedAchievement(AchievementEntry const* achievement)
{
    sLog.outString("AchievementMgr::CompletedAchievement(%u)", achievement->ID);
    if(achievement->flags & ACHIEVEMENT_FLAG_COUNTER || m_completedAchievements.find(achievement->ID)!=m_completedAchievements.end())
        return;

    SendAchievementEarned(achievement->ID);
    m_completedAchievements[achievement->ID] = time(NULL);

    // don't insert for ACHIEVEMENT_FLAG_REALM_FIRST_KILL since otherwise only the first group member would reach that achievement
    // TODO: where do set this instead?
    if(!(achievement->flags & ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
        objmgr.allCompletedAchievements.insert(achievement->ID);

    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT);
    // TODO: reward titles and items
}

void AchievementMgr::SendAllAchievementData()
{
    // since we don't know the exact size of the packed GUIDs this is just an approximation
    WorldPacket data(SMSG_ALL_ACHIEVEMENT_DATA, 4*2+m_completedAchievements.size()*4*2+m_completedAchievements.size()*7*4);
    BuildAllDataPacket(&data);
    GetPlayer()->GetSession()->SendPacket(&data);
}

void AchievementMgr::SendRespondInspectAchievements(Player* player)
{
    // since we don't know the exact size of the packed GUIDs this is just an approximation
    WorldPacket data(SMSG_RESPOND_INSPECT_ACHIEVEMENTS, 4+4*2+m_completedAchievements.size()*4*2+m_completedAchievements.size()*7*4);
    data.append(GetPlayer()->GetPackGUID());
    BuildAllDataPacket(&data);
    player->GetSession()->SendPacket(&data);
}

/**
 * used by both SMSG_ALL_ACHIEVEMENT_DATA  and SMSG_RESPOND_INSPECT_ACHIEVEMENT
 */
void AchievementMgr::BuildAllDataPacket(WorldPacket *data)
{
    for(CompletedAchievementMap::iterator iter = m_completedAchievements.begin(); iter!=m_completedAchievements.end(); ++iter)
    {
        *data << uint32(iter->first);
        *data << uint32(secsToTimeBitFields(iter->second));
    }
    *data << int32(-1);

    for(CriteriaProgressMap::iterator iter = m_criteriaProgress.begin(); iter!=m_criteriaProgress.end(); ++iter)
    {
        *data << uint32(iter->second->id);
        data->appendPackGUID(iter->second->counter);
        data->append(GetPlayer()->GetPackGUID());
        *data << uint32(0);
        *data << uint32(secsToTimeBitFields(iter->second->date));
        *data << uint32(0);
        *data << uint32(0);
    }

    *data << int32(-1);
}

