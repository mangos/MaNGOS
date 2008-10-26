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

#ifndef DBCSTRUCTURE_H
#define DBCSTRUCTURE_H

#include "DBCEnums.h"
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

struct AreaTableEntry
{
    uint32  ID;                                             // 0
    uint32  mapid;                                          // 1
    uint32  zone;                                           // 2 if 0 then it's zone, else it's zone id of this area
    uint32  exploreFlag;                                    // 3, main index
    uint32  flags;                                          // 4, unknown value but 312 for all cities
                                                            // 5-9 unused
    int32   area_level;                                     // 10
    char*   area_name[16];                                  // 11-26
                                                            // 27, string flags, unused
    uint32  team;                                           // 28
};

struct AreaTriggerEntry
{
    uint32  id;                                             // 0 m_ID
    uint32  mapid;                                          // 1 m_ContinentID
    float   x;                                              // 2 m_x
    float   y;                                              // 3 m_y
    float   z;                                              // 4 m_z
    float   radius;                                         // 5 m_radius
    float   box_x;                                          // 6 m_box_length extent x edge
    float   box_y;                                          // 7 m_box_width extent y edge
    float   box_z;                                          // 8 m_box_heigh extent z edge
    float   box_orientation;                                // 9 m_box_yaw extent rotation by about z axis
};

struct BankBagSlotPricesEntry
{
    uint32  ID;
    uint32  price;
};

struct BarberShopStyleEntry
{
    uint32  Id;                                             // 0
    //uint32  type;                                         // 1 value 0 -> hair, value 2 -> facialhair
    //char*   name[16];                                     // 2-17 name of hair style
    //uint32  name_flags;                                   // 18
    //uint32  unk_name[16];                                 // 19-34, all empty
    //uint32  unk_flags;                                    // 35
    //float   unk3;                                         // 36 values 1 and 0,75
    //uint32  race;                                         // 37 race
    //uint32  gender;                                       // 38 0 -> male, 1 -> female
    uint32  hair_id;                                        // 39 real ID to hair/facial hair
};

struct BattlemasterListEntry
{
    uint32  id;                                             // 0
    int32   mapid[8];                                       // 1-8 mapid
    uint32  type;                                           // 9 (3 - BG, 4 - arena)
    uint32  minlvl;                                         // 10
    uint32  maxlvl;                                         // 11
    uint32  maxplayersperteam;                              // 12
                                                            // 13 minplayers
                                                            // 14 0 or 9
                                                            // 15
    char*   name[16];                                       // 16-31
                                                            // 32 string flag, unused
                                                            // 33 unused
};

struct CharTitlesEntry
{
    uint32  ID;                                             // 0, title ids, for example in Quest::GetCharTitleId()
    //uint32      unk1;                                     // 1 flags?
    //char*       name[16];                                 // 2-17, unused
                                                            // 18 string flag, unused
    //char*       name2[16];                                // 19-34, unused
                                                            // 35 string flag, unused
    uint32  bit_index;                                      // 36 used in PLAYER_CHOSEN_TITLE and 1<<index in PLAYER__FIELD_KNOWN_TITLES
};

struct ChatChannelsEntry
{
    uint32  ChannelID;                                      // 0
    uint32  flags;                                          // 1
    char*   pattern[16];                                    // 3-18
                                                            // 19 string flags, unused
    //char*       name[16];                                 // 20-35 unused
                                                            // 36 string flag, unused
};

struct ChrClassesEntry
{
    uint32  ClassID;                                        // 0
                                                            // 1-2, unused
    uint32  powerType;                                      // 3
                                                            // 4, unused
    //char*       name[16];                                 // 5-20 unused
                                                            // 21 string flag, unused
    //char*       string1[16];                              // 21-36 unused
                                                            // 37 string flag, unused
    //char*       string2[16];                              // 38-53 unused
                                                            // 54 string flag, unused
                                                            // 55, unused
    uint32  spellfamily;                                    // 56
                                                            // 57, unused
    uint32  CinematicSequence;                              // 58 id from CinematicSequences.dbc
};

