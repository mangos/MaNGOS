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

#ifndef MANGOS_DBCSTRUCTURE_H
#define MANGOS_DBCSTRUCTURE_H

#include "Common.h"
#include "DBCEnums.h"
#include "Path.h"
#include "Platform/Define.h"

#include <map>
#include <set>
#include <vector>

// Structures using to access raw DBC data and required packing to portability

// GCC have alternative #pragma pack(N) syntax and old gcc version not support pack(push,N), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

struct AchievementEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    factionFlag;                                  // 1        m_faction -1=all, 0=horde, 1=alliance
    uint32    mapID;                                        // 2        m_instance_id -1=none
    //uint32 parentAchievement;                             // 3        m_supercedes its Achievement parent (can`t start while parent uncomplete, use its Criteria if don`t have own, use its progress on begin)
    char *name[16];                                         // 4-19     m_title_lang
    //uint32 name_flags;                                    // 20 string flags
    //char *description[16];                                // 21-36    m_description_lang
    //uint32 desc_flags;                                    // 37 string flags
    uint32    categoryId;                                   // 38       m_category
    uint32    points;                                       // 39       m_points
    //uint32 OrderInCategory;                               // 40       m_ui_order
    uint32    flags;                                        // 41       m_flags
    //uint32    icon;                                       // 42       m_iconID
    //char *titleReward[16];                                // 43-58    m_reward_lang
    //uint32 titleReward_flags;                             // 59 string flags
    uint32 count;                                           // 60       m_minimum_criteria - need this count of completed criterias (own or referenced achievement criterias)
    uint32 refAchievement;                                  // 61       m_shares_criteria - referenced achievement (counting of all completed criterias)
};

struct AchievementCategoryEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    parentCategory;                               // 1        m_parent -1 for main category
    //char *name[16];                                       // 2-17     m_name_lang
    //uint32 name_flags;                                    // 18 string flags
    //uint32    sortOrder;                                  // 19       m_ui_order
};

struct AchievementCriteriaEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  referredAchievement;                            // 1        m_achievement_id
    uint32  requiredType;                                   // 2        m_type
    union
    {
        // ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE          = 0
        // TODO: also used for player deaths..
        struct
        {
            uint32  creatureID;                             // 3
            uint32  creatureCount;                          // 4
        } kill_creature;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_BG                 = 1
        struct
        {
            uint32  bgMapID;                                // 3
            uint32  winCount;                               // 4
            uint32  additionalRequirement1_type;            // 5
            uint32  additionalRequirement1_value;           // 6
            uint32  additionalRequirement2_type;            // 7
            uint32  additionalRequirement2_value;           // 8
        } win_bg;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL            = 5
        struct
        {
            uint32  unused;                                 // 3
            uint32  level;                                  // 4
        } reach_level;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL      = 7
        struct
        {
            uint32  skillID;                                // 3
            uint32  skillLevel;                             // 4
        } reach_skill_level;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT   = 8
        struct
        {
            uint32  linkedAchievement;                      // 3
        } complete_achievement;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT   = 9
        struct
        {
            uint32  unused;                                 // 3
            uint32  totalQuestCount;                        // 4
        } complete_quest_count;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY = 10
        struct
        {
            uint32  unused;                                 // 3
            uint32  numberOfDays;                           // 4
        } complete_daily_quest_daily;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE = 11
        struct
        {
            uint32  zoneID;                                 // 3
            uint32  questCount;                             // 4
        } complete_quests_in_zone;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST   = 14
        struct
        {
            uint32  unused;                                 // 3
            uint32  questCount;                             // 4
        } complete_daily_quest;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND  = 15
        struct
        {
            uint32  mapID;                                  // 3
        } complete_battleground;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP           = 16
        struct
        {
            uint32  mapID;                                  // 3
        } death_at_map;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON       = 18
        struct
        {
            uint32  manLimit;                               // 3
        } death_in_dungeon;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID          = 19
        struct
        {
            uint32  groupSize;                              // 3 can be 5, 10 or 25
        } complete_raid;

        // ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE     = 20
        struct
        {
            uint32  creatureEntry;                          // 3
        } killed_by_creature;

        // ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING     = 24
        struct
        {
            uint32  unused;                                 // 3
            uint32  fallHeight;                             // 4
        } fall_without_dying;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM            = 26
        struct
        {
            uint32 type;                                    // 3, see enum EnviromentalDamage
        } death_from;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST         = 27
        struct
        {
            uint32  questID;                                // 3
            uint32  questCount;                             // 4
        } complete_quest;

        // ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET        = 28
        // ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2       = 69
        struct
        {
            uint32  spellID;                                // 3
            uint32  spellCount;                             // 4
        } be_spell_target;

        // ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL             = 29
        // ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2            = 110
        struct
        {
            uint32  spellID;                                // 3
            uint32  castCount;                              // 4
        } cast_spell;

        // ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA = 31
        struct
        {
            uint32  areaID;                                 // 3 Reference to AreaTable.dbc
            uint32  killCount;                              // 4
        } honorable_kill_at_area;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA              = 32
        struct
        {
            uint32  mapID;                                  // 3 Reference to Map.dbc
        } win_arena;

        // ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA             = 33
        struct
        {
            uint32  mapID;                                  // 3 Reference to Map.dbc
        } play_arena;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL            = 34
        struct
        {
            uint32  spellID;                                // 3 Reference to Map.dbc
        } learn_spell;

        // ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM               = 36
        struct
        {
            uint32  itemID;                                 // 3
            uint32  itemCount;                              // 4
        } own_item;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA        = 37
        struct
        {
            uint32  unused;                                 // 3
            uint32  count;                                  // 4
            uint32  flag;                                   // 5 4=in a row
        } win_rated_arena;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING    = 38
        struct
        {
            uint32  teamtype;                               // 3 {2,3,5}
        } highest_team_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING= 39
        struct
        {
            uint32  teamtype;                               // 3 {2,3,5}
            uint32  teamrating;                             // 4
        } highest_personal_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL      = 40
        struct
        {
            uint32  skillID;                                // 3
            uint32  skillLevel;                             // 4 apprentice=1, journeyman=2, expert=3, artisan=4, master=5, grand master=6
        } learn_skill_level;

        // ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM               = 41
        struct
        {
            uint32  itemID;                                 // 3
            uint32  itemCount;                              // 4
        } use_item;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM              = 42
        struct
        {
            uint32  itemID;                                 // 3
            uint32  itemCount;                              // 4
        } loot_item;

        // ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA           = 43
        struct
        {
            // TODO: This rank is _NOT_ the index from AreaTable.dbc
            uint32  areaReference;                          // 3
        } explore_area;

        // ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK               = 44
        struct
        {
            // TODO: This rank is _NOT_ the index from CharTitles.dbc
            uint32  rank;                                   // 3
        } own_rank;

        // ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT          = 45
        struct
        {
            uint32  unused;                                 // 3
            uint32  numberOfSlots;                          // 4
        } buy_bank_slot;

        // ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION        = 46
        struct
        {
            uint32  factionID;                              // 3
            uint32  reputationAmount;                       // 4 Total reputation amount, so 42000 = exalted
        } gain_reputation;

        // ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION= 47
        struct
        {
            uint32  unused;                                 // 3
            uint32  numberOfExaltedFactions;                // 4
        } gain_exalted_reputation;

        // ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP      = 48
        struct
        {
            uint32 unused;                                  // 3
            uint32 numberOfVisits;                          // 4
        } visit_barber;

        // ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM        = 49
        // TODO: where is the required itemlevel stored?
        struct
        {
            uint32  itemSlot;                               // 3
            uint32  count;                                  // 4
        } equip_epic_item;

        // ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT      = 50
        struct
        {
            uint32  rollValue;                              // 3
            uint32  count;                                  // 4
        } roll_need_on_loot;
        // ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT      = 51
        struct
        {
            uint32  rollValue;                              // 3
            uint32  count;                                  // 4
        } roll_greed_on_loot;

        // ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS               = 52
        struct
        {
            uint32  classID;                                // 3
            uint32  count;                                  // 4
        } hk_class;

        // ACHIEVEMENT_CRITERIA_TYPE_HK_RACE                = 53
        struct
        {
            uint32  raceID;                                 // 3
            uint32  count;                                  // 4
        } hk_race;

        // ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE               = 54
        // TODO: where is the information about the target stored?
        struct
        {
            uint32  emoteID;                                // 3 enum TextEmotes
            uint32  count;                                  // 4 count of emotes, always required special target or requirements
        } do_emote;
        // ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE            = 13
        // ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE           = 55
        // ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS      = 56
        struct
        {
            uint32  unused;                                 // 3
            uint32  count;                                  // 4
            uint32  flag;                                   // 5 =3 for battleground healing
            uint32  mapid;                                  // 6
        } healing_done;

        // ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM             = 57
        struct
        {
            uint32  itemID;                                 // 3
            uint32  count;                                  // 4
        } equip_item;

        // ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD= 62
        struct
        {
            uint32  unused;                                 // 3
            uint32  goldInCopper;                           // 4
        } quest_reward_money;


        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY             = 67
        struct
        {
            uint32  unused;                                 // 3
            uint32  goldInCopper;                           // 4
        } loot_money;

        // ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT         = 68
        struct
        {
            uint32  goEntry;                                // 3
            uint32  useCount;                               // 4
        } use_gameobject;

        // ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL       = 70
        // TODO: are those special criteria stored in the dbc or do we have to add another sql table?
        struct
        {
            uint32  unused;                                 // 3
            uint32  killCount;                              // 4
        } special_pvp_kill;

        // ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT     = 72
        struct
        {
            uint32  goEntry;                                // 3
            uint32  lootCount;                              // 4
        } fish_in_gameobject;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS = 75
        struct
        {
            uint32  skillLine;                              // 3
            uint32  spellCount;                             // 4
        } learn_skillline_spell;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL               = 76
        struct
        {
            uint32  unused;                                 // 3
            uint32  duelCount;                              // 4
        } win_duel;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_POWER          = 96
        struct
        {
            uint32  powerType;                              // 3 mana=0, 1=rage, 3=energy, 6=runic power
        } highest_power;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_STAT           = 97
        struct
        {
            uint32  statType;                               // 3 4=spirit, 3=int, 2=stamina, 1=agi, 0=strength
        } highest_stat;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELLPOWER     = 98
        struct
        {
            uint32  spellSchool;                            // 3
        } highest_spellpower;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_RATING         = 100
        struct
        {
            uint32  ratingType;                             // 3
        } highest_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE              = 109
        struct
        {
            uint32  lootType;                               // 3 3=fishing, 2=pickpocket, 4=disentchant
            uint32  lootTypeCount;                          // 4
        } loot_type;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE       = 112
        struct
        {
            uint32  skillLine;                              // 3
            uint32  spellCount;                             // 4
        } learn_skill_line;

        // ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL    = 113
        struct
        {
            uint32  unused;                                 // 3
            uint32  killCount;                              // 4
        } honorable_kill;

        struct
        {
            uint32  value;                                  // 3        m_asset_id
            uint32  count;                                  // 4        m_quantity
            uint32  additionalRequirement1_type;            // 5        m_start_event
            uint32  additionalRequirement1_value;           // 6        m_start_asset
            uint32  additionalRequirement2_type;            // 7        m_fail_event
            uint32  additionalRequirement2_value;           // 8        m_fail_asset
        } raw;
    };
    char*  name[16];                                        // 9-24     m_description_lang
    //uint32 name_flags;                                    // 25
    uint32  completionFlag;                                 // 26       m_flags
    //uint32  timedCriteriaStartType;                       // 27       m_timer_start_event Only appears with timed achievements, seems to be the type of starting a timed Achievement, only type 1 and some of type 6 need manual starting: 1: ByEventId(?) (serverside IDs), 2: ByQuestId, 5: ByCastSpellId(?), 6: BySpellIdTarget(some of these are unknown spells, some not, some maybe spells), 7: ByKillNpcId,  9: ByUseItemId
    uint32  timedCriteriaMiscId;                            // 28       m_timer_asset_id Alway appears with timed events, used internally to start the achievement, store
    uint32  timeLimit;                                      // 29       m_timer_time
    uint32  showOrder;                                      // 30       m_ui_order also used in achievement shift-links as index in state bitmask

    // helpers
    bool IsExplicitlyStartedTimedCriteria() const
    {
        if (!timeLimit)
            return false;

        // in case raw.value == timedCriteriaMiscId in timedCriteriaMiscId stored spellid/itemids for cast/use, so repeating aura start at first cast/use until fails
        return requiredType == ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST || raw.value != timedCriteriaMiscId;
    }
};

