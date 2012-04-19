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

#ifndef DBCENUMS_H
#define DBCENUMS_H

// Client expected level limitation, like as used in DBC item max levels for "until max player level"
// use as default max player level, must be fit max level for used client
// also see MAX_LEVEL and STRONG_MAX_LEVEL define
#define DEFAULT_MAX_LEVEL 80

// client supported max level for player/pets/etc. Avoid overflow or client stability affected.
// also see GT_MAX_LEVEL define
#define MAX_LEVEL    100

// Server side limitation. Base at used code requirements.
// also see MAX_LEVEL and GT_MAX_LEVEL define
#define STRONG_MAX_LEVEL 255

enum BattleGroundBracketId                                  // bracketId for level ranges
{
    BG_BRACKET_ID_TEMPLATE       = -1,                      // used to mark bg as template
    BG_BRACKET_ID_FIRST          = 0,
    BG_BRACKET_ID_LAST           = 15
};

// must be max value in PvPDificulty slot+1
#define MAX_BATTLEGROUND_BRACKETS  16

enum AreaTeams
{
    AREATEAM_NONE  = 0,
    AREATEAM_ALLY  = 2,
    AREATEAM_HORDE = 4
};

enum AchievementFactionFlags
{
    ACHIEVEMENT_FACTION_FLAG_HORDE    = 0x00000000,
    ACHIEVEMENT_FACTION_FLAG_ALLIANCE = 0x00000001,
};

enum AchievementFlags
{
    ACHIEVEMENT_FLAG_NONE                   = 0x00000000,
    ACHIEVEMENT_FLAG_COUNTER                = 0x00000001,   // ACHIEVEMENT_FLAG_STATISTIC Just count statistic (never stop and complete)
    ACHIEVEMENT_FLAG_UNK2                   = 0x00000002,   // ACHIEVEMENT_FLAG_HIDDEN not used
    ACHIEVEMENT_FLAG_STORE_MAX_VALUE        = 0x00000004,   // ACHIEVEMENT_FLAG_HIDDEN_TILL_AWARDED Store only max value? used only in "Reach level xx"
    ACHIEVEMENT_FLAG_SUMM                   = 0x00000008,   // ACHIEVEMENT_FLAG_CUMULATIVE Use summ criteria value from all requirements (and calculate max value)
    ACHIEVEMENT_FLAG_MAX_USED               = 0x00000010,   // ACHIEVEMENT_FLAG_DISPLAY_HIGHEST Show max criteria (and calculate max value ??)
    ACHIEVEMENT_FLAG_REQ_COUNT              = 0x00000020,   // ACHIEVEMENT_FLAG_CRITERIA_COUNT Use not zero req count (and calculate max value)
    ACHIEVEMENT_FLAG_AVERAGE                = 0x00000040,   // ACHIEVEMENT_FLAG_AVG_PER_DAY Show as average value (value / time_in_days) depend from other flag (by def use last criteria value)
    ACHIEVEMENT_FLAG_BAR                    = 0x00000080,   // ACHIEVEMENT_FLAG_HAS_PROGRESS_BAR Show as progress bar (value / max vale) depend from other flag (by def use last criteria value)
    ACHIEVEMENT_FLAG_REALM_FIRST_REACH      = 0x00000100,   // ACHIEVEMENT_FLAG_SERVER_FIRST
    ACHIEVEMENT_FLAG_REALM_FIRST_KILL       = 0x00000200,   //

};

enum AchievementCriteriaCondition
{
    ACHIEVEMENT_CRITERIA_CONDITION_NONE      = 0,
    ACHIEVEMENT_CRITERIA_CONDITION_NO_DEATH  = 1,
    ACHIEVEMENT_CRITERIA_CONDITION_UNK1      = 2,           // only used in "Complete a daily quest every day for five consecutive days"
    ACHIEVEMENT_CRITERIA_CONDITION_MAP       = 3,           // requires you to be on specific map
    ACHIEVEMENT_CRITERIA_CONDITION_NO_LOOSE  = 4,           // only used in "Win 10 arenas without losing"
    ACHIEVEMENT_CRITERIA_CONDITION_UNK2      = 9,           // unk
    ACHIEVEMENT_CRITERIA_CONDITION_UNK3      = 13,          // unk
};

