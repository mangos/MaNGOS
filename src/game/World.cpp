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

/** \file
    \ingroup world
*/

#include "World.h"
#include "Database/DatabaseEnv.h"
#include "Config/Config.h"
#include "Platform/Define.h"
#include "SystemConfig.h"
#include "Log.h"
#include "Opcodes.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "Weather.h"
#include "Player.h"
#include "Vehicle.h"
#include "SkillExtraItems.h"
#include "SkillDiscovery.h"
#include "AccountMgr.h"
#include "AchievementMgr.h"
#include "AuctionHouseMgr.h"
#include "ObjectMgr.h"
#include "CreatureEventAIMgr.h"
#include "SpellMgr.h"
#include "Chat.h"
#include "DBCStores.h"
#include "MassMailMgr.h"
#include "LootMgr.h"
#include "ItemEnchantmentMgr.h"
#include "MapManager.h"
#include "ScriptMgr.h"
#include "CreatureAIRegistry.h"
#include "Policies/SingletonImp.h"
#include "BattleGroundMgr.h"
#include "Language.h"
#include "TemporarySummon.h"
#include "VMapFactory.h"
#include "GameEventMgr.h"
#include "PoolManager.h"
#include "Database/DatabaseImpl.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "InstanceSaveMgr.h"
#include "WaypointManager.h"
#include "GMTicketMgr.h"
#include "Util.h"
#include "CharacterDatabaseCleaner.h"
#include "AuctionHouseBot.h"

INSTANTIATE_SINGLETON_1( World );

volatile bool World::m_stopEvent = false;
uint8 World::m_ExitCode = SHUTDOWN_EXIT_CODE;
volatile uint32 World::m_worldLoopCounter = 0;

float World::m_MaxVisibleDistanceOnContinents = DEFAULT_VISIBILITY_DISTANCE;
float World::m_MaxVisibleDistanceInInstances  = DEFAULT_VISIBILITY_INSTANCE;
float World::m_MaxVisibleDistanceInBGArenas   = DEFAULT_VISIBILITY_BGARENAS;
float World::m_MaxVisibleDistanceForObject    = DEFAULT_VISIBILITY_DISTANCE;

float World::m_MaxVisibleDistanceInFlight     = DEFAULT_VISIBILITY_DISTANCE;
float World::m_VisibleUnitGreyDistance        = 0;
float World::m_VisibleObjectGreyDistance      = 0;

/// World constructor
World::World()
{
    m_playerLimit = 0;
    m_allowMovement = true;
    m_ShutdownMask = 0;
    m_ShutdownTimer = 0;
    m_gameTime=time(NULL);
    m_startTime=m_gameTime;
    m_maxActiveSessionCount = 0;
    m_maxQueuedSessionCount = 0;
    m_NextDailyQuestReset = 0;
    m_NextWeeklyQuestReset = 0;
    m_scheduledScripts = 0;

    m_defaultDbcLocale = LOCALE_enUS;
    m_availableDbcLocaleMask = 0;

    for(int i = 0; i < CONFIG_UINT32_VALUE_COUNT; ++i)
        m_configUint32Values[i] = 0;

    for(int i = 0; i < CONFIG_INT32_VALUE_COUNT; ++i)
        m_configInt32Values[i] = 0;

    for(int i = 0; i < CONFIG_FLOAT_VALUE_COUNT; ++i)
        m_configFloatValues[i] = 0.0f;

    for(int i = 0; i < CONFIG_BOOL_VALUE_COUNT; ++i)
        m_configBoolValues[i] = false;
}

/// World destructor
World::~World()
{
    ///- Empty the kicked session set
    while (!m_sessions.empty())
    {
        // not remove from queue, prevent loading new sessions
        delete m_sessions.begin()->second;
        m_sessions.erase(m_sessions.begin());
    }

    ///- Empty the WeatherMap
    for (WeatherMap::const_iterator itr = m_weathers.begin(); itr != m_weathers.end(); ++itr)
        delete itr->second;

    m_weathers.clear();

    CliCommandHolder* command;
    while (cliCmdQueue.next(command))
        delete command;

    VMAP::VMapFactory::clear();

    //TODO free addSessQueue
}

/// Find a player in a specified zone
Player* World::FindPlayerInZone(uint32 zone)
{
    ///- circle through active sessions and return the first player found in the zone
    SessionMap::const_iterator itr;
    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if(!itr->second)
            continue;
        Player *player = itr->second->GetPlayer();
        if(!player)
            continue;
        if( player->IsInWorld() && player->GetZoneId() == zone )
        {
            // Used by the weather system. We return the player to broadcast the change weather message to him and all players in the zone.
            return player;
        }
    }
    return NULL;
}

/// Find a session by its id
WorldSession* World::FindSession(uint32 id) const
{
    SessionMap::const_iterator itr = m_sessions.find(id);

    if(itr != m_sessions.end())
        return itr->second;                                 // also can return NULL for kicked session
    else
        return NULL;
}

/// Remove a given session
bool World::RemoveSession(uint32 id)
{
    ///- Find the session, kick the user, but we can't delete session at this moment to prevent iterator invalidation
    SessionMap::const_iterator itr = m_sessions.find(id);

    if(itr != m_sessions.end() && itr->second)
    {
        if (itr->second->PlayerLoading())
            return false;
        itr->second->KickPlayer();
    }

    return true;
}

void World::AddSession(WorldSession* s)
{
    addSessQueue.add(s);
}

void
World::AddSession_ (WorldSession* s)
{
    MANGOS_ASSERT (s);

    //NOTE - Still there is race condition in WorldSession* being used in the Sockets

    ///- kick already loaded player with same account (if any) and remove session
    ///- if player is in loading and want to load again, return
    if (!RemoveSession (s->GetAccountId ()))
    {
        s->KickPlayer ();
        delete s;                                           // session not added yet in session list, so not listed in queue
        return;
    }

    // decrease session counts only at not reconnection case
    bool decrease_session = true;

    // if session already exist, prepare to it deleting at next world update
    // NOTE - KickPlayer() should be called on "old" in RemoveSession()
    {
        SessionMap::const_iterator old = m_sessions.find(s->GetAccountId ());

        if(old != m_sessions.end())
        {
            // prevent decrease sessions count if session queued
            if(RemoveQueuedSession(old->second))
                decrease_session = false;
            // not remove replaced session form queue if listed
            delete old->second;
        }
    }

    m_sessions[s->GetAccountId ()] = s;

    uint32 Sessions = GetActiveAndQueuedSessionCount();
    uint32 pLimit = GetPlayerAmountLimit();
    uint32 QueueSize = GetQueuedSessionCount();             //number of players in the queue

    //so we don't count the user trying to
    //login as a session and queue the socket that we are using
    if(decrease_session)
        --Sessions;

    if (pLimit > 0 && Sessions >= pLimit && s->GetSecurity () == SEC_PLAYER )
    {
        AddQueuedSession(s);
        UpdateMaxSessionCounters();
        DETAIL_LOG("PlayerQueue: Account id %u is in Queue Position (%u).", s->GetAccountId (), ++QueueSize);
        return;
    }

    WorldPacket packet(SMSG_AUTH_RESPONSE, 1 + 4 + 1 + 4 + 1);
    packet << uint8 (AUTH_OK);
    packet << uint32 (0);                                   // BillingTimeRemaining
    packet << uint8 (0);                                    // BillingPlanFlags
    packet << uint32 (0);                                   // BillingTimeRested
    packet << uint8 (s->Expansion());                       // 0 - normal, 1 - TBC, must be set in database manually for each account
    s->SendPacket (&packet);

    s->SendAddonsInfo();

    WorldPacket pkt(SMSG_CLIENTCACHE_VERSION, 4);
    pkt << uint32(getConfig(CONFIG_UINT32_CLIENTCACHE_VERSION));
    s->SendPacket(&pkt);

    s->SendTutorialsData();

    UpdateMaxSessionCounters ();

    // Updates the population
    if (pLimit > 0)
    {
        float popu = float(GetActiveSessionCount());        // updated number of users on the server
        popu /= pLimit;
        popu *= 2;
        LoginDatabase.PExecute ("UPDATE realmlist SET population = '%f' WHERE id = '%u'", popu, realmID);
        DETAIL_LOG("Server Population (%f).", popu);
    }
}

int32 World::GetQueuedSessionPos(WorldSession* sess)
{
    uint32 position = 1;

    for(Queue::const_iterator iter = m_QueuedSessions.begin(); iter != m_QueuedSessions.end(); ++iter, ++position)
        if((*iter) == sess)
            return position;

    return 0;
}

void World::AddQueuedSession(WorldSession* sess)
{
    sess->SetInQueue(true);
    m_QueuedSessions.push_back (sess);

    // The 1st SMSG_AUTH_RESPONSE needs to contain other info too.
    WorldPacket packet (SMSG_AUTH_RESPONSE, 1 + 4 + 1 + 4 + 1 + 4 + 1);
    packet << uint8(AUTH_WAIT_QUEUE);
    packet << uint32(0);                                    // BillingTimeRemaining
    packet << uint8(0);                                     // BillingPlanFlags
    packet << uint32(0);                                    // BillingTimeRested
    packet << uint8(sess->Expansion());                     // 0 - normal, 1 - TBC, must be set in database manually for each account
    packet << uint32(GetQueuedSessionPos(sess));            // position in queue
    packet << uint8(0);                                     // unk 3.3.0
    sess->SendPacket(&packet);
}

bool World::RemoveQueuedSession(WorldSession* sess)
{
    // sessions count including queued to remove (if removed_session set)
    uint32 sessions = GetActiveSessionCount();

    uint32 position = 1;
    Queue::iterator iter = m_QueuedSessions.begin();

    // search to remove and count skipped positions
    bool found = false;

    for(;iter != m_QueuedSessions.end(); ++iter, ++position)
    {
        if(*iter==sess)
        {
            sess->SetInQueue(false);
            iter = m_QueuedSessions.erase(iter);
            found = true;                                   // removing queued session
            break;
        }
    }

    // iter point to next socked after removed or end()
    // position store position of removed socket and then new position next socket after removed

    // if session not queued then we need decrease sessions count
    if(!found && sessions)
        --sessions;

    // accept first in queue
    if( (!m_playerLimit || (int32)sessions < m_playerLimit) && !m_QueuedSessions.empty() )
    {
        WorldSession* pop_sess = m_QueuedSessions.front();
        pop_sess->SetInQueue(false);
        pop_sess->SendAuthWaitQue(0);
        pop_sess->SendAddonsInfo();

        WorldPacket pkt(SMSG_CLIENTCACHE_VERSION, 4);
        pkt << uint32(getConfig(CONFIG_UINT32_CLIENTCACHE_VERSION));
        pop_sess->SendPacket(&pkt);

        pop_sess->SendAccountDataTimes(GLOBAL_CACHE_MASK);
        pop_sess->SendTutorialsData();

        m_QueuedSessions.pop_front();

        // update iter to point first queued socket or end() if queue is empty now
        iter = m_QueuedSessions.begin();
        position = 1;
    }

    // update position from iter to end()
    // iter point to first not updated socket, position store new position
    for(; iter != m_QueuedSessions.end(); ++iter, ++position)
        (*iter)->SendAuthWaitQue(position);

    return found;
}

/// Find a Weather object by the given zoneid
Weather* World::FindWeather(uint32 id) const
{
    WeatherMap::const_iterator itr = m_weathers.find(id);

    if(itr != m_weathers.end())
        return itr->second;
    else
        return 0;
}

/// Remove a Weather object for the given zoneid
void World::RemoveWeather(uint32 id)
{
    // not called at the moment. Kept for completeness
    WeatherMap::iterator itr = m_weathers.find(id);

    if(itr != m_weathers.end())
    {
        delete itr->second;
        m_weathers.erase(itr);
    }
}

/// Add a Weather object to the list
Weather* World::AddWeather(uint32 zone_id)
{
    WeatherZoneChances const* weatherChances = sObjectMgr.GetWeatherChances(zone_id);

    // zone not have weather, ignore
    if(!weatherChances)
        return NULL;

    Weather* w = new Weather(zone_id,weatherChances);
    m_weathers[w->GetZone()] = w;
    w->ReGenerate();
    w->UpdateWeather();
    return w;
}