struct AreaTableEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  mapid;                                          // 1        m_ContinentID
    uint32  zone;                                           // 2        m_ParentAreaID
    uint32  exploreFlag;                                    // 3        m_AreaBit
    uint32  flags;                                          // 4        m_flags
                                                            // 5        m_SoundProviderPref
                                                            // 6        m_SoundProviderPrefUnderwater
                                                            // 7        m_AmbienceID
                                                            // 8        m_ZoneMusic
                                                            // 9        m_IntroSound
    int32   area_level;                                     // 10       m_ExplorationLevel
    char*   area_name[16];                                  // 11-26    m_AreaName_lang
                                                            // 27 string flags
    uint32  team;                                           // 28       m_factionGroupMask
                                                            // 29-32    m_liquidTypeID[4]
                                                            // 33       m_minElevation
                                                            // 34       m_ambient_multiplier
                                                            // 35       m_lightid
};

struct AreaGroupEntry
{
    uint32  AreaGroupId;                                    // 0        m_ID
    uint32  AreaId[6];                                      // 1-6      m_areaID
    uint32  nextGroup;                                      // 7        m_nextAreaID
};

struct AreaTriggerEntry
{
    uint32  id;                                             // 0        m_ID
    uint32  mapid;                                          // 1        m_ContinentID
    float   x;                                              // 2        m_x
    float   y;                                              // 3        m_y
    float   z;                                              // 4        m_z
    float   radius;                                         // 5        m_radius
    float   box_x;                                          // 6        m_box_length
    float   box_y;                                          // 7        m_box_width
    float   box_z;                                          // 8        m_box_heigh
    float   box_orientation;                                // 9        m_box_yaw
};

struct AuctionHouseEntry
{
    uint32    houseId;                                      // 0        m_ID
    uint32    faction;                                      // 1        m_factionID
    uint32    depositPercent;                               // 2        m_depositRate
    uint32    cutPercent;                                   // 3        m_consignmentRate
    //char*     name[16];                                   // 4-19     m_name_lang
                                                            // 20 string flags
};

struct BankBagSlotPricesEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  price;                                          // 1        m_Cost
};

struct BarberShopStyleEntry
{
    uint32  Id;                                             // 0        m_ID
    uint32  type;                                           // 1        m_type
    //char*   name[16];                                     // 2-17     m_DisplayName_lang
    //uint32  name_flags;                                   // 18 string flags
    //uint32  unk_name[16];                                 // 19-34    m_Description_lang
    //uint32  unk_flags;                                    // 35 string flags
    //float   CostMultiplier;                               // 36       m_Cost_Modifier
    uint32  race;                                           // 37       m_race
    uint32  gender;                                         // 38       m_sex
    uint32  hair_id;                                        // 39       m_data (real ID to hair/facial hair)
};

struct BattlemasterListEntry
{
    uint32  id;                                             // 0        m_ID
    int32   mapid[8];                                       // 1-8      m_mapID[8]
    uint32  type;                                           // 9        m_instanceType
    //uint32 canJoinAsGroup;                                // 10       m_groupsAllowed
    char*   name[16];                                       // 11-26    m_name_lang
    //uint32 nameFlags                                      // 27 string flags
    uint32 maxGroupSize;                                    // 28       m_maxGroupSize
    uint32 HolidayWorldStateId;                             // 29       m_holidayWorldState
    uint32 minLevel;                                        // 30       m_minlevel (sync with PvPDifficulty.dbc content)
    uint32 maxLevel;                                        // 31       m_maxlevel (sync with PvPDifficulty.dbc content)
};

/*struct Cfg_CategoriesEntry
{
    uint32 Index;                                           //          m_ID categoryId (sent in RealmList packet)
    uint32 Unk1;                                            //          m_localeMask
    uint32 Unk2;                                            //          m_charsetMask
    uint32 IsTournamentRealm;                               //          m_flags
    char *categoryName[16];                                 //          m_name_lang
    uint32 categoryNameFlags;
}*/

/*struct Cfg_ConfigsEntry
{
    uint32 Id;                                              //          m_ID
    uint32 Type;                                            //          m_realmType (sent in RealmList packet)
    uint32 IsPvp;                                           //          m_playerKillingAllowed
    uint32 IsRp;                                            //          m_roleplaying
};*/

#define MAX_OUTFIT_ITEMS 24

struct CharStartOutfitEntry
{
    //uint32 Id;                                            // 0        m_ID
    uint32 RaceClassGender;                                 // 1        m_raceID m_classID m_sexID m_outfitID (UNIT_FIELD_BYTES_0 & 0x00FFFFFF) comparable (0 byte = race, 1 byte = class, 2 byte = gender)
    int32 ItemId[MAX_OUTFIT_ITEMS];                         // 2-25     m_ItemID
    //int32 ItemDisplayId[MAX_OUTFIT_ITEMS];                // 26-29    m_DisplayItemID not required at server side
    //int32 ItemInventorySlot[MAX_OUTFIT_ITEMS];            // 50-73    m_InventoryType not required at server side
    //uint32 Unknown1;                                      // 74 unique values (index-like with gaps ordered in other way as ids)
    //uint32 Unknown2;                                      // 75
    //uint32 Unknown3;                                      // 76
};

struct CharTitlesEntry
{
    uint32  ID;                                             // 0,       m_ID
    //uint32      unk1;                                     // 1        m_Condition_ID
    char*   name[16];                                       // 2-17     m_name_lang
                                                            // 18 string flags
    //char*       name2[16];                                // 19-34    m_name1_lang
                                                            // 35 string flags
    uint32  bit_index;                                      // 36       m_mask_ID used in PLAYER_CHOSEN_TITLE and 1<<index in PLAYER__FIELD_KNOWN_TITLES
};

struct ChatChannelsEntry
{
    uint32  ChannelID;                                      // 0        m_ID
    uint32  flags;                                          // 1        m_flags
                                                            // 2        m_factionGroup
    char*   pattern[16];                                    // 3-18     m_name_lang
                                                            // 19 string flags
    //char*       name[16];                                 // 20-35    m_shortcut_lang
                                                            // 36 string flags
};

struct ChrClassesEntry
{
    uint32  ClassID;                                        // 0        m_ID
    //uint32 flags;                                         // 1 unknown
    uint32  powerType;                                      // 2        m_DisplayPower
                                                            // 3        m_petNameToken
    char const* name[16];                                   // 4-19     m_name_lang
                                                            // 20 string flags
    //char*       nameFemale[16];                           // 21-36    m_name_female_lang
                                                            // 37 string flags
    //char*       nameNeutralGender[16];                    // 38-53    m_name_male_lang
                                                            // 54 string flags
                                                            // 55       m_filename
    uint32  spellfamily;                                    // 56       m_spellClassSet
    //uint32 flags2;                                        // 57       m_flags (0x08 HasRelicSlot)
    uint32  CinematicSequence;                              // 58       m_cinematicSequenceID
    uint32  expansion;                                      // 59       m_required_expansion
};

struct ChrRacesEntry
{
    uint32      RaceID;                                     // 0        m_ID
                                                            // 1        m_flags
    uint32      FactionID;                                  // 2        m_factionID
                                                            // 3        m_ExplorationSoundID
    uint32      model_m;                                    // 4        m_MaleDisplayId
    uint32      model_f;                                    // 5        m_FemaleDisplayId
                                                            // 6        m_ClientPrefix
    uint32      TeamID;                                     // 7        m_BaseLanguage (7-Alliance 1-Horde)
                                                            // 8        m_creatureType
                                                            // 9        m_ResSicknessSpellID
                                                            // 10       m_SplashSoundID
                                                            // 11       m_clientFileString
    uint32      CinematicSequence;                          // 12       m_cinematicSequenceID
    //uint32    unk_322;                                    // 13       m_alliance (0 alliance, 1 horde, 2 not available?)
    char*       name[16];                                   // 14-29    m_name_lang used for DBC language detection/selection
                                                            // 30 string flags
    //char*       nameFemale[16];                           // 31-46    m_name_female_lang
                                                            // 47 string flags
    //char*       nameNeutralGender[16];                    // 48-63    m_name_male_lang
                                                            // 64 string flags
                                                            // 65-66    m_facialHairCustomization[2]
                                                            // 67       m_hairCustomization
    uint32      expansion;                                  // 68       m_required_expansion
};

