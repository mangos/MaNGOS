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
#include "GameEvent.h"
#include "World.h"

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

            AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(progress->id);
            if(!criteria ||
                    criteria->timeLimit && progress->date + criteria->timeLimit < time(NULL))
            {
                delete progress;
                continue;
            }
            m_criteriaProgress[progress->id] = progress;
        } while(criteriaResult->NextRow());
        delete criteriaResult;
    }

}

void AchievementMgr::SendAchievementEarned(AchievementEntry const* achievement)
{
    sLog.outString("AchievementMgr::SendAchievementEarned(%u)", achievement->ID);

    const char *msg = "|Hplayer:$N|h[$N]|h has earned the achievement $a!";
    if(Guild* guild = objmgr.GetGuildById(GetPlayer()->GetGuildId()))
    {
        WorldPacket data(SMSG_MESSAGECHAT, 200);
        data << uint8(CHAT_MSG_ACHIEVEMENT);
        data << uint8(CHAT_MSG_GUILD_ACHIEVEMENT);
        data << uint32(LANG_UNIVERSAL);
        data << uint64(GetPlayer()->GetGUID());
        data << uint32(5);
        data << uint64(GetPlayer()->GetGUID());
        data << uint32(strlen(msg)+1);
        data << msg;
        data << uint8(0);
        data << uint32(achievement->ID);
        guild->BroadcastPacket(&data);
    }
    if(achievement->flags & (ACHIEVEMENT_FLAG_REALM_FIRST_KILL|ACHIEVEMENT_FLAG_REALM_FIRST_REACH))
    {
        // broadcast realm first reached
        WorldPacket data(SMSG_SERVER_FIRST_ACHIEVEMENT, strlen(GetPlayer()->GetName())+1+8+4+4);
        data << GetPlayer()->GetName();
        data << uint64(GetPlayer()->GetGUID());
        data << uint32(achievement->ID);
        data << uint32(0);  // 1=link supplied string as player name, 0=display plain string
        sWorld.SendGlobalMessage(&data);
    }
    else
    {
        WorldPacket data(SMSG_MESSAGECHAT, 200);
        data << uint8(CHAT_MSG_ACHIEVEMENT);
        data << uint32(LANG_UNIVERSAL);
        data << uint64(GetPlayer()->GetGUID());
        data << uint32(5);
        data << uint64(GetPlayer()->GetGUID());
        data << uint32(strlen(msg)+1);
        data << msg;
        data << uint8(0);
        data << uint32(achievement->ID);
        GetPlayer()->SendMessageToSet(&data, true);

    }
    WorldPacket data(SMSG_ACHIEVEMENT_EARNED, 8+4+8);
    data.append(GetPlayer()->GetPackGUID());
    data << uint32(achievement->ID);
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

        AchievementEntry const *achievement = sAchievementStore.LookupEntry(achievementCriteria->referredAchievement);
        if(!achievement)
            continue;

        if(achievement->factionFlag == ACHIEVEMENT_FACTION_FLAG_HORDE && GetPlayer()->GetTeam() != HORDE ||
            achievement->factionFlag == ACHIEVEMENT_FACTION_FLAG_ALLIANCE && GetPlayer()->GetTeam() != ALLIANCE)
            continue;

        switch (type)
        {
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
                SetCriteriaProgress(achievementCriteria, GetPlayer()->getLevel());
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
                SetCriteriaProgress(achievementCriteria, GetPlayer()->GetByteValue(PLAYER_BYTES_2, 2)+1);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(achievementCriteria->kill_creature.creatureID != miscvalue1)
                    continue;
                SetCriteriaProgress(achievementCriteria, miscvalue2, true);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
                if(uint32 skillvalue = GetPlayer()->GetBaseSkillValue(achievementCriteria->reach_skill_level.skillID))
                    SetCriteriaProgress(achievementCriteria, skillvalue);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
            {
                uint32 counter =0;
                for(QuestStatusMap::iterator itr = GetPlayer()->getQuestStatusMap().begin(); itr!=GetPlayer()->getQuestStatusMap().end(); itr++)
                    if(itr->second.m_rewarded)
                        counter++;
                SetCriteriaProgress(achievementCriteria, counter);
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            {
                uint32 counter =0;
                for(QuestStatusMap::iterator itr = GetPlayer()->getQuestStatusMap().begin(); itr!=GetPlayer()->getQuestStatusMap().end(); itr++)
                {
                    Quest const* quest = objmgr.GetQuestTemplate(itr->first);
                    if(itr->second.m_rewarded && quest->GetZoneOrSort() == achievementCriteria->complete_quests_in_zone.zoneID)
                        counter++;
                }
                SetCriteriaProgress(achievementCriteria, counter);
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                SetCriteriaProgress(achievementCriteria, miscvalue1, true);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(GetPlayer()->GetMapId() != achievementCriteria->complete_battleground.mapID)
                    continue;
                SetCriteriaProgress(achievementCriteria, miscvalue1, true);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
                if(GetPlayer()->HasSpell(achievementCriteria->learn_spell.spellID))
                    SetCriteriaProgress(achievementCriteria, 1);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(GetPlayer()->GetMapId() != achievementCriteria->death_at_map.mapID)
                    continue;
                SetCriteriaProgress(achievementCriteria, 1, true);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(miscvalue1 != achievementCriteria->killed_by_creature.creatureEntry)
                    continue;
                SetCriteriaProgress(achievementCriteria, 1, true);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                SetCriteriaProgress(achievementCriteria, 1, true);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(achievement->ID == 1260)
                {
                    if(Player::GetDrunkenstateByValue(GetPlayer()->GetDrunkValue()) != DRUNKEN_SMASHED)
                        continue;
                    // TODO: hardcoding eventid is bad, it can differ from DB to DB - maye implement something using HolidayNames.dbc?
                    if(!gameeventmgr.IsActiveEvent(26))
                        continue;
                }
                // miscvalue1 is falltime, calculate to fall height format given in dbc
                uint32 fallHeight = uint32(0.06f*miscvalue1-91.5)*100;
                SetCriteriaProgress(achievementCriteria, fallHeight);
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
                if(GetPlayer()->GetQuestRewardStatus(achievementCriteria->complete_quest.questID))
                    SetCriteriaProgress(achievementCriteria, 1);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(achievementCriteria->use_item.itemID != miscvalue1)
                    continue;
                SetCriteriaProgress(achievementCriteria, 1, true);
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
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            return progress->counter >= achievementCriteria->kill_creature.creatureCount;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            return m_completedAchievements.find(achievementCriteria->complete_achievement.linkedAchievement) != m_completedAchievements.end();
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
            return progress->counter >= achievementCriteria->reach_skill_level.skillLevel;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            return progress->counter >= achievementCriteria->complete_quests_in_zone.questCount;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            return progress->counter >= achievementCriteria->complete_daily_quest.questCount;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
            return progress->counter >= 1;
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
            return progress->counter >= achievementCriteria->fall_without_dying.fallHeight;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            return progress->counter >= 1;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
            return progress->counter >= achievementCriteria->use_item.itemCount;

        // handle all statistic-only criteria here
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
            return false;
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

// TODO: achievement 705 requires 4 criteria to be fulfilled
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

void AchievementMgr::SetCriteriaProgress(AchievementCriteriaEntry const* entry, uint32 newValue, bool relative)
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
        if(relative)
            newValue += progress->counter;
        if(progress->counter == newValue)
            return;
        progress->counter = newValue;
    }
    if(entry->timeLimit)
    {
        time_t now = time(NULL);
        if(progress->date + entry->timeLimit < now)
        {
            progress->counter = 1;
        }
        // also it seems illogical, the timeframe will be extended at every criteria update
        progress->date = now;
    }
    SendCriteriaUpdate(progress);
}

void AchievementMgr::CompletedAchievement(AchievementEntry const* achievement)
{
    sLog.outString("AchievementMgr::CompletedAchievement(%u)", achievement->ID);
    if(achievement->flags & ACHIEVEMENT_FLAG_COUNTER || m_completedAchievements.find(achievement->ID)!=m_completedAchievements.end())
        return;

    SendAchievementEarned(achievement);
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

