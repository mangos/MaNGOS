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
#include "AchievementMgr.h"
#include "Player.h"
#include "WorldPacket.h"
#include "DBCEnums.h"
#include "GameEventMgr.h"
#include "ObjectMgr.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Database/DatabaseEnv.h"
#include "World.h"
#include "SpellMgr.h"
#include "ArenaTeam.h"
#include "ProgressBar.h"
#include "Mail.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Language.h"
#include "MapManager.h"
#include "BattleGround.h"
#include "BattleGroundAB.h"
#include "Map.h"
#include "InstanceData.h"

#include "Policies/SingletonImp.h"

INSTANTIATE_SINGLETON_1(AchievementGlobalMgr);

namespace MaNGOS
{
    class AchievementChatBuilder
    {
        public:
            AchievementChatBuilder(Player const& pl, ChatMsg msgtype, int32 textId, uint32 ach_id)
                : i_player(pl), i_msgtype(msgtype), i_textId(textId), i_achievementId(ach_id) {}
            void operator()(WorldPacket& data, int32 loc_idx)
            {
                char const* text = sObjectMgr.GetMangosString(i_textId,loc_idx);

                data << uint8(i_msgtype);
                data << uint32(LANG_UNIVERSAL);
                data << i_player.GetObjectGuid();
                data << uint32(5);
                data << i_player.GetObjectGuid();
                data << uint32(strlen(text)+1);
                data << text;
                data << uint8(0);
                data << uint32(i_achievementId);
            }

        private:
            Player const& i_player;
            ChatMsg i_msgtype;
            int32 i_textId;
            uint32 i_achievementId;
    };
}                                                           // namespace MaNGOS


bool AchievementCriteriaRequirement::IsValid(AchievementCriteriaEntry const* criteria)
{
    switch(criteria->requiredType)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:      // only hardcoded list
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
            break;
        default:
            sLog.outErrorDb( "Table `achievement_criteria_requirement` have not supported data for criteria %u (Not supported as of its criteria type: %u), ignore.", criteria->ID, criteria->requiredType);
            return false;
    }

    switch(requirementType)
    {
        case ACHIEVEMENT_CRITERIA_REQUIRE_NONE:
        case ACHIEVEMENT_CRITERIA_REQUIRE_VALUE:
        case ACHIEVEMENT_CRITERIA_REQUIRE_DISABLED:
        case ACHIEVEMENT_CRITERIA_REQUIRE_BG_LOSS_TEAM_SCORE:
        case ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT:
        case ACHIEVEMENT_CRITERIA_REQUIRE_NTH_BIRTHDAY:
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_CREATURE:
            if (!creature.id || !ObjectMgr::GetCreatureTemplate(creature.id))
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_CREATURE (%u) have nonexistent creature id in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,creature.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_CLASS_RACE:
            if (!classRace.class_id && !classRace.race_id)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_PLAYER_CLASS_RACE (%u) must have not 0 in one from value fields, ignore.",
                    criteria->ID, criteria->requiredType,requirementType);
                return false;
            }
            if (classRace.class_id && ((1 << (classRace.class_id-1)) & CLASSMASK_ALL_PLAYABLE)==0)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_CREATURE (%u) have nonexistent class in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,classRace.class_id);
                return false;
            }
            if (classRace.race_id && ((1 << (classRace.race_id-1)) & RACEMASK_ALL_PLAYABLE)==0)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_CREATURE (%u) have nonexistent race in value2 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,classRace.race_id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_LESS_HEALTH:
            if (health.percent < 1 || health.percent > 100)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_PLAYER_LESS_HEALTH (%u) have wrong percent value in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,health.percent);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_DEAD:
            if (player_dead.own_team_flag > 1)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_DEAD (%u) have wrong boolean value1 (%u).",
                    criteria->ID, criteria->requiredType,requirementType,player_dead.own_team_flag);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA:
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA:
        {
            SpellEntry const* spellEntry = sSpellStore.LookupEntry(aura.spell_id);
            if (!spellEntry)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement %s (%u) have wrong spell id in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,(requirementType==ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA?"ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA":"ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA"),requirementType,aura.spell_id);
                return false;
            }
            if (aura.effect_idx >= MAX_EFFECT_INDEX)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement %s (%u) have wrong spell effect index in value2 (%u), ignore.",
                    criteria->ID, criteria->requiredType,(requirementType==ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA?"ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA":"ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA"),requirementType,aura.effect_idx);
                return false;
            }
            if (!spellEntry->EffectApplyAuraName[aura.effect_idx])
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement %s (%u) have non-aura spell effect (ID: %u Effect: %u), ignore.",
                    criteria->ID, criteria->requiredType,(requirementType==ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA?"ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA":"ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA"),requirementType,aura.spell_id,aura.effect_idx);
                return false;
            }
            return true;
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_AREA:
            if (!GetAreaEntryByAreaID(area.id))
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_S_AREA (%u) have wrong area id in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,area.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_LEVEL:
            if (level.minlevel > STRONG_MAX_LEVEL)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_T_LEVEL (%u) have wrong minlevel in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,level.minlevel);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_GENDER:
            if (gender.gender > GENDER_NONE)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_T_GENDER (%u) have wrong gender in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,gender.gender);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_MAP_DIFFICULTY:
            if (difficulty.difficulty >= MAX_DIFFICULTY)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_MAP_DIFFICULTY (%u) have wrong difficulty in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,difficulty.difficulty);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_MAP_PLAYER_COUNT:
            if (map_players.maxcount <= 0)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_MAP_PLAYER_COUNT (%u) have wrong max players count in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,map_players.maxcount);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_TEAM:
            if (team.team != ALLIANCE && team.team != HORDE)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_T_TEAM (%u) have unknown team in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,team.team);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_DRUNK:
            if(drunk.state >= MAX_DRUNKEN)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_S_DRUNK (%u) have unknown drunken state in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,drunk.state);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_HOLIDAY:
            if (!sHolidaysStore.LookupEntry(holiday.id))
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_HOLIDAY (%u) have unknown holiday in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,holiday.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPPED_ITEM_LVL:
            if(equipped_item.item_quality >= MAX_ITEM_QUALITY)
            {
                sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPPED_ITEM_LVL (%u) have unknown quality state in value1 (%u), ignore.",
                    criteria->ID, criteria->requiredType,requirementType,equipped_item.item_quality);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_KNOWN_TITLE:
            {
                CharTitlesEntry const *titleInfo = sCharTitlesStore.LookupEntry(known_title.title_id);
                if (!titleInfo)
                {
                    sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_KNOWN_TITLE (%u) have unknown title_id in value1 (%u), ignore.",
                        criteria->ID, criteria->requiredType, requirementType, known_title.title_id);
                    return false;
                }
                return true;
            }
        default:
            sLog.outErrorDb( "Table `achievement_criteria_requirement` (Entry: %u Type: %u) have data for not supported data type (%u), ignore.", criteria->ID, criteria->requiredType,requirementType);
            return false;
    }
}