/*struct CinematicCameraEntry
{
    uint32      id;                                         // 0        m_ID
    char*       filename;                                   // 1        m_model
    uint32      soundid;                                    // 2        m_soundID
    float       start_x;                                    // 3        m_originX
    float       start_y;                                    // 4        m_originY
    float       start_z;                                    // 5        m_originZ
    float       unk6;                                       // 6        m_originFacing
};*/

struct CinematicSequencesEntry
{
    uint32      Id;                                         // 0        m_ID
    //uint32      unk1;                                     // 1        m_soundID
    //uint32      cinematicCamera;                          // 2        m_camera[8]
};

struct CreatureDisplayInfoEntry
{
    uint32      Displayid;                                  // 0        m_ID
                                                            // 1        m_modelID
                                                            // 2        m_soundID
    uint32      ExtendedDisplayInfoID;                      // 3        m_extendedDisplayInfoID -> CreatureDisplayInfoExtraEntry::DisplayExtraId
    float       scale;                                      // 4        m_creatureModelScale
                                                            // 5        m_creatureModelAlpha
                                                            // 6-8      m_textureVariation[3]
                                                            // 9        m_portraitTextureName
                                                            // 10       m_sizeClass
                                                            // 11       m_bloodID
                                                            // 12       m_NPCSoundID
                                                            // 13       m_particleColorID
                                                            // 14       m_creatureGeosetData
                                                            // 15       m_objectEffectPackageID
};

struct CreatureDisplayInfoExtraEntry
{
    uint32      DisplayExtraId;                             // 0        m_ID CreatureDisplayInfoEntry::m_extendedDisplayInfoID
    uint32      Race;                                       // 1        m_DisplayRaceID
    //uint32    Gender;                                     // 2        m_DisplaySexID
    //uint32    SkinColor;                                  // 3        m_SkinID
    //uint32    FaceType;                                   // 4        m_FaceID
    //uint32    HairType;                                   // 5        m_HairStyleID
    //uint32    HairStyle;                                  // 6        m_HairColorID
    //uint32    BeardStyle;                                 // 7        m_FacialHairID
    //uint32    Equipment[11];                              // 8-18     m_NPCItemDisplay equipped static items EQUIPMENT_SLOT_HEAD..EQUIPMENT_SLOT_HANDS, client show its by self
    //uint32    CanEquip;                                   // 19       m_flags 0..1 Can equip additional things when used for players
    //char*                                                 // 20       m_BakeName CreatureDisplayExtra-*.blp
};

struct CreatureFamilyEntry
{
    uint32  ID;                                             // 0        m_ID
    float   minScale;                                       // 1        m_minScale
    uint32  minScaleLevel;                                  // 2        m_minScaleLevel
    float   maxScale;                                       // 3        m_maxScale
    uint32  maxScaleLevel;                                  // 4        m_maxScaleLevel
    uint32  skillLine[2];                                   // 5-6      m_skillLine
    uint32  petFoodMask;                                    // 7        m_petFoodMask
    int32   petTalentType;                                  // 8        m_petTalentType
                                                            // 9        m_categoryEnumID
    char*   Name[16];                                       // 10-25    m_name_lang
                                                            // 26 string flags
                                                            // 27       m_iconFile
};

#define MAX_CREATURE_SPELL_DATA_SLOT 4

struct CreatureSpellDataEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    spellId[MAX_CREATURE_SPELL_DATA_SLOT];        // 1-4      m_spells[4]
    //uint32    availability[MAX_CREATURE_SPELL_DATA_SLOT]; // 4-7      m_availability[4]
};

struct CreatureTypeEntry
{
    uint32    ID;                                           // 0        m_ID
    //char*   Name[16];                                     // 1-16     m_name_lang
                                                            // 17 string flags
    //uint32    no_expirience;                              // 18       m_flags
};

/* not used
struct CurrencyCategoryEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    Unk1;                                         // 1        m_flags 0 for known categories and 3 for unknown one (3.0.9)
    char*   Name[16];                                       // 2-17     m_name_lang
    //                                                      // 18 string flags
};
*/

struct CurrencyTypesEntry
{
    //uint32    ID;                                         // 0        m_ID
    uint32    ItemId;                                       // 1        m_itemID used as real index
    //uint32    Category;                                   // 2        m_categoryID may be category
    uint32    BitIndex;                                     // 3        m_bitIndex bit index in PLAYER_FIELD_KNOWN_CURRENCIES (1 << (index-1))
};

struct DungeonEncounterEntry
{
    uint32 Id;                                              // 0        m_ID
    uint32 mapId;                                           // 1        m_mapID
    uint32 Difficulty;                                      // 2        m_difficulty
    uint32 encounterData;                                   // 3        m_orderIndex
    uint32 encounterIndex;                                  // 4        m_Bit
    char*  encounterName[16];                               // 5-20     m_name_lang
    //uint32 nameLangFlags;                                 // 21       m_name_lang_flags
    //uint32 spellIconID;                                   // 22       m_spellIconID
};

struct DurabilityCostsEntry
{
    uint32    Itemlvl;                                      // 0        m_ID
    uint32    multiplier[29];                               // 1-29     m_weaponSubClassCost m_armorSubClassCost
};

struct DurabilityQualityEntry
{
    uint32    Id;                                           // 0        m_ID
    float     quality_mod;                                  // 1        m_data
};

struct EmotesEntry
{
    uint32  Id;                                             // 0        m_ID
    //char*   Name;                                         // 1        m_EmoteSlashCommand
    //uint32  AnimationId;                                  // 2        m_AnimID
    uint32  Flags;                                          // 3        m_EmoteFlags
    uint32  EmoteType;                                      // 4        m_EmoteSpecProc (determine how emote are shown)
    uint32  UnitStandState;                                 // 5        m_EmoteSpecProcParam
    //uint32  SoundId;                                      // 6        m_EventSoundID
};

struct EmotesTextEntry
{
    uint32  Id;                                             //          m_ID
                                                            //          m_name
    uint32  textid;                                         //          m_emoteID
                                                            //          m_emoteText
};

struct FactionEntry
{
    uint32      ID;                                         // 0        m_ID
    int32       reputationListID;                           // 1        m_reputationIndex
    uint32      BaseRepRaceMask[4];                         // 2-5      m_reputationRaceMask
    uint32      BaseRepClassMask[4];                        // 6-9      m_reputationClassMask
    int32       BaseRepValue[4];                            // 10-13    m_reputationBase
    uint32      ReputationFlags[4];                         // 14-17    m_reputationFlags
    uint32      team;                                       // 18       m_parentFactionID
    float       spilloverRateIn;                            // 19       m_parentFactionMod[2] Faction gains incoming rep * spilloverRateIn
    float       spilloverRateOut;                           // 20       Faction outputs rep * spilloverRateOut as spillover reputation
    uint32      spilloverMaxRankIn;                         // 21       m_parentFactionCap[2] The highest rank the faction will profit from incoming spillover
    //uint32    spilloverRank_unk;                          // 22       It does not seem to be the max standing at which a faction outputs spillover ...so no idea
    char*       name[16];                                   // 23-38    m_name_lang
                                                            // 39 string flags
    //char*     description[16];                            // 40-55    m_description_lang
                                                            // 56 string flags

    // helpers

    int GetIndexFitTo(uint32 raceMask, uint32 classMask) const
    {
        for (int i = 0; i < 4; ++i)
        {
            if ((BaseRepRaceMask[i] == 0 || (BaseRepRaceMask[i] & raceMask)) &&
                (BaseRepClassMask[i] == 0 || (BaseRepClassMask[i] & classMask)))
                return i;
        }

        return -1;
    }
};

struct FactionTemplateEntry
{
    uint32      ID;                                         // 0        m_ID
    uint32      faction;                                    // 1        m_faction
    uint32      factionFlags;                               // 2        m_flags
    uint32      ourMask;                                    // 3        m_factionGroup
    uint32      friendlyMask;                               // 4        m_friendGroup
    uint32      hostileMask;                                // 5        m_enemyGroup
    uint32      enemyFaction[4];                            // 6        m_enemies[4]
    uint32      friendFaction[4];                           // 10       m_friend[4]
    //-------------------------------------------------------  end structure

    // helpers
    bool IsFriendlyTo(FactionTemplateEntry const& entry) const
    {
        if(entry.faction)
        {
            for(int i = 0; i < 4; ++i)
                if (enemyFaction[i]  == entry.faction)
                    return false;
            for(int i = 0; i < 4; ++i)
                if (friendFaction[i] == entry.faction)
                    return true;
        }
        return (friendlyMask & entry.ourMask) || (ourMask & entry.friendlyMask);
    }
    bool IsHostileTo(FactionTemplateEntry const& entry) const
    {
        if(entry.faction)
        {
            for(int i = 0; i < 4; ++i)
                if (enemyFaction[i]  == entry.faction)
                    return true;
            for(int i = 0; i < 4; ++i)
                if (friendFaction[i] == entry.faction)
                    return false;
        }
        return (hostileMask & entry.ourMask) != 0;
    }
    bool IsHostileToPlayers() const { return (hostileMask & FACTION_MASK_PLAYER) !=0; }
    bool IsNeutralToAll() const
    {
        for(int i = 0; i < 4; ++i)
            if (enemyFaction[i] != 0)
                return false;
        return hostileMask == 0 && friendlyMask == 0;
    }
    bool IsContestedGuardFaction() const { return (factionFlags & FACTION_TEMPLATE_FLAG_CONTESTED_GUARD)!=0; }
};

struct GameObjectDisplayInfoEntry
{
    uint32      Displayid;                                  // 0        m_ID
    // char* filename;                                      // 1        m_modelName
                                                            // 2-11     m_Sound
    float  unknown12;                                       // 12       m_geoBoxMinX (use first value as interact dist, mostly in hacks way)
                                                            // 13       m_geoBoxMinY
                                                            // 14       m_geoBoxMinZ
                                                            // 15       m_geoBoxMaxX
                                                            // 16       m_geoBoxMaxY
                                                            // 17       m_geoBoxMaxZ
                                                            // 18       m_objectEffectPackageID
};

struct GemPropertiesEntry
{
    uint32      ID;                                         //          m_id
    uint32      spellitemenchantement;                      //          m_enchant_id
                                                            //          m_maxcount_inv
                                                            //          m_maxcount_item
    uint32      color;                                      //          m_type
};