enum AchievementCriteriaCompletionFlags
{
    ACHIEVEMENT_CRITERIA_FLAG_PROGRESS_BAR      = 0x00000001,         // Show progress as bar
    ACHIEVEMENT_CRITERIA_FLAG_HIDDEN            = 0x00000002,         // Not show criteria in client
    ACHIEVEMENT_CRITERIA_FLAG_FAIL_ACHIEVEMENT  = 0x00000004,         // BG related??
    ACHIEVEMENT_CRITERIA_FLAG_RESET_ON_START    = 0x00000008,         //
    ACHIEVEMENT_CRITERIA_FLAG_IS_DATE           = 0x00000010,         // not used
    ACHIEVEMENT_CRITERIA_FLAG_IS_MONEY          = 0x00000020,         // Displays counter as money
    ACHIEVEMENT_CRITERIA_FLAG_IS_ACHIEVEMENT_ID = 0x00000040,
};

enum AchievementCriteriaTypes
{
    ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE = 0,
    ACHIEVEMENT_CRITERIA_TYPE_WIN_BG = 1,
    ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL = 5,
    ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL = 7,
    ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT = 8,
    ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT = 9,
    // you have to complete a daily quest x times in a row
    ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY = 10,
    ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE = 11,
    ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE = 13,
    ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST = 14,
    ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND= 15,
    ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP= 16,
    ACHIEVEMENT_CRITERIA_TYPE_DEATH= 17,
    ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON = 18,
    ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID = 19,
    ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE = 20,
    ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER = 23,
    ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING = 24,
    ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM = 26,
    ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST = 27,
    ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET = 28,
    ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL= 29,
    ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE = 30,
    ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA = 31,
    ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA = 32,
    ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA = 33,
    ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL = 34,
    // TODO: this criteria has additional conditions which can not be found in the dbcs
    ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL = 35,
    ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM = 36,
    // TODO: the archievements 1162 and 1163 requires a special rating which can't be found in the dbc
    ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA = 37,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING = 38,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING = 39,
    ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL = 40,
    ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM = 41,
    ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM= 42,
    ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA = 43,
    ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK= 44,
    ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT= 45,
    ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION= 46,
    ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION= 47,
    // noted: rewarded as soon as the player payed, not at taking place at the seat
    ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP= 48,
    ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM = 49,
    // TODO: itemlevel is mentioned in text but not present in dbc
    ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT = 50,
    ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT= 51,
    ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS = 52,
    ACHIEVEMENT_CRITERIA_TYPE_HK_RACE = 53,
    ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE = 54,
    ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE = 55,
    // TODO: in some cases map not present, and in some cases need do without die
    ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS = 56,
    ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM = 57,
    ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS = 59,
    ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS = 60,
    ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS = 61,
    ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD = 62,
    ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING = 63,
    ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER = 65,
    ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL = 66,
    ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY = 67,
    ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT = 68,
    ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2= 69,
    ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL= 70,
    ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT = 72,
    // TODO: title id is not mentioned in dbc
    ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN = 74,
    ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS= 75,
    ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL = 76,
    ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL = 77,
    // TODO: creature type (demon, undead etc.) is not stored in dbc
    ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE = 78,
    ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS= 80,
    ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION= 82,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID= 83,
    ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS= 84,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD = 85,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED = 86,
    ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION = 87,
    ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION = 88,
    ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS = 89,
    ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM = 90,
    ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM = 91,
    ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED = 93,
    ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED = 94,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALTH = 95,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_POWER = 96,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_STAT = 97,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELLPOWER = 98,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_ARMOR = 99,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_RATING = 100,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_DEALT = 101,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_RECEIVED = 102,
    ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED = 103,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEAL_CASTED = 104,
    ACHIEVEMENT_CRITERIA_TYPE_TOTAL_HEALING_RECEIVED = 105,
    ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED = 106,
    ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED = 107,
    ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN = 108,
    ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE = 109,
    // TODO: target entry is missing
    ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2 = 110,
    ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE= 112,
    ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL = 113,
    ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS = 114,
    ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS = 115,
    ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS = 119,
    // 120 unused
    // 121 unused
    // 122 unused
    // 123 unused
    // 0..123 => 124 criteria types total
};