/// Initialize config values
void World::LoadConfigSettings(bool reload)
{
    if (reload)
    {
        if (!sConfig.Reload())
        {
            sLog.outError("World settings reload fail: can't read settings from %s.",sConfig.GetFilename().c_str());
            return;
        }
    }

    ///- Read the version of the configuration file and warn the user in case of emptiness or mismatch
    uint32 confVersion = sConfig.GetIntDefault("ConfVersion", 0);
    if (!confVersion)
    {
        sLog.outError("*****************************************************************************");
        sLog.outError(" WARNING: mangosd.conf does not include a ConfVersion variable.");
        sLog.outError("          Your configuration file may be out of date!");
        sLog.outError("*****************************************************************************");
        Log::WaitBeforeContinueIfNeed();
    }
    else
    {
        if (confVersion < _MANGOSDCONFVERSION)
        {
            sLog.outError("*****************************************************************************");
            sLog.outError(" WARNING: Your mangosd.conf version indicates your conf file is out of date!");
            sLog.outError("          Please check for updates, as your current default values may cause");
            sLog.outError("          unexpected behavior.");
            sLog.outError("*****************************************************************************");
            Log::WaitBeforeContinueIfNeed();
        }
    }

    ///- Read the player limit and the Message of the day from the config file
    SetPlayerLimit( sConfig.GetIntDefault("PlayerLimit", DEFAULT_PLAYER_LIMIT), true );
    SetMotd( sConfig.GetStringDefault("Motd", "Welcome to the Massive Network Game Object Server." ) );

    ///- Read all rates from the config file
    setConfigPos(CONFIG_FLOAT_RATE_HEALTH, "Rate.Health", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_POWER_MANA, "Rate.Mana", 1.0f);
    setConfig(CONFIG_FLOAT_RATE_POWER_RAGE_INCOME, "Rate.Rage.Income", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_POWER_RAGE_LOSS, "Rate.Rage.Loss", 1.0f);
    setConfig(CONFIG_FLOAT_RATE_POWER_RUNICPOWER_INCOME, "Rate.RunicPower.Income", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_POWER_RUNICPOWER_LOSS,"Rate.RunicPower.Loss",   1.0f);
    setConfig(CONFIG_FLOAT_RATE_POWER_FOCUS,             "Rate.Focus",  1.0f);
    setConfig(CONFIG_FLOAT_RATE_POWER_ENERGY,            "Rate.Energy", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_SKILL_DISCOVERY,      "Rate.Skill.Discovery",      1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_DROP_ITEM_POOR,       "Rate.Drop.Item.Poor",       1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_DROP_ITEM_NORMAL,     "Rate.Drop.Item.Normal",     1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_DROP_ITEM_UNCOMMON,   "Rate.Drop.Item.Uncommon",   1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_DROP_ITEM_RARE,       "Rate.Drop.Item.Rare",       1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_DROP_ITEM_EPIC,       "Rate.Drop.Item.Epic",       1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_DROP_ITEM_LEGENDARY,  "Rate.Drop.Item.Legendary",  1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_DROP_ITEM_ARTIFACT,   "Rate.Drop.Item.Artifact",   1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_DROP_ITEM_REFERENCED, "Rate.Drop.Item.Referenced", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_DROP_MONEY,           "Rate.Drop.Money", 1.0f);
    setConfig(CONFIG_FLOAT_RATE_XP_KILL,    "Rate.XP.Kill",    1.0f);
    setConfig(CONFIG_FLOAT_RATE_XP_QUEST,   "Rate.XP.Quest",   1.0f);
    setConfig(CONFIG_FLOAT_RATE_XP_EXPLORE, "Rate.XP.Explore", 1.0f);
    setConfig(CONFIG_FLOAT_RATE_REPUTATION_GAIN,           "Rate.Reputation.Gain", 1.0f);
    setConfig(CONFIG_FLOAT_RATE_REPUTATION_LOWLEVEL_KILL,  "Rate.Reputation.LowLevel.Kill", 1.0f);
    setConfig(CONFIG_FLOAT_RATE_REPUTATION_LOWLEVEL_QUEST, "Rate.Reputation.LowLevel.Quest", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_NORMAL_DAMAGE,          "Rate.Creature.Normal.Damage", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_ELITE_ELITE_DAMAGE,     "Rate.Creature.Elite.Elite.Damage", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_ELITE_RAREELITE_DAMAGE, "Rate.Creature.Elite.RAREELITE.Damage", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_ELITE_WORLDBOSS_DAMAGE, "Rate.Creature.Elite.WORLDBOSS.Damage", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_ELITE_RARE_DAMAGE,      "Rate.Creature.Elite.RARE.Damage", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_NORMAL_HP,          "Rate.Creature.Normal.HP", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_ELITE_ELITE_HP,     "Rate.Creature.Elite.Elite.HP", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_ELITE_RAREELITE_HP, "Rate.Creature.Elite.RAREELITE.HP", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_ELITE_WORLDBOSS_HP, "Rate.Creature.Elite.WORLDBOSS.HP", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_ELITE_RARE_HP,      "Rate.Creature.Elite.RARE.HP", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_NORMAL_SPELLDAMAGE,          "Rate.Creature.Normal.SpellDamage", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_ELITE_ELITE_SPELLDAMAGE,     "Rate.Creature.Elite.Elite.SpellDamage", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_ELITE_RAREELITE_SPELLDAMAGE, "Rate.Creature.Elite.RAREELITE.SpellDamage", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_ELITE_WORLDBOSS_SPELLDAMAGE, "Rate.Creature.Elite.WORLDBOSS.SpellDamage", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_ELITE_RARE_SPELLDAMAGE,      "Rate.Creature.Elite.RARE.SpellDamage", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CREATURE_AGGRO, "Rate.Creature.Aggro", 1.0f);
    setConfig(CONFIG_FLOAT_RATE_REST_INGAME,                    "Rate.Rest.InGame", 1.0f);
    setConfig(CONFIG_FLOAT_RATE_REST_OFFLINE_IN_TAVERN_OR_CITY, "Rate.Rest.Offline.InTavernOrCity", 1.0f);
    setConfig(CONFIG_FLOAT_RATE_REST_OFFLINE_IN_WILDERNESS,     "Rate.Rest.Offline.InWilderness", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_DAMAGE_FALL,  "Rate.Damage.Fall", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_AUCTION_TIME, "Rate.Auction.Time", 1.0f);
    setConfig(CONFIG_FLOAT_RATE_AUCTION_DEPOSIT, "Rate.Auction.Deposit", 1.0f);
    setConfig(CONFIG_FLOAT_RATE_AUCTION_CUT,     "Rate.Auction.Cut", 1.0f);
    setConfigPos(CONFIG_UINT32_AUCTION_DEPOSIT_MIN, "Auction.Deposit.Min", SILVER);
    setConfig(CONFIG_FLOAT_RATE_HONOR, "Rate.Honor",1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_MINING_AMOUNT, "Rate.Mining.Amount", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_MINING_NEXT,   "Rate.Mining.Next", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_INSTANCE_RESET_TIME, "Rate.InstanceResetTime", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_TALENT, "Rate.Talent", 1.0f);
    setConfigPos(CONFIG_FLOAT_RATE_CORPSE_DECAY_LOOTED, "Rate.Corpse.Decay.Looted", 0.0f);

    setConfigMinMax(CONFIG_FLOAT_RATE_TARGET_POS_RECALCULATION_RANGE, "TargetPosRecalculateRange", 1.5f, CONTACT_DISTANCE, ATTACK_DISTANCE);

    setConfigPos(CONFIG_FLOAT_RATE_DURABILITY_LOSS_DAMAGE, "DurabilityLossChance.Damage", 0.5f);
    setConfigPos(CONFIG_FLOAT_RATE_DURABILITY_LOSS_ABSORB, "DurabilityLossChance.Absorb", 0.5f);
    setConfigPos(CONFIG_FLOAT_RATE_DURABILITY_LOSS_PARRY,  "DurabilityLossChance.Parry",  0.05f);
    setConfigPos(CONFIG_FLOAT_RATE_DURABILITY_LOSS_BLOCK,  "DurabilityLossChance.Block",  0.05f);

    setConfigPos(CONFIG_FLOAT_LISTEN_RANGE_SAY,       "ListenRange.Say",       25.0f);
    setConfigPos(CONFIG_FLOAT_LISTEN_RANGE_YELL,      "ListenRange.Yell",     300.0f);
    setConfigPos(CONFIG_FLOAT_LISTEN_RANGE_TEXTEMOTE, "ListenRange.TextEmote", 25.0f);

    setConfigPos(CONFIG_FLOAT_GROUP_XP_DISTANCE, "MaxGroupXPDistance", 74.0f);
    setConfigPos(CONFIG_FLOAT_SIGHT_GUARDER,     "GuarderSight",       50.0f);
    setConfigPos(CONFIG_FLOAT_SIGHT_MONSTER,     "MonsterSight",       50.0f);

    setConfigPos(CONFIG_FLOAT_CREATURE_FAMILY_ASSISTANCE_RADIUS,      "CreatureFamilyAssistanceRadius",     10.0f);
    setConfigPos(CONFIG_FLOAT_CREATURE_FAMILY_FLEE_ASSISTANCE_RADIUS, "CreatureFamilyFleeAssistanceRadius", 30.0f);

    ///- Read other configuration items from the config file

    // movement anticheat
    setConfig(CONFIG_BOOL_ANTICHEAT_ENABLE,              "Anticheat.Enable", false);
    setConfigPos(CONFIG_UINT32_ANTICHEAT_ACTION_DELAY,   "Anticheat.DelayAfterAction",30);
    setConfigPos(CONFIG_UINT32_ANTICHEAT_GMLEVEL,        "Anticheat.GmLevel",0);

    setConfigMinMax(CONFIG_UINT32_COMPRESSION, "Compression", 1, 1, 9);
    setConfig(CONFIG_BOOL_ADDON_CHANNEL, "AddonChannel", true);
    setConfig(CONFIG_BOOL_CLEAN_CHARACTER_DB, "CleanCharacterDB", true);
    setConfig(CONFIG_BOOL_GRID_UNLOAD, "GridUnload", true);
    setConfigPos(CONFIG_UINT32_INTERVAL_SAVE, "PlayerSave.Interval", 15 * MINUTE * IN_MILLISECONDS);
    setConfigMinMax(CONFIG_UINT32_MIN_LEVEL_STAT_SAVE, "PlayerSave.Stats.MinLevel", 0, 0, MAX_LEVEL);
    setConfig(CONFIG_BOOL_STATS_SAVE_ONLY_ON_LOGOUT, "PlayerSave.Stats.SaveOnlyOnLogout", true);

    setConfigMin(CONFIG_UINT32_INTERVAL_GRIDCLEAN, "GridCleanUpDelay", 5 * MINUTE * IN_MILLISECONDS, MIN_GRID_DELAY);
    if (reload)
        sMapMgr.SetGridCleanUpDelay(getConfig(CONFIG_UINT32_INTERVAL_GRIDCLEAN));

    setConfig(CONFIG_UINT32_NUMTHREADS, "MapUpdate.Threads", 3);

    setConfigMin(CONFIG_UINT32_INTERVAL_MAPUPDATE, "MapUpdateInterval", 100, MIN_MAP_UPDATE_DELAY);
    if (reload)
        sMapMgr.SetMapUpdateInterval(getConfig(CONFIG_UINT32_INTERVAL_MAPUPDATE));

    setConfig(CONFIG_UINT32_INTERVAL_CHANGEWEATHER, "ChangeWeatherInterval", 10 * MINUTE * IN_MILLISECONDS);

    if (configNoReload(reload, CONFIG_UINT32_PORT_WORLD, "WorldServerPort", DEFAULT_WORLDSERVER_PORT))
        setConfig(CONFIG_UINT32_PORT_WORLD, "WorldServerPort", DEFAULT_WORLDSERVER_PORT);

    if (configNoReload(reload, CONFIG_UINT32_GAME_TYPE, "GameType", 0))
        setConfig(CONFIG_UINT32_GAME_TYPE, "GameType", 0);

    if (configNoReload(reload, CONFIG_UINT32_REALM_ZONE, "RealmZone", REALM_ZONE_DEVELOPMENT))
        setConfig(CONFIG_UINT32_REALM_ZONE, "RealmZone", REALM_ZONE_DEVELOPMENT);

    setConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_ACCOUNTS,            "AllowTwoSide.Accounts", true);
    setConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_CHAT,    "AllowTwoSide.Interaction.Chat", false);
    setConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_CHANNEL, "AllowTwoSide.Interaction.Channel", false);
    setConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_GROUP,   "AllowTwoSide.Interaction.Group", false);
    setConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_GUILD,   "AllowTwoSide.Interaction.Guild", false);
    setConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_AUCTION, "AllowTwoSide.Interaction.Auction", false);
    setConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_MAIL,    "AllowTwoSide.Interaction.Mail", false);
    setConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_WHO_LIST,            "AllowTwoSide.WhoList", false);
    setConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_ADD_FRIEND,          "AllowTwoSide.AddFriend", false);

    setConfig(CONFIG_UINT32_STRICT_PLAYER_NAMES,  "StrictPlayerNames",  0);
    setConfig(CONFIG_UINT32_STRICT_CHARTER_NAMES, "StrictCharterNames", 0);
    setConfig(CONFIG_UINT32_STRICT_PET_NAMES,     "StrictPetNames",     0);

    setConfigMinMax(CONFIG_UINT32_MIN_PLAYER_NAME,  "MinPlayerName",  2, 1, MAX_PLAYER_NAME);
    setConfigMinMax(CONFIG_UINT32_MIN_CHARTER_NAME, "MinCharterName", 2, 1, MAX_CHARTER_NAME);
    setConfigMinMax(CONFIG_UINT32_MIN_PET_NAME,     "MinPetName",     2, 1, MAX_PET_NAME);

    setConfig(CONFIG_UINT32_CHARACTERS_CREATING_DISABLED, "CharactersCreatingDisabled", 0);

    setConfigMinMax(CONFIG_UINT32_CHARACTERS_PER_REALM, "CharactersPerRealm", 10, 1, 10);

    // must be after CONFIG_UINT32_CHARACTERS_PER_REALM
    setConfigMin(CONFIG_UINT32_CHARACTERS_PER_ACCOUNT, "CharactersPerAccount", 50, getConfig(CONFIG_UINT32_CHARACTERS_PER_REALM));

    setConfigMinMax(CONFIG_UINT32_HEROIC_CHARACTERS_PER_REALM, "HeroicCharactersPerRealm", 1, 1, 10);

    setConfig(CONFIG_UINT32_MIN_LEVEL_FOR_HEROIC_CHARACTER_CREATING, "MinLevelForHeroicCharacterCreating", 55);

    setConfigMinMax(CONFIG_UINT32_SKIP_CINEMATICS, "SkipCinematics", 0, 0, 2);

    if (configNoReload(reload, CONFIG_UINT32_MAX_PLAYER_LEVEL, "MaxPlayerLevel", DEFAULT_MAX_LEVEL))
        setConfigMinMax(CONFIG_UINT32_MAX_PLAYER_LEVEL, "MaxPlayerLevel", DEFAULT_MAX_LEVEL, 1, DEFAULT_MAX_LEVEL);

    setConfigMinMax(CONFIG_UINT32_START_PLAYER_LEVEL, "StartPlayerLevel", 1, 1, getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
    setConfigMinMax(CONFIG_UINT32_START_HEROIC_PLAYER_LEVEL, "StartHeroicPlayerLevel", 55, 1, getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));

    setConfigMinMax(CONFIG_UINT32_RAF_MAXGRANTLEVEL, "RAF.MaxGrantLevel", 60, 1, 80);
    setConfigMinMax(CONFIG_UINT32_RAF_MAXREFERALS, "RAF.MaxReferals", 5, 0, 15);
    setConfigMinMax(CONFIG_UINT32_RAF_MAXREFERERS, "RAF.MaxReferers", 5, 0, 15);
    setConfig(CONFIG_FLOAT_RATE_RAF_XP, "Rate.RAF.XP", 3.0f);
    setConfig(CONFIG_FLOAT_RATE_RAF_LEVELPERLEVEL, "Rate.RAF.XP", 0.5f);

    setConfigMinMax(CONFIG_UINT32_START_PLAYER_MONEY, "StartPlayerMoney", 0, 0, MAX_MONEY_AMOUNT);

    setConfigPos(CONFIG_UINT32_MAX_HONOR_POINTS, "MaxHonorPoints", 75000);

    setConfigMinMax(CONFIG_UINT32_START_HONOR_POINTS, "StartHonorPoints", 0, 0, getConfig(CONFIG_UINT32_MAX_HONOR_POINTS));

    setConfigPos(CONFIG_UINT32_MAX_ARENA_POINTS, "MaxArenaPoints", 5000);

    setConfigMinMax(CONFIG_UINT32_START_ARENA_POINTS, "StartArenaPoints", 0, 0, getConfig(CONFIG_UINT32_MAX_ARENA_POINTS));

    setConfig(CONFIG_BOOL_ALL_TAXI_PATHS, "AllFlightPaths", false);

    setConfig(CONFIG_BOOL_INSTANCE_IGNORE_LEVEL, "Instance.IgnoreLevel", false);
    setConfig(CONFIG_BOOL_INSTANCE_IGNORE_RAID,  "Instance.IgnoreRaid", false);

    setConfig(CONFIG_BOOL_CAST_UNSTUCK, "CastUnstuck", true);
    setConfig(CONFIG_UINT32_MAX_SPELL_CASTS_IN_CHAIN, "MaxSpellCastsInChain", 10);
    setConfig(CONFIG_UINT32_INSTANCE_RESET_TIME_HOUR, "Instance.ResetTimeHour", 4);
    setConfig(CONFIG_UINT32_INSTANCE_UNLOAD_DELAY,    "Instance.UnloadDelay", 30 * MINUTE * IN_MILLISECONDS);

    setConfig(CONFIG_UINT32_MAX_PRIMARY_TRADE_SKILL, "MaxPrimaryTradeSkill", 2);
    setConfigMinMax(CONFIG_UINT32_MIN_PETITION_SIGNS, "MinPetitionSigns", 9, 0, 9);

    setConfig(CONFIG_UINT32_GM_LOGIN_STATE,    "GM.LoginState",    2);
    setConfig(CONFIG_UINT32_GM_VISIBLE_STATE,  "GM.Visible",       2);
    setConfig(CONFIG_UINT32_GM_ACCEPT_TICKETS, "GM.AcceptTickets", 2);
    setConfig(CONFIG_UINT32_GM_CHAT,           "GM.Chat",          2);
    setConfig(CONFIG_UINT32_GM_WISPERING_TO,   "GM.WhisperingTo",  2);

    setConfig(CONFIG_UINT32_GM_LEVEL_IN_GM_LIST,  "GM.InGMList.Level",  SEC_ADMINISTRATOR);
    setConfig(CONFIG_UINT32_GM_LEVEL_IN_WHO_LIST, "GM.InWhoList.Level", SEC_ADMINISTRATOR);
    setConfig(CONFIG_BOOL_GM_LOG_TRADE,           "GM.LogTrade", false);

    setConfigMinMax(CONFIG_UINT32_START_GM_LEVEL, "GM.StartLevel", 1, getConfig(CONFIG_UINT32_START_PLAYER_LEVEL), MAX_LEVEL);
    setConfig(CONFIG_BOOL_GM_LOWER_SECURITY, "GM.LowerSecurity", false);
    setConfig(CONFIG_BOOL_GM_ALLOW_ACHIEVEMENT_GAINS, "GM.AllowAchievementGain", true);

    setConfig(CONFIG_UINT32_GROUP_VISIBILITY, "Visibility.GroupMode", 0);

    setConfig(CONFIG_UINT32_MAIL_DELIVERY_DELAY, "MailDeliveryDelay", HOUR);

    setConfigMin(CONFIG_UINT32_MASS_MAILER_SEND_PER_TICK, "MassMailer.SendPerTick", 10, 1);

    setConfigPos(CONFIG_UINT32_UPTIME_UPDATE, "UpdateUptimeInterval", 10);
    if (reload)
    {
        m_timers[WUPDATE_UPTIME].SetInterval(getConfig(CONFIG_UINT32_UPTIME_UPDATE)*MINUTE*IN_MILLISECONDS);
        m_timers[WUPDATE_UPTIME].Reset();
    }

    setConfig(CONFIG_UINT32_SKILL_CHANCE_ORANGE, "SkillChance.Orange", 100);
    setConfig(CONFIG_UINT32_SKILL_CHANCE_YELLOW, "SkillChance.Yellow", 75);
    setConfig(CONFIG_UINT32_SKILL_CHANCE_GREEN,  "SkillChance.Green",  25);
    setConfig(CONFIG_UINT32_SKILL_CHANCE_GREY,   "SkillChance.Grey",   0);

    setConfigPos(CONFIG_UINT32_SKILL_CHANCE_MINING_STEPS,   "SkillChance.MiningSteps",   75);
    setConfigPos(CONFIG_UINT32_SKILL_CHANCE_SKINNING_STEPS, "SkillChance.SkinningSteps", 75);

    setConfig(CONFIG_BOOL_SKILL_PROSPECTING, "SkillChance.Prospecting", false);
    setConfig(CONFIG_BOOL_SKILL_MILLING,     "SkillChance.Milling",     false);

    setConfigPos(CONFIG_UINT32_SKILL_GAIN_CRAFTING,  "SkillGain.Crafting",  1);
    setConfigPos(CONFIG_UINT32_SKILL_GAIN_DEFENSE,   "SkillGain.Defense",   1);
    setConfigPos(CONFIG_UINT32_SKILL_GAIN_GATHERING, "SkillGain.Gathering", 1);
    setConfig(CONFIG_UINT32_SKILL_GAIN_WEAPON,       "SkillGain.Weapon",    1);

    setConfig(CONFIG_BOOL_SKILL_FAIL_LOOT_FISHING,         "SkillFail.Loot.Fishing", true);
    setConfig(CONFIG_BOOL_SKILL_FAIL_GAIN_FISHING,         "SkillFail.Gain.Fishing", true);
    setConfig(CONFIG_BOOL_SKILL_FAIL_POSSIBLE_FISHINGPOOL, "SkillFail.Possible.FishingPool", false);

    setConfig(CONFIG_UINT32_MAX_OVERSPEED_PINGS, "MaxOverspeedPings", 2);
    if (getConfig(CONFIG_UINT32_MAX_OVERSPEED_PINGS) != 0 && getConfig(CONFIG_UINT32_MAX_OVERSPEED_PINGS) < 2)
    {
        sLog.outError("MaxOverspeedPings (%i) must be in range 2..infinity (or 0 to disable check). Set to 2.", getConfig(CONFIG_UINT32_MAX_OVERSPEED_PINGS));
        setConfig(CONFIG_UINT32_MAX_OVERSPEED_PINGS, 2);
    }

    setConfig(CONFIG_BOOL_SAVE_RESPAWN_TIME_IMMEDIATELY, "SaveRespawnTimeImmediately", true);
    setConfig(CONFIG_BOOL_WEATHER, "ActivateWeather", true);

    setConfig(CONFIG_BOOL_ALWAYS_MAX_SKILL_FOR_LEVEL, "AlwaysMaxSkillForLevel", false);

    if (configNoReload(reload, CONFIG_UINT32_EXPANSION, "Expansion", MAX_EXPANSION))
        setConfigMinMax(CONFIG_UINT32_EXPANSION, "Expansion", MAX_EXPANSION, 0, MAX_EXPANSION);

    setConfig(CONFIG_UINT32_CHATFLOOD_MESSAGE_COUNT, "ChatFlood.MessageCount", 10);
    setConfig(CONFIG_UINT32_CHATFLOOD_MESSAGE_DELAY, "ChatFlood.MessageDelay", 1);
    setConfig(CONFIG_UINT32_CHATFLOOD_MUTE_TIME,     "ChatFlood.MuteTime", 10);

    setConfig(CONFIG_BOOL_EVENT_ANNOUNCE, "Event.Announce", false);

    setConfig(CONFIG_UINT32_CREATURE_FAMILY_ASSISTANCE_DELAY, "CreatureFamilyAssistanceDelay", 1500);
    setConfig(CONFIG_UINT32_CREATURE_FAMILY_FLEE_DELAY,       "CreatureFamilyFleeDelay",       7000);

    setConfig(CONFIG_UINT32_WORLD_BOSS_LEVEL_DIFF, "WorldBossLevelDiff", 3);

    // note: disable value (-1) will assigned as 0xFFFFFFF, to prevent overflow at calculations limit it to max possible player level MAX_LEVEL(100)
    setConfig(CONFIG_UINT32_QUEST_LOW_LEVEL_HIDE_DIFF, "Quests.LowLevelHideDiff", 4);
    if (getConfig(CONFIG_UINT32_QUEST_LOW_LEVEL_HIDE_DIFF) > MAX_LEVEL)
        setConfig(CONFIG_UINT32_QUEST_LOW_LEVEL_HIDE_DIFF, MAX_LEVEL);
    setConfig(CONFIG_UINT32_QUEST_HIGH_LEVEL_HIDE_DIFF, "Quests.HighLevelHideDiff", 7);
    if (getConfig(CONFIG_UINT32_QUEST_HIGH_LEVEL_HIDE_DIFF) > MAX_LEVEL)
        setConfig(CONFIG_UINT32_QUEST_HIGH_LEVEL_HIDE_DIFF, MAX_LEVEL);

    setConfigMinMax(CONFIG_UINT32_QUEST_DAILY_RESET_HOUR, "Quests.Daily.ResetHour", 6, 0, 23);
    setConfigMinMax(CONFIG_UINT32_QUEST_WEEKLY_RESET_WEEK_DAY, "Quests.Weekly.ResetWeekDay", 3, 0, 6);
    setConfigMinMax(CONFIG_UINT32_QUEST_WEEKLY_RESET_HOUR, "Quests.Weekly.ResetHour", 6, 0 , 23);

    setConfig(CONFIG_BOOL_QUEST_IGNORE_RAID, "Quests.IgnoreRaid", false);

    setConfig(CONFIG_BOOL_LOOT_CHESTS_IGNORE_DB, "Loot.IgnoreChestGroupRulesFromDB", false);

    setConfig(CONFIG_BOOL_DETECT_POS_COLLISION, "DetectPosCollision", true);

    setConfig(CONFIG_BOOL_RESTRICTED_LFG_CHANNEL,      "Channel.RestrictedLfg", true);
    setConfig(CONFIG_BOOL_SILENTLY_GM_JOIN_TO_CHANNEL, "Channel.SilentlyGMJoin", false);

    setConfig(CONFIG_BOOL_TALENTS_INSPECTING,           "TalentsInspecting", true);
    setConfig(CONFIG_BOOL_CHAT_FAKE_MESSAGE_PREVENTING, "ChatFakeMessagePreventing", false);

    setConfig(CONFIG_UINT32_CHAT_STRICT_LINK_CHECKING_SEVERITY, "ChatStrictLinkChecking.Severity", 0);
    setConfig(CONFIG_UINT32_CHAT_STRICT_LINK_CHECKING_KICK,     "ChatStrictLinkChecking.Kick", 0);

    setConfig(CONFIG_BOOL_CORPSE_EMPTY_LOOT_SHOW,      "Corpse.EmptyLootShow", true);
    setConfigPos(CONFIG_UINT32_CORPSE_DECAY_NORMAL,    "Corpse.Decay.NORMAL",    300);
    setConfigPos(CONFIG_UINT32_CORPSE_DECAY_RARE,      "Corpse.Decay.RARE",      900);
    setConfigPos(CONFIG_UINT32_CORPSE_DECAY_ELITE,     "Corpse.Decay.ELITE",     600);
    setConfigPos(CONFIG_UINT32_CORPSE_DECAY_RAREELITE, "Corpse.Decay.RAREELITE", 1200);
    setConfigPos(CONFIG_UINT32_CORPSE_DECAY_WORLDBOSS, "Corpse.Decay.WORLDBOSS", 3600);

    setConfig(CONFIG_INT32_DEATH_SICKNESS_LEVEL, "Death.SicknessLevel", 11);

    setConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVP, "Death.CorpseReclaimDelay.PvP", true);
    setConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVE, "Death.CorpseReclaimDelay.PvE", true);
    setConfig(CONFIG_BOOL_DEATH_BONES_WORLD,              "Death.Bones.World", true);
    setConfig(CONFIG_BOOL_DEATH_BONES_BG_OR_ARENA,        "Death.Bones.BattlegroundOrArena", true);
    setConfigMinMax(CONFIG_FLOAT_GHOST_RUN_SPEED_WORLD,   "Death.Ghost.RunSpeed.World", 1.0f, 0.1f, 10.0f);
    setConfigMinMax(CONFIG_FLOAT_GHOST_RUN_SPEED_BG,      "Death.Ghost.RunSpeed.Battleground", 1.0f, 0.1f, 10.0f);

    setConfig(CONFIG_FLOAT_THREAT_RADIUS, "ThreatRadius", 100.0f);

    // always use declined names in the russian client
    if (getConfig(CONFIG_UINT32_REALM_ZONE) == REALM_ZONE_RUSSIAN)
        setConfig(CONFIG_BOOL_DECLINED_NAMES_USED, true);
    else
        setConfig(CONFIG_BOOL_DECLINED_NAMES_USED, "DeclinedNames", false);

    setConfig(CONFIG_BOOL_BATTLEGROUND_CAST_DESERTER,                  "Battleground.CastDeserter", true);
    setConfigMinMax(CONFIG_UINT32_BATTLEGROUND_QUEUE_ANNOUNCER_JOIN,   "Battleground.QueueAnnouncer.Join", 0, 0, 2);
    setConfig(CONFIG_BOOL_BATTLEGROUND_QUEUE_ANNOUNCER_START,          "Battleground.QueueAnnouncer.Start", false);
    setConfig(CONFIG_UINT32_BATTLEGROUND_INVITATION_TYPE,              "Battleground.InvitationType", 0);
    setConfig(CONFIG_UINT32_BATTLEGROUND_PREMATURE_FINISH_TIMER,       "BattleGround.PrematureFinishTimer", 5 * MINUTE * IN_MILLISECONDS);
    setConfig(CONFIG_UINT32_BATTLEGROUND_PREMADE_GROUP_WAIT_FOR_MATCH, "BattleGround.PremadeGroupWaitForMatch", 30 * MINUTE * IN_MILLISECONDS);
    setConfigMinMax(CONFIG_UINT32_RANDOM_BG_RESET_HOUR,                "BattleGround.Random.ResetHour", 6, 0, 23);
    setConfig(CONFIG_UINT32_ARENA_MAX_RATING_DIFFERENCE,               "Arena.MaxRatingDifference", 150);
    setConfig(CONFIG_UINT32_ARENA_RATING_DISCARD_TIMER,                "Arena.RatingDiscardTimer", 10 * MINUTE * IN_MILLISECONDS);
    setConfig(CONFIG_BOOL_ARENA_AUTO_DISTRIBUTE_POINTS,                "Arena.AutoDistributePoints", false);
    setConfig(CONFIG_UINT32_ARENA_AUTO_DISTRIBUTE_INTERVAL_DAYS,       "Arena.AutoDistributeInterval", 7);
    setConfig(CONFIG_BOOL_ARENA_QUEUE_ANNOUNCER_JOIN,                  "Arena.QueueAnnouncer.Join", false);
    setConfig(CONFIG_BOOL_ARENA_QUEUE_ANNOUNCER_EXIT,                  "Arena.QueueAnnouncer.Exit", false);
    setConfig(CONFIG_UINT32_ARENA_SEASON_ID,                           "Arena.ArenaSeason.ID", 1);
    setConfig(CONFIG_UINT32_ARENA_SEASON_PREVIOUS_ID,                  "Arena.ArenaSeasonPrevious.ID", 0);
    setConfigMin(CONFIG_INT32_ARENA_STARTRATING,                       "Arena.StartRating", -1, -1);
    setConfigMin(CONFIG_INT32_ARENA_STARTPERSONALRATING,               "Arena.StartPersonalRating", -1, -1);

    setConfig(CONFIG_BOOL_OFFHAND_CHECK_AT_TALENTS_RESET, "OffhandCheckAtTalentsReset", false);

    setConfig(CONFIG_BOOL_KICK_PLAYER_ON_BAD_PACKET, "Network.KickOnBadPacket", false);

    if(int clientCacheId = sConfig.GetIntDefault("ClientCacheVersion", 0))
    {
        // overwrite DB/old value
        if(clientCacheId > 0)
        {
            setConfig(CONFIG_UINT32_CLIENTCACHE_VERSION, clientCacheId);
            sLog.outString("Client cache version set to: %u", clientCacheId);
        }
        else
            sLog.outError("ClientCacheVersion can't be negative %d, ignored.", clientCacheId);
    }

    setConfig(CONFIG_UINT32_INSTANT_LOGOUT, "InstantLogout", SEC_MODERATOR);

    setConfigMin(CONFIG_UINT32_GUILD_EVENT_LOG_COUNT, "Guild.EventLogRecordsCount", GUILD_EVENTLOG_MAX_RECORDS, GUILD_EVENTLOG_MAX_RECORDS);
    setConfigMin(CONFIG_UINT32_GUILD_BANK_EVENT_LOG_COUNT, "Guild.BankEventLogRecordsCount", GUILD_BANK_MAX_LOGS, GUILD_BANK_MAX_LOGS);

    setConfig(CONFIG_UINT32_TIMERBAR_FATIGUE_GMLEVEL, "TimerBar.Fatigue.GMLevel", SEC_CONSOLE);
    setConfig(CONFIG_UINT32_TIMERBAR_FATIGUE_MAX,     "TimerBar.Fatigue.Max", 60);
    setConfig(CONFIG_UINT32_TIMERBAR_BREATH_GMLEVEL,  "TimerBar.Breath.GMLevel", SEC_CONSOLE);
    setConfig(CONFIG_UINT32_TIMERBAR_BREATH_MAX,      "TimerBar.Breath.Max", 180);
    setConfig(CONFIG_UINT32_TIMERBAR_FIRE_GMLEVEL,    "TimerBar.Fire.GMLevel", SEC_CONSOLE);
    setConfig(CONFIG_UINT32_TIMERBAR_FIRE_MAX,        "TimerBar.Fire.Max", 1);

    setConfig(CONFIG_BOOL_PET_UNSUMMON_AT_MOUNT,      "PetUnsummonAtMount", true);

    setConfig(CONFIG_BOOL_ALLOW_FLIGHT_ON_OLD_MAPS, "AllowFlightOnOldMaps", false);

    m_VisibleUnitGreyDistance = sConfig.GetFloatDefault("Visibility.Distance.Grey.Unit", 1);
    if(m_VisibleUnitGreyDistance >  MAX_VISIBILITY_DISTANCE)
    {
        sLog.outError("Visibility.Distance.Grey.Unit can't be greater %f",MAX_VISIBILITY_DISTANCE);
        m_VisibleUnitGreyDistance = MAX_VISIBILITY_DISTANCE;
    }
    m_VisibleObjectGreyDistance = sConfig.GetFloatDefault("Visibility.Distance.Grey.Object", 10);
    if(m_VisibleObjectGreyDistance >  MAX_VISIBILITY_DISTANCE)
    {
        sLog.outError("Visibility.Distance.Grey.Object can't be greater %f",MAX_VISIBILITY_DISTANCE);
        m_VisibleObjectGreyDistance = MAX_VISIBILITY_DISTANCE;
    }

    //visibility on continents
    m_MaxVisibleDistanceOnContinents      = sConfig.GetFloatDefault("Visibility.Distance.Continents",     DEFAULT_VISIBILITY_DISTANCE);
    if(m_MaxVisibleDistanceOnContinents < 45*getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO))
    {
        sLog.outError("Visibility.Distance.Continents can't be less max aggro radius %f", 45*getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO));
        m_MaxVisibleDistanceOnContinents = 45*getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO);
    }
    else if(m_MaxVisibleDistanceOnContinents + m_VisibleUnitGreyDistance >  MAX_VISIBILITY_DISTANCE)
    {
        sLog.outError("Visibility.Distance.Continents can't be greater %f",MAX_VISIBILITY_DISTANCE - m_VisibleUnitGreyDistance);
        m_MaxVisibleDistanceOnContinents = MAX_VISIBILITY_DISTANCE - m_VisibleUnitGreyDistance;
    }

    //visibility in instances
    m_MaxVisibleDistanceInInstances        = sConfig.GetFloatDefault("Visibility.Distance.Instances",       DEFAULT_VISIBILITY_INSTANCE);
    if(m_MaxVisibleDistanceInInstances < 45*getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO))
    {
        sLog.outError("Visibility.Distance.Instances can't be less max aggro radius %f",45*getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO));
        m_MaxVisibleDistanceInInstances = 45*getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO);
    }
    else if(m_MaxVisibleDistanceInInstances + m_VisibleUnitGreyDistance >  MAX_VISIBILITY_DISTANCE)
    {
        sLog.outError("Visibility.Distance.Instances can't be greater %f",MAX_VISIBILITY_DISTANCE - m_VisibleUnitGreyDistance);
        m_MaxVisibleDistanceInInstances = MAX_VISIBILITY_DISTANCE - m_VisibleUnitGreyDistance;
    }

    //visibility in BG/Arenas
    m_MaxVisibleDistanceInBGArenas        = sConfig.GetFloatDefault("Visibility.Distance.BGArenas",       DEFAULT_VISIBILITY_BGARENAS);
    if(m_MaxVisibleDistanceInBGArenas < 45*getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO))
    {
        sLog.outError("Visibility.Distance.BGArenas can't be less max aggro radius %f",45*getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO));
        m_MaxVisibleDistanceInBGArenas = 45*getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO);
    }
    else if(m_MaxVisibleDistanceInBGArenas + m_VisibleUnitGreyDistance >  MAX_VISIBILITY_DISTANCE)
    {
        sLog.outError("Visibility.Distance.BGArenas can't be greater %f",MAX_VISIBILITY_DISTANCE - m_VisibleUnitGreyDistance);
        m_MaxVisibleDistanceInBGArenas = MAX_VISIBILITY_DISTANCE - m_VisibleUnitGreyDistance;
    }

    m_MaxVisibleDistanceForObject    = sConfig.GetFloatDefault("Visibility.Distance.Object",   DEFAULT_VISIBILITY_DISTANCE);
    if(m_MaxVisibleDistanceForObject < INTERACTION_DISTANCE)
    {
        sLog.outError("Visibility.Distance.Object can't be less max aggro radius %f",float(INTERACTION_DISTANCE));
        m_MaxVisibleDistanceForObject = INTERACTION_DISTANCE;
    }
    else if(m_MaxVisibleDistanceForObject + m_VisibleObjectGreyDistance >  MAX_VISIBILITY_DISTANCE)
    {
        sLog.outError("Visibility.Distance.Object can't be greater %f",MAX_VISIBILITY_DISTANCE-m_VisibleObjectGreyDistance);
        m_MaxVisibleDistanceForObject = MAX_VISIBILITY_DISTANCE - m_VisibleObjectGreyDistance;
    }
    m_MaxVisibleDistanceInFlight    = sConfig.GetFloatDefault("Visibility.Distance.InFlight",      DEFAULT_VISIBILITY_DISTANCE);
    if(m_MaxVisibleDistanceInFlight + m_VisibleObjectGreyDistance > MAX_VISIBILITY_DISTANCE)
    {
        sLog.outError("Visibility.Distance.InFlight can't be greater %f",MAX_VISIBILITY_DISTANCE-m_VisibleObjectGreyDistance);
        m_MaxVisibleDistanceInFlight = MAX_VISIBILITY_DISTANCE - m_VisibleObjectGreyDistance;
    }

    ///- Load the CharDelete related config options
    setConfigMinMax(CONFIG_UINT32_CHARDELETE_METHOD, "CharDelete.Method", 0, 0, 1);
    setConfigMinMax(CONFIG_UINT32_CHARDELETE_MIN_LEVEL, "CharDelete.MinLevel", 0, 0, getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
    setConfigPos(CONFIG_UINT32_CHARDELETE_KEEP_DAYS, "CharDelete.KeepDays", 30);

    ///- Read the "Data" directory from the config file
    std::string dataPath = sConfig.GetStringDefault("DataDir","./");
    if( dataPath.at(dataPath.length()-1)!='/' && dataPath.at(dataPath.length()-1)!='\\' )
        dataPath.append("/");

    if(reload)
    {
        if(dataPath!=m_dataPath)
            sLog.outError("DataDir option can't be changed at mangosd.conf reload, using current value (%s).",m_dataPath.c_str());
    }
    else
    {
        m_dataPath = dataPath;
        sLog.outString("Using DataDir %s",m_dataPath.c_str());
    }

    setConfig(CONFIG_BOOL_VMAP_INDOOR_CHECK, "vmap.enableIndoorCheck", true);
    bool enableLOS = sConfig.GetBoolDefault("vmap.enableLOS", false);
    bool enableHeight = sConfig.GetBoolDefault("vmap.enableHeight", false);
    std::string ignoreSpellIds = sConfig.GetStringDefault("vmap.ignoreSpellIds", "");

    if (!enableHeight)
        sLog.outError("VMAP height use disabled! Creatures movements and other things will be in broken state.");

    VMAP::VMapFactory::createOrGetVMapManager()->setEnableLineOfSightCalc(enableLOS);
    VMAP::VMapFactory::createOrGetVMapManager()->setEnableHeightCalc(enableHeight);
    VMAP::VMapFactory::preventSpellsFromBeingTestedForLoS(ignoreSpellIds.c_str());
    sLog.outString( "WORLD: VMap support included. LineOfSight:%i, getHeight:%i, indoorCheck:%i",
        enableLOS, enableHeight, getConfig(CONFIG_BOOL_VMAP_INDOOR_CHECK) ? 1 : 0);
    sLog.outString( "WORLD: VMap data directory is: %svmaps",m_dataPath.c_str());
}