struct ChrRacesEntry
{
    uint32      RaceID;                                     // 0
                                                            // 1 unused
    uint32      FactionID;                                  // 2 facton template id
                                                            // 3 unused
    uint32      model_m;                                    // 4
    uint32      model_f;                                    // 5
                                                            // 6-7 unused
    uint32      TeamID;                                     // 8 (7-Alliance 1-Horde)
                                                            // 9-12 unused
    uint32      CinematicSequence;                          // 13 id from CinematicSequences.dbc
    char*       name[16];                                   // 14-29 used for DBC language detection/selection
                                                            // 30 string flags, unused
    //char*       string1[16];                              // 31-46 used for DBC language detection/selection
                                                            // 47 string flags, unused
    //char*       string2[16];                              // 48-63 used for DBC language detection/selection
                                                            // 64 string flags, unused
                                                            // 65-67 unused
    uint32      addon;                                      // 68 (0 - original race, 1 - tbc addon, ...)
};

struct CreatureDisplayInfoEntry
{
    uint32      Displayid;                                  // 0    m_ID
                                                            // 1    m_modelID
                                                            // 2    m_soundID
                                                            // 3    m_extendedDisplayInfoID
    float       scale;                                      // 4    m_creatureModelScale
                                                            // 5    m_creatureModelAlpha
                                                            // 6-8  m_textureVariation[3]
                                                            // 9    m_portraitTextureName
                                                            // 10   m_sizeClass
                                                            // 11   m_bloodID
                                                            // 12   m_NPCSoundID
                                                            // 13   m_particleColorID
                                                            // 14   m_creatureGeosetData
                                                            // 15   m_objectEffectPackageID
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
    uint32  petTalentType;                                  // 8        m_petTalentType
                                                            // 9        m_categoryEnumID
    char*   Name[16];                                       // 10-25    m_name_lang
                                                            // 26 string flags, unused
                                                            // 27       m_iconFile unused
};

struct CreatureSpellDataEntry
{
    uint32    ID;                                           // 0    m_ID
    //uint32    spellId[4];                                 // 1-4  m_spells hunter pet learned spell (for later use)
    //uint32    availability[4];                            // 4-7  m_availability
};

struct DurabilityCostsEntry
{
    uint32    Itemlvl;                                      // 0
    uint32    multiplier[29];                               // 1-29
};

struct DurabilityQualityEntry
{
    uint32    Id;                                           // 0
    float     quality_mod;                                  // 1
};

struct EmotesTextEntry
{
    uint32  Id;
    uint32  textid;
};

struct FactionEntry
{
    uint32      ID;                                         // 0        m_ID
    int32       reputationListID;                           // 1        m_reputationIndex
    uint32      BaseRepRaceMask[4];                         // 2-5      m_reputationRaceMask Base reputation race masks (see enum Races)
    uint32      BaseRepClassMask[4];                        // 6-9      m_reputationClassMask Base reputation class masks (see enum Classes)
    int32       BaseRepValue[4];                            // 10-13    m_reputationBase Base reputation values
    uint32      ReputationFlags[4];                         // 14-17    m_reputationFlags Default flags to apply
    uint32      team;                                       // 18       m_parentFactionID enum Team
    char*       name[16];                                   // 19-34    m_name_lang
                                                            // 35 string flags, unused
    //char*     description[16];                            // 36-51    m_description_lang unused
                                                            // 52 string flags, unused
};

struct FactionTemplateEntry
{
    uint32      ID;                                         // 0 m_ID
    uint32      faction;                                    // 1 m_faction
    uint32      factionFlags;                               // 2 m_flags specific flags for that faction
    uint32      ourMask;                                    // 3 m_factionGroup if mask set (see FactionMasks) then faction included in masked team
    uint32      friendlyMask;                               // 4 m_friendGroup if mask set (see FactionMasks) then faction friendly to masked team
    uint32      hostileMask;                                // 5 m_enemyGroup if mask set (see FactionMasks) then faction hostile to masked team
    uint32      enemyFaction1;                              // 6 m_enemies[4]
    uint32      enemyFaction2;                              // 7
    uint32      enemyFaction3;                              // 8
    uint32      enemyFaction4;                              // 9
    uint32      friendFaction1;                             // 10 m_friend[4]
    uint32      friendFaction2;                             // 11
    uint32      friendFaction3;                             // 12
    uint32      friendFaction4;                             // 13
    //-------------------------------------------------------  end structure