#define ACHIEVEMENT_CRITERIA_TYPE_TOTAL 124

enum AreaFlags
{
    AREA_FLAG_SNOW                  = 0x00000001,           // snow (only Dun Morogh, Naxxramas, Razorfen Downs and Winterspring)
    AREA_FLAG_UNK1                  = 0x00000002,           // may be necropolis?
    AREA_FLAG_UNK2                  = 0x00000004,           // Only used for areas on map 571 (development before)
    AREA_FLAG_SLAVE_CAPITAL         = 0x00000008,           // city and city subsones
    AREA_FLAG_UNK3                  = 0x00000010,           // can't find common meaning
    AREA_FLAG_SLAVE_CAPITAL2        = 0x00000020,           // slave capital city flag?
    AREA_FLAG_DUEL                  = 0x00000040,           // zones where duels allowed
    AREA_FLAG_ARENA                 = 0x00000080,           // arena, both instanced and world arenas
    AREA_FLAG_CAPITAL               = 0x00000100,           // main capital city flag
    AREA_FLAG_CITY                  = 0x00000200,           // only for one zone named "City" (where it located?)
    AREA_FLAG_OUTLAND               = 0x00000400,           // expansion zones? (only Eye of the Storm not have this flag, but have 0x00004000 flag)
    AREA_FLAG_SANCTUARY             = 0x00000800,           // sanctuary area (PvP disabled)
    AREA_FLAG_NEED_FLY              = 0x00001000,           // only Netherwing Ledge, Socrethar's Seat, Tempest Keep, The Arcatraz, The Botanica, The Mechanar, Sorrow Wing Point, Dragonspine Ridge, Netherwing Mines, Dragonmaw Base Camp, Dragonmaw Skyway
    AREA_FLAG_UNUSED1               = 0x00002000,           // not used now (no area/zones with this flag set in 3.0.3)
    AREA_FLAG_OUTLAND2              = 0x00004000,           // expansion zones? (only Circle of Blood Arena not have this flag, but have 0x00000400 flag)
    AREA_FLAG_PVP                   = 0x00008000,           // pvp objective area? (Death's Door also has this flag although it's no pvp object area)
    AREA_FLAG_ARENA_INSTANCE        = 0x00010000,           // used by instanced arenas only
    AREA_FLAG_UNUSED2               = 0x00020000,           // not used now (no area/zones with this flag set in 3.0.3)
    AREA_FLAG_UNK5                  = 0x00040000,           // only used for Amani Pass, Hatchet Hills
    AREA_FLAG_UNK6                  = 0x00080000,           // Valgarde and Acherus: The Ebon Hold
    AREA_FLAG_LOWLEVEL              = 0x00100000,           // used for some starting areas with area_level <=15
    AREA_FLAG_TOWN                  = 0x00200000,           // small towns with Inn
    AREA_FLAG_UNK7                  = 0x00400000,           // Warsong Hold, Acherus: The Ebon Hold, New Agamand Inn, Vengeance Landing Inn
    AREA_FLAG_UNK8                  = 0x00800000,           // Westguard Inn, Acherus: The Ebon Hold, Valgarde
    AREA_FLAG_OUTDOOR_PVP           = 0x01000000,           // Wintergrasp and it's subzones
    AREA_FLAG_INSIDE                = 0x02000000,           // used for determinating spell related inside/outside questions in Map::IsOutdoors
    AREA_FLAG_OUTSIDE               = 0x04000000,           // used for determinating spell related inside/outside questions in Map::IsOutdoors
    AREA_FLAG_CAN_HEARTH_AND_RES    = 0x08000000,           // Wintergrasp and it's subzones
    AREA_FLAG_CANNOT_FLY            = 0x20000000            // not allowed to fly, only used in Dalaran areas (zone 4395)
};