bool AchievementCriteriaRequirement::Meets(uint32 criteria_id, Player const* source, Unit const* target, uint32 miscvalue1 /*= 0*/) const
{
    switch(requirementType)
    {
        case ACHIEVEMENT_CRITERIA_REQUIRE_NONE:
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_CREATURE:
            if (!target || target->GetTypeId()!=TYPEID_UNIT)
                return false;
            return target->GetEntry() == creature.id;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_CLASS_RACE:
            if (!target || target->GetTypeId()!=TYPEID_PLAYER)
                return false;
            if(classRace.class_id && classRace.class_id != ((Player*)target)->getClass())
                return false;
            if(classRace.race_id && classRace.race_id != ((Player*)target)->getRace())
                return false;
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_LESS_HEALTH:
            if (!target || target->GetTypeId()!=TYPEID_PLAYER)
                return false;
            return target->GetHealth()*100 <= health.percent*target->GetMaxHealth();
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_DEAD:
            if (!target || target->GetTypeId() != TYPEID_PLAYER || target->isAlive() || ((Player*)target)->GetDeathTimer() == 0)
                return false;
            // flag set == must be same team, not set == different team
            return (((Player*)target)->GetTeam() == source->GetTeam()) == (player_dead.own_team_flag != 0);
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA:
            return source->HasAura(aura.spell_id,SpellEffectIndex(aura.effect_idx));
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_AREA:
        {
            uint32 zone_id,area_id;
            source->GetZoneAndAreaId(zone_id,area_id);
            return area.id==zone_id || area.id==area_id;
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA:
            return target && target->HasAura(aura.spell_id,SpellEffectIndex(aura.effect_idx));
        case ACHIEVEMENT_CRITERIA_REQUIRE_VALUE:
            return miscvalue1 >= value.minvalue;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_LEVEL:
            if (!target)
                return false;
            return target->getLevel() >= level.minlevel;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_GENDER:
            if (!target)
                return false;
            return target->getGender() == gender.gender;
        case ACHIEVEMENT_CRITERIA_REQUIRE_DISABLED:
            return false;                                   // always fail
        case ACHIEVEMENT_CRITERIA_REQUIRE_MAP_DIFFICULTY:
            return source->GetMap()->GetSpawnMode()==difficulty.difficulty;
        case ACHIEVEMENT_CRITERIA_REQUIRE_MAP_PLAYER_COUNT:
            return source->GetMap()->GetPlayersCountExceptGMs() <= map_players.maxcount;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_TEAM:
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
                return false;
            return ((Player*)target)->GetTeam() == team.team;
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_DRUNK:
            return (uint32)Player::GetDrunkenstateByValue(source->GetDrunkValue()) >= drunk.state;
        case ACHIEVEMENT_CRITERIA_REQUIRE_HOLIDAY:
            return sGameEventMgr.IsActiveHoliday(HolidayIds(holiday.id));
        case ACHIEVEMENT_CRITERIA_REQUIRE_BG_LOSS_TEAM_SCORE:
        {
            BattleGround* bg = source->GetBattleGround();
            if(!bg)
                return false;
            return bg->IsTeamScoreInRange(source->GetTeam()==ALLIANCE ? HORDE : ALLIANCE,bg_loss_team_score.min_score,bg_loss_team_score.max_score);
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT:
        {
            if (!source->IsInWorld())
                return false;
            InstanceData* data = source->GetInstanceData();
            if (!data)
            {
                sLog.outErrorDb("Achievement system call ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT (%u) for achievement criteria %u for map %u but map not have instance script",
                    ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT, criteria_id, source->GetMapId());
                return false;
            }
            return data->CheckAchievementCriteriaMeet(criteria_id, source, target, miscvalue1);
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPPED_ITEM_LVL:
        {
            Item* item = source->GetItemByPos(INVENTORY_SLOT_BAG_0,miscvalue1);
            if (!item)
                return false;
            ItemPrototype const* proto = item->GetProto();
            return proto->ItemLevel >= equipped_item.item_level && proto->Quality >= equipped_item.item_quality;
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_NTH_BIRTHDAY:
        {
            time_t birthday_start = time_t(sWorld.getConfig(CONFIG_UINT32_BIRTHDAY_TIME));

            tm birthday_tm = *localtime(&birthday_start);

            // exactly N birthday
            birthday_tm.tm_year += birthday_login.nth_birthday;

            time_t birthday = mktime(&birthday_tm);

            time_t now = sWorld.GetGameTime();
            return now <= birthday + DAY && now >= birthday;
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_KNOWN_TITLE:
        {
            if (CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(known_title.title_id))
                return source && source->HasTitle(titleInfo->bit_index);

            return false;
        }
    }
    return false;
}

bool AchievementCriteriaRequirementSet::Meets(Player const* source, Unit const* target, uint32 miscvalue /*= 0*/) const
{
    for(Storage::const_iterator itr = storage.begin(); itr != storage.end(); ++itr)
        if(!itr->Meets(criteria_id, source, target, miscvalue))
            return false;

    return true;
}

AchievementMgr::AchievementMgr(Player *player)
{
    m_player = player;
}

AchievementMgr::~AchievementMgr()
{
}

void AchievementMgr::Reset()
{
    for(CompletedAchievementMap::const_iterator iter = m_completedAchievements.begin(); iter!=m_completedAchievements.end(); ++iter)
    {
        WorldPacket data(SMSG_ACHIEVEMENT_DELETED,4);
        data << uint32(iter->first);
        m_player->SendDirectMessage(&data);
    }

    for(CriteriaProgressMap::const_iterator iter = m_criteriaProgress.begin(); iter!=m_criteriaProgress.end(); ++iter)
    {
        WorldPacket data(SMSG_CRITERIA_DELETED,4);
        data << uint32(iter->first);
        m_player->SendDirectMessage(&data);
    }

    m_completedAchievements.clear();
    m_criteriaProgress.clear();
    DeleteFromDB(m_player->GetObjectGuid());

    // re-fill data
    CheckAllAchievementCriteria();
}

void AchievementMgr::ResetAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1, uint32 miscvalue2)
{
    DETAIL_FILTER_LOG(LOG_FILTER_ACHIEVEMENT_UPDATES, "AchievementMgr::ResetAchievementCriteria(%u, %u, %u)", type, miscvalue1, miscvalue2);

    if (!sWorld.getConfig(CONFIG_BOOL_GM_ALLOW_ACHIEVEMENT_GAINS) && m_player->GetSession()->GetSecurity() > SEC_PLAYER)
        return;

    AchievementCriteriaEntryList const& achievementCriteriaList = sAchievementMgr.GetAchievementCriteriaByType(type);
    for(AchievementCriteriaEntryList::const_iterator i = achievementCriteriaList.begin(); i!=achievementCriteriaList.end(); ++i)
    {
        AchievementCriteriaEntry const *achievementCriteria = (*i);

        AchievementEntry const *achievement = sAchievementStore.LookupEntry(achievementCriteria->referredAchievement);
        // Checked in LoadAchievementCriteriaList

        // don't update already completed criteria
        if (IsCompletedCriteria(achievementCriteria,achievement))
            continue;

        switch (type)
        {
            case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:     // have total statistic also not expected to be reset
            case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:    // have total statistic also not expected to be reset
                if (achievementCriteria->healing_done.flag == miscvalue1 &&
                    achievementCriteria->healing_done.mapid == miscvalue2)
                    SetCriteriaProgress(achievementCriteria, achievement, 0, PROGRESS_SET);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA: // have total statistic also not expected to be reset
                // reset only the criteria having the miscvalue1 condition
                if (achievementCriteria->win_rated_arena.flag == miscvalue1)
                    SetCriteriaProgress(achievementCriteria, achievement, 0, PROGRESS_SET);
                break;
            default:                                        // reset all cases
                break;
        }
    }
}

void AchievementMgr::DeleteFromDB(ObjectGuid guid)
{
    uint32 lowguid = guid.GetCounter();
    CharacterDatabase.BeginTransaction ();
    CharacterDatabase.PExecute("DELETE FROM character_achievement WHERE guid = %u", lowguid);
    CharacterDatabase.PExecute("DELETE FROM character_achievement_progress WHERE guid = %u", lowguid);
    CharacterDatabase.CommitTransaction ();
}

void AchievementMgr::SaveToDB()
{
    static SqlStatementID delComplAchievements ;
    static SqlStatementID insComplAchievements ;
    static SqlStatementID delProgress ;
    static SqlStatementID insProgress ;

    if(!m_completedAchievements.empty())
    {
        //delete existing achievements in the loop
        for(CompletedAchievementMap::iterator iter = m_completedAchievements.begin(); iter!=m_completedAchievements.end(); ++iter)
        {
            if(!iter->second.changed)
                continue;

            /// mark as saved in db
            iter->second.changed = false;

            SqlStatement stmt = CharacterDatabase.CreateStatement(delComplAchievements, "DELETE FROM character_achievement WHERE guid = ? AND achievement = ?");
            stmt.PExecute(GetPlayer()->GetGUIDLow(), iter->first);

            stmt = CharacterDatabase.CreateStatement(insComplAchievements, "INSERT INTO character_achievement (guid, achievement, date) VALUES (?, ?, ?)");
            stmt.PExecute(GetPlayer()->GetGUIDLow(), iter->first, uint64(iter->second.date));
        }
    }

    if(!m_criteriaProgress.empty())
    {
        //insert achievements
        for(CriteriaProgressMap::iterator iter = m_criteriaProgress.begin(); iter!=m_criteriaProgress.end(); ++iter)
        {
            if(!iter->second.changed)
                continue;

            /// mark as updated in db
            iter->second.changed = false;

            // new/changed record data
            SqlStatement stmt = CharacterDatabase.CreateStatement(delProgress, "DELETE FROM character_achievement_progress WHERE guid = ? AND criteria = ?");
            stmt.PExecute(GetPlayer()->GetGUIDLow(), iter->first);

            bool needSave = iter->second.counter != 0;
            if (!needSave)
            {
                AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(iter->first);
                needSave = criteria && criteria->timeLimit > 0;
            }

            if (needSave)
            {
                stmt = CharacterDatabase.CreateStatement(insProgress, "INSERT INTO character_achievement_progress (guid, criteria, counter, date) VALUES (?, ?, ?, ?)");
                stmt.PExecute(GetPlayer()->GetGUIDLow(), iter->first, iter->second.counter, uint64(iter->second.date));
            }
        }
    }
}

void AchievementMgr::LoadFromDB(QueryResult *achievementResult, QueryResult *criteriaResult)
{
    // Note: this code called before any character data loading so don't must triggering any events req. inventory/etc
    // all like cases must be happens in CheckAllAchievementCriteria called after character data load

    if(achievementResult)
    {
        do
        {
            Field *fields = achievementResult->Fetch();

            uint32 achievement_id = fields[0].GetUInt32();

            // don't must happen: cleanup at server startup in sAchievementMgr.LoadCompletedAchievements()
            if(!sAchievementStore.LookupEntry(achievement_id))
                continue;

            CompletedAchievementData& ca = m_completedAchievements[achievement_id];
            ca.date = time_t(fields[1].GetUInt64());
            ca.changed = false;
        } while(achievementResult->NextRow());
        delete achievementResult;
    }

    if(criteriaResult)
    {
        do
        {
            Field *fields = criteriaResult->Fetch();

            uint32 id      = fields[0].GetUInt32();
            uint32 counter = fields[1].GetUInt32();
            time_t date    = time_t(fields[2].GetUInt64());

            AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(id);
            if (!criteria)
            {
                // we will remove nonexistent criteria for all characters
                sLog.outError("Nonexistent achievement criteria %u data removed from table `character_achievement_progress`.",id);
                CharacterDatabase.PExecute("DELETE FROM character_achievement_progress WHERE criteria = %u",id);
                continue;
            }

            CriteriaProgress& progress = m_criteriaProgress[id];
            progress.counter = counter;
            progress.date    = date;
            progress.changed = false;
            progress.timedCriteriaFailed = false;

            AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->referredAchievement);
            // Checked in LoadAchievementCriteriaList

            // A failed achievement will be removed on next tick - TODO: Possible that timer 2 is reseted
            if (criteria->timeLimit)
            {
                // Add not-completed achievements to time map
                if (!IsCompletedCriteria(criteria, achievement))
                {
                    time_t failTime = time_t(progress.date + criteria->timeLimit);
                    m_criteriaFailTimes[criteria->ID] = failTime;
                    // A failed Achievement - will be removed by DoFailedTimedAchievementCriterias on next tick for player
                    if (failTime <= time(NULL))
                        progress.timedCriteriaFailed = true;
                }
            }

            // check integrity with max allowed counter value
            if (uint32 maxcounter = GetCriteriaProgressMaxCounter(criteria, achievement))
            {
                if (progress.counter > maxcounter)
                {
                    progress.counter = maxcounter;
                    progress.changed = true;
                }
            }
        } while(criteriaResult->NextRow());
        delete criteriaResult;
    }

}

void AchievementMgr::SendAchievementEarned(AchievementEntry const* achievement)
{
    if(GetPlayer()->GetSession()->PlayerLoading())
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_ACHIEVEMENT_UPDATES, "AchievementMgr::SendAchievementEarned(%u)", achievement->ID);

    if (Guild* guild = sGuildMgr.GetGuildById(GetPlayer()->GetGuildId()))
    {
        MaNGOS::AchievementChatBuilder say_builder(*GetPlayer(), CHAT_MSG_GUILD_ACHIEVEMENT, LANG_ACHIEVEMENT_EARNED,achievement->ID);
        MaNGOS::LocalizedPacketDo<MaNGOS::AchievementChatBuilder> say_do(say_builder);
        guild->BroadcastWorker(say_do,GetPlayer());
    }

    if(achievement->flags & (ACHIEVEMENT_FLAG_REALM_FIRST_KILL|ACHIEVEMENT_FLAG_REALM_FIRST_REACH))
    {
        // broadcast realm first reached
        WorldPacket data(SMSG_SERVER_FIRST_ACHIEVEMENT, strlen(GetPlayer()->GetName())+1+8+4+4);
        data << GetPlayer()->GetName();
        data << GetPlayer()->GetObjectGuid();
        data << uint32(achievement->ID);
        data << uint32(0);                                  // 1=link supplied string as player name, 0=display plain string
        sWorld.SendGlobalMessage(&data);
    }
    // if player is in world he can tell his friends about new achievement
    else if (GetPlayer()->IsInWorld())
    {
        MaNGOS::AchievementChatBuilder say_builder(*GetPlayer(), CHAT_MSG_ACHIEVEMENT, LANG_ACHIEVEMENT_EARNED,achievement->ID);
        MaNGOS::LocalizedPacketDo<MaNGOS::AchievementChatBuilder> say_do(say_builder);
        MaNGOS::CameraDistWorker<MaNGOS::LocalizedPacketDo<MaNGOS::AchievementChatBuilder> > say_worker(GetPlayer(),sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_SAY),say_do);

        Cell::VisitWorldObjects(GetPlayer(), say_worker, sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_SAY));
    }

    WorldPacket data(SMSG_ACHIEVEMENT_EARNED, 8+4+8);
    data << GetPlayer()->GetPackGUID();
    data << uint32(achievement->ID);
    data << uint32(secsToTimeBitFields(time(NULL)));
    data << uint32(0);
    GetPlayer()->SendMessageToSetInRange(&data, sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_SAY), true);
}