    // helpers
    bool IsFriendlyTo(FactionTemplateEntry const& entry) const
    {
        if(enemyFaction1  == entry.faction || enemyFaction2  == entry.faction || enemyFaction3 == entry.faction || enemyFaction4 == entry.faction )
            return false;
        if(friendFaction1 == entry.faction || friendFaction2 == entry.faction || friendFaction3 == entry.faction || friendFaction4 == entry.faction )
            return true;
        return (friendlyMask & entry.ourMask) || (ourMask & entry.friendlyMask);
    }
    bool IsHostileTo(FactionTemplateEntry const& entry) const
    {
        if(enemyFaction1  == entry.faction || enemyFaction2  == entry.faction || enemyFaction3 == entry.faction || enemyFaction4 == entry.faction )
            return true;
        if(friendFaction1 == entry.faction || friendFaction2 == entry.faction || friendFaction3 == entry.faction || friendFaction4 == entry.faction )
            return false;
        return (hostileMask & entry.ourMask) != 0;
    }
    bool IsHostileToPlayers() const { return (hostileMask & FACTION_MASK_PLAYER) !=0; }
    bool IsNeutralToAll() const { return hostileMask == 0 && friendlyMask == 0 && enemyFaction1==0 && enemyFaction2==0 && enemyFaction3==0 && enemyFaction4==0; }
    bool IsContestedGuardFaction() const { return (factionFlags & FACTION_TEMPLATE_FLAG_CONTESTED_GUARD)!=0; }
};

struct GemPropertiesEntry
{
    uint32      ID;
    uint32      spellitemenchantement;
    uint32      color;
};

struct GlyphPropertiesEntry
{
    uint32  Id;
    uint32  SpellId;
    uint32  TypeFlags;
    uint32  Unk1;
};

struct GlyphSlotEntry
{
    uint32  Id;
    uint32  TypeFlags;
    uint32  Order;
};

#define GT_MAX_LEVEL    100

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

struct ItemEntry
{
   uint32   ID;
   //uint32   Class;
   //uint32   SubClass;
   //uint32   Unk0;
   //uint32   Material;
   uint32   DisplayId;
   uint32   InventoryType;
   uint32   Sheath;
};

struct ItemDisplayInfoEntry
{
    uint32      ID;
    uint32      randomPropertyChance;
};

//struct ItemCondExtCostsEntry
//{
//    uint32      ID;
//    uint32      condExtendedCost;                         // ItemPrototype::CondExtendedCost
//    uint32      itemextendedcostentry;                    // ItemPrototype::ExtendedCost
//    uint32      arenaseason;                              // arena season number(1-4)
//};

struct ItemExtendedCostEntry
{
    uint32      ID;                                         // 0 extended-cost entry id
    uint32      reqhonorpoints;                             // 1 required honor points
    uint32      reqarenapoints;                             // 2 required arena points
    uint32      reqitem[5];                                 // 3-7 required item id
    uint32      reqitemcount[5];                            // 8-12 required count of 1st item
    uint32      reqpersonalarenarating;                     // 13 required personal arena rating
};

struct ItemRandomPropertiesEntry
{
    uint32    ID;                                           // 0    m_ID
    //char*     internalName                                // 1    m_Name
    uint32    enchant_id[5];                                // 2-6  m_Enchantment
    //char*     nameSuffix[16]                              // 7-22 m_name_lang
                                                            // 23 nameSufix flags
};

struct ItemRandomSuffixEntry
{
    uint32    ID;                                           // 0        m_ID
    //char*     name[16]                                    // 1-16     m_name_lang unused
                                                            // 17, name flags, unused
                                                            // 18       m_internalName, unused
    uint32    enchant_id[5];                                // 19-21    m_enchantment
    uint32    prefix[5];                                    // 22-24    m_allocationPct
};

struct ItemSetEntry
{
    //uint32    id                                          // 0        m_ID
    char*     name[16];                                     // 1-16     m_name_lang
                                                            // 17 string flags, unused
    //uint32    itemId[17];                                 // 18-34    m_itemID
    uint32    spells[8];                                    // 35-42    m_setSpellID
    uint32    items_to_triggerspell[8];                     // 43-50    m_setThreshold
    uint32    required_skill_id;                            // 51       m_requiredSkill
    uint32    required_skill_value;                         // 52       m_requiredSkillRank
};

struct LockEntry
{
    uint32      ID;                                         // 0        m_ID
    uint32      Type[8];                                    // 1-8      m_Type
    uint32      Index[8];                                   // 9-16     m_Index
    uint32      Skill[8];                                   // 17-24    m_Skill
    //uint32      Action[8];                                // 25-32    m_Action
};

struct MailTemplateEntry
{
    uint32      ID;                                         // 0
    //char*       subject[16];                              // 1-16
                                                            // 17 name flags, unused
    //char*       content[16];                              // 18-33
};