enum Difficulty
{
    REGULAR_DIFFICULTY           = 0,

    DUNGEON_DIFFICULTY_NORMAL    = 0,
    DUNGEON_DIFFICULTY_HEROIC    = 1,
    //DUNGEON_DIFFICULTY_EPIC    = 2,                       // not used, but exists

    RAID_DIFFICULTY_10MAN_NORMAL = 0,
    RAID_DIFFICULTY_25MAN_NORMAL = 1,
    RAID_DIFFICULTY_10MAN_HEROIC = 2,
    RAID_DIFFICULTY_25MAN_HEROIC = 3,
};

#define MAX_DUNGEON_DIFFICULTY     2
#define MAX_RAID_DIFFICULTY        4
#define MAX_DIFFICULTY             4

enum SpawnMask
{
    SPAWNMASK_REGULAR           = (1 << REGULAR_DIFFICULTY),// any maps without spawn modes (continents/subway) or in minimal spawnmode

    SPAWNMASK_DUNGEON_NORMAL    = (1 << DUNGEON_DIFFICULTY_NORMAL),
    SPAWNMASK_DUNGEON_HEROIC    = (1 << DUNGEON_DIFFICULTY_HEROIC),
    SPAWNMASK_DUNGEON_ALL       = (SPAWNMASK_DUNGEON_NORMAL | SPAWNMASK_DUNGEON_HEROIC),

    SPAWNMASK_RAID_10MAN_NORMAL = (1 << RAID_DIFFICULTY_10MAN_NORMAL),
    SPAWNMASK_RAID_25MAN_NORMAL = (1 << RAID_DIFFICULTY_25MAN_NORMAL),
    SPAWNMASK_RAID_NORMAL_ALL   = (SPAWNMASK_RAID_10MAN_NORMAL | SPAWNMASK_RAID_25MAN_NORMAL),

    SPAWNMASK_RAID_10MAN_HEROIC = (1 << RAID_DIFFICULTY_10MAN_HEROIC),
    SPAWNMASK_RAID_25MAN_HEROIC = (1 << RAID_DIFFICULTY_25MAN_HEROIC),
    SPAWNMASK_RAID_HEROIC_ALL   = (SPAWNMASK_RAID_10MAN_HEROIC | SPAWNMASK_RAID_25MAN_HEROIC),

    SPAWNMASK_RAID_ALL          = (SPAWNMASK_RAID_NORMAL_ALL | SPAWNMASK_RAID_HEROIC_ALL),
};

enum FactionTemplateFlags
{
    FACTION_TEMPLATE_FLAG_PVP               = 0x00000800,   // flagged for PvP
    FACTION_TEMPLATE_FLAG_CONTESTED_GUARD   = 0x00001000,   // faction will attack players that were involved in PvP combats
};

enum FactionMasks
{
    FACTION_MASK_PLAYER   = 1,                              // any player
    FACTION_MASK_ALLIANCE = 2,                              // player or creature from alliance team
    FACTION_MASK_HORDE    = 4,                              // player or creature from horde team
    FACTION_MASK_MONSTER  = 8                               // aggressive creature from monster team
    // if none flags set then non-aggressive creature
};

enum MapTypes                                               // Lua_IsInInstance
{
    MAP_COMMON          = 0,                                // none
    MAP_INSTANCE        = 1,                                // party
    MAP_RAID            = 2,                                // raid
    MAP_BATTLEGROUND    = 3,                                // pvp
    MAP_ARENA           = 4                                 // arena
};