struct GlyphPropertiesEntry
{
    uint32  Id;                                             //          m_id
    uint32  SpellId;                                        //          m_spellID
    uint32  TypeFlags;                                      //          m_glyphSlotFlags
    uint32  Unk1;                                           //          m_spellIconID
};

struct GlyphSlotEntry
{
    uint32  Id;                                             //          m_id
    uint32  TypeFlags;                                      //          m_type
    uint32  Order;                                          //          m_tooltip
};

// All Gt* DBC store data for 100 levels, some by 100 per class/race
#define GT_MAX_LEVEL    100
// gtOCTClassCombatRatingScalar.dbc stores data for 32 ratings, look at MAX_COMBAT_RATING for real used amount
#define GT_MAX_RATING   32

struct GtBarberShopCostBaseEntry
{
    float   cost;
};

struct GtCombatRatingsEntry
{
    float    ratio;
};

struct GtChanceToMeleeCritBaseEntry
{
    float    base;
};

struct GtChanceToMeleeCritEntry
{
    float    ratio;
};

struct GtChanceToSpellCritBaseEntry
{
    float    base;
};

struct GtChanceToSpellCritEntry
{
    float    ratio;
};

struct GtOCTClassCombatRatingScalarEntry
{
    float    ratio;
};

struct GtOCTRegenHPEntry
{
    float    ratio;
};

//struct GtOCTRegenMPEntry
//{
//    float    ratio;
//};

struct GtRegenHPPerSptEntry
{
    float    ratio;
};

struct GtRegenMPPerSptEntry
{
    float    ratio;
};

/*struct HolidayDescriptionsEntry
{
    uint32 ID;                                              // 0        m_ID this is NOT holiday id
    //char*     name[16]                                    // 1-16     m_name_lang
                                                            // 17 string flags
};*/

/* no used
struct HolidayNamesEntry
{
    uint32 ID;                                              // 0        m_ID this is NOT holiday id
    //char*     name[16]                                    // 1-16     m_name_lang
                                                            // 17 string flags
};
*/

struct HolidaysEntry
{
    uint32 ID;                                              // 0        m_ID
    //uint32 duration[10];                                  // 1-10     m_duration
    //uint32 date[26];                                      // 11-36    m_date (dates in unix time starting at January, 1, 2000)
    //uint32 region;                                        // 37       m_region (wow region)
    //uint32 looping;                                       // 38       m_looping
    //uint32 calendarFlags[10];                             // 39-48    m_calendarFlags
    //uint32 holidayNameId;                                 // 49       m_holidayNameID (HolidayNames.dbc)
    //uint32 holidayDescriptionId;                          // 50       m_holidayDescriptionID (HolidayDescriptions.dbc)
    //char *textureFilename;                                // 51       m_textureFilename
    //uint32 priority;                                      // 52       m_priority
    //uint32 calendarFilterType;                            // 53       m_calendarFilterType (-1,0,1 or 2)
    //uint32 flags;                                         // 54       m_flags
};

struct ItemEntry
{
   uint32   ID;                                             // 0        m_ID
   uint32   Class;                                          // 1        m_classID
   uint32   SubClass;                                       // 2        m_subclassID (some items have strange subclasses)
   int32    Unk0;                                           // 3        m_sound_override_subclassid
   int32    Material;                                       // 4        m_material
   uint32   DisplayId;                                      // 5        m_displayInfoID
   uint32   InventoryType;                                  // 6        m_inventoryType
   uint32   Sheath;                                         // 7        m_sheatheType
};

struct ItemBagFamilyEntry
{
    uint32   ID;                                            // 0        m_ID
    //char*     name[16]                                    // 1-16     m_name_lang
    //                                                      // 17       name flags
};

struct ItemClassEntry
{
    uint32   ID;                                            // 0        m_ID
    //uint32   unk1;                                        // 1
    //uint32   unk2;                                        // 2        only weapon have 1 in field, other 0
    char*    name[16];                                      // 3-19     m_name_lang
    //                                                      // 20       name flags
};

struct ItemDisplayInfoEntry
{
    uint32      ID;                                         // 0        m_ID
                                                            // 1        m_modelName[2]
                                                            // 2        m_modelTexture[2]
                                                            // 3        m_inventoryIcon
                                                            // 4        m_geosetGroup[3]
                                                            // 5        m_flags
                                                            // 6        m_spellVisualID
                                                            // 7        m_groupSoundIndex
                                                            // 8        m_helmetGeosetVis[2]
                                                            // 9        m_texture[2]
                                                            // 10       m_itemVisual[8]
                                                            // 11       m_particleColorID
};

//struct ItemCondExtCostsEntry
//{
//    uint32      ID;
//    uint32      condExtendedCost;                         // ItemPrototype::CondExtendedCost
//    uint32      itemextendedcostentry;                    // ItemPrototype::ExtendedCost
//    uint32      arenaseason;                              // arena season number(1-4)
//};

#define MAX_EXTENDED_COST_ITEMS 5

struct ItemExtendedCostEntry
{
    uint32      ID;                                         // 0        m_ID
    uint32      reqhonorpoints;                             // 1        m_honorPoints
    uint32      reqarenapoints;                             // 2        m_arenaPoints
    uint32      reqarenaslot;                               // 4        m_arenaBracket
    uint32      reqitem[MAX_EXTENDED_COST_ITEMS];           // 5-8      m_itemID
    uint32      reqitemcount[MAX_EXTENDED_COST_ITEMS];      // 9-13     m_itemCount
    uint32      reqpersonalarenarating;                     // 14       m_requiredArenaRating
                                                            // 15       m_itemPurchaseGroup
};

struct ItemLimitCategoryEntry
{
    uint32      ID;                                         // 0 Id     m_ID
    //char*     name[16]                                    // 1-16     m_name_lang
                                                            // 17 string flags
    uint32      maxCount;                                   // 18       m_quantity max allowed equipped as item or in gem slot
    uint32      mode;                                       // 19       m_flags 0 = have, 1 = equip (enum ItemLimitCategoryMode)
};

struct ItemRandomPropertiesEntry
{
    uint32    ID;                                           // 0        m_ID
    //char*     internalName                                // 1        m_Name
    uint32    enchant_id[5];                                // 2-6      m_Enchantment
    char*     nameSuffix[16];                               // 7-22     m_name_lang
                                                            // 23 string flags
};

struct ItemRandomSuffixEntry
{
    uint32    ID;                                           // 0        m_ID
    char*     nameSuffix[16];                               // 1-16     m_name_lang
                                                            // 17 string flags
                                                            // 18       m_internalName
    uint32    enchant_id[5];                                // 19-21    m_enchantment
    uint32    prefix[5];                                    // 22-24    m_allocationPct
};

struct ItemSetEntry
{
    //uint32    id                                          // 0        m_ID
    char*     name[16];                                     // 1-16     m_name_lang
                                                            // 17 string flags
    //uint32    itemId[17];                                 // 18-34    m_itemID
    uint32    spells[8];                                    // 35-42    m_setSpellID
    uint32    items_to_triggerspell[8];                     // 43-50    m_setThreshold
    uint32    required_skill_id;                            // 51       m_requiredSkill
    uint32    required_skill_value;                         // 52       m_requiredSkillRank
};

/*struct LfgDungeonsEntry
{
    m_ID
    m_name_lang
    m_minLevel
    m_maxLevel
    m_target_level
    m_target_level_min
    m_target_level_max
    m_mapID
    m_difficulty
    m_flags
    m_typeID
    m_faction
    m_textureFilename
    m_expansionLevel
    m_order_index
    m_group_id
    m_description_lang
};*/

/*struct LfgDungeonGroupEntry
{
    m_ID
    m_name_lang
    m_order_index
    m_parent_group_id
    m_typeid
};*/

/*struct LfgDungeonExpansionEntry
{
    m_ID
    m_lfg_id
    m_expansion_level
    m_random_id
    m_hard_level_min
    m_hard_level_max
    m_target_level_min
    m_target_level_max
};*/

#define MAX_LOCK_CASE 8

struct LockEntry
{
    uint32      ID;                                         // 0        m_ID
    uint32      Type[MAX_LOCK_CASE];                        // 1-8      m_Type
    uint32      Index[MAX_LOCK_CASE];                       // 9-16     m_Index
    uint32      Skill[MAX_LOCK_CASE];                       // 17-24    m_Skill
    //uint32      Action[MAX_LOCK_CASE];                    // 25-32    m_Action
};

struct MailTemplateEntry
{
    uint32      ID;                                         // 0        m_ID
    //char*       subject[16];                              // 1-16     m_subject_lang
                                                            // 17 string flags
    char*       content[16];                                // 18-33    m_body_lang
};

struct MapEntry
{
    uint32  MapID;                                          // 0        m_ID
    //char*       internalname;                             // 1        m_Directory
    uint32  map_type;                                       // 2        m_InstanceType
    //uint32 mapFlags;                                      // 3        m_Flags (0x100 - CAN_CHANGE_PLAYER_DIFFICULTY)
    //uint32 isPvP;                                         // 4        m_PVP 0 or 1 for battlegrounds (not arenas)
    char*   name[16];                                       // 5-20     m_MapName_lang
                                                            // 21 string flags
    uint32  linked_zone;                                    // 22       m_areaTableID
    //char*     hordeIntro[16];                             // 23-38    m_MapDescription0_lang
                                                            // 39 string flags
    //char*     allianceIntro[16];                          // 40-55    m_MapDescription1_lang
                                                            // 56 string flags
    uint32  multimap_id;                                    // 57       m_LoadingScreenID (LoadingScreens.dbc)
    //float   BattlefieldMapIconScale;                      // 58       m_minimapIconScale
    int32   ghost_entrance_map;                             // 59       m_corpseMapID map_id of entrance map in ghost mode (continent always and in most cases = normal entrance)
    float   ghost_entrance_x;                               // 60       m_corpseX entrance x coordinate in ghost mode  (in most cases = normal entrance)
    float   ghost_entrance_y;                               // 61       m_corpseY entrance y coordinate in ghost mode  (in most cases = normal entrance)
    //uint32  timeOfDayOverride;                            // 62       m_timeOfDayOverride
    uint32  addon;                                          // 63       m_expansionID
                                                            // 64       m_raidOffset
    //uint32 maxPlayers;                                    // 65       m_maxPlayers

    // Helpers
    uint32 Expansion() const { return addon; }