struct MapEntry
{
    uint32      MapID;                                      // 0
    //char*       internalname;                             // 1 unused
    uint32      map_type;                                   // 2
                                                            // 3 0 or 1 for battlegrounds (not arenas)
    char*       name[16];                                   // 4-19
                                                            // 20 name flags, unused
    uint32      linked_zone;                                // 21 common zone for instance and continent map
    //char*     hordeIntro[16];                             // 23-37 text for PvP Zones
                                                            // 38 intro text flags
    //char*     allianceIntro[16];                          // 39-54 text for PvP Zones
                                                            // 55 intro text flags
    uint32      multimap_id;                                // 56
                                                            // 57
    //chat*     unknownText1[16];                           // 58-73 unknown empty text fields, possible normal Intro text.
                                                            // 74 text flags
    //chat*     heroicIntroText[16];                        // 75-90 heroic mode requirement text
                                                            // 91 text flags
    //chat*     unknownText2[16];                           // 92-107 unknown empty text fields
                                                            // 108 text flags
    int32       parent_map;                                 // 109 map_id of parent map
    //float start_x                                         // 110 enter x coordinate (if exist single entry)
    //float start_y                                         // 111 enter y coordinate (if exist single entry)
    uint32 resetTimeRaid;                                   // 112
    uint32 resetTimeHeroic;                                 // 113
                                                            // 114 all 0
                                                            // 115 -1, 0 and 720
    uint32      addon;                                      // 116 (0-original maps,1-tbc addon)
                                                            // 117 some kind of time?

    // Helpers
    uint32 Expansion() const { return addon; }
    bool Instanceable() const { return map_type == MAP_INSTANCE || map_type == MAP_RAID; }
    // NOTE: this duplicate of Instanceable(), but Instanceable() can be changed when BG also will be instanceable
    bool IsDungeon() const { return map_type == MAP_INSTANCE || map_type == MAP_RAID; }
    bool IsRaid() const { return map_type == MAP_RAID; }
    bool IsBattleGround() const { return map_type == MAP_BATTLEGROUND; }
    bool IsBattleArena() const { return map_type == MAP_ARENA; }
    bool IsBattleGroundOrArena() const { return map_type == MAP_BATTLEGROUND || map_type == MAP_ARENA; }
    bool SupportsHeroicMode() const { return resetTimeHeroic && !resetTimeRaid; }
    bool HasResetTime() const { return resetTimeHeroic || resetTimeRaid; }

    bool IsMountAllowed() const
    {
        return !IsDungeon() ||
            MapID==568 || MapID==309 || MapID==209 || MapID==534 ||
            MapID==560 || MapID==509 || MapID==269;
    }
};

struct QuestSortEntry
{
    uint32      id;                                         // 0, sort id
    //char*       name[16];                                 // 1-16, unused
                                                            // 17 name flags, unused
};

struct RandomPropertiesPointsEntry
{
    //uint32  Id;                                           // 0 hidden key
    uint32    itemLevel;                                    // 1
    uint32    EpicPropertiesPoints[5];                      // 2-6
    uint32    RarePropertiesPoints[5];                      // 7-11
    uint32    UncommonPropertiesPoints[5];                  // 12-16
};

struct ScalingStatDistributionEntry
{
    uint32  Id;
    uint32  StatMod[10];
    uint32  Modifier[10];
    uint32  MaxLevel;
};

struct ScalingStatValuesEntry
{
    uint32  Id;
    uint32  Level;
    uint32  Multiplier[17];
};

//struct SkillLineCategoryEntry{
//    uint32    id;                                         // 0      m_ID
//    char*     name[16];                                   // 1-17   m_name_lang
//                                                          // 18 string flag
//    uint32    displayOrder;                               // 19     m_sortIndex
//};

//struct SkillRaceClassInfoEntry{
//    uint32    id;                                         // 0      m_ID
//    uint32    skillId;                                    // 1      m_skillID
//    uint32    raceMask;                                   // 2      m_raceMask
//    uint32    classMask;                                  // 3      m_classMask
//    uint32    flags;                                      // 4      m_flags
//    uint32    reqLevel;                                   // 5      m_minLevel
//    uint32    skillTierId;                                // 6      m_skillTierID
//    uint32    skillCostID;                                // 7      m_skillCostIndex
//};

//struct SkillTiersEntry{
//    uint32    id;                                         // 0      m_ID
//    uint32    skillValue[16];                             // 1-17   m_cost
//    uint32    maxSkillValue[16];                          // 18-32  m_valueMax
//};