enum AbilytyLearnType
{
    ABILITY_LEARNED_ON_GET_PROFESSION_SKILL     = 1,
    ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL  = 2
};

enum AbilitySkillFlags
{
    ABILITY_SKILL_NONTRAINABLE = 0x100
};

enum ItemEnchantmentType
{
    ITEM_ENCHANTMENT_TYPE_NONE             = 0,
    ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL     = 1,
    ITEM_ENCHANTMENT_TYPE_DAMAGE           = 2,
    ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL      = 3,
    ITEM_ENCHANTMENT_TYPE_RESISTANCE       = 4,
    ITEM_ENCHANTMENT_TYPE_STAT             = 5,
    ITEM_ENCHANTMENT_TYPE_TOTEM            = 6,
    ITEM_ENCHANTMENT_TYPE_USE_SPELL        = 7,
    ITEM_ENCHANTMENT_TYPE_PRISMATIC_SOCKET = 8
};

enum ItemLimitCategoryMode
{
    ITEM_LIMIT_CATEGORY_MODE_HAVE  = 0,                      // limit applied to amount items in inventory/bank
    ITEM_LIMIT_CATEGORY_MODE_EQUIP = 1,                      // limit applied to amount equipped items (including used gems)
};

// some used in code cases
enum ItemLimitCategory
{
    ITEM_LIMIT_CATEGORY_MANA_GEM   = 4,
};

enum TotemCategoryType
{
    TOTEM_CATEGORY_TYPE_KNIFE   = 1,
    TOTEM_CATEGORY_TYPE_TOTEM   = 2,
    TOTEM_CATEGORY_TYPE_ROD     = 3,
    TOTEM_CATEGORY_TYPE_PICK    = 21,
    TOTEM_CATEGORY_TYPE_STONE   = 22,
    TOTEM_CATEGORY_TYPE_HAMMER  = 23,
    TOTEM_CATEGORY_TYPE_SPANNER = 24
};

// SummonProperties.dbc, col 1
enum SummonPropGroup
{
    SUMMON_PROP_GROUP_WILD           = 0,
    SUMMON_PROP_GROUP_FRIENDLY       = 1,
    SUMMON_PROP_GROUP_PETS           = 2,
    SUMMON_PROP_GROUP_CONTROLLABLE   = 3,
    SUMMON_PROP_GROUP_VEHICLE        = 4
};

// SummonProperties.dbc, col 3
enum UnitNameSummonTitle
{
    UNITNAME_SUMMON_TITLE_NONE         = 0,                 // no default title, different summons, 1330 spells in 3.0.3
    UNITNAME_SUMMON_TITLE_PET          = 1,                 // 's Pet,           generic summons, 49 spells in 3.0.3
    UNITNAME_SUMMON_TITLE_GUARDIAN     = 2,                 // 's Guardian,      summon guardian, 393 spells in 3.0.3
    UNITNAME_SUMMON_TITLE_MINION       = 3,                 // 's Minion,        summon army, 5 spells in 3.0.3
    UNITNAME_SUMMON_TITLE_TOTEM        = 4,                 // 's Totem,         summon totem, 169 spells in 3.0.3
    UNITNAME_SUMMON_TITLE_COMPANION    = 5,                 // 's Companion,     critter/minipet, 195 spells in 3.0.3
    UNITNAME_SUMMON_TITLE_RUNEBLADE    = 6,                 // 's Runeblade,     summon DRW/Ghoul, 2 spells in 3.0.3"
    UNITNAME_SUMMON_TITLE_CONSTRUCT    = 7,                 // 's Construct,     summon bot/bomb, 4 spells in 3.0.3
    UNITNAME_SUMMON_TITLE_OPPONENT     = 8,                 // 's Opponent,      something todo with DK prequest line, 2 spells in 3.0.3
    UNITNAME_SUMMON_TITLE_VEHICLE      = 9,                 // 's Vehicle,       summon different vehicles, 14 spells in 3.0.3
    UNITNAME_SUMMON_TITLE_MOUNT        = 10,                // 's Mount,         summon drake (vehicle), 3 spells
    UNITNAME_SUMMON_TITLE_LIGHTWELL    = 11,                // 's Lightwell,     summon lightwell, 6 spells in 3.0.3
    UNITNAME_SUMMON_TITLE_BUTLER       = 12                 // 's Butler,        summon repair bot, 1 spells in 3.2.2a
};

