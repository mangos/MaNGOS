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

#ifndef _OBJECTMGR_H
#define _OBJECTMGR_H

#include "Common.h"
#include "Log.h"
#include "Object.h"
#include "Bag.h"
#include "Creature.h"
#include "Player.h"
#include "GameObject.h"
#include "Corpse.h"
#include "QuestDef.h"
#include "ItemPrototype.h"
#include "NPCHandler.h"
#include "Database/DatabaseEnv.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "Policies/Singleton.h"
#include "Vehicle.h"
#include "SQLStorages.h"

#include <string>
#include <map>
#include <limits>

class Group;
class Guild;
class ArenaTeam;
class Item;

struct GameTele
{
    float  position_x;
    float  position_y;
    float  position_z;
    float  orientation;
    uint32 mapId;
    std::string name;
    std::wstring wnameLow;
};

typedef UNORDERED_MAP<uint32, GameTele > GameTeleMap;

struct SpellClickInfo
{
    uint32 spellId;
    uint32 questStart;                                      // quest start (quest must be active or rewarded for spell apply)
    uint32 questEnd;                                        // quest end (quest don't must be rewarded for spell apply)
    bool   questStartCanActive;                             // if true then quest start can be active (not only rewarded)
    uint8 castFlags;

    // helpers
    bool IsFitToRequirements(Player const* player) const;
};

typedef std::multimap<uint32, SpellClickInfo> SpellClickInfoMap;
typedef std::pair<SpellClickInfoMap::const_iterator,SpellClickInfoMap::const_iterator> SpellClickInfoMapBounds;

struct AreaTrigger
{
    uint8  requiredLevel;
    uint32 requiredItem;
    uint32 requiredItem2;
    uint32 heroicKey;
    uint32 heroicKey2;
    uint32 requiredQuest;
    uint32 requiredQuestHeroic;
    std::string requiredFailedText;
    uint32 target_mapId;
    float  target_X;
    float  target_Y;
    float  target_Z;
    float  target_Orientation;
};

typedef std::set<uint32> CellGuidSet;
typedef std::map<uint32/*player guid*/,uint32/*instance*/> CellCorpseSet;
struct CellObjectGuids
{
    CellGuidSet creatures;
    CellGuidSet gameobjects;
    CellCorpseSet corpses;
};
typedef UNORDERED_MAP<uint32/*cell_id*/,CellObjectGuids> CellObjectGuidsMap;
typedef UNORDERED_MAP<uint32/*(mapid,spawnMode) pair*/,CellObjectGuidsMap> MapObjectGuids;

typedef UNORDERED_MAP<uint64/*(instance,guid) pair*/,time_t> RespawnTimes;


// mangos string ranges
#define MIN_MANGOS_STRING_ID           1                    // 'mangos_string'
#define MAX_MANGOS_STRING_ID           2000000000
#define MIN_DB_SCRIPT_STRING_ID        MAX_MANGOS_STRING_ID // 'db_script_string'
#define MAX_DB_SCRIPT_STRING_ID        2000010000
#define MIN_CREATURE_AI_TEXT_STRING_ID (-1)                 // 'creature_ai_texts'
#define MAX_CREATURE_AI_TEXT_STRING_ID (-1000000)

struct MangosStringLocale
{
    std::vector<std::string> Content;                       // 0 -> default, i -> i-1 locale index
};

typedef UNORDERED_MAP<uint32,CreatureData> CreatureDataMap;
typedef CreatureDataMap::value_type CreatureDataPair;

class FindCreatureData
{
    public:
        FindCreatureData(uint32 id, Player* player) : i_id(id), i_player(player),
            i_anyData(NULL), i_mapData(NULL), i_mapDist(0.0f), i_spawnedData(NULL), i_spawnedDist(0.0f) {}

        bool operator() (CreatureDataPair const& dataPair);
        CreatureDataPair const* GetResult() const;

    private:
        uint32 i_id;
        Player* i_player;

        CreatureDataPair const* i_anyData;
        CreatureDataPair const* i_mapData;
        float i_mapDist;
        CreatureDataPair const* i_spawnedData;
        float i_spawnedDist;
};

typedef UNORDERED_MAP<uint32,GameObjectData> GameObjectDataMap;
typedef GameObjectDataMap::value_type GameObjectDataPair;

class FindGOData
{
    public:
        FindGOData(uint32 id, Player* player) : i_id(id), i_player(player),
            i_anyData(NULL), i_mapData(NULL), i_mapDist(0.0f), i_spawnedData(NULL), i_spawnedDist(0.0f) {}

        bool operator() (GameObjectDataPair const& dataPair);
        GameObjectDataPair const* GetResult() const;

    private:
        uint32 i_id;
        Player* i_player;

        GameObjectDataPair const* i_anyData;
        GameObjectDataPair const* i_mapData;
        float i_mapDist;
        GameObjectDataPair const* i_spawnedData;
        float i_spawnedDist;
};

typedef UNORDERED_MAP<uint32,CreatureLocale> CreatureLocaleMap;
typedef UNORDERED_MAP<uint32,GameObjectLocale> GameObjectLocaleMap;
typedef UNORDERED_MAP<uint32,ItemLocale> ItemLocaleMap;
typedef UNORDERED_MAP<uint32,QuestLocale> QuestLocaleMap;
typedef UNORDERED_MAP<uint32,NpcTextLocale> NpcTextLocaleMap;
typedef UNORDERED_MAP<uint32,PageTextLocale> PageTextLocaleMap;
typedef UNORDERED_MAP<int32,MangosStringLocale> MangosStringLocaleMap;
typedef UNORDERED_MAP<uint32,GossipMenuItemsLocale> GossipMenuItemsLocaleMap;
typedef UNORDERED_MAP<uint32,PointOfInterestLocale> PointOfInterestLocaleMap;
typedef UNORDERED_MAP<uint32,uint32> ItemConvertMap;

typedef std::multimap<int32, uint32> ExclusiveQuestGroupsMap;
typedef std::multimap<uint32, ItemRequiredTarget> ItemRequiredTargetMap;
typedef std::multimap<uint32, uint32> QuestRelationsMap;
typedef std::pair<ExclusiveQuestGroupsMap::const_iterator, ExclusiveQuestGroupsMap::const_iterator> ExclusiveQuestGroupsMapBounds;
typedef std::pair<ItemRequiredTargetMap::const_iterator, ItemRequiredTargetMap::const_iterator> ItemRequiredTargetMapBounds;
typedef std::pair<QuestRelationsMap::const_iterator, QuestRelationsMap::const_iterator> QuestRelationsMapBounds;

struct PetLevelInfo
{
    PetLevelInfo() : health(0), mana(0) { for(int i=0; i < MAX_STATS; ++i ) stats[i] = 0; }