struct SkillLineEntry
{
    uint32    id;                                           // 0        m_ID
    uint32    categoryId;                                   // 1        m_categoryID (index from SkillLineCategory.dbc)
    //uint32    skillCostID;                                // 2        m_skillCostsID not used
    char*     name[16];                                     // 3-18     m_displayName_lang
                                                            // 19 string flags, not used
    //char*     description[16];                            // 20-35    m_description_lang, not used
                                                            // 36 string flags, not used
    uint32    spellIcon;                                    // 37       m_spellIconID
    //char*     alternateVerb[16];                          // 38-53    m_alternateVerb_lang
                                                            // 54 string flags, not used
                                                            // 55       m_canLink
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
    //uint32    characterPoints[2];                         // 12-13    m_characterPoints
};

struct SoundEntriesEntry
{
    uint32    Id;                                           // 0        m_ID
    //uint32    Type;                                       // 1        m_soundType
    //char*     InternalName;                               // 2        m_name
    //char*     FileName[10];                               // 3-12     m_File
    //uint32    Unk13[10];                                  // 13-22    m_Freq
    //char*     Path;                                       // 23       m_DirectoryBase
                                                            // 24       m_volumeFloat
                                                            // 25       m_flags
                                                            // 26       m_minDistance
                                                            // 27       m_distanceCutoff
                                                            // 28       m_EAXDef
};