// SummonProperties.dbc, col 5
enum SummonPropFlags
{
    SUMMON_PROP_FLAG_NONE              = 0x0000,            // 1342 spells in 3.0.3
    SUMMON_PROP_FLAG_UNK1              = 0x0001,            // 75 spells in 3.0.3, something unfriendly
    SUMMON_PROP_FLAG_UNK2              = 0x0002,            // 616 spells in 3.0.3, something friendly
    SUMMON_PROP_FLAG_UNK3              = 0x0004,            // 22 spells in 3.0.3, no idea...
    SUMMON_PROP_FLAG_UNK4              = 0x0008,            // 49 spells in 3.0.3, some mounts
    SUMMON_PROP_FLAG_UNK5              = 0x0010,            // 25 spells in 3.0.3, quest related?
    SUMMON_PROP_FLAG_CANT_BE_DISMISSED = 0x0020,            // 0 spells in 3.0.3, unused
    SUMMON_PROP_FLAG_UNK7              = 0x0040,            // 12 spells in 3.0.3, no idea
    SUMMON_PROP_FLAG_UNK8              = 0x0080,            // 4 spells in 3.0.3, no idea
    SUMMON_PROP_FLAG_UNK9              = 0x0100,            // 51 spells in 3.0.3, no idea, many quest related
    SUMMON_PROP_FLAG_UNK10             = 0x0200,            // 51 spells in 3.0.3, something defensive
    SUMMON_PROP_FLAG_UNK11             = 0x0400,            // 3 spells, requires something near?
    SUMMON_PROP_FLAG_UNK12             = 0x0800,            // 30 spells in 3.0.3, no idea
    SUMMON_PROP_FLAG_UNK13             = 0x1000,            // 8 spells in 3.0.3, siege vehicle
    SUMMON_PROP_FLAG_UNK14             = 0x2000,            // 2 spells in 3.0.3, escort?
};

// SpellEntry::Targets
enum SpellCastTargetFlags
{
    TARGET_FLAG_SELF            = 0x00000000,
    TARGET_FLAG_UNUSED1         = 0x00000001,               // not used in any spells as of 3.0.3 (can be set dynamically)
    TARGET_FLAG_UNIT            = 0x00000002,               // pguid
    TARGET_FLAG_UNUSED2         = 0x00000004,               // not used in any spells as of 3.0.3 (can be set dynamically)
    TARGET_FLAG_UNUSED3         = 0x00000008,               // not used in any spells as of 3.0.3 (can be set dynamically)
    TARGET_FLAG_ITEM            = 0x00000010,               // pguid
    TARGET_FLAG_SOURCE_LOCATION = 0x00000020,               // pguid + 3 float
    TARGET_FLAG_DEST_LOCATION   = 0x00000040,               // pguid + 3 float
    TARGET_FLAG_OBJECT_UNK      = 0x00000080,               // used in 7 spells only
    TARGET_FLAG_UNIT_UNK        = 0x00000100,               // looks like self target (480 spells)
    TARGET_FLAG_PVP_CORPSE      = 0x00000200,               // pguid
    TARGET_FLAG_UNIT_CORPSE     = 0x00000400,               // 10 spells (gathering professions)
    TARGET_FLAG_OBJECT          = 0x00000800,               // pguid, 2 spells
    TARGET_FLAG_TRADE_ITEM      = 0x00001000,               // pguid, 0 spells
    TARGET_FLAG_STRING          = 0x00002000,               // string, 0 spells
    TARGET_FLAG_UNK1            = 0x00004000,               // 199 spells, opening object/lock
    TARGET_FLAG_CORPSE          = 0x00008000,               // pguid, resurrection spells
    TARGET_FLAG_UNK2            = 0x00010000,               // pguid, not used in any spells as of 3.0.3 (can be set dynamically)
    TARGET_FLAG_GLYPH           = 0x00020000,               // used in glyph spells
    TARGET_FLAG_UNK3            = 0x00040000,               //
    TARGET_FLAG_VISUAL_CHAIN    = 0x00080000                // uint32, loop { vec3, guid -> if guid == 0 break }
};

