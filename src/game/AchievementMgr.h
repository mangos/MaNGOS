/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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
#ifndef __MANGOS_ACHIEVEMENTMGR_H
#define __MANGOS_ACHIEVEMENTMGR_H

#include "Common.h"
#include "Policies/Singleton.h"
#include "Database/DatabaseEnv.h"
#include "DBCEnums.h"
#include "DBCStores.h"
#include "SharedDefines.h"

#include <map>
#include <string>

typedef std::list<AchievementCriteriaEntry const*> AchievementCriteriaEntryList;
typedef std::list<AchievementEntry const*>         AchievementEntryList;

typedef std::map<uint32,AchievementCriteriaEntryList> AchievementCriteriaListByAchievement;
typedef std::map<uint32,AchievementEntryList>         AchievementListByReferencedId;

struct CriteriaProgress
{
    uint32 counter;
    time_t date;
    bool changed;
};

enum AchievementCriteriaRequirementType
{                                                           // value1         value2        comment
    ACHIEVEMENT_CRITERIA_REQUIRE_NONE                = 0,   // 0              0
    ACHIEVEMENT_CRITERIA_REQUIRE_T_CREATURE          = 1,   // creature_id    0
    ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_CLASS_RACE = 2,   // class_id       race_id
    ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_LESS_HEALTH= 3,   // health_percent 0
    ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_DEAD       = 4,   // own_team       0             not corpse (not released body), own_team==false if enemy team expected
    ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA              = 5,   // spell_id       effect_idx
    ACHIEVEMENT_CRITERIA_REQUIRE_S_AREA              = 6,   // area id        0
    ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA              = 7,   // spell_id       effect_idx
    ACHIEVEMENT_CRITERIA_REQUIRE_VALUE               = 8,   // minvalue                     value provided with achievement update must be not less that limit
    ACHIEVEMENT_CRITERIA_REQUIRE_T_LEVEL             = 9,   // minlevel                     minlevel of target
    ACHIEVEMENT_CRITERIA_REQUIRE_T_GENDER            = 10,  // gender                       0=male; 1=female
    ACHIEVEMENT_CRITERIA_REQUIRE_DISABLED            = 11,  //                              used to prevent achievement creteria complete if not all requirement implemented and listed in table
    ACHIEVEMENT_CRITERIA_REQUIRE_MAP_DIFFICULTY      = 12,  // difficulty                   normal/heroic difficulty for current event map
    ACHIEVEMENT_CRITERIA_REQUIRE_MAP_PLAYER_COUNT    = 13,  // count                        "with less than %u people in the zone"
    ACHIEVEMENT_CRITERIA_REQUIRE_T_TEAM              = 14,  // team                         HORDE(67), ALLIANCE(469)
    ACHIEVEMENT_CRITERIA_REQUIRE_S_DRUNK             = 15,  // drunken_state  0             (enum DrunkenState) of player
    ACHIEVEMENT_CRITERIA_REQUIRE_HOLIDAY             = 16,  // holiday_id     0             event in holiday time
    ACHIEVEMENT_CRITERIA_REQUIRE_BG_LOSS_TEAM_SCORE  = 17,  // min_score      max_score     player's team win bg and opposition team have team score in range
    ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT     = 18,  // 0              0             maker instance script call for check current criteria requirements fit
    ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPPED_ITEM_LVL = 19,  // item_level     item_quality  fir equipped item in slot `misc1` to item level and quality
};

#define MAX_ACHIEVEMENT_CRITERIA_REQUIREMENT_TYPE      20   // maximum value in AchievementCriteriaRequirementType enum

class Player;
class Unit;

struct AchievementCriteriaRequirement
{
    AchievementCriteriaRequirementType requirementType;
    union
    {
        // ACHIEVEMENT_CRITERIA_REQUIRE_NONE              = 0 (no data)
        // ACHIEVEMENT_CRITERIA_REQUIRE_T_CREATURE        = 1
        struct
        {
            uint32 id;
        } creature;
        // ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_CLASS_RACE = 2
        struct
        {
            uint32 class_id;
            uint32 race_id;
        } classRace;
        // ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_LESS_HEALTH = 3
        struct
        {
            uint32 percent;
        } health;
        // ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_DEAD     = 4
        struct
        {
            uint32 own_team_flag;
        } player_dead;
        // ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA            = 5
        // ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA            = 7
        struct
        {
            uint32 spell_id;
            uint32 effect_idx;
        } aura;
        // ACHIEVEMENT_CRITERIA_REQUIRE_S_AREA            = 6
        struct
        {
            uint32 id;
        } area;
        // ACHIEVEMENT_CRITERIA_REQUIRE_VALUE             = 8
        struct
        {
            uint32 minvalue;
        } value;
        // ACHIEVEMENT_CRITERIA_REQUIRE_T_LEVEL           = 9
        struct
        {
            uint32 minlevel;
        } level;
        // ACHIEVEMENT_CRITERIA_REQUIRE_T_GENDER          = 10
        struct
        {
            uint32 gender;
        } gender;
        // ACHIEVEMENT_CRITERIA_REQUIRE_DISABLED          = 11 (no data)
        // ACHIEVEMENT_CRITERIA_REQUIRE_MAP_DIFFICULTY    = 12
        struct
        {
            uint32 difficulty;
        } difficulty;
        // ACHIEVEMENT_CRITERIA_REQUIRE_MAP_PLAYER_COUNT  = 13
        struct
        {
            uint32 maxcount;
        } map_players;
        // ACHIEVEMENT_CRITERIA_REQUIRE_T_TEAM            = 14
        struct
        {
            uint32 team;
        } team;
        // ACHIEVEMENT_CRITERIA_REQUIRE_S_DRUNK           = 15
        struct
        {
            uint32 state;
        } drunk;
        // ACHIEVEMENT_CRITERIA_REQUIRE_HOLIDAY           = 16
        struct
        {
            uint32 id;
        } holiday;
        // ACHIEVEMENT_CRITERIA_REQUIRE_BG_LOSS_TEAM_SCORE= 17
        struct
        {
            uint32 min_score;
            uint32 max_score;
        } bg_loss_team_score;
        // ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT   = 18 (no data)
        // ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPPED_ITEM_LVL=19
        struct
        {
            uint32 item_level;
            uint32 item_quality;
        } equipped_item;
        // ...
        struct
        {
            uint32 value1;
            uint32 value2;
        } raw;
    };