void AchievementMgr::SendCriteriaUpdate(uint32 id, CriteriaProgress const* progress)
{
    WorldPacket data(SMSG_CRITERIA_UPDATE, 8+4+8);
    data << uint32(id);

    time_t now = time(NULL);
    // the counter is packed like a packed Guid
    data.appendPackGUID(progress->counter);

    data << GetPlayer()->GetPackGUID();
    data << uint32(progress->timedCriteriaFailed ? 1 : 0);
    data << uint32(secsToTimeBitFields(now));
    data << uint32(now - progress->date);                   // timer 1
    data << uint32(now - progress->date);                   // timer 2
    GetPlayer()->SendDirectMessage(&data);
}

/**
 * called at player login. The player might have fulfilled some achievements when the achievement system wasn't working yet
 */
void AchievementMgr::CheckAllAchievementCriteria()
{
    // suppress sending packets
    for(uint32 i=0; i<ACHIEVEMENT_CRITERIA_TYPE_TOTAL; ++i)
        UpdateAchievementCriteria(AchievementCriteriaTypes(i));
}

static const uint32 achievIdByArenaSlot[MAX_ARENA_SLOT] = { 1057, 1107, 1108 };
static const uint32 achievIdForDungeon[][4] =
{
    // ach_cr_id,is_dungeon,is_raid,is_heroic_dungeon
    { 321,       true,      true,   true  },                // Total raid and dungeon deaths
    //323                                                   // Total deaths to Lich King 10-player raid bosses
    //324                                                   // Total deaths to Lich King 25-player raid bosses
    { 916,       false,     true,   false },                // Total deaths in 25-player raids
    { 917,       false,     true,   false },                // Total deaths in 10-player raids
    { 918,       true,      false,  false },                // Total deaths in 5-player dungeons
    { 2219,      false,     false,  true  },                // Total deaths in 5-player heroic dungeons
    { 0,         false,     false,  false }
};

static const uint32 achievIdByClass[MAX_CLASSES] = { 0, 459, 465 , 462, 458, 464, 461, 467, 460, 463, 0, 466 };
static const uint32 achievIdByRace[MAX_RACES]    = { 0, 1408, 1410, 1407, 1409, 1413, 1411, 1404, 1412, 0, 1405, 1406 };

/**
 * this function will be called whenever the user might have done a timed-criteria relevant action, or by scripting side?
 */
void AchievementMgr::StartTimedAchievementCriteria(AchievementCriteriaTypes type, uint32 timedRequirementId, time_t startTime /*= 0*/)
{
    DETAIL_FILTER_LOG(LOG_FILTER_ACHIEVEMENT_UPDATES, "AchievementMgr::StartTimedAchievementCriteria(%u, %u)", type, timedRequirementId);

    if (!sWorld.getConfig(CONFIG_BOOL_GM_ALLOW_ACHIEVEMENT_GAINS) && m_player->GetSession()->GetSecurity() > SEC_PLAYER)
        return;

    AchievementCriteriaEntryList const& achievementCriteriaList = sAchievementMgr.GetAchievementCriteriaByType(type);
    for(AchievementCriteriaEntryList::const_iterator i = achievementCriteriaList.begin(); i!=achievementCriteriaList.end(); ++i)
    {
        AchievementCriteriaEntry const *achievementCriteria = (*i);

        // only apply to specific timedRequirementId related criteria
        if (achievementCriteria->timedCriteriaMiscId != timedRequirementId)
            continue;

        if (!achievementCriteria->IsExplicitlyStartedTimedCriteria())
            continue;

        AchievementEntry const *achievement = sAchievementStore.LookupEntry(achievementCriteria->referredAchievement);
        // Checked in LoadAchievementCriteriaList

        if ((achievement->factionFlag == ACHIEVEMENT_FACTION_FLAG_HORDE    && GetPlayer()->GetTeam() != HORDE) ||
            (achievement->factionFlag == ACHIEVEMENT_FACTION_FLAG_ALLIANCE && GetPlayer()->GetTeam() != ALLIANCE))
            continue;

        // don't update already completed criteria
        if (IsCompletedCriteria(achievementCriteria,achievement))
            continue;

        // Only the Quest-Complete Timed Achievements need the groupcheck, so this check is only needed here
        if (achievementCriteria->requiredType == ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST && GetPlayer()->GetGroup())
            continue;

        // do not start already failed timers
        if (startTime && time_t(startTime + achievementCriteria->timeLimit) < time(NULL))
            continue;

        CriteriaProgress* progress = NULL;

        CriteriaProgressMap::iterator iter = m_criteriaProgress.find(achievementCriteria->ID);
        if (iter == m_criteriaProgress.end())
            progress = &m_criteriaProgress[achievementCriteria->ID];
        else
            progress = &iter->second;

        progress->changed = true;
        progress->counter = 0;

        // Start with given startTime or now
        progress->date = startTime ? startTime : time(NULL);
        progress->timedCriteriaFailed = false;

        // Add to timer map
        m_criteriaFailTimes[achievementCriteria->ID] = time_t(progress->date + achievementCriteria->timeLimit);

        SendCriteriaUpdate(achievementCriteria->ID, progress);
    }
}

/**
 * this function will be called whenever there could be a timed achievement criteria failed because of ellapsed time
 */
void AchievementMgr::DoFailedTimedAchievementCriterias()
{
    if (m_criteriaFailTimes.empty())
        return;

    time_t now = time(NULL);
    for (AchievementCriteriaFailTimeMap::iterator iter = m_criteriaFailTimes.begin(); iter != m_criteriaFailTimes.end();)
    {
        if (iter->second > now)
        {
            ++iter;
            continue;
        }

        // Possible failed achievement criteria found
        AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(iter->first);
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->referredAchievement);
        // Checked in LoadAchievementCriteriaList

        // Send Fail for failed criterias
        if (!IsCompletedCriteria(criteria, achievement))
        {
            DETAIL_FILTER_LOG(LOG_FILTER_ACHIEVEMENT_UPDATES, "AchievementMgr::DoFailedTimedAchievementCriterias for criteria %u", criteria->ID);

            CriteriaProgressMap::iterator pro_iter = m_criteriaProgress.find(criteria->ID);
            MANGOS_ASSERT(pro_iter != m_criteriaProgress.end());

            CriteriaProgress* progress = &pro_iter->second;

            // Set to failed, and send to client
            progress->timedCriteriaFailed = true;
            SendCriteriaUpdate(criteria->ID, progress);

            // Remove failed progress
            m_criteriaProgress.erase(pro_iter);
        }

        m_criteriaFailTimes.erase(iter++);
    }
}

/**
 * this function will be called whenever the user might have done a criteria relevant action
 */
