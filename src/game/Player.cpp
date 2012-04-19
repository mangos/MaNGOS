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

#include "Player.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "Opcodes.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "UpdateMask.h"
#include "SkillDiscovery.h"
#include "QuestDef.h"
#include "GossipDef.h"
#include "UpdateData.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "MapManager.h"
#include "MapPersistentStateMgr.h"
#include "InstanceData.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "Formulas.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Pet.h"
#include "Util.h"
#include "Transports.h"
#include "Weather.h"
#include "BattleGround.h"
#include "BattleGroundAV.h"
#include "BattleGroundMgr.h"
#include "ArenaTeam.h"
#include "Chat.h"
#include "Database/DatabaseImpl.h"
#include "Spell.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "AchievementMgr.h"
#include "Mail.h"
#include "SpellAuras.h"

#include <cmath>

#define ZONE_UPDATE_INTERVAL (1*IN_MILLISECONDS)

#define PLAYER_SKILL_INDEX(x)       (PLAYER_SKILL_INFO_1_1 + ((x)*3))
#define PLAYER_SKILL_VALUE_INDEX(x) (PLAYER_SKILL_INDEX(x)+1)
#define PLAYER_SKILL_BONUS_INDEX(x) (PLAYER_SKILL_INDEX(x)+2)

#define SKILL_VALUE(x)         PAIR32_LOPART(x)
#define SKILL_MAX(x)           PAIR32_HIPART(x)
#define MAKE_SKILL_VALUE(v, m) MAKE_PAIR32(v,m)

#define SKILL_TEMP_BONUS(x)    int16(PAIR32_LOPART(x))
#define SKILL_PERM_BONUS(x)    int16(PAIR32_HIPART(x))
#define MAKE_SKILL_BONUS(t, p) MAKE_PAIR32(t,p)

enum CharacterFlags
{
    CHARACTER_FLAG_NONE                 = 0x00000000,
    CHARACTER_FLAG_UNK1                 = 0x00000001,
    CHARACTER_FLAG_UNK2                 = 0x00000002,
    CHARACTER_LOCKED_FOR_TRANSFER       = 0x00000004,
    CHARACTER_FLAG_UNK4                 = 0x00000008,
    CHARACTER_FLAG_UNK5                 = 0x00000010,
    CHARACTER_FLAG_UNK6                 = 0x00000020,
    CHARACTER_FLAG_UNK7                 = 0x00000040,
    CHARACTER_FLAG_UNK8                 = 0x00000080,
    CHARACTER_FLAG_UNK9                 = 0x00000100,
    CHARACTER_FLAG_UNK10                = 0x00000200,
    CHARACTER_FLAG_HIDE_HELM            = 0x00000400,
    CHARACTER_FLAG_HIDE_CLOAK           = 0x00000800,
    CHARACTER_FLAG_UNK13                = 0x00001000,
    CHARACTER_FLAG_GHOST                = 0x00002000,
    CHARACTER_FLAG_RENAME               = 0x00004000,
    CHARACTER_FLAG_UNK16                = 0x00008000,
    CHARACTER_FLAG_UNK17                = 0x00010000,
    CHARACTER_FLAG_UNK18                = 0x00020000,
    CHARACTER_FLAG_UNK19                = 0x00040000,
    CHARACTER_FLAG_UNK20                = 0x00080000,
    CHARACTER_FLAG_UNK21                = 0x00100000,
    CHARACTER_FLAG_UNK22                = 0x00200000,
    CHARACTER_FLAG_UNK23                = 0x00400000,
    CHARACTER_FLAG_UNK24                = 0x00800000,
    CHARACTER_FLAG_LOCKED_BY_BILLING    = 0x01000000,
    CHARACTER_FLAG_DECLINED             = 0x02000000,
    CHARACTER_FLAG_UNK27                = 0x04000000,
    CHARACTER_FLAG_UNK28                = 0x08000000,
    CHARACTER_FLAG_UNK29                = 0x10000000,
    CHARACTER_FLAG_UNK30                = 0x20000000,
    CHARACTER_FLAG_UNK31                = 0x40000000,
    CHARACTER_FLAG_UNK32                = 0x80000000
};

enum CharacterCustomizeFlags
{
    CHAR_CUSTOMIZE_FLAG_NONE            = 0x00000000,
    CHAR_CUSTOMIZE_FLAG_CUSTOMIZE       = 0x00000001,       // name, gender, etc...
    CHAR_CUSTOMIZE_FLAG_FACTION         = 0x00010000,       // name, gender, faction, etc...
    CHAR_CUSTOMIZE_FLAG_RACE            = 0x00100000        // name, gender, race, etc...
};

// corpse reclaim times
#define DEATH_EXPIRE_STEP (5*MINUTE)
#define MAX_DEATH_COUNT 3

static uint32 copseReclaimDelay[MAX_DEATH_COUNT] = { 30, 60, 120 };

//== PlayerTaxi ================================================

PlayerTaxi::PlayerTaxi()
{
    // Taxi nodes
    memset(m_taximask, 0, sizeof(m_taximask));
}

void PlayerTaxi::InitTaxiNodesForLevel(uint32 race, uint32 chrClass, uint32 level)
{
    // class specific initial known nodes
    switch(chrClass)
    {
        case CLASS_DEATH_KNIGHT:
        {
            for(int i = 0; i < TaxiMaskSize; ++i)
                m_taximask[i] |= sOldContinentsNodesMask[i];
            break;
        }
    }

    // race specific initial known nodes: capital and taxi hub masks
    switch(race)
    {
        case RACE_HUMAN:    SetTaximaskNode(2);  break;     // Human
        case RACE_ORC:      SetTaximaskNode(23); break;     // Orc
        case RACE_DWARF:    SetTaximaskNode(6);  break;     // Dwarf
        case RACE_NIGHTELF: SetTaximaskNode(26);
                            SetTaximaskNode(27); break;     // Night Elf
        case RACE_UNDEAD:   SetTaximaskNode(11); break;     // Undead
        case RACE_TAUREN:   SetTaximaskNode(22); break;     // Tauren
        case RACE_GNOME:    SetTaximaskNode(6);  break;     // Gnome
        case RACE_TROLL:    SetTaximaskNode(23); break;     // Troll
        case RACE_BLOODELF: SetTaximaskNode(82); break;     // Blood Elf
        case RACE_DRAENEI:  SetTaximaskNode(94); break;     // Draenei
    }

    // new continent starting masks (It will be accessible only at new map)
    switch(Player::TeamForRace(race))
    {
        case ALLIANCE: SetTaximaskNode(100); break;
        case HORDE:    SetTaximaskNode(99);  break;
    }
    // level dependent taxi hubs
    if(level>=68)
        SetTaximaskNode(213);                               //Shattered Sun Staging Area
}

void PlayerTaxi::LoadTaxiMask(const char* data)
{
    Tokens tokens = StrSplit(data, " ");

    int index;
    Tokens::iterator iter;
    for (iter = tokens.begin(), index = 0;
        (index < TaxiMaskSize) && (iter != tokens.end()); ++iter, ++index)
    {
        // load and set bits only for existing taxi nodes
        m_taximask[index] = sTaxiNodesMask[index] & uint32(atol((*iter).c_str()));
    }
}

void PlayerTaxi::AppendTaximaskTo( ByteBuffer& data, bool all )
{
    if(all)
    {
        for (uint8 i=0; i<TaxiMaskSize; ++i)
            data << uint32(sTaxiNodesMask[i]);              // all existing nodes
    }
    else
    {
        for (uint8 i=0; i<TaxiMaskSize; ++i)
            data << uint32(m_taximask[i]);                  // known nodes
    }
}

bool PlayerTaxi::LoadTaxiDestinationsFromString(const std::string& values, Team team)
{
    ClearTaxiDestinations();

    Tokens tokens = StrSplit(values," ");

    for(Tokens::iterator iter = tokens.begin(); iter != tokens.end(); ++iter)
    {
        uint32 node = uint32(atol(iter->c_str()));
        AddTaxiDestination(node);
    }

    if(m_TaxiDestinations.empty())
        return true;

    // Check integrity
    if(m_TaxiDestinations.size() < 2)
        return false;

    for(size_t i = 1; i < m_TaxiDestinations.size(); ++i)
    {
        uint32 cost;
        uint32 path;
        sObjectMgr.GetTaxiPath(m_TaxiDestinations[i-1],m_TaxiDestinations[i], path, cost);
        if (!path)
            return false;
    }

    // can't load taxi path without mount set (quest taxi path?)
    if (!sObjectMgr.GetTaxiMountDisplayId(GetTaxiSource(), team, true))
        return false;

    return true;
}

std::string PlayerTaxi::SaveTaxiDestinationsToString()
{
    if(m_TaxiDestinations.empty())
        return "";

    std::ostringstream ss;

    for(size_t i=0; i < m_TaxiDestinations.size(); ++i)
        ss << m_TaxiDestinations[i] << " ";

    return ss.str();
}

uint32 PlayerTaxi::GetCurrentTaxiPath() const
{
    if(m_TaxiDestinations.size() < 2)
        return 0;

    uint32 path;
    uint32 cost;

    sObjectMgr.GetTaxiPath(m_TaxiDestinations[0],m_TaxiDestinations[1],path,cost);

    return path;
}

std::ostringstream& operator<< (std::ostringstream& ss, PlayerTaxi const& taxi)
{
    for(int i = 0; i < TaxiMaskSize; ++i)
        ss << taxi.m_taximask[i] << " ";
    return ss;
}
//== TradeData =================================================

TradeData* TradeData::GetTraderData() const
{
    return m_trader->GetTradeData();
}

Item* TradeData::GetItem( TradeSlots slot ) const
{
    return m_items[slot] ? m_player->GetItemByGuid(m_items[slot]) : NULL;
}

bool TradeData::HasItem( ObjectGuid item_guid ) const
{
    for(int i = 0; i < TRADE_SLOT_COUNT; ++i)
        if (m_items[i] == item_guid)
            return true;
    return false;
}


Item* TradeData::GetSpellCastItem() const
{
    return m_spellCastItem ?  m_player->GetItemByGuid(m_spellCastItem) : NULL;
}

void TradeData::SetItem( TradeSlots slot, Item* item )
{
    ObjectGuid itemGuid = item ? item->GetObjectGuid() : ObjectGuid();

    if (m_items[slot] == itemGuid)
        return;

    m_items[slot] = itemGuid;

    SetAccepted(false);
    GetTraderData()->SetAccepted(false);

    Update();

    // need remove possible trader spell applied to changed item
    if (slot == TRADE_SLOT_NONTRADED)
        GetTraderData()->SetSpell(0);

    // need remove possible player spell applied (possible move reagent)
    SetSpell(0);
}

void TradeData::SetSpell( uint32 spell_id, Item* castItem /*= NULL*/ )
{
    ObjectGuid itemGuid = castItem ? castItem->GetObjectGuid() : ObjectGuid();

    if (m_spell == spell_id && m_spellCastItem == itemGuid)
        return;

    m_spell = spell_id;
    m_spellCastItem = itemGuid;

    SetAccepted(false);
    GetTraderData()->SetAccepted(false);

    Update(true);                                           // send spell info to item owner
    Update(false);                                          // send spell info to caster self
}

void TradeData::SetMoney( uint32 money )
{
    if (m_money == money)
        return;

    m_money = money;

    SetAccepted(false);
    GetTraderData()->SetAccepted(false);

    Update();
}

void TradeData::Update( bool for_trader /*= true*/ )
{
    if (for_trader)
        m_trader->GetSession()->SendUpdateTrade(true);      // player state for trader
    else
        m_player->GetSession()->SendUpdateTrade(false);     // player state for player
}

void TradeData::SetAccepted(bool state, bool crosssend /*= false*/)
{
    m_accepted = state;

    if (!state)
    {
        if (crosssend)
            m_trader->GetSession()->SendTradeStatus(TRADE_STATUS_BACK_TO_TRADE);
        else
            m_player->GetSession()->SendTradeStatus(TRADE_STATUS_BACK_TO_TRADE);
    }
}

//== Player ====================================================

UpdateMask Player::updateVisualBits;

Player::Player (WorldSession *session): Unit(), m_mover(this), m_camera(this), m_achievementMgr(this), m_reputationMgr(this)
{
    m_transport = 0;

    m_speakTime = 0;
    m_speakCount = 0;

    m_objectType |= TYPEMASK_PLAYER;
    m_objectTypeId = TYPEID_PLAYER;

    m_valuesCount = PLAYER_END;

    SetActiveObjectState(true);                                // player is always active object

    m_session = session;

    m_ExtraFlags = 0;
    if(GetSession()->GetSecurity() >= SEC_GAMEMASTER)
        SetAcceptTicket(true);

    // players always accept
    if(GetSession()->GetSecurity() == SEC_PLAYER)
        SetAcceptWhispers(true);

    m_comboPoints = 0;

    m_usedTalentCount = 0;
    m_questRewardTalentCount = 0;

    m_regenTimer = 0;
    m_weaponChangeTimer = 0;

    m_zoneUpdateId = 0;
    m_zoneUpdateTimer = 0;

    m_areaUpdateId = 0;

    m_nextSave = sWorld.getConfig(CONFIG_UINT32_INTERVAL_SAVE);

    // randomize first save time in range [CONFIG_UINT32_INTERVAL_SAVE] around [CONFIG_UINT32_INTERVAL_SAVE]
    // this must help in case next save after mass player load after server startup
    m_nextSave = urand(m_nextSave/2,m_nextSave*3/2);

    clearResurrectRequestData();

    memset(m_items, 0, sizeof(Item*)*PLAYER_SLOTS_COUNT);

    m_social = NULL;

    // group is initialized in the reference constructor
    SetGroupInvite(NULL);
    m_groupUpdateMask = 0;
    m_auraUpdateMask = 0;

    duel = NULL;

    m_GuildIdInvited = 0;
    m_ArenaTeamIdInvited = 0;

    m_atLoginFlags = AT_LOGIN_NONE;

    mSemaphoreTeleport_Near = false;
    mSemaphoreTeleport_Far = false;

    m_DelayedOperations = 0;
    m_bCanDelayTeleport = false;
    m_bHasDelayedTeleport = false;
    m_bHasBeenAliveAtDelayedTeleport = true;                // overwrite always at setup teleport data, so not used infact
    m_teleport_options = 0;

    m_trade = NULL;

    m_cinematic = 0;

    PlayerTalkClass = new PlayerMenu( GetSession() );
    m_currentBuybackSlot = BUYBACK_SLOT_START;

    m_DailyQuestChanged = false;
    m_WeeklyQuestChanged = false;

    for (int i=0; i<MAX_TIMERS; ++i)
        m_MirrorTimer[i] = DISABLED_MIRROR_TIMER;

    m_MirrorTimerFlags = UNDERWATER_NONE;
    m_MirrorTimerFlagsLast = UNDERWATER_NONE;
    m_isInWater = false;
    m_drunkTimer = 0;
    m_drunk = 0;
    m_restTime = 0;
    m_deathTimer = 0;
    m_deathExpireTime = 0;

    m_swingErrorMsg = 0;

    m_DetectInvTimer = 1*IN_MILLISECONDS;

    for (int j=0; j < PLAYER_MAX_BATTLEGROUND_QUEUES; ++j)
    {
        m_bgBattleGroundQueueID[j].bgQueueTypeId  = BATTLEGROUND_QUEUE_NONE;
        m_bgBattleGroundQueueID[j].invitedToInstance = 0;
    }

    m_logintime = time(NULL);
    m_Last_tick = m_logintime;
    m_WeaponProficiency = 0;
    m_ArmorProficiency = 0;
    m_canParry = false;
    m_canBlock = false;
    m_canDualWield = false;
    m_canTitanGrip = false;
    m_ammoDPS = 0.0f;

    m_temporaryUnsummonedPetNumber = 0;

    ////////////////////Rest System/////////////////////
    time_inn_enter=0;
    inn_trigger_id=0;
    m_rest_bonus=0;
    rest_type=REST_TYPE_NO;
    ////////////////////Rest System/////////////////////

    m_mailsUpdated = false;
    unReadMails = 0;
    m_nextMailDelivereTime = 0;

    m_resetTalentsCost = 0;
    m_resetTalentsTime = 0;
    m_itemUpdateQueueBlocked = false;

    for (int i = 0; i < MAX_MOVE_TYPE; ++i)
        m_forced_speed_changes[i] = 0;

    m_stableSlots = 0;

    /////////////////// Instance System /////////////////////

    m_HomebindTimer = 0;
    m_InstanceValid = true;
    m_dungeonDifficulty = DUNGEON_DIFFICULTY_NORMAL;
    m_raidDifficulty = RAID_DIFFICULTY_10MAN_NORMAL;

    m_lastPotionId = 0;

    m_activeSpec = 0;
    m_specsCount = 1;

    for (int i = 0; i < BASEMOD_END; ++i)
    {
        m_auraBaseMod[i][FLAT_MOD] = 0.0f;
        m_auraBaseMod[i][PCT_MOD] = 1.0f;
    }

    for (int i = 0; i < MAX_COMBAT_RATING; ++i)
        m_baseRatingValue[i] = 0;

    m_baseSpellPower = 0;
    m_baseFeralAP = 0;
    m_baseManaRegen = 0;
    m_armorPenetrationPct = 0.0f;
    m_spellPenetrationItemMod = 0;

    // Honor System
    m_lastHonorUpdateTime = time(NULL);

    // Player summoning
    m_summon_expire = 0;
    m_summon_mapid = 0;
    m_summon_x = 0.0f;
    m_summon_y = 0.0f;
    m_summon_z = 0.0f;

    m_contestedPvPTimer = 0;

    m_declinedname = NULL;
    m_runes = NULL;

    m_lastFallTime = 0;
    m_lastFallZ = 0;

    m_cachedGS = 0;
}

Player::~Player ()
{
    CleanupsBeforeDelete();

    // it must be unloaded already in PlayerLogout and accessed only for loggined player
    //m_social = NULL;

    // Note: buy back item already deleted from DB when player was saved
    for(int i = 0; i < PLAYER_SLOTS_COUNT; ++i)
    {
        if(m_items[i])
            delete m_items[i];
    }
    CleanupChannels();

    //all mailed items should be deleted, also all mail should be deallocated
    for (PlayerMails::const_iterator itr =  m_mail.begin(); itr != m_mail.end();++itr)
        delete *itr;

    for (ItemMap::const_iterator iter = mMitems.begin(); iter != mMitems.end(); ++iter)
        delete iter->second;                                //if item is duplicated... then server may crash ... but that item should be deallocated

    delete PlayerTalkClass;

    if (m_transport)
    {
        m_transport->RemovePassenger(this);
    }

    for(size_t x = 0; x < ItemSetEff.size(); x++)
        if(ItemSetEff[x])
            delete ItemSetEff[x];

    // clean up player-instance binds, may unload some instance saves
    for(uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        for(BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
            itr->second.state->RemovePlayer(this);

    delete m_declinedname;
    delete m_runes;
}

void Player::CleanupsBeforeDelete()
{
    if(m_uint32Values)                                      // only for fully created Object
    {
        TradeCancel(false);
        DuelComplete(DUEL_INTERUPTED);
    }
    Unit::CleanupsBeforeDelete();
}

bool Player::Create( uint32 guidlow, const std::string& name, uint8 race, uint8 class_, uint8 gender, uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair, uint8 /*outfitId */)
{
    //FIXME: outfitId not used in player creating

    Object::_Create(guidlow, 0, HIGHGUID_PLAYER);

    m_name = name;

    PlayerInfo const* info = sObjectMgr.GetPlayerInfo(race, class_);
    if(!info)
    {
        sLog.outError("Player have incorrect race/class pair. Can't be loaded.");
        return false;
    }

    ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(class_);
    if(!cEntry)
    {
        sLog.outError("Class %u not found in DBC (Wrong DBC files?)",class_);
        return false;
    }

    // player store gender in single bit
    if (gender != uint8(GENDER_MALE) && gender != uint8(GENDER_FEMALE))
    {
        sLog.outError("Invalid gender %u at player creating", uint32(gender));
        return false;
    }

    for (int i = 0; i < PLAYER_SLOTS_COUNT; ++i)
        m_items[i] = NULL;

    SetLocationMapId(info->mapId);
    Relocate(info->positionX,info->positionY,info->positionZ, info->orientation);

    SetMap(sMapMgr.CreateMap(info->mapId, this));

    uint8 powertype = cEntry->powerType;

    setFactionForRace(race);

    SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
    SetByteValue(UNIT_FIELD_BYTES_0, 1, class_);
    SetByteValue(UNIT_FIELD_BYTES_0, 2, gender);
    SetByteValue(UNIT_FIELD_BYTES_0, 3, powertype);

    InitDisplayIds();                                       // model, scale and model data

    SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP );
    SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE );
    SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);               // fix cast time showed in spell tooltip on client
    SetFloatValue(UNIT_FIELD_HOVERHEIGHT, 1.0f);            // default for players in 3.0.3

    SetInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, -1);  // -1 is default value

    SetByteValue(PLAYER_BYTES, 0, skin);
    SetByteValue(PLAYER_BYTES, 1, face);
    SetByteValue(PLAYER_BYTES, 2, hairStyle);
    SetByteValue(PLAYER_BYTES, 3, hairColor);

    SetByteValue(PLAYER_BYTES_2, 0, facialHair);
    SetByteValue(PLAYER_BYTES_2, 3, 0x02);                  // rest state = normal

    SetUInt16Value(PLAYER_BYTES_3, 0, gender);              // only GENDER_MALE/GENDER_FEMALE (1 bit) allowed, drunk state = 0
    SetByteValue(PLAYER_BYTES_3, 3, 0);                     // BattlefieldArenaFaction (0 or 1)

    SetUInt32Value( PLAYER_GUILDID, 0 );
    SetUInt32Value( PLAYER_GUILDRANK, 0 );
    SetUInt32Value( PLAYER_GUILD_TIMESTAMP, 0 );

    for(int i = 0; i < KNOWN_TITLES_SIZE; ++i)
        SetUInt64Value(PLAYER__FIELD_KNOWN_TITLES + i, 0);  // 0=disabled
    SetUInt32Value( PLAYER_CHOSEN_TITLE, 0 );

    SetUInt32Value( PLAYER_FIELD_KILLS, 0 );
    SetUInt32Value( PLAYER_FIELD_LIFETIME_HONORBALE_KILLS, 0 );
    SetUInt32Value( PLAYER_FIELD_TODAY_CONTRIBUTION, 0 );
    SetUInt32Value( PLAYER_FIELD_YESTERDAY_CONTRIBUTION, 0 );

    // set starting level
    uint32 start_level = getClass() != CLASS_DEATH_KNIGHT
        ? sWorld.getConfig(CONFIG_UINT32_START_PLAYER_LEVEL)
        : sWorld.getConfig(CONFIG_UINT32_START_HEROIC_PLAYER_LEVEL);

    if (GetSession()->GetSecurity() >= SEC_MODERATOR)
    {
        uint32 gm_level = sWorld.getConfig(CONFIG_UINT32_START_GM_LEVEL);
        if(gm_level > start_level)
            start_level = gm_level;
    }

    SetUInt32Value(UNIT_FIELD_LEVEL, start_level);

    InitRunes();

    SetUInt32Value (PLAYER_FIELD_COINAGE, sWorld.getConfig(CONFIG_UINT32_START_PLAYER_MONEY));
    SetHonorPoints(sWorld.getConfig(CONFIG_UINT32_START_HONOR_POINTS));
    SetArenaPoints(sWorld.getConfig(CONFIG_UINT32_START_ARENA_POINTS));

    // Played time
    m_Last_tick = time(NULL);
    m_Played_time[PLAYED_TIME_TOTAL] = 0;
    m_Played_time[PLAYED_TIME_LEVEL] = 0;

    // base stats and related field values
    InitStatsForLevel();
    InitTaxiNodesForLevel();
    InitGlyphsForLevel();
    InitTalentForLevel();
    InitPrimaryProfessions();                               // to max set before any spell added

    // apply original stats mods before spell loading or item equipment that call before equip _RemoveStatsMods()
    UpdateMaxHealth();                                      // Update max Health (for add bonus from stamina)
    SetHealth(GetMaxHealth());

    if (getPowerType() == POWER_MANA)
    {
        UpdateMaxPower(POWER_MANA);                         // Update max Mana (for add bonus from intellect)
        SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    }

    if(getPowerType() != POWER_MANA)                        // hide additional mana bar if we have no mana
    {
        SetPower(POWER_MANA, 0);
        SetMaxPower(POWER_MANA, 0);
    }

    // original spells
    learnDefaultSpells();

    // original action bar
    for (PlayerCreateInfoActions::const_iterator action_itr = info->action.begin(); action_itr != info->action.end(); ++action_itr)
        addActionButton(0, action_itr->button,action_itr->action,action_itr->type);

    // original items
    uint32 raceClassGender = GetUInt32Value(UNIT_FIELD_BYTES_0) & 0x00FFFFFF;

    CharStartOutfitEntry const* oEntry = NULL;
    for (uint32 i = 1; i < sCharStartOutfitStore.GetNumRows(); ++i)
    {
        if(CharStartOutfitEntry const* entry = sCharStartOutfitStore.LookupEntry(i))
        {
            if(entry->RaceClassGender == raceClassGender)
            {
                oEntry = entry;
                break;
            }
        }
    }

    if(oEntry)
    {
        for(int j = 0; j < MAX_OUTFIT_ITEMS; ++j)
        {
            if(oEntry->ItemId[j] <= 0)
                continue;

            uint32 item_id = oEntry->ItemId[j];

            // just skip, reported in ObjectMgr::LoadItemPrototypes
            ItemPrototype const* iProto = ObjectMgr::GetItemPrototype(item_id);
            if(!iProto)
                continue;

            // BuyCount by default
            int32 count = iProto->BuyCount;

            // special amount for foor/drink
            if(iProto->Class==ITEM_CLASS_CONSUMABLE && iProto->SubClass==ITEM_SUBCLASS_FOOD)
            {
                switch(iProto->Spells[0].SpellCategory)
                {
                    case 11:                                // food
                        count = getClass()==CLASS_DEATH_KNIGHT ? 10 : 4;
                        break;
                    case 59:                                // drink
                        count = 2;
                        break;
                }
                if(iProto->Stackable < count)
                    count = iProto->Stackable;
            }

            StoreNewItemInBestSlots(item_id, count);
        }
    }

    for (PlayerCreateInfoItems::const_iterator item_id_itr = info->item.begin(); item_id_itr != info->item.end(); ++item_id_itr)
        StoreNewItemInBestSlots(item_id_itr->item_id, item_id_itr->item_amount);

    // bags and main-hand weapon must equipped at this moment
    // now second pass for not equipped (offhand weapon/shield if it attempt equipped before main-hand weapon)
    // or ammo not equipped in special bag
    for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            uint16 eDest;
            // equip offhand weapon/shield if it attempt equipped before main-hand weapon
            InventoryResult msg = CanEquipItem(NULL_SLOT, eDest, pItem, false);
            if (msg == EQUIP_ERR_OK)
            {
                RemoveItem(INVENTORY_SLOT_BAG_0, i, true);
                EquipItem(eDest, pItem, true);
            }
            // move other items to more appropriate slots (ammo not equipped in special bag)
            else
            {
                ItemPosCountVec sDest;
                msg = CanStoreItem(NULL_BAG, NULL_SLOT, sDest, pItem, false);
                if (msg == EQUIP_ERR_OK)
                {
                    RemoveItem(INVENTORY_SLOT_BAG_0, i,true);
                    pItem = StoreItem( sDest, pItem, true);
                }

                // if  this is ammo then use it
                msg = CanUseAmmo(pItem->GetEntry());
                if (msg == EQUIP_ERR_OK)
                    SetAmmo(pItem->GetEntry());
            }
        }
    }
    // all item positions resolved

    return true;
}

bool Player::StoreNewItemInBestSlots(uint32 titem_id, uint32 titem_amount)
{
    DEBUG_LOG("STORAGE: Creating initial item, itemId = %u, count = %u",titem_id, titem_amount);

    // attempt equip by one
    while(titem_amount > 0)
    {
        uint16 eDest;
        uint8 msg = CanEquipNewItem( NULL_SLOT, eDest, titem_id, false );
        if( msg != EQUIP_ERR_OK )
            break;

        EquipNewItem( eDest, titem_id, true);
        AutoUnequipOffhandIfNeed();
        --titem_amount;
    }

    if(titem_amount == 0)
        return true;                                        // equipped

    // attempt store
    ItemPosCountVec sDest;
    // store in main bag to simplify second pass (special bags can be not equipped yet at this moment)
    uint8 msg = CanStoreNewItem( INVENTORY_SLOT_BAG_0, NULL_SLOT, sDest, titem_id, titem_amount );
    if( msg == EQUIP_ERR_OK )
    {
        StoreNewItem( sDest, titem_id, true, Item::GenerateItemRandomPropertyId(titem_id) );
        return true;                                        // stored
    }

    // item can't be added
    sLog.outError("STORAGE: Can't equip or store initial item %u for race %u class %u , error msg = %u",titem_id,getRace(),getClass(),msg);
    return false;
}

// helper function, mainly for script side, but can be used for simple task in mangos also.
Item* Player::StoreNewItemInInventorySlot(uint32 itemEntry, uint32 amount)
{
    ItemPosCountVec vDest;

    uint8 msg = CanStoreNewItem(INVENTORY_SLOT_BAG_0, NULL_SLOT, vDest, itemEntry, amount);

    if (msg == EQUIP_ERR_OK)
    {
        if (Item* pItem = StoreNewItem(vDest, itemEntry, true, Item::GenerateItemRandomPropertyId(itemEntry)))
            return pItem;
    }

    return NULL;
}

void Player::SendMirrorTimer(MirrorTimerType Type, uint32 MaxValue, uint32 CurrentValue, int32 Regen)
{
    if (int(MaxValue) == DISABLED_MIRROR_TIMER)
    {
        if (int(CurrentValue) != DISABLED_MIRROR_TIMER)
            StopMirrorTimer(Type);
        return;
    }
    WorldPacket data(SMSG_START_MIRROR_TIMER, (21));
    data << (uint32)Type;
    data << CurrentValue;
    data << MaxValue;
    data << Regen;
    data << (uint8)0;
    data << (uint32)0;                                      // spell id
    GetSession()->SendPacket( &data );
}

void Player::StopMirrorTimer(MirrorTimerType Type)
{
    m_MirrorTimer[Type] = DISABLED_MIRROR_TIMER;
    WorldPacket data(SMSG_STOP_MIRROR_TIMER, 4);
    data << (uint32)Type;
    GetSession()->SendPacket( &data );
}

uint32 Player::EnvironmentalDamage(EnviromentalDamage type, uint32 damage)
{
    if(!isAlive() || isGameMaster())
        return 0;

    // Absorb, resist some environmental damage type
    uint32 absorb = 0;
    uint32 resist = 0;
    if (type == DAMAGE_LAVA)
        CalculateDamageAbsorbAndResist(this, SPELL_SCHOOL_MASK_FIRE, DIRECT_DAMAGE, damage, &absorb, &resist);
    else if (type == DAMAGE_SLIME)
        CalculateDamageAbsorbAndResist(this, SPELL_SCHOOL_MASK_NATURE, DIRECT_DAMAGE, damage, &absorb, &resist);

    damage-=absorb+resist;

    DealDamageMods(this,damage,&absorb);

    WorldPacket data(SMSG_ENVIRONMENTALDAMAGELOG, (21));
    data << GetObjectGuid();
    data << uint8(type!=DAMAGE_FALL_TO_VOID ? type : DAMAGE_FALL);
    data << uint32(damage);
    data << uint32(absorb);
    data << uint32(resist);
    SendMessageToSet(&data, true);

    uint32 final_damage = DealDamage(this, damage, NULL, SELF_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);

    if(!isAlive())
    {
        if(type==DAMAGE_FALL)                               // DealDamage not apply item durability loss at self damage
        {
            DEBUG_LOG("We are fall to death, loosing 10 percents durability");
            DurabilityLossAll(0.10f,false);
            // durability lost message
            WorldPacket data2(SMSG_DURABILITY_DAMAGE_DEATH, 0);
            GetSession()->SendPacket(&data2);
        }

        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM, 1, type);
    }

    return final_damage;
}

int32 Player::getMaxTimer(MirrorTimerType timer)
{
    switch (timer)
    {
        case FATIGUE_TIMER:
            if (GetSession()->GetSecurity() >= (AccountTypes)sWorld.getConfig(CONFIG_UINT32_TIMERBAR_FATIGUE_GMLEVEL))
                return DISABLED_MIRROR_TIMER;
            return sWorld.getConfig(CONFIG_UINT32_TIMERBAR_FATIGUE_MAX)*IN_MILLISECONDS;
        case BREATH_TIMER:
        {
            if (!isAlive() || HasAuraType(SPELL_AURA_WATER_BREATHING) ||
                GetSession()->GetSecurity() >= (AccountTypes)sWorld.getConfig(CONFIG_UINT32_TIMERBAR_BREATH_GMLEVEL))
                return DISABLED_MIRROR_TIMER;
            int32 UnderWaterTime = sWorld.getConfig(CONFIG_UINT32_TIMERBAR_BREATH_MAX)*IN_MILLISECONDS;
            AuraList const& mModWaterBreathing = GetAurasByType(SPELL_AURA_MOD_WATER_BREATHING);
            for(AuraList::const_iterator i = mModWaterBreathing.begin(); i != mModWaterBreathing.end(); ++i)
                UnderWaterTime = uint32(UnderWaterTime * (100.0f + (*i)->GetModifier()->m_amount) / 100.0f);
            return UnderWaterTime;
        }
        case FIRE_TIMER:
        {
            if (!isAlive() || GetSession()->GetSecurity() >= (AccountTypes)sWorld.getConfig(CONFIG_UINT32_TIMERBAR_FIRE_GMLEVEL))
                return DISABLED_MIRROR_TIMER;
            return sWorld.getConfig(CONFIG_UINT32_TIMERBAR_FIRE_MAX)*IN_MILLISECONDS;
        }
        default:
            return 0;
    }
    return 0;
}

void Player::UpdateMirrorTimers()
{
    // Desync flags for update on next HandleDrowning
    if (m_MirrorTimerFlags)
        m_MirrorTimerFlagsLast = ~m_MirrorTimerFlags;
}

void Player::HandleDrowning(uint32 time_diff)
{
    if (!m_MirrorTimerFlags)
        return;

    // In water
    if (m_MirrorTimerFlags & UNDERWATER_INWATER)
    {
        // Breath timer not activated - activate it
        if (m_MirrorTimer[BREATH_TIMER] == DISABLED_MIRROR_TIMER)
        {
            m_MirrorTimer[BREATH_TIMER] = getMaxTimer(BREATH_TIMER);
            SendMirrorTimer(BREATH_TIMER, m_MirrorTimer[BREATH_TIMER], m_MirrorTimer[BREATH_TIMER], -1);
        }
        else                                                              // If activated - do tick
        {
            m_MirrorTimer[BREATH_TIMER]-=time_diff;
            // Timer limit - need deal damage
            if (m_MirrorTimer[BREATH_TIMER] < 0)
            {
                m_MirrorTimer[BREATH_TIMER] += 2 * IN_MILLISECONDS;
                // Calculate and deal damage
                // TODO: Check this formula
                uint32 damage = GetMaxHealth() / 5 + urand(0, getLevel()-1);
                EnvironmentalDamage(DAMAGE_DROWNING, damage);
            }
            else if (!(m_MirrorTimerFlagsLast & UNDERWATER_INWATER))      // Update time in client if need
                SendMirrorTimer(BREATH_TIMER, getMaxTimer(BREATH_TIMER), m_MirrorTimer[BREATH_TIMER], -1);
        }
    }
    else if (m_MirrorTimer[BREATH_TIMER] != DISABLED_MIRROR_TIMER)        // Regen timer
    {
        int32 UnderWaterTime = getMaxTimer(BREATH_TIMER);
        // Need breath regen
        m_MirrorTimer[BREATH_TIMER]+=10*time_diff;
        if (m_MirrorTimer[BREATH_TIMER] >= UnderWaterTime || !isAlive())
            StopMirrorTimer(BREATH_TIMER);
        else if (m_MirrorTimerFlagsLast & UNDERWATER_INWATER)
            SendMirrorTimer(BREATH_TIMER, UnderWaterTime, m_MirrorTimer[BREATH_TIMER], 10);
    }

    // In dark water
    if (m_MirrorTimerFlags & UNDERWATER_INDARKWATER)
    {
        // Fatigue timer not activated - activate it
        if (m_MirrorTimer[FATIGUE_TIMER] == DISABLED_MIRROR_TIMER)
        {
            m_MirrorTimer[FATIGUE_TIMER] = getMaxTimer(FATIGUE_TIMER);
            SendMirrorTimer(FATIGUE_TIMER, m_MirrorTimer[FATIGUE_TIMER], m_MirrorTimer[FATIGUE_TIMER], -1);
        }
        else
        {
            m_MirrorTimer[FATIGUE_TIMER]-=time_diff;
            // Timer limit - need deal damage or teleport ghost to graveyard
            if (m_MirrorTimer[FATIGUE_TIMER] < 0)
            {
                m_MirrorTimer[FATIGUE_TIMER] += 2 * IN_MILLISECONDS;
                if (isAlive())                                            // Calculate and deal damage
                {
                    uint32 damage = GetMaxHealth() / 5 + urand(0, getLevel()-1);
                    EnvironmentalDamage(DAMAGE_EXHAUSTED, damage);
                }
                else if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))       // Teleport ghost to graveyard
                    RepopAtGraveyard();
            }
            else if (!(m_MirrorTimerFlagsLast & UNDERWATER_INDARKWATER))
                SendMirrorTimer(FATIGUE_TIMER, getMaxTimer(FATIGUE_TIMER), m_MirrorTimer[FATIGUE_TIMER], -1);
        }
    }
    else if (m_MirrorTimer[FATIGUE_TIMER] != DISABLED_MIRROR_TIMER)       // Regen timer
    {
        int32 DarkWaterTime = getMaxTimer(FATIGUE_TIMER);
        m_MirrorTimer[FATIGUE_TIMER]+=10*time_diff;
        if (m_MirrorTimer[FATIGUE_TIMER] >= DarkWaterTime || !isAlive())
            StopMirrorTimer(FATIGUE_TIMER);
        else if (m_MirrorTimerFlagsLast & UNDERWATER_INDARKWATER)
            SendMirrorTimer(FATIGUE_TIMER, DarkWaterTime, m_MirrorTimer[FATIGUE_TIMER], 10);
    }

    if (m_MirrorTimerFlags & (UNDERWATER_INLAVA|UNDERWATER_INSLIME))
    {
        // Breath timer not activated - activate it
        if (m_MirrorTimer[FIRE_TIMER] == DISABLED_MIRROR_TIMER)
            m_MirrorTimer[FIRE_TIMER] = getMaxTimer(FIRE_TIMER);
        else
        {
            m_MirrorTimer[FIRE_TIMER]-=time_diff;
            if (m_MirrorTimer[FIRE_TIMER] < 0)
            {
                m_MirrorTimer[FIRE_TIMER] += 2 * IN_MILLISECONDS;
                // Calculate and deal damage
                // TODO: Check this formula
                uint32 damage = urand(600, 700);
                if (m_MirrorTimerFlags&UNDERWATER_INLAVA)
                    EnvironmentalDamage(DAMAGE_LAVA, damage);
                // need to skip Slime damage in Undercity,
                // maybe someone can find better way to handle environmental damage
                else if (m_zoneUpdateId != 1497)
                    EnvironmentalDamage(DAMAGE_SLIME, damage);
            }
        }
    }
    else
        m_MirrorTimer[FIRE_TIMER] = DISABLED_MIRROR_TIMER;

    // Recheck timers flag
    m_MirrorTimerFlags&=~UNDERWATER_EXIST_TIMERS;
    for (int i = 0; i< MAX_TIMERS; ++i)
        if (m_MirrorTimer[i]!=DISABLED_MIRROR_TIMER)
        {
            m_MirrorTimerFlags|=UNDERWATER_EXIST_TIMERS;
            break;
        }
    m_MirrorTimerFlagsLast = m_MirrorTimerFlags;
}

///The player sobers by 256 every 10 seconds
void Player::HandleSobering()
{
    m_drunkTimer = 0;

    uint32 drunk = (m_drunk <= 256) ? 0 : (m_drunk - 256);
    SetDrunkValue(drunk);
}

DrunkenState Player::GetDrunkenstateByValue(uint16 value)
{
    if(value >= 23000)
        return DRUNKEN_SMASHED;
    if(value >= 12800)
        return DRUNKEN_DRUNK;
    if(value & 0xFFFE)
        return DRUNKEN_TIPSY;
    return DRUNKEN_SOBER;
}

void Player::SetDrunkValue(uint16 newDrunkenValue, uint32 itemId)
{
    uint32 oldDrunkenState = Player::GetDrunkenstateByValue(m_drunk);

    m_drunk = newDrunkenValue;
    SetUInt16Value(PLAYER_BYTES_3, 0, uint16(getGender()) | (m_drunk & 0xFFFE));

    uint32 newDrunkenState = Player::GetDrunkenstateByValue(m_drunk);

    // special drunk invisibility detection
    if(newDrunkenState >= DRUNKEN_DRUNK)
        m_detectInvisibilityMask |= (1<<6);
    else
        m_detectInvisibilityMask &= ~(1<<6);

    if(newDrunkenState == oldDrunkenState)
        return;

    WorldPacket data(SMSG_CROSSED_INEBRIATION_THRESHOLD, (8+4+4));
    data << GetObjectGuid();
    data << uint32(newDrunkenState);
    data << uint32(itemId);

    SendMessageToSet(&data, true);
}

void Player::Update( uint32 update_diff, uint32 p_time )
{
    if(!IsInWorld())
        return;

    // remove failed timed Achievements
    GetAchievementMgr().DoFailedTimedAchievementCriterias();

    // undelivered mail
    if(m_nextMailDelivereTime && m_nextMailDelivereTime <= time(NULL))
    {
        SendNewMail();
        ++unReadMails;

        // It will be recalculate at mailbox open (for unReadMails important non-0 until mailbox open, it also will be recalculated)
        m_nextMailDelivereTime = 0;
    }

    //used to implement delayed far teleports
    SetCanDelayTeleport(true);
    Unit::Update( update_diff, p_time );
    SetCanDelayTeleport(false);

    // update player only attacks
    if(uint32 ranged_att = getAttackTimer(RANGED_ATTACK))
    {
        setAttackTimer(RANGED_ATTACK, (update_diff >= ranged_att ? 0 : ranged_att - update_diff) );
    }

    if(uint32 off_att = getAttackTimer(OFF_ATTACK))
    {
        setAttackTimer(OFF_ATTACK, (update_diff >= off_att ? 0 : off_att - update_diff) );
    }

    time_t now = time (NULL);

    UpdatePvPFlag(now);

    UpdateContestedPvP(update_diff);

    UpdateDuelFlag(now);

    CheckDuelDistance(now);

    UpdateAfkReport(now);

    // Update items that have just a limited lifetime
    if (now>m_Last_tick)
        UpdateItemDuration(uint32(now- m_Last_tick));

    if (!m_timedquests.empty())
    {
        QuestSet::iterator iter = m_timedquests.begin();
        while (iter != m_timedquests.end())
        {
            QuestStatusData& q_status = mQuestStatus[*iter];
            if( q_status.m_timer <= update_diff )
            {
                uint32 quest_id  = *iter;
                ++iter;                                     // current iter will be removed in FailQuest
                FailQuest(quest_id);
            }
            else
            {
                q_status.m_timer -= update_diff;
                if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;
                ++iter;
            }
        }
    }

    if (hasUnitState(UNIT_STAT_MELEE_ATTACKING))
    {
        UpdateMeleeAttackingState();

        Unit *pVictim = getVictim();
        if (pVictim && !IsNonMeleeSpellCasted(false))
        {
            Player *vOwner = pVictim->GetCharmerOrOwnerPlayerOrPlayerItself();
            if (vOwner && vOwner->IsPvP() && !IsInDuelWith(vOwner))
            {
                UpdatePvP(true);
                RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
            }
        }
    }

    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING))
    {
        if (roll_chance_i(3) && GetTimeInnEnter() > 0)      //freeze update
        {
            time_t time_inn = time(NULL)-GetTimeInnEnter();
            if (time_inn >= 10)                             //freeze update
            {
                float bubble = 0.125f*sWorld.getConfig(CONFIG_FLOAT_RATE_REST_INGAME);
                //speed collect rest bonus (section/in hour)
                SetRestBonus( float(GetRestBonus()+ time_inn*(GetUInt32Value(PLAYER_NEXT_LEVEL_XP)/72000)*bubble ));
                UpdateInnerTime(time(NULL));
            }
        }
    }

    if (m_regenTimer)
    {
        if(update_diff >= m_regenTimer)
            m_regenTimer = 0;
        else
            m_regenTimer -= update_diff;
    }

    if (m_weaponChangeTimer > 0)
    {
        if(update_diff >= m_weaponChangeTimer)
            m_weaponChangeTimer = 0;
        else
            m_weaponChangeTimer -= update_diff;
    }

    if (m_zoneUpdateTimer > 0)
    {
        if(update_diff >= m_zoneUpdateTimer)
        {
            uint32 newzone, newarea;
            GetZoneAndAreaId(newzone,newarea);

            if( m_zoneUpdateId != newzone )
                UpdateZone(newzone,newarea);                // also update area
            else
            {
                // use area updates as well
                // needed for free far all arenas for example
                if( m_areaUpdateId != newarea )
                    UpdateArea(newarea);

                m_zoneUpdateTimer = ZONE_UPDATE_INTERVAL;
            }
        }
        else
            m_zoneUpdateTimer -= update_diff;
    }

    if (m_timeSyncTimer > 0)
    {
        if(update_diff >= m_timeSyncTimer)
            SendTimeSync();
        else
            m_timeSyncTimer -= update_diff;
    }

    if (isAlive())
    {
        // if no longer casting, set regen power as soon as it is up.
        if (!IsUnderLastManaUseEffect() && !HasAuraType(SPELL_AURA_STOP_NATURAL_MANA_REGEN))
            SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);

        if (!m_regenTimer)
            RegenerateAll();
    }

    if (m_deathState == JUST_DIED)
        KillPlayer();

    if(m_nextSave > 0)
    {
        if(update_diff >= m_nextSave)
        {
            // m_nextSave reseted in SaveToDB call
            SaveToDB();
            DETAIL_LOG("Player '%s' (GUID: %u) saved", GetName(), GetGUIDLow());
        }
        else
            m_nextSave -= update_diff;
    }

    //Handle Water/drowning
    HandleDrowning(update_diff);

    //Handle detect stealth players
    if (m_DetectInvTimer > 0)
    {
        if (update_diff >= m_DetectInvTimer)
        {
            HandleStealthedUnitsDetection();
            m_DetectInvTimer = 3000;
        }
        else
            m_DetectInvTimer -= update_diff;
    }

    // Played time
    if (now > m_Last_tick)
    {
        uint32 elapsed = uint32(now - m_Last_tick);
        m_Played_time[PLAYED_TIME_TOTAL] += elapsed;        // Total played time
        m_Played_time[PLAYED_TIME_LEVEL] += elapsed;        // Level played time
        m_Last_tick = now;
    }

    if (m_drunk)
    {
        m_drunkTimer += update_diff;

        if (m_drunkTimer > 10*IN_MILLISECONDS)
            HandleSobering();
    }

    // not auto-free ghost from body in instances
    if(m_deathTimer > 0  && !GetMap()->Instanceable())
    {
        if(p_time >= m_deathTimer)
        {
            m_deathTimer = 0;
            BuildPlayerRepop();
            RepopAtGraveyard();
        }
        else
            m_deathTimer -= p_time;
    }

    UpdateEnchantTime(update_diff);
    UpdateHomebindTime(update_diff);

    // group update
    SendUpdateToOutOfRangeGroupMembers();

    Pet* pet = GetPet();
    if (pet && !pet->IsWithinDistInMap(this, GetMap()->GetVisibilityDistance()) && (GetCharmGuid() && (pet->GetObjectGuid() != GetCharmGuid())))
        pet->Unsummon(PET_SAVE_REAGENTS, this);

    if (IsHasDelayedTeleport())
        TeleportTo(m_teleport_dest, m_teleport_options);
}

void Player::SetDeathState(DeathState s)
{
    uint32 ressSpellId = 0;

    bool cur = isAlive();

    if(s == JUST_DIED && cur)
    {
        // drunken state is cleared on death
        SetDrunkValue(0);
        // lost combo points at any target (targeted combo points clear in Unit::SetDeathState)
        ClearComboPoints();

        clearResurrectRequestData();

        // remove form before other mods to prevent incorrect stats calculation
        RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT);

        //FIXME: is pet dismissed at dying or releasing spirit? if second, add SetDeathState(DEAD) to HandleRepopRequestOpcode and define pet unsummon here with (s == DEAD)
        RemovePet(PET_SAVE_REAGENTS);

        // save value before aura remove in Unit::SetDeathState
        ressSpellId = GetUInt32Value(PLAYER_SELF_RES_SPELL);

        // passive spell
        if(!ressSpellId)
            ressSpellId = GetResurrectionSpellId();

        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP, 1);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DEATH, 1);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON, 1);

        if (InstanceData* mapInstance = GetInstanceData())
            mapInstance->OnPlayerDeath(this);
    }

    Unit::SetDeathState(s);

    // restore resurrection spell id for player after aura remove
    if(s == JUST_DIED && cur && ressSpellId)
        SetUInt32Value(PLAYER_SELF_RES_SPELL, ressSpellId);

    if(isAlive() && !cur)
    {
        //clear aura case after resurrection by another way (spells will be applied before next death)
        SetUInt32Value(PLAYER_SELF_RES_SPELL, 0);

        // restore default warrior stance
        if(getClass()== CLASS_WARRIOR)
            CastSpell(this,SPELL_ID_PASSIVE_BATTLE_STANCE,true);
    }
}

bool Player::BuildEnumData( QueryResult * result, WorldPacket * p_data )
{
    //             0               1                2                3                 4                  5                       6                        7
    //    "SELECT characters.guid, characters.name, characters.race, characters.class, characters.gender, characters.playerBytes, characters.playerBytes2, characters.level, "
    //     8                9               10                     11                     12                     13                    14
    //    "characters.zone, characters.map, characters.position_x, characters.position_y, characters.position_z, guild_member.guildid, characters.playerFlags, "
    //    15                    16                   17                     18                   19                         20
    //    "characters.at_login, character_pet.entry, character_pet.modelid, character_pet.level, characters.equipmentCache, character_declinedname.genitive "

    Field *fields = result->Fetch();

    uint32 guid = fields[0].GetUInt32();
    uint8 pRace = fields[2].GetUInt8();
    uint8 pClass = fields[3].GetUInt8();

    PlayerInfo const *info = sObjectMgr.GetPlayerInfo(pRace, pClass);
    if(!info)
    {
        sLog.outError("Player %u has incorrect race/class pair. Don't build enum.", guid);
        return false;
    }

    *p_data << ObjectGuid(HIGHGUID_PLAYER, guid);
    *p_data << fields[1].GetString();                       // name
    *p_data << uint8(pRace);                                // race
    *p_data << uint8(pClass);                               // class
    *p_data << uint8(fields[4].GetUInt8());                 // gender

    uint32 playerBytes = fields[5].GetUInt32();
    *p_data << uint8(playerBytes);                          // skin
    *p_data << uint8(playerBytes >> 8);                     // face
    *p_data << uint8(playerBytes >> 16);                    // hair style
    *p_data << uint8(playerBytes >> 24);                    // hair color

    uint32 playerBytes2 = fields[6].GetUInt32();
    *p_data << uint8(playerBytes2 & 0xFF);                  // facial hair

    *p_data << uint8(fields[7].GetUInt8());                 // level
    *p_data << uint32(fields[8].GetUInt32());               // zone
    *p_data << uint32(fields[9].GetUInt32());               // map

    *p_data << fields[10].GetFloat();                       // x
    *p_data << fields[11].GetFloat();                       // y
    *p_data << fields[12].GetFloat();                       // z

    *p_data << uint32(fields[13].GetUInt32());              // guild id

    uint32 char_flags = 0;
    uint32 playerFlags = fields[14].GetUInt32();
    uint32 atLoginFlags = fields[15].GetUInt32();
    if(playerFlags & PLAYER_FLAGS_HIDE_HELM)
        char_flags |= CHARACTER_FLAG_HIDE_HELM;
    if(playerFlags & PLAYER_FLAGS_HIDE_CLOAK)
        char_flags |= CHARACTER_FLAG_HIDE_CLOAK;
    if(playerFlags & PLAYER_FLAGS_GHOST)
        char_flags |= CHARACTER_FLAG_GHOST;
    if(atLoginFlags & AT_LOGIN_RENAME)
        char_flags |= CHARACTER_FLAG_RENAME;
    if(sWorld.getConfig(CONFIG_BOOL_DECLINED_NAMES_USED))
    {
        if(!fields[20].GetCppString().empty())
            char_flags |= CHARACTER_FLAG_DECLINED;
    }
    else
        char_flags |= CHARACTER_FLAG_DECLINED;

    *p_data << uint32(char_flags);                          // character flags
    // character customize flags
    *p_data << uint32(atLoginFlags & AT_LOGIN_CUSTOMIZE ? CHAR_CUSTOMIZE_FLAG_CUSTOMIZE : CHAR_CUSTOMIZE_FLAG_NONE);
    // First login
    *p_data << uint8(atLoginFlags & AT_LOGIN_FIRST ? 1 : 0);

    // Pets info
    {
        uint32 petDisplayId = 0;
        uint32 petLevel   = 0;
        uint32 petFamily  = 0;

        // show pet at selection character in character list only for non-ghost character
        if (result && !(playerFlags & PLAYER_FLAGS_GHOST) && (pClass == CLASS_WARLOCK || pClass == CLASS_HUNTER || pClass == CLASS_DEATH_KNIGHT))
        {
            uint32 entry = fields[16].GetUInt32();
            CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(entry);
            if(cInfo)
            {
                petDisplayId = fields[17].GetUInt32();
                petLevel     = fields[18].GetUInt32();
                petFamily    = cInfo->family;
            }
        }

        *p_data << uint32(petDisplayId);
        *p_data << uint32(petLevel);
        *p_data << uint32(petFamily);
    }


    Tokens data = StrSplit(fields[19].GetCppString(), " ");
    for (uint8 slot = 0; slot < EQUIPMENT_SLOT_END; slot++)
    {
        uint32 visualbase = slot * 2;
        uint32 item_id = GetUInt32ValueFromArray(data, visualbase);
        const ItemPrototype * proto = ObjectMgr::GetItemPrototype(item_id);
        if(!proto)
        {
            *p_data << uint32(0);
            *p_data << uint8(0);
            *p_data << uint32(0);
            continue;
        }

        SpellItemEnchantmentEntry const *enchant = NULL;

        uint32 enchants = GetUInt32ValueFromArray(data, visualbase + 1);
        for(uint8 enchantSlot = PERM_ENCHANTMENT_SLOT; enchantSlot <= TEMP_ENCHANTMENT_SLOT; ++enchantSlot)
        {
            // values stored in 2 uint16
            uint32 enchantId = 0x0000FFFF & (enchants >> enchantSlot*16);
            if(!enchantId)
                continue;

            if ((enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId)))
                break;
        }

        *p_data << uint32(proto->DisplayInfoID);
        *p_data << uint8(proto->InventoryType);
        *p_data << uint32(enchant ? enchant->aura_id : 0);
    }

    *p_data << uint32(0);                                   // bag 1 display id
    *p_data << uint8(0);                                    // bag 1 inventory type
    *p_data << uint32(0);                                   // enchant?
    *p_data << uint32(0);                                   // bag 2 display id
    *p_data << uint8(0);                                    // bag 2 inventory type
    *p_data << uint32(0);                                   // enchant?
    *p_data << uint32(0);                                   // bag 3 display id
    *p_data << uint8(0);                                    // bag 3 inventory type
    *p_data << uint32(0);                                   // enchant?
    *p_data << uint32(0);                                   // bag 4 display id
    *p_data << uint8(0);                                    // bag 4 inventory type
    *p_data << uint32(0);                                   // enchant?

    return true;
}

void Player::ToggleAFK()
{
    ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_AFK);

    // afk player not allowed in battleground
    if (isAFK() && InBattleGround() && !InArena())
        LeaveBattleground();
}

void Player::ToggleDND()
{
    ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_DND);
}

uint8 Player::chatTag() const
{
    // it's bitmask
    // 0x1 - afk
    // 0x2 - dnd
    // 0x4 - gm
    // 0x8 - ??

    if (isGMChat())                                         // Always show GM icons if activated
        return 4;

    if (isAFK())
        return 1;
    if (isDND())
        return 3;

    return 0;
}

bool Player::TeleportTo(uint32 mapid, float x, float y, float z, float orientation, uint32 options)
{
    if(!MapManager::IsValidMapCoord(mapid, x, y, z, orientation))
    {
        sLog.outError("TeleportTo: invalid map %d or absent instance template.", mapid);
        return false;
    }

    // preparing unsummon pet if lost (we must get pet before teleportation or will not find it later)
    Pet* pet = GetPet();

    MapEntry const* mEntry = sMapStore.LookupEntry(mapid);

    // don't let enter battlegrounds without assigned battleground id (for example through areatrigger)...
    // don't let gm level > 1 either
    if(!InBattleGround() && mEntry->IsBattleGroundOrArena())
        return false;

    // client without expansion support
    if(GetSession()->Expansion() < mEntry->Expansion())
    {
        DEBUG_LOG("Player %s using client without required expansion tried teleport to non accessible map %u", GetName(), mapid);

        if(GetTransport())
            RepopAtGraveyard();                             // teleport to near graveyard if on transport, looks blizz like :)

        SendTransferAborted(mapid, TRANSFER_ABORT_INSUF_EXPAN_LVL, mEntry->Expansion());

        return false;                                       // normal client can't teleport to this map...
    }
    else
    {
        DEBUG_LOG("Player %s is being teleported to map %u", GetName(), mapid);
    }

    if (Group* grp = GetGroup())
        grp->SetPlayerMap(GetObjectGuid(), mapid);

    // if we were on a transport, leave
    if (!(options & TELE_TO_NOT_LEAVE_TRANSPORT) && m_transport)
    {
        m_transport->RemovePassenger(this);
        m_transport = NULL;
        m_movementInfo.ClearTransportData();
    }

    // The player was ported to another map and looses the duel immediately.
    // We have to perform this check before the teleport, otherwise the
    // ObjectAccessor won't find the flag.
    if (duel && GetMapId() != mapid)
        if (GameObject* obj = GetMap()->GetGameObject(GetGuidValue(PLAYER_DUEL_ARBITER)))
            DuelComplete(DUEL_FLED);

    // reset movement flags at teleport, because player will continue move with these flags after teleport
    m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
    DisableSpline();

    if ((GetMapId() == mapid) && (!m_transport))
    {
        //lets reset far teleport flag if it wasn't reset during chained teleports
        SetSemaphoreTeleportFar(false);
        //setup delayed teleport flag
        //if teleport spell is casted in Unit::Update() func
        //then we need to delay it until update process will be finished
        if (SetDelayedTeleportFlagIfCan())
        {
            SetSemaphoreTeleportNear(true);
            //lets save teleport destination for player
            m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
            m_teleport_options = options;
            return true;
        }

        if (!(options & TELE_TO_NOT_UNSUMMON_PET))
        {
            //same map, only remove pet if out of range for new position
            if(pet && !pet->IsWithinDist3d(x, y, z, GetMap()->GetVisibilityDistance()))
                UnsummonPetTemporaryIfAny();
        }

        if (!(options & TELE_TO_NOT_LEAVE_COMBAT))
            CombatStop();

        // this will be used instead of the current location in SaveToDB
        m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
        SetFallInformation(0, z);

        // code for finish transfer called in WorldSession::HandleMovementOpcodes()
        // at client packet MSG_MOVE_TELEPORT_ACK
        SetSemaphoreTeleportNear(true);
        // near teleport, triggering send MSG_MOVE_TELEPORT_ACK from client at landing
        if(!GetSession()->PlayerLogout())
        {
            WorldPacket data;
            BuildTeleportAckMsg(data, x, y, z, orientation);
            GetSession()->SendPacket(&data);
        }
    }
    else
    {
        // far teleport to another map
        Map* oldmap = IsInWorld() ? GetMap() : NULL;
        // check if we can enter before stopping combat / removing pet / totems / interrupting spells

        // Check enter rights before map getting to avoid creating instance copy for player
        // this check not dependent from map instance copy and same for all instance copies of selected map
        if (!sMapMgr.CanPlayerEnter(mapid, this))
            return false;

        // If the map is not created, assume it is possible to enter it.
        // It will be created in the WorldPortAck.
        DungeonPersistentState* state = GetBoundInstanceSaveForSelfOrGroup(mapid);
        Map *map = sMapMgr.FindMap(mapid, state ? state->GetInstanceId() : 0);
        if (!map ||  map->CanEnter(this))
        {
            //lets reset near teleport flag if it wasn't reset during chained teleports
            SetSemaphoreTeleportNear(false);
            //setup delayed teleport flag
            //if teleport spell is casted in Unit::Update() func
            //then we need to delay it until update process will be finished
            if (SetDelayedTeleportFlagIfCan())
            {
                SetSemaphoreTeleportFar(true);
                //lets save teleport destination for player
                m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
                m_teleport_options = options;
                return true;
            }

            SetSelectionGuid(ObjectGuid());

            CombatStop();

            ResetContestedPvP();

            // remove player from battleground on far teleport (when changing maps)
            if (BattleGround const* bg = GetBattleGround())
            {
                // Note: at battleground join battleground id set before teleport
                // and we already will found "current" battleground
                // just need check that this is targeted map or leave
                if(bg->GetMapId() != mapid)
                    LeaveBattleground(false);                   // don't teleport to entry point
            }

            // remove pet on map change
            if (pet)
                UnsummonPetTemporaryIfAny();

            // remove all dyn objects
            RemoveAllDynObjects();

            // stop spellcasting
            // not attempt interrupt teleportation spell at caster teleport
            if (!(options & TELE_TO_SPELL))
                if (IsNonMeleeSpellCasted(true))
                    InterruptNonMeleeSpells(true);

            //remove auras before removing from map...
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_CHANGE_MAP | AURA_INTERRUPT_FLAG_MOVE | AURA_INTERRUPT_FLAG_TURNING);

            if (!GetSession()->PlayerLogout())
            {
                // send transfer packet to display load screen
                WorldPacket data(SMSG_TRANSFER_PENDING, (4+4+4));
                data << uint32(mapid);
                if (m_transport)
                {
                    data << uint32(m_transport->GetEntry());
                    data << uint32(GetMapId());
                }
                GetSession()->SendPacket(&data);
            }

            // remove from old map now
            if (oldmap)
                oldmap->Remove(this, false);

            // new final coordinates
            float final_x = x;
            float final_y = y;
            float final_z = z;
            float final_o = orientation;

            if (m_transport)
            {
                final_x += m_movementInfo.GetTransportPos()->x;
                final_y += m_movementInfo.GetTransportPos()->y;
                final_z += m_movementInfo.GetTransportPos()->z;
                final_o += m_movementInfo.GetTransportPos()->o;
            }

            m_teleport_dest = WorldLocation(mapid, final_x, final_y, final_z, final_o);
            SetFallInformation(0, final_z);
            // if the player is saved before worldport ack (at logout for example)
            // this will be used instead of the current location in SaveToDB

            // move packet sent by client always after far teleport
            // code for finish transfer to new map called in WorldSession::HandleMoveWorldportAckOpcode at client packet
            SetSemaphoreTeleportFar(true);

            if (!GetSession()->PlayerLogout())
            {
                // transfer finished, inform client to start load
                WorldPacket data(SMSG_NEW_WORLD, (20));
                data << uint32(mapid);
                if (m_transport)
                {
                    data << float(m_movementInfo.GetTransportPos()->x);
                    data << float(m_movementInfo.GetTransportPos()->y);
                    data << float(m_movementInfo.GetTransportPos()->z);
                    data << float(m_movementInfo.GetTransportPos()->o);
                }
                else
                {
                    data << float(final_x);
                    data << float(final_y);
                    data << float(final_z);
                    data << float(final_o);
                }

                GetSession()->SendPacket( &data );
                SendSavedInstances();
            }
        }
        else
            return false;
    }
    return true;
}

bool Player::TeleportToBGEntryPoint()
{
    ScheduleDelayedOperation(DELAYED_BG_MOUNT_RESTORE);
    ScheduleDelayedOperation(DELAYED_BG_TAXI_RESTORE);
    return TeleportTo(m_bgData.joinPos);
}

void Player::ProcessDelayedOperations()
{
    if(m_DelayedOperations == 0)
        return;

    if(m_DelayedOperations & DELAYED_RESURRECT_PLAYER)
    {
        ResurrectPlayer(0.0f, false);

        if(GetMaxHealth() > m_resurrectHealth)
            SetHealth( m_resurrectHealth );
        else
            SetHealth( GetMaxHealth() );

        if(GetMaxPower(POWER_MANA) > m_resurrectMana)
            SetPower(POWER_MANA, m_resurrectMana );
        else
            SetPower(POWER_MANA, GetMaxPower(POWER_MANA) );

        SetPower(POWER_RAGE, 0 );
        SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY) );

        SpawnCorpseBones();
    }

    if(m_DelayedOperations & DELAYED_SAVE_PLAYER)
    {
        SaveToDB();
    }

    if(m_DelayedOperations & DELAYED_SPELL_CAST_DESERTER)
    {
        CastSpell(this, 26013, true);               // Deserter
    }

    if (m_DelayedOperations & DELAYED_BG_MOUNT_RESTORE)
    {
        if (m_bgData.mountSpell)
        {
            CastSpell(this, m_bgData.mountSpell, true);
            m_bgData.mountSpell = 0;
        }
    }

    if (m_DelayedOperations & DELAYED_BG_TAXI_RESTORE)
    {
        if (m_bgData.HasTaxiPath())
        {
            m_taxi.AddTaxiDestination(m_bgData.taxiPath[0]);
            m_taxi.AddTaxiDestination(m_bgData.taxiPath[1]);
            m_bgData.ClearTaxiPath();

            ContinueTaxiFlight();
        }
    }

    //we have executed ALL delayed ops, so clear the flag
    m_DelayedOperations = 0;
}

void Player::AddToWorld()
{
    ///- Do not add/remove the player from the object storage
    ///- It will crash when updating the ObjectAccessor
    ///- The player should only be added when logging in
    Unit::AddToWorld();

    for(int i = PLAYER_SLOT_START; i < PLAYER_SLOT_END; ++i)
    {
        if(m_items[i])
            m_items[i]->AddToWorld();
    }
}

void Player::RemoveFromWorld()
{
    for(int i = PLAYER_SLOT_START; i < PLAYER_SLOT_END; ++i)
    {
        if(m_items[i])
            m_items[i]->RemoveFromWorld();
    }

    ///- Do not add/remove the player from the object storage
    ///- It will crash when updating the ObjectAccessor
    ///- The player should only be removed when logging out
    if (IsInWorld())
        GetCamera().ResetView();

    Unit::RemoveFromWorld();
}

void Player::RewardRage( uint32 damage, uint32 weaponSpeedHitFactor, bool attacker )
{
    float addRage;

    float rageconversion = float((0.0091107836 * getLevel()*getLevel())+3.225598133*getLevel())+4.2652911f;

    if(attacker)
    {
        addRage = ((damage/rageconversion*7.5f + weaponSpeedHitFactor)/2.0f);

        // talent who gave more rage on attack
        addRage *= 1.0f + GetTotalAuraModifier(SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT) / 100.0f;
    }
    else
    {
        addRage = damage/rageconversion*2.5f;

        // Berserker Rage effect
        if (HasAura(18499, EFFECT_INDEX_0))
            addRage *= 1.3f;
    }

    addRage *= sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RAGE_INCOME);

    ModifyPower(POWER_RAGE, uint32(addRage*10));
}

void Player::RegenerateAll(uint32 diff)
{
    // Not in combat or they have regeneration
    if (!isInCombat() || HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT) ||
        HasAuraType(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT) || IsPolymorphed() )
    {
        RegenerateHealth(diff);
        if (!isInCombat() && !HasAuraType(SPELL_AURA_INTERRUPT_REGEN))
        {
            Regenerate(POWER_RAGE, diff);
            if(getClass() == CLASS_DEATH_KNIGHT)
                Regenerate(POWER_RUNIC_POWER, diff);
        }
    }

    Regenerate(POWER_ENERGY, diff);

    Regenerate(POWER_MANA, diff);

    if (getClass() == CLASS_DEATH_KNIGHT)
        Regenerate(POWER_RUNE, diff);

    m_regenTimer = REGEN_TIME_FULL;
}

// diff contains the time in milliseconds since last regen.
void Player::Regenerate(Powers power, uint32 diff)
{
    uint32 curValue = GetPower(power);
    uint32 maxValue = GetMaxPower(power);

    float addvalue = 0.0f;

    switch (power)
    {
        case POWER_MANA:
        {
            if (HasAuraType(SPELL_AURA_STOP_NATURAL_MANA_REGEN))
                break;
            bool recentCast = IsUnderLastManaUseEffect();
            float ManaIncreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_MANA);
            if (recentCast)
            {
                // Mangos Updates Mana in intervals of 2s, which is correct
                addvalue = GetFloatValue(UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER) *  ManaIncreaseRate * 2.00f;
            }
            else
            {
                addvalue = GetFloatValue(UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER) * ManaIncreaseRate * 2.00f;
            }
        }   break;
        case POWER_RAGE:                                    // Regenerate rage
        {
            float RageDecreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RAGE_LOSS);
            addvalue = 20 * RageDecreaseRate;               // 2 rage by tick (= 2 seconds => 1 rage/sec)
        }   break;
        case POWER_ENERGY:                                  // Regenerate energy (rogue)
        {
            float EnergyRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_ENERGY);
            addvalue = 20 * EnergyRate;
            break;
        }
        case POWER_RUNIC_POWER:
        {
            float RunicPowerDecreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RUNICPOWER_LOSS);
            addvalue = 30 * RunicPowerDecreaseRate;         // 3 RunicPower by tick
        }   break;
        case POWER_RUNE:
        {
            if (getClass() != CLASS_DEATH_KNIGHT)
                break;

            for(uint32 rune = 0; rune < MAX_RUNES; ++rune)
            {
                if(uint16 cd = GetRuneCooldown(rune))       // if we have cooldown, reduce it...
                {
                    uint32 cd_diff = diff;
                    AuraList const& ModPowerRegenPCTAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
                    for(AuraList::const_iterator i = ModPowerRegenPCTAuras.begin(); i != ModPowerRegenPCTAuras.end(); ++i)
                        if ((*i)->GetModifier()->m_miscvalue == int32(power) && (*i)->GetMiscBValue()==GetCurrentRune(rune))
                            cd_diff = cd_diff * ((*i)->GetModifier()->m_amount + 100) / 100;

                    SetRuneCooldown(rune, (cd < cd_diff) ? 0 : cd - cd_diff);
                }
            }
        }   break;
        case POWER_FOCUS:
        case POWER_HAPPINESS:
        case POWER_HEALTH:
            break;
    }

    // Mana regen calculated in Player::UpdateManaRegen()
    // Exist only for POWER_MANA, POWER_ENERGY, POWER_FOCUS auras
    if(power != POWER_MANA)
    {
        AuraList const& ModPowerRegenPCTAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
        for(AuraList::const_iterator i = ModPowerRegenPCTAuras.begin(); i != ModPowerRegenPCTAuras.end(); ++i)
            if ((*i)->GetModifier()->m_miscvalue == int32(power))
                addvalue *= ((*i)->GetModifier()->m_amount + 100) / 100.0f;
    }

    // addvalue computed on a 2sec basis. => update to diff time
    addvalue *= float(diff) / REGEN_TIME_FULL;

    if (power != POWER_RAGE && power != POWER_RUNIC_POWER)
    {
        curValue += uint32(addvalue);
        if (curValue > maxValue)
            curValue = maxValue;
    }
    else
    {
        if(curValue <= uint32(addvalue))
            curValue = 0;
        else
            curValue -= uint32(addvalue);
    }
    SetPower(power, curValue);
}

void Player::RegenerateHealth(uint32 diff)
{
    uint32 curValue = GetHealth();
    uint32 maxValue = GetMaxHealth();

    if (curValue >= maxValue) return;

    float HealthIncreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_HEALTH);

    float addvalue = 0.0f;

    // polymorphed case
    if ( IsPolymorphed() )
        addvalue = (float)GetMaxHealth()/3;
    // normal regen case (maybe partly in combat case)
    else if (!isInCombat() || HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT) )
    {
        addvalue = OCTRegenHPPerSpirit()* HealthIncreaseRate;
        if (!isInCombat())
        {
            AuraList const& mModHealthRegenPct = GetAurasByType(SPELL_AURA_MOD_HEALTH_REGEN_PERCENT);
            for(AuraList::const_iterator i = mModHealthRegenPct.begin(); i != mModHealthRegenPct.end(); ++i)
                addvalue *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
        }
        else if(HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT))
            addvalue *= GetTotalAuraModifier(SPELL_AURA_MOD_REGEN_DURING_COMBAT) / 100.0f;

        if(!IsStandState())
            addvalue *= 1.5;
    }

    // always regeneration bonus (including combat)
    addvalue += GetTotalAuraModifier(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT);

    if(addvalue < 0)
        addvalue = 0;

    addvalue *= (float)diff / REGEN_TIME_FULL;

    ModifyHealth(int32(addvalue));
}

Creature* Player::GetNPCIfCanInteractWith(ObjectGuid guid, uint32 npcflagmask)
{
    // some basic checks
    if (!guid || !IsInWorld() || IsTaxiFlying())
        return NULL;

    // not in interactive state
    if (hasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL))
        return NULL;

    // exist (we need look pets also for some interaction (quest/etc)
    Creature *unit = GetMap()->GetAnyTypeCreature(guid);
    if (!unit)
        return NULL;

    // appropriate npc type
    if (npcflagmask && !unit->HasFlag( UNIT_NPC_FLAGS, npcflagmask ))
        return NULL;

    if (npcflagmask == UNIT_NPC_FLAG_STABLEMASTER)
    {
        if (getClass() != CLASS_HUNTER)
            return NULL;
    }

    // if a dead unit should be able to talk - the creature must be alive and have special flags
    if (!unit->isAlive())
        return NULL;

    if (isAlive() && unit->isInvisibleForAlive())
        return NULL;

    // not allow interaction under control, but allow with own pets
    if (unit->GetCharmerGuid())
        return NULL;

    // not enemy
    if (unit->IsHostileTo(this))
        return NULL;

    // not too far
    if (!unit->IsWithinDistInMap(this, INTERACTION_DISTANCE))
        return NULL;

    return unit;
}

GameObject* Player::GetGameObjectIfCanInteractWith(ObjectGuid guid, uint32 gameobject_type) const
{
    // some basic checks
    if (!guid || !IsInWorld() || IsTaxiFlying())
        return NULL;

    // not in interactive state
    if (hasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL))
        return NULL;

    if (GameObject *go = GetMap()->GetGameObject(guid))
    {
        if (uint32(go->GetGoType()) == gameobject_type || gameobject_type == MAX_GAMEOBJECT_TYPE)
        {
            float maxdist;
            switch(go->GetGoType())
            {
                // TODO: find out how the client calculates the maximal usage distance to spellless working
                // gameobjects like guildbanks and mailboxes - 10.0 is a just an abitrary choosen number
                case GAMEOBJECT_TYPE_GUILD_BANK:
                case GAMEOBJECT_TYPE_MAILBOX:
                    maxdist = 10.0f;
                    break;
                case GAMEOBJECT_TYPE_FISHINGHOLE:
                    maxdist = 20.0f+CONTACT_DISTANCE;       // max spell range
                    break;
                default:
                    maxdist = INTERACTION_DISTANCE;
                    break;
            }

            if (go->IsWithinDistInMap(this, maxdist) && go->isSpawned())
                return go;

            sLog.outError("GetGameObjectIfCanInteractWith: GameObject '%s' [GUID: %u] is too far away from player %s [GUID: %u] to be used by him (distance=%f, maximal %f is allowed)",
                go->GetGOInfo()->name,  go->GetGUIDLow(), GetName(), GetGUIDLow(), go->GetDistance(this), maxdist);
        }
    }
    return NULL;
}

bool Player::IsUnderWater() const
{
    return GetTerrain()->IsUnderWater(GetPositionX(), GetPositionY(), GetPositionZ()+2);
}

void Player::SetInWater(bool apply)
{
    if(m_isInWater==apply)
        return;

    //define player in water by opcodes
    //move player's guid into HateOfflineList of those mobs
    //which can't swim and move guid back into ThreatList when
    //on surface.
    //TODO: exist also swimming mobs, and function must be symmetric to enter/leave water
    m_isInWater = apply;

    // remove auras that need water/land
    RemoveAurasWithInterruptFlags(apply ? AURA_INTERRUPT_FLAG_NOT_ABOVEWATER : AURA_INTERRUPT_FLAG_NOT_UNDERWATER);

    getHostileRefManager().updateThreatTables();
}

struct SetGameMasterOnHelper
{
    explicit SetGameMasterOnHelper() {}
    void operator()(Unit* unit) const
    {
        unit->setFaction(35);
        unit->getHostileRefManager().setOnlineOfflineState(false);
    }
};

struct SetGameMasterOffHelper
{
    explicit SetGameMasterOffHelper(uint32 _faction) : faction(_faction) {}
    void operator()(Unit* unit) const
    {
        unit->setFaction(faction);
        unit->getHostileRefManager().setOnlineOfflineState(true);
    }
    uint32 faction;
};

void Player::SetGameMaster(bool on)
{
    if (on)
    {
        m_ExtraFlags |= PLAYER_EXTRA_GM_ON;
        setFaction(35);
        SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GM);

        CallForAllControlledUnits(SetGameMasterOnHelper(), CONTROLLED_PET|CONTROLLED_TOTEMS|CONTROLLED_GUARDIANS|CONTROLLED_CHARM);

        SetFFAPvP(false);
        ResetContestedPvP();

        getHostileRefManager().setOnlineOfflineState(false);
        CombatStopWithPets();

        SetPhaseMask(PHASEMASK_ANYWHERE,false);             // see and visible in all phases
    }
    else
    {
        m_ExtraFlags &= ~ PLAYER_EXTRA_GM_ON;
        setFactionForRace(getRace());
        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GM);

        // restore phase
        AuraList const& phases = GetAurasByType(SPELL_AURA_PHASE);
        SetPhaseMask(!phases.empty() ? phases.front()->GetMiscValue() : PHASEMASK_NORMAL,false);

        CallForAllControlledUnits(SetGameMasterOffHelper(getFaction()), CONTROLLED_PET|CONTROLLED_TOTEMS|CONTROLLED_GUARDIANS|CONTROLLED_CHARM);

        // restore FFA PvP Server state
        if(sWorld.IsFFAPvPRealm())
            SetFFAPvP(true);

        // restore FFA PvP area state, remove not allowed for GM mounts
        UpdateArea(m_areaUpdateId);

        getHostileRefManager().setOnlineOfflineState(true);
    }

    m_camera.UpdateVisibilityForOwner();
    UpdateObjectVisibility();
    UpdateForQuestWorldObjects();
}

void Player::SetGMVisible(bool on)
{
    if(on)
    {
        m_ExtraFlags &= ~PLAYER_EXTRA_GM_INVISIBLE;         //remove flag

        // Reapply stealth/invisibility if active or show if not any
        if(HasAuraType(SPELL_AURA_MOD_STEALTH))
            SetVisibility(VISIBILITY_GROUP_STEALTH);
        else if(HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
            SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
        else
            SetVisibility(VISIBILITY_ON);
    }
    else
    {
        m_ExtraFlags |= PLAYER_EXTRA_GM_INVISIBLE;          //add flag

        SetAcceptWhispers(false);
        SetGameMaster(true);

        SetVisibility(VISIBILITY_OFF);
    }
}

bool Player::IsGroupVisibleFor(Player* p) const
{
    switch(sWorld.getConfig(CONFIG_UINT32_GROUP_VISIBILITY))
    {
        default: return IsInSameGroupWith(p);
        case 1:  return IsInSameRaidWith(p);
        case 2:  return GetTeam()==p->GetTeam();
    }
}

bool Player::IsInSameGroupWith(Player const* p) const
{
    return (p==this || (GetGroup() != NULL &&
        GetGroup()->SameSubGroup((Player*)this, (Player*)p)));
}

///- If the player is invited, remove him. If the group if then only 1 person, disband the group.
/// \todo Shouldn't we also check if there is no other invitees before disbanding the group?
void Player::UninviteFromGroup()
{
    Group* group = GetGroupInvite();
    if(!group)
        return;

    group->RemoveInvite(this);

    if(group->GetMembersCount() <= 1)                       // group has just 1 member => disband
    {
        if(group->IsCreated())
        {
            group->Disband(true);
            sObjectMgr.RemoveGroup(group);
        }
        else
            group->RemoveAllInvites();

        delete group;
    }
}

void Player::RemoveFromGroup(Group* group, ObjectGuid guid)
{
    if (group)
    {
        if (group->RemoveMember(guid, 0) <= 1)
        {
            // group->Disband(); already disbanded in RemoveMember
            sObjectMgr.RemoveGroup(group);
            delete group;
            // removemember sets the player's group pointer to NULL
        }
    }
}

void Player::SendLogXPGain(uint32 GivenXP, Unit* victim, uint32 RestXP)
{
    WorldPacket data(SMSG_LOG_XPGAIN, 21);
    data << (victim ? victim->GetObjectGuid() : ObjectGuid());// guid
    data << uint32(GivenXP+RestXP);                         // given experience
    data << uint8(victim ? 0 : 1);                          // 00-kill_xp type, 01-non_kill_xp type
    if(victim)
    {
        data << uint32(GivenXP);                            // experience without rested bonus
        data << float(1);                                   // 1 - none 0 - 100% group bonus output
    }
    data << uint8(0);                                       // new 2.4.0
    GetSession()->SendPacket(&data);
}

void Player::GiveXP(uint32 xp, Unit* victim)
{
    if ( xp < 1 )
        return;

    if(!isAlive())
        return;

    uint32 level = getLevel();

    // XP to money conversion processed in Player::RewardQuest
    if(level >= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
        return;

    if(victim)
    {
        // handle SPELL_AURA_MOD_KILL_XP_PCT auras
        Unit::AuraList const& ModXPPctAuras = GetAurasByType(SPELL_AURA_MOD_KILL_XP_PCT);
        for(Unit::AuraList::const_iterator i = ModXPPctAuras.begin();i != ModXPPctAuras.end(); ++i)
            xp = uint32(xp*(1.0f + (*i)->GetModifier()->m_amount / 100.0f));
    }
    else
    {
        // handle SPELL_AURA_MOD_QUEST_XP_PCT auras
        Unit::AuraList const& ModXPPctAuras = GetAurasByType(SPELL_AURA_MOD_QUEST_XP_PCT);
        for(Unit::AuraList::const_iterator i = ModXPPctAuras.begin();i != ModXPPctAuras.end(); ++i)
            xp = uint32(xp*(1.0f + (*i)->GetModifier()->m_amount / 100.0f));
    }

    // XP resting bonus for kill
    uint32 rested_bonus_xp = victim ? GetXPRestBonus(xp) : 0;

    SendLogXPGain(xp,victim,rested_bonus_xp);

    uint32 curXP = GetUInt32Value(PLAYER_XP);
    uint32 nextLvlXP = GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
    uint32 newXP = curXP + xp + rested_bonus_xp;

    while( newXP >= nextLvlXP && level < sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL) )
    {
        newXP -= nextLvlXP;

        if ( level < sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL) )
            GiveLevel(level + 1);

        level = getLevel();
        nextLvlXP = GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
    }

    SetUInt32Value(PLAYER_XP, newXP);
}

// Update player to next level
// Current player experience not update (must be update by caller)
void Player::GiveLevel(uint32 level)
{
    if ( level == getLevel() )
        return;

    PlayerLevelInfo info;
    sObjectMgr.GetPlayerLevelInfo(getRace(),getClass(),level,&info);

    PlayerClassLevelInfo classInfo;
    sObjectMgr.GetPlayerClassLevelInfo(getClass(),level,&classInfo);

    // send levelup info to client
    WorldPacket data(SMSG_LEVELUP_INFO, (4+4+MAX_POWERS*4+MAX_STATS*4));
    data << uint32(level);
    data << uint32(int32(classInfo.basehealth) - int32(GetCreateHealth()));
    // for(int i = 0; i < MAX_POWERS; ++i)                  // Powers loop (0-6)
    data << uint32(int32(classInfo.basemana)   - int32(GetCreateMana()));
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    // end for
    for(int i = STAT_STRENGTH; i < MAX_STATS; ++i)          // Stats loop (0-4)
        data << uint32(int32(info.stats[i]) - GetCreateStat(Stats(i)));

    GetSession()->SendPacket(&data);

    SetUInt32Value(PLAYER_NEXT_LEVEL_XP, sObjectMgr.GetXPForLevel(level));

    //update level, max level of skills
    m_Played_time[PLAYED_TIME_LEVEL] = 0;                   // Level Played Time reset

    _ApplyAllLevelScaleItemMods(false);

    SetLevel(level);

    UpdateSkillsForLevel ();

    // save base values (bonuses already included in stored stats
    for(int i = STAT_STRENGTH; i < MAX_STATS; ++i)
        SetCreateStat(Stats(i), info.stats[i]);

    SetCreateHealth(classInfo.basehealth);
    SetCreateMana(classInfo.basemana);

    InitTalentForLevel();
    InitTaxiNodesForLevel();
    InitGlyphsForLevel();

    UpdateAllStats();

    // set current level health and mana/energy to maximum after applying all mods.
    if (isAlive())
        SetHealth(GetMaxHealth());
    SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));
    if(GetPower(POWER_RAGE) > GetMaxPower(POWER_RAGE))
        SetPower(POWER_RAGE, GetMaxPower(POWER_RAGE));
    SetPower(POWER_FOCUS, 0);
    SetPower(POWER_HAPPINESS, 0);

    _ApplyAllLevelScaleItemMods(true);

    // update level to hunter/summon pet
    if (Pet* pet = GetPet())
        pet->SynchronizeLevelWithOwner();

    if (MailLevelReward const* mailReward = sObjectMgr.GetMailLevelReward(level,getRaceMask()))
        MailDraft(mailReward->mailTemplateId).SendMailTo(this,MailSender(MAIL_CREATURE,mailReward->senderEntry));

    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL);
}

void Player::UpdateFreeTalentPoints(bool resetIfNeed)
{
    uint32 level = getLevel();
    // talents base at level diff ( talents = level - 9 but some can be used already)
    if (level < 10)
    {
        // Remove all talent points
        if (m_usedTalentCount > 0)                           // Free any used talents
        {
            if (resetIfNeed)
                resetTalents(true);
            SetFreeTalentPoints(0);
        }
    }
    else
    {
        uint32 talentPointsForLevel = CalculateTalentsPoints();

        // if used more that have then reset
        if (m_usedTalentCount > talentPointsForLevel)
        {
            if (resetIfNeed && GetSession()->GetSecurity() < SEC_ADMINISTRATOR)
                resetTalents(true);
            else
                SetFreeTalentPoints(0);
        }
        // else update amount of free points
        else
            SetFreeTalentPoints(talentPointsForLevel-m_usedTalentCount);
    }
}

void Player::InitTalentForLevel()
{
    UpdateFreeTalentPoints();

    if (!GetSession()->PlayerLoading())
        SendTalentsInfoData(false);                         // update at client
}

void Player::InitStatsForLevel(bool reapplyMods)
{
    if(reapplyMods)                                         //reapply stats values only on .reset stats (level) command
        _RemoveAllStatBonuses();

    PlayerClassLevelInfo classInfo;
    sObjectMgr.GetPlayerClassLevelInfo(getClass(),getLevel(),&classInfo);

    PlayerLevelInfo info;
    sObjectMgr.GetPlayerLevelInfo(getRace(),getClass(),getLevel(),&info);

    SetUInt32Value(PLAYER_FIELD_MAX_LEVEL, sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL) );
    SetUInt32Value(PLAYER_NEXT_LEVEL_XP, sObjectMgr.GetXPForLevel(getLevel()));

    // reset before any aura state sources (health set/aura apply)
    SetUInt32Value(UNIT_FIELD_AURASTATE, 0);

    UpdateSkillsForLevel ();

    // set default cast time multiplier
    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);

    // save base values (bonuses already included in stored stats
    for(int i = STAT_STRENGTH; i < MAX_STATS; ++i)
        SetCreateStat(Stats(i), info.stats[i]);

    for(int i = STAT_STRENGTH; i < MAX_STATS; ++i)
        SetStat(Stats(i), info.stats[i]);

    SetCreateHealth(classInfo.basehealth);

    //set create powers
    SetCreateMana(classInfo.basemana);

    SetArmor(int32(m_createStats[STAT_AGILITY]*2));

    InitStatBuffMods();

    //reset rating fields values
    for(uint16 index = PLAYER_FIELD_COMBAT_RATING_1; index < PLAYER_FIELD_COMBAT_RATING_1 + MAX_COMBAT_RATING; ++index)
        SetUInt32Value(index, 0);

    SetUInt32Value(PLAYER_FIELD_MOD_HEALING_DONE_POS,0);
    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG+i, 0);
        SetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS+i, 0);
        SetFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT+i, 1.00f);
    }

    //reset attack power, damage and attack speed fields
    SetFloatValue(UNIT_FIELD_BASEATTACKTIME, 2000.0f );
    SetFloatValue(UNIT_FIELD_BASEATTACKTIME + 1, 2000.0f ); // offhand attack time
    SetFloatValue(UNIT_FIELD_RANGEDATTACKTIME, 2000.0f );

    SetFloatValue(UNIT_FIELD_MINDAMAGE, 0.0f );
    SetFloatValue(UNIT_FIELD_MAXDAMAGE, 0.0f );
    SetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE, 0.0f );
    SetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE, 0.0f );
    SetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE, 0.0f );
    SetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE, 0.0f );

    SetInt32Value(UNIT_FIELD_ATTACK_POWER,            0 );
    SetInt32Value(UNIT_FIELD_ATTACK_POWER_MODS,       0 );
    SetFloatValue(UNIT_FIELD_ATTACK_POWER_MULTIPLIER,0.0f);
    SetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER,     0 );
    SetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER_MODS,0 );
    SetFloatValue(UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER,0.0f);

    // Base crit values (will be recalculated in UpdateAllStats() at loading and in _ApplyAllStatBonuses() at reset
    SetFloatValue(PLAYER_CRIT_PERCENTAGE,0.0f);
    SetFloatValue(PLAYER_OFFHAND_CRIT_PERCENTAGE,0.0f);
    SetFloatValue(PLAYER_RANGED_CRIT_PERCENTAGE,0.0f);

    // Init spell schools (will be recalculated in UpdateAllStats() at loading and in _ApplyAllStatBonuses() at reset
    for (uint8 i = 0; i < MAX_SPELL_SCHOOL; ++i)
        SetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1+i, 0.0f);

    SetFloatValue(PLAYER_PARRY_PERCENTAGE, 0.0f);
    SetFloatValue(PLAYER_BLOCK_PERCENTAGE, 0.0f);
    SetUInt32Value(PLAYER_SHIELD_BLOCK, 0);

    // Dodge percentage
    SetFloatValue(PLAYER_DODGE_PERCENTAGE, 0.0f);

    // set armor (resistance 0) to original value (create_agility*2)
    SetArmor(int32(m_createStats[STAT_AGILITY]*2));
    SetResistanceBuffMods(SpellSchools(0), true, 0.0f);
    SetResistanceBuffMods(SpellSchools(0), false, 0.0f);
    // set other resistance to original value (0)
    for (int i = 1; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetResistance(SpellSchools(i), 0);
        SetResistanceBuffMods(SpellSchools(i), true, 0.0f);
        SetResistanceBuffMods(SpellSchools(i), false, 0.0f);
    }

    SetUInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE,0);
    SetUInt32Value(PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE,0);
    for(int i = 0; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetUInt32Value(UNIT_FIELD_POWER_COST_MODIFIER+i,0);
        SetFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER+i,0.0f);
    }
    // Reset no reagent cost field
    for(int i = 0; i < 3; ++i)
        SetUInt32Value(PLAYER_NO_REAGENT_COST_1 + i, 0);
    // Init data for form but skip reapply item mods for form
    InitDataForForm(reapplyMods);

    // save new stats
    for (int i = POWER_MANA; i < MAX_POWERS; ++i)
        SetMaxPower(Powers(i),  GetCreatePowers(Powers(i)));

    SetMaxHealth(classInfo.basehealth);                     // stamina bonus will applied later

    // cleanup mounted state (it will set correctly at aura loading if player saved at mount.
    SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 0);

    // cleanup unit flags (will be re-applied if need at aura load).
    RemoveFlag( UNIT_FIELD_FLAGS,
        UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_ATTACKABLE_1 |
        UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_PASSIVE  | UNIT_FLAG_LOOTING          |
        UNIT_FLAG_PET_IN_COMBAT  | UNIT_FLAG_SILENCED     | UNIT_FLAG_PACIFIED         |
        UNIT_FLAG_STUNNED        | UNIT_FLAG_IN_COMBAT    | UNIT_FLAG_DISARMED         |
        UNIT_FLAG_CONFUSED       | UNIT_FLAG_FLEEING      | UNIT_FLAG_NOT_SELECTABLE   |
        UNIT_FLAG_SKINNABLE      | UNIT_FLAG_MOUNT        | UNIT_FLAG_TAXI_FLIGHT      );
    SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE );   // must be set

    SetFlag(UNIT_FIELD_FLAGS_2,UNIT_FLAG2_REGENERATE_POWER);// must be set

    // cleanup player flags (will be re-applied if need at aura load), to avoid have ghost flag without ghost aura, for example.
    RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_AFK | PLAYER_FLAGS_DND | PLAYER_FLAGS_GM | PLAYER_FLAGS_GHOST);

    RemoveStandFlags(UNIT_STAND_FLAGS_ALL);                 // one form stealth modified bytes
    RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP | UNIT_BYTE2_FLAG_SANCTUARY);

    // restore if need some important flags
    SetUInt32Value(PLAYER_FIELD_BYTES2, 0 );                // flags empty by default

    if(reapplyMods)                                         //reapply stats values only on .reset stats (level) command
        _ApplyAllStatBonuses();

    // set current level health and mana/energy to maximum after applying all mods.
    SetHealth(GetMaxHealth());
    SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));
    if(GetPower(POWER_RAGE) > GetMaxPower(POWER_RAGE))
        SetPower(POWER_RAGE, GetMaxPower(POWER_RAGE));
    SetPower(POWER_FOCUS, 0);
    SetPower(POWER_HAPPINESS, 0);
    SetPower(POWER_RUNIC_POWER, 0);

    // update level to hunter/summon pet
    if (Pet* pet = GetPet())
        pet->SynchronizeLevelWithOwner();
}

void Player::SendInitialSpells()
{
    time_t curTime = time(NULL);
    time_t infTime = curTime + infinityCooldownDelayCheck;

    uint16 spellCount = 0;

    WorldPacket data(SMSG_INITIAL_SPELLS, (1+2+4*m_spells.size()+2+m_spellCooldowns.size()*(2+2+2+4+4)));
    data << uint8(0);

    size_t countPos = data.wpos();
    data << uint16(spellCount);                             // spell count placeholder

    for (PlayerSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        if(itr->second.state == PLAYERSPELL_REMOVED)
            continue;

        if(!itr->second.active || itr->second.disabled)
            continue;

        data << uint32(itr->first);
        data << uint16(0);                                  // it's not slot id

        spellCount +=1;
    }

    data.put<uint16>(countPos,spellCount);                  // write real count value

    uint16 spellCooldowns = m_spellCooldowns.size();
    data << uint16(spellCooldowns);
    for(SpellCooldowns::const_iterator itr=m_spellCooldowns.begin(); itr!=m_spellCooldowns.end(); ++itr)
    {
        SpellEntry const *sEntry = sSpellStore.LookupEntry(itr->first);
        if(!sEntry)
            continue;

        data << uint32(itr->first);

        data << uint16(itr->second.itemid);                 // cast item id
        data << uint16(sEntry->Category);                   // spell category

        // send infinity cooldown in special format
        if(itr->second.end >= infTime)
        {
            data << uint32(1);                              // cooldown
            data << uint32(0x80000000);                     // category cooldown
            continue;
        }

        time_t cooldown = itr->second.end > curTime ? (itr->second.end-curTime)*IN_MILLISECONDS : 0;

        if(sEntry->Category)                                // may be wrong, but anyway better than nothing...
        {
            data << uint32(0);                              // cooldown
            data << uint32(cooldown);                       // category cooldown
        }
        else
        {
            data << uint32(cooldown);                       // cooldown
            data << uint32(0);                              // category cooldown
        }
    }

    GetSession()->SendPacket(&data);

    DETAIL_LOG( "CHARACTER: Sent Initial Spells" );
}

void Player::RemoveMail(uint32 id)
{
    for(PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end();++itr)
    {
        if ((*itr)->messageID == id)
        {
            //do not delete item, because Player::removeMail() is called when returning mail to sender.
            m_mail.erase(itr);
            return;
        }
    }
}

void Player::SendMailResult(uint32 mailId, MailResponseType mailAction, MailResponseResult mailError, uint32 equipError, uint32 item_guid, uint32 item_count)
{
    WorldPacket data(SMSG_SEND_MAIL_RESULT, (4+4+4+(mailError == MAIL_ERR_EQUIP_ERROR?4:(mailAction == MAIL_ITEM_TAKEN?4+4:0))));
    data << (uint32) mailId;
    data << (uint32) mailAction;
    data << (uint32) mailError;
    if ( mailError == MAIL_ERR_EQUIP_ERROR )
        data << (uint32) equipError;
    else if( mailAction == MAIL_ITEM_TAKEN )
    {
        data << (uint32) item_guid;                         // item guid low?
        data << (uint32) item_count;                        // item count?
    }
    GetSession()->SendPacket(&data);
}

void Player::SendNewMail()
{
    // deliver undelivered mail
    WorldPacket data(SMSG_RECEIVED_MAIL, 4);
    data << (uint32) 0;
    GetSession()->SendPacket(&data);
}

void Player::UpdateNextMailTimeAndUnreads()
{
    // calculate next delivery time (min. from non-delivered mails
    // and recalculate unReadMail
    time_t cTime = time(NULL);
    m_nextMailDelivereTime = 0;
    unReadMails = 0;
    for(PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        if((*itr)->deliver_time > cTime)
        {
            if(!m_nextMailDelivereTime || m_nextMailDelivereTime > (*itr)->deliver_time)
                m_nextMailDelivereTime = (*itr)->deliver_time;
        }
        else if(((*itr)->checked & MAIL_CHECK_MASK_READ) == 0)
            ++unReadMails;
    }
}

void Player::AddNewMailDeliverTime(time_t deliver_time)
{
    if(deliver_time <= time(NULL))                          // ready now
    {
        ++unReadMails;
        SendNewMail();
    }
    else                                                    // not ready and no have ready mails
    {
        if(!m_nextMailDelivereTime || m_nextMailDelivereTime > deliver_time)
            m_nextMailDelivereTime =  deliver_time;
    }
}

bool Player::addSpell(uint32 spell_id, bool active, bool learning, bool dependent, bool disabled)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spell_id);
    if (!spellInfo)
    {
        // do character spell book cleanup (all characters)
        if(!IsInWorld() && !learning)                       // spell load case
        {
            sLog.outError("Player::addSpell: nonexistent in SpellStore spell #%u request, deleting for all characters in `character_spell`.",spell_id);
            CharacterDatabase.PExecute("DELETE FROM character_spell WHERE spell = '%u'",spell_id);
        }
        else
            sLog.outError("Player::addSpell: nonexistent in SpellStore spell #%u request.",spell_id);

        return false;
    }

    if(!SpellMgr::IsSpellValid(spellInfo,this,false))
    {
        // do character spell book cleanup (all characters)
        if(!IsInWorld() && !learning)                       // spell load case
        {
            sLog.outError("Player::addSpell: Broken spell #%u learning not allowed, deleting for all characters in `character_spell`.",spell_id);
            CharacterDatabase.PExecute("DELETE FROM character_spell WHERE spell = '%u'",spell_id);
        }
        else
            sLog.outError("Player::addSpell: Broken spell #%u learning not allowed.",spell_id);

        return false;
    }

    PlayerSpellState state = learning ? PLAYERSPELL_NEW : PLAYERSPELL_UNCHANGED;

    bool dependent_set = false;
    bool disabled_case = false;
    bool superceded_old = false;

    PlayerSpellMap::iterator itr = m_spells.find(spell_id);
    if (itr != m_spells.end())
    {
        uint32 next_active_spell_id = 0;
        // fix activate state for non-stackable low rank (and find next spell for !active case)
        if(sSpellMgr.IsRankedSpellNonStackableInSpellBook(spellInfo))
        {
            SpellChainMapNext const& nextMap = sSpellMgr.GetSpellChainNext();
            for(SpellChainMapNext::const_iterator next_itr = nextMap.lower_bound(spell_id); next_itr != nextMap.upper_bound(spell_id); ++next_itr)
            {
                if(HasSpell(next_itr->second))
                {
                    // high rank already known so this must !active
                    active = false;
                    next_active_spell_id = next_itr->second;
                    break;
                }
            }
        }

        // not do anything if already known in expected state
        if(itr->second.state != PLAYERSPELL_REMOVED && itr->second.active == active &&
            itr->second.dependent == dependent && itr->second.disabled == disabled)
        {
            if(!IsInWorld() && !learning)                   // explicitly load from DB and then exist in it already and set correctly
                itr->second.state = PLAYERSPELL_UNCHANGED;

            return false;
        }

        // dependent spell known as not dependent, overwrite state
        if (itr->second.state != PLAYERSPELL_REMOVED && !itr->second.dependent && dependent)
        {
            itr->second.dependent = dependent;
            if (itr->second.state != PLAYERSPELL_NEW)
                itr->second.state = PLAYERSPELL_CHANGED;
            dependent_set = true;
        }

        // update active state for known spell
        if(itr->second.active != active && itr->second.state != PLAYERSPELL_REMOVED && !itr->second.disabled)
        {
            itr->second.active = active;

            if(!IsInWorld() && !learning && !dependent_set) // explicitly load from DB and then exist in it already and set correctly
                itr->second.state = PLAYERSPELL_UNCHANGED;
            else if(itr->second.state != PLAYERSPELL_NEW)
                itr->second.state = PLAYERSPELL_CHANGED;

            if(active)
            {
                if (IsNeedCastPassiveLikeSpellAtLearn(spellInfo))
                    CastSpell (this, spell_id, true);
            }
            else if(IsInWorld())
            {
                if(next_active_spell_id)
                {
                    // update spell ranks in spellbook and action bar
                    WorldPacket data(SMSG_SUPERCEDED_SPELL, 4 + 4);
                    data << uint32(spell_id);
                    data << uint32(next_active_spell_id);
                    GetSession()->SendPacket( &data );
                }
                else
                {
                    WorldPacket data(SMSG_REMOVED_SPELL, 4);
                    data << uint32(spell_id);
                    GetSession()->SendPacket(&data);
                }
            }

            return active;                                  // learn (show in spell book if active now)
        }

        if(itr->second.disabled != disabled && itr->second.state != PLAYERSPELL_REMOVED)
        {
            if(itr->second.state != PLAYERSPELL_NEW)
                itr->second.state = PLAYERSPELL_CHANGED;
            itr->second.disabled = disabled;

            if(disabled)
                return false;

            disabled_case = true;
        }
        else switch(itr->second.state)
        {
            case PLAYERSPELL_UNCHANGED:                     // known saved spell
                return false;
            case PLAYERSPELL_REMOVED:                       // re-learning removed not saved spell
            {
                m_spells.erase(itr);
                state = PLAYERSPELL_CHANGED;
                break;                                      // need re-add
            }
            default:                                        // known not saved yet spell (new or modified)
            {
                // can be in case spell loading but learned at some previous spell loading
                if(!IsInWorld() && !learning && !dependent_set)
                    itr->second.state = PLAYERSPELL_UNCHANGED;

                return false;
            }
        }
    }

    TalentSpellPos const* talentPos = GetTalentSpellPos(spell_id);

    if(!disabled_case) // skip new spell adding if spell already known (disabled spells case)
    {
        // talent: unlearn all other talent ranks (high and low)
        if (talentPos)
        {
            if(TalentEntry const *talentInfo = sTalentStore.LookupEntry( talentPos->talent_id ))
            {
                for(int i=0; i < MAX_TALENT_RANK; ++i)
                {
                    // skip learning spell and no rank spell case
                    uint32 rankSpellId = talentInfo->RankID[i];
                    if(!rankSpellId || rankSpellId == spell_id)
                        continue;

                    removeSpell(rankSpellId, false, false);
                }
            }
        }
        // non talent spell: learn low ranks (recursive call)
        else if(uint32 prev_spell = sSpellMgr.GetPrevSpellInChain(spell_id))
        {
            if(!IsInWorld() || disabled)                    // at spells loading, no output, but allow save
                addSpell(prev_spell, active, true, true, disabled);
            else                                            // at normal learning
                learnSpell(prev_spell, true);
        }

        PlayerSpell newspell;
        newspell.state     = state;
        newspell.active    = active;
        newspell.dependent = dependent;
        newspell.disabled  = disabled;

        // replace spells in action bars and spellbook to bigger rank if only one spell rank must be accessible
        if(newspell.active && !newspell.disabled && sSpellMgr.IsRankedSpellNonStackableInSpellBook(spellInfo))
        {
            for( PlayerSpellMap::iterator itr2 = m_spells.begin(); itr2 != m_spells.end(); ++itr2 )
            {
                if(itr2->second.state == PLAYERSPELL_REMOVED) continue;
                SpellEntry const *i_spellInfo = sSpellStore.LookupEntry(itr2->first);
                if(!i_spellInfo) continue;

                if( sSpellMgr.IsRankSpellDueToSpell(spellInfo, itr2->first) )
                {
                    if(itr2->second.active)
                    {
                        if(sSpellMgr.IsHighRankOfSpell(spell_id,itr2->first))
                        {
                            if(IsInWorld())                 // not send spell (re-/over-)learn packets at loading
                            {
                                WorldPacket data(SMSG_SUPERCEDED_SPELL, 4 + 4);
                                data << uint32(itr2->first);
                                data << uint32(spell_id);
                                GetSession()->SendPacket( &data );
                            }

                            // mark old spell as disable (SMSG_SUPERCEDED_SPELL replace it in client by new)
                            itr2->second.active = false;
                            if(itr2->second.state != PLAYERSPELL_NEW)
                                itr2->second.state = PLAYERSPELL_CHANGED;
                            superceded_old = true;          // new spell replace old in action bars and spell book.
                        }
                        else if(sSpellMgr.IsHighRankOfSpell(itr2->first,spell_id))
                        {
                            if(IsInWorld())                 // not send spell (re-/over-)learn packets at loading
                            {
                                WorldPacket data(SMSG_SUPERCEDED_SPELL, 4 + 4);
                                data << uint32(spell_id);
                                data << uint32(itr2->first);
                                GetSession()->SendPacket( &data );
                            }

                            // mark new spell as disable (not learned yet for client and will not learned)
                            newspell.active = false;
                            if(newspell.state != PLAYERSPELL_NEW)
                                newspell.state = PLAYERSPELL_CHANGED;
                        }
                    }
                }
            }
        }

        m_spells[spell_id] = newspell;

        // return false if spell disabled
        if (newspell.disabled)
            return false;
    }

    if (talentPos)
    {
        // update talent map
        PlayerTalentMap::iterator iter = m_talents[m_activeSpec].find(talentPos->talent_id);
        if (iter != m_talents[m_activeSpec].end())
        {
            // check if ranks different or removed
            if ((*iter).second.state == PLAYERSPELL_REMOVED || talentPos->rank != (*iter).second.currentRank)
            {
                (*iter).second.currentRank = talentPos->rank;

                if ((*iter).second.state != PLAYERSPELL_NEW)
                    (*iter).second.state = PLAYERSPELL_CHANGED;
            }
        }
        else
        {
            PlayerTalent talent;
            talent.currentRank = talentPos->rank;
            talent.talentEntry = sTalentStore.LookupEntry(talentPos->talent_id);
            talent.state       = IsInWorld() ? PLAYERSPELL_NEW : PLAYERSPELL_UNCHANGED;
            m_talents[m_activeSpec][talentPos->talent_id] = talent;
        }

        // update used talent points count
        m_usedTalentCount += GetTalentSpellCost(talentPos);
        UpdateFreeTalentPoints(false);
    }

    // update free primary prof.points (if any, can be none in case GM .learn prof. learning)
    if (uint32 freeProfs = GetFreePrimaryProfessionPoints())
    {
        if(sSpellMgr.IsPrimaryProfessionFirstRankSpell(spell_id))
            SetFreePrimaryProfessions(freeProfs-1);
    }

    // cast talents with SPELL_EFFECT_LEARN_SPELL (other dependent spells will learned later as not auto-learned)
    // note: all spells with SPELL_EFFECT_LEARN_SPELL isn't passive
    if (talentPos && IsSpellHaveEffect(spellInfo,SPELL_EFFECT_LEARN_SPELL))
    {
        // ignore stance requirement for talent learn spell (stance set for spell only for client spell description show)
        CastSpell(this, spell_id, true);
    }
    // also cast passive (and passive like) spells (including all talents without SPELL_EFFECT_LEARN_SPELL) with additional checks
    else if (IsNeedCastPassiveLikeSpellAtLearn(spellInfo))
    {
        CastSpell(this, spell_id, true);
    }
    else if (IsSpellHaveEffect(spellInfo,SPELL_EFFECT_SKILL_STEP))
    {
        CastSpell(this, spell_id, true);
        return false;
    }

    // add dependent skills
    uint16 maxskill = GetMaxSkillValueForLevel();

    SpellLearnSkillNode const* spellLearnSkill = sSpellMgr.GetSpellLearnSkill(spell_id);

    SkillLineAbilityMapBounds skill_bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spell_id);

    if (spellLearnSkill)
    {
        uint32 skill_value = GetPureSkillValue(spellLearnSkill->skill);
        uint32 skill_max_value = GetPureMaxSkillValue(spellLearnSkill->skill);

        if (skill_value < spellLearnSkill->value)
            skill_value = spellLearnSkill->value;

        uint32 new_skill_max_value = spellLearnSkill->maxvalue == 0 ? maxskill : spellLearnSkill->maxvalue;

        if (skill_max_value < new_skill_max_value)
            skill_max_value =  new_skill_max_value;

        SetSkill(spellLearnSkill->skill, skill_value, skill_max_value, spellLearnSkill->step);
    }
    else
    {
        // not ranked skills
        for(SkillLineAbilityMap::const_iterator _spell_idx = skill_bounds.first; _spell_idx != skill_bounds.second; ++_spell_idx)
        {
            SkillLineEntry const *pSkill = sSkillLineStore.LookupEntry(_spell_idx->second->skillId);
            if (!pSkill)
                continue;

            if (HasSkill(pSkill->id))
                continue;

            if (_spell_idx->second->learnOnGetSkill == ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL ||
                // lockpicking/runeforging special case, not have ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL
                ((pSkill->id==SKILL_LOCKPICKING || pSkill->id==SKILL_RUNEFORGING) && _spell_idx->second->max_value==0))
            {
                switch(GetSkillRangeType(pSkill, _spell_idx->second->racemask != 0))
                {
                    case SKILL_RANGE_LANGUAGE:
                        SetSkill(pSkill->id, 300, 300 );
                        break;
                    case SKILL_RANGE_LEVEL:
                        SetSkill(pSkill->id, 1, GetMaxSkillValueForLevel() );
                        break;
                    case SKILL_RANGE_MONO:
                        SetSkill(pSkill->id, 1, 1 );
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // learn dependent spells
    SpellLearnSpellMapBounds spell_bounds = sSpellMgr.GetSpellLearnSpellMapBounds(spell_id);

    for(SpellLearnSpellMap::const_iterator itr2 = spell_bounds.first; itr2 != spell_bounds.second; ++itr2)
    {
        if (!itr2->second.autoLearned)
        {
            if (!IsInWorld() || !itr2->second.active)       // at spells loading, no output, but allow save
                addSpell(itr2->second.spell,itr2->second.active,true,true,false);
            else                                            // at normal learning
                learnSpell(itr2->second.spell, true);
        }
    }

    if (!GetSession()->PlayerLoading())
    {
        // not ranked skills
        for(SkillLineAbilityMap::const_iterator _spell_idx = skill_bounds.first; _spell_idx != skill_bounds.second; ++_spell_idx)
        {
            GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE,_spell_idx->second->skillId);
            GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS,_spell_idx->second->skillId);
        }

        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL,spell_id);
    }

    // return true (for send learn packet) only if spell active (in case ranked spells) and not replace old spell
    return active && !disabled && !superceded_old;
}

bool Player::IsNeedCastPassiveLikeSpellAtLearn(SpellEntry const* spellInfo) const
{
    ShapeshiftForm form = GetShapeshiftForm();

    if (IsNeedCastSpellAtFormApply(spellInfo, form))        // SPELL_ATTR_PASSIVE | SPELL_ATTR_UNK7 spells
        return true;                                        // all stance req. cases, not have auarastate cases

    if (!(spellInfo->Attributes & SPELL_ATTR_PASSIVE))
        return false;

    // note: form passives activated with shapeshift spells be implemented by HandleShapeshiftBoosts instead of spell_learn_spell
    // talent dependent passives activated at form apply have proper stance data
    bool need_cast = (!spellInfo->Stances || !form && (spellInfo->AttributesEx2 & SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT));

    // Check CasterAuraStates
    return need_cast && (!spellInfo->CasterAuraState || HasAuraState(AuraState(spellInfo->CasterAuraState)));
}

void Player::learnSpell(uint32 spell_id, bool dependent)
{
    PlayerSpellMap::iterator itr = m_spells.find(spell_id);

    bool disabled = (itr != m_spells.end()) ? itr->second.disabled : false;
    bool active = disabled ? itr->second.active : true;

    bool learning = addSpell(spell_id, active, true, dependent, false);

    // prevent duplicated entires in spell book, also not send if not in world (loading)
    if (learning && IsInWorld())
    {
        WorldPacket data(SMSG_LEARNED_SPELL, 6);
        data << uint32(spell_id);
        data << uint16(0);                                  // 3.3.3 unk
        GetSession()->SendPacket(&data);
    }

    // learn all disabled higher ranks (recursive)
    if(disabled)
    {
        SpellChainMapNext const& nextMap = sSpellMgr.GetSpellChainNext();
        for(SpellChainMapNext::const_iterator i = nextMap.lower_bound(spell_id); i != nextMap.upper_bound(spell_id); ++i)
        {
            PlayerSpellMap::iterator iter = m_spells.find(i->second);
            if (iter != m_spells.end() && iter->second.disabled)
                learnSpell(i->second, false);
        }
    }
}

void Player::removeSpell(uint32 spell_id, bool disabled, bool learn_low_rank, bool sendUpdate)
{
    PlayerSpellMap::iterator itr = m_spells.find(spell_id);
    if (itr == m_spells.end())
        return;

    if (itr->second.state == PLAYERSPELL_REMOVED || (disabled && itr->second.disabled))
        return;

    // unlearn non talent higher ranks (recursive)
    SpellChainMapNext const& nextMap = sSpellMgr.GetSpellChainNext();
    for(SpellChainMapNext::const_iterator itr2 = nextMap.lower_bound(spell_id); itr2 != nextMap.upper_bound(spell_id); ++itr2)
        if(HasSpell(itr2->second) && !GetTalentSpellPos(itr2->second))
            removeSpell(itr2->second, disabled, false);

    // re-search, it can be corrupted in prev loop
    itr = m_spells.find(spell_id);
    if (itr == m_spells.end() || itr->second.state == PLAYERSPELL_REMOVED)
        return;                                             // already unleared

    bool cur_active    = itr->second.active;
    bool cur_dependent = itr->second.dependent;

    if (disabled)
    {
        itr->second.disabled = disabled;
        if(itr->second.state != PLAYERSPELL_NEW)
            itr->second.state = PLAYERSPELL_CHANGED;
    }
    else
    {
        if(itr->second.state == PLAYERSPELL_NEW)
            m_spells.erase(itr);
        else
            itr->second.state = PLAYERSPELL_REMOVED;
    }

    RemoveAurasDueToSpell(spell_id);

    // remove pet auras
    for(int i = 0; i < MAX_EFFECT_INDEX; ++i)
        if(PetAura const* petSpell = sSpellMgr.GetPetAura(spell_id, SpellEffectIndex(i)))
            RemovePetAura(petSpell);

    TalentSpellPos const* talentPos = GetTalentSpellPos(spell_id);
    if (talentPos)
    {
        // update talent map
        PlayerTalentMap::iterator iter = m_talents[m_activeSpec].find(talentPos->talent_id);
        if (iter != m_talents[m_activeSpec].end())
        {
            if ((*iter).second.state != PLAYERSPELL_NEW)
                (*iter).second.state = PLAYERSPELL_REMOVED;
            else
                m_talents[m_activeSpec].erase(iter);
        }
        else
            sLog.outError("removeSpell: Player (GUID: %u) has talent spell (id: %u) but doesn't have talent",GetGUIDLow(), spell_id );

        // free talent points
        uint32 talentCosts = GetTalentSpellCost(talentPos);

        if(talentCosts < m_usedTalentCount)
            m_usedTalentCount -= talentCosts;
        else
            m_usedTalentCount = 0;

        UpdateFreeTalentPoints(false);
    }

    // update free primary prof.points (if not overflow setting, can be in case GM use before .learn prof. learning)
    if(sSpellMgr.IsPrimaryProfessionFirstRankSpell(spell_id))
    {
        uint32 freeProfs = GetFreePrimaryProfessionPoints()+1;
        uint32 maxProfs = GetSession()->GetSecurity() < AccountTypes(sWorld.getConfig(CONFIG_UINT32_TRADE_SKILL_GMIGNORE_MAX_PRIMARY_COUNT)) ? sWorld.getConfig(CONFIG_UINT32_MAX_PRIMARY_TRADE_SKILL) : 10;
        if(freeProfs <= maxProfs)
            SetFreePrimaryProfessions(freeProfs);
    }

    // remove dependent skill
    SpellLearnSkillNode const* spellLearnSkill = sSpellMgr.GetSpellLearnSkill(spell_id);
    if(spellLearnSkill)
    {
        uint32 prev_spell = sSpellMgr.GetPrevSpellInChain(spell_id);
        if(!prev_spell)                                     // first rank, remove skill
            SetSkill(spellLearnSkill->skill, 0, 0);
        else
        {
            // search prev. skill setting by spell ranks chain
            SpellLearnSkillNode const* prevSkill = sSpellMgr.GetSpellLearnSkill(prev_spell);
            while(!prevSkill && prev_spell)
            {
                prev_spell = sSpellMgr.GetPrevSpellInChain(prev_spell);
                prevSkill = sSpellMgr.GetSpellLearnSkill(sSpellMgr.GetFirstSpellInChain(prev_spell));
            }

            if (!prevSkill)                                 // not found prev skill setting, remove skill
                SetSkill(spellLearnSkill->skill, 0, 0);
            else                                            // set to prev. skill setting values
            {
                uint32 skill_value = GetPureSkillValue(prevSkill->skill);
                uint32 skill_max_value = GetPureMaxSkillValue(prevSkill->skill);

                if (skill_value >  prevSkill->value)
                    skill_value = prevSkill->value;

                uint32 new_skill_max_value = prevSkill->maxvalue == 0 ? GetMaxSkillValueForLevel() : prevSkill->maxvalue;

                if (skill_max_value > new_skill_max_value)
                    skill_max_value =  new_skill_max_value;

                SetSkill(prevSkill->skill, skill_value, skill_max_value, prevSkill->step);
            }
        }

    }
    else
    {
        // not ranked skills
        SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spell_id);

        for(SkillLineAbilityMap::const_iterator _spell_idx = bounds.first; _spell_idx != bounds.second; ++_spell_idx)
        {
            SkillLineEntry const *pSkill = sSkillLineStore.LookupEntry(_spell_idx->second->skillId);
            if (!pSkill)
                continue;

            if ((_spell_idx->second->learnOnGetSkill == ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL &&
                pSkill->categoryId != SKILL_CATEGORY_CLASS) ||// not unlearn class skills (spellbook/talent pages)
                // lockpicking/runeforging special case, not have ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL
                ((pSkill->id == SKILL_LOCKPICKING || pSkill->id == SKILL_RUNEFORGING) && _spell_idx->second->max_value == 0))
            {
                // not reset skills for professions and racial abilities
                if ((pSkill->categoryId == SKILL_CATEGORY_SECONDARY || pSkill->categoryId == SKILL_CATEGORY_PROFESSION) &&
                    (IsProfessionSkill(pSkill->id) || _spell_idx->second->racemask != 0))
                    continue;

                SetSkill(pSkill->id, 0, 0);
            }
        }
    }

    // remove dependent spells
    SpellLearnSpellMapBounds spell_bounds = sSpellMgr.GetSpellLearnSpellMapBounds(spell_id);

    for(SpellLearnSpellMap::const_iterator itr2 = spell_bounds.first; itr2 != spell_bounds.second; ++itr2)
        removeSpell(itr2->second.spell, disabled);

    // activate lesser rank in spellbook/action bar, and cast it if need
    bool prev_activate = false;

    if (uint32 prev_id = sSpellMgr.GetPrevSpellInChain (spell_id))
    {
        SpellEntry const *spellInfo = sSpellStore.LookupEntry(spell_id);

        // if talent then lesser rank also talent and need learn
        if (talentPos)
        {
            if(learn_low_rank)
                learnSpell(prev_id, false);
        }
        // if ranked non-stackable spell: need activate lesser rank and update dependence state
        else if (cur_active && sSpellMgr.IsRankedSpellNonStackableInSpellBook(spellInfo))
        {
            // need manually update dependence state (learn spell ignore like attempts)
            PlayerSpellMap::iterator prev_itr = m_spells.find(prev_id);
            if (prev_itr != m_spells.end())
            {
                if (prev_itr->second.dependent != cur_dependent)
                {
                    prev_itr->second.dependent = cur_dependent;
                    if (prev_itr->second.state != PLAYERSPELL_NEW)
                        prev_itr->second.state = PLAYERSPELL_CHANGED;
                }

                // now re-learn if need re-activate
                if (cur_active && !prev_itr->second.active && learn_low_rank)
                {
                    if (addSpell(prev_id, true, false, prev_itr->second.dependent, prev_itr->second.disabled))
                    {
                        // downgrade spell ranks in spellbook and action bar
                        WorldPacket data(SMSG_SUPERCEDED_SPELL, 4 + 4);
                        data << uint32(spell_id);
                        data << uint32(prev_id);
                        GetSession()->SendPacket( &data );
                        prev_activate = true;
                    }
                }
            }
        }
    }

    // for Titan's Grip and shaman Dual-wield
    if (CanDualWield() || CanTitanGrip())
    {
        SpellEntry const *spellInfo = sSpellStore.LookupEntry(spell_id);

        if (CanDualWield() && IsSpellHaveEffect(spellInfo, SPELL_EFFECT_DUAL_WIELD))
            SetCanDualWield(false);

        if (CanTitanGrip() && IsSpellHaveEffect(spellInfo, SPELL_EFFECT_TITAN_GRIP))
        {
            SetCanTitanGrip(false);
            // Remove Titan's Grip damage penalty now
            RemoveAurasDueToSpell(49152);
        }
    }

    // for talents and normal spell unlearn that allow offhand use for some weapons
    if(sWorld.getConfig(CONFIG_BOOL_OFFHAND_CHECK_AT_TALENTS_RESET))
        AutoUnequipOffhandIfNeed();

    // remove from spell book if not replaced by lesser rank
    if (!prev_activate && sendUpdate)
    {
        WorldPacket data(SMSG_REMOVED_SPELL, 4);
        data << uint32(spell_id);
        GetSession()->SendPacket(&data);
    }
}

void Player::RemoveSpellCooldown( uint32 spell_id, bool update /* = false */ )
{
    m_spellCooldowns.erase(spell_id);

    if(update)
        SendClearCooldown(spell_id, this);
}

void Player::RemoveSpellCategoryCooldown(uint32 cat, bool update /* = false */)
{
    SpellCategoryStore::const_iterator ct = sSpellCategoryStore.find(cat);
    if (ct == sSpellCategoryStore.end())
        return;

    const SpellCategorySet& ct_set = ct->second;
    for (SpellCooldowns::const_iterator i = m_spellCooldowns.begin(); i != m_spellCooldowns.end();)
    {
        if (ct_set.find(i->first) != ct_set.end())
            RemoveSpellCooldown((i++)->first, update);
        else
            ++i;
    }
}

void Player::RemoveArenaSpellCooldowns()
{
    // remove cooldowns on spells that has < 15 min CD
    SpellCooldowns::iterator itr, next;
    // iterate spell cooldowns
    for(itr = m_spellCooldowns.begin();itr != m_spellCooldowns.end(); itr = next)
    {
        next = itr;
        ++next;
        SpellEntry const * entry = sSpellStore.LookupEntry(itr->first);
        // check if spellentry is present and if the cooldown is less than 15 mins
        if( entry &&
            entry->RecoveryTime <= 15 * MINUTE * IN_MILLISECONDS &&
            entry->CategoryRecoveryTime <= 15 * MINUTE * IN_MILLISECONDS )
        {
            // remove & notify
            RemoveSpellCooldown(itr->first, true);
        }
    }
}

void Player::RemoveAllSpellCooldown()
{
    if(!m_spellCooldowns.empty())
    {
        for(SpellCooldowns::const_iterator itr = m_spellCooldowns.begin();itr != m_spellCooldowns.end(); ++itr)
            SendClearCooldown(itr->first, this);

        m_spellCooldowns.clear();
    }
}

void Player::_LoadSpellCooldowns(QueryResult *result)
{
    // some cooldowns can be already set at aura loading...

    //QueryResult *result = CharacterDatabase.PQuery("SELECT spell,item,time FROM character_spell_cooldown WHERE guid = '%u'",GetGUIDLow());

    if(result)
    {
        time_t curTime = time(NULL);

        do
        {
            Field *fields = result->Fetch();

            uint32 spell_id = fields[0].GetUInt32();
            uint32 item_id  = fields[1].GetUInt32();
            time_t db_time  = (time_t)fields[2].GetUInt64();

            if(!sSpellStore.LookupEntry(spell_id))
            {
                sLog.outError("Player %u has unknown spell %u in `character_spell_cooldown`, skipping.",GetGUIDLow(),spell_id);
                continue;
            }

            // skip outdated cooldown
            if(db_time <= curTime)
                continue;

            AddSpellCooldown(spell_id, item_id, db_time);

            DEBUG_LOG("Player (GUID: %u) spell %u, item %u cooldown loaded (%u secs).", GetGUIDLow(), spell_id, item_id, uint32(db_time-curTime));
        }
        while( result->NextRow() );

        delete result;
    }
}

void Player::_SaveSpellCooldowns()
{
    static SqlStatementID deleteSpellCooldown ;
    static SqlStatementID insertSpellCooldown ;

    SqlStatement stmt = CharacterDatabase.CreateStatement(deleteSpellCooldown, "DELETE FROM character_spell_cooldown WHERE guid = ?");
    stmt.PExecute(GetGUIDLow());

    time_t curTime = time(NULL);
    time_t infTime = curTime + infinityCooldownDelayCheck;

    // remove outdated and save active
    for(SpellCooldowns::iterator itr = m_spellCooldowns.begin();itr != m_spellCooldowns.end();)
    {
        if(itr->second.end <= curTime)
            m_spellCooldowns.erase(itr++);
        else if(itr->second.end <= infTime)                 // not save locked cooldowns, it will be reset or set at reload
        {
            stmt = CharacterDatabase.CreateStatement(insertSpellCooldown, "INSERT INTO character_spell_cooldown (guid,spell,item,time) VALUES( ?, ?, ?, ?)");
            stmt.PExecute(GetGUIDLow(), itr->first, itr->second.itemid, uint64(itr->second.end));
            ++itr;
        }
        else
            ++itr;
    }
}

uint32 Player::resetTalentsCost() const
{
    // The first time reset costs 1 gold
    if(m_resetTalentsCost < 1*GOLD)
        return 1*GOLD;
    // then 5 gold
    else if(m_resetTalentsCost < 5*GOLD)
        return 5*GOLD;
    // After that it increases in increments of 5 gold
    else if(m_resetTalentsCost < 10*GOLD)
        return 10*GOLD;
    else
    {
        time_t months = (sWorld.GetGameTime() - m_resetTalentsTime)/MONTH;
        if(months > 0)
        {
            // This cost will be reduced by a rate of 5 gold per month
            int32 new_cost = int32((m_resetTalentsCost) - 5*GOLD*months);
            // to a minimum of 10 gold.
            return uint32(new_cost < 10*GOLD ? 10*GOLD : new_cost);
        }
        else
        {
            // After that it increases in increments of 5 gold
            int32 new_cost = m_resetTalentsCost + 5*GOLD;
            // until it hits a cap of 50 gold.
            if(new_cost > 50*GOLD)
                new_cost = 50*GOLD;
            return new_cost;
        }
    }
}

bool Player::resetTalents(bool no_cost, bool all_specs)
{
    // not need after this call
    if(HasAtLoginFlag(AT_LOGIN_RESET_TALENTS) && all_specs)
        RemoveAtLoginFlag(AT_LOGIN_RESET_TALENTS,true);

    if (m_usedTalentCount == 0 && !all_specs)
    {
        UpdateFreeTalentPoints(false);                      // for fix if need counter
        return false;
    }

    uint32 cost = 0;

    if(!no_cost)
    {
        cost = resetTalentsCost();

        if (GetMoney() < cost)
        {
            SendBuyError( BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
            return false;
        }
    }

    for (PlayerTalentMap::iterator iter = m_talents[m_activeSpec].begin(); iter != m_talents[m_activeSpec].end();)
    {
        if (iter->second.state == PLAYERSPELL_REMOVED)
        {
            ++iter;
            continue;
        }

        TalentEntry const* talentInfo = iter->second.talentEntry;
        if (!talentInfo)
        {
            m_talents[m_activeSpec].erase(iter++);
            continue;
        }

        TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);

        if (!talentTabInfo)
        {
            m_talents[m_activeSpec].erase(iter++);
            continue;
        }

        // unlearn only talents for character class
        // some spell learned by one class as normal spells or know at creation but another class learn it as talent,
        // to prevent unexpected lost normal learned spell skip another class talents
        if ((getClassMask() & talentTabInfo->ClassMask) == 0)
        {
            ++iter;
            continue;
        }

        for (int j = 0; j < MAX_TALENT_RANK; ++j)
            if (talentInfo->RankID[j])
                removeSpell(talentInfo->RankID[j],!IsPassiveSpell(talentInfo->RankID[j]),false);

        iter = m_talents[m_activeSpec].begin();
    }

    // for not current spec just mark removed all saved to DB case and drop not saved
    if (all_specs)
    {
        for (uint8 spec = 0; spec < MAX_TALENT_SPEC_COUNT; ++spec)
        {
            if (spec == m_activeSpec)
                continue;

            for (PlayerTalentMap::iterator iter = m_talents[spec].begin(); iter != m_talents[spec].end();)
            {
                switch (iter->second.state)
                {
                case PLAYERSPELL_REMOVED:
                    ++iter;
                    break;
                case PLAYERSPELL_NEW:
                    m_talents[spec].erase(iter++);
                    break;
                default:
                    iter->second.state = PLAYERSPELL_REMOVED;
                    ++iter;
                    break;
                }
            }
        }
    }

    UpdateFreeTalentPoints(false);

    if(!no_cost)
    {
        ModifyMoney(-(int32)cost);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS, cost);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS, 1);

        m_resetTalentsCost = cost;
        m_resetTalentsTime = time(NULL);
    }

    //FIXME: remove pet before or after unlearn spells? for now after unlearn to allow removing of talent related, pet affecting auras
    RemovePet(PET_SAVE_REAGENTS);
    /* when prev line will dropped use next line
    if(Pet* pet = GetPet())
    {
        if(pet->getPetType()==HUNTER_PET && !pet->GetCreatureInfo()->isTameable(CanTameExoticPets()))
            pet->Unsummon(PET_SAVE_REAGENTS, this);
    }
    */
    return true;
}

Mail* Player::GetMail(uint32 id)
{
    for(PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        if ((*itr)->messageID == id)
        {
            return (*itr);
        }
    }
    return NULL;
}

void Player::_SetCreateBits(UpdateMask *updateMask, Player *target) const
{
    if(target == this)
    {
        Object::_SetCreateBits(updateMask, target);
    }
    else
    {
        for(uint16 index = 0; index < m_valuesCount; index++)
        {
            if(GetUInt32Value(index) != 0 && updateVisualBits.GetBit(index))
                updateMask->SetBit(index);
        }
    }
}

void Player::_SetUpdateBits(UpdateMask *updateMask, Player *target) const
{
    if(target == this)
    {
        Object::_SetUpdateBits(updateMask, target);
    }
    else
    {
        Object::_SetUpdateBits(updateMask, target);
        *updateMask &= updateVisualBits;
    }
}

void Player::InitVisibleBits()
{
    updateVisualBits.SetCount(PLAYER_END);

    updateVisualBits.SetBit(OBJECT_FIELD_GUID);
    updateVisualBits.SetBit(OBJECT_FIELD_TYPE);
    updateVisualBits.SetBit(OBJECT_FIELD_ENTRY);
    updateVisualBits.SetBit(OBJECT_FIELD_SCALE_X);
    updateVisualBits.SetBit(UNIT_FIELD_CHARM + 0);
    updateVisualBits.SetBit(UNIT_FIELD_CHARM + 1);
    updateVisualBits.SetBit(UNIT_FIELD_SUMMON + 0);
    updateVisualBits.SetBit(UNIT_FIELD_SUMMON + 1);
    updateVisualBits.SetBit(UNIT_FIELD_CHARMEDBY + 0);
    updateVisualBits.SetBit(UNIT_FIELD_CHARMEDBY + 1);
    updateVisualBits.SetBit(UNIT_FIELD_TARGET + 0);
    updateVisualBits.SetBit(UNIT_FIELD_TARGET + 1);
    updateVisualBits.SetBit(UNIT_FIELD_CHANNEL_OBJECT + 0);
    updateVisualBits.SetBit(UNIT_FIELD_CHANNEL_OBJECT + 1);
    updateVisualBits.SetBit(UNIT_FIELD_BYTES_0);
    updateVisualBits.SetBit(UNIT_FIELD_HEALTH);
    updateVisualBits.SetBit(UNIT_FIELD_POWER1);
    updateVisualBits.SetBit(UNIT_FIELD_POWER2);
    updateVisualBits.SetBit(UNIT_FIELD_POWER3);
    updateVisualBits.SetBit(UNIT_FIELD_POWER4);
    updateVisualBits.SetBit(UNIT_FIELD_POWER5);
    updateVisualBits.SetBit(UNIT_FIELD_POWER6);
    updateVisualBits.SetBit(UNIT_FIELD_POWER7);
    updateVisualBits.SetBit(UNIT_FIELD_MAXHEALTH);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER1);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER2);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER3);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER4);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER5);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER6);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER7);
    updateVisualBits.SetBit(UNIT_FIELD_LEVEL);
    updateVisualBits.SetBit(UNIT_FIELD_FACTIONTEMPLATE);
    updateVisualBits.SetBit(UNIT_VIRTUAL_ITEM_SLOT_ID + 0);
    updateVisualBits.SetBit(UNIT_VIRTUAL_ITEM_SLOT_ID + 1);
    updateVisualBits.SetBit(UNIT_VIRTUAL_ITEM_SLOT_ID + 2);
    updateVisualBits.SetBit(UNIT_FIELD_FLAGS);
    updateVisualBits.SetBit(UNIT_FIELD_FLAGS_2);
    updateVisualBits.SetBit(UNIT_FIELD_AURASTATE);
    updateVisualBits.SetBit(UNIT_FIELD_BASEATTACKTIME + 0);
    updateVisualBits.SetBit(UNIT_FIELD_BASEATTACKTIME + 1);
    updateVisualBits.SetBit(UNIT_FIELD_BOUNDINGRADIUS);
    updateVisualBits.SetBit(UNIT_FIELD_COMBATREACH);
    updateVisualBits.SetBit(UNIT_FIELD_DISPLAYID);
    updateVisualBits.SetBit(UNIT_FIELD_NATIVEDISPLAYID);
    updateVisualBits.SetBit(UNIT_FIELD_MOUNTDISPLAYID);
    updateVisualBits.SetBit(UNIT_FIELD_BYTES_1);
    updateVisualBits.SetBit(UNIT_FIELD_PETNUMBER);
    updateVisualBits.SetBit(UNIT_FIELD_PET_NAME_TIMESTAMP);
    updateVisualBits.SetBit(UNIT_DYNAMIC_FLAGS);
    updateVisualBits.SetBit(UNIT_CHANNEL_SPELL);
    updateVisualBits.SetBit(UNIT_MOD_CAST_SPEED);
    updateVisualBits.SetBit(UNIT_FIELD_BASE_MANA);
    updateVisualBits.SetBit(UNIT_FIELD_BYTES_2);
    updateVisualBits.SetBit(UNIT_FIELD_HOVERHEIGHT);

    updateVisualBits.SetBit(PLAYER_DUEL_ARBITER + 0);
    updateVisualBits.SetBit(PLAYER_DUEL_ARBITER + 1);
    updateVisualBits.SetBit(PLAYER_FLAGS);
    updateVisualBits.SetBit(PLAYER_GUILDID);
    updateVisualBits.SetBit(PLAYER_GUILDRANK);
    updateVisualBits.SetBit(PLAYER_BYTES);
    updateVisualBits.SetBit(PLAYER_BYTES_2);
    updateVisualBits.SetBit(PLAYER_BYTES_3);
    updateVisualBits.SetBit(PLAYER_DUEL_TEAM);
    updateVisualBits.SetBit(PLAYER_GUILD_TIMESTAMP);

    // PLAYER_QUEST_LOG_x also visible bit on official (but only on party/raid)...
    for(uint16 i = PLAYER_QUEST_LOG_1_1; i < PLAYER_QUEST_LOG_25_2; i += MAX_QUEST_OFFSET)
        updateVisualBits.SetBit(i);

    // Players visible items are not inventory stuff
    for(uint16 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        uint32 offset = i * 2;

        // item entry
        updateVisualBits.SetBit(PLAYER_VISIBLE_ITEM_1_ENTRYID + offset);
        // enchant
        updateVisualBits.SetBit(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + offset);
    }

    updateVisualBits.SetBit(PLAYER_CHOSEN_TITLE);
}

void Player::BuildCreateUpdateBlockForPlayer( UpdateData *data, Player *target ) const
{
    if(target == this)
    {
        for(int i = 0; i < EQUIPMENT_SLOT_END; ++i)
        {
            if(m_items[i] == NULL)
                continue;

            m_items[i]->BuildCreateUpdateBlockForPlayer( data, target );
        }
        for(int i = INVENTORY_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if(m_items[i] == NULL)
                continue;

            m_items[i]->BuildCreateUpdateBlockForPlayer( data, target );
        }
        for(int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
        {
            if(m_items[i] == NULL)
                continue;

            m_items[i]->BuildCreateUpdateBlockForPlayer( data, target );
        }
    }

    Unit::BuildCreateUpdateBlockForPlayer( data, target );
}

void Player::DestroyForPlayer( Player *target, bool anim ) const
{
    Unit::DestroyForPlayer( target, anim );

    for(int i = 0; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if(m_items[i] == NULL)
            continue;

        m_items[i]->DestroyForPlayer( target );
    }

    if(target == this)
    {
        for(int i = INVENTORY_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if(m_items[i] == NULL)
                continue;

            m_items[i]->DestroyForPlayer( target );
        }
        for(int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
        {
            if(m_items[i] == NULL)
                continue;

            m_items[i]->DestroyForPlayer( target );
        }
    }
}

bool Player::HasSpell(uint32 spell) const
{
    PlayerSpellMap::const_iterator itr = m_spells.find(spell);
    return (itr != m_spells.end() && itr->second.state != PLAYERSPELL_REMOVED &&
        !itr->second.disabled);
}

bool Player::HasActiveSpell(uint32 spell) const
{
    PlayerSpellMap::const_iterator itr = m_spells.find(spell);
    return (itr != m_spells.end() && itr->second.state != PLAYERSPELL_REMOVED &&
        itr->second.active && !itr->second.disabled);
}

TrainerSpellState Player::GetTrainerSpellState(TrainerSpell const* trainer_spell, uint32 reqLevel) const
{
    if (!trainer_spell)
        return TRAINER_SPELL_RED;

    if (!trainer_spell->learnedSpell)
        return TRAINER_SPELL_RED;

    // known spell
    if (HasSpell(trainer_spell->learnedSpell))
        return TRAINER_SPELL_GRAY;

    // check race/class requirement
    if (!IsSpellFitByClassAndRace(trainer_spell->learnedSpell))
        return TRAINER_SPELL_RED;

    bool prof = SpellMgr::IsProfessionSpell(trainer_spell->learnedSpell);

    // check level requirement
    if (!prof || GetSession()->GetSecurity() < AccountTypes(sWorld.getConfig(CONFIG_UINT32_TRADE_SKILL_GMIGNORE_LEVEL)))
        if (getLevel() < reqLevel)
            return TRAINER_SPELL_RED;

    if(SpellChainNode const* spell_chain = sSpellMgr.GetSpellChainNode(trainer_spell->learnedSpell))
    {
        // check prev.rank requirement
        if(spell_chain->prev && !HasSpell(spell_chain->prev))
            return TRAINER_SPELL_RED;

        // check additional spell requirement
        if(spell_chain->req && !HasSpell(spell_chain->req))
            return TRAINER_SPELL_RED;
    }

    // check skill requirement
    if (!prof || GetSession()->GetSecurity() < AccountTypes(sWorld.getConfig(CONFIG_UINT32_TRADE_SKILL_GMIGNORE_SKILL)))
        if (trainer_spell->reqSkill && GetBaseSkillValue(trainer_spell->reqSkill) < trainer_spell->reqSkillValue)
            return TRAINER_SPELL_RED;

    // exist, already checked at loading
    SpellEntry const* spell = sSpellStore.LookupEntry(trainer_spell->learnedSpell);

    // secondary prof. or not prof. spell
    uint32 skill = spell->EffectMiscValue[1];

    if(spell->Effect[1] != SPELL_EFFECT_SKILL || !IsPrimaryProfessionSkill(skill))
        return TRAINER_SPELL_GREEN;

    // check primary prof. limit
    if(sSpellMgr.IsPrimaryProfessionFirstRankSpell(spell->Id) && GetFreePrimaryProfessionPoints() == 0)
        return TRAINER_SPELL_GREEN_DISABLED;

    return TRAINER_SPELL_GREEN;
}

/**
 * Deletes a character from the database
 *
 * The way, how the characters will be deleted is decided based on the config option.
 *
 * @see Player::DeleteOldCharacters
 *
 * @param playerguid       the low-GUID from the player which should be deleted
 * @param accountId        the account id from the player
 * @param updateRealmChars when this flag is set, the amount of characters on that realm will be updated in the realmlist
 * @param deleteFinally    if this flag is set, the config option will be ignored and the character will be permanently removed from the database
 */
void Player::DeleteFromDB(ObjectGuid playerguid, uint32 accountId, bool updateRealmChars, bool deleteFinally)
{
    // for nonexistent account avoid update realm
    if (accountId == 0)
        updateRealmChars = false;

    uint32 charDelete_method = sWorld.getConfig(CONFIG_UINT32_CHARDELETE_METHOD);
    uint32 charDelete_minLvl = sWorld.getConfig(CONFIG_UINT32_CHARDELETE_MIN_LEVEL);

    // if we want to finally delete the character or the character does not meet the level requirement, we set it to mode 0
    if (deleteFinally || Player::GetLevelFromDB(playerguid) < charDelete_minLvl)
        charDelete_method = 0;

    uint32 lowguid = playerguid.GetCounter();

    // convert corpse to bones if exist (to prevent exiting Corpse in World without DB entry)
    // bones will be deleted by corpse/bones deleting thread shortly
    sObjectAccessor.ConvertCorpseForPlayer(playerguid);

    // remove from guild
    if (uint32 guildId = GetGuildIdFromDB(playerguid))
    {
        if (Guild* guild = sGuildMgr.GetGuildById(guildId))
        {
            if (guild->DelMember(playerguid))
            {
                guild->Disband();
                delete guild;
            }
        }
    }

    // remove from arena teams
    LeaveAllArenaTeams(playerguid);

    // the player was uninvited already on logout so just remove from group
    QueryResult *resultGroup = CharacterDatabase.PQuery("SELECT groupId FROM group_member WHERE memberGuid='%u'", lowguid);
    if (resultGroup)
    {
        uint32 groupId = (*resultGroup)[0].GetUInt32();
        delete resultGroup;
        if (Group* group = sObjectMgr.GetGroupById(groupId))
            RemoveFromGroup(group, playerguid);
    }

    // remove signs from petitions (also remove petitions if owner);
    RemovePetitionsAndSigns(playerguid, 10);

    switch(charDelete_method)
    {
        // completely remove from the database
        case 0:
        {
            // return back all mails with COD and Item                 0  1           2              3      4       5    6     7
            QueryResult *resultMail = CharacterDatabase.PQuery("SELECT id,messageType,mailTemplateId,sender,subject,body,money,has_items FROM mail WHERE receiver='%u' AND has_items<>0 AND cod<>0", lowguid);
            if (resultMail)
            {
                do
                {
                    Field *fields = resultMail->Fetch();

                    uint32 mail_id       = fields[0].GetUInt32();
                    uint16 mailType      = fields[1].GetUInt16();
                    uint16 mailTemplateId= fields[2].GetUInt16();
                    uint32 sender        = fields[3].GetUInt32();
                    std::string subject  = fields[4].GetCppString();
                    std::string body     = fields[5].GetCppString();
                    uint32 money         = fields[6].GetUInt32();
                    bool has_items       = fields[7].GetBool();

                    //we can return mail now
                    //so firstly delete the old one
                    CharacterDatabase.PExecute("DELETE FROM mail WHERE id = '%u'", mail_id);

                    // mail not from player
                    if (mailType != MAIL_NORMAL)
                    {
                        if(has_items)
                            CharacterDatabase.PExecute("DELETE FROM mail_items WHERE mail_id = '%u'", mail_id);
                        continue;
                    }

                    MailDraft draft;
                    if (mailTemplateId)
                        draft.SetMailTemplate(mailTemplateId, false);// items already included
                    else
                        draft.SetSubjectAndBody(subject, body);

                    if (has_items)
                    {
                        // data needs to be at first place for Item::LoadFromDB
                        //                                                          0    1    2         3
                        QueryResult *resultItems = CharacterDatabase.PQuery("SELECT data,text,item_guid,item_template FROM mail_items JOIN item_instance ON item_guid = guid WHERE mail_id='%u'", mail_id);
                        if (resultItems)
                        {
                            do
                            {
                                Field *fields2 = resultItems->Fetch();

                                uint32 item_guidlow = fields2[2].GetUInt32();
                                uint32 item_template = fields2[3].GetUInt32();

                                ItemPrototype const* itemProto = ObjectMgr::GetItemPrototype(item_template);
                                if (!itemProto)
                                {
                                    CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid = '%u'", item_guidlow);
                                    continue;
                                }

                                Item *pItem = NewItemOrBag(itemProto);
                                if (!pItem->LoadFromDB(item_guidlow, fields2, playerguid))
                                {
                                    pItem->FSetState(ITEM_REMOVED);
                                    pItem->SaveToDB();              // it also deletes item object !
                                    continue;
                                }

                                draft.AddItem(pItem);
                            }
                            while (resultItems->NextRow());

                            delete resultItems;
                        }
                    }

                    CharacterDatabase.PExecute("DELETE FROM mail_items WHERE mail_id = '%u'", mail_id);

                    uint32 pl_account = sObjectMgr.GetPlayerAccountIdByGUID(playerguid);

                    draft.SetMoney(money).SendReturnToSender(pl_account, playerguid, ObjectGuid(HIGHGUID_PLAYER, sender));
                }
                while (resultMail->NextRow());

                delete resultMail;
            }

            // unsummon and delete for pets in world is not required: player deleted from CLI or character list with not loaded pet.
            // Get guids of character's pets, will deleted in transaction
            QueryResult *resultPets = CharacterDatabase.PQuery("SELECT id FROM character_pet WHERE owner = '%u'", lowguid);

            // delete char from friends list when selected chars is online (non existing - error)
            QueryResult *resultFriend = CharacterDatabase.PQuery("SELECT DISTINCT guid FROM character_social WHERE friend = '%u'", lowguid);

            // NOW we can finally clear other DB data related to character
            CharacterDatabase.BeginTransaction();
            if (resultPets)
            {
                do
                {
                    Field *fields3 = resultPets->Fetch();
                    uint32 petguidlow = fields3[0].GetUInt32();
                    //do not create separate transaction for pet delete otherwise we will get fatal error!
                    Pet::DeleteFromDB(petguidlow, false);
                } while (resultPets->NextRow());
                delete resultPets;
            }

            // cleanup friends for online players, offline case will cleanup later in code
            if (resultFriend)
            {
                do
                {
                    Field* fieldsFriend = resultFriend->Fetch();
                    if (Player* sFriend = sObjectAccessor.FindPlayer(ObjectGuid(HIGHGUID_PLAYER, fieldsFriend[0].GetUInt32())))
                    {
                        if (sFriend->IsInWorld())
                        {
                            sFriend->GetSocial()->RemoveFromSocialList(playerguid, false);
                            sSocialMgr.SendFriendStatus(sFriend, FRIEND_REMOVED, playerguid, false);
                        }
                    }
                } while (resultFriend->NextRow());
                delete resultFriend;
            }

            CharacterDatabase.PExecute("DELETE FROM characters WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_account_data WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_declinedname WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_action WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_aura WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_battleground_data WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_gifts WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_glyphs WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_homebind WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_instance WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM group_instance WHERE leaderGuid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_queststatus WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_queststatus_daily WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_queststatus_weekly WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_reputation WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_skills WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_spell WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_spell_cooldown WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_talent WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_ticket WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM item_instance WHERE owner_guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_social WHERE guid = '%u' OR friend='%u'", lowguid, lowguid);
            CharacterDatabase.PExecute("DELETE FROM mail WHERE receiver = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM mail_items WHERE receiver = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_pet WHERE owner = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_pet_declinedname WHERE owner = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_achievement WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_achievement_progress WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM character_equipmentsets WHERE guid = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM guild_eventlog WHERE PlayerGuid1 = '%u' OR PlayerGuid2 = '%u'", lowguid, lowguid);
            CharacterDatabase.PExecute("DELETE FROM guild_bank_eventlog WHERE PlayerGuid = '%u'", lowguid);
            CharacterDatabase.CommitTransaction();
            break;
        }
        // The character gets unlinked from the account, the name gets freed up and appears as deleted ingame
        case 1:
            CharacterDatabase.PExecute("UPDATE characters SET deleteInfos_Name=name, deleteInfos_Account=account, deleteDate='" UI64FMTD "', name='', account=0 WHERE guid=%u", uint64(time(NULL)), lowguid);
            break;
        default:
            sLog.outError("Player::DeleteFromDB: Unsupported delete method: %u.", charDelete_method);
    }

    if (updateRealmChars)
        sWorld.UpdateRealmCharCount(accountId);
}

/**
 * Characters which were kept back in the database after being deleted and are now too old (see config option "CharDelete.KeepDays"), will be completely deleted.
 *
 * @see Player::DeleteFromDB
 */
void Player::DeleteOldCharacters()
{
    uint32 keepDays = sWorld.getConfig(CONFIG_UINT32_CHARDELETE_KEEP_DAYS);
    if (!keepDays)
        return;

    Player::DeleteOldCharacters(keepDays);
}

/**
 * Characters which were kept back in the database after being deleted and are older than the specified amount of days, will be completely deleted.
 *
 * @see Player::DeleteFromDB
 *
 * @param keepDays overrite the config option by another amount of days
 */
void Player::DeleteOldCharacters(uint32 keepDays)
{
    sLog.outString("Player::DeleteOldChars: Deleting all characters which have been deleted %u days before...", keepDays);

    QueryResult *resultChars = CharacterDatabase.PQuery("SELECT guid, deleteInfos_Account FROM characters WHERE deleteDate IS NOT NULL AND deleteDate < '" UI64FMTD "'", uint64(time(NULL) - time_t(keepDays * DAY)));
    if (resultChars)
    {
        sLog.outString("Player::DeleteOldChars: Found %u character(s) to delete",uint32(resultChars->GetRowCount()));
        do
        {
            Field *charFields = resultChars->Fetch();
            ObjectGuid guid = ObjectGuid(HIGHGUID_PLAYER, charFields[0].GetUInt32());
            Player::DeleteFromDB(guid, charFields[1].GetUInt32(), true, true);
        } while(resultChars->NextRow());
        delete resultChars;
    }
}

void Player::SetMovement(PlayerMovementType pType)
{
    WorldPacket data;
    switch(pType)
    {
        case MOVE_ROOT:       data.Initialize(SMSG_FORCE_MOVE_ROOT,   GetPackGUID().size()+4); break;
        case MOVE_UNROOT:     data.Initialize(SMSG_FORCE_MOVE_UNROOT, GetPackGUID().size()+4); break;
        case MOVE_WATER_WALK: data.Initialize(SMSG_MOVE_WATER_WALK,   GetPackGUID().size()+4); break;
        case MOVE_LAND_WALK:  data.Initialize(SMSG_MOVE_LAND_WALK,    GetPackGUID().size()+4); break;
        default:
            sLog.outError("Player::SetMovement: Unsupported move type (%d), data not sent to client.",pType);
            return;
    }
    data << GetPackGUID();
    data << uint32(0);
    GetSession()->SendPacket( &data );
}

/* Preconditions:
  - a resurrectable corpse must not be loaded for the player (only bones)
  - the player must be in world
*/
void Player::BuildPlayerRepop()
{
    WorldPacket data(SMSG_PRE_RESURRECT, GetPackGUID().size());
    data << GetPackGUID();
    GetSession()->SendPacket(&data);

    if(getRace() == RACE_NIGHTELF)
        CastSpell(this, 20584, true);                       // auras SPELL_AURA_INCREASE_SPEED(+speed in wisp form), SPELL_AURA_INCREASE_SWIM_SPEED(+swim speed in wisp form), SPELL_AURA_TRANSFORM (to wisp form)
    CastSpell(this, 8326, true);                            // auras SPELL_AURA_GHOST, SPELL_AURA_INCREASE_SPEED(why?), SPELL_AURA_INCREASE_SWIM_SPEED(why?)

    // there must be SMSG.FORCE_RUN_SPEED_CHANGE, SMSG.FORCE_SWIM_SPEED_CHANGE, SMSG.MOVE_WATER_WALK
    // there must be SMSG.STOP_MIRROR_TIMER
    // there we must send 888 opcode

    // the player cannot have a corpse already, only bones which are not returned by GetCorpse
    if(GetCorpse())
    {
        sLog.outError("BuildPlayerRepop: player %s(%d) already has a corpse", GetName(), GetGUIDLow());
        MANGOS_ASSERT(false);
    }

    // create a corpse and place it at the player's location
    Corpse *corpse = CreateCorpse();
    if(!corpse)
    {
        sLog.outError("Error creating corpse for Player %s [%u]", GetName(), GetGUIDLow());
        return;
    }
    GetMap()->Add(corpse);

    // convert player body to ghost
    SetHealth( 1 );

    SetMovement(MOVE_WATER_WALK);
    if(!GetSession()->isLogingOut())
        SetMovement(MOVE_UNROOT);

    // BG - remove insignia related
    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

    SendCorpseReclaimDelay();

    // to prevent cheating
    corpse->ResetGhostTime();

    StopMirrorTimers();                                     //disable timers(bars)

    // set and clear other
    SetByteValue(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND);
}

void Player::ResurrectPlayer(float restore_percent, bool applySickness)
{
    WorldPacket data(SMSG_DEATH_RELEASE_LOC, 4*4);          // remove spirit healer position
    data << uint32(-1);
    data << float(0);
    data << float(0);
    data << float(0);
    GetSession()->SendPacket(&data);

    // speed change, land walk

    // remove death flag + set aura
    SetByteValue(UNIT_FIELD_BYTES_1, 3, 0x00);

    SetDeathState(ALIVE);

    if(getRace() == RACE_NIGHTELF)
        RemoveAurasDueToSpell(20584);                       // speed bonuses
    RemoveAurasDueToSpell(8326);                            // SPELL_AURA_GHOST

    SetMovement(MOVE_LAND_WALK);
    SetMovement(MOVE_UNROOT);

    m_deathTimer = 0;

    // set health/powers (0- will be set in caller)
    if(restore_percent>0.0f)
    {
        SetHealth(uint32(GetMaxHealth()*restore_percent));
        SetPower(POWER_MANA, uint32(GetMaxPower(POWER_MANA)*restore_percent));
        SetPower(POWER_RAGE, 0);
        SetPower(POWER_ENERGY, uint32(GetMaxPower(POWER_ENERGY)*restore_percent));
    }

    // trigger update zone for alive state zone updates
    uint32 newzone, newarea;
    GetZoneAndAreaId(newzone,newarea);
    UpdateZone(newzone,newarea);

    // update visibility of world around viewpoint
    m_camera.UpdateVisibilityForOwner();
    // update visibility of player for nearby cameras
    UpdateObjectVisibility();

    if(!applySickness)
        return;

    //Characters from level 1-10 are not affected by resurrection sickness.
    //Characters from level 11-19 will suffer from one minute of sickness
    //for each level they are above 10.
    //Characters level 20 and up suffer from ten minutes of sickness.
    int32 startLevel = sWorld.getConfig(CONFIG_INT32_DEATH_SICKNESS_LEVEL);

    if(int32(getLevel()) >= startLevel)
    {
        // set resurrection sickness
        CastSpell(this,SPELL_ID_PASSIVE_RESURRECTION_SICKNESS,true);

        // not full duration
        if(int32(getLevel()) < startLevel+9)
        {
            int32 delta = (int32(getLevel()) - startLevel + 1)*MINUTE;

            if (SpellAuraHolder* holder = GetSpellAuraHolder(SPELL_ID_PASSIVE_RESURRECTION_SICKNESS))
            {
                holder->SetAuraDuration(delta*IN_MILLISECONDS);
                holder->SendAuraUpdate(false);
            }
        }
    }
}

void Player::KillPlayer()
{
    SetMovement(MOVE_ROOT);

    StopMirrorTimers();                                     //disable timers(bars)

    SetDeathState(CORPSE);
    //SetFlag( UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_IN_PVP );

    SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_NONE);
    ApplyModByteFlag(PLAYER_FIELD_BYTES, 0, PLAYER_FIELD_BYTE_RELEASE_TIMER, !sMapStore.LookupEntry(GetMapId())->Instanceable());

    // 6 minutes until repop at graveyard
    m_deathTimer = 6*MINUTE*IN_MILLISECONDS;

    UpdateCorpseReclaimDelay();                             // dependent at use SetDeathPvP() call before kill

    // don't create corpse at this moment, player might be falling

    // update visibility
    UpdateObjectVisibility();
}

Corpse* Player::CreateCorpse()
{
    // prevent existence 2 corpse for player
    SpawnCorpseBones();

    Corpse *corpse = new Corpse( (m_ExtraFlags & PLAYER_EXTRA_PVP_DEATH) ? CORPSE_RESURRECTABLE_PVP : CORPSE_RESURRECTABLE_PVE );
    SetPvPDeath(false);

    if (!corpse->Create(sObjectMgr.GenerateCorpseLowGuid(), this))
    {
        delete corpse;
        return NULL;
    }

    uint8 skin       = GetByteValue(PLAYER_BYTES, 0);
    uint8 face       = GetByteValue(PLAYER_BYTES, 1);
    uint8 hairstyle  = GetByteValue(PLAYER_BYTES, 2);
    uint8 haircolor  = GetByteValue(PLAYER_BYTES, 3);
    uint8 facialhair = GetByteValue(PLAYER_BYTES_2, 0);

    corpse->SetByteValue(CORPSE_FIELD_BYTES_1, 1, getRace());
    corpse->SetByteValue(CORPSE_FIELD_BYTES_1, 2, getGender());
    corpse->SetByteValue(CORPSE_FIELD_BYTES_1, 3, skin);

    corpse->SetByteValue(CORPSE_FIELD_BYTES_2, 0, face);
    corpse->SetByteValue(CORPSE_FIELD_BYTES_2, 1, hairstyle);
    corpse->SetByteValue(CORPSE_FIELD_BYTES_2, 2, haircolor);
    corpse->SetByteValue(CORPSE_FIELD_BYTES_2, 3, facialhair);

    uint32 flags = CORPSE_FLAG_UNK2;
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_HELM))
        flags |= CORPSE_FLAG_HIDE_HELM;
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_CLOAK))
        flags |= CORPSE_FLAG_HIDE_CLOAK;
    if (InBattleGround() && !InArena())
        flags |= CORPSE_FLAG_LOOTABLE;                      // to be able to remove insignia
    corpse->SetUInt32Value( CORPSE_FIELD_FLAGS, flags );

    corpse->SetUInt32Value( CORPSE_FIELD_DISPLAY_ID, GetNativeDisplayId() );

    corpse->SetUInt32Value( CORPSE_FIELD_GUILD, GetGuildId() );

    uint32 iDisplayID;
    uint32 iIventoryType;
    uint32 _cfi;
    for (int i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (m_items[i])
        {
            iDisplayID = m_items[i]->GetProto()->DisplayInfoID;
            iIventoryType = m_items[i]->GetProto()->InventoryType;

            _cfi =  iDisplayID | (iIventoryType << 24);
            corpse->SetUInt32Value(CORPSE_FIELD_ITEM + i, _cfi);
        }
    }

    // we not need saved corpses for BG/arenas
    if (!GetMap()->IsBattleGroundOrArena())
        corpse->SaveToDB();

    // register for player, but not show
    sObjectAccessor.AddCorpse(corpse);
    return corpse;
}

void Player::SpawnCorpseBones()
{
    if(sObjectAccessor.ConvertCorpseForPlayer(GetObjectGuid()))
        if (!GetSession()->PlayerLogoutWithSave())          // at logout we will already store the player
            SaveToDB();                                     // prevent loading as ghost without corpse
}

Corpse* Player::GetCorpse() const
{
    return sObjectAccessor.GetCorpseForPlayerGUID(GetObjectGuid());
}

void Player::DurabilityLossAll(double percent, bool inventory)
{
    for(int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
        if(Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            DurabilityLoss(pItem,percent);

    if(inventory)
    {
        // bags not have durability
        // for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)

        for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
            if(Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
                DurabilityLoss(pItem,percent);

        // keys not have durability
        //for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; ++i)

        for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
            if(Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
                for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if(Item* pItem = GetItemByPos( i, j ))
                        DurabilityLoss(pItem,percent);
    }
}

void Player::DurabilityLoss(Item* item, double percent)
{
    if(!item )
        return;

    uint32 pMaxDurability =  item ->GetUInt32Value(ITEM_FIELD_MAXDURABILITY);

    if(!pMaxDurability)
        return;

    uint32 pDurabilityLoss = uint32(pMaxDurability*percent);

    if(pDurabilityLoss < 1 )
        pDurabilityLoss = 1;

    DurabilityPointsLoss(item,pDurabilityLoss);
}

void Player::DurabilityPointsLossAll(int32 points, bool inventory)
{
    for(int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
        if(Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            DurabilityPointsLoss(pItem,points);

    if(inventory)
    {
        // bags not have durability
        // for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)

        for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
            if(Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
                DurabilityPointsLoss(pItem,points);

        // keys not have durability
        //for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; ++i)

        for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
            if(Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
                for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if(Item* pItem = GetItemByPos( i, j ))
                        DurabilityPointsLoss(pItem,points);
    }
}

void Player::DurabilityPointsLoss(Item* item, int32 points)
{
    int32 pMaxDurability = item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY);
    int32 pOldDurability = item->GetUInt32Value(ITEM_FIELD_DURABILITY);
    int32 pNewDurability = pOldDurability - points;

    if (pNewDurability < 0)
        pNewDurability = 0;
    else if (pNewDurability > pMaxDurability)
        pNewDurability = pMaxDurability;

    if (pOldDurability != pNewDurability)
    {
        // modify item stats _before_ Durability set to 0 to pass _ApplyItemMods internal check
        if ( pNewDurability == 0 && pOldDurability > 0 && item->IsEquipped())
            _ApplyItemMods(item,item->GetSlot(), false);

        item->SetUInt32Value(ITEM_FIELD_DURABILITY, pNewDurability);

        // modify item stats _after_ restore durability to pass _ApplyItemMods internal check
        if ( pNewDurability > 0 && pOldDurability == 0 && item->IsEquipped())
            _ApplyItemMods(item,item->GetSlot(), true);

        item->SetState(ITEM_CHANGED, this);
    }
}

void Player::DurabilityPointLossForEquipSlot(EquipmentSlots slot)
{
    if(Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, slot ))
        DurabilityPointsLoss(pItem,1);
}

uint32 Player::DurabilityRepairAll(bool cost, float discountMod, bool guildBank)
{
    uint32 TotalCost = 0;
    // equipped, backpack, bags itself
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        TotalCost += DurabilityRepair(( (INVENTORY_SLOT_BAG_0 << 8) | i ),cost,discountMod, guildBank);

    // bank, buyback and keys not repaired

    // items in inventory bags
    for(int j = INVENTORY_SLOT_BAG_START; j < INVENTORY_SLOT_BAG_END; ++j)
        for(int i = 0; i < MAX_BAG_SIZE; ++i)
            TotalCost += DurabilityRepair(( (j << 8) | i ),cost,discountMod, guildBank);
    return TotalCost;
}

uint32 Player::DurabilityRepair(uint16 pos, bool cost, float discountMod, bool guildBank)
{
    Item* item = GetItemByPos(pos);

    uint32 TotalCost = 0;
    if(!item)
        return TotalCost;

    uint32 maxDurability = item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY);
    if(!maxDurability)
        return TotalCost;

    uint32 curDurability = item->GetUInt32Value(ITEM_FIELD_DURABILITY);

    if(cost)
    {
        uint32 LostDurability = maxDurability - curDurability;
        if(LostDurability>0)
        {
            ItemPrototype const *ditemProto = item->GetProto();

            DurabilityCostsEntry const *dcost = sDurabilityCostsStore.LookupEntry(ditemProto->ItemLevel);
            if(!dcost)
            {
                sLog.outError("RepairDurability: Wrong item lvl %u", ditemProto->ItemLevel);
                return TotalCost;
            }

            uint32 dQualitymodEntryId = (ditemProto->Quality+1)*2;
            DurabilityQualityEntry const *dQualitymodEntry = sDurabilityQualityStore.LookupEntry(dQualitymodEntryId);
            if(!dQualitymodEntry)
            {
                sLog.outError("RepairDurability: Wrong dQualityModEntry %u", dQualitymodEntryId);
                return TotalCost;
            }

            uint32 dmultiplier = dcost->multiplier[ItemSubClassToDurabilityMultiplierId(ditemProto->Class,ditemProto->SubClass)];
            uint32 costs = uint32(LostDurability*dmultiplier*double(dQualitymodEntry->quality_mod));

            costs = uint32(costs * discountMod);

            if (costs==0)                                   //fix for ITEM_QUALITY_ARTIFACT
                costs = 1;

            if (guildBank)
            {
                if (GetGuildId()==0)
                {
                    DEBUG_LOG("You are not member of a guild");
                    return TotalCost;
                }

                Guild* pGuild = sGuildMgr.GetGuildById(GetGuildId());
                if (!pGuild)
                    return TotalCost;

                if (!pGuild->HasRankRight(GetRank(), GR_RIGHT_WITHDRAW_REPAIR))
                {
                    DEBUG_LOG("You do not have rights to withdraw for repairs");
                    return TotalCost;
                }

                if (pGuild->GetMemberMoneyWithdrawRem(GetGUIDLow()) < costs)
                {
                    DEBUG_LOG("You do not have enough money withdraw amount remaining");
                    return TotalCost;
                }

                if (pGuild->GetGuildBankMoney() < costs)
                {
                    DEBUG_LOG("There is not enough money in bank");
                    return TotalCost;
                }

                pGuild->MemberMoneyWithdraw(costs, GetGUIDLow());
                TotalCost = costs;
            }
            else if (GetMoney() < costs)
            {
                DEBUG_LOG("You do not have enough money");
                return TotalCost;
            }
            else
                ModifyMoney( -int32(costs) );
        }
    }

    item->SetUInt32Value(ITEM_FIELD_DURABILITY, maxDurability);
    item->SetState(ITEM_CHANGED, this);

    // reapply mods for total broken and repaired item if equipped
    if(IsEquipmentPos(pos) && !curDurability)
        _ApplyItemMods(item,pos & 255, true);
    return TotalCost;
}

void Player::RepopAtGraveyard()
{
    // note: this can be called also when the player is alive
    // for example from WorldSession::HandleMovementOpcodes

    AreaTableEntry const *zone = GetAreaEntryByAreaID(GetAreaId());

    // Such zones are considered unreachable as a ghost and the player must be automatically revived
    if ((!isAlive() && zone && zone->flags & AREA_FLAG_NEED_FLY) || GetTransport())
    {
        ResurrectPlayer(0.5f);
        SpawnCorpseBones();
    }

    WorldSafeLocsEntry const *ClosestGrave = NULL;

    // Special handle for battleground maps
    if( BattleGround *bg = GetBattleGround() )
        ClosestGrave = bg->GetClosestGraveYard(this);
    else
        ClosestGrave = sObjectMgr.GetClosestGraveYard( GetPositionX(), GetPositionY(), GetPositionZ(), GetMapId(), GetTeam() );

    // stop countdown until repop
    m_deathTimer = 0;

    // if no grave found, stay at the current location
    // and don't show spirit healer location
    if(ClosestGrave)
    {
        TeleportTo(ClosestGrave->map_id, ClosestGrave->x, ClosestGrave->y, ClosestGrave->z, GetOrientation());
        if(isDead())                                        // not send if alive, because it used in TeleportTo()
        {
            WorldPacket data(SMSG_DEATH_RELEASE_LOC, 4*4);  // show spirit healer position on minimap
            data << ClosestGrave->map_id;
            data << ClosestGrave->x;
            data << ClosestGrave->y;
            data << ClosestGrave->z;
            GetSession()->SendPacket(&data);
        }
    }
}

void Player::JoinedChannel(Channel *c)
{
    m_channels.push_back(c);
}

void Player::LeftChannel(Channel *c)
{
    m_channels.remove(c);
}

void Player::CleanupChannels()
{
    while (!m_channels.empty())
    {
        Channel* ch = *m_channels.begin();
        m_channels.erase(m_channels.begin());               // remove from player's channel list
        ch->Leave(GetObjectGuid(), false);                  // not send to client, not remove from player's channel list
        if (ChannelMgr* cMgr = channelMgr(GetTeam()))
            cMgr->LeftChannel(ch->GetName());               // deleted channel if empty

    }
    DEBUG_LOG("Player: channels cleaned up!");
}

void Player::UpdateLocalChannels(uint32 newZone )
{
    if(m_channels.empty())
        return;

    AreaTableEntry const* current_zone = GetAreaEntryByAreaID(newZone);
    if(!current_zone)
        return;

    ChannelMgr* cMgr = channelMgr(GetTeam());
    if(!cMgr)
        return;

    std::string current_zone_name = current_zone->area_name[GetSession()->GetSessionDbcLocale()];

    for(JoinedChannelsList::iterator i = m_channels.begin(), next; i != m_channels.end(); i = next)
    {
        next = i; ++next;

        // skip non built-in channels
        if(!(*i)->IsConstant())
            continue;

        ChatChannelsEntry const* ch = GetChannelEntryFor((*i)->GetChannelId());
        if(!ch)
            continue;

        if((ch->flags & 4) == 4)                            // global channel without zone name in pattern
            continue;

        //  new channel
        char new_channel_name_buf[100];
        snprintf(new_channel_name_buf,100,ch->pattern[m_session->GetSessionDbcLocale()],current_zone_name.c_str());
        Channel* new_channel = cMgr->GetJoinChannel(new_channel_name_buf,ch->ChannelID);

        if ((*i)!=new_channel)
        {
            new_channel->Join(GetObjectGuid(),"");          // will output Changed Channel: N. Name

            // leave old channel
            (*i)->Leave(GetObjectGuid(),false);             // not send leave channel, it already replaced at client
            std::string name = (*i)->GetName();             // store name, (*i)erase in LeftChannel
            LeftChannel(*i);                                // remove from player's channel list
            cMgr->LeftChannel(name);                        // delete if empty
        }
    }
    DEBUG_LOG("Player: channels cleaned up!");
}

void Player::LeaveLFGChannel()
{
    for(JoinedChannelsList::iterator i = m_channels.begin(); i != m_channels.end(); ++i )
    {
        if ((*i)->IsLFG())
        {
            (*i)->Leave(GetObjectGuid());
            break;
        }
    }
}

void Player::UpdateDefense()
{
    uint32 defense_skill_gain = sWorld.getConfig(CONFIG_UINT32_SKILL_GAIN_DEFENSE);

    if(UpdateSkill(SKILL_DEFENSE,defense_skill_gain))
    {
        // update dependent from defense skill part
        UpdateDefenseBonusesMod();
    }
}

void Player::HandleBaseModValue(BaseModGroup modGroup, BaseModType modType, float amount, bool apply)
{
    if(modGroup >= BASEMOD_END || modType >= MOD_END)
    {
        sLog.outError("ERROR in HandleBaseModValue(): nonexistent BaseModGroup of wrong BaseModType!");
        return;
    }

    float val = 1.0f;

    switch(modType)
    {
        case FLAT_MOD:
            m_auraBaseMod[modGroup][modType] += apply ? amount : -amount;
            break;
        case PCT_MOD:
            if(amount <= -100.0f)
                amount = -200.0f;

            val = (100.0f + amount) / 100.0f;
            m_auraBaseMod[modGroup][modType] *= apply ? val : (1.0f/val);
            break;
    }

    if(!CanModifyStats())
        return;

    switch(modGroup)
    {
        case CRIT_PERCENTAGE:              UpdateCritPercentage(BASE_ATTACK);                          break;
        case RANGED_CRIT_PERCENTAGE:       UpdateCritPercentage(RANGED_ATTACK);                        break;
        case OFFHAND_CRIT_PERCENTAGE:      UpdateCritPercentage(OFF_ATTACK);                           break;
        case SHIELD_BLOCK_VALUE:           UpdateShieldBlockValue();                                   break;
        default: break;
    }
}

float Player::GetBaseModValue(BaseModGroup modGroup, BaseModType modType) const
{
    if(modGroup >= BASEMOD_END || modType > MOD_END)
    {
        sLog.outError("trial to access nonexistent BaseModGroup or wrong BaseModType!");
        return 0.0f;
    }

    if(modType == PCT_MOD && m_auraBaseMod[modGroup][PCT_MOD] <= 0.0f)
        return 0.0f;

    return m_auraBaseMod[modGroup][modType];
}

float Player::GetTotalBaseModValue(BaseModGroup modGroup) const
{
    if(modGroup >= BASEMOD_END)
    {
        sLog.outError("wrong BaseModGroup in GetTotalBaseModValue()!");
        return 0.0f;
    }

    if(m_auraBaseMod[modGroup][PCT_MOD] <= 0.0f)
        return 0.0f;

    return m_auraBaseMod[modGroup][FLAT_MOD] * m_auraBaseMod[modGroup][PCT_MOD];
}

uint32 Player::GetShieldBlockValue() const
{
    float value = (m_auraBaseMod[SHIELD_BLOCK_VALUE][FLAT_MOD] + GetStat(STAT_STRENGTH) * 0.5f - 10)*m_auraBaseMod[SHIELD_BLOCK_VALUE][PCT_MOD];

    value = (value < 0) ? 0 : value;

    return uint32(value);
}

float Player::GetMeleeCritFromAgility()
{
    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level>GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtChanceToMeleeCritBaseEntry const *critBase  = sGtChanceToMeleeCritBaseStore.LookupEntry(pclass-1);
    GtChanceToMeleeCritEntry     const *critRatio = sGtChanceToMeleeCritStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    if (critBase==NULL || critRatio==NULL)
        return 0.0f;

    float crit=critBase->base + GetStat(STAT_AGILITY)*critRatio->ratio;
    return crit*100.0f;
}

void Player::GetDodgeFromAgility(float &diminishing, float &nondiminishing)
{
    // Table for base dodge values
    const float dodge_base[MAX_CLASSES] =
    {
         0.036640f, // Warrior
         0.034943f, // Paladin
        -0.040873f, // Hunter
         0.020957f, // Rogue
         0.034178f, // Priest
         0.036640f, // DK
         0.021080f, // Shaman
         0.036587f, // Mage
         0.024211f, // Warlock
         0.0f,      // ??
         0.056097f  // Druid
    };
    // Crit/agility to dodge/agility coefficient multipliers; 3.2.0 increased required agility by 15%
    const float crit_to_dodge[MAX_CLASSES] =
    {
         0.85f/1.15f,    // Warrior
         1.00f/1.15f,    // Paladin
         1.11f/1.15f,    // Hunter
         2.00f/1.15f,    // Rogue
         1.00f/1.15f,    // Priest
         0.85f/1.15f,    // DK
         1.60f/1.15f,    // Shaman
         1.00f/1.15f,    // Mage
         0.97f/1.15f,    // Warlock (?)
         0.0f,           // ??
         2.00f/1.15f     // Druid
    };

    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level>GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    // Dodge per agility is proportional to crit per agility, which is available from DBC files
    GtChanceToMeleeCritEntry  const *dodgeRatio = sGtChanceToMeleeCritStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    if (dodgeRatio==NULL || pclass > MAX_CLASSES)
        return;

    // TODO: research if talents/effects that increase total agility by x% should increase non-diminishing part
    float base_agility = GetCreateStat(STAT_AGILITY) * m_auraModifiersGroup[UNIT_MOD_STAT_START + STAT_AGILITY][BASE_PCT];
    float bonus_agility = GetStat(STAT_AGILITY) - base_agility;
    // calculate diminishing (green in char screen) and non-diminishing (white) contribution
    diminishing = 100.0f * bonus_agility * dodgeRatio->ratio * crit_to_dodge[pclass-1];
    nondiminishing = 100.0f * (dodge_base[pclass-1] + base_agility * dodgeRatio->ratio * crit_to_dodge[pclass-1]);
}

float Player::GetSpellCritFromIntellect()
{
    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level>GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtChanceToSpellCritBaseEntry const *critBase  = sGtChanceToSpellCritBaseStore.LookupEntry(pclass-1);
    GtChanceToSpellCritEntry     const *critRatio = sGtChanceToSpellCritStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    if (critBase==NULL || critRatio==NULL)
        return 0.0f;

    float crit=critBase->base + GetStat(STAT_INTELLECT)*critRatio->ratio;
    return crit*100.0f;
}

float Player::GetRatingMultiplier(CombatRating cr) const
{
    uint32 level = getLevel();

    if (level>GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtCombatRatingsEntry const *Rating = sGtCombatRatingsStore.LookupEntry(cr*GT_MAX_LEVEL+level-1);
    // gtOCTClassCombatRatingScalarStore.dbc starts with 1, CombatRating with zero, so cr+1
    GtOCTClassCombatRatingScalarEntry const *classRating = sGtOCTClassCombatRatingScalarStore.LookupEntry((getClass()-1)*GT_MAX_RATING+cr+1);
    if (!Rating || !classRating)
        return 1.0f;                                        // By default use minimum coefficient (not must be called)

    return classRating->ratio / Rating->ratio;
}

float Player::GetRatingBonusValue(CombatRating cr) const
{
    return float(GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + cr)) * GetRatingMultiplier(cr);
}

float Player::GetExpertiseDodgeOrParryReduction(WeaponAttackType attType) const
{
    switch (attType)
    {
        case BASE_ATTACK:
            return GetUInt32Value(PLAYER_EXPERTISE) / 4.0f;
        case OFF_ATTACK:
            return GetUInt32Value(PLAYER_OFFHAND_EXPERTISE) / 4.0f;
        default:
            break;
    }
    return 0.0f;
}

float Player::OCTRegenHPPerSpirit()
{
    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level>GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtOCTRegenHPEntry     const *baseRatio = sGtOCTRegenHPStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    GtRegenHPPerSptEntry  const *moreRatio = sGtRegenHPPerSptStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    if (baseRatio==NULL || moreRatio==NULL)
        return 0.0f;

    // Formula from PaperDollFrame script
    float spirit = GetStat(STAT_SPIRIT);
    float baseSpirit = spirit;
    if (baseSpirit>50) baseSpirit = 50;
    float moreSpirit = spirit - baseSpirit;
    float regen = baseSpirit * baseRatio->ratio + moreSpirit * moreRatio->ratio;
    return regen;
}

float Player::OCTRegenMPPerSpirit()
{
    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level>GT_MAX_LEVEL) level = GT_MAX_LEVEL;

//    GtOCTRegenMPEntry     const *baseRatio = sGtOCTRegenMPStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    GtRegenMPPerSptEntry  const *moreRatio = sGtRegenMPPerSptStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    if (moreRatio==NULL)
        return 0.0f;

    // Formula get from PaperDollFrame script
    float spirit    = GetStat(STAT_SPIRIT);
    float regen     = spirit * moreRatio->ratio;
    return regen;
}

void Player::ApplyRatingMod(CombatRating cr, int32 value, bool apply)
{
    m_baseRatingValue[cr]+=(apply ? value : -value);

    // explicit affected values
    switch (cr)
    {
        case CR_HASTE_MELEE:
        {
            float RatingChange = value * GetRatingMultiplier(cr);
            ApplyAttackTimePercentMod(BASE_ATTACK,RatingChange,apply);
            ApplyAttackTimePercentMod(OFF_ATTACK,RatingChange,apply);
            break;
        }
        case CR_HASTE_RANGED:
        {
            float RatingChange = value * GetRatingMultiplier(cr);
            ApplyAttackTimePercentMod(RANGED_ATTACK, RatingChange, apply);
            break;
        }
        case CR_HASTE_SPELL:
        {
            float RatingChange = value * GetRatingMultiplier(cr);
            ApplyCastTimePercentMod(RatingChange,apply);
            break;
        }
        default:
            break;
    }

    UpdateRating(cr);
}

void Player::UpdateRating(CombatRating cr)
{
    int32 amount = m_baseRatingValue[cr];
    // Apply bonus from SPELL_AURA_MOD_RATING_FROM_STAT
    // stat used stored in miscValueB for this aura
    AuraList const& modRatingFromStat = GetAurasByType(SPELL_AURA_MOD_RATING_FROM_STAT);
    for(AuraList::const_iterator i = modRatingFromStat.begin();i != modRatingFromStat.end(); ++i)
        if ((*i)->GetMiscValue() & (1<<cr))
            amount += int32(GetStat(Stats((*i)->GetMiscBValue())) * (*i)->GetModifier()->m_amount / 100.0f);
    if (amount < 0)
        amount = 0;
    SetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + cr, uint32(amount));

    bool affectStats = CanModifyStats();

    switch (cr)
    {
        case CR_WEAPON_SKILL:                               // Implemented in Unit::RollMeleeOutcomeAgainst
        case CR_DEFENSE_SKILL:
            UpdateDefenseBonusesMod();
            break;
        case CR_DODGE:
            UpdateDodgePercentage();
            break;
        case CR_PARRY:
            UpdateParryPercentage();
            break;
        case CR_BLOCK:
            UpdateBlockPercentage();
            break;
        case CR_HIT_MELEE:
            UpdateMeleeHitChances();
            break;
        case CR_HIT_RANGED:
            UpdateRangedHitChances();
            break;
        case CR_HIT_SPELL:
            UpdateSpellHitChances();
            break;
        case CR_CRIT_MELEE:
            if(affectStats)
            {
                UpdateCritPercentage(BASE_ATTACK);
                UpdateCritPercentage(OFF_ATTACK);
            }
            break;
        case CR_CRIT_RANGED:
            if(affectStats)
                UpdateCritPercentage(RANGED_ATTACK);
            break;
        case CR_CRIT_SPELL:
            if(affectStats)
                UpdateAllSpellCritChances();
            break;
        case CR_HIT_TAKEN_MELEE:                            // Implemented in Unit::MeleeMissChanceCalc
        case CR_HIT_TAKEN_RANGED:
            break;
        case CR_HIT_TAKEN_SPELL:                            // Implemented in Unit::MagicSpellHitResult
            break;
        case CR_CRIT_TAKEN_MELEE:                           // Implemented in Unit::RollMeleeOutcomeAgainst (only for chance to crit)
        case CR_CRIT_TAKEN_RANGED:
            break;
        case CR_CRIT_TAKEN_SPELL:                           // Implemented in Unit::SpellCriticalBonus (only for chance to crit)
            break;
        case CR_HASTE_MELEE:                                // Implemented in Player::ApplyRatingMod
        case CR_HASTE_RANGED:
        case CR_HASTE_SPELL:
            break;
        case CR_WEAPON_SKILL_MAINHAND:                      // Implemented in Unit::RollMeleeOutcomeAgainst
        case CR_WEAPON_SKILL_OFFHAND:
        case CR_WEAPON_SKILL_RANGED:
            break;
        case CR_EXPERTISE:
            if(affectStats)
            {
                UpdateExpertise(BASE_ATTACK);
                UpdateExpertise(OFF_ATTACK);
            }
            break;
        case CR_ARMOR_PENETRATION:
            if(affectStats)
                UpdateArmorPenetration();
            break;
    }
}

void Player::UpdateAllRatings()
{
    for(int cr = 0; cr < MAX_COMBAT_RATING; ++cr)
        UpdateRating(CombatRating(cr));
}

void Player::SetRegularAttackTime()
{
    for(int i = 0; i < MAX_ATTACK; ++i)
    {
        Item *tmpitem = GetWeaponForAttack(WeaponAttackType(i),true,false);
        if (tmpitem)
        {
            ItemPrototype const *proto = tmpitem->GetProto();
            if(proto->Delay)
                SetAttackTime(WeaponAttackType(i), proto->Delay);
            else
                SetAttackTime(WeaponAttackType(i), BASE_ATTACK_TIME);
        }
    }
}

//skill+step, checking for max value
bool Player::UpdateSkill(uint32 skill_id, uint32 step)
{
    if(!skill_id)
        return false;

    SkillStatusMap::iterator itr = mSkillStatus.find(skill_id);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return false;

    uint32 valueIndex = PLAYER_SKILL_VALUE_INDEX(itr->second.pos);
    uint32 data = GetUInt32Value(valueIndex);
    uint32 value = SKILL_VALUE(data);
    uint32 max = SKILL_MAX(data);

    if ((!max) || (!value) || (value >= max))
        return false;

    if (value*512 < max*urand(0,512))
    {
        uint32 new_value = value+step;
        if(new_value > max)
            new_value = max;

        SetUInt32Value(valueIndex,MAKE_SKILL_VALUE(new_value,max));
        if(itr->second.uState != SKILL_NEW)
            itr->second.uState = SKILL_CHANGED;
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL,skill_id);
        return true;
    }

    return false;
}

inline int SkillGainChance(uint32 SkillValue, uint32 GrayLevel, uint32 GreenLevel, uint32 YellowLevel)
{
    if ( SkillValue >= GrayLevel )
        return sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_GREY)*10;
    if ( SkillValue >= GreenLevel )
        return sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_GREEN)*10;
    if ( SkillValue >= YellowLevel )
        return sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_YELLOW)*10;
    return sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_ORANGE)*10;
}

bool Player::UpdateCraftSkill(uint32 spellid)
{
    DEBUG_LOG("UpdateCraftSkill spellid %d", spellid);

    SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spellid);

    for(SkillLineAbilityMap::const_iterator _spell_idx = bounds.first; _spell_idx != bounds.second; ++_spell_idx)
    {
        if (_spell_idx->second->skillId)
        {
            uint32 SkillValue = GetPureSkillValue(_spell_idx->second->skillId);

            // Alchemy Discoveries here
            SpellEntry const* spellEntry = sSpellStore.LookupEntry(spellid);
            if (spellEntry && spellEntry->Mechanic == MECHANIC_DISCOVERY)
            {
                if (uint32 discoveredSpell = GetSkillDiscoverySpell(_spell_idx->second->skillId, spellid, this))
                    learnSpell(discoveredSpell, false);
            }

            uint32 craft_skill_gain = sWorld.getConfig(CONFIG_UINT32_SKILL_GAIN_CRAFTING);

            return UpdateSkillPro(_spell_idx->second->skillId, SkillGainChance(SkillValue,
                _spell_idx->second->max_value,
                (_spell_idx->second->max_value + _spell_idx->second->min_value)/2,
                _spell_idx->second->min_value),
                craft_skill_gain);
        }
    }
    return false;
}

bool Player::UpdateGatherSkill(uint32 SkillId, uint32 SkillValue, uint32 RedLevel, uint32 Multiplicator )
{
    DEBUG_LOG("UpdateGatherSkill(SkillId %d SkillLevel %d RedLevel %d)", SkillId, SkillValue, RedLevel);

    uint32 gathering_skill_gain = sWorld.getConfig(CONFIG_UINT32_SKILL_GAIN_GATHERING);

    // For skinning and Mining chance decrease with level. 1-74 - no decrease, 75-149 - 2 times, 225-299 - 8 times
    switch (SkillId)
    {
        case SKILL_HERBALISM:
        case SKILL_LOCKPICKING:
        case SKILL_JEWELCRAFTING:
        case SKILL_INSCRIPTION:
            return UpdateSkillPro(SkillId, SkillGainChance(SkillValue, RedLevel+100, RedLevel+50, RedLevel+25)*Multiplicator,gathering_skill_gain);
        case SKILL_SKINNING:
            if( sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_SKINNING_STEPS)==0)
                return UpdateSkillPro(SkillId, SkillGainChance(SkillValue, RedLevel+100, RedLevel+50, RedLevel+25)*Multiplicator,gathering_skill_gain);
            else
                return UpdateSkillPro(SkillId, (SkillGainChance(SkillValue, RedLevel+100, RedLevel+50, RedLevel+25)*Multiplicator) >> (SkillValue/sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_SKINNING_STEPS)), gathering_skill_gain);
        case SKILL_MINING:
            if (sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_MINING_STEPS)==0)
                return UpdateSkillPro(SkillId, SkillGainChance(SkillValue, RedLevel+100, RedLevel+50, RedLevel+25)*Multiplicator,gathering_skill_gain);
            else
                return UpdateSkillPro(SkillId, (SkillGainChance(SkillValue, RedLevel+100, RedLevel+50, RedLevel+25)*Multiplicator) >> (SkillValue/sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_MINING_STEPS)),gathering_skill_gain);
    }
    return false;
}

bool Player::UpdateFishingSkill()
{
    DEBUG_LOG("UpdateFishingSkill");

    uint32 SkillValue = GetPureSkillValue(SKILL_FISHING);

    int32 chance = SkillValue < 75 ? 100 : 2500/(SkillValue-50);

    uint32 gathering_skill_gain = sWorld.getConfig(CONFIG_UINT32_SKILL_GAIN_GATHERING);

    return UpdateSkillPro(SKILL_FISHING,chance*10,gathering_skill_gain);
}

// levels sync. with spell requirement for skill levels to learn
// bonus abilities in sSkillLineAbilityStore
// Used only to avoid scan DBC at each skill grow
static uint32 bonusSkillLevels[] = {75,150,225,300,375,450};

bool Player::UpdateSkillPro(uint16 SkillId, int32 Chance, uint32 step)
{
    DEBUG_LOG("UpdateSkillPro(SkillId %d, Chance %3.1f%%)", SkillId, Chance/10.0);
    if ( !SkillId )
        return false;

    if(Chance <= 0)                                         // speedup in 0 chance case
    {
        DEBUG_LOG("Player::UpdateSkillPro Chance=%3.1f%% missed", Chance/10.0);
        return false;
    }

    SkillStatusMap::iterator itr = mSkillStatus.find(SkillId);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return false;

    uint32 valueIndex = PLAYER_SKILL_VALUE_INDEX(itr->second.pos);

    uint32 data = GetUInt32Value(valueIndex);
    uint16 SkillValue = SKILL_VALUE(data);
    uint16 MaxValue   = SKILL_MAX(data);

    if ( !MaxValue || !SkillValue || SkillValue >= MaxValue )
        return false;

    int32 Roll = irand(1,1000);

    if ( Roll <= Chance )
    {
        uint32 new_value = SkillValue+step;
        if(new_value > MaxValue)
            new_value = MaxValue;

        SetUInt32Value(valueIndex,MAKE_SKILL_VALUE(new_value,MaxValue));
        if(itr->second.uState != SKILL_NEW)
            itr->second.uState = SKILL_CHANGED;
        for(uint32* bsl = &bonusSkillLevels[0]; *bsl; ++bsl)
        {
            if((SkillValue < *bsl && new_value >= *bsl))
            {
                learnSkillRewardedSpells( SkillId, new_value);
                break;
            }
        }
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL,SkillId);
        DEBUG_LOG("Player::UpdateSkillPro Chance=%3.1f%% taken", Chance/10.0);
        return true;
    }

    DEBUG_LOG("Player::UpdateSkillPro Chance=%3.1f%% missed", Chance/10.0);
    return false;
}

void Player::UpdateWeaponSkill(WeaponAttackType attType)
{
    // no skill gain in pvp
    Unit* pVictim = getVictim();
    if (pVictim && pVictim->IsCharmerOrOwnerPlayerOrPlayerItself())
        return;

    if (IsInFeralForm())
        return;                                             // always maximized SKILL_FERAL_COMBAT in fact

    if (GetShapeshiftForm() == FORM_TREE)
        return;                                             // use weapon but not skill up

    uint32 weaponSkillGain = sWorld.getConfig(CONFIG_UINT32_SKILL_GAIN_WEAPON);

    Item* pWeapon = GetWeaponForAttack(attType, true, true);
    if (pWeapon && pWeapon->GetProto()->SubClass != ITEM_SUBCLASS_WEAPON_FISHING_POLE)
        UpdateSkill(pWeapon->GetSkill(), weaponSkillGain);
    else if (!pWeapon && attType == BASE_ATTACK)
        UpdateSkill(SKILL_UNARMED, weaponSkillGain);

    UpdateAllCritPercentages();
}

void Player::UpdateCombatSkills(Unit *pVictim, WeaponAttackType attType, bool defence)
{
    uint32 plevel = getLevel();                             // if defense than pVictim == attacker
    uint32 greylevel = MaNGOS::XP::GetGrayLevel(plevel);
    uint32 moblevel = pVictim->GetLevelForTarget(this);
    if(moblevel < greylevel)
        return;

    if (moblevel > plevel + 5)
        moblevel = plevel + 5;

    uint32 lvldif = moblevel - greylevel;
    if(lvldif < 3)
        lvldif = 3;

    int32 skilldif = 5 * plevel - (defence ? GetBaseDefenseSkillValue() : GetBaseWeaponSkillValue(attType));

    // Max skill reached for level.
    // Can in some cases be less than 0: having max skill and then .level -1 as example.
    if (skilldif <= 0)
        return;

    float chance = float(3 * lvldif * skilldif) / plevel;
    if(!defence)
        chance *= 0.1f * GetStat(STAT_INTELLECT);

    chance = chance < 1.0f ? 1.0f : chance;                 //minimum chance to increase skill is 1%

    if(roll_chance_f(chance))
    {
        if(defence)
            UpdateDefense();
        else
            UpdateWeaponSkill(attType);
    }
    else
        return;
}

void Player::ModifySkillBonus(uint32 skillid,int32 val, bool talent)
{
    SkillStatusMap::const_iterator itr = mSkillStatus.find(skillid);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return;

    uint32 bonusIndex = PLAYER_SKILL_BONUS_INDEX(itr->second.pos);

    uint32 bonus_val = GetUInt32Value(bonusIndex);
    int16 temp_bonus = SKILL_TEMP_BONUS(bonus_val);
    int16 perm_bonus = SKILL_PERM_BONUS(bonus_val);

    if(talent)                                          // permanent bonus stored in high part
        SetUInt32Value(bonusIndex,MAKE_SKILL_BONUS(temp_bonus,perm_bonus+val));
    else                                                // temporary/item bonus stored in low part
        SetUInt32Value(bonusIndex,MAKE_SKILL_BONUS(temp_bonus+val,perm_bonus));
}

void Player::UpdateSkillsForLevel()
{
    uint16 maxconfskill = sWorld.GetConfigMaxSkillValue();
    uint32 maxSkill = GetMaxSkillValueForLevel();

    bool alwaysMaxSkill = sWorld.getConfig(CONFIG_BOOL_ALWAYS_MAX_SKILL_FOR_LEVEL);

    for(SkillStatusMap::iterator itr = mSkillStatus.begin(); itr != mSkillStatus.end(); ++itr)
    {
        if(itr->second.uState == SKILL_DELETED)
            continue;

        uint32 pskill = itr->first;

        SkillLineEntry const *pSkill = sSkillLineStore.LookupEntry(pskill);
        if(!pSkill)
            continue;

        if(GetSkillRangeType(pSkill,false) != SKILL_RANGE_LEVEL)
            continue;

        uint32 valueIndex = PLAYER_SKILL_VALUE_INDEX(itr->second.pos);
        uint32 data = GetUInt32Value(valueIndex);
        uint32 max = SKILL_MAX(data);
        uint32 val = SKILL_VALUE(data);

        /// update only level dependent max skill values
        if(max!=1)
        {
            /// maximize skill always
            if(alwaysMaxSkill)
            {
                SetUInt32Value(valueIndex, MAKE_SKILL_VALUE(maxSkill,maxSkill));
                if(itr->second.uState != SKILL_NEW)
                    itr->second.uState = SKILL_CHANGED;
                GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL, pskill);
            }
            else if(max != maxconfskill)                    /// update max skill value if current max skill not maximized
            {
                SetUInt32Value(valueIndex, MAKE_SKILL_VALUE(val,maxSkill));
                if(itr->second.uState != SKILL_NEW)
                    itr->second.uState = SKILL_CHANGED;
            }
        }
    }
}

void Player::UpdateSkillsToMaxSkillsForLevel()
{
    for(SkillStatusMap::iterator itr = mSkillStatus.begin(); itr != mSkillStatus.end(); ++itr)
    {
        if(itr->second.uState == SKILL_DELETED)
            continue;

        uint32 pskill = itr->first;
        if( IsProfessionOrRidingSkill(pskill))
            continue;
        uint32 valueIndex = PLAYER_SKILL_VALUE_INDEX(itr->second.pos);
        uint32 data = GetUInt32Value(valueIndex);

        uint32 max = SKILL_MAX(data);

        if(max > 1)
        {
            SetUInt32Value(valueIndex,MAKE_SKILL_VALUE(max,max));
            if(itr->second.uState != SKILL_NEW)
                itr->second.uState = SKILL_CHANGED;
            GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL, pskill);
        }

        if(pskill == SKILL_DEFENSE)
            UpdateDefenseBonusesMod();
    }
}

// This functions sets a skill line value (and adds if doesn't exist yet)
// To "remove" a skill line, set it's values to zero
void Player::SetSkill(uint16 id, uint16 currVal, uint16 maxVal, uint16 step /*=0*/)
{
    if(!id)
        return;

    SkillStatusMap::iterator itr = mSkillStatus.find(id);

    // has skill
    if(itr != mSkillStatus.end() && itr->second.uState != SKILL_DELETED)
    {
        if(currVal)
        {
            if (step)                                      // need update step
                SetUInt32Value(PLAYER_SKILL_INDEX(itr->second.pos), MAKE_PAIR32(id, step));
            // update value
            SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos), MAKE_SKILL_VALUE(currVal, maxVal));
            if(itr->second.uState != SKILL_NEW)
                itr->second.uState = SKILL_CHANGED;
            learnSkillRewardedSpells(id, currVal);
            GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL, id);
            GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL, id);
        }
        else                                                //remove
        {
            // clear skill fields
            SetUInt32Value(PLAYER_SKILL_INDEX(itr->second.pos), 0);
            SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos), 0);
            SetUInt32Value(PLAYER_SKILL_BONUS_INDEX(itr->second.pos), 0);

            // mark as deleted or simply remove from map if not saved yet
            if(itr->second.uState != SKILL_NEW)
                itr->second.uState = SKILL_DELETED;
            else
                mSkillStatus.erase(itr);

            // remove all spells that related to this skill
            for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
                if(SkillLineAbilityEntry const *pAbility = sSkillLineAbilityStore.LookupEntry(j))
                    if (pAbility->skillId == id)
                        removeSpell(sSpellMgr.GetFirstSpellInChain(pAbility->spellId));
        }
    }
    else if(currVal)                                        // add
    {
        for (int i = 0; i < PLAYER_MAX_SKILLS; ++i)
        {
            if (!GetUInt32Value(PLAYER_SKILL_INDEX(i)))
            {
                SkillLineEntry const *pSkill = sSkillLineStore.LookupEntry(id);
                if(!pSkill)
                {
                    sLog.outError("Skill not found in SkillLineStore: skill #%u", id);
                    return;
                }

                SetUInt32Value(PLAYER_SKILL_INDEX(i), MAKE_PAIR32(id, step));
                SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(i),MAKE_SKILL_VALUE(currVal, maxVal));
                GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL, id);
                GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL, id);

                // insert new entry or update if not deleted old entry yet
                if(itr != mSkillStatus.end())
                {
                    itr->second.pos = i;
                    itr->second.uState = SKILL_CHANGED;
                }
                else
                    mSkillStatus.insert(SkillStatusMap::value_type(id, SkillStatusData(i, SKILL_NEW)));

                // apply skill bonuses
                SetUInt32Value(PLAYER_SKILL_BONUS_INDEX(i), 0);

                // temporary bonuses
                AuraList const& mModSkill = GetAurasByType(SPELL_AURA_MOD_SKILL);
                for(AuraList::const_iterator j = mModSkill.begin(); j != mModSkill.end(); ++j)
                    if ((*j)->GetModifier()->m_miscvalue == int32(id))
                        (*j)->ApplyModifier(true);

                // permanent bonuses
                AuraList const& mModSkillTalent = GetAurasByType(SPELL_AURA_MOD_SKILL_TALENT);
                for(AuraList::const_iterator j = mModSkillTalent.begin(); j != mModSkillTalent.end(); ++j)
                    if ((*j)->GetModifier()->m_miscvalue == int32(id))
                        (*j)->ApplyModifier(true);

                // Learn all spells for skill
                learnSkillRewardedSpells(id, currVal);
                return;
            }
        }
    }
}

bool Player::HasSkill(uint32 skill) const
{
    if(!skill)
        return false;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    return (itr != mSkillStatus.end() && itr->second.uState != SKILL_DELETED);
}

uint16 Player::GetSkillValue(uint32 skill) const
{
    if(!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    uint32 bonus = GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(itr->second.pos));

    int32 result = int32(SKILL_VALUE(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos))));
    result += SKILL_TEMP_BONUS(bonus);
    result += SKILL_PERM_BONUS(bonus);
    return result < 0 ? 0 : result;
}

uint16 Player::GetMaxSkillValue(uint32 skill) const
{
    if(!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    uint32 bonus = GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(itr->second.pos));

    int32 result = int32(SKILL_MAX(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos))));
    result += SKILL_TEMP_BONUS(bonus);
    result += SKILL_PERM_BONUS(bonus);
    return result < 0 ? 0 : result;
}

uint16 Player::GetPureMaxSkillValue(uint32 skill) const
{
    if(!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    return SKILL_MAX(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos)));
}

uint16 Player::GetBaseSkillValue(uint32 skill) const
{
    if(!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    int32 result = int32(SKILL_VALUE(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos))));
    result +=  SKILL_PERM_BONUS(GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(itr->second.pos)));
    return result < 0 ? 0 : result;
}

uint16 Player::GetPureSkillValue(uint32 skill) const
{
    if(!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    return SKILL_VALUE(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos)));
}

int16 Player::GetSkillPermBonusValue(uint32 skill) const
{
    if(!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    return SKILL_PERM_BONUS(GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(itr->second.pos)));
}

int16 Player::GetSkillTempBonusValue(uint32 skill) const
{
    if(!skill)
        return 0;

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if(itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
        return 0;

    return SKILL_TEMP_BONUS(GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(itr->second.pos)));
}

void Player::SendInitialActionButtons() const
{
    DETAIL_LOG( "Initializing Action Buttons for '%u' spec '%u'", GetGUIDLow(), m_activeSpec);

    WorldPacket data(SMSG_ACTION_BUTTONS, 1+(MAX_ACTION_BUTTONS*4));
    data << uint8(1);                                       // talent spec amount (in packet)
    ActionButtonList const& currentActionButtonList = m_actionButtons[m_activeSpec];
    for(uint8 button = 0; button < MAX_ACTION_BUTTONS; ++button)
    {
        ActionButtonList::const_iterator itr = currentActionButtonList.find(button);
        if(itr != currentActionButtonList.end() && itr->second.uState != ACTIONBUTTON_DELETED)
            data << uint32(itr->second.packedData);
        else
            data << uint32(0);
    }

    GetSession()->SendPacket( &data );
    DETAIL_LOG( "Action Buttons for '%u' spec '%u' Initialized", GetGUIDLow(), m_activeSpec );
}

void Player::SendLockActionButtons() const
{
    DETAIL_LOG( "Locking Action Buttons for '%u' spec '%u'", GetGUIDLow(), m_activeSpec);
    WorldPacket data(SMSG_ACTION_BUTTONS, 1);
    // sending 2 locks actions bars, neither user can remove buttons, nor client removes buttons at spell unlearn
    // they remain locked until server sends new action buttons
    data << uint8(2);
    GetSession()->SendPacket( &data );
}

bool Player::IsActionButtonDataValid(uint8 button, uint32 action, uint8 type, Player* player, bool msg)
{
    if(button >= MAX_ACTION_BUTTONS)
    {
        if (msg)
        {
            if (player)
                sLog.outError( "Action %u not added into button %u for player %s: button must be < %u", action, button, player->GetName(), MAX_ACTION_BUTTONS );
            else
                sLog.outError( "Table `playercreateinfo_action` have action %u into button %u : button must be < %u", action, button, MAX_ACTION_BUTTONS );

        }
        return false;
    }

    if(action >= MAX_ACTION_BUTTON_ACTION_VALUE)
    {
        if (msg)
        {
            if (player)
                sLog.outError( "Action %u not added into button %u for player %s: action must be < %u", action, button, player->GetName(), MAX_ACTION_BUTTON_ACTION_VALUE );
            else
                sLog.outError( "Table `playercreateinfo_action` have action %u into button %u : action must be < %u", action, button, MAX_ACTION_BUTTON_ACTION_VALUE );
        }
        return false;
    }

    switch(type)
    {
        case ACTION_BUTTON_SPELL:
        {
            SpellEntry const* spellProto = sSpellStore.LookupEntry(action);
            if(!spellProto)
            {
                if (msg)
                {
                    if (player)
                        sLog.outError( "Spell action %u not added into button %u for player %s: spell not exist", action, button, player->GetName() );
                    else
                        sLog.outError( "Table `playercreateinfo_action` have spell action %u into button %u: spell not exist", action, button );
                }
                return false;
            }

            if(player)
            {
                if(!player->HasSpell(spellProto->Id))
                {
                    if (msg)
                        sLog.outError( "Spell action %u not added into button %u for player %s: player don't known this spell", action, button, player->GetName() );
                    return false;
                }
                else if(IsPassiveSpell(spellProto))
                {
                    if (msg)
                        sLog.outError( "Spell action %u not added into button %u for player %s: spell is passive", action, button, player->GetName() );
                    return false;
                }
                // current range for button of totem bar is from ACTION_BUTTON_SHAMAN_TOTEMS_BAR to (but not including) ACTION_BUTTON_SHAMAN_TOTEMS_BAR + 12
                else if(button >= ACTION_BUTTON_SHAMAN_TOTEMS_BAR && button < (ACTION_BUTTON_SHAMAN_TOTEMS_BAR + 12)
                    && !(spellProto->AttributesEx7 & SPELL_ATTR_EX7_TOTEM_SPELL))
                {
                    if (msg)
                        sLog.outError( "Spell action %u not added into button %u for player %s: attempt to add non totem spell to totem bar", action, button, player->GetName() );
                    return false;
                }
            }
            break;
        }
        case ACTION_BUTTON_ITEM:
        {
            if(!ObjectMgr::GetItemPrototype(action))
            {
                if (msg)
                {
                    if (player)
                        sLog.outError( "Item action %u not added into button %u for player %s: item not exist", action, button, player->GetName() );
                    else
                        sLog.outError( "Table `playercreateinfo_action` have item action %u into button %u: item not exist", action, button );
                }
                return false;
            }
            break;
        }
        default:
            break;                                          // other cases not checked at this moment
    }

    return true;
}

ActionButton* Player::addActionButton(uint8 spec, uint8 button, uint32 action, uint8 type)
{
    // check action only for active spec (so not check at copy/load passive spec)
    if (spec == GetActiveSpec() && !IsActionButtonDataValid(button,action,type,this))
        return NULL;

    // it create new button (NEW state) if need or return existing
    ActionButton& ab = m_actionButtons[spec][button];

    // set data and update to CHANGED if not NEW
    ab.SetActionAndType(action,ActionButtonType(type));

    DETAIL_LOG("Player '%u' Added Action '%u' (type %u) to Button '%u' for spec %u", GetGUIDLow(), action, uint32(type), button, spec);
    return &ab;
}

void Player::removeActionButton(uint8 spec, uint8 button)
{
    ActionButtonList& currentActionButtonList = m_actionButtons[spec];
    ActionButtonList::iterator buttonItr = currentActionButtonList.find(button);
    if (buttonItr == currentActionButtonList.end() || buttonItr->second.uState == ACTIONBUTTON_DELETED)
        return;

    if (buttonItr->second.uState == ACTIONBUTTON_NEW)
        currentActionButtonList.erase(buttonItr);           // new and not saved
    else
        buttonItr->second.uState = ACTIONBUTTON_DELETED;    // saved, will deleted at next save

    DETAIL_LOG("Action Button '%u' Removed from Player '%u' for spec %u", button, GetGUIDLow(), spec);
}

ActionButton const* Player::GetActionButton(uint8 button)
{
    ActionButtonList& currentActionButtonList = m_actionButtons[m_activeSpec];
    ActionButtonList::iterator buttonItr = currentActionButtonList.find(button);
    if (buttonItr==currentActionButtonList.end() || buttonItr->second.uState == ACTIONBUTTON_DELETED)
        return NULL;

    return &buttonItr->second;
}

bool Player::SetPosition(float x, float y, float z, float orientation, bool teleport)
{
    // prevent crash when a bad coord is sent by the client
    if(!MaNGOS::IsValidMapCoord(x,y,z,orientation))
    {
        DEBUG_LOG("Player::SetPosition(%f, %f, %f, %f, %d) .. bad coordinates for player %d!",x,y,z,orientation,teleport,GetGUIDLow());
        return false;
    }

    Map *m = GetMap();

    const float old_x = GetPositionX();
    const float old_y = GetPositionY();
    const float old_z = GetPositionZ();
    const float old_r = GetOrientation();

    if( teleport || old_x != x || old_y != y || old_z != z || old_r != orientation )
    {
        if (teleport || old_x != x || old_y != y || old_z != z)
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOVE | AURA_INTERRUPT_FLAG_TURNING);
        else
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TURNING);

        RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

        // move and update visible state if need
        m->PlayerRelocation(this, x, y, z, orientation);

        // reread after Map::Relocation
        m = GetMap();
        x = GetPositionX();
        y = GetPositionY();
        z = GetPositionZ();

        // group update
        if (GetGroup() && (old_x != x || old_y != y))
            SetGroupUpdateFlag(GROUP_UPDATE_FLAG_POSITION);

        if (GetTrader() && !IsWithinDistInMap(GetTrader(), INTERACTION_DISTANCE))
            GetSession()->SendCancelTrade();   // will close both side trade windows
    }

    // code block for underwater state update
    UpdateUnderwaterState(m, x, y, z);

    CheckAreaExploreAndOutdoor();

    return true;
}

void Player::SaveRecallPosition()
{
    m_recallMap = GetMapId();
    m_recallX = GetPositionX();
    m_recallY = GetPositionY();
    m_recallZ = GetPositionZ();
    m_recallO = GetOrientation();
}

void Player::SendMessageToSet(WorldPacket *data, bool self)
{
    if (IsInWorld())
        GetMap()->MessageBroadcast(this, data, false);

    //if player is not in world and map in not created/already destroyed
    //no need to create one, just send packet for itself!
    if (self)
        GetSession()->SendPacket(data);
}

void Player::SendMessageToSetInRange(WorldPacket *data, float dist, bool self)
{
    if (IsInWorld())
        GetMap()->MessageDistBroadcast(this, data, dist, false);

    if (self)
        GetSession()->SendPacket(data);
}

void Player::SendMessageToSetInRange(WorldPacket *data, float dist, bool self, bool own_team_only)
{
    if (IsInWorld())
        GetMap()->MessageDistBroadcast(this, data, dist, false, own_team_only);

    if (self)
        GetSession()->SendPacket(data);
}

void Player::SendDirectMessage(WorldPacket *data)
{
    GetSession()->SendPacket(data);
}

void Player::SendCinematicStart(uint32 CinematicSequenceId)
{
    WorldPacket data(SMSG_TRIGGER_CINEMATIC, 4);
    data << uint32(CinematicSequenceId);
    SendDirectMessage(&data);
}

void Player::SendMovieStart(uint32 MovieId)
{
    WorldPacket data(SMSG_TRIGGER_MOVIE, 4);
    data << uint32(MovieId);
    SendDirectMessage(&data);
}

void Player::CheckAreaExploreAndOutdoor()
{
    if (!isAlive())
        return;

    if (IsTaxiFlying())
        return;

    bool isOutdoor;
    uint16 areaFlag = GetTerrain()->GetAreaFlag(GetPositionX(),GetPositionY(),GetPositionZ(), &isOutdoor);

    if (isOutdoor)
    {
        if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING) && GetRestType() == REST_TYPE_IN_TAVERN)
        {
            AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(inn_trigger_id);
            if (!at || !IsPointInAreaTriggerZone(at, GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ()))
            {
                // Player left inn (REST_TYPE_IN_CITY overrides REST_TYPE_IN_TAVERN, so just clear rest)
                SetRestType(REST_TYPE_NO);
            }
        }
    }
    else if (sWorld.getConfig(CONFIG_BOOL_VMAP_INDOOR_CHECK) && !isGameMaster())
        RemoveAurasWithAttribute(SPELL_ATTR_OUTDOORS_ONLY);

    if (areaFlag==0xffff)
        return;
    int offset = areaFlag / 32;

    if (offset >= PLAYER_EXPLORED_ZONES_SIZE)
    {
        sLog.outError("Wrong area flag %u in map data for (X: %f Y: %f) point to field PLAYER_EXPLORED_ZONES_1 + %u ( %u must be < %u ).",areaFlag,GetPositionX(),GetPositionY(),offset,offset, PLAYER_EXPLORED_ZONES_SIZE);
        return;
    }

    uint32 val = (uint32)(1 << (areaFlag % 32));
    uint32 currFields = GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset);

    if (!(currFields & val))
    {
        SetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset, (uint32)(currFields | val));

        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA);

        AreaTableEntry const *p = GetAreaEntryByAreaFlagAndMap(areaFlag,GetMapId());
        if(!p)
        {
            sLog.outError("PLAYER: Player %u discovered unknown area (x: %f y: %f map: %u", GetGUIDLow(), GetPositionX(),GetPositionY(),GetMapId());
        }
        else if(p->area_level > 0)
        {
            uint32 area = p->ID;
            if (getLevel() >= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
            {
                SendExplorationExperience(area,0);
            }
            else
            {
                int32 diff = int32(getLevel()) - p->area_level;
                uint32 XP = 0;
                if (diff < -5)
                {
                    XP = uint32(sObjectMgr.GetBaseXP(getLevel()+5)*sWorld.getConfig(CONFIG_FLOAT_RATE_XP_EXPLORE));
                }
                else if (diff > 5)
                {
                    int32 exploration_percent = (100-((diff-5)*5));
                    if (exploration_percent > 100)
                        exploration_percent = 100;
                    else if (exploration_percent < 0)
                        exploration_percent = 0;

                    XP = uint32(sObjectMgr.GetBaseXP(p->area_level)*exploration_percent/100*sWorld.getConfig(CONFIG_FLOAT_RATE_XP_EXPLORE));
                }
                else
                {
                    XP = uint32(sObjectMgr.GetBaseXP(p->area_level)*sWorld.getConfig(CONFIG_FLOAT_RATE_XP_EXPLORE));
                }

                GiveXP( XP, NULL );
                SendExplorationExperience(area,XP);
            }
            DETAIL_LOG("PLAYER: Player %u discovered a new area: %u", GetGUIDLow(), area);
        }
    }
}

Team Player::TeamForRace(uint8 race)
{
    ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(race);
    if (!rEntry)
    {
        sLog.outError("Race %u not found in DBC: wrong DBC files?",uint32(race));
        return ALLIANCE;
    }

    switch(rEntry->TeamID)
    {
        case 7: return ALLIANCE;
        case 1: return HORDE;
    }

    sLog.outError("Race %u have wrong teamid %u in DBC: wrong DBC files?",uint32(race),rEntry->TeamID);
    return TEAM_NONE;
}

uint32 Player::getFactionForRace(uint8 race)
{
    ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(race);
    if(!rEntry)
    {
        sLog.outError("Race %u not found in DBC: wrong DBC files?",uint32(race));
        return 0;
    }

    return rEntry->FactionID;
}

void Player::setFactionForRace(uint8 race)
{
    m_team = TeamForRace(race);
    setFaction(getFactionForRace(race));
}

ReputationRank Player::GetReputationRank(uint32 faction) const
{
    FactionEntry const* factionEntry = sFactionStore.LookupEntry(faction);
    return GetReputationMgr().GetRank(factionEntry);
}

//Calculate total reputation percent player gain with quest/creature level
int32 Player::CalculateReputationGain(ReputationSource source, int32 rep, int32 faction, uint32 creatureOrQuestLevel, bool noAuraBonus)
{
    float percent = 100.0f;

    float repMod = noAuraBonus ? 0.0f : (float)GetTotalAuraModifier(SPELL_AURA_MOD_REPUTATION_GAIN);

    // faction specific auras only seem to apply to kills
    if (source == REPUTATION_SOURCE_KILL)
        repMod += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_FACTION_REPUTATION_GAIN, faction);

    percent += rep > 0 ? repMod : -repMod;

    float rate;
    switch (source)
    {
        case REPUTATION_SOURCE_KILL:
            rate = sWorld.getConfig(CONFIG_FLOAT_RATE_REPUTATION_LOWLEVEL_KILL);
            break;
        case REPUTATION_SOURCE_QUEST:
            rate = sWorld.getConfig(CONFIG_FLOAT_RATE_REPUTATION_LOWLEVEL_QUEST);
            break;
        case REPUTATION_SOURCE_SPELL:
        default:
            rate = 1.0f;
            break;
    }

    if (rate != 1.0f && creatureOrQuestLevel <= MaNGOS::XP::GetGrayLevel(getLevel()))
        percent *= rate;

    if (percent <= 0.0f)
        return 0;

    // Multiply result with the faction specific rate
    if (const RepRewardRate *repData = sObjectMgr.GetRepRewardRate(faction))
    {
        float repRate = 0.0f;
        switch (source)
        {
            case REPUTATION_SOURCE_KILL:
                repRate = repData->creature_rate;
                break;
            case REPUTATION_SOURCE_QUEST:
                repRate = repData->quest_rate;
                break;
            case REPUTATION_SOURCE_SPELL:
                repRate = repData->spell_rate;
                break;
        }

        // for custom, a rate of 0.0 will totally disable reputation gain for this faction/type
        if (repRate <= 0.0f)
            return 0;

        percent *= repRate;
    }

    return int32(sWorld.getConfig(CONFIG_FLOAT_RATE_REPUTATION_GAIN)*rep*percent/100.0f);
}

//Calculates how many reputation points player gains in victim's enemy factions
void Player::RewardReputation(Unit *pVictim, float rate)
{
    if(!pVictim || pVictim->GetTypeId() == TYPEID_PLAYER)
        return;

    // used current difficulty creature entry instead normal version (GetEntry())
    ReputationOnKillEntry const* Rep = sObjectMgr.GetReputationOnKillEntry(((Creature*)pVictim)->GetCreatureInfo()->Entry);

    if(!Rep)
        return;

    if(Rep->repfaction1 && (!Rep->team_dependent || GetTeam()==ALLIANCE))
    {
        int32 donerep1 = CalculateReputationGain(REPUTATION_SOURCE_KILL, Rep->repvalue1, Rep->repfaction1, pVictim->getLevel());
        donerep1 = int32(donerep1*rate);
        FactionEntry const *factionEntry1 = sFactionStore.LookupEntry(Rep->repfaction1);
        uint32 current_reputation_rank1 = GetReputationMgr().GetRank(factionEntry1);
        if (factionEntry1 && current_reputation_rank1 <= Rep->reputation_max_cap1)
            GetReputationMgr().ModifyReputation(factionEntry1, donerep1);

        // Wiki: Team factions value divided by 2
        if (factionEntry1 && Rep->is_teamaward1)
        {
            FactionEntry const *team1_factionEntry = sFactionStore.LookupEntry(factionEntry1->team);
            if(team1_factionEntry)
                GetReputationMgr().ModifyReputation(team1_factionEntry, donerep1 / 2);
        }
    }

    if(Rep->repfaction2 && (!Rep->team_dependent || GetTeam()==HORDE))
    {
        int32 donerep2 = CalculateReputationGain(REPUTATION_SOURCE_KILL, Rep->repvalue2, Rep->repfaction2, pVictim->getLevel());
        donerep2 = int32(donerep2*rate);
        FactionEntry const *factionEntry2 = sFactionStore.LookupEntry(Rep->repfaction2);
        uint32 current_reputation_rank2 = GetReputationMgr().GetRank(factionEntry2);
        if (factionEntry2 && current_reputation_rank2 <= Rep->reputation_max_cap2)
            GetReputationMgr().ModifyReputation(factionEntry2, donerep2);

        // Wiki: Team factions value divided by 2
        if (factionEntry2 && Rep->is_teamaward2)
        {
            FactionEntry const *team2_factionEntry = sFactionStore.LookupEntry(factionEntry2->team);
            if(team2_factionEntry)
                GetReputationMgr().ModifyReputation(team2_factionEntry, donerep2 / 2);
        }
    }
}

//Calculate how many reputation points player gain with the quest
void Player::RewardReputation(Quest const *pQuest)
{
    // quest reputation reward/loss
    for(int i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)
    {
        if (!pQuest->RewRepFaction[i])
            continue;

        // No diplomacy mod are applied to the final value (flat). Note the formula (finalValue = DBvalue/100)
        if (pQuest->RewRepValue[i])
        {
            int32 rep = CalculateReputationGain(REPUTATION_SOURCE_QUEST, pQuest->RewRepValue[i]/100, pQuest->RewRepFaction[i], GetQuestLevelForPlayer(pQuest), true);

            if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(pQuest->RewRepFaction[i]))
                GetReputationMgr().ModifyReputation(factionEntry, rep);
        }
        else
        {
            uint32 row = ((pQuest->RewRepValueId[i] < 0) ? 1 : 0) + 1;
            uint32 field = abs(pQuest->RewRepValueId[i]);

            if (const QuestFactionRewardEntry *pRow = sQuestFactionRewardStore.LookupEntry(row))
            {
                int32 repPoints = pRow->rewardValue[field];

                if (!repPoints)
                    continue;

                repPoints = CalculateReputationGain(REPUTATION_SOURCE_QUEST, repPoints, pQuest->RewRepFaction[i], GetQuestLevelForPlayer(pQuest));

                if (const FactionEntry* factionEntry = sFactionStore.LookupEntry(pQuest->RewRepFaction[i]))
                    GetReputationMgr().ModifyReputation(factionEntry, repPoints);
            }
        }
    }

    // TODO: implement reputation spillover
}

void Player::UpdateArenaFields(void)
{
    /* arena calcs go here */
}

void Player::UpdateHonorFields()
{
    /// called when rewarding honor and at each save
    time_t now = time(NULL);
    time_t today = (time(NULL) / DAY) * DAY;

    if(m_lastHonorUpdateTime < today)
    {
        time_t yesterday = today - DAY;

        uint16 kills_today = PAIR32_LOPART(GetUInt32Value(PLAYER_FIELD_KILLS));

        // update yesterday's contribution
        if(m_lastHonorUpdateTime >= yesterday )
        {
            SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, GetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION));

            // this is the first update today, reset today's contribution
            SetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, 0);
            SetUInt32Value(PLAYER_FIELD_KILLS, MAKE_PAIR32(0,kills_today));
        }
        else
        {
            // no honor/kills yesterday or today, reset
            SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, 0);
            SetUInt32Value(PLAYER_FIELD_KILLS, 0);
        }
    }

    m_lastHonorUpdateTime = now;
}

///Calculate the amount of honor gained based on the victim
///and the size of the group for which the honor is divided
///An exact honor value can also be given (overriding the calcs)
bool Player::RewardHonor(Unit *uVictim, uint32 groupsize, float honor)
{
    // do not reward honor in arenas, but enable onkill spellproc
    if (InArena())
    {
        if (!uVictim || uVictim == this || uVictim->GetTypeId() != TYPEID_PLAYER)
            return false;

        if (GetBGTeam() == ((Player*)uVictim)->GetBGTeam())
            return false;

        return true;
    }

    // 'Inactive' this aura prevents the player from gaining honor points and battleground tokens
    if (GetDummyAura(SPELL_AURA_PLAYER_INACTIVE))
        return false;

    ObjectGuid victim_guid;
    uint32 victim_rank = 0;

    // need call before fields update to have chance move yesterday data to appropriate fields before today data change.
    UpdateHonorFields();

    if (honor <= 0)
    {
        if (!uVictim || uVictim == this || uVictim->HasAuraType(SPELL_AURA_NO_PVP_CREDIT))
            return false;

        victim_guid = uVictim->GetObjectGuid();

        if (uVictim->GetTypeId() == TYPEID_PLAYER)
        {
            Player *pVictim = (Player *)uVictim;

            if (GetTeam() == pVictim->GetTeam() && !sWorld.IsFFAPvPRealm())
                return false;

            float f = 1;                                    //need for total kills (?? need more info)
            uint32 k_grey = 0;
            uint32 k_level = getLevel();
            uint32 v_level = pVictim->getLevel();

            {
                // PLAYER_CHOSEN_TITLE VALUES DESCRIPTION
                //  [0]      Just name
                //  [1..14]  Alliance honor titles and player name
                //  [15..28] Horde honor titles and player name
                //  [29..38] Other title and player name
                //  [39+]    Nothing
                uint32 victim_title = pVictim->GetUInt32Value(PLAYER_CHOSEN_TITLE);
                                                            // Get Killer titles, CharTitlesEntry::bit_index
                // Ranks:
                //  title[1..14]  -> rank[5..18]
                //  title[15..28] -> rank[5..18]
                //  title[other]  -> 0
                if (victim_title == 0)
                    victim_guid.Clear();                    // Don't show HK: <rank> message, only log.
                else if (victim_title < 15)
                    victim_rank = victim_title + 4;
                else if (victim_title < 29)
                    victim_rank = victim_title - 14 + 4;
                else
                    victim_guid.Clear();                    // Don't show HK: <rank> message, only log.
            }

            k_grey = MaNGOS::XP::GetGrayLevel(k_level);

            if (v_level<=k_grey)
                return false;

            float diff_level = (k_level == k_grey) ? 1 : ((float(v_level) - float(k_grey)) / (float(k_level) - float(k_grey)));

            int32 v_rank =1;                                //need more info

            honor = ((f * diff_level * (190 + v_rank*10))/6);
            honor *= float(k_level) / 70.0f;                //factor of dependence on levels of the killer

            // count the number of playerkills in one day
            ApplyModUInt32Value(PLAYER_FIELD_KILLS, 1, true);
            // and those in a lifetime
            ApplyModUInt32Value(PLAYER_FIELD_LIFETIME_HONORBALE_KILLS, 1, true);
            UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL);
            UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS, pVictim->getClass());
            UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HK_RACE, pVictim->getRace());
        }
        else
        {
            Creature *cVictim = (Creature *)uVictim;

            if (!cVictim->IsRacialLeader())
                return false;

            honor = 100;                                    // ??? need more info
            victim_rank = 19;                               // HK: Leader
        }
    }

    if (uVictim != NULL)
    {
        honor *= sWorld.getConfig(CONFIG_FLOAT_RATE_HONOR);
        honor *= (GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HONOR_GAIN) + 100.0f)/100.0f;

        if (groupsize > 1)
            honor /= groupsize;

        honor *= (((float)urand(8,12))/10);                 // approx honor: 80% - 120% of real honor
    }

    // honor - for show honor points in log
    // victim_guid - for show victim name in log
    // victim_rank [1..4]  HK: <dishonored rank>
    // victim_rank [5..19] HK: <alliance\horde rank>
    // victim_rank [0,20+] HK: <>
    WorldPacket data(SMSG_PVP_CREDIT, 4 + 8 + 4);
    data << uint32(honor);
    data << ObjectGuid(victim_guid);
    data << uint32(victim_rank);
    GetSession()->SendPacket(&data);

    // add honor points
    ModifyHonorPoints(int32(honor));

    ApplyModUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, uint32(honor), true);
    return true;
}

void Player::SetHonorPoints(uint32 value)
{
    if (value > sWorld.getConfig(CONFIG_UINT32_MAX_HONOR_POINTS))
        value = sWorld.getConfig(CONFIG_UINT32_MAX_HONOR_POINTS);

    SetUInt32Value(PLAYER_FIELD_HONOR_CURRENCY, value);
}

void Player::SetArenaPoints(uint32 value)
{
    if (value > sWorld.getConfig(CONFIG_UINT32_MAX_ARENA_POINTS))
        value = sWorld.getConfig(CONFIG_UINT32_MAX_ARENA_POINTS);

    SetUInt32Value(PLAYER_FIELD_ARENA_CURRENCY, value);
}

void Player::ModifyHonorPoints(int32 value)
{
    int32 newValue = (int32)GetHonorPoints() + value;

    if (newValue < 0)
        newValue = 0;

    SetHonorPoints(newValue);
}

void Player::ModifyArenaPoints(int32 value)
{
    int32 newValue = (int32)GetArenaPoints() + value;

    if (newValue < 0)
        newValue = 0;

    SetArenaPoints(newValue);
}

uint32 Player::GetGuildIdFromDB(ObjectGuid guid)
{
    uint32 lowguid = guid.GetCounter();

    QueryResult* result = CharacterDatabase.PQuery("SELECT guildid FROM guild_member WHERE guid='%u'", lowguid);
    if (!result)
        return 0;

    uint32 id = result->Fetch()[0].GetUInt32();
    delete result;
    return id;
}

uint32 Player::GetRankFromDB(ObjectGuid guid)
{
    QueryResult *result = CharacterDatabase.PQuery("SELECT rank FROM guild_member WHERE guid='%u'", guid.GetCounter());
    if (result)
    {
        uint32 v = result->Fetch()[0].GetUInt32();
        delete result;
        return v;
    }
    else
        return 0;
}

uint32 Player::GetArenaTeamIdFromDB(ObjectGuid guid, ArenaType type)
{
    QueryResult *result = CharacterDatabase.PQuery("SELECT arena_team_member.arenateamid FROM arena_team_member JOIN arena_team ON arena_team_member.arenateamid = arena_team.arenateamid WHERE guid='%u' AND type='%u' LIMIT 1", guid.GetCounter(), type);
    if (!result)
        return 0;

    uint32 id = (*result)[0].GetUInt32();
    delete result;
    return id;
}

uint32 Player::GetZoneIdFromDB(ObjectGuid guid)
{
    uint32 lowguid = guid.GetCounter();
    QueryResult *result = CharacterDatabase.PQuery("SELECT zone FROM characters WHERE guid='%u'", lowguid);
    if (!result)
        return 0;
    Field* fields = result->Fetch();
    uint32 zone = fields[0].GetUInt32();
    delete result;

    if (!zone)
    {
        // stored zone is zero, use generic and slow zone detection
        result = CharacterDatabase.PQuery("SELECT map,position_x,position_y,position_z FROM characters WHERE guid='%u'", lowguid);
        if( !result )
            return 0;
        fields = result->Fetch();
        uint32 map = fields[0].GetUInt32();
        float posx = fields[1].GetFloat();
        float posy = fields[2].GetFloat();
        float posz = fields[3].GetFloat();
        delete result;

        zone = sTerrainMgr.GetZoneId(map,posx,posy,posz);

        if (zone > 0)
            CharacterDatabase.PExecute("UPDATE characters SET zone='%u' WHERE guid='%u'", zone, lowguid);
    }

    return zone;
}

uint32 Player::GetLevelFromDB(ObjectGuid guid)
{
    uint32 lowguid = guid.GetCounter();

    QueryResult *result = CharacterDatabase.PQuery("SELECT level FROM characters WHERE guid='%u'", lowguid);
    if (!result)
        return 0;

    Field* fields = result->Fetch();
    uint32 level = fields[0].GetUInt32();
    delete result;

    return level;
}

void Player::UpdateArea(uint32 newArea)
{
    m_areaUpdateId    = newArea;

    AreaTableEntry const* area = GetAreaEntryByAreaID(newArea);

    // FFA_PVP flags are area and not zone id dependent
    // so apply them accordingly
    if (area && (area->flags & AREA_FLAG_ARENA))
    {
        if (!isGameMaster())
            SetFFAPvP(true);
    }
    else
    {
        // remove ffa flag only if not ffapvp realm
        // removal in sanctuaries and capitals is handled in zone update
        if(IsFFAPvP() && !sWorld.IsFFAPvPRealm())
            SetFFAPvP(false);
    }

    if (area)
    {
        // Dalaran restricted flight zone
        if ((area->flags & AREA_FLAG_CANNOT_FLY) && IsFreeFlying() && !isGameMaster() && !HasAura(58600))
            CastSpell(this, 58600, true);                   // Restricted Flight Area

        // TODO: implement wintergrasp parachute when battle in progress
        /* if ((area->flags & AREA_FLAG_OUTDOOR_PVP) && IsFreeFlying() && <WINTERGRASP_BATTLE_IN_PROGRESS> && !isGameMaster())
            CastSpell(this, 58730, true); */
    }

    UpdateAreaDependentAuras();
}

void Player::UpdateZone(uint32 newZone, uint32 newArea)
{
    AreaTableEntry const* zone = GetAreaEntryByAreaID(newZone);
    if(!zone)
        return;

    if(m_zoneUpdateId != newZone)
    {
        SendInitWorldStates(newZone, newArea);              // only if really enters to new zone, not just area change, works strange...

        if (sWorld.getConfig(CONFIG_BOOL_WEATHER))
        {
            if(Weather *wth = sWorld.FindWeather(zone->ID))
                wth->SendWeatherUpdateToPlayer(this);
            else if(!sWorld.AddWeather(zone->ID))
            {
                // send fine weather packet to remove old zone's weather
                Weather::SendFineWeatherUpdateToPlayer(this);
            }
        }
    }

    m_zoneUpdateId    = newZone;
    m_zoneUpdateTimer = ZONE_UPDATE_INTERVAL;

    // zone changed, so area changed as well, update it
    UpdateArea(newArea);

    // in PvP, any not controlled zone (except zone->team == 6, default case)
    // in PvE, only opposition team capital
    switch(zone->team)
    {
        case AREATEAM_ALLY:
            pvpInfo.inHostileArea = GetTeam() != ALLIANCE && (sWorld.IsPvPRealm() || zone->flags & AREA_FLAG_CAPITAL);
            break;
        case AREATEAM_HORDE:
            pvpInfo.inHostileArea = GetTeam() != HORDE && (sWorld.IsPvPRealm() || zone->flags & AREA_FLAG_CAPITAL);
            break;
        case AREATEAM_NONE:
            // overwrite for battlegrounds, maybe batter some zone flags but current known not 100% fit to this
            pvpInfo.inHostileArea = sWorld.IsPvPRealm() || InBattleGround();
            break;
        default:                                            // 6 in fact
            pvpInfo.inHostileArea = false;
            break;
    }

    if(pvpInfo.inHostileArea)                               // in hostile area
    {
        if(!IsPvP() || pvpInfo.endTimer != 0)
            UpdatePvP(true, true);
    }
    else                                                    // in friendly area
    {
        if(IsPvP() && !HasFlag(PLAYER_FLAGS,PLAYER_FLAGS_IN_PVP) && pvpInfo.endTimer == 0)
            pvpInfo.endTimer = time(0);                     // start toggle-off
    }

    if(zone->flags & AREA_FLAG_SANCTUARY)                   // in sanctuary
    {
        SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY);
        if(sWorld.IsFFAPvPRealm())
            SetFFAPvP(false);
    }
    else
    {
        RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY);
    }

    if(zone->flags & AREA_FLAG_CAPITAL)                     // in capital city
        SetRestType(REST_TYPE_IN_CITY);
    else if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING) && GetRestType() != REST_TYPE_IN_TAVERN)
        // resting and not in tavern (leave city then); tavern leave handled in CheckAreaExploreAndOutdoor
        SetRestType(REST_TYPE_NO);

    // remove items with area/map limitations (delete only for alive player to allow back in ghost mode)
    // if player resurrected at teleport this will be applied in resurrect code
    if(isAlive())
        DestroyZoneLimitedItem( true, newZone );

    // check some item equip limitations (in result lost CanTitanGrip at talent reset, for example)
    AutoUnequipOffhandIfNeed();

    // recent client version not send leave/join channel packets for built-in local channels
    UpdateLocalChannels( newZone );

    // group update
    if(GetGroup())
        SetGroupUpdateFlag(GROUP_UPDATE_FLAG_ZONE);

    UpdateZoneDependentAuras();
    UpdateZoneDependentPets();
}

//If players are too far way of duel flag... then player loose the duel
void Player::CheckDuelDistance(time_t currTime)
{
    if (!duel)
        return;

    GameObject* obj = GetMap()->GetGameObject(GetGuidValue(PLAYER_DUEL_ARBITER));
    if (!obj)
    {
        // player not at duel start map
        DuelComplete(DUEL_FLED);
        return;
    }

    if (duel->outOfBound == 0)
    {
        if (!IsWithinDistInMap(obj, 50))
        {
            duel->outOfBound = currTime;

            WorldPacket data(SMSG_DUEL_OUTOFBOUNDS, 0);
            GetSession()->SendPacket(&data);
        }
    }
    else
    {
        if (IsWithinDistInMap(obj, 40))
        {
            duel->outOfBound = 0;

            WorldPacket data(SMSG_DUEL_INBOUNDS, 0);
            GetSession()->SendPacket(&data);
        }
        else if (currTime >= (duel->outOfBound+10))
        {
            DuelComplete(DUEL_FLED);
        }
    }
}

void Player::DuelComplete(DuelCompleteType type)
{
    // duel not requested
    if (!duel)
        return;

    WorldPacket data(SMSG_DUEL_COMPLETE, (1));
    data << (uint8)((type != DUEL_INTERUPTED) ? 1 : 0);
    GetSession()->SendPacket(&data);
    duel->opponent->GetSession()->SendPacket(&data);

    if (type != DUEL_INTERUPTED)
    {
        data.Initialize(SMSG_DUEL_WINNER, (1+20));          // we guess size
        data << (uint8)((type==DUEL_WON) ? 0 : 1);          // 0 = just won; 1 = fled
        data << duel->opponent->GetName();
        data << GetName();
        SendMessageToSet(&data,true);
    }

    if (type == DUEL_WON)
    {
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL, 1);
        if (duel->opponent)
            duel->opponent->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL, 1);
    }

    //Remove Duel Flag object
    if (GameObject* obj = GetMap()->GetGameObject(GetGuidValue(PLAYER_DUEL_ARBITER)))
        duel->initiator->RemoveGameObject(obj, true);

    /* remove auras */
    std::vector<uint32> auras2remove;
    SpellAuraHolderMap const& vAuras = duel->opponent->GetSpellAuraHolderMap();
    for (SpellAuraHolderMap::const_iterator i = vAuras.begin(); i != vAuras.end(); ++i)
    {
        if (!i->second->IsPositive() && i->second->GetCasterGuid() == GetObjectGuid() && i->second->GetAuraApplyTime() >= duel->startTime)
            auras2remove.push_back(i->second->GetId());
    }

    for(size_t i=0; i<auras2remove.size(); ++i)
        duel->opponent->RemoveAurasDueToSpell(auras2remove[i]);

    auras2remove.clear();
    SpellAuraHolderMap const& auras = GetSpellAuraHolderMap();
    for (SpellAuraHolderMap::const_iterator i = auras.begin(); i != auras.end(); ++i)
    {
        if (!i->second->IsPositive() && i->second->GetCasterGuid() == duel->opponent->GetObjectGuid() && i->second->GetAuraApplyTime() >= duel->startTime)
            auras2remove.push_back(i->second->GetId());
    }
    for (size_t i=0; i<auras2remove.size(); ++i)
        RemoveAurasDueToSpell(auras2remove[i]);

    // cleanup combo points
    if (GetComboTargetGuid() == duel->opponent->GetObjectGuid())
        ClearComboPoints();
    else if (GetComboTargetGuid() == duel->opponent->GetPetGuid())
        ClearComboPoints();

    if (duel->opponent->GetComboTargetGuid() == GetObjectGuid())
        duel->opponent->ClearComboPoints();
    else if (duel->opponent->GetComboTargetGuid() == GetPetGuid())
        duel->opponent->ClearComboPoints();

    //cleanups
    SetGuidValue(PLAYER_DUEL_ARBITER, ObjectGuid());
    SetUInt32Value(PLAYER_DUEL_TEAM, 0);
    duel->opponent->SetGuidValue(PLAYER_DUEL_ARBITER, ObjectGuid());
    duel->opponent->SetUInt32Value(PLAYER_DUEL_TEAM, 0);

    delete duel->opponent->duel;
    duel->opponent->duel = NULL;
    delete duel;
    duel = NULL;
}

//---------------------------------------------------------//

void Player::_ApplyItemMods(Item *item, uint8 slot,bool apply)
{
    if(slot >= INVENTORY_SLOT_BAG_END || !item)
        return;

    // not apply/remove mods for broken item
    if(item->IsBroken())
        return;

    ItemPrototype const *proto = item->GetProto();

    if(!proto)
        return;

    DETAIL_LOG("applying mods for item %u ",item->GetGUIDLow());

    uint32 attacktype = Player::GetAttackBySlot(slot);
    if(attacktype < MAX_ATTACK)
        _ApplyWeaponDependentAuraMods(item,WeaponAttackType(attacktype),apply);

    _ApplyItemBonuses(proto,slot,apply);

    if( slot==EQUIPMENT_SLOT_RANGED )
        _ApplyAmmoBonuses();

    ApplyItemEquipSpell(item,apply);
    ApplyEnchantment(item, apply);

    if(proto->Socket[0].Color)                              //only (un)equipping of items with sockets can influence metagems, so no need to waste time with normal items
        CorrectMetaGemEnchants(slot, apply);

    DEBUG_LOG("_ApplyItemMods complete.");
}

void Player::_ApplyItemBonuses(ItemPrototype const *proto, uint8 slot, bool apply, bool only_level_scale /*= false*/)
{
    if (slot >= INVENTORY_SLOT_BAG_END || !proto)
        return;

    ScalingStatDistributionEntry const *ssd = proto->ScalingStatDistribution ? sScalingStatDistributionStore.LookupEntry(proto->ScalingStatDistribution) : NULL;
    if (only_level_scale && !ssd)
        return;

    // req. check at equip, but allow use for extended range if range limit max level, set proper level
    uint32 ssd_level = getLevel();
    if (ssd && ssd_level > ssd->MaxLevel)
        ssd_level = ssd->MaxLevel;

    ScalingStatValuesEntry const *ssv = proto->ScalingStatValue ? sScalingStatValuesStore.LookupEntry(ssd_level) : NULL;
    if (only_level_scale && !ssv)
        return;

    for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
    {
        uint32 statType = 0;
        int32  val = 0;
        // If set ScalingStatDistribution need get stats and values from it
        if (ssd && ssv)
        {
            if (ssd->StatMod[i] < 0)
                continue;
            statType = ssd->StatMod[i];
            val = (ssv->getssdMultiplier(proto->ScalingStatValue) * ssd->Modifier[i]) / 10000;
        }
        else
        {
            if (i >= proto->StatsCount)
                continue;
            statType = proto->ItemStat[i].ItemStatType;
            val = proto->ItemStat[i].ItemStatValue;
        }

        if(val == 0)
            continue;

        switch (statType)
        {
            case ITEM_MOD_MANA:
                HandleStatModifier(UNIT_MOD_MANA, BASE_VALUE, float(val), apply);
                break;
            case ITEM_MOD_HEALTH:                           // modify HP
                HandleStatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(val), apply);
                break;
            case ITEM_MOD_AGILITY:                          // modify agility
                HandleStatModifier(UNIT_MOD_STAT_AGILITY, BASE_VALUE, float(val), apply);
                ApplyStatBuffMod(STAT_AGILITY, float(val), apply);
                break;
            case ITEM_MOD_STRENGTH:                         //modify strength
                HandleStatModifier(UNIT_MOD_STAT_STRENGTH, BASE_VALUE, float(val), apply);
                ApplyStatBuffMod(STAT_STRENGTH, float(val), apply);
                break;
            case ITEM_MOD_INTELLECT:                        //modify intellect
                HandleStatModifier(UNIT_MOD_STAT_INTELLECT, BASE_VALUE, float(val), apply);
                ApplyStatBuffMod(STAT_INTELLECT, float(val), apply);
                break;
            case ITEM_MOD_SPIRIT:                           //modify spirit
                HandleStatModifier(UNIT_MOD_STAT_SPIRIT, BASE_VALUE, float(val), apply);
                ApplyStatBuffMod(STAT_SPIRIT, float(val), apply);
                break;
            case ITEM_MOD_STAMINA:                          //modify stamina
                HandleStatModifier(UNIT_MOD_STAT_STAMINA, BASE_VALUE, float(val), apply);
                ApplyStatBuffMod(STAT_STAMINA, float(val), apply);
                break;
            case ITEM_MOD_DEFENSE_SKILL_RATING:
                ApplyRatingMod(CR_DEFENSE_SKILL, int32(val), apply);
                break;
            case ITEM_MOD_DODGE_RATING:
                ApplyRatingMod(CR_DODGE, int32(val), apply);
                break;
            case ITEM_MOD_PARRY_RATING:
                ApplyRatingMod(CR_PARRY, int32(val), apply);
                break;
            case ITEM_MOD_BLOCK_RATING:
                ApplyRatingMod(CR_BLOCK, int32(val), apply);
                break;
            case ITEM_MOD_HIT_MELEE_RATING:
                ApplyRatingMod(CR_HIT_MELEE, int32(val), apply);
                break;
            case ITEM_MOD_HIT_RANGED_RATING:
                ApplyRatingMod(CR_HIT_RANGED, int32(val), apply);
                break;
            case ITEM_MOD_HIT_SPELL_RATING:
                ApplyRatingMod(CR_HIT_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_MELEE_RATING:
                ApplyRatingMod(CR_CRIT_MELEE, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_RANGED_RATING:
                ApplyRatingMod(CR_CRIT_RANGED, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_SPELL_RATING:
                ApplyRatingMod(CR_CRIT_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_HIT_TAKEN_MELEE_RATING:
                ApplyRatingMod(CR_HIT_TAKEN_MELEE, int32(val), apply);
                break;
            case ITEM_MOD_HIT_TAKEN_RANGED_RATING:
                ApplyRatingMod(CR_HIT_TAKEN_RANGED, int32(val), apply);
                break;
            case ITEM_MOD_HIT_TAKEN_SPELL_RATING:
                ApplyRatingMod(CR_HIT_TAKEN_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_TAKEN_MELEE_RATING:
                ApplyRatingMod(CR_CRIT_TAKEN_MELEE, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_TAKEN_RANGED_RATING:
                ApplyRatingMod(CR_CRIT_TAKEN_RANGED, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_TAKEN_SPELL_RATING:
                ApplyRatingMod(CR_CRIT_TAKEN_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_HASTE_MELEE_RATING:
                ApplyRatingMod(CR_HASTE_MELEE, int32(val), apply);
                break;
            case ITEM_MOD_HASTE_RANGED_RATING:
                ApplyRatingMod(CR_HASTE_RANGED, int32(val), apply);
                break;
            case ITEM_MOD_HASTE_SPELL_RATING:
                ApplyRatingMod(CR_HASTE_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_HIT_RATING:
                ApplyRatingMod(CR_HIT_MELEE, int32(val), apply);
                ApplyRatingMod(CR_HIT_RANGED, int32(val), apply);
                ApplyRatingMod(CR_HIT_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_RATING:
                ApplyRatingMod(CR_CRIT_MELEE, int32(val), apply);
                ApplyRatingMod(CR_CRIT_RANGED, int32(val), apply);
                ApplyRatingMod(CR_CRIT_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_HIT_TAKEN_RATING:
                ApplyRatingMod(CR_HIT_TAKEN_MELEE, int32(val), apply);
                ApplyRatingMod(CR_HIT_TAKEN_RANGED, int32(val), apply);
                ApplyRatingMod(CR_HIT_TAKEN_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_CRIT_TAKEN_RATING:
                ApplyRatingMod(CR_CRIT_TAKEN_MELEE, int32(val), apply);
                ApplyRatingMod(CR_CRIT_TAKEN_RANGED, int32(val), apply);
                ApplyRatingMod(CR_CRIT_TAKEN_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_RESILIENCE_RATING:
                ApplyRatingMod(CR_CRIT_TAKEN_MELEE, int32(val), apply);
                ApplyRatingMod(CR_CRIT_TAKEN_RANGED, int32(val), apply);
                ApplyRatingMod(CR_CRIT_TAKEN_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_HASTE_RATING:
                ApplyRatingMod(CR_HASTE_MELEE, int32(val), apply);
                ApplyRatingMod(CR_HASTE_RANGED, int32(val), apply);
                ApplyRatingMod(CR_HASTE_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_EXPERTISE_RATING:
                ApplyRatingMod(CR_EXPERTISE, int32(val), apply);
                break;
            case ITEM_MOD_ATTACK_POWER:
                HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(val), apply);
                HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(val), apply);
                break;
            case ITEM_MOD_RANGED_ATTACK_POWER:
                HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(val), apply);
                break;
            case ITEM_MOD_MANA_REGENERATION:
                ApplyManaRegenBonus(int32(val), apply);
                break;
            case ITEM_MOD_ARMOR_PENETRATION_RATING:
                ApplyRatingMod(CR_ARMOR_PENETRATION, int32(val), apply);
                break;
            case ITEM_MOD_SPELL_POWER:
                ApplySpellPowerBonus(int32(val), apply);
                break;
            case ITEM_MOD_SPELL_PENETRATION:
                ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE, -int32(val), apply);
                m_spellPenetrationItemMod += apply ? val : -val;
                break;
            case ITEM_MOD_BLOCK_VALUE:
                HandleBaseModValue(SHIELD_BLOCK_VALUE, FLAT_MOD, float(val), apply);
                break;
            // deprecated item mods
            case ITEM_MOD_FERAL_ATTACK_POWER:
            case ITEM_MOD_SPELL_HEALING_DONE:
            case ITEM_MOD_SPELL_DAMAGE_DONE:
                break;
        }
    }

    // Apply Spell Power from ScalingStatValue if set
    if(ssv)
    {
        if (int32 spellbonus = ssv->getSpellBonus(proto->ScalingStatValue))
            ApplySpellPowerBonus(spellbonus, apply);
    }

    // If set ScalingStatValue armor get it or use item armor
    uint32 armor = proto->Armor;
    if (ssv)
    {
        if (uint32 ssvarmor = ssv->getArmorMod(proto->ScalingStatValue))
            armor = ssvarmor;
    }
    // Add armor bonus from ArmorDamageModifier if > 0
    if (proto->ArmorDamageModifier > 0)
        armor += uint32(proto->ArmorDamageModifier);

    if (armor)
    {
        switch(proto->InventoryType)
        {
            case INVTYPE_TRINKET:
            case INVTYPE_NECK:
            case INVTYPE_CLOAK:
            case INVTYPE_FINGER:
                HandleStatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, float(armor), apply);
                break;
            default:
                HandleStatModifier(UNIT_MOD_ARMOR, BASE_VALUE, float(armor), apply);
                break;
        }
    }

    if (proto->Block)
        HandleBaseModValue(SHIELD_BLOCK_VALUE, FLAT_MOD, float(proto->Block), apply);

    if (proto->HolyRes)
        HandleStatModifier(UNIT_MOD_RESISTANCE_HOLY, BASE_VALUE, float(proto->HolyRes), apply);

    if (proto->FireRes)
        HandleStatModifier(UNIT_MOD_RESISTANCE_FIRE, BASE_VALUE, float(proto->FireRes), apply);

    if (proto->NatureRes)
        HandleStatModifier(UNIT_MOD_RESISTANCE_NATURE, BASE_VALUE, float(proto->NatureRes), apply);

    if (proto->FrostRes)
        HandleStatModifier(UNIT_MOD_RESISTANCE_FROST, BASE_VALUE, float(proto->FrostRes), apply);

    if (proto->ShadowRes)
        HandleStatModifier(UNIT_MOD_RESISTANCE_SHADOW, BASE_VALUE, float(proto->ShadowRes), apply);

    if (proto->ArcaneRes)
        HandleStatModifier(UNIT_MOD_RESISTANCE_ARCANE, BASE_VALUE, float(proto->ArcaneRes), apply);

    WeaponAttackType attType = BASE_ATTACK;
    float damage = 0.0f;

    if( slot == EQUIPMENT_SLOT_RANGED && (
        proto->InventoryType == INVTYPE_RANGED || proto->InventoryType == INVTYPE_THROWN ||
        proto->InventoryType == INVTYPE_RANGEDRIGHT ))
    {
        attType = RANGED_ATTACK;
    }
    else if(slot==EQUIPMENT_SLOT_OFFHAND)
    {
        attType = OFF_ATTACK;
    }

    float minDamage = proto->Damage[0].DamageMin;
    float maxDamage = proto->Damage[0].DamageMax;
    int32 extraDPS = 0;
    // If set dpsMod in ScalingStatValue use it for min (70% from average), max (130% from average) damage
    if (ssv)
    {
        if ((extraDPS = ssv->getDPSMod(proto->ScalingStatValue)))
        {
            float average = extraDPS * proto->Delay / 1000.0f;
            minDamage = 0.7f * average;
            maxDamage = 1.3f * average;
        }
    }
    if (minDamage > 0 )
    {
        damage = apply ? minDamage : BASE_MINDAMAGE;
        SetBaseWeaponDamage(attType, MINDAMAGE, damage);
        //sLog.outError("applying mindam: assigning %f to weapon mindamage, now is: %f", damage, GetWeaponDamageRange(attType, MINDAMAGE));
    }

    if (maxDamage  > 0 )
    {
        damage = apply ? maxDamage : BASE_MAXDAMAGE;
        SetBaseWeaponDamage(attType, MAXDAMAGE, damage);
    }

    // Apply feral bonus from ScalingStatValue if set
    if (ssv)
    {
        if (int32 feral_bonus = ssv->getFeralBonus(proto->ScalingStatValue))
            ApplyFeralAPBonus(feral_bonus, apply);
    }
    // Druids get feral AP bonus from weapon dps (also use DPS from ScalingStatValue)
    if (getClass() == CLASS_DRUID)
    {
        int32 feral_bonus = proto->getFeralBonus(extraDPS);
        if (feral_bonus > 0)
            ApplyFeralAPBonus(feral_bonus, apply);
    }

    if (!CanUseEquippedWeapon(attType))
        return;

    if (proto->Delay)
    {
        if(slot == EQUIPMENT_SLOT_RANGED)
            SetAttackTime(RANGED_ATTACK, apply ? proto->Delay: BASE_ATTACK_TIME);
        else if(slot==EQUIPMENT_SLOT_MAINHAND)
            SetAttackTime(BASE_ATTACK, apply ? proto->Delay: BASE_ATTACK_TIME);
        else if(slot==EQUIPMENT_SLOT_OFFHAND)
            SetAttackTime(OFF_ATTACK, apply ? proto->Delay: BASE_ATTACK_TIME);
    }

    if(CanModifyStats() && (damage || proto->Delay))
        UpdateDamagePhysical(attType);
}

void Player::_ApplyWeaponDependentAuraMods(Item *item,WeaponAttackType attackType,bool apply)
{
    AuraList const& auraCritList = GetAurasByType(SPELL_AURA_MOD_CRIT_PERCENT);
    for(AuraList::const_iterator itr = auraCritList.begin(); itr!=auraCritList.end();++itr)
        _ApplyWeaponDependentAuraCritMod(item,attackType,*itr,apply);

    AuraList const& auraDamageFlatList = GetAurasByType(SPELL_AURA_MOD_DAMAGE_DONE);
    for(AuraList::const_iterator itr = auraDamageFlatList.begin(); itr!=auraDamageFlatList.end();++itr)
        _ApplyWeaponDependentAuraDamageMod(item,attackType,*itr,apply);

    AuraList const& auraDamagePCTList = GetAurasByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
    for(AuraList::const_iterator itr = auraDamagePCTList.begin(); itr!=auraDamagePCTList.end();++itr)
        _ApplyWeaponDependentAuraDamageMod(item,attackType,*itr,apply);
}

void Player::_ApplyWeaponDependentAuraCritMod(Item *item, WeaponAttackType attackType, Aura* aura, bool apply)
{
    // generic not weapon specific case processes in aura code
    if(aura->GetSpellProto()->EquippedItemClass == -1)
        return;

    BaseModGroup mod = BASEMOD_END;
    switch(attackType)
    {
        case BASE_ATTACK:   mod = CRIT_PERCENTAGE;        break;
        case OFF_ATTACK:    mod = OFFHAND_CRIT_PERCENTAGE;break;
        case RANGED_ATTACK: mod = RANGED_CRIT_PERCENTAGE; break;
        default: return;
    }

    if (item->IsFitToSpellRequirements(aura->GetSpellProto()))
    {
        HandleBaseModValue(mod, FLAT_MOD, float (aura->GetModifier()->m_amount), apply);
    }
}

void Player::_ApplyWeaponDependentAuraDamageMod(Item *item, WeaponAttackType attackType, Aura* aura, bool apply)
{
    // ignore spell mods for not wands
    Modifier const* modifier = aura->GetModifier();
    if((modifier->m_miscvalue & SPELL_SCHOOL_MASK_NORMAL)==0 && (getClassMask() & CLASSMASK_WAND_USERS)==0)
        return;

    // generic not weapon specific case processes in aura code
    if(aura->GetSpellProto()->EquippedItemClass == -1)
        return;

    UnitMods unitMod = UNIT_MOD_END;
    switch(attackType)
    {
        case BASE_ATTACK:   unitMod = UNIT_MOD_DAMAGE_MAINHAND; break;
        case OFF_ATTACK:    unitMod = UNIT_MOD_DAMAGE_OFFHAND;  break;
        case RANGED_ATTACK: unitMod = UNIT_MOD_DAMAGE_RANGED;   break;
        default: return;
    }

    UnitModifierType unitModType = TOTAL_VALUE;
    switch(modifier->m_auraname)
    {
        case SPELL_AURA_MOD_DAMAGE_DONE:         unitModType = TOTAL_VALUE; break;
        case SPELL_AURA_MOD_DAMAGE_PERCENT_DONE: unitModType = TOTAL_PCT;   break;
        default: return;
    }

    if (item->IsFitToSpellRequirements(aura->GetSpellProto()))
    {
        HandleStatModifier(unitMod, unitModType, float(modifier->m_amount),apply);
    }
}

void Player::ApplyItemEquipSpell(Item *item, bool apply, bool form_change)
{
    if(!item)
        return;

    ItemPrototype const *proto = item->GetProto();
    if(!proto)
        return;

    for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        _Spell const& spellData = proto->Spells[i];

        // no spell
        if(!spellData.SpellId )
            continue;

        if (apply)
        {
            // apply only at-equip spells
            if (spellData.SpellTrigger != ITEM_SPELLTRIGGER_ON_EQUIP)
                continue;
        }
        else
        {
            // at un-apply remove all spells (not only at-apply, so any at-use active affects from item and etc)
            // except with at-use with negative charges, so allow consuming item spells (including with extra flag that prevent consume really)
            // applied to player after item remove from equip slot
            if (spellData.SpellTrigger == ITEM_SPELLTRIGGER_ON_USE && spellData.SpellCharges < 0)
                continue;
        }

        // check if it is valid spell
        SpellEntry const* spellproto = sSpellStore.LookupEntry(spellData.SpellId);
        if(!spellproto)
            continue;

        ApplyEquipSpell(spellproto,item,apply,form_change);
    }
}

void Player::ApplyEquipSpell(SpellEntry const* spellInfo, Item* item, bool apply, bool form_change)
{
    if(apply)
    {
        // Cannot be used in this stance/form
        if (GetErrorAtShapeshiftedCast(spellInfo, GetShapeshiftForm()) != SPELL_CAST_OK)
            return;

        if (form_change)                                    // check aura active state from other form
        {
            bool found = false;
            for (int k=0; k < MAX_EFFECT_INDEX; ++k)
            {
                SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(spellInfo->Id);
                for (SpellAuraHolderMap::const_iterator iter = spair.first; iter != spair.second; ++iter)
                {
                    if(!item || iter->second->GetCastItemGuid() == item->GetObjectGuid())
                    {
                        found = true;
                        break;
                    }
                }
                if(found)
                    break;
            }

            if(found)                                       // and skip re-cast already active aura at form change
                return;
        }

        DEBUG_LOG("WORLD: cast %s Equip spellId - %i", (item ? "item" : "itemset"), spellInfo->Id);

        CastSpell(this,spellInfo,true,item);
    }
    else
    {
        if (form_change)                                    // check aura compatibility
        {
            // Cannot be used in this stance/form
            if (GetErrorAtShapeshiftedCast(spellInfo, GetShapeshiftForm()) == SPELL_CAST_OK)
                return;                                     // and remove only not compatible at form change
        }

        if (item)
            RemoveAurasDueToItemSpell(item,spellInfo->Id);  // un-apply all spells , not only at-equipped
        else
            RemoveAurasDueToSpell(spellInfo->Id);           // un-apply spell (item set case)
    }
}

void Player::UpdateEquipSpellsAtFormChange()
{
    for (int i = 0; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if(m_items[i] && !m_items[i]->IsBroken())
        {
            ApplyItemEquipSpell(m_items[i],false,true);     // remove spells that not fit to form
            ApplyItemEquipSpell(m_items[i],true,true);      // add spells that fit form but not active
        }
    }

    // item set bonuses not dependent from item broken state
    for(size_t setindex = 0; setindex < ItemSetEff.size(); ++setindex)
    {
        ItemSetEffect* eff = ItemSetEff[setindex];
        if(!eff)
            continue;

        for(uint32 y=0;y<8; ++y)
        {
            SpellEntry const* spellInfo = eff->spells[y];
            if(!spellInfo)
                continue;

            ApplyEquipSpell(spellInfo,NULL,false,true);       // remove spells that not fit to form
            ApplyEquipSpell(spellInfo,NULL,true,true);        // add spells that fit form but not active
        }
    }
}

/**
 * (un-)Apply item spells triggered at adding item to inventory ITEM_SPELLTRIGGER_ON_STORE
 *
 * @param item  added/removed item to/from inventory
 * @param apply (un-)apply spell affects.
 *
 * Note: item moved from slot to slot in 2 steps RemoveItem and StoreItem/EquipItem
 * In result function not called in RemoveItem for prevent unexpected re-apply auras from related spells
 * with duration reset and etc. Instead unapply done in StoreItem/EquipItem and in specialized
 * functions for item final remove/destroy from inventory. If new RemoveItem calls added need be sure that
 * function will call after it in some way if need.
 */

void Player::ApplyItemOnStoreSpell(Item *item, bool apply)
{
    if (!item)
        return;

    ItemPrototype const *proto = item->GetProto();
    if (!proto)
        return;

    for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        _Spell const& spellData = proto->Spells[i];

        // no spell
        if (!spellData.SpellId)
            continue;

        // apply/unapply only at-store spells
        if (spellData.SpellTrigger != ITEM_SPELLTRIGGER_ON_STORE)
            continue;

        if (apply)
        {
            // can be attempt re-applied at move in inventory slots
            if (!HasAura(spellData.SpellId))
                CastSpell(this, spellData.SpellId, true, item);
        }
        else
            RemoveAurasDueToItemSpell(item, spellData.SpellId);
    }
}

void Player::DestroyItemWithOnStoreSpell(Item* item, uint32 spellId)
{
    if (!item)
        return;

    ItemPrototype const *proto = item->GetProto();
    if (!proto)
        return;

    for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        _Spell const& spellData = proto->Spells[i];

        if (spellData.SpellId != spellId)
            continue;

        // apply/unapply only at-store spells
        if (spellData.SpellTrigger != ITEM_SPELLTRIGGER_ON_STORE)
            continue;

        DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
        break;
    }
}


/// handles unique effect of Deadly Poison: apply poison of the other weapon when already at max. stack
void Player::_HandleDeadlyPoison(Unit* Target, WeaponAttackType attType, SpellEntry const *spellInfo)
{
    SpellAuraHolder const* dPoison = NULL;
    SpellAuraHolderConstBounds holders = Target->GetSpellAuraHolderBounds(spellInfo->Id);
    for (SpellAuraHolderMap::const_iterator iter = holders.first; iter != holders.second; ++iter)
    {
        if (iter->second->GetCaster() == this)
        {
            dPoison = iter->second;
            break;
        }
    }
    if (dPoison && dPoison->GetStackAmount() == spellInfo->StackAmount)
    {
        Item *otherWeapon = GetWeaponForAttack(attType == BASE_ATTACK ? OFF_ATTACK : BASE_ATTACK );
        if (!otherWeapon)
            return;

        // all poison enchantments are temporary
        uint32 enchant_id = otherWeapon->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT);
        if (!enchant_id)
            return;

        SpellItemEnchantmentEntry const* pSecondEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!pSecondEnchant)
            return;

        for (int s = 0; s < 3; ++s)
        {
            if (pSecondEnchant->type[s] != ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL)
                continue;

            SpellEntry const* combatEntry = sSpellStore.LookupEntry(pSecondEnchant->spellid[s]);
            if (combatEntry && combatEntry->Dispel == DISPEL_POISON)
                CastSpell(Target, combatEntry, true, otherWeapon);
        }
    }
}

void Player::CastItemCombatSpell(Unit* Target, WeaponAttackType attType)
{
    Item *item = GetWeaponForAttack(attType, true, false);
    if(!item)
        return;

    ItemPrototype const *proto = item->GetProto();
    if(!proto)
        return;

    if (!Target || Target == this )
        return;

    for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        _Spell const& spellData = proto->Spells[i];

        // no spell
        if(!spellData.SpellId )
            continue;

        // wrong triggering type
        if(spellData.SpellTrigger != ITEM_SPELLTRIGGER_CHANCE_ON_HIT)
            continue;

        SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellData.SpellId);
        if(!spellInfo)
        {
            sLog.outError("WORLD: unknown Item spellid %i", spellData.SpellId);
            continue;
        }

        // not allow proc extra attack spell at extra attack
        if( m_extraAttacks && IsSpellHaveEffect(spellInfo,SPELL_EFFECT_ADD_EXTRA_ATTACKS) )
            return;

        float chance = (float)spellInfo->procChance;

        if(spellData.SpellPPMRate)
        {
            uint32 WeaponSpeed = proto->Delay;
            chance = GetPPMProcChance(WeaponSpeed, spellData.SpellPPMRate);
        }
        else if(chance > 100.0f)
        {
            chance = GetWeaponProcChance();
        }

        if (roll_chance_f(chance))
            CastSpell(Target, spellInfo->Id, true, item);
    }

    // item combat enchantments
    for(int e_slot = 0; e_slot < MAX_ENCHANTMENT_SLOT; ++e_slot)
    {
        uint32 enchant_id = item->GetEnchantmentId(EnchantmentSlot(e_slot));
        SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if(!pEnchant) continue;
        for (int s = 0; s < 3; ++s)
        {
            uint32 proc_spell_id = pEnchant->spellid[s];

            if (pEnchant->type[s] != ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL)
                continue;

            SpellEntry const *spellInfo = sSpellStore.LookupEntry(proc_spell_id);
            if (!spellInfo)
            {
                sLog.outError("Player::CastItemCombatSpell Enchant %i, cast unknown spell %i", pEnchant->ID, proc_spell_id);
                continue;
            }

            // Use first rank to access spell item enchant procs
            float ppmRate = sSpellMgr.GetItemEnchantProcChance(spellInfo->Id);

            float chance = ppmRate
                ? GetPPMProcChance(proto->Delay, ppmRate)
                : pEnchant->amount[s] != 0 ? float(pEnchant->amount[s]) : GetWeaponProcChance();


            ApplySpellMod(spellInfo->Id,SPELLMOD_CHANCE_OF_SUCCESS, chance);
            ApplySpellMod(spellInfo->Id,SPELLMOD_FREQUENCY_OF_SUCCESS, chance);

            if (roll_chance_f(chance))
            {
                if (IsPositiveSpell(spellInfo->Id))
                    CastSpell(this, spellInfo->Id, true, item);
                else
                {
                    // Deadly Poison, unique effect needs to be handled before casting triggered spell
                    if (spellInfo->SpellFamilyName == SPELLFAMILY_ROGUE && spellInfo->SpellFamilyFlags & UI64LIT(0x10000))
                        _HandleDeadlyPoison(Target, attType, spellInfo);

                    CastSpell(Target, spellInfo->Id, true, item);
                }
            }
        }
    }
}

void Player::CastItemUseSpell(Item *item,SpellCastTargets const& targets,uint8 cast_count, uint32 glyphIndex)
{
    ItemPrototype const* proto = item->GetProto();
    // special learning case
    if (proto->Spells[0].SpellId==SPELL_ID_GENERIC_LEARN || proto->Spells[0].SpellId==SPELL_ID_GENERIC_LEARN_PET)
    {
        uint32 learn_spell_id = proto->Spells[0].SpellId;
        uint32 learning_spell_id = proto->Spells[1].SpellId;

        SpellEntry const *spellInfo = sSpellStore.LookupEntry(learn_spell_id);
        if (!spellInfo)
        {
            sLog.outError("Player::CastItemUseSpell: Item (Entry: %u) in have wrong spell id %u, ignoring ",proto->ItemId, learn_spell_id);
            SendEquipError(EQUIP_ERR_NONE, item);
            return;
        }

        Spell *spell = new Spell(this, spellInfo, false);
        spell->m_CastItem = item;
        spell->m_cast_count = cast_count;                   //set count of casts
        spell->m_currentBasePoints[EFFECT_INDEX_0] = learning_spell_id;
        spell->prepare(&targets);
        return;
    }

    // use triggered flag only for items with many spell casts and for not first cast
    int count = 0;

    // item spells casted at use
    for(int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        _Spell const& spellData = proto->Spells[i];

        // no spell
        if (!spellData.SpellId)
            continue;

        // wrong triggering type
        if (spellData.SpellTrigger != ITEM_SPELLTRIGGER_ON_USE)
            continue;

        SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellData.SpellId);
        if (!spellInfo)
        {
            sLog.outError("Player::CastItemUseSpell: Item (Entry: %u) in have wrong spell id %u, ignoring",proto->ItemId, spellData.SpellId);
            continue;
        }

        Spell *spell = new Spell(this, spellInfo, (count > 0));
        spell->m_CastItem = item;
        spell->m_cast_count = cast_count;                   // set count of casts
        spell->m_glyphIndex = glyphIndex;                   // glyph index
        spell->prepare(&targets);

        ++count;
    }

    // Item enchantments spells casted at use
    for (int e_slot = 0; e_slot < MAX_ENCHANTMENT_SLOT; ++e_slot)
    {
        uint32 enchant_id = item->GetEnchantmentId(EnchantmentSlot(e_slot));
        SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!pEnchant)
            continue;

        for (int s = 0; s < 3; ++s)
        {
            if (pEnchant->type[s]!=ITEM_ENCHANTMENT_TYPE_USE_SPELL)
                continue;

            SpellEntry const *spellInfo = sSpellStore.LookupEntry(pEnchant->spellid[s]);
            if (!spellInfo)
            {
                sLog.outError("Player::CastItemUseSpell Enchant %i, cast unknown spell %i", pEnchant->ID, pEnchant->spellid[s]);
                continue;
            }

            Spell *spell = new Spell(this, spellInfo, (count > 0));
            spell->m_CastItem = item;
            spell->m_cast_count = cast_count;               // set count of casts
            spell->m_glyphIndex = glyphIndex;               // glyph index
            spell->prepare(&targets);

            ++count;
        }
    }
}

void Player::_RemoveAllItemMods()
{
    DEBUG_LOG("_RemoveAllItemMods start.");

    for (int i = 0; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if(m_items[i])
        {
            ItemPrototype const *proto = m_items[i]->GetProto();
            if(!proto)
                continue;

            // item set bonuses not dependent from item broken state
            if(proto->ItemSet)
                RemoveItemsSetItem(this,proto);

            if(m_items[i]->IsBroken())
                continue;

            ApplyItemEquipSpell(m_items[i],false);
            ApplyEnchantment(m_items[i], false);
        }
    }

    for (int i = 0; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if(m_items[i])
        {
            if(m_items[i]->IsBroken())
                continue;
            ItemPrototype const *proto = m_items[i]->GetProto();
            if(!proto)
                continue;

            uint32 attacktype = Player::GetAttackBySlot(i);
            if(attacktype < MAX_ATTACK)
                _ApplyWeaponDependentAuraMods(m_items[i],WeaponAttackType(attacktype),false);

            _ApplyItemBonuses(proto,i, false);

            if( i == EQUIPMENT_SLOT_RANGED )
                _ApplyAmmoBonuses();
        }
    }

    DEBUG_LOG("_RemoveAllItemMods complete.");
}

void Player::_ApplyAllItemMods()
{
    DEBUG_LOG("_ApplyAllItemMods start.");

    for (int i = 0; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if(m_items[i])
        {
            if(m_items[i]->IsBroken())
                continue;

            ItemPrototype const *proto = m_items[i]->GetProto();
            if(!proto)
                continue;

            uint32 attacktype = Player::GetAttackBySlot(i);
            if(attacktype < MAX_ATTACK)
                _ApplyWeaponDependentAuraMods(m_items[i],WeaponAttackType(attacktype),true);

            _ApplyItemBonuses(proto,i, true);

            if( i == EQUIPMENT_SLOT_RANGED )
                _ApplyAmmoBonuses();
        }
    }

    for (int i = 0; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if(m_items[i])
        {
            ItemPrototype const *proto = m_items[i]->GetProto();
            if(!proto)
                continue;

            // item set bonuses not dependent from item broken state
            if(proto->ItemSet)
                AddItemsSetItem(this,m_items[i]);

            if(m_items[i]->IsBroken())
                continue;

            ApplyItemEquipSpell(m_items[i],true);
            ApplyEnchantment(m_items[i], true);
        }
    }

    DEBUG_LOG("_ApplyAllItemMods complete.");
}

void Player::_ApplyAllLevelScaleItemMods(bool apply)
{
    for (int i = 0; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if(m_items[i])
        {
            if(m_items[i]->IsBroken())
                continue;

            ItemPrototype const *proto = m_items[i]->GetProto();
            if(!proto)
                continue;

            _ApplyItemBonuses(proto,i, apply, true);
        }
    }
}

void Player::_ApplyAmmoBonuses()
{
    // check ammo
    uint32 ammo_id = GetUInt32Value(PLAYER_AMMO_ID);
    if(!ammo_id)
        return;

    float currentAmmoDPS;

    ItemPrototype const *ammo_proto = ObjectMgr::GetItemPrototype( ammo_id );
    if( !ammo_proto || ammo_proto->Class!=ITEM_CLASS_PROJECTILE || !CheckAmmoCompatibility(ammo_proto))
        currentAmmoDPS = 0.0f;
    else
        currentAmmoDPS = ammo_proto->Damage[0].DamageMin;

    if(currentAmmoDPS == GetAmmoDPS())
        return;

    m_ammoDPS = currentAmmoDPS;

    if(CanModifyStats())
        UpdateDamagePhysical(RANGED_ATTACK);
}

bool Player::CheckAmmoCompatibility(const ItemPrototype *ammo_proto) const
{
    if(!ammo_proto)
        return false;

    // check ranged weapon
    Item *weapon = GetWeaponForAttack( RANGED_ATTACK, true, false );
    if (!weapon)
        return false;

    ItemPrototype const* weapon_proto = weapon->GetProto();
    if(!weapon_proto || weapon_proto->Class!=ITEM_CLASS_WEAPON )
        return false;

    // check ammo ws. weapon compatibility
    switch(weapon_proto->SubClass)
    {
        case ITEM_SUBCLASS_WEAPON_BOW:
        case ITEM_SUBCLASS_WEAPON_CROSSBOW:
            if(ammo_proto->SubClass!=ITEM_SUBCLASS_ARROW)
                return false;
            break;
        case ITEM_SUBCLASS_WEAPON_GUN:
            if(ammo_proto->SubClass!=ITEM_SUBCLASS_BULLET)
                return false;
            break;
        default:
            return false;
    }

    return true;
}

/*  If in a battleground a player dies, and an enemy removes the insignia, the player's bones is lootable
    Called by remove insignia spell effect    */
void Player::RemovedInsignia(Player* looterPlr)
{
    if (!GetBattleGroundId())
        return;

    // If not released spirit, do it !
    if(m_deathTimer > 0)
    {
        m_deathTimer = 0;
        BuildPlayerRepop();
        RepopAtGraveyard();
    }

    Corpse *corpse = GetCorpse();
    if (!corpse)
        return;

    // We have to convert player corpse to bones, not to be able to resurrect there
    // SpawnCorpseBones isn't handy, 'cos it saves player while he in BG
    Corpse *bones = sObjectAccessor.ConvertCorpseForPlayer(GetObjectGuid(), true);
    if (!bones)
        return;

    // Now we must make bones lootable, and send player loot
    bones->SetFlag(CORPSE_FIELD_DYNAMIC_FLAGS, CORPSE_DYNFLAG_LOOTABLE);

    // We store the level of our player in the gold field
    // We retrieve this information at Player::SendLoot()
    bones->loot.gold = getLevel();
    bones->lootRecipient = looterPlr;
    looterPlr->SendLoot(bones->GetObjectGuid(), LOOT_INSIGNIA);
}

void Player::SendLootRelease(ObjectGuid guid)
{
    WorldPacket data( SMSG_LOOT_RELEASE_RESPONSE, (8+1) );
    data << guid;
    data << uint8(1);
    SendDirectMessage( &data );
}

void Player::SendLoot(ObjectGuid guid, LootType loot_type)
{
    if (ObjectGuid lootGuid = GetLootGuid())
        m_session->DoLootRelease(lootGuid);

    Loot* loot = NULL;
    PermissionTypes permission = ALL_PERMISSION;

    DEBUG_LOG("Player::SendLoot");
    switch(guid.GetHigh())
    {
        case HIGHGUID_GAMEOBJECT:
        {
            DEBUG_LOG("       IS_GAMEOBJECT_GUID(guid)");
            GameObject *go = GetMap()->GetGameObject(guid);

            // not check distance for GO in case owned GO (fishing bobber case, for example)
            // And permit out of range GO with no owner in case fishing hole
            if (!go || (loot_type != LOOT_FISHINGHOLE && (loot_type != LOOT_FISHING && loot_type != LOOT_FISHING_FAIL || go->GetOwnerGuid() != GetObjectGuid()) && !go->IsWithinDistInMap(this,INTERACTION_DISTANCE)))
            {
                SendLootRelease(guid);
                return;
            }

            loot = &go->loot;

            Player* recipient = go->GetLootRecipient();
            if (!recipient)
            {
                go->SetLootRecipient(this);
                recipient = this;
            }

            // generate loot only if ready for open and spawned in world
            if (go->getLootState() == GO_READY && go->isSpawned())
            {
                uint32 lootid =  go->GetGOInfo()->GetLootId();
                if ((go->GetEntry() == BG_AV_OBJECTID_MINE_N || go->GetEntry() == BG_AV_OBJECTID_MINE_S))
                    if (BattleGround *bg = GetBattleGround())
                        if (bg->GetTypeID() == BATTLEGROUND_AV)
                            if (!(((BattleGroundAV*)bg)->PlayerCanDoMineQuest(go->GetEntry(), GetTeam())))
                            {
                                SendLootRelease(guid);
                                return;
                            }

                // Entry 0 in fishing loot template used for store junk fish loot at fishing fail it junk allowed by config option
                // this is overwrite fishinghole loot for example
                if (loot_type == LOOT_FISHING_FAIL)
                    loot->FillLoot(0, LootTemplates_Fishing, this, true);
                else if (lootid)
                {
                    DEBUG_LOG("       if(lootid)");
                    loot->clear();
                    loot->FillLoot(lootid, LootTemplates_Gameobject, this, false);
                    loot->generateMoneyLoot(go->GetGOInfo()->MinMoneyLoot, go->GetGOInfo()->MaxMoneyLoot);

                    if (go->GetGoType() == GAMEOBJECT_TYPE_CHEST && go->GetGOInfo()->chest.groupLootRules)
                    {
                        if (Group* group = go->GetGroupLootRecipient())
                        {
                            group->UpdateLooterGuid(go, true);

                            switch (group->GetLootMethod())
                            {
                                case GROUP_LOOT:
                                    // GroupLoot delete items over threshold (threshold even not implemented), and roll them. Items with quality<threshold, round robin
                                    group->GroupLoot(go, loot);
                                    permission = GROUP_PERMISSION;
                                    break;
                                case NEED_BEFORE_GREED:
                                    group->NeedBeforeGreed(go, loot);
                                    permission = GROUP_PERMISSION;
                                    break;
                                case MASTER_LOOT:
                                    group->MasterLoot(go, loot);
                                    permission = MASTER_PERMISSION;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
                else if (loot_type == LOOT_FISHING)
                    go->getFishLoot(loot,this);

                go->SetLootState(GO_ACTIVATED);
            }
            if (go->getLootState() == GO_ACTIVATED && go->GetGoType() == GAMEOBJECT_TYPE_CHEST && go->GetGOInfo()->chest.groupLootRules)
            {
                if (Group* group = go->GetGroupLootRecipient())
                {
                    if (group == GetGroup())
                    {
                        if (group->GetLootMethod() == FREE_FOR_ALL)
                            permission = ALL_PERMISSION;
                        else if (group->GetLooterGuid() == GetObjectGuid())
                        {
                            if (group->GetLootMethod() == MASTER_LOOT)
                                permission = MASTER_PERMISSION;
                            else
                                permission = ALL_PERMISSION;
                        }
                        else
                            permission = GROUP_PERMISSION;
                    }
                    else
                        permission = NONE_PERMISSION;
                }
                else if (recipient == this)
                    permission = ALL_PERMISSION;
                else
                    permission = NONE_PERMISSION;
            }
            break;
        }
        case HIGHGUID_ITEM:
        {
            Item *item = GetItemByGuid( guid );

            if (!item)
            {
                SendLootRelease(guid);
                return;
            }

            permission = OWNER_PERMISSION;

            loot = &item->loot;

            if (!item->HasGeneratedLoot())
            {
                item->loot.clear();

                switch(loot_type)
                {
                    case LOOT_DISENCHANTING:
                        loot->FillLoot(item->GetProto()->DisenchantID, LootTemplates_Disenchant, this, true);
                        item->SetLootState(ITEM_LOOT_TEMPORARY);
                        break;
                    case LOOT_PROSPECTING:
                        loot->FillLoot(item->GetEntry(), LootTemplates_Prospecting, this, true);
                        item->SetLootState(ITEM_LOOT_TEMPORARY);
                        break;
                    case LOOT_MILLING:
                        loot->FillLoot(item->GetEntry(), LootTemplates_Milling, this, true);
                        item->SetLootState(ITEM_LOOT_TEMPORARY);
                        break;
                    default:
                        loot->FillLoot(item->GetEntry(), LootTemplates_Item, this, true, item->GetProto()->MaxMoneyLoot == 0);
                        loot->generateMoneyLoot(item->GetProto()->MinMoneyLoot, item->GetProto()->MaxMoneyLoot);
                        item->SetLootState(ITEM_LOOT_CHANGED);
                        break;
                }
            }
            break;
        }
        case HIGHGUID_CORPSE:                               // remove insignia
        {
            Corpse *bones = GetMap()->GetCorpse(guid);

            if (!bones || !((loot_type == LOOT_CORPSE) || (loot_type == LOOT_INSIGNIA)) || (bones->GetType() != CORPSE_BONES) )
            {
                SendLootRelease(guid);
                return;
            }

            loot = &bones->loot;

            if (!bones->lootForBody)
            {
                bones->lootForBody = true;
                uint32 pLevel = bones->loot.gold;
                bones->loot.clear();
                if (GetBattleGround()->GetTypeID() == BATTLEGROUND_AV)
                    loot->FillLoot(0, LootTemplates_Creature, this, false);
                // It may need a better formula
                // Now it works like this: lvl10: ~6copper, lvl70: ~9silver
                bones->loot.gold = (uint32)( urand(50, 150) * 0.016f * pow( ((float)pLevel)/5.76f, 2.5f) * sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_MONEY) );
            }

            if (bones->lootRecipient != this)
                permission = NONE_PERMISSION;
            else
                permission = OWNER_PERMISSION;
            break;
        }
        case HIGHGUID_UNIT:
        case HIGHGUID_VEHICLE:
        {
            Creature *creature = GetMap()->GetCreature(guid);

            // must be in range and creature must be alive for pickpocket and must be dead for another loot
            if (!creature || creature->isAlive()!=(loot_type == LOOT_PICKPOCKETING) || !creature->IsWithinDistInMap(this,INTERACTION_DISTANCE))
            {
                SendLootRelease(guid);
                return;
            }

            if (loot_type == LOOT_PICKPOCKETING && IsFriendlyTo(creature))
            {
                SendLootRelease(guid);
                return;
            }

            loot   = &creature->loot;

            if (loot_type == LOOT_PICKPOCKETING)
            {
                if (!creature->lootForPickPocketed)
                {
                    creature->lootForPickPocketed = true;
                    loot->clear();

                    if (uint32 lootid = creature->GetCreatureInfo()->pickpocketLootId)
                        loot->FillLoot(lootid, LootTemplates_Pickpocketing, this, false);

                    // Generate extra money for pick pocket loot
                    const uint32 a = urand(0, creature->getLevel()/2);
                    const uint32 b = urand(0, getLevel()/2);
                    loot->gold = uint32(10 * (a + b) * sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_MONEY));
                    permission = OWNER_PERMISSION;
                }
            }
            else
            {
                // the player whose group may loot the corpse
                Player *recipient = creature->GetLootRecipient();
                if (!recipient)
                {
                    creature->SetLootRecipient(this);
                    recipient = this;
                }

                if (creature->lootForPickPocketed)
                {
                    creature->lootForPickPocketed = false;
                    loot->clear();
                }

                if (!creature->lootForBody)
                {
                    creature->lootForBody = true;
                    loot->clear();

                    if (uint32 lootid = creature->GetCreatureInfo()->lootid)
                        loot->FillLoot(lootid, LootTemplates_Creature, recipient, false);

                    loot->generateMoneyLoot(creature->GetCreatureInfo()->mingold,creature->GetCreatureInfo()->maxgold);

                    if (Group* group = creature->GetGroupLootRecipient())
                    {
                        group->UpdateLooterGuid(creature,true);

                        switch (group->GetLootMethod())
                        {
                            case GROUP_LOOT:
                                // GroupLoot delete items over threshold (threshold even not implemented), and roll them. Items with quality<threshold, round robin
                                group->GroupLoot(creature, loot);
                                break;
                            case NEED_BEFORE_GREED:
                                group->NeedBeforeGreed(creature, loot);
                                break;
                            case MASTER_LOOT:
                                group->MasterLoot(creature, loot);
                                break;
                            default:
                                break;
                        }
                    }
                }

                // possible only if creature->lootForBody && loot->empty() at spell cast check
                if (loot_type == LOOT_SKINNING)
                {
                    if (!creature->lootForSkin)
                    {
                        creature->lootForSkin = true;
                        loot->clear();
                        loot->FillLoot(creature->GetCreatureInfo()->SkinLootId, LootTemplates_Skinning, this, false);

                        // let reopen skinning loot if will closed.
                        if (!loot->empty())
                            creature->SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);

                        permission = OWNER_PERMISSION;
                    }
                }
                // set group rights only for loot_type != LOOT_SKINNING
                else
                {
                    if(Group* group = creature->GetGroupLootRecipient())
                    {
                        if (group == GetGroup())
                        {
                            if (group->GetLootMethod() == FREE_FOR_ALL)
                                permission = ALL_PERMISSION;
                            else if (group->GetLooterGuid() == GetObjectGuid())
                            {
                                if (group->GetLootMethod() == MASTER_LOOT)
                                    permission = MASTER_PERMISSION;
                                else
                                    permission = ALL_PERMISSION;
                            }
                            else
                                permission = GROUP_PERMISSION;
                        }
                        else
                            permission = NONE_PERMISSION;
                    }
                    else if (recipient == this)
                        permission = OWNER_PERMISSION;
                    else
                        permission = NONE_PERMISSION;
                }
            }
            break;
        }
        default:
        {
            sLog.outError("%s is unsupported for looting.", guid.GetString().c_str());
            return;
        }
    }

    SetLootGuid(guid);

    // LOOT_INSIGNIA and LOOT_FISHINGHOLE unsupported by client
    switch(loot_type)
    {
        case LOOT_INSIGNIA:     loot_type = LOOT_SKINNING; break;
        case LOOT_FISHING_FAIL: loot_type = LOOT_FISHING; break;
        case LOOT_FISHINGHOLE:  loot_type = LOOT_FISHING; break;
        default: break;
    }

    // need know merged fishing/corpse loot type for achievements
    loot->loot_type = loot_type;

    WorldPacket data(SMSG_LOOT_RESPONSE, (9+50));           // we guess size
    data << ObjectGuid(guid);
    data << uint8(loot_type);
    data << LootView(*loot, this, permission);
    SendDirectMessage(&data);

    // add 'this' player as one of the players that are looting 'loot'
    if (permission != NONE_PERMISSION)
        loot->AddLooter(GetObjectGuid());

    if (loot_type == LOOT_CORPSE && !guid.IsItem())
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_LOOTING);
}

void Player::SendNotifyLootMoneyRemoved()
{
    WorldPacket data(SMSG_LOOT_CLEAR_MONEY, 0);
    GetSession()->SendPacket( &data );
}

void Player::SendNotifyLootItemRemoved(uint8 lootSlot)
{
    WorldPacket data(SMSG_LOOT_REMOVED, 1);
    data << uint8(lootSlot);
    GetSession()->SendPacket( &data );
}

void Player::SendUpdateWorldState(uint32 Field, uint32 Value)
{
    WorldPacket data(SMSG_UPDATE_WORLD_STATE, 8);
    data << Field;
    data << Value;
    GetSession()->SendPacket(&data);
}

static WorldStatePair AV_world_states[] =
{
    { 0x7ae, 0x1 },                                         // 1966  7 snowfall n
    { 0x532, 0x1 },                                         // 1330  8 frostwolfhut hc
    { 0x531, 0x0 },                                         // 1329  9 frostwolfhut ac
    { 0x52e, 0x0 },                                         // 1326 10 stormpike firstaid a_a
    { 0x571, 0x0 },                                         // 1393 11 east frostwolf tower horde assaulted -unused
    { 0x570, 0x0 },                                         // 1392 12 west frostwolf tower horde assaulted - unused
    { 0x567, 0x1 },                                         // 1383 13 frostwolfe c
    { 0x566, 0x1 },                                         // 1382 14 frostwolfw c
    { 0x550, 0x1 },                                         // 1360 15 irondeep (N) ally
    { 0x544, 0x0 },                                         // 1348 16 ice grave a_a
    { 0x536, 0x0 },                                         // 1334 17 stormpike grave h_c
    { 0x535, 0x1 },                                         // 1333 18 stormpike grave a_c
    { 0x518, 0x0 },                                         // 1304 19 stoneheart grave a_a
    { 0x517, 0x0 },                                         // 1303 20 stoneheart grave h_a
    { 0x574, 0x0 },                                         // 1396 21 unk
    { 0x573, 0x0 },                                         // 1395 22 iceblood tower horde assaulted -unused
    { 0x572, 0x0 },                                         // 1394 23 towerpoint horde assaulted - unused
    { 0x56f, 0x0 },                                         // 1391 24 unk
    { 0x56e, 0x0 },                                         // 1390 25 iceblood a
    { 0x56d, 0x0 },                                         // 1389 26 towerp a
    { 0x56c, 0x0 },                                         // 1388 27 frostwolfe a
    { 0x56b, 0x0 },                                         // 1387 28 froswolfw a
    { 0x56a, 0x1 },                                         // 1386 29 unk
    { 0x569, 0x1 },                                         // 1385 30 iceblood c
    { 0x568, 0x1 },                                         // 1384 31 towerp c
    { 0x565, 0x0 },                                         // 1381 32 stoneh tower a
    { 0x564, 0x0 },                                         // 1380 33 icewing tower a
    { 0x563, 0x0 },                                         // 1379 34 dunn a
    { 0x562, 0x0 },                                         // 1378 35 duns a
    { 0x561, 0x0 },                                         // 1377 36 stoneheart bunker alliance assaulted - unused
    { 0x560, 0x0 },                                         // 1376 37 icewing bunker alliance assaulted - unused
    { 0x55f, 0x0 },                                         // 1375 38 dunbaldar south alliance assaulted - unused
    { 0x55e, 0x0 },                                         // 1374 39 dunbaldar north alliance assaulted - unused
    { 0x55d, 0x0 },                                         // 1373 40 stone tower d
    { 0x3c6, 0x0 },                                         //  966 41 unk
    { 0x3c4, 0x0 },                                         //  964 42 unk
    { 0x3c2, 0x0 },                                         //  962 43 unk
    { 0x516, 0x1 },                                         // 1302 44 stoneheart grave a_c
    { 0x515, 0x0 },                                         // 1301 45 stonheart grave h_c
    { 0x3b6, 0x0 },                                         //  950 46 unk
    { 0x55c, 0x0 },                                         // 1372 47 icewing tower d
    { 0x55b, 0x0 },                                         // 1371 48 dunn d
    { 0x55a, 0x0 },                                         // 1370 49 duns d
    { 0x559, 0x0 },                                         // 1369 50 unk
    { 0x558, 0x0 },                                         // 1368 51 iceblood d
    { 0x557, 0x0 },                                         // 1367 52 towerp d
    { 0x556, 0x0 },                                         // 1366 53 frostwolfe d
    { 0x555, 0x0 },                                         // 1365 54 frostwolfw d
    { 0x554, 0x1 },                                         // 1364 55 stoneh tower c
    { 0x553, 0x1 },                                         // 1363 56 icewing tower c
    { 0x552, 0x1 },                                         // 1362 57 dunn c
    { 0x551, 0x1 },                                         // 1361 58 duns c
    { 0x54f, 0x0 },                                         // 1359 59 irondeep (N) horde
    { 0x54e, 0x0 },                                         // 1358 60 irondeep (N) ally
    { 0x54d, 0x1 },                                         // 1357 61 mine (S) neutral
    { 0x54c, 0x0 },                                         // 1356 62 mine (S) horde
    { 0x54b, 0x0 },                                         // 1355 63 mine (S) ally
    { 0x545, 0x0 },                                         // 1349 64 iceblood h_a
    { 0x543, 0x1 },                                         // 1347 65 iceblod h_c
    { 0x542, 0x0 },                                         // 1346 66 iceblood a_c
    { 0x540, 0x0 },                                         // 1344 67 snowfall h_a
    { 0x53f, 0x0 },                                         // 1343 68 snowfall a_a
    { 0x53e, 0x0 },                                         // 1342 69 snowfall h_c
    { 0x53d, 0x0 },                                         // 1341 70 snowfall a_c
    { 0x53c, 0x0 },                                         // 1340 71 frostwolf g h_a
    { 0x53b, 0x0 },                                         // 1339 72 frostwolf g a_a
    { 0x53a, 0x1 },                                         // 1338 73 frostwolf g h_c
    { 0x539, 0x0 },                                         // l33t 74 frostwolf g a_c
    { 0x538, 0x0 },                                         // 1336 75 stormpike grave h_a
    { 0x537, 0x0 },                                         // 1335 76 stormpike grave a_a
    { 0x534, 0x0 },                                         // 1332 77 frostwolf hut h_a
    { 0x533, 0x0 },                                         // 1331 78 frostwolf hut a_a
    { 0x530, 0x0 },                                         // 1328 79 stormpike first aid h_a
    { 0x52f, 0x0 },                                         // 1327 80 stormpike first aid h_c
    { 0x52d, 0x1 },                                         // 1325 81 stormpike first aid a_c
    { 0x0,   0x0 }
};

static WorldStatePair WS_world_states[] =
{
    { 0x62d, 0x0 },                                         // 1581  7 alliance flag captures
    { 0x62e, 0x0 },                                         // 1582  8 horde flag captures
    { 0x609, 0x0 },                                         // 1545  9 unk, set to 1 on alliance flag pickup...
    { 0x60a, 0x0 },                                         // 1546 10 unk, set to 1 on horde flag pickup, after drop it's -1
    { 0x60b, 0x2 },                                         // 1547 11 unk
    { 0x641, 0x3 },                                         // 1601 12 unk (max flag captures?)
    { 0x922, 0x1 },                                         // 2338 13 horde (0 - hide, 1 - flag ok, 2 - flag picked up (flashing), 3 - flag picked up (not flashing)
    { 0x923, 0x1 },                                         // 2339 14 alliance (0 - hide, 1 - flag ok, 2 - flag picked up (flashing), 3 - flag picked up (not flashing)
    { 0x1097,0x1 },                                         // 4247 15 show time limit?
    { 0x1098,0x19 },                                        // 4248 16 time remaining in minutes
    { 0x0,   0x0 }
};

static WorldStatePair AB_world_states[] =
{
    { 0x6e7, 0x0 },                                         // 1767  7 stables alliance
    { 0x6e8, 0x0 },                                         // 1768  8 stables horde
    { 0x6e9, 0x0 },                                         // 1769  9 unk, ST?
    { 0x6ea, 0x0 },                                         // 1770 10 stables (show/hide)
    { 0x6ec, 0x0 },                                         // 1772 11 farm (0 - horde controlled, 1 - alliance controlled)
    { 0x6ed, 0x0 },                                         // 1773 12 farm (show/hide)
    { 0x6ee, 0x0 },                                         // 1774 13 farm color
    { 0x6ef, 0x0 },                                         // 1775 14 gold mine color, may be FM?
    { 0x6f0, 0x0 },                                         // 1776 15 alliance resources
    { 0x6f1, 0x0 },                                         // 1777 16 horde resources
    { 0x6f2, 0x0 },                                         // 1778 17 horde bases
    { 0x6f3, 0x0 },                                         // 1779 18 alliance bases
    { 0x6f4, 0x7d0 },                                       // 1780 19 max resources (2000)
    { 0x6f6, 0x0 },                                         // 1782 20 blacksmith color
    { 0x6f7, 0x0 },                                         // 1783 21 blacksmith (show/hide)
    { 0x6f8, 0x0 },                                         // 1784 22 unk, bs?
    { 0x6f9, 0x0 },                                         // 1785 23 unk, bs?
    { 0x6fb, 0x0 },                                         // 1787 24 gold mine (0 - horde contr, 1 - alliance contr)
    { 0x6fc, 0x0 },                                         // 1788 25 gold mine (0 - conflict, 1 - horde)
    { 0x6fd, 0x0 },                                         // 1789 26 gold mine (1 - show/0 - hide)
    { 0x6fe, 0x0 },                                         // 1790 27 gold mine color
    { 0x700, 0x0 },                                         // 1792 28 gold mine color, wtf?, may be LM?
    { 0x701, 0x0 },                                         // 1793 29 lumber mill color (0 - conflict, 1 - horde contr)
    { 0x702, 0x0 },                                         // 1794 30 lumber mill (show/hide)
    { 0x703, 0x0 },                                         // 1795 31 lumber mill color color
    { 0x732, 0x1 },                                         // 1842 32 stables (1 - uncontrolled)
    { 0x733, 0x1 },                                         // 1843 33 gold mine (1 - uncontrolled)
    { 0x734, 0x1 },                                         // 1844 34 lumber mill (1 - uncontrolled)
    { 0x735, 0x1 },                                         // 1845 35 farm (1 - uncontrolled)
    { 0x736, 0x1 },                                         // 1846 36 blacksmith (1 - uncontrolled)
    { 0x745, 0x2 },                                         // 1861 37 unk
    { 0x7a3, 0x708 },                                       // 1955 38 warning limit (1800)
    { 0x0,   0x0 }
};

static WorldStatePair EY_world_states[] =
{
    { 0xac1, 0x0 },                                         // 2753  7 Horde Bases
    { 0xac0, 0x0 },                                         // 2752  8 Alliance Bases
    { 0xab6, 0x0 },                                         // 2742  9 Mage Tower - Horde conflict
    { 0xab5, 0x0 },                                         // 2741 10 Mage Tower - Alliance conflict
    { 0xab4, 0x0 },                                         // 2740 11 Fel Reaver - Horde conflict
    { 0xab3, 0x0 },                                         // 2739 12 Fel Reaver - Alliance conflict
    { 0xab2, 0x0 },                                         // 2738 13 Draenei - Alliance conflict
    { 0xab1, 0x0 },                                         // 2737 14 Draenei - Horde conflict
    { 0xab0, 0x0 },                                         // 2736 15 unk // 0 at start
    { 0xaaf, 0x0 },                                         // 2735 16 unk // 0 at start
    { 0xaad, 0x0 },                                         // 2733 17 Draenei - Horde control
    { 0xaac, 0x0 },                                         // 2732 18 Draenei - Alliance control
    { 0xaab, 0x1 },                                         // 2731 19 Draenei uncontrolled (1 - yes, 0 - no)
    { 0xaaa, 0x0 },                                         // 2730 20 Mage Tower - Alliance control
    { 0xaa9, 0x0 },                                         // 2729 21 Mage Tower - Horde control
    { 0xaa8, 0x1 },                                         // 2728 22 Mage Tower uncontrolled (1 - yes, 0 - no)
    { 0xaa7, 0x0 },                                         // 2727 23 Fel Reaver - Horde control
    { 0xaa6, 0x0 },                                         // 2726 24 Fel Reaver - Alliance control
    { 0xaa5, 0x1 },                                         // 2725 25 Fel Reaver uncontrolled (1 - yes, 0 - no)
    { 0xaa4, 0x0 },                                         // 2724 26 Boold Elf - Horde control
    { 0xaa3, 0x0 },                                         // 2723 27 Boold Elf - Alliance control
    { 0xaa2, 0x1 },                                         // 2722 28 Boold Elf uncontrolled (1 - yes, 0 - no)
    { 0xac5, 0x1 },                                         // 2757 29 Flag (1 - show, 0 - hide) - doesn't work exactly this way!
    { 0xad2, 0x1 },                                         // 2770 30 Horde top-stats (1 - show, 0 - hide) // 02 -> horde picked up the flag
    { 0xad1, 0x1 },                                         // 2769 31 Alliance top-stats (1 - show, 0 - hide) // 02 -> alliance picked up the flag
    { 0xabe, 0x0 },                                         // 2750 32 Horde resources
    { 0xabd, 0x0 },                                         // 2749 33 Alliance resources
    { 0xa05, 0x8e },                                        // 2565 34 unk, constant?
    { 0xaa0, 0x0 },                                         // 2720 35 Capturing progress-bar (100 -> empty (only grey), 0 -> blue|red (no grey), default 0)
    { 0xa9f, 0x0 },                                         // 2719 36 Capturing progress-bar (0 - left, 100 - right)
    { 0xa9e, 0x0 },                                         // 2718 37 Capturing progress-bar (1 - show, 0 - hide)
    { 0xc0d, 0x17b },                                       // 3085 38 unk
    // and some more ... unknown
    { 0x0,   0x0 }
};

static WorldStatePair HP_world_states[] =                   // Hellfire Peninsula
{
    { 0x9ba, 0x1 },                                         // 2490 10
    { 0x9b9, 0x1 },                                         // 2489 11
    { 0x9b5, 0x0 },                                         // 2485 12
    { 0x9b4, 0x1 },                                         // 2484 13
    { 0x9b3, 0x0 },                                         // 2483 14
    { 0x9b2, 0x0 },                                         // 2482 15
    { 0x9b1, 0x1 },                                         // 2481 16
    { 0x9b0, 0x0 },                                         // 2480 17
    { 0x9ae, 0x0 },                                         // 2478 18 horde pvp objectives captured
    { 0x9ac, 0x0 },                                         // 2476 19
    { 0x9a8, 0x0 },                                         // 2472 20
    { 0x9a7, 0x0 },                                         // 2471 21
    { 0x9a6, 0x1 },                                         // 2470 22
    { 0x0,   0x0 }
};

static WorldStatePair TF_world_states[] =                   // Terokkar Forest
{
    { 0xa41, 0x0 },                                         // 2625 10
    { 0xa40, 0x14 },                                        // 2624 11
    { 0xa3f, 0x0 },                                         // 2623 12
    { 0xa3e, 0x0 },                                         // 2622 13
    { 0xa3d, 0x5 },                                         // 2621 14
    { 0xa3c, 0x0 },                                         // 2620 15
    { 0xa87, 0x0 },                                         // 2695 16
    { 0xa86, 0x0 },                                         // 2694 17
    { 0xa85, 0x0 },                                         // 2693 18
    { 0xa84, 0x0 },                                         // 2692 19
    { 0xa83, 0x0 },                                         // 2691 20
    { 0xa82, 0x0 },                                         // 2690 21
    { 0xa81, 0x0 },                                         // 2689 22
    { 0xa80, 0x0 },                                         // 2688 23
    { 0xa7e, 0x0 },                                         // 2686 24
    { 0xa7d, 0x0 },                                         // 2685 25
    { 0xa7c, 0x0 },                                         // 2684 26
    { 0xa7b, 0x0 },                                         // 2683 27
    { 0xa7a, 0x0 },                                         // 2682 28
    { 0xa79, 0x0 },                                         // 2681 29
    { 0x9d0, 0x5 },                                         // 2512 30
    { 0x9ce, 0x0 },                                         // 2510 31
    { 0x9cd, 0x0 },                                         // 2509 32
    { 0x9cc, 0x0 },                                         // 2508 33
    { 0xa88, 0x0 },                                         // 2696 34
    { 0xad0, 0x0 },                                         // 2768 35
    { 0xacf, 0x1 },                                         // 2767 36
    { 0x0,   0x0 }
};

static WorldStatePair ZM_world_states[] =                   // Zangarmarsh
{
    { 0x9e1, 0x0 },                                         // 2529 10
    { 0x9e0, 0x0 },                                         // 2528 11
    { 0x9df, 0x0 },                                         // 2527 12
    { 0xa5d, 0x1 },                                         // 2526 13
    { 0xa5c, 0x0 },                                         // 2525 14
    { 0xa5b, 0x1 },                                         // 2524 15
    { 0xa5a, 0x0 },                                         // 2523 16
    { 0xa59, 0x1 },                                         // 2649 17
    { 0xa58, 0x0 },                                         // 2648 18
    { 0xa57, 0x0 },                                         // 2647 19
    { 0xa56, 0x0 },                                         // 2646 20
    { 0xa55, 0x1 },                                         // 2645 21
    { 0xa54, 0x0 },                                         // 2644 22
    { 0x9e7, 0x0 },                                         // 2535 23
    { 0x9e6, 0x0 },                                         // 2534 24
    { 0x9e5, 0x0 },                                         // 2533 25
    { 0xa00, 0x0 },                                         // 2560 26
    { 0x9ff, 0x1 },                                         // 2559 27
    { 0x9fe, 0x0 },                                         // 2558 28
    { 0x9fd, 0x0 },                                         // 2557 29
    { 0x9fc, 0x1 },                                         // 2556 30
    { 0x9fb, 0x0 },                                         // 2555 31
    { 0xa62, 0x0 },                                         // 2658 32
    { 0xa61, 0x1 },                                         // 2657 33
    { 0xa60, 0x1 },                                         // 2656 34
    { 0xa5f, 0x0 },                                         // 2655 35
    { 0x0,   0x0 }
};

void Player::SendInitWorldStates(uint32 zoneid, uint32 areaid)
{
    // data depends on zoneid/mapid...
    BattleGround* bg = GetBattleGround();
    uint32 mapid = GetMapId();

    DEBUG_LOG("Sending SMSG_INIT_WORLD_STATES to Map:%u, Zone: %u", mapid, zoneid);

    uint32 count = 0;                                       // count of world states in packet

    WorldPacket data(SMSG_INIT_WORLD_STATES, (4+4+4+2+8*8));// guess
    data << uint32(mapid);                                  // mapid
    data << uint32(zoneid);                                 // zone id
    data << uint32(areaid);                                 // area id, new 2.1.0
    size_t count_pos = data.wpos();
    data << uint16(0);                                      // count of uint64 blocks, placeholder

    // common fields
    FillInitialWorldState(data, count, 0x8d8, 0x0);         // 2264 1
    FillInitialWorldState(data, count, 0x8d7, 0x0);         // 2263 2
    FillInitialWorldState(data, count, 0x8d6, 0x0);         // 2262 3
    FillInitialWorldState(data, count, 0x8d5, 0x0);         // 2261 4
    FillInitialWorldState(data, count, 0x8d4, 0x0);         // 2260 5
    FillInitialWorldState(data, count, 0x8d3, 0x0);         // 2259 6
                                                            // 3191 7 Current arena season
    FillInitialWorldState(data, count, 0xC77, sWorld.getConfig(CONFIG_UINT32_ARENA_SEASON_ID));
                                                            // 3901 8 Previous arena season
    FillInitialWorldState(data, count, 0xF3D, sWorld.getConfig(CONFIG_UINT32_ARENA_SEASON_PREVIOUS_ID));
    FillInitialWorldState(data, count, 0xED9, 1);           // 3801 9  0 - Battle for Wintergrasp in progress, 1 - otherwise
                                                            // 4354 10 Time when next Battle for Wintergrasp starts
    FillInitialWorldState(data, count, 0x1102, uint32(time(NULL) + 9000));

    if(mapid == 530)                                        // Outland
    {
        FillInitialWorldState(data, count, 0x9bf, 0x0);     // 2495
        FillInitialWorldState(data, count, 0x9bd, 0xF);     // 2493
        FillInitialWorldState(data, count, 0x9bb, 0xF);     // 2491
    }
    switch(zoneid)
    {
        case 1:
        case 11:
        case 12:
        case 38:
        case 40:
        case 51:
        case 1519:
        case 1537:
        case 2257:
            break;
        case 2597:                                          // AV
            if (bg && bg->GetTypeID() == BATTLEGROUND_AV)
                bg->FillInitialWorldStates(data, count);
            else
                FillInitialWorldState(data,count, AV_world_states);
            break;
        case 3277:                                          // WS
            if (bg && bg->GetTypeID() == BATTLEGROUND_WS)
                bg->FillInitialWorldStates(data, count);
            else
                FillInitialWorldState(data,count, WS_world_states);
            break;
        case 3358:                                          // AB
            if (bg && bg->GetTypeID() == BATTLEGROUND_AB)
                bg->FillInitialWorldStates(data, count);
            else
                FillInitialWorldState(data,count, AB_world_states);
            break;
        case 3820:                                          // EY
            if (bg && bg->GetTypeID() == BATTLEGROUND_EY)
                bg->FillInitialWorldStates(data, count);
            else
                FillInitialWorldState(data,count, EY_world_states);
            break;
        case 3483:                                          // Hellfire Peninsula
            FillInitialWorldState(data,count, HP_world_states);
            break;
        case 3519:                                          // Terokkar Forest
            FillInitialWorldState(data,count, TF_world_states);
            break;
        case 3521:                                          // Zangarmarsh
            FillInitialWorldState(data,count, ZM_world_states);
            break;
        case 3698:                                          // Nagrand Arena
            if (bg && bg->GetTypeID() == BATTLEGROUND_NA)
                bg->FillInitialWorldStates(data, count);
            else
            {
                FillInitialWorldState(data,count,0xa0f,0x0);// 2575 7
                FillInitialWorldState(data,count,0xa10,0x0);// 2576 8
                FillInitialWorldState(data,count,0xa11,0x0);// 2577 9 show
            }
            break;
        case 3702:                                          // Blade's Edge Arena
            if (bg && bg->GetTypeID() == BATTLEGROUND_BE)
                bg->FillInitialWorldStates(data, count);
            else
            {
                FillInitialWorldState(data,count,0x9f0,0x0);// 2544 7 gold
                FillInitialWorldState(data,count,0x9f1,0x0);// 2545 8 green
                FillInitialWorldState(data,count,0x9f3,0x0);// 2547 9 show
            }
            break;
        case 3968:                                          // Ruins of Lordaeron
            if (bg && bg->GetTypeID() == BATTLEGROUND_RL)
                bg->FillInitialWorldStates(data, count);
            else
            {
                FillInitialWorldState(data,count,0xbb8,0x0);// 3000 7 gold
                FillInitialWorldState(data,count,0xbb9,0x0);// 3001 8 green
                FillInitialWorldState(data,count,0xbba,0x0);// 3002 9 show
            }
            break;
        case 3703:                                          // Shattrath City
            break;
        default:
            FillInitialWorldState(data,count, 0x914, 0x0);  // 2324 7
            FillInitialWorldState(data,count, 0x913, 0x0);  // 2323 8
            FillInitialWorldState(data,count, 0x912, 0x0);  // 2322 9
            FillInitialWorldState(data,count, 0x915, 0x0);  // 2325 10
            break;
    }

    FillBGWeekendWorldStates(data,count);

    data.put<uint16>(count_pos,count);                      // set actual world state amount

    GetSession()->SendPacket(&data);
}

void Player::FillBGWeekendWorldStates(WorldPacket& data, uint32& count)
{
    for(uint32 i = 1; i < sBattlemasterListStore.GetNumRows(); ++i)
    {
        BattlemasterListEntry const * bl = sBattlemasterListStore.LookupEntry(i);
        if (bl && bl->HolidayWorldStateId)
        {
            if (BattleGroundMgr::IsBGWeekend(BattleGroundTypeId(bl->id)))
                FillInitialWorldState(data, count, bl->HolidayWorldStateId, 1);
            else
                FillInitialWorldState(data, count, bl->HolidayWorldStateId, 0);
        }
    }
}

uint32 Player::GetXPRestBonus(uint32 xp)
{
    uint32 rested_bonus = (uint32)GetRestBonus();           // xp for each rested bonus

    if(rested_bonus > xp)                                   // max rested_bonus == xp or (r+x) = 200% xp
        rested_bonus = xp;

    SetRestBonus( GetRestBonus() - rested_bonus);

    DETAIL_LOG("Player gain %u xp (+ %u Rested Bonus). Rested points=%f",xp+rested_bonus,rested_bonus,GetRestBonus());
    return rested_bonus;
}

void Player::SetBindPoint(ObjectGuid guid)
{
    WorldPacket data(SMSG_BINDER_CONFIRM, 8);
    data << ObjectGuid(guid);
    GetSession()->SendPacket( &data );
}

void Player::SendTalentWipeConfirm(ObjectGuid guid)
{
    WorldPacket data(MSG_TALENT_WIPE_CONFIRM, (8+4));
    data << ObjectGuid(guid);
    data << uint32(resetTalentsCost());
    GetSession()->SendPacket( &data );
}

void Player::SendPetSkillWipeConfirm()
{
    Pet* pet = GetPet();
    if(!pet)
        return;
    WorldPacket data(SMSG_PET_UNLEARN_CONFIRM, (8+4));
    data << ObjectGuid(pet->GetObjectGuid());
    data << uint32(pet->resetTalentsCost());
    GetSession()->SendPacket( &data );
}

/*********************************************************/
/***                    STORAGE SYSTEM                 ***/
/*********************************************************/

void Player::SetVirtualItemSlot( uint8 i, Item* item)
{
    MANGOS_ASSERT(i < 3);
    if(i < 2 && item)
    {
        if(!item->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
            return;
        uint32 charges = item->GetEnchantmentCharges(TEMP_ENCHANTMENT_SLOT);
        if(charges == 0)
            return;
        if(charges > 1)
            item->SetEnchantmentCharges(TEMP_ENCHANTMENT_SLOT,charges-1);
        else if(charges <= 1)
        {
            ApplyEnchantment(item,TEMP_ENCHANTMENT_SLOT,false);
            item->ClearEnchantment(TEMP_ENCHANTMENT_SLOT);
        }
    }
}

void Player::SetSheath( SheathState sheathed )
{
    switch (sheathed)
    {
        case SHEATH_STATE_UNARMED:                          // no prepared weapon
            SetVirtualItemSlot(0,NULL);
            SetVirtualItemSlot(1,NULL);
            SetVirtualItemSlot(2,NULL);
            break;
        case SHEATH_STATE_MELEE:                            // prepared melee weapon
        {
            SetVirtualItemSlot(0,GetWeaponForAttack(BASE_ATTACK,true,true));
            SetVirtualItemSlot(1,GetWeaponForAttack(OFF_ATTACK,true,true));
            SetVirtualItemSlot(2,NULL);
        };  break;
        case SHEATH_STATE_RANGED:                           // prepared ranged weapon
            SetVirtualItemSlot(0,NULL);
            SetVirtualItemSlot(1,NULL);
            SetVirtualItemSlot(2,GetWeaponForAttack(RANGED_ATTACK,true,true));
            break;
        default:
            SetVirtualItemSlot(0,NULL);
            SetVirtualItemSlot(1,NULL);
            SetVirtualItemSlot(2,NULL);
            break;
    }
    Unit::SetSheath(sheathed);                              // this must visualize Sheath changing for other players...
}

uint8 Player::FindEquipSlot( ItemPrototype const* proto, uint32 slot, bool swap ) const
{
    uint8 pClass = getClass();

    uint8 slots[4];
    slots[0] = NULL_SLOT;
    slots[1] = NULL_SLOT;
    slots[2] = NULL_SLOT;
    slots[3] = NULL_SLOT;
    switch( proto->InventoryType )
    {
        case INVTYPE_HEAD:
            slots[0] = EQUIPMENT_SLOT_HEAD;
            break;
        case INVTYPE_NECK:
            slots[0] = EQUIPMENT_SLOT_NECK;
            break;
        case INVTYPE_SHOULDERS:
            slots[0] = EQUIPMENT_SLOT_SHOULDERS;
            break;
        case INVTYPE_BODY:
            slots[0] = EQUIPMENT_SLOT_BODY;
            break;
        case INVTYPE_CHEST:
            slots[0] = EQUIPMENT_SLOT_CHEST;
            break;
        case INVTYPE_ROBE:
            slots[0] = EQUIPMENT_SLOT_CHEST;
            break;
        case INVTYPE_WAIST:
            slots[0] = EQUIPMENT_SLOT_WAIST;
            break;
        case INVTYPE_LEGS:
            slots[0] = EQUIPMENT_SLOT_LEGS;
            break;
        case INVTYPE_FEET:
            slots[0] = EQUIPMENT_SLOT_FEET;
            break;
        case INVTYPE_WRISTS:
            slots[0] = EQUIPMENT_SLOT_WRISTS;
            break;
        case INVTYPE_HANDS:
            slots[0] = EQUIPMENT_SLOT_HANDS;
            break;
        case INVTYPE_FINGER:
            slots[0] = EQUIPMENT_SLOT_FINGER1;
            slots[1] = EQUIPMENT_SLOT_FINGER2;
            break;
        case INVTYPE_TRINKET:
            slots[0] = EQUIPMENT_SLOT_TRINKET1;
            slots[1] = EQUIPMENT_SLOT_TRINKET2;
            break;
        case INVTYPE_CLOAK:
            slots[0] =  EQUIPMENT_SLOT_BACK;
            break;
        case INVTYPE_WEAPON:
        {
            slots[0] = EQUIPMENT_SLOT_MAINHAND;

            // suggest offhand slot only if know dual wielding
            // (this will be replace mainhand weapon at auto equip instead unwonted "you don't known dual wielding" ...
            if(CanDualWield())
                slots[1] = EQUIPMENT_SLOT_OFFHAND;
            break;
        };
        case INVTYPE_SHIELD:
            slots[0] = EQUIPMENT_SLOT_OFFHAND;
            break;
        case INVTYPE_RANGED:
            slots[0] = EQUIPMENT_SLOT_RANGED;
            break;
        case INVTYPE_2HWEAPON:
            slots[0] = EQUIPMENT_SLOT_MAINHAND;
            if (CanDualWield() && CanTitanGrip())
                slots[1] = EQUIPMENT_SLOT_OFFHAND;
            break;
        case INVTYPE_TABARD:
            slots[0] = EQUIPMENT_SLOT_TABARD;
            break;
        case INVTYPE_WEAPONMAINHAND:
            slots[0] = EQUIPMENT_SLOT_MAINHAND;
            break;
        case INVTYPE_WEAPONOFFHAND:
            slots[0] = EQUIPMENT_SLOT_OFFHAND;
            break;
        case INVTYPE_HOLDABLE:
            slots[0] = EQUIPMENT_SLOT_OFFHAND;
            break;
        case INVTYPE_THROWN:
            slots[0] = EQUIPMENT_SLOT_RANGED;
            break;
        case INVTYPE_RANGEDRIGHT:
            slots[0] = EQUIPMENT_SLOT_RANGED;
            break;
        case INVTYPE_BAG:
            slots[0] = INVENTORY_SLOT_BAG_START + 0;
            slots[1] = INVENTORY_SLOT_BAG_START + 1;
            slots[2] = INVENTORY_SLOT_BAG_START + 2;
            slots[3] = INVENTORY_SLOT_BAG_START + 3;
            break;
        case INVTYPE_RELIC:
        {
            switch(proto->SubClass)
            {
                case ITEM_SUBCLASS_ARMOR_LIBRAM:
                    if (pClass == CLASS_PALADIN)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
                case ITEM_SUBCLASS_ARMOR_IDOL:
                    if (pClass == CLASS_DRUID)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
                case ITEM_SUBCLASS_ARMOR_TOTEM:
                    if (pClass == CLASS_SHAMAN)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
                case ITEM_SUBCLASS_ARMOR_MISC:
                    if (pClass == CLASS_WARLOCK)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
                case ITEM_SUBCLASS_ARMOR_SIGIL:
                    if (pClass == CLASS_DEATH_KNIGHT)
                        slots[0] = EQUIPMENT_SLOT_RANGED;
                    break;
            }
            break;
        }
        default :
            return NULL_SLOT;
    }

    if( slot != NULL_SLOT )
    {
        if( swap || !GetItemByPos( INVENTORY_SLOT_BAG_0, slot ) )
        {
            for (int i = 0; i < 4; ++i)
            {
                if ( slots[i] == slot )
                    return slot;
            }
        }
    }
    else
    {
        // search free slot at first
        for (int i = 0; i < 4; ++i)
        {
            if ( slots[i] != NULL_SLOT && !GetItemByPos( INVENTORY_SLOT_BAG_0, slots[i] ) )
            {
                // in case 2hand equipped weapon (without titan grip) offhand slot empty but not free
                if(slots[i]!=EQUIPMENT_SLOT_OFFHAND || !IsTwoHandUsed())
                    return slots[i];
            }
        }

        // if not found free and can swap return first appropriate from used
        for (int i = 0; i < 4; ++i)
        {
            if ( slots[i] != NULL_SLOT && swap )
                return slots[i];
        }
    }

    // no free position
    return NULL_SLOT;
}

InventoryResult Player::CanUnequipItems( uint32 item, uint32 count ) const
{
    Item *pItem;
    uint32 tempcount = 0;

    InventoryResult res = EQUIP_ERR_OK;

    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item )
        {
            InventoryResult ires = CanUnequipItem(INVENTORY_SLOT_BAG_0 << 8 | i, false);
            if(ires == EQUIP_ERR_OK)
            {
                tempcount += pItem->GetCount();
                if( tempcount >= count )
                    return EQUIP_ERR_OK;
            }
            else
                res = ires;
        }
    }
    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item )
        {
            tempcount += pItem->GetCount();
            if( tempcount >= count )
                return EQUIP_ERR_OK;
        }
    }
    for(int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
    {
        pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item )
        {
            tempcount += pItem->GetCount();
            if( tempcount >= count )
                return EQUIP_ERR_OK;
        }
    }
    Bag *pBag;
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pBag )
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                pItem = GetItemByPos( i, j );
                if( pItem && pItem->GetEntry() == item )
                {
                    tempcount += pItem->GetCount();
                    if( tempcount >= count )
                        return EQUIP_ERR_OK;
                }
            }
        }
    }

    // not found req. item count and have unequippable items
    return res;
}

uint32 Player::GetItemCount(uint32 item, bool inBankAlso, Item* skipItem) const
{
    uint32 count = 0;
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem != skipItem &&  pItem->GetEntry() == item )
            count += pItem->GetCount();
    }
    for(int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
    {
        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem != skipItem && pItem->GetEntry() == item )
            count += pItem->GetCount();
    }
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pBag )
            count += pBag->GetItemCount(item,skipItem);
    }

    if(skipItem && skipItem->GetProto()->GemProperties)
    {
        for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
            if( pItem && pItem != skipItem && pItem->GetProto()->Socket[0].Color )
                count += pItem->GetGemCountWithID(item);
        }
    }

    if(inBankAlso)
    {
        for(int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
        {
            Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
            if( pItem && pItem != skipItem && pItem->GetEntry() == item )
                count += pItem->GetCount();
        }
        for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i );
            if( pBag )
                count += pBag->GetItemCount(item,skipItem);
        }

        if(skipItem && skipItem->GetProto()->GemProperties)
        {
            for(int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
            {
                Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
                if( pItem && pItem != skipItem && pItem->GetProto()->Socket[0].Color )
                    count += pItem->GetGemCountWithID(item);
            }
        }
    }

    return count;
}

uint32 Player::GetItemCountWithLimitCategory( uint32 limitCategory, Item* skipItem) const
{
    uint32 count = 0;
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        if (Item *pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (pItem->GetProto()->ItemLimitCategory == limitCategory && pItem != skipItem)
                count += pItem->GetCount();

    for(int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
        if (Item *pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (pItem->GetProto()->ItemLimitCategory == limitCategory && pItem != skipItem)
                count += pItem->GetCount();

    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        if (Bag* pBag = (Bag*)GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            count += pBag->GetItemCountWithLimitCategory(limitCategory, skipItem);

    for(int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
        if (Item *pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (pItem->GetProto()->ItemLimitCategory == limitCategory && pItem != skipItem)
                count += pItem->GetCount();

    for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        if (Bag* pBag = (Bag*)GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            count += pBag->GetItemCountWithLimitCategory(limitCategory, skipItem);

    return count;
}

Item* Player::GetItemByEntry( uint32 item ) const
{
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        if (Item *pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (pItem->GetEntry() == item)
                return pItem;

    for(int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
        if (Item *pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (pItem->GetEntry() == item)
                return pItem;

    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        if (Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i))
            if (Item* itemPtr = pBag->GetItemByEntry(item))
                return itemPtr;

    return NULL;
}

Item* Player::GetItemByLimitedCategory(uint32 limitedCategory) const
{
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        if (Item *pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (pItem->GetProto()->ItemLimitCategory == limitedCategory)
                return pItem;

    for(int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
        if (Item *pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (pItem->GetProto()->ItemLimitCategory == limitedCategory)
                return pItem;

    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        if (Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i))
            if (Item* itemPtr = pBag->GetItemByLimitedCategory(limitedCategory))
                return itemPtr;

    return NULL;
}

Item* Player::GetItemByGuid(ObjectGuid guid) const
{
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        if (Item *pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (pItem->GetObjectGuid() == guid)
                return pItem;

    for(int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
        if (Item *pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (pItem->GetObjectGuid() == guid)
                return pItem;

    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        if (Bag *pBag = (Bag*)GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
                if (Item* pItem = pBag->GetItemByPos(j))
                    if (pItem->GetObjectGuid() == guid)
                        return pItem;

    for(int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
        if (Item *pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (pItem->GetObjectGuid() == guid)
                return pItem;

    for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        if (Bag *pBag = (Bag*)GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
                if (Item* pItem = pBag->GetItemByPos(j))
                    if (pItem->GetObjectGuid() == guid)
                        return pItem;

    return NULL;
}

Item* Player::GetItemByPos( uint16 pos ) const
{
    uint8 bag = pos >> 8;
    uint8 slot = pos & 255;
    return GetItemByPos( bag, slot );
}

Item* Player::GetItemByPos( uint8 bag, uint8 slot ) const
{
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot < BANK_SLOT_BAG_END || (slot >= KEYRING_SLOT_START && slot < CURRENCYTOKEN_SLOT_END )) )
        return m_items[slot];
    else if ((bag >= INVENTORY_SLOT_BAG_START && bag < INVENTORY_SLOT_BAG_END)
        || (bag >= BANK_SLOT_BAG_START && bag < BANK_SLOT_BAG_END) )
    {
        Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, bag );
        if ( pBag )
            return pBag->GetItemByPos(slot);
    }
    return NULL;
}

uint32 Player::GetItemDisplayIdInSlot(uint8 bag, uint8 slot) const
{
    const Item* pItem = GetItemByPos(bag, slot);

    if (!pItem)
        return 0;

    return pItem->GetProto()->DisplayInfoID;
}

Item* Player::GetWeaponForAttack(WeaponAttackType attackType, bool nonbroken, bool useable) const
{
    uint8 slot;
    switch (attackType)
    {
        case BASE_ATTACK:   slot = EQUIPMENT_SLOT_MAINHAND; break;
        case OFF_ATTACK:    slot = EQUIPMENT_SLOT_OFFHAND;  break;
        case RANGED_ATTACK: slot = EQUIPMENT_SLOT_RANGED;   break;
        default: return NULL;
    }

    Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    if (!item || item->GetProto()->Class != ITEM_CLASS_WEAPON)
        return NULL;

    if (useable && !CanUseEquippedWeapon(attackType))
        return NULL;

    if (nonbroken && item->IsBroken())
        return NULL;

    return item;
}

Item* Player::GetShield(bool useable) const
{
    Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
    if (!item || item->GetProto()->Class != ITEM_CLASS_ARMOR)
        return NULL;

    if (!useable)
        return item;

    if (item->IsBroken() || !CanUseEquippedWeapon(OFF_ATTACK))
        return NULL;

    return item;
}

uint32 Player::GetAttackBySlot( uint8 slot )
{
    switch(slot)
    {
        case EQUIPMENT_SLOT_MAINHAND: return BASE_ATTACK;
        case EQUIPMENT_SLOT_OFFHAND:  return OFF_ATTACK;
        case EQUIPMENT_SLOT_RANGED:   return RANGED_ATTACK;
        default:                      return MAX_ATTACK;
    }
}

bool Player::IsInventoryPos( uint8 bag, uint8 slot )
{
    if( bag == INVENTORY_SLOT_BAG_0 && slot == NULL_SLOT )
        return true;
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= INVENTORY_SLOT_ITEM_START && slot < INVENTORY_SLOT_ITEM_END ) )
        return true;
    if( bag >= INVENTORY_SLOT_BAG_START && bag < INVENTORY_SLOT_BAG_END )
        return true;
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= KEYRING_SLOT_START && slot < CURRENCYTOKEN_SLOT_END ) )
        return true;
    return false;
}

bool Player::IsEquipmentPos( uint8 bag, uint8 slot )
{
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot < EQUIPMENT_SLOT_END ) )
        return true;
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= INVENTORY_SLOT_BAG_START && slot < INVENTORY_SLOT_BAG_END ) )
        return true;
    return false;
}

bool Player::IsBankPos( uint8 bag, uint8 slot )
{
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= BANK_SLOT_ITEM_START && slot < BANK_SLOT_ITEM_END ) )
        return true;
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= BANK_SLOT_BAG_START && slot < BANK_SLOT_BAG_END ) )
        return true;
    if( bag >= BANK_SLOT_BAG_START && bag < BANK_SLOT_BAG_END )
        return true;
    return false;
}

bool Player::IsBagPos( uint16 pos )
{
    uint8 bag = pos >> 8;
    uint8 slot = pos & 255;
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= INVENTORY_SLOT_BAG_START && slot < INVENTORY_SLOT_BAG_END ) )
        return true;
    if( bag == INVENTORY_SLOT_BAG_0 && ( slot >= BANK_SLOT_BAG_START && slot < BANK_SLOT_BAG_END ) )
        return true;
    return false;
}

bool Player::IsValidPos( uint8 bag, uint8 slot, bool explicit_pos ) const
{
    // post selected
    if(bag == NULL_BAG && !explicit_pos)
        return true;

    if (bag == INVENTORY_SLOT_BAG_0)
    {
        // any post selected
        if (slot == NULL_SLOT && !explicit_pos)
            return true;

        // equipment
        if (slot < EQUIPMENT_SLOT_END)
            return true;

        // bag equip slots
        if (slot >= INVENTORY_SLOT_BAG_START && slot < INVENTORY_SLOT_BAG_END)
            return true;

        // backpack slots
        if (slot >= INVENTORY_SLOT_ITEM_START && slot < INVENTORY_SLOT_ITEM_END)
            return true;

        // keyring slots
        if (slot >= KEYRING_SLOT_START && slot < KEYRING_SLOT_END)
            return true;

        // bank main slots
        if (slot >= BANK_SLOT_ITEM_START && slot < BANK_SLOT_ITEM_END)
            return true;

        // bank bag slots
        if (slot >= BANK_SLOT_BAG_START && slot < BANK_SLOT_BAG_END)
            return true;

        return false;
    }

    // bag content slots
    if (bag >= INVENTORY_SLOT_BAG_START && bag < INVENTORY_SLOT_BAG_END)
    {
        Bag* pBag = (Bag*)GetItemByPos (INVENTORY_SLOT_BAG_0, bag);
        if(!pBag)
            return false;

        // any post selected
        if (slot == NULL_SLOT && !explicit_pos)
            return true;

        return slot < pBag->GetBagSize();
    }

    // bank bag content slots
    if( bag >= BANK_SLOT_BAG_START && bag < BANK_SLOT_BAG_END )
    {
        Bag* pBag = (Bag*)GetItemByPos (INVENTORY_SLOT_BAG_0, bag);
        if(!pBag)
            return false;

        // any post selected
        if (slot == NULL_SLOT && !explicit_pos)
            return true;

        return slot < pBag->GetBagSize();
    }

    // where this?
    return false;
}


bool Player::HasItemCount( uint32 item, uint32 count, bool inBankAlso ) const
{
    uint32 tempcount = 0;
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if (pItem && pItem->GetEntry() == item && !pItem->IsInTrade())
        {
            tempcount += pItem->GetCount();
            if (tempcount >= count)
                return true;
        }
    }
    for(int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
    {
        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if (pItem && pItem->GetEntry() == item && !pItem->IsInTrade())
        {
            tempcount += pItem->GetCount();
            if (tempcount >= count)
                return true;
        }
    }
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                Item* pItem = GetItemByPos( i, j );
                if (pItem && pItem->GetEntry() == item && !pItem->IsInTrade())
                {
                    tempcount += pItem->GetCount();
                    if (tempcount >= count)
                        return true;
                }
            }
        }
    }

    if(inBankAlso)
    {
        for(int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
        {
            Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
            if (pItem && pItem->GetEntry() == item && !pItem->IsInTrade())
            {
                tempcount += pItem->GetCount();
                if (tempcount >= count)
                    return true;
            }
        }
        for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if (Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            {
                for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
                {
                    Item* pItem = GetItemByPos( i, j );
                    if (pItem && pItem->GetEntry() == item && !pItem->IsInTrade())
                    {
                        tempcount += pItem->GetCount();
                        if (tempcount >= count)
                            return true;
                    }
                }
            }
        }
    }

    return false;
}

bool Player::HasItemOrGemWithIdEquipped( uint32 item, uint32 count, uint8 except_slot ) const
{
    uint32 tempcount = 0;
    for(int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if(i==int(except_slot))
            continue;

        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && pItem->GetEntry() == item)
        {
            tempcount += pItem->GetCount();
            if( tempcount >= count )
                return true;
        }
    }

    ItemPrototype const *pProto = ObjectMgr::GetItemPrototype(item);
    if (pProto && pProto->GemProperties)
    {
        for(int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
        {
            if(i==int(except_slot))
                continue;

            Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
            if( pItem && pItem->GetProto()->Socket[0].Color)
            {
                tempcount += pItem->GetGemCountWithID(item);
                if( tempcount >= count )
                    return true;
            }
        }
    }

    return false;
}

bool Player::HasItemOrGemWithLimitCategoryEquipped( uint32 limitCategory, uint32 count, uint8 except_slot ) const
{
    uint32 tempcount = 0;
    for(int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if(i==int(except_slot))
            continue;

        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if (!pItem)
            continue;

        ItemPrototype const *pProto = pItem->GetProto();
        if (!pProto)
            continue;

        if (pProto->ItemLimitCategory == limitCategory)
        {
            tempcount += pItem->GetCount();
            if( tempcount >= count )
                return true;
        }

        if( pProto->Socket[0].Color)
        {
            tempcount += pItem->GetGemCountWithLimitCategory(limitCategory);
            if( tempcount >= count )
                return true;
        }
    }

    return false;
}

InventoryResult Player::_CanTakeMoreSimilarItems(uint32 entry, uint32 count, Item* pItem, uint32* no_space_count) const
{
    ItemPrototype const *pProto = ObjectMgr::GetItemPrototype(entry);
    if (!pProto)
    {
        if (no_space_count)
            *no_space_count = count;
        return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
    }

    // no maximum
    if (pProto->MaxCount > 0)
    {
        uint32 curcount = GetItemCount(pProto->ItemId, true, pItem);

        if (curcount + count > uint32(pProto->MaxCount))
        {
            if (no_space_count)
                *no_space_count = count +curcount - pProto->MaxCount;
            return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
        }
    }

    // check unique-equipped limit
    if (pProto->ItemLimitCategory)
    {
        ItemLimitCategoryEntry const* limitEntry = sItemLimitCategoryStore.LookupEntry(pProto->ItemLimitCategory);
        if (!limitEntry)
        {
            if(no_space_count)
                *no_space_count = count;
            return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;
        }

        if (limitEntry->mode == ITEM_LIMIT_CATEGORY_MODE_HAVE)
        {
            uint32 curcount = GetItemCountWithLimitCategory(pProto->ItemLimitCategory, pItem);

            if (curcount + count > uint32(limitEntry->maxCount))
            {
                if (no_space_count)
                    *no_space_count = count + curcount - limitEntry->maxCount;
                return EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS;
            }
        }
    }

    return EQUIP_ERR_OK;
}

bool Player::HasItemTotemCategory( uint32 TotemCategory ) const
{
    Item *pItem;
    for(uint8 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && IsTotemCategoryCompatiableWith(pItem->GetProto()->TotemCategory,TotemCategory ))
            return true;
    }
    for(uint8 i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
    {
        pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if( pItem && IsTotemCategoryCompatiableWith(pItem->GetProto()->TotemCategory,TotemCategory ))
            return true;
    }
    for(uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if(Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                pItem = GetItemByPos( i, j );
                if( pItem && IsTotemCategoryCompatiableWith(pItem->GetProto()->TotemCategory,TotemCategory ))
                    return true;
            }
        }
    }
    return false;
}

InventoryResult Player::_CanStoreItem_InSpecificSlot( uint8 bag, uint8 slot, ItemPosCountVec &dest, ItemPrototype const *pProto, uint32& count, bool swap, Item* pSrcItem ) const
{
    Item* pItem2 = GetItemByPos( bag, slot );

    // ignore move item (this slot will be empty at move)
    if (pItem2==pSrcItem)
        pItem2 = NULL;

    uint32 need_space;

    // empty specific slot - check item fit to slot
    if (!pItem2 || swap)
    {
        if (bag == INVENTORY_SLOT_BAG_0)
        {
            // keyring case
            if (slot >= KEYRING_SLOT_START && slot < KEYRING_SLOT_START+GetMaxKeyringSize() && !(pProto->BagFamily & BAG_FAMILY_MASK_KEYS))
                return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

            // currencytoken case
            if (slot >= CURRENCYTOKEN_SLOT_START && slot < CURRENCYTOKEN_SLOT_END && !(pProto->BagFamily & BAG_FAMILY_MASK_CURRENCY_TOKENS))
                return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

            // prevent cheating
            if ((slot >= BUYBACK_SLOT_START && slot < BUYBACK_SLOT_END) || slot >= PLAYER_SLOT_END)
                return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;
        }
        else
        {
            Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, bag );
            if (!pBag)
                return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

            ItemPrototype const* pBagProto = pBag->GetProto();
            if (!pBagProto)
                return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

            if (slot >= pBagProto->ContainerSlots)
                return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

            if (!ItemCanGoIntoBag(pProto,pBagProto))
                return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;
        }

        // non empty stack with space
        need_space = pProto->GetMaxStackSize();
    }
    // non empty slot, check item type
    else
    {
        // can be merged at least partly
        InventoryResult res  = pItem2->CanBeMergedPartlyWith(pProto);
        if (res != EQUIP_ERR_OK)
            return res;

        // free stack space or infinity
        need_space = pProto->GetMaxStackSize() - pItem2->GetCount();
    }

    if (need_space > count)
        need_space = count;

    ItemPosCount newPosition = ItemPosCount((bag << 8) | slot, need_space);
    if (!newPosition.isContainedIn(dest))
    {
        dest.push_back(newPosition);
        count -= need_space;
    }
    return EQUIP_ERR_OK;
}

InventoryResult Player::_CanStoreItem_InBag( uint8 bag, ItemPosCountVec &dest, ItemPrototype const *pProto, uint32& count, bool merge, bool non_specialized, Item* pSrcItem, uint8 skip_bag, uint8 skip_slot ) const
{
    // skip specific bag already processed in first called _CanStoreItem_InBag
    if (bag == skip_bag)
        return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

    // skip nonexistent bag or self targeted bag
    Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, bag );
    if (!pBag || pBag==pSrcItem)
        return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

    ItemPrototype const* pBagProto = pBag->GetProto();
    if (!pBagProto)
        return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

    // specialized bag mode or non-specilized
    if (non_specialized != (pBagProto->Class == ITEM_CLASS_CONTAINER && pBagProto->SubClass == ITEM_SUBCLASS_CONTAINER))
        return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

    if (!ItemCanGoIntoBag(pProto,pBagProto))
        return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

    for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
    {
        // skip specific slot already processed in first called _CanStoreItem_InSpecificSlot
        if (j==skip_slot)
            continue;

        Item* pItem2 = GetItemByPos( bag, j );

        // ignore move item (this slot will be empty at move)
        if (pItem2 == pSrcItem)
            pItem2 = NULL;

        // if merge skip empty, if !merge skip non-empty
        if ((pItem2 != NULL) != merge)
            continue;

        uint32 need_space = pProto->GetMaxStackSize();

        if (pItem2)
        {
            // can be merged at least partly
            uint8 res  = pItem2->CanBeMergedPartlyWith(pProto);
            if (res != EQUIP_ERR_OK)
                continue;

            // descrease at current stacksize
            need_space -= pItem2->GetCount();
        }

        if (need_space > count)
            need_space = count;

        ItemPosCount newPosition = ItemPosCount((bag << 8) | j, need_space);
        if (!newPosition.isContainedIn(dest))
        {
            dest.push_back(newPosition);
            count -= need_space;

            if (count==0)
                return EQUIP_ERR_OK;
        }
    }
    return EQUIP_ERR_OK;
}

InventoryResult Player::_CanStoreItem_InInventorySlots( uint8 slot_begin, uint8 slot_end, ItemPosCountVec &dest, ItemPrototype const *pProto, uint32& count, bool merge, Item* pSrcItem, uint8 skip_bag, uint8 skip_slot ) const
{
    for(uint32 j = slot_begin; j < slot_end; ++j)
    {
        // skip specific slot already processed in first called _CanStoreItem_InSpecificSlot
        if (INVENTORY_SLOT_BAG_0==skip_bag && j==skip_slot)
            continue;

        Item* pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, j );

        // ignore move item (this slot will be empty at move)
        if (pItem2 == pSrcItem)
            pItem2 = NULL;

        // if merge skip empty, if !merge skip non-empty
        if ((pItem2 != NULL) != merge)
            continue;

        uint32 need_space = pProto->GetMaxStackSize();

        if (pItem2)
        {
            // can be merged at least partly
            uint8 res  = pItem2->CanBeMergedPartlyWith(pProto);
            if (res != EQUIP_ERR_OK)
                continue;

            // descrease at current stacksize
            need_space -= pItem2->GetCount();
        }

        if (need_space > count)
            need_space = count;

        ItemPosCount newPosition = ItemPosCount((INVENTORY_SLOT_BAG_0 << 8) | j, need_space);
        if (!newPosition.isContainedIn(dest))
        {
            dest.push_back(newPosition);
            count -= need_space;

            if (count==0)
                return EQUIP_ERR_OK;
        }
    }
    return EQUIP_ERR_OK;
}

InventoryResult Player::_CanStoreItem( uint8 bag, uint8 slot, ItemPosCountVec &dest, uint32 entry, uint32 count, Item *pItem, bool swap, uint32* no_space_count ) const
{
    DEBUG_LOG( "STORAGE: CanStoreItem bag = %u, slot = %u, item = %u, count = %u", bag, slot, entry, count);

    ItemPrototype const *pProto = ObjectMgr::GetItemPrototype(entry);
    if (!pProto)
    {
        if (no_space_count)
            *no_space_count = count;
        return swap ? EQUIP_ERR_ITEMS_CANT_BE_SWAPPED :EQUIP_ERR_ITEM_NOT_FOUND;
    }

    if (pItem)
    {
        // item used
        if (pItem->HasTemporaryLoot())
        {
            if (no_space_count)
                *no_space_count = count;
            return EQUIP_ERR_ALREADY_LOOTED;
        }

        if (pItem->IsBindedNotWith(this))
        {
            if (no_space_count)
                *no_space_count = count;
            return EQUIP_ERR_DONT_OWN_THAT_ITEM;
        }
    }

    // check count of items (skip for auto move for same player from bank)
    uint32 no_similar_count = 0;                            // can't store this amount similar items
    InventoryResult res = _CanTakeMoreSimilarItems(entry,count,pItem,&no_similar_count);
    if (res != EQUIP_ERR_OK)
    {
        if (count == no_similar_count)
        {
            if (no_space_count)
                *no_space_count = no_similar_count;
            return res;
        }
        count -= no_similar_count;
    }

    // in specific slot
    if (bag != NULL_BAG && slot != NULL_SLOT)
    {
        res = _CanStoreItem_InSpecificSlot(bag,slot,dest,pProto,count,swap,pItem);
        if (res != EQUIP_ERR_OK)
        {
            if (no_space_count)
                *no_space_count = count + no_similar_count;
            return res;
        }

        if (count == 0)
        {
            if (no_similar_count == 0)
                return EQUIP_ERR_OK;

            if (no_space_count)
                *no_space_count = count + no_similar_count;
            return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
        }
    }

    // not specific slot or have space for partly store only in specific slot

    // in specific bag
    if (bag != NULL_BAG)
    {
        // search stack in bag for merge to
        if (pProto->Stackable != 1)
        {
            if (bag == INVENTORY_SLOT_BAG_0)               // inventory
            {
                res = _CanStoreItem_InInventorySlots(KEYRING_SLOT_START,CURRENCYTOKEN_SLOT_END,dest,pProto,count,true,pItem,bag,slot);
                if (res != EQUIP_ERR_OK)
                {
                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return res;
                }

                if (count == 0)
                {
                    if (no_similar_count == 0)
                        return EQUIP_ERR_OK;

                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
                }

                res = _CanStoreItem_InInventorySlots(INVENTORY_SLOT_ITEM_START,INVENTORY_SLOT_ITEM_END,dest,pProto,count,true,pItem,bag,slot);
                if (res != EQUIP_ERR_OK)
                {
                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return res;
                }

                if (count == 0)
                {
                    if (no_similar_count == 0)
                        return EQUIP_ERR_OK;

                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
                }
            }
            else                                            // equipped bag
            {
                // we need check 2 time (specialized/non_specialized), use NULL_BAG to prevent skipping bag
                res = _CanStoreItem_InBag(bag,dest,pProto,count,true,false,pItem,NULL_BAG,slot);
                if (res != EQUIP_ERR_OK)
                    res = _CanStoreItem_InBag(bag,dest,pProto,count,true,true,pItem,NULL_BAG,slot);

                if (res != EQUIP_ERR_OK)
                {
                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return res;
                }

                if (count == 0)
                {
                    if (no_similar_count == 0)
                        return EQUIP_ERR_OK;

                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
                }
            }
        }

        // search free slot in bag for place to
        if(bag == INVENTORY_SLOT_BAG_0)                     // inventory
        {
            // search free slot - keyring case
            if (pProto->BagFamily & BAG_FAMILY_MASK_KEYS)
            {
                uint32 keyringSize = GetMaxKeyringSize();
                res = _CanStoreItem_InInventorySlots(KEYRING_SLOT_START,KEYRING_SLOT_START+keyringSize,dest,pProto,count,false,pItem,bag,slot);
                if (res!=EQUIP_ERR_OK)
                {
                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return res;
                }

                if (count == 0)
                {
                    if (no_similar_count==0)
                        return EQUIP_ERR_OK;

                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
                }

                res = _CanStoreItem_InInventorySlots(CURRENCYTOKEN_SLOT_START,CURRENCYTOKEN_SLOT_END,dest,pProto,count,false,pItem,bag,slot);
                if (res != EQUIP_ERR_OK)
                {
                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return res;
                }

                if (count == 0)
                {
                    if (no_similar_count == 0)
                        return EQUIP_ERR_OK;

                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
                }
            }
            else if (pProto->BagFamily & BAG_FAMILY_MASK_CURRENCY_TOKENS)
            {
                res = _CanStoreItem_InInventorySlots(CURRENCYTOKEN_SLOT_START,CURRENCYTOKEN_SLOT_END,dest,pProto,count,false,pItem,bag,slot);
                if (res!=EQUIP_ERR_OK)
                {
                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return res;
                }

                if (count == 0)
                {
                    if (no_similar_count == 0)
                        return EQUIP_ERR_OK;

                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
                }
            }

            res = _CanStoreItem_InInventorySlots(INVENTORY_SLOT_ITEM_START,INVENTORY_SLOT_ITEM_END,dest,pProto,count,false,pItem,bag,slot);
            if (res != EQUIP_ERR_OK)
            {
                if (no_space_count)
                    *no_space_count = count + no_similar_count;
                return res;
            }

            if (count == 0)
            {
                if (no_similar_count == 0)
                    return EQUIP_ERR_OK;

                if (no_space_count)
                    *no_space_count = count + no_similar_count;
                return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
            }
        }
        else                                                // equipped bag
        {
            res = _CanStoreItem_InBag(bag,dest,pProto,count,false,false,pItem,NULL_BAG,slot);
            if (res != EQUIP_ERR_OK)
                res = _CanStoreItem_InBag(bag,dest,pProto,count,false,true,pItem,NULL_BAG,slot);

            if (res != EQUIP_ERR_OK)
            {
                if (no_space_count)
                    *no_space_count = count + no_similar_count;
                return res;
            }

            if (count == 0)
            {
                if (no_similar_count == 0)
                    return EQUIP_ERR_OK;

                if (no_space_count)
                    *no_space_count = count + no_similar_count;
                return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
            }
        }
    }

    // not specific bag or have space for partly store only in specific bag

    // search stack for merge to
    if (pProto->Stackable != 1)
    {
        res = _CanStoreItem_InInventorySlots(KEYRING_SLOT_START,CURRENCYTOKEN_SLOT_END,dest,pProto,count,true,pItem,bag,slot);
        if (res != EQUIP_ERR_OK)
        {
            if (no_space_count)
                *no_space_count = count + no_similar_count;
            return res;
        }

        if (count == 0)
        {
            if (no_similar_count == 0)
                return EQUIP_ERR_OK;

            if (no_space_count)
                *no_space_count = count + no_similar_count;
            return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
        }

        res = _CanStoreItem_InInventorySlots(INVENTORY_SLOT_ITEM_START,INVENTORY_SLOT_ITEM_END,dest,pProto,count,true,pItem,bag,slot);
        if (res != EQUIP_ERR_OK)
        {
            if (no_space_count)
                *no_space_count = count + no_similar_count;
            return res;
        }

        if (count == 0)
        {
            if (no_similar_count == 0)
                return EQUIP_ERR_OK;

            if (no_space_count)
                *no_space_count = count + no_similar_count;
            return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
        }

        if (pProto->BagFamily)
        {
            for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
            {
                res = _CanStoreItem_InBag(i,dest,pProto,count,true,false,pItem,bag,slot);
                if (res != EQUIP_ERR_OK)
                    continue;

                if (count == 0)
                {
                    if (no_similar_count == 0)
                        return EQUIP_ERR_OK;

                    if (no_space_count)
                        *no_space_count = count + no_similar_count;
                    return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
                }
            }
        }

        for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            res = _CanStoreItem_InBag(i,dest,pProto,count,true,true,pItem,bag,slot);
            if (res != EQUIP_ERR_OK)
                continue;

            if (count == 0)
            {
                if (no_similar_count==0)
                    return EQUIP_ERR_OK;

                if (no_space_count)
                    *no_space_count = count + no_similar_count;
                return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
            }
        }
    }

    // search free slot - special bag case
    if (pProto->BagFamily)
    {
        if (pProto->BagFamily & BAG_FAMILY_MASK_KEYS)
        {
            uint32 keyringSize = GetMaxKeyringSize();
            res = _CanStoreItem_InInventorySlots(KEYRING_SLOT_START,KEYRING_SLOT_START+keyringSize,dest,pProto,count,false,pItem,bag,slot);
            if (res != EQUIP_ERR_OK)
            {
                if (no_space_count)
                    *no_space_count = count + no_similar_count;
                return res;
            }

            if (count == 0)
            {
                if (no_similar_count == 0)
                    return EQUIP_ERR_OK;

                if (no_space_count)
                    *no_space_count = count + no_similar_count;
                return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
            }
        }
        else if (pProto->BagFamily & BAG_FAMILY_MASK_CURRENCY_TOKENS)
        {
            res = _CanStoreItem_InInventorySlots(CURRENCYTOKEN_SLOT_START,CURRENCYTOKEN_SLOT_END,dest,pProto,count,false,pItem,bag,slot);
            if (res != EQUIP_ERR_OK)
            {
                if (no_space_count)
                    *no_space_count = count + no_similar_count;
                return res;
            }

            if (count == 0)
            {
                if (no_similar_count == 0)
                    return EQUIP_ERR_OK;

                if (no_space_count)
                    *no_space_count = count + no_similar_count;
                return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
            }
        }

        for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            res = _CanStoreItem_InBag(i,dest,pProto,count,false,false,pItem,bag,slot);
            if (res != EQUIP_ERR_OK)
                continue;

            if (count == 0)
            {
                if (no_similar_count==0)
                    return EQUIP_ERR_OK;

                if (no_space_count)
                    *no_space_count = count + no_similar_count;
                return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
            }
        }
    }

    // Normally it would be impossible to autostore not empty bags
    if (pItem && pItem->IsBag() && !((Bag*)pItem)->IsEmpty())
        return EQUIP_ERR_NONEMPTY_BAG_OVER_OTHER_BAG;

    // search free slot
    res = _CanStoreItem_InInventorySlots(INVENTORY_SLOT_ITEM_START,INVENTORY_SLOT_ITEM_END,dest,pProto,count,false,pItem,bag,slot);
    if (res != EQUIP_ERR_OK)
    {
        if (no_space_count)
            *no_space_count = count + no_similar_count;
        return res;
    }

    if (count == 0)
    {
        if (no_similar_count==0)
            return EQUIP_ERR_OK;

        if (no_space_count)
            *no_space_count = count + no_similar_count;
        return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
    }

    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        res = _CanStoreItem_InBag(i,dest,pProto,count,false,true,pItem,bag,slot);
        if (res != EQUIP_ERR_OK)
            continue;

        if (count == 0)
        {
            if (no_similar_count == 0)
                return EQUIP_ERR_OK;

            if (no_space_count)
                *no_space_count = count + no_similar_count;
            return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;
        }
    }

    if (no_space_count)
        *no_space_count = count + no_similar_count;

    return EQUIP_ERR_INVENTORY_FULL;
}

//////////////////////////////////////////////////////////////////////////
InventoryResult Player::CanStoreItems( Item **pItems,int count) const
{
    Item    *pItem2;

    // fill space table
    int inv_slot_items[INVENTORY_SLOT_ITEM_END-INVENTORY_SLOT_ITEM_START];
    int inv_bags[INVENTORY_SLOT_BAG_END-INVENTORY_SLOT_BAG_START][MAX_BAG_SIZE];
    int inv_keys[KEYRING_SLOT_END-KEYRING_SLOT_START];
    int inv_tokens[CURRENCYTOKEN_SLOT_END-CURRENCYTOKEN_SLOT_START];

    memset(inv_slot_items,0,sizeof(int)*(INVENTORY_SLOT_ITEM_END-INVENTORY_SLOT_ITEM_START));
    memset(inv_bags,0,sizeof(int)*(INVENTORY_SLOT_BAG_END-INVENTORY_SLOT_BAG_START)*MAX_BAG_SIZE);
    memset(inv_keys,0,sizeof(int)*(KEYRING_SLOT_END-KEYRING_SLOT_START));
    memset(inv_tokens,0,sizeof(int)*(CURRENCYTOKEN_SLOT_END-CURRENCYTOKEN_SLOT_START));

    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, i );

        if (pItem2 && !pItem2->IsInTrade())
        {
            inv_slot_items[i-INVENTORY_SLOT_ITEM_START] = pItem2->GetCount();
        }
    }

    for(int i = KEYRING_SLOT_START; i < KEYRING_SLOT_END; ++i)
    {
        pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, i );

        if (pItem2 && !pItem2->IsInTrade())
        {
            inv_keys[i-KEYRING_SLOT_START] = pItem2->GetCount();
        }
    }

    for(int i = CURRENCYTOKEN_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
    {
        pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, i );

        if (pItem2 && !pItem2->IsInTrade())
        {
            inv_tokens[i-CURRENCYTOKEN_SLOT_START] = pItem2->GetCount();
        }
    }

    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if(Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                pItem2 = GetItemByPos( i, j );
                if (pItem2 && !pItem2->IsInTrade())
                {
                    inv_bags[i-INVENTORY_SLOT_BAG_START][j] = pItem2->GetCount();
                }
            }
        }
    }

    // check free space for all items
    for (int k = 0; k < count; ++k)
    {
        Item  *pItem = pItems[k];

        // no item
        if (!pItem)  continue;

        DEBUG_LOG( "STORAGE: CanStoreItems %i. item = %u, count = %u", k+1, pItem->GetEntry(), pItem->GetCount());
        ItemPrototype const *pProto = pItem->GetProto();

        // strange item
        if (!pProto)
            return EQUIP_ERR_ITEM_NOT_FOUND;

        // item used
        if (pItem->HasTemporaryLoot())
            return EQUIP_ERR_ALREADY_LOOTED;

        // item it 'bind'
        if (pItem->IsBindedNotWith(this))
            return EQUIP_ERR_DONT_OWN_THAT_ITEM;

        Bag *pBag;
        ItemPrototype const *pBagProto;

        // item is 'one item only'
        InventoryResult res = CanTakeMoreSimilarItems(pItem);
        if (res != EQUIP_ERR_OK)
            return res;

        // search stack for merge to
        if (pProto->Stackable != 1)
        {
            bool b_found = false;

            for(int t = KEYRING_SLOT_START; t < KEYRING_SLOT_END; ++t)
            {
                pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, t );
                if (pItem2 && pItem2->CanBeMergedPartlyWith(pProto) == EQUIP_ERR_OK && inv_keys[t-KEYRING_SLOT_START] + pItem->GetCount() <= pProto->GetMaxStackSize())
                {
                    inv_keys[t-KEYRING_SLOT_START] += pItem->GetCount();
                    b_found = true;
                    break;
                }
            }
            if (b_found) continue;

            for(int t = CURRENCYTOKEN_SLOT_START; t < CURRENCYTOKEN_SLOT_END; ++t)
            {
                pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, t );
                if (pItem2 && pItem2->CanBeMergedPartlyWith(pProto) == EQUIP_ERR_OK && inv_tokens[t-CURRENCYTOKEN_SLOT_START] + pItem->GetCount() <= pProto->GetMaxStackSize())
                {
                    inv_tokens[t-CURRENCYTOKEN_SLOT_START] += pItem->GetCount();
                    b_found = true;
                    break;
                }
            }
            if (b_found) continue;

            for(int t = INVENTORY_SLOT_ITEM_START; t < INVENTORY_SLOT_ITEM_END; ++t)
            {
                pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, t );
                if (pItem2 && pItem2->CanBeMergedPartlyWith(pProto) == EQUIP_ERR_OK && inv_slot_items[t-INVENTORY_SLOT_ITEM_START] + pItem->GetCount() <= pProto->GetMaxStackSize())
                {
                    inv_slot_items[t-INVENTORY_SLOT_ITEM_START] += pItem->GetCount();
                    b_found = true;
                    break;
                }
            }
            if (b_found) continue;

            for(int t = INVENTORY_SLOT_BAG_START; !b_found && t < INVENTORY_SLOT_BAG_END; ++t)
            {
                pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, t );
                if (pBag)
                {
                    for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    {
                        pItem2 = GetItemByPos( t, j );
                        if (pItem2 && pItem2->CanBeMergedPartlyWith(pProto) == EQUIP_ERR_OK && inv_bags[t-INVENTORY_SLOT_BAG_START][j] + pItem->GetCount() <= pProto->GetMaxStackSize())
                        {
                            inv_bags[t-INVENTORY_SLOT_BAG_START][j] += pItem->GetCount();
                            b_found = true;
                            break;
                        }
                    }
                }
            }
            if (b_found) continue;
        }

        // special bag case
        if (pProto->BagFamily)
        {
            bool b_found = false;
            if (pProto->BagFamily & BAG_FAMILY_MASK_KEYS)
            {
                uint32 keyringSize = GetMaxKeyringSize();
                for(uint32 t = KEYRING_SLOT_START; t < KEYRING_SLOT_START+keyringSize; ++t)
                {
                    if (inv_keys[t-KEYRING_SLOT_START] == 0)
                    {
                        inv_keys[t-KEYRING_SLOT_START] = 1;
                        b_found = true;
                        break;
                    }
                }
            }

            if (b_found) continue;

            if (pProto->BagFamily & BAG_FAMILY_MASK_CURRENCY_TOKENS)
            {
                for(uint32 t = CURRENCYTOKEN_SLOT_START; t < CURRENCYTOKEN_SLOT_END; ++t)
                {
                    if (inv_tokens[t-CURRENCYTOKEN_SLOT_START] == 0)
                    {
                        inv_tokens[t-CURRENCYTOKEN_SLOT_START] = 1;
                        b_found = true;
                        break;
                    }
                }
            }

            if (b_found) continue;

            for(int t = INVENTORY_SLOT_BAG_START; !b_found && t < INVENTORY_SLOT_BAG_END; ++t)
            {
                pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, t );
                if (pBag)
                {
                    pBagProto = pBag->GetProto();

                    // not plain container check
                    if (pBagProto && (pBagProto->Class != ITEM_CLASS_CONTAINER || pBagProto->SubClass != ITEM_SUBCLASS_CONTAINER) &&
                        ItemCanGoIntoBag(pProto,pBagProto) )
                    {
                        for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
                        {
                            if (inv_bags[t-INVENTORY_SLOT_BAG_START][j] == 0)
                            {
                                inv_bags[t-INVENTORY_SLOT_BAG_START][j] = 1;
                                b_found = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (b_found) continue;
        }

        // search free slot
        bool b_found = false;
        for(int t = INVENTORY_SLOT_ITEM_START; t < INVENTORY_SLOT_ITEM_END; ++t)
        {
            if (inv_slot_items[t-INVENTORY_SLOT_ITEM_START] == 0)
            {
                inv_slot_items[t-INVENTORY_SLOT_ITEM_START] = 1;
                b_found = true;
                break;
            }
        }
        if (b_found) continue;

        // search free slot in bags
        for(int t = INVENTORY_SLOT_BAG_START; !b_found && t < INVENTORY_SLOT_BAG_END; ++t)
        {
            pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, t );
            if (pBag)
            {
                pBagProto = pBag->GetProto();

                // special bag already checked
                if (pBagProto && (pBagProto->Class != ITEM_CLASS_CONTAINER || pBagProto->SubClass != ITEM_SUBCLASS_CONTAINER))
                    continue;

                for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
                {
                    if (inv_bags[t-INVENTORY_SLOT_BAG_START][j] == 0)
                    {
                        inv_bags[t-INVENTORY_SLOT_BAG_START][j] = 1;
                        b_found = true;
                        break;
                    }
                }
            }
        }

        // no free slot found?
        if (!b_found)
            return EQUIP_ERR_INVENTORY_FULL;
    }

    return EQUIP_ERR_OK;
}

//////////////////////////////////////////////////////////////////////////
InventoryResult Player::CanEquipNewItem( uint8 slot, uint16 &dest, uint32 item, bool swap ) const
{
    dest = 0;
    Item *pItem = Item::CreateItem( item, 1, this );
    if (pItem)
    {
        InventoryResult result = CanEquipItem(slot, dest, pItem, swap );
        delete pItem;
        return result;
    }

    return EQUIP_ERR_ITEM_NOT_FOUND;
}

InventoryResult Player::CanEquipItem( uint8 slot, uint16 &dest, Item *pItem, bool swap, bool direct_action ) const
{
    dest = 0;
    if (pItem)
    {
        DEBUG_LOG( "STORAGE: CanEquipItem slot = %u, item = %u, count = %u", slot, pItem->GetEntry(), pItem->GetCount());
        ItemPrototype const *pProto = pItem->GetProto();
        if (pProto)
        {
            // item used
            if (pItem->HasTemporaryLoot())
                return EQUIP_ERR_ALREADY_LOOTED;

            if (pItem->IsBindedNotWith(this))
                return EQUIP_ERR_DONT_OWN_THAT_ITEM;

            // check count of items (skip for auto move for same player from bank)
            InventoryResult res = CanTakeMoreSimilarItems(pItem);
            if (res != EQUIP_ERR_OK)
                return res;

            // check this only in game
            if (direct_action)
            {
                // May be here should be more stronger checks; STUNNED checked
                // ROOT, CONFUSED, DISTRACTED, FLEEING this needs to be checked.
                if (hasUnitState(UNIT_STAT_STUNNED))
                    return EQUIP_ERR_YOU_ARE_STUNNED;

                // do not allow equipping gear except weapons, offhands, projectiles, relics in
                // - combat
                // - in-progress arenas
                if (!pProto->CanChangeEquipStateInCombat())
                {
                    if (isInCombat())
                        return EQUIP_ERR_NOT_IN_COMBAT;

                    if (BattleGround* bg = GetBattleGround())
                        if (bg->isArena() && bg->GetStatus() == STATUS_IN_PROGRESS)
                            return EQUIP_ERR_NOT_DURING_ARENA_MATCH;
                }

                // prevent equip item in process logout
                if (GetSession()->isLogingOut())
                    return EQUIP_ERR_YOU_ARE_STUNNED;

                if (isInCombat()&& pProto->Class == ITEM_CLASS_WEAPON && m_weaponChangeTimer != 0)
                    return EQUIP_ERR_CANT_DO_RIGHT_NOW;         // maybe exist better err

                if (IsNonMeleeSpellCasted(false))
                    return EQUIP_ERR_CANT_DO_RIGHT_NOW;
            }

            ScalingStatDistributionEntry const *ssd = pProto->ScalingStatDistribution ? sScalingStatDistributionStore.LookupEntry(pProto->ScalingStatDistribution) : 0;
            // check allowed level (extend range to upper values if MaxLevel more or equal max player level, this let GM set high level with 1...max range items)
            if (ssd && ssd->MaxLevel < DEFAULT_MAX_LEVEL && ssd->MaxLevel < getLevel())
                return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;

            uint8 eslot = FindEquipSlot( pProto, slot, swap );
            if (eslot == NULL_SLOT)
                return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;

            InventoryResult msg = CanUseItem(pItem , direct_action);
            if (msg != EQUIP_ERR_OK)
                return msg;
            if (!swap && GetItemByPos(INVENTORY_SLOT_BAG_0, eslot))
                return EQUIP_ERR_NO_EQUIPMENT_SLOT_AVAILABLE;

            // if swap ignore item (equipped also)
            if (InventoryResult res2 = CanEquipUniqueItem(pItem, swap ? eslot : NULL_SLOT))
                return res2;

            // check unique-equipped special item classes
            if (pProto->Class == ITEM_CLASS_QUIVER)
            {
                for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
                {
                    if (Item* pBag = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    {
                        if (pBag != pItem)
                        {
                            if (ItemPrototype const* pBagProto = pBag->GetProto())
                            {
                                if (pBagProto->Class==pProto->Class && (!swap || pBag->GetSlot() != eslot))
                                    return (pBagProto->SubClass == ITEM_SUBCLASS_AMMO_POUCH)
                                        ? EQUIP_ERR_CAN_EQUIP_ONLY1_AMMOPOUCH
                                        : EQUIP_ERR_CAN_EQUIP_ONLY1_QUIVER;
                            }
                        }
                    }
                }
            }

            uint32 type = pProto->InventoryType;

            if (eslot == EQUIPMENT_SLOT_OFFHAND)
            {
                if (type == INVTYPE_WEAPON || type == INVTYPE_WEAPONOFFHAND)
                {
                    if (!CanDualWield())
                        return EQUIP_ERR_CANT_DUAL_WIELD;
                }
                else if (type == INVTYPE_2HWEAPON)
                {
                    if (!CanDualWield() || !CanTitanGrip())
                        return EQUIP_ERR_CANT_DUAL_WIELD;
                }

                if (IsTwoHandUsed())
                    return EQUIP_ERR_CANT_EQUIP_WITH_TWOHANDED;
            }

            // equip two-hand weapon case (with possible unequip 2 items)
            if (type == INVTYPE_2HWEAPON)
            {
                if (eslot == EQUIPMENT_SLOT_OFFHAND)
                {
                    if (!CanTitanGrip())
                        return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;
                }
                else if (eslot != EQUIPMENT_SLOT_MAINHAND)
                    return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;

                if (!CanTitanGrip())
                {
                    // offhand item must can be stored in inventory for offhand item and it also must be unequipped
                    Item *offItem = GetItemByPos( INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND );
                    ItemPosCountVec off_dest;
                    if (offItem && (!direct_action ||
                        CanUnequipItem(uint16(INVENTORY_SLOT_BAG_0) << 8 | EQUIPMENT_SLOT_OFFHAND,false) !=  EQUIP_ERR_OK ||
                        CanStoreItem( NULL_BAG, NULL_SLOT, off_dest, offItem, false ) !=  EQUIP_ERR_OK ))
                        return swap ? EQUIP_ERR_ITEMS_CANT_BE_SWAPPED : EQUIP_ERR_INVENTORY_FULL;
                }
            }
            dest = ((INVENTORY_SLOT_BAG_0 << 8) | eslot);
            return EQUIP_ERR_OK;
        }
    }

    return !swap ? EQUIP_ERR_ITEM_NOT_FOUND : EQUIP_ERR_ITEMS_CANT_BE_SWAPPED;
}

InventoryResult Player::CanUnequipItem( uint16 pos, bool swap ) const
{
    // Applied only to equipped items and bank bags
    if (!IsEquipmentPos(pos) && !IsBagPos(pos))
        return EQUIP_ERR_OK;

    Item* pItem = GetItemByPos(pos);

    // Applied only to existing equipped item
    if (!pItem)
        return EQUIP_ERR_OK;

    DEBUG_LOG( "STORAGE: CanUnequipItem slot = %u, item = %u, count = %u", pos, pItem->GetEntry(), pItem->GetCount());

    ItemPrototype const *pProto = pItem->GetProto();
    if (!pProto)
        return EQUIP_ERR_ITEM_NOT_FOUND;

    // item used
    if (pItem->HasTemporaryLoot())
        return EQUIP_ERR_ALREADY_LOOTED;

    // do not allow unequipping gear except weapons, offhands, projectiles, relics in
    // - combat
    // - in-progress arenas
    if( !pProto->CanChangeEquipStateInCombat() )
    {
        if( isInCombat() )
            return EQUIP_ERR_NOT_IN_COMBAT;

        if(BattleGround* bg = GetBattleGround())
            if( bg->isArena() && bg->GetStatus() == STATUS_IN_PROGRESS )
                return EQUIP_ERR_NOT_DURING_ARENA_MATCH;
    }

    // prevent unequip item in process logout
    if (GetSession()->isLogingOut())
        return EQUIP_ERR_YOU_ARE_STUNNED;

    if(!swap && pItem->IsBag() && !((Bag*)pItem)->IsEmpty())
        return EQUIP_ERR_CAN_ONLY_DO_WITH_EMPTY_BAGS;

    return EQUIP_ERR_OK;
}

InventoryResult Player::CanBankItem( uint8 bag, uint8 slot, ItemPosCountVec &dest, Item *pItem, bool swap, bool not_loading ) const
{
    if (!pItem)
        return swap ? EQUIP_ERR_ITEMS_CANT_BE_SWAPPED : EQUIP_ERR_ITEM_NOT_FOUND;

    uint32 count = pItem->GetCount();

    DEBUG_LOG( "STORAGE: CanBankItem bag = %u, slot = %u, item = %u, count = %u", bag, slot, pItem->GetEntry(), pItem->GetCount());
    ItemPrototype const *pProto = pItem->GetProto();
    if (!pProto)
        return swap ? EQUIP_ERR_ITEMS_CANT_BE_SWAPPED : EQUIP_ERR_ITEM_NOT_FOUND;

    // item used
    if (pItem->HasTemporaryLoot())
        return EQUIP_ERR_ALREADY_LOOTED;

    if (pItem->IsBindedNotWith(this))
        return EQUIP_ERR_DONT_OWN_THAT_ITEM;

    // check count of items (skip for auto move for same player from bank)
    InventoryResult res = CanTakeMoreSimilarItems(pItem);
    if (res != EQUIP_ERR_OK)
        return res;

    // in specific slot
    if (bag != NULL_BAG && slot != NULL_SLOT)
    {
        if (slot >= BANK_SLOT_BAG_START && slot < BANK_SLOT_BAG_END)
        {
            if (!pItem->IsBag())
                return EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT;

            if (slot - BANK_SLOT_BAG_START >= GetBankBagSlotCount())
                return EQUIP_ERR_MUST_PURCHASE_THAT_BAG_SLOT;

            res = CanUseItem( pItem, not_loading );
            if (res != EQUIP_ERR_OK)
                return res;
        }

        res = _CanStoreItem_InSpecificSlot(bag,slot,dest,pProto,count,swap,pItem);
        if (res!=EQUIP_ERR_OK)
            return res;

        if (count==0)
            return EQUIP_ERR_OK;
    }

    // not specific slot or have space for partly store only in specific slot

    // in specific bag
    if( bag != NULL_BAG )
    {
        if( pProto->InventoryType == INVTYPE_BAG )
        {
            Bag *pBag = (Bag*)pItem;
            if( pBag && !pBag->IsEmpty() )
                return EQUIP_ERR_NONEMPTY_BAG_OVER_OTHER_BAG;
        }

        // search stack in bag for merge to
        if( pProto->Stackable != 1 )
        {
            if( bag == INVENTORY_SLOT_BAG_0 )
            {
                res = _CanStoreItem_InInventorySlots(BANK_SLOT_ITEM_START,BANK_SLOT_ITEM_END,dest,pProto,count,true,pItem,bag,slot);
                if(res!=EQUIP_ERR_OK)
                    return res;

                if(count==0)
                    return EQUIP_ERR_OK;
            }
            else
            {
                res = _CanStoreItem_InBag(bag,dest,pProto,count,true,false,pItem,NULL_BAG,slot);
                if(res!=EQUIP_ERR_OK)
                    res = _CanStoreItem_InBag(bag,dest,pProto,count,true,true,pItem,NULL_BAG,slot);

                if(res!=EQUIP_ERR_OK)
                    return res;

                if(count==0)
                    return EQUIP_ERR_OK;
            }
        }

        // search free slot in bag
        if( bag == INVENTORY_SLOT_BAG_0 )
        {
            res = _CanStoreItem_InInventorySlots(BANK_SLOT_ITEM_START,BANK_SLOT_ITEM_END,dest,pProto,count,false,pItem,bag,slot);
            if(res!=EQUIP_ERR_OK)
                return res;

            if(count==0)
                return EQUIP_ERR_OK;
        }
        else
        {
            res = _CanStoreItem_InBag(bag, dest, pProto, count, false, false, pItem, NULL_BAG, slot);
            if(res != EQUIP_ERR_OK)
                res = _CanStoreItem_InBag(bag, dest, pProto, count, false, true, pItem, NULL_BAG, slot);

            if(res != EQUIP_ERR_OK)
                return res;

            if(count == 0)
                return EQUIP_ERR_OK;
        }
    }

    // not specific bag or have space for partly store only in specific bag

    // search stack for merge to
    if( pProto->Stackable != 1 )
    {
        // in slots
        res = _CanStoreItem_InInventorySlots(BANK_SLOT_ITEM_START,BANK_SLOT_ITEM_END,dest,pProto,count,true,pItem,bag,slot);
        if(res != EQUIP_ERR_OK)
            return res;

        if(count == 0)
            return EQUIP_ERR_OK;

        // in special bags
        if( pProto->BagFamily )
        {
            for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
            {
                res = _CanStoreItem_InBag(i,dest,pProto,count,true,false,pItem,bag,slot);
                if(res!=EQUIP_ERR_OK)
                    continue;

                if(count==0)
                    return EQUIP_ERR_OK;
            }
        }

        for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            res = _CanStoreItem_InBag(i,dest,pProto,count,true,true,pItem,bag,slot);
            if(res!=EQUIP_ERR_OK)
                continue;

            if(count==0)
                return EQUIP_ERR_OK;
        }
    }

    // search free place in special bag
    if( pProto->BagFamily )
    {
        for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            res = _CanStoreItem_InBag(i,dest,pProto,count,false,false,pItem,bag,slot);
            if(res!=EQUIP_ERR_OK)
                continue;

            if(count==0)
                return EQUIP_ERR_OK;
        }
    }

    // search free space
    res = _CanStoreItem_InInventorySlots(BANK_SLOT_ITEM_START,BANK_SLOT_ITEM_END,dest,pProto,count,false,pItem,bag,slot);
    if(res!=EQUIP_ERR_OK)
        return res;

    if(count==0)
        return EQUIP_ERR_OK;

    for(int i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
    {
        res = _CanStoreItem_InBag(i,dest,pProto,count,false,true,pItem,bag,slot);
        if(res!=EQUIP_ERR_OK)
            continue;

        if(count==0)
            return EQUIP_ERR_OK;
    }
    return EQUIP_ERR_BANK_FULL;
}

InventoryResult Player::CanUseItem(Item *pItem, bool direct_action) const
{
    if (pItem)
    {
        DEBUG_LOG( "STORAGE: CanUseItem item = %u", pItem->GetEntry());

        if (!isAlive() && direct_action)
            return EQUIP_ERR_YOU_ARE_DEAD;

        //if (isStunned())
        //    return EQUIP_ERR_YOU_ARE_STUNNED;

        ItemPrototype const *pProto = pItem->GetProto();
        if (pProto)
        {
            if (pItem->IsBindedNotWith(this))
                return EQUIP_ERR_DONT_OWN_THAT_ITEM;

            InventoryResult msg = CanUseItem(pProto);
            if (msg != EQUIP_ERR_OK)
                return msg;

            if (uint32 item_use_skill = pItem->GetSkill())
            {
                if (GetSkillValue(item_use_skill) == 0)
                {
                    // armor items with scaling stats can downgrade armor skill reqs if related class can learn armor use at some level
                    if (pProto->Class != ITEM_CLASS_ARMOR)
                        return EQUIP_ERR_NO_REQUIRED_PROFICIENCY;

                    ScalingStatDistributionEntry const *ssd = pProto->ScalingStatDistribution ? sScalingStatDistributionStore.LookupEntry(pProto->ScalingStatDistribution) : NULL;
                    if (!ssd)
                        return EQUIP_ERR_NO_REQUIRED_PROFICIENCY;

                    bool allowScaleSkill = false;
                    for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
                    {
                        SkillLineAbilityEntry const *skillInfo = sSkillLineAbilityStore.LookupEntry(i);
                        if (!skillInfo)
                            continue;

                        if (skillInfo->skillId != item_use_skill)
                            continue;

                        // can't learn
                        if (skillInfo->classmask && (skillInfo->classmask & getClassMask()) == 0)
                            continue;

                        if (skillInfo->racemask && (skillInfo->racemask & getRaceMask()) == 0)
                            continue;

                        allowScaleSkill = true;
                        break;
                    }

                    if (!allowScaleSkill)
                        return EQUIP_ERR_NO_REQUIRED_PROFICIENCY;
                }
            }

            if (pProto->RequiredReputationFaction && uint32(GetReputationRank(pProto->RequiredReputationFaction)) < pProto->RequiredReputationRank)
                return EQUIP_ERR_CANT_EQUIP_REPUTATION;

            return EQUIP_ERR_OK;
        }
    }
    return EQUIP_ERR_ITEM_NOT_FOUND;
}

InventoryResult Player::CanUseItem( ItemPrototype const *pProto ) const
{
    // Used by group, function NeedBeforeGreed, to know if a prototype can be used by a player

    if( pProto )
    {
        if ((pProto->Flags2 & ITEM_FLAG2_HORDE_ONLY) && GetTeam() != HORDE)
            return EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM;

        if ((pProto->Flags2 & ITEM_FLAG2_ALLIANCE_ONLY) && GetTeam() != ALLIANCE)
            return EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM;

        if ((pProto->AllowableClass & getClassMask()) == 0 || (pProto->AllowableRace & getRaceMask()) == 0)
            return EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM;

        if( pProto->RequiredSkill != 0  )
        {
            if( GetSkillValue( pProto->RequiredSkill ) == 0 )
                return EQUIP_ERR_NO_REQUIRED_PROFICIENCY;
            else if( GetSkillValue( pProto->RequiredSkill ) < pProto->RequiredSkillRank )
                return EQUIP_ERR_CANT_EQUIP_SKILL;
        }

        if( pProto->RequiredSpell != 0 && !HasSpell( pProto->RequiredSpell ) )
            return EQUIP_ERR_NO_REQUIRED_PROFICIENCY;

        if( getLevel() < pProto->RequiredLevel )
            return EQUIP_ERR_CANT_EQUIP_LEVEL_I;

        return EQUIP_ERR_OK;
    }
    return EQUIP_ERR_ITEM_NOT_FOUND;
}

InventoryResult Player::CanUseAmmo( uint32 item ) const
{
    DEBUG_LOG( "STORAGE: CanUseAmmo item = %u", item);
    if( !isAlive() )
        return EQUIP_ERR_YOU_ARE_DEAD;
    //if( isStunned() )
    //    return EQUIP_ERR_YOU_ARE_STUNNED;
    ItemPrototype const *pProto = ObjectMgr::GetItemPrototype( item );
    if( pProto )
    {
        if( pProto->InventoryType!= INVTYPE_AMMO )
            return EQUIP_ERR_ONLY_AMMO_CAN_GO_HERE;

        InventoryResult msg = CanUseItem(pProto);
        if (msg != EQUIP_ERR_OK)
            return msg;

        /*if ( GetReputationMgr().GetReputation() < pProto->RequiredReputation )
        return EQUIP_ERR_CANT_EQUIP_REPUTATION;
        */

        // Requires No Ammo
        if(GetDummyAura(46699))
            return EQUIP_ERR_BAG_FULL6;

        return EQUIP_ERR_OK;
    }
    return EQUIP_ERR_ITEM_NOT_FOUND;
}

void Player::SetAmmo( uint32 item )
{
    if(!item)
        return;

    // already set
    if( GetUInt32Value(PLAYER_AMMO_ID) == item )
        return;

    // check ammo
    if (item)
    {
        InventoryResult msg = CanUseAmmo( item );
        if (msg != EQUIP_ERR_OK)
        {
            SendEquipError(msg, NULL, NULL, item);
            return;
        }
    }

    SetUInt32Value(PLAYER_AMMO_ID, item);

    _ApplyAmmoBonuses();
}

void Player::RemoveAmmo()
{
    SetUInt32Value(PLAYER_AMMO_ID, 0);

    m_ammoDPS = 0.0f;

    if (CanModifyStats())
        UpdateDamagePhysical(RANGED_ATTACK);
}

// Return stored item (if stored to stack, it can diff. from pItem). And pItem ca be deleted in this case.
Item* Player::StoreNewItem( ItemPosCountVec const& dest, uint32 item, bool update,int32 randomPropertyId )
{
    uint32 count = 0;
    for(ItemPosCountVec::const_iterator itr = dest.begin(); itr != dest.end(); ++itr)
        count += itr->count;

    Item *pItem = Item::CreateItem(item, count, this, randomPropertyId);
    if (pItem)
    {
        ItemAddedQuestCheck( item, count );
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM, item, count);
        pItem = StoreItem( dest, pItem, update );
    }
    return pItem;
}

Item* Player::StoreItem( ItemPosCountVec const& dest, Item* pItem, bool update )
{
    if( !pItem )
        return NULL;

    Item* lastItem = pItem;
    uint32 entry = pItem->GetEntry();
    for(ItemPosCountVec::const_iterator itr = dest.begin(); itr != dest.end(); )
    {
        uint16 pos = itr->pos;
        uint32 count = itr->count;

        ++itr;

        if(itr == dest.end())
        {
            lastItem = _StoreItem(pos,pItem,count,false,update);
            break;
        }

        lastItem = _StoreItem(pos,pItem,count,true,update);
    }
    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM, entry);
    return lastItem;
}

// Return stored item (if stored to stack, it can diff. from pItem). And pItem ca be deleted in this case.
Item* Player::_StoreItem( uint16 pos, Item *pItem, uint32 count, bool clone, bool update )
{
    if( !pItem )
        return NULL;

    uint8 bag = pos >> 8;
    uint8 slot = pos & 255;

    DEBUG_LOG( "STORAGE: StoreItem bag = %u, slot = %u, item = %u, count = %u", bag, slot, pItem->GetEntry(), count);

    Item *pItem2 = GetItemByPos( bag, slot );

    if (!pItem2)
    {
        if (clone)
            pItem = pItem->CloneItem(count, this);
        else
            pItem->SetCount(count);

        if (!pItem)
            return NULL;

        if (pItem->GetProto()->Bonding == BIND_WHEN_PICKED_UP ||
            pItem->GetProto()->Bonding == BIND_QUEST_ITEM ||
            (pItem->GetProto()->Bonding == BIND_WHEN_EQUIPPED && IsBagPos(pos)))
            pItem->SetBinding( true );

        if (bag == INVENTORY_SLOT_BAG_0)
        {
            m_items[slot] = pItem;
            SetGuidValue(PLAYER_FIELD_INV_SLOT_HEAD + (slot * 2), pItem->GetObjectGuid());
            pItem->SetGuidValue(ITEM_FIELD_CONTAINED, GetObjectGuid());
            pItem->SetGuidValue(ITEM_FIELD_OWNER, GetObjectGuid());

            pItem->SetSlot( slot );
            pItem->SetContainer( NULL );

            // need update known currency
            if (slot >= CURRENCYTOKEN_SLOT_START && slot < CURRENCYTOKEN_SLOT_END)
                UpdateKnownCurrencies(pItem->GetEntry(), true);

            if (IsInWorld() && update)
            {
                pItem->AddToWorld();
                pItem->SendCreateUpdateToPlayer( this );
            }

            pItem->SetState(ITEM_CHANGED, this);
        }
        else if (Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, bag ))
        {
            pBag->StoreItem( slot, pItem, update );
            if( IsInWorld() && update )
            {
                pItem->AddToWorld();
                pItem->SendCreateUpdateToPlayer( this );
            }
            pItem->SetState(ITEM_CHANGED, this);
            pBag->SetState(ITEM_CHANGED, this);
        }

        AddEnchantmentDurations(pItem);
        AddItemDurations(pItem);

        // at place into not appropriate slot (bank, for example) remove aura
        ApplyItemOnStoreSpell(pItem, IsEquipmentPos(pItem->GetBagSlot(), pItem->GetSlot()) || IsInventoryPos(pItem->GetBagSlot(), pItem->GetSlot()));

        return pItem;
    }
    else
    {
        if (pItem2->GetProto()->Bonding == BIND_WHEN_PICKED_UP ||
            pItem2->GetProto()->Bonding == BIND_QUEST_ITEM ||
            (pItem2->GetProto()->Bonding == BIND_WHEN_EQUIPPED && IsBagPos(pos)))
            pItem2->SetBinding(true);

        pItem2->SetCount( pItem2->GetCount() + count );
        if (IsInWorld() && update)
            pItem2->SendCreateUpdateToPlayer( this );

        if (!clone)
        {
            // delete item (it not in any slot currently)
            if (IsInWorld() && update)
            {
                pItem->RemoveFromWorld();
                pItem->DestroyForPlayer( this );
            }

            RemoveEnchantmentDurations(pItem);
            RemoveItemDurations(pItem);

            pItem->SetOwnerGuid(GetObjectGuid());           // prevent error at next SetState in case trade/mail/buy from vendor
            pItem->SetState(ITEM_REMOVED, this);
        }

        // AddItemDurations(pItem2); - pItem2 already have duration listed for player
        AddEnchantmentDurations(pItem2);

        pItem2->SetState(ITEM_CHANGED, this);

        return pItem2;
    }
}

Item* Player::EquipNewItem( uint16 pos, uint32 item, bool update )
{
    if (Item *pItem = Item::CreateItem(item, 1, this))
    {
        ItemAddedQuestCheck( item, 1 );
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM, item, 1);
        return EquipItem( pos, pItem, update );
    }

    return NULL;
}

Item* Player::EquipItem( uint16 pos, Item *pItem, bool update )
{
    AddEnchantmentDurations(pItem);
    AddItemDurations(pItem);

    uint8 bag = pos >> 8;
    uint8 slot = pos & 255;

    Item *pItem2 = GetItemByPos(bag, slot);
    if (!pItem2)
    {
        VisualizeItem( slot, pItem);

        if (isAlive())
        {
            ItemPrototype const *pProto = pItem->GetProto();

            // item set bonuses applied only at equip and removed at unequip, and still active for broken items
            if (pProto && pProto->ItemSet)
                AddItemsSetItem(this, pItem);

            _ApplyItemMods(pItem, slot, true);

            ApplyItemOnStoreSpell(pItem, true);

            // Weapons and also Totem/Relic/Sigil/etc
            if (pProto && isInCombat() && (pProto->Class == ITEM_CLASS_WEAPON || pProto->InventoryType == INVTYPE_RELIC) && m_weaponChangeTimer == 0)
            {
                uint32 cooldownSpell = SPELL_ID_WEAPON_SWITCH_COOLDOWN_1_5s;

                if (getClass() == CLASS_ROGUE)
                    cooldownSpell = SPELL_ID_WEAPON_SWITCH_COOLDOWN_1_0s;

                SpellEntry const* spellProto = sSpellStore.LookupEntry(cooldownSpell);

                if (!spellProto)
                    sLog.outError("Weapon switch cooldown spell %u couldn't be found in Spell.dbc", cooldownSpell);
                else
                {
                    m_weaponChangeTimer = spellProto->StartRecoveryTime;

                    WorldPacket data(SMSG_SPELL_COOLDOWN, 8+1+4);
                    data << GetObjectGuid();
                    data << uint8(1);
                    data << uint32(cooldownSpell);
                    data << uint32(0);
                    GetSession()->SendPacket(&data);
                }
            }
        }

        if( IsInWorld() && update )
        {
            pItem->AddToWorld();
            pItem->SendCreateUpdateToPlayer( this );
        }

        ApplyEquipCooldown(pItem);

        if( slot == EQUIPMENT_SLOT_MAINHAND )
        {
            UpdateExpertise(BASE_ATTACK);
            UpdateArmorPenetration();
        }
        else if( slot == EQUIPMENT_SLOT_OFFHAND )
        {
            UpdateExpertise(OFF_ATTACK);
            UpdateArmorPenetration();
        }
    }
    else
    {
        pItem2->SetCount( pItem2->GetCount() + pItem->GetCount() );
        if( IsInWorld() && update )
            pItem2->SendCreateUpdateToPlayer( this );

        // delete item (it not in any slot currently)
        //pItem->DeleteFromDB();
        if( IsInWorld() && update )
        {
            pItem->RemoveFromWorld();
            pItem->DestroyForPlayer( this );
        }

        RemoveEnchantmentDurations(pItem);
        RemoveItemDurations(pItem);

        pItem->SetOwnerGuid(GetObjectGuid());               // prevent error at next SetState in case trade/mail/buy from vendor
        pItem->SetState(ITEM_REMOVED, this);
        pItem2->SetState(ITEM_CHANGED, this);

        ApplyEquipCooldown(pItem2);

        return pItem2;
    }
    // Apply Titan's Grip damage penalty if necessary
    if ((slot == EQUIPMENT_SLOT_MAINHAND || slot == EQUIPMENT_SLOT_OFFHAND) && CanTitanGrip() && HasTwoHandWeaponInOneHand() && !HasAura(49152))
        CastSpell(this, 49152, true);

    // only for full equip instead adding to stack
    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM, pItem->GetEntry());
    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM, slot+1);

    return pItem;
}

void Player::QuickEquipItem( uint16 pos, Item *pItem)
{
    if( pItem )
    {
        AddEnchantmentDurations(pItem);
        AddItemDurations(pItem);
        ApplyItemOnStoreSpell(pItem, true);

        uint8 slot = pos & 255;
        VisualizeItem( slot, pItem);

        if( IsInWorld() )
        {
            pItem->AddToWorld();
            pItem->SendCreateUpdateToPlayer( this );
        }
        // Apply Titan's Grip damage penalty if necessary
        if ((slot == EQUIPMENT_SLOT_MAINHAND || slot == EQUIPMENT_SLOT_OFFHAND) && CanTitanGrip() && HasTwoHandWeaponInOneHand() && !HasAura(49152))
            CastSpell(this, 49152, true);

        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM, pItem->GetEntry());
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM, slot+1);
    }
}

void Player::SetVisibleItemSlot(uint8 slot, Item *pItem)
{
    if(pItem)
    {
        SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + (slot * 2), pItem->GetEntry());
        SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (slot * 2), 0, pItem->GetEnchantmentId(PERM_ENCHANTMENT_SLOT));
        SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (slot * 2), 1, pItem->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT));
    }
    else
    {
        SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + (slot * 2), 0);
        SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (slot * 2), 0);
    }
}

void Player::VisualizeItem( uint8 slot, Item *pItem)
{
    if(!pItem)
        return;

    // check also  BIND_WHEN_PICKED_UP and BIND_QUEST_ITEM for .additem or .additemset case by GM (not binded at adding to inventory)
    if( pItem->GetProto()->Bonding == BIND_WHEN_EQUIPPED || pItem->GetProto()->Bonding == BIND_WHEN_PICKED_UP || pItem->GetProto()->Bonding == BIND_QUEST_ITEM )
        pItem->SetBinding( true );

    DEBUG_LOG( "STORAGE: EquipItem slot = %u, item = %u", slot, pItem->GetEntry());

    m_items[slot] = pItem;
    SetGuidValue(PLAYER_FIELD_INV_SLOT_HEAD + (slot * 2), pItem->GetObjectGuid());
    pItem->SetGuidValue(ITEM_FIELD_CONTAINED, GetObjectGuid());
    pItem->SetGuidValue(ITEM_FIELD_OWNER, GetObjectGuid());
    pItem->SetSlot( slot );
    pItem->SetContainer( NULL );

    if( slot < EQUIPMENT_SLOT_END )
        SetVisibleItemSlot(slot, pItem);

    pItem->SetState(ITEM_CHANGED, this);
}

void Player::RemoveItem( uint8 bag, uint8 slot, bool update )
{
    // note: removeitem does not actually change the item
    // it only takes the item out of storage temporarily
    // note2: if removeitem is to be used for delinking
    // the item must be removed from the player's updatequeue

    if (Item *pItem = GetItemByPos(bag, slot))
    {
        DEBUG_LOG( "STORAGE: RemoveItem bag = %u, slot = %u, item = %u", bag, slot, pItem->GetEntry());

        RemoveEnchantmentDurations(pItem);
        RemoveItemDurations(pItem);

        if( bag == INVENTORY_SLOT_BAG_0 )
        {
            if ( slot < INVENTORY_SLOT_BAG_END )
            {
                ItemPrototype const *pProto = pItem->GetProto();
                // item set bonuses applied only at equip and removed at unequip, and still active for broken items

                if(pProto && pProto->ItemSet)
                    RemoveItemsSetItem(this, pProto);

                _ApplyItemMods(pItem, slot, false);

                // remove item dependent auras and casts (only weapon and armor slots)
                if(slot < EQUIPMENT_SLOT_END)
                {
                    RemoveItemDependentAurasAndCasts(pItem);

                    // remove held enchantments, update expertise
                    if ( slot == EQUIPMENT_SLOT_MAINHAND )
                    {
                        if (pItem->GetItemSuffixFactor())
                        {
                            pItem->ClearEnchantment(PROP_ENCHANTMENT_SLOT_3);
                            pItem->ClearEnchantment(PROP_ENCHANTMENT_SLOT_4);
                        }
                        else
                        {
                            pItem->ClearEnchantment(PROP_ENCHANTMENT_SLOT_0);
                            pItem->ClearEnchantment(PROP_ENCHANTMENT_SLOT_1);
                        }

                        UpdateExpertise(BASE_ATTACK);
                        UpdateArmorPenetration();
                    }
                    else if( slot == EQUIPMENT_SLOT_OFFHAND )
                    {
                        UpdateExpertise(OFF_ATTACK);
                        UpdateArmorPenetration();
                    }
                }
            }
            // need update known currency
            else if (slot >= CURRENCYTOKEN_SLOT_START && slot < CURRENCYTOKEN_SLOT_END)
                UpdateKnownCurrencies(pItem->GetEntry(), false);

            m_items[slot] = NULL;
            SetGuidValue(PLAYER_FIELD_INV_SLOT_HEAD + (slot * 2), ObjectGuid());

            if ( slot < EQUIPMENT_SLOT_END )
            {
                SetVisibleItemSlot(slot, NULL);
                // Remove Titan's Grip damage penalty if necessary
                if ((slot == EQUIPMENT_SLOT_MAINHAND || slot == EQUIPMENT_SLOT_OFFHAND) && CanTitanGrip() && !HasTwoHandWeaponInOneHand())
                    RemoveAurasDueToSpell(49152);
            }
        }
        else
        {
            Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, bag );
            if( pBag )
                pBag->RemoveItem(slot, update);
        }
        pItem->SetGuidValue(ITEM_FIELD_CONTAINED, ObjectGuid());
        // pItem->SetGuidValue(ITEM_FIELD_OWNER, ObjectGuid()); not clear owner at remove (it will be set at store). This used in mail and auction code
        pItem->SetSlot( NULL_SLOT );

        //ApplyItemOnStoreSpell, for avoid re-apply will remove at _adding_ to not appropriate slot

        if (IsInWorld() && update)
            pItem->SendCreateUpdateToPlayer( this );
    }
}

// Common operation need to remove item from inventory without delete in trade, auction, guild bank, mail....
void Player::MoveItemFromInventory(uint8 bag, uint8 slot, bool update)
{
    if(Item* it = GetItemByPos(bag,slot))
    {
        ItemRemovedQuestCheck(it->GetEntry(), it->GetCount());
        RemoveItem(bag, slot, update);

        // item atStore spell not removed in RemoveItem (for avoid reappaly in slots changes), so do it directly
        if (IsEquipmentPos(bag, slot) || IsInventoryPos(bag, slot))
            ApplyItemOnStoreSpell(it, false);

        it->RemoveFromUpdateQueueOf(this);
        if(it->IsInWorld())
        {
            it->RemoveFromWorld();
            it->DestroyForPlayer( this );
        }
    }
}

// Common operation need to add item from inventory without delete in trade, guild bank, mail....
void Player::MoveItemToInventory(ItemPosCountVec const& dest, Item* pItem, bool update, bool in_characterInventoryDB)
{
    // update quest counters
    ItemAddedQuestCheck(pItem->GetEntry(), pItem->GetCount());
    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM, pItem->GetEntry(), pItem->GetCount());

    // store item
    Item* pLastItem = StoreItem(dest, pItem, update);

    // only set if not merged to existing stack (pItem can be deleted already but we can compare pointers any way)
    if(pLastItem == pItem)
    {
        // update owner for last item (this can be original item with wrong owner
        if(pLastItem->GetOwnerGuid() != GetObjectGuid())
            pLastItem->SetOwnerGuid(GetObjectGuid());

        // if this original item then it need create record in inventory
        // in case trade we already have item in other player inventory
        pLastItem->SetState(in_characterInventoryDB ? ITEM_CHANGED : ITEM_NEW, this);
    }
}

void Player::DestroyItem( uint8 bag, uint8 slot, bool update )
{
    Item *pItem = GetItemByPos( bag, slot );
    if( pItem )
    {
        DEBUG_LOG( "STORAGE: DestroyItem bag = %u, slot = %u, item = %u", bag, slot, pItem->GetEntry());

        // start from destroy contained items (only equipped bag can have its)
        if (pItem->IsBag() && pItem->IsEquipped())          // this also prevent infinity loop if empty bag stored in bag==slot
        {
            for (int i = 0; i < MAX_BAG_SIZE; ++i)
                DestroyItem(slot, i, update);
        }

        if (pItem->HasFlag(ITEM_FIELD_FLAGS, ITEM_DYNFLAG_WRAPPED))
        {
            static SqlStatementID delGifts ;

            SqlStatement stmt = CharacterDatabase.CreateStatement(delGifts, "DELETE FROM character_gifts WHERE item_guid = ?");
            stmt.PExecute(pItem->GetGUIDLow());
        }

        RemoveEnchantmentDurations(pItem);
        RemoveItemDurations(pItem);

        if (IsEquipmentPos(bag, slot) || IsInventoryPos(bag, slot))
            ApplyItemOnStoreSpell(pItem, false);

        ItemRemovedQuestCheck( pItem->GetEntry(), pItem->GetCount() );

        if( bag == INVENTORY_SLOT_BAG_0 )
        {
            SetGuidValue(PLAYER_FIELD_INV_SLOT_HEAD + (slot * 2), ObjectGuid());

            // equipment and equipped bags can have applied bonuses
            if ( slot < INVENTORY_SLOT_BAG_END )
            {
                ItemPrototype const *pProto = pItem->GetProto();

                // item set bonuses applied only at equip and removed at unequip, and still active for broken items
                if(pProto && pProto->ItemSet)
                    RemoveItemsSetItem(this, pProto);

                _ApplyItemMods(pItem, slot, false);
            }

            if ( slot < EQUIPMENT_SLOT_END )
            {
                // remove item dependent auras and casts (only weapon and armor slots)
                RemoveItemDependentAurasAndCasts(pItem);

                // update expertise
                if( slot == EQUIPMENT_SLOT_MAINHAND )
                {
                    UpdateExpertise(BASE_ATTACK);
                    UpdateArmorPenetration();
                }
                else if( slot == EQUIPMENT_SLOT_OFFHAND )
                {
                    UpdateExpertise(OFF_ATTACK);
                    UpdateArmorPenetration();
                }

                // equipment visual show
                SetVisibleItemSlot(slot, NULL);
            }
            // need update known currency
            else if (slot >= CURRENCYTOKEN_SLOT_START && slot < CURRENCYTOKEN_SLOT_END)
                UpdateKnownCurrencies(pItem->GetEntry(), false);

            m_items[slot] = NULL;
        }
        else if(Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, bag ))
            pBag->RemoveItem(slot, update);

        if( IsInWorld() && update )
        {
            pItem->RemoveFromWorld();
            pItem->DestroyForPlayer(this);
        }

        //pItem->SetOwnerGUID(0);
        pItem->SetGuidValue(ITEM_FIELD_CONTAINED, ObjectGuid());
        pItem->SetSlot( NULL_SLOT );
        pItem->SetState(ITEM_REMOVED, this);
    }
}

void Player::DestroyItemCount( uint32 item, uint32 count, bool update, bool unequip_check)
{
    DEBUG_LOG( "STORAGE: DestroyItemCount item = %u, count = %u", item, count);
    uint32 remcount = 0;

    // in inventory
    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
        {
            if (pItem->GetEntry() == item && !pItem->IsInTrade())
            {
                if (pItem->GetCount() + remcount <= count)
                {
                    // all items in inventory can unequipped
                    remcount += pItem->GetCount();
                    DestroyItem( INVENTORY_SLOT_BAG_0, i, update);

                    if (remcount >= count)
                        return;
                }
                else
                {
                    ItemRemovedQuestCheck( pItem->GetEntry(), count - remcount );
                    pItem->SetCount( pItem->GetCount() - count + remcount );
                    if (IsInWorld() & update)
                        pItem->SendCreateUpdateToPlayer( this );
                    pItem->SetState(ITEM_CHANGED, this);
                    return;
                }
            }
        }
    }

    for(int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
    {
        if (Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
        {
            if (pItem->GetEntry() == item && !pItem->IsInTrade())
            {
                if (pItem->GetCount() + remcount <= count)
                {
                    // all keys can be unequipped
                    remcount += pItem->GetCount();
                    DestroyItem( INVENTORY_SLOT_BAG_0, i, update);

                    if (remcount >= count)
                        return;
                }
                else
                {
                    ItemRemovedQuestCheck( pItem->GetEntry(), count - remcount );
                    pItem->SetCount( pItem->GetCount() - count + remcount );
                    if (IsInWorld() & update)
                        pItem->SendCreateUpdateToPlayer( this );
                    pItem->SetState(ITEM_CHANGED, this);
                    return;
                }
            }
        }
    }

    // in inventory bags
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if(Bag *pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
        {
            for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                if(Item* pItem = pBag->GetItemByPos(j))
                {
                    if (pItem->GetEntry() == item && !pItem->IsInTrade())
                    {
                        // all items in bags can be unequipped
                        if (pItem->GetCount() + remcount <= count)
                        {
                            remcount += pItem->GetCount();
                            DestroyItem( i, j, update );

                            if (remcount >= count)
                                return;
                        }
                        else
                        {
                            ItemRemovedQuestCheck( pItem->GetEntry(), count - remcount );
                            pItem->SetCount( pItem->GetCount() - count + remcount );
                            if (IsInWorld() && update)
                                pItem->SendCreateUpdateToPlayer( this );
                            pItem->SetState(ITEM_CHANGED, this);
                            return;
                        }
                    }
                }
            }
        }
    }

    // in equipment and bag list
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
        {
            if (pItem && pItem->GetEntry() == item && !pItem->IsInTrade())
            {
                if (pItem->GetCount() + remcount <= count)
                {
                    if (!unequip_check || CanUnequipItem(INVENTORY_SLOT_BAG_0 << 8 | i, false) == EQUIP_ERR_OK )
                    {
                        remcount += pItem->GetCount();
                        DestroyItem( INVENTORY_SLOT_BAG_0, i, update);

                        if (remcount >= count)
                            return;
                    }
                }
                else
                {
                    ItemRemovedQuestCheck( pItem->GetEntry(), count - remcount );
                    pItem->SetCount( pItem->GetCount() - count + remcount );
                    if (IsInWorld() & update)
                        pItem->SendCreateUpdateToPlayer( this );
                    pItem->SetState(ITEM_CHANGED, this);
                    return;
                }
            }
        }
    }
}

void Player::DestroyZoneLimitedItem( bool update, uint32 new_zone )
{
    DEBUG_LOG( "STORAGE: DestroyZoneLimitedItem in map %u and area %u", GetMapId(), new_zone );

    // in inventory
    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        if (Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            if (pItem->IsLimitedToAnotherMapOrZone(GetMapId(), new_zone))
                DestroyItem( INVENTORY_SLOT_BAG_0, i, update);

    for(int i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
        if (Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            if (pItem->IsLimitedToAnotherMapOrZone(GetMapId(), new_zone))
                DestroyItem( INVENTORY_SLOT_BAG_0, i, update);

    // in inventory bags
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        if (Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
                if (Item* pItem = pBag->GetItemByPos(j))
                    if (pItem->IsLimitedToAnotherMapOrZone(GetMapId(), new_zone))
                        DestroyItem(i, j, update);

    // in equipment and bag list
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; ++i)
        if (Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            if (pItem->IsLimitedToAnotherMapOrZone(GetMapId(), new_zone))
                DestroyItem( INVENTORY_SLOT_BAG_0, i, update);
}

void Player::DestroyConjuredItems( bool update )
{
    // used when entering arena
    // destroys all conjured items
    DEBUG_LOG( "STORAGE: DestroyConjuredItems" );

    // in inventory
    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        if (Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            if (pItem->IsConjuredConsumable())
                DestroyItem( INVENTORY_SLOT_BAG_0, i, update);

    // in inventory bags
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        if (Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
                if (Item* pItem = pBag->GetItemByPos(j))
                    if (pItem->IsConjuredConsumable())
                        DestroyItem( i, j, update);

    // in equipment and bag list
    for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; ++i)
        if (Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            if (pItem->IsConjuredConsumable())
                DestroyItem( INVENTORY_SLOT_BAG_0, i, update);
}

void Player::DestroyItemCount( Item* pItem, uint32 &count, bool update )
{
    if(!pItem)
        return;

    DEBUG_LOG( "STORAGE: DestroyItemCount item (GUID: %u, Entry: %u) count = %u", pItem->GetGUIDLow(),pItem->GetEntry(), count);

    if( pItem->GetCount() <= count )
    {
        count -= pItem->GetCount();

        DestroyItem( pItem->GetBagSlot(),pItem->GetSlot(), update);
    }
    else
    {
        ItemRemovedQuestCheck( pItem->GetEntry(), count);
        pItem->SetCount( pItem->GetCount() - count );
        count = 0;
        if( IsInWorld() & update )
            pItem->SendCreateUpdateToPlayer( this );
        pItem->SetState(ITEM_CHANGED, this);
    }
}

void Player::SplitItem( uint16 src, uint16 dst, uint32 count )
{
    uint8 srcbag = src >> 8;
    uint8 srcslot = src & 255;

    uint8 dstbag = dst >> 8;
    uint8 dstslot = dst & 255;

    Item *pSrcItem = GetItemByPos( srcbag, srcslot );
    if (!pSrcItem)
    {
        SendEquipError( EQUIP_ERR_ITEM_NOT_FOUND, pSrcItem, NULL );
        return;
    }

    if (pSrcItem->HasGeneratedLoot())                       // prevent split looting item (stackable items can has only temporary loot and this meaning that loot window open)
    {
        //best error message found for attempting to split while looting
        SendEquipError( EQUIP_ERR_COULDNT_SPLIT_ITEMS, pSrcItem, NULL );
        return;
    }

    // not let split all items (can be only at cheating)
    if (pSrcItem->GetCount() == count)
    {
        SendEquipError( EQUIP_ERR_COULDNT_SPLIT_ITEMS, pSrcItem, NULL );
        return;
    }

    // not let split more existing items (can be only at cheating)
    if (pSrcItem->GetCount() < count)
    {
        SendEquipError( EQUIP_ERR_TRIED_TO_SPLIT_MORE_THAN_COUNT, pSrcItem, NULL );
        return;
    }

    DEBUG_LOG( "STORAGE: SplitItem bag = %u, slot = %u, item = %u, count = %u", dstbag, dstslot, pSrcItem->GetEntry(), count);
    Item *pNewItem = pSrcItem->CloneItem( count, this );
    if (!pNewItem)
    {
        SendEquipError( EQUIP_ERR_ITEM_NOT_FOUND, pSrcItem, NULL );
        return;
    }

    if (IsInventoryPos(dst))
    {
        // change item amount before check (for unique max count check)
        pSrcItem->SetCount( pSrcItem->GetCount() - count );

        ItemPosCountVec dest;
        InventoryResult msg = CanStoreItem( dstbag, dstslot, dest, pNewItem, false );
        if (msg != EQUIP_ERR_OK)
        {
            delete pNewItem;
            pSrcItem->SetCount( pSrcItem->GetCount() + count );
            SendEquipError( msg, pSrcItem, NULL );
            return;
        }

        if (IsInWorld())
            pSrcItem->SendCreateUpdateToPlayer( this );
        pSrcItem->SetState(ITEM_CHANGED, this);
        StoreItem( dest, pNewItem, true);
    }
    else if (IsBankPos (dst))
    {
        // change item amount before check (for unique max count check)
        pSrcItem->SetCount( pSrcItem->GetCount() - count );

        ItemPosCountVec dest;
        InventoryResult msg = CanBankItem( dstbag, dstslot, dest, pNewItem, false );
        if( msg != EQUIP_ERR_OK )
        {
            delete pNewItem;
            pSrcItem->SetCount( pSrcItem->GetCount() + count );
            SendEquipError( msg, pSrcItem, NULL );
            return;
        }

        if (IsInWorld())
            pSrcItem->SendCreateUpdateToPlayer( this );
        pSrcItem->SetState(ITEM_CHANGED, this);
        BankItem( dest, pNewItem, true);
    }
    else if (IsEquipmentPos (dst))
    {
        // change item amount before check (for unique max count check), provide space for splitted items
        pSrcItem->SetCount( pSrcItem->GetCount() - count );

        uint16 dest;
        InventoryResult msg = CanEquipItem( dstslot, dest, pNewItem, false );
        if (msg != EQUIP_ERR_OK)
        {
            delete pNewItem;
            pSrcItem->SetCount( pSrcItem->GetCount() + count );
            SendEquipError( msg, pSrcItem, NULL );
            return;
        }

        if (IsInWorld())
            pSrcItem->SendCreateUpdateToPlayer( this );
        pSrcItem->SetState(ITEM_CHANGED, this);
        EquipItem( dest, pNewItem, true);
        AutoUnequipOffhandIfNeed();
    }
}

void Player::SwapItem( uint16 src, uint16 dst )
{
    uint8 srcbag = src >> 8;
    uint8 srcslot = src & 255;

    uint8 dstbag = dst >> 8;
    uint8 dstslot = dst & 255;

    Item *pSrcItem = GetItemByPos( srcbag, srcslot );
    Item *pDstItem = GetItemByPos( dstbag, dstslot );

    if (!pSrcItem)
        return;

    DEBUG_LOG( "STORAGE: SwapItem bag = %u, slot = %u, item = %u", dstbag, dstslot, pSrcItem->GetEntry());

    if (!isAlive())
    {
        SendEquipError( EQUIP_ERR_YOU_ARE_DEAD, pSrcItem, pDstItem );
        return;
    }

    // SRC checks

    // check unequip potability for equipped items and bank bags
    if (IsEquipmentPos(src) || IsBagPos(src))
    {
        // bags can be swapped with empty bag slots, or with empty bag (items move possibility checked later)
        InventoryResult msg = CanUnequipItem( src, !IsBagPos ( src ) || IsBagPos ( dst ) || (pDstItem && pDstItem->IsBag() && ((Bag*)pDstItem)->IsEmpty()));
        if (msg != EQUIP_ERR_OK)
        {
            SendEquipError( msg, pSrcItem, pDstItem );
            return;
        }
    }

    // prevent put equipped/bank bag in self
    if (IsBagPos(src) && srcslot == dstbag)
    {
        SendEquipError( EQUIP_ERR_NONEMPTY_BAG_OVER_OTHER_BAG, pSrcItem, pDstItem );
        return;
    }

    // prevent put equipped/bank bag in self
    if (IsBagPos(dst) && dstslot == srcbag)
    {
        SendEquipError( EQUIP_ERR_NONEMPTY_BAG_OVER_OTHER_BAG, pDstItem, pSrcItem );
        return;
    }

    // DST checks

    if (pDstItem)
    {
        // check unequip potability for equipped items and bank bags
        if(IsEquipmentPos ( dst ) || IsBagPos ( dst ))
        {
            // bags can be swapped with empty bag slots, or with empty bag (items move possibility checked later)
            InventoryResult msg = CanUnequipItem( dst, !IsBagPos ( dst ) || IsBagPos ( src ) || (pSrcItem->IsBag() && ((Bag*)pSrcItem)->IsEmpty()));
            if(msg != EQUIP_ERR_OK)
            {
                SendEquipError( msg, pSrcItem, pDstItem );
                return;
            }
        }
    }

    // NOW this is or item move (swap with empty), or swap with another item (including bags in bag possitions)
    // or swap empty bag with another empty or not empty bag (with items exchange)

    // Move case
    if( !pDstItem )
    {
        if( IsInventoryPos( dst ) )
        {
            ItemPosCountVec dest;
            InventoryResult msg = CanStoreItem( dstbag, dstslot, dest, pSrcItem, false );
            if( msg != EQUIP_ERR_OK )
            {
                SendEquipError( msg, pSrcItem, NULL );
                return;
            }

            RemoveItem(srcbag, srcslot, true);
            StoreItem( dest, pSrcItem, true);
        }
        else if( IsBankPos ( dst ) )
        {
            ItemPosCountVec dest;
            InventoryResult msg = CanBankItem( dstbag, dstslot, dest, pSrcItem, false);
            if( msg != EQUIP_ERR_OK )
            {
                SendEquipError( msg, pSrcItem, NULL );
                return;
            }

            RemoveItem(srcbag, srcslot, true);
            BankItem( dest, pSrcItem, true);
        }
        else if( IsEquipmentPos ( dst ) )
        {
            uint16 dest;
            InventoryResult msg = CanEquipItem( dstslot, dest, pSrcItem, false );
            if( msg != EQUIP_ERR_OK )
            {
                SendEquipError( msg, pSrcItem, NULL );
                return;
            }

            RemoveItem(srcbag, srcslot, true);
            EquipItem(dest, pSrcItem, true);
            AutoUnequipOffhandIfNeed();
        }

        return;
    }

    // attempt merge to / fill target item
    if(!pSrcItem->IsBag() && !pDstItem->IsBag())
    {
        InventoryResult msg;
        ItemPosCountVec sDest;
        uint16 eDest;
        if( IsInventoryPos( dst ) )
            msg = CanStoreItem( dstbag, dstslot, sDest, pSrcItem, false );
        else if( IsBankPos ( dst ) )
            msg = CanBankItem( dstbag, dstslot, sDest, pSrcItem, false );
        else if( IsEquipmentPos ( dst ) )
            msg = CanEquipItem( dstslot, eDest, pSrcItem, false );
        else
            return;

        // can be merge/fill
        if(msg == EQUIP_ERR_OK)
        {
            if( pSrcItem->GetCount() + pDstItem->GetCount() <= pSrcItem->GetProto()->GetMaxStackSize())
            {
                RemoveItem(srcbag, srcslot, true);

                if( IsInventoryPos( dst ) )
                    StoreItem( sDest, pSrcItem, true);
                else if( IsBankPos ( dst ) )
                    BankItem( sDest, pSrcItem, true);
                else if( IsEquipmentPos ( dst ) )
                {
                    EquipItem( eDest, pSrcItem, true);
                    AutoUnequipOffhandIfNeed();
                }
            }
            else
            {
                pSrcItem->SetCount( pSrcItem->GetCount() + pDstItem->GetCount() - pSrcItem->GetProto()->GetMaxStackSize());
                pDstItem->SetCount( pSrcItem->GetProto()->GetMaxStackSize());
                pSrcItem->SetState(ITEM_CHANGED, this);
                pDstItem->SetState(ITEM_CHANGED, this);
                if( IsInWorld() )
                {
                    pSrcItem->SendCreateUpdateToPlayer( this );
                    pDstItem->SendCreateUpdateToPlayer( this );
                }
            }
            return;
        }
    }

    // impossible merge/fill, do real swap
    InventoryResult msg;

    // check src->dest move possibility
    ItemPosCountVec sDest;
    uint16 eDest = 0;
    if( IsInventoryPos( dst ) )
        msg = CanStoreItem( dstbag, dstslot, sDest, pSrcItem, true );
    else if( IsBankPos( dst ) )
        msg = CanBankItem( dstbag, dstslot, sDest, pSrcItem, true );
    else if( IsEquipmentPos( dst ) )
    {
        msg = CanEquipItem( dstslot, eDest, pSrcItem, true );
        if( msg == EQUIP_ERR_OK )
            msg = CanUnequipItem( eDest, true );
    }

    if( msg != EQUIP_ERR_OK )
    {
        SendEquipError( msg, pSrcItem, pDstItem );
        return;
    }

    // check dest->src move possibility
    ItemPosCountVec sDest2;
    uint16 eDest2 = 0;
    if( IsInventoryPos( src ) )
        msg = CanStoreItem( srcbag, srcslot, sDest2, pDstItem, true );
    else if( IsBankPos( src ) )
        msg = CanBankItem( srcbag, srcslot, sDest2, pDstItem, true );
    else if( IsEquipmentPos( src ) )
    {
        msg = CanEquipItem( srcslot, eDest2, pDstItem, true);
        if( msg == EQUIP_ERR_OK )
            msg = CanUnequipItem( eDest2, true);
    }

    if( msg != EQUIP_ERR_OK )
    {
        SendEquipError( msg, pDstItem, pSrcItem );
        return;
    }

    // Check bag swap with item exchange (one from empty in not bag possition (equipped (not possible in fact) or store)
    if(pSrcItem->IsBag() && pDstItem->IsBag())
    {
        Bag* emptyBag = NULL;
        Bag* fullBag = NULL;
        if(((Bag*)pSrcItem)->IsEmpty() && !IsBagPos(src))
        {
            emptyBag = (Bag*)pSrcItem;
            fullBag  = (Bag*)pDstItem;
        }
        else if(((Bag*)pDstItem)->IsEmpty() && !IsBagPos(dst))
        {
            emptyBag = (Bag*)pDstItem;
            fullBag  = (Bag*)pSrcItem;
        }

        // bag swap (with items exchange) case
        if(emptyBag && fullBag)
        {
            ItemPrototype const* emotyProto = emptyBag->GetProto();

            uint32 count = 0;

            for(uint32 i=0; i < fullBag->GetBagSize(); ++i)
            {
                Item *bagItem = fullBag->GetItemByPos(i);
                if (!bagItem)
                    continue;

                ItemPrototype const* bagItemProto = bagItem->GetProto();
                if (!bagItemProto || !ItemCanGoIntoBag(bagItemProto, emotyProto))
                {
                    // one from items not go to empty target bag
                    SendEquipError( EQUIP_ERR_NONEMPTY_BAG_OVER_OTHER_BAG, pSrcItem, pDstItem );
                    return;
                }

                ++count;
            }

            if (count > emptyBag->GetBagSize())
            {
                // too small targeted bag
                SendEquipError( EQUIP_ERR_ITEMS_CANT_BE_SWAPPED, pSrcItem, pDstItem );
                return;
            }

            // Items swap
            count = 0;                                      // will pos in new bag
            for(uint32 i = 0; i< fullBag->GetBagSize(); ++i)
            {
                Item *bagItem = fullBag->GetItemByPos(i);
                if (!bagItem)
                    continue;

                fullBag->RemoveItem(i, true);
                emptyBag->StoreItem(count, bagItem, true);
                bagItem->SetState(ITEM_CHANGED, this);

                ++count;
            }
        }
    }

    // now do moves, remove...
    RemoveItem(dstbag, dstslot, false);
    RemoveItem(srcbag, srcslot, false);

    // add to dest
    if (IsInventoryPos(dst))
        StoreItem(sDest, pSrcItem, true);
    else if (IsBankPos(dst))
        BankItem(sDest, pSrcItem, true);
    else if (IsEquipmentPos(dst))
        EquipItem(eDest, pSrcItem, true);

    // add to src
    if (IsInventoryPos(src))
        StoreItem(sDest2, pDstItem, true);
    else if (IsBankPos(src))
        BankItem(sDest2, pDstItem, true);
    else if (IsEquipmentPos(src))
        EquipItem(eDest2, pDstItem, true);

    AutoUnequipOffhandIfNeed();
}

void Player::AddItemToBuyBackSlot( Item *pItem )
{
    if (pItem)
    {
        uint32 slot = m_currentBuybackSlot;
        // if current back slot non-empty search oldest or free
        if (m_items[slot])
        {
            uint32 oldest_time = GetUInt32Value( PLAYER_FIELD_BUYBACK_TIMESTAMP_1 );
            uint32 oldest_slot = BUYBACK_SLOT_START;

            for(uint32 i = BUYBACK_SLOT_START+1; i < BUYBACK_SLOT_END; ++i )
            {
                // found empty
                if (!m_items[i])
                {
                    slot = i;
                    break;
                }

                uint32 i_time = GetUInt32Value( PLAYER_FIELD_BUYBACK_TIMESTAMP_1 + i - BUYBACK_SLOT_START);

                if (oldest_time > i_time)
                {
                    oldest_time = i_time;
                    oldest_slot = i;
                }
            }

            // find oldest
            slot = oldest_slot;
        }

        RemoveItemFromBuyBackSlot( slot, true );
        DEBUG_LOG( "STORAGE: AddItemToBuyBackSlot item = %u, slot = %u", pItem->GetEntry(), slot);

        m_items[slot] = pItem;
        time_t base = time(NULL);
        uint32 etime = uint32(base - m_logintime + (30 * 3600));
        uint32 eslot = slot - BUYBACK_SLOT_START;

        SetGuidValue(PLAYER_FIELD_VENDORBUYBACK_SLOT_1 + (eslot * 2), pItem->GetObjectGuid());
        if (ItemPrototype const *pProto = pItem->GetProto())
            SetUInt32Value( PLAYER_FIELD_BUYBACK_PRICE_1 + eslot, pProto->SellPrice * pItem->GetCount() );
        else
            SetUInt32Value( PLAYER_FIELD_BUYBACK_PRICE_1 + eslot, 0 );
        SetUInt32Value( PLAYER_FIELD_BUYBACK_TIMESTAMP_1 + eslot, (uint32)etime );

        // move to next (for non filled list is move most optimized choice)
        if (m_currentBuybackSlot < BUYBACK_SLOT_END - 1)
            ++m_currentBuybackSlot;
    }
}

Item* Player::GetItemFromBuyBackSlot( uint32 slot )
{
    DEBUG_LOG( "STORAGE: GetItemFromBuyBackSlot slot = %u", slot);
    if (slot >= BUYBACK_SLOT_START && slot < BUYBACK_SLOT_END)
        return m_items[slot];
    return NULL;
}

void Player::RemoveItemFromBuyBackSlot( uint32 slot, bool del )
{
    DEBUG_LOG( "STORAGE: RemoveItemFromBuyBackSlot slot = %u", slot);
    if (slot >= BUYBACK_SLOT_START && slot < BUYBACK_SLOT_END)
    {
        Item *pItem = m_items[slot];
        if (pItem)
        {
            pItem->RemoveFromWorld();
            if(del) pItem->SetState(ITEM_REMOVED, this);
        }

        m_items[slot] = NULL;

        uint32 eslot = slot - BUYBACK_SLOT_START;
        SetGuidValue(PLAYER_FIELD_VENDORBUYBACK_SLOT_1 + (eslot * 2), ObjectGuid());
        SetUInt32Value(PLAYER_FIELD_BUYBACK_PRICE_1 + eslot, 0);
        SetUInt32Value(PLAYER_FIELD_BUYBACK_TIMESTAMP_1 + eslot, 0);

        // if current backslot is filled set to now free slot
        if (m_items[m_currentBuybackSlot])
            m_currentBuybackSlot = slot;
    }
}

void Player::SendEquipError( InventoryResult msg, Item* pItem, Item *pItem2, uint32 itemid /*= 0*/ ) const
{
    DEBUG_LOG( "WORLD: Sent SMSG_INVENTORY_CHANGE_FAILURE (%u)", msg);
    WorldPacket data(SMSG_INVENTORY_CHANGE_FAILURE, 1+8+8+1);
    data << uint8(msg);

    if (msg != EQUIP_ERR_OK)
    {
        data << (pItem ? pItem->GetObjectGuid() : ObjectGuid());
        data << (pItem2 ? pItem2->GetObjectGuid() : ObjectGuid());
        data << uint8(0);                                   // bag type subclass, used with EQUIP_ERR_EVENT_AUTOEQUIP_BIND_CONFIRM and EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG2

        switch(msg)
        {
            case EQUIP_ERR_CANT_EQUIP_LEVEL_I:
            case EQUIP_ERR_PURCHASE_LEVEL_TOO_LOW:
            {
                ItemPrototype const* proto = pItem ? pItem->GetProto() : ObjectMgr::GetItemPrototype(itemid);
                data << uint32(proto ? proto->RequiredLevel : 0);
                break;
            }
            case EQUIP_ERR_EVENT_AUTOEQUIP_BIND_CONFIRM:    // no idea about this one...
            {
                data << uint64(0);
                data << uint32(0);
                data << uint64(0);
                break;
            }
            case EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS:
            case EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_SOCKETED_EXCEEDED_IS:
            case EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_EQUIPPED_EXCEEDED_IS:
            {
                ItemPrototype const* proto = pItem ? pItem->GetProto() : ObjectMgr::GetItemPrototype(itemid);
                data << uint32(proto ? proto->ItemLimitCategory : 0);
                break;
            }
            default:
                break;
        }
    }
    GetSession()->SendPacket(&data);
}

void Player::SendBuyError( BuyResult msg, Creature* pCreature, uint32 item, uint32 param )
{
    DEBUG_LOG( "WORLD: Sent SMSG_BUY_FAILED" );
    WorldPacket data( SMSG_BUY_FAILED, (8+4+4+1) );
    data << (pCreature ? pCreature->GetObjectGuid() : ObjectGuid());
    data << uint32(item);
    if (param > 0)
        data << uint32(param);
    data << uint8(msg);
    GetSession()->SendPacket(&data);
}

void Player::SendSellError( SellResult msg, Creature* pCreature, ObjectGuid itemGuid, uint32 param )
{
    DEBUG_LOG( "WORLD: Sent SMSG_SELL_ITEM" );
    WorldPacket data( SMSG_SELL_ITEM,(8+8+(param?4:0)+1));  // last check 2.0.10
    data << (pCreature ? pCreature->GetObjectGuid() : ObjectGuid());
    data << ObjectGuid(itemGuid);
    if (param > 0)
        data << uint32(param);
    data << uint8(msg);
    GetSession()->SendPacket(&data);
}

void Player::TradeCancel(bool sendback)
{
    if (m_trade)
    {
        Player* trader = m_trade->GetTrader();

        // send yellow "Trade canceled" message to both traders
        if (sendback)
            GetSession()->SendCancelTrade();

        trader->GetSession()->SendCancelTrade();

        // cleanup
        delete m_trade;
        m_trade = NULL;
        delete trader->m_trade;
        trader->m_trade = NULL;
    }
}

void Player::UpdateItemDuration(uint32 time, bool realtimeonly)
{
    if (m_itemDuration.empty())
        return;

    DEBUG_LOG("Player::UpdateItemDuration(%u,%u)", time, realtimeonly);

    for(ItemDurationList::const_iterator itr = m_itemDuration.begin(); itr != m_itemDuration.end(); )
    {
        Item* item = *itr;
        ++itr;                                              // current element can be erased in UpdateDuration

        if ((realtimeonly && (item->GetProto()->ExtraFlags & ITEM_EXTRA_REAL_TIME_DURATION)) || !realtimeonly)
            item->UpdateDuration(this,time);
    }
}

void Player::UpdateEnchantTime(uint32 time)
{
    for(EnchantDurationList::iterator itr = m_enchantDuration.begin(),next;itr != m_enchantDuration.end();itr=next)
    {
        MANGOS_ASSERT(itr->item);
        next = itr;
        if (!itr->item->GetEnchantmentId(itr->slot))
        {
            next = m_enchantDuration.erase(itr);
        }
        else if (itr->leftduration <= time)
        {
            ApplyEnchantment(itr->item, itr->slot, false, false);
            itr->item->ClearEnchantment(itr->slot);
            next = m_enchantDuration.erase(itr);
        }
        else if (itr->leftduration > time)
        {
            itr->leftduration -= time;
            ++next;
        }
    }
}

void Player::AddEnchantmentDurations(Item *item)
{
    for(int x = 0; x < MAX_ENCHANTMENT_SLOT; ++x)
    {
        if (!item->GetEnchantmentId(EnchantmentSlot(x)))
            continue;

        uint32 duration = item->GetEnchantmentDuration(EnchantmentSlot(x));
        if (duration > 0)
            AddEnchantmentDuration(item, EnchantmentSlot(x), duration);
    }
}

void Player::RemoveEnchantmentDurations(Item *item)
{
    for(EnchantDurationList::iterator itr = m_enchantDuration.begin(); itr != m_enchantDuration.end();)
    {
        if (itr->item == item)
        {
            // save duration in item
            item->SetEnchantmentDuration(EnchantmentSlot(itr->slot), itr->leftduration);
            itr = m_enchantDuration.erase(itr);
        }
        else
            ++itr;
    }
}

void Player::RemoveAllEnchantments(EnchantmentSlot slot)
{
    // remove enchantments from equipped items first to clean up the m_enchantDuration list
    for(EnchantDurationList::iterator itr = m_enchantDuration.begin(), next; itr != m_enchantDuration.end(); itr = next)
    {
        next = itr;
        if (itr->slot == slot)
        {
            if (itr->item && itr->item->GetEnchantmentId(slot))
            {
                // remove from stats
                ApplyEnchantment(itr->item, slot, false, false);
                // remove visual
                itr->item->ClearEnchantment(slot);
            }
            // remove from update list
            next = m_enchantDuration.erase(itr);
        }
        else
            ++next;
    }

    // remove enchants from inventory items
    // NOTE: no need to remove these from stats, since these aren't equipped
    // in inventory
    for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        if (Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (pItem->GetEnchantmentId(slot))
                pItem->ClearEnchantment(slot);

    // in inventory bags
    for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        if (Bag* pBag = (Bag*)GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
            for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
                if (Item* pItem = pBag->GetItemByPos(j))
                    if (pItem->GetEnchantmentId(slot))
                        pItem->ClearEnchantment(slot);
}

// duration == 0 will remove item enchant
void Player::AddEnchantmentDuration(Item *item,EnchantmentSlot slot,uint32 duration)
{
    if (!item)
        return;

    if (slot >= MAX_ENCHANTMENT_SLOT)
        return;

    for(EnchantDurationList::iterator itr = m_enchantDuration.begin(); itr != m_enchantDuration.end(); ++itr)
    {
        if (itr->item == item && itr->slot == slot)
        {
            itr->item->SetEnchantmentDuration(itr->slot, itr->leftduration);
            m_enchantDuration.erase(itr);
            break;
        }
    }
    if (item && duration > 0 )
    {
        GetSession()->SendItemEnchantTimeUpdate(GetObjectGuid(), item->GetObjectGuid(), slot, uint32(duration/1000));
        m_enchantDuration.push_back(EnchantDuration(item, slot, duration));
    }
}

void Player::ApplyEnchantment(Item *item,bool apply)
{
    for(uint32 slot = 0; slot < MAX_ENCHANTMENT_SLOT; ++slot)
        ApplyEnchantment(item, EnchantmentSlot(slot), apply);
}

void Player::ApplyEnchantment(Item *item, EnchantmentSlot slot, bool apply, bool apply_dur, bool ignore_condition)
{
    if (!item)
        return;

    if (!item->IsEquipped())
        return;

    if (slot >= MAX_ENCHANTMENT_SLOT)
        return;

    uint32 enchant_id = item->GetEnchantmentId(slot);
    if (!enchant_id)
        return;

    SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
    if (!pEnchant)
        return;

    if (!ignore_condition && pEnchant->EnchantmentCondition && !((Player*)this)->EnchantmentFitsRequirements(pEnchant->EnchantmentCondition, -1))
        return;

    if (!item->IsBroken())
    {
        for (int s = 0; s < 3; ++s)
        {
            uint32 enchant_display_type = pEnchant->type[s];
            uint32 enchant_amount = pEnchant->amount[s];
            uint32 enchant_spell_id = pEnchant->spellid[s];

            switch(enchant_display_type)
            {
                case ITEM_ENCHANTMENT_TYPE_NONE:
                    break;
                case ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL:
                    // processed in Player::CastItemCombatSpell
                    break;
                case ITEM_ENCHANTMENT_TYPE_DAMAGE:
                    if (item->GetSlot() == EQUIPMENT_SLOT_MAINHAND)
                        HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(enchant_amount), apply);
                    else if (item->GetSlot() == EQUIPMENT_SLOT_OFFHAND)
                        HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(enchant_amount), apply);
                    else if (item->GetSlot() == EQUIPMENT_SLOT_RANGED)
                        HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(enchant_amount), apply);
                    break;
                case ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL:
                    if (enchant_spell_id)
                    {
                        if (apply)
                        {
                            int32 basepoints = 0;
                            // Random Property Exist - try found basepoints for spell (basepoints depends from item suffix factor)
                            if (item->GetItemRandomPropertyId())
                            {
                                ItemRandomSuffixEntry const *item_rand = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
                                if (item_rand)
                                {
                                    // Search enchant_amount
                                    for (int k = 0; k < 3; ++k)
                                    {
                                        if(item_rand->enchant_id[k] == enchant_id)
                                        {
                                            basepoints = int32((item_rand->prefix[k] * item->GetItemSuffixFactor()) / 10000 );
                                            break;
                                        }
                                    }
                                }
                            }
                            // Cast custom spell vs all equal basepoints getted from enchant_amount
                            if (basepoints)
                                CastCustomSpell(this, enchant_spell_id, &basepoints, &basepoints, &basepoints, true, item);
                            else
                                CastSpell(this, enchant_spell_id, true, item);
                        }
                        else
                            RemoveAurasDueToItemSpell(item, enchant_spell_id);
                    }
                    break;
                case ITEM_ENCHANTMENT_TYPE_RESISTANCE:
                    if (!enchant_amount)
                    {
                        ItemRandomSuffixEntry const *item_rand = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
                        if(item_rand)
                        {
                            for (int k = 0; k < 3; ++k)
                            {
                                if(item_rand->enchant_id[k] == enchant_id)
                                {
                                    enchant_amount = uint32((item_rand->prefix[k] * item->GetItemSuffixFactor()) / 10000 );
                                    break;
                                }
                            }
                        }
                    }

                    HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + enchant_spell_id), TOTAL_VALUE, float(enchant_amount), apply);
                    break;
                case ITEM_ENCHANTMENT_TYPE_STAT:
                {
                    if (!enchant_amount)
                    {
                        ItemRandomSuffixEntry const *item_rand_suffix = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
                        if(item_rand_suffix)
                        {
                            for (int k = 0; k < 3; ++k)
                            {
                                if(item_rand_suffix->enchant_id[k] == enchant_id)
                                {
                                    enchant_amount = uint32((item_rand_suffix->prefix[k] * item->GetItemSuffixFactor()) / 10000 );
                                    break;
                                }
                            }
                        }
                    }

                    DEBUG_LOG("Adding %u to stat nb %u",enchant_amount,enchant_spell_id);
                    switch (enchant_spell_id)
                    {
                        case ITEM_MOD_MANA:
                            DEBUG_LOG("+ %u MANA",enchant_amount);
                            HandleStatModifier(UNIT_MOD_MANA, BASE_VALUE, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_HEALTH:
                            DEBUG_LOG("+ %u HEALTH",enchant_amount);
                            HandleStatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_AGILITY:
                            DEBUG_LOG("+ %u AGILITY",enchant_amount);
                            HandleStatModifier(UNIT_MOD_STAT_AGILITY, TOTAL_VALUE, float(enchant_amount), apply);
                            ApplyStatBuffMod(STAT_AGILITY, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_STRENGTH:
                            DEBUG_LOG("+ %u STRENGTH",enchant_amount);
                            HandleStatModifier(UNIT_MOD_STAT_STRENGTH, TOTAL_VALUE, float(enchant_amount), apply);
                            ApplyStatBuffMod(STAT_STRENGTH, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_INTELLECT:
                            DEBUG_LOG("+ %u INTELLECT",enchant_amount);
                            HandleStatModifier(UNIT_MOD_STAT_INTELLECT, TOTAL_VALUE, float(enchant_amount), apply);
                            ApplyStatBuffMod(STAT_INTELLECT, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_SPIRIT:
                            DEBUG_LOG("+ %u SPIRIT",enchant_amount);
                            HandleStatModifier(UNIT_MOD_STAT_SPIRIT, TOTAL_VALUE, float(enchant_amount), apply);
                            ApplyStatBuffMod(STAT_SPIRIT, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_STAMINA:
                            DEBUG_LOG("+ %u STAMINA",enchant_amount);
                            HandleStatModifier(UNIT_MOD_STAT_STAMINA, TOTAL_VALUE, float(enchant_amount), apply);
                            ApplyStatBuffMod(STAT_STAMINA, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_DEFENSE_SKILL_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_DEFENSE_SKILL, enchant_amount, apply);
                            DEBUG_LOG("+ %u DEFENCE", enchant_amount);
                            break;
                        case  ITEM_MOD_DODGE_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_DODGE, enchant_amount, apply);
                            DEBUG_LOG("+ %u DODGE", enchant_amount);
                            break;
                        case ITEM_MOD_PARRY_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_PARRY, enchant_amount, apply);
                            DEBUG_LOG("+ %u PARRY", enchant_amount);
                            break;
                        case ITEM_MOD_BLOCK_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_BLOCK, enchant_amount, apply);
                            DEBUG_LOG("+ %u SHIELD_BLOCK", enchant_amount);
                            break;
                        case ITEM_MOD_HIT_MELEE_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_HIT_MELEE, enchant_amount, apply);
                            DEBUG_LOG("+ %u MELEE_HIT", enchant_amount);
                            break;
                        case ITEM_MOD_HIT_RANGED_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_HIT_RANGED, enchant_amount, apply);
                            DEBUG_LOG("+ %u RANGED_HIT", enchant_amount);
                            break;
                        case ITEM_MOD_HIT_SPELL_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_HIT_SPELL, enchant_amount, apply);
                            DEBUG_LOG("+ %u SPELL_HIT", enchant_amount);
                            break;
                        case ITEM_MOD_CRIT_MELEE_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_CRIT_MELEE, enchant_amount, apply);
                            DEBUG_LOG("+ %u MELEE_CRIT", enchant_amount);
                            break;
                        case ITEM_MOD_CRIT_RANGED_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_CRIT_RANGED, enchant_amount, apply);
                            DEBUG_LOG("+ %u RANGED_CRIT", enchant_amount);
                            break;
                        case ITEM_MOD_CRIT_SPELL_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_CRIT_SPELL, enchant_amount, apply);
                            DEBUG_LOG("+ %u SPELL_CRIT", enchant_amount);
                            break;
//                        Values from ITEM_STAT_MELEE_HA_RATING to ITEM_MOD_HASTE_RANGED_RATING are never used
//                        in Enchantments
//                        case ITEM_MOD_HIT_TAKEN_MELEE_RATING:
//                            ((Player*)this)->ApplyRatingMod(CR_HIT_TAKEN_MELEE, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_HIT_TAKEN_RANGED_RATING:
//                            ((Player*)this)->ApplyRatingMod(CR_HIT_TAKEN_RANGED, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_HIT_TAKEN_SPELL_RATING:
//                            ((Player*)this)->ApplyRatingMod(CR_HIT_TAKEN_SPELL, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_CRIT_TAKEN_MELEE_RATING:
//                            ((Player*)this)->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_CRIT_TAKEN_RANGED_RATING:
//                            ((Player*)this)->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_CRIT_TAKEN_SPELL_RATING:
//                            ((Player*)this)->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_HASTE_MELEE_RATING:
//                            ((Player*)this)->ApplyRatingMod(CR_HASTE_MELEE, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_HASTE_RANGED_RATING:
//                            ((Player*)this)->ApplyRatingMod(CR_HASTE_RANGED, enchant_amount, apply);
//                            break;
                        case ITEM_MOD_HASTE_SPELL_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_HASTE_SPELL, enchant_amount, apply);
                            break;
                        case ITEM_MOD_HIT_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_HIT_MELEE, enchant_amount, apply);
                            ((Player*)this)->ApplyRatingMod(CR_HIT_RANGED, enchant_amount, apply);
                            ((Player*)this)->ApplyRatingMod(CR_HIT_SPELL, enchant_amount, apply);
                            DEBUG_LOG("+ %u HIT", enchant_amount);
                            break;
                        case ITEM_MOD_CRIT_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_CRIT_MELEE, enchant_amount, apply);
                            ((Player*)this)->ApplyRatingMod(CR_CRIT_RANGED, enchant_amount, apply);
                            ((Player*)this)->ApplyRatingMod(CR_CRIT_SPELL, enchant_amount, apply);
                            DEBUG_LOG("+ %u CRITICAL", enchant_amount);
                            break;
//                        Values ITEM_MOD_HIT_TAKEN_RATING and ITEM_MOD_CRIT_TAKEN_RATING are never used in Enchantment
//                        case ITEM_MOD_HIT_TAKEN_RATING:
//                            ((Player*)this)->ApplyRatingMod(CR_HIT_TAKEN_MELEE, enchant_amount, apply);
//                            ((Player*)this)->ApplyRatingMod(CR_HIT_TAKEN_RANGED, enchant_amount, apply);
//                            ((Player*)this)->ApplyRatingMod(CR_HIT_TAKEN_SPELL, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_CRIT_TAKEN_RATING:
//                            ((Player*)this)->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, enchant_amount, apply);
//                            ((Player*)this)->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, enchant_amount, apply);
//                            ((Player*)this)->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, enchant_amount, apply);
//                            break;
                        case ITEM_MOD_RESILIENCE_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, enchant_amount, apply);
                            ((Player*)this)->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, enchant_amount, apply);
                            ((Player*)this)->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, enchant_amount, apply);
                            DEBUG_LOG("+ %u RESILIENCE", enchant_amount);
                            break;
                        case ITEM_MOD_HASTE_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_HASTE_MELEE, enchant_amount, apply);
                            ((Player*)this)->ApplyRatingMod(CR_HASTE_RANGED, enchant_amount, apply);
                            ((Player*)this)->ApplyRatingMod(CR_HASTE_SPELL, enchant_amount, apply);
                            DEBUG_LOG("+ %u HASTE", enchant_amount);
                            break;
                        case ITEM_MOD_EXPERTISE_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_EXPERTISE, enchant_amount, apply);
                            DEBUG_LOG("+ %u EXPERTISE", enchant_amount);
                            break;
                        case ITEM_MOD_ATTACK_POWER:
                            HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(enchant_amount), apply);
                            HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(enchant_amount), apply);
                            DEBUG_LOG("+ %u ATTACK_POWER", enchant_amount);
                            break;
                        case ITEM_MOD_RANGED_ATTACK_POWER:
                            HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(enchant_amount), apply);
                            DEBUG_LOG("+ %u RANGED_ATTACK_POWER", enchant_amount);
                            break;
                        case ITEM_MOD_MANA_REGENERATION:
                            ((Player*)this)->ApplyManaRegenBonus(enchant_amount, apply);
                            DEBUG_LOG("+ %u MANA_REGENERATION", enchant_amount);
                            break;
                        case ITEM_MOD_ARMOR_PENETRATION_RATING:
                            ((Player*)this)->ApplyRatingMod(CR_ARMOR_PENETRATION, enchant_amount, apply);
                            DEBUG_LOG("+ %u ARMOR PENETRATION", enchant_amount);
                            break;
                        case ITEM_MOD_SPELL_POWER:
                            ((Player*)this)->ApplySpellPowerBonus(enchant_amount, apply);
                            DEBUG_LOG("+ %u SPELL_POWER", enchant_amount);
                            break;
                        case ITEM_MOD_BLOCK_VALUE:
                            HandleBaseModValue(SHIELD_BLOCK_VALUE, FLAT_MOD, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_FERAL_ATTACK_POWER:
                        case ITEM_MOD_SPELL_HEALING_DONE:   // deprecated
                        case ITEM_MOD_SPELL_DAMAGE_DONE:    // deprecated
                        default:
                            break;
                    }
                    break;
                }
                case ITEM_ENCHANTMENT_TYPE_TOTEM:           // Shaman Rockbiter Weapon
                {
                    if(getClass() == CLASS_SHAMAN)
                    {
                        float addValue = 0.0f;
                        if(item->GetSlot() == EQUIPMENT_SLOT_MAINHAND)
                        {
                            addValue = float(enchant_amount * item->GetProto()->Delay / 1000.0f);
                            HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, addValue, apply);
                        }
                        else if(item->GetSlot() == EQUIPMENT_SLOT_OFFHAND )
                        {
                            addValue = float(enchant_amount * item->GetProto()->Delay / 1000.0f);
                            HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, addValue, apply);
                        }
                    }
                    break;
                }
                case ITEM_ENCHANTMENT_TYPE_USE_SPELL:
                    // processed in Player::CastItemUseSpell
                    break;
                case ITEM_ENCHANTMENT_TYPE_PRISMATIC_SOCKET:
                    // nothing do..
                    break;
                default:
                    sLog.outError("Unknown item enchantment (id = %d) display type: %d", enchant_id, enchant_display_type);
                    break;
            }                                               /*switch(enchant_display_type)*/
        }                                                   /*for*/
    }

    // visualize enchantment at player and equipped items
    if(slot == PERM_ENCHANTMENT_SLOT)
        SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0, apply ? item->GetEnchantmentId(slot) : 0);

    if(slot == TEMP_ENCHANTMENT_SLOT)
        SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 1, apply ? item->GetEnchantmentId(slot) : 0);


    if(apply_dur)
    {
        if(apply)
        {
            // set duration
            uint32 duration = item->GetEnchantmentDuration(slot);
            if(duration > 0)
                AddEnchantmentDuration(item, slot, duration);
        }
        else
        {
            // duration == 0 will remove EnchantDuration
            AddEnchantmentDuration(item, slot, 0);
        }
    }
}

void Player::SendEnchantmentDurations()
{
    for(EnchantDurationList::const_iterator itr = m_enchantDuration.begin(); itr != m_enchantDuration.end(); ++itr)
    {
        GetSession()->SendItemEnchantTimeUpdate(GetObjectGuid(), itr->item->GetObjectGuid(), itr->slot, uint32(itr->leftduration) / 1000);
    }
}

void Player::SendItemDurations()
{
    for(ItemDurationList::const_iterator itr = m_itemDuration.begin(); itr != m_itemDuration.end(); ++itr)
    {
        (*itr)->SendTimeUpdate(this);
    }
}

void Player::SendNewItem(Item *item, uint32 count, bool received, bool created, bool broadcast)
{
    if(!item)                                               // prevent crash
        return;

                                                            // last check 2.0.10
    WorldPacket data( SMSG_ITEM_PUSH_RESULT, (8+4+4+4+1+4+4+4+4+4) );
    data << GetObjectGuid();                                // player GUID
    data << uint32(received);                               // 0=looted, 1=from npc
    data << uint32(created);                                // 0=received, 1=created
    data << uint32(1);                                      // IsShowChatMessage
    data << uint8(item->GetBagSlot());                      // bagslot
                                                            // item slot, but when added to stack: 0xFFFFFFFF
    data << uint32((item->GetCount() == count) ? item->GetSlot() : -1);
    data << uint32(item->GetEntry());                       // item id
    data << uint32(item->GetItemSuffixFactor());            // SuffixFactor
    data << uint32(item->GetItemRandomPropertyId());        // random item property id
    data << uint32(count);                                  // count of items
    data << uint32(GetItemCount(item->GetEntry()));         // count of items in inventory

    if (broadcast && GetGroup())
        GetGroup()->BroadcastPacket(&data, true);
    else
        GetSession()->SendPacket(&data);
}

/*********************************************************/
/***                    GOSSIP SYSTEM                  ***/
/*********************************************************/

void Player::PrepareGossipMenu(WorldObject *pSource, uint32 menuId)
{
    PlayerMenu* pMenu = PlayerTalkClass;
    pMenu->ClearMenus();

    pMenu->GetGossipMenu().SetMenuId(menuId);

    GossipMenuItemsMapBounds pMenuItemBounds = sObjectMgr.GetGossipMenuItemsMapBounds(menuId);

    // prepares quest menu when true
    bool canSeeQuests = menuId == GetDefaultGossipMenuForSource(pSource);

    // if canSeeQuests (the default, top level menu) and no menu options exist for this, use options from default options
    if (pMenuItemBounds.first == pMenuItemBounds.second && canSeeQuests)
        pMenuItemBounds = sObjectMgr.GetGossipMenuItemsMapBounds(0);

    bool canTalkToCredit = pSource->GetTypeId() == TYPEID_UNIT;

    for(GossipMenuItemsMap::const_iterator itr = pMenuItemBounds.first; itr != pMenuItemBounds.second; ++itr)
    {
        bool hasMenuItem = true;

        if (!isGameMaster())                                // Let GM always see menu items regardless of conditions
        {
            if (itr->second.conditionId && !sObjectMgr.IsPlayerMeetToNEWCondition(this, itr->second.conditionId))
            {
                if (itr->second.option_id == GOSSIP_OPTION_QUESTGIVER)
                    canSeeQuests = false;
                continue;
            }
            else if (!itr->second.conditionId)
        {
            if (itr->second.cond_1 && !sObjectMgr.IsPlayerMeetToCondition(this, itr->second.cond_1))
            {
                if (itr->second.option_id == GOSSIP_OPTION_QUESTGIVER)
                    canSeeQuests = false;
                continue;
            }

            if (itr->second.cond_2 && !sObjectMgr.IsPlayerMeetToCondition(this, itr->second.cond_2))
            {
                if (itr->second.option_id == GOSSIP_OPTION_QUESTGIVER)
                    canSeeQuests = false;
                continue;
            }

            if (itr->second.cond_3 && !sObjectMgr.IsPlayerMeetToCondition(this, itr->second.cond_3))
            {
                if (itr->second.option_id == GOSSIP_OPTION_QUESTGIVER)
                    canSeeQuests = false;
                continue;
            }
        }
        }

        if (pSource->GetTypeId() == TYPEID_UNIT)
        {
            Creature *pCreature = (Creature*)pSource;

            uint32 npcflags = pCreature->GetUInt32Value(UNIT_NPC_FLAGS);

            if (!(itr->second.npc_option_npcflag & npcflags))
                continue;

            switch(itr->second.option_id)
            {
                case GOSSIP_OPTION_GOSSIP:
                    if (itr->second.action_menu_id != 0)    // has sub menu (or close gossip), so do not "talk" with this NPC yet
                        canTalkToCredit = false;
                    break;
                case GOSSIP_OPTION_QUESTGIVER:
                    hasMenuItem = false;
                    break;
                case GOSSIP_OPTION_ARMORER:
                    hasMenuItem = false;                    // added in special mode
                    break;
                case GOSSIP_OPTION_SPIRITHEALER:
                    if (!isDead())
                        hasMenuItem = false;
                    break;
                case GOSSIP_OPTION_VENDOR:
                {
                    VendorItemData const* vItems = pCreature->GetVendorItems();
                    VendorItemData const* tItems = pCreature->GetVendorTemplateItems();
                    if ((!vItems || vItems->Empty()) && (!tItems || tItems->Empty()))
                    {
                        sLog.outErrorDb("Creature %u (Entry: %u) have UNIT_NPC_FLAG_VENDOR but have empty trading item list.", pCreature->GetGUIDLow(), pCreature->GetEntry());
                        hasMenuItem = false;
                    }
                    break;
                }
                case GOSSIP_OPTION_TRAINER:
                    // pet trainers not have spells in fact now
                    /* FIXME: gossip menu with single unlearn pet talents option not show by some reason
                    if (pCreature->GetCreatureInfo()->trainer_type == TRAINER_TYPE_PETS)
                        hasMenuItem = false;
                    else */
                    if (!pCreature->IsTrainerOf(this, false))
                        hasMenuItem = false;
                    break;
                case GOSSIP_OPTION_UNLEARNTALENTS:
                    if (!pCreature->CanTrainAndResetTalentsOf(this))
                        hasMenuItem = false;
                    break;
                case GOSSIP_OPTION_UNLEARNPETSKILLS:
                    if (pCreature->GetCreatureInfo()->trainer_type != TRAINER_TYPE_PETS || pCreature->GetCreatureInfo()->trainer_class != CLASS_HUNTER)
                        hasMenuItem = false;
                    else if (Pet * pet = GetPet())
                    {
                        if (pet->getPetType() != HUNTER_PET || pet->m_spells.size() <= 1)
                            hasMenuItem = false;
                    }
                    else
                        hasMenuItem = false;
                    break;
                case GOSSIP_OPTION_TAXIVENDOR:
                    if (GetSession()->SendLearnNewTaxiNode(pCreature))
                        return;
                    break;
                case GOSSIP_OPTION_BATTLEFIELD:
                    if (!pCreature->CanInteractWithBattleMaster(this, false))
                        hasMenuItem = false;
                    break;
                case GOSSIP_OPTION_STABLEPET:
                    if (getClass() != CLASS_HUNTER)
                        hasMenuItem = false;
                    break;
                case GOSSIP_OPTION_SPIRITGUIDE:
                case GOSSIP_OPTION_INNKEEPER:
                case GOSSIP_OPTION_BANKER:
                case GOSSIP_OPTION_PETITIONER:
                case GOSSIP_OPTION_TABARDDESIGNER:
                case GOSSIP_OPTION_AUCTIONEER:
                case GOSSIP_OPTION_MAILBOX:
                    break;                                  // no checks
                default:
                    sLog.outErrorDb("Creature entry %u have unknown gossip option %u for menu %u", pCreature->GetEntry(), itr->second.option_id, itr->second.menu_id);
                    hasMenuItem = false;
                    break;
            }
        }
        else if (pSource->GetTypeId() == TYPEID_GAMEOBJECT)
        {
            GameObject *pGo = (GameObject*)pSource;

            switch(itr->second.option_id)
            {
                case GOSSIP_OPTION_QUESTGIVER:
                    hasMenuItem = false;
                    break;
                case GOSSIP_OPTION_GOSSIP:
                    if (pGo->GetGoType() != GAMEOBJECT_TYPE_QUESTGIVER && pGo->GetGoType() != GAMEOBJECT_TYPE_GOOBER)
                        hasMenuItem = false;
                    break;
                default:
                    hasMenuItem = false;
                    break;
            }
        }

        if (hasMenuItem)
        {
            std::string strOptionText = itr->second.option_text;
            std::string strBoxText = itr->second.box_text;

            int loc_idx = GetSession()->GetSessionDbLocaleIndex();

            if (loc_idx >= 0)
            {
                uint32 idxEntry = MAKE_PAIR32(menuId, itr->second.id);

                if (GossipMenuItemsLocale const *no = sObjectMgr.GetGossipMenuItemsLocale(idxEntry))
                {
                    if (no->OptionText.size() > (size_t)loc_idx && !no->OptionText[loc_idx].empty())
                        strOptionText = no->OptionText[loc_idx];

                    if (no->BoxText.size() > (size_t)loc_idx && !no->BoxText[loc_idx].empty())
                        strBoxText = no->BoxText[loc_idx];
                }
            }

            pMenu->GetGossipMenu().AddMenuItem(itr->second.option_icon, strOptionText, 0, itr->second.option_id, strBoxText, itr->second.box_money, itr->second.box_coded);
            pMenu->GetGossipMenu().AddGossipMenuItemData(itr->second.action_menu_id, itr->second.action_poi_id, itr->second.action_script_id);
        }
    }

    if (canSeeQuests)
        PrepareQuestMenu(pSource->GetObjectGuid());

    if (canTalkToCredit)
    {
        if (pSource->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP) && !(((Creature*)pSource)->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NO_TALKTO_CREDIT))
            TalkedToCreature(pSource->GetEntry(), pSource->GetObjectGuid());
    }

    // some gossips aren't handled in normal way ... so we need to do it this way .. TODO: handle it in normal way ;-)
    /*if (pMenu->Empty())
    {
        if (pCreature->HasFlag(UNIT_NPC_FLAGS,UNIT_NPC_FLAG_TRAINER))
        {
            // output error message if need
            pCreature->IsTrainerOf(this, true);
        }

        if (pCreature->HasFlag(UNIT_NPC_FLAGS,UNIT_NPC_FLAG_BATTLEMASTER))
        {
            // output error message if need
            pCreature->CanInteractWithBattleMaster(this, true);
        }
    }*/
}

void Player::SendPreparedGossip(WorldObject *pSource)
{
    if (!pSource)
        return;

    if (pSource->GetTypeId() == TYPEID_UNIT)
    {
        // in case no gossip flag and quest menu not empty, open quest menu (client expect gossip menu with this flag)
        if (!((Creature*)pSource)->HasFlag(UNIT_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP) && !PlayerTalkClass->GetQuestMenu().Empty())
        {
            SendPreparedQuest(pSource->GetObjectGuid());
            return;
        }
    }
    else if (pSource->GetTypeId() == TYPEID_GAMEOBJECT)
    {
        // probably need to find a better way here
        if (!PlayerTalkClass->GetGossipMenu().GetMenuId() && !PlayerTalkClass->GetQuestMenu().Empty())
        {
            SendPreparedQuest(pSource->GetObjectGuid());
            return;
        }
    }

    // in case non empty gossip menu (that not included quests list size) show it
    // (quest entries from quest menu will be included in list)

    uint32 textId = GetGossipTextId(pSource);

    if (uint32 menuId = PlayerTalkClass->GetGossipMenu().GetMenuId())
        textId = GetGossipTextId(menuId, pSource);

    PlayerTalkClass->SendGossipMenu(textId, pSource->GetObjectGuid());
}

void Player::OnGossipSelect(WorldObject* pSource, uint32 gossipListId, uint32 menuId)
{
    GossipMenu& gossipmenu = PlayerTalkClass->GetGossipMenu();

    if (gossipListId >= gossipmenu.MenuItemCount())
        return;

    // if not same, then something funky is going on
    if (menuId != gossipmenu.GetMenuId())
        return;

    GossipMenuItem const&  menu_item = gossipmenu.GetItem(gossipListId);

    uint32 gossipOptionId = menu_item.m_gOptionId;
    ObjectGuid guid = pSource->GetObjectGuid();
    uint32 moneyTake = menu_item.m_gBoxMoney;

    // if this function called and player have money for pay MoneyTake or cheating, proccess both cases
    if (moneyTake > 0)
    {
        if (GetMoney() >= moneyTake)
            ModifyMoney(-int32(moneyTake));
        else
            return;                                         // cheating
    }

    if (pSource->GetTypeId() == TYPEID_GAMEOBJECT)
    {
        if (gossipOptionId > GOSSIP_OPTION_QUESTGIVER)
        {
            sLog.outError("Player guid %u request invalid gossip option for GameObject entry %u", GetGUIDLow(), pSource->GetEntry());
            return;
        }
    }

    GossipMenuItemData pMenuData = gossipmenu.GetItemData(gossipListId);

    switch (gossipOptionId)
    {
        case GOSSIP_OPTION_GOSSIP:
        {
            if (pMenuData.m_gAction_poi)
                PlayerTalkClass->SendPointOfInterest(pMenuData.m_gAction_poi);

            // send new menu || close gossip || stay at current menu
            if (pMenuData.m_gAction_menu > 0)
            {
                PrepareGossipMenu(pSource, uint32(pMenuData.m_gAction_menu));
                SendPreparedGossip(pSource);
            }
            else if (pMenuData.m_gAction_menu < 0)
            {
                PlayerTalkClass->CloseGossip();
                TalkedToCreature(pSource->GetEntry(), pSource->GetObjectGuid());
            }
            break;
        }
        case GOSSIP_OPTION_SPIRITHEALER:
            if (isDead())
                ((Creature*)pSource)->CastSpell(((Creature*)pSource), 17251, true, NULL, NULL, GetObjectGuid());
            break;
        case GOSSIP_OPTION_QUESTGIVER:
            PrepareQuestMenu(guid);
            SendPreparedQuest(guid);
            break;
        case GOSSIP_OPTION_VENDOR:
        case GOSSIP_OPTION_ARMORER:
            GetSession()->SendListInventory(guid);
            break;
        case GOSSIP_OPTION_STABLEPET:
            GetSession()->SendStablePet(guid);
            break;
        case GOSSIP_OPTION_TRAINER:
            GetSession()->SendTrainerList(guid);
            break;
        case GOSSIP_OPTION_UNLEARNTALENTS:
            PlayerTalkClass->CloseGossip();
            SendTalentWipeConfirm(guid);
            break;
        case GOSSIP_OPTION_UNLEARNPETSKILLS:
            PlayerTalkClass->CloseGossip();
            SendPetSkillWipeConfirm();
            break;
        case GOSSIP_OPTION_TAXIVENDOR:
            GetSession()->SendTaxiMenu(((Creature*)pSource));
            break;
        case GOSSIP_OPTION_INNKEEPER:
            PlayerTalkClass->CloseGossip();
            SetBindPoint(guid);
            break;
        case GOSSIP_OPTION_BANKER:
            GetSession()->SendShowBank(guid);
            break;
        case GOSSIP_OPTION_PETITIONER:
            PlayerTalkClass->CloseGossip();
            GetSession()->SendPetitionShowList(guid);
            break;
        case GOSSIP_OPTION_TABARDDESIGNER:
            PlayerTalkClass->CloseGossip();
            GetSession()->SendTabardVendorActivate(guid);
            break;
        case GOSSIP_OPTION_AUCTIONEER:
            GetSession()->SendAuctionHello(((Creature*)pSource));
            break;
        case GOSSIP_OPTION_MAILBOX:
            PlayerTalkClass->CloseGossip();
            GetSession()->SendShowMailBox(guid);
            break;
        case GOSSIP_OPTION_SPIRITGUIDE:
            PrepareGossipMenu(pSource);
            SendPreparedGossip(pSource);
            break;
        case GOSSIP_OPTION_BATTLEFIELD:
        {
            BattleGroundTypeId bgTypeId = sBattleGroundMgr.GetBattleMasterBG(pSource->GetEntry());

            if (bgTypeId == BATTLEGROUND_TYPE_NONE)
            {
                sLog.outError("a user (guid %u) requested battlegroundlist from a npc who is no battlemaster", GetGUIDLow());
                return;
            }

            GetSession()->SendBattlegGroundList(guid, bgTypeId);
            break;
        }
    }

    if (pMenuData.m_gAction_script)
    {
        if (pSource->GetTypeId() == TYPEID_UNIT)
            GetMap()->ScriptsStart(sGossipScripts, pMenuData.m_gAction_script, pSource, this);
        else if (pSource->GetTypeId() == TYPEID_GAMEOBJECT)
            GetMap()->ScriptsStart(sGossipScripts, pMenuData.m_gAction_script, this, pSource);
    }
}

uint32 Player::GetGossipTextId(WorldObject *pSource)
{
    if (!pSource || pSource->GetTypeId() != TYPEID_UNIT)
        return DEFAULT_GOSSIP_MESSAGE;

    if (uint32 pos = sObjectMgr.GetNpcGossip(((Creature*)pSource)->GetGUIDLow()))
        return pos;

    return DEFAULT_GOSSIP_MESSAGE;
}

uint32 Player::GetGossipTextId(uint32 menuId, WorldObject* pSource)
{
    uint32 textId = DEFAULT_GOSSIP_MESSAGE;

    if (!menuId)
        return textId;

    GossipMenusMapBounds pMenuBounds = sObjectMgr.GetGossipMenusMapBounds(menuId);

    for (GossipMenusMap::const_iterator itr = pMenuBounds.first; itr != pMenuBounds.second; ++itr)
    {
        if (itr->second.conditionId && sObjectMgr.IsPlayerMeetToNEWCondition(this, itr->second.conditionId))
        {
            textId = itr->second.text_id;

            // Start related script
            if (itr->second.script_id)
                GetMap()->ScriptsStart(sGossipScripts, itr->second.script_id, this, pSource);
            break;
        }
        else if (!itr->second.conditionId)
    {
        if (sObjectMgr.IsPlayerMeetToCondition(this, itr->second.cond_1) && sObjectMgr.IsPlayerMeetToCondition(this, itr->second.cond_2))
        {
            textId = itr->second.text_id;

            // Start related script
            if (itr->second.script_id)
                GetMap()->ScriptsStart(sGossipScripts, itr->second.script_id, this, pSource);
            break;
        }
    }
    }

    return textId;
}

uint32 Player::GetDefaultGossipMenuForSource(WorldObject *pSource)
{
    if (pSource->GetTypeId() == TYPEID_UNIT)
        return ((Creature*)pSource)->GetCreatureInfo()->GossipMenuId;
    else if (pSource->GetTypeId() == TYPEID_GAMEOBJECT)
        return((GameObject*)pSource)->GetGOInfo()->GetGossipMenuId();

    return 0;
}

/*********************************************************/
/***                    QUEST SYSTEM                   ***/
/*********************************************************/

void Player::PrepareQuestMenu(ObjectGuid guid)
{
    QuestRelationsMapBounds rbounds;
    QuestRelationsMapBounds irbounds;

    // pets also can have quests
    if (Creature *pCreature = GetMap()->GetAnyTypeCreature(guid))
    {
        rbounds = sObjectMgr.GetCreatureQuestRelationsMapBounds(pCreature->GetEntry());
        irbounds = sObjectMgr.GetCreatureQuestInvolvedRelationsMapBounds(pCreature->GetEntry());
    }
    else
    {
        //we should obtain map pointer from GetMap() in 99% of cases. Special case
        //only for quests which cast teleport spells on player
        Map * _map = IsInWorld() ? GetMap() : sMapMgr.FindMap(GetMapId(), GetInstanceId());
        MANGOS_ASSERT(_map);

        if (GameObject *pGameObject = _map->GetGameObject(guid))
        {
            rbounds = sObjectMgr.GetGOQuestRelationsMapBounds(pGameObject->GetEntry());
            irbounds = sObjectMgr.GetGOQuestInvolvedRelationsMapBounds(pGameObject->GetEntry());
        }
        else
            return;
    }

    QuestMenu &qm = PlayerTalkClass->GetQuestMenu();
    qm.ClearMenu();

    for(QuestRelationsMap::const_iterator itr = irbounds.first; itr != irbounds.second; ++itr)
    {
        uint32 quest_id = itr->second;

        Quest const* pQuest = sObjectMgr.GetQuestTemplate(quest_id);

        if (!pQuest || !pQuest->IsActive())
            continue;

        QuestStatus status = GetQuestStatus(quest_id);

        if (status == QUEST_STATUS_COMPLETE && !GetQuestRewardStatus(quest_id))
            qm.AddMenuItem(quest_id, 4);
        else if (status == QUEST_STATUS_INCOMPLETE)
            qm.AddMenuItem(quest_id, 4);
        else if (status == QUEST_STATUS_AVAILABLE)
            qm.AddMenuItem(quest_id, 2);
    }

    for(QuestRelationsMap::const_iterator itr = rbounds.first; itr != rbounds.second; ++itr)
    {
        uint32 quest_id = itr->second;

        Quest const* pQuest = sObjectMgr.GetQuestTemplate(quest_id);

        if (!pQuest || !pQuest->IsActive())
            continue;

        QuestStatus status = GetQuestStatus(quest_id);

        if (pQuest->IsAutoComplete() && CanTakeQuest(pQuest, false))
            qm.AddMenuItem(quest_id, 4);
        else if (status == QUEST_STATUS_NONE && CanTakeQuest(pQuest, false))
            qm.AddMenuItem(quest_id, 2);
    }
}

void Player::SendPreparedQuest(ObjectGuid guid)
{
    QuestMenu& questMenu = PlayerTalkClass->GetQuestMenu();

    if (questMenu.Empty())
        return;

    QuestMenuItem const& qmi0 = questMenu.GetItem(0);

    uint32 icon = qmi0.m_qIcon;

    // single element case
    if (questMenu.MenuItemCount() == 1)
    {
        // Auto open -- maybe also should verify there is no greeting
        uint32 quest_id = qmi0.m_qId;
        Quest const* pQuest = sObjectMgr.GetQuestTemplate(quest_id);

        if (pQuest)
        {
            if (icon == 4 && !GetQuestRewardStatus(quest_id))
                PlayerTalkClass->SendQuestGiverRequestItems(pQuest, guid, CanRewardQuest(pQuest, false), true);
            else if (icon == 4)
                PlayerTalkClass->SendQuestGiverRequestItems(pQuest, guid, CanRewardQuest(pQuest, false), true);
            // Send completable on repeatable and autoCompletable quest if player don't have quest
            // TODO: verify if check for !pQuest->IsDaily() is really correct (possibly not)
            else if (pQuest->IsAutoComplete() && pQuest->IsRepeatable() && !pQuest->IsDailyOrWeekly())
                PlayerTalkClass->SendQuestGiverRequestItems(pQuest, guid, CanCompleteRepeatableQuest(pQuest), true);
            else
                PlayerTalkClass->SendQuestGiverQuestDetails(pQuest, guid, true);
        }
    }
    // multiply entries
    else
    {
        QEmote qe;
        qe._Delay = 0;
        qe._Emote = 0;
        std::string title = "";

        // need pet case for some quests
        if (Creature *pCreature = GetMap()->GetAnyTypeCreature(guid))
        {
            uint32 textid = GetGossipTextId(pCreature);

            GossipText const* gossiptext = sObjectMgr.GetGossipText(textid);
            if (!gossiptext)
            {
                qe._Delay = 0;                              //TEXTEMOTE_MESSAGE;              //zyg: player emote
                qe._Emote = 0;                              //TEXTEMOTE_HELLO;                //zyg: NPC emote
                title = "";
            }
            else
            {
                qe = gossiptext->Options[0].Emotes[0];

                int loc_idx = GetSession()->GetSessionDbLocaleIndex();

                std::string title0 = gossiptext->Options[0].Text_0;
                std::string title1 = gossiptext->Options[0].Text_1;
                sObjectMgr.GetNpcTextLocaleStrings0(textid, loc_idx, &title0, &title1);

                title = !title0.empty() ? title0 : title1;
            }
        }
        PlayerTalkClass->SendQuestGiverQuestList(qe, title, guid);
    }
}

bool Player::IsActiveQuest( uint32 quest_id ) const
{
    QuestStatusMap::const_iterator itr = mQuestStatus.find(quest_id);

    return itr != mQuestStatus.end() && itr->second.m_status != QUEST_STATUS_NONE;
}

bool Player::IsCurrentQuest(uint32 quest_id, uint8 completed_or_not) const
{
    QuestStatusMap::const_iterator itr = mQuestStatus.find(quest_id);
    if (itr == mQuestStatus.end())
        return false;

    switch (completed_or_not)
    {
        case 1:
            return itr->second.m_status == QUEST_STATUS_INCOMPLETE;
        case 2:
            return itr->second.m_status == QUEST_STATUS_COMPLETE && !itr->second.m_rewarded;
        default:
            return itr->second.m_status == QUEST_STATUS_INCOMPLETE || (itr->second.m_status == QUEST_STATUS_COMPLETE && !itr->second.m_rewarded);
    }
}

Quest const* Player::GetNextQuest(ObjectGuid guid, Quest const *pQuest)
{
    QuestRelationsMapBounds rbounds;

    if (Creature *pCreature = GetMap()->GetAnyTypeCreature(guid))
    {
        rbounds = sObjectMgr.GetCreatureQuestRelationsMapBounds(pCreature->GetEntry());
    }
    else
    {
        //we should obtain map pointer from GetMap() in 99% of cases. Special case
        //only for quests which cast teleport spells on player
        Map * _map = IsInWorld() ? GetMap() : sMapMgr.FindMap(GetMapId(), GetInstanceId());
        MANGOS_ASSERT(_map);

        if (GameObject *pGameObject = _map->GetGameObject(guid))
        {
            rbounds = sObjectMgr.GetGOQuestRelationsMapBounds(pGameObject->GetEntry());
        }
        else
            return NULL;
    }

    uint32 nextQuestID = pQuest->GetNextQuestInChain();
    for(QuestRelationsMap::const_iterator itr = rbounds.first; itr != rbounds.second; ++itr)
    {
        if (itr->second == nextQuestID)
            return sObjectMgr.GetQuestTemplate(nextQuestID);
    }

    return NULL;
}

bool Player::CanSeeStartQuest(Quest const *pQuest) const
{
    if (SatisfyQuestClass(pQuest, false) && SatisfyQuestRace(pQuest, false) && SatisfyQuestSkill(pQuest, false) &&
        SatisfyQuestExclusiveGroup(pQuest, false) && SatisfyQuestReputation(pQuest, false) &&
        SatisfyQuestPreviousQuest(pQuest, false) && SatisfyQuestNextChain(pQuest, false) &&
        SatisfyQuestPrevChain(pQuest, false) && SatisfyQuestDay(pQuest, false) && SatisfyQuestWeek(pQuest, false) &&
        SatisfyQuestMonth(pQuest, false) &&
        pQuest->IsActive())
    {
        return int32(getLevel()) + sWorld.getConfig(CONFIG_INT32_QUEST_HIGH_LEVEL_HIDE_DIFF) >= int32(pQuest->GetMinLevel());
    }

    return false;
}

bool Player::CanTakeQuest(Quest const *pQuest, bool msg) const
{
    return SatisfyQuestStatus(pQuest, msg) && SatisfyQuestExclusiveGroup(pQuest, msg) &&
        SatisfyQuestClass(pQuest, msg) && SatisfyQuestRace(pQuest, msg) && SatisfyQuestLevel(pQuest, msg) &&
        SatisfyQuestSkill(pQuest, msg) && SatisfyQuestReputation(pQuest, msg) &&
        SatisfyQuestPreviousQuest(pQuest, msg) && SatisfyQuestTimed(pQuest, msg) &&
        SatisfyQuestNextChain(pQuest, msg) && SatisfyQuestPrevChain(pQuest, msg) &&
        SatisfyQuestDay(pQuest, msg) && SatisfyQuestWeek(pQuest, msg) && SatisfyQuestMonth(pQuest, msg) &&
        pQuest->IsActive();
}

bool Player::CanAddQuest(Quest const *pQuest, bool msg) const
{
    if (!SatisfyQuestLog(msg))
        return false;

    if (!CanGiveQuestSourceItemIfNeed(pQuest))
        return false;

    return true;
}

bool Player::CanCompleteQuest(uint32 quest_id) const
{
    if (!quest_id)
        return false;

    QuestStatusMap::const_iterator q_itr = mQuestStatus.find(quest_id);

    // some quests can be auto taken and auto completed in one step
    QuestStatus status = q_itr != mQuestStatus.end() ? q_itr->second.m_status : QUEST_STATUS_NONE;

    if (status == QUEST_STATUS_COMPLETE)
        return false;                                       // not allow re-complete quest

    Quest const* qInfo = sObjectMgr.GetQuestTemplate(quest_id);

    if (!qInfo)
        return false;

    // only used for "flag" quests and not real in-game quests
    if (qInfo->HasQuestFlag(QUEST_FLAGS_AUTO_REWARDED))
    {
        // a few checks, not all "satisfy" is needed
        if (SatisfyQuestPreviousQuest(qInfo, false) && SatisfyQuestLevel(qInfo, false) &&
            SatisfyQuestSkill(qInfo, false) && SatisfyQuestRace(qInfo, false) && SatisfyQuestClass(qInfo, false))
            return true;

        return false;
    }

    // auto complete quest
    if (qInfo->IsAutoComplete() && CanTakeQuest(qInfo, false))
        return true;

    if (status != QUEST_STATUS_INCOMPLETE)
        return false;

    // incomplete quest have status data
    QuestStatusData const& q_status = q_itr->second;

    if (qInfo->HasSpecialFlag(QUEST_SPECIAL_FLAG_DELIVER))
    {
        for(int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
        {
            if (qInfo->ReqItemCount[i] != 0 && q_status.m_itemcount[i] < qInfo->ReqItemCount[i])
                return false;
        }
    }

    if (qInfo->HasSpecialFlag(QuestSpecialFlags(QUEST_SPECIAL_FLAG_KILL_OR_CAST | QUEST_SPECIAL_FLAG_SPEAKTO)))
    {
        for(int i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        {
            if (qInfo->ReqCreatureOrGOId[i] == 0)
                continue;

            if (qInfo->ReqCreatureOrGOCount[i] != 0 && q_status.m_creatureOrGOcount[i] < qInfo->ReqCreatureOrGOCount[i])
                return false;
        }
    }

    if (qInfo->HasSpecialFlag(QUEST_SPECIAL_FLAG_EXPLORATION_OR_EVENT) && !q_status.m_explored)
        return false;

    if (qInfo->HasSpecialFlag(QUEST_SPECIAL_FLAG_TIMED) && q_status.m_timer == 0)
        return false;

    if (qInfo->GetRewOrReqMoney() < 0)
    {
        if (GetMoney() < uint32(-qInfo->GetRewOrReqMoney()))
            return false;
    }

    uint32 repFacId = qInfo->GetRepObjectiveFaction();
    if (repFacId && GetReputationMgr().GetReputation(repFacId) < qInfo->GetRepObjectiveValue())
        return false;

    return true;
}

bool Player::CanCompleteRepeatableQuest(Quest const *pQuest) const
{
    // Solve problem that player don't have the quest and try complete it.
    // if repeatable she must be able to complete event if player don't have it.
    // Seem that all repeatable quest are DELIVER Flag so, no need to add more.
    if (!CanTakeQuest(pQuest, false))
        return false;

    if (pQuest->HasSpecialFlag(QUEST_SPECIAL_FLAG_DELIVER))
        for(int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
            if (pQuest->ReqItemId[i] && pQuest->ReqItemCount[i] && !HasItemCount(pQuest->ReqItemId[i], pQuest->ReqItemCount[i]))
                return false;

    if (!CanRewardQuest(pQuest, false))
        return false;

    return true;
}

bool Player::CanRewardQuest(Quest const *pQuest, bool msg) const
{
    // not auto complete quest and not completed quest (only cheating case, then ignore without message)
    if (!pQuest->IsAutoComplete() && GetQuestStatus(pQuest->GetQuestId()) != QUEST_STATUS_COMPLETE)
        return false;

    // daily quest can't be rewarded (25 daily quest already completed)
    if (!SatisfyQuestDay(pQuest, true) || !SatisfyQuestWeek(pQuest, true) || !SatisfyQuestMonth(pQuest, true))
        return false;

    // rewarded and not repeatable quest (only cheating case, then ignore without message)
    if (GetQuestRewardStatus(pQuest->GetQuestId()))
        return false;

    // prevent receive reward with quest items in bank
    if (pQuest->HasSpecialFlag(QUEST_SPECIAL_FLAG_DELIVER))
    {
        for(int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
        {
            if (pQuest->ReqItemCount[i] != 0 &&
                GetItemCount(pQuest->ReqItemId[i]) < pQuest->ReqItemCount[i])
            {
                if (msg)
                    SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL, pQuest->ReqItemId[i]);

                return false;
            }
        }
    }

    // prevent receive reward with low money and GetRewOrReqMoney() < 0
    if (pQuest->GetRewOrReqMoney() < 0 && GetMoney() < uint32(-pQuest->GetRewOrReqMoney()))
        return false;

    return true;
}

bool Player::CanRewardQuest(Quest const *pQuest, uint32 reward, bool msg) const
{
    // prevent receive reward with quest items in bank or for not completed quest
    if (!CanRewardQuest(pQuest,msg))
        return false;

    if (pQuest->GetRewChoiceItemsCount() > 0)
    {
        if (pQuest->RewChoiceItemId[reward])
        {
            ItemPosCountVec dest;
            InventoryResult res = CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, pQuest->RewChoiceItemId[reward], pQuest->RewChoiceItemCount[reward] );
            if (res != EQUIP_ERR_OK)
            {
                SendEquipError(res, NULL, NULL, pQuest->RewChoiceItemId[reward]);
                return false;
            }
        }
    }

    if (pQuest->GetRewItemsCount() > 0)
    {
        for (uint32 i = 0; i < pQuest->GetRewItemsCount(); ++i)
        {
            if (pQuest->RewItemId[i])
            {
                ItemPosCountVec dest;
                InventoryResult res = CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, pQuest->RewItemId[i], pQuest->RewItemCount[i] );
                if (res != EQUIP_ERR_OK)
                {
                    SendEquipError(res, NULL, NULL);
                    return false;
                }
            }
        }
    }

    return true;
}

void Player::SendPetTameFailure(PetTameFailureReason reason)
{
    WorldPacket data(SMSG_PET_TAME_FAILURE, 1);
    data << uint8(reason);
    GetSession()->SendPacket(&data);
}

void Player::AddQuest( Quest const *pQuest, Object *questGiver )
{
    uint16 log_slot = FindQuestSlot( 0 );
    MANGOS_ASSERT(log_slot < MAX_QUEST_LOG_SIZE);

    uint32 quest_id = pQuest->GetQuestId();

    // if not exist then created with set uState==NEW and rewarded=false
    QuestStatusData& questStatusData = mQuestStatus[quest_id];

    // check for repeatable quests status reset
    questStatusData.m_status = QUEST_STATUS_INCOMPLETE;
    questStatusData.m_explored = false;

    if (pQuest->HasSpecialFlag(QUEST_SPECIAL_FLAG_DELIVER))
    {
        for(int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
            questStatusData.m_itemcount[i] = 0;
    }

    if (pQuest->HasSpecialFlag(QuestSpecialFlags(QUEST_SPECIAL_FLAG_KILL_OR_CAST | QUEST_SPECIAL_FLAG_SPEAKTO)))
    {
        for(int i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
            questStatusData.m_creatureOrGOcount[i] = 0;
    }

    if( pQuest->GetRepObjectiveFaction() )
        if(FactionEntry const* factionEntry = sFactionStore.LookupEntry(pQuest->GetRepObjectiveFaction()))
            GetReputationMgr().SetVisible(factionEntry);

    uint32 qtime = 0;
    if (pQuest->HasSpecialFlag(QUEST_SPECIAL_FLAG_TIMED))
    {
        uint32 limittime = pQuest->GetLimitTime();

        // shared timed quest
        if(questGiver && questGiver->GetTypeId()==TYPEID_PLAYER)
            limittime = ((Player*)questGiver)->getQuestStatusMap()[quest_id].m_timer / IN_MILLISECONDS;

        AddTimedQuest( quest_id );
        questStatusData.m_timer = limittime * IN_MILLISECONDS;
        qtime = static_cast<uint32>(time(NULL)) + limittime;
    }
    else
        questStatusData.m_timer = 0;

    SetQuestSlot(log_slot, quest_id, qtime);

    if (questStatusData.uState != QUEST_NEW)
        questStatusData.uState = QUEST_CHANGED;

    // quest accept scripts
    if (questGiver)
    {
        switch (questGiver->GetTypeId())
        {
            case TYPEID_UNIT:
                sScriptMgr.OnQuestAccept(this, (Creature*)questGiver, pQuest);
                break;
            case TYPEID_ITEM:
            case TYPEID_CONTAINER:
                sScriptMgr.OnQuestAccept(this, (Item*)questGiver, pQuest);
                break;
            case TYPEID_GAMEOBJECT:
                sScriptMgr.OnQuestAccept(this, (GameObject*)questGiver, pQuest);
                break;
        }

        // starting initial DB quest script
        if (pQuest->GetQuestStartScript() != 0)
            GetMap()->ScriptsStart(sQuestStartScripts, pQuest->GetQuestStartScript(), questGiver, this);

    }

    // remove start item if not need
    if (questGiver && questGiver->isType(TYPEMASK_ITEM))
    {
        // destroy not required for quest finish quest starting item
        bool notRequiredItem = true;
        for(int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
        {
            if (pQuest->ReqItemId[i] == questGiver->GetEntry())
            {
                notRequiredItem = false;
                break;
            }
        }

        if (pQuest->GetSrcItemId() == questGiver->GetEntry())
            notRequiredItem = false;

        if (notRequiredItem)
            DestroyItem(((Item*)questGiver)->GetBagSlot(), ((Item*)questGiver)->GetSlot(), true);
    }

    GiveQuestSourceItemIfNeed(pQuest);

    AdjustQuestReqItemCount( pQuest, questStatusData );

    // Some spells applied at quest activation
    SpellAreaForQuestMapBounds saBounds = sSpellMgr.GetSpellAreaForQuestMapBounds(quest_id,true);
    if(saBounds.first != saBounds.second)
    {
        uint32 zone, area;
        GetZoneAndAreaId(zone,area);

        for(SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
            if(itr->second->autocast && itr->second->IsFitToRequirements(this,zone,area))
                if (!HasAura(itr->second->spellId, EFFECT_INDEX_0) )
                    CastSpell(this,itr->second->spellId,true);
    }

    UpdateForQuestWorldObjects();
}

void Player::CompleteQuest(uint32 quest_id)
{
    if (quest_id)
    {
        SetQuestStatus(quest_id, QUEST_STATUS_COMPLETE);

        uint16 log_slot = FindQuestSlot(quest_id);
        if (log_slot < MAX_QUEST_LOG_SIZE)
            SetQuestSlotState(log_slot, QUEST_STATE_COMPLETE);

        if (Quest const* qInfo = sObjectMgr.GetQuestTemplate(quest_id))
        {
            if (qInfo->HasQuestFlag(QUEST_FLAGS_AUTO_REWARDED))
                RewardQuest(qInfo, 0, this, false);
        }
    }
}

void Player::IncompleteQuest(uint32 quest_id)
{
    if (quest_id)
    {
        SetQuestStatus(quest_id, QUEST_STATUS_INCOMPLETE);

        uint16 log_slot = FindQuestSlot( quest_id );
        if (log_slot < MAX_QUEST_LOG_SIZE)
            RemoveQuestSlotState(log_slot, QUEST_STATE_COMPLETE);
    }
}

void Player::RewardQuest(Quest const *pQuest, uint32 reward, Object* questGiver, bool announce)
{
    uint32 quest_id = pQuest->GetQuestId();

    for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i )
    {
        if (pQuest->ReqItemId[i])
            DestroyItemCount(pQuest->ReqItemId[i], pQuest->ReqItemCount[i], true);
    }

    RemoveTimedQuest(quest_id);

    if (BattleGround* bg = GetBattleGround())
        if (bg->GetTypeID() == BATTLEGROUND_AV)
            ((BattleGroundAV*)bg)->HandleQuestComplete(pQuest->GetQuestId(), this);

    if (pQuest->GetRewChoiceItemsCount() > 0)
    {
        if (uint32 itemId = pQuest->RewChoiceItemId[reward])
        {
            ItemPosCountVec dest;
            if (CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, pQuest->RewChoiceItemCount[reward]) == EQUIP_ERR_OK)
            {
                Item* item = StoreNewItem( dest, itemId, true, Item::GenerateItemRandomPropertyId(itemId));
                SendNewItem(item, pQuest->RewChoiceItemCount[reward], true, false);
            }
        }
    }

    if (pQuest->GetRewItemsCount() > 0)
    {
        for (uint32 i=0; i < pQuest->GetRewItemsCount(); ++i)
        {
            if (uint32 itemId = pQuest->RewItemId[i])
            {
                ItemPosCountVec dest;
                if (CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, pQuest->RewItemCount[i]) == EQUIP_ERR_OK)
                {
                    Item* item = StoreNewItem( dest, itemId, true, Item::GenerateItemRandomPropertyId(itemId));
                    SendNewItem(item, pQuest->RewItemCount[i], true, false);
                }
            }
        }
    }

    RewardReputation(pQuest);

    uint16 log_slot = FindQuestSlot(quest_id);
    if (log_slot < MAX_QUEST_LOG_SIZE)
        SetQuestSlot(log_slot,0);

    QuestStatusData& q_status = mQuestStatus[quest_id];

    // Used for client inform but rewarded only in case not max level
    uint32 xp = uint32(pQuest->XPValue(this) * sWorld.getConfig(CONFIG_FLOAT_RATE_XP_QUEST));

    if (getLevel() < sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        GiveXP(xp , NULL);

        // Give player extra money (for max level already included in pQuest->GetRewMoneyMaxLevel())
        if (pQuest->GetRewOrReqMoney() > 0)
        {
            ModifyMoney(pQuest->GetRewOrReqMoney());
            GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD, pQuest->GetRewOrReqMoney());
        }
    }
    else
    {
        // reward money for max level already included in pQuest->GetRewMoneyMaxLevel()
        uint32 money = uint32(pQuest->GetRewMoneyMaxLevel() * sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_MONEY));

        // reward money used if > xp replacement money
        if (pQuest->GetRewOrReqMoney() > int32(money))
            money = pQuest->GetRewOrReqMoney();

        ModifyMoney(money);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD, money);
    }

    // req money case
    if (pQuest->GetRewOrReqMoney() < 0)
        ModifyMoney(pQuest->GetRewOrReqMoney());

    // honor reward
    if (uint32 honor = pQuest->CalculateRewardHonor(getLevel()))
        RewardHonor(NULL, 0, honor);

    // title reward
    if (pQuest->GetCharTitleId())
    {
        if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(pQuest->GetCharTitleId()))
            SetTitle(titleEntry);
    }

    if (pQuest->GetBonusTalents())
    {
        m_questRewardTalentCount += pQuest->GetBonusTalents();
        InitTalentForLevel();
    }

    // Send reward mail
    if (uint32 mail_template_id = pQuest->GetRewMailTemplateId())
        MailDraft(mail_template_id).SendMailTo(this, questGiver, MAIL_CHECK_MASK_HAS_BODY, pQuest->GetRewMailDelaySecs());

    if (pQuest->IsDaily())
    {
        SetDailyQuestStatus(quest_id);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST, 1);
    }

    if (pQuest->IsWeekly())
        SetWeeklyQuestStatus(quest_id);

    if (pQuest->IsMonthly())
        SetMonthlyQuestStatus(quest_id);

    if (!pQuest->IsRepeatable())
        SetQuestStatus(quest_id, QUEST_STATUS_COMPLETE);
    else
        SetQuestStatus(quest_id, QUEST_STATUS_NONE);

    q_status.m_rewarded = true;
    if (q_status.uState != QUEST_NEW)
        q_status.uState = QUEST_CHANGED;

    if (announce)
        SendQuestReward(pQuest, xp, questGiver);

    bool handled = false;

    switch(questGiver->GetTypeId())
    {
        case TYPEID_UNIT:
            handled = sScriptMgr.OnQuestRewarded(this, (Creature*)questGiver, pQuest);
            break;
        case TYPEID_GAMEOBJECT:
            handled = sScriptMgr.OnQuestRewarded(this, (GameObject*)questGiver, pQuest);
            break;
    }

    if (!handled && pQuest->GetQuestCompleteScript() != 0)
        GetMap()->ScriptsStart(sQuestEndScripts, pQuest->GetQuestCompleteScript(), questGiver, this);

    // cast spells after mark quest complete (some spells have quest completed state reqyurements in spell_area data)
    if (pQuest->GetRewSpellCast() > 0)
        CastSpell(this, pQuest->GetRewSpellCast(), true);
    else if (pQuest->GetRewSpell() > 0)
        CastSpell(this, pQuest->GetRewSpell(), true);

    if (pQuest->GetZoneOrSort() > 0)
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE, pQuest->GetZoneOrSort());

    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT);
    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST, pQuest->GetQuestId());

    uint32 zone = 0;
    uint32 area = 0;

    // remove auras from spells with quest reward state limitations
    SpellAreaForQuestMapBounds saEndBounds = sSpellMgr.GetSpellAreaForQuestEndMapBounds(quest_id);
    if (saEndBounds.first != saEndBounds.second)
    {
        GetZoneAndAreaId(zone,area);

        for(SpellAreaForAreaMap::const_iterator itr = saEndBounds.first; itr != saEndBounds.second; ++itr)
            if (!itr->second->IsFitToRequirements(this, zone, area))
                RemoveAurasDueToSpell(itr->second->spellId);
    }

    // Some spells applied at quest reward
    SpellAreaForQuestMapBounds saBounds = sSpellMgr.GetSpellAreaForQuestMapBounds(quest_id, false);
    if (saBounds.first != saBounds.second)
    {
        if (!zone || !area)
            GetZoneAndAreaId(zone, area);

        for(SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
            if (itr->second->autocast && itr->second->IsFitToRequirements(this, zone, area))
                if (!HasAura(itr->second->spellId, EFFECT_INDEX_0))
                    CastSpell(this, itr->second->spellId, true);
    }
}

void Player::FailQuest(uint32 questId)
{
    if (Quest const* pQuest = sObjectMgr.GetQuestTemplate(questId))
    {
        SetQuestStatus(questId, QUEST_STATUS_FAILED);

        uint16 log_slot = FindQuestSlot(questId);

        if (log_slot < MAX_QUEST_LOG_SIZE)
        {
            SetQuestSlotTimer(log_slot, 1);
            SetQuestSlotState(log_slot, QUEST_STATE_FAIL);
        }

        if (pQuest->HasSpecialFlag(QUEST_SPECIAL_FLAG_TIMED))
        {
            QuestStatusData& q_status = mQuestStatus[questId];

            RemoveTimedQuest(questId);
            q_status.m_timer = 0;

            SendQuestTimerFailed(questId);
        }
        else
            SendQuestFailed(questId);
    }
}

bool Player::SatisfyQuestSkill(Quest const* qInfo, bool msg) const
{
    uint32 skill = qInfo->GetRequiredSkill();

    // skip 0 case RequiredSkill
    if (skill == 0)
        return true;

    // check skill value
    if (GetSkillValue(skill) < qInfo->GetRequiredSkillValue())
    {
        if (msg)
            SendCanTakeQuestResponse(INVALIDREASON_DONT_HAVE_REQ);

        return false;
    }

    return true;
}

bool Player::SatisfyQuestLevel(Quest const* qInfo, bool msg) const
{
    if (getLevel() < qInfo->GetMinLevel())
    {
        if (msg)
            SendCanTakeQuestResponse(INVALIDREASON_DONT_HAVE_REQ);

        return false;
    }

    return true;
}

bool Player::SatisfyQuestLog(bool msg) const
{
    // exist free slot
    if (FindQuestSlot(0) < MAX_QUEST_LOG_SIZE)
        return true;

    if (msg)
    {
        WorldPacket data(SMSG_QUESTLOG_FULL, 0);
        GetSession()->SendPacket(&data);
        DEBUG_LOG("WORLD: Sent SMSG_QUESTLOG_FULL");
    }
    return false;
}

bool Player::SatisfyQuestPreviousQuest(Quest const* qInfo, bool msg) const
{
    // No previous quest (might be first quest in a series)
    if (qInfo->prevQuests.empty())
        return true;

    for(Quest::PrevQuests::const_iterator iter = qInfo->prevQuests.begin(); iter != qInfo->prevQuests.end(); ++iter)
    {
        uint32 prevId = abs(*iter);

        QuestStatusMap::const_iterator i_prevstatus = mQuestStatus.find(prevId);
        Quest const* qPrevInfo = sObjectMgr.GetQuestTemplate(prevId);

        if (qPrevInfo && i_prevstatus != mQuestStatus.end())
        {
            // If any of the positive previous quests completed, return true
            if (*iter > 0 && i_prevstatus->second.m_rewarded)
            {
                // skip one-from-all exclusive group
                if (qPrevInfo->GetExclusiveGroup() >= 0)
                    return true;

                // each-from-all exclusive group ( < 0)
                // can be start if only all quests in prev quest exclusive group completed and rewarded
                ExclusiveQuestGroupsMapBounds bounds = sObjectMgr.GetExclusiveQuestGroupsMapBounds(qPrevInfo->GetExclusiveGroup());

                MANGOS_ASSERT(bounds.first != bounds.second); // always must be found if qPrevInfo->ExclusiveGroup != 0

                for(ExclusiveQuestGroupsMap::const_iterator iter2 = bounds.first; iter2 != bounds.second; ++iter2)
                {
                    uint32 exclude_Id = iter2->second;

                    // skip checked quest id, only state of other quests in group is interesting
                    if (exclude_Id == prevId)
                        continue;

                    QuestStatusMap::const_iterator i_exstatus = mQuestStatus.find(exclude_Id);

                    // alternative quest from group also must be completed and rewarded(reported)
                    if (i_exstatus == mQuestStatus.end() || !i_exstatus->second.m_rewarded)
                    {
                        if (msg)
                            SendCanTakeQuestResponse(INVALIDREASON_DONT_HAVE_REQ);

                        return false;
                    }
                }
                return true;
            }
            // If any of the negative previous quests active, return true
            if (*iter < 0 && IsCurrentQuest(prevId))
            {
                // skip one-from-all exclusive group
                if (qPrevInfo->GetExclusiveGroup() >= 0)
                    return true;

                // each-from-all exclusive group ( < 0)
                // can be start if only all quests in prev quest exclusive group active
                ExclusiveQuestGroupsMapBounds bounds = sObjectMgr.GetExclusiveQuestGroupsMapBounds(qPrevInfo->GetExclusiveGroup());

                MANGOS_ASSERT(bounds.first != bounds.second); // always must be found if qPrevInfo->ExclusiveGroup != 0

                for(ExclusiveQuestGroupsMap::const_iterator iter2 = bounds.first; iter2 != bounds.second; ++iter2)
                {
                    uint32 exclude_Id = iter2->second;

                    // skip checked quest id, only state of other quests in group is interesting
                    if (exclude_Id == prevId)
                        continue;

                    // alternative quest from group also must be active
                    if (!IsCurrentQuest(exclude_Id))
                    {
                        if (msg)
                            SendCanTakeQuestResponse(INVALIDREASON_DONT_HAVE_REQ);

                        return false;
                    }
                }
                return true;
            }
        }
    }

    // Has only positive prev. quests in non-rewarded state
    // and negative prev. quests in non-active state
    if (msg)
        SendCanTakeQuestResponse(INVALIDREASON_DONT_HAVE_REQ);

    return false;
}

bool Player::SatisfyQuestClass(Quest const* qInfo, bool msg) const
{
    uint32 reqClass = qInfo->GetRequiredClasses();

    if (reqClass == 0)
        return true;

    if ((reqClass & getClassMask()) == 0)
    {
        if (msg)
            SendCanTakeQuestResponse(INVALIDREASON_DONT_HAVE_REQ);

        return false;
    }

    return true;
}

bool Player::SatisfyQuestRace(Quest const* qInfo, bool msg) const
{
    uint32 reqraces = qInfo->GetRequiredRaces();

    if (reqraces == 0)
        return true;

    if ((reqraces & getRaceMask()) == 0)
    {
        if (msg)
            SendCanTakeQuestResponse(INVALIDREASON_QUEST_FAILED_WRONG_RACE);

        return false;
    }

    return true;
}

bool Player::SatisfyQuestReputation(Quest const* qInfo, bool msg) const
{
    uint32 fIdMin = qInfo->GetRequiredMinRepFaction();      //Min required rep
    if (fIdMin && GetReputationMgr().GetReputation(fIdMin) < qInfo->GetRequiredMinRepValue())
    {
        if (msg)
            SendCanTakeQuestResponse(INVALIDREASON_DONT_HAVE_REQ);

        return false;
    }

    uint32 fIdMax = qInfo->GetRequiredMaxRepFaction();      //Max required rep
    if (fIdMax && GetReputationMgr().GetReputation(fIdMax) >= qInfo->GetRequiredMaxRepValue())
    {
        if (msg)
            SendCanTakeQuestResponse(INVALIDREASON_DONT_HAVE_REQ);

        return false;
    }

    return true;
}

bool Player::SatisfyQuestStatus(Quest const* qInfo, bool msg) const
{
    QuestStatusMap::const_iterator itr = mQuestStatus.find(qInfo->GetQuestId());

    if (itr != mQuestStatus.end() && itr->second.m_status != QUEST_STATUS_NONE)
    {
        if (msg)
            SendCanTakeQuestResponse(INVALIDREASON_QUEST_ALREADY_ON);

        return false;
    }

    return true;
}

bool Player::SatisfyQuestTimed(Quest const* qInfo, bool msg) const
{
    if (!m_timedquests.empty() && qInfo->HasSpecialFlag(QUEST_SPECIAL_FLAG_TIMED))
    {
        if (msg)
            SendCanTakeQuestResponse(INVALIDREASON_QUEST_ONLY_ONE_TIMED);

        return false;
    }

    return true;
}

bool Player::SatisfyQuestExclusiveGroup(Quest const* qInfo, bool msg) const
{
    // non positive exclusive group, if > 0 then can be start if any other quest in exclusive group already started/completed
    if (qInfo->GetExclusiveGroup() <= 0)
        return true;

    ExclusiveQuestGroupsMapBounds bounds = sObjectMgr.GetExclusiveQuestGroupsMapBounds(qInfo->GetExclusiveGroup());

    MANGOS_ASSERT(bounds.first != bounds.second);           // must always be found if qInfo->ExclusiveGroup != 0

    for(ExclusiveQuestGroupsMap::const_iterator iter = bounds.first; iter != bounds.second; ++iter)
    {
        uint32 exclude_Id = iter->second;

        // skip checked quest id, only state of other quests in group is interesting
        if (exclude_Id == qInfo->GetQuestId())
            continue;

        // not allow have daily quest if daily quest from exclusive group already recently completed
        Quest const* Nquest = sObjectMgr.GetQuestTemplate(exclude_Id);
        if (!SatisfyQuestDay(Nquest, false) || !SatisfyQuestWeek(Nquest, false))
        {
            if (msg)
                SendCanTakeQuestResponse(INVALIDREASON_DONT_HAVE_REQ);

            return false;
        }

        QuestStatusMap::const_iterator i_exstatus = mQuestStatus.find(exclude_Id);

        // alternative quest already started or completed
        if (i_exstatus != mQuestStatus.end() &&
            (i_exstatus->second.m_status == QUEST_STATUS_COMPLETE || i_exstatus->second.m_status == QUEST_STATUS_INCOMPLETE))
        {
            if (msg)
                SendCanTakeQuestResponse(INVALIDREASON_DONT_HAVE_REQ);

            return false;
        }
    }

    return true;
}

bool Player::SatisfyQuestNextChain(Quest const* qInfo, bool msg) const
{
    if (!qInfo->GetNextQuestInChain())
        return true;

    // next quest in chain already started or completed
    QuestStatusMap::const_iterator itr = mQuestStatus.find(qInfo->GetNextQuestInChain());
    if (itr != mQuestStatus.end() &&
        (itr->second.m_status == QUEST_STATUS_COMPLETE || itr->second.m_status == QUEST_STATUS_INCOMPLETE))
    {
        if (msg)
            SendCanTakeQuestResponse(INVALIDREASON_DONT_HAVE_REQ);

        return false;
    }

    // check for all quests further up the chain
    // only necessary if there are quest chains with more than one quest that can be skipped
    //return SatisfyQuestNextChain( qInfo->GetNextQuestInChain(), msg );
    return true;
}

bool Player::SatisfyQuestPrevChain(Quest const* qInfo, bool msg) const
{
    // No previous quest in chain
    if (qInfo->prevChainQuests.empty())
        return true;

    for(Quest::PrevChainQuests::const_iterator iter = qInfo->prevChainQuests.begin(); iter != qInfo->prevChainQuests.end(); ++iter)
    {
        uint32 prevId = *iter;

        // If any of the previous quests in chain active, return false
        if (IsCurrentQuest(prevId))
        {
            if (msg)
                SendCanTakeQuestResponse(INVALIDREASON_DONT_HAVE_REQ);

            return false;
        }

        // check for all quests further down the chain
        // only necessary if there are quest chains with more than one quest that can be skipped
        //if( !SatisfyQuestPrevChain( prevId, msg ) )
        //    return false;
    }

    // No previous quest in chain active
    return true;
}

bool Player::SatisfyQuestDay(Quest const* qInfo, bool msg) const
{
    if (!qInfo->IsDaily())
        return true;

    bool have_slot = false;
    for(uint32 quest_daily_idx = 0; quest_daily_idx < PLAYER_MAX_DAILY_QUESTS; ++quest_daily_idx)
    {
        uint32 id = GetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx);
        if (qInfo->GetQuestId()==id)
            return false;

        if (!id)
            have_slot = true;
    }

    if (!have_slot)
    {
        if (msg)
            SendCanTakeQuestResponse(INVALIDREASON_QUEST_FAILED_TOO_MANY_DAILY_QUESTS);

        return false;
    }

    return true;
}

bool Player::SatisfyQuestWeek(Quest const* qInfo, bool msg) const
{
    if (!qInfo->IsWeekly() || m_weeklyquests.empty())
        return true;

    // if not found in cooldown list
    return m_weeklyquests.find(qInfo->GetQuestId()) == m_weeklyquests.end();
}

bool Player::SatisfyQuestMonth(Quest const* qInfo, bool msg) const
{
    if (!qInfo->IsMonthly() || m_monthlyquests.empty())
        return true;

    // if not found in cooldown list
    return m_monthlyquests.find(qInfo->GetQuestId()) == m_monthlyquests.end();
}

bool Player::CanGiveQuestSourceItemIfNeed( Quest const *pQuest, ItemPosCountVec* dest) const
{
    if (uint32 srcitem = pQuest->GetSrcItemId())
    {
        uint32 count = pQuest->GetSrcItemCount();

        // player already have max amount required item (including bank), just report success
        uint32 has_count = GetItemCount(srcitem, true);
        if (has_count >= count)
            return true;

        count -= has_count;                                 // real need amount

        InventoryResult msg;
        if (!dest)
        {
            ItemPosCountVec destTemp;
            msg = CanStoreNewItem( NULL_BAG, NULL_SLOT, destTemp, srcitem, count );
        }
        else
            msg = CanStoreNewItem( NULL_BAG, NULL_SLOT, *dest, srcitem, count );

        if (msg == EQUIP_ERR_OK)
            return true;
        else
            SendEquipError( msg, NULL, NULL, srcitem );
        return false;
    }

    return true;
}

void Player::GiveQuestSourceItemIfNeed(Quest const *pQuest)
{
    ItemPosCountVec dest;
    if (CanGiveQuestSourceItemIfNeed(pQuest, &dest) && !dest.empty())
    {
        uint32 count = 0;
        for(ItemPosCountVec::const_iterator c_itr = dest.begin(); c_itr != dest.end(); ++c_itr)
            count += c_itr->count;

        Item * item = StoreNewItem(dest, pQuest->GetSrcItemId(), true);
        SendNewItem(item, count, true, false);
    }
}


bool Player::TakeQuestSourceItem( uint32 quest_id, bool msg )
{
    Quest const* qInfo = sObjectMgr.GetQuestTemplate(quest_id);
    if( qInfo )
    {
        uint32 srcitem = qInfo->GetSrcItemId();
        if( srcitem > 0 )
        {
            uint32 count = qInfo->GetSrcItemCount();
            if( count <= 0 )
                count = 1;

            // exist one case when destroy source quest item not possible:
            // non un-equippable item (equipped non-empty bag, for example)
            InventoryResult res = CanUnequipItems(srcitem,count);
            if(res != EQUIP_ERR_OK)
            {
                if(msg)
                    SendEquipError( res, NULL, NULL, srcitem );
                return false;
            }

            DestroyItemCount(srcitem, count, true, true);
        }
    }
    return true;
}

bool Player::GetQuestRewardStatus( uint32 quest_id ) const
{
    Quest const* qInfo = sObjectMgr.GetQuestTemplate(quest_id);
    if( qInfo )
    {
        // for repeatable quests: rewarded field is set after first reward only to prevent getting XP more than once
        QuestStatusMap::const_iterator itr = mQuestStatus.find( quest_id );
        if( itr != mQuestStatus.end() && itr->second.m_status != QUEST_STATUS_NONE
            && !qInfo->IsRepeatable() )
            return itr->second.m_rewarded;

        return false;
    }
    return false;
}

QuestStatus Player::GetQuestStatus( uint32 quest_id ) const
{
    if( quest_id )
    {
        QuestStatusMap::const_iterator itr = mQuestStatus.find( quest_id );
        if( itr != mQuestStatus.end() )
            return itr->second.m_status;
    }
    return QUEST_STATUS_NONE;
}

bool Player::CanShareQuest(uint32 quest_id) const
{
    if (Quest const* qInfo = sObjectMgr.GetQuestTemplate(quest_id))
        if (qInfo->HasQuestFlag(QUEST_FLAGS_SHARABLE))
            return IsCurrentQuest(quest_id);

    return false;
}

void Player::SetQuestStatus(uint32 quest_id, QuestStatus status)
{
    if (sObjectMgr.GetQuestTemplate(quest_id))
    {
        QuestStatusData& q_status = mQuestStatus[quest_id];

        q_status.m_status = status;

        if (q_status.uState != QUEST_NEW)
            q_status.uState = QUEST_CHANGED;
    }

    UpdateForQuestWorldObjects();
}

// not used in MaNGOS, but used in scripting code
uint32 Player::GetReqKillOrCastCurrentCount(uint32 quest_id, int32 entry)
{
    Quest const* qInfo = sObjectMgr.GetQuestTemplate(quest_id);
    if( !qInfo )
        return 0;

    for (int j = 0; j < QUEST_OBJECTIVES_COUNT; ++j)
        if ( qInfo->ReqCreatureOrGOId[j] == entry )
            return mQuestStatus[quest_id].m_creatureOrGOcount[j];

    return 0;
}

void Player::AdjustQuestReqItemCount( Quest const* pQuest, QuestStatusData& questStatusData )
{
    if (pQuest->HasSpecialFlag(QUEST_SPECIAL_FLAG_DELIVER))
    {
        for(int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
        {
            uint32 reqitemcount = pQuest->ReqItemCount[i];
            if( reqitemcount != 0 )
            {
                uint32 curitemcount = GetItemCount(pQuest->ReqItemId[i], true);

                questStatusData.m_itemcount[i] = std::min(curitemcount, reqitemcount);
                if (questStatusData.uState != QUEST_NEW) questStatusData.uState = QUEST_CHANGED;
            }
        }
    }
}

uint16 Player::FindQuestSlot( uint32 quest_id ) const
{
    for ( uint16 i = 0; i < MAX_QUEST_LOG_SIZE; ++i )
        if ( GetQuestSlotQuestId(i) == quest_id )
            return i;

    return MAX_QUEST_LOG_SIZE;
}

void Player::AreaExploredOrEventHappens( uint32 questId )
{
    if( questId )
    {
        uint16 log_slot = FindQuestSlot( questId );
        if( log_slot < MAX_QUEST_LOG_SIZE)
        {
            QuestStatusData& q_status = mQuestStatus[questId];

            if(!q_status.m_explored)
            {
                SetQuestSlotState(log_slot, QUEST_STATE_COMPLETE);
                SendQuestCompleteEvent(questId);
                q_status.m_explored = true;

                if (q_status.uState != QUEST_NEW)
                    q_status.uState = QUEST_CHANGED;
            }
        }
        if( CanCompleteQuest( questId ) )
            CompleteQuest( questId );
    }
}

//not used in mangosd, function for external script library
void Player::GroupEventHappens( uint32 questId, WorldObject const* pEventObject )
{
    if( Group *pGroup = GetGroup() )
    {
        for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player *pGroupGuy = itr->getSource();

            // for any leave or dead (with not released body) group member at appropriate distance
            if( pGroupGuy && pGroupGuy->IsAtGroupRewardDistance(pEventObject) && !pGroupGuy->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST) )
                pGroupGuy->AreaExploredOrEventHappens(questId);
        }
    }
    else
        AreaExploredOrEventHappens(questId);
}

void Player::ItemAddedQuestCheck( uint32 entry, uint32 count )
{
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; ++i )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if ( questid == 0 )
            continue;

        QuestStatusData& q_status = mQuestStatus[questid];

        if ( q_status.m_status != QUEST_STATUS_INCOMPLETE )
            continue;

        Quest const* qInfo = sObjectMgr.GetQuestTemplate(questid);
        if (!qInfo || !qInfo->HasSpecialFlag(QUEST_SPECIAL_FLAG_DELIVER))
            continue;

        for (int j = 0; j < QUEST_ITEM_OBJECTIVES_COUNT; ++j)
        {
            uint32 reqitem = qInfo->ReqItemId[j];
            if ( reqitem == entry )
            {
                uint32 reqitemcount = qInfo->ReqItemCount[j];
                uint32 curitemcount = q_status.m_itemcount[j];
                if ( curitemcount < reqitemcount )
                {
                    uint32 additemcount = ( curitemcount + count <= reqitemcount ? count : reqitemcount - curitemcount);
                    q_status.m_itemcount[j] += additemcount;
                    if (q_status.uState != QUEST_NEW)
                        q_status.uState = QUEST_CHANGED;
                }
                if ( CanCompleteQuest( questid ) )
                    CompleteQuest( questid );
                return;
            }
        }
    }
    UpdateForQuestWorldObjects();
}

void Player::ItemRemovedQuestCheck( uint32 entry, uint32 count )
{
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; ++i )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if(!questid)
            continue;
        Quest const* qInfo = sObjectMgr.GetQuestTemplate(questid);
        if ( !qInfo )
            continue;
        if (!qInfo->HasSpecialFlag(QUEST_SPECIAL_FLAG_DELIVER))
            continue;

        for (int j = 0; j < QUEST_ITEM_OBJECTIVES_COUNT; ++j)
        {
            uint32 reqitem = qInfo->ReqItemId[j];
            if ( reqitem == entry )
            {
                QuestStatusData& q_status = mQuestStatus[questid];

                uint32 reqitemcount = qInfo->ReqItemCount[j];
                uint32 curitemcount;
                if( q_status.m_status != QUEST_STATUS_COMPLETE )
                    curitemcount = q_status.m_itemcount[j];
                else
                    curitemcount = GetItemCount(entry, true);
                if ( curitemcount < reqitemcount + count )
                {
                    uint32 remitemcount = ( curitemcount <= reqitemcount ? count : count + reqitemcount - curitemcount);
                    q_status.m_itemcount[j] = curitemcount - remitemcount;
                    if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;

                    IncompleteQuest( questid );
                }
                return;
            }
        }
    }
    UpdateForQuestWorldObjects();
}

void Player::KilledMonster( CreatureInfo const* cInfo, ObjectGuid guid )
{
    if(cInfo->Entry)
        KilledMonsterCredit(cInfo->Entry, guid);

    for(int i = 0; i < MAX_KILL_CREDIT; ++i)
        if(cInfo->KillCredit[i])
            KilledMonsterCredit(cInfo->KillCredit[i], guid);
}

void Player::KilledMonsterCredit( uint32 entry, ObjectGuid guid )
{
    uint32 addkillcount = 1;
    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE, entry, addkillcount);
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; ++i )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if(!questid)
            continue;

        Quest const* qInfo = sObjectMgr.GetQuestTemplate(questid);
        if (!qInfo)
            continue;
        // just if !ingroup || !noraidgroup || raidgroup
        QuestStatusData& q_status = mQuestStatus[questid];
        if (q_status.m_status == QUEST_STATUS_INCOMPLETE && (!GetGroup() || !GetGroup()->isRaidGroup() || qInfo->IsAllowedInRaid()))
        {
            if (qInfo->HasSpecialFlag(QUEST_SPECIAL_FLAG_KILL_OR_CAST))
            {
                for (int j = 0; j < QUEST_OBJECTIVES_COUNT; ++j)
                {
                    // skip GO activate objective or none
                    if (qInfo->ReqCreatureOrGOId[j] <=0)
                        continue;

                    // skip Cast at creature objective
                    if (qInfo->ReqSpell[j] !=0 )
                        continue;

                    uint32 reqkill = qInfo->ReqCreatureOrGOId[j];

                    if (reqkill == entry)
                    {
                        uint32 reqkillcount = qInfo->ReqCreatureOrGOCount[j];
                        uint32 curkillcount = q_status.m_creatureOrGOcount[j];
                        if (curkillcount < reqkillcount)
                        {
                            q_status.m_creatureOrGOcount[j] = curkillcount + addkillcount;
                            if (q_status.uState != QUEST_NEW)
                                q_status.uState = QUEST_CHANGED;

                            SendQuestUpdateAddCreatureOrGo( qInfo, guid, j, q_status.m_creatureOrGOcount[j]);
                        }

                        if (CanCompleteQuest( questid ))
                            CompleteQuest( questid );

                        // same objective target can be in many active quests, but not in 2 objectives for single quest (code optimization).
                        continue;
                    }
                }
            }
        }
    }
}

void Player::CastedCreatureOrGO( uint32 entry, ObjectGuid guid, uint32 spell_id, bool original_caster )
{
    bool isCreature = guid.IsCreatureOrVehicle();

    uint32 addCastCount = 1;
    for(int i = 0; i < MAX_QUEST_LOG_SIZE; ++i)
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if (!questid)
            continue;

        Quest const* qInfo = sObjectMgr.GetQuestTemplate(questid);
        if (!qInfo)
            continue;

        if (!original_caster && !qInfo->HasQuestFlag(QUEST_FLAGS_SHARABLE))
            continue;

        if (!qInfo->HasSpecialFlag(QUEST_SPECIAL_FLAG_KILL_OR_CAST))
            continue;

        QuestStatusData& q_status = mQuestStatus[questid];

        if (q_status.m_status != QUEST_STATUS_INCOMPLETE)
            continue;

        for (int j = 0; j < QUEST_OBJECTIVES_COUNT; ++j)
        {
            // skip kill creature objective (0) or wrong spell casts
            if (qInfo->ReqSpell[j] != spell_id)
                continue;

            uint32 reqTarget = 0;

            if (isCreature)
            {
                // creature activate objectives
                if (qInfo->ReqCreatureOrGOId[j] > 0)
                    // checked at quest_template loading
                    reqTarget = qInfo->ReqCreatureOrGOId[j];
            }
            else
            {
                // GO activate objective
                if (qInfo->ReqCreatureOrGOId[j] < 0)
                    // checked at quest_template loading
                    reqTarget = - qInfo->ReqCreatureOrGOId[j];
            }

            // other not this creature/GO related objectives
            if (reqTarget != entry)
                continue;

            uint32 reqCastCount = qInfo->ReqCreatureOrGOCount[j];
            uint32 curCastCount = q_status.m_creatureOrGOcount[j];
            if (curCastCount < reqCastCount)
            {
                q_status.m_creatureOrGOcount[j] = curCastCount + addCastCount;
                if (q_status.uState != QUEST_NEW)
                    q_status.uState = QUEST_CHANGED;

                SendQuestUpdateAddCreatureOrGo( qInfo, guid, j, q_status.m_creatureOrGOcount[j]);
            }

            if (CanCompleteQuest(questid))
                CompleteQuest(questid);

            // same objective target can be in many active quests, but not in 2 objectives for single quest (code optimization).
            break;
        }
    }
}

void Player::TalkedToCreature( uint32 entry, ObjectGuid guid )
{
    uint32 addTalkCount = 1;
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; ++i )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if(!questid)
            continue;

        Quest const* qInfo = sObjectMgr.GetQuestTemplate(questid);
        if ( !qInfo )
            continue;

        QuestStatusData& q_status = mQuestStatus[questid];

        if ( q_status.m_status == QUEST_STATUS_INCOMPLETE )
        {
            if (qInfo->HasSpecialFlag(QuestSpecialFlags(QUEST_SPECIAL_FLAG_KILL_OR_CAST | QUEST_SPECIAL_FLAG_SPEAKTO)))
            {
                for (int j = 0; j < QUEST_OBJECTIVES_COUNT; ++j)
                {
                                                            // skip spell casts and Gameobject objectives
                    if(qInfo->ReqSpell[j] > 0 || qInfo->ReqCreatureOrGOId[j] < 0)
                        continue;

                    uint32 reqTarget = 0;

                    if(qInfo->ReqCreatureOrGOId[j] > 0)     // creature activate objectives
                                                            // checked at quest_template loading
                        reqTarget = qInfo->ReqCreatureOrGOId[j];
                    else
                        continue;

                    if ( reqTarget == entry )
                    {
                        uint32 reqTalkCount = qInfo->ReqCreatureOrGOCount[j];
                        uint32 curTalkCount = q_status.m_creatureOrGOcount[j];
                        if ( curTalkCount < reqTalkCount )
                        {
                            q_status.m_creatureOrGOcount[j] = curTalkCount + addTalkCount;
                            if (q_status.uState != QUEST_NEW) q_status.uState = QUEST_CHANGED;

                            SendQuestUpdateAddCreatureOrGo( qInfo, guid, j, q_status.m_creatureOrGOcount[j]);
                        }
                        if ( CanCompleteQuest( questid ) )
                            CompleteQuest( questid );

                        // same objective target can be in many active quests, but not in 2 objectives for single quest (code optimization).
                        continue;
                    }
                }
            }
        }
    }
}

void Player::MoneyChanged( uint32 count )
{
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; ++i )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if (!questid)
            continue;

        Quest const* qInfo = sObjectMgr.GetQuestTemplate(questid);
        if( qInfo && qInfo->GetRewOrReqMoney() < 0 )
        {
            QuestStatusData& q_status = mQuestStatus[questid];

            if( q_status.m_status == QUEST_STATUS_INCOMPLETE )
            {
                if(int32(count) >= -qInfo->GetRewOrReqMoney())
                {
                    if ( CanCompleteQuest( questid ) )
                        CompleteQuest( questid );
                }
            }
            else if( q_status.m_status == QUEST_STATUS_COMPLETE )
            {
                if(int32(count) < -qInfo->GetRewOrReqMoney())
                    IncompleteQuest( questid );
            }
        }
    }
}

void Player::ReputationChanged(FactionEntry const* factionEntry )
{
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; ++i )
    {
        if(uint32 questid = GetQuestSlotQuestId(i))
        {
            if(Quest const* qInfo = sObjectMgr.GetQuestTemplate(questid))
            {
                if(qInfo->GetRepObjectiveFaction() == factionEntry->ID )
                {
                    QuestStatusData& q_status = mQuestStatus[questid];
                    if( q_status.m_status == QUEST_STATUS_INCOMPLETE )
                    {
                        if(GetReputationMgr().GetReputation(factionEntry) >= qInfo->GetRepObjectiveValue())
                            if ( CanCompleteQuest( questid ) )
                                CompleteQuest( questid );
                    }
                    else if( q_status.m_status == QUEST_STATUS_COMPLETE )
                    {
                        if(GetReputationMgr().GetReputation(factionEntry) < qInfo->GetRepObjectiveValue())
                            IncompleteQuest( questid );
                    }
                }
            }
        }
    }
}

bool Player::HasQuestForItem( uint32 itemid ) const
{
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; ++i )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if ( questid == 0 )
            continue;

        QuestStatusMap::const_iterator qs_itr = mQuestStatus.find(questid);
        if(qs_itr == mQuestStatus.end())
            continue;

        QuestStatusData const& q_status = qs_itr->second;

        if (q_status.m_status == QUEST_STATUS_INCOMPLETE)
        {
            Quest const* qinfo = sObjectMgr.GetQuestTemplate(questid);
            if(!qinfo)
                continue;

            // hide quest if player is in raid-group and quest is no raid quest
            if (GetGroup() && GetGroup()->isRaidGroup() && !qinfo->IsAllowedInRaid() && !InBattleGround())
                continue;

            // There should be no mixed ReqItem/ReqSource drop
            // This part for ReqItem drop
            for (int j = 0; j < QUEST_ITEM_OBJECTIVES_COUNT; ++j)
            {
                if(itemid == qinfo->ReqItemId[j] && q_status.m_itemcount[j] < qinfo->ReqItemCount[j] )
                    return true;
            }
            // This part - for ReqSource
            for (int j = 0; j < QUEST_SOURCE_ITEM_IDS_COUNT; ++j)
            {
                // examined item is a source item
                if (qinfo->ReqSourceId[j] == itemid)
                {
                    ItemPrototype const *pProto = ObjectMgr::GetItemPrototype(itemid);

                    // 'unique' item
                    if (pProto->MaxCount && (int32)GetItemCount(itemid,true) < pProto->MaxCount)
                        return true;

                    // allows custom amount drop when not 0
                    if (qinfo->ReqSourceCount[j])
                    {
                        if (GetItemCount(itemid,true) < qinfo->ReqSourceCount[j])
                            return true;
                    } else if ((int32)GetItemCount(itemid,true) < pProto->Stackable)
                        return true;
                }
            }
        }
    }
    return false;
}

// Used for quests having some event (explore, escort, "external event") as quest objective.
void Player::SendQuestCompleteEvent(uint32 quest_id)
{
    if (quest_id)
    {
        WorldPacket data(SMSG_QUESTUPDATE_COMPLETE, 4);
        data << uint32(quest_id);
        GetSession()->SendPacket(&data);
        DEBUG_LOG("WORLD: Sent SMSG_QUESTUPDATE_COMPLETE quest = %u", quest_id);
    }
}

void Player::SendQuestReward( Quest const *pQuest, uint32 XP, Object * questGiver )
{
    uint32 questid = pQuest->GetQuestId();
    DEBUG_LOG( "WORLD: Sent SMSG_QUESTGIVER_QUEST_COMPLETE quest = %u", questid );
    WorldPacket data( SMSG_QUESTGIVER_QUEST_COMPLETE, (4+4+4+4+4) );
    data << uint32(questid);

    if ( getLevel() < sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL) )
    {
        data << uint32(XP);
        data << uint32(pQuest->GetRewOrReqMoney());
    }
    else
    {
        data << uint32(0);
        data << uint32(pQuest->GetRewOrReqMoney() + int32(pQuest->GetRewMoneyMaxLevel() * sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_MONEY)));
    }

    data << uint32(10*MaNGOS::Honor::hk_honor_at_level(getLevel(), pQuest->GetRewHonorAddition()));
    data << uint32(pQuest->GetBonusTalents());              // bonus talents
    data << uint32(0);                                      // arena points
    GetSession()->SendPacket( &data );
}

void Player::SendQuestFailed( uint32 quest_id, InventoryResult reason)
{
    if( quest_id )
    {
        WorldPacket data( SMSG_QUESTGIVER_QUEST_FAILED, 4+4 );
        data << uint32(quest_id);
        data << uint32(reason);                             // failed reason (valid reasons: 4, 16, 50, 17, 74, other values show default message)
        GetSession()->SendPacket( &data );
        DEBUG_LOG("WORLD: Sent SMSG_QUESTGIVER_QUEST_FAILED");
    }
}

void Player::SendQuestTimerFailed( uint32 quest_id )
{
    if( quest_id )
    {
        WorldPacket data( SMSG_QUESTUPDATE_FAILEDTIMER, 4 );
        data << uint32(quest_id);
        GetSession()->SendPacket( &data );
        DEBUG_LOG("WORLD: Sent SMSG_QUESTUPDATE_FAILEDTIMER");
    }
}

void Player::SendCanTakeQuestResponse( uint32 msg ) const
{
    WorldPacket data( SMSG_QUESTGIVER_QUEST_INVALID, 4 );
    data << uint32(msg);
    GetSession()->SendPacket( &data );
    DEBUG_LOG("WORLD: Sent SMSG_QUESTGIVER_QUEST_INVALID");
}

void Player::SendQuestConfirmAccept(const Quest* pQuest, Player* pReceiver)
{
    if (pReceiver)
    {
        int loc_idx = pReceiver->GetSession()->GetSessionDbLocaleIndex();
        std::string title = pQuest->GetTitle();
        sObjectMgr.GetQuestLocaleStrings(pQuest->GetQuestId(), loc_idx, &title);

        WorldPacket data(SMSG_QUEST_CONFIRM_ACCEPT, (4 + title.size() + 8));
        data << uint32(pQuest->GetQuestId());
        data << title;
        data << GetObjectGuid();
        pReceiver->GetSession()->SendPacket(&data);

        DEBUG_LOG("WORLD: Sent SMSG_QUEST_CONFIRM_ACCEPT");
    }
}

void Player::SendPushToPartyResponse( Player *pPlayer, uint32 msg )
{
    if( pPlayer )
    {
        WorldPacket data( MSG_QUEST_PUSH_RESULT, (8+1) );
        data << pPlayer->GetObjectGuid();
        data << uint8(msg);                                 // valid values: 0-8
        GetSession()->SendPacket( &data );
        DEBUG_LOG("WORLD: Sent MSG_QUEST_PUSH_RESULT");
    }
}

void Player::SendQuestUpdateAddCreatureOrGo( Quest const* pQuest, ObjectGuid guid, uint32 creatureOrGO_idx, uint32 count)
{
    MANGOS_ASSERT(count < 65536 && "mob/GO count store in 16 bits 2^16 = 65536 (0..65536)");

    int32 entry = pQuest->ReqCreatureOrGOId[ creatureOrGO_idx ];
    if (entry < 0)
        // client expected gameobject template id in form (id|0x80000000)
        entry = (-entry) | 0x80000000;

    WorldPacket data( SMSG_QUESTUPDATE_ADD_KILL, (4*4+8) );
    DEBUG_LOG( "WORLD: Sent SMSG_QUESTUPDATE_ADD_KILL" );
    data << uint32(pQuest->GetQuestId());
    data << uint32(entry);
    data << uint32(count);
    data << uint32(pQuest->ReqCreatureOrGOCount[ creatureOrGO_idx ]);
    data << guid;
    GetSession()->SendPacket(&data);

    uint16 log_slot = FindQuestSlot( pQuest->GetQuestId() );
    if (log_slot < MAX_QUEST_LOG_SIZE)
        SetQuestSlotCounter(log_slot, creatureOrGO_idx, count);
}

/*********************************************************/
/***                   LOAD SYSTEM                     ***/
/*********************************************************/

void Player::_LoadDeclinedNames(QueryResult* result)
{
    if(!result)
        return;

    if(m_declinedname)
        delete m_declinedname;

    m_declinedname = new DeclinedName;
    Field *fields = result->Fetch();
    for(int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        m_declinedname->name[i] = fields[i].GetCppString();

    delete result;
}

void Player::_LoadArenaTeamInfo(QueryResult *result)
{
    // arenateamid, played_week, played_season, personal_rating
    memset((void*)&m_uint32Values[PLAYER_FIELD_ARENA_TEAM_INFO_1_1], 0, sizeof(uint32) * MAX_ARENA_SLOT * ARENA_TEAM_END);
    if (!result)
        return;

    do
    {
        Field *fields = result->Fetch();

        uint32 arenateamid     = fields[0].GetUInt32();
        uint32 played_week     = fields[1].GetUInt32();
        uint32 played_season   = fields[2].GetUInt32();
        uint32 wons_season     = fields[3].GetUInt32();
        uint32 personal_rating = fields[4].GetUInt32();

        ArenaTeam* aTeam = sObjectMgr.GetArenaTeamById(arenateamid);
        if(!aTeam)
        {
            sLog.outError("Player::_LoadArenaTeamInfo: couldn't load arenateam %u", arenateamid);
            continue;
        }
        uint8  arenaSlot = aTeam->GetSlot();

        SetArenaTeamInfoField(arenaSlot, ARENA_TEAM_ID, arenateamid);
        SetArenaTeamInfoField(arenaSlot, ARENA_TEAM_TYPE, aTeam->GetType());
        SetArenaTeamInfoField(arenaSlot, ARENA_TEAM_MEMBER, (aTeam->GetCaptainGuid() == GetObjectGuid()) ? 0 : 1);
        SetArenaTeamInfoField(arenaSlot, ARENA_TEAM_GAMES_WEEK, played_week);
        SetArenaTeamInfoField(arenaSlot, ARENA_TEAM_GAMES_SEASON, played_season);
        SetArenaTeamInfoField(arenaSlot, ARENA_TEAM_WINS_SEASON, wons_season);
        SetArenaTeamInfoField(arenaSlot, ARENA_TEAM_PERSONAL_RATING, personal_rating);

    } while (result->NextRow());
    delete result;
}

void Player::_LoadEquipmentSets(QueryResult *result)
{
    // SetPQuery(PLAYER_LOGIN_QUERY_LOADEQUIPMENTSETS,   "SELECT setguid, setindex, name, iconname, ignore_mask, item0, item1, item2, item3, item4, item5, item6, item7, item8, item9, item10, item11, item12, item13, item14, item15, item16, item17, item18 FROM character_equipmentsets WHERE guid = '%u' ORDER BY setindex", GUID_LOPART(m_guid));
    if (!result)
        return;

    uint32 count = 0;
    do
    {
        Field *fields = result->Fetch();

        EquipmentSet eqSet;

        eqSet.Guid          = fields[0].GetUInt64();
        uint32 index        = fields[1].GetUInt32();
        eqSet.Name          = fields[2].GetCppString();
        eqSet.IconName      = fields[3].GetCppString();
        eqSet.IgnoreMask    = fields[4].GetUInt32();
        eqSet.state         = EQUIPMENT_SET_UNCHANGED;

        for(uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
            eqSet.Items[i] = fields[5+i].GetUInt32();

        m_EquipmentSets[index] = eqSet;

        ++count;

        if(count >= MAX_EQUIPMENT_SET_INDEX)                // client limit
            break;
    } while (result->NextRow());
    delete result;
}

void Player::_LoadBGData(QueryResult* result)
{
    if (!result)
        return;

    // Expecting only one row
    Field *fields = result->Fetch();
    /* bgInstanceID, bgTeam, x, y, z, o, map, taxi[0], taxi[1], mountSpell */
    m_bgData.bgInstanceID = fields[0].GetUInt32();
    m_bgData.bgTeam       = Team(fields[1].GetUInt32());
    m_bgData.joinPos      = WorldLocation(fields[6].GetUInt32(),    // Map
                                          fields[2].GetFloat(),     // X
                                          fields[3].GetFloat(),     // Y
                                          fields[4].GetFloat(),     // Z
                                          fields[5].GetFloat());    // Orientation
    m_bgData.taxiPath[0]  = fields[7].GetUInt32();
    m_bgData.taxiPath[1]  = fields[8].GetUInt32();
    m_bgData.mountSpell   = fields[9].GetUInt32();

    delete result;
}

bool Player::LoadPositionFromDB(ObjectGuid guid, uint32& mapid, float& x, float& y, float& z, float& o, bool& in_flight)
{
    QueryResult *result = CharacterDatabase.PQuery("SELECT position_x,position_y,position_z,orientation,map,taxi_path FROM characters WHERE guid = '%u'", guid.GetCounter());
    if(!result)
        return false;

    Field *fields = result->Fetch();

    x = fields[0].GetFloat();
    y = fields[1].GetFloat();
    z = fields[2].GetFloat();
    o = fields[3].GetFloat();
    mapid = fields[4].GetUInt32();
    in_flight = !fields[5].GetCppString().empty();

    delete result;
    return true;
}

void Player::_LoadIntoDataField(const char* data, uint32 startOffset, uint32 count)
{
    if(!data)
        return;

    Tokens tokens = StrSplit(data, " ");

    if(tokens.size() != count)
        return;

    Tokens::iterator iter;
    uint32 index;
    for (iter = tokens.begin(), index = 0; index < count; ++iter, ++index)
    {
        m_uint32Values[startOffset + index] = atol((*iter).c_str());
    }
}

bool Player::LoadFromDB(ObjectGuid guid, SqlQueryHolder *holder )
{
    //       0     1        2     3     4      5       6      7   8      9            10            11
    //SELECT guid, account, name, race, class, gender, level, xp, money, playerBytes, playerBytes2, playerFlags,"
    // 12          13          14          15   16           17        18         19         20         21          22           23                 24
    //"position_x, position_y, position_z, map, orientation, taximask, cinematic, totaltime, leveltime, rest_bonus, logout_time, is_logout_resting, resettalents_cost,"
    // 25                 26       27       28       29       30         31           32            33        34    35      36                 37         38
    //"resettalents_time, trans_x, trans_y, trans_z, trans_o, transguid, extra_flags, stable_slots, at_login, zone, online, death_expire_time, taxi_path, dungeon_difficulty,"
    // 39           40                41                42                    43          44          45              46           47               48              49
    //"arenaPoints, totalHonorPoints, todayHonorPoints, yesterdayHonorPoints, totalKills, todayKills, yesterdayKills, chosenTitle, knownCurrencies, watchedFaction, drunk,"
    // 50      51      52      53      54      55      56      57      58         59          60             61              62      63           64
    //"health, power1, power2, power3, power4, power5, power6, power7, specCount, activeSpec, exploredZones, equipmentCache, ammoId, knownTitles, actionBars  FROM characters WHERE guid = '%u'", GUID_LOPART(m_guid));
    QueryResult *result = holder->GetResult(PLAYER_LOGIN_QUERY_LOADFROM);

    if(!result)
    {
        sLog.outError("%s not found in table `characters`, can't load. ", guid.GetString().c_str());
        return false;
    }

    Field *fields = result->Fetch();

    uint32 dbAccountId = fields[1].GetUInt32();

    // check if the character's account in the db and the logged in account match.
    // player should be able to load/delete character only with correct account!
    if( dbAccountId != GetSession()->GetAccountId() )
    {
        sLog.outError("%s loading from wrong account (is: %u, should be: %u)",
            guid.GetString().c_str(), GetSession()->GetAccountId(), dbAccountId);
        delete result;
        return false;
    }

    Object::_Create(guid.GetCounter(), 0, HIGHGUID_PLAYER );

    m_name = fields[2].GetCppString();

    // check name limitations
    if (ObjectMgr::CheckPlayerName(m_name) != CHAR_NAME_SUCCESS ||
        (GetSession()->GetSecurity() == SEC_PLAYER && sObjectMgr.IsReservedName(m_name)))
    {
        delete result;
        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '%u' WHERE guid ='%u'",
            uint32(AT_LOGIN_RENAME), guid.GetCounter());
        return false;
    }

    // overwrite possible wrong/corrupted guid
    SetGuidValue(OBJECT_FIELD_GUID, guid);

    // overwrite some data fields
    SetByteValue(UNIT_FIELD_BYTES_0,0,fields[3].GetUInt8());// race
    SetByteValue(UNIT_FIELD_BYTES_0,1,fields[4].GetUInt8());// class

    uint8 gender = fields[5].GetUInt8() & 0x01;             // allowed only 1 bit values male/female cases (for fit drunk gender part)
    SetByteValue(UNIT_FIELD_BYTES_0,2,gender);              // gender

    SetUInt32Value(UNIT_FIELD_LEVEL, fields[6].GetUInt8());
    SetUInt32Value(PLAYER_XP, fields[7].GetUInt32());

    _LoadIntoDataField(fields[60].GetString(), PLAYER_EXPLORED_ZONES_1, PLAYER_EXPLORED_ZONES_SIZE);
    _LoadIntoDataField(fields[63].GetString(), PLAYER__FIELD_KNOWN_TITLES, KNOWN_TITLES_SIZE*2);

    InitDisplayIds();                                       // model, scale and model data

    SetFloatValue(UNIT_FIELD_HOVERHEIGHT, 1.0f);

    // just load criteria/achievement data, safe call before any load, and need, because some spell/item/quest loading
    // can triggering achievement criteria update that will be lost if this call will later
    m_achievementMgr.LoadFromDB(holder->GetResult(PLAYER_LOGIN_QUERY_LOADACHIEVEMENTS), holder->GetResult(PLAYER_LOGIN_QUERY_LOADCRITERIAPROGRESS));

    uint32 money = fields[8].GetUInt32();
    if(money > MAX_MONEY_AMOUNT)
        money = MAX_MONEY_AMOUNT;
    SetMoney(money);

    SetUInt32Value(PLAYER_BYTES, fields[9].GetUInt32());
    SetUInt32Value(PLAYER_BYTES_2, fields[10].GetUInt32());

    m_drunk = fields[49].GetUInt16();

    SetUInt16Value(PLAYER_BYTES_3, 0, (m_drunk & 0xFFFE) | gender);

    SetUInt32Value(PLAYER_FLAGS, fields[11].GetUInt32());
    SetInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, fields[48].GetInt32());

    SetUInt64Value(PLAYER_FIELD_KNOWN_CURRENCIES, fields[47].GetUInt64());

    SetUInt32Value(PLAYER_AMMO_ID, fields[62].GetUInt32());

    // Action bars state
    SetByteValue(PLAYER_FIELD_BYTES, 2, fields[64].GetUInt8());

    // cleanup inventory related item value fields (its will be filled correctly in _LoadInventory)
    for(uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        SetGuidValue(PLAYER_FIELD_INV_SLOT_HEAD + (slot * 2), ObjectGuid());
        SetVisibleItemSlot(slot, NULL);

        if (m_items[slot])
        {
            delete m_items[slot];
            m_items[slot] = NULL;
        }
    }

    DEBUG_FILTER_LOG(LOG_FILTER_PLAYER_STATS, "Load Basic value of player %s is: ", m_name.c_str());
    outDebugStatsValues();

    //Need to call it to initialize m_team (m_team can be calculated from race)
    //Other way is to saves m_team into characters table.
    setFactionForRace(getRace());
    SetCharm(NULL);

    // load home bind and check in same time class/race pair, it used later for restore broken positions
    if(!_LoadHomeBind(holder->GetResult(PLAYER_LOGIN_QUERY_LOADHOMEBIND)))
    {
        delete result;
        return false;
    }

    InitPrimaryProfessions();                               // to max set before any spell loaded

    // init saved position, and fix it later if problematic
    uint32 transGUID = fields[30].GetUInt32();
    Relocate(fields[12].GetFloat(),fields[13].GetFloat(),fields[14].GetFloat(),fields[16].GetFloat());
    SetLocationMapId(fields[15].GetUInt32());

    uint32 difficulty = fields[38].GetUInt32();
    if(difficulty >= MAX_DUNGEON_DIFFICULTY)
        difficulty = DUNGEON_DIFFICULTY_NORMAL;
    SetDungeonDifficulty(Difficulty(difficulty));           // may be changed in _LoadGroup

    _LoadGroup(holder->GetResult(PLAYER_LOGIN_QUERY_LOADGROUP));

    _LoadArenaTeamInfo(holder->GetResult(PLAYER_LOGIN_QUERY_LOADARENAINFO));

    SetArenaPoints(fields[39].GetUInt32());

    // check arena teams integrity
    for(uint32 arena_slot = 0; arena_slot < MAX_ARENA_SLOT; ++arena_slot)
    {
        uint32 arena_team_id = GetArenaTeamId(arena_slot);
        if (!arena_team_id)
            continue;

        if (ArenaTeam * at = sObjectMgr.GetArenaTeamById(arena_team_id))
            if (at->HaveMember(GetObjectGuid()))
                continue;

        // arena team not exist or not member, cleanup fields
        for(int j = 0; j < ARENA_TEAM_END; ++j)
            SetArenaTeamInfoField(arena_slot, ArenaTeamInfoType(j), 0);
    }

    SetHonorPoints(fields[40].GetUInt32());

    SetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, fields[41].GetUInt32());
    SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, fields[42].GetUInt32());
    SetUInt32Value(PLAYER_FIELD_LIFETIME_HONORBALE_KILLS, fields[43].GetUInt32());
    SetUInt16Value(PLAYER_FIELD_KILLS, 0, fields[44].GetUInt16());
    SetUInt16Value(PLAYER_FIELD_KILLS, 1, fields[45].GetUInt16());

    _LoadBoundInstances(holder->GetResult(PLAYER_LOGIN_QUERY_LOADBOUNDINSTANCES));

    if (!IsPositionValid())
    {
        sLog.outError("%s have invalid coordinates (X: %f Y: %f Z: %f O: %f). Teleport to default race/class locations.",
            guid.GetString().c_str(), GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
        RelocateToHomebind();

        transGUID = 0;

        m_movementInfo.ClearTransportData();
    }

    _LoadBGData(holder->GetResult(PLAYER_LOGIN_QUERY_LOADBGDATA));

    if (m_bgData.bgInstanceID)                              //saved in BattleGround
    {
        BattleGround *currentBg = sBattleGroundMgr.GetBattleGround(m_bgData.bgInstanceID, BATTLEGROUND_TYPE_NONE);

        bool player_at_bg = currentBg && currentBg->IsPlayerInBattleGround(GetObjectGuid());

        if (player_at_bg && currentBg->GetStatus() != STATUS_WAIT_LEAVE)
        {
            BattleGroundQueueTypeId bgQueueTypeId = BattleGroundMgr::BGQueueTypeId(currentBg->GetTypeID(), currentBg->GetArenaType());
            AddBattleGroundQueueId(bgQueueTypeId);

            m_bgData.bgTypeID = currentBg->GetTypeID();     // bg data not marked as modified

            //join player to battleground group
            currentBg->EventPlayerLoggedIn(this, GetObjectGuid());
            currentBg->AddOrSetPlayerToCorrectBgGroup(this, GetObjectGuid(), m_bgData.bgTeam);

            SetInviteForBattleGroundQueueType(bgQueueTypeId,currentBg->GetInstanceID());
        }
        else
        {
            // leave bg
            if (player_at_bg)
                currentBg->RemovePlayerAtLeave(GetObjectGuid(), false, true);

            // move to bg enter point
            const WorldLocation& _loc = GetBattleGroundEntryPoint();
            SetLocationMapId(_loc.mapid);
            Relocate(_loc.coord_x, _loc.coord_y, _loc.coord_z, _loc.orientation);

            // We are not in BG anymore
            SetBattleGroundId(0, BATTLEGROUND_TYPE_NONE);
            // remove outdated DB data in DB
            _SaveBGData();
        }
    }
    else
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(GetMapId());
        // if server restart after player save in BG or area
        // player can have current coordinates in to BG/Arena map, fix this
        if(!mapEntry || mapEntry->IsBattleGroundOrArena())
        {
            const WorldLocation& _loc = GetBattleGroundEntryPoint();
            SetLocationMapId(_loc.mapid);
            Relocate(_loc.coord_x, _loc.coord_y, _loc.coord_z, _loc.orientation);

            // We are not in BG anymore
            SetBattleGroundId(0, BATTLEGROUND_TYPE_NONE);
            // remove outdated DB data in DB
            _SaveBGData();
        }
    }

    if (transGUID != 0)
    {
        m_movementInfo.SetTransportData(ObjectGuid(HIGHGUID_MO_TRANSPORT,transGUID), fields[26].GetFloat(), fields[27].GetFloat(), fields[28].GetFloat(), fields[29].GetFloat(), 0, -1);

        if( !MaNGOS::IsValidMapCoord(
            GetPositionX() + m_movementInfo.GetTransportPos()->x, GetPositionY() + m_movementInfo.GetTransportPos()->y,
            GetPositionZ() + m_movementInfo.GetTransportPos()->z, GetOrientation() + m_movementInfo.GetTransportPos()->o) ||
            // transport size limited
            m_movementInfo.GetTransportPos()->x > 50 || m_movementInfo.GetTransportPos()->y > 50 || m_movementInfo.GetTransportPos()->z > 50 )
        {
            sLog.outError("%s have invalid transport coordinates (X: %f Y: %f Z: %f O: %f). Teleport to default race/class locations.",
                guid.GetString().c_str(), GetPositionX() + m_movementInfo.GetTransportPos()->x, GetPositionY() + m_movementInfo.GetTransportPos()->y,
                GetPositionZ() + m_movementInfo.GetTransportPos()->z, GetOrientation() + m_movementInfo.GetTransportPos()->o);

            RelocateToHomebind();

            m_movementInfo.ClearTransportData();

            transGUID = 0;
        }
    }

    if (transGUID != 0)
    {
        for (MapManager::TransportSet::const_iterator iter = sMapMgr.m_Transports.begin(); iter != sMapMgr.m_Transports.end(); ++iter)
        {
            if( (*iter)->GetGUIDLow() == transGUID)
            {
                MapEntry const* transMapEntry = sMapStore.LookupEntry((*iter)->GetMapId());
                // client without expansion support
                if(GetSession()->Expansion() < transMapEntry->Expansion())
                {
                    DEBUG_LOG("Player %s using client without required expansion tried login at transport at non accessible map %u", GetName(), (*iter)->GetMapId());
                    break;
                }

                m_transport = *iter;
                m_transport->AddPassenger(this);
                SetLocationMapId(m_transport->GetMapId());
                break;
            }
        }

        if(!m_transport)
        {
            sLog.outError("%s have problems with transport guid (%u). Teleport to default race/class locations.",
                guid.GetString().c_str(), transGUID);

            RelocateToHomebind();

            m_movementInfo.ClearTransportData();

            transGUID = 0;
        }
    }
    else                                                    // not transport case
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(GetMapId());
        // client without expansion support
        if(GetSession()->Expansion() < mapEntry->Expansion())
        {
            DEBUG_LOG("Player %s using client without required expansion tried login at non accessible map %u", GetName(), GetMapId());
            RelocateToHomebind();
        }
    }

    // player bounded instance saves loaded in _LoadBoundInstances, group versions at group loading
    DungeonPersistentState* state = GetBoundInstanceSaveForSelfOrGroup(GetMapId());

    // load the player's map here if it's not already loaded
    SetMap(sMapMgr.CreateMap(GetMapId(), this));

    // if the player is in an instance and it has been reset in the meantime teleport him to the entrance
    if(GetInstanceId() && !state)
    {
        AreaTrigger const* at = sObjectMgr.GetMapEntranceTrigger(GetMapId());
        if(at)
            Relocate(at->target_X, at->target_Y, at->target_Z, at->target_Orientation);
        else
            sLog.outError("Player %s(GUID: %u) logged in to a reset instance (map: %u) and there is no area-trigger leading to this map. Thus he can't be ported back to the entrance. This _might_ be an exploit attempt.", GetName(), GetGUIDLow(), GetMapId());
    }

    SaveRecallPosition();

    time_t now = time(NULL);
    time_t logoutTime = time_t(fields[22].GetUInt64());

    // since last logout (in seconds)
    uint32 time_diff = uint32(now - logoutTime);

    // set value, including drunk invisibility detection
    // calculate sobering. after 15 minutes logged out, the player will be sober again
    float soberFactor;
    if(time_diff > 15*MINUTE)
        soberFactor = 0;
    else
        soberFactor = 1-time_diff/(15.0f*MINUTE);
    uint16 newDrunkenValue = uint16(soberFactor* m_drunk);
    SetDrunkValue(newDrunkenValue);

    m_cinematic = fields[18].GetUInt32();
    m_Played_time[PLAYED_TIME_TOTAL]= fields[19].GetUInt32();
    m_Played_time[PLAYED_TIME_LEVEL]= fields[20].GetUInt32();

    m_resetTalentsCost = fields[24].GetUInt32();
    m_resetTalentsTime = time_t(fields[25].GetUInt64());

    // reserve some flags
    uint32 old_safe_flags = GetUInt32Value(PLAYER_FLAGS) & ( PLAYER_FLAGS_HIDE_CLOAK | PLAYER_FLAGS_HIDE_HELM );

    if( HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GM) )
        SetUInt32Value(PLAYER_FLAGS, 0 | old_safe_flags);

    m_taxi.LoadTaxiMask( fields[17].GetString() );          // must be before InitTaxiNodesForLevel

    uint32 extraflags = fields[31].GetUInt32();

    m_stableSlots = fields[32].GetUInt32();
    if(m_stableSlots > MAX_PET_STABLES)
    {
        sLog.outError("Player can have not more %u stable slots, but have in DB %u",MAX_PET_STABLES,uint32(m_stableSlots));
        m_stableSlots = MAX_PET_STABLES;
    }

    m_atLoginFlags = fields[33].GetUInt32();

    // Honor system
    // Update Honor kills data
    m_lastHonorUpdateTime = logoutTime;
    UpdateHonorFields();

    m_deathExpireTime = (time_t)fields[36].GetUInt64();
    if(m_deathExpireTime > now+MAX_DEATH_COUNT*DEATH_EXPIRE_STEP)
        m_deathExpireTime = now+MAX_DEATH_COUNT*DEATH_EXPIRE_STEP-1;

    std::string taxi_nodes = fields[37].GetCppString();

    // clear channel spell data (if saved at channel spell casting)
    SetChannelObjectGuid(ObjectGuid());
    SetUInt32Value(UNIT_CHANNEL_SPELL,0);

    // clear charm/summon related fields
    SetCharm(NULL);
    SetPet(NULL);
    SetTargetGuid(ObjectGuid());
    SetCharmerGuid(ObjectGuid());
    SetOwnerGuid(ObjectGuid());
    SetCreatorGuid(ObjectGuid());

    // reset some aura modifiers before aura apply

    SetGuidValue(PLAYER_FARSIGHT, ObjectGuid());
    SetUInt32Value(PLAYER_TRACK_CREATURES, 0 );
    SetUInt32Value(PLAYER_TRACK_RESOURCES, 0 );

    // cleanup aura list explicitly before skill load where some spells can be applied
    RemoveAllAuras();

    // make sure the unit is considered out of combat for proper loading
    ClearInCombat();

    // make sure the unit is considered not in duel for proper loading
    SetGuidValue(PLAYER_DUEL_ARBITER, ObjectGuid());
    SetUInt32Value(PLAYER_DUEL_TEAM, 0);

    // reset stats before loading any modifiers
    InitStatsForLevel();
    InitGlyphsForLevel();
    InitTaxiNodesForLevel();
    InitRunes();

    // rest bonus can only be calculated after InitStatsForLevel()
    m_rest_bonus = fields[21].GetFloat();

    if (time_diff > 0)
    {
        //speed collect rest bonus in offline, in logout, far from tavern, city (section/in hour)
        float bubble0 = 0.031f;
        //speed collect rest bonus in offline, in logout, in tavern, city (section/in hour)
        float bubble1 = 0.125f;
        float bubble = fields[23].GetUInt32() > 0
            ? bubble1*sWorld.getConfig(CONFIG_FLOAT_RATE_REST_OFFLINE_IN_TAVERN_OR_CITY)
            : bubble0*sWorld.getConfig(CONFIG_FLOAT_RATE_REST_OFFLINE_IN_WILDERNESS);

        SetRestBonus(GetRestBonus()+ time_diff*((float)GetUInt32Value(PLAYER_NEXT_LEVEL_XP)/72000)*bubble);
    }

    // load skills after InitStatsForLevel because it triggering aura apply also
    _LoadSkills(holder->GetResult(PLAYER_LOGIN_QUERY_LOADSKILLS));

    // apply original stats mods before spell loading or item equipment that call before equip _RemoveStatsMods()

    // Mail
    _LoadMails(holder->GetResult(PLAYER_LOGIN_QUERY_LOADMAILS));
    _LoadMailedItems(holder->GetResult(PLAYER_LOGIN_QUERY_LOADMAILEDITEMS));
    UpdateNextMailTimeAndUnreads();

    m_specsCount = fields[58].GetUInt8();
    m_activeSpec = fields[59].GetUInt8();

    _LoadGlyphs(holder->GetResult(PLAYER_LOGIN_QUERY_LOADGLYPHS));

    _LoadAuras(holder->GetResult(PLAYER_LOGIN_QUERY_LOADAURAS), time_diff);
    ApplyGlyphs(true);

    // add ghost flag (must be after aura load: PLAYER_FLAGS_GHOST set in aura)
    if( HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST) )
        m_deathState = DEAD;

    _LoadSpells(holder->GetResult(PLAYER_LOGIN_QUERY_LOADSPELLS));

    // after spell load, learn rewarded spell if need also
    _LoadQuestStatus(holder->GetResult(PLAYER_LOGIN_QUERY_LOADQUESTSTATUS));
    _LoadDailyQuestStatus(holder->GetResult(PLAYER_LOGIN_QUERY_LOADDAILYQUESTSTATUS));
    _LoadWeeklyQuestStatus(holder->GetResult(PLAYER_LOGIN_QUERY_LOADWEEKLYQUESTSTATUS));
    _LoadMonthlyQuestStatus(holder->GetResult(PLAYER_LOGIN_QUERY_LOADMONTHLYQUESTSTATUS));

    _LoadTalents(holder->GetResult(PLAYER_LOGIN_QUERY_LOADTALENTS));

    // after spell and quest load
    InitTalentForLevel();
    learnDefaultSpells();

    // must be before inventory (some items required reputation check)
    m_reputationMgr.LoadFromDB(holder->GetResult(PLAYER_LOGIN_QUERY_LOADREPUTATION));

    _LoadInventory(holder->GetResult(PLAYER_LOGIN_QUERY_LOADINVENTORY), time_diff);
    _LoadItemLoot(holder->GetResult(PLAYER_LOGIN_QUERY_LOADITEMLOOT));

    // update items with duration and realtime
    UpdateItemDuration(time_diff, true);

    _LoadActions(holder->GetResult(PLAYER_LOGIN_QUERY_LOADACTIONS));

    m_social = sSocialMgr.LoadFromDB(holder->GetResult(PLAYER_LOGIN_QUERY_LOADSOCIALLIST), GetObjectGuid());

    // check PLAYER_CHOSEN_TITLE compatibility with PLAYER__FIELD_KNOWN_TITLES
    // note: PLAYER__FIELD_KNOWN_TITLES updated at quest status loaded
    uint32 curTitle = fields[46].GetUInt32();
    if (curTitle && !HasTitle(curTitle))
        curTitle = 0;

    SetUInt32Value(PLAYER_CHOSEN_TITLE, curTitle);

    // Not finish taxi flight path
    if (m_bgData.HasTaxiPath())
    {
        m_taxi.ClearTaxiDestinations();
        for (int i = 0; i < 2; ++i)
            m_taxi.AddTaxiDestination(m_bgData.taxiPath[i]);
    }
    else if (!m_taxi.LoadTaxiDestinationsFromString(taxi_nodes, GetTeam()))
    {
        // problems with taxi path loading
        TaxiNodesEntry const* nodeEntry = NULL;
        if (uint32 node_id = m_taxi.GetTaxiSource())
            nodeEntry = sTaxiNodesStore.LookupEntry(node_id);

        if (!nodeEntry)                                     // don't know taxi start node, to homebind
        {
            sLog.outError("Character %u have wrong data in taxi destination list, teleport to homebind.",GetGUIDLow());
            RelocateToHomebind();
        }
        else                                                // have start node, to it
        {
            sLog.outError("Character %u have too short taxi destination list, teleport to original node.",GetGUIDLow());
            SetLocationMapId(nodeEntry->map_id);
            Relocate(nodeEntry->x, nodeEntry->y, nodeEntry->z,0.0f);
        }

        //we can be relocated from taxi and still have an outdated Map pointer!
        //so we need to get a new Map pointer!
        SetMap(sMapMgr.CreateMap(GetMapId(), this));
        SaveRecallPosition();                           // save as recall also to prevent recall and fall from sky

        m_taxi.ClearTaxiDestinations();
    }

    if(uint32 node_id = m_taxi.GetTaxiSource())
    {
        // save source node as recall coord to prevent recall and fall from sky
        TaxiNodesEntry const* nodeEntry = sTaxiNodesStore.LookupEntry(node_id);
        MANGOS_ASSERT(nodeEntry);                           // checked in m_taxi.LoadTaxiDestinationsFromString
        m_recallMap = nodeEntry->map_id;
        m_recallX = nodeEntry->x;
        m_recallY = nodeEntry->y;
        m_recallZ = nodeEntry->z;

        // flight will started later
    }

    // has to be called after last Relocate() in Player::LoadFromDB
    SetFallInformation(0, GetPositionZ());

    _LoadSpellCooldowns(holder->GetResult(PLAYER_LOGIN_QUERY_LOADSPELLCOOLDOWNS));

    // Spell code allow apply any auras to dead character in load time in aura/spell/item loading
    // Do now before stats re-calculation cleanup for ghost state unexpected auras
    if(!isAlive())
        RemoveAllAurasOnDeath();

    //apply all stat bonuses from items and auras
    SetCanModifyStats(true);
    UpdateAllStats();

    // restore remembered power/health values (but not more max values)
    uint32 savedhealth = fields[50].GetUInt32();
    SetHealth(savedhealth > GetMaxHealth() ? GetMaxHealth() : savedhealth);
    for(uint32 i = 0; i < MAX_POWERS; ++i)
    {
        uint32 savedpower = fields[51+i].GetUInt32();
        SetPower(Powers(i),savedpower > GetMaxPower(Powers(i)) ? GetMaxPower(Powers(i)) : savedpower);
    }

    DEBUG_FILTER_LOG(LOG_FILTER_PLAYER_STATS, "The value of player %s after load item and aura is: ", m_name.c_str());
    outDebugStatsValues();

    // all fields read
    delete result;

    // GM state
    if(GetSession()->GetSecurity() > SEC_PLAYER)
    {
        switch(sWorld.getConfig(CONFIG_UINT32_GM_LOGIN_STATE))
        {
            default:
            case 0:                      break;             // disable
            case 1: SetGameMaster(true); break;             // enable
            case 2:                                         // save state
                if(extraflags & PLAYER_EXTRA_GM_ON)
                    SetGameMaster(true);
                break;
        }

        switch(sWorld.getConfig(CONFIG_UINT32_GM_VISIBLE_STATE))
        {
            default:
            case 0: SetGMVisible(false); break;             // invisible
            case 1:                      break;             // visible
            case 2:                                         // save state
                if(extraflags & PLAYER_EXTRA_GM_INVISIBLE)
                    SetGMVisible(false);
                break;
        }

        switch(sWorld.getConfig(CONFIG_UINT32_GM_ACCEPT_TICKETS))
        {
            default:
            case 0:                        break;           // disable
            case 1: SetAcceptTicket(true); break;           // enable
            case 2:                                         // save state
            if(extraflags & PLAYER_EXTRA_GM_ACCEPT_TICKETS)
                SetAcceptTicket(true);
            break;
        }

        switch(sWorld.getConfig(CONFIG_UINT32_GM_CHAT))
        {
            default:
            case 0:                  break;                 // disable
            case 1: SetGMChat(true); break;                 // enable
            case 2:                                         // save state
                if(extraflags & PLAYER_EXTRA_GM_CHAT)
                    SetGMChat(true);
                break;
        }

        switch(sWorld.getConfig(CONFIG_UINT32_GM_WISPERING_TO))
        {
            default:
            case 0:                          break;         // disable
            case 1: SetAcceptWhispers(true); break;         // enable
            case 2:                                         // save state
                if(extraflags & PLAYER_EXTRA_ACCEPT_WHISPERS)
                    SetAcceptWhispers(true);
                break;
        }
    }

    _LoadDeclinedNames(holder->GetResult(PLAYER_LOGIN_QUERY_LOADDECLINEDNAMES));

    m_achievementMgr.CheckAllAchievementCriteria();

    _LoadEquipmentSets(holder->GetResult(PLAYER_LOGIN_QUERY_LOADEQUIPMENTSETS));

    return true;
}

bool Player::isAllowedToLoot(Creature* creature)
{
    // never tapped by any (mob solo kill)
    if (!creature->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED))
        return false;

    if (Player* recipient = creature->GetLootRecipient())
    {
        if (recipient == this)
            return true;

        if (Group* otherGroup = recipient->GetGroup())
        {
            Group* thisGroup = GetGroup();
            if (!thisGroup)
                return false;

            return thisGroup == otherGroup;
        }
        return false;
    }
    else
        // prevent other players from looting if the recipient got disconnected
        return !creature->HasLootRecipient();
}

void Player::_LoadActions(QueryResult *result)
{
    for(int i = 0; i < MAX_TALENT_SPEC_COUNT; ++i)
        m_actionButtons[i].clear();

    //QueryResult *result = CharacterDatabase.PQuery("SELECT spec, button,action,type FROM character_action WHERE guid = '%u' ORDER BY button",GetGUIDLow());

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();

            uint8 spec = fields[0].GetUInt8();
            uint8 button = fields[1].GetUInt8();
            uint32 action = fields[2].GetUInt32();
            uint8 type = fields[3].GetUInt8();

            if(ActionButton* ab = addActionButton(spec, button, action, type))
                ab->uState = ACTIONBUTTON_UNCHANGED;
            else
            {
                sLog.outError( "  ...at loading, and will deleted in DB also");

                // Will deleted in DB at next save (it can create data until save but marked as deleted)
                m_actionButtons[spec][button].uState = ACTIONBUTTON_DELETED;
            }
        }
        while( result->NextRow() );

        delete result;
    }
}

void Player::_LoadAuras(QueryResult *result, uint32 timediff)
{
    //RemoveAllAuras(); -- some spells casted before aura load, for example in LoadSkills, aura list explicitly cleaned early

    //QueryResult *result = CharacterDatabase.PQuery("SELECT caster_guid,item_guid,spell,stackcount,remaincharges,basepoints0,basepoints1,basepoints2,periodictime0,periodictime1,periodictime2,maxduration,remaintime,effIndexMask FROM character_aura WHERE guid = '%u'",GetGUIDLow());

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();
            ObjectGuid caster_guid = ObjectGuid(fields[0].GetUInt64());
            uint32 item_lowguid = fields[1].GetUInt32();
            uint32 spellid = fields[2].GetUInt32();
            uint32 stackcount = fields[3].GetUInt32();
            uint32 remaincharges = fields[4].GetUInt32();
            int32  damage[MAX_EFFECT_INDEX];
            uint32 periodicTime[MAX_EFFECT_INDEX];

            for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
            {
                damage[i] = fields[i+5].GetInt32();
                periodicTime[i] = fields[i+8].GetUInt32();
            }

            int32 maxduration = fields[11].GetInt32();
            int32 remaintime = fields[12].GetInt32();
            uint32 effIndexMask = fields[13].GetUInt32();

            SpellEntry const* spellproto = sSpellStore.LookupEntry(spellid);
            if (!spellproto)
            {
                sLog.outError("Unknown spell (spellid %u), ignore.",spellid);
                continue;
            }

            if (remaintime != -1 && !IsPositiveSpell(spellproto))
            {
                if (remaintime/IN_MILLISECONDS <= int32(timediff))
                    continue;

                remaintime -= timediff*IN_MILLISECONDS;
            }

            // prevent wrong values of remaincharges
            if (spellproto->procCharges == 0)
                remaincharges = 0;

            if (!spellproto->StackAmount)
                stackcount = 1;
            else if (spellproto->StackAmount < stackcount)
                stackcount = spellproto->StackAmount;
            else if (!stackcount)
                stackcount = 1;

            SpellAuraHolder *holder = CreateSpellAuraHolder(spellproto, this, NULL);
            holder->SetLoadedState(caster_guid, ObjectGuid(HIGHGUID_ITEM, item_lowguid), stackcount, remaincharges, maxduration, remaintime);

            for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
            {
                if ((effIndexMask & (1 << i)) == 0)
                    continue;

                Aura* aura = CreateAura(spellproto, SpellEffectIndex(i), NULL, holder, this);
                if (!damage[i])
                    damage[i] = aura->GetModifier()->m_amount;

                aura->SetLoadedState(damage[i], periodicTime[i]);
                holder->AddAura(aura, SpellEffectIndex(i));
            }

            if (!holder->IsEmptyHolder())
            {
                // reset stolen single target auras
                if (caster_guid != GetObjectGuid() && holder->IsSingleTarget())
                    holder->SetIsSingleTarget(false);

                AddSpellAuraHolder(holder);
                DETAIL_LOG("Added auras from spellid %u", spellproto->Id);
            }
            else
                delete holder;
        }
        while( result->NextRow() );
        delete result;
    }

    if(getClass() == CLASS_WARRIOR && !HasAuraType(SPELL_AURA_MOD_SHAPESHIFT))
        CastSpell(this,SPELL_ID_PASSIVE_BATTLE_STANCE,true);
}

void Player::_LoadGlyphs(QueryResult *result)
{
    if(!result)
        return;

    //         0     1     2
    // "SELECT spec, slot, glyph FROM character_glyphs WHERE guid='%u'"

    do
    {
        Field *fields = result->Fetch();
        uint8 spec = fields[0].GetUInt8();
        uint8 slot = fields[1].GetUInt8();
        uint32 glyph = fields[2].GetUInt32();

        GlyphPropertiesEntry const *gp = sGlyphPropertiesStore.LookupEntry(glyph);
        if(!gp)
        {
            sLog.outError("Player %s has not existing glyph entry %u on index %u, spec %u", m_name.c_str(), glyph, slot, spec);
            continue;
        }

        GlyphSlotEntry const *gs = sGlyphSlotStore.LookupEntry(GetGlyphSlot(slot));
        if (!gs)
        {
            sLog.outError("Player %s has not existing glyph slot entry %u on index %u, spec %u", m_name.c_str(), GetGlyphSlot(slot), slot, spec);
            continue;
        }

        if(gp->TypeFlags != gs->TypeFlags)
        {
            sLog.outError("Player %s has glyph with typeflags %u in slot with typeflags %u, removing.", m_name.c_str(), gp->TypeFlags, gs->TypeFlags);
            continue;
        }

        m_glyphs[spec][slot].id = glyph;

    } while( result->NextRow() );

    delete result;


}

void Player::LoadCorpse()
{
    if( isAlive() )
    {
        sObjectAccessor.ConvertCorpseForPlayer(GetObjectGuid());
    }
    else
    {
        if(Corpse *corpse = GetCorpse())
        {
            ApplyModByteFlag(PLAYER_FIELD_BYTES, 0, PLAYER_FIELD_BYTE_RELEASE_TIMER, corpse && !sMapStore.LookupEntry(corpse->GetMapId())->Instanceable() );
        }
        else
        {
            //Prevent Dead Player login without corpse
            ResurrectPlayer(0.5f);
        }
    }
}

void Player::_LoadInventory(QueryResult *result, uint32 timediff)
{
    //QueryResult *result = CharacterDatabase.PQuery("SELECT data,text,bag,slot,item,item_template FROM character_inventory JOIN item_instance ON character_inventory.item = item_instance.guid WHERE character_inventory.guid = '%u' ORDER BY bag,slot", GetGUIDLow());
    std::map<uint32, Bag*> bagMap;                          // fast guid lookup for bags
    //NOTE: the "order by `bag`" is important because it makes sure
    //the bagMap is filled before items in the bags are loaded
    //NOTE2: the "order by `slot`" is needed because mainhand weapons are (wrongly?)
    //expected to be equipped before offhand items (TODO: fixme)

    uint32 zone = GetZoneId();

    if (result)
    {
        std::list<Item*> problematicItems;

        // prevent items from being added to the queue when stored
        m_itemUpdateQueueBlocked = true;
        do
        {
            Field *fields = result->Fetch();
            uint32 bag_guid  = fields[2].GetUInt32();
            uint8  slot      = fields[3].GetUInt8();
            uint32 item_lowguid = fields[4].GetUInt32();
            uint32 item_id   = fields[5].GetUInt32();

            ItemPrototype const * proto = ObjectMgr::GetItemPrototype(item_id);

            if (!proto)
            {
                CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'", item_lowguid);
                CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid = '%u'", item_lowguid);
                sLog.outError( "Player::_LoadInventory: Player %s has an unknown item (id: #%u) in inventory, deleted.", GetName(),item_id );
                continue;
            }

            Item *item = NewItemOrBag(proto);

            if (!item->LoadFromDB(item_lowguid, fields, GetObjectGuid()))
            {
                sLog.outError( "Player::_LoadInventory: Player %s has broken item (id: #%u) in inventory, deleted.", GetName(),item_id );
                CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'", item_lowguid);
                item->FSetState(ITEM_REMOVED);
                item->SaveToDB();                           // it also deletes item object !
                continue;
            }

            // not allow have in alive state item limited to another map/zone
            if (isAlive() && item->IsLimitedToAnotherMapOrZone(GetMapId(),zone))
            {
                CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'", item_lowguid);
                item->FSetState(ITEM_REMOVED);
                item->SaveToDB();                           // it also deletes item object !
                continue;
            }

            // "Conjured items disappear if you are logged out for more than 15 minutes"
            if (timediff > 15*MINUTE && (item->GetProto()->Flags & ITEM_FLAG_CONJURED))
            {
                CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'", item_lowguid);
                item->FSetState(ITEM_REMOVED);
                item->SaveToDB();                           // it also deletes item object !
                continue;
            }

            bool success = true;

            // the item/bag is not in a bag
            if (!bag_guid)
            {
                item->SetContainer( NULL );
                item->SetSlot(slot);

                if (IsInventoryPos( INVENTORY_SLOT_BAG_0, slot))
                {
                    ItemPosCountVec dest;
                    if (CanStoreItem( INVENTORY_SLOT_BAG_0, slot, dest, item, false) == EQUIP_ERR_OK)
                        item = StoreItem(dest, item, true);
                    else
                        success = false;
                }
                else if (IsEquipmentPos( INVENTORY_SLOT_BAG_0, slot))
                {
                    uint16 dest;
                    if (CanEquipItem( slot, dest, item, false, false ) == EQUIP_ERR_OK)
                        QuickEquipItem(dest, item);
                    else
                        success = false;
                }
                else if (IsBankPos( INVENTORY_SLOT_BAG_0, slot))
                {
                    ItemPosCountVec dest;
                    if (CanBankItem( INVENTORY_SLOT_BAG_0, slot, dest, item, false, false ) == EQUIP_ERR_OK)
                        item = BankItem(dest, item, true);
                    else
                        success = false;
                }

                if (success)
                {
                    // store bags that may contain items in them
                    if (item->IsBag() && IsBagPos(item->GetPos()))
                        bagMap[item_lowguid] = (Bag*)item;
                }
            }
            // the item/bag in a bag
            else
            {
                item->SetSlot(NULL_SLOT);
                // the item is in a bag, find the bag
                std::map<uint32, Bag*>::const_iterator itr = bagMap.find(bag_guid);
                if (itr != bagMap.end() && slot < itr->second->GetBagSize())
                {
                    ItemPosCountVec dest;
                    if (CanStoreItem(itr->second->GetSlot(), slot, dest, item, false) == EQUIP_ERR_OK)
                        item = StoreItem(dest, item, true);
                    else
                        success = false;
                }
                else
                    success = false;
            }

            // item's state may have changed after stored
            if (success)
            {
                item->SetState(ITEM_UNCHANGED, this);

                // restore container unchanged state also
                if (item->GetContainer())
                    item->GetContainer()->SetState(ITEM_UNCHANGED, this);

                // recharged mana gem
                if (timediff > 15*MINUTE && proto->ItemLimitCategory ==ITEM_LIMIT_CATEGORY_MANA_GEM)
                    item->RestoreCharges();
            }
            else
            {
                sLog.outError("Player::_LoadInventory: Player %s has item (GUID: %u Entry: %u) can't be loaded to inventory (Bag GUID: %u Slot: %u) by some reason, will send by mail.", GetName(),item_lowguid, item_id, bag_guid, slot);
                CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'", item_lowguid);
                problematicItems.push_back(item);
            }
        } while (result->NextRow());

        delete result;
        m_itemUpdateQueueBlocked = false;

        // send by mail problematic items
        while(!problematicItems.empty())
        {
            std::string subject = GetSession()->GetMangosString(LANG_NOT_EQUIPPED_ITEM);

            // fill mail
            MailDraft draft(subject, "There's were problems with equipping item(s).");

            for(int i = 0; !problematicItems.empty() && i < MAX_MAIL_ITEMS; ++i)
            {
                Item* item = problematicItems.front();
                problematicItems.pop_front();

                draft.AddItem(item);
            }

            draft.SendMailTo(this, MailSender(this, MAIL_STATIONERY_GM), MAIL_CHECK_MASK_COPIED);
        }
    }

    //if(isAlive())
    _ApplyAllItemMods();
}

void Player::_LoadItemLoot(QueryResult *result)
{
    //QueryResult *result = CharacterDatabase.PQuery("SELECT guid,itemid,amount,suffix,property FROM item_loot WHERE guid = '%u'", GetGUIDLow());

    if (result)
    {
        do
        {
            Field *fields = result->Fetch();
            uint32 item_guid   = fields[0].GetUInt32();

            Item* item = GetItemByGuid(ObjectGuid(HIGHGUID_ITEM, item_guid));

            if (!item)
            {
                CharacterDatabase.PExecute("DELETE FROM item_loot WHERE guid = '%u'", item_guid);
                sLog.outError("Player::_LoadItemLoot: Player %s has loot for nonexistent item (GUID: %u) in `item_loot`, deleted.", GetName(), item_guid );
                continue;
            }

            item->LoadLootFromDB(fields);

        } while (result->NextRow());

        delete result;
    }
}

// load mailed item which should receive current player
void Player::_LoadMailedItems(QueryResult *result)
{
    // data needs to be at first place for Item::LoadFromDB
    //         0     1     2        3          4
    // "SELECT data, text, mail_id, item_guid, item_template FROM mail_items JOIN item_instance ON item_guid = guid WHERE receiver = '%u'", GUID_LOPART(m_guid)
    if(!result)
        return;

    do
    {
        Field *fields = result->Fetch();
        uint32 mail_id       = fields[2].GetUInt32();
        uint32 item_guid_low = fields[3].GetUInt32();
        uint32 item_template = fields[4].GetUInt32();

        Mail* mail = GetMail(mail_id);
        if(!mail)
            continue;
        mail->AddItem(item_guid_low, item_template);

        ItemPrototype const *proto = ObjectMgr::GetItemPrototype(item_template);

        if(!proto)
        {
            sLog.outError( "Player %u has unknown item_template (ProtoType) in mailed items(GUID: %u template: %u) in mail (%u), deleted.", GetGUIDLow(), item_guid_low, item_template,mail->messageID);
            CharacterDatabase.PExecute("DELETE FROM mail_items WHERE item_guid = '%u'", item_guid_low);
            CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid = '%u'", item_guid_low);
            continue;
        }

        Item *item = NewItemOrBag(proto);

        if (!item->LoadFromDB(item_guid_low, fields, GetObjectGuid()))
        {
            sLog.outError( "Player::_LoadMailedItems - Item in mail (%u) doesn't exist !!!! - item guid: %u, deleted from mail", mail->messageID, item_guid_low);
            CharacterDatabase.PExecute("DELETE FROM mail_items WHERE item_guid = '%u'", item_guid_low);
            item->FSetState(ITEM_REMOVED);
            item->SaveToDB();                               // it also deletes item object !
            continue;
        }

        AddMItem(item);
    } while (result->NextRow());

    delete result;
}

void Player::_LoadMails(QueryResult *result)
{
    m_mail.clear();
    //        0  1           2      3        4       5    6           7            8     9   10      11         12             13
    //"SELECT id,messageType,sender,receiver,subject,body,expire_time,deliver_time,money,cod,checked,stationery,mailTemplateId,has_items FROM mail WHERE receiver = '%u' ORDER BY id DESC", GetGUIDLow()
    if(!result)
        return;

    do
    {
        Field *fields = result->Fetch();
        Mail *m = new Mail;
        m->messageID = fields[0].GetUInt32();
        m->messageType = fields[1].GetUInt8();
        m->sender = fields[2].GetUInt32();
        m->receiverGuid = ObjectGuid(HIGHGUID_PLAYER, fields[3].GetUInt32());
        m->subject = fields[4].GetCppString();
        m->body = fields[5].GetCppString();
        m->expire_time = (time_t)fields[6].GetUInt64();
        m->deliver_time = (time_t)fields[7].GetUInt64();
        m->money = fields[8].GetUInt32();
        m->COD = fields[9].GetUInt32();
        m->checked = fields[10].GetUInt32();
        m->stationery = fields[11].GetUInt8();
        m->mailTemplateId = fields[12].GetInt16();
        m->has_items = fields[13].GetBool();                // true, if mail have items or mail have template and items generated (maybe none)

        if (m->mailTemplateId && !sMailTemplateStore.LookupEntry(m->mailTemplateId))
        {
            sLog.outError( "Player::_LoadMail - Mail (%u) have nonexistent MailTemplateId (%u), remove at load", m->messageID, m->mailTemplateId);
            m->mailTemplateId = 0;
        }

        m->state = MAIL_STATE_UNCHANGED;

        m_mail.push_back(m);

        if (m->mailTemplateId && !m->has_items)
            m->prepareTemplateItems(this);

    } while( result->NextRow() );
    delete result;
}

void Player::LoadPet()
{
    // fixme: the pet should still be loaded if the player is not in world
    // just not added to the map
    if(IsInWorld())
    {
        Pet *pet = new Pet;
        if(!pet->LoadPetFromDB(this, 0, 0, true))
            delete pet;
    }
}

void Player::_LoadQuestStatus(QueryResult *result)
{
    mQuestStatus.clear();

    uint32 slot = 0;

    ////                                                     0      1       2         3         4      5          6          7          8          9           10          11          12          13          14
    //QueryResult *result = CharacterDatabase.PQuery("SELECT quest, status, rewarded, explored, timer, mobcount1, mobcount2, mobcount3, mobcount4, itemcount1, itemcount2, itemcount3, itemcount4, itemcount5, itemcount6 FROM character_queststatus WHERE guid = '%u'", GetGUIDLow());

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();

            uint32 quest_id = fields[0].GetUInt32();
                                                            // used to be new, no delete?
            Quest const* pQuest = sObjectMgr.GetQuestTemplate(quest_id);
            if( pQuest )
            {
                // find or create
                QuestStatusData& questStatusData = mQuestStatus[quest_id];

                uint32 qstatus = fields[1].GetUInt32();
                if(qstatus < MAX_QUEST_STATUS)
                    questStatusData.m_status = QuestStatus(qstatus);
                else
                {
                    questStatusData.m_status = QUEST_STATUS_NONE;
                    sLog.outError("Player %s have invalid quest %d status (%d), replaced by QUEST_STATUS_NONE(0).",GetName(),quest_id,qstatus);
                }

                questStatusData.m_rewarded = ( fields[2].GetUInt8() > 0 );
                questStatusData.m_explored = ( fields[3].GetUInt8() > 0 );

                time_t quest_time = time_t(fields[4].GetUInt64());

                if (pQuest->HasSpecialFlag(QUEST_SPECIAL_FLAG_TIMED) && !GetQuestRewardStatus(quest_id) && questStatusData.m_status != QUEST_STATUS_NONE)
                {
                    AddTimedQuest( quest_id );

                    if (quest_time <= sWorld.GetGameTime())
                        questStatusData.m_timer = 1;
                    else
                        questStatusData.m_timer = uint32(quest_time - sWorld.GetGameTime()) * IN_MILLISECONDS;
                }
                else
                    quest_time = 0;

                questStatusData.m_creatureOrGOcount[0] = fields[5].GetUInt32();
                questStatusData.m_creatureOrGOcount[1] = fields[6].GetUInt32();
                questStatusData.m_creatureOrGOcount[2] = fields[7].GetUInt32();
                questStatusData.m_creatureOrGOcount[3] = fields[8].GetUInt32();
                questStatusData.m_itemcount[0] = fields[9].GetUInt32();
                questStatusData.m_itemcount[1] = fields[10].GetUInt32();
                questStatusData.m_itemcount[2] = fields[11].GetUInt32();
                questStatusData.m_itemcount[3] = fields[12].GetUInt32();
                questStatusData.m_itemcount[4] = fields[13].GetUInt32();
                questStatusData.m_itemcount[5] = fields[14].GetUInt32();

                questStatusData.uState = QUEST_UNCHANGED;

                // add to quest log
                if (slot < MAX_QUEST_LOG_SIZE &&
                    ((questStatusData.m_status == QUEST_STATUS_INCOMPLETE ||
                    questStatusData.m_status == QUEST_STATUS_COMPLETE ||
                    questStatusData.m_status == QUEST_STATUS_FAILED) &&
                    (!questStatusData.m_rewarded || pQuest->IsRepeatable())))
                {
                    SetQuestSlot(slot, quest_id, uint32(quest_time));

                    if (questStatusData.m_explored)
                        SetQuestSlotState(slot, QUEST_STATE_COMPLETE);

                    if (questStatusData.m_status == QUEST_STATUS_COMPLETE)
                        SetQuestSlotState(slot, QUEST_STATE_COMPLETE);

                    if (questStatusData.m_status == QUEST_STATUS_FAILED)
                        SetQuestSlotState(slot, QUEST_STATE_FAIL);

                    for(uint8 idx = 0; idx < QUEST_OBJECTIVES_COUNT; ++idx)
                        if(questStatusData.m_creatureOrGOcount[idx])
                            SetQuestSlotCounter(slot, idx, questStatusData.m_creatureOrGOcount[idx]);

                    ++slot;
                }

                if(questStatusData.m_rewarded)
                {
                    // learn rewarded spell if unknown
                    learnQuestRewardedSpells(pQuest);

                    // set rewarded title if any
                    if(pQuest->GetCharTitleId())
                    {
                        if(CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(pQuest->GetCharTitleId()))
                            SetTitle(titleEntry);
                    }

                    if(pQuest->GetBonusTalents())
                        m_questRewardTalentCount += pQuest->GetBonusTalents();
                }

                DEBUG_LOG("Quest status is {%u} for quest {%u} for player (GUID: %u)", questStatusData.m_status, quest_id, GetGUIDLow());
            }
        }
        while( result->NextRow() );

        delete result;
    }

    // clear quest log tail
    for ( uint16 i = slot; i < MAX_QUEST_LOG_SIZE; ++i )
        SetQuestSlot(i, 0);
}

void Player::_LoadDailyQuestStatus(QueryResult *result)
{
    for(uint32 quest_daily_idx = 0; quest_daily_idx < PLAYER_MAX_DAILY_QUESTS; ++quest_daily_idx)
        SetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx,0);

    //QueryResult *result = CharacterDatabase.PQuery("SELECT quest FROM character_queststatus_daily WHERE guid = '%u'", GetGUIDLow());

    if(result)
    {
        uint32 quest_daily_idx = 0;

        do
        {
            if(quest_daily_idx >= PLAYER_MAX_DAILY_QUESTS)  // max amount with exist data in query
            {
                sLog.outError("Player (GUID: %u) have more 25 daily quest records in `charcter_queststatus_daily`",GetGUIDLow());
                break;
            }

            Field *fields = result->Fetch();

            uint32 quest_id = fields[0].GetUInt32();

            Quest const* pQuest = sObjectMgr.GetQuestTemplate(quest_id);
            if( !pQuest )
                continue;

            SetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx,quest_id);
            ++quest_daily_idx;

            DEBUG_LOG("Daily quest {%u} cooldown for player (GUID: %u)", quest_id, GetGUIDLow());
        }
        while( result->NextRow() );

        delete result;
    }

    m_DailyQuestChanged = false;
}

void Player::_LoadWeeklyQuestStatus(QueryResult *result)
{
    m_weeklyquests.clear();

    //QueryResult *result = CharacterDatabase.PQuery("SELECT quest FROM character_queststatus_weekly WHERE guid = '%u'", GetGUIDLow());

    if (result)
    {
        do
        {
            Field *fields = result->Fetch();

            uint32 quest_id = fields[0].GetUInt32();

            Quest const* pQuest = sObjectMgr.GetQuestTemplate(quest_id);
            if (!pQuest)
                continue;

            m_weeklyquests.insert(quest_id);

            DEBUG_LOG("Weekly quest {%u} cooldown for player (GUID: %u)", quest_id, GetGUIDLow());
        }
        while( result->NextRow() );

        delete result;
    }
    m_WeeklyQuestChanged = false;
}

void Player::_LoadMonthlyQuestStatus(QueryResult *result)
{
    m_monthlyquests.clear();

    //QueryResult *result = CharacterDatabase.PQuery("SELECT quest FROM character_queststatus_weekly WHERE guid = '%u'", GetGUIDLow());

    if (result)
    {
        do
        {
            Field *fields = result->Fetch();

            uint32 quest_id = fields[0].GetUInt32();

            Quest const* pQuest = sObjectMgr.GetQuestTemplate(quest_id);
            if (!pQuest)
                continue;

            m_monthlyquests.insert(quest_id);

            DEBUG_LOG("Monthly quest {%u} cooldown for player (GUID: %u)", quest_id, GetGUIDLow());
        }
        while( result->NextRow() );

        delete result;
    }

    m_MonthlyQuestChanged = false;
}

void Player::_LoadSpells(QueryResult *result)
{
    //QueryResult *result = CharacterDatabase.PQuery("SELECT spell,active,disabled FROM character_spell WHERE guid = '%u'",GetGUIDLow());

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();

            uint32 spell_id = fields[0].GetUInt32();

            // skip talents & drop unneeded data
            if(GetTalentSpellPos(spell_id))
            {
                sLog.outError("Player::_LoadSpells: %s has talent spell %u in character_spell, removing it.",
                    GetGuidStr().c_str(), spell_id);
                CharacterDatabase.PExecute("DELETE FROM character_spell WHERE spell = '%u'", spell_id);
                continue;
            }

            addSpell(spell_id, fields[1].GetBool(), false, false, fields[2].GetBool());
        }
        while( result->NextRow() );

        delete result;
    }
}

void Player::_LoadTalents(QueryResult *result)
{
    //QueryResult *result = CharacterDatabase.PQuery("SELECT talent_id, current_rank, spec FROM character_talent WHERE guid = '%u'",GetGUIDLow());
    if (result)
    {
        do
        {
            Field *fields = result->Fetch();

            uint32 talent_id = fields[0].GetUInt32();
            TalentEntry const* talentInfo = sTalentStore.LookupEntry(talent_id);

            if (!talentInfo)
            {
                sLog.outError("Player::_LoadTalents:Player (GUID: %u) has invalid talent_id: %u , this talent will be deleted from character_talent",GetGUIDLow(), talent_id );
                CharacterDatabase.PExecute("DELETE FROM character_talent WHERE talent_id = '%u'", talent_id);
                continue;
            }

            TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);

            if (!talentTabInfo)
            {
                sLog.outError("Player::_LoadTalents:Player (GUID: %u) has invalid talentTabInfo: %u for talentID: %u , this talent will be deleted from character_talent",GetGUIDLow(), talentInfo->TalentTab, talentInfo->TalentID );
                CharacterDatabase.PExecute("DELETE FROM character_talent WHERE talent_id = '%u'", talent_id);
                continue;
            }

            // prevent load talent for different class (cheating)
            if ((getClassMask() & talentTabInfo->ClassMask) == 0)
            {
                sLog.outError("Player::_LoadTalents:Player (GUID: %u) has talent with ClassMask: %u , but Player's ClassMask is: %u , talentID: %u , this talent will be deleted from character_talent",GetGUIDLow(), talentTabInfo->ClassMask, getClassMask() ,talentInfo->TalentID );
                CharacterDatabase.PExecute("DELETE FROM character_talent WHERE guid = '%u' AND talent_id = '%u'", GetGUIDLow(), talent_id);
                continue;
            }

            uint32 currentRank = fields[1].GetUInt32();

            if (currentRank > MAX_TALENT_RANK || talentInfo->RankID[currentRank] == 0)
            {
                sLog.outError("Player::_LoadTalents:Player (GUID: %u) has invalid talent rank: %u , talentID: %u , this talent will be deleted from character_talent",GetGUIDLow(), currentRank, talentInfo->TalentID );
                CharacterDatabase.PExecute("DELETE FROM character_talent WHERE guid = '%u' AND talent_id = '%u'", GetGUIDLow(), talent_id);
                continue;
            }

            uint32 spec = fields[2].GetUInt32();

            if (spec > MAX_TALENT_SPEC_COUNT)
            {
                sLog.outError("Player::_LoadTalents:Player (GUID: %u) has invalid talent spec: %u, spec will be deleted from character_talent", GetGUIDLow(), spec);
                CharacterDatabase.PExecute("DELETE FROM character_talent WHERE spec = '%u' ", spec);
                continue;
            }

            if (spec >= m_specsCount)
            {
                sLog.outError("Player::_LoadTalents:Player (GUID: %u) has invalid talent spec: %u , this spec will be deleted from character_talent.", GetGUIDLow(), spec);
                CharacterDatabase.PExecute("DELETE FROM character_talent WHERE guid = '%u' AND spec = '%u' ", GetGUIDLow(), spec);
                continue;
            }

            if (m_activeSpec == spec)
                addSpell(talentInfo->RankID[currentRank], true,false,false,false);
            else
            {
                PlayerTalent talent;
                talent.currentRank = currentRank;
                talent.talentEntry = talentInfo;
                talent.state       = PLAYERSPELL_UNCHANGED;
                m_talents[spec][talentInfo->TalentID] = talent;
            }
        }
        while (result->NextRow());
        delete result;
    }
}

void Player::_LoadGroup(QueryResult *result)
{
    //QueryResult *result = CharacterDatabase.PQuery("SELECT groupId FROM group_member WHERE memberGuid='%u'", GetGUIDLow());
    if (result)
    {
        uint32 groupId = (*result)[0].GetUInt32();
        delete result;

        if (Group* group = sObjectMgr.GetGroupById(groupId))
        {
            uint8 subgroup = group->GetMemberGroup(GetObjectGuid());
            SetGroup(group, subgroup);
            if (getLevel() >= LEVELREQUIREMENT_HEROIC)
            {
                // the group leader may change the instance difficulty while the player is offline
                SetDungeonDifficulty(group->GetDungeonDifficulty());
                SetRaidDifficulty(group->GetRaidDifficulty());
            }
        }
    }
}

void Player::_LoadBoundInstances(QueryResult *result)
{
    for(uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        m_boundInstances[i].clear();

    Group *group = GetGroup();

    //QueryResult *result = CharacterDatabase.PQuery("SELECT id, permanent, map, difficulty, resettime FROM character_instance LEFT JOIN instance ON instance = id WHERE guid = '%u'", GUID_LOPART(m_guid));
    if(result)
    {
        do
        {
            Field *fields = result->Fetch();
            bool perm = fields[1].GetBool();
            uint32 mapId = fields[2].GetUInt32();
            uint32 instanceId = fields[0].GetUInt32();
            uint8 difficulty = fields[3].GetUInt8();

            time_t resetTime = (time_t)fields[4].GetUInt64();
            // the resettime for normal instances is only saved when the InstanceSave is unloaded
            // so the value read from the DB may be wrong here but only if the InstanceSave is loaded
            // and in that case it is not used

            MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
            if(!mapEntry || !mapEntry->IsDungeon())
            {
                sLog.outError("_LoadBoundInstances: player %s(%d) has bind to nonexistent or not dungeon map %d", GetName(), GetGUIDLow(), mapId);
                CharacterDatabase.PExecute("DELETE FROM character_instance WHERE guid = '%u' AND instance = '%u'", GetGUIDLow(), instanceId);
                continue;
            }

            if(difficulty >= MAX_DIFFICULTY)
            {
                sLog.outError("_LoadBoundInstances: player %s(%d) has bind to nonexistent difficulty %d instance for map %u", GetName(), GetGUIDLow(), difficulty, mapId);
                CharacterDatabase.PExecute("DELETE FROM character_instance WHERE guid = '%u' AND instance = '%u'", GetGUIDLow(), instanceId);
                continue;
            }

            MapDifficulty const* mapDiff = GetMapDifficultyData(mapId,Difficulty(difficulty));
            if(!mapDiff)
            {
                sLog.outError("_LoadBoundInstances: player %s(%d) has bind to nonexistent difficulty %d instance for map %u", GetName(), GetGUIDLow(), difficulty, mapId);
                CharacterDatabase.PExecute("DELETE FROM character_instance WHERE guid = '%u' AND instance = '%u'", GetGUIDLow(), instanceId);
                continue;
            }

            if(!perm && group)
            {
                sLog.outError("_LoadBoundInstances: %s is in group (Id: %d) but has a non-permanent character bind to map %d,%d,%d",
                    GetGuidStr().c_str(), group->GetId(), mapId, instanceId, difficulty);
                CharacterDatabase.PExecute("DELETE FROM character_instance WHERE guid = '%u' AND instance = '%u'",
                    GetGUIDLow(), instanceId);
                continue;
            }

            // since non permanent binds are always solo bind, they can always be reset
            DungeonPersistentState *state = (DungeonPersistentState*)sMapPersistentStateMgr.AddPersistentState(mapEntry, instanceId, Difficulty(difficulty), resetTime, !perm, true);
            if(state) BindToInstance(state, perm, true);
        } while(result->NextRow());
        delete result;
    }
}

InstancePlayerBind* Player::GetBoundInstance(uint32 mapid, Difficulty difficulty)
{
    // some instances only have one difficulty
    MapDifficulty const* mapDiff = GetMapDifficultyData(mapid,difficulty);
    if(!mapDiff)
        return NULL;

    BoundInstancesMap::iterator itr = m_boundInstances[difficulty].find(mapid);
    if(itr != m_boundInstances[difficulty].end())
        return &itr->second;
    else
        return NULL;
}

void Player::UnbindInstance(uint32 mapid, Difficulty difficulty, bool unload)
{
    BoundInstancesMap::iterator itr = m_boundInstances[difficulty].find(mapid);
    UnbindInstance(itr, difficulty, unload);
}

void Player::UnbindInstance(BoundInstancesMap::iterator &itr, Difficulty difficulty, bool unload)
{
    if(itr != m_boundInstances[difficulty].end())
    {
        if (!unload)
            CharacterDatabase.PExecute("DELETE FROM character_instance WHERE guid = '%u' AND instance = '%u'",
                GetGUIDLow(), itr->second.state->GetInstanceId());
        itr->second.state->RemovePlayer(this);              // state can become invalid
        m_boundInstances[difficulty].erase(itr++);
    }
}

InstancePlayerBind* Player::BindToInstance(DungeonPersistentState *state, bool permanent, bool load)
{
    if (state)
    {
        InstancePlayerBind& bind = m_boundInstances[state->GetDifficulty()][state->GetMapId()];
        if (bind.state)
        {
            // update the state when the group kills a boss
            if(permanent != bind.perm || state != bind.state)
                if (!load)
                    CharacterDatabase.PExecute("UPDATE character_instance SET instance = '%u', permanent = '%u' WHERE guid = '%u' AND instance = '%u'",
                        state->GetInstanceId(), permanent, GetGUIDLow(), bind.state->GetInstanceId());
        }
        else
        {
            if (!load)
                CharacterDatabase.PExecute("INSERT INTO character_instance (guid, instance, permanent) VALUES ('%u', '%u', '%u')",
                    GetGUIDLow(), state->GetInstanceId(), permanent);
        }

        if (bind.state != state)
        {
            if (bind.state)
                bind.state->RemovePlayer(this);
            state->AddPlayer(this);
        }

        if (permanent)
            state->SetCanReset(false);

        bind.state = state;
        bind.perm = permanent;
        if (!load)
            DEBUG_LOG("Player::BindToInstance: %s(%d) is now bound to map %d, instance %d, difficulty %d",
                GetName(), GetGUIDLow(), state->GetMapId(), state->GetInstanceId(), state->GetDifficulty());
        return &bind;
    }
    else
        return NULL;
}

DungeonPersistentState* Player::GetBoundInstanceSaveForSelfOrGroup(uint32 mapid)
{
    MapEntry const* mapEntry = sMapStore.LookupEntry(mapid);
    if (!mapEntry)
        return NULL;

    InstancePlayerBind *pBind = GetBoundInstance(mapid, GetDifficulty(mapEntry->IsRaid()));
    DungeonPersistentState *state = pBind ? pBind->state : NULL;

    // the player's permanent player bind is taken into consideration first
    // then the player's group bind and finally the solo bind.
    if (!pBind || !pBind->perm)
    {
        InstanceGroupBind *groupBind = NULL;
        // use the player's difficulty setting (it may not be the same as the group's)
        if (Group *group = GetGroup())
            if (groupBind = group->GetBoundInstance(mapid, this))
                state = groupBind->state;
    }

    return state;
}

void Player::SendRaidInfo()
{
    uint32 counter = 0;

    WorldPacket data(SMSG_RAID_INSTANCE_INFO, 4);

    size_t p_counter = data.wpos();
    data << uint32(counter);                                // placeholder

    time_t now = time(NULL);

    for(int i = 0; i < MAX_DIFFICULTY; ++i)
    {
        for (BoundInstancesMap::const_iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
        {
            if (itr->second.perm)
            {
                DungeonPersistentState *state = itr->second.state;
                data << uint32(state->GetMapId());          // map id
                data << uint32(state->GetDifficulty());     // difficulty
                data << ObjectGuid(state->GetInstanceGuid());// instance guid
                data << uint8(1);                           // expired = 0
                data << uint8(0);                           // extended = 1
                data << uint32(state->GetResetTime() - now);// reset time
                ++counter;
            }
        }
    }
    data.put<uint32>(p_counter, counter);
    GetSession()->SendPacket(&data);
}

/*
- called on every successful teleportation to a map
*/
void Player::SendSavedInstances()
{
    bool hasBeenSaved = false;
    WorldPacket data;

    for(uint8 i = 0; i < MAX_DIFFICULTY; ++i)
    {
        for (BoundInstancesMap::const_iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
        {
            if(itr->second.perm)                                // only permanent binds are sent
            {
                hasBeenSaved = true;
                break;
            }
        }
    }

    //Send opcode 811. true or false means, whether you have current raid/heroic instances
    data.Initialize(SMSG_UPDATE_INSTANCE_OWNERSHIP);
    data << uint32(hasBeenSaved);
    GetSession()->SendPacket(&data);

    if(!hasBeenSaved)
        return;

    for(uint8 i = 0; i < MAX_DIFFICULTY; ++i)
    {
        for (BoundInstancesMap::const_iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
        {
            if(itr->second.perm)
            {
                data.Initialize(SMSG_UPDATE_LAST_INSTANCE);
                data << uint32(itr->second.state->GetMapId());
                GetSession()->SendPacket(&data);
            }
        }
    }
}

/// convert the player's binds to the group
void Player::ConvertInstancesToGroup(Player *player, Group *group, ObjectGuid player_guid)
{
    bool has_binds = false;
    bool has_solo = false;

    if (player)
    {
        player_guid = player->GetObjectGuid();
        if (!group)
            group = player->GetGroup();
    }

    MANGOS_ASSERT(player_guid);

    // copy all binds to the group, when changing leader it's assumed the character
    // will not have any solo binds

    if (player)
    {
        for(uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        {
            for (BoundInstancesMap::iterator itr = player->m_boundInstances[i].begin(); itr != player->m_boundInstances[i].end();)
            {
                has_binds = true;

                if (group)
                    group->BindToInstance(itr->second.state, itr->second.perm, true);

                // permanent binds are not removed
                if (!itr->second.perm)
                {
                    // increments itr in call
                    player->UnbindInstance(itr, Difficulty(i), true);
                    has_solo = true;
                }
                else
                    ++itr;
            }
        }
    }

    uint32 player_lowguid = player_guid.GetCounter();

    // if the player's not online we don't know what binds it has
    if (!player || !group || has_binds)
        CharacterDatabase.PExecute("INSERT INTO group_instance SELECT guid, instance, permanent FROM character_instance WHERE guid = '%u'", player_lowguid);

    // the following should not get executed when changing leaders
    if (!player || has_solo)
        CharacterDatabase.PExecute("DELETE FROM character_instance WHERE guid = '%u' AND permanent = 0", player_lowguid);
}

bool Player::_LoadHomeBind(QueryResult *result)
{
    PlayerInfo const *info = sObjectMgr.GetPlayerInfo(getRace(), getClass());
    if(!info)
    {
        sLog.outError("Player have incorrect race/class pair. Can't be loaded.");
        return false;
    }

    bool ok = false;
    //QueryResult *result = CharacterDatabase.PQuery("SELECT map,zone,position_x,position_y,position_z FROM character_homebind WHERE guid = '%u'", GUID_LOPART(playerGuid));
    if (result)
    {
        Field *fields = result->Fetch();
        m_homebindMapId = fields[0].GetUInt32();
        m_homebindAreaId = fields[1].GetUInt16();
        m_homebindX = fields[2].GetFloat();
        m_homebindY = fields[3].GetFloat();
        m_homebindZ = fields[4].GetFloat();
        delete result;

        MapEntry const* bindMapEntry = sMapStore.LookupEntry(m_homebindMapId);

        // accept saved data only for valid position (and non instanceable), and accessable
        if( MapManager::IsValidMapCoord(m_homebindMapId,m_homebindX,m_homebindY,m_homebindZ) &&
            !bindMapEntry->Instanceable() && GetSession()->Expansion() >= bindMapEntry->Expansion())
        {
            ok = true;
        }
        else
            CharacterDatabase.PExecute("DELETE FROM character_homebind WHERE guid = '%u'", GetGUIDLow());
    }

    if(!ok)
    {
        m_homebindMapId = info->mapId;
        m_homebindAreaId = info->areaId;
        m_homebindX = info->positionX;
        m_homebindY = info->positionY;
        m_homebindZ = info->positionZ;

        CharacterDatabase.PExecute("INSERT INTO character_homebind (guid,map,zone,position_x,position_y,position_z) VALUES ('%u', '%u', '%u', '%f', '%f', '%f')", GetGUIDLow(), m_homebindMapId, (uint32)m_homebindAreaId, m_homebindX, m_homebindY, m_homebindZ);
    }

    DEBUG_LOG("Setting player home position: mapid is: %u, zoneid is %u, X is %f, Y is %f, Z is %f",
        m_homebindMapId, m_homebindAreaId, m_homebindX, m_homebindY, m_homebindZ);

    return true;
}

/*********************************************************/
/***                   SAVE SYSTEM                     ***/
/*********************************************************/

void Player::SaveToDB()
{
    // we should assure this: ASSERT((m_nextSave != sWorld.getConfig(CONFIG_UINT32_INTERVAL_SAVE)));
    // delay auto save at any saves (manual, in code, or autosave)
    m_nextSave = sWorld.getConfig(CONFIG_UINT32_INTERVAL_SAVE);

    //lets allow only players in world to be saved
    if(IsBeingTeleportedFar())
    {
        ScheduleDelayedOperation(DELAYED_SAVE_PLAYER);
        return;
    }

    // first save/honor gain after midnight will also update the player's honor fields
    UpdateHonorFields();

    DEBUG_FILTER_LOG(LOG_FILTER_PLAYER_STATS, "The value of player %s at save: ", m_name.c_str());
    outDebugStatsValues();

    CharacterDatabase.BeginTransaction();

    static SqlStatementID delChar ;
    static SqlStatementID insChar ;

    SqlStatement stmt = CharacterDatabase.CreateStatement(delChar, "DELETE FROM characters WHERE guid = ?");
    stmt.PExecute(GetGUIDLow());

    SqlStatement uberInsert = CharacterDatabase.CreateStatement(insChar, "INSERT INTO characters (guid,account,name,race,class,gender,level,xp,money,playerBytes,playerBytes2,playerFlags,"
        "map, dungeon_difficulty, position_x, position_y, position_z, orientation, "
        "taximask, online, cinematic, "
        "totaltime, leveltime, rest_bonus, logout_time, is_logout_resting, resettalents_cost, resettalents_time, "
        "trans_x, trans_y, trans_z, trans_o, transguid, extra_flags, stable_slots, at_login, zone, "
        "death_expire_time, taxi_path, arenaPoints, totalHonorPoints, todayHonorPoints, yesterdayHonorPoints, totalKills, "
        "todayKills, yesterdayKills, chosenTitle, knownCurrencies, watchedFaction, drunk, health, power1, power2, power3, "
        "power4, power5, power6, power7, specCount, activeSpec, exploredZones, equipmentCache, ammoId, knownTitles, actionBars) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, "
        "?, ?, ?, "
        "?, ?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) ");

    uberInsert.addUInt32(GetGUIDLow());
    uberInsert.addUInt32(GetSession()->GetAccountId());
    uberInsert.addString(m_name);
    uberInsert.addUInt8(getRace());
    uberInsert.addUInt8(getClass());
    uberInsert.addUInt8(getGender());
    uberInsert.addUInt32(getLevel());
    uberInsert.addUInt32(GetUInt32Value(PLAYER_XP));
    uberInsert.addUInt32(GetMoney());
    uberInsert.addUInt32(GetUInt32Value(PLAYER_BYTES));
    uberInsert.addUInt32(GetUInt32Value(PLAYER_BYTES_2));
    uberInsert.addUInt32(GetUInt32Value(PLAYER_FLAGS));

    if(!IsBeingTeleported())
    {
        uberInsert.addUInt32(GetMapId());
        uberInsert.addUInt32(uint32(GetDungeonDifficulty()));
        uberInsert.addFloat(finiteAlways(GetPositionX()));
        uberInsert.addFloat(finiteAlways(GetPositionY()));
        uberInsert.addFloat(finiteAlways(GetPositionZ()));
        uberInsert.addFloat(finiteAlways(GetOrientation()));
    }
    else
    {
        uberInsert.addUInt32(GetTeleportDest().mapid);
        uberInsert.addUInt32(uint32(GetDungeonDifficulty()));
        uberInsert.addFloat(finiteAlways(GetTeleportDest().coord_x));
        uberInsert.addFloat(finiteAlways(GetTeleportDest().coord_y));
        uberInsert.addFloat(finiteAlways(GetTeleportDest().coord_z));
        uberInsert.addFloat(finiteAlways(GetTeleportDest().orientation));
    }

    std::ostringstream ss;
    ss << m_taxi;                                   // string with TaxiMaskSize numbers
    uberInsert.addString(ss);

    uberInsert.addUInt32(IsInWorld() ? 1 : 0);

    uberInsert.addUInt32(m_cinematic);

    uberInsert.addUInt32(m_Played_time[PLAYED_TIME_TOTAL]);
    uberInsert.addUInt32(m_Played_time[PLAYED_TIME_LEVEL]);

    uberInsert.addFloat(finiteAlways(m_rest_bonus));
    uberInsert.addUInt64(uint64(time(NULL)));
    uberInsert.addUInt32(HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING) ? 1 : 0);
                                                            //save, far from tavern/city
                                                            //save, but in tavern/city
    uberInsert.addUInt32(m_resetTalentsCost);
    uberInsert.addUInt64(uint64(m_resetTalentsTime));

    uberInsert.addFloat(finiteAlways(m_movementInfo.GetTransportPos()->x));
    uberInsert.addFloat(finiteAlways(m_movementInfo.GetTransportPos()->y));
    uberInsert.addFloat(finiteAlways(m_movementInfo.GetTransportPos()->z));
    uberInsert.addFloat(finiteAlways(m_movementInfo.GetTransportPos()->o));
    if (m_transport)
        uberInsert.addUInt32(m_transport->GetGUIDLow());
    else
        uberInsert.addUInt32(0);

    uberInsert.addUInt32(m_ExtraFlags);

    uberInsert.addUInt32(uint32(m_stableSlots));                    // to prevent save uint8 as char

    uberInsert.addUInt32(uint32(m_atLoginFlags));

    uberInsert.addUInt32(IsInWorld() ? GetZoneId() : GetCachedZoneId());

    uberInsert.addUInt64(uint64(m_deathExpireTime));

    ss << m_taxi.SaveTaxiDestinationsToString();       //string
    uberInsert.addString(ss);

    uberInsert.addUInt32(GetArenaPoints());

    uberInsert.addUInt32(GetHonorPoints());

    uberInsert.addUInt32(GetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION) );

    uberInsert.addUInt32(GetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION));

    uberInsert.addUInt32(GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORBALE_KILLS));

    uberInsert.addUInt16(GetUInt16Value(PLAYER_FIELD_KILLS, 0));

    uberInsert.addUInt16(GetUInt16Value(PLAYER_FIELD_KILLS, 1));

    uberInsert.addUInt32(GetUInt32Value(PLAYER_CHOSEN_TITLE));

    uberInsert.addUInt64(GetUInt64Value(PLAYER_FIELD_KNOWN_CURRENCIES));

    // FIXME: at this moment send to DB as unsigned, including unit32(-1)
    uberInsert.addUInt32(GetUInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX));

    uberInsert.addUInt16(uint16(GetUInt32Value(PLAYER_BYTES_3) & 0xFFFE));

    uberInsert.addUInt32(GetHealth());

    for(uint32 i = 0; i < MAX_POWERS; ++i)
        uberInsert.addUInt32(GetPower(Powers(i)));

    uberInsert.addUInt32(uint32(m_specsCount));
    uberInsert.addUInt32(uint32(m_activeSpec));

    for(uint32 i = 0; i < PLAYER_EXPLORED_ZONES_SIZE; ++i )         //string
    {
        ss << GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + i) << " ";
    }
    uberInsert.addString(ss);

    for(uint32 i = 0; i < EQUIPMENT_SLOT_END * 2; ++i )             //string
    {
        ss << GetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + i) << " ";
    }
    uberInsert.addString(ss);

    uberInsert.addUInt32(GetUInt32Value(PLAYER_AMMO_ID));

    for(uint32 i = 0; i < KNOWN_TITLES_SIZE*2; ++i )                //string
    {
        ss << GetUInt32Value(PLAYER__FIELD_KNOWN_TITLES + i) << " ";
    }
    uberInsert.addString(ss);

    uberInsert.addUInt32(uint32(GetByteValue(PLAYER_FIELD_BYTES, 2)));

    uberInsert.Execute();

    if (m_mailsUpdated)                                     //save mails only when needed
        _SaveMail();

    _SaveBGData();
    _SaveInventory();
    _SaveQuestStatus();
    _SaveDailyQuestStatus();
    _SaveWeeklyQuestStatus();
    _SaveMonthlyQuestStatus();
    _SaveSpells();
    _SaveSpellCooldowns();
    _SaveActions();
    _SaveAuras();
    _SaveSkills();
    m_achievementMgr.SaveToDB();
    m_reputationMgr.SaveToDB();
    _SaveEquipmentSets();
    GetSession()->SaveTutorialsData();                      // changed only while character in game
    _SaveGlyphs();
    _SaveTalents();

    CharacterDatabase.CommitTransaction();

    // check if stats should only be saved on logout
    // save stats can be out of transaction
    if (m_session->isLogingOut() || !sWorld.getConfig(CONFIG_BOOL_STATS_SAVE_ONLY_ON_LOGOUT))
        _SaveStats();

    // save pet (hunter pet level and experience and all type pets health/mana).
    if (Pet* pet = GetPet())
        pet->SavePetToDB(PET_SAVE_AS_CURRENT);
}

// fast save function for item/money cheating preventing - save only inventory and money state
void Player::SaveInventoryAndGoldToDB()
{
    _SaveInventory();
    SaveGoldToDB();
}

void Player::SaveGoldToDB()
{
    static SqlStatementID updateGold ;

    SqlStatement stmt = CharacterDatabase.CreateStatement(updateGold, "UPDATE characters SET money = ? WHERE guid = ?");
    stmt.PExecute(GetMoney(), GetGUIDLow());
}

void Player::_SaveActions()
{
    static SqlStatementID insertAction ;
    static SqlStatementID updateAction ;
    static SqlStatementID deleteAction ;

    for(int i = 0; i < MAX_TALENT_SPEC_COUNT; ++i)
    {
        for(ActionButtonList::iterator itr = m_actionButtons[i].begin(); itr != m_actionButtons[i].end(); )
        {
            switch (itr->second.uState)
            {
                case ACTIONBUTTON_NEW:
                    {
                        SqlStatement stmt = CharacterDatabase.CreateStatement(insertAction, "INSERT INTO character_action (guid,spec, button,action,type) VALUES (?, ?, ?, ?, ?)");
                        stmt.addUInt32(GetGUIDLow());
                        stmt.addUInt32(i);
                        stmt.addUInt32(uint32(itr->first));
                        stmt.addUInt32(itr->second.GetAction());
                        stmt.addUInt32(uint32(itr->second.GetType()));
                        stmt.Execute();
                        itr->second.uState = ACTIONBUTTON_UNCHANGED;
                        ++itr;
                    }
                    break;
                case ACTIONBUTTON_CHANGED:
                    {
                        SqlStatement stmt = CharacterDatabase.CreateStatement(updateAction, "UPDATE character_action  SET action = ?, type = ? WHERE guid = ? AND button = ? AND spec = ?");
                        stmt.addUInt32(itr->second.GetAction());
                        stmt.addUInt32(uint32(itr->second.GetType()));
                        stmt.addUInt32(GetGUIDLow());
                        stmt.addUInt32(uint32(itr->first));
                        stmt.addUInt32(i);
                        stmt.Execute();
                        itr->second.uState = ACTIONBUTTON_UNCHANGED;
                        ++itr;
                    }
                    break;
                case ACTIONBUTTON_DELETED:
                    {
                        SqlStatement stmt = CharacterDatabase.CreateStatement(deleteAction, "DELETE FROM character_action WHERE guid = ? AND button = ? AND spec = ?");
                        stmt.addUInt32(GetGUIDLow());
                        stmt.addUInt32(uint32(itr->first));
                        stmt.addUInt32(i);
                        stmt.Execute();
                        m_actionButtons[i].erase(itr++);
                    }
                    break;
                default:
                    ++itr;
                    break;
            }
        }
    }
}

void Player::_SaveAuras()
{
    static SqlStatementID deleteAuras ;
    static SqlStatementID insertAuras ;

    SqlStatement stmt = CharacterDatabase.CreateStatement(deleteAuras, "DELETE FROM character_aura WHERE guid = ?");
    stmt.PExecute(GetGUIDLow());

    SpellAuraHolderMap const& auraHolders = GetSpellAuraHolderMap();

    if (auraHolders.empty())
        return;

    stmt = CharacterDatabase.CreateStatement(insertAuras, "INSERT INTO character_aura (guid, caster_guid, item_guid, spell, stackcount, remaincharges, "
        "basepoints0, basepoints1, basepoints2, periodictime0, periodictime1, periodictime2, maxduration, remaintime, effIndexMask) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    for(SpellAuraHolderMap::const_iterator itr = auraHolders.begin(); itr != auraHolders.end(); ++itr)
    {
        SpellAuraHolder *holder = itr->second;
        //skip all holders from spells that are passive or channeled
        //do not save single target holders (unless they were cast by the player)
        if (!holder->IsPassive() && !IsChanneledSpell(holder->GetSpellProto()) && (holder->GetCasterGuid() == GetObjectGuid() || !holder->IsSingleTarget()))
        {
            int32  damage[MAX_EFFECT_INDEX];
            uint32 periodicTime[MAX_EFFECT_INDEX];
            uint32 effIndexMask = 0;

            for (uint32 i = 0; i < MAX_EFFECT_INDEX; ++i)
            {
                damage[i] = 0;
                periodicTime[i] = 0;

                if (Aura *aur = holder->GetAuraByEffectIndex(SpellEffectIndex(i)))
                {
                    // don't save not own area auras
                    if (aur->IsAreaAura() && holder->GetCasterGuid() != GetObjectGuid())
                        continue;

                    damage[i] = aur->GetModifier()->m_amount;
                    periodicTime[i] = aur->GetModifier()->periodictime;
                    effIndexMask |= (1 << i);
                }
            }

            if (!effIndexMask)
                continue;

            stmt.addUInt32(GetGUIDLow());
            stmt.addUInt64(holder->GetCasterGuid().GetRawValue());
            stmt.addUInt32(holder->GetCastItemGuid().GetCounter());
            stmt.addUInt32(holder->GetId());
            stmt.addUInt32(holder->GetStackAmount());
            stmt.addUInt8(holder->GetAuraCharges());

            for (uint32 i = 0; i < MAX_EFFECT_INDEX; ++i)
                stmt.addInt32(damage[i]);

            for (uint32 i = 0; i < MAX_EFFECT_INDEX; ++i)
                stmt.addUInt32(periodicTime[i]);

            stmt.addInt32(holder->GetAuraMaxDuration());
            stmt.addInt32(holder->GetAuraDuration());
            stmt.addUInt32(effIndexMask);
            stmt.Execute();
        }
    }
}

void Player::_SaveGlyphs()
{
    static SqlStatementID insertGlyph ;
    static SqlStatementID updateGlyph ;
    static SqlStatementID deleteGlyph ;

    for (uint8 spec = 0; spec < m_specsCount; ++spec)
    {
        for (uint8 slot = 0; slot < MAX_GLYPH_SLOT_INDEX; ++slot)
        {
            switch(m_glyphs[spec][slot].uState)
            {
                case GLYPH_NEW:
                    {
                        SqlStatement stmt = CharacterDatabase.CreateStatement(insertGlyph, "INSERT INTO character_glyphs (guid, spec, slot, glyph) VALUES (?, ?, ?, ?)");
                        stmt.PExecute(GetGUIDLow(), spec, slot, m_glyphs[spec][slot].GetId());
                    }
                    break;
                case GLYPH_CHANGED:
                    {
                        SqlStatement stmt = CharacterDatabase.CreateStatement(updateGlyph, "UPDATE character_glyphs SET glyph = ? WHERE guid = ? AND spec = ? AND slot = ?");
                        stmt.PExecute(m_glyphs[spec][slot].GetId(), GetGUIDLow(), spec, slot);
                    }
                    break;
                case GLYPH_DELETED:
                    {
                        SqlStatement stmt = CharacterDatabase.CreateStatement(deleteGlyph, "DELETE FROM character_glyphs WHERE guid = ? AND spec = ? AND slot = ?");
                        stmt.PExecute(GetGUIDLow(), spec, slot);
                    }
                    break;
                case GLYPH_UNCHANGED:
                    break;
            }
            m_glyphs[spec][slot].uState = GLYPH_UNCHANGED;
        }
    }
}

void Player::_SaveInventory()
{
    // force items in buyback slots to new state
    // and remove those that aren't already
    for (uint8 i = BUYBACK_SLOT_START; i < BUYBACK_SLOT_END; ++i)
    {
        Item *item = m_items[i];
        if (!item || item->GetState() == ITEM_NEW) continue;

        static SqlStatementID delInv ;
        static SqlStatementID delItemInst ;

        SqlStatement stmt = CharacterDatabase.CreateStatement(delInv, "DELETE FROM character_inventory WHERE item = ?");
        stmt.PExecute(item->GetGUIDLow());

        stmt = CharacterDatabase.CreateStatement(delItemInst, "DELETE FROM item_instance WHERE guid = ?");
        stmt.PExecute(item->GetGUIDLow());

        m_items[i]->FSetState(ITEM_NEW);
    }

    // update enchantment durations
    for(EnchantDurationList::const_iterator itr = m_enchantDuration.begin();itr != m_enchantDuration.end();++itr)
    {
        itr->item->SetEnchantmentDuration(itr->slot,itr->leftduration);
    }

    // if no changes
    if (m_itemUpdateQueue.empty()) return;

    // do not save if the update queue is corrupt
    bool error = false;
    for(size_t i = 0; i < m_itemUpdateQueue.size(); ++i)
    {
        Item *item = m_itemUpdateQueue[i];
        if(!item || item->GetState() == ITEM_REMOVED) continue;
        Item *test = GetItemByPos( item->GetBagSlot(), item->GetSlot());

        if (test == NULL)
        {
            sLog.outError("Player(GUID: %u Name: %s)::_SaveInventory - the bag(%d) and slot(%d) values for the item with guid %d are incorrect, the player doesn't have an item at that position!", GetGUIDLow(), GetName(), item->GetBagSlot(), item->GetSlot(), item->GetGUIDLow());
            error = true;
        }
        else if (test != item)
        {
            sLog.outError("Player(GUID: %u Name: %s)::_SaveInventory - the bag(%d) and slot(%d) values for the item with guid %d are incorrect, the item with guid %d is there instead!", GetGUIDLow(), GetName(), item->GetBagSlot(), item->GetSlot(), item->GetGUIDLow(), test->GetGUIDLow());
            error = true;
        }
    }

    if (error)
    {
        sLog.outError("Player::_SaveInventory - one or more errors occurred save aborted!");
        ChatHandler(this).SendSysMessage(LANG_ITEM_SAVE_FAILED);
        return;
    }

    static SqlStatementID insertInventory ;
    static SqlStatementID updateInventory ;
    static SqlStatementID deleteInventory ;

    for(size_t i = 0; i < m_itemUpdateQueue.size(); ++i)
    {
        Item *item = m_itemUpdateQueue[i];
        if(!item) continue;

        Bag *container = item->GetContainer();
        uint32 bag_guid = container ? container->GetGUIDLow() : 0;

        switch(item->GetState())
        {
            case ITEM_NEW:
                {
                    SqlStatement stmt = CharacterDatabase.CreateStatement(insertInventory, "INSERT INTO character_inventory (guid,bag,slot,item,item_template) VALUES (?, ?, ?, ?, ?)");
                    stmt.addUInt32(GetGUIDLow());
                    stmt.addUInt32(bag_guid);
                    stmt.addUInt8(item->GetSlot());
                    stmt.addUInt32(item->GetGUIDLow());
                    stmt.addUInt32(item->GetEntry());
                    stmt.Execute();
                }
                break;
            case ITEM_CHANGED:
                {
                    SqlStatement stmt = CharacterDatabase.CreateStatement(updateInventory, "UPDATE character_inventory SET guid = ?, bag = ?, slot = ?, item_template = ? WHERE item = ?");
                    stmt.addUInt32(GetGUIDLow());
                    stmt.addUInt32(bag_guid);
                    stmt.addUInt8(item->GetSlot());
                    stmt.addUInt32(item->GetEntry());
                    stmt.addUInt32(item->GetGUIDLow());
                    stmt.Execute();
                }
                break;
            case ITEM_REMOVED:
                {
                    SqlStatement stmt = CharacterDatabase.CreateStatement(deleteInventory, "DELETE FROM character_inventory WHERE item = ?");
                    stmt.PExecute(item->GetGUIDLow());
                }
                break;
            case ITEM_UNCHANGED:
                break;
        }

        item->SaveToDB();                                   // item have unchanged inventory record and can be save standalone
    }
    m_itemUpdateQueue.clear();
}

void Player::_SaveMail()
{
    static SqlStatementID updateMail ;
    static SqlStatementID deleteMailItems ;

    static SqlStatementID deleteItem ;
    static SqlStatementID deleteMain ;
    static SqlStatementID deleteItems ;

    for (PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        Mail *m = (*itr);
        if (m->state == MAIL_STATE_CHANGED)
        {
            SqlStatement stmt = CharacterDatabase.CreateStatement(updateMail, "UPDATE mail SET has_items = ?, expire_time = ?, deliver_time = ?, money = ?, cod = ?, checked = ? WHERE id = ?");
            stmt.addUInt32(m->HasItems() ? 1 : 0);
            stmt.addUInt64(uint64(m->expire_time));
            stmt.addUInt64(uint64(m->deliver_time));
            stmt.addUInt32(m->money);
            stmt.addUInt32(m->COD);
            stmt.addUInt32(m->checked);
            stmt.addUInt32(m->messageID);
            stmt.Execute();

            if(m->removedItems.size())
            {
                stmt = CharacterDatabase.CreateStatement(deleteMailItems, "DELETE FROM mail_items WHERE item_guid = ?");

                for(std::vector<uint32>::const_iterator itr2 = m->removedItems.begin(); itr2 != m->removedItems.end(); ++itr2)
                    stmt.PExecute(*itr2);

                m->removedItems.clear();
            }
            m->state = MAIL_STATE_UNCHANGED;
        }
        else if (m->state == MAIL_STATE_DELETED)
        {
            if (m->HasItems())
            {
                SqlStatement stmt = CharacterDatabase.CreateStatement(deleteItem, "DELETE FROM item_instance WHERE guid = ?");
                for(MailItemInfoVec::const_iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
                    stmt.PExecute(itr2->item_guid);
            }

            SqlStatement stmt = CharacterDatabase.CreateStatement(deleteMain, "DELETE FROM mail WHERE id = ?");
            stmt.PExecute(m->messageID);

            stmt = CharacterDatabase.CreateStatement(deleteItems, "DELETE FROM mail_items WHERE mail_id = ?");
            stmt.PExecute(m->messageID);
        }
    }

    //deallocate deleted mails...
    for (PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); )
    {
        if ((*itr)->state == MAIL_STATE_DELETED)
        {
            Mail* m = *itr;
            m_mail.erase(itr);
            delete m;
            itr = m_mail.begin();
        }
        else
            ++itr;
    }

    m_mailsUpdated = false;
}

void Player::_SaveQuestStatus()
{
    static SqlStatementID insertQuestStatus ;

    static SqlStatementID updateQuestStatus ;

    // we don't need transactions here.
    for( QuestStatusMap::iterator i = mQuestStatus.begin( ); i != mQuestStatus.end( ); ++i )
    {
        switch (i->second.uState)
        {
            case QUEST_NEW :
                {
                    SqlStatement stmt = CharacterDatabase.CreateStatement(insertQuestStatus, "INSERT INTO character_queststatus (guid,quest,status,rewarded,explored,timer,mobcount1,mobcount2,mobcount3,mobcount4,itemcount1,itemcount2,itemcount3,itemcount4,itemcount5,itemcount6) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

                    stmt.addUInt32(GetGUIDLow());
                    stmt.addUInt32(i->first);
                    stmt.addUInt8(i->second.m_status);
                    stmt.addUInt8(i->second.m_rewarded);
                    stmt.addUInt8(i->second.m_explored);
                    stmt.addUInt64(uint64(i->second.m_timer / IN_MILLISECONDS+ sWorld.GetGameTime()));
                    for (int k = 0; k < QUEST_OBJECTIVES_COUNT; ++k)
                        stmt.addUInt32(i->second.m_creatureOrGOcount[k]);
                    for (int k = 0; k < QUEST_ITEM_OBJECTIVES_COUNT; ++k)
                        stmt.addUInt32(i->second.m_itemcount[k]);
                    stmt.Execute();
                }
                break;
            case QUEST_CHANGED :
                {
                    SqlStatement stmt = CharacterDatabase.CreateStatement(updateQuestStatus, "UPDATE character_queststatus SET status = ?,rewarded = ?,explored = ?,timer = ?,"
                        "mobcount1 = ?,mobcount2 = ?,mobcount3 = ?,mobcount4 = ?,itemcount1 = ?,itemcount2 = ?,itemcount3 = ?,itemcount4 = ?,itemcount5 = ?,itemcount6 = ? WHERE guid = ? AND quest = ?");

                    stmt.addUInt8(i->second.m_status);
                    stmt.addUInt8(i->second.m_rewarded);
                    stmt.addUInt8(i->second.m_explored);
                    stmt.addUInt64(uint64(i->second.m_timer / IN_MILLISECONDS + sWorld.GetGameTime()));
                    for (int k = 0; k < QUEST_OBJECTIVES_COUNT; ++k)
                        stmt.addUInt32(i->second.m_creatureOrGOcount[k]);
                    for (int k = 0; k < QUEST_ITEM_OBJECTIVES_COUNT; ++k)
                        stmt.addUInt32(i->second.m_itemcount[k]);
                    stmt.addUInt32(GetGUIDLow());
                    stmt.addUInt32(i->first);
                    stmt.Execute();
                }
                break;
            case QUEST_UNCHANGED:
                break;
        };
        i->second.uState = QUEST_UNCHANGED;
    }
}

void Player::_SaveDailyQuestStatus()
{
    if (!m_DailyQuestChanged)
        return;

    // we don't need transactions here.
    static SqlStatementID delQuestStatus ;
    static SqlStatementID insQuestStatus ;

    SqlStatement stmtDel = CharacterDatabase.CreateStatement(delQuestStatus, "DELETE FROM character_queststatus_daily WHERE guid = ?");
    SqlStatement stmtIns = CharacterDatabase.CreateStatement(insQuestStatus, "INSERT INTO character_queststatus_daily (guid,quest) VALUES (?, ?)");

    stmtDel.PExecute(GetGUIDLow());

    for(uint32 quest_daily_idx = 0; quest_daily_idx < PLAYER_MAX_DAILY_QUESTS; ++quest_daily_idx)
        if (GetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx))
            stmtIns.PExecute(GetGUIDLow(), GetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx));

    m_DailyQuestChanged = false;
}

void Player::_SaveWeeklyQuestStatus()
{
    if (!m_WeeklyQuestChanged || m_weeklyquests.empty())
        return;

    // we don't need transactions here.
    static SqlStatementID delQuestStatus ;
    static SqlStatementID insQuestStatus  ;

    SqlStatement stmtDel = CharacterDatabase.CreateStatement(delQuestStatus, "DELETE FROM character_queststatus_weekly WHERE guid = ?");
    SqlStatement stmtIns =  CharacterDatabase.CreateStatement(insQuestStatus, "INSERT INTO character_queststatus_weekly (guid,quest) VALUES (?, ?)");

    stmtDel.PExecute(GetGUIDLow());

    for (QuestSet::const_iterator iter = m_weeklyquests.begin(); iter != m_weeklyquests.end(); ++iter)
    {
        uint32 quest_id  = *iter;
        stmtIns.PExecute(GetGUIDLow(), quest_id);
    }

    m_WeeklyQuestChanged = false;
}

void Player::_SaveMonthlyQuestStatus()
{
    if (!m_MonthlyQuestChanged || m_monthlyquests.empty())
        return;

    // we don't need transactions here.
    static SqlStatementID deleteQuest ;
    static SqlStatementID insertQuest ;

    SqlStatement stmtDel = CharacterDatabase.CreateStatement(deleteQuest, "DELETE FROM character_queststatus_monthly WHERE guid = ?");
    SqlStatement stmtIns = CharacterDatabase.CreateStatement(insertQuest, "INSERT INTO character_queststatus_monthly (guid, quest) VALUES (?, ?)");

    stmtDel.PExecute(GetGUIDLow());

    for (QuestSet::const_iterator iter = m_monthlyquests.begin(); iter != m_monthlyquests.end(); ++iter)
    {
        uint32 quest_id = *iter;
        stmtIns.PExecute(GetGUIDLow(), quest_id);
    }

    m_MonthlyQuestChanged = false;
}

void Player::_SaveSkills()
{
    static SqlStatementID delSkills ;
    static SqlStatementID insSkills ;
    static SqlStatementID updSkills ;

    // we don't need transactions here.
    for( SkillStatusMap::iterator itr = mSkillStatus.begin(); itr != mSkillStatus.end(); )
    {
        if(itr->second.uState == SKILL_UNCHANGED)
        {
            ++itr;
            continue;
        }

        if(itr->second.uState == SKILL_DELETED)
        {
            SqlStatement stmt = CharacterDatabase.CreateStatement(delSkills, "DELETE FROM character_skills WHERE guid = ? AND skill = ?");
            stmt.PExecute(GetGUIDLow(), itr->first );
            mSkillStatus.erase(itr++);
            continue;
        }

        uint32 valueData = GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(itr->second.pos));
        uint16 value = SKILL_VALUE(valueData);
        uint16 max = SKILL_MAX(valueData);

        switch (itr->second.uState)
        {
            case SKILL_NEW:
                {
                    SqlStatement stmt = CharacterDatabase.CreateStatement(insSkills, "INSERT INTO character_skills (guid, skill, value, max) VALUES (?, ?, ?, ?)");
                    stmt.PExecute(GetGUIDLow(), itr->first, value, max);
                }
                break;
            case SKILL_CHANGED:
                {
                    SqlStatement stmt = CharacterDatabase.CreateStatement(updSkills, "UPDATE character_skills SET value = ?, max = ? WHERE guid = ? AND skill = ?");
                    stmt.PExecute(value, max, GetGUIDLow(), itr->first );
                }
                break;
            case SKILL_UNCHANGED:
            case SKILL_DELETED:
                MANGOS_ASSERT(false);
                break;
        };
        itr->second.uState = SKILL_UNCHANGED;

        ++itr;
    }
}

void Player::_SaveSpells()
{
    static SqlStatementID delSpells ;
    static SqlStatementID insSpells ;

    SqlStatement stmtDel = CharacterDatabase.CreateStatement(delSpells, "DELETE FROM character_spell WHERE guid = ? and spell = ?");
    SqlStatement stmtIns = CharacterDatabase.CreateStatement(insSpells, "INSERT INTO character_spell (guid,spell,active,disabled) VALUES (?, ?, ?, ?)");

    for (PlayerSpellMap::iterator itr = m_spells.begin(), next = m_spells.begin(); itr != m_spells.end();)
    {
        uint32 talentCosts = GetTalentSpellCost(itr->first);

        if (!talentCosts)
        {
            if (itr->second.state == PLAYERSPELL_REMOVED || itr->second.state == PLAYERSPELL_CHANGED)
                stmtDel.PExecute(GetGUIDLow(), itr->first);

            // add only changed/new not dependent spells
            if (!itr->second.dependent && (itr->second.state == PLAYERSPELL_NEW || itr->second.state == PLAYERSPELL_CHANGED))
                stmtIns.PExecute(GetGUIDLow(), itr->first, uint8(itr->second.active ? 1 : 0), uint8(itr->second.disabled ? 1 : 0));
        }

        if (itr->second.state == PLAYERSPELL_REMOVED)
            m_spells.erase(itr++);
        else
        {
            itr->second.state = PLAYERSPELL_UNCHANGED;
            ++itr;
        }

    }
}

void Player::_SaveTalents()
{
    static SqlStatementID delTalents ;
    static SqlStatementID insTalents ;

    SqlStatement stmtDel = CharacterDatabase.CreateStatement(delTalents, "DELETE FROM character_talent WHERE guid = ? and talent_id = ? and spec = ?");
    SqlStatement stmtIns = CharacterDatabase.CreateStatement(insTalents, "INSERT INTO character_talent (guid, talent_id, current_rank , spec) VALUES (?, ?, ?, ?)");

    for (uint32 i = 0; i < MAX_TALENT_SPEC_COUNT; ++i)
    {
        for (PlayerTalentMap::iterator itr = m_talents[i].begin(); itr != m_talents[i].end();)
        {
            if (itr->second.state == PLAYERSPELL_REMOVED || itr->second.state == PLAYERSPELL_CHANGED)
                stmtDel.PExecute(GetGUIDLow(),itr->first, i);

            // add only changed/new talents
            if (itr->second.state == PLAYERSPELL_NEW || itr->second.state == PLAYERSPELL_CHANGED)
                stmtIns.PExecute(GetGUIDLow(), itr->first, itr->second.currentRank, i);

            if (itr->second.state == PLAYERSPELL_REMOVED)
                m_talents[i].erase(itr++);
            else
            {
                itr->second.state = PLAYERSPELL_UNCHANGED;
                ++itr;
            }
        }
    }
}

// save player stats -- only for external usage
// real stats will be recalculated on player login
void Player::_SaveStats()
{
    // check if stat saving is enabled and if char level is high enough
    if(!sWorld.getConfig(CONFIG_UINT32_MIN_LEVEL_STAT_SAVE) || getLevel() < sWorld.getConfig(CONFIG_UINT32_MIN_LEVEL_STAT_SAVE))
        return;

    static SqlStatementID delStats ;
    static SqlStatementID insertStats ;

    SqlStatement stmt = CharacterDatabase.CreateStatement(delStats, "DELETE FROM character_stats WHERE guid = ?");
    stmt.PExecute(GetGUIDLow());

    stmt = CharacterDatabase.CreateStatement(insertStats, "INSERT INTO character_stats (guid, maxhealth, maxpower1, maxpower2, maxpower3, maxpower4, maxpower5, maxpower6, maxpower7, "
        "strength, agility, stamina, intellect, spirit, armor, resHoly, resFire, resNature, resFrost, resShadow, resArcane, "
        "blockPct, dodgePct, parryPct, critPct, rangedCritPct, spellCritPct, attackPower, rangedAttackPower, spellPower) "
        "VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    stmt.addUInt32(GetGUIDLow());
    stmt.addUInt32(GetMaxHealth());
    for(int i = 0; i < MAX_POWERS; ++i)
        stmt.addUInt32(GetMaxPower(Powers(i)));
    for(int i = 0; i < MAX_STATS; ++i)
        stmt.addFloat(GetStat(Stats(i)));
    // armor + school resistances
    for(int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        stmt.addUInt32(GetResistance(SpellSchools(i)));
    stmt.addFloat(GetFloatValue(PLAYER_BLOCK_PERCENTAGE));
    stmt.addFloat(GetFloatValue(PLAYER_DODGE_PERCENTAGE));
    stmt.addFloat(GetFloatValue(PLAYER_PARRY_PERCENTAGE));
    stmt.addFloat(GetFloatValue(PLAYER_CRIT_PERCENTAGE));
    stmt.addFloat(GetFloatValue(PLAYER_RANGED_CRIT_PERCENTAGE));
    stmt.addFloat(GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1));
    stmt.addUInt32(GetUInt32Value(UNIT_FIELD_ATTACK_POWER));
    stmt.addUInt32(GetUInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER));
    stmt.addUInt32(GetBaseSpellPowerBonus());

    stmt.Execute();
}

void Player::outDebugStatsValues() const
{
    // optimize disabled debug output
    if(!sLog.HasLogLevelOrHigher(LOG_LVL_DEBUG) || sLog.HasLogFilter(LOG_FILTER_PLAYER_STATS))
        return;

    sLog.outDebug("HP is: \t\t\t%u\t\tMP is: \t\t\t%u",GetMaxHealth(), GetMaxPower(POWER_MANA));
    sLog.outDebug("AGILITY is: \t\t%f\t\tSTRENGTH is: \t\t%f",GetStat(STAT_AGILITY), GetStat(STAT_STRENGTH));
    sLog.outDebug("INTELLECT is: \t\t%f\t\tSPIRIT is: \t\t%f",GetStat(STAT_INTELLECT), GetStat(STAT_SPIRIT));
    sLog.outDebug("STAMINA is: \t\t%f",GetStat(STAT_STAMINA));
    sLog.outDebug("Armor is: \t\t%u\t\tBlock is: \t\t%f",GetArmor(), GetFloatValue(PLAYER_BLOCK_PERCENTAGE));
    sLog.outDebug("HolyRes is: \t\t%u\t\tFireRes is: \t\t%u",GetResistance(SPELL_SCHOOL_HOLY), GetResistance(SPELL_SCHOOL_FIRE));
    sLog.outDebug("NatureRes is: \t\t%u\t\tFrostRes is: \t\t%u",GetResistance(SPELL_SCHOOL_NATURE), GetResistance(SPELL_SCHOOL_FROST));
    sLog.outDebug("ShadowRes is: \t\t%u\t\tArcaneRes is: \t\t%u",GetResistance(SPELL_SCHOOL_SHADOW), GetResistance(SPELL_SCHOOL_ARCANE));
    sLog.outDebug("MIN_DAMAGE is: \t\t%f\tMAX_DAMAGE is: \t\t%f",GetFloatValue(UNIT_FIELD_MINDAMAGE), GetFloatValue(UNIT_FIELD_MAXDAMAGE));
    sLog.outDebug("MIN_OFFHAND_DAMAGE is: \t%f\tMAX_OFFHAND_DAMAGE is: \t%f",GetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE), GetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE));
    sLog.outDebug("MIN_RANGED_DAMAGE is: \t%f\tMAX_RANGED_DAMAGE is: \t%f",GetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE), GetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE));
    sLog.outDebug("ATTACK_TIME is: \t%u\t\tRANGE_ATTACK_TIME is: \t%u",GetAttackTime(BASE_ATTACK), GetAttackTime(RANGED_ATTACK));
}

/*********************************************************/
/***               FLOOD FILTER SYSTEM                 ***/
/*********************************************************/

void Player::UpdateSpeakTime()
{
    // ignore chat spam protection for GMs in any mode
    if(GetSession()->GetSecurity() > SEC_PLAYER)
        return;

    time_t current = time (NULL);
    if(m_speakTime > current)
    {
        uint32 max_count = sWorld.getConfig(CONFIG_UINT32_CHATFLOOD_MESSAGE_COUNT);
        if(!max_count)
            return;

        ++m_speakCount;
        if(m_speakCount >= max_count)
        {
            // prevent overwrite mute time, if message send just before mutes set, for example.
            time_t new_mute = current + sWorld.getConfig(CONFIG_UINT32_CHATFLOOD_MUTE_TIME);
            if(GetSession()->m_muteTime < new_mute)
                GetSession()->m_muteTime = new_mute;

            m_speakCount = 0;
        }
    }
    else
        m_speakCount = 0;

    m_speakTime = current + sWorld.getConfig(CONFIG_UINT32_CHATFLOOD_MESSAGE_DELAY);
}

bool Player::CanSpeak() const
{
    return  GetSession()->m_muteTime <= time (NULL);
}

/*********************************************************/
/***              LOW LEVEL FUNCTIONS:Notifiers        ***/
/*********************************************************/

void Player::SendAttackSwingNotInRange()
{
    WorldPacket data(SMSG_ATTACKSWING_NOTINRANGE, 0);
    GetSession()->SendPacket( &data );
}

void Player::SavePositionInDB(ObjectGuid guid, uint32 mapid, float x, float y, float z, float o, uint32 zone)
{
    std::ostringstream ss;
    ss << "UPDATE characters SET position_x='"<<x<<"',position_y='"<<y
        << "',position_z='"<<z<<"',orientation='"<<o<<"',map='"<<mapid
        << "',zone='"<<zone<<"',trans_x='0',trans_y='0',trans_z='0',"
        << "transguid='0',taxi_path='' WHERE guid='"<< guid.GetCounter() <<"'";
    DEBUG_LOG("%s", ss.str().c_str());
    CharacterDatabase.Execute(ss.str().c_str());
}

void Player::SetUInt32ValueInArray(Tokens& tokens,uint16 index, uint32 value)
{
    char buf[11];
    snprintf(buf,11,"%u",value);

    if(index >= tokens.size())
        return;

    tokens[index] = buf;
}

void Player::Customize(ObjectGuid guid, uint8 gender, uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair)
{
    //                                                     0
    QueryResult* result = CharacterDatabase.PQuery("SELECT playerBytes2 FROM characters WHERE guid = '%u'", guid.GetCounter());
    if (!result)
        return;

    Field* fields = result->Fetch();

    uint32 player_bytes2 = fields[0].GetUInt32();
    player_bytes2 &= ~0xFF;
    player_bytes2 |= facialHair;

    CharacterDatabase.PExecute("UPDATE characters SET gender = '%u', playerBytes = '%u', playerBytes2 = '%u' WHERE guid = '%u'", gender, skin | (face << 8) | (hairStyle << 16) | (hairColor << 24), player_bytes2, guid.GetCounter());

    delete result;
}

void Player::SendAttackSwingDeadTarget()
{
    WorldPacket data(SMSG_ATTACKSWING_DEADTARGET, 0);
    GetSession()->SendPacket( &data );
}

void Player::SendAttackSwingCantAttack()
{
    WorldPacket data(SMSG_ATTACKSWING_CANT_ATTACK, 0);
    GetSession()->SendPacket( &data );
}

void Player::SendAttackSwingCancelAttack()
{
    WorldPacket data(SMSG_CANCEL_COMBAT, 0);
    GetSession()->SendPacket( &data );
}

void Player::SendAttackSwingBadFacingAttack()
{
    WorldPacket data(SMSG_ATTACKSWING_BADFACING, 0);
    GetSession()->SendPacket( &data );
}

void Player::SendAutoRepeatCancel(Unit *target)
{
    WorldPacket data(SMSG_CANCEL_AUTO_REPEAT, target->GetPackGUID().size());
    data << target->GetPackGUID();
    GetSession()->SendPacket( &data );
}

void Player::SendExplorationExperience(uint32 Area, uint32 Experience)
{
    WorldPacket data( SMSG_EXPLORATION_EXPERIENCE, 8 );
    data << uint32(Area);
    data << uint32(Experience);
    GetSession()->SendPacket(&data);
}

void Player::SendDungeonDifficulty(bool IsInGroup)
{
    uint8 val = 0x00000001;
    WorldPacket data(MSG_SET_DUNGEON_DIFFICULTY, 12);
    data << uint32(GetDungeonDifficulty());
    data << uint32(val);
    data << uint32(IsInGroup);
    GetSession()->SendPacket(&data);
}

void Player::SendRaidDifficulty(bool IsInGroup)
{
    uint8 val = 0x00000001;
    WorldPacket data(MSG_SET_RAID_DIFFICULTY, 12);
    data << uint32(GetRaidDifficulty());
    data << uint32(val);
    data << uint32(IsInGroup);
    GetSession()->SendPacket(&data);
}

void Player::SendResetFailedNotify(uint32 mapid)
{
    WorldPacket data(SMSG_RESET_FAILED_NOTIFY, 4);
    data << uint32(mapid);
    GetSession()->SendPacket(&data);
}

/// Reset all solo instances and optionally send a message on success for each
void Player::ResetInstances(InstanceResetMethod method, bool isRaid)
{
    // method can be INSTANCE_RESET_ALL, INSTANCE_RESET_CHANGE_DIFFICULTY, INSTANCE_RESET_GROUP_JOIN

    // we assume that when the difficulty changes, all instances that can be reset will be
    Difficulty diff = GetDifficulty(isRaid);

    for (BoundInstancesMap::iterator itr = m_boundInstances[diff].begin(); itr != m_boundInstances[diff].end();)
    {
        DungeonPersistentState *state = itr->second.state;
        const MapEntry *entry = sMapStore.LookupEntry(itr->first);
        if (!entry || entry->IsRaid() != isRaid || !state->CanReset())
        {
            ++itr;
            continue;
        }

        if (method == INSTANCE_RESET_ALL)
        {
            // the "reset all instances" method can only reset normal maps
            if (entry->map_type == MAP_RAID || diff == DUNGEON_DIFFICULTY_HEROIC)
            {
                ++itr;
                continue;
            }
        }

        // if the map is loaded, reset it
        if (Map *map = sMapMgr.FindMap(state->GetMapId(), state->GetInstanceId()))
            if (map->IsDungeon())
                ((DungeonMap*)map)->Reset(method);

        // since this is a solo instance there should not be any players inside
        if (method == INSTANCE_RESET_ALL || method == INSTANCE_RESET_CHANGE_DIFFICULTY)
            SendResetInstanceSuccess(state->GetMapId());

        state->DeleteFromDB();
        m_boundInstances[diff].erase(itr++);

        // the following should remove the instance save from the manager and delete it as well
        state->RemovePlayer(this);
    }
}

void Player::SendResetInstanceSuccess(uint32 MapId)
{
    WorldPacket data(SMSG_INSTANCE_RESET, 4);
    data << uint32(MapId);
    GetSession()->SendPacket(&data);
}

void Player::SendResetInstanceFailed(uint32 reason, uint32 MapId)
{
    // TODO: find what other fail reasons there are besides players in the instance
    WorldPacket data(SMSG_INSTANCE_RESET_FAILED, 4);
    data << uint32(reason);
    data << uint32(MapId);
    GetSession()->SendPacket(&data);
}

/*********************************************************/
/***              Update timers                        ***/
/*********************************************************/

///checks the 15 afk reports per 5 minutes limit
void Player::UpdateAfkReport(time_t currTime)
{
    if(m_bgData.bgAfkReportedTimer <= currTime)
    {
        m_bgData.bgAfkReportedCount = 0;
        m_bgData.bgAfkReportedTimer = currTime+5*MINUTE;
    }
}

void Player::UpdateContestedPvP(uint32 diff)
{
    if(!m_contestedPvPTimer||isInCombat())
        return;
    if(m_contestedPvPTimer <= diff)
    {
        ResetContestedPvP();
    }
    else
        m_contestedPvPTimer -= diff;
}

void Player::UpdatePvPFlag(time_t currTime)
{
    if(!IsPvP())
        return;
    if(pvpInfo.endTimer == 0 || currTime < (pvpInfo.endTimer + 300))
        return;

    UpdatePvP(false);
}

void Player::UpdateDuelFlag(time_t currTime)
{
    if(!duel || duel->startTimer == 0 ||currTime < duel->startTimer + 3)
        return;

    SetUInt32Value(PLAYER_DUEL_TEAM, 1);
    duel->opponent->SetUInt32Value(PLAYER_DUEL_TEAM, 2);

    duel->startTimer = 0;
    duel->startTime  = currTime;
    duel->opponent->duel->startTimer = 0;
    duel->opponent->duel->startTime  = currTime;
}

void Player::RemovePet(PetSaveMode mode)
{
    if (Pet* pet = GetPet())
        pet->Unsummon(mode, this);
}

void Player::BuildPlayerChat(WorldPacket *data, uint8 msgtype, const std::string& text, uint32 language) const
{
    *data << uint8(msgtype);
    *data << uint32(language);
    *data << ObjectGuid(GetObjectGuid());
    *data << uint32(language);                              //language 2.1.0 ?
    *data << ObjectGuid(GetObjectGuid());
    *data << uint32(text.length()+1);
    *data << text;
    *data << uint8(chatTag());
}

void Player::Say(const std::string& text, const uint32 language)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildPlayerChat(&data, CHAT_MSG_SAY, text, language);
    SendMessageToSetInRange(&data,sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_SAY),true);
}

void Player::Yell(const std::string& text, const uint32 language)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildPlayerChat(&data, CHAT_MSG_YELL, text, language);
    SendMessageToSetInRange(&data,sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_YELL),true);
}

void Player::TextEmote(const std::string& text)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildPlayerChat(&data, CHAT_MSG_EMOTE, text, LANG_UNIVERSAL);
    SendMessageToSetInRange(&data,sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_TEXTEMOTE),true, !sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_CHAT) );
}

void Player::Whisper(const std::string& text, uint32 language, ObjectGuid receiver)
{
    if (language != LANG_ADDON)                             // if not addon data
        language = LANG_UNIVERSAL;                          // whispers should always be readable

    Player *rPlayer = sObjectMgr.GetPlayer(receiver);

    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildPlayerChat(&data, CHAT_MSG_WHISPER, text, language);
    rPlayer->GetSession()->SendPacket(&data);

    // not send confirmation for addon messages
    if (language != LANG_ADDON)
    {
        data.Initialize(SMSG_MESSAGECHAT, 200);
        rPlayer->BuildPlayerChat(&data, CHAT_MSG_WHISPER_INFORM, text, language);
        GetSession()->SendPacket(&data);
    }

    if (!isAcceptWhispers())
    {
        SetAcceptWhispers(true);
        ChatHandler(this).SendSysMessage(LANG_COMMAND_WHISPERON);
    }

    // announce afk or dnd message
    if (rPlayer->isAFK())
        ChatHandler(this).PSendSysMessage(LANG_PLAYER_AFK, rPlayer->GetName(), rPlayer->autoReplyMsg.c_str());
    else if (rPlayer->isDND())
        ChatHandler(this).PSendSysMessage(LANG_PLAYER_DND, rPlayer->GetName(), rPlayer->autoReplyMsg.c_str());
}

void Player::PetSpellInitialize()
{
    Pet* pet = GetPet();

    if(!pet)
        return;

    DEBUG_LOG("Pet Spells Groups");

    CharmInfo *charmInfo = pet->GetCharmInfo();

    WorldPacket data(SMSG_PET_SPELLS, 8+2+4+4+4*MAX_UNIT_ACTION_BAR_INDEX+1+1);
    data << pet->GetObjectGuid();
    data << uint16(pet->GetCreatureInfo()->family);         // creature family (required for pet talents)
    data << uint32(0);
    data << uint8(charmInfo->GetReactState()) << uint8(charmInfo->GetCommandState()) << uint16(0);

    // action bar loop
    charmInfo->BuildActionBar(&data);

    size_t spellsCountPos = data.wpos();

    // spells count
    uint8 addlist = 0;
    data << uint8(addlist);                                 // placeholder

    if (pet->IsPermanentPetFor(this))
    {
        // spells loop
        for (PetSpellMap::const_iterator itr = pet->m_spells.begin(); itr != pet->m_spells.end(); ++itr)
        {
            if(itr->second.state == PETSPELL_REMOVED)
                continue;

            data << uint32(MAKE_UNIT_ACTION_BUTTON(itr->first,itr->second.active));
            ++addlist;
        }
    }

    data.put<uint8>(spellsCountPos, addlist);

    uint8 cooldownsCount = pet->m_CreatureSpellCooldowns.size() + pet->m_CreatureCategoryCooldowns.size();
    data << uint8(cooldownsCount);

    time_t curTime = time(NULL);

    for(CreatureSpellCooldowns::const_iterator itr = pet->m_CreatureSpellCooldowns.begin(); itr != pet->m_CreatureSpellCooldowns.end(); ++itr)
    {
        time_t cooldown = (itr->second > curTime) ? (itr->second - curTime) * IN_MILLISECONDS : 0;

        data << uint32(itr->first);                         // spellid
        data << uint16(0);                                  // spell category?
        data << uint32(cooldown);                           // cooldown
        data << uint32(0);                                  // category cooldown
    }

    for(CreatureSpellCooldowns::const_iterator itr = pet->m_CreatureCategoryCooldowns.begin(); itr != pet->m_CreatureCategoryCooldowns.end(); ++itr)
    {
        time_t cooldown = (itr->second > curTime) ? (itr->second - curTime) * IN_MILLISECONDS : 0;

        data << uint32(itr->first);                         // spellid
        data << uint16(0);                                  // spell category?
        data << uint32(0);                                  // cooldown
        data << uint32(cooldown);                           // category cooldown
    }

    GetSession()->SendPacket(&data);
}

void Player::SendPetGUIDs()
{
    if (!GetPetGuid())
        return;

    // Later this function might get modified for multiple guids
    WorldPacket data(SMSG_PET_GUIDS, 12);
    data << uint32(1);                      // count
    data << ObjectGuid(GetPetGuid());
    GetSession()->SendPacket(&data);
}

void Player::PossessSpellInitialize()
{
    Unit* charm = GetCharm();

    if(!charm)
        return;

    CharmInfo *charmInfo = charm->GetCharmInfo();

    if(!charmInfo)
    {
        sLog.outError("Player::PossessSpellInitialize(): charm (GUID: %u TypeId: %u) has no charminfo!", charm->GetGUIDLow(),charm->GetTypeId());
        return;
    }

    WorldPacket data(SMSG_PET_SPELLS, 8+2+4+4+4*MAX_UNIT_ACTION_BAR_INDEX+1+1);
    data << charm->GetObjectGuid();
    data << uint16(0);
    data << uint32(0);
    data << uint32(0);

    charmInfo->BuildActionBar(&data);

    data << uint8(0);                                       // spells count
    data << uint8(0);                                       // cooldowns count

    GetSession()->SendPacket(&data);
}

void Player::CharmSpellInitialize()
{
    Unit* charm = GetCharm();

    if(!charm)
        return;

    CharmInfo *charmInfo = charm->GetCharmInfo();
    if(!charmInfo)
    {
        sLog.outError("Player::CharmSpellInitialize(): the player's charm (GUID: %u TypeId: %u) has no charminfo!", charm->GetGUIDLow(),charm->GetTypeId());
        return;
    }

    uint8 addlist = 0;

    if(charm->GetTypeId() != TYPEID_PLAYER)
    {
        CreatureInfo const *cinfo = ((Creature*)charm)->GetCreatureInfo();

        if(cinfo && cinfo->type == CREATURE_TYPE_DEMON && getClass() == CLASS_WARLOCK)
        {
            for(uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
            {
                if(charmInfo->GetCharmSpell(i)->GetAction())
                    ++addlist;
            }
        }
    }

    WorldPacket data(SMSG_PET_SPELLS, 8+2+4+4+4*MAX_UNIT_ACTION_BAR_INDEX+1+4*addlist+1);
    data << charm->GetObjectGuid();
    data << uint16(0);
    data << uint32(0);

    if(charm->GetTypeId() != TYPEID_PLAYER)
        data << uint8(charmInfo->GetReactState()) << uint8(charmInfo->GetCommandState()) << uint16(0);
    else
        data << uint8(0) << uint8(0) << uint16(0);

    charmInfo->BuildActionBar(&data);

    data << uint8(addlist);

    if(addlist)
    {
        for(uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
        {
            CharmSpellEntry *cspell = charmInfo->GetCharmSpell(i);
            if(cspell->GetAction())
                data << uint32(cspell->packedData);
        }
    }

    data << uint8(0);                                       // cooldowns count

    GetSession()->SendPacket(&data);
}

void Player::RemovePetActionBar()
{
    WorldPacket data(SMSG_PET_SPELLS, 8);
    data << ObjectGuid();
    SendDirectMessage(&data);
}

void Player::AddSpellMod(Aura* aura, bool apply)
{
    Modifier const* mod = aura->GetModifier();
    uint16 Opcode= (mod->m_auraname == SPELL_AURA_ADD_FLAT_MODIFIER) ? SMSG_SET_FLAT_SPELL_MODIFIER : SMSG_SET_PCT_SPELL_MODIFIER;

    for(int eff = 0; eff < 96; ++eff)
    {
        uint64 _mask = 0;
        uint32 _mask2= 0;

        if (eff < 64)
            _mask = uint64(1) << (eff - 0);
        else
            _mask2= uint32(1) << (eff - 64);

        if (aura->GetAuraSpellClassMask().IsFitToFamilyMask(_mask, _mask2))
        {
            int32 val = 0;
            for (AuraList::const_iterator itr = m_spellMods[mod->m_miscvalue].begin(); itr != m_spellMods[mod->m_miscvalue].end(); ++itr)
            {
                if ((*itr)->GetModifier()->m_auraname == mod->m_auraname && ((*itr)->GetAuraSpellClassMask().IsFitToFamilyMask(_mask, _mask2)))
                    val += (*itr)->GetModifier()->m_amount;
            }
            val += apply ? mod->m_amount : -(mod->m_amount);
            WorldPacket data(Opcode, (1+1+4));
            data << uint8(eff);
            data << uint8(mod->m_miscvalue);
            data << int32(val);
            SendDirectMessage(&data);
        }
    }

    if (apply)
        m_spellMods[mod->m_miscvalue].push_back(aura);
    else
        m_spellMods[mod->m_miscvalue].remove(aura);
}

template <class T> T Player::ApplySpellMod(uint32 spellId, SpellModOp op, T &basevalue, Spell const* spell)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
        return 0;

    int32 totalpct = 0;
    int32 totalflat = 0;
    for (AuraList::iterator itr = m_spellMods[op].begin(); itr != m_spellMods[op].end(); ++itr)
    {
        Aura *aura = *itr;

        Modifier const* mod = aura->GetModifier();

        if (!aura->isAffectedOnSpell(spellInfo))
            continue;

        if (mod->m_auraname == SPELL_AURA_ADD_FLAT_MODIFIER)
            totalflat += mod->m_amount;
        else
        {
            // skip percent mods for null basevalue (most important for spell mods with charges )
            if (basevalue == T(0))
                continue;

            // special case (skip >10sec spell casts for instant cast setting)
            if (mod->m_miscvalue == SPELLMOD_CASTING_TIME
                && basevalue >= T(10*IN_MILLISECONDS) && mod->m_amount <= -100)
                continue;

            totalpct += mod->m_amount;
        }
    }

    float diff = (float)basevalue*(float)totalpct/100.0f + (float)totalflat;
    basevalue = T((float)basevalue + diff);
    return T(diff);
}

template int32 Player::ApplySpellMod<int32>(uint32 spellId, SpellModOp op, int32 &basevalue, Spell const* spell);
template uint32 Player::ApplySpellMod<uint32>(uint32 spellId, SpellModOp op, uint32 &basevalue, Spell const* spell);
template float Player::ApplySpellMod<float>(uint32 spellId, SpellModOp op, float &basevalue, Spell const* spell);

// send Proficiency
void Player::SendProficiency(ItemClass itemClass, uint32 itemSubclassMask)
{
    WorldPacket data(SMSG_SET_PROFICIENCY, 1 + 4);
    data << uint8(itemClass) << uint32(itemSubclassMask);
    GetSession()->SendPacket (&data);
}

void Player::RemovePetitionsAndSigns(ObjectGuid guid, uint32 type)
{
    uint32 lowguid = guid.GetCounter();

    QueryResult *result = NULL;
    if(type == 10)
        result = CharacterDatabase.PQuery("SELECT ownerguid,petitionguid FROM petition_sign WHERE playerguid = '%u'", lowguid);
    else
        result = CharacterDatabase.PQuery("SELECT ownerguid,petitionguid FROM petition_sign WHERE playerguid = '%u' AND type = '%u'", lowguid, type);
    if(result)
    {
        do                                                  // this part effectively does nothing, since the deletion / modification only takes place _after_ the PetitionQuery. Though I don't know if the result remains intact if I execute the delete query beforehand.
        {                                                   // and SendPetitionQueryOpcode reads data from the DB
            Field *fields = result->Fetch();
            ObjectGuid ownerguid   = ObjectGuid(HIGHGUID_PLAYER, fields[0].GetUInt32());
            ObjectGuid petitionguid = ObjectGuid(HIGHGUID_ITEM, fields[1].GetUInt32());

            // send update if charter owner in game
            Player* owner = sObjectMgr.GetPlayer(ownerguid);
            if(owner)
                owner->GetSession()->SendPetitionQueryOpcode(petitionguid);

        } while ( result->NextRow() );

        delete result;

        if(type==10)
            CharacterDatabase.PExecute("DELETE FROM petition_sign WHERE playerguid = '%u'", lowguid);
        else
            CharacterDatabase.PExecute("DELETE FROM petition_sign WHERE playerguid = '%u' AND type = '%u'", lowguid, type);
    }

    CharacterDatabase.BeginTransaction();
    if(type == 10)
    {
        CharacterDatabase.PExecute("DELETE FROM petition WHERE ownerguid = '%u'", lowguid);
        CharacterDatabase.PExecute("DELETE FROM petition_sign WHERE ownerguid = '%u'", lowguid);
    }
    else
    {
        CharacterDatabase.PExecute("DELETE FROM petition WHERE ownerguid = '%u' AND type = '%u'", lowguid, type);
        CharacterDatabase.PExecute("DELETE FROM petition_sign WHERE ownerguid = '%u' AND type = '%u'", lowguid, type);
    }
    CharacterDatabase.CommitTransaction();
}

void Player::LeaveAllArenaTeams(ObjectGuid guid)
{
    uint32 lowguid = guid.GetCounter();
    QueryResult *result = CharacterDatabase.PQuery("SELECT arena_team_member.arenateamid FROM arena_team_member JOIN arena_team ON arena_team_member.arenateamid = arena_team.arenateamid WHERE guid='%u'", lowguid);
    if (!result)
        return;

    do
    {
        Field *fields = result->Fetch();
        if (uint32 at_id = fields[0].GetUInt32())
            if (ArenaTeam * at = sObjectMgr.GetArenaTeamById(at_id))
                at->DelMember(guid);

    } while (result->NextRow());

    delete result;
}

void Player::SetRestBonus (float rest_bonus_new)
{
    // Prevent resting on max level
    if(getLevel() >= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
        rest_bonus_new = 0;

    if(rest_bonus_new < 0)
        rest_bonus_new = 0;

    float rest_bonus_max = (float)GetUInt32Value(PLAYER_NEXT_LEVEL_XP)*1.5f/2.0f;

    if(rest_bonus_new > rest_bonus_max)
        m_rest_bonus = rest_bonus_max;
    else
        m_rest_bonus = rest_bonus_new;

    // update data for client
    if(m_rest_bonus>10)
        SetByteValue(PLAYER_BYTES_2, 3, 0x01);              // Set Reststate = Rested
    else if(m_rest_bonus<=1)
        SetByteValue(PLAYER_BYTES_2, 3, 0x02);              // Set Reststate = Normal

    //RestTickUpdate
    SetUInt32Value(PLAYER_REST_STATE_EXPERIENCE, uint32(m_rest_bonus));
}

void Player::HandleStealthedUnitsDetection()
{
    std::list<Unit*> stealthedUnits;

    MaNGOS::AnyStealthedCheck u_check(this);
    MaNGOS::UnitListSearcher<MaNGOS::AnyStealthedCheck > searcher(stealthedUnits, u_check);
    Cell::VisitAllObjects(this, searcher, MAX_PLAYER_STEALTH_DETECT_RANGE);

    WorldObject const* viewPoint = GetCamera().GetBody();

    for (std::list<Unit*>::const_iterator i = stealthedUnits.begin(); i != stealthedUnits.end(); ++i)
    {
        if((*i)==this)
            continue;

        bool hasAtClient = HaveAtClient((*i));
        bool hasDetected = (*i)->isVisibleForOrDetect(this, viewPoint, true);

        if (hasDetected)
        {
            if(!hasAtClient)
            {
                ObjectGuid i_guid = (*i)->GetObjectGuid();
                (*i)->SendCreateUpdateToPlayer(this);
                m_clientGUIDs.insert(i_guid);

                DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "%s is detected in stealth by player %u. Distance = %f",i_guid.GetString().c_str(),GetGUIDLow(),GetDistance(*i));

                // target aura duration for caster show only if target exist at caster client
                // send data at target visibility change (adding to client)
                if((*i)!=this && (*i)->isType(TYPEMASK_UNIT))
                    SendAurasForTarget(*i);
            }
        }
        else
        {
            if(hasAtClient)
            {
                (*i)->DestroyForPlayer(this);
                m_clientGUIDs.erase((*i)->GetObjectGuid());
            }
        }
    }
}

bool Player::ActivateTaxiPathTo(std::vector<uint32> const& nodes, Creature* npc /*= NULL*/, uint32 spellid /*= 0*/)
{
    if(nodes.size() < 2)
        return false;

    // not let cheating with start flight in time of logout process || if casting not finished || while in combat || if not use Spell's with EffectSendTaxi
    if(GetSession()->isLogingOut() || isInCombat())
    {
        WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
        data << uint32(ERR_TAXIPLAYERBUSY);
        GetSession()->SendPacket(&data);
        return false;
    }

    if(HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE))
        return false;

    // taximaster case
    if (npc)
    {
        // not let cheating with start flight mounted
        if (IsMounted())
        {
            WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
            data << uint32(ERR_TAXIPLAYERALREADYMOUNTED);
            GetSession()->SendPacket(&data);
            return false;
        }

        if (IsInDisallowedMountForm())
        {
            WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
            data << uint32(ERR_TAXIPLAYERSHAPESHIFTED);
            GetSession()->SendPacket(&data);
            return false;
        }

        // not let cheating with start flight in time of logout process || if casting not finished || while in combat || if not use Spell's with EffectSendTaxi
        if(IsNonMeleeSpellCasted(false))
        {
            WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
            data << uint32(ERR_TAXIPLAYERBUSY);
            GetSession()->SendPacket(&data);
            return false;
        }
    }
    // cast case or scripted call case
    else
    {
        RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);

        if (IsInDisallowedMountForm())
            RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT);

        if (Spell* spell = GetCurrentSpell(CURRENT_GENERIC_SPELL))
            if (spell->m_spellInfo->Id != spellid)
                InterruptSpell(CURRENT_GENERIC_SPELL,false);

        InterruptSpell(CURRENT_AUTOREPEAT_SPELL,false);

        if (Spell* spell = GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            if (spell->m_spellInfo->Id != spellid)
                InterruptSpell(CURRENT_CHANNELED_SPELL,true);
    }

    uint32 sourcenode = nodes[0];

    // starting node too far away (cheat?)
    TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(sourcenode);
    if (!node)
    {
        WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
        data << uint32(ERR_TAXINOSUCHPATH);
        GetSession()->SendPacket(&data);
        return false;
    }

    // check node starting pos data set case if provided
    if (node->x != 0.0f || node->y != 0.0f || node->z != 0.0f)
    {
        if (node->map_id != GetMapId() ||
            (node->x - GetPositionX())*(node->x - GetPositionX())+
            (node->y - GetPositionY())*(node->y - GetPositionY())+
            (node->z - GetPositionZ())*(node->z - GetPositionZ()) >
            (2*INTERACTION_DISTANCE)*(2*INTERACTION_DISTANCE)*(2*INTERACTION_DISTANCE))
        {
            WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
            data << uint32(ERR_TAXITOOFARAWAY);
            GetSession()->SendPacket(&data);
            return false;
        }
    }
    // node must have pos if taxi master case (npc != NULL)
    else if (npc)
    {
        WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
        data << uint32(ERR_TAXIUNSPECIFIEDSERVERERROR);
        GetSession()->SendPacket(&data);
        return false;
    }

    // Prepare to flight start now

    // stop combat at start taxi flight if any
    CombatStop();

    // stop trade (client cancel trade at taxi map open but cheating tools can be used for reopen it)
    TradeCancel(true);

    // clean not finished taxi path if any
    m_taxi.ClearTaxiDestinations();

    // 0 element current node
    m_taxi.AddTaxiDestination(sourcenode);

    // fill destinations path tail
    uint32 sourcepath = 0;
    uint32 totalcost = 0;

    uint32 prevnode = sourcenode;
    uint32 lastnode = 0;

    for(uint32 i = 1; i < nodes.size(); ++i)
    {
        uint32 path, cost;

        lastnode = nodes[i];
        sObjectMgr.GetTaxiPath(prevnode, lastnode, path, cost);

        if(!path)
        {
            m_taxi.ClearTaxiDestinations();
            return false;
        }

        totalcost += cost;

        if(prevnode == sourcenode)
            sourcepath = path;

        m_taxi.AddTaxiDestination(lastnode);

        prevnode = lastnode;
    }

    // get mount model (in case non taximaster (npc==NULL) allow more wide lookup)
    uint32 mount_display_id = sObjectMgr.GetTaxiMountDisplayId(sourcenode, GetTeam(), npc == NULL);

    // in spell case allow 0 model
    if ((mount_display_id == 0 && spellid == 0) || sourcepath == 0)
    {
        WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
        data << uint32(ERR_TAXIUNSPECIFIEDSERVERERROR);
        GetSession()->SendPacket(&data);
        m_taxi.ClearTaxiDestinations();
        return false;
    }

    uint32 money = GetMoney();

    if (npc)
        totalcost = (uint32)ceil(totalcost*GetReputationPriceDiscount(npc));

    if(money < totalcost)
    {
        WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
        data << uint32(ERR_TAXINOTENOUGHMONEY);
        GetSession()->SendPacket(&data);
        m_taxi.ClearTaxiDestinations();
        return false;
    }

    //Checks and preparations done, DO FLIGHT
    ModifyMoney(-(int32)totalcost);
    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING, totalcost);
    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN, 1);

    // prevent stealth flight
    RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);

    WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
    data << uint32(ERR_TAXIOK);
    GetSession()->SendPacket(&data);

    DEBUG_LOG("WORLD: Sent SMSG_ACTIVATETAXIREPLY");

    GetSession()->SendDoFlight(mount_display_id, sourcepath);

    return true;
}

bool Player::ActivateTaxiPathTo( uint32 taxi_path_id, uint32 spellid /*= 0*/ )
{
    TaxiPathEntry const* entry = sTaxiPathStore.LookupEntry(taxi_path_id);
    if(!entry)
        return false;

    std::vector<uint32> nodes;

    nodes.resize(2);
    nodes[0] = entry->from;
    nodes[1] = entry->to;

    return ActivateTaxiPathTo(nodes,NULL,spellid);
}

void Player::ContinueTaxiFlight()
{
    uint32 sourceNode = m_taxi.GetTaxiSource();
    if (!sourceNode)
        return;

    DEBUG_LOG( "WORLD: Restart character %u taxi flight", GetGUIDLow() );

    uint32 mountDisplayId = sObjectMgr.GetTaxiMountDisplayId(sourceNode, GetTeam(), true);
    uint32 path = m_taxi.GetCurrentTaxiPath();

    // search appropriate start path node
    uint32 startNode = 0;

    TaxiPathNodeList const& nodeList = sTaxiPathNodesByPath[path];

    float distPrev = MAP_SIZE*MAP_SIZE;
    float distNext =
        (nodeList[0].x-GetPositionX())*(nodeList[0].x-GetPositionX())+
        (nodeList[0].y-GetPositionY())*(nodeList[0].y-GetPositionY())+
        (nodeList[0].z-GetPositionZ())*(nodeList[0].z-GetPositionZ());

    for(uint32 i = 1; i < nodeList.size(); ++i)
    {
        TaxiPathNodeEntry const& node = nodeList[i];
        TaxiPathNodeEntry const& prevNode = nodeList[i-1];

        // skip nodes at another map
        if (node.mapid != GetMapId())
            continue;

        distPrev = distNext;

        distNext =
            (node.x-GetPositionX())*(node.x-GetPositionX())+
            (node.y-GetPositionY())*(node.y-GetPositionY())+
            (node.z-GetPositionZ())*(node.z-GetPositionZ());

        float distNodes =
            (node.x-prevNode.x)*(node.x-prevNode.x)+
            (node.y-prevNode.y)*(node.y-prevNode.y)+
            (node.z-prevNode.z)*(node.z-prevNode.z);

        if (distNext + distPrev < distNodes)
        {
            startNode = i;
            break;
        }
    }

    GetSession()->SendDoFlight(mountDisplayId, path, startNode);
}

void Player::ProhibitSpellSchool(SpellSchoolMask idSchoolMask, uint32 unTimeMs )
{
                                                            // last check 2.0.10
    WorldPacket data(SMSG_SPELL_COOLDOWN, 8+1+m_spells.size()*8);
    data << GetObjectGuid();
    data << uint8(0x0);                                     // flags (0x1, 0x2)
    time_t curTime = time(NULL);
    for(PlayerSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        if (itr->second.state == PLAYERSPELL_REMOVED)
            continue;
        uint32 unSpellId = itr->first;
        SpellEntry const *spellInfo = sSpellStore.LookupEntry(unSpellId);
        if (!spellInfo)
        {
            MANGOS_ASSERT(spellInfo);
            continue;
        }

        // Not send cooldown for this spells
        if (spellInfo->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
            continue;

        if((idSchoolMask & GetSpellSchoolMask(spellInfo)) && GetSpellCooldownDelay(unSpellId) < unTimeMs )
        {
            data << uint32(unSpellId);
            data << uint32(unTimeMs);                       // in m.secs
            AddSpellCooldown(unSpellId, 0, curTime + unTimeMs/IN_MILLISECONDS);
        }
    }
    GetSession()->SendPacket(&data);
}

void Player::InitDataForForm(bool reapplyMods)
{
    ShapeshiftForm form = GetShapeshiftForm();

    SpellShapeshiftFormEntry const* ssEntry = sSpellShapeshiftFormStore.LookupEntry(form);
    if(ssEntry && ssEntry->attackSpeed)
    {
        SetAttackTime(BASE_ATTACK,ssEntry->attackSpeed);
        SetAttackTime(OFF_ATTACK,ssEntry->attackSpeed);
        SetAttackTime(RANGED_ATTACK, BASE_ATTACK_TIME);
    }
    else
        SetRegularAttackTime();

    switch(form)
    {
        case FORM_CAT:
        {
            if(getPowerType()!=POWER_ENERGY)
                setPowerType(POWER_ENERGY);
            break;
        }
        case FORM_BEAR:
        case FORM_DIREBEAR:
        {
            if(getPowerType()!=POWER_RAGE)
                setPowerType(POWER_RAGE);
            break;
        }
        default:                                            // 0, for example
        {
            ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(getClass());
            if(cEntry && cEntry->powerType < MAX_POWERS && uint32(getPowerType()) != cEntry->powerType)
                setPowerType(Powers(cEntry->powerType));
            break;
        }
    }

    // update auras at form change, ignore this at mods reapply (.reset stats/etc) when form not change.
    if (!reapplyMods)
        UpdateEquipSpellsAtFormChange();

    UpdateAttackPowerAndDamage();
    UpdateAttackPowerAndDamage(true);
}

void Player::InitDisplayIds()
{
    PlayerInfo const *info = sObjectMgr.GetPlayerInfo(getRace(), getClass());
    if(!info)
    {
        sLog.outError("Player %u has incorrect race/class pair. Can't init display ids.", GetGUIDLow());
        return;
    }

    // reset scale before reapply auras
    SetObjectScale(DEFAULT_OBJECT_SCALE);

    uint8 gender = getGender();
    switch(gender)
    {
        case GENDER_FEMALE:
            SetDisplayId(info->displayId_f );
            SetNativeDisplayId(info->displayId_f );
            break;
        case GENDER_MALE:
            SetDisplayId(info->displayId_m );
            SetNativeDisplayId(info->displayId_m );
            break;
        default:
            sLog.outError("Invalid gender %u for player",gender);
            return;
    }
}

void Player::TakeExtendedCost(uint32 extendedCostId, uint32 count)
{
    ItemExtendedCostEntry const* extendedCost = sItemExtendedCostStore.LookupEntry(extendedCostId);

    if (extendedCost->reqhonorpoints)
        ModifyHonorPoints(-int32(extendedCost->reqhonorpoints * count));
    if (extendedCost->reqarenapoints)
        ModifyArenaPoints(-int32(extendedCost->reqarenapoints * count));

    for (uint8 i = 0; i < MAX_EXTENDED_COST_ITEMS; ++i)
    {
        if (extendedCost->reqitem[i])
            DestroyItemCount(extendedCost->reqitem[i], extendedCost->reqitemcount[i] * count, true);
    }
}

// Return true is the bought item has a max count to force refresh of window by caller
bool Player::BuyItemFromVendorSlot(ObjectGuid vendorGuid, uint32 vendorslot, uint32 item, uint8 count, uint8 bag, uint8 slot)
{
    // cheating attempt
    if (count < 1) count = 1;

    if (!isAlive())
        return false;

    ItemPrototype const *pProto = ObjectMgr::GetItemPrototype(item);
    if (!pProto)
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, NULL, item, 0);
        return false;
    }

    Creature *pCreature = GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_VENDOR);
    if (!pCreature)
    {
        DEBUG_LOG("WORLD: BuyItemFromVendor - %s not found or you can't interact with him.", vendorGuid.GetString().c_str());
        SendBuyError(BUY_ERR_DISTANCE_TOO_FAR, NULL, item, 0);
        return false;
    }

    VendorItemData const* vItems = pCreature->GetVendorItems();
    VendorItemData const* tItems = pCreature->GetVendorTemplateItems();
    if ((!vItems || vItems->Empty()) && (!tItems || tItems->Empty()))
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    uint32 vCount = vItems ? vItems->GetItemCount() : 0;
    uint32 tCount = tItems ? tItems->GetItemCount() : 0;

    if (vendorslot >= vCount+tCount)
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    VendorItem const* crItem = vendorslot < vCount ? vItems->GetItem(vendorslot) : tItems->GetItem(vendorslot - vCount);
    if (!crItem)                                            // store diff item (cheating)
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    if (crItem->item != item)                               // store diff item (cheating or special convert)
    {
        bool converted = false;

        // possible item converted for BoA case
        ItemPrototype const* crProto = ObjectMgr::GetItemPrototype(crItem->item);
        if (crProto->Flags & ITEM_FLAG_BOA && crProto->RequiredReputationFaction &&
            uint32(GetReputationRank(crProto->RequiredReputationFaction)) >= crProto->RequiredReputationRank)
            converted = (sObjectMgr.GetItemConvert(crItem->item, getRaceMask()) != 0);

        if (!converted)
        {
            SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
            return false;
        }
    }

    uint32 totalCount = pProto->BuyCount * count;

    // check current item amount if it limited
    if (crItem->maxcount != 0)
    {
        if (pCreature->GetVendorItemCurrentCount(crItem) < totalCount)
        {
            SendBuyError(BUY_ERR_ITEM_ALREADY_SOLD, pCreature, item, 0);
            return false;
        }
    }

    if (uint32(GetReputationRank(pProto->RequiredReputationFaction)) < pProto->RequiredReputationRank)
    {
        SendBuyError(BUY_ERR_REPUTATION_REQUIRE, pCreature, item, 0);
        return false;
    }

    if (uint32 extendedCostId = crItem->ExtendedCost)
    {
        ItemExtendedCostEntry const* iece = sItemExtendedCostStore.LookupEntry(extendedCostId);
        if (!iece)
        {
            sLog.outError("Item %u have wrong ExtendedCost field value %u", pProto->ItemId, extendedCostId);
            return false;
        }

        // honor points price
        if (GetHonorPoints() < (iece->reqhonorpoints * count))
        {
            SendEquipError(EQUIP_ERR_NOT_ENOUGH_HONOR_POINTS, NULL, NULL);
            return false;
        }

        // arena points price
        if (GetArenaPoints() < (iece->reqarenapoints * count))
        {
            SendEquipError(EQUIP_ERR_NOT_ENOUGH_ARENA_POINTS, NULL, NULL);
            return false;
        }

        // item base price
        for (uint8 i = 0; i < MAX_EXTENDED_COST_ITEMS; ++i)
        {
            if (iece->reqitem[i] && !HasItemCount(iece->reqitem[i], iece->reqitemcount[i] * count))
            {
                SendEquipError(EQUIP_ERR_VENDOR_MISSING_TURNINS, NULL, NULL);
                return false;
            }
        }

        // check for personal arena rating requirement
        if (GetMaxPersonalArenaRatingRequirement(iece->reqarenaslot) < iece->reqpersonalarenarating)
        {
            // probably not the proper equip err
            SendEquipError(EQUIP_ERR_CANT_EQUIP_RANK, NULL, NULL);
            return false;
        }
    }

    uint32 price = (crItem->ExtendedCost == 0 || pProto->Flags2 & ITEM_FLAG2_EXT_COST_REQUIRES_GOLD) ? pProto->BuyPrice * count : 0;

    // reputation discount
    if (price)
        price = uint32(floor(price * GetReputationPriceDiscount(pCreature)));

    if (GetMoney() < price)
    {
        SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, pCreature, item, 0);
        return false;
    }

    Item* pItem = NULL;

    if ((bag == NULL_BAG && slot == NULL_SLOT) || IsInventoryPos(bag, slot))
    {
        ItemPosCountVec dest;
        InventoryResult msg = CanStoreNewItem(bag, slot, dest, item, totalCount);
        if (msg != EQUIP_ERR_OK)
        {
            SendEquipError(msg, NULL, NULL, item);
            return false;
        }

        ModifyMoney(-int32(price));

        if (crItem->ExtendedCost)
            TakeExtendedCost(crItem->ExtendedCost, count);

        pItem = StoreNewItem(dest, item, true);
    }
    else if (IsEquipmentPos(bag, slot))
    {
        if (totalCount != 1)
        {
            SendEquipError(EQUIP_ERR_ITEM_CANT_BE_EQUIPPED, NULL, NULL);
            return false;
        }

        uint16 dest;
        InventoryResult msg = CanEquipNewItem(slot, dest, item, false);
        if (msg != EQUIP_ERR_OK)
        {
            SendEquipError(msg, NULL, NULL, item);
            return false;
        }

        ModifyMoney(-int32(price));

        if (crItem->ExtendedCost)
            TakeExtendedCost(crItem->ExtendedCost, count);

        pItem = EquipNewItem(dest, item, true);

        if (pItem)
            AutoUnequipOffhandIfNeed();
    }
    else
    {
        SendEquipError(EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT, NULL, NULL);
        return false;
    }

    if (!pItem)
        return false;

    uint32 new_count = pCreature->UpdateVendorItemCurrentCount(crItem, totalCount);

    WorldPacket data(SMSG_BUY_ITEM, 8+4+4+4);
    data << pCreature->GetObjectGuid();
    data << uint32(vendorslot + 1);                 // numbered from 1 at client
    data << uint32(crItem->maxcount > 0 ? new_count : 0xFFFFFFFF);
    data << uint32(count);
    GetSession()->SendPacket(&data);

    SendNewItem(pItem, totalCount, true, false, false);

    return crItem->maxcount != 0;
}

uint32 Player::GetMaxPersonalArenaRatingRequirement(uint32 minarenaslot)
{
    // returns the maximal personal arena rating that can be used to purchase items requiring this condition
    // the personal rating of the arena team must match the required limit as well
    // so return max[in arenateams](min(personalrating[teamtype], teamrating[teamtype]))
    uint32 max_personal_rating = 0;
    for(int i = minarenaslot; i < MAX_ARENA_SLOT; ++i)
    {
        if(ArenaTeam * at = sObjectMgr.GetArenaTeamById(GetArenaTeamId(i)))
        {
            uint32 p_rating = GetArenaPersonalRating(i);
            uint32 t_rating = at->GetRating();
            p_rating = p_rating < t_rating ? p_rating : t_rating;
            if(max_personal_rating < p_rating)
                max_personal_rating = p_rating;
        }
    }
    return max_personal_rating;
}

void Player::UpdateHomebindTime(uint32 time)
{
    // GMs never get homebind timer online
    if (m_InstanceValid || isGameMaster())
    {
        if(m_HomebindTimer)                                 // instance valid, but timer not reset
        {
            // hide reminder
            WorldPacket data(SMSG_RAID_GROUP_ONLY, 4+4);
            data << uint32(0);
            data << uint32(ERR_RAID_GROUP_NONE);            // error used only when timer = 0
            GetSession()->SendPacket(&data);
        }
        // instance is valid, reset homebind timer
        m_HomebindTimer = 0;
    }
    else if (m_HomebindTimer > 0)
    {
        if (time >= m_HomebindTimer)
        {
            // teleport to nearest graveyard
            RepopAtGraveyard();
        }
        else
            m_HomebindTimer -= time;
    }
    else
    {
        // instance is invalid, start homebind timer
        m_HomebindTimer = 60000;
        // send message to player
        WorldPacket data(SMSG_RAID_GROUP_ONLY, 4+4);
        data << uint32(m_HomebindTimer);
        data << uint32(ERR_RAID_GROUP_NONE);                // error used only when timer = 0
        GetSession()->SendPacket(&data);
        DEBUG_LOG("PLAYER: Player '%s' (GUID: %u) will be teleported to homebind in 60 seconds", GetName(),GetGUIDLow());
    }
}

void Player::UpdatePvP(bool state, bool ovrride)
{
    if(!state || ovrride)
    {
        SetPvP(state);
        pvpInfo.endTimer = 0;
    }
    else
    {
        if(pvpInfo.endTimer != 0)
            pvpInfo.endTimer = time(NULL);
        else
            SetPvP(state);
    }
}

void Player::AddSpellAndCategoryCooldowns(SpellEntry const* spellInfo, uint32 itemId, Spell* spell, bool infinityCooldown)
{
    // init cooldown values
    uint32 cat   = 0;
    int32 rec    = -1;
    int32 catrec = -1;

    // some special item spells without correct cooldown in SpellInfo
    // cooldown information stored in item prototype
    // This used in same way in WorldSession::HandleItemQuerySingleOpcode data sending to client.

    if (itemId)
    {
        if (ItemPrototype const* proto = ObjectMgr::GetItemPrototype(itemId))
        {
            for(int idx = 0; idx < 5; ++idx)
            {
                if (proto->Spells[idx].SpellId == spellInfo->Id)
                {
                    cat    = proto->Spells[idx].SpellCategory;
                    rec    = proto->Spells[idx].SpellCooldown;
                    catrec = proto->Spells[idx].SpellCategoryCooldown;
                    break;
                }
            }
        }
    }

    // if no cooldown found above then base at DBC data
    if (rec < 0 && catrec < 0)
    {
        cat = spellInfo->Category;
        rec = spellInfo->RecoveryTime;
        catrec = spellInfo->CategoryRecoveryTime;
    }

    time_t curTime = time(NULL);

    time_t catrecTime;
    time_t recTime;

    // overwrite time for selected category
    if (infinityCooldown)
    {
        // use +MONTH as infinity mark for spell cooldown (will checked as MONTH/2 at save ans skipped)
        // but not allow ignore until reset or re-login
        catrecTime = catrec > 0 ? curTime+infinityCooldownDelay : 0;
        recTime    = rec    > 0 ? curTime+infinityCooldownDelay : catrecTime;
    }
    else
    {
        // shoot spells used equipped item cooldown values already assigned in GetAttackTime(RANGED_ATTACK)
        // prevent 0 cooldowns set by another way
        if (rec <= 0 && catrec <= 0 && (cat == 76 || (IsAutoRepeatRangedSpell(spellInfo) && spellInfo->Id != SPELL_ID_AUTOSHOT)))
            rec = GetAttackTime(RANGED_ATTACK);

        // Now we have cooldown data (if found any), time to apply mods
        if (rec > 0)
            ApplySpellMod(spellInfo->Id, SPELLMOD_COOLDOWN, rec, spell);

        if (catrec > 0)
            ApplySpellMod(spellInfo->Id, SPELLMOD_COOLDOWN, catrec, spell);

        // replace negative cooldowns by 0
        if (rec < 0) rec = 0;
        if (catrec < 0) catrec = 0;

        // no cooldown after applying spell mods
        if (rec == 0 && catrec == 0)
            return;

        catrecTime = catrec ? curTime+catrec/IN_MILLISECONDS : 0;
        recTime    = rec ? curTime+rec/IN_MILLISECONDS : catrecTime;
    }

    // self spell cooldown
    if (recTime > 0)
        AddSpellCooldown(spellInfo->Id, itemId, recTime);

    // category spells
    if (cat && catrec > 0)
    {
        SpellCategoryStore::const_iterator i_scstore = sSpellCategoryStore.find(cat);
        if (i_scstore != sSpellCategoryStore.end())
        {
            for(SpellCategorySet::const_iterator i_scset = i_scstore->second.begin(); i_scset != i_scstore->second.end(); ++i_scset)
            {
                if (*i_scset == spellInfo->Id)              // skip main spell, already handled above
                    continue;

                AddSpellCooldown(*i_scset, itemId, catrecTime);
            }
        }
    }
}

void Player::AddSpellCooldown(uint32 spellid, uint32 itemid, time_t end_time)
{
    SpellCooldown sc;
    sc.end = end_time;
    sc.itemid = itemid;
    m_spellCooldowns[spellid] = sc;
}

void Player::SendCooldownEvent(SpellEntry const *spellInfo, uint32 itemId, Spell* spell)
{
    // start cooldowns at server side, if any
    AddSpellAndCategoryCooldowns(spellInfo, itemId, spell);

    // Send activate cooldown timer (possible 0) at client side
    WorldPacket data(SMSG_COOLDOWN_EVENT, (4+8));
    data << uint32(spellInfo->Id);
    data << GetObjectGuid();
    SendDirectMessage(&data);
}

void Player::UpdatePotionCooldown(Spell* spell)
{
    // no potion used in combat or still in combat
    if(!m_lastPotionId || isInCombat())
        return;

    // Call not from spell cast, send cooldown event for item spells if no in combat
    if(!spell)
    {
        // spell/item pair let set proper cooldown (except nonexistent charged spell cooldown spellmods for potions)
        if(ItemPrototype const* proto = ObjectMgr::GetItemPrototype(m_lastPotionId))
            for(int idx = 0; idx < 5; ++idx)
                if(proto->Spells[idx].SpellId && proto->Spells[idx].SpellTrigger == ITEM_SPELLTRIGGER_ON_USE)
                    if(SpellEntry const* spellInfo = sSpellStore.LookupEntry(proto->Spells[idx].SpellId))
                        SendCooldownEvent(spellInfo,m_lastPotionId);
    }
    // from spell cases (m_lastPotionId set in Spell::SendSpellCooldown)
    else
        SendCooldownEvent(spell->m_spellInfo,m_lastPotionId,spell);

    m_lastPotionId = 0;
}

                                                            //slot to be excluded while counting
bool Player::EnchantmentFitsRequirements(uint32 enchantmentcondition, int8 slot)
{
    if(!enchantmentcondition)
        return true;

    SpellItemEnchantmentConditionEntry const *Condition = sSpellItemEnchantmentConditionStore.LookupEntry(enchantmentcondition);

    if(!Condition)
        return true;

    uint8 curcount[4] = {0, 0, 0, 0};

    //counting current equipped gem colors
    for(uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if(i == slot)
            continue;
        Item *pItem2 = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
        if(pItem2 && !pItem2->IsBroken() && pItem2->GetProto()->Socket[0].Color)
        {
            for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+3; ++enchant_slot)
            {
                uint32 enchant_id = pItem2->GetEnchantmentId(EnchantmentSlot(enchant_slot));
                if(!enchant_id)
                    continue;

                SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
                if(!enchantEntry)
                    continue;

                uint32 gemid = enchantEntry->GemID;
                if(!gemid)
                    continue;

                ItemPrototype const* gemProto = sItemStorage.LookupEntry<ItemPrototype>(gemid);
                if(!gemProto)
                    continue;

                GemPropertiesEntry const* gemProperty = sGemPropertiesStore.LookupEntry(gemProto->GemProperties);
                if(!gemProperty)
                    continue;

                uint8 GemColor = gemProperty->color;

                for(uint8 b = 0, tmpcolormask = 1; b < 4; b++, tmpcolormask <<= 1)
                {
                    if(tmpcolormask & GemColor)
                        ++curcount[b];
                }
            }
        }
    }

    bool activate = true;

    for(int i = 0; i < 5; ++i)
    {
        if(!Condition->Color[i])
            continue;

        uint32 _cur_gem = curcount[Condition->Color[i] - 1];

        // if have <CompareColor> use them as count, else use <value> from Condition
        uint32 _cmp_gem = Condition->CompareColor[i] ? curcount[Condition->CompareColor[i] - 1]: Condition->Value[i];

        switch(Condition->Comparator[i])
        {
            case 2:                                         // requires less <color> than (<value> || <comparecolor>) gems
                activate &= (_cur_gem < _cmp_gem) ? true : false;
                break;
            case 3:                                         // requires more <color> than (<value> || <comparecolor>) gems
                activate &= (_cur_gem > _cmp_gem) ? true : false;
                break;
            case 5:                                         // requires at least <color> than (<value> || <comparecolor>) gems
                activate &= (_cur_gem >= _cmp_gem) ? true : false;
                break;
        }
    }

    DEBUG_LOG("Checking Condition %u, there are %u Meta Gems, %u Red Gems, %u Yellow Gems and %u Blue Gems, Activate:%s", enchantmentcondition, curcount[0], curcount[1], curcount[2], curcount[3], activate ? "yes" : "no");

    return activate;
}

void Player::CorrectMetaGemEnchants(uint8 exceptslot, bool apply)
{
                                                            //cycle all equipped items
    for(uint32 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        //enchants for the slot being socketed are handled by Player::ApplyItemMods
        if(slot == exceptslot)
            continue;

        Item* pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, slot );

        if(!pItem || !pItem->GetProto()->Socket[0].Color)
            continue;

        for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+3; ++enchant_slot)
        {
            uint32 enchant_id = pItem->GetEnchantmentId(EnchantmentSlot(enchant_slot));
            if(!enchant_id)
                continue;

            SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
            if(!enchantEntry)
                continue;

            uint32 condition = enchantEntry->EnchantmentCondition;
            if(condition)
            {
                                                            //was enchant active with/without item?
                bool wasactive = EnchantmentFitsRequirements(condition, apply ? exceptslot : -1);
                                                            //should it now be?
                if(wasactive != EnchantmentFitsRequirements(condition, apply ? -1 : exceptslot))
                {
                    // ignore item gem conditions
                                                            //if state changed, (dis)apply enchant
                    ApplyEnchantment(pItem, EnchantmentSlot(enchant_slot), !wasactive, true, true);
                }
            }
        }
    }
}

                                                            //if false -> then toggled off if was on| if true -> toggled on if was off AND meets requirements
void Player::ToggleMetaGemsActive(uint8 exceptslot, bool apply)
{
    //cycle all equipped items
    for(int slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        //enchants for the slot being socketed are handled by WorldSession::HandleSocketOpcode(WorldPacket& recv_data)
        if(slot == exceptslot)
            continue;

        Item *pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, slot );

        if(!pItem || !pItem->GetProto()->Socket[0].Color)   //if item has no sockets or no item is equipped go to next item
            continue;

        //cycle all (gem)enchants
        for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+3; ++enchant_slot)
        {
            uint32 enchant_id = pItem->GetEnchantmentId(EnchantmentSlot(enchant_slot));
            if(!enchant_id)                                 //if no enchant go to next enchant(slot)
                continue;

            SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
            if(!enchantEntry)
                continue;

            //only metagems to be (de)activated, so only enchants with condition
            uint32 condition = enchantEntry->EnchantmentCondition;
            if(condition)
                ApplyEnchantment(pItem,EnchantmentSlot(enchant_slot), apply);
        }
    }
}

void Player::SetBattleGroundEntryPoint()
{
    // Taxi path store
    if (!m_taxi.empty())
    {
        m_bgData.mountSpell  = 0;
        m_bgData.taxiPath[0] = m_taxi.GetTaxiSource();
        m_bgData.taxiPath[1] = m_taxi.GetTaxiDestination();

        // On taxi we don't need check for dungeon
        m_bgData.joinPos = WorldLocation(GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
        m_bgData.m_needSave = true;
        return;
    }
    else
    {
        m_bgData.ClearTaxiPath();

        // Mount spell id storing
        if (IsMounted())
        {
            AuraList const& auras = GetAurasByType(SPELL_AURA_MOUNTED);
            if (!auras.empty())
                m_bgData.mountSpell = (*auras.begin())->GetId();
        }
        else
            m_bgData.mountSpell = 0;

        // If map is dungeon find linked graveyard
        if(GetMap()->IsDungeon())
        {
            if (const WorldSafeLocsEntry* entry = sObjectMgr.GetClosestGraveYard(GetPositionX(), GetPositionY(), GetPositionZ(), GetMapId(), GetTeam()))
            {
                m_bgData.joinPos = WorldLocation(entry->map_id, entry->x, entry->y, entry->z, 0.0f);
                m_bgData.m_needSave = true;
                return;
            }
            else
                sLog.outError("SetBattleGroundEntryPoint: Dungeon map %u has no linked graveyard, setting home location as entry point.", GetMapId());
        }
        // If new entry point is not BG or arena set it
        else if (!GetMap()->IsBattleGroundOrArena())
        {
            m_bgData.joinPos = WorldLocation(GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
            m_bgData.m_needSave = true;
            return;
        }
    }

    // In error cases use homebind position
    m_bgData.joinPos = WorldLocation(m_homebindMapId, m_homebindX, m_homebindY, m_homebindZ, 0.0f);
    m_bgData.m_needSave = true;
}

void Player::LeaveBattleground(bool teleportToEntryPoint)
{
    if(BattleGround *bg = GetBattleGround())
    {
        bg->RemovePlayerAtLeave(GetObjectGuid(), teleportToEntryPoint, true);

        // call after remove to be sure that player resurrected for correct cast
        if( bg->isBattleGround() && !isGameMaster() && sWorld.getConfig(CONFIG_BOOL_BATTLEGROUND_CAST_DESERTER) )
        {
            if( bg->GetStatus() == STATUS_IN_PROGRESS || bg->GetStatus() == STATUS_WAIT_JOIN )
            {
                //lets check if player was teleported from BG and schedule delayed Deserter spell cast
                if(IsBeingTeleportedFar())
                {
                    ScheduleDelayedOperation(DELAYED_SPELL_CAST_DESERTER);
                    return;
                }

                CastSpell(this, 26013, true);               // Deserter
            }
        }
    }
}

bool Player::CanJoinToBattleground() const
{
    // check Deserter debuff
    if(GetDummyAura(26013))
        return false;

    return true;
}

bool Player::CanReportAfkDueToLimit()
{
    // a player can complain about 15 people per 5 minutes
    if(m_bgData.bgAfkReportedCount++ >= 15)
        return false;

    return true;
}

///This player has been blamed to be inactive in a battleground
void Player::ReportedAfkBy(Player* reporter)
{
    BattleGround *bg = GetBattleGround();
    if(!bg || bg != reporter->GetBattleGround() || GetTeam() != reporter->GetTeam())
        return;

    // check if player has 'Idle' or 'Inactive' debuff
    if(m_bgData.bgAfkReporter.find(reporter->GetGUIDLow()) == m_bgData.bgAfkReporter.end() && !HasAura(43680, EFFECT_INDEX_0) && !HasAura(43681, EFFECT_INDEX_0) && reporter->CanReportAfkDueToLimit())
    {
        m_bgData.bgAfkReporter.insert(reporter->GetGUIDLow());
        // 3 players have to complain to apply debuff
        if(m_bgData.bgAfkReporter.size() >= 3)
        {
            // cast 'Idle' spell
            CastSpell(this, 43680, true);
            m_bgData.bgAfkReporter.clear();
        }
    }
}

bool Player::IsVisibleInGridForPlayer( Player* pl ) const
{
    // gamemaster in GM mode see all, including ghosts
    if (pl->isGameMaster() && GetSession()->GetSecurity() <= pl->GetSession()->GetSecurity())
        return true;

    // player see dead player/ghost from own group/raid
    if (IsInSameRaidWith(pl))
        return true;

    // Live player see live player or dead player with not realized corpse
    if (pl->isAlive() || pl->m_deathTimer > 0)
        return isAlive() || m_deathTimer > 0;

    // Ghost see other friendly ghosts, that's for sure
    if (!(isAlive() || m_deathTimer > 0) && IsFriendlyTo(pl))
        return true;

    // Dead player see live players near own corpse
    if (isAlive())
    {
        if (Corpse *corpse = pl->GetCorpse())
        {
            // 20 - aggro distance for same level, 25 - max additional distance if player level less that creature level
            if (corpse->IsWithinDistInMap(this, (20 + 25) * sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO)))
                return true;
        }
    }

    // and not see any other
    return false;
}

bool Player::IsVisibleGloballyFor( Player* u ) const
{
    if (!u)
        return false;

    // Always can see self
    if (u==this)
        return true;

    // Visible units, always are visible for all players
    if (GetVisibility() == VISIBILITY_ON)
        return true;

    // GMs are visible for higher gms (or players are visible for gms)
    if (u->GetSession()->GetSecurity() > SEC_PLAYER)
        return GetSession()->GetSecurity() <= u->GetSession()->GetSecurity();

    // non faction visibility non-breakable for non-GMs
    if (GetVisibility() == VISIBILITY_OFF)
        return false;

    // non-gm stealth/invisibility not hide from global player lists
    return true;
}

template<class T>
inline void BeforeVisibilityDestroy(T* /*t*/, Player* /*p*/)
{
}

template<>
inline void BeforeVisibilityDestroy<Creature>(Creature* t, Player* p)
{
    if (p->GetPetGuid() == t->GetObjectGuid() && ((Creature*)t)->IsPet())
        ((Pet*)t)->Unsummon(PET_SAVE_REAGENTS);
}

void Player::UpdateVisibilityOf(WorldObject const* viewPoint, WorldObject* target)
{
    if(HaveAtClient(target))
    {
        if(!target->isVisibleForInState(this, viewPoint, true))
        {
            ObjectGuid t_guid = target->GetObjectGuid();

            if (target->GetTypeId()==TYPEID_UNIT)
            {
                BeforeVisibilityDestroy<Creature>((Creature*)target,this);

                // at remove from map (destroy) show kill animation (in different out of range/stealth case)
                target->DestroyForPlayer(this, !target->IsInWorld() && ((Creature*)target)->isDead());
            }
            else
                target->DestroyForPlayer(this);

            m_clientGUIDs.erase(t_guid);

            DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "%s out of range for player %u. Distance = %f",t_guid.GetString().c_str(),GetGUIDLow(),GetDistance(target));
        }
    }
    else
    {
        if(target->isVisibleForInState(this, viewPoint, false))
        {
            target->SendCreateUpdateToPlayer(this);
            if(target->GetTypeId()!=TYPEID_GAMEOBJECT||!((GameObject*)target)->IsTransport())
                m_clientGUIDs.insert(target->GetObjectGuid());

            DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "Object %u (Type: %u) is visible now for player %u. Distance = %f",target->GetGUIDLow(),target->GetTypeId(),GetGUIDLow(),GetDistance(target));

            // target aura duration for caster show only if target exist at caster client
            // send data at target visibility change (adding to client)
            if(target!=this && target->isType(TYPEMASK_UNIT))
                SendAurasForTarget((Unit*)target);
        }
    }
}

template<class T>
inline void UpdateVisibilityOf_helper(ObjectGuidSet& s64, T* target)
{
    s64.insert(target->GetObjectGuid());
}

template<>
inline void UpdateVisibilityOf_helper(ObjectGuidSet& s64, GameObject* target)
{
    if(!target->IsTransport())
        s64.insert(target->GetObjectGuid());
}

template<class T>
void Player::UpdateVisibilityOf(WorldObject const* viewPoint, T* target, UpdateData& data, std::set<WorldObject*>& visibleNow)
{
    if(HaveAtClient(target))
    {
        if(!target->isVisibleForInState(this,viewPoint,true))
        {
            BeforeVisibilityDestroy<T>(target,this);

            ObjectGuid t_guid = target->GetObjectGuid();

            target->BuildOutOfRangeUpdateBlock(&data);
            m_clientGUIDs.erase(t_guid);

            DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "%s is out of range for %s. Distance = %f", t_guid.GetString().c_str(), GetGuidStr().c_str(), GetDistance(target));
        }
    }
    else
    {
        if(target->isVisibleForInState(this,viewPoint,false))
        {
            visibleNow.insert(target);
            target->BuildCreateUpdateBlockForPlayer(&data, this);
            UpdateVisibilityOf_helper(m_clientGUIDs,target);

            DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "%s is visible now for %s. Distance = %f", target->GetGuidStr().c_str(), GetGuidStr().c_str(), GetDistance(target));
        }
    }
}

template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, Player*        target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, Creature*      target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, Corpse*        target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, GameObject*    target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, DynamicObject* target, UpdateData& data, std::set<WorldObject*>& visibleNow);

void Player::SetPhaseMask(uint32 newPhaseMask, bool update)
{
    // GM-mode have mask PHASEMASK_ANYWHERE always
    if (isGameMaster())
        newPhaseMask = PHASEMASK_ANYWHERE;

    // phase auras normally not expected at BG but anyway better check
    if (BattleGround *bg = GetBattleGround())
        bg->EventPlayerDroppedFlag(this);

    Unit::SetPhaseMask(newPhaseMask, update);
    GetSession()->SendSetPhaseShift(GetPhaseMask());
}

void Player::InitPrimaryProfessions()
{
    uint32 maxProfs = GetSession()->GetSecurity() < AccountTypes(sWorld.getConfig(CONFIG_UINT32_TRADE_SKILL_GMIGNORE_MAX_PRIMARY_COUNT))
        ? sWorld.getConfig(CONFIG_UINT32_MAX_PRIMARY_TRADE_SKILL) : 10;
    SetFreePrimaryProfessions(maxProfs);
}

void Player::SendComboPoints()
{
    Unit *combotarget = ObjectAccessor::GetUnit(*this, m_comboTargetGuid);
    if (combotarget)
    {
        WorldPacket data(SMSG_UPDATE_COMBO_POINTS, combotarget->GetPackGUID().size()+1);
        data << combotarget->GetPackGUID();
        data << uint8(m_comboPoints);
        GetSession()->SendPacket(&data);
    }
    /*else
    {
        // can be NULL, and then points=0. Use unknown; to reset points of some sort?
        data << PackedGuid();
        data << uint8(0);
        GetSession()->SendPacket(&data);
    }*/
}

void Player::AddComboPoints(Unit* target, int8 count)
{
    if(!count)
        return;

    // without combo points lost (duration checked in aura)
    RemoveSpellsCausingAura(SPELL_AURA_RETAIN_COMBO_POINTS);

    if(target->GetObjectGuid() == m_comboTargetGuid)
    {
        m_comboPoints += count;
    }
    else
    {
        if (m_comboTargetGuid)
            if(Unit* target2 = ObjectAccessor::GetUnit(*this, m_comboTargetGuid))
                target2->RemoveComboPointHolder(GetGUIDLow());

        m_comboTargetGuid = target->GetObjectGuid();
        m_comboPoints = count;

        target->AddComboPointHolder(GetGUIDLow());
    }

    if (m_comboPoints > 5) m_comboPoints = 5;
    if (m_comboPoints < 0) m_comboPoints = 0;

    SendComboPoints();
}

void Player::ClearComboPoints()
{
    if (!m_comboTargetGuid)
        return;

    // without combopoints lost (duration checked in aura)
    RemoveSpellsCausingAura(SPELL_AURA_RETAIN_COMBO_POINTS);

    m_comboPoints = 0;

    SendComboPoints();

    if(Unit* target = ObjectAccessor::GetUnit(*this,m_comboTargetGuid))
        target->RemoveComboPointHolder(GetGUIDLow());

    m_comboTargetGuid.Clear();
}

void Player::SetGroup(Group *group, int8 subgroup)
{
    if(group == NULL)
        m_group.unlink();
    else
    {
        // never use SetGroup without a subgroup unless you specify NULL for group
        MANGOS_ASSERT(subgroup >= 0);
        m_group.link(group, this);
        m_group.setSubGroup((uint8)subgroup);
    }
}

void Player::SendInitialPacketsBeforeAddToMap()
{
    GetSocial()->SendSocialList();

    // Homebind
    WorldPacket data(SMSG_BINDPOINTUPDATE, 5*4);
    data << m_homebindX << m_homebindY << m_homebindZ;
    data << (uint32) m_homebindMapId;
    data << (uint32) m_homebindAreaId;
    GetSession()->SendPacket(&data);

    // SMSG_SET_PROFICIENCY
    // SMSG_SET_PCT_SPELL_MODIFIER
    // SMSG_SET_FLAT_SPELL_MODIFIER

    SendTalentsInfoData(false);

    data.Initialize(SMSG_INSTANCE_DIFFICULTY, 4+4);
    data << uint32(GetMap()->GetDifficulty());
    data << uint32(0);
    GetSession()->SendPacket(&data);

    SendInitialSpells();

    data.Initialize(SMSG_SEND_UNLEARN_SPELLS, 4);
    data << uint32(0);                                      // count, for(count) uint32;
    GetSession()->SendPacket(&data);

    SendInitialActionButtons();
    m_reputationMgr.SendInitialReputations();

    if(!isAlive())
        SendCorpseReclaimDelay(true);

    SendInitWorldStates(GetZoneId(), GetAreaId());

    SendEquipmentSetList();

    m_achievementMgr.SendAllAchievementData();

    data.Initialize(SMSG_LOGIN_SETTIMESPEED, 4 + 4 + 4);
    data << uint32(secsToTimeBitFields(sWorld.GetGameTime()));
    data << (float)0.01666667f;                             // game speed
    data << uint32(0);                                      // added in 3.1.2
    GetSession()->SendPacket( &data );

    // SMSG_TALENTS_INFO x 2 for pet (unspent points and talents in separate packets...)
    // SMSG_PET_GUIDS
    // SMSG_POWER_UPDATE

    // set fly flag if in fly form or taxi flight to prevent visually drop at ground in showup moment
    if (IsFreeFlying() || IsTaxiFlying())
        m_movementInfo.AddMovementFlag(MOVEFLAG_FLYING);

    SetMover(this);
}

void Player::SendInitialPacketsAfterAddToMap()
{
    // update zone
    uint32 newzone, newarea;
    GetZoneAndAreaId(newzone,newarea);
    UpdateZone(newzone,newarea);                            // also call SendInitWorldStates();

    ResetTimeSync();
    SendTimeSync();

    CastSpell(this, 836, true);                             // LOGINEFFECT

    // set some aura effects that send packet to player client after add player to map
    // SendMessageToSet not send it to player not it map, only for aura that not changed anything at re-apply
    // same auras state lost at far teleport, send it one more time in this case also
    static const AuraType auratypes[] =
    {
        SPELL_AURA_MOD_FEAR,     SPELL_AURA_TRANSFORM,                 SPELL_AURA_WATER_WALK,
        SPELL_AURA_FEATHER_FALL, SPELL_AURA_HOVER,                     SPELL_AURA_SAFE_FALL,
        SPELL_AURA_FLY,          SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED,  SPELL_AURA_NONE
    };
    for(AuraType const* itr = &auratypes[0]; itr && itr[0] != SPELL_AURA_NONE; ++itr)
    {
        Unit::AuraList const& auraList = GetAurasByType(*itr);
        if(!auraList.empty())
            auraList.front()->ApplyModifier(true,true);
    }

    if(HasAuraType(SPELL_AURA_MOD_STUN))
        SetMovement(MOVE_ROOT);

    // manual send package (have code in ApplyModifier(true,true); that don't must be re-applied.
    if(HasAuraType(SPELL_AURA_MOD_ROOT))
    {
        WorldPacket data2(SMSG_FORCE_MOVE_ROOT, 10);
        data2 << GetPackGUID();
        data2 << (uint32)2;
        SendMessageToSet(&data2,true);
    }

    SendAurasForTarget(this);
    SendEnchantmentDurations();                             // must be after add to map
    SendItemDurations();                                    // must be after add to map
}

void Player::SendUpdateToOutOfRangeGroupMembers()
{
    if (m_groupUpdateMask == GROUP_UPDATE_FLAG_NONE)
        return;
    if(Group* group = GetGroup())
        group->UpdatePlayerOutOfRange(this);

    m_groupUpdateMask = GROUP_UPDATE_FLAG_NONE;
    m_auraUpdateMask = 0;
    if(Pet *pet = GetPet())
        pet->ResetAuraUpdateMask();
}

void Player::SendTransferAborted(uint32 mapid, uint8 reason, uint8 arg)
{
    WorldPacket data(SMSG_TRANSFER_ABORTED, 4+2);
    data << uint32(mapid);
    data << uint8(reason);                                 // transfer abort reason
    switch(reason)
    {
        case TRANSFER_ABORT_INSUF_EXPAN_LVL:
        case TRANSFER_ABORT_DIFFICULTY:
        case TRANSFER_ABORT_UNIQUE_MESSAGE:
            data << uint8(arg);
            break;
    }
    GetSession()->SendPacket(&data);
}

void Player::SendInstanceResetWarning( uint32 mapid, Difficulty difficulty, uint32 time )
{
    // type of warning, based on the time remaining until reset
    uint32 type;
    if(time > 3600)
        type = RAID_INSTANCE_WELCOME;
    else if(time > 900 && time <= 3600)
        type = RAID_INSTANCE_WARNING_HOURS;
    else if(time > 300 && time <= 900)
        type = RAID_INSTANCE_WARNING_MIN;
    else
        type = RAID_INSTANCE_WARNING_MIN_SOON;

    WorldPacket data(SMSG_RAID_INSTANCE_MESSAGE, 4+4+4+4);
    data << uint32(type);
    data << uint32(mapid);
    data << uint32(difficulty);                             // difficulty
    data << uint32(time);
    if(type == RAID_INSTANCE_WELCOME)
    {
        data << uint8(0);                                   // is your (1)
        data << uint8(0);                                   // is extended (1), ignored if prev field is 0
    }
    GetSession()->SendPacket(&data);
}

void Player::ApplyEquipCooldown( Item * pItem )
{
    if (pItem->GetProto()->Flags & ITEM_FLAG_NO_EQUIP_COOLDOWN)
        return;

    for(int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        _Spell const& spellData = pItem->GetProto()->Spells[i];

        // no spell
        if( !spellData.SpellId )
            continue;

        // wrong triggering type (note: ITEM_SPELLTRIGGER_ON_NO_DELAY_USE not have cooldown)
        if( spellData.SpellTrigger != ITEM_SPELLTRIGGER_ON_USE )
            continue;

        AddSpellCooldown(spellData.SpellId, pItem->GetEntry(), time(NULL) + 30);

        WorldPacket data(SMSG_ITEM_COOLDOWN, 12);
        data << ObjectGuid(pItem->GetObjectGuid());
        data << uint32(spellData.SpellId);
        GetSession()->SendPacket(&data);
    }
}

void Player::resetSpells()
{
    // not need after this call
    if(HasAtLoginFlag(AT_LOGIN_RESET_SPELLS))
        RemoveAtLoginFlag(AT_LOGIN_RESET_SPELLS,true);

    // make full copy of map (spells removed and marked as deleted at another spell remove
    // and we can't use original map for safe iterative with visit each spell at loop end
    PlayerSpellMap smap = GetSpellMap();

    for(PlayerSpellMap::const_iterator iter = smap.begin();iter != smap.end(); ++iter)
        removeSpell(iter->first,false,false);               // only iter->first can be accessed, object by iter->second can be deleted already

    learnDefaultSpells();
    learnQuestRewardedSpells();
}

void Player::learnDefaultSpells()
{
    // learn default race/class spells
    PlayerInfo const *info = sObjectMgr.GetPlayerInfo(getRace(),getClass());
    for (PlayerCreateInfoSpells::const_iterator itr = info->spell.begin(); itr!=info->spell.end(); ++itr)
    {
        uint32 tspell = *itr;
        DEBUG_LOG("PLAYER (Class: %u Race: %u): Adding initial spell, id = %u",uint32(getClass()),uint32(getRace()), tspell);
        if(!IsInWorld())                                    // will send in INITIAL_SPELLS in list anyway at map add
            addSpell(tspell, true, true, true, false);
        else                                                // but send in normal spell in game learn case
            learnSpell(tspell, true);
    }
}

void Player::learnQuestRewardedSpells(Quest const* quest)
{
    uint32 spell_id = quest->GetRewSpellCast();

    // skip quests without rewarded spell
    if( !spell_id )
        return;

    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spell_id);
    if(!spellInfo)
        return;

    // check learned spells state
    bool found = false;
    for(int i=0; i < MAX_EFFECT_INDEX; ++i)
    {
        if(spellInfo->Effect[i] == SPELL_EFFECT_LEARN_SPELL && !HasSpell(spellInfo->EffectTriggerSpell[i]))
        {
            found = true;
            break;
        }
    }

    // skip quests with not teaching spell or already known spell
    if(!found)
        return;

    // prevent learn non first rank unknown profession and second specialization for same profession)
    uint32 learned_0 = spellInfo->EffectTriggerSpell[EFFECT_INDEX_0];
    if( sSpellMgr.GetSpellRank(learned_0) > 1 && !HasSpell(learned_0) )
    {
        // not have first rank learned (unlearned prof?)
        uint32 first_spell = sSpellMgr.GetFirstSpellInChain(learned_0);
        if( !HasSpell(first_spell) )
            return;

        SpellEntry const *learnedInfo = sSpellStore.LookupEntry(learned_0);
        if(!learnedInfo)
            return;

        // specialization
        if (learnedInfo->Effect[EFFECT_INDEX_0] == SPELL_EFFECT_TRADE_SKILL && learnedInfo->Effect[EFFECT_INDEX_1] == 0)
        {
            // search other specialization for same prof
            for(PlayerSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
            {
                if(itr->second.state == PLAYERSPELL_REMOVED || itr->first==learned_0)
                    continue;

                SpellEntry const *itrInfo = sSpellStore.LookupEntry(itr->first);
                if(!itrInfo)
                    return;

                // compare only specializations
                if (itrInfo->Effect[EFFECT_INDEX_0] != SPELL_EFFECT_TRADE_SKILL || itrInfo->Effect[EFFECT_INDEX_1] != 0)
                    continue;

                // compare same chain spells
                if(sSpellMgr.GetFirstSpellInChain(itr->first) != first_spell)
                    continue;

                // now we have 2 specialization, learn possible only if found is lesser specialization rank
                if(!sSpellMgr.IsHighRankOfSpell(learned_0,itr->first))
                    return;
            }
        }
    }

    CastSpell( this, spell_id, true);
}

void Player::learnQuestRewardedSpells()
{
    // learn spells received from quest completing
    for(QuestStatusMap::const_iterator itr = mQuestStatus.begin(); itr != mQuestStatus.end(); ++itr)
    {
        // skip no rewarded quests
        if(!itr->second.m_rewarded)
            continue;

        Quest const* quest = sObjectMgr.GetQuestTemplate(itr->first);
        if( !quest )
            continue;

        learnQuestRewardedSpells(quest);
    }
}

void Player::learnSkillRewardedSpells(uint32 skill_id, uint32 skill_value )
{
    uint32 raceMask  = getRaceMask();
    uint32 classMask = getClassMask();
    for (uint32 j = 0; j<sSkillLineAbilityStore.GetNumRows(); ++j)
    {
        SkillLineAbilityEntry const *pAbility = sSkillLineAbilityStore.LookupEntry(j);
        if (!pAbility || pAbility->skillId!=skill_id || pAbility->learnOnGetSkill != ABILITY_LEARNED_ON_GET_PROFESSION_SKILL)
            continue;
        // Check race if set
        if (pAbility->racemask && !(pAbility->racemask & raceMask))
            continue;
        // Check class if set
        if (pAbility->classmask && !(pAbility->classmask & classMask))
            continue;

        if (sSpellStore.LookupEntry(pAbility->spellId))
        {
            // need unlearn spell
            if (skill_value < pAbility->req_skill_value)
                removeSpell(pAbility->spellId);
            // need learn
            else if (!IsInWorld())
                addSpell(pAbility->spellId, true, true, true, false);
            else
                learnSpell(pAbility->spellId, true);
        }
    }
}

void Player::SendAurasForTarget(Unit *target)
{
    WorldPacket data(SMSG_AURA_UPDATE_ALL);
    data << target->GetPackGUID();

    Unit::VisibleAuraMap const& visibleAuras = target->GetVisibleAuras();
    for (Unit::VisibleAuraMap::const_iterator itr = visibleAuras.begin(); itr != visibleAuras.end(); ++itr)
    {
        SpellAuraHolderConstBounds bounds = target->GetSpellAuraHolderBounds(itr->second);
        for (SpellAuraHolderMap::const_iterator iter = bounds.first; iter != bounds.second; ++iter)
            iter->second->BuildUpdatePacket(data);
    }

    GetSession()->SendPacket(&data);
}

void Player::SetDailyQuestStatus( uint32 quest_id )
{
    for(uint32 quest_daily_idx = 0; quest_daily_idx < PLAYER_MAX_DAILY_QUESTS; ++quest_daily_idx)
    {
        if(!GetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx))
        {
            SetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx,quest_id);
            m_DailyQuestChanged = true;
            break;
        }
    }
}

void Player::SetWeeklyQuestStatus( uint32 quest_id )
{
    m_weeklyquests.insert(quest_id);
    m_WeeklyQuestChanged = true;
}

void Player::SetMonthlyQuestStatus(uint32 quest_id)
{
    m_monthlyquests.insert(quest_id);
    m_MonthlyQuestChanged = true;
}

void Player::ResetDailyQuestStatus()
{
    for(uint32 quest_daily_idx = 0; quest_daily_idx < PLAYER_MAX_DAILY_QUESTS; ++quest_daily_idx)
        SetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1+quest_daily_idx,0);

    // DB data deleted in caller
    m_DailyQuestChanged = false;
}

void Player::ResetWeeklyQuestStatus()
{
    if (m_weeklyquests.empty())
        return;

    m_weeklyquests.clear();
    // DB data deleted in caller
    m_WeeklyQuestChanged = false;
}

void Player::ResetMonthlyQuestStatus()
{
    if (m_monthlyquests.empty())
        return;

    m_monthlyquests.clear();
    // DB data deleted in caller
    m_MonthlyQuestChanged = false;
}

BattleGround* Player::GetBattleGround() const
{
    if(GetBattleGroundId()==0)
        return NULL;

    return sBattleGroundMgr.GetBattleGround(GetBattleGroundId(), m_bgData.bgTypeID);
}

bool Player::InArena() const
{
    BattleGround *bg = GetBattleGround();
    if(!bg || !bg->isArena())
        return false;

    return true;
}

bool Player::GetBGAccessByLevel(BattleGroundTypeId bgTypeId) const
{
    // get a template bg instead of running one
    BattleGround *bg = sBattleGroundMgr.GetBattleGroundTemplate(bgTypeId);
    if(!bg)
        return false;

    // limit check leel to dbc compatible level range
    uint32 level = getLevel();
    if (level > DEFAULT_MAX_LEVEL)
        level = DEFAULT_MAX_LEVEL;

    if(level < bg->GetMinLevel() || level > bg->GetMaxLevel())
        return false;

    return true;
}

float Player::GetReputationPriceDiscount( Creature const* pCreature ) const
{
    FactionTemplateEntry const* vendor_faction = pCreature->getFactionTemplateEntry();
    if(!vendor_faction || !vendor_faction->faction)
        return 1.0f;

    ReputationRank rank = GetReputationRank(vendor_faction->faction);
    if(rank <= REP_NEUTRAL)
        return 1.0f;

    return 1.0f - 0.05f* (rank - REP_NEUTRAL);
}

/**
 * Check spell availability for training base at SkillLineAbility/SkillRaceClassInfo data.
 * Checked allowed race/class and dependent from race/class allowed min level
 *
 * @param spell_id  checked spell id
 * @param pReqlevel if arg provided then function work in view mode (level check not applied but detected minlevel returned to var by arg pointer.
                    if arg not provided then considered train action mode and level checked
 * @return          true if spell available for show in trainer list (with skip level check) or training.
 */
bool Player::IsSpellFitByClassAndRace(uint32 spell_id, uint32* pReqlevel /*= NULL*/) const
{
    uint32 racemask  = getRaceMask();
    uint32 classmask = getClassMask();

    SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spell_id);
    if (bounds.first==bounds.second)
        return true;

    for (SkillLineAbilityMap::const_iterator _spell_idx = bounds.first; _spell_idx != bounds.second; ++_spell_idx)
    {
        SkillLineAbilityEntry const* abilityEntry = _spell_idx->second;
        // skip wrong race skills
        if (abilityEntry->racemask && (abilityEntry->racemask & racemask) == 0)
            continue;

        // skip wrong class skills
        if (abilityEntry->classmask && (abilityEntry->classmask & classmask) == 0)
            continue;

        SkillRaceClassInfoMapBounds bounds = sSpellMgr.GetSkillRaceClassInfoMapBounds(abilityEntry->skillId);
        for (SkillRaceClassInfoMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
        {
            SkillRaceClassInfoEntry const* skillRCEntry = itr->second;
            if ((skillRCEntry->raceMask & racemask) && (skillRCEntry->classMask & classmask))
            {
                if (skillRCEntry->flags & ABILITY_SKILL_NONTRAINABLE)
                    return false;

                if (pReqlevel)                              // show trainers list case
                {
                    if (skillRCEntry->reqLevel)
                    {
                        *pReqlevel = skillRCEntry->reqLevel;
                        return true;
                    }
                }
                else                                        // check availble case at train
                {
                    if (skillRCEntry->reqLevel && getLevel() < skillRCEntry->reqLevel)
                        return false;
                }
            }
        }

        return true;
    }

    return false;
}

bool Player::HasQuestForGO(int32 GOId) const
{
    for( int i = 0; i < MAX_QUEST_LOG_SIZE; ++i )
    {
        uint32 questid = GetQuestSlotQuestId(i);
        if ( questid == 0 )
            continue;

        QuestStatusMap::const_iterator qs_itr = mQuestStatus.find(questid);
        if(qs_itr == mQuestStatus.end())
            continue;

        QuestStatusData const& qs = qs_itr->second;

        if (qs.m_status == QUEST_STATUS_INCOMPLETE)
        {
            Quest const* qinfo = sObjectMgr.GetQuestTemplate(questid);
            if(!qinfo)
                continue;

            if(GetGroup() && GetGroup()->isRaidGroup() && !qinfo->IsAllowedInRaid())
                continue;

            for (int j = 0; j < QUEST_OBJECTIVES_COUNT; ++j)
            {
                if (qinfo->ReqCreatureOrGOId[j]>=0)         //skip non GO case
                    continue;

                if((-1)*GOId == qinfo->ReqCreatureOrGOId[j] && qs.m_creatureOrGOcount[j] < qinfo->ReqCreatureOrGOCount[j])
                    return true;
            }
        }
    }
    return false;
}

void Player::UpdateForQuestWorldObjects()
{
    if(m_clientGUIDs.empty())
        return;

    UpdateData udata;
    WorldPacket packet;
    for(ObjectGuidSet::const_iterator itr=m_clientGUIDs.begin(); itr!=m_clientGUIDs.end(); ++itr)
    {
        if (itr->IsGameObject())
        {
            if (GameObject *obj = GetMap()->GetGameObject(*itr))
                obj->BuildValuesUpdateBlockForPlayer(&udata,this);
        }
        else if (itr->IsCreatureOrVehicle())
        {
            Creature *obj = GetMap()->GetAnyTypeCreature(*itr);
            if(!obj)
                continue;

            // check if this unit requires quest specific flags
            if(!obj->HasFlag(UNIT_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK))
                continue;

            SpellClickInfoMapBounds clickPair = sObjectMgr.GetSpellClickInfoMapBounds(obj->GetEntry());
            for(SpellClickInfoMap::const_iterator _itr = clickPair.first; _itr != clickPair.second; ++_itr)
            {
                if(_itr->second.questStart || _itr->second.questEnd)
                {
                    obj->BuildCreateUpdateBlockForPlayer(&udata,this);
                    break;
                }
            }
        }
    }
    udata.BuildPacket(&packet);
    GetSession()->SendPacket(&packet);
}

void Player::SummonIfPossible(bool agree)
{
    if(!agree)
    {
        m_summon_expire = 0;
        return;
    }

    // expire and auto declined
    if(m_summon_expire < time(NULL))
        return;

    // stop taxi flight at summon
    if(IsTaxiFlying())
    {
        GetMotionMaster()->MovementExpired();
        m_taxi.ClearTaxiDestinations();
    }

    // drop flag at summon
    // this code can be reached only when GM is summoning player who carries flag, because player should be immune to summoning spells when he carries flag
    if(BattleGround *bg = GetBattleGround())
        bg->EventPlayerDroppedFlag(this);

    m_summon_expire = 0;

    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS, 1);

    TeleportTo(m_summon_mapid, m_summon_x, m_summon_y, m_summon_z,GetOrientation());
}

void Player::RemoveItemDurations( Item *item )
{
    for(ItemDurationList::iterator itr = m_itemDuration.begin();itr != m_itemDuration.end(); ++itr)
    {
        if(*itr==item)
        {
            m_itemDuration.erase(itr);
            break;
        }
    }
}

void Player::AddItemDurations( Item *item )
{
    if(item->GetUInt32Value(ITEM_FIELD_DURATION))
    {
        m_itemDuration.push_back(item);
        item->SendTimeUpdate(this);
    }
}

void Player::AutoUnequipOffhandIfNeed()
{
    Item *offItem = GetItemByPos( INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND );
    if(!offItem)
        return;

    // need unequip offhand for 2h-weapon without TitanGrip (in any from hands)
    if ((CanDualWield() || offItem->GetProto()->InventoryType == INVTYPE_SHIELD || offItem->GetProto()->InventoryType == INVTYPE_HOLDABLE) &&
        (CanTitanGrip() || (offItem->GetProto()->InventoryType != INVTYPE_2HWEAPON && !IsTwoHandUsed())))
        return;

    ItemPosCountVec off_dest;
    uint8 off_msg = CanStoreItem( NULL_BAG, NULL_SLOT, off_dest, offItem, false );
    if( off_msg == EQUIP_ERR_OK )
    {
        RemoveItem(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND, true);
        StoreItem( off_dest, offItem, true );
    }
    else
    {
        MoveItemFromInventory(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND, true);
        CharacterDatabase.BeginTransaction();
        offItem->DeleteFromInventoryDB();                   // deletes item from character's inventory
        offItem->SaveToDB();                                // recursive and not have transaction guard into self, item not in inventory and can be save standalone
        CharacterDatabase.CommitTransaction();

        std::string subject = GetSession()->GetMangosString(LANG_NOT_EQUIPPED_ITEM);
        MailDraft(subject, "There's were problems with equipping this item.").AddItem(offItem).SendMailTo(this, MailSender(this, MAIL_STATIONERY_GM), MAIL_CHECK_MASK_COPIED);
    }
}

bool Player::HasItemFitToSpellReqirements(SpellEntry const* spellInfo, Item const* ignoreItem)
{
    if(spellInfo->EquippedItemClass < 0)
        return true;

    // scan other equipped items for same requirements (mostly 2 daggers/etc)
    // for optimize check 2 used cases only
    switch(spellInfo->EquippedItemClass)
    {
        case ITEM_CLASS_WEAPON:
        {
            for(int i= EQUIPMENT_SLOT_MAINHAND; i < EQUIPMENT_SLOT_TABARD; ++i)
                if(Item *item = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
                    if(item!=ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                        return true;
            break;
        }
        case ITEM_CLASS_ARMOR:
        {
            // tabard not have dependent spells
            for(int i= EQUIPMENT_SLOT_START; i< EQUIPMENT_SLOT_MAINHAND; ++i)
                if(Item *item = GetItemByPos( INVENTORY_SLOT_BAG_0, i ))
                    if(item!=ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                        return true;

            // shields can be equipped to offhand slot
            if(Item *item = GetItemByPos( INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
                if(item!=ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                    return true;

            // ranged slot can have some armor subclasses
            if(Item *item = GetItemByPos( INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED))
                if(item!=ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                    return true;

            break;
        }
        default:
            sLog.outError("HasItemFitToSpellReqirements: Not handled spell requirement for item class %u",spellInfo->EquippedItemClass);
            break;
    }

    return false;
}

bool Player::CanNoReagentCast(SpellEntry const* spellInfo) const
{
    // don't take reagents for spells with SPELL_ATTR_EX5_NO_REAGENT_WHILE_PREP
    if (spellInfo->AttributesEx5 & SPELL_ATTR_EX5_NO_REAGENT_WHILE_PREP &&
        HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION))
        return true;

    // Check no reagent use mask
    uint64 noReagentMask_0_1 = GetUInt64Value(PLAYER_NO_REAGENT_COST_1);
    uint32 noReagentMask_2   = GetUInt32Value(PLAYER_NO_REAGENT_COST_1+2);
    if (spellInfo->IsFitToFamilyMask(noReagentMask_0_1, noReagentMask_2))
        return true;

    return false;
}

void Player::RemoveItemDependentAurasAndCasts( Item * pItem )
{
    SpellAuraHolderMap& auras = GetSpellAuraHolderMap();
    for(SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); )
    {
        SpellAuraHolder* holder = itr->second;

        // skip passive (passive item dependent spells work in another way) and not self applied auras
        SpellEntry const* spellInfo = holder->GetSpellProto();
        if(holder->IsPassive() ||  holder->GetCasterGuid() != GetObjectGuid())
        {
            ++itr;
            continue;
        }

        // skip if not item dependent or have alternative item
        if(HasItemFitToSpellReqirements(spellInfo,pItem))
        {
            ++itr;
            continue;
        }

        // no alt item, remove aura, restart check
        RemoveAurasDueToSpell(holder->GetId());
        itr = auras.begin();
    }

    // currently casted spells can be dependent from item
    for (uint32 i = 0; i < CURRENT_MAX_SPELL; ++i)
        if (Spell* spell = GetCurrentSpell(CurrentSpellTypes(i)))
            if (spell->getState()!=SPELL_STATE_DELAYED && !HasItemFitToSpellReqirements(spell->m_spellInfo,pItem) )
                InterruptSpell(CurrentSpellTypes(i));
}

uint32 Player::GetResurrectionSpellId()
{
    // search priceless resurrection possibilities
    uint32 prio = 0;
    uint32 spell_id = 0;
    AuraList const& dummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
    for(AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
    {
        // Soulstone Resurrection                           // prio: 3 (max, non death persistent)
        if( prio < 2 && (*itr)->GetSpellProto()->SpellVisual[0] == 99 && (*itr)->GetSpellProto()->SpellIconID == 92 )
        {
            switch((*itr)->GetId())
            {
                case 20707: spell_id =  3026; break;        // rank 1
                case 20762: spell_id = 20758; break;        // rank 2
                case 20763: spell_id = 20759; break;        // rank 3
                case 20764: spell_id = 20760; break;        // rank 4
                case 20765: spell_id = 20761; break;        // rank 5
                case 27239: spell_id = 27240; break;        // rank 6
                case 47883: spell_id = 47882; break;        // rank 7
                default:
                    sLog.outError("Unhandled spell %u: S.Resurrection",(*itr)->GetId());
                    continue;
            }

            prio = 3;
        }
        // Twisting Nether                                  // prio: 2 (max)
        else if((*itr)->GetId()==23701 && roll_chance_i(10))
        {
            prio = 2;
            spell_id = 23700;
        }
    }

    // Reincarnation (passive spell)                        // prio: 1
    // Glyph of Renewed Life remove reagent requiremnnt
    if (prio < 1 && HasSpell(20608) && !HasSpellCooldown(21169) && (HasItemCount(17030,1) || HasAura(58059, EFFECT_INDEX_0)))
        spell_id = 21169;

    return spell_id;
}

// Used in triggers for check "Only to targets that grant experience or honor" req
bool Player::isHonorOrXPTarget(Unit* pVictim) const
{
    uint32 v_level = pVictim->getLevel();
    uint32 k_grey  = MaNGOS::XP::GetGrayLevel(getLevel());

    // Victim level less gray level
    if(v_level<=k_grey)
        return false;

    if(pVictim->GetTypeId() == TYPEID_UNIT)
    {
        if (((Creature*)pVictim)->IsTotem() ||
            ((Creature*)pVictim)->IsPet() ||
            ((Creature*)pVictim)->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NO_XP_AT_KILL)
                return false;
    }
    return true;
}

void Player::RewardSinglePlayerAtKill(Unit* pVictim)
{
    bool PvP = pVictim->isCharmedOwnedByPlayerOrPlayer();
    uint32 xp = PvP ? 0 : MaNGOS::XP::Gain(this, pVictim);

    // honor can be in PvP and !PvP (racial leader) cases
    RewardHonor(pVictim,1);

    // xp and reputation only in !PvP case
    if(!PvP)
    {
        RewardReputation(pVictim,1);
        GiveXP(xp, pVictim);

        if(Pet* pet = GetPet())
            pet->GivePetXP(xp);

        // normal creature (not pet/etc) can be only in !PvP case
        if(pVictim->GetTypeId()==TYPEID_UNIT)
            if(CreatureInfo const* normalInfo = ObjectMgr::GetCreatureTemplate(pVictim->GetEntry()))
                KilledMonster(normalInfo, pVictim->GetObjectGuid());
    }
}

void Player::RewardPlayerAndGroupAtEvent(uint32 creature_id, WorldObject* pRewardSource)
{
    ObjectGuid creature_guid = pRewardSource->GetTypeId()==TYPEID_UNIT ? pRewardSource->GetObjectGuid() : ObjectGuid();

    // prepare data for near group iteration
    if (Group *pGroup = GetGroup())
    {
        for (GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* pGroupGuy = itr->getSource();
            if (!pGroupGuy)
                continue;

            if (!pGroupGuy->IsAtGroupRewardDistance(pRewardSource))
                continue;                               // member (alive or dead) or his corpse at req. distance

            // quest objectives updated only for alive group member or dead but with not released body
            if (pGroupGuy->isAlive()|| !pGroupGuy->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
                pGroupGuy->KilledMonsterCredit(creature_id, creature_guid);
        }
    }
    else                                                    // if (!pGroup)
        KilledMonsterCredit(creature_id, creature_guid);
}

void Player::RewardPlayerAndGroupAtCast(WorldObject* pRewardSource, uint32 spellid)
{
    // prepare data for near group iteration
    if(Group *pGroup = GetGroup())
    {
        for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* pGroupGuy = itr->getSource();
            if(!pGroupGuy)
                continue;

            if(!pGroupGuy->IsAtGroupRewardDistance(pRewardSource))
                continue;                               // member (alive or dead) or his corpse at req. distance

            // quest objectives updated only for alive group member or dead but with not released body
            if(pGroupGuy->isAlive()|| !pGroupGuy->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
                pGroupGuy->CastedCreatureOrGO(pRewardSource->GetEntry(), pRewardSource->GetObjectGuid(), spellid, pGroupGuy == this);
        }
    }
    else                                                    // if (!pGroup)
        CastedCreatureOrGO(pRewardSource->GetEntry(), pRewardSource->GetObjectGuid(), spellid);
}

bool Player::IsAtGroupRewardDistance(WorldObject const* pRewardSource) const
{
    if (pRewardSource->IsWithinDistInMap(this,sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE)))
        return true;

    if (isAlive())
        return false;

    Corpse* corpse = GetCorpse();
    if (!corpse)
        return false;

    return pRewardSource->IsWithinDistInMap(corpse,sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE));
}

uint32 Player::GetBaseWeaponSkillValue (WeaponAttackType attType) const
{
    Item* item = GetWeaponForAttack(attType,true,true);

    // unarmed only with base attack
    if(attType != BASE_ATTACK && !item)
        return 0;

    // weapon skill or (unarmed for base attack)
    uint32  skill = item ? item->GetSkill() : uint32(SKILL_UNARMED);
    return GetBaseSkillValue(skill);
}

void Player::ResurectUsingRequestData()
{
    /// Teleport before resurrecting by player, otherwise the player might get attacked from creatures near his corpse
    if (m_resurrectGuid.IsPlayer())
        TeleportTo(m_resurrectMap, m_resurrectX, m_resurrectY, m_resurrectZ, GetOrientation());

    //we cannot resurrect player when we triggered far teleport
    //player will be resurrected upon teleportation
    if(IsBeingTeleportedFar())
    {
        ScheduleDelayedOperation(DELAYED_RESURRECT_PLAYER);
        return;
    }

    ResurrectPlayer(0.0f,false);

    if(GetMaxHealth() > m_resurrectHealth)
        SetHealth( m_resurrectHealth );
    else
        SetHealth( GetMaxHealth() );

    if(GetMaxPower(POWER_MANA) > m_resurrectMana)
        SetPower(POWER_MANA, m_resurrectMana );
    else
        SetPower(POWER_MANA, GetMaxPower(POWER_MANA) );

    SetPower(POWER_RAGE, 0 );

    SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY) );

    SpawnCorpseBones();
}

void Player::SetClientControl(Unit* target, uint8 allowMove)
{
    WorldPacket data(SMSG_CLIENT_CONTROL_UPDATE, target->GetPackGUID().size()+1);
    data << target->GetPackGUID();
    data << uint8(allowMove);
    GetSession()->SendPacket(&data);
}

void Player::UpdateZoneDependentAuras()
{
    // Some spells applied at enter into zone (with subzones), aura removed in UpdateAreaDependentAuras that called always at zone->area update
    SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAreaMapBounds(m_zoneUpdateId);
    for(SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
        if(itr->second->autocast && itr->second->IsFitToRequirements(this, m_zoneUpdateId, 0))
            if (!HasAura(itr->second->spellId, EFFECT_INDEX_0))
                CastSpell(this,itr->second->spellId,true);
}

void Player::UpdateAreaDependentAuras()
{
    // remove auras from spells with area limitations
    for(SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end();)
    {
        // use m_zoneUpdateId for speed: UpdateArea called from UpdateZone or instead UpdateZone in both cases m_zoneUpdateId up-to-date
        if(sSpellMgr.GetSpellAllowedInLocationError(iter->second->GetSpellProto(), GetMapId(), m_zoneUpdateId, m_areaUpdateId, this) != SPELL_CAST_OK)
        {
            RemoveSpellAuraHolder(iter->second);
            iter = m_spellAuraHolders.begin();
        }
        else
            ++iter;
    }

    // some auras applied at subzone enter
    SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAreaMapBounds(m_areaUpdateId);
    for(SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
        if(itr->second->autocast && itr->second->IsFitToRequirements(this, m_zoneUpdateId, m_areaUpdateId))
            if (!HasAura(itr->second->spellId, EFFECT_INDEX_0))
                CastSpell(this,itr->second->spellId,true);
}

struct UpdateZoneDependentPetsHelper
{
    explicit UpdateZoneDependentPetsHelper(Player* _owner, uint32 zone, uint32 area) : owner(_owner), zone_id(zone), area_id(area) {}
    void operator()(Unit* unit) const
    {
        if (unit->GetTypeId() == TYPEID_UNIT && ((Creature*)unit)->IsPet() && !((Pet*)unit)->IsPermanentPetFor(owner))
            if (uint32 spell_id = unit->GetUInt32Value(UNIT_CREATED_BY_SPELL))
                if (SpellEntry const* spellEntry = sSpellStore.LookupEntry(spell_id))
                    if (sSpellMgr.GetSpellAllowedInLocationError(spellEntry, owner->GetMapId(), zone_id, area_id, owner) != SPELL_CAST_OK)
                      ((Pet*)unit)->Unsummon(PET_SAVE_AS_DELETED, owner);
    }
    Player* owner;
    uint32 zone_id;
    uint32 area_id;
};

void Player::UpdateZoneDependentPets()
{
    // check pet (permanent pets ignored), minipet, guardians (including protector)
    CallForAllControlledUnits(UpdateZoneDependentPetsHelper(this, m_zoneUpdateId, m_areaUpdateId), CONTROLLED_PET|CONTROLLED_GUARDIANS|CONTROLLED_MINIPET);
}

uint32 Player::GetCorpseReclaimDelay(bool pvp) const
{
    if ((pvp && !sWorld.getConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVP)) ||
       (!pvp && !sWorld.getConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVE) ))
    {
        return copseReclaimDelay[0];
    }

    time_t now = time(NULL);
    // 0..2 full period
    uint32 count = (now < m_deathExpireTime) ? uint32((m_deathExpireTime - now)/DEATH_EXPIRE_STEP) : 0;
    return copseReclaimDelay[count];
}

void Player::UpdateCorpseReclaimDelay()
{
    bool pvp = m_ExtraFlags & PLAYER_EXTRA_PVP_DEATH;

    if ((pvp && !sWorld.getConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVP)) ||
       (!pvp && !sWorld.getConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVE) ))
        return;

    time_t now = time(NULL);
    if(now < m_deathExpireTime)
    {
        // full and partly periods 1..3
        uint32 count = uint32((m_deathExpireTime - now)/DEATH_EXPIRE_STEP +1);
        if(count < MAX_DEATH_COUNT)
            m_deathExpireTime = now+(count+1)*DEATH_EXPIRE_STEP;
        else
            m_deathExpireTime = now+MAX_DEATH_COUNT*DEATH_EXPIRE_STEP;
    }
    else
        m_deathExpireTime = now+DEATH_EXPIRE_STEP;
}

void Player::SendCorpseReclaimDelay(bool load)
{
    Corpse* corpse = GetCorpse();
    if (!corpse)
        return;

    uint32 delay;
    if (load)
    {
        if (corpse->GetGhostTime() > m_deathExpireTime)
            return;

        bool pvp = corpse->GetType()==CORPSE_RESURRECTABLE_PVP;

        uint32 count;
        if ((pvp && sWorld.getConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVP)) ||
            (!pvp && sWorld.getConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVE)))
        {
            count = uint32(m_deathExpireTime-corpse->GetGhostTime())/DEATH_EXPIRE_STEP;
            if (count>=MAX_DEATH_COUNT)
                count = MAX_DEATH_COUNT-1;
        }
        else
            count=0;

        time_t expected_time = corpse->GetGhostTime()+copseReclaimDelay[count];

        time_t now = time(NULL);
        if (now >= expected_time)
            return;

        delay = uint32(expected_time-now);
    }
    else
        delay = GetCorpseReclaimDelay(corpse->GetType()==CORPSE_RESURRECTABLE_PVP);

    //! corpse reclaim delay 30 * 1000ms or longer at often deaths
    WorldPacket data(SMSG_CORPSE_RECLAIM_DELAY, 4);
    data << uint32(delay*IN_MILLISECONDS);
    GetSession()->SendPacket( &data );
}

Player* Player::GetNextRandomRaidMember(float radius)
{
    Group *pGroup = GetGroup();
    if(!pGroup)
        return NULL;

    std::vector<Player*> nearMembers;
    nearMembers.reserve(pGroup->GetMembersCount());

    for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* Target = itr->getSource();

        // IsHostileTo check duel and controlled by enemy
        if( Target && Target != this && IsWithinDistInMap(Target, radius) &&
            !Target->HasInvisibilityAura() && !IsHostileTo(Target) )
            nearMembers.push_back(Target);
    }

    if (nearMembers.empty())
        return NULL;

    uint32 randTarget = urand(0,nearMembers.size()-1);
    return nearMembers[randTarget];
}

PartyResult Player::CanUninviteFromGroup() const
{
    const Group* grp = GetGroup();
    if (!grp)
        return ERR_NOT_IN_GROUP;

    if (!grp->IsLeader(GetObjectGuid()) && !grp->IsAssistant(GetObjectGuid()))
        return ERR_NOT_LEADER;

    if (InBattleGround())
        return ERR_INVITE_RESTRICTED;

    return ERR_PARTY_RESULT_OK;
}

void Player::SetBattleGroundRaid(Group* group, int8 subgroup)
{
    //we must move references from m_group to m_originalGroup
    SetOriginalGroup(GetGroup(), GetSubGroup());

    m_group.unlink();
    m_group.link(group, this);
    m_group.setSubGroup((uint8)subgroup);
}

void Player::RemoveFromBattleGroundRaid()
{
    //remove existing reference
    m_group.unlink();
    if( Group* group = GetOriginalGroup() )
    {
        m_group.link(group, this);
        m_group.setSubGroup(GetOriginalSubGroup());
    }
    SetOriginalGroup(NULL);
}

void Player::SetOriginalGroup(Group *group, int8 subgroup)
{
    if( group == NULL )
        m_originalGroup.unlink();
    else
    {
        // never use SetOriginalGroup without a subgroup unless you specify NULL for group
        MANGOS_ASSERT(subgroup >= 0);
        m_originalGroup.link(group, this);
        m_originalGroup.setSubGroup((uint8)subgroup);
    }
}

void Player::UpdateUnderwaterState( Map* m, float x, float y, float z )
{
    GridMapLiquidData liquid_status;
    GridMapLiquidStatus res = m->GetTerrain()->getLiquidStatus(x, y, z, MAP_ALL_LIQUIDS, &liquid_status);
    if (!res)
    {
        m_MirrorTimerFlags &= ~(UNDERWATER_INWATER|UNDERWATER_INLAVA|UNDERWATER_INSLIME|UNDERWATER_INDARKWATER);
        // Small hack for enable breath in WMO
        /* if (IsInWater())
            m_MirrorTimerFlags|=UNDERWATER_INWATER; */
        return;
    }

    // All liquids type - check under water position
    if (liquid_status.type&(MAP_LIQUID_TYPE_WATER|MAP_LIQUID_TYPE_OCEAN|MAP_LIQUID_TYPE_MAGMA|MAP_LIQUID_TYPE_SLIME))
    {
        if ( res & LIQUID_MAP_UNDER_WATER)
            m_MirrorTimerFlags |= UNDERWATER_INWATER;
        else
            m_MirrorTimerFlags &= ~UNDERWATER_INWATER;
    }

    // Allow travel in dark water on taxi or transport
    if ((liquid_status.type & MAP_LIQUID_TYPE_DARK_WATER) && !IsTaxiFlying() && !GetTransport())
        m_MirrorTimerFlags |= UNDERWATER_INDARKWATER;
    else
        m_MirrorTimerFlags &= ~UNDERWATER_INDARKWATER;

    // in lava check, anywhere in lava level
    if (liquid_status.type&MAP_LIQUID_TYPE_MAGMA)
    {
        if (res & (LIQUID_MAP_UNDER_WATER|LIQUID_MAP_IN_WATER|LIQUID_MAP_WATER_WALK))
            m_MirrorTimerFlags |= UNDERWATER_INLAVA;
        else
            m_MirrorTimerFlags &= ~UNDERWATER_INLAVA;
    }
    // in slime check, anywhere in slime level
    if (liquid_status.type&MAP_LIQUID_TYPE_SLIME)
    {
        if (res & (LIQUID_MAP_UNDER_WATER|LIQUID_MAP_IN_WATER|LIQUID_MAP_WATER_WALK))
            m_MirrorTimerFlags |= UNDERWATER_INSLIME;
        else
            m_MirrorTimerFlags &= ~UNDERWATER_INSLIME;
    }
}

void Player::SetCanParry( bool value )
{
    if(m_canParry==value)
        return;

    m_canParry = value;
    UpdateParryPercentage();
}

void Player::SetCanBlock( bool value )
{
    if(m_canBlock==value)
        return;

    m_canBlock = value;
    UpdateBlockPercentage();
}

bool ItemPosCount::isContainedIn(ItemPosCountVec const& vec) const
{
    for(ItemPosCountVec::const_iterator itr = vec.begin(); itr != vec.end();++itr)
        if(itr->pos == pos)
            return true;

    return false;
}

bool Player::CanUseBattleGroundObject()
{
    // TODO : some spells gives player ForceReaction to one faction (ReputationMgr::ApplyForceReaction)
    // maybe gameobject code should handle that ForceReaction usage
    // BUG: sometimes when player clicks on flag in AB - client won't send gameobject_use, only gameobject_report_use packet
    return ( //InBattleGround() &&                          // in battleground - not need, check in other cases
             //!IsMounted() && - not correct, player is dismounted when he clicks on flag
             //player cannot use object when he is invulnerable (immune)
             !isTotalImmune() &&                            // not totally immune
             //i'm not sure if these two are correct, because invisible players should get visible when they click on flag
             !HasStealthAura() &&                           // not stealthed
             !HasInvisibilityAura() &&                      // not invisible
             !HasAura(SPELL_RECENTLY_DROPPED_FLAG, EFFECT_INDEX_0) &&// can't pickup
             isAlive()                                      // live player
           );
}

bool Player::CanCaptureTowerPoint()
{
    return ( !HasStealthAura() &&                           // not stealthed
             !HasInvisibilityAura() &&                      // not invisible
             isAlive()                                      // live player
           );
}

uint32 Player::GetBarberShopCost(uint8 newhairstyle, uint8 newhaircolor, uint8 newfacialhair)
{
    uint32 level = getLevel();

    if(level > GT_MAX_LEVEL)
        level = GT_MAX_LEVEL;                               // max level in this dbc

    uint8 hairstyle = GetByteValue(PLAYER_BYTES, 2);
    uint8 haircolor = GetByteValue(PLAYER_BYTES, 3);
    uint8 facialhair = GetByteValue(PLAYER_BYTES_2, 0);

    if((hairstyle == newhairstyle) && (haircolor == newhaircolor) && (facialhair == newfacialhair))
        return 0;

    GtBarberShopCostBaseEntry const *bsc = sGtBarberShopCostBaseStore.LookupEntry(level - 1);

    if(!bsc)                                                // shouldn't happen
        return 0xFFFFFFFF;

    float cost = 0;

    if(hairstyle != newhairstyle)
        cost += bsc->cost;                                  // full price

    if((haircolor != newhaircolor) && (hairstyle == newhairstyle))
        cost += bsc->cost * 0.5f;                           // +1/2 of price

    if(facialhair != newfacialhair)
        cost += bsc->cost * 0.75f;                          // +3/4 of price

    return uint32(cost);
}

void Player::InitGlyphsForLevel()
{
    for(uint32 i = 0; i < sGlyphSlotStore.GetNumRows(); ++i)
        if(GlyphSlotEntry const * gs = sGlyphSlotStore.LookupEntry(i))
            if(gs->Order)
                SetGlyphSlot(gs->Order - 1, gs->Id);

    uint32 level = getLevel();
    uint32 value = 0;

    // 0x3F = 0x01 | 0x02 | 0x04 | 0x08 | 0x10 | 0x20 for 80 level
    if(level >= 15)
        value |= (0x01 | 0x02);
    if(level >= 30)
        value |= 0x08;
    if(level >= 50)
        value |= 0x04;
    if(level >= 70)
        value |= 0x10;
    if(level >= 80)
        value |= 0x20;

    SetUInt32Value(PLAYER_GLYPHS_ENABLED, value);
}

void Player::ApplyGlyph(uint8 slot, bool apply)
{
    if (uint32 glyph = GetGlyph(slot))
    {
        if(GlyphPropertiesEntry const *gp = sGlyphPropertiesStore.LookupEntry(glyph))
        {
            if(apply)
            {
                CastSpell(this, gp->SpellId, true);
                SetUInt32Value(PLAYER_FIELD_GLYPHS_1 + slot, glyph);
            }
            else
            {
                RemoveAurasDueToSpell(gp->SpellId);
                SetUInt32Value(PLAYER_FIELD_GLYPHS_1 + slot, 0);
            }
        }
    }
}

void Player::ApplyGlyphs(bool apply)
{
    for (uint8 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
        ApplyGlyph(i,apply);
}

bool Player::isTotalImmune()
{
    AuraList const& immune = GetAurasByType(SPELL_AURA_SCHOOL_IMMUNITY);

    uint32 immuneMask = 0;
    for(AuraList::const_iterator itr = immune.begin(); itr != immune.end(); ++itr)
    {
        immuneMask |= (*itr)->GetModifier()->m_miscvalue;
        if( immuneMask & SPELL_SCHOOL_MASK_ALL )            // total immunity
            return true;
    }
    return false;
}

bool Player::HasTitle(uint32 bitIndex) const
{
    if (bitIndex > MAX_TITLE_INDEX)
        return false;

    uint32 fieldIndexOffset = bitIndex / 32;
    uint32 flag = 1 << (bitIndex % 32);
    return HasFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag);
}

void Player::SetTitle(CharTitlesEntry const* title, bool lost)
{
    uint32 fieldIndexOffset = title->bit_index / 32;
    uint32 flag = 1 << (title->bit_index % 32);

    if(lost)
    {
        if(!HasFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag))
            return;

        RemoveFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag);
    }
    else
    {
        if(HasFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag))
            return;

        SetFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag);
    }

    WorldPacket data(SMSG_TITLE_EARNED, 4 + 4);
    data << uint32(title->bit_index);
    data << uint32(lost ? 0 : 1);                           // 1 - earned, 0 - lost
    GetSession()->SendPacket(&data);
}

void Player::ConvertRune(uint8 index, RuneType newType)
{
    SetCurrentRune(index, newType);

    WorldPacket data(SMSG_CONVERT_RUNE, 2);
    data << uint8(index);
    data << uint8(newType);
    GetSession()->SendPacket(&data);
}

bool Player::ActivateRunes(RuneType type, uint32 count)
{
    bool modify = false;
    for(uint32 j = 0; count > 0 && j < MAX_RUNES; ++j)
    {
        if (GetRuneCooldown(j) && GetCurrentRune(j) == type)
        {
            SetRuneCooldown(j, 0);
            --count;
            modify = true;
        }
    }

    return modify;
}

void Player::ResyncRunes()
{
    WorldPacket data(SMSG_RESYNC_RUNES, 4 + MAX_RUNES * 2);
    data << uint32(MAX_RUNES);
    for(uint32 i = 0; i < MAX_RUNES; ++i)
    {
        data << uint8(GetCurrentRune(i));                   // rune type
        data << uint8(255 - ((GetRuneCooldown(i) / REGEN_TIME_FULL) * 51));     // passed cooldown time (0-255)
    }
    GetSession()->SendPacket(&data);
}

void Player::AddRunePower(uint8 index)
{
    WorldPacket data(SMSG_ADD_RUNE_POWER, 4);
    data << uint32(1 << index);                             // mask (0x00-0x3F probably)
    GetSession()->SendPacket(&data);
}

static RuneType runeSlotTypes[MAX_RUNES] = {
    /*0*/ RUNE_BLOOD,
    /*1*/ RUNE_BLOOD,
    /*2*/ RUNE_UNHOLY,
    /*3*/ RUNE_UNHOLY,
    /*4*/ RUNE_FROST,
    /*5*/ RUNE_FROST
};

void Player::InitRunes()
{
    if(getClass() != CLASS_DEATH_KNIGHT)
        return;

    m_runes = new Runes;

    m_runes->runeState = 0;

    for(uint32 i = 0; i < MAX_RUNES; ++i)
    {
        SetBaseRune(i, runeSlotTypes[i]);                   // init base types
        SetCurrentRune(i, runeSlotTypes[i]);                // init current types
        SetRuneCooldown(i, 0);                              // reset cooldowns
        m_runes->SetRuneState(i);
    }

    for(uint32 i = 0; i < NUM_RUNE_TYPES; ++i)
        SetFloatValue(PLAYER_RUNE_REGEN_1 + i, 0.1f);
}


bool Player::IsBaseRuneSlotsOnCooldown( RuneType runeType ) const
{
    for(uint32 i = 0; i < MAX_RUNES; ++i)
        if (GetBaseRune(i) == runeType && GetRuneCooldown(i) == 0)
            return false;

    return true;
}

void Player::AutoStoreLoot(uint32 loot_id, LootStore const& store, bool broadcast, uint8 bag, uint8 slot)
{
    Loot loot;
    loot.FillLoot (loot_id, store, this, true);

    AutoStoreLoot(loot, broadcast, bag, slot);
}

void Player::AutoStoreLoot(Loot& loot, bool broadcast, uint8 bag, uint8 slot)
{
    uint32 max_slot = loot.GetMaxSlotInLootFor(this);
    for(uint32 i = 0; i < max_slot; ++i)
    {
        LootItem* lootItem = loot.LootItemInSlot(i,this);

        ItemPosCountVec dest;
        InventoryResult msg = CanStoreNewItem(bag,slot,dest,lootItem->itemid,lootItem->count);
        if(msg != EQUIP_ERR_OK && slot != NULL_SLOT)
            msg = CanStoreNewItem( bag, NULL_SLOT,dest,lootItem->itemid,lootItem->count);
        if( msg != EQUIP_ERR_OK && bag != NULL_BAG)
            msg = CanStoreNewItem( NULL_BAG, NULL_SLOT,dest,lootItem->itemid,lootItem->count);
        if(msg != EQUIP_ERR_OK)
        {
            SendEquipError( msg, NULL, NULL, lootItem->itemid );
            continue;
        }

        Item* pItem = StoreNewItem (dest,lootItem->itemid,true,lootItem->randomPropertyId);
        SendNewItem(pItem, lootItem->count, false, false, broadcast);
    }
}

Item* Player::ConvertItem(Item* item, uint32 newItemId)
{
    uint16 pos = item->GetPos();

    Item *pNewItem = Item::CreateItem(newItemId, 1, this);
    if (!pNewItem)
        return NULL;

    // copy enchantments
    for (uint8 j= PERM_ENCHANTMENT_SLOT; j<=TEMP_ENCHANTMENT_SLOT; ++j)
    {
        if (item->GetEnchantmentId(EnchantmentSlot(j)))
            pNewItem->SetEnchantment(EnchantmentSlot(j), item->GetEnchantmentId(EnchantmentSlot(j)),
                item->GetEnchantmentDuration(EnchantmentSlot(j)), item->GetEnchantmentCharges(EnchantmentSlot(j)));
    }

    // copy durability
    if (item->GetUInt32Value(ITEM_FIELD_DURABILITY) < item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY))
    {
        double loosePercent = 1 - item->GetUInt32Value(ITEM_FIELD_DURABILITY) / double(item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY));
        DurabilityLoss(pNewItem, loosePercent);
    }

    if (IsInventoryPos(pos))
    {
        ItemPosCountVec dest;
        InventoryResult msg = CanStoreItem(item->GetBagSlot(), item->GetSlot(), dest, pNewItem, true);
        // ignore cast/combat time restriction
        if (msg == EQUIP_ERR_OK)
        {
            DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
            return StoreItem( dest, pNewItem, true);
        }
    }
    else if (IsBankPos(pos))
    {
        ItemPosCountVec dest;
        InventoryResult msg = CanBankItem(item->GetBagSlot(), item->GetSlot(), dest, pNewItem, true);
        // ignore cast/combat time restriction
        if (msg == EQUIP_ERR_OK)
        {
            DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
            return BankItem(dest, pNewItem, true);
        }
    }
    else if (IsEquipmentPos (pos))
    {
        uint16 dest;
        InventoryResult msg = CanEquipItem(item->GetSlot(), dest, pNewItem, true, false);
        // ignore cast/combat time restriction
        if (msg == EQUIP_ERR_OK)
        {
            DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
            pNewItem = EquipItem(dest, pNewItem, true);
            AutoUnequipOffhandIfNeed();
            return pNewItem;
        }
    }

    // fail
    delete pNewItem;
    return NULL;
}

uint32 Player::CalculateTalentsPoints() const
{
    uint32 base_level = getClass() == CLASS_DEATH_KNIGHT ? 55 : 9;
    uint32 base_talent = getLevel() <= base_level ? 0 : getLevel() - base_level;

    uint32 talentPointsForLevel = base_talent + m_questRewardTalentCount;

    return uint32(talentPointsForLevel * sWorld.getConfig(CONFIG_FLOAT_RATE_TALENT));
}

bool Player::CanStartFlyInArea(uint32 mapid, uint32 zone, uint32 area) const
{
    if (isGameMaster())
        return true;
    // continent checked in SpellMgr::GetSpellAllowedInLocationError at cast and area update
    uint32 v_map = GetVirtualMapForMapAndZone(mapid, zone);

    if (v_map == 571 && !HasSpell(54197))   // Cold Weather Flying
        return false;

    // don't allow flying in Dalaran restricted areas
    // (no other zones currently has areas with AREA_FLAG_CANNOT_FLY)
    if (AreaTableEntry const* atEntry = GetAreaEntryByAreaID(area))
        return (!(atEntry->flags & AREA_FLAG_CANNOT_FLY));

    // TODO: disallow mounting in wintergrasp too when battle is in progress
    // forced dismount part in Player::UpdateArea()
    return true;
}

struct DoPlayerLearnSpell
{
    DoPlayerLearnSpell(Player& _player) : player(_player) {}
    void operator() (uint32 spell_id) { player.learnSpell(spell_id, false); }
    Player& player;
};

void Player::learnSpellHighRank(uint32 spellid)
{
    learnSpell(spellid, false);

    DoPlayerLearnSpell worker(*this);
    sSpellMgr.doForHighRanks(spellid, worker);
}

void Player::_LoadSkills(QueryResult *result)
{
    //                                                           0      1      2
    // SetPQuery(PLAYER_LOGIN_QUERY_LOADSKILLS,          "SELECT skill, value, max FROM character_skills WHERE guid = '%u'", GUID_LOPART(m_guid));

    uint32 count = 0;
    if (result)
    {
        do
        {
            Field *fields = result->Fetch();

            uint16 skill    = fields[0].GetUInt16();
            uint16 value    = fields[1].GetUInt16();
            uint16 max      = fields[2].GetUInt16();

            SkillLineEntry const *pSkill = sSkillLineStore.LookupEntry(skill);
            if(!pSkill)
            {
                sLog.outError("Character %u has skill %u that does not exist.", GetGUIDLow(), skill);
                continue;
            }

            // set fixed skill ranges
            switch(GetSkillRangeType(pSkill,false))
            {
                case SKILL_RANGE_LANGUAGE:                      // 300..300
                    value = max = 300;
                    break;
                case SKILL_RANGE_MONO:                          // 1..1, grey monolite bar
                    value = max = 1;
                    break;
                default:
                    break;
            }

            if(value == 0)
            {
                sLog.outError("Character %u has skill %u with value 0. Will be deleted.", GetGUIDLow(), skill);
                CharacterDatabase.PExecute("DELETE FROM character_skills WHERE guid = '%u' AND skill = '%u' ", GetGUIDLow(), skill );
                continue;
            }

            SetUInt32Value(PLAYER_SKILL_INDEX(count), MAKE_PAIR32(skill,0));
            SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(count),MAKE_SKILL_VALUE(value, max));
            SetUInt32Value(PLAYER_SKILL_BONUS_INDEX(count),0);

            mSkillStatus.insert(SkillStatusMap::value_type(skill, SkillStatusData(count, SKILL_UNCHANGED)));

            learnSkillRewardedSpells(skill, value);

            ++count;

            if(count >= PLAYER_MAX_SKILLS)                      // client limit
            {
                sLog.outError("Character %u has more than %u skills.", GetGUIDLow(), PLAYER_MAX_SKILLS);
                break;
            }
        } while (result->NextRow());
        delete result;
    }

    for (; count < PLAYER_MAX_SKILLS; ++count)
    {
        SetUInt32Value(PLAYER_SKILL_INDEX(count), 0);
        SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(count),0);
        SetUInt32Value(PLAYER_SKILL_BONUS_INDEX(count),0);
    }

    // special settings
    if(getClass()==CLASS_DEATH_KNIGHT)
    {
        uint32 base_level = std::min(getLevel(),sWorld.getConfig (CONFIG_UINT32_START_HEROIC_PLAYER_LEVEL));
        if(base_level < 1)
            base_level = 1;
        uint32 base_skill = (base_level-1)*5;               // 270 at starting level 55
        if(base_skill < 1)
            base_skill = 1;                                 // skill mast be known and then > 0 in any case

        if(GetPureSkillValue (SKILL_FIRST_AID) < base_skill)
            SetSkill(SKILL_FIRST_AID, base_skill, base_skill);
        if(GetPureSkillValue (SKILL_AXES) < base_skill)
            SetSkill(SKILL_AXES, base_skill, base_skill);
        if(GetPureSkillValue (SKILL_DEFENSE) < base_skill)
            SetSkill(SKILL_DEFENSE, base_skill, base_skill);
        if(GetPureSkillValue (SKILL_POLEARMS) < base_skill)
            SetSkill(SKILL_POLEARMS, base_skill, base_skill);
        if(GetPureSkillValue (SKILL_SWORDS) < base_skill)
            SetSkill(SKILL_SWORDS, base_skill, base_skill);
        if(GetPureSkillValue (SKILL_2H_AXES) < base_skill)
            SetSkill(SKILL_2H_AXES, base_skill, base_skill);
        if(GetPureSkillValue (SKILL_2H_SWORDS) < base_skill)
            SetSkill(SKILL_2H_SWORDS, base_skill, base_skill);
        if(GetPureSkillValue (SKILL_UNARMED) < base_skill)
            SetSkill(SKILL_UNARMED, base_skill, base_skill);
    }
}

uint32 Player::GetPhaseMaskForSpawn() const
{
    uint32 phase = PHASEMASK_NORMAL;
    if(!isGameMaster())
        phase = GetPhaseMask();
    else
    {
        AuraList const& phases = GetAurasByType(SPELL_AURA_PHASE);
        if(!phases.empty())
            phase = phases.front()->GetMiscValue();
    }

    // some aura phases include 1 normal map in addition to phase itself
    if(uint32 n_phase = phase & ~PHASEMASK_NORMAL)
        return n_phase;

    return PHASEMASK_NORMAL;
}

InventoryResult Player::CanEquipUniqueItem(Item* pItem, uint8 eslot, uint32 limit_count) const
{
    ItemPrototype const* pProto = pItem->GetProto();

    // proto based limitations
    if(InventoryResult res = CanEquipUniqueItem(pProto,eslot,limit_count))
        return res;

    // check unique-equipped on gems
    for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+3; ++enchant_slot)
    {
        uint32 enchant_id = pItem->GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if(!enchant_id)
            continue;
        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if(!enchantEntry)
            continue;

        ItemPrototype const* pGem = ObjectMgr::GetItemPrototype(enchantEntry->GemID);
        if(!pGem)
            continue;

        // include for check equip another gems with same limit category for not equipped item (and then not counted)
        uint32 gem_limit_count = !pItem->IsEquipped() && pGem->ItemLimitCategory
            ? pItem->GetGemCountWithLimitCategory(pGem->ItemLimitCategory) : 1;

        if(InventoryResult res = CanEquipUniqueItem(pGem, eslot,gem_limit_count))
            return res;
    }

    return EQUIP_ERR_OK;
}

InventoryResult Player::CanEquipUniqueItem( ItemPrototype const* itemProto, uint8 except_slot, uint32 limit_count) const
{
    // check unique-equipped on item
    if (itemProto->Flags & ITEM_FLAG_UNIQUE_EQUIPPED)
    {
        // there is an equip limit on this item
        if(HasItemOrGemWithIdEquipped(itemProto->ItemId,1,except_slot))
            return EQUIP_ERR_ITEM_UNIQUE_EQUIPABLE;
    }

    // check unique-equipped limit
    if (itemProto->ItemLimitCategory)
    {
        ItemLimitCategoryEntry const* limitEntry = sItemLimitCategoryStore.LookupEntry(itemProto->ItemLimitCategory);
        if(!limitEntry)
            return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;

        // NOTE: limitEntry->mode not checked because if item have have-limit then it applied and to equip case

        if(limit_count > limitEntry->maxCount)
            return EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_EQUIPPED_EXCEEDED_IS;

        // there is an equip limit on this item
        if(HasItemOrGemWithLimitCategoryEquipped(itemProto->ItemLimitCategory,limitEntry->maxCount-limit_count+1,except_slot))
            return EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_EQUIPPED_EXCEEDED_IS;
    }

    return EQUIP_ERR_OK;
}

void Player::HandleFall(MovementInfo const& movementInfo)
{
    // calculate total z distance of the fall
    float z_diff = m_lastFallZ - movementInfo.GetPos()->z;
    DEBUG_LOG("zDiff = %f", z_diff);

    //Players with low fall distance, Feather Fall or physical immunity (charges used) are ignored
    // 14.57 can be calculated by resolving damageperc formula below to 0
    if (z_diff >= 14.57f && !isDead() && !isGameMaster() &&
        !HasAuraType(SPELL_AURA_HOVER) && !HasAuraType(SPELL_AURA_FEATHER_FALL) &&
        !HasAuraType(SPELL_AURA_FLY) && !IsImmunedToDamage(SPELL_SCHOOL_MASK_NORMAL) )
    {
        //Safe fall, fall height reduction
        int32 safe_fall = GetTotalAuraModifier(SPELL_AURA_SAFE_FALL);

        float damageperc = 0.018f*(z_diff-safe_fall)-0.2426f;

        if(damageperc >0 )
        {
            uint32 damage = (uint32)(damageperc * GetMaxHealth()*sWorld.getConfig(CONFIG_FLOAT_RATE_DAMAGE_FALL));

            float height = movementInfo.GetPos()->z;
            UpdateAllowedPositionZ(movementInfo.GetPos()->x, movementInfo.GetPos()->y, height);

            if (damage > 0)
            {
                //Prevent fall damage from being more than the player maximum health
                if (damage > GetMaxHealth())
                    damage = GetMaxHealth();

                // Gust of Wind
                if (GetDummyAura(43621))
                    damage = GetMaxHealth()/2;

                uint32 original_health = GetHealth();
                uint32 final_damage = EnvironmentalDamage(DAMAGE_FALL, damage);

                // recheck alive, might have died of EnvironmentalDamage, avoid cases when player die in fact like Spirit of Redemption case
                if (isAlive() && final_damage < original_health)
                    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING, uint32(z_diff*100));
            }

            //Z given by moveinfo, LastZ, FallTime, WaterZ, MapZ, Damage, Safefall reduction
            DEBUG_LOG("FALLDAMAGE z=%f sz=%f pZ=%f FallTime=%d mZ=%f damage=%d SF=%d" , movementInfo.GetPos()->z, height, GetPositionZ(), movementInfo.GetFallTime(), height, damage, safe_fall);
        }
    }
}

void Player::UpdateAchievementCriteria( AchievementCriteriaTypes type, uint32 miscvalue1/*=0*/, uint32 miscvalue2/*=0*/, Unit *unit/*=NULL*/, uint32 time/*=0*/ )
{
    GetAchievementMgr().UpdateAchievementCriteria(type, miscvalue1,miscvalue2,unit,time);
}

void Player::StartTimedAchievementCriteria(AchievementCriteriaTypes type, uint32 timedRequirementId, time_t startTime /*= 0*/)
{
    GetAchievementMgr().StartTimedAchievementCriteria(type, timedRequirementId, startTime);
}

PlayerTalent const* Player::GetKnownTalentById(int32 talentId) const
{
    PlayerTalentMap::const_iterator itr = m_talents[m_activeSpec].find(talentId);
    if (itr != m_talents[m_activeSpec].end() && itr->second.state != PLAYERSPELL_REMOVED)
        return &itr->second;
    else
        return NULL;
}

SpellEntry const* Player::GetKnownTalentRankById(int32 talentId) const
{
    if (PlayerTalent const* talent = GetKnownTalentById(talentId))
        return sSpellStore.LookupEntry(talent->talentEntry->RankID[talent->currentRank]);
    else
        return NULL;
}

void Player::LearnTalent(uint32 talentId, uint32 talentRank)
{
    uint32 CurTalentPoints = GetFreeTalentPoints();

    if(CurTalentPoints == 0)
        return;

    if (talentRank >= MAX_TALENT_RANK)
        return;

    TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);

    if(!talentInfo)
        return;

    TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);

    if(!talentTabInfo)
        return;

    // prevent learn talent for different class (cheating)
    if( (getClassMask() & talentTabInfo->ClassMask) == 0 )
        return;

    // find current max talent rank
    uint32 curtalent_maxrank = 0;
    if (PlayerTalent const* talent = GetKnownTalentById(talentId))
        curtalent_maxrank = talent->currentRank + 1;

    // we already have same or higher talent rank learned
    if(curtalent_maxrank >= (talentRank + 1))
        return;

    // check if we have enough talent points
    if(CurTalentPoints < (talentRank - curtalent_maxrank + 1))
        return;

    // Check if it requires another talent
    if (talentInfo->DependsOn > 0)
    {
        if(TalentEntry const *depTalentInfo = sTalentStore.LookupEntry(talentInfo->DependsOn))
        {
            bool hasEnoughRank = false;
            PlayerTalentMap::iterator dependsOnTalent = m_talents[m_activeSpec].find(depTalentInfo->TalentID);
            if (dependsOnTalent != m_talents[m_activeSpec].end() && dependsOnTalent->second.state != PLAYERSPELL_REMOVED)
            {
                PlayerTalent depTalent = (*dependsOnTalent).second;
                if (depTalent.currentRank >= talentInfo->DependsOnRank)
                    hasEnoughRank = true;
            }

            if (!hasEnoughRank)
                return;
        }
    }

    // Find out how many points we have in this field
    uint32 spentPoints = 0;

    uint32 tTab = talentInfo->TalentTab;
    if (talentInfo->Row > 0)
    {
        for (PlayerTalentMap::const_iterator iter = m_talents[m_activeSpec].begin(); iter != m_talents[m_activeSpec].end(); ++iter)
            if (iter->second.state != PLAYERSPELL_REMOVED && iter->second.talentEntry->TalentTab == tTab)
                spentPoints += iter->second.currentRank + 1;
    }

    // not have required min points spent in talent tree
    if(spentPoints < (talentInfo->Row * MAX_TALENT_RANK))
        return;

    // spell not set in talent.dbc
    uint32 spellid = talentInfo->RankID[talentRank];
    if( spellid == 0 )
    {
        sLog.outError("Talent.dbc have for talent: %u Rank: %u spell id = 0", talentId, talentRank);
        return;
    }

    // already known
    if(HasSpell(spellid))
        return;

    // learn! (other talent ranks will unlearned at learning)
    learnSpell(spellid, false);
    DETAIL_LOG("TalentID: %u Rank: %u Spell: %u\n", talentId, talentRank, spellid);
}

void Player::LearnPetTalent(ObjectGuid petGuid, uint32 talentId, uint32 talentRank)
{
    Pet *pet = GetPet();
    if (!pet)
        return;

    if (petGuid != pet->GetObjectGuid())
        return;

    uint32 CurTalentPoints = pet->GetFreeTalentPoints();

    if(CurTalentPoints == 0)
        return;

    if (talentRank >= MAX_PET_TALENT_RANK)
        return;

    TalentEntry const *talentInfo = sTalentStore.LookupEntry(talentId);

    if(!talentInfo)
        return;

    TalentTabEntry const *talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);

    if(!talentTabInfo)
        return;

    CreatureInfo const *ci = pet->GetCreatureInfo();

    if(!ci)
        return;

    CreatureFamilyEntry const *pet_family = sCreatureFamilyStore.LookupEntry(ci->family);

    if(!pet_family)
        return;

    if(pet_family->petTalentType < 0)                       // not hunter pet
        return;

    // prevent learn talent for different family (cheating)
    if(!((1 << pet_family->petTalentType) & talentTabInfo->petTalentMask))
        return;

    // find current max talent rank
    int32 curtalent_maxrank = 0;
    for(int32 k = MAX_TALENT_RANK-1; k > -1; --k)
    {
        if(talentInfo->RankID[k] && pet->HasSpell(talentInfo->RankID[k]))
        {
            curtalent_maxrank = k + 1;
            break;
        }
    }

    // we already have same or higher talent rank learned
    if(curtalent_maxrank >= int32(talentRank + 1))
        return;

    // check if we have enough talent points
    if(CurTalentPoints < (talentRank - curtalent_maxrank + 1))
        return;

    // Check if it requires another talent
    if (talentInfo->DependsOn > 0)
    {
        if(TalentEntry const *depTalentInfo = sTalentStore.LookupEntry(talentInfo->DependsOn))
        {
            bool hasEnoughRank = false;
            for (int i = talentInfo->DependsOnRank; i < MAX_TALENT_RANK; ++i)
            {
                if (depTalentInfo->RankID[i] != 0)
                    if (pet->HasSpell(depTalentInfo->RankID[i]))
                        hasEnoughRank = true;
            }
            if (!hasEnoughRank)
                return;
        }
    }

    // Find out how many points we have in this field
    uint32 spentPoints = 0;

    uint32 tTab = talentInfo->TalentTab;
    if (talentInfo->Row > 0)
    {
        unsigned int numRows = sTalentStore.GetNumRows();
        for (unsigned int i = 0; i < numRows; ++i)          // Loop through all talents.
        {
            // Someday, someone needs to revamp
            const TalentEntry *tmpTalent = sTalentStore.LookupEntry(i);
            if (tmpTalent)                                  // the way talents are tracked
            {
                if (tmpTalent->TalentTab == tTab)
                {
                    for (int j = 0; j < MAX_TALENT_RANK; ++j)
                    {
                        if (tmpTalent->RankID[j] != 0)
                        {
                            if (pet->HasSpell(tmpTalent->RankID[j]))
                            {
                                spentPoints += j + 1;
                            }
                        }
                    }
                }
            }
        }
    }

    // not have required min points spent in talent tree
    if(spentPoints < (talentInfo->Row * MAX_PET_TALENT_RANK))
        return;

    // spell not set in talent.dbc
    uint32 spellid = talentInfo->RankID[talentRank];
    if( spellid == 0 )
    {
        sLog.outError("Talent.dbc have for talent: %u Rank: %u spell id = 0", talentId, talentRank);
        return;
    }

    // already known
    if(pet->HasSpell(spellid))
        return;

    // learn! (other talent ranks will unlearned at learning)
    pet->learnSpell(spellid);
    DETAIL_LOG("PetTalentID: %u Rank: %u Spell: %u\n", talentId, talentRank, spellid);
}

void Player::UpdateKnownCurrencies(uint32 itemId, bool apply)
{
    if(CurrencyTypesEntry const* ctEntry = sCurrencyTypesStore.LookupEntry(itemId))
    {
        if(apply)
            SetFlag64(PLAYER_FIELD_KNOWN_CURRENCIES, (UI64LIT(1) << (ctEntry->BitIndex - 1)));
        else
            RemoveFlag64(PLAYER_FIELD_KNOWN_CURRENCIES, (UI64LIT(1) << (ctEntry->BitIndex - 1)));
    }
}

void Player::UpdateFallInformationIfNeed( MovementInfo const& minfo,uint16 opcode )
{
    if (m_lastFallTime >= minfo.GetFallTime() || m_lastFallZ <= minfo.GetPos()->z || opcode == MSG_MOVE_FALL_LAND)
        SetFallInformation(minfo.GetFallTime(), minfo.GetPos()->z);
}

void Player::UnsummonPetTemporaryIfAny()
{
    Pet* pet = GetPet();
    if(!pet)
        return;

    if(!m_temporaryUnsummonedPetNumber && pet->isControlled() && !pet->isTemporarySummoned() )
        m_temporaryUnsummonedPetNumber = pet->GetCharmInfo()->GetPetNumber();

    pet->Unsummon(PET_SAVE_AS_CURRENT, this);
}

void Player::ResummonPetTemporaryUnSummonedIfAny()
{
    if (!m_temporaryUnsummonedPetNumber)
        return;

    // not resummon in not appropriate state
    if (IsPetNeedBeTemporaryUnsummoned())
        return;

    if (GetPetGuid())
        return;

    Pet* NewPet = new Pet;
    if(!NewPet->LoadPetFromDB(this, 0, m_temporaryUnsummonedPetNumber, true))
        delete NewPet;

    m_temporaryUnsummonedPetNumber = 0;
}

bool Player::canSeeSpellClickOn(Creature const *c) const
{
    if(!c->HasFlag(UNIT_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK))
        return false;

    SpellClickInfoMapBounds clickPair = sObjectMgr.GetSpellClickInfoMapBounds(c->GetEntry());
    for(SpellClickInfoMap::const_iterator itr = clickPair.first; itr != clickPair.second; ++itr)
        if(itr->second.IsFitToRequirements(this))
            return true;

    return false;
}

void Player::BuildPlayerTalentsInfoData(WorldPacket *data)
{
    *data << uint32(GetFreeTalentPoints());                 // unspentTalentPoints
    *data << uint8(m_specsCount);                           // talent group count (0, 1 or 2)
    *data << uint8(m_activeSpec);                           // talent group index (0 or 1)

    if(m_specsCount)
    {
        // loop through all specs (only 1 for now)
        for(uint32 specIdx = 0; specIdx < m_specsCount; ++specIdx)
        {
            uint8 talentIdCount = 0;
            size_t pos = data->wpos();
            *data << uint8(talentIdCount);                  // [PH], talentIdCount

            // find class talent tabs (all players have 3 talent tabs)
            uint32 const* talentTabIds = GetTalentTabPages(getClass());

            for(uint32 i = 0; i < 3; ++i)
            {
                uint32 talentTabId = talentTabIds[i];
                for(PlayerTalentMap::iterator iter = m_talents[specIdx].begin(); iter != m_talents[specIdx].end(); ++iter)
                {
                    PlayerTalent talent = (*iter).second;

                    if (talent.state == PLAYERSPELL_REMOVED)
                        continue;

                    // skip another tab talents
                    if (talent.talentEntry->TalentTab != talentTabId)
                        continue;

                    *data << uint32(talent.talentEntry->TalentID);  // Talent.dbc
                    *data << uint8(talent.currentRank);      // talentMaxRank (0-4)

                    ++talentIdCount;
                }
            }

            data->put<uint8>(pos, talentIdCount);           // put real count

            *data << uint8(MAX_GLYPH_SLOT_INDEX);           // glyphs count

            // GlyphProperties.dbc
            for(uint8 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
                *data << uint16(m_glyphs[specIdx][i].GetId());
        }
    }
}

void Player::BuildPetTalentsInfoData(WorldPacket *data)
{
    uint32 unspentTalentPoints = 0;
    size_t pointsPos = data->wpos();
    *data << uint32(unspentTalentPoints);                   // [PH], unspentTalentPoints

    uint8 talentIdCount = 0;
    size_t countPos = data->wpos();
    *data << uint8(talentIdCount);                          // [PH], talentIdCount

    Pet *pet = GetPet();
    if(!pet)
        return;

    unspentTalentPoints = pet->GetFreeTalentPoints();

    data->put<uint32>(pointsPos, unspentTalentPoints);      // put real points

    CreatureInfo const *ci = pet->GetCreatureInfo();
    if(!ci)
        return;

    CreatureFamilyEntry const *pet_family = sCreatureFamilyStore.LookupEntry(ci->family);
    if(!pet_family || pet_family->petTalentType < 0)
        return;

    for(uint32 talentTabId = 1; talentTabId < sTalentTabStore.GetNumRows(); ++talentTabId)
    {
        TalentTabEntry const *talentTabInfo = sTalentTabStore.LookupEntry( talentTabId );
        if(!talentTabInfo)
            continue;

        if(!((1 << pet_family->petTalentType) & talentTabInfo->petTalentMask))
            continue;

        for(uint32 talentId = 0; talentId < sTalentStore.GetNumRows(); ++talentId)
        {
            TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);
            if(!talentInfo)
                continue;

            // skip another tab talents
            if(talentInfo->TalentTab != talentTabId)
                continue;

            // find max talent rank
            int32 curtalent_maxrank = -1;
            for(int32 k = 4; k > -1; --k)
            {
                if(talentInfo->RankID[k] && pet->HasSpell(talentInfo->RankID[k]))
                {
                    curtalent_maxrank = k;
                    break;
                }
            }

            // not learned talent
            if(curtalent_maxrank < 0)
                continue;

            *data << uint32(talentInfo->TalentID);          // Talent.dbc
            *data << uint8(curtalent_maxrank);              // talentMaxRank (0-4)

            ++talentIdCount;
        }

        data->put<uint8>(countPos, talentIdCount);          // put real count

        break;
    }
}

void Player::SendTalentsInfoData(bool pet)
{
    WorldPacket data(SMSG_TALENT_UPDATE, 50);
    data << uint8(pet ? 1 : 0);
    if(pet)
        BuildPetTalentsInfoData(&data);
    else
        BuildPlayerTalentsInfoData(&data);
    GetSession()->SendPacket(&data);
}

void Player::BuildEnchantmentsInfoData(WorldPacket *data)
{
    uint32 slotUsedMask = 0;
    size_t slotUsedMaskPos = data->wpos();
    *data << uint32(slotUsedMask);                          // slotUsedMask < 0x80000

    for(uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        Item *item = GetItemByPos(INVENTORY_SLOT_BAG_0, i);

        if(!item)
            continue;

        slotUsedMask |= (1 << i);

        *data << uint32(item->GetEntry());                  // item entry

        uint16 enchantmentMask = 0;
        size_t enchantmentMaskPos = data->wpos();
        *data << uint16(enchantmentMask);                   // enchantmentMask < 0x1000

        for(uint32 j = 0; j < MAX_ENCHANTMENT_SLOT; ++j)
        {
            uint32 enchId = item->GetEnchantmentId(EnchantmentSlot(j));

            if(!enchId)
                continue;

            enchantmentMask |= (1 << j);

            *data << uint16(enchId);                        // enchantmentId?
        }

        data->put<uint16>(enchantmentMaskPos, enchantmentMask);

        *data << uint16(item->GetItemRandomPropertyId());
        *data << item->GetGuidValue(ITEM_FIELD_CREATOR).WriteAsPacked();
        *data << uint32(item->GetItemSuffixFactor());
    }

    data->put<uint32>(slotUsedMaskPos, slotUsedMask);
}

void Player::SendEquipmentSetList()
{
    uint32 count = 0;
    WorldPacket data(SMSG_LOAD_EQUIPMENT_SET, 4);
    size_t count_pos = data.wpos();
    data << uint32(count);                                  // count placeholder
    for(EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end(); ++itr)
    {
        if(itr->second.state==EQUIPMENT_SET_DELETED)
            continue;
        data.appendPackGUID(itr->second.Guid);
        data << uint32(itr->first);
        data << itr->second.Name;
        data << itr->second.IconName;
        for(uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
        {
            // ignored slots stored in IgnoreMask, client wants "1" as raw GUID, so no HIGHGUID_ITEM
            if (itr->second.IgnoreMask & (1 << i))
                data << ObjectGuid(uint64(1)).WriteAsPacked();
            else
                data << ObjectGuid(HIGHGUID_ITEM, itr->second.Items[i]).WriteAsPacked();
        }

        ++count;                                            // client have limit but it checked at loading and set
    }
    data.put<uint32>(count_pos, count);
    GetSession()->SendPacket(&data);
}

void Player::SetEquipmentSet(uint32 index, EquipmentSet eqset)
{
    if(eqset.Guid != 0)
    {
        bool found = false;

        for(EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end(); ++itr)
        {
            if((itr->second.Guid == eqset.Guid) && (itr->first == index))
            {
                found = true;
                break;
            }
        }

        if(!found)                                          // something wrong...
        {
            sLog.outError("Player %s tried to save equipment set "UI64FMTD" (index %u), but that equipment set not found!", GetName(), eqset.Guid, index);
            return;
        }
    }

    EquipmentSet& eqslot = m_EquipmentSets[index];

    EquipmentSetUpdateState old_state = eqslot.state;

    eqslot = eqset;

    if(eqset.Guid == 0)
    {
        eqslot.Guid = sObjectMgr.GenerateEquipmentSetGuid();

        WorldPacket data(SMSG_EQUIPMENT_SET_ID, 4 + 1);
        data << uint32(index);
        data.appendPackGUID(eqslot.Guid);
        GetSession()->SendPacket(&data);
    }

    eqslot.state = old_state == EQUIPMENT_SET_NEW ? EQUIPMENT_SET_NEW : EQUIPMENT_SET_CHANGED;
}

void Player::_SaveEquipmentSets()
{
    static SqlStatementID updSets ;
    static SqlStatementID insSets ;
    static SqlStatementID delSets ;

    for(EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end();)
    {
        uint32 index = itr->first;
        EquipmentSet& eqset = itr->second;
        switch(eqset.state)
        {
            case EQUIPMENT_SET_UNCHANGED:
                ++itr;
                break;                                      // nothing do
            case EQUIPMENT_SET_CHANGED:
            {
                SqlStatement stmt = CharacterDatabase.CreateStatement(updSets, "UPDATE character_equipmentsets SET name=?, iconname=?, ignore_mask=?, item0=?, item1=?, item2=?, item3=?, item4=?, "
                    "item5=?, item6=?, item7=?, item8=?, item9=?, item10=?, item11=?, item12=?, item13=?, item14=?, "
                    "item15=?, item16=?, item17=?, item18=? WHERE guid=? AND setguid=? AND setindex=?");

                stmt.addString(eqset.Name);
                stmt.addString(eqset.IconName);
                stmt.addUInt32(eqset.IgnoreMask);

                for (int i = 0; i < EQUIPMENT_SLOT_END; ++i)
                    stmt.addUInt32(eqset.Items[i]);

                stmt.addUInt32(GetGUIDLow());
                stmt.addUInt64(eqset.Guid);
                stmt.addUInt32(index);

                stmt.Execute();

                eqset.state = EQUIPMENT_SET_UNCHANGED;
                ++itr;
                break;
            }
            case EQUIPMENT_SET_NEW:
            {
                SqlStatement stmt = CharacterDatabase.CreateStatement(insSets, "INSERT INTO character_equipmentsets VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
                stmt.addUInt32(GetGUIDLow());
                stmt.addUInt64(eqset.Guid);
                stmt.addUInt32(index);
                stmt.addString(eqset.Name);
                stmt.addString(eqset.IconName);
                stmt.addUInt32(eqset.IgnoreMask);

                for (int i = 0; i < EQUIPMENT_SLOT_END; ++i)
                    stmt.addUInt32(eqset.Items[i]);

                stmt.Execute();

                eqset.state = EQUIPMENT_SET_UNCHANGED;
                ++itr;
                break;
            }
            case EQUIPMENT_SET_DELETED:
            {
                SqlStatement stmt = CharacterDatabase.CreateStatement(delSets, "DELETE FROM character_equipmentsets WHERE setguid = ?");
                stmt.PExecute(eqset.Guid);
                m_EquipmentSets.erase(itr++);
                break;
            }
        }
    }
}

void Player::_SaveBGData()
{
    // nothing save
    if (!m_bgData.m_needSave)
        return;

    static SqlStatementID delBGData ;
    static SqlStatementID insBGData ;

    SqlStatement stmt =  CharacterDatabase.CreateStatement(delBGData, "DELETE FROM character_battleground_data WHERE guid = ?");

    stmt.PExecute(GetGUIDLow());

    if (m_bgData.bgInstanceID)
    {
        stmt = CharacterDatabase.CreateStatement(insBGData, "INSERT INTO character_battleground_data VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        /* guid, bgInstanceID, bgTeam, x, y, z, o, map, taxi[0], taxi[1], mountSpell */
        stmt.addUInt32(GetGUIDLow());
        stmt.addUInt32(m_bgData.bgInstanceID);
        stmt.addUInt32(uint32(m_bgData.bgTeam));
        stmt.addFloat(m_bgData.joinPos.coord_x);
        stmt.addFloat(m_bgData.joinPos.coord_y);
        stmt.addFloat(m_bgData.joinPos.coord_z);
        stmt.addFloat(m_bgData.joinPos.orientation);
        stmt.addUInt32(m_bgData.joinPos.mapid);
        stmt.addUInt32(m_bgData.taxiPath[0]);
        stmt.addUInt32(m_bgData.taxiPath[1]);
        stmt.addUInt32(m_bgData.mountSpell);

        stmt.Execute();
    }

    m_bgData.m_needSave = false;
}

void Player::DeleteEquipmentSet(uint64 setGuid)
{
    for(EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end(); ++itr)
    {
        if(itr->second.Guid == setGuid)
        {
            if(itr->second.state == EQUIPMENT_SET_NEW)
                m_EquipmentSets.erase(itr);
            else
                itr->second.state = EQUIPMENT_SET_DELETED;
            break;
        }
    }
}

void Player::ActivateSpec(uint8 specNum)
{
    if(GetActiveSpec() == specNum)
        return;

    if(specNum >= GetSpecsCount())
        return;

    UnsummonPetTemporaryIfAny();

    // prevent deletion of action buttons by client at spell unlearn or by player while spec change in progress
    SendLockActionButtons();

    ApplyGlyphs(false);

    // copy of new talent spec (we will use it as model for converting current tlanet state to new)
    PlayerTalentMap tempSpec = m_talents[specNum];

    // copy old spec talents to new one, must be before spec switch to have previous spec num(as m_activeSpec)
    m_talents[specNum] = m_talents[m_activeSpec];

    SetActiveSpec(specNum);

    // remove all talent spells that don't exist in next spec but exist in old
    for (PlayerTalentMap::iterator specIter = m_talents[m_activeSpec].begin(); specIter != m_talents[m_activeSpec].end();)
    {
        PlayerTalent& talent = specIter->second;

        if (talent.state == PLAYERSPELL_REMOVED)
        {
            ++specIter;
            continue;
        }

        PlayerTalentMap::iterator iterTempSpec = tempSpec.find(specIter->first);

        // remove any talent rank if talent not listed in temp spec
        if (iterTempSpec == tempSpec.end() || iterTempSpec->second.state == PLAYERSPELL_REMOVED)
        {
            TalentEntry const *talentInfo = talent.talentEntry;

            for(int r = 0; r < MAX_TALENT_RANK; ++r)
                if (talentInfo->RankID[r])
                    removeSpell(talentInfo->RankID[r],!IsPassiveSpell(talentInfo->RankID[r]),false);

            specIter = m_talents[m_activeSpec].begin();
        }
        else
            ++specIter;
    }

    // now new spec data have only talents (maybe different rank) as in temp spec data, sync ranks then.
    for (PlayerTalentMap::const_iterator tempIter = tempSpec.begin(); tempIter != tempSpec.end(); ++tempIter)
    {
        PlayerTalent const& talent = tempIter->second;

        // removed state talent already unlearned in prev. loop
        // but we need restore it if it deleted for finish removed-marked data in DB
        if (talent.state == PLAYERSPELL_REMOVED)
        {
            m_talents[m_activeSpec][tempIter->first] = talent;
            continue;
        }

        uint32 talentSpellId = talent.talentEntry->RankID[talent.currentRank];

        // learn talent spells if they not in new spec (old spec copy)
        // and if they have different rank
        if (PlayerTalent const* cur_talent = GetKnownTalentById(tempIter->first))
        {
            if (cur_talent->currentRank != talent.currentRank)
                learnSpell(talentSpellId, false);
        }
        else
            learnSpell(talentSpellId, false);

        // sync states - original state is changed in addSpell that learnSpell calls
        PlayerTalentMap::iterator specIter = m_talents[m_activeSpec].find(tempIter->first);
        if (specIter != m_talents[m_activeSpec].end())
            specIter->second.state = talent.state;
        else
        {
            sLog.outError("ActivateSpec: Talent spell %u expected to learned at spec switch but not listed in talents at final check!", talentSpellId);

            // attempt resync DB state (deleted lost spell from DB)
            if (talent.state != PLAYERSPELL_NEW)
            {
                PlayerTalent& talentNew = m_talents[m_activeSpec][tempIter->first];
                talentNew = talent;
                talentNew.state = PLAYERSPELL_REMOVED;
            }
        }
    }

    InitTalentForLevel();

    // recheck action buttons (not checked at loading/spec copy)
    ActionButtonList const& currentActionButtonList = m_actionButtons[m_activeSpec];
    for(ActionButtonList::const_iterator itr = currentActionButtonList.begin(); itr != currentActionButtonList.end(); )
    {
        if (itr->second.uState != ACTIONBUTTON_DELETED)
        {
            // remove broken without any output (it can be not correct because talents not copied at spec creating)
            if (!IsActionButtonDataValid(itr->first,itr->second.GetAction(),itr->second.GetType(), this, false))
            {
                removeActionButton(m_activeSpec,itr->first);
                itr = currentActionButtonList.begin();
                continue;
            }
        }
        ++itr;
    }

    ResummonPetTemporaryUnSummonedIfAny();

    ApplyGlyphs(true);

    SendInitialActionButtons();

    Powers pw = getPowerType();
    if(pw != POWER_MANA)
        SetPower(POWER_MANA, 0);

    SetPower(pw, 0);
}

void Player::UpdateSpecCount(uint8 count)
{
    uint8 curCount = GetSpecsCount();
    if (curCount == count)
        return;

    // maybe current spec data must be copied to 0 spec?
    if (m_activeSpec >= count)
        ActivateSpec(0);

    // copy spec data from new specs
    if (count > curCount)
    {
        // copy action buttons from active spec (more easy in this case iterate first by button)
        ActionButtonList const& currentActionButtonList = m_actionButtons[m_activeSpec];

        for(ActionButtonList::const_iterator itr = currentActionButtonList.begin(); itr != currentActionButtonList.end(); ++itr)
        {
            if (itr->second.uState != ACTIONBUTTON_DELETED)
            {
                for(uint8 spec = curCount; spec < count; ++spec)
                    addActionButton(spec,itr->first,itr->second.GetAction(),itr->second.GetType());
            }
        }
    }
    // delete spec data for removed specs
    else if (count < curCount)
    {
        // delete action buttons for removed spec
        for(uint8 spec = count; spec < curCount; ++spec)
        {
            // delete action buttons for removed spec
            for(uint8 button = 0; button < MAX_ACTION_BUTTONS; ++button)
                removeActionButton(spec,button);
        }
    }

    SetSpecsCount(count);

    SendTalentsInfoData(false);
}

void Player::RemoveAtLoginFlag( AtLoginFlags f, bool in_db_also /*= false*/ )
{
    m_atLoginFlags &= ~f;

    if(in_db_also)
        CharacterDatabase.PExecute("UPDATE characters set at_login = at_login & ~ %u WHERE guid ='%u'", uint32(f), GetGUIDLow());
}

void Player::SendClearCooldown( uint32 spell_id, Unit* target )
{
    WorldPacket data(SMSG_CLEAR_COOLDOWN, 4+8);
    data << uint32(spell_id);
    data << target->GetObjectGuid();
    SendDirectMessage(&data);
}

void Player::BuildTeleportAckMsg(WorldPacket& data, float x, float y, float z, float ang) const
{
    MovementInfo mi = m_movementInfo;
    mi.ChangePosition(x, y, z, ang);

    data.Initialize(MSG_MOVE_TELEPORT_ACK, 64);
    data << GetPackGUID();
    data << uint32(0);                                      // this value increments every time
    data << mi;
}

bool Player::HasMovementFlag( MovementFlags f ) const
{
    return m_movementInfo.HasMovementFlag(f);
}

void Player::ResetTimeSync()
{
    m_timeSyncCounter = 0;
    m_timeSyncTimer = 0;
    m_timeSyncClient = 0;
    m_timeSyncServer = WorldTimer::getMSTime();
}

void Player::SendTimeSync()
{
    WorldPacket data(SMSG_TIME_SYNC_REQ, 4);
    data << uint32(m_timeSyncCounter++);
    GetSession()->SendPacket(&data);

    // Schedule next sync in 10 sec
    m_timeSyncTimer = 10000;
    m_timeSyncServer = WorldTimer::getMSTime();
}

void Player::SendDuelCountdown(uint32 counter)
{
    WorldPacket data(SMSG_DUEL_COUNTDOWN, 4);
    data << uint32(counter);                                // seconds
    GetSession()->SendPacket(&data);
}

bool Player::IsImmuneToSpellEffect(SpellEntry const* spellInfo, SpellEffectIndex index) const
{
    switch(spellInfo->Effect[index])
    {
        case SPELL_EFFECT_ATTACK_ME:
            return true;
        default:
            break;
    }
    switch(spellInfo->EffectApplyAuraName[index])
    {
        case SPELL_AURA_MOD_TAUNT:
            return true;
        default:
            break;
    }
    return Unit::IsImmuneToSpellEffect(spellInfo, index);
}

void Player::SetHomebindToLocation(WorldLocation const& loc, uint32 area_id)
{
    m_homebindMapId = loc.mapid;
    m_homebindAreaId = area_id;
    m_homebindX = loc.coord_x;
    m_homebindY = loc.coord_y;
    m_homebindZ = loc.coord_z;

    // update sql homebind
    CharacterDatabase.PExecute("UPDATE character_homebind SET map = '%u', zone = '%u', position_x = '%f', position_y = '%f', position_z = '%f' WHERE guid = '%u'",
        m_homebindMapId, m_homebindAreaId, m_homebindX, m_homebindY, m_homebindZ, GetGUIDLow());
}

Object* Player::GetObjectByTypeMask(ObjectGuid guid, TypeMask typemask)
{
    switch(guid.GetHigh())
    {
        case HIGHGUID_ITEM:
            if (typemask & TYPEMASK_ITEM)
                return GetItemByGuid(guid);
            break;
        case HIGHGUID_PLAYER:
            if (GetObjectGuid()==guid)
                return this;
            if ((typemask & TYPEMASK_PLAYER) && IsInWorld())
                return ObjectAccessor::FindPlayer(guid);
            break;
        case HIGHGUID_GAMEOBJECT:
            if ((typemask & TYPEMASK_GAMEOBJECT) && IsInWorld())
                return GetMap()->GetGameObject(guid);
            break;
        case HIGHGUID_UNIT:
        case HIGHGUID_VEHICLE:
            if ((typemask & TYPEMASK_UNIT) && IsInWorld())
                return GetMap()->GetCreature(guid);
            break;
        case HIGHGUID_PET:
            if ((typemask & TYPEMASK_UNIT) && IsInWorld())
                return GetMap()->GetPet(guid);
            break;
        case HIGHGUID_DYNAMICOBJECT:
            if ((typemask & TYPEMASK_DYNAMICOBJECT) && IsInWorld())
                return GetMap()->GetDynamicObject(guid);
            break;
        case HIGHGUID_TRANSPORT:
        case HIGHGUID_CORPSE:
        case HIGHGUID_MO_TRANSPORT:
        case HIGHGUID_INSTANCE:
        case HIGHGUID_GROUP:
        default:
            break;
    }

    return NULL;
}

void Player::SetRestType( RestType n_r_type, uint32 areaTriggerId /*= 0*/)
{
    rest_type = n_r_type;

    if (rest_type == REST_TYPE_NO)
    {
        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING);

        // Set player to FFA PVP when not in rested environment.
        if(sWorld.IsFFAPvPRealm())
            SetFFAPvP(true);
    }
    else
    {
        SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING);

        inn_trigger_id = areaTriggerId;
        time_inn_enter = time(NULL);

        if(sWorld.IsFFAPvPRealm())
            SetFFAPvP(false);
    }
}

uint32 Player::GetEquipGearScore(bool withBags, bool withBank)
{
    if (withBags && withBank && m_cachedGS > 0)
        return m_cachedGS;

    GearScoreVec gearScore (EQUIPMENT_SLOT_END);
    uint32 twoHandScore = 0;

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            _fillGearScoreData(item, &gearScore, twoHandScore);
    }

    if (withBags)
    {
        // check inventory
        for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                _fillGearScoreData(item, &gearScore, twoHandScore);
        }

        // check bags
        for(int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            if(Bag* pBag = (Bag*)GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                for(uint32 j = 0; j < pBag->GetBagSize(); ++j)
                {
                    if (Item* item2 = pBag->GetItemByPos(j))
                        _fillGearScoreData(item2, &gearScore, twoHandScore);
                }
            }
        }
    }

    if (withBank)
    {
        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
        {
            if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                _fillGearScoreData(item, &gearScore, twoHandScore);
        }

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                if (item->IsBag())
                {
                    Bag* bag = (Bag*)item;
                    for (uint8 j = 0; j < bag->GetBagSize(); ++j)
                    {
                        if (Item* item2 = bag->GetItemByPos(j))
                            _fillGearScoreData(item2, &gearScore, twoHandScore);
                    }
                }
            }
        }
    }

    uint8 count = EQUIPMENT_SLOT_END - 2;   // ignore body and tabard slots
    uint32 sum = 0;

    // check if 2h hand is higher level than main hand + off hand
    if (gearScore[EQUIPMENT_SLOT_MAINHAND] + gearScore[EQUIPMENT_SLOT_OFFHAND] < twoHandScore * 2)
    {
        gearScore[EQUIPMENT_SLOT_OFFHAND] = 0;  // off hand is ignored in calculations if 2h weapon has higher score
        --count;
        gearScore[EQUIPMENT_SLOT_MAINHAND] = twoHandScore;
    }

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
       sum += gearScore[i];
    }

    if (count)
    {
        uint32 res = uint32(sum / count);
        DEBUG_LOG("Player: calculating gear score for %u. Result is %u", GetObjectGuid().GetCounter(), res);

        if (withBags && withBank)
            m_cachedGS = res;

        return res;
    }
    else
        return 0;
}

void Player::_fillGearScoreData(Item* item, GearScoreVec* gearScore, uint32& twoHandScore)
{
    if (!item)
        return;

    if (CanUseItem(item->GetProto()) != EQUIP_ERR_OK)
        return;

    uint8 type   = item->GetProto()->InventoryType;
    uint32 level = item->GetProto()->ItemLevel;

    switch (type)
    {
        case INVTYPE_2HWEAPON:
            twoHandScore = std::max(twoHandScore, level);
            break;
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
            (*gearScore)[SLOT_MAIN_HAND] = std::max((*gearScore)[SLOT_MAIN_HAND], level);
            break;
        case INVTYPE_SHIELD:
        case INVTYPE_WEAPONOFFHAND:
            (*gearScore)[EQUIPMENT_SLOT_OFFHAND] = std::max((*gearScore)[EQUIPMENT_SLOT_OFFHAND], level);
            break;
        case INVTYPE_THROWN:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_RANGED:
        case INVTYPE_QUIVER:
        case INVTYPE_RELIC:
            (*gearScore)[EQUIPMENT_SLOT_RANGED] = std::max((*gearScore)[EQUIPMENT_SLOT_RANGED], level);
            break;
        case INVTYPE_HEAD:
            (*gearScore)[EQUIPMENT_SLOT_HEAD] = std::max((*gearScore)[EQUIPMENT_SLOT_HEAD], level);
            break;
        case INVTYPE_NECK:
            (*gearScore)[EQUIPMENT_SLOT_NECK] = std::max((*gearScore)[EQUIPMENT_SLOT_NECK], level);
            break;
        case INVTYPE_SHOULDERS:
            (*gearScore)[EQUIPMENT_SLOT_SHOULDERS] = std::max((*gearScore)[EQUIPMENT_SLOT_SHOULDERS], level);
            break;
        case INVTYPE_BODY:
            (*gearScore)[EQUIPMENT_SLOT_BODY] = std::max((*gearScore)[EQUIPMENT_SLOT_BODY], level);
            break;
        case INVTYPE_CHEST:
            (*gearScore)[EQUIPMENT_SLOT_CHEST] = std::max((*gearScore)[EQUIPMENT_SLOT_CHEST], level);
            break;
        case INVTYPE_WAIST:
            (*gearScore)[EQUIPMENT_SLOT_WAIST] = std::max((*gearScore)[EQUIPMENT_SLOT_WAIST], level);
            break;
        case INVTYPE_LEGS:
            (*gearScore)[EQUIPMENT_SLOT_LEGS] = std::max((*gearScore)[EQUIPMENT_SLOT_LEGS], level);
            break;
        case INVTYPE_FEET:
            (*gearScore)[EQUIPMENT_SLOT_FEET] = std::max((*gearScore)[EQUIPMENT_SLOT_FEET], level);
            break;
        case INVTYPE_WRISTS:
            (*gearScore)[EQUIPMENT_SLOT_WRISTS] = std::max((*gearScore)[EQUIPMENT_SLOT_WRISTS], level);
            break;
        case INVTYPE_HANDS:
            (*gearScore)[EQUIPMENT_SLOT_HEAD] = std::max((*gearScore)[EQUIPMENT_SLOT_HEAD], level);
            break;
        // equipped gear score check uses both rings and trinkets for calculation, assume that for bags/banks it is the same
        // with keeping second highest score at second slot
        case INVTYPE_FINGER:
        {
            if ((*gearScore)[EQUIPMENT_SLOT_FINGER1] < level)
            {
                (*gearScore)[EQUIPMENT_SLOT_FINGER2] = (*gearScore)[EQUIPMENT_SLOT_FINGER1];
                (*gearScore)[EQUIPMENT_SLOT_FINGER1] = level;
            }
            else if ((*gearScore)[EQUIPMENT_SLOT_FINGER2] < level)
                (*gearScore)[EQUIPMENT_SLOT_FINGER2] = level;
            break;
        }
        case INVTYPE_TRINKET:
        {
            if ((*gearScore)[EQUIPMENT_SLOT_TRINKET1] < level)
            {
                (*gearScore)[EQUIPMENT_SLOT_TRINKET2] = (*gearScore)[EQUIPMENT_SLOT_TRINKET1];
                (*gearScore)[EQUIPMENT_SLOT_TRINKET1] = level;
            }
            else if ((*gearScore)[EQUIPMENT_SLOT_TRINKET2] < level)
                (*gearScore)[EQUIPMENT_SLOT_TRINKET2] = level;
            break;
        }
        case INVTYPE_CLOAK:
            (*gearScore)[EQUIPMENT_SLOT_BACK] = std::max((*gearScore)[EQUIPMENT_SLOT_BACK], level);
            break;
        default:
            break;
    }
}