/// Initialize the World
void World::SetInitialWorldSettings()
{
    ///- Initialize the random number generator
    srand((unsigned int)time(NULL));

    ///- Time server startup
    uint32 uStartTime = WorldTimer::getMSTime();

    ///- Initialize config settings
    LoadConfigSettings();

    ///- Check the existence of the map files for all races start areas.
    if (!MapManager::ExistMapAndVMap(0,-6240.32f, 331.033f) ||
        !MapManager::ExistMapAndVMap(0,-8949.95f,-132.493f) ||
        !MapManager::ExistMapAndVMap(0,-8949.95f,-132.493f) ||
        !MapManager::ExistMapAndVMap(1,-618.518f,-4251.67f) ||
        !MapManager::ExistMapAndVMap(0, 1676.35f, 1677.45f) ||
        !MapManager::ExistMapAndVMap(1, 10311.3f, 832.463f) ||
        !MapManager::ExistMapAndVMap(1,-2917.58f,-257.98f) ||
        (m_configUint32Values[CONFIG_UINT32_EXPANSION] &&
        (!MapManager::ExistMapAndVMap(530,10349.6f,-6357.29f) || !MapManager::ExistMapAndVMap(530,-3961.64f,-13931.2f))))
    {
        sLog.outError("Correct *.map files not found in path '%smaps' or *.vmtree/*.vmtile files in '%svmaps'. Please place *.map and vmap files in appropriate directories or correct the DataDir value in the mangosd.conf file.",m_dataPath.c_str(),m_dataPath.c_str());
        Log::WaitBeforeContinueIfNeed();
        exit(1);
    }

    ///- Loading strings. Getting no records means core load has to be canceled because no error message can be output.
    sLog.outString();
    sLog.outString("Loading MaNGOS strings...");
    if (!sObjectMgr.LoadMangosStrings())
    {
        Log::WaitBeforeContinueIfNeed();
        exit(1);                                            // Error message displayed in function already
    }

    ///- Update the realm entry in the database with the realm type from the config file
    //No SQL injection as values are treated as integers

    // not send custom type REALM_FFA_PVP to realm list
    uint32 server_type = IsFFAPvPRealm() ? REALM_TYPE_PVP : getConfig(CONFIG_UINT32_GAME_TYPE);
    uint32 realm_zone = getConfig(CONFIG_UINT32_REALM_ZONE);
    LoginDatabase.PExecute("UPDATE realmlist SET icon = %u, timezone = %u WHERE id = '%u'", server_type, realm_zone, realmID);

    ///- Remove the bones (they should not exist in DB though) and old corpses after a restart
    CharacterDatabase.PExecute("DELETE FROM corpse WHERE corpse_type = '0' OR time < (UNIX_TIMESTAMP()-'%u')", 3*DAY);

    ///- Load the DBC files
    sLog.outString("Initialize data stores...");
    LoadDBCStores(m_dataPath);
    DetectDBCLang();
    sObjectMgr.SetDBCLocaleIndex(GetDefaultDbcLocale());    // Get once for all the locale index of DBC language (console/broadcasts)

    sLog.outString( "Loading Script Names...");
    sScriptMgr.LoadScriptNames();

    sLog.outString( "Loading InstanceTemplate..." );
    sObjectMgr.LoadInstanceTemplate();

    sLog.outString( "Loading SkillLineAbilityMultiMap Data..." );
    sSpellMgr.LoadSkillLineAbilityMap();

    ///- Clean up and pack instances
    sLog.outString( "Cleaning up instances..." );
    sInstanceSaveMgr.CleanupInstances();                    // must be called before `creature_respawn`/`gameobject_respawn` tables

    sLog.outString( "Packing instances..." );
    sInstanceSaveMgr.PackInstances();

    sLog.outString( "Packing groups..." );
    sObjectMgr.PackGroupIds();                              // must be after CleanupInstances

    ///- Init highest guids before any guid using table loading to prevent using not initialized guids in some code.
    sObjectMgr.SetHighestGuids();                           // must be after packing instances
    sLog.outString();

    sLog.outString( "Loading Page Texts..." );
    sObjectMgr.LoadPageTexts();

    sLog.outString( "Loading Game Object Templates..." );   // must be after LoadPageTexts
    sObjectMgr.LoadGameobjectInfo();

    sLog.outString( "Loading Spell Chain Data..." );
    sSpellMgr.LoadSpellChains();

    sLog.outString( "Loading Spell Elixir types..." );
    sSpellMgr.LoadSpellElixirs();

    sLog.outString( "Loading Spell Learn Skills..." );
    sSpellMgr.LoadSpellLearnSkills();                       // must be after LoadSpellChains

    sLog.outString( "Loading Spell Learn Spells..." );
    sSpellMgr.LoadSpellLearnSpells();

    sLog.outString( "Loading Spell Proc Event conditions..." );
    sSpellMgr.LoadSpellProcEvents();

    sLog.outString( "Loading Spell Bonus Data..." );
    sSpellMgr.LoadSpellBonuses();                           // must be after LoadSpellChains

    sLog.outString( "Loading Spell Proc Item Enchant..." );
    sSpellMgr.LoadSpellProcItemEnchant();                   // must be after LoadSpellChains

    sLog.outString( "Loading Aggro Spells Definitions...");
    sSpellMgr.LoadSpellThreats();

    sLog.outString( "Loading NPC Texts..." );
    sObjectMgr.LoadGossipText();

    sLog.outString( "Loading Item Random Enchantments Table..." );
    LoadRandomEnchantmentsTable();

    sLog.outString( "Loading Items..." );                   // must be after LoadRandomEnchantmentsTable and LoadPageTexts
    sObjectMgr.LoadItemPrototypes();

    sLog.outString( "Loading Item converts..." );           // must be after LoadItemPrototypes
    sObjectMgr.LoadItemConverts();

    sLog.outString( "Loading Creature Model Based Info Data..." );
    sObjectMgr.LoadCreatureModelInfo();

    sLog.outString( "Loading Equipment templates...");
    sObjectMgr.LoadEquipmentTemplates();

    sLog.outString( "Loading Creature templates..." );
    sObjectMgr.LoadCreatureTemplates();

    sLog.outString( "Loading Creature Model for race..." ); // must be after creature templates
    sObjectMgr.LoadCreatureModelRace();

    sLog.outString( "Loading SpellsScriptTarget...");
    sSpellMgr.LoadSpellScriptTarget();                      // must be after LoadCreatureTemplates and LoadGameobjectInfo

    sLog.outString( "Loading ItemRequiredTarget...");
    sObjectMgr.LoadItemRequiredTarget();

    sLog.outString( "Loading Reputation Reward Rates...");
    sObjectMgr.LoadReputationRewardRate();

    sLog.outString( "Loading Creature Reputation OnKill Data..." );
    sObjectMgr.LoadReputationOnKill();

    sLog.outString( "Loading Reputation Spillover Data..." );
    sObjectMgr.LoadReputationSpilloverTemplate();

    sLog.outString( "Loading Points Of Interest Data..." );
    sObjectMgr.LoadPointsOfInterest();

    sLog.outString( "Loading Creature Data..." );
    sObjectMgr.LoadCreatures();

    sLog.outString( "Loading pet levelup spells..." );
    sSpellMgr.LoadPetLevelupSpellMap();

    sLog.outString( "Loading pet default spell additional to levelup spells..." );
    sSpellMgr.LoadPetDefaultSpells();

    sLog.outString( "Loading Creature Addon Data..." );
    sLog.outString();
    sObjectMgr.LoadCreatureAddons();                        // must be after LoadCreatureTemplates() and LoadCreatures()
    sLog.outString( ">>> Creature Addon Data loaded" );
    sLog.outString();

    sLog.outString("Loading Vehicle Accessories...");
    sObjectMgr.LoadVehicleAccessories();

    sLog.outString( "Loading Creature Respawn Data..." );   // must be after PackInstances()
    sObjectMgr.LoadCreatureRespawnTimes();

    sLog.outString( "Loading Gameobject Data..." );
    sObjectMgr.LoadGameobjects();

    sLog.outString( "Loading Gameobject Respawn Data..." ); // must be after PackInstances()
    sObjectMgr.LoadGameobjectRespawnTimes();

    sLog.outString( "Loading Objects Pooling Data...");
    sPoolMgr.LoadFromDB();

    sLog.outString( "Loading Weather Data..." );
    sObjectMgr.LoadWeatherZoneChances();

    sLog.outString( "Loading Quests..." );
    sObjectMgr.LoadQuests();                                // must be loaded after DBCs, creature_template, item_template, gameobject tables

    sLog.outString( "Loading Quest POI" );
    sObjectMgr.LoadQuestPOI();

    sLog.outString( "Loading Quests Relations..." );
    sLog.outString();
    sObjectMgr.LoadQuestRelations();                        // must be after quest load
    sLog.outString( ">>> Quests Relations loaded" );
    sLog.outString();

    sLog.outString( "Loading Game Event Data...");          // must be after sPoolMgr.LoadFromDB and quests to properly load pool events and quests for events
    sLog.outString();
    sGameEventMgr.LoadFromDB();
    sLog.outString( ">>> Game Event Data loaded" );
    sLog.outString();

    sLog.outString( "Loading UNIT_NPC_FLAG_SPELLCLICK Data..." );
    sObjectMgr.LoadNPCSpellClickSpells();

    sLog.outString( "Loading SpellArea Data..." );          // must be after quest load
    sSpellMgr.LoadSpellAreas();

    sLog.outString( "Loading AreaTrigger definitions..." );
    sObjectMgr.LoadAreaTriggerTeleports();                  // must be after item template load

    sLog.outString( "Loading Quest Area Triggers..." );
    sObjectMgr.LoadQuestAreaTriggers();                     // must be after LoadQuests

    sLog.outString( "Loading Tavern Area Triggers..." );
    sObjectMgr.LoadTavernAreaTriggers();

    sLog.outString( "Loading AreaTrigger script names..." );
    sScriptMgr.LoadAreaTriggerScripts();

    sLog.outString( "Loading event id script names..." );
    sScriptMgr.LoadEventIdScripts();

    sLog.outString( "Loading Graveyard-zone links...");
    sObjectMgr.LoadGraveyardZones();

    sLog.outString( "Loading Spell target coordinates..." );
    sSpellMgr.LoadSpellTargetPositions();

    sLog.outString( "Loading spell pet auras..." );
    sSpellMgr.LoadSpellPetAuras();

    sLog.outString( "Loading Player Create Info & Level Stats..." );
    sLog.outString();
    sObjectMgr.LoadPlayerInfo();
    sLog.outString( ">>> Player Create Info & Level Stats loaded" );
    sLog.outString();

    sLog.outString( "Loading Exploration BaseXP Data..." );
    sObjectMgr.LoadExplorationBaseXP();

    sLog.outString( "Loading Pet Name Parts..." );
    sObjectMgr.LoadPetNames();

    CharacterDatabaseCleaner::CleanDatabase();

    sLog.outString( "Loading the max pet number..." );
    sObjectMgr.LoadPetNumber();

    sLog.outString( "Loading pet level stats..." );
    sObjectMgr.LoadPetLevelInfo();

    sLog.outString( "Loading pet scaling data..." );
    sObjectMgr.LoadPetScalingData();

    sLog.outString( "Loading Player Corpses..." );
    sObjectMgr.LoadCorpses();

    sLog.outString( "Loading Player level dependent mail rewards..." );
    sObjectMgr.LoadMailLevelRewards();

    sLog.outString( "Loading Spell disabled..." );
    sObjectMgr.LoadSpellDisabledEntrys();

    sLog.outString( "Loading Loot Tables..." );
    sLog.outString();
    LoadLootTables();
    sLog.outString( ">>> Loot Tables loaded" );
    sLog.outString();

    sLog.outString( "Loading Skill Discovery Table..." );
    LoadSkillDiscoveryTable();

    sLog.outString( "Loading Skill Extra Item Table..." );
    LoadSkillExtraItemTable();

    sLog.outString( "Loading Skill Fishing base level requirements..." );
    sObjectMgr.LoadFishingBaseSkillLevel();

    sLog.outString( "Loading Achievements..." );
    sLog.outString();
    sAchievementMgr.LoadAchievementReferenceList();
    sAchievementMgr.LoadAchievementCriteriaList();
    sAchievementMgr.LoadAchievementCriteriaRequirements();
    sAchievementMgr.LoadRewards();
    sAchievementMgr.LoadRewardLocales();
    sAchievementMgr.LoadCompletedAchievements();
    sLog.outString( ">>> Achievements loaded" );
    sLog.outString();

    sLog.outString( "Loading Npc Text Id..." );
    sObjectMgr.LoadNpcGossips();                            // must be after load Creature and LoadGossipText

    sLog.outString( "Loading Gossip scripts..." );
    sScriptMgr.LoadGossipScripts();                         // must be before gossip menu options

    sLog.outString( "Loading Gossip menus..." );
    sObjectMgr.LoadGossipMenu();

    sLog.outString( "Loading Gossip menu options..." );
    sObjectMgr.LoadGossipMenuItems();

    sLog.outString( "Loading Vendors..." );
    sObjectMgr.LoadVendorTemplates();                       // must be after load ItemTemplate
    sObjectMgr.LoadVendors();                               // must be after load CreatureTemplate, VendorTemplate, and ItemTemplate

    sLog.outString( "Loading Trainers..." );
    sObjectMgr.LoadTrainerTemplates();                      // must be after load CreatureTemplate
    sObjectMgr.LoadTrainers();                              // must be after load CreatureTemplate, TrainerTemplate

    sLog.outString( "Loading Waypoint scripts..." );        // before loading from creature_movement
    sScriptMgr.LoadCreatureMovementScripts();

    sLog.outString( "Loading Waypoints..." );
    sLog.outString();
    sWaypointMgr.Load();

    ///- Loading localization data
    sLog.outString( "Loading Localization strings..." );
    sObjectMgr.LoadCreatureLocales();                       // must be after CreatureInfo loading
    sObjectMgr.LoadGameObjectLocales();                     // must be after GameobjectInfo loading
    sObjectMgr.LoadItemLocales();                           // must be after ItemPrototypes loading
    sObjectMgr.LoadQuestLocales();                          // must be after QuestTemplates loading
    sObjectMgr.LoadGossipTextLocales();                     // must be after LoadGossipText
    sObjectMgr.LoadPageTextLocales();                       // must be after PageText loading
    sObjectMgr.LoadGossipMenuItemsLocales();                // must be after gossip menu items loading
    sObjectMgr.LoadPointOfInterestLocales();                // must be after POI loading
    sLog.outString( ">>> Localization strings loaded" );
    sLog.outString();

    ///- Load dynamic data tables from the database
    sLog.outString( "Loading Auctions..." );
    sLog.outString();
    sAuctionMgr.LoadAuctionItems();
    sAuctionMgr.LoadAuctions();
    sLog.outString( ">>> Auctions loaded" );
    sLog.outString();

    sLog.outString( "Loading Guilds..." );
    sObjectMgr.LoadGuilds();

    sLog.outString( "Loading ArenaTeams..." );
    sObjectMgr.LoadArenaTeams();

    sLog.outString( "Loading Groups..." );
    sObjectMgr.LoadGroups();

    sLog.outString( "Loading ReservedNames..." );
    sObjectMgr.LoadReservedPlayersNames();

    sLog.outString( "Loading GameObjects for quests..." );
    sObjectMgr.LoadGameObjectForQuests();

    sLog.outString( "Loading BattleMasters..." );
    sBattleGroundMgr.LoadBattleMastersEntry();

    sLog.outString( "Loading BattleGround event indexes..." );
    sBattleGroundMgr.LoadBattleEventIndexes();

    sLog.outString( "Loading GameTeleports..." );
    sObjectMgr.LoadGameTele();

    sLog.outString( "Loading GM tickets...");
    sTicketMgr.LoadGMTickets();

    sLog.outString( "Loading AntiCheat config..." );
    sObjectMgr.LoadAntiCheatConfig();

    ///- Handle outdated emails (delete/return)
    sLog.outString( "Returning old mails..." );
    sObjectMgr.ReturnOrDeleteOldMails(false);

    ///- Load and initialize scripts
    sLog.outString( "Loading Scripts..." );
    sLog.outString();
    sScriptMgr.LoadQuestStartScripts();                         // must be after load Creature/Gameobject(Template/Data) and QuestTemplate
    sScriptMgr.LoadQuestEndScripts();                           // must be after load Creature/Gameobject(Template/Data) and QuestTemplate
    sScriptMgr.LoadSpellScripts();                              // must be after load Creature/Gameobject(Template/Data)
    sScriptMgr.LoadGameObjectScripts();                         // must be after load Creature/Gameobject(Template/Data)
    sScriptMgr.LoadEventScripts();                              // must be after load Creature/Gameobject(Template/Data)
    sLog.outString( ">>> Scripts loaded" );
    sLog.outString();

    sLog.outString( "Loading Scripts text locales..." );    // must be after Load*Scripts calls
    sScriptMgr.LoadDbScriptStrings();

    sLog.outString( "Loading CreatureEventAI Texts...");
    sEventAIMgr.LoadCreatureEventAI_Texts(false);       // false, will checked in LoadCreatureEventAI_Scripts

    sLog.outString( "Loading CreatureEventAI Summons...");
    sEventAIMgr.LoadCreatureEventAI_Summons(false);     // false, will checked in LoadCreatureEventAI_Scripts

    sLog.outString( "Loading CreatureEventAI Scripts...");
    sEventAIMgr.LoadCreatureEventAI_Scripts();

    sLog.outString("Initializing Scripts...");
    switch(sScriptMgr.LoadScriptLibrary(MANGOS_SCRIPT_NAME))
    {
        case SCRIPT_LOAD_OK:
            sLog.outString("Scripting library loaded.");
            break;
        case SCRIPT_LOAD_ERR_NOT_FOUND:
            sLog.outError("Scripting library not found or not accessible.");
            break;
        case SCRIPT_LOAD_ERR_WRONG_API:
            sLog.outError("Scripting library has wrong list functions (outdated?).");
            break;
        case SCRIPT_LOAD_ERR_OUTDATED:
            sLog.outError("Scripting library build for old mangosd revision. You need rebuild it.");
            break;
    }

    ///- Initialize game time and timers
    sLog.outString( "DEBUG:: Initialize game time and timers" );
    m_gameTime = time(NULL);
    m_startTime=m_gameTime;

    tm local;
    time_t curr;
    time(&curr);
    local=*(localtime(&curr));                              // dereference and assign
    char isoDate[128];
    sprintf( isoDate, "%04d-%02d-%02d %02d:%02d:%02d",
        local.tm_year+1900, local.tm_mon+1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);

    LoginDatabase.PExecute("INSERT INTO uptime (realmid, starttime, startstring, uptime) VALUES('%u', " UI64FMTD ", '%s', 0)", realmID, uint64(m_startTime), isoDate);

    static uint32 abtimer = 0;
    abtimer = sConfig.GetIntDefault("AutoBroadcast.Timer", 60000);

    m_timers[WUPDATE_OBJECTS].SetInterval(0);
    m_timers[WUPDATE_SESSIONS].SetInterval(0);
    m_timers[WUPDATE_WEATHERS].SetInterval(1*IN_MILLISECONDS);
    m_timers[WUPDATE_AUCTIONS].SetInterval(MINUTE*IN_MILLISECONDS);
    m_timers[WUPDATE_UPTIME].SetInterval(getConfig(CONFIG_UINT32_UPTIME_UPDATE)*MINUTE*IN_MILLISECONDS);
                                                            //Update "uptime" table based on configuration entry in minutes.
    m_timers[WUPDATE_CORPSES].SetInterval(20*MINUTE*IN_MILLISECONDS);
    m_timers[WUPDATE_DELETECHARS].SetInterval(DAY*IN_MILLISECONDS); // check for chars to delete every day
    m_timers[WUPDATE_AUTOBROADCAST].SetInterval(abtimer);

    //to set mailtimer to return mails every day between 4 and 5 am
    //mailtimer is increased when updating auctions
    //one second is 1000 -(tested on win system)
    mail_timer = uint32((((localtime( &m_gameTime )->tm_hour + 20) % 24)* HOUR * IN_MILLISECONDS) / m_timers[WUPDATE_AUCTIONS].GetInterval() );
                                                            //1440
    mail_timer_expires = uint32( (DAY * IN_MILLISECONDS) / (m_timers[WUPDATE_AUCTIONS].GetInterval()));
    DEBUG_LOG("Mail timer set to: %u, mail return is called every %u minutes", mail_timer, mail_timer_expires);

    ///- Initialize static helper structures
    AIRegistry::Initialize();
    Player::InitVisibleBits();

    ///- Initialize MapManager
    sLog.outString( "Starting Map System" );
    sMapMgr.Initialize();

    ///- Initialize Battlegrounds
    sLog.outString( "Starting BattleGround System" );
    sBattleGroundMgr.CreateInitialBattleGrounds();
    sBattleGroundMgr.InitAutomaticArenaPointDistribution();

    //Not sure if this can be moved up in the sequence (with static data loading) as it uses MapManager
    sLog.outString( "Loading Transports..." );
    sMapMgr.LoadTransports();

    sLog.outString("Deleting expired bans..." );
    LoginDatabase.Execute("DELETE FROM ip_banned WHERE unbandate<=UNIX_TIMESTAMP() AND unbandate<>bandate");

    sLog.outString("Calculate next daily quest reset time..." );
    InitDailyQuestResetTime();

    sLog.outString("Calculate next weekly quest reset time..." );
    InitWeeklyQuestResetTime();

    sLog.outString("Calculate next monthly quest reset time..." );
    SetMonthlyQuestResetTime();

    sLog.outString("Calculate random battleground reset time..." );
    InitRandomBGResetTime();

    sLog.outString("Starting objects Pooling system..." );
    sPoolMgr.Initialize();

    sLog.outString("Starting Game Event system..." );
    uint32 nextGameEvent = sGameEventMgr.Initialize();
    m_timers[WUPDATE_EVENTS].SetInterval(nextGameEvent);    //depend on next event

    // Delete all characters which have been deleted X days before
    Player::DeleteOldCharacters();

    sLog.outString("Initialize AuctionHouseBot...");
    auctionbot.Initialize();

    sLog.outString("Starting Autobroadcast system by Xeross..." );

    sLog.outString( "WORLD: World initialized" );

    uint32 uStartInterval = WorldTimer::getMSTimeDiff(uStartTime, WorldTimer::getMSTime());
    sLog.outString( "SERVER STARTUP TIME: %i minutes %i seconds", uStartInterval / 60000, (uStartInterval % 60000) / 1000 );
}