    bool IsDungeon() const { return map_type == MAP_INSTANCE || map_type == MAP_RAID; }
    bool IsNonRaidDungeon() const { return map_type == MAP_INSTANCE; }
    bool Instanceable() const { return map_type == MAP_INSTANCE || map_type == MAP_RAID || map_type == MAP_BATTLEGROUND || map_type == MAP_ARENA; }
    bool IsRaid() const { return map_type == MAP_RAID; }
    bool IsBattleGround() const { return map_type == MAP_BATTLEGROUND; }
    bool IsBattleArena() const { return map_type == MAP_ARENA; }
    bool IsBattleGroundOrArena() const { return map_type == MAP_BATTLEGROUND || map_type == MAP_ARENA; }

    bool IsMountAllowed() const
    {
        return !IsDungeon() ||
            MapID==209 || MapID==269 || MapID==309 ||       // TanarisInstance, CavernsOfTime, Zul'gurub
            MapID==509 || MapID==534 || MapID==560 ||       // AhnQiraj, HyjalPast, HillsbradPast
            MapID==568 || MapID==580 || MapID==595 ||       // ZulAman, Sunwell Plateau, Culling of Stratholme
            MapID==603 || MapID==615 || MapID==616;         // Ulduar, The Obsidian Sanctum, The Eye Of Eternity
    }

    bool IsContinent() const
    {
        return MapID == 0 || MapID == 1 || MapID == 530 || MapID == 571;
    }
};

struct MapDifficultyEntry
{
    //uint32      Id;                                       // 0        m_ID
    uint32      MapId;                                      // 1        m_mapID
    uint32      Difficulty;                                 // 2        m_difficulty (for arenas: arena slot)
    //char*       areaTriggerText[16];                      // 3-18     m_message_lang (text showed when transfer to map failed)
    //uint32      textFlags;                                // 19 
    uint32      resetTime;                                  // 20       m_raidDuration in secs, 0 if no fixed reset time
    uint32      maxPlayers;                                 // 21       m_maxPlayers some heroic versions have 0 when expected same amount as in normal version
    //char*       difficultyString;                         // 22       m_difficultystring
};

struct MovieEntry
{
    uint32      Id;                                         // 0        m_ID
    //char*       filename;                                 // 1        m_filename
    //uint32      unk2;                                     // 2        m_volume
};

#define MAX_OVERRIDE_SPELLS     10

struct OverrideSpellDataEntry
{
    uint32      Id;                                         // 0        m_ID
    uint32      Spells[MAX_OVERRIDE_SPELLS];                // 1-10     m_spells
    //uint32      unk2;                                     // 11       m_flags
};

struct PvPDifficultyEntry
{
    //uint32      id;                                       // 0        m_ID
    uint32      mapId;                                      // 1        m_mapID
    uint32      bracketId;                                  // 2        m_rangeIndex
    uint32      minLevel;                                   // 3        m_minLevel
    uint32      maxLevel;                                   // 4        m_maxLevel
    uint32      difficulty;                                 // 5        m_difficulty

    // helpers
    BattleGroundBracketId GetBracketId() const { return BattleGroundBracketId(bracketId); }
};

struct QuestFactionRewardEntry
{
    uint32      id;                                         // 0        m_ID
    int32       rewardValue[10];                            // 1-10     m_Difficulty
};

struct QuestSortEntry
{
    uint32      id;                                         // 0        m_ID
    //char*       name[16];                                 // 1-16     m_SortName_lang
                                                            // 17 string flags
};

struct QuestXPLevel
{
    uint32      questLevel;                                 // 0        m_ID
    uint32      xpIndex[10];                                // 1-10     m_difficulty[10]
};

struct RandomPropertiesPointsEntry
{
    //uint32  Id;                                           // 0        m_ID
    uint32    itemLevel;                                    // 1        m_ItemLevel
    uint32    EpicPropertiesPoints[5];                      // 2-6      m_Epic
    uint32    RarePropertiesPoints[5];                      // 7-11     m_Superior
    uint32    UncommonPropertiesPoints[5];                  // 12-16    m_Good
};

struct ScalingStatDistributionEntry
{
    uint32  Id;                                             // 0        m_ID
    int32   StatMod[10];                                    // 1-10     m_statID
    uint32  Modifier[10];                                   // 11-20    m_bonus
    uint32  MaxLevel;                                       // 21       m_maxlevel
};

struct ScalingStatValuesEntry
{
    uint32  Id;                                             // 0        m_ID
    uint32  Level;                                          // 1        m_charlevel
    uint32  ssdMultiplier[4];                               // 2-5 Multiplier for ScalingStatDistribution
    uint32  armorMod[4];                                    // 6-9 Armor for level
    uint32  dpsMod[6];                                      // 10-15 DPS mod for level
    uint32  spellBonus;                                     // 16 spell power for level
    uint32  ssdMultiplier2;                                 // 17 there's data from 3.1 dbc ssdMultiplier[3]
    uint32  ssdMultiplier3;                                 // 18 3.3
    //uint32 unk2;                                          // 19 unk, probably also Armor for level (flag 0x80000?)
    uint32  armorMod2[4];                                   // 20-23 Armor for level

/*struct ScalingStatValuesEntry
{
    m_ID
    m_charlevel
    m_shoulderBudget
    m_trinketBudget
    m_weaponBudget1H
    m_rangedBudget
    m_clothShoulderArmor
    m_leatherShoulderArmor
    m_mailShoulderArmor
    m_plateShoulderArmor
    m_weaponDPS1H
    m_weaponDPS2H
    m_spellcasterDPS1H
    m_spellcasterDPS2H
    m_rangedDPS
    m_wandDPS
    m_spellPower
    m_primaryBudget
    m_tertiaryBudget
    m_clothCloakArmor
    m_clothChestArmor
    m_leatherChestArmor
    m_mailChestArmor
    m_plateChestArmor
};*/
    uint32  getssdMultiplier(uint32 mask) const
    {
        if (mask & 0x4001F)
        {
            if(mask & 0x00000001) return ssdMultiplier[0];
            if(mask & 0x00000002) return ssdMultiplier[1];
            if(mask & 0x00000004) return ssdMultiplier[2];
            if(mask & 0x00000008) return ssdMultiplier2;
            if(mask & 0x00000010) return ssdMultiplier[3];
            if(mask & 0x00040000) return ssdMultiplier3;
        }
        return 0;
    }

    uint32  getArmorMod(uint32 mask) const
    {
        if (mask & 0x00F001E0)
        {
            if(mask & 0x00000020) return armorMod[0];
            if(mask & 0x00000040) return armorMod[1];
            if(mask & 0x00000080) return armorMod[2];
            if(mask & 0x00000100) return armorMod[3];

            if(mask & 0x00100000) return armorMod2[0];      // cloth
            if(mask & 0x00200000) return armorMod2[1];      // leather
            if(mask & 0x00400000) return armorMod2[2];      // mail
            if(mask & 0x00800000) return armorMod2[3];      // plate
        }
        return 0;
    }

    uint32 getDPSMod(uint32 mask) const
    {
        if (mask & 0x7E00)
        {
            if(mask & 0x00000200) return dpsMod[0];
            if(mask & 0x00000400) return dpsMod[1];
            if(mask & 0x00000800) return dpsMod[2];
            if(mask & 0x00001000) return dpsMod[3];
            if(mask & 0x00002000) return dpsMod[4];
            if(mask & 0x00004000) return dpsMod[5];         // not used?
        }
        return 0;
    }

    uint32 getSpellBonus(uint32 mask) const
    {
        if (mask & 0x00008000)
            return spellBonus;
        return 0;
    }

    uint32 getFeralBonus(uint32 mask) const                 // removed in 3.2.x?
    {
        if (mask & 0x00010000)                              // not used?
            return 0;
        return 0;
    }
};

/*struct SkillLineCategoryEntry
{
    uint32    id;                                           // 0        m_ID
    char*     name[16];                                     // 1-17     m_name_lang
                                                            // 18 string flags
    uint32    displayOrder;                                 // 19       m_sortIndex
};*/

struct SkillRaceClassInfoEntry
{
    //uint32    id;                                         // 0        m_ID
    uint32    skillId;                                      // 1        m_skillID
    uint32    raceMask;                                     // 2        m_raceMask
    uint32    classMask;                                    // 3        m_classMask
    uint32    flags;                                        // 4        m_flags
    uint32    reqLevel;                                     // 5        m_minLevel
    //uint32    skillTierId;                                // 6        m_skillTierID
    //uint32    skillCostID;                                // 7        m_skillCostIndex
};

/*struct SkillTiersEntry{
    uint32    id;                                           // 0        m_ID
    uint32    skillValue[16];                               // 1-17     m_cost
    uint32    maxSkillValue[16];                            // 18-3     m_valueMax
};*/

struct SkillLineEntry
{
    uint32    id;                                           // 0        m_ID
    int32     categoryId;                                   // 1        m_categoryID
    //uint32    skillCostID;                                // 2        m_skillCostsID
    char*     name[16];                                     // 3-18     m_displayName_lang
                                                            // 19 string flags
    //char*     description[16];                            // 20-35    m_description_lang
                                                            // 36 string flags
    uint32    spellIcon;                                    // 37       m_spellIconID
    //char*     alternateVerb[16];                          // 38-53    m_alternateVerb_lang
                                                            // 54 string flags
    uint32    canLink;                                      // 55       m_canLink (prof. with recipes)
};

struct SkillLineAbilityEntry
{
    uint32    id;                                           // 0        m_ID
    uint32    skillId;                                      // 1        m_skillLine
    uint32    spellId;                                      // 2        m_spell
    uint32    racemask;                                     // 3        m_raceMask
    uint32    classmask;                                    // 4        m_classMask
    //uint32    racemaskNot;                                // 5        m_excludeRace
    //uint32    classmaskNot;                               // 6        m_excludeClass
    uint32    req_skill_value;                              // 7        m_minSkillLineRank
    uint32    forward_spellid;                              // 8        m_supercededBySpell
    uint32    learnOnGetSkill;                              // 9        m_acquireMethod
    uint32    max_value;                                    // 10       m_trivialSkillLineRankHigh
    uint32    min_value;                                    // 11       m_trivialSkillLineRankLow
    //uint32    characterPoints[2];                         // 12-13    m_characterPoints[2]
};