    uint16 stats[MAX_STATS];
    uint16 health;
    uint16 mana;
    uint16 armor;
    uint32 mindmg;
    uint32 maxdmg;
    uint32 attackpower;
};

struct PetScalingData
{
    PetScalingData() : creatureID(0), requiredAura(0),
    healthBasepoint(0), healthScale(0), powerBasepoint(0), powerScale(0),
    APBasepoint(0), APBaseScale(0), attackpowerScale(0), damageScale(0), spelldamageScale(0), spellHitScale(0),
    meleeHitScale(0), expertizeScale(0), attackspeedScale(0), critScale(0), powerregenScale(0)
    {
        for(int i=0; i < MAX_STATS; ++i ) statScale[i] = 0;
        for(int i=0; i < MAX_SPELL_SCHOOL; ++i ) resistanceScale[i] = 0;
    }

    uint32 creatureID;
    uint32 requiredAura;
    int32  healthBasepoint;
    int32  healthScale;
    int32  powerBasepoint;
    int32  powerScale;
    int32  APBasepoint;
    int32  APBaseScale;
    int32  statScale[MAX_STATS];
    int32  resistanceScale[MAX_SPELL_SCHOOL];
    int32  attackpowerScale;
    int32  damageScale;
    int32  spelldamageScale;
    int32  spellHitScale;
    int32  meleeHitScale;
    int32  expertizeScale;
    int32  attackspeedScale;
    int32  critScale;
    int32  powerregenScale;
};

typedef std::vector<PetScalingData> PetScalingDataList;

#define ANTICHEAT_ACTIONS 2
#define ANTICHEAT_CHECK_PARAMETERS 2

struct AntiCheatConfig
{
    AntiCheatConfig() : checkType(0), alarmsCount(0),disableOperation(false), messageNum(0)
    {
        for (int i=0; i < ANTICHEAT_ACTIONS; ++i )
        {
            actionType[i] = 0;
            actionParam[i] = 0;
        };

        for (int i=0; i < ANTICHEAT_CHECK_PARAMETERS; ++i )
        {
            checkParam[i] = 0;
        }

        for (int i=0; i < ANTICHEAT_CHECK_PARAMETERS; ++i )
        {
            checkFloatParam[i] = 0.0f;
        }
    }

    uint32 checkType;
    uint32 checkPeriod;
    uint32 alarmsCount;
    bool   disableOperation;
    uint32 messageNum;
    uint32 checkParam[ANTICHEAT_CHECK_PARAMETERS];
    float  checkFloatParam[ANTICHEAT_CHECK_PARAMETERS];
    uint32 actionType[ANTICHEAT_ACTIONS];
    uint32 actionParam[ANTICHEAT_ACTIONS];
    std::string description;

};

struct MailLevelReward
{
    MailLevelReward() : raceMask(0), mailTemplateId(0), senderEntry(0) {}
    MailLevelReward(uint32 _raceMask, uint32 _mailTemplateId, uint32 _senderEntry) : raceMask(_raceMask), mailTemplateId(_mailTemplateId), senderEntry(_senderEntry) {}

    uint32 raceMask;
    uint32 mailTemplateId;
    uint32 senderEntry;
};

typedef std::list<MailLevelReward> MailLevelRewardList;
typedef UNORDERED_MAP<uint8,MailLevelRewardList> MailLevelRewardMap;

// We assume the rate is in general the same for all three types below, but chose to keep three for scalability and customization
struct RepRewardRate
{
    float quest_rate;                                       // We allow rate = 0.0 in database. For this case,
    float creature_rate;                                    // it means that no reputation are given at all
    float spell_rate;                                       // for this faction/rate type.
};

struct ReputationOnKillEntry
{
    uint32 repfaction1;
    uint32 repfaction2;
    bool is_teamaward1;
    uint32 reputation_max_cap1;
    int32 repvalue1;
    bool is_teamaward2;
    uint32 reputation_max_cap2;
    int32 repvalue2;
    bool team_dependent;
    uint32 championingAura;
};

struct RepSpilloverTemplate
{
    uint32 faction[MAX_SPILLOVER_FACTIONS];
    float faction_rate[MAX_SPILLOVER_FACTIONS];
    uint32 faction_rank[MAX_SPILLOVER_FACTIONS];
};

struct PointOfInterest
{
    uint32 entry;
    float x;
    float y;
    uint32 icon;
    uint32 flags;
    uint32 data;
    std::string icon_name;
};

struct GossipMenuItems
{
    uint32          menu_id;
    uint32          id;
    uint8           option_icon;
    std::string     option_text;
    uint32          option_id;
    uint32          npc_option_npcflag;
    int32           action_menu_id;
    uint32          action_poi_id;
    uint32          action_script_id;
    bool            box_coded;
    uint32          box_money;
    std::string     box_text;
    uint16          cond_1;
    uint16          cond_2;
    uint16          cond_3;
};

struct GossipMenus
{
    uint32          entry;
    uint32          text_id;
    uint16          cond_1;
    uint16          cond_2;
};

typedef std::multimap<uint32,GossipMenus> GossipMenusMap;
typedef std::pair<GossipMenusMap::const_iterator, GossipMenusMap::const_iterator> GossipMenusMapBounds;
typedef std::multimap<uint32,GossipMenuItems> GossipMenuItemsMap;
typedef std::pair<GossipMenuItemsMap::const_iterator, GossipMenuItemsMap::const_iterator> GossipMenuItemsMapBounds;

struct QuestPOIPoint
{
    int32 x;
    int32 y;

    QuestPOIPoint() : x(0), y(0) {}
    QuestPOIPoint(int32 _x, int32 _y) : x(_x), y(_y) {}
};

struct QuestPOI
{
    uint32 PoiId;
    int32  ObjectiveIndex;
    uint32 MapId;
    uint32 MapAreaId;
    uint32 FloorId;
    uint32 Unk3;
    uint32 Unk4;
    std::vector<QuestPOIPoint> points;

    QuestPOI() : PoiId(0), ObjectiveIndex(0), MapId(0), MapAreaId(0), FloorId(0), Unk3(0), Unk4(0) {}
    QuestPOI(uint32 poiId, int32 objIndex, uint32 mapId, uint32 mapAreaId, uint32 floorId, uint32 unk3, uint32 unk4) : PoiId(poiId), ObjectiveIndex(objIndex), MapId(mapId), MapAreaId(mapAreaId), FloorId(floorId), Unk3(unk3), Unk4(unk4) {}
};

typedef std::vector<QuestPOI> QuestPOIVector;
typedef UNORDERED_MAP<uint32, QuestPOIVector> QuestPOIMap;