void World::DetectDBCLang()
{
    uint32 m_lang_confid = sConfig.GetIntDefault("DBC.Locale", 255);

    if (m_lang_confid != 255 && m_lang_confid >= MAX_LOCALE)
    {
        sLog.outError("Incorrect DBC.Locale! Must be >= 0 and < %d (set to 0)",MAX_LOCALE);
        m_lang_confid = LOCALE_enUS;
    }

    ChrRacesEntry const* race = sChrRacesStore.LookupEntry(1);

    std::string availableLocalsStr;

    uint32 default_locale = MAX_LOCALE;
    for (int i = MAX_LOCALE-1; i >= 0; --i)
    {
        if (strlen(race->name[i]) > 0)                      // check by race names
        {
            default_locale = i;
            m_availableDbcLocaleMask |= (1 << i);
            availableLocalsStr += localeNames[i];
            availableLocalsStr += " ";
        }
    }

    if (default_locale != m_lang_confid && m_lang_confid < MAX_LOCALE &&
        (m_availableDbcLocaleMask & (1 << m_lang_confid)) )
    {
        default_locale = m_lang_confid;
    }

    if (default_locale >= MAX_LOCALE)
    {
        sLog.outError("Unable to determine your DBC Locale! (corrupt DBC?)");
        Log::WaitBeforeContinueIfNeed();
        exit(1);
    }

    m_defaultDbcLocale = LocaleConstant(default_locale);

    sLog.outString("Using %s DBC Locale as default. All available DBC locales: %s",localeNames[m_defaultDbcLocale],availableLocalsStr.empty() ? "<none>" : availableLocalsStr.c_str());
    sLog.outString();
}