struct SoundEntriesEntry
{
    uint32    Id;                                           // 0        m_ID
    //uint32    Type;                                       // 1        m_soundType
    //char*     InternalName;                               // 2        m_name
    //char*     FileName[10];                               // 3-12     m_File[10]
    //uint32    Unk13[10];                                  // 13-22    m_Freq[10]
    //char*     Path;                                       // 23       m_DirectoryBase
                                                            // 24       m_volumeFloat
                                                            // 25       m_flags
                                                            // 26       m_minDistance
                                                            // 27       m_distanceCutoff
                                                            // 28       m_EAXDef
                                                            // 29       m_soundEntriesAdvancedID
};


struct ClassFamilyMask
{
    uint64 Flags;
    uint32 Flags2;

    ClassFamilyMask() : Flags(0), Flags2(0) {}
    explicit ClassFamilyMask(uint64 familyFlags, uint32 familyFlags2 = 0) : Flags(familyFlags), Flags2(familyFlags2) {}

    bool Empty() const { return Flags == 0 && Flags2 == 0; }
    bool operator! () const { return Empty(); }
    operator void const* () const { return Empty() ? NULL : this; }// for allow normal use in if(mask)

    bool IsFitToFamilyMask(uint64 familyFlags, uint32 familyFlags2 = 0) const
    {
        return (Flags & familyFlags) || (Flags2 & familyFlags2);
    }

    bool IsFitToFamilyMask(ClassFamilyMask const& mask) const
    {
        return (Flags & mask.Flags) || (Flags2 & mask.Flags2);
    }

    uint64 operator& (uint64 mask) const                     // possible will removed at finish convertion code use IsFitToFamilyMask
    {
        return Flags & mask;
    }

    ClassFamilyMask& operator|= (ClassFamilyMask const& mask)
    {
        Flags |= mask.Flags;
        Flags2 |= mask.Flags2;
        return *this;
    }
};

#define MAX_SPELL_REAGENTS 8
#define MAX_SPELL_TOTEMS 2
#define MAX_SPELL_TOTEM_CATEGORIES 2

struct SpellEntry
{
    uint32    Id;                                           // 0        m_ID
    uint32    Category;                                     // 1        m_category
    uint32    Dispel;                                       // 2        m_dispelType
    uint32    Mechanic;                                     // 3        m_mechanic
    uint32    Attributes;                                   // 4        m_attributes
    uint32    AttributesEx;                                 // 5        m_attributesEx
    uint32    AttributesEx2;                                // 6        m_attributesExB
    uint32    AttributesEx3;                                // 7        m_attributesExC
    uint32    AttributesEx4;                                // 8        m_attributesExD
    uint32    AttributesEx5;                                // 9        m_attributesExE
    uint32    AttributesEx6;                                // 10       m_attributesExF
    uint32    AttributesEx7;                                // 11       m_attributesExG (0x20 - totems, 0x4 - paladin auras, etc...)
    uint32    Stances;                                      // 12       m_shapeshiftMask
    // uint32 unk_320_1;                                    // 13       3.2.0
    uint32    StancesNot;                                   // 14       m_shapeshiftExclude
    // uint32 unk_320_2;                                    // 15       3.2.0
    uint32    Targets;                                      // 16       m_targets
    uint32    TargetCreatureType;                           // 17       m_targetCreatureType
    uint32    RequiresSpellFocus;                           // 18       m_requiresSpellFocus
    uint32    FacingCasterFlags;                            // 19       m_facingCasterFlags
    uint32    CasterAuraState;                              // 20       m_casterAuraState
    uint32    TargetAuraState;                              // 21       m_targetAuraState
    uint32    CasterAuraStateNot;                           // 22       m_excludeCasterAuraState
    uint32    TargetAuraStateNot;                           // 23       m_excludeTargetAuraState
    uint32    casterAuraSpell;                              // 24       m_casterAuraSpell
    uint32    targetAuraSpell;                              // 25       m_targetAuraSpell
    uint32    excludeCasterAuraSpell;                       // 26       m_excludeCasterAuraSpell
    uint32    excludeTargetAuraSpell;                       // 27       m_excludeTargetAuraSpell
    uint32    CastingTimeIndex;                             // 28       m_castingTimeIndex
    uint32    RecoveryTime;                                 // 29       m_recoveryTime
    uint32    CategoryRecoveryTime;                         // 30       m_categoryRecoveryTime
    uint32    InterruptFlags;                               // 31       m_interruptFlags
    uint32    AuraInterruptFlags;                           // 32       m_auraInterruptFlags
    uint32    ChannelInterruptFlags;                        // 33       m_channelInterruptFlags
    uint32    procFlags;                                    // 34       m_procTypeMask
    uint32    procChance;                                   // 35       m_procChance
    uint32    procCharges;                                  // 36       m_procCharges
    uint32    maxLevel;                                     // 37       m_maxLevel
    uint32    baseLevel;                                    // 38       m_baseLevel
    uint32    spellLevel;                                   // 39       m_spellLevel
    uint32    DurationIndex;                                // 40       m_durationIndex
    uint32    powerType;                                    // 41       m_powerType
    uint32    manaCost;                                     // 42       m_manaCost
    uint32    manaCostPerlevel;                             // 43       m_manaCostPerLevel
    uint32    manaPerSecond;                                // 44       m_manaPerSecond
    uint32    manaPerSecondPerLevel;                        // 45       m_manaPerSecondPerLevel
    uint32    rangeIndex;                                   // 46       m_rangeIndex
    float     speed;                                        // 47       m_speed
    //uint32    modalNextSpell;                             // 48       m_modalNextSpell not used
    uint32    StackAmount;                                  // 49       m_cumulativeAura
    uint32    Totem[MAX_SPELL_TOTEMS];                      // 50-51    m_totem
    int32     Reagent[MAX_SPELL_REAGENTS];                  // 52-59    m_reagent
    uint32    ReagentCount[MAX_SPELL_REAGENTS];             // 60-67    m_reagentCount
    int32     EquippedItemClass;                            // 68       m_equippedItemClass (value)
    int32     EquippedItemSubClassMask;                     // 69       m_equippedItemSubclass (mask)
    int32     EquippedItemInventoryTypeMask;                // 70       m_equippedItemInvTypes (mask)
    uint32    Effect[MAX_EFFECT_INDEX];                     // 71-73    m_effect
    int32     EffectDieSides[MAX_EFFECT_INDEX];             // 74-76    m_effectDieSides
    float     EffectRealPointsPerLevel[MAX_EFFECT_INDEX];   // 77-79    m_effectRealPointsPerLevel
    int32     EffectBasePoints[MAX_EFFECT_INDEX];           // 80-82    m_effectBasePoints (don't must be used in spell/auras explicitly, must be used cached Spell::m_currentBasePoints)
    uint32    EffectMechanic[MAX_EFFECT_INDEX];             // 83-85    m_effectMechanic
    uint32    EffectImplicitTargetA[MAX_EFFECT_INDEX];      // 86-88    m_implicitTargetA
    uint32    EffectImplicitTargetB[MAX_EFFECT_INDEX];      // 89-91    m_implicitTargetB
    uint32    EffectRadiusIndex[MAX_EFFECT_INDEX];          // 92-94    m_effectRadiusIndex - spellradius.dbc
    uint32    EffectApplyAuraName[MAX_EFFECT_INDEX];        // 95-97    m_effectAura
    uint32    EffectAmplitude[MAX_EFFECT_INDEX];            // 98-100   m_effectAuraPeriod
    float     EffectMultipleValue[MAX_EFFECT_INDEX];        // 101-103  m_effectAmplitude
    uint32    EffectChainTarget[MAX_EFFECT_INDEX];          // 104-106  m_effectChainTargets
    uint32    EffectItemType[MAX_EFFECT_INDEX];             // 107-109  m_effectItemType
    int32     EffectMiscValue[MAX_EFFECT_INDEX];            // 110-112  m_effectMiscValue
    int32     EffectMiscValueB[MAX_EFFECT_INDEX];           // 113-115  m_effectMiscValueB
    uint32    EffectTriggerSpell[MAX_EFFECT_INDEX];         // 116-118  m_effectTriggerSpell
    float     EffectPointsPerComboPoint[MAX_EFFECT_INDEX];  // 119-121  m_effectPointsPerCombo
    ClassFamilyMask EffectSpellClassMask[MAX_EFFECT_INDEX]; // 122-130  m_effectSpellClassMaskA/B/C, effect 0/1/2
    uint32    SpellVisual[2];                               // 131-132  m_spellVisualID
    uint32    SpellIconID;                                  // 133      m_spellIconID
    uint32    activeIconID;                                 // 134      m_activeIconID
    //uint32    spellPriority;                              // 135      m_spellPriority not used
    char*     SpellName[16];                                // 136-151  m_name_lang
    //uint32    SpellNameFlag;                              // 152      m_name_flag not used
    char*     Rank[16];                                     // 153-168  m_nameSubtext_lang
    //uint32    RankFlags;                                  // 169      m_nameSubtext_flag not used
    //char*     Description[16];                            // 170-185  m_description_lang not used
    //uint32    DescriptionFlags;                           // 186      m_description_flag not used
    //char*     ToolTip[16];                                // 187-202  m_auraDescription_lang not used
    //uint32    ToolTipFlags;                               // 203      m_auraDescription_flag not used
    uint32    ManaCostPercentage;                           // 204      m_manaCostPct
    uint32    StartRecoveryCategory;                        // 205      m_startRecoveryCategory
    uint32    StartRecoveryTime;                            // 206      m_startRecoveryTime
    uint32    MaxTargetLevel;                               // 207      m_maxTargetLevel
    uint32    SpellFamilyName;                              // 208      m_spellClassSet
    ClassFamilyMask SpellFamilyFlags;                       // 209-211  m_spellClassMask NOTE: size is 12 bytes!!!
    uint32    MaxAffectedTargets;                           // 212      m_maxTargets
    uint32    DmgClass;                                     // 213      m_defenseType
    uint32    PreventionType;                               // 214      m_preventionType
    //uint32    StanceBarOrder;                             // 215      m_stanceBarOrder not used
    float     DmgMultiplier[MAX_EFFECT_INDEX];              // 216-218  m_effectChainAmplitude
    //uint32    MinFactionId;                               // 219      m_minFactionID not used
    //uint32    MinReputation;                              // 220      m_minReputation not used
    //uint32    RequiredAuraVision;                         // 221      m_requiredAuraVision not used
    uint32    TotemCategory[MAX_SPELL_TOTEM_CATEGORIES];    // 222-223  m_requiredTotemCategoryID
    int32     AreaGroupId;                                  // 224      m_requiredAreasId
    uint32    SchoolMask;                                   // 225      m_schoolMask
    uint32    runeCostID;                                   // 226      m_runeCostID
    //uint32    spellMissileID;                             // 227      m_spellMissileID
    //uint32  PowerDisplayId;                               // 228      m_powerDisplayID (PowerDisplay.dbc)
    //float   effectBonusCoefficient[3];                    // 229-231  m_effectBonusCoefficient
    //uint32  spellDescriptionVariableID;                   // 232      m_descriptionVariablesID
    uint32  SpellDifficultyId;                              // 233      m_difficulty (SpellDifficulty.dbc)