struct SpellEntry
{
    uint32    Id;                                           // 0        m_ID
    uint32    Category;                                     // 1        m_category
    uint32    Dispel;                                       // 2        m_dispelType
    uint32    Mechanic;                                     // 3        m_mechanic
    uint32    Attributes;                                   // 4        m_attribute
    uint32    AttributesEx;                                 // 5        m_attributesEx
    uint32    AttributesEx2;                                // 6        m_attributesExB
    uint32    AttributesEx3;                                // 7        m_attributesExC
    uint32    AttributesEx4;                                // 8        m_attributesExD
    uint32    AttributesEx5;                                // 9        m_attributesExE
    //uint32    AttributesEx6;                              // 10       m_attributesExF not used
    uint32    Stances;                                      // 11       m_shapeshiftMask
    uint32    StancesNot;                                   // 12       m_shapeshiftExclude
    uint32    Targets;                                      // 13       m_targets
    uint32    TargetCreatureType;                           // 14       m_targetCreatureType
    uint32    RequiresSpellFocus;                           // 15       m_requiresSpellFocus
    uint32    FacingCasterFlags;                            // 16       m_facingCasterFlags
    uint32    CasterAuraState;                              // 17       m_casterAuraState
    uint32    TargetAuraState;                              // 18       m_targetAuraState
    uint32    CasterAuraStateNot;                           // 19       m_excludeCasterAuraState
    uint32    TargetAuraStateNot;                           // 20       m_excludeTargetAuraState
    //uint32    casterAuraSpell;                            // 21       m_casterAuraSpell not used
    //uint32    targetAuraSpell;                            // 22       m_targetAuraSpell not used
    //uint32    excludeCasterAuraSpell;                     // 23       m_excludeCasterAuraSpell not used
    //uint32    excludeTargetAuraSpell;                     // 24       m_excludeTargetAuraSpell not used
    uint32    CastingTimeIndex;                             // 25       m_castingTimeIndex
    uint32    RecoveryTime;                                 // 26       m_recoveryTime
    uint32    CategoryRecoveryTime;                         // 27       m_categoryRecoveryTime
    uint32    InterruptFlags;                               // 28       m_interruptFlags
    uint32    AuraInterruptFlags;                           // 29       m_auraInterruptFlags
    uint32    ChannelInterruptFlags;                        // 30       m_channelInterruptFlags
    uint32    procFlags;                                    // 31       m_procTypeMask
    uint32    procChance;                                   // 32       m_procChance
    uint32    procCharges;                                  // 33       m_procCharges
    uint32    maxLevel;                                     // 34       m_maxLevel
    uint32    baseLevel;                                    // 35       m_baseLevel
    uint32    spellLevel;                                   // 36       m_spellLevel
    uint32    DurationIndex;                                // 37       m_durationIndex
    uint32    powerType;                                    // 38       m_powerType
    uint32    manaCost;                                     // 39       m_manaCost
    uint32    manaCostPerlevel;                             // 40       m_manaCostPerLevel
    uint32    manaPerSecond;                                // 41       m_manaPerSecond
    uint32    manaPerSecondPerLevel;                        // 42       m_manaPerSecondPerLeve
    uint32    rangeIndex;                                   // 43       m_rangeIndex
    float     speed;                                        // 44       m_speed
    //uint32    modalNextSpell;                             // 45       m_modalNextSpell not used
    uint32    StackAmount;                                  // 46       m_cumulativeAura
    uint32    Totem[2];                                     // 47-48    m_totem
    int32     Reagent[8];                                   // 49-56    m_reagent
    uint32    ReagentCount[8];                              // 57-64    m_reagentCount
    int32     EquippedItemClass;                            // 65       m_equippedItemClass (value)
    int32     EquippedItemSubClassMask;                     // 66       m_equippedItemSubclass (mask)
    int32     EquippedItemInventoryTypeMask;                // 67       m_equippedItemInvTypes (mask)
    uint32    Effect[3];                                    // 68-70    m_effect
    int32     EffectDieSides[3];                            // 71-73    m_effectDieSides
    uint32    EffectBaseDice[3];                            // 74-76    m_effectBaseDice
    float     EffectDicePerLevel[3];                        // 77-79    m_effectDicePerLevel
    float     EffectRealPointsPerLevel[3];                  // 80-82    m_effectRealPointsPerLevel
    int32     EffectBasePoints[3];                          // 83-85    m_effectBasePoints (don't must be used in spell/auras explicitly, must be used cached Spell::m_currentBasePoints)
    uint32    EffectMechanic[3];                            // 86-88    m_effectMechanic
    uint32    EffectImplicitTargetA[3];                     // 89-91    m_implicitTargetA
    uint32    EffectImplicitTargetB[3];                     // 92-94    m_implicitTargetB
    uint32    EffectRadiusIndex[3];                         // 95-97    m_effectRadiusIndex - spellradius.dbc
    uint32    EffectApplyAuraName[3];                       // 98-100   m_effectAura
    uint32    EffectAmplitude[3];                           // 101-103  m_effectAuraPeriod
    float     EffectMultipleValue[3];                       // 104-106  m_effectAmplitude
    uint32    EffectChainTarget[3];                         // 107-109  m_effectChainTargets
    uint32    EffectItemType[3];                            // 110-112  m_effectItemType
    int32     EffectMiscValue[3];                           // 113-115  m_effectMiscValue
    int32     EffectMiscValueB[3];                          // 116-118  m_effectMiscValueB
    uint32    EffectTriggerSpell[3];                        // 119-121  m_effectTriggerSpell
    float     EffectPointsPerComboPoint[3];                 // 122-124  m_effectPointsPerCombo
    //uint32    EffectSpellClassMaskA[3];                   // 125-127  m_effectSpellClassMaskA not used
    //uint32    EffectSpellClassMaskB[3];                   // 128-130  m_effectSpellClassMaskB not used
    //uint32    EffectSpellClassMaskC[3];                   // 131-133  m_effectSpellClassMaskC not used
    uint32    SpellVisual;                                  // 134      m_spellVisualID
                                                            // 135 not used - no data and name in client?
    uint32    SpellIconID;                                  // 136      m_spellIconID
    uint32    activeIconID;                                 // 137      m_activeIconID
    //uint32    spellPriority;                              // 138      m_spellPriority not used
    char*     SpellName[16];                                // 139-154  m_name_lang
    //uint32    SpellNameFlag;                              // 155 not used
    char*     Rank[16];                                     // 156-171  m_nameSubtext_lang
    //uint32    RankFlags;                                  // 172 not used
    //char*     Description[16];                            // 173-188  m_description_lang not used
    //uint32    DescriptionFlags;                           // 189 not used
    //char*     ToolTip[16];                                // 190-205  m_auraDescription_lang not used
    //uint32    ToolTipFlags;                               // 206 not used
    uint32    ManaCostPercentage;                           // 207      m_manaCostPct
    uint32    StartRecoveryCategory;                        // 208      m_startRecoveryCategory
    uint32    StartRecoveryTime;                            // 209      m_startRecoveryTime
    uint32    MaxTargetLevel;                               // 210      m_maxTargetLevel
    uint32    SpellFamilyName;                              // 211      m_spellClassSet
    uint64    SpellFamilyFlags;                             // 212-213  m_spellClassMask
    uint32    MaxAffectedTargets;                           // 214      m_maxTargets
    uint32    DmgClass;                                     // 215      m_defenseType
    uint32    PreventionType;                               // 216      m_preventionType
    //uint32    StanceBarOrder;                             // 217      m_stanceBarOrder not used
    float     DmgMultiplier[3];                             // 218-220  m_effectChainAmplitude
    //uint32    MinFactionId;                               // 221      m_minFactionID not used
    //uint32    MinReputation;                              // 222      m_minReputation not used
    //uint32    RequiredAuraVision;                         // 223      m_requiredAuraVision not used
    uint32    TotemCategory[2];                             // 224-225  m_requiredTotemCategoryID
    int32     AreaId;                                       // 226      m_requiredAreasID
    uint32    SchoolMask;                                   // 227      m_schoolMask
    uint32    runeCostID;                                   // 228      m_runeCostID
    //uint32    spellMissileID;                             // 229      m_spellMissileID not used