/// Update the World !
void World::Update(uint32 diff)
{
    ///- Update the different timers
    for(int i = 0; i < WUPDATE_COUNT; ++i)
    {
        if (m_timers[i].GetCurrent()>=0)
            m_timers[i].Update(diff);
        else
            m_timers[i].SetCurrent(0);
    }

    ///- Update the game time and check for shutdown time
    _UpdateGameTime();

    ///-Update mass mailer tasks if any
    sMassMailMgr.Update();

    /// Handle daily quests reset time
    if (m_gameTime > m_NextDailyQuestReset)
        ResetDailyQuests();

    /// Handle weekly quests reset time
    if (m_gameTime > m_NextWeeklyQuestReset)
        ResetWeeklyQuests();

    /// Handle monthly quests reset time
    if (m_gameTime > m_NextMonthlyQuestReset)
        ResetMonthlyQuests();

    if (m_gameTime > m_NextRandomBGReset)
        ResetRandomBG();

    /// <ul><li> Handle auctions when the timer has passed
    if (m_timers[WUPDATE_AUCTIONS].Passed())
    {
        auctionbot.Update();
        m_timers[WUPDATE_AUCTIONS].Reset();

        ///- Update mails (return old mails with item, or delete them)
        //(tested... works on win)
        if (++mail_timer > mail_timer_expires)
        {
            mail_timer = 0;
            sObjectMgr.ReturnOrDeleteOldMails(true);
        }

        ///- Handle expired auctions
        sAuctionMgr.Update();
    }

    /// <li> Handle session updates when the timer has passed
    if (m_timers[WUPDATE_SESSIONS].Passed())
    {
        m_timers[WUPDATE_SESSIONS].Reset();

        UpdateSessions(diff);
    }

    /// <li> Handle weather updates when the timer has passed
    if (m_timers[WUPDATE_WEATHERS].Passed())
    {
        ///- Send an update signal to Weather objects
        for (WeatherMap::iterator itr = m_weathers.begin(); itr != m_weathers.end(); )
        {
            ///- and remove Weather objects for zones with no player
                                                            //As interval > WorldTick
            if(!itr->second->Update(m_timers[WUPDATE_WEATHERS].GetInterval()))
            {
                delete itr->second;
                m_weathers.erase(itr++);
            }
            else
                ++itr;
        }

        m_timers[WUPDATE_WEATHERS].SetCurrent(0);
    }
    /// <li> Update uptime table
    if (m_timers[WUPDATE_UPTIME].Passed())
    {
        uint32 tmpDiff = uint32(m_gameTime - m_startTime);
        uint32 maxClientsNum = GetMaxActiveSessionCount();

        m_timers[WUPDATE_UPTIME].Reset();
        LoginDatabase.PExecute("UPDATE uptime SET uptime = %u, maxplayers = %u WHERE realmid = %u AND starttime = " UI64FMTD, tmpDiff, maxClientsNum, realmID, uint64(m_startTime));
    }

    /// <li> Handle all other objects
    if (m_timers[WUPDATE_OBJECTS].Passed())
    {
        m_timers[WUPDATE_OBJECTS].Reset();
        ///- Update objects when the timer has passed (maps, transport, creatures,...)
        sMapMgr.Update(diff);                // As interval = 0

        sBattleGroundMgr.Update(diff);
    }

    ///- Delete all characters which have been deleted X days before
    if (m_timers[WUPDATE_DELETECHARS].Passed())
    {
        m_timers[WUPDATE_DELETECHARS].Reset();
        Player::DeleteOldCharacters();
    }

    // execute callbacks from sql queries that were queued recently
    UpdateResultQueue();

    ///- Erase corpses once every 20 minutes
    if (m_timers[WUPDATE_CORPSES].Passed())
    {
        m_timers[WUPDATE_CORPSES].Reset();

        sObjectAccessor.RemoveOldCorpses();
    }

    ///- Process Game events when necessary
    if (m_timers[WUPDATE_EVENTS].Passed())
    {
        m_timers[WUPDATE_EVENTS].Reset();                   // to give time for Update() to be processed
        uint32 nextGameEvent = sGameEventMgr.Update();
        m_timers[WUPDATE_EVENTS].SetInterval(nextGameEvent);
        m_timers[WUPDATE_EVENTS].Reset();
    }
    static uint32 autobroadcaston = 0;
    autobroadcaston = sConfig.GetIntDefault("AutoBroadcast.On", 0);
    if(autobroadcaston == 1)
    {
        if (m_timers[WUPDATE_AUTOBROADCAST].Passed())
        {
            m_timers[WUPDATE_AUTOBROADCAST].Reset();
            SendBroadcast();
        }
    }

    /// </ul>
    ///- Move all creatures with "delayed move" and remove and delete all objects with "delayed remove"
    sMapMgr.RemoveAllObjectsInRemoveList();

    // update the instance reset times
    sInstanceSaveMgr.Update();

    // And last, but not least handle the issued cli commands
    ProcessCliCommands();

    //cleanup unused GridMap objects as well as VMaps
    sTerrainMgr.Update(diff);
}