#define WEATHER_SEASONS 4
struct WeatherSeasonChances
{
    uint32 rainChance;
    uint32 snowChance;
    uint32 stormChance;
};

struct WeatherZoneChances
{
    WeatherSeasonChances data[WEATHER_SEASONS];
};

struct GraveYardData
{
    uint32 safeLocId;
    Team team;
};
typedef std::multimap<uint32, GraveYardData> GraveYardMap;
typedef std::pair<GraveYardMap::const_iterator, GraveYardMap::const_iterator> GraveYardMapBounds;

enum ConditionType
{                                                           // value1       value2  for the Condition enumed
    CONDITION_NONE                  = 0,                    // 0            0
    CONDITION_AURA                  = 1,                    // spell_id     effindex
    CONDITION_ITEM                  = 2,                    // item_id      count   check present req. amount items in inventory
    CONDITION_ITEM_EQUIPPED         = 3,                    // item_id      0
    CONDITION_AREAID                = 4,                    // area_id      0, 1 (0: in (sub)area, 1: not in (sub)area)
    CONDITION_REPUTATION_RANK       = 5,                    // faction_id   min_rank
    CONDITION_TEAM                  = 6,                    // player_team  0,      (469 - Alliance 67 - Horde)
    CONDITION_SKILL                 = 7,                    // skill_id     skill_value
    CONDITION_QUESTREWARDED         = 8,                    // quest_id     0
    CONDITION_QUESTTAKEN            = 9,                    // quest_id     0,      for condition true while quest active.
    CONDITION_AD_COMMISSION_AURA    = 10,                   // 0            0,      for condition true while one from AD commission aura active
    CONDITION_NO_AURA               = 11,                   // spell_id     effindex
    CONDITION_ACTIVE_GAME_EVENT     = 12,                   // event_id     0
    CONDITION_AREA_FLAG             = 13,                   // area_flag    area_flag_not
    CONDITION_RACE_CLASS            = 14,                   // race_mask    class_mask
    CONDITION_LEVEL                 = 15,                   // player_level 0, 1 or 2 (0: equal to, 1: equal or higher than, 2: equal or less than)
    CONDITION_NOITEM                = 16,                   // item_id      count   check not present req. amount items in inventory
    CONDITION_SPELL                 = 17,                   // spell_id     0, 1 (0: has spell, 1: hasn't spell)
    CONDITION_INSTANCE_SCRIPT       = 18,                   // map_id       instance_condition_id (instance script specific enum)
    CONDITION_QUESTAVAILABLE        = 19,                   // quest_id     0       for case when loot/gossip possible only if player can start quest
    CONDITION_ACHIEVEMENT           = 20,                   // ach_id       0, 1 (0: has achievement, 1: hasn't achievement) for player
    CONDITION_ACHIEVEMENT_REALM     = 21,                   // ach_id       0, 1 (0: has achievement, 1: hasn't achievement) for server
    CONDITION_QUEST_NONE            = 22,                   // quest_id     0 (quest did not take and not rewarded)
    CONDITION_ITEM_WITH_BANK        = 23,                   // item_id      count   check present req. amount items in inventory or bank
    CONDITION_NOITEM_WITH_BANK      = 24,                   // item_id      count   check not present req. amount items in inventory or bank
    CONDITION_NOT_ACTIVE_GAME_EVENT = 25,                   // event_id     0
    CONDITION_ACTIVE_HOLIDAY        = 26,                   // holiday_id   0       preferred use instead CONDITION_ACTIVE_GAME_EVENT when possible
    CONDITION_NOT_ACTIVE_HOLIDAY    = 27,                   // holiday_id   0       preferred use instead CONDITION_NOT_ACTIVE_GAME_EVENT when possible
};

#define MAX_CONDITION                 28                    // maximum value in ConditionType enum

struct PlayerCondition
{
    ConditionType condition;                                // additional condition type
    uint32  value1;                                         // data for the condition - see ConditionType definition
    uint32  value2;

    PlayerCondition(uint8 _condition = 0, uint32 _value1 = 0, uint32 _value2 = 0)
        : condition(ConditionType(_condition)), value1(_value1), value2(_value2) {}

    static bool IsValid(ConditionType condition, uint32 value1, uint32 value2);
    // Checks correctness of values
    bool Meets(Player const * APlayer) const;               // Checks if the player meets the condition
    bool operator == (PlayerCondition const& lc) const
    {
        return (lc.condition == condition && lc.value1 == value1 && lc.value2 == value2);
    }
};

// NPC gossip text id
typedef UNORDERED_MAP<uint32, uint32> CacheNpcTextIdMap;

typedef UNORDERED_MAP<uint32, VendorItemData> CacheVendorItemMap;
typedef UNORDERED_MAP<uint32, TrainerSpellData> CacheTrainerSpellMap;

enum SkillRangeType
{
    SKILL_RANGE_LANGUAGE,                                   // 300..300
    SKILL_RANGE_LEVEL,                                      // 1..max skill for level
    SKILL_RANGE_MONO,                                       // 1..1, grey monolite bar
    SKILL_RANGE_RANK,                                       // 1..skill for known rank
    SKILL_RANGE_NONE,                                       // 0..0 always
};

SkillRangeType GetSkillRangeType(SkillLineEntry const *pSkill, bool racial);

#define MAX_PLAYER_NAME          12                         // max allowed by client name length
#define MAX_INTERNAL_PLAYER_NAME 15                         // max server internal player name length ( > MAX_PLAYER_NAME for support declined names )
#define MAX_PET_NAME             12                         // max allowed by client name length
#define MAX_CHARTER_NAME         24                         // max allowed by client name length

bool normalizePlayerName(std::string& name);

struct MANGOS_DLL_SPEC LanguageDesc
{
    Language lang_id;
    uint32   spell_id;
    uint32   skill_id;
};

extern LanguageDesc lang_description[LANGUAGES_COUNT];
MANGOS_DLL_SPEC LanguageDesc const* GetLanguageDescByID(uint32 lang);

class PlayerDumpReader;

template<typename T>
class IdGenerator
{
    public:                                                 // constructors
        explicit IdGenerator(char const* _name) : m_name(_name), m_nextGuid(1) {}

    public:                                                 // modifiers
        void Set(T val) { m_nextGuid = val; }
        T Generate();

    public:                                                 // accessors
        T GetNextAfterMaxUsed() const { return m_nextGuid; }

    private:                                                // fields
        char const* m_name;
        T m_nextGuid;
};

class ObjectMgr
{
    friend class PlayerDumpReader;

    public:
        ObjectMgr();
        ~ObjectMgr();

        typedef UNORDERED_MAP<uint32, Item*> ItemMap;

        typedef UNORDERED_MAP<uint32, Group*> GroupMap;