    private:
        // prevent creating custom entries (copy data from original in fact)
        SpellEntry(SpellEntry const&);                      // DON'T must have implementation
};

typedef std::set<uint32> SpellCategorySet;
typedef std::map<uint32,SpellCategorySet > SpellCategoryStore;
typedef std::set<uint32> PetFamilySpellsSet;
typedef std::map<uint32,PetFamilySpellsSet > PetFamilySpellsStore;

struct SpellCastTimesEntry
{
    uint32    ID;                                           // 0
    int32     CastTime;                                     // 1
    //float     CastTimePerLevel;                           // 2 unsure / per skill?
    //int32     MinCastTime;                                // 3 unsure
};

struct SpellFocusObjectEntry
{
    uint32    ID;                                           // 0
    //char*     Name[16];                                   // 1-15 unused
                                                            // 16 string flags, unused
};

// stored in SQL table
struct SpellThreatEntry
{
    uint32      spellId;
    int32       threat;
};

struct SpellRadiusEntry
{
    uint32    ID;
    float     Radius;
    float     Radius2;
};

struct SpellRangeEntry
{
    uint32    ID;
    float     minRange;
    float     maxRange;
};

struct SpellRuneCostEntry
{
    uint32  ID;
    uint32  bloodRuneCost;
    uint32  frostRuneCost;
    uint32  unholyRuneCost;
    uint32  runePowerGain;
};

struct SpellShapeshiftEntry
{
    uint32 ID;                                              // 0
    //uint32 buttonPosition;                                // 1 unused
    //char*  Name[16];                                      // 2-17 unused
    //uint32 NameFlags;                                     // 18 unused
    uint32 flags1;                                          // 19
    int32  creatureType;                                    // 20 <=0 humanoid, other normal creature types
    //uint32 unk1;                                          // 21 unused
    uint32 attackSpeed;                                     // 22
    //uint32 modelID;                                       // 23 unused, alliance modelid (where horde case?)
    //uint32 unk2;                                          // 24 unused
    //uint32 unk3;                                          // 25 unused
    //uint32 unk4;                                          // 26 unused
    //uint32 unk5;                                          // 27 unused
    //uint32 unk6;                                          // 28 unused
    //uint32 unk7;                                          // 29 unused
    //uint32 unk8;                                          // 30 unused
    //uint32 unk9;                                          // 31 unused
    //uint32 unk10;                                         // 32 unused
    //uint32 unk11;                                         // 33 unused
    //uint32 unk12;                                         // 34 unused
};

struct SpellDurationEntry
{
    uint32    ID;
    int32     Duration[3];
};

struct SpellItemEnchantmentEntry
{
    uint32      ID;                                         // 0 m_ID
    //uint32      charges;                                  // 1 m_charges
    uint32      type[3];                                    // 2-4 m_effect
    uint32      amount[3];                                  // 5-7 m_effectPointsMin
    //uint32      amount2[3]                                // 8-10 m_effectPointsMax
    uint32      spellid[3];                                 // 11-13 m_effectArg
    char*       description[16];                            // 14-30 m_name_lang
    //uint32      descriptionFlags;                         // 31 name flags
    uint32      aura_id;                                    // 32 m_itemVisual
    uint32      slot;                                       // 33 m_flags
    uint32      GemID;                                      // 34 m_src_itemID
    uint32      EnchantmentCondition;                       // 35 m_condition_id
    //uint32      requiredSkill;                            // 36 m_requiredSkillID
    //uint32      requiredSkillValue;                       // 37 m_requiredSkillRank
};

struct SpellItemEnchantmentConditionEntry
{
    uint32  ID;
    uint8   Color[5];
    uint8   Comparator[5];
    uint8   CompareColor[5];
    uint32  Value[5];
};

struct StableSlotPricesEntry
{
    uint32 Slot;
    uint32 Price;
};