/// Send a packet to all players (except self if mentioned)
void World::SendGlobalMessage(WorldPacket *packet, WorldSession *self, uint32 team, AccountTypes security)
{
    SessionMap::const_iterator itr;
    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second &&
            itr->second->GetPlayer() &&
            itr->second->GetPlayer()->IsInWorld() &&
            itr->second->GetSecurity() >= security &&
            itr->second != self &&
            (team == 0 || itr->second->GetPlayer()->GetTeam() == team) )
        {
            itr->second->SendPacket(packet);
        }
    }
}

namespace MaNGOS
{
    class WorldWorldTextBuilder
    {
        public:
            typedef std::vector<WorldPacket*> WorldPacketList;
            explicit WorldWorldTextBuilder(int32 textId, va_list* args = NULL) : i_textId(textId), i_args(args) {}
            void operator()(WorldPacketList& data_list, int32 loc_idx)
            {
                char const* text = sObjectMgr.GetMangosString(i_textId,loc_idx);

                if(i_args)
                {
                    // we need copy va_list before use or original va_list will corrupted
                    va_list ap;
                    va_copy(ap,*i_args);

                    char str [2048];
                    vsnprintf(str,2048,text, ap );
                    va_end(ap);

                    do_helper(data_list,&str[0]);
                }
                else
                    do_helper(data_list,(char*)text);
            }
        private:
            char* lineFromMessage(char*& pos) { char* start = strtok(pos,"\n"); pos = NULL; return start; }
            void do_helper(WorldPacketList& data_list, char* text)
            {
                char* pos = text;

                while(char* line = lineFromMessage(pos))
                {
                    WorldPacket* data = new WorldPacket();

                    uint32 lineLength = (line ? strlen(line) : 0) + 1;

                    data->Initialize(SMSG_MESSAGECHAT, 100);                // guess size
                    *data << uint8(CHAT_MSG_SYSTEM);
                    *data << uint32(LANG_UNIVERSAL);
                    *data << uint64(0);
                    *data << uint32(0);                                     // can be chat msg group or something
                    *data << uint64(0);
                    *data << uint32(lineLength);
                    *data << line;
                    *data << uint8(0);

                    data_list.push_back(data);
                }
            }

            int32 i_textId;
            va_list* i_args;
    };
}                                                           // namespace MaNGOS

/// Send a System Message to all players (except self if mentioned)
void World::SendWorldText(int32 string_id, ...)
{
    va_list ap;
    va_start(ap, string_id);

    MaNGOS::WorldWorldTextBuilder wt_builder(string_id, &ap);
    MaNGOS::LocalizedPacketListDo<MaNGOS::WorldWorldTextBuilder> wt_do(wt_builder);
    for(SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if(!itr->second || !itr->second->GetPlayer() || !itr->second->GetPlayer()->IsInWorld() )
            continue;

        wt_do(itr->second->GetPlayer());
    }

    va_end(ap);
}

/// Send a System Message to all players (except self if mentioned)
void World::SendWorldTextWithSecurity(AccountTypes security, int32 string_id, ...)
{
    va_list ap;
    va_start(ap, string_id);

    MaNGOS::WorldWorldTextBuilder wt_builder(string_id, &ap);
    MaNGOS::LocalizedPacketListDo<MaNGOS::WorldWorldTextBuilder> wt_do(wt_builder);
    for(SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if(!itr->second || !itr->second->GetPlayer() || !itr->second->GetPlayer()->IsInWorld() || itr->second->GetSecurity() < security )
            continue;

        wt_do(itr->second->GetPlayer());
    }

    va_end(ap);
}