        typedef UNORDERED_MAP<uint32, Guild*> GuildMap;

        typedef UNORDERED_MAP<uint32, ArenaTeam*> ArenaTeamMap;

        typedef UNORDERED_MAP<uint32, Quest*> QuestMap;

        typedef UNORDERED_MAP<uint32, AreaTrigger> AreaTriggerMap;

        typedef UNORDERED_MAP<uint32, RepRewardRate > RepRewardRateMap;
        typedef UNORDERED_MAP<uint32, ReputationOnKillEntry> RepOnKillMap;
        typedef UNORDERED_MAP<uint32, RepSpilloverTemplate> RepSpilloverTemplateMap;

        typedef UNORDERED_MAP<uint32, PointOfInterest> PointOfInterestMap;

        typedef UNORDERED_MAP<uint32, WeatherZoneChances> WeatherZoneMap;

        Player* GetPlayer(const char* name) const { return ObjectAccessor::FindPlayerByName(name);}
        Player* GetPlayer(ObjectGuid guid) const { return ObjectAccessor::FindPlayer(guid); }

        static GameObjectInfo const *GetGameObjectInfo(uint32 id) { return sGOStorage.LookupEntry<GameObjectInfo>(id); }

        void LoadGameobjectInfo();
        void AddGameobjectInfo(GameObjectInfo *goinfo);

        void PackGroupIds();
        Group* GetGroupById(uint32 id) const;
        void AddGroup(Group* group);
        void RemoveGroup(Group* group);

        Guild* GetGuildByLeader(ObjectGuid guid) const;
        Guild* GetGuildById(uint32 GuildId) const;
        Guild* GetGuildByName(const std::string& guildname) const;
        std::string GetGuildNameById(uint32 GuildId) const;
        void AddGuild(Guild* guild);
        void RemoveGuild(uint32 Id);

        ArenaTeam* GetArenaTeamById(uint32 arenateamid) const;
        ArenaTeam* GetArenaTeamByName(const std::string& arenateamname) const;
        ArenaTeam* GetArenaTeamByCaptain(ObjectGuid guid) const;
        void AddArenaTeam(ArenaTeam* arenaTeam);
        void RemoveArenaTeam(uint32 Id);
        ArenaTeamMap::iterator GetArenaTeamMapBegin() { return mArenaTeamMap.begin(); }
        ArenaTeamMap::iterator GetArenaTeamMapEnd()   { return mArenaTeamMap.end(); }

        static CreatureInfo const *GetCreatureTemplate( uint32 id );
        CreatureModelInfo const *GetCreatureModelInfo( uint32 modelid );
        CreatureModelInfo const* GetCreatureModelRandomGender(uint32 display_id);
        uint32 GetCreatureModelAlternativeModel(uint32 modelId);

        EquipmentInfo const *GetEquipmentInfo( uint32 entry );
        static CreatureDataAddon const *GetCreatureAddon( uint32 lowguid )
        {
            return sCreatureDataAddonStorage.LookupEntry<CreatureDataAddon>(lowguid);
        }

        static CreatureDataAddon const *GetCreatureTemplateAddon( uint32 entry )
        {
            return sCreatureInfoAddonStorage.LookupEntry<CreatureDataAddon>(entry);
        }

        static ItemPrototype const* GetItemPrototype(uint32 id) { return sItemStorage.LookupEntry<ItemPrototype>(id); }

        static InstanceTemplate const* GetInstanceTemplate(uint32 map)
        {
            return sInstanceTemplate.LookupEntry<InstanceTemplate>(map);
        }

        PetLevelInfo const* GetPetLevelInfo(uint32 creature_id, uint32 level) const;

        PetScalingDataList const* GetPetScalingData(uint32 creature_id) const;

        AntiCheatConfig const* GetAntiCheatConfig(uint32 checkType) const;

        PlayerClassInfo const* GetPlayerClassInfo(uint32 class_) const
        {
            if(class_ >= MAX_CLASSES) return NULL;
            return &playerClassInfo[class_];
        }
        void GetPlayerClassLevelInfo(uint32 class_,uint32 level, PlayerClassLevelInfo* info) const;

        PlayerInfo const* GetPlayerInfo(uint32 race, uint32 class_) const
        {
            if(race   >= MAX_RACES)   return NULL;
            if(class_ >= MAX_CLASSES) return NULL;
            PlayerInfo const* info = &playerInfo[race][class_];
            if(info->displayId_m==0 || info->displayId_f==0) return NULL;
            return info;
        }
        void GetPlayerLevelInfo(uint32 race, uint32 class_,uint32 level, PlayerLevelInfo* info) const;

        uint64 GetPlayerGUIDByName(std::string name) const;
        bool GetPlayerNameByGUID(ObjectGuid guid, std::string &name) const;
        Team GetPlayerTeamByGUID(ObjectGuid guid) const;
        uint32 GetPlayerAccountIdByGUID(ObjectGuid guid) const;
        uint32 GetPlayerAccountIdByPlayerName(const std::string& name) const;

        uint32 GetNearestTaxiNode( float x, float y, float z, uint32 mapid, Team team );
        void GetTaxiPath( uint32 source, uint32 destination, uint32 &path, uint32 &cost);
        uint32 GetTaxiMountDisplayId( uint32 id, Team team, bool allowed_alt_team = false);

        Quest const* GetQuestTemplate(uint32 quest_id) const
        {
            QuestMap::const_iterator itr = mQuestTemplates.find(quest_id);
            return itr != mQuestTemplates.end() ? itr->second : NULL;
        }
        QuestMap const& GetQuestTemplates() const { return mQuestTemplates; }

        uint32 GetQuestForAreaTrigger(uint32 Trigger_ID) const
        {
            QuestAreaTriggerMap::const_iterator itr = mQuestAreaTriggerMap.find(Trigger_ID);
            if(itr != mQuestAreaTriggerMap.end())
                return itr->second;
            return 0;
        }
        bool IsTavernAreaTrigger(uint32 Trigger_ID) const
        {
            return mTavernAreaTriggerSet.find(Trigger_ID) != mTavernAreaTriggerSet.end();
        }

        bool IsGameObjectForQuests(uint32 entry) const
        {
            return mGameObjectForQuestSet.find(entry) != mGameObjectForQuestSet.end();
        }

        GossipText const* GetGossipText(uint32 Text_ID) const;

        WorldSafeLocsEntry const *GetClosestGraveYard(float x, float y, float z, uint32 MapId, Team team);
        bool AddGraveYardLink(uint32 id, uint32 zone, Team team, bool inDB = true);
        void LoadGraveyardZones();
        GraveYardData const* FindGraveYardData(uint32 id, uint32 zone) const;