struct TalentEntry
{
    uint32    TalentID;                                     // 0
    uint32    TalentTab;                                    // 1 index in TalentTab.dbc (TalentTabEntry)
    uint32    Row;                                          // 2
    uint32    Col;                                          // 3
    uint32    RankID[5];                                    // 4-8
                                                            // 9-12 not used, always 0, maybe not used high ranks
    uint32    DependsOn;                                    // 13 index in Talent.dbc (TalentEntry)
                                                            // 14-15 not used
    uint32    DependsOnRank;                                // 16
                                                            // 17-18 not used
    //uint32  unk1;                                         // 19, 0 or 1
    //uint32  unk2;                                         // 20, all 0
    //uint32  unkFlags1;                                    // 21, related to hunter pet talents
    //uint32  unkFlags2;                                    // 22, related to hunter pet talents
};

struct TalentTabEntry
{
    uint32  TalentTabID;                                    // 0
    //char* name[16];                                       // 1-16, unused
    //uint32  nameFlags;                                    // 17, unused
    //unit32  spellicon;                                    // 18
                                                            // 19 not used
    uint32  ClassMask;                                      // 20
    uint32  petTalentMask;                                  // 21
    uint32  tabpage;                                        // 22
    //char* internalname;                                   // 23
};

struct TaxiNodesEntry
{
    uint32    ID;                                           // 0    m_ID
    uint32    map_id;                                       // 1    m_ContinentID
    float     x;                                            // 2    m_x
    float     y;                                            // 3    m_y
    float     z;                                            // 4    m_z
    //char*     name[16];                                   // 5-21 m_Name_lang
                                                            // 22 string flags, unused
    uint32    horde_mount_type;                             // 23   m_MountCreatureID[2]
    uint32    alliance_mount_type;                          // 24
};

struct TaxiPathEntry
{
    uint32    ID;                                           // 0 m_ID
    uint32    from;                                         // 1 m_FromTaxiNode
    uint32    to;                                           // 2 m_ToTaxiNode
    uint32    price;                                        // 3 m_Cost
};

struct TaxiPathNodeEntry
{
                                                            // 0    m_ID
    uint32    path;                                         // 1    m_PathID
    uint32    index;                                        // 2    m_NodeIndex
    uint32    mapid;                                        // 3    m_ContinentID
    float     x;                                            // 4    m_LocX
    float     y;                                            // 5    m_LocY
    float     z;                                            // 6    m_LocZ
    uint32    actionFlag;                                   // 7    m_flags
    uint32    delay;                                        // 8    m_delay
                                                            // 9    m_arrivalEventID
                                                            // 10   m_departureEventID
};

struct TotemCategoryEntry
{
    uint32    ID;                                           // 0
    //char*   name[16];                                     // 1-16
                                                            // 17 string flags, unused
    uint32    categoryType;                                 // 18 (one for specialization)
    uint32    categoryMask;                                 // 19 (compatibility mask for same type: different for totems, compatible from high to low for rods)
};

struct WorldMapAreaEntry
{
    //uint32  ID;                                           // 0
    uint32  map_id;                                         // 1
    uint32  area_id;                                        // 2 index (continent 0 areas ignored)
    //char* internal_name                                   // 3
    float   y1;                                             // 4
    float   y2;                                             // 5
    float   x1;                                             // 6
    float   x2;                                             // 7
    int32   virtual_map_id;                                 // 8 -1 (map_id have correct map) other: virtual map where zone show (map_id - where zone in fact internally)
};

struct WorldSafeLocsEntry
{
    uint32    ID;                                           // 0
    uint32    map_id;                                       // 1
    float     x;                                            // 2
    float     y;                                            // 3
    float     z;                                            // 4
    //char*   name[16]                                      // 5-20 name, unused
                                                            // 21 name flags, unused
};

// GCC have alternative #pragma pack() syntax and old gcc version not support pack(pop), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

// Structures not used for casting to loaded DBC data and not required then packing
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

struct TaxiPathNode
{
    TaxiPathNode() : mapid(0), x(0),y(0),z(0),actionFlag(0),delay(0) {}
    TaxiPathNode(uint32 _mapid, float _x, float _y, float _z, uint32 _actionFlag, uint32 _delay) : mapid(_mapid), x(_x),y(_y),z(_z),actionFlag(_actionFlag),delay(_delay) {}

    uint32    mapid;
    float     x;
    float     y;
    float     z;
    uint32    actionFlag;
    uint32    delay;
};
typedef std::vector<TaxiPathNode> TaxiPathNodeList;
typedef std::vector<TaxiPathNodeList> TaxiPathNodesByPath;

#define TaxiMaskSize 12
typedef uint32 TaxiMask[TaxiMaskSize];
#endif