    AchievementCriteriaRequirement() : requirementType(ACHIEVEMENT_CRITERIA_REQUIRE_NONE)
    {
        raw.value1 = 0;
        raw.value2 = 0;
    }

    AchievementCriteriaRequirement(uint32 reqType, uint32 _value1, uint32 _value2)
        : requirementType(AchievementCriteriaRequirementType(reqType))
    {
        raw.value1 = _value1;
        raw.value2 = _value2;
    }

    bool IsValid(AchievementCriteriaEntry const* criteria);
    bool Meets(uint32 criteria_id, Player const* source, Unit const* target, uint32 miscvalue1 = 0) const;
};

struct AchievementCriteriaRequirementSet
{
        AchievementCriteriaRequirementSet() : criteria_id(0) {}
        typedef std::vector<AchievementCriteriaRequirement> Storage;
        void Add(AchievementCriteriaRequirement const& data) { storage.push_back(data); }
        bool Meets(Player const* source, Unit const* target, uint32 miscvalue = 0) const;
        void SetCriteriaId(uint32 id) {criteria_id = id;}
    private:
        uint32 criteria_id;
        Storage storage;
};

typedef std::map<uint32,AchievementCriteriaRequirementSet> AchievementCriteriaRequirementMap;

struct AchievementReward
{
    Gender gender;
    uint32 titleId[2];
    uint32 itemId;
    uint32 sender;
    std::string subject;
    std::string text;
};

typedef std::multimap<uint32,AchievementReward> AchievementRewards;

struct AchievementRewardLocale
{
    Gender gender;
    std::vector<std::string> subject;
    std::vector<std::string> text;
};

typedef std::multimap<uint32,AchievementRewardLocale> AchievementRewardLocales;


struct CompletedAchievementData
{
    time_t date;
    bool changed;
};

typedef UNORDERED_MAP<uint32, CriteriaProgress> CriteriaProgressMap;
typedef UNORDERED_MAP<uint32, CompletedAchievementData> CompletedAchievementMap;

class Unit;
class Player;
class WorldPacket;

class AchievementMgr
{
    public:
        AchievementMgr(Player* pl);
        ~AchievementMgr();

        void Reset();
        static void DeleteFromDB(uint32 lowguid);
        void LoadFromDB(QueryResult *achievementResult, QueryResult *criteriaResult);
        void SaveToDB();
        void ResetAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1=0, uint32 miscvalue2=0);
        void StartTimedAchievementCriteria(AchievementCriteriaTypes type, uint32 timedRequirementId, time_t startTime = 0);
        void UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1=0, uint32 miscvalue2=0, Unit *unit=NULL, uint32 time=0);
        void CheckAllAchievementCriteria();
        void SendAllAchievementData();
        void SendRespondInspectAchievements(Player* player);

        Player* GetPlayer() const { return m_player;}

        CompletedAchievementData const* GetCompleteData(uint32 achievement_id) const
        {
            CompletedAchievementMap::const_iterator itr = m_completedAchievements.find(achievement_id);
            return itr != m_completedAchievements.end() ? &itr->second : NULL;
        }

        bool HasAchievement(uint32 achievement_id) const { return GetCompleteData(achievement_id) != NULL; }
        CompletedAchievementMap const& GetCompletedAchievements() const { return m_completedAchievements; }
        bool IsCompletedCriteria(AchievementCriteriaEntry const* criteria, AchievementEntry const* achievement) const;

        uint32 GetCriteriaProgressCounter(AchievementCriteriaEntry const* entry) const
        {
            CriteriaProgressMap::const_iterator iter = m_criteriaProgress.find(entry->ID);
            return iter != m_criteriaProgress.end() ? iter->second.counter : 0;
        }

        static uint32 GetCriteriaProgressMaxCounter(AchievementCriteriaEntry const* entry);

        // Use PROGRESS_SET only for reset/downgrade criteria progress
        enum ProgressType { PROGRESS_SET, PROGRESS_ACCUMULATE, PROGRESS_HIGHEST };
        void SetCriteriaProgress(AchievementCriteriaEntry const* criteria, AchievementEntry const* achievement, uint32 changeValue, ProgressType ptype);

    private:
        void SendAchievementEarned(AchievementEntry const* achievement);
        void SendCriteriaUpdate(uint32 id, CriteriaProgress const* progress);
        void CompletedCriteriaFor(AchievementEntry const* achievement);
        void CompletedAchievement(AchievementEntry const* entry);
        void IncompletedAchievement(AchievementEntry const* entry);
        bool IsCompletedAchievement(AchievementEntry const* entry);
        void CompleteAchievementsWithRefs(AchievementEntry const* entry);
        void BuildAllDataPacket(WorldPacket *data);

        Player* m_player;
        CriteriaProgressMap m_criteriaProgress;
        CompletedAchievementMap m_completedAchievements;
};