        AreaTrigger const* GetAreaTrigger(uint32 trigger) const
        {
            AreaTriggerMap::const_iterator itr = mAreaTriggers.find( trigger );
            if( itr != mAreaTriggers.end( ) )
                return &itr->second;
            return NULL;
        }

        AreaTrigger const* GetGoBackTrigger(uint32 Map) const;
        AreaTrigger const* GetMapEntranceTrigger(uint32 Map) const;

        RepRewardRate const* GetRepRewardRate(uint32 factionId) const
        {
            RepRewardRateMap::const_iterator itr = m_RepRewardRateMap.find(factionId);
            if (itr != m_RepRewardRateMap.end())
                return &itr->second;

            return NULL;
        }

        ReputationOnKillEntry const* GetReputationOnKillEntry(uint32 id) const
        {
            RepOnKillMap::const_iterator itr = mRepOnKill.find(id);
            if(itr != mRepOnKill.end())
                return &itr->second;
            return NULL;
        }

        RepSpilloverTemplate const* GetRepSpilloverTemplate(uint32 factionId) const
        {
            RepSpilloverTemplateMap::const_iterator itr = m_RepSpilloverTemplateMap.find(factionId);
            if (itr != m_RepSpilloverTemplateMap.end())
                return &itr->second;

            return NULL;
        }

        PointOfInterest const* GetPointOfInterest(uint32 id) const
        {
            PointOfInterestMap::const_iterator itr = mPointsOfInterest.find(id);
            if(itr != mPointsOfInterest.end())
                return &itr->second;
            return NULL;
        }

        QuestPOIVector const* GetQuestPOIVector(uint32 questId)
        {
            QuestPOIMap::const_iterator itr = mQuestPOIMap.find(questId);
            if(itr != mQuestPOIMap.end())
                return &itr->second;
            return NULL;
        }

        VehicleAccessoryList const* GetVehicleAccessoryList(uint32 uiEntry) const
        {
            VehicleAccessoryMap::const_iterator itr = m_VehicleAccessoryMap.find(uiEntry);
            if (itr != m_VehicleAccessoryMap.end())
                return &itr->second;
            return NULL;
        }

        void LoadGuilds();
        void LoadArenaTeams();
        void LoadGroups();
        void LoadQuests();
        void LoadQuestRelations()
        {
            LoadGameobjectQuestRelations();
            LoadGameobjectInvolvedRelations();
            LoadCreatureQuestRelations();
            LoadCreatureInvolvedRelations();
        }
        void LoadGameobjectQuestRelations();
        void LoadGameobjectInvolvedRelations();
        void LoadCreatureQuestRelations();
        void LoadCreatureInvolvedRelations();

        bool LoadMangosStrings(DatabaseType& db, char const* table, int32 min_value, int32 max_value);
        bool LoadMangosStrings() { return LoadMangosStrings(WorldDatabase,"mangos_string",MIN_MANGOS_STRING_ID,MAX_MANGOS_STRING_ID); }
        void LoadCreatureLocales();
        void LoadCreatureTemplates();
        void LoadCreatures();
        void LoadCreatureRespawnTimes();
        void LoadCreatureAddons();
        void LoadCreatureModelInfo();
        void LoadCreatureModelRace();
        void LoadEquipmentTemplates();
        void LoadGameObjectLocales();
        void LoadGameobjects();
        void LoadGameobjectRespawnTimes();
        void LoadItemConverts();
        void LoadItemPrototypes();
        void LoadItemRequiredTarget();
        void LoadItemLocales();
        void LoadQuestLocales();
        void LoadGossipTextLocales();
        void LoadPageTextLocales();
        void LoadGossipMenuItemsLocales();
        void LoadPointOfInterestLocales();
        void LoadInstanceTemplate();
        void LoadMailLevelRewards();

        void LoadGossipText();

        void LoadAreaTriggerTeleports();
        void LoadQuestAreaTriggers();
        void LoadTavernAreaTriggers();
        void LoadGameObjectForQuests();

        void LoadPageTexts();

        void LoadAntiCheatConfig();

        void LoadPlayerInfo();
        void LoadPetLevelInfo();
        void LoadPetScalingData();
        void LoadExplorationBaseXP();
        void LoadPetNames();
        void LoadPetNumber();
        void LoadCorpses();
        void LoadFishingBaseSkillLevel();

        void LoadReputationRewardRate();
        void LoadReputationOnKill();
        void LoadReputationSpilloverTemplate();

        void LoadPointsOfInterest();
        void LoadQuestPOI();

        void LoadNPCSpellClickSpells();

        void LoadWeatherZoneChances();
        void LoadGameTele();

        void LoadNpcGossips();

        void LoadGossipMenu();
        void LoadGossipMenuItems();

        void LoadVendorTemplates();
        void LoadVendors() { LoadVendors("npc_vendor", false); }
        void LoadTrainerTemplates();
        void LoadTrainers() { LoadTrainers("npc_trainer", false); }

        void LoadVehicleAccessories();

        std::string GeneratePetName(uint32 entry);
        uint32 GetBaseXP(uint32 level) const;
        uint32 GetXPForLevel(uint32 level) const;
        uint32 GetXPForPetLevel(uint32 level) const { return GetXPForLevel(level)/20; }

        int32 GetFishingBaseSkillLevel(uint32 entry) const
        {
            FishingBaseSkillMap::const_iterator itr = mFishingBaseForArea.find(entry);
            return itr != mFishingBaseForArea.end() ? itr->second : 0;
        }

        void ReturnOrDeleteOldMails(bool serverUp);

        void SetHighestGuids();
        uint32 GenerateLowGuid(HighGuid guidhigh);
        uint32 GenerateArenaTeamId() { return m_ArenaTeamIds.Generate(); }
        uint32 GenerateAuctionID() { return m_AuctionIds.Generate(); }
        uint64 GenerateEquipmentSetGuid() { return m_EquipmentSetIds.Generate(); }
        uint32 GenerateGuildId() { return m_GuildIds.Generate(); }
        uint32 GenerateGroupId() { return m_GroupIds.Generate(); }
        //uint32 GenerateItemTextID() { return m_ItemGuids.Generate(); }
        uint32 GenerateMailID() { return m_MailIds.Generate(); }
        uint32 GeneratePetNumber() { return m_PetNumbers.Generate(); }

        MailLevelReward const* GetMailLevelReward(uint32 level,uint32 raceMask)
        {
            MailLevelRewardMap::const_iterator map_itr = m_mailLevelRewardMap.find(level);
            if (map_itr == m_mailLevelRewardMap.end())
                return NULL;

            for(MailLevelRewardList::const_iterator set_itr = map_itr->second.begin(); set_itr != map_itr->second.end(); ++set_itr)
                if (set_itr->raceMask & raceMask)
                    return &*set_itr;

            return NULL;
        }