/// DEPRICATED, only for debug purpose. Send a System Message to all players (except self if mentioned)
void World::SendGlobalText(const char* text, WorldSession *self)
{
    WorldPacket data;

    // need copy to prevent corruption by strtok call in LineFromMessage original string
    char* buf = mangos_strdup(text);
    char* pos = buf;

    while(char* line = ChatHandler::LineFromMessage(pos))
    {
        ChatHandler::FillMessageData(&data, NULL, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, NULL, 0, line, NULL);
        SendGlobalMessage(&data, self);
    }

    delete [] buf;
}

/// Send a packet to all players (or players selected team) in the zone (except self if mentioned)
void World::SendZoneMessage(uint32 zone, WorldPacket *packet, WorldSession *self, uint32 team)
{
    SessionMap::const_iterator itr;
    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second &&
            itr->second->GetPlayer() &&
            itr->second->GetPlayer()->IsInWorld() &&
            itr->second->GetPlayer()->GetZoneId() == zone &&
            itr->second != self &&
            (team == 0 || itr->second->GetPlayer()->GetTeam() == team) )
        {
            itr->second->SendPacket(packet);
        }
    }
}

/// Send a System Message to all players in the zone (except self if mentioned)
void World::SendZoneText(uint32 zone, const char* text, WorldSession *self, uint32 team)
{
    WorldPacket data;
    ChatHandler::FillMessageData(&data, NULL, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, NULL, 0, text, NULL);
    SendZoneMessage(zone, &data, self,team);
}

/// Kick (and save) all players
void World::KickAll()
{
    m_QueuedSessions.clear();                               // prevent send queue update packet and login queued sessions

    // session not removed at kick and will removed in next update tick
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        itr->second->KickPlayer();
}

/// Kick (and save) all players with security level less `sec`
void World::KickAllLess(AccountTypes sec)
{
    // session not removed at kick and will removed in next update tick
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if(itr->second->GetSecurity() < sec)
            itr->second->KickPlayer();
}

/// Ban an account or ban an IP address, duration_secs if it is positive used, otherwise permban
BanReturn World::BanAccount(BanMode mode, std::string nameOrIP, uint32 duration_secs, std::string reason, std::string author)
{
    LoginDatabase.escape_string(nameOrIP);
    LoginDatabase.escape_string(reason);
    std::string safe_author=author;
    LoginDatabase.escape_string(safe_author);

    QueryResult *resultAccounts = NULL;                     //used for kicking

    ///- Update the database with ban information
    switch(mode)
    {
        case BAN_IP:
            //No SQL injection as strings are escaped
            resultAccounts = LoginDatabase.PQuery("SELECT id FROM account WHERE last_ip = '%s'",nameOrIP.c_str());
            LoginDatabase.PExecute("INSERT INTO ip_banned VALUES ('%s',UNIX_TIMESTAMP(),UNIX_TIMESTAMP()+%u,'%s','%s')",nameOrIP.c_str(),duration_secs,safe_author.c_str(),reason.c_str());
            break;
        case BAN_ACCOUNT:
            //No SQL injection as string is escaped
            resultAccounts = LoginDatabase.PQuery("SELECT id FROM account WHERE username = '%s'",nameOrIP.c_str());
            break;
        case BAN_CHARACTER:
            //No SQL injection as string is escaped
            resultAccounts = CharacterDatabase.PQuery("SELECT account FROM characters WHERE name = '%s'",nameOrIP.c_str());
            break;
        default:
            return BAN_SYNTAX_ERROR;
    }

    if(!resultAccounts)
    {
        if(mode==BAN_IP)
            return BAN_SUCCESS;                             // ip correctly banned but nobody affected (yet)
        else
            return BAN_NOTFOUND;                                // Nobody to ban
    }

    ///- Disconnect all affected players (for IP it can be several)
    do
    {
        Field* fieldsAccount = resultAccounts->Fetch();
        uint32 account = fieldsAccount->GetUInt32();

        if(mode!=BAN_IP)
        {
            //No SQL injection as strings are escaped
            LoginDatabase.PExecute("INSERT INTO account_banned VALUES ('%u', UNIX_TIMESTAMP(), UNIX_TIMESTAMP()+%u, '%s', '%s', '1')",
                account,duration_secs,safe_author.c_str(),reason.c_str());
        }

        if (WorldSession* sess = FindSession(account))
            if(std::string(sess->GetPlayerName()) != author)
                sess->KickPlayer();
    }
    while( resultAccounts->NextRow() );

    delete resultAccounts;
    return BAN_SUCCESS;
}

/// Remove a ban from an account or IP address
bool World::RemoveBanAccount(BanMode mode, std::string nameOrIP)
{
    if (mode == BAN_IP)
    {
        LoginDatabase.escape_string(nameOrIP);
        LoginDatabase.PExecute("DELETE FROM ip_banned WHERE ip = '%s'",nameOrIP.c_str());
    }
    else
    {
        uint32 account = 0;
        if (mode == BAN_ACCOUNT)
            account = sAccountMgr.GetId (nameOrIP);
        else if (mode == BAN_CHARACTER)
            account = sObjectMgr.GetPlayerAccountIdByPlayerName (nameOrIP);

        if (!account)
            return false;

        //NO SQL injection as account is uint32
        LoginDatabase.PExecute("UPDATE account_banned SET active = '0' WHERE id = '%u'",account);
    }
    return true;
}

/// Update the game time
void World::_UpdateGameTime()
{
    ///- update the time
    time_t thisTime = time(NULL);
    uint32 elapsed = uint32(thisTime - m_gameTime);
    m_gameTime = thisTime;

    ///- if there is a shutdown timer
    if(!m_stopEvent && m_ShutdownTimer > 0 && elapsed > 0)
    {
        ///- ... and it is overdue, stop the world (set m_stopEvent)
        if( m_ShutdownTimer <= elapsed )
        {
            if(!(m_ShutdownMask & SHUTDOWN_MASK_IDLE) || GetActiveAndQueuedSessionCount()==0)
                m_stopEvent = true;                         // exist code already set
            else
                m_ShutdownTimer = 1;                        // minimum timer value to wait idle state
        }
        ///- ... else decrease it and if necessary display a shutdown countdown to the users
        else
        {
            m_ShutdownTimer -= elapsed;

            ShutdownMsg();
        }
    }
}

/// Shutdown the server
void World::ShutdownServ(uint32 time, uint32 options, uint8 exitcode)
{
    // ignore if server shutdown at next tick
    if(m_stopEvent)
        return;

    m_ShutdownMask = options;
    m_ExitCode = exitcode;

    ///- If the shutdown time is 0, set m_stopEvent (except if shutdown is 'idle' with remaining sessions)
    if(time==0)
    {
        if(!(options & SHUTDOWN_MASK_IDLE) || GetActiveAndQueuedSessionCount()==0)
            m_stopEvent = true;                             // exist code already set
        else
            m_ShutdownTimer = 1;                            //So that the session count is re-evaluated at next world tick
    }
    ///- Else set the shutdown timer and warn users
    else
    {
        m_ShutdownTimer = time;
        ShutdownMsg(true);
    }
}

/// Display a shutdown message to the user(s)
void World::ShutdownMsg(bool show, Player* player)
{
    // not show messages for idle shutdown mode
    if(m_ShutdownMask & SHUTDOWN_MASK_IDLE)
        return;

    ///- Display a message every 12 hours, hours, 5 minutes, minute, 5 seconds and finally seconds
    if ( show ||
        (m_ShutdownTimer < 10) ||
                                                            // < 30 sec; every 5 sec
        (m_ShutdownTimer<30        && (m_ShutdownTimer % 5         )==0) ||
                                                            // < 5 min ; every 1 min
        (m_ShutdownTimer<5*MINUTE  && (m_ShutdownTimer % MINUTE    )==0) ||
                                                            // < 30 min ; every 5 min
        (m_ShutdownTimer<30*MINUTE && (m_ShutdownTimer % (5*MINUTE))==0) ||
                                                            // < 12 h ; every 1 h
        (m_ShutdownTimer<12*HOUR   && (m_ShutdownTimer % HOUR      )==0) ||
                                                            // > 12 h ; every 12 h
        (m_ShutdownTimer>12*HOUR   && (m_ShutdownTimer % (12*HOUR) )==0))
    {
        std::string str = secsToTimeString(m_ShutdownTimer);

        ServerMessageType msgid = (m_ShutdownMask & SHUTDOWN_MASK_RESTART) ? SERVER_MSG_RESTART_TIME : SERVER_MSG_SHUTDOWN_TIME;

        SendServerMessage(msgid,str.c_str(),player);
        DEBUG_LOG("Server is %s in %s",(m_ShutdownMask & SHUTDOWN_MASK_RESTART ? "restart" : "shutting down"),str.c_str());
    }
}

/// Cancel a planned server shutdown
void World::ShutdownCancel()
{
    // nothing cancel or too later
    if(!m_ShutdownTimer || m_stopEvent)
        return;

    ServerMessageType msgid = (m_ShutdownMask & SHUTDOWN_MASK_RESTART) ? SERVER_MSG_RESTART_CANCELLED : SERVER_MSG_SHUTDOWN_CANCELLED;

    m_ShutdownMask = 0;
    m_ShutdownTimer = 0;
    m_ExitCode = SHUTDOWN_EXIT_CODE;                       // to default value
    SendServerMessage(msgid);

    DEBUG_LOG("Server %s cancelled.",(m_ShutdownMask & SHUTDOWN_MASK_RESTART ? "restart" : "shutdown"));
}

/// Send a server message to the user(s)
void World::SendServerMessage(ServerMessageType type, const char *text, Player* player)
{
    WorldPacket data(SMSG_SERVER_MESSAGE, 50);              // guess size
    data << uint32(type);
    data << text;

    if(player)
        player->GetSession()->SendPacket(&data);
    else
        SendGlobalMessage( &data );
}

void World::UpdateSessions( uint32 diff )
{
    ///- Add new sessions
    WorldSession* sess;
    while(addSessQueue.next(sess))
        AddSession_ (sess);

    ///- Then send an update signal to remaining ones
    for (SessionMap::iterator itr = m_sessions.begin(), next; itr != m_sessions.end(); itr = next)
    {
        next = itr;
        ++next;
        ///- and remove not active sessions from the list
        WorldSession * pSession = itr->second;
        WorldSessionFilter updater(pSession);

        if(!pSession->Update(diff, updater))    // As interval = 0
        {
            RemoveQueuedSession(pSession);
            m_sessions.erase(itr);
            delete pSession;
        }
    }
}

// This handles the issued and queued CLI/RA commands
void World::ProcessCliCommands()
{
    CliCommandHolder::Print* zprint = NULL;
    void* callbackArg = NULL;
    CliCommandHolder* command;
    while (cliCmdQueue.next(command))
    {
        DEBUG_LOG("CLI command under processing...");
        zprint = command->m_print;
        callbackArg = command->m_callbackArg;
        CliHandler handler(command->m_cliAccountId, command->m_cliAccessLevel, callbackArg, zprint);
        handler.ParseCommands(command->m_command);

        if(command->m_commandFinished)
            command->m_commandFinished(callbackArg, !handler.HasSentErrorMessage());

        delete command;
    }
}

void World::SendBroadcast()
{
    std::string msg;
    static int nextid;

    QueryResult *result;
    if(nextid != 0)
    {
        result = CharacterDatabase.PQuery("SELECT `text`, `next` FROM `autobroadcast` WHERE `id` = %u", nextid);
    }
    else
    {
        result = CharacterDatabase.PQuery("SELECT `text`, `next` FROM `autobroadcast` ORDER BY RAND() LIMIT 1");
    }

    if(!result)
        return;

    Field *fields = result->Fetch();
    nextid  = fields[1].GetUInt32();
    msg = fields[0].GetString();
    delete result;

    static uint32 abcenter = 0;
    abcenter = sConfig.GetIntDefault("AutoBroadcast.Center", 0);
    if(abcenter == 0)
    {
        sWorld.SendWorldText(LANG_AUTO_BROADCAST, msg.c_str());

        sLog.outString("AutoBroadcast: '%s'",msg.c_str());
    }
    if(abcenter == 1)
    {
        WorldPacket data(SMSG_NOTIFICATION, (msg.size()+1));
        data << msg;
        sWorld.SendGlobalMessage(&data);

        sLog.outString("AutoBroadcast: '%s'",msg.c_str());
    }
    if(abcenter == 2)
    {
        sWorld.SendWorldText(LANG_AUTO_BROADCAST, msg.c_str());

        WorldPacket data(SMSG_NOTIFICATION, (msg.size()+1));
        data << msg;
        sWorld.SendGlobalMessage(&data);

        sLog.outString("AutoBroadcast: '%s'",msg.c_str());
   }
}

void World::InitResultQueue()
{
}

void World::UpdateResultQueue()
{
    //process async result queues
    CharacterDatabase.ProcessResultQueue();
    WorldDatabase.ProcessResultQueue();
    LoginDatabase.ProcessResultQueue();
}

void World::UpdateRealmCharCount(uint32 accountId)
{
    CharacterDatabase.AsyncPQuery(this, &World::_UpdateRealmCharCount, accountId,
        "SELECT COUNT(guid) FROM characters WHERE account = '%u'", accountId);
}

void World::_UpdateRealmCharCount(QueryResult *resultCharCount, uint32 accountId)
{
    if (resultCharCount)
    {
        Field *fields = resultCharCount->Fetch();
        uint32 charCount = fields[0].GetUInt32();
        delete resultCharCount;

        LoginDatabase.BeginTransaction();
        LoginDatabase.PExecute("DELETE FROM realmcharacters WHERE acctid= '%u' AND realmid = '%u'", accountId, realmID);
        LoginDatabase.PExecute("INSERT INTO realmcharacters (numchars, acctid, realmid) VALUES (%u, %u, %u)", charCount, accountId, realmID);
        LoginDatabase.CommitTransaction();
    }
}