class AchievementGlobalMgr
{
    public:
        AchievementCriteriaEntryList const& GetAchievementCriteriaByType(AchievementCriteriaTypes type);
        AchievementCriteriaEntryList const* GetAchievementCriteriaByAchievement(uint32 id)
        {
            AchievementCriteriaListByAchievement::const_iterator itr = m_AchievementCriteriaListByAchievement.find(id);
            return itr != m_AchievementCriteriaListByAchievement.end() ? &itr->second : NULL;
        }

        AchievementEntryList const* GetAchievementByReferencedId(uint32 id) const
        {
            AchievementListByReferencedId::const_iterator itr = m_AchievementListByReferencedId.find(id);
            return itr != m_AchievementListByReferencedId.end() ? &itr->second : NULL;
        }

        AchievementReward const* GetAchievementReward(AchievementEntry const* achievement, uint8 gender) const
        {
            AchievementRewards::const_iterator iter_low = m_achievementRewards.lower_bound(achievement->ID);
            AchievementRewards::const_iterator iter_up  = m_achievementRewards.upper_bound(achievement->ID);
            for (AchievementRewards::const_iterator iter = iter_low; iter != iter_up; ++iter)
                if(iter->second.gender == GENDER_NONE || uint8(iter->second.gender) == gender)
                    return &iter->second;

            return NULL;
        }

        AchievementRewardLocale const* GetAchievementRewardLocale(AchievementEntry const* achievement, uint8 gender) const
        {
            AchievementRewardLocales::const_iterator iter_low = m_achievementRewardLocales.lower_bound(achievement->ID);
            AchievementRewardLocales::const_iterator iter_up  = m_achievementRewardLocales.upper_bound(achievement->ID);
            for (AchievementRewardLocales::const_iterator iter = iter_low; iter != iter_up; ++iter)
                if(iter->second.gender == GENDER_NONE || uint8(iter->second.gender) == gender)
                    return &iter->second;

            return NULL;
        }

        AchievementCriteriaRequirementSet const* GetCriteriaRequirementSet(AchievementCriteriaEntry const *achievementCriteria)
        {
            AchievementCriteriaRequirementMap::const_iterator iter = m_criteriaRequirementMap.find(achievementCriteria->ID);
            return iter!=m_criteriaRequirementMap.end() ? &iter->second : NULL;
        }

        bool IsRealmCompleted(AchievementEntry const* achievement) const
        {
            return m_allCompletedAchievements.find(achievement->ID) != m_allCompletedAchievements.end();
        }

        void SetRealmCompleted(AchievementEntry const* achievement)
        {
            m_allCompletedAchievements.insert(achievement->ID);
        }

        void LoadAchievementCriteriaList();
        void LoadAchievementCriteriaRequirements();
        void LoadAchievementReferenceList();
        void LoadCompletedAchievements();
        void LoadRewards();
        void LoadRewardLocales();
    private:
        AchievementCriteriaRequirementMap m_criteriaRequirementMap;

        // store achievement criterias by type to speed up lookup
        AchievementCriteriaEntryList m_AchievementCriteriasByType[ACHIEVEMENT_CRITERIA_TYPE_TOTAL];
        // store achievement criterias by achievement to speed up lookup
        AchievementCriteriaListByAchievement m_AchievementCriteriaListByAchievement;
        // store achievements by referenced achievement id to speed up lookup
        AchievementListByReferencedId m_AchievementListByReferencedId;

        typedef std::set<uint32> AllCompletedAchievements;
        AllCompletedAchievements m_allCompletedAchievements;

        AchievementRewards m_achievementRewards;
        AchievementRewardLocales m_achievementRewardLocales;
};

#define sAchievementMgr MaNGOS::Singleton<AchievementGlobalMgr>::Instance()

#endif