enum SpellEffectIndex
{
    EFFECT_INDEX_0     = 0,
    EFFECT_INDEX_1     = 1,
    EFFECT_INDEX_2     = 2
};

#define MAX_EFFECT_INDEX 3

enum SpellFamily
{
    SPELLFAMILY_GENERIC     = 0,
    SPELLFAMILY_UNK1        = 1,                            // events, holidays
    // 2 - unused
    SPELLFAMILY_MAGE        = 3,
    SPELLFAMILY_WARRIOR     = 4,
    SPELLFAMILY_WARLOCK     = 5,
    SPELLFAMILY_PRIEST      = 6,
    SPELLFAMILY_DRUID       = 7,
    SPELLFAMILY_ROGUE       = 8,
    SPELLFAMILY_HUNTER      = 9,
    SPELLFAMILY_PALADIN     = 10,
    SPELLFAMILY_SHAMAN      = 11,
    SPELLFAMILY_UNK2        = 12,                           // 2 spells (silence resistance)
    SPELLFAMILY_POTION      = 13,
    // 14 - unused
    SPELLFAMILY_DEATHKNIGHT = 15,
    // 16 - unused
    SPELLFAMILY_PET         = 17
};

enum VehicleFlags
{
    VEHICLE_FLAG_NO_STRAFE          = 0x00000001,           // Sets MOVEFLAG2_NO_STRAFE
    VEHICLE_FLAG_NO_JUMPING         = 0x00000002,           // Sets MOVEFLAG2_NO_JUMPING
    VEHICLE_FLAG_FULLSPEEDTURNING   = 0x00000004,           // Sets MOVEFLAG2_FULLSPEEDTURNING
    VEHICLE_FLAG_ALLOW_PITCHING     = 0x00000010,           // Sets MOVEFLAG2_ALLOW_PITCHING
    VEHICLE_FLAG_FULLSPEEDPITCHING  = 0x00000020,           // Sets MOVEFLAG2_FULLSPEEDPITCHING
    VEHICLE_FLAG_CUSTOM_PITCH       = 0x00000040,           // If set use pitchMin and pitchMax from DBC, otherwise pitchMin = -pi/2, pitchMax = pi/2
    VEHICLE_FLAG_ADJUST_AIM_ANGLE   = 0x00000400,           // Lua_IsVehicleAimAngleAdjustable
    VEHICLE_FLAG_ADJUST_AIM_POWER   = 0x00000800,           // Lua_IsVehicleAimPowerAdjustable
};

enum VehicleSeatFlags
{
    SEAT_FLAG_HIDE_PASSENGER        = 0x00000200,           // Passenger is hidden
    SEAT_FLAG_CAN_CONTROL           = 0x00000800,           // Lua_UnitInVehicleControlSeat
    SEAT_FLAG_CAN_ATTACK            = 0x00004000,           // Can attack, cast spells and use items from vehicle?
    SEAT_FLAG_USABLE                = 0x02000000,           // Lua_CanExitVehicle
    SEAT_FLAG_CAN_SWITCH            = 0x04000000,           // Lua_CanSwitchVehicleSeats
    SEAT_FLAG_CAN_CAST              = 0x20000000,           // Lua_UnitHasVehicleUI
};

#endif