void World::InitWeeklyQuestResetTime()
{
    QueryResult * result = CharacterDatabase.Query("SELECT NextWeeklyQuestResetTime FROM saved_variables");
    if (!result)
        m_NextWeeklyQuestReset = time_t(time(NULL));        // game time not yet init
    else
        m_NextWeeklyQuestReset = time_t((*result)[0].GetUInt64());

    // generate time by config
    time_t curTime = time(NULL);
    tm localTm = *localtime(&curTime);

    int week_day_offset = localTm.tm_wday - int(getConfig(CONFIG_UINT32_QUEST_WEEKLY_RESET_WEEK_DAY));

    // current week reset time
    localTm.tm_hour = getConfig(CONFIG_UINT32_QUEST_WEEKLY_RESET_HOUR);
    localTm.tm_min  = 0;
    localTm.tm_sec  = 0;
    time_t nextWeekResetTime = mktime(&localTm);
    nextWeekResetTime -= week_day_offset * DAY;             // move time to proper day

    // next reset time before current moment
    if (curTime >= nextWeekResetTime)
        nextWeekResetTime += WEEK;

    // normalize reset time
    m_NextWeeklyQuestReset = m_NextWeeklyQuestReset < curTime ? nextWeekResetTime - WEEK : nextWeekResetTime;

    if (!result)
        CharacterDatabase.PExecute("INSERT INTO saved_variables (NextWeeklyQuestResetTime) VALUES ('"UI64FMTD"')", uint64(m_NextWeeklyQuestReset));
    else
        delete result;
}

void World::InitDailyQuestResetTime()
{
    QueryResult * result = CharacterDatabase.Query("SELECT NextDailyQuestResetTime FROM saved_variables");
    if (!result)
        m_NextDailyQuestReset = time_t(time(NULL));         // game time not yet init
    else
        m_NextDailyQuestReset = time_t((*result)[0].GetUInt64());

    // generate time by config
    time_t curTime = time(NULL);
    tm localTm = *localtime(&curTime);
    localTm.tm_hour = getConfig(CONFIG_UINT32_QUEST_DAILY_RESET_HOUR);
    localTm.tm_min  = 0;
    localTm.tm_sec  = 0;

    // current day reset time
    time_t nextDayResetTime = mktime(&localTm);

    // next reset time before current moment
    if (curTime >= nextDayResetTime)
        nextDayResetTime += DAY;

    // normalize reset time
    m_NextDailyQuestReset = m_NextDailyQuestReset < curTime ? nextDayResetTime - DAY : nextDayResetTime;

    if (!result)
        CharacterDatabase.PExecute("INSERT INTO saved_variables (NextDailyQuestResetTime) VALUES ('"UI64FMTD"')", uint64(m_NextDailyQuestReset));
    else
        delete result;
}

void World::InitRandomBGResetTime()
{
    QueryResult * result = CharacterDatabase.Query("SELECT NextRandomBGResetTime FROM saved_variables");
    if (!result)
        m_NextRandomBGReset = time_t(time(NULL));         // game time not yet init
    else
        m_NextRandomBGReset = time_t((*result)[0].GetUInt64());

    // generate time by config
    time_t curTime = time(NULL);
    tm localTm = *localtime(&curTime);
    localTm.tm_hour = getConfig(CONFIG_UINT32_RANDOM_BG_RESET_HOUR);
    localTm.tm_min  = 0;
    localTm.tm_sec  = 0;

    // current day reset time
    time_t nextDayResetTime = mktime(&localTm);

    // next reset time before current moment
    if (curTime >= nextDayResetTime)
        nextDayResetTime += DAY;

    // normalize reset time
    m_NextRandomBGReset = m_NextRandomBGReset < curTime ? nextDayResetTime - DAY : nextDayResetTime;

    if (!result)
        CharacterDatabase.PExecute("INSERT INTO saved_variables (NextRandomBGResetTime) VALUES ('"UI64FMTD"')", uint64(m_NextRandomBGReset));
    else
        delete result;
}

void World::SetMonthlyQuestResetTime(bool initialize)
{
    if (initialize)
    {
        QueryResult * result = CharacterDatabase.Query("SELECT NextMonthlyQuestResetTime FROM saved_variables");

        if (!result)
            m_NextMonthlyQuestReset = time_t(time(NULL));
        else
            m_NextMonthlyQuestReset = time_t((*result)[0].GetUInt64());

        delete result;
    }

    // generate time
    time_t currentTime = time(NULL);
    tm localTm = *localtime(&currentTime);

    int month = localTm.tm_mon;
    int year = localTm.tm_year;

    ++month;

    // month 11 is december, next is january (0)
    if (month > 11)
    {
        month = 0;
        year += 1;
    }

    // reset time for next month
    localTm.tm_year = year;
    localTm.tm_mon = month;
    localTm.tm_mday = 1;                                    // don't know if we really need config option for day/hour
    localTm.tm_hour = 0;
    localTm.tm_min  = 0;
    localTm.tm_sec  = 0;

    time_t nextMonthResetTime = mktime(&localTm);

    m_NextMonthlyQuestReset = (initialize && m_NextMonthlyQuestReset < nextMonthResetTime) ? m_NextMonthlyQuestReset : nextMonthResetTime;

    // Row must exist for this to work. Currently row is added by InitDailyQuestResetTime(), called before this function
    CharacterDatabase.PExecute("UPDATE saved_variables SET NextMonthlyQuestResetTime = '"UI64FMTD"'", uint64(m_NextMonthlyQuestReset));
}

void World::ResetDailyQuests()
{
    DETAIL_LOG("Daily quests reset for all characters.");
    CharacterDatabase.Execute("DELETE FROM character_queststatus_daily");
    for(SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetDailyQuestStatus();

    m_NextDailyQuestReset = time_t(m_NextDailyQuestReset + DAY);
    CharacterDatabase.PExecute("UPDATE saved_variables SET NextDailyQuestResetTime = '"UI64FMTD"'", uint64(m_NextDailyQuestReset));
}

void World::ResetWeeklyQuests()
{
    DETAIL_LOG("Weekly quests reset for all characters.");
    CharacterDatabase.Execute("DELETE FROM character_queststatus_weekly");
    for(SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetWeeklyQuestStatus();

    m_NextWeeklyQuestReset = time_t(m_NextWeeklyQuestReset + WEEK);
    CharacterDatabase.PExecute("UPDATE saved_variables SET NextWeeklyQuestResetTime = '"UI64FMTD"'", uint64(m_NextWeeklyQuestReset));
}

void World::ResetRandomBG()
{
    sLog.outDetail("Random BG status reset for all characters.");
    CharacterDatabase.Execute("DELETE FROM character_battleground_random");
    for(SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->SetRandomWinner(false);

    m_NextRandomBGReset = time_t(m_NextRandomBGReset + DAY);
    CharacterDatabase.PExecute("UPDATE saved_variables SET NextRandomBGResetTime = '"UI64FMTD"'", uint64(m_NextRandomBGReset));
}

void World::ResetMonthlyQuests()
{
    DETAIL_LOG("Monthly quests reset for all characters.");
    CharacterDatabase.Execute("TRUNCATE character_queststatus_monthly");

    for(SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetMonthlyQuestStatus();

    SetMonthlyQuestResetTime(false);
}

void World::SetPlayerLimit( int32 limit, bool needUpdate )
{
    if (limit < -SEC_ADMINISTRATOR)
        limit = -SEC_ADMINISTRATOR;

    // lock update need
    bool db_update_need = needUpdate || (limit < 0) != (m_playerLimit < 0) || (limit < 0 && m_playerLimit < 0 && limit != m_playerLimit);

    m_playerLimit = limit;

    if (db_update_need)
        LoginDatabase.PExecute("UPDATE realmlist SET allowedSecurityLevel = '%u' WHERE id = '%u'",
            uint32(GetPlayerSecurityLimit()), realmID);
}

void World::UpdateMaxSessionCounters()
{
    m_maxActiveSessionCount = std::max(m_maxActiveSessionCount,uint32(m_sessions.size()-m_QueuedSessions.size()));
    m_maxQueuedSessionCount = std::max(m_maxQueuedSessionCount,uint32(m_QueuedSessions.size()));
}

void World::LoadDBVersion()
{
    QueryResult* result = WorldDatabase.Query("SELECT version, creature_ai_version, cache_id FROM db_version LIMIT 1");
    if(result)
    {
        Field* fields = result->Fetch();

        m_DBVersion              = fields[0].GetCppString();
        m_CreatureEventAIVersion = fields[1].GetCppString();

        // will be overwrite by config values if different and non-0
        setConfig(CONFIG_UINT32_CLIENTCACHE_VERSION, fields[2].GetUInt32());
        delete result;
    }

    if(m_DBVersion.empty())
        m_DBVersion = "Unknown world database.";

    if(m_CreatureEventAIVersion.empty())
        m_CreatureEventAIVersion = "Unknown creature EventAI.";
}

void World::setConfig(eConfigUInt32Values index, char const* fieldname, uint32 defvalue)
{
    setConfig(index, sConfig.GetIntDefault(fieldname,defvalue));
}

void World::setConfig(eConfigInt32Values index, char const* fieldname, int32 defvalue)
{
    setConfig(index, sConfig.GetIntDefault(fieldname,defvalue));
}

void World::setConfig(eConfigFloatValues index, char const* fieldname, float defvalue)
{
    setConfig(index, sConfig.GetFloatDefault(fieldname,defvalue));
}

void World::setConfig( eConfigBoolValues index, char const* fieldname, bool defvalue )
{
    setConfig(index, sConfig.GetBoolDefault(fieldname,defvalue));
}

void World::setConfigPos(eConfigUInt32Values index, char const* fieldname, uint32 defvalue)
{
    setConfig(index, fieldname, defvalue);
    if (int32(getConfig(index)) < 0)
    {
        sLog.outError("%s (%i) can't be negative. Using %u instead.", fieldname, int32(getConfig(index)), defvalue);
        setConfig(index, defvalue);
    }
}

void World::setConfigPos(eConfigFloatValues index, char const* fieldname, float defvalue)
{
    setConfig(index, fieldname, defvalue);
    if (getConfig(index) < 0.0f)
    {
        sLog.outError("%s (%f) can't be negative. Using %f instead.", fieldname, getConfig(index), defvalue);
        setConfig(index, defvalue);
    }
}

void World::setConfigMin(eConfigUInt32Values index, char const* fieldname, uint32 defvalue, uint32 minvalue)
{
    setConfig(index, fieldname, defvalue);
    if (getConfig(index) < minvalue)
    {
        sLog.outError("%s (%u) must be >= %u. Using %u instead.", fieldname, getConfig(index), minvalue, minvalue);
        setConfig(index, minvalue);
    }
}

void World::setConfigMin(eConfigInt32Values index, char const* fieldname, int32 defvalue, int32 minvalue)
{
    setConfig(index, fieldname, defvalue);
    if (getConfig(index) < minvalue)
    {
        sLog.outError("%s (%i) must be >= %i. Using %i instead.", fieldname, getConfig(index), minvalue, minvalue);
        setConfig(index, minvalue);
    }
}

void World::setConfigMin(eConfigFloatValues index, char const* fieldname, float defvalue, float minvalue)
{
    setConfig(index, fieldname, defvalue);
    if (getConfig(index) < minvalue)
    {
        sLog.outError("%s (%f) must be >= %f. Using %f instead.", fieldname, getConfig(index), minvalue, minvalue);
        setConfig(index, minvalue);
    }
}

void World::setConfigMinMax(eConfigUInt32Values index, char const* fieldname, uint32 defvalue, uint32 minvalue, uint32 maxvalue)
{
    setConfig(index, fieldname, defvalue);
    if (getConfig(index) < minvalue)
    {
        sLog.outError("%s (%u) must be in range %u...%u. Using %u instead.", fieldname, getConfig(index), minvalue, maxvalue, minvalue);
        setConfig(index, minvalue);
    }
    else if (getConfig(index) > maxvalue)
    {
        sLog.outError("%s (%u) must be in range %u...%u. Using %u instead.", fieldname, getConfig(index), minvalue, maxvalue, maxvalue);
        setConfig(index, maxvalue);
    }
}

void World::setConfigMinMax(eConfigInt32Values index, char const* fieldname, int32 defvalue, int32 minvalue, int32 maxvalue)
{
    setConfig(index, fieldname, defvalue);
    if (getConfig(index) < minvalue)
    {
        sLog.outError("%s (%i) must be in range %i...%i. Using %i instead.", fieldname, getConfig(index), minvalue, maxvalue, minvalue);
        setConfig(index, minvalue);
    }
    else if (getConfig(index) > maxvalue)
    {
        sLog.outError("%s (%i) must be in range %i...%i. Using %i instead.", fieldname, getConfig(index), minvalue, maxvalue, maxvalue);
        setConfig(index, maxvalue);
    }
}

void World::setConfigMinMax(eConfigFloatValues index, char const* fieldname, float defvalue, float minvalue, float maxvalue)
{
    setConfig(index, fieldname, defvalue);
    if (getConfig(index) < minvalue)
    {
        sLog.outError("%s (%f) must be in range %f...%f. Using %f instead.", fieldname, getConfig(index), minvalue, maxvalue, minvalue);
        setConfig(index, minvalue);
    }
    else if (getConfig(index) > maxvalue)
    {
        sLog.outError("%s (%f) must be in range %f...%f. Using %f instead.", fieldname, getConfig(index), minvalue, maxvalue, maxvalue);
        setConfig(index, maxvalue);
    }
}

bool World::configNoReload(bool reload, eConfigUInt32Values index, char const* fieldname, uint32 defvalue)
{
    if (!reload)
        return true;

    uint32 val = sConfig.GetIntDefault(fieldname, defvalue);
    if (val != getConfig(index))
        sLog.outError("%s option can't be changed at mangosd.conf reload, using current value (%u).", fieldname, getConfig(index));

    return false;
}

bool World::configNoReload(bool reload, eConfigInt32Values index, char const* fieldname, int32 defvalue)
{
    if (!reload)
        return true;

    int32 val = sConfig.GetIntDefault(fieldname, defvalue);
    if (val != getConfig(index))
        sLog.outError("%s option can't be changed at mangosd.conf reload, using current value (%i).", fieldname, getConfig(index));

    return false;
}

bool World::configNoReload(bool reload, eConfigFloatValues index, char const* fieldname, float defvalue)
{
    if (!reload)
        return true;

    float val = sConfig.GetFloatDefault(fieldname, defvalue);
    if (val != getConfig(index))
        sLog.outError("%s option can't be changed at mangosd.conf reload, using current value (%f).", fieldname, getConfig(index));

    return false;
}

bool World::configNoReload(bool reload, eConfigBoolValues index, char const* fieldname, bool defvalue)
{
    if (!reload)
        return true;

    bool val = sConfig.GetBoolDefault(fieldname, defvalue);
    if (val != getConfig(index))
        sLog.outError("%s option can't be changed at mangosd.conf reload, using current value (%s).", fieldname, getConfig(index) ? "'true'" : "'false'");

    return false;
}