        WeatherZoneChances const* GetWeatherChances(uint32 zone_id) const
        {
            WeatherZoneMap::const_iterator itr = mWeatherZoneMap.find(zone_id);
            if(itr != mWeatherZoneMap.end())
                return &itr->second;
            else
                return NULL;
        }

        CellObjectGuids const& GetCellObjectGuids(uint16 mapid, uint8 spawnMode, uint32 cell_id)
        {
            return mMapObjectGuids[MAKE_PAIR32(mapid,spawnMode)][cell_id];
        }

        CreatureDataPair const* GetCreatureDataPair(uint32 guid) const
        {
            CreatureDataMap::const_iterator itr = mCreatureDataMap.find(guid);
            if(itr==mCreatureDataMap.end()) return NULL;
            return &*itr;
        }

        CreatureData const* GetCreatureData(uint32 guid) const
        {
            CreatureDataPair const* dataPair = GetCreatureDataPair(guid);
            return dataPair ? &dataPair->second : NULL;
        }

        CreatureData& NewOrExistCreatureData(uint32 guid) { return mCreatureDataMap[guid]; }
        void DeleteCreatureData(uint32 guid);

        template<typename Worker>
        void DoCreatureData(Worker& worker) const
        {
            for (CreatureDataMap::const_iterator itr = mCreatureDataMap.begin(); itr != mCreatureDataMap.end(); ++itr)
                if (worker(*itr))
                    break;
        }

        CreatureLocale const* GetCreatureLocale(uint32 entry) const
        {
            CreatureLocaleMap::const_iterator itr = mCreatureLocaleMap.find(entry);
            if(itr==mCreatureLocaleMap.end()) return NULL;
            return &itr->second;
        }

        GameObjectLocale const* GetGameObjectLocale(uint32 entry) const
        {
            GameObjectLocaleMap::const_iterator itr = mGameObjectLocaleMap.find(entry);
            if(itr==mGameObjectLocaleMap.end()) return NULL;
            return &itr->second;
        }

        ItemLocale const* GetItemLocale(uint32 entry) const
        {
            ItemLocaleMap::const_iterator itr = mItemLocaleMap.find(entry);
            if(itr==mItemLocaleMap.end()) return NULL;
            return &itr->second;
        }

        QuestLocale const* GetQuestLocale(uint32 entry) const
        {
            QuestLocaleMap::const_iterator itr = mQuestLocaleMap.find(entry);
            if(itr==mQuestLocaleMap.end()) return NULL;
            return &itr->second;
        }

        NpcTextLocale const* GetNpcTextLocale(uint32 entry) const
        {
            NpcTextLocaleMap::const_iterator itr = mNpcTextLocaleMap.find(entry);
            if(itr==mNpcTextLocaleMap.end()) return NULL;
            return &itr->second;
        }

        PageTextLocale const* GetPageTextLocale(uint32 entry) const
        {
            PageTextLocaleMap::const_iterator itr = mPageTextLocaleMap.find(entry);
            if(itr==mPageTextLocaleMap.end()) return NULL;
            return &itr->second;
        }

        GossipMenuItemsLocale const* GetGossipMenuItemsLocale(uint32 entry) const
        {
            GossipMenuItemsLocaleMap::const_iterator itr = mGossipMenuItemsLocaleMap.find(entry);
            if(itr==mGossipMenuItemsLocaleMap.end()) return NULL;
            return &itr->second;
        }

        PointOfInterestLocale const* GetPointOfInterestLocale(uint32 poi_id) const
        {
            PointOfInterestLocaleMap::const_iterator itr = mPointOfInterestLocaleMap.find(poi_id);
            if(itr==mPointOfInterestLocaleMap.end()) return NULL;
            return &itr->second;
        }

        GameObjectDataPair const* GetGODataPair(uint32 guid) const
        {
            GameObjectDataMap::const_iterator itr = mGameObjectDataMap.find(guid);
            if(itr==mGameObjectDataMap.end()) return NULL;
            return &*itr;
        }

        GameObjectData const* GetGOData(uint32 guid) const
        {
            GameObjectDataPair const* dataPair = GetGODataPair(guid);
            return dataPair ? &dataPair->second : NULL;
        }

        GameObjectData& NewGOData(uint32 guid) { return mGameObjectDataMap[guid]; }
        void DeleteGOData(uint32 guid);

        template<typename Worker>
        void DoGOData(Worker& worker) const
        {
            for (GameObjectDataMap::const_iterator itr = mGameObjectDataMap.begin(); itr != mGameObjectDataMap.end(); ++itr)
                if (worker(*itr))                           // arg = GameObjectDataPair
                    break;
        }

        MangosStringLocale const* GetMangosStringLocale(int32 entry) const
        {
            MangosStringLocaleMap::const_iterator itr = mMangosStringLocaleMap.find(entry);
            if(itr==mMangosStringLocaleMap.end()) return NULL;
            return &itr->second;
        }

        const char *GetMangosString(int32 entry, int locale_idx) const;
        const char *GetMangosStringForDBCLocale(int32 entry) const { return GetMangosString(entry,DBCLocaleIndex); }
        int32 GetDBCLocaleIndex() const { return DBCLocaleIndex; }
        void SetDBCLocaleIndex(uint32 lang) { DBCLocaleIndex = GetIndexForLocale(LocaleConstant(lang)); }

        void AddCorpseCellData(uint32 mapid, uint32 cellid, uint32 player_guid, uint32 instance);
        void DeleteCorpseCellData(uint32 mapid, uint32 cellid, uint32 player_guid);

        time_t GetCreatureRespawnTime(uint32 loguid, uint32 instance) { return mCreatureRespawnTimes[MAKE_PAIR64(loguid,instance)]; }
        void SaveCreatureRespawnTime(uint32 loguid, uint32 instance, time_t t);
        time_t GetGORespawnTime(uint32 loguid, uint32 instance) { return mGORespawnTimes[MAKE_PAIR64(loguid,instance)]; }
        void SaveGORespawnTime(uint32 loguid, uint32 instance, time_t t);
        void DeleteRespawnTimeForInstance(uint32 instance);

        // grid objects
        void AddCreatureToGrid(uint32 guid, CreatureData const* data);
        void RemoveCreatureFromGrid(uint32 guid, CreatureData const* data);
        void AddGameobjectToGrid(uint32 guid, GameObjectData const* data);
        void RemoveGameobjectFromGrid(uint32 guid, GameObjectData const* data);

        // reserved names
        void LoadReservedPlayersNames();
        bool IsReservedName(const std::string& name) const;

        // name with valid structure and symbols
        static uint8 CheckPlayerName( const std::string& name, bool create = false );
        static PetNameInvalidReason CheckPetName( const std::string& name );
        static bool IsValidCharterName( const std::string& name );