    // helpers
    int32 CalculateSimpleValue(SpellEffectIndex eff) const { return EffectBasePoints[eff] + int32(1); }
    ClassFamilyMask const& GetEffectSpellClassMask(SpellEffectIndex effect) const
    {
        return EffectSpellClassMask[effect];
    }

    bool IsFitToFamilyMask(uint64 familyFlags, uint32 familyFlags2 = 0) const
    {
        return SpellFamilyFlags.IsFitToFamilyMask(familyFlags, familyFlags2);
    }

    bool IsFitToFamily(SpellFamily family, uint64 familyFlags, uint32 familyFlags2 = 0) const
    {
        return SpellFamily(SpellFamilyName) == family && IsFitToFamilyMask(familyFlags, familyFlags2);
    }

    bool IsFitToFamilyMask(ClassFamilyMask const& mask) const
    {
        return SpellFamilyFlags.IsFitToFamilyMask(mask);
    }

    bool IsFitToFamily(SpellFamily family, ClassFamilyMask const& mask) const
    {
        return SpellFamily(SpellFamilyName) == family && IsFitToFamilyMask(mask);
    }

    private:
        // prevent creating custom entries (copy data from original in fact)
        SpellEntry(SpellEntry const&);                      // DON'T must have implementation

        // catch wrong uses
        template<typename T>
        bool IsFitToFamilyMask(SpellFamily family, T t) const;
};

struct SpellCastTimesEntry
{
    uint32    ID;                                           // 0        m_ID
    int32     CastTime;                                     // 1        m_base
    //float     CastTimePerLevel;                           // 2        m_perLevel
    //int32     MinCastTime;                                // 3        m_minimum
};

struct SpellFocusObjectEntry
{
    uint32    ID;                                           // 0        m_ID
    //char*     Name[16];                                   // 1-15     m_name_lang
                                                            // 16 string flags
};

struct SpellRadiusEntry
{
    uint32    ID;                                           //          m_ID
    float     Radius;                                       //          m_radius
                                                            //          m_radiusPerLevel
    //float     RadiusMax;                                  //          m_radiusMax
};

struct SpellRangeEntry
{
    uint32    ID;                                           // 0        m_ID
    float     minRange;                                     // 1        m_rangeMin[2]
    float     minRangeFriendly;                             // 2 
    float     maxRange;                                     // 3        m_rangeMax[2]
    float     maxRangeFriendly;                             // 4
    //uint32  Flags;                                        // 5        m_flags
    //char*   Name[16];                                     // 6-21     m_displayName_lang
    //uint32  NameFlags;                                    // 22 string flags
    //char*   ShortName[16];                                // 23-38    m_displayNameShort_lang
    //uint32  NameFlags;                                    // 39 string flags
};

struct SpellRuneCostEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  RuneCost[3];                                    // 1-3      m_blood m_unholy m_frost (0=blood, 1=frost, 2=unholy)
    uint32  runePowerGain;                                  // 4        m_runicPower

    bool NoRuneCost() const { return RuneCost[0] == 0 && RuneCost[1] == 0 && RuneCost[2] == 0; }
    bool NoRunicPowerGain() const { return runePowerGain == 0; }
};

struct SpellShapeshiftFormEntry
{
    uint32 ID;                                              // 0        m_ID
    //uint32 buttonPosition;                                // 1        m_bonusActionBar
    //char*  Name[16];                                      // 2-17     m_name_lang
    //uint32 NameFlags;                                     // 18 string flags
    uint32 flags1;                                          // 19       m_flags
    int32  creatureType;                                    // 20       m_creatureType <=0 humanoid, other normal creature types
    //uint32 unk1;                                          // 21       m_attackIconID
    uint32 attackSpeed;                                     // 22       m_combatRoundTime
    uint32 modelID_A;                                       // 23       m_creatureDisplayID[4]
    uint32 modelID_H;                                       // 24
    //uint32 unk3;                                          // 25
    //uint32 unk4;                                          // 26
    uint32 spellId[8];                                      // 27-34    m_presetSpellID[8]
};

struct SpellDifficultyEntry
{
    uint32 ID;                                              // 0        m_ID
    uint32 spellId[MAX_DIFFICULTY];                         // 1-4      m_difficultySpellID[4]
};

struct SpellDurationEntry
{
    uint32    ID;                                           //          m_ID
    int32     Duration[3];                                  //          m_duration, m_durationPerLevel, m_maxDuration
};

struct SpellItemEnchantmentEntry
{
    uint32      ID;                                         // 0        m_ID
    //uint32      charges;                                  // 1        m_charges
    uint32      type[3];                                    // 2-4      m_effect[3]
    uint32      amount[3];                                  // 5-7      m_effectPointsMin[3]
    //uint32      amount2[3]                                // 8-10     m_effectPointsMax[3]
    uint32      spellid[3];                                 // 11-13    m_effectArg[3]
    char*       description[16];                            // 14-29    m_name_lang[16]
    //uint32      descriptionFlags;                         // 30 string flags
    uint32      aura_id;                                    // 31       m_itemVisual
    uint32      slot;                                       // 32       m_flags
    uint32      GemID;                                      // 33       m_src_itemID
    uint32      EnchantmentCondition;                       // 34       m_condition_id
    //uint32      requiredSkill;                            // 35       m_requiredSkillID
    //uint32      requiredSkillValue;                       // 36       m_requiredSkillRank
                                                            // 37       m_minLevel
};

struct SpellItemEnchantmentConditionEntry
{
    uint32  ID;                                             // 0        m_ID
    uint8   Color[5];                                       // 1-5      m_lt_operandType[5]
    //uint32  LT_Operand[5];                                // 6-10     m_lt_operand[5]
    uint8   Comparator[5];                                  // 11-15    m_operator[5]
    uint8   CompareColor[5];                                // 15-20    m_rt_operandType[5]
    uint32  Value[5];                                       // 21-25    m_rt_operand[5]
    //uint8   Logic[5]                                      // 25-30    m_logic[5]
};

struct StableSlotPricesEntry
{
    uint32 Slot;                                            //          m_ID
    uint32 Price;                                           //          m_cost
};

struct SummonPropertiesEntry
{
    uint32  Id;                                             // 0        m_id
    uint32  Group;                                          // 1        m_control (enum SummonPropGroup)
    uint32  FactionId;                                      // 2        m_faction
    uint32  Title;                                          // 3        m_title (enum UnitNameSummonTitle)
    uint32  Slot;                                           // 4        m_slot if title = UNITNAME_SUMMON_TITLE_TOTEM, its actual slot (0-6).
                                                            //      if title = UNITNAME_SUMMON_TITLE_COMPANION, slot=6 -> defensive guardian, in other cases criter/minipet
                                                            //      Slot may have other uses, selection of pet type in some cases?
    uint32  Flags;                                          // 5        m_flags (enum SummonPropFlags)
};

#define MAX_TALENT_RANK 5
#define MAX_PET_TALENT_RANK 3                               // use in calculations, expected <= MAX_TALENT_RANK

struct TalentEntry
{
    uint32    TalentID;                                     // 0        m_ID
    uint32    TalentTab;                                    // 1        m_tabID (TalentTab.dbc)
    uint32    Row;                                          // 2        m_tierID
    uint32    Col;                                          // 3        m_columnIndex
    uint32    RankID[MAX_TALENT_RANK];                      // 4-8      m_spellRank
                                                            // 9-12 part of prev field
    uint32    DependsOn;                                    // 13       m_prereqTalent (Talent.dbc)
                                                            // 14-15 part of prev field
    uint32    DependsOnRank;                                // 16       m_prereqRank
                                                            // 17-18 part of prev field
    //uint32  needAddInSpellBook;                           // 19       m_flags also need disable higest ranks on reset talent tree
    //uint32  unk2;                                         // 20       m_requiredSpellID
    //uint64  allowForPet;                                  // 21       m_categoryMask its a 64 bit mask for pet 1<<m_categoryEnumID in CreatureFamily.dbc
};

struct TalentTabEntry
{
    uint32  TalentTabID;                                    // 0        m_ID
    //char* name[16];                                       // 1-16     m_name_lang
    //uint32  nameFlags;                                    // 17 string flags
    //unit32  spellicon;                                    // 18       m_spellIconID
                                                            // 19       m_raceMask
    uint32  ClassMask;                                      // 20       m_classMask
    uint32  petTalentMask;                                  // 21       m_petTalentMask
    uint32  tabpage;                                        // 22       m_orderIndex
    //char* internalname;                                   // 23       m_backgroundFile
};

struct TaxiNodesEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    map_id;                                       // 1        m_ContinentID
    float     x;                                            // 2        m_x
    float     y;                                            // 3        m_y
    float     z;                                            // 4        m_z
    char*     name[16];                                     // 5-21     m_Name_lang
                                                            // 22 string flags
    uint32    MountCreatureID[2];                           // 23-24    m_MountCreatureID[2]
};

struct TaxiPathEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    from;                                         // 1        m_FromTaxiNode
    uint32    to;                                           // 2        m_ToTaxiNode
    uint32    price;                                        // 3        m_Cost
};

struct TaxiPathNodeEntry
{
                                                            // 0        m_ID
    uint32    path;                                         // 1        m_PathID
    uint32    index;                                        // 2        m_NodeIndex
    uint32    mapid;                                        // 3        m_ContinentID
    float     x;                                            // 4        m_LocX
    float     y;                                            // 5        m_LocY
    float     z;                                            // 6        m_LocZ
    uint32    actionFlag;                                   // 7        m_flags
    uint32    delay;                                        // 8        m_delay
    uint32    arrivalEventID;                               // 9        m_arrivalEventID
    uint32    departureEventID;                             // 10       m_departureEventID
};

struct TeamContributionPoints
{
    //uint32    Entry;                                      // 0        m_ID
    float     Value;                                        // 1        m_data
};