void AchievementMgr::UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1, uint32 miscvalue2, Unit *unit, uint32 time)
{
    DETAIL_FILTER_LOG(LOG_FILTER_ACHIEVEMENT_UPDATES, "AchievementMgr::UpdateAchievementCriteria(%u, %u, %u, %u)", type, miscvalue1, miscvalue2, time);

    if (!sWorld.getConfig(CONFIG_BOOL_GM_ALLOW_ACHIEVEMENT_GAINS) && m_player->GetSession()->GetSecurity() > SEC_PLAYER)
        return;

    AchievementCriteriaEntryList const& achievementCriteriaList = sAchievementMgr.GetAchievementCriteriaByType(type);
    for(AchievementCriteriaEntryList::const_iterator itr = achievementCriteriaList.begin(); itr != achievementCriteriaList.end(); ++itr)
    {
        AchievementCriteriaEntry const *achievementCriteria = *itr;

        AchievementEntry const *achievement = sAchievementStore.LookupEntry(achievementCriteria->referredAchievement);
        // Checked in LoadAchievementCriteriaList

        if ((achievement->factionFlag == ACHIEVEMENT_FACTION_FLAG_HORDE    && GetPlayer()->GetTeam() != HORDE) ||
            (achievement->factionFlag == ACHIEVEMENT_FACTION_FLAG_ALLIANCE && GetPlayer()->GetTeam() != ALLIANCE))
            continue;

        // don't update already completed criteria
        if (IsCompletedCriteria(achievementCriteria,achievement))
            continue;

        // init values, real set in switch
        uint32 change = 0;
        ProgressType progressType = PROGRESS_HIGHEST;       // when need it will replaced by PROGRESS_ACCUMULATE

        switch (type)
        {
            // std. case: increment at 1
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
            case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
            case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
            case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:    /* FIXME: for online player only currently */
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
            case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
            case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
            case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            // std case: increment at miscvalue1
            case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
            case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:/* FIXME: for online player only currently */
            case ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED:
            case ACHIEVEMENT_CRITERIA_TYPE_TOTAL_HEALING_RECEIVED:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                change = miscvalue1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            // std case: high value at miscvalue1
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD: /* FIXME: for online player only currently */
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_DEALT:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_RECEIVED:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEAL_CASTED:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                change = miscvalue1;
                progressType = PROGRESS_HIGHEST;
                break;

            // specialized cases

            case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                if (achievementCriteria->win_bg.bgMapID != GetPlayer()->GetMapId())
                    continue;

                if (achievementCriteria->win_bg.additionalRequirement1_type || achievementCriteria->win_bg.additionalRequirement2_type)
                {
                    // those requirements couldn't be found in the dbc
                    AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                    if (!data || !data->Meets(GetPlayer(),unit))
                        continue;
                }
                // some hardcoded requirements
                else
                {
                    BattleGround* bg = GetPlayer()->GetBattleGround();
                    if (!bg)
                        continue;

                    switch(achievementCriteria->referredAchievement)
                    {
                        case 161:                           // AB, Overcome a 500 resource disadvantage
                        {
                            if (bg->GetTypeID() != BATTLEGROUND_AB)
                                continue;
                            if (!((BattleGroundAB*)bg)->IsTeamScores500Disadvantage(GetPlayer()->GetTeam()))
                                continue;
                            break;
                        }
                        case 156:                           // AB, win while controlling all 5 flags (all nodes)
                        case 784:                           // EY, win while holding 4 bases (all nodes)
                        {
                            if(!bg->IsAllNodesConrolledByTeam(GetPlayer()->GetTeam()))
                                continue;
                            break;
                        }
                        case 1762:                          // SA, win without losing any siege vehicles
                        case 2192:                          // SA, win without losing any siege vehicles
                            continue;                       // not implemented
                    }
                }

                change = miscvalue1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(achievementCriteria->kill_creature.creatureID != miscvalue1)
                    continue;

                // those requirements couldn't be found in the dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if(!data || !data->Meets(GetPlayer(),unit))
                    continue;

                change = miscvalue2;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
            {
                bool ok = true;

                // skip wrong class achievements
                for(uint8 i = 1; i < MAX_CLASSES; ++i)
                {
                    if (achievIdByClass[i] == achievement->ID && i != GetPlayer()->getClass())
                    {
                        ok = false;
                        break;
                    }
                }

                if (!ok)
                    continue;

                // skip wrong race achievements
                for(uint8 i = 1; i < MAX_RACES; ++i)
                {
                    if (achievIdByRace[i] == achievement->ID && i != GetPlayer()->getRace())
                    {
                        ok = false;
                        break;
                    }
                }

                if (!ok)
                    continue;

                change = GetPlayer()->getLevel();
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
            {
                // update at loading or specific skill update
                if(miscvalue1 && miscvalue1 != achievementCriteria->reach_skill_level.skillID)
                    continue;
                change = GetPlayer()->GetBaseSkillValue(achievementCriteria->reach_skill_level.skillID);
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
            {
                // update at loading or specific skill update
                if(miscvalue1 && miscvalue1 != achievementCriteria->learn_skill_level.skillID)
                    continue;
                change = GetPlayer()->GetPureMaxSkillValue(achievementCriteria->learn_skill_level.skillID);
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            {
                if(m_completedAchievements.find(achievementCriteria->complete_achievement.linkedAchievement) == m_completedAchievements.end())
                    continue;

                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
            {
                uint32 counter =0;
                for(QuestStatusMap::const_iterator itr = GetPlayer()->getQuestStatusMap().begin(); itr!=GetPlayer()->getQuestStatusMap().end(); ++itr)
                    if(itr->second.m_rewarded)
                        counter++;
                change = counter;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            {
                // speedup for non-login case
                if(miscvalue1 && miscvalue1 != achievementCriteria->complete_quests_in_zone.zoneID)
                    continue;

                uint32 counter =0;
                for(QuestStatusMap::const_iterator itr = GetPlayer()->getQuestStatusMap().begin(); itr!=GetPlayer()->getQuestStatusMap().end(); ++itr)
                {
                    Quest const* quest = sObjectMgr.GetQuestTemplate(itr->first);
                    if(itr->second.m_rewarded && quest->GetZoneOrSort() >= 0 && uint32(quest->GetZoneOrSort()) == achievementCriteria->complete_quests_in_zone.zoneID)
                        counter++;
                }
                change = counter;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(GetPlayer()->GetMapId() != achievementCriteria->complete_battleground.mapID)
                    continue;
                change = miscvalue1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(GetPlayer()->GetMapId() != achievementCriteria->death_at_map.mapID)
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                // skip wrong arena achievements, if not achievIdByArenaSlot then normal total death counter
                bool notfit = false;
                for(int j = 0; j < MAX_ARENA_SLOT; ++j)
                {
                    if(achievIdByArenaSlot[j] == achievement->ID)
                    {
                        BattleGround* bg = GetPlayer()->GetBattleGround();
                        if(!bg || !bg->isArena() || ArenaTeam::GetSlotByType(bg->GetArenaType()) != j)
                            notfit = true;

                        break;
                    }
                }
                if(notfit)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;

                Map const* map = GetPlayer()->IsInWorld() ? GetPlayer()->GetMap() : sMapMgr.FindMap(GetPlayer()->GetMapId(), GetPlayer()->GetInstanceId());
                if(!map || !map->IsDungeon())
                    continue;

                // search case
                bool found = false;
                for(int j = 0; achievIdForDungeon[j][0]; ++j)
                {
                    if(achievIdForDungeon[j][0] == achievement->ID)
                    {
                        if(map->IsRaid())
                        {
                            // if raid accepted (ignore difficulty)
                            if(!achievIdForDungeon[j][2])
                                break;                      // for
                        }
                        else if(GetPlayer()->GetDungeonDifficulty()==DUNGEON_DIFFICULTY_NORMAL)
                        {
                            // dungeon in normal mode accepted
                            if(!achievIdForDungeon[j][1])
                                break;                      // for
                        }
                        else
                        {
                            // dungeon in heroic mode accepted
                            if(!achievIdForDungeon[j][3])
                                break;                      // for
                        }

                        found = true;
                        break;                              // for
                    }
                }
                if(!found)
                    continue;

                //FIXME: work only for instances where max==min for players
                if (map->GetMaxPlayers() != achievementCriteria->death_in_dungeon.manLimit)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;

            }
            case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(miscvalue1 != achievementCriteria->killed_by_creature.creatureEntry)
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;

                // if team check required: must kill by opposition faction
                if(achievement->ID==318 && miscvalue2==uint32(GetPlayer()->GetTeam()))
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;

                // those requirements couldn't be found in the dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if(!data || !data->Meets(GetPlayer(),unit))
                    continue;

                // miscvalue1 is the ingame fallheight*100 as stored in dbc
                change = miscvalue1;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if(!miscvalue1)
                    continue;
                if(miscvalue2 != achievementCriteria->death_from.type)
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            {
                // if miscvalues != 0, it contains the questID.
                if (miscvalue1)
                {
                    if (miscvalue1 != achievementCriteria->complete_quest.questID)
                        continue;
                }
                else
                {
                    // login case.
                    if(!GetPlayer()->GetQuestRewardStatus(achievementCriteria->complete_quest.questID))
                        continue;
                }

                // exist many achievements with this criteria, use at this moment hardcoded check to skip simple case
                switch(achievement->ID)
                {
                    case 31:
                    //case 1275: // these timed achievements have to be "started" on Quest Accept
                    //case 1276:
                    //case 1277:
                    case 1282:
                    case 1789:
                    {
                        // those requirements couldn't be found in the dbc
                        AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                        if(!data || !data->Meets(GetPlayer(),unit))
                            continue;
                        break;
                    }
                    default:
                        break;
                }

                // As the groupFlag had wrong meaning, only the Quest-Complete Timed Achievements need the groupcheck, so this check is only needed here
                if (achievementCriteria->timeLimit > 0 && GetPlayer()->GetGroup())
                    continue;

                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            {
                if (!miscvalue1 || miscvalue1 != achievementCriteria->be_spell_target.spellID)
                    continue;

                // those requirements couldn't be found in the dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if(!data)
                    continue;

                if(!data->Meets(GetPlayer(),unit))
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
            {
                if (!miscvalue1 || miscvalue1 != achievementCriteria->cast_spell.spellID)
                    continue;

                // those requirements couldn't be found in the dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if(!data)
                    continue;

                if(!data->Meets(GetPlayer(),unit))
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
                if(miscvalue1 && miscvalue1!=achievementCriteria->learn_spell.spellID)
                    continue;

                if(!GetPlayer()->HasSpell(achievementCriteria->learn_spell.spellID))
                    continue;

                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
            {
                // miscvalue1=loot_type (note: 0 = LOOT_CORPSE and then it ignored)
                // miscvalue2=count of item loot
                if (!miscvalue1 || !miscvalue2)
                    continue;
                if (miscvalue1 != achievementCriteria->loot_type.lootType)
                    continue;

                // zone specific
                if(achievementCriteria->loot_type.lootTypeCount==1)
                {
                    // those requirements couldn't be found in the dbc
                    AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                    if(!data || !data->Meets(GetPlayer(),unit))
                        continue;
                }

                change = miscvalue2;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
                // speedup for non-login case
                if(miscvalue1 && achievementCriteria->own_item.itemID != miscvalue1)
                    continue;
                change = GetPlayer()->GetItemCount(achievementCriteria->own_item.itemID, true);
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
                // miscvalue1 contains the personal rating
                if (!miscvalue1)                            // no update at login
                    continue;

                // additional requirements
                if(achievementCriteria->win_rated_arena.flag==ACHIEVEMENT_CRITERIA_CONDITION_NO_LOOSE)
                {
                    // those requirements couldn't be found in the dbc
                    AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                    if(!data || !data->Meets(GetPlayer(),unit,miscvalue1))
                    {
                        // reset the progress as we have a win without the requirement.
                        SetCriteriaProgress(achievementCriteria, achievement, 0, PROGRESS_SET);
                        continue;
                    }
                }

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                if (achievementCriteria->use_item.itemID != miscvalue1)
                    continue;
                // possible additional requirements
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if (data && !data->Meets(GetPlayer(), unit, miscvalue1))
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
                // You _have_ to loot that item, just owning it when logging in does _not_ count!
                if(!miscvalue1)
                    continue;
                if(miscvalue1 != achievementCriteria->own_item.itemID)
                    continue;
                change = miscvalue2;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
            {
                WorldMapOverlayEntry const* worldOverlayEntry = sWorldMapOverlayStore.LookupEntry(achievementCriteria->explore_area.areaReference);
                if(!worldOverlayEntry)
                    break;

                bool matchFound = false;
                for (int j = 0; j < MAX_WORLD_MAP_OVERLAY_AREA_IDX; ++j)
                {
                    uint32 area_id = worldOverlayEntry->areatableID[j];
                    if(!area_id)                            // array have 0 only in empty tail
                        break;

                    int32 exploreFlag = GetAreaFlagByAreaID(area_id);
                    if(exploreFlag < 0)
                        continue;

                    uint32 playerIndexOffset = uint32(exploreFlag) / 32;
                    uint32 mask = 1<< (uint32(exploreFlag) % 32);

                    if(GetPlayer()->GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + playerIndexOffset) & mask)
                    {
                        matchFound = true;
                        break;
                    }
                }

                if(!matchFound)
                    continue;

                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
                change = GetPlayer()->GetBankBagSlotCount();
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
            {
                // skip faction check only at loading
                if (miscvalue1 && miscvalue1 != achievementCriteria->gain_reputation.factionID)
                    continue;

                int32 reputation = GetPlayer()->GetReputationMgr().GetReputation(achievementCriteria->gain_reputation.factionID);
                if (reputation <= 0)
                    continue;

                change = reputation;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
            {
                change = GetPlayer()->GetReputationMgr().GetExaltedFactionCount();
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
            {
                // skip for login case
                if(!miscvalue1)
                    continue;
                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            {
                // miscvalue1 = equip_slot+1 (for avoid use 0)
                if(!miscvalue1)
                    continue;
                uint32 item_slot = miscvalue1-1;
                if(item_slot != achievementCriteria->equip_epic_item.itemSlot)
                    continue;
                // those requirements couldn't be found in the dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if(!data || !data->Meets(GetPlayer(),unit,item_slot))
                    continue;
                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            {
                // miscvalue1 = itemid
                // miscvalue2 = diced value
                if(!miscvalue1)
                    continue;
                if(miscvalue2 != achievementCriteria->roll_greed_on_loot.rollValue)
                    continue;
                ItemPrototype const *pProto = ObjectMgr::GetItemPrototype( miscvalue1 );

                uint32 requiredItemLevel = 0;
                if (achievementCriteria->ID == 2412 || achievementCriteria->ID == 2358)
                    requiredItemLevel = 185;

                if(!pProto || pProto->ItemLevel <requiredItemLevel)
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
            {
                // miscvalue1 = emote
                if(!miscvalue1)
                    continue;
                if(miscvalue1 != achievementCriteria->do_emote.emoteID)
                    continue;
                if(achievementCriteria->do_emote.count)
                {
                    // those requirements couldn't be found in the dbc
                    AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                    if(!data || !data->Meets(GetPlayer(),unit))
                        continue;
                }

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
            case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
            {
                if (!miscvalue1)
                    continue;

                if (achievementCriteria->healing_done.flag == ACHIEVEMENT_CRITERIA_CONDITION_MAP)
                {
                    if(GetPlayer()->GetMapId() != achievementCriteria->healing_done.mapid)
                        continue;

                    // map specific case (BG in fact) expected player targeted damage/heal
                    if(!unit || unit->GetTypeId()!=TYPEID_PLAYER)
                        continue;
                }

                change = miscvalue1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
                // miscvalue1 = item_id
                if (!miscvalue1)
                    continue;
                if (miscvalue1 != achievementCriteria->equip_item.itemID)
                    continue;

                change = 1;
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
                // miscvalue1 = go entry
                if (!miscvalue1)
                    continue;
                if (miscvalue1 != achievementCriteria->use_gameobject.goEntry)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;

                // those requirements couldn't be found in the dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if (!data)
                    continue;

                if (!data->Meets(GetPlayer(),unit))
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
                if (!miscvalue1)
                    continue;
                if (miscvalue1 != achievementCriteria->fish_in_gameobject.goEntry)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
            {
                if (miscvalue1 && miscvalue1 != achievementCriteria->learn_skillline_spell.skillLine)
                    continue;

                uint32 spellCount = 0;
                for (PlayerSpellMap::const_iterator spellIter = GetPlayer()->GetSpellMap().begin();
                    spellIter != GetPlayer()->GetSpellMap().end();
                    ++spellIter)
                {
                    SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spellIter->first);
                    for(SkillLineAbilityMap::const_iterator skillIter = bounds.first; skillIter != bounds.second; ++skillIter)
                    {
                        if(skillIter->second->skillId == achievementCriteria->learn_skillline_spell.skillLine)
                            spellCount++;
                    }
                }
                change = spellCount;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;

                if (achievementCriteria->win_duel.duelCount)
                {
                    // those requirements couldn't be found in the dbc
                    AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                    if (!data)
                        continue;

                    if (!data->Meets(GetPlayer(),unit))
                        continue;
                }

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
                change = GetPlayer()->GetReputationMgr().GetReveredFactionCount();
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
                change = GetPlayer()->GetReputationMgr().GetHonoredFactionCount();
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
                change = GetPlayer()->GetReputationMgr().GetVisibleFactionCount();
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
            case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
            {
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscvalue1)
                    continue;
                ItemPrototype const* proto = ObjectMgr::GetItemPrototype(miscvalue1);
                if (!proto || proto->Quality < ITEM_QUALITY_EPIC)
                    continue;
                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
            {
                if (miscvalue1 && miscvalue1 != achievementCriteria->learn_skill_line.skillLine)
                    continue;

                uint32 spellCount = 0;
                for (PlayerSpellMap::const_iterator spellIter = GetPlayer()->GetSpellMap().begin();
                    spellIter != GetPlayer()->GetSpellMap().end();
                    ++spellIter)
                {
                    SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spellIter->first);
                    for(SkillLineAbilityMap::const_iterator skillIter = bounds.first; skillIter != bounds.second; ++skillIter)
                        if (skillIter->second->skillId == achievementCriteria->learn_skill_line.skillLine)
                            spellCount++;
                }
                change = spellCount;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
                change = GetPlayer()->GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORBALE_KILLS);
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
                if (!miscvalue1 || miscvalue1 != achievementCriteria->hk_class.classID)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
                if (!miscvalue1 || miscvalue1 != achievementCriteria->hk_race.raceID)
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
                change = GetPlayer()->GetMoney();
                progressType = PROGRESS_HIGHEST;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
            {
                if(!miscvalue1 || achievementCriteria->highest_team_rating.teamtype != miscvalue1)
                    continue;

                change = miscvalue2;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
            {
                if(!miscvalue1 || achievementCriteria->highest_personal_rating.teamtype != miscvalue1)
                    continue;

                if(achievementCriteria->highest_personal_rating.teamrating != 0 && achievementCriteria->highest_personal_rating.teamrating > miscvalue2)
                    continue;

                change = miscvalue2;
                progressType = PROGRESS_HIGHEST;
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN:
            {
                // This criteria is only called directly after login - with expected miscvalue1 == 1
                if (!miscvalue1)
                    continue;

                // They have no proper requirements in dbc
                AchievementCriteriaRequirementSet const* data = sAchievementMgr.GetCriteriaRequirementSet(achievementCriteria);
                if (!data || !data->Meets(GetPlayer(), NULL))
                    continue;

                change = 1;
                progressType = PROGRESS_ACCUMULATE;
                break;
            }
            // std case: not exist in DBC, not triggered in code as result
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALTH:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELLPOWER:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_ARMOR:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_POWER:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_STAT:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_RATING:
                break;
            // FIXME: not triggered in code as result, need to implement
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID:
            case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
            case ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA:
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
            case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
            case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
            case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
            case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
                break;                                   // Not implemented yet :(
        }

        SetCriteriaProgress(achievementCriteria, achievement, change, progressType);
    }
}

uint32 AchievementMgr::GetCriteriaProgressMaxCounter(AchievementCriteriaEntry const* achievementCriteria, AchievementEntry const* achievement)
{
    uint32 resultValue = 0;
    switch (achievementCriteria->requiredType)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
            resultValue = achievementCriteria->win_bg.winCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            resultValue = achievementCriteria->kill_creature.creatureCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
            resultValue = achievementCriteria->reach_level.level;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
            resultValue = achievementCriteria->reach_skill_level.skillLevel;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            resultValue = 1;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
            resultValue = achievementCriteria->complete_quest_count.totalQuestCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            resultValue = achievementCriteria->complete_quests_in_zone.questCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
        case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
            resultValue = achievementCriteria->healing_done.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            resultValue = achievementCriteria->complete_daily_quest.questCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
            resultValue = achievementCriteria->fall_without_dying.fallHeight;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            resultValue = 1;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            resultValue = achievementCriteria->be_spell_target.spellCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
            resultValue = achievementCriteria->cast_spell.castCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
            resultValue = 1;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
            resultValue = achievementCriteria->own_item.itemCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
            resultValue = achievementCriteria->win_rated_arena.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
            resultValue = achievementCriteria->learn_skill_level.skillLevel * 75;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
            resultValue = achievementCriteria->use_item.itemCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
            resultValue = achievementCriteria->loot_item.itemCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
            resultValue = 1;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
            resultValue = achievementCriteria->buy_bank_slot.numberOfSlots;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
            resultValue = achievementCriteria->gain_reputation.reputationAmount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
            resultValue = achievementCriteria->gain_exalted_reputation.numberOfExaltedFactions;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
            resultValue = achievementCriteria->visit_barber.numberOfVisits;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            resultValue = achievementCriteria->equip_epic_item.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            resultValue = achievementCriteria->roll_greed_on_loot.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
            resultValue = achievementCriteria->hk_class.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
            resultValue = achievementCriteria->hk_race.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
            resultValue = achievementCriteria->do_emote.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
            resultValue = achievementCriteria->equip_item.count;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
            resultValue = achievementCriteria->quest_reward_money.goldInCopper;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
            resultValue = achievementCriteria->loot_money.goldInCopper;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
            resultValue = achievementCriteria->use_gameobject.useCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
            resultValue = achievementCriteria->special_pvp_kill.killCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
            resultValue = achievementCriteria->fish_in_gameobject.lootCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN:
            resultValue = 1;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
            resultValue = achievementCriteria->learn_skillline_spell.spellCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
            resultValue = achievementCriteria->win_duel.duelCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
            resultValue = achievementCriteria->loot_type.lootTypeCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
            resultValue = achievementCriteria->learn_skill_line.spellCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
            resultValue = achievementCriteria->honorable_kill.killCount;
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
            resultValue = achievementCriteria->highest_personal_rating.teamrating;
            break;

        // handle all statistic-only criteria here
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
        case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
        case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALTH:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELLPOWER:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_ARMOR:
        case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
        case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
        case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
            resultValue = 0;
            break;
    }

    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        resultValue = std::numeric_limits<uint32>::max();

    return resultValue;
}

bool AchievementMgr::IsCompletedCriteria(AchievementCriteriaEntry const* achievementCriteria, AchievementEntry const* achievement) const
{
    // counter can never complete
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    if (achievement->flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
    {
        // someone on this realm has already completed that achievement
        if (sAchievementMgr.IsRealmCompleted(achievement))
            return false;
    }

    CriteriaProgressMap::const_iterator itr = m_criteriaProgress.find(achievementCriteria->ID);
    if (itr == m_criteriaProgress.end())
        return false;

    CriteriaProgress const* progress = &itr->second;

    uint32 maxcounter = GetCriteriaProgressMaxCounter(achievementCriteria, achievement);

    return progress->counter >= maxcounter || (achievement->flags & ACHIEVEMENT_FLAG_REQ_COUNT && progress->counter);
}

void AchievementMgr::CompletedCriteriaFor(AchievementEntry const* achievement)
{
    // counter can never complete
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return;

    // already completed and stored
    if (m_completedAchievements.find(achievement->ID)!=m_completedAchievements.end())
        return;

    if (IsCompletedAchievement(achievement))
        CompletedAchievement(achievement);
}

bool AchievementMgr::IsCompletedAchievement(AchievementEntry const* entry)
{
    // counter can never complete
    if (entry->flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    // for achievement with referenced achievement criterias get from referenced and counter from self
    uint32 achievementForTestId = entry->refAchievement ? entry->refAchievement : entry->ID;
    uint32 achievementForTestCount = entry->count;

    AchievementCriteriaEntryList const* cList = sAchievementMgr.GetAchievementCriteriaByAchievement(achievementForTestId);
    if (!cList)
        return false;
    uint32 count = 0;

    // For SUMM achievements, we have to count the progress of each criteria of the achievement.
    // Oddly, the target count is NOT countained in the achievement, but in each individual criteria
    if (entry->flags & ACHIEVEMENT_FLAG_SUMM)
    {
        for (AchievementCriteriaEntryList::const_iterator itr = cList->begin(); itr != cList->end(); ++itr)
        {
            AchievementCriteriaEntry const* criteria = *itr;

            CriteriaProgressMap::const_iterator itrProgress = m_criteriaProgress.find(criteria->ID);
            if(itrProgress == m_criteriaProgress.end())
                continue;

            CriteriaProgress const* progress = &itrProgress->second;
            count += progress->counter;

            // for counters, field4 contains the main count requirement
            if (count >= criteria->raw.count)
                return true;
        }
        return false;
    }

    // Default case - need complete all or
    bool completed_all = true;
    for (AchievementCriteriaEntryList::const_iterator itr = cList->begin(); itr != cList->end(); ++itr)
    {
        AchievementCriteriaEntry const* criteria = *itr;

        bool completed = IsCompletedCriteria(criteria, entry);

        // found an uncompleted criteria, but DONT return false yet - there might be a completed criteria with ACHIEVEMENT_CRITERIA_COMPLETE_FLAG_ALL
        if (completed)
            ++count;
        else
            completed_all = false;

        // completed as have req. count of completed criterias
        if (achievementForTestCount > 0 && achievementForTestCount <= count)
            return true;
    }

    // all criterias completed requirement
    if (completed_all && achievementForTestCount == 0)
        return true;

    return false;
}

void AchievementMgr::SetCriteriaProgress(AchievementCriteriaEntry const* criteria, AchievementEntry const* achievement, uint32 changeValue, ProgressType ptype)
{
    DETAIL_FILTER_LOG(LOG_FILTER_ACHIEVEMENT_UPDATES, "AchievementMgr::SetCriteriaProgress(%u, %u) for (GUID:%u)", criteria->ID, changeValue, m_player->GetGUIDLow());

    uint32 max_value = GetCriteriaProgressMaxCounter(criteria, achievement);

    // change value must be in allowed value range for SET/HIGHEST directly
    if (changeValue > max_value)
        changeValue = max_value;

    CriteriaProgress *progress = NULL;
    uint32 old_value = 0;
    uint32 newValue = 0;

    CriteriaProgressMap::iterator iter = m_criteriaProgress.find(criteria->ID);
    if (iter == m_criteriaProgress.end())
    {
        // not create record for 0 counter
        if (changeValue == 0)
            return;

        // not start manually started timed achievements
        if (criteria->IsExplicitlyStartedTimedCriteria())
            return;

        progress = &m_criteriaProgress[criteria->ID];

        progress->date = time(NULL);
        progress->timedCriteriaFailed = false;

        // timed criterias are added to fail-timer map, and send the starting with counter=0
        if (criteria->timeLimit)
        {
            m_criteriaFailTimes[criteria->ID] = time_t(progress->date + criteria->timeLimit);
            progress->counter = 0;
            SendCriteriaUpdate(criteria->ID, progress);
        }

        newValue = changeValue;
    }
    else
    {
        progress = &iter->second;

        old_value = progress->counter;
        switch(ptype)
        {
            case PROGRESS_SET:
                newValue = changeValue;
                break;
            case PROGRESS_ACCUMULATE:
            {
                // avoid overflow
                newValue = max_value - progress->counter > changeValue ? progress->counter + changeValue : max_value;
                break;
            }
            case PROGRESS_HIGHEST:
                newValue = progress->counter < changeValue ? changeValue : progress->counter;
                break;
        }

        // not update (not mark as changed) if counter will have same value
        if (progress->counter == newValue)
            return;
    }

    progress->counter = newValue;
    progress->changed = true;

    // update client side value
    SendCriteriaUpdate(criteria->ID, progress);

    // update dependent achievements state at criteria complete
    if (old_value < progress->counter)
    {
        if (IsCompletedCriteria(criteria, achievement))
            CompletedCriteriaFor(achievement);

        // check again the completeness for SUMM and REQ COUNT achievements,
        // as they don't depend on the completed criteria but on the sum of the progress of each individual criteria
        if (achievement->flags & ACHIEVEMENT_FLAG_SUMM)
        {
            if (IsCompletedAchievement(achievement))
                CompletedAchievement(achievement);
        }

        if (AchievementEntryList const* achRefList = sAchievementMgr.GetAchievementByReferencedId(achievement->ID))
        {
            for (AchievementEntryList::const_iterator itr = achRefList->begin(); itr != achRefList->end(); ++itr)
                if (IsCompletedAchievement(*itr))
                    CompletedAchievement(*itr);
        }
    }
    // update dependent achievements state at criteria incomplete
    else if (old_value > progress->counter)
    {
        if (progress->counter < max_value)
        {
            WorldPacket data(SMSG_CRITERIA_DELETED,4);
            data << uint32(criteria->ID);
            m_player->SendDirectMessage(&data);
        }

        if (HasAchievement(achievement->ID))
            if (!IsCompletedAchievement(achievement))
                IncompletedAchievement(achievement);

        if (AchievementEntryList const* achRefList = sAchievementMgr.GetAchievementByReferencedId(achievement->ID))
            for (AchievementEntryList::const_iterator itr = achRefList->begin(); itr != achRefList->end(); ++itr)
                if (HasAchievement((*itr)->ID))
                    if (!IsCompletedAchievement(*itr))
                        IncompletedAchievement(*itr);
    }
}

void AchievementMgr::CompletedAchievement(AchievementEntry const* achievement)
{
    DETAIL_LOG("AchievementMgr::CompletedAchievement(%u)", achievement->ID);
    if(achievement->flags & ACHIEVEMENT_FLAG_COUNTER || m_completedAchievements.find(achievement->ID)!=m_completedAchievements.end())
        return;

    SendAchievementEarned(achievement);
    CompletedAchievementData& ca =  m_completedAchievements[achievement->ID];
    ca.date = time(NULL);
    ca.changed = true;

    // don't insert for ACHIEVEMENT_FLAG_REALM_FIRST_KILL since otherwise only the first group member would reach that achievement
    // TODO: where do set this instead?
    if(!(achievement->flags & ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
        sAchievementMgr.SetRealmCompleted(achievement);

    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT);

    // reward items and titles if any
    AchievementReward const* reward = sAchievementMgr.GetAchievementReward(achievement, GetPlayer()->getGender());

    // no rewards
    if(!reward)
        return;

    // titles
    if(uint32 titleId = reward->titleId[GetPlayer()->GetTeam() == HORDE ? 1 : 0])
    {
        if(CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
            GetPlayer()->SetTitle(titleEntry);
    }

    // mail
    if(reward->sender)
    {
        Item* item = reward->itemId ? Item::CreateItem(reward->itemId, 1, GetPlayer ()) : NULL;

        int loc_idx = GetPlayer()->GetSession()->GetSessionDbLocaleIndex();

        // subject and text
        std::string subject = reward->subject;
        std::string text = reward->text;
        if ( loc_idx >= 0 )
        {
            if(AchievementRewardLocale const* loc = sAchievementMgr.GetAchievementRewardLocale(achievement, GetPlayer()->getGender()))
            {
                if (loc->subject.size() > size_t(loc_idx) && !loc->subject[loc_idx].empty())
                    subject = loc->subject[loc_idx];
                if (loc->text.size() > size_t(loc_idx) && !loc->text[loc_idx].empty())
                    text = loc->text[loc_idx];
            }
        }

        MailDraft draft(subject, text);

        if(item)
        {
            // save new item before send
            item->SaveToDB();                               // save for prevent lost at next mail load, if send fail then item will deleted

            // item
            draft.AddItem(item);
        }

        draft.SendMailTo(GetPlayer(), MailSender(MAIL_CREATURE, reward->sender));
    }
}

void AchievementMgr::IncompletedAchievement(AchievementEntry const* achievement)
{
    DETAIL_LOG("AchievementMgr::IncompletedAchievement(%u)", achievement->ID);
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return;

    CompletedAchievementMap::iterator itr = m_completedAchievements.find(achievement->ID);
    if (itr == m_completedAchievements.end())
        return;

    WorldPacket data(SMSG_ACHIEVEMENT_DELETED,4);
    data << uint32(achievement->ID);
    m_player->SendDirectMessage(&data);

    if (!itr->second.changed)                               // complete state saved
        CharacterDatabase.PExecute("DELETE FROM character_achievement WHERE guid = %u AND achievement = %u",
            GetPlayer()->GetGUIDLow(), achievement->ID);

    m_completedAchievements.erase(achievement->ID);

    // reward items and titles if any
    AchievementReward const* reward = sAchievementMgr.GetAchievementReward(achievement, GetPlayer()->getGender());

    // no rewards
    if(!reward)
        return;

    // titles
    if(uint32 titleId = reward->titleId[GetPlayer()->GetTeam() == HORDE ? 0 : 1])
    {
        if(CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
            GetPlayer()->SetTitle(titleEntry, true);
    }

    // items impossible remove in clear way...
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
    data << GetPlayer()->GetPackGUID();
    BuildAllDataPacket(&data);
    player->GetSession()->SendPacket(&data);
}

/**
 * used by both SMSG_ALL_ACHIEVEMENT_DATA  and SMSG_RESPOND_INSPECT_ACHIEVEMENT
 */
void AchievementMgr::BuildAllDataPacket(WorldPacket *data)
{
    for(CompletedAchievementMap::const_iterator iter = m_completedAchievements.begin(); iter!=m_completedAchievements.end(); ++iter)
    {
        *data << uint32(iter->first);
        *data << uint32(secsToTimeBitFields(iter->second.date));
    }
    *data << int32(-1);

    time_t now = time(NULL);
    for(CriteriaProgressMap::const_iterator iter = m_criteriaProgress.begin(); iter!=m_criteriaProgress.end(); ++iter)
    {
        *data << uint32(iter->first);
        data->appendPackGUID(iter->second.counter);
        *data << GetPlayer()->GetPackGUID();
        *data << uint32(iter->second.timedCriteriaFailed ? 1 : 0);
        *data << uint32(secsToTimeBitFields(now));
        *data << uint32(now - iter->second.date);
        *data << uint32(now - iter->second.date);
    }

    *data << int32(-1);
}

//==========================================================
AchievementCriteriaEntryList const& AchievementGlobalMgr::GetAchievementCriteriaByType(AchievementCriteriaTypes type)
{
    return m_AchievementCriteriasByType[type];
}

void AchievementGlobalMgr::LoadAchievementCriteriaList()
{
    if (sAchievementCriteriaStore.GetNumRows()==0)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 achievement criteria.");
        return;
    }

    BarGoLink bar(sAchievementCriteriaStore.GetNumRows());
    for (uint32 entryId = 0; entryId < sAchievementCriteriaStore.GetNumRows(); ++entryId)
    {
        bar.step();

        AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(entryId);
        if (!criteria)
            continue;

        MANGOS_ASSERT(criteria->requiredType < ACHIEVEMENT_CRITERIA_TYPE_TOTAL && "Not updated ACHIEVEMENT_CRITERIA_TYPE_TOTAL?");

        // check if referredAchievement exists!
        AchievementEntry const* achiev = sAchievementStore.LookupEntry(criteria->referredAchievement);
        if (!achiev)
        {
            sLog.outDetail("Removed achievement-criteria %u, because referred achievement does not exist", entryId);
            sAchievementCriteriaStore.EraseEntry(entryId);
            continue;
        }

        m_AchievementCriteriasByType[criteria->requiredType].push_back(criteria);
        m_AchievementCriteriaListByAchievement[criteria->referredAchievement].push_back(criteria);
    }

    sLog.outString();
    sLog.outString(">> Loaded %lu achievement criteria.",(unsigned long)m_AchievementCriteriasByType->size());
}

void AchievementGlobalMgr::LoadAchievementReferenceList()
{
    if(sAchievementStore.GetNumRows()==0)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 achievement references.");
        return;
    }

    uint32 count = 0;
    BarGoLink bar(sAchievementStore.GetNumRows());
    for (uint32 entryId = 0; entryId < sAchievementStore.GetNumRows(); ++entryId)
    {
        bar.step();

        AchievementEntry const* achievement = sAchievementStore.LookupEntry(entryId);
        if (!achievement || !achievement->refAchievement)
            continue;

        // Check refAchievement exists
        AchievementEntry const* refAchiev = sAchievementStore.LookupEntry(achievement->refAchievement);
        if (!refAchiev)
        {
            sLog.outDetail("Removed achieviement %u, because referred achievement does not exist", entryId);
            sAchievementStore.EraseEntry(entryId);
            continue;
        }

        m_AchievementListByReferencedId[achievement->refAchievement].push_back(achievement);
        ++count;
    }

    sLog.outString();
    sLog.outString(">> Loaded %u achievement references.", count);
}

void AchievementGlobalMgr::LoadAchievementCriteriaRequirements()
{
    m_criteriaRequirementMap.clear();                       // need for reload case

    QueryResult *result = WorldDatabase.Query("SELECT criteria_id, type, value1, value2 FROM achievement_criteria_requirement");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded 0 additional achievement criteria data. DB table `achievement_criteria_requirement` is empty.");
        return;
    }

    uint32 count = 0;
    uint32 disabled_count = 0;
    BarGoLink bar(result->GetRowCount());
    do
    {
        bar.step();
        Field *fields = result->Fetch();
        uint32 criteria_id = fields[0].GetUInt32();

        AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(criteria_id);

        if (!criteria)
        {
            sLog.outErrorDb( "Table `achievement_criteria_requirement`.`criteria_id` %u does not exist, ignoring.", criteria_id);
            continue;
        }

        AchievementCriteriaRequirement data(fields[1].GetUInt32(),fields[2].GetUInt32(),fields[3].GetUInt32());

        if (!data.IsValid(criteria))
        {
            continue;
        }

        // this will allocate empty data set storage
        AchievementCriteriaRequirementSet& dataSet = m_criteriaRequirementMap[criteria_id];
        dataSet.SetCriteriaId(criteria_id);

        // counting disable criteria requirements
        if (data.requirementType == ACHIEVEMENT_CRITERIA_REQUIRE_DISABLED)
            ++disabled_count;

        // add real data only for not NONE requirements
        if (data.requirementType != ACHIEVEMENT_CRITERIA_REQUIRE_NONE)
            dataSet.Add(data);

        // counting requirements
        ++count;
    } while(result->NextRow());

    delete result;

    // post loading checks
    for (uint32 entryId = 0; entryId < sAchievementCriteriaStore.GetNumRows(); ++entryId)
    {
        AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(entryId);
        if(!criteria)
            continue;

        switch(criteria->requiredType)
        {
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
                if(!criteria->win_bg.additionalRequirement1_type && !criteria->win_bg.additionalRequirement2_type)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
                break;                                      // any cases
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            {
                AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->referredAchievement);
                // Checked in LoadAchievementCriteriaList

                // exist many achievements with this criteria, use at this moment hardcoded check to skil simple case
                switch(achievement->ID)
                {
                    case 31:
                    //case 1275: // these timed achievements are "started" on Quest Accept, and simple ended on quest-complete
                    //case 1276:
                    //case 1277:
                    case 1282:
                    case 1789:
                        break;
                    default:
                        continue;
                }
            }
            case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
                break;                                      // any cases
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:      // any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA: // need skip generic cases
                if(criteria->win_rated_arena.flag!=ACHIEVEMENT_CRITERIA_CONDITION_NO_LOOSE)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM: // any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:        // need skip generic cases
                if(criteria->do_emote.count==0)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:// any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:        // skip statistics
                if(criteria->win_duel.duelCount==0)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:     // any cases
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:       // need skip generic cases
                if(criteria->loot_type.lootTypeCount!=1)
                    continue;
                break;
            default:                                        // type not use DB data, ignore
                continue;
        }

        if (!GetCriteriaRequirementSet(criteria))
            sLog.outErrorDb("Table `achievement_criteria_requirement` is missing expected data for `criteria_id` %u (type: %u) for achievement %u.", criteria->ID, criteria->requiredType, criteria->referredAchievement);
    }

    sLog.outString();
    sLog.outString(">> Loaded %u additional achievement criteria data (%u disabled).",count,disabled_count);
}

void AchievementGlobalMgr::LoadCompletedAchievements()
{
    QueryResult *result = CharacterDatabase.Query("SELECT achievement FROM character_achievement GROUP BY achievement");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded 0 realm completed achievements . DB table `character_achievement` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());
    do
    {
        bar.step();
        Field *fields = result->Fetch();

        uint32 achievement_id = fields[0].GetUInt32();
        if (!sAchievementStore.LookupEntry(achievement_id))
        {
            // we will remove nonexistent achievement for all characters
            sLog.outError("Nonexistent achievement %u data removed from table `character_achievement`.",achievement_id);
            CharacterDatabase.PExecute("DELETE FROM character_achievement WHERE achievement = %u",achievement_id);
            continue;
        }

        m_allCompletedAchievements.insert(achievement_id);
    } while(result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %lu realm completed achievements.",(unsigned long)m_allCompletedAchievements.size());
}

void AchievementGlobalMgr::LoadRewards()
{
    m_achievementRewards.clear();                           // need for reload case

    //                                                0      1       2        3        4     5       6        7
    QueryResult *result = WorldDatabase.Query("SELECT entry, gender, title_A, title_H, item, sender, subject, text FROM achievement_reward");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 achievement rewards. DB table `achievement_reward` is empty.");
        return;
    }

    uint32 count = 0;
    BarGoLink bar(result->GetRowCount());

    do
    {
        bar.step();

        Field *fields = result->Fetch();
        uint32 entry = fields[0].GetUInt32();
        if (!sAchievementStore.LookupEntry(entry))
        {
            sLog.outErrorDb( "Table `achievement_reward` has wrong achievement (Entry: %u), ignore", entry);
            continue;
        }

        AchievementReward reward;
        reward.gender     = Gender(fields[1].GetUInt8());
        reward.titleId[0] = fields[2].GetUInt32();
        reward.titleId[1] = fields[3].GetUInt32();
        reward.itemId     = fields[4].GetUInt32();
        reward.sender     = fields[5].GetUInt32();
        reward.subject    = fields[6].GetCppString();
        reward.text       = fields[7].GetCppString();

        if (reward.gender >= MAX_GENDER)
            sLog.outErrorDb( "Table `achievement_reward` (Entry: %u) has wrong gender %u.", entry, reward.gender);

        // GENDER_NONE must be single (so or already in and none must be attempt added new data or just adding and none in)
        // other duplicate cases prevented by DB primary key
        bool dup = false;
        AchievementRewardsMapBounds bounds = m_achievementRewards.equal_range(entry);
        for (AchievementRewardsMap::const_iterator iter = bounds.first; iter != bounds.second; ++iter)
        {
            if (iter->second.gender == GENDER_NONE || reward.gender == GENDER_NONE)
            {
                dup = true;
                sLog.outErrorDb( "Table `achievement_reward` must have single GENDER_NONE (%u) case (Entry: %u), ignore duplicate case", GENDER_NONE, entry);
                break;
            }
        }
        if (dup)
            continue;

        if ((reward.titleId[0]==0)!=(reward.titleId[1]==0))
            sLog.outErrorDb( "Table `achievement_reward` (Entry: %u) has title (A: %u H: %u) only for one from teams.", entry, reward.titleId[0], reward.titleId[1]);

        // must be title or mail at least
        if (!reward.titleId[0] && !reward.titleId[1] && !reward.sender)
        {
            sLog.outErrorDb( "Table `achievement_reward` (Entry: %u) not have title or item reward data, ignore.", entry);
            continue;
        }

        if (reward.titleId[0])
        {
            CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(reward.titleId[0]);
            if (!titleEntry)
            {
                sLog.outErrorDb( "Table `achievement_reward` (Entry: %u) has invalid title id (%u) in `title_A`, set to 0", entry, reward.titleId[0]);
                reward.titleId[0] = 0;
            }
        }

        if (reward.titleId[1])
        {
            CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(reward.titleId[1]);
            if (!titleEntry)
            {
                sLog.outErrorDb( "Table `achievement_reward` (Entry: %u) has invalid title id (%u) in `title_A`, set to 0", entry, reward.titleId[1]);
                reward.titleId[1] = 0;
            }
        }

        //check mail data before item for report including wrong item case
        if (reward.sender)
        {
            if (!ObjectMgr::GetCreatureTemplate(reward.sender))
            {
                sLog.outErrorDb( "Table `achievement_reward` (Entry: %u) has invalid creature entry %u as sender, mail reward skipped.", entry, reward.sender);
                reward.sender = 0;
            }
        }
        else
        {
            if (reward.itemId)
                sLog.outErrorDb( "Table `achievement_reward` (Entry: %u) not have sender data but have item reward, item will not rewarded", entry);

            if (!reward.subject.empty())
                sLog.outErrorDb( "Table `achievement_reward` (Entry: %u) not have sender data but have mail subject.", entry);

            if (!reward.text.empty())
                sLog.outErrorDb( "Table `achievement_reward` (Entry: %u) not have sender data but have mail text.", entry);
        }

        if (reward.itemId)
        {
            if (!ObjectMgr::GetItemPrototype(reward.itemId))
            {
                sLog.outErrorDb( "Table `achievement_reward` (Entry: %u) has invalid item id %u, reward mail will be without item.", entry, reward.itemId);
                reward.itemId = 0;
            }
        }

        m_achievementRewards.insert(AchievementRewardsMap::value_type(entry, reward));
        ++count;

    } while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString( ">> Loaded %u achievement rewards", count );
}

void AchievementGlobalMgr::LoadRewardLocales()
{
    m_achievementRewardLocales.clear();                       // need for reload case

    QueryResult *result = WorldDatabase.Query("SELECT entry,gender,subject_loc1,text_loc1,subject_loc2,text_loc2,subject_loc3,text_loc3,subject_loc4,text_loc4,subject_loc5,text_loc5,subject_loc6,text_loc6,subject_loc7,text_loc7,subject_loc8,text_loc8 FROM locales_achievement_reward");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded 0 achievement reward locale strings. DB table `locales_achievement_reward` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field *fields = result->Fetch();
        bar.step();

        uint32 entry = fields[0].GetUInt32();

        if (m_achievementRewards.find(entry)==m_achievementRewards.end())
        {
            sLog.outErrorDb( "Table `locales_achievement_reward` (Entry: %u) has locale strings for nonexistent achievement reward .", entry);
            continue;
        }

        AchievementRewardLocale data;

        data.gender = Gender(fields[1].GetUInt8());

        if (data.gender >= MAX_GENDER)
            sLog.outErrorDb( "Table `locales_achievement_reward` (Entry: %u) has wrong gender %u.", entry, data.gender);

        // GENDER_NONE must be single (so or already in and none must be attempt added new data or just adding and none in)
        // other duplicate cases prevented by DB primary key
        bool dup = false;
        AchievementRewardLocalesMapBounds bounds = m_achievementRewardLocales.equal_range(entry);
        for (AchievementRewardLocalesMap::const_iterator iter = bounds.first; iter != bounds.second; ++iter)
        {
            if (iter->second.gender == GENDER_NONE || data.gender == GENDER_NONE)
            {
                dup = true;
                sLog.outErrorDb( "Table `locales_achievement_reward` must have single GENDER_NONE (%u) case (Entry: %u), ignore duplicate case", GENDER_NONE, entry);
                break;
            }
        }
        if (dup)
            continue;

        for(int i = 1; i < MAX_LOCALE; ++i)
        {
            std::string str = fields[2+2*(i-1)].GetCppString();
            if(!str.empty())
            {
                int idx = sObjectMgr.GetOrNewIndexForLocale(LocaleConstant(i));
                if(idx >= 0)
                {
                    if(data.subject.size() <= size_t(idx))
                        data.subject.resize(idx+1);

                    data.subject[idx] = str;
                }
            }
            str = fields[2+2*(i-1)+1].GetCppString();
            if(!str.empty())
            {
                int idx = sObjectMgr.GetOrNewIndexForLocale(LocaleConstant(i));
                if(idx >= 0)
                {
                    if(data.text.size() <= size_t(idx))
                        data.text.resize(idx+1);

                    data.text[idx] = str;
                }
            }
        }

        m_achievementRewardLocales.insert(AchievementRewardLocalesMap::value_type(entry, data));

    } while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString( ">> Loaded %lu achievement reward locale strings", (unsigned long)m_achievementRewardLocales.size() );
}