        static bool CheckDeclinedNames(std::wstring w_ownname, DeclinedName const& names);

        void LoadSpellDisabledEntrys();
        uint8 IsSpellDisabled(uint32 spellid)
        {
            uint8 result=0;
            SpellDisabledMap::const_iterator itr = m_spell_disabled.find(spellid);
            if(itr != m_spell_disabled.end())
            {
                result=1;
                if(itr->second != 0)
                    result=2;
            }
            return result;
        }

        int GetIndexForLocale(LocaleConstant loc);
        LocaleConstant GetLocaleForIndex(int i);

        uint16 GetConditionId(ConditionType condition, uint32 value1, uint32 value2);
        bool IsPlayerMeetToCondition(Player const* player, uint16 condition_id) const
        {
            if(condition_id >= mConditions.size())
                return false;

            return mConditions[condition_id].Meets(player);
        }

        GameTele const* GetGameTele(uint32 id) const
        {
            GameTeleMap::const_iterator itr = m_GameTeleMap.find(id);
            if(itr==m_GameTeleMap.end()) return NULL;
            return &itr->second;
        }

        GameTele const* GetGameTele(const std::string& name) const;
        GameTeleMap const& GetGameTeleMap() const { return m_GameTeleMap; }
        bool AddGameTele(GameTele& data);
        bool DeleteGameTele(const std::string& name);

        uint32 GetNpcGossip(uint32 entry) const
        {
            CacheNpcTextIdMap::const_iterator iter = m_mCacheNpcTextIdMap.find(entry);
            if(iter == m_mCacheNpcTextIdMap.end())
                return 0;

            return iter->second;
        }

        TrainerSpellData const* GetNpcTrainerSpells(uint32 entry) const
        {
            CacheTrainerSpellMap::const_iterator iter = m_mCacheTrainerSpellMap.find(entry);
            if(iter == m_mCacheTrainerSpellMap.end())
                return NULL;

            return &iter->second;
        }

        TrainerSpellData const* GetNpcTrainerTemplateSpells(uint32 entry) const
        {
            CacheTrainerSpellMap::const_iterator iter = m_mCacheTrainerTemplateSpellMap.find(entry);
            if(iter == m_mCacheTrainerTemplateSpellMap.end())
                return NULL;

            return &iter->second;
        }

        VendorItemData const* GetNpcVendorItemList(uint32 entry) const
        {
            CacheVendorItemMap::const_iterator  iter = m_mCacheVendorItemMap.find(entry);
            if(iter == m_mCacheVendorItemMap.end())
                return NULL;

            return &iter->second;
        }

        VendorItemData const* GetNpcVendorTemplateItemList(uint32 entry) const
        {
            CacheVendorItemMap::const_iterator  iter = m_mCacheVendorTemplateItemMap.find(entry);
            if(iter == m_mCacheVendorTemplateItemMap.end())
                return NULL;

            return &iter->second;
        }

        void AddVendorItem(uint32 entry,uint32 item, uint32 maxcount, uint32 incrtime, uint32 ExtendedCost);
        bool RemoveVendorItem(uint32 entry,uint32 item);
        bool IsVendorItemValid(bool isTemplate, char const* tableName, uint32 vendor_entry, uint32 item, uint32 maxcount, uint32 ptime, uint32 ExtendedCost, Player* pl = NULL, std::set<uint32>* skip_vendors = NULL) const;

        int GetOrNewIndexForLocale(LocaleConstant loc);

        SpellClickInfoMapBounds GetSpellClickInfoMapBounds(uint32 creature_id) const
        {
            return mSpellClickInfoMap.equal_range(creature_id);
        }

        uint32 GetItemConvert(uint32 itemEntry, uint32 raceMask) const
        {
            ItemConvertMap::const_iterator iter = m_ItemConvert.find(itemEntry);
            if (iter == m_ItemConvert.end())
                return itemEntry;

            ItemPrototype const* proto = GetItemPrototype(iter->second);
            return (proto && proto->AllowableRace & raceMask) ? iter->second : itemEntry;
        }

        ItemRequiredTargetMapBounds GetItemRequiredTargetMapBounds(uint32 uiItemEntry) const
        {
            return m_ItemRequiredTarget.equal_range(uiItemEntry);
        }

        GossipMenusMapBounds GetGossipMenusMapBounds(uint32 uiMenuId) const
        {
            return m_mGossipMenusMap.equal_range(uiMenuId);
        }

        GossipMenuItemsMapBounds GetGossipMenuItemsMapBounds(uint32 uiMenuId) const
        {
            return m_mGossipMenuItemsMap.equal_range(uiMenuId);
        }

        ExclusiveQuestGroupsMapBounds GetExclusiveQuestGroupsMapBounds(int32 groupId) const
        {
            return m_ExclusiveQuestGroups.equal_range(groupId);
        }

        QuestRelationsMapBounds GetCreatureQuestRelationsMapBounds(uint32 entry) const
        {
            return m_CreatureQuestRelations.equal_range(entry);
        }

        QuestRelationsMapBounds GetCreatureQuestInvolvedRelationsMapBounds(uint32 entry) const
        {
            return m_CreatureQuestInvolvedRelations.equal_range(entry);
        }

        QuestRelationsMapBounds GetGOQuestRelationsMapBounds(uint32 entry) const
        {
            return m_GOQuestRelations.equal_range(entry);
        }

        QuestRelationsMapBounds GetGOQuestInvolvedRelationsMapBounds(uint32 entry) const
        {
            return m_GOQuestInvolvedRelations.equal_range(entry);
        }

        QuestRelationsMap& GetCreatureQuestRelationsMap() { return m_CreatureQuestRelations; }

        uint32 GetModelForRace(uint32 sourceModelId, uint32 racemask);
    protected:

        // first free id for selected id type
        IdGenerator<uint32> m_ArenaTeamIds;
        IdGenerator<uint32> m_AuctionIds;
        IdGenerator<uint64> m_EquipmentSetIds;
        IdGenerator<uint32> m_GuildIds;
        IdGenerator<uint32> m_MailIds;
        IdGenerator<uint32> m_PetNumbers;
        IdGenerator<uint32> m_GroupIds;

        // first free low guid for selected guid type
        ObjectGuidGenerator<HIGHGUID_PLAYER>     m_CharGuids;
        ObjectGuidGenerator<HIGHGUID_UNIT>       m_CreatureGuids;
        ObjectGuidGenerator<HIGHGUID_ITEM>       m_ItemGuids;
        ObjectGuidGenerator<HIGHGUID_GAMEOBJECT> m_GameobjectGuids;
        ObjectGuidGenerator<HIGHGUID_CORPSE>     m_CorpseGuids;
        ObjectGuidGenerator<HIGHGUID_INSTANCE>   m_InstanceGuids;