struct TotemCategoryEntry
{
    uint32    ID;                                           // 0        m_ID
    //char*   name[16];                                     // 1-16     m_name_lang
                                                            // 17 string flags
    uint32    categoryType;                                 // 18       m_totemCategoryType (one for specialization)
    uint32    categoryMask;                                 // 19       m_totemCategoryMask (compatibility mask for same type: different for totems, compatible from high to low for rods)
};

#define MAX_VEHICLE_SEAT 8

struct VehicleEntry
{
    uint32  m_ID;                                           // 0
    uint32  m_flags;                                        // 1
    float   m_turnSpeed;                                    // 2
    float   m_pitchSpeed;                                   // 3
    float   m_pitchMin;                                     // 4
    float   m_pitchMax;                                     // 5
    uint32  m_seatID[MAX_VEHICLE_SEAT];                     // 6-13
    float   m_mouseLookOffsetPitch;                         // 14
    float   m_cameraFadeDistScalarMin;                      // 15
    float   m_cameraFadeDistScalarMax;                      // 16
    float   m_cameraPitchOffset;                            // 17
    float   m_facingLimitRight;                             // 18
    float   m_facingLimitLeft;                              // 19
    float   m_msslTrgtTurnLingering;                        // 20
    float   m_msslTrgtPitchLingering;                       // 21
    float   m_msslTrgtMouseLingering;                       // 22
    float   m_msslTrgtEndOpacity;                           // 23
    float   m_msslTrgtArcSpeed;                             // 24
    float   m_msslTrgtArcRepeat;                            // 25
    float   m_msslTrgtArcWidth;                             // 26
    float   m_msslTrgtImpactRadius[2];                      // 27-28
    char*   m_msslTrgtArcTexture;                           // 29
    char*   m_msslTrgtImpactTexture;                        // 30
    char*   m_msslTrgtImpactModel[2];                       // 31-32
    float   m_cameraYawOffset;                              // 33
    uint32  m_uiLocomotionType;                             // 34
    float   m_msslTrgtImpactTexRadius;                      // 35
    uint32  m_uiSeatIndicatorType;                          // 36       m_vehicleUIIndicatorID
                                                            // 37       m_powerDisplayID
                                                            // 38 new in 3.1
                                                            // 39 new in 3.1
};

struct VehicleSeatEntry
{
    uint32  m_ID;                                           // 0
    uint32  m_flags;                                        // 1
    int32   m_attachmentID;                                 // 2
    float   m_attachmentOffsetX;                            // 3
    float   m_attachmentOffsetY;                            // 4
    float   m_attachmentOffsetZ;                            // 5
    float   m_enterPreDelay;                                // 6
    float   m_enterSpeed;                                   // 7
    float   m_enterGravity;                                 // 8
    float   m_enterMinDuration;                             // 9
    float   m_enterMaxDuration;                             // 10
    float   m_enterMinArcHeight;                            // 11
    float   m_enterMaxArcHeight;                            // 12
    int32   m_enterAnimStart;                               // 13
    int32   m_enterAnimLoop;                                // 14
    int32   m_rideAnimStart;                                // 15
    int32   m_rideAnimLoop;                                 // 16
    int32   m_rideUpperAnimStart;                           // 17
    int32   m_rideUpperAnimLoop;                            // 18
    float   m_exitPreDelay;                                 // 19
    float   m_exitSpeed;                                    // 20
    float   m_exitGravity;                                  // 21
    float   m_exitMinDuration;                              // 22
    float   m_exitMaxDuration;                              // 23
    float   m_exitMinArcHeight;                             // 24
    float   m_exitMaxArcHeight;                             // 25
    int32   m_exitAnimStart;                                // 26
    int32   m_exitAnimLoop;                                 // 27
    int32   m_exitAnimEnd;                                  // 28
    float   m_passengerYaw;                                 // 29
    float   m_passengerPitch;                               // 30
    float   m_passengerRoll;                                // 31
    int32   m_passengerAttachmentID;                        // 32
    int32   m_vehicleEnterAnim;                             // 33
    int32   m_vehicleExitAnim;                              // 34
    int32   m_vehicleRideAnimLoop;                          // 35
    int32   m_vehicleEnterAnimBone;                         // 36
    int32   m_vehicleExitAnimBone;                          // 37
    int32   m_vehicleRideAnimLoopBone;                      // 38
    float   m_vehicleEnterAnimDelay;                        // 39
    float   m_vehicleExitAnimDelay;                         // 40
    uint32  m_vehicleAbilityDisplay;                        // 41
    uint32  m_enterUISoundID;                               // 42
    uint32  m_exitUISoundID;                                // 43
    int32   m_uiSkin;                                       // 44
    uint32  m_flagsB;                                       // 45
                                                            // 46       m_cameraEnteringDelay
                                                            // 47       m_cameraEnteringDuration
                                                            // 48       m_cameraExitingDelay
                                                            // 49       m_cameraExitingDuration
                                                            // 50       m_cameraOffsetX
                                                            // 51       m_cameraOffsetY
                                                            // 52       m_cameraOffsetZ
                                                            // 53       m_cameraPosChaseRate
                                                            // 54       m_cameraFacingChaseRate
                                                            // 55       m_cameraEnteringZoom"
                                                            // 56       m_cameraSeatZoomMin
                                                            // 57       m_cameraSeatZoomMax
};

struct WMOAreaTableEntry
{
    uint32 Id;                                              // 0        m_ID index
    int32 rootId;                                           // 1        m_WMOID used in root WMO
    int32 adtId;                                            // 2        m_NameSetID used in adt file
    int32 groupId;                                          // 3        m_WMOGroupID used in group WMO
    //uint32 field4;                                        // 4        m_SoundProviderPref
    //uint32 field5;                                        // 5        m_SoundProviderPrefUnderwater
    //uint32 field6;                                        // 6        m_AmbienceID
    //uint32 field7;                                        // 7        m_ZoneMusic
    //uint32 field8;                                        // 8        m_IntroSound
    uint32 Flags;                                           // 9        m_flags (used for indoor/outdoor determination)
    uint32 areaId;                                          // 10       m_AreaTableID (AreaTable.dbc)
    //char *Name[16];                                       //          m_AreaName_lang
    //uint32 nameFlags;
};

struct WorldMapAreaEntry
{
    //uint32  ID;                                           // 0        m_ID
    uint32  map_id;                                         // 1        m_mapID
    uint32  area_id;                                        // 2        m_areaID index (continent 0 areas ignored)
    //char* internal_name                                   // 3        m_areaName
    float   y1;                                             // 4        m_locLeft
    float   y2;                                             // 5        m_locRight
    float   x1;                                             // 6        m_locTop
    float   x2;                                             // 7        m_locBottom
    int32   virtual_map_id;                                 // 8        m_displayMapID -1 (map_id have correct map) other: virtual map where zone show (map_id - where zone in fact internally)
    // int32   dungeonMap_id;                               // 9        m_defaultDungeonFloor (DungeonMap.dbc)
    // uint32  someMapID;                                   // 10       m_parentWorldMapID
};

#define MAX_WORLD_MAP_OVERLAY_AREA_IDX 4

struct WorldMapOverlayEntry
{
    uint32    ID;                                           // 0        m_ID
    //uint32    worldMapAreaId;                             // 1        m_mapAreaID (WorldMapArea.dbc)
    uint32    areatableID[MAX_WORLD_MAP_OVERLAY_AREA_IDX];  // 2-5      m_areaID
                                                            // 6        m_mapPointX
                                                            // 7        m_mapPointY
    //char* internal_name                                   // 8        m_textureName
                                                            // 9        m_textureWidth
                                                            // 10       m_textureHeight
                                                            // 11       m_offsetX
                                                            // 12       m_offsetY
                                                            // 13       m_hitRectTop
                                                            // 14       m_hitRectLeft
                                                            // 15       m_hitRectBottom
                                                            // 16       m_hitRectRight
};

struct WorldSafeLocsEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    map_id;                                       // 1        m_continent
    float     x;                                            // 2        m_locX
    float     y;                                            // 3        m_locY
    float     z;                                            // 4        m_locZ
    //char*   name[16]                                      // 5-20     m_AreaName_lang
                                                            // 21 string flags
};

// GCC have alternative #pragma pack() syntax and old gcc version not support pack(pop), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

typedef std::set<uint32> SpellCategorySet;
typedef std::map<uint32,SpellCategorySet > SpellCategoryStore;
typedef std::set<uint32> PetFamilySpellsSet;
typedef std::map<uint32,PetFamilySpellsSet > PetFamilySpellsStore;

// Structures not used for casting to loaded DBC data and not required then packing
struct MapDifficulty
{
    MapDifficulty() : resetTime(0), maxPlayers(0) {}
    MapDifficulty(uint32 _resetTime, uint32 _maxPlayers) : resetTime(_resetTime), maxPlayers(_maxPlayers) {}

    uint32 resetTime;                                       // in secs, 0 if no fixed reset time
    uint32 maxPlayers;                                      // some heroic dungeons have 0 when expect same value as in normal dificulty case
};

struct TalentSpellPos
{
    TalentSpellPos() : talent_id(0), rank(0) {}
    TalentSpellPos(uint16 _talent_id, uint8 _rank) : talent_id(_talent_id), rank(_rank) {}

    uint16 talent_id;
    uint8  rank;
};

typedef std::map<uint32,TalentSpellPos> TalentSpellPosMap;

struct TaxiPathBySourceAndDestination
{
    TaxiPathBySourceAndDestination() : ID(0),price(0) {}
    TaxiPathBySourceAndDestination(uint32 _id,uint32 _price) : ID(_id),price(_price) {}

    uint32    ID;
    uint32    price;
};
typedef std::map<uint32,TaxiPathBySourceAndDestination> TaxiPathSetForSource;
typedef std::map<uint32,TaxiPathSetForSource> TaxiPathSetBySource;

struct TaxiPathNodePtr
{
    TaxiPathNodePtr() : i_ptr(NULL) {}
    TaxiPathNodePtr(TaxiPathNodeEntry const* ptr) : i_ptr(ptr) {}

    TaxiPathNodeEntry const* i_ptr;

    operator TaxiPathNodeEntry const& () const { return *i_ptr; }
};

typedef Path<TaxiPathNodePtr,TaxiPathNodeEntry const> TaxiPathNodeList;
typedef std::vector<TaxiPathNodeList> TaxiPathNodesByPath;

#define TaxiMaskSize 14
typedef uint32 TaxiMask[TaxiMaskSize];
#endif