        QuestMap            mQuestTemplates;

        typedef UNORDERED_MAP<uint32, GossipText> GossipTextMap;
        typedef UNORDERED_MAP<uint32, uint32> QuestAreaTriggerMap;
        typedef std::set<uint32> TavernAreaTriggerSet;
        typedef std::set<uint32> GameObjectForQuestSet;

        typedef std::multimap<uint32, CreatureModelRace> CreatureModelRaceMap;
        typedef std::pair<CreatureModelRaceMap::const_iterator, CreatureModelRaceMap::const_iterator> CreatureModelRaceMapBounds;

        GroupMap            mGroupMap;
        GuildMap            mGuildMap;
        ArenaTeamMap        mArenaTeamMap;

        QuestAreaTriggerMap mQuestAreaTriggerMap;
        TavernAreaTriggerSet mTavernAreaTriggerSet;
        GameObjectForQuestSet mGameObjectForQuestSet;
        GossipTextMap       mGossipText;
        AreaTriggerMap      mAreaTriggers;

        RepRewardRateMap    m_RepRewardRateMap;
        RepOnKillMap        mRepOnKill;
        RepSpilloverTemplateMap m_RepSpilloverTemplateMap;

        GossipMenusMap      m_mGossipMenusMap;
        GossipMenuItemsMap  m_mGossipMenuItemsMap;
        PointOfInterestMap  mPointsOfInterest;

        QuestPOIMap         mQuestPOIMap;

        WeatherZoneMap      mWeatherZoneMap;

        //character reserved names
        typedef std::set<std::wstring> ReservedNamesMap;
        ReservedNamesMap    m_ReservedNames;

        typedef UNORDERED_MAP<uint32, uint32> SpellDisabledMap;
        SpellDisabledMap  m_spell_disabled;

        GraveYardMap        mGraveYardMap;

        GameTeleMap         m_GameTeleMap;

        SpellClickInfoMap   mSpellClickInfoMap;

        ItemConvertMap        m_ItemConvert;
        ItemRequiredTargetMap m_ItemRequiredTarget;

        VehicleAccessoryMap m_VehicleAccessoryMap;

        typedef             std::vector<LocaleConstant> LocalForIndex;
        LocalForIndex        m_LocalForIndex;

        ExclusiveQuestGroupsMap m_ExclusiveQuestGroups;

        QuestRelationsMap       m_CreatureQuestRelations;
        QuestRelationsMap       m_CreatureQuestInvolvedRelations;
        QuestRelationsMap       m_GOQuestRelations;
        QuestRelationsMap       m_GOQuestInvolvedRelations;

        int DBCLocaleIndex;

    private:
        void LoadCreatureAddons(SQLStorage& creatureaddons, char const* entryName, char const* comment);
        void ConvertCreatureAddonAuras(CreatureDataAddon* addon, char const* table, char const* guidEntryStr);
        void LoadQuestRelationsHelper(QuestRelationsMap& map, char const* table);
        void LoadVendors(char const* tableName, bool isTemplates);
        void LoadTrainers(char const* tableName, bool isTemplates);

        MailLevelRewardMap m_mailLevelRewardMap;

        typedef std::map<uint32,PetLevelInfo*> PetLevelInfoMap;
        // PetLevelInfoMap[creature_id][level]
        PetLevelInfoMap petInfo;                            // [creature_id][level]

        typedef std::map<uint32, PetScalingDataList*> PetScalingDataMap;
        PetScalingDataMap m_PetScalingData;                 // [creature_id]

        typedef std::map<uint32, AntiCheatConfig> AntiCheatConfigMap;
        AntiCheatConfigMap m_AntiCheatConfig;               // [check_type]

        PlayerClassInfo playerClassInfo[MAX_CLASSES];

        void BuildPlayerLevelInfo(uint8 race, uint8 class_, uint8 level, PlayerLevelInfo* plinfo) const;
        PlayerInfo playerInfo[MAX_RACES][MAX_CLASSES];

        typedef std::vector<uint32> PlayerXPperLevel;       // [level]
        PlayerXPperLevel mPlayerXPperLevel;

        typedef std::map<uint32,uint32> BaseXPMap;          // [area level][base xp]
        BaseXPMap mBaseXPTable;

        typedef std::map<uint32,int32> FishingBaseSkillMap; // [areaId][base skill level]
        FishingBaseSkillMap mFishingBaseForArea;

        typedef std::map<uint32,std::vector<std::string> > HalfNameMap;
        HalfNameMap PetHalfName0;
        HalfNameMap PetHalfName1;

        MapObjectGuids mMapObjectGuids;
        CreatureDataMap mCreatureDataMap;
        CreatureLocaleMap mCreatureLocaleMap;
        GameObjectDataMap mGameObjectDataMap;
        GameObjectLocaleMap mGameObjectLocaleMap;
        ItemLocaleMap mItemLocaleMap;
        QuestLocaleMap mQuestLocaleMap;
        NpcTextLocaleMap mNpcTextLocaleMap;
        PageTextLocaleMap mPageTextLocaleMap;
        MangosStringLocaleMap mMangosStringLocaleMap;
        GossipMenuItemsLocaleMap mGossipMenuItemsLocaleMap;
        PointOfInterestLocaleMap mPointOfInterestLocaleMap;
        RespawnTimes mCreatureRespawnTimes;
        RespawnTimes mGORespawnTimes;

        // Storage for Conditions. First element (index 0) is reserved for zero-condition (nothing required)
        typedef std::vector<PlayerCondition> ConditionStore;
        ConditionStore mConditions;

        CreatureModelRaceMap    m_mCreatureModelRaceMap;

        CacheNpcTextIdMap m_mCacheNpcTextIdMap;
        CacheVendorItemMap m_mCacheVendorTemplateItemMap;
        CacheVendorItemMap m_mCacheVendorItemMap;
        CacheTrainerSpellMap m_mCacheTrainerTemplateSpellMap;
        CacheTrainerSpellMap m_mCacheTrainerSpellMap;
};

#define sObjectMgr MaNGOS::Singleton<ObjectMgr>::Instance()

// scripting access functions
MANGOS_DLL_SPEC bool LoadMangosStrings(DatabaseType& db, char const* table,int32 start_value = MAX_CREATURE_AI_TEXT_STRING_ID, int32 end_value = std::numeric_limits<int32>::min());
MANGOS_DLL_SPEC CreatureInfo const* GetCreatureTemplateStore(uint32 entry);
MANGOS_DLL_SPEC Quest const* GetQuestTemplateStore(uint32 entry);

#endif
