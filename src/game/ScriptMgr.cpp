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

#include "ScriptMgr.h"
#include "Policies/SingletonImp.h"
#include "Log.h"
#include "ProgressBar.h"
#include "ObjectMgr.h"
#include "WaypointManager.h"
#include "World.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Cell.h"
#include "CellImpl.h"
#include "SQLStorages.h"

#include "revision_nr.h"

ScriptMapMapName sQuestEndScripts;
ScriptMapMapName sQuestStartScripts;
ScriptMapMapName sSpellScripts;
ScriptMapMapName sGameObjectScripts;
ScriptMapMapName sGameObjectTemplateScripts;
ScriptMapMapName sEventScripts;
ScriptMapMapName sGossipScripts;
ScriptMapMapName sCreatureMovementScripts;

INSTANTIATE_SINGLETON_1(ScriptMgr);

ScriptMgr::ScriptMgr() :
    m_hScriptLib(NULL),
    m_scheduledScripts(0),

    m_pOnInitScriptLibrary(NULL),
    m_pOnFreeScriptLibrary(NULL),
    m_pGetScriptLibraryVersion(NULL),

    m_pGetCreatureAI(NULL),
    m_pCreateInstanceData(NULL),

    m_pOnGossipHello(NULL),
    m_pOnGOGossipHello(NULL),
    m_pOnGossipSelect(NULL),
    m_pOnGOGossipSelect(NULL),
    m_pOnGossipSelectWithCode(NULL),
    m_pOnGOGossipSelectWithCode(NULL),
    m_pOnQuestAccept(NULL),
    m_pOnGOQuestAccept(NULL),
    m_pOnItemQuestAccept(NULL),
    m_pOnQuestRewarded(NULL),
    m_pOnGOQuestRewarded(NULL),
    m_pGetNPCDialogStatus(NULL),
    m_pGetGODialogStatus(NULL),
    m_pOnGOUse(NULL),
    m_pOnItemUse(NULL),
    m_pOnAreaTrigger(NULL),
    m_pOnProcessEvent(NULL),
    m_pOnEffectDummyCreature(NULL),
    m_pOnEffectDummyGO(NULL),
    m_pOnEffectDummyItem(NULL),
    m_pOnAuraDummy(NULL)
{
}

ScriptMgr::~ScriptMgr()
{
    UnloadScriptLibrary();
}

// /////////////////////////////////////////////////////////
//              DB SCRIPTS (loaders of static data)
// /////////////////////////////////////////////////////////
// returns priority (0 == cannot start script)
uint8 GetSpellStartDBScriptPriority(SpellEntry const* spellinfo, SpellEffectIndex effIdx)
{
    SpellEffectEntry const* spellEffect = spellinfo->GetSpellEffect(effIdx);
    if (!spellEffect)
        return 0;

    if (spellEffect->Effect == SPELL_EFFECT_SCRIPT_EFFECT)
        return 10;

    if (spellEffect->Effect == SPELL_EFFECT_DUMMY)
        return 9;

    // NonExisting triggered spells can also start DB-Spell-Scripts
    if (spellEffect->Effect == SPELL_EFFECT_TRIGGER_SPELL && !sSpellStore.LookupEntry(spellEffect->EffectTriggerSpell))
        return 5;

    // Can not start script
    return 0;
}

// Priorize: SCRIPT_EFFECT before DUMMY before Non-Existing triggered spell, for same priority the first effect with the priority triggers
bool ScriptMgr::CanSpellEffectStartDBScript(SpellEntry const* spellinfo, SpellEffectIndex effIdx)
{
    uint8 priority = GetSpellStartDBScriptPriority(spellinfo, effIdx);
    if (!priority)
        return false;

    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        uint8 currentPriority = GetSpellStartDBScriptPriority(spellinfo, SpellEffectIndex(i));
        if (currentPriority < priority)                     // lower priority, continue checking
            continue;
        if (currentPriority > priority)                     // take other index with higher priority
            return false;
        if (i < effIdx)                                     // same priority at lower index
            return false;
    }

    return true;
}

void ScriptMgr::LoadScripts(ScriptMapMapName& scripts, const char* tablename)
{
    if (IsScriptScheduled())                                // function don't must be called in time scripts use.
        return;

    sLog.outString("%s :", tablename);

    scripts.first = tablename;
    scripts.second.clear();                                 // need for reload support

    //                                                 0   1      2        3         4          5            6              7           8        9         10        11        12 13 14 15
    QueryResult* result = WorldDatabase.PQuery("SELECT id, delay, command, datalong, datalong2, buddy_entry, search_radius, data_flags, dataint, dataint2, dataint3, dataint4, x, y, z, o FROM %s", tablename);

    uint32 count = 0;

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded %u script definitions", count);
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        bar.step();

        Field* fields = result->Fetch();

        ScriptInfo tmp;
        tmp.id           = fields[0].GetUInt32();
        tmp.delay        = fields[1].GetUInt32();
        tmp.command      = fields[2].GetUInt32();
        tmp.raw.data[0]  = fields[3].GetUInt32();
        tmp.raw.data[1]  = fields[4].GetUInt32();
        tmp.buddyEntry   = fields[5].GetUInt32();
        tmp.searchRadius = fields[6].GetUInt32();
        tmp.data_flags   = fields[7].GetUInt8();
        tmp.textId[0]    = fields[8].GetInt32();
        tmp.textId[1]    = fields[9].GetInt32();
        tmp.textId[2]    = fields[10].GetInt32();
        tmp.textId[3]    = fields[11].GetInt32();
        tmp.x            = fields[12].GetFloat();
        tmp.y            = fields[13].GetFloat();
        tmp.z            = fields[14].GetFloat();
        tmp.o            = fields[15].GetFloat();

        // generic command args check
        if (tmp.buddyEntry)                                 // Check Buddy args
        {
            if (tmp.IsCreatureBuddy() && !ObjectMgr::GetCreatureTemplate(tmp.buddyEntry))
            {
                sLog.outErrorDb("Table `%s` has buddyEntry = %u in command %u for script id %u, but this creature_template does not exist, skipping.", tablename, tmp.buddyEntry, tmp.command, tmp.id);
                continue;
            }
            else if (!tmp.IsCreatureBuddy() && !ObjectMgr::GetGameObjectInfo(tmp.buddyEntry))
            {
                sLog.outErrorDb("Table `%s` has buddyEntry = %u in command %u for script id %u, but this gameobject_template does not exist, skipping.", tablename, tmp.buddyEntry, tmp.command, tmp.id);
                continue;
            }
            if (!tmp.searchRadius)
            {
                sLog.outErrorDb("Table `%s` has searchRadius = 0 in command %u for script id %u for buddy %u, skipping.", tablename, tmp.command, tmp.id, tmp.buddyEntry);
                continue;
            }
        }

        if (tmp.data_flags)                                 // Check flags
        {
            if (tmp.data_flags & ~(SCRIPT_FLAG_COMMAND_ADDITIONAL * 2 - 1))
            {
                sLog.outErrorDb("Table `%s` has invalid data_flags %u in command %u for script id %u, skipping.", tablename, tmp.data_flags, tmp.command, tmp.id);
                continue;
            }
            if (!tmp.HasAdditionalScriptFlag() && tmp.data_flags & SCRIPT_FLAG_COMMAND_ADDITIONAL)
            {
                sLog.outErrorDb("Table `%s` has invalid data_flags %u in command %u for script id %u, skipping.", tablename, tmp.data_flags, tmp.command, tmp.id);
                continue;
            }
            if (tmp.data_flags & SCRIPT_FLAG_BUDDY_AS_TARGET && ! tmp.buddyEntry)
            {
                sLog.outErrorDb("Table `%s` has buddy required in data_flags %u in command %u for script id %u, but no buddy defined, skipping.", tablename, tmp.data_flags, tmp.command, tmp.id);
                continue;
            }
        }

        switch (tmp.command)
        {
            case SCRIPT_COMMAND_TALK:                       // 0
            {
                if (tmp.talk.chatType > CHAT_TYPE_ZONE_YELL)
                {
                    sLog.outErrorDb("Table `%s` has invalid CHAT_TYPE_ (datalong = %u) in SCRIPT_COMMAND_TALK for script id %u", tablename, tmp.talk.chatType, tmp.id);
                    continue;
                }

                if (!GetLanguageDescByID(tmp.talk.language))
                {
                    sLog.outErrorDb("Table `%s` has datalong2 = %u in SCRIPT_COMMAND_TALK for script id %u, but this language does not exist.", tablename, tmp.talk.language, tmp.id);
                    continue;
                }

                if (tmp.textId[0] == 0)
                {
                    sLog.outErrorDb("Table `%s` has invalid talk text id (dataint = %i) in SCRIPT_COMMAND_TALK for script id %u", tablename, tmp.textId[0], tmp.id);
                    continue;
                }

                for (int i = 0; i < MAX_TEXT_ID; ++i)
                {
                    if (tmp.textId[i] && (tmp.textId[i] < MIN_DB_SCRIPT_STRING_ID || tmp.textId[i] >= MAX_DB_SCRIPT_STRING_ID))
                    {
                        sLog.outErrorDb("Table `%s` has out of range text id (dataint = %i expected %u-%u) in SCRIPT_COMMAND_TALK for script id %u", tablename, tmp.textId[i], MIN_DB_SCRIPT_STRING_ID, MAX_DB_SCRIPT_STRING_ID, tmp.id);
                        continue;
                    }
                }

                // if (!GetMangosStringLocale(tmp.dataint)) will be checked after db_script_string loading
                break;
            }
            case SCRIPT_COMMAND_EMOTE:                      // 1
            {
                if (!sEmotesStore.LookupEntry(tmp.emote.emoteId))
                {
                    sLog.outErrorDb("Table `%s` has invalid emote id (datalong = %u) in SCRIPT_COMMAND_EMOTE for script id %u", tablename, tmp.emote.emoteId, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_FIELD_SET:                  // 2
            case SCRIPT_COMMAND_MOVE_TO:                    // 3
            case SCRIPT_COMMAND_FLAG_SET:                   // 4
            case SCRIPT_COMMAND_FLAG_REMOVE:                // 5
                break;
            case SCRIPT_COMMAND_TELEPORT_TO:                // 6
            {
                if (!sMapStore.LookupEntry(tmp.teleportTo.mapId))
                {
                    sLog.outErrorDb("Table `%s` has invalid map (Id: %u) in SCRIPT_COMMAND_TELEPORT_TO for script id %u", tablename, tmp.teleportTo.mapId, tmp.id);
                    continue;
                }

                if (!MaNGOS::IsValidMapCoord(tmp.x, tmp.y, tmp.z, tmp.o))
                {
                    sLog.outErrorDb("Table `%s` has invalid coordinates (X: %f Y: %f) in SCRIPT_COMMAND_TELEPORT_TO for script id %u", tablename, tmp.x, tmp.y, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_QUEST_EXPLORED:             // 7
            {
                Quest const* quest = sObjectMgr.GetQuestTemplate(tmp.questExplored.questId);
                if (!quest)
                {
                    sLog.outErrorDb("Table `%s` has invalid quest (ID: %u) in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u", tablename, tmp.questExplored.questId, tmp.id);
                    continue;
                }

                if (!quest->HasSpecialFlag(QUEST_SPECIAL_FLAG_EXPLORATION_OR_EVENT))
                {
                    sLog.outErrorDb("Table `%s` has quest (ID: %u) in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u, but quest not have flag QUEST_SPECIAL_FLAG_EXPLORATION_OR_EVENT in quest flags. Script command or quest flags wrong. Quest modified to require objective.", tablename, tmp.questExplored.questId, tmp.id);

                    // this will prevent quest completing without objective
                    const_cast<Quest*>(quest)->SetSpecialFlag(QUEST_SPECIAL_FLAG_EXPLORATION_OR_EVENT);

                    // continue; - quest objective requirement set and command can be allowed
                }

                if (float(tmp.questExplored.distance) > DEFAULT_VISIBILITY_DISTANCE)
                {
                    sLog.outErrorDb("Table `%s` has too large distance (%u) for exploring objective complete in `datalong2` in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u",
                                    tablename, tmp.questExplored.distance, tmp.id);
                    continue;
                }

                if (tmp.questExplored.distance && float(tmp.questExplored.distance) > DEFAULT_VISIBILITY_DISTANCE)
                {
                    sLog.outErrorDb("Table `%s` has too large distance (%u) for exploring objective complete in `datalong2` in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u, max distance is %f or 0 for disable distance check",
                                    tablename, tmp.questExplored.distance, tmp.id, DEFAULT_VISIBILITY_DISTANCE);
                    continue;
                }

                if (tmp.questExplored.distance && float(tmp.questExplored.distance) < INTERACTION_DISTANCE)
                {
                    sLog.outErrorDb("Table `%s` has too small distance (%u) for exploring objective complete in `datalong2` in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u, min distance is %f or 0 for disable distance check",
                                    tablename, tmp.questExplored.distance, tmp.id, INTERACTION_DISTANCE);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_KILL_CREDIT:                // 8
            {
                if (tmp.killCredit.creatureEntry && !ObjectMgr::GetCreatureTemplate(tmp.killCredit.creatureEntry))
                {
                    sLog.outErrorDb("Table `%s` has invalid creature (Entry: %u) in SCRIPT_COMMAND_KILL_CREDIT for script id %u", tablename, tmp.killCredit.creatureEntry, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT:         // 9
            {
                uint32 goEntry = 0;

                if (!tmp.GetGOGuid())
                {
                    if (!tmp.buddyEntry)
                    {
                        sLog.outErrorDb("Table `%s` has no gameobject nor buddy defined in SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id %u", tablename, tmp.id);
                        continue;
                    }
                    goEntry = tmp.buddyEntry;
                }
                else
                {
                    GameObjectData const* data = sObjectMgr.GetGOData(tmp.GetGOGuid());
                    if (!data)
                    {
                        sLog.outErrorDb("Table `%s` has invalid gameobject (GUID: %u) in SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id %u", tablename, tmp.GetGOGuid(), tmp.id);
                        continue;
                    }
                    goEntry = data->id;
                }

                GameObjectInfo const* info = ObjectMgr::GetGameObjectInfo(goEntry);
                if (!info)
                {
                    sLog.outErrorDb("Table `%s` has gameobject with invalid entry (GUID: %u Entry: %u) in SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id %u", tablename, tmp.GetGOGuid(), goEntry, tmp.id);
                    continue;
                }

                if (info->type == GAMEOBJECT_TYPE_FISHINGNODE ||
                        info->type == GAMEOBJECT_TYPE_FISHINGHOLE ||
                        info->type == GAMEOBJECT_TYPE_DOOR        ||
                        info->type == GAMEOBJECT_TYPE_BUTTON      ||
                        info->type == GAMEOBJECT_TYPE_TRAP)
                {
                    sLog.outErrorDb("Table `%s` have gameobject type (%u) unsupported by command SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id %u", tablename, info->type, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:       // 10
            {
                if (!MaNGOS::IsValidMapCoord(tmp.x, tmp.y, tmp.z, tmp.o))
                {
                    sLog.outErrorDb("Table `%s` has invalid coordinates (X: %f Y: %f) in SCRIPT_COMMAND_TEMP_SUMMON_CREATURE for script id %u", tablename, tmp.x, tmp.y, tmp.id);
                    continue;
                }

                if (!ObjectMgr::GetCreatureTemplate(tmp.summonCreature.creatureEntry))
                {
                    sLog.outErrorDb("Table `%s` has invalid creature (Entry: %u) in SCRIPT_COMMAND_TEMP_SUMMON_CREATURE for script id %u", tablename, tmp.summonCreature.creatureEntry, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_OPEN_DOOR:                  // 11
            case SCRIPT_COMMAND_CLOSE_DOOR:                 // 12
            {
                uint32 goEntry = 0;

                if (!tmp.GetGOGuid())
                {
                    if (!tmp.buddyEntry)
                    {
                        sLog.outErrorDb("Table `%s` has no gameobject nor buddy defined in %s for script id %u", tablename, (tmp.command == SCRIPT_COMMAND_OPEN_DOOR ? "SCRIPT_COMMAND_OPEN_DOOR" : "SCRIPT_COMMAND_CLOSE_DOOR"), tmp.id);
                        continue;
                    }
                    goEntry = tmp.buddyEntry;
                }
                else
                {
                    GameObjectData const* data = sObjectMgr.GetGOData(tmp.GetGOGuid());
                    if (!data)
                    {
                        sLog.outErrorDb("Table `%s` has invalid gameobject (GUID: %u) in %s for script id %u", tablename, tmp.GetGOGuid(), (tmp.command == SCRIPT_COMMAND_OPEN_DOOR ? "SCRIPT_COMMAND_OPEN_DOOR" : "SCRIPT_COMMAND_CLOSE_DOOR"), tmp.id);
                        continue;
                    }
                    goEntry = data->id;
                }

                GameObjectInfo const* info = ObjectMgr::GetGameObjectInfo(goEntry);
                if (!info)
                {
                    sLog.outErrorDb("Table `%s` has gameobject with invalid entry (GUID: %u Entry: %u) in %s for script id %u", tablename, tmp.GetGOGuid(), goEntry, (tmp.command == SCRIPT_COMMAND_OPEN_DOOR ? "SCRIPT_COMMAND_OPEN_DOOR" : "SCRIPT_COMMAND_CLOSE_DOOR"), tmp.id);
                    continue;
                }

                if (info->type != GAMEOBJECT_TYPE_DOOR)
                {
                    sLog.outErrorDb("Table `%s` has gameobject type (%u) non supported by command %s for script id %u", tablename, info->id, (tmp.command == SCRIPT_COMMAND_OPEN_DOOR ? "SCRIPT_COMMAND_OPEN_DOOR" : "SCRIPT_COMMAND_CLOSE_DOOR"), tmp.id);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_ACTIVATE_OBJECT:            // 13
                break;
            case SCRIPT_COMMAND_REMOVE_AURA:                // 14
            {
                if (!sSpellStore.LookupEntry(tmp.removeAura.spellId))
                {
                    sLog.outErrorDb("Table `%s` using nonexistent spell (id: %u) in SCRIPT_COMMAND_REMOVE_AURA or SCRIPT_COMMAND_CAST_SPELL for script id %u",
                                    tablename, tmp.removeAura.spellId, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_CAST_SPELL:                 // 15
            {
                if (!sSpellStore.LookupEntry(tmp.castSpell.spellId))
                {
                    sLog.outErrorDb("Table `%s` using nonexistent spell (id: %u) in SCRIPT_COMMAND_REMOVE_AURA or SCRIPT_COMMAND_CAST_SPELL for script id %u",
                                    tablename, tmp.castSpell.spellId, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_PLAY_SOUND:                 // 16
            {
                if (!sSoundEntriesStore.LookupEntry(tmp.playSound.soundId))
                {
                    sLog.outErrorDb("Table `%s` using nonexistent sound (id: %u) in SCRIPT_COMMAND_PLAY_SOUND for script id %u",
                                    tablename, tmp.playSound.soundId, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_CREATE_ITEM:                // 17
            {
                if (!ObjectMgr::GetItemPrototype(tmp.createItem.itemEntry))
                {
                    sLog.outErrorDb("Table `%s` has nonexistent item (entry: %u) in SCRIPT_COMMAND_CREATE_ITEM for script id %u",
                                    tablename, tmp.createItem.itemEntry, tmp.id);
                    continue;
                }
                if (!tmp.createItem.amount)
                {
                    sLog.outErrorDb("Table `%s` SCRIPT_COMMAND_CREATE_ITEM but amount is %u for script id %u",
                                    tablename, tmp.createItem.amount, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_DESPAWN_SELF:               // 18
            {
                // for later, we might consider despawn by database guid, and define in datalong2 as option to despawn self.
                break;
            }
            case SCRIPT_COMMAND_PLAY_MOVIE:                 // 19
            {
                if (!sMovieStore.LookupEntry(tmp.playMovie.movieId))
                {
                    sLog.outErrorDb("Table `%s` use non-existing movie_id (id: %u) in SCRIPT_COMMAND_PLAY_MOVIE for script id %u",
                                    tablename, tmp.playMovie.movieId, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_MOVEMENT:                   // 20
            {
                if (tmp.movement.movementType >= MAX_DB_MOTION_TYPE)
                {
                    sLog.outErrorDb("Table `%s` SCRIPT_COMMAND_MOVEMENT has invalid MovementType %u for script id %u",
                                    tablename, tmp.movement.movementType, tmp.id);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_SET_ACTIVEOBJECT:           // 21
                break;
            case SCRIPT_COMMAND_SET_FACTION:                // 22
            {
                if (tmp.faction.factionId && !sFactionTemplateStore.LookupEntry(tmp.faction.factionId))
                {
                    sLog.outErrorDb("Table `%s` has datalong = %u in SCRIPT_COMMAND_SET_FACTION for script id %u, but this faction-template does not exist.", tablename, tmp.faction.factionId, tmp.id);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_MORPH_TO_ENTRY_OR_MODEL:    // 23
            {
                if (tmp.data_flags & SCRIPT_FLAG_COMMAND_ADDITIONAL)
                {
                    if (tmp.morph.creatureOrModelEntry && !sCreatureDisplayInfoStore.LookupEntry(tmp.morph.creatureOrModelEntry))
                    {
                        sLog.outErrorDb("Table `%s` has datalong2 = %u in SCRIPT_COMMAND_MORPH_TO_ENTRY_OR_MODEL for script id %u, but this model does not exist.", tablename, tmp.morph.creatureOrModelEntry, tmp.id);
                        continue;
                    }
                }
                else
                {
                    if (tmp.morph.creatureOrModelEntry && !ObjectMgr::GetCreatureTemplate(tmp.morph.creatureOrModelEntry))
                    {
                        sLog.outErrorDb("Table `%s` has datalong2 = %u in SCRIPT_COMMAND_MORPH_TO_ENTRY_OR_MODEL for script id %u, but this creature_template does not exist.", tablename, tmp.morph.creatureOrModelEntry, tmp.id);
                        continue;
                    }
                }

                break;
            }
            case SCRIPT_COMMAND_MOUNT_TO_ENTRY_OR_MODEL:    // 24
            {
                if (tmp.data_flags & SCRIPT_FLAG_COMMAND_ADDITIONAL)
                {
                    if (tmp.mount.creatureOrModelEntry && !sCreatureDisplayInfoStore.LookupEntry(tmp.mount.creatureOrModelEntry))
                    {
                        sLog.outErrorDb("Table `%s` has datalong2 = %u in SCRIPT_COMMAND_MOUNT_TO_ENTRY_OR_MODEL for script id %u, but this model does not exist.", tablename, tmp.mount.creatureOrModelEntry, tmp.id);
                        continue;
                    }
                }
                else
                {
                    if (tmp.mount.creatureOrModelEntry && !ObjectMgr::GetCreatureTemplate(tmp.mount.creatureOrModelEntry))
                    {
                        sLog.outErrorDb("Table `%s` has datalong2 = %u in SCRIPT_COMMAND_MOUNT_TO_ENTRY_OR_MODEL for script id %u, but this creature_template does not exist.", tablename, tmp.mount.creatureOrModelEntry, tmp.id);
                        continue;
                    }
                }

                break;
            }
            case SCRIPT_COMMAND_SET_RUN:                    // 25
            case SCRIPT_COMMAND_ATTACK_START:               // 26
                break;
            case SCRIPT_COMMAND_GO_LOCK_STATE:              // 27
            {
                if (// lock(0x01) and unlock(0x02) together
                    ((tmp.goLockState.lockState & 0x01) && (tmp.goLockState.lockState & 0x02)) ||
                    // non-interact (0x4) and interact (0x08) together
                    ((tmp.goLockState.lockState & 0x04) && (tmp.goLockState.lockState & 0x08)) ||
                    // no setting
                    !tmp.goLockState.lockState ||
                    // invalid number
                    tmp.goLockState.lockState >= 0x10)
                {
                    sLog.outErrorDb("Table `%s` has invalid lock state (datalong = %u) in SCRIPT_COMMAND_GO_LOCK_STATE for script id %u.", tablename, tmp.goLockState.lockState, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_STAND_STATE:                // 28
            {
                if (tmp.standState.stand_state >= MAX_UNIT_STAND_STATE)
                {
                    sLog.outErrorDb("Table `%s` has invalid stand state (datalong = %u) in SCRIPT_COMMAND_STAND_STATE for script id %u", tablename, tmp.standState.stand_state, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_MODIFY_NPC_FLAGS:           // 29
                break;
            case SCRIPT_COMMAND_SEND_TAXI_PATH:             // 30
            {
                if (!sTaxiPathStore.LookupEntry(tmp.sendTaxiPath.taxiPathId))
                {
                    sLog.outErrorDb("Table `%s` has datalong = %u in SCRIPT_COMMAND_SEND_TAXI_PATH for script id %u, but this taxi path does not exist.", tablename, tmp.sendTaxiPath.taxiPathId, tmp.id);
                    continue;
                }
                // Check if this taxi path can be triggered with a spell
                if (!sLog.HasLogFilter(LOG_FILTER_DB_STRICTED_CHECK))
                {
                    uint32 taxiSpell = 0;
                    for (uint32 i = 1; i < sSpellStore.GetNumRows() && taxiSpell == 0; ++i)
                    {
                        if (SpellEntry const* spell = sSpellStore.LookupEntry(i))
                            for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
                            {
                                SpellEffectEntry const* spellEffect = spell->GetSpellEffect(SpellEffectIndex(j));
                                if (!spellEffect)
                                    continue;

                                if (spellEffect->Effect == SPELL_EFFECT_SEND_TAXI && spellEffect->EffectMiscValue == tmp.sendTaxiPath.taxiPathId)
                                {
                                    taxiSpell = i;
                                    break;
                                }
                            }
                    }

                    if (taxiSpell)
                    {
                        sLog.outErrorDb("Table `%s` has datalong = %u in SCRIPT_COMMAND_SEND_TAXI_PATH for script id %u, but this taxi path can be triggered by spell %u.", tablename, tmp.sendTaxiPath.taxiPathId, tmp.id, taxiSpell);
                        continue;
                    }
                }
                break;
            }
            default:
            {
                sLog.outErrorDb("Table `%s` unknown command %u, skipping.", tablename, tmp.command);
                continue;
            }
        }

        if (scripts.second.find(tmp.id) == scripts.second.end())
        {
            ScriptMap emptyMap;
            scripts.second[tmp.id] = emptyMap;
        }
        scripts.second[tmp.id].insert(ScriptMap::value_type(tmp.delay, tmp));

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u script definitions", count);
}

void ScriptMgr::LoadGameObjectScripts()
{
    LoadScripts(sGameObjectScripts, "gameobject_scripts");

    // check ids
    for (ScriptMapMap::const_iterator itr = sGameObjectScripts.second.begin(); itr != sGameObjectScripts.second.end(); ++itr)
    {
        if (!sObjectMgr.GetGOData(itr->first))
            sLog.outErrorDb("Table `gameobject_scripts` has not existing gameobject (GUID: %u) as script id", itr->first);
    }
}

void ScriptMgr::LoadGameObjectTemplateScripts()
{
    LoadScripts(sGameObjectTemplateScripts, "gameobject_template_scripts");

    // check ids
    for (ScriptMapMap::const_iterator itr = sGameObjectTemplateScripts.second.begin(); itr != sGameObjectTemplateScripts.second.end(); ++itr)
    {
        if (!sObjectMgr.GetGameObjectInfo(itr->first))
            sLog.outErrorDb("Table `gameobject_template_scripts` has not existing gameobject (Entry: %u) as script id", itr->first);
    }
}

void ScriptMgr::LoadQuestEndScripts()
{
    LoadScripts(sQuestEndScripts, "quest_end_scripts");

    // check ids
    for (ScriptMapMap::const_iterator itr = sQuestEndScripts.second.begin(); itr != sQuestEndScripts.second.end(); ++itr)
    {
        if (!sObjectMgr.GetQuestTemplate(itr->first))
            sLog.outErrorDb("Table `quest_end_scripts` has not existing quest (Id: %u) as script id", itr->first);
    }
}

void ScriptMgr::LoadQuestStartScripts()
{
    LoadScripts(sQuestStartScripts, "quest_start_scripts");

    // check ids
    for (ScriptMapMap::const_iterator itr = sQuestStartScripts.second.begin(); itr != sQuestStartScripts.second.end(); ++itr)
    {
        if (!sObjectMgr.GetQuestTemplate(itr->first))
            sLog.outErrorDb("Table `quest_start_scripts` has not existing quest (Id: %u) as script id", itr->first);
    }
}

void ScriptMgr::LoadSpellScripts()
{
    LoadScripts(sSpellScripts, "spell_scripts");

    // check ids
    for (ScriptMapMap::const_iterator itr = sSpellScripts.second.begin(); itr != sSpellScripts.second.end(); ++itr)
    {
        SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->first);
        if (!spellInfo)
        {
            sLog.outErrorDb("Table `spell_scripts` has not existing spell (Id: %u) as script id", itr->first);
            continue;
        }

        // check for correct spellEffect
        bool found = false;
        for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            if (GetSpellStartDBScriptPriority(spellInfo, SpellEffectIndex(i)))
            {
                found =  true;
                break;
            }
        }

        if (!found)
            sLog.outErrorDb("Table `spell_scripts` has unsupported spell (Id: %u)", itr->first);
    }
}

void ScriptMgr::LoadEventScripts()
{
    LoadScripts(sEventScripts, "event_scripts");

    std::set<uint32> evt_scripts;

    // Load all possible script entries from gameobjects
    for (uint32 i = 1; i < sGOStorage.GetMaxEntry(); ++i)
    {
        if (GameObjectInfo const* goInfo = sGOStorage.LookupEntry<GameObjectInfo>(i))
        {
            if (uint32 eventId = goInfo->GetEventScriptId())
                evt_scripts.insert(eventId);

            if (goInfo->type == GAMEOBJECT_TYPE_CAPTURE_POINT)
            {
                evt_scripts.insert(goInfo->capturePoint.neutralEventID1);
                evt_scripts.insert(goInfo->capturePoint.neutralEventID2);
                evt_scripts.insert(goInfo->capturePoint.contestedEventID1);
                evt_scripts.insert(goInfo->capturePoint.contestedEventID2);
                evt_scripts.insert(goInfo->capturePoint.progressEventID1);
                evt_scripts.insert(goInfo->capturePoint.progressEventID2);
                evt_scripts.insert(goInfo->capturePoint.winEventID1);
                evt_scripts.insert(goInfo->capturePoint.winEventID2);
            }
        }
    }

    // Load all possible script entries from spells
    for (uint32 i = 1; i < sSpellStore.GetNumRows(); ++i)
    {
        SpellEntry const* spell = sSpellStore.LookupEntry(i);
        if (spell)
        {
            for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
            {
                SpellEffectEntry const* spellEffect = spell->GetSpellEffect(SpellEffectIndex(j));
                if (!spellEffect)
                    continue;

                if (spellEffect->Effect == SPELL_EFFECT_SEND_EVENT)
                {
                    if (spellEffect->EffectMiscValue)
                        evt_scripts.insert(spellEffect->EffectMiscValue);
                }
            }
        }
    }

    for (size_t path_idx = 0; path_idx < sTaxiPathNodesByPath.size(); ++path_idx)
    {
        for (size_t node_idx = 0; node_idx < sTaxiPathNodesByPath[path_idx].size(); ++node_idx)
        {
            TaxiPathNodeEntry const& node = sTaxiPathNodesByPath[path_idx][node_idx];

            if (node.arrivalEventID)
                evt_scripts.insert(node.arrivalEventID);

            if (node.departureEventID)
                evt_scripts.insert(node.departureEventID);
        }
    }

    // Then check if all scripts are in above list of possible script entries
    for (ScriptMapMap::const_iterator itr = sEventScripts.second.begin(); itr != sEventScripts.second.end(); ++itr)
    {
        std::set<uint32>::const_iterator itr2 = evt_scripts.find(itr->first);
        if (itr2 == evt_scripts.end())
            sLog.outErrorDb("Table `event_scripts` has script (Id: %u) not referring to any gameobject_template type 10 data2 field, type 3 data6 field, type 13 data 2 field, type 29 or any spell effect %u or path taxi node data",
                            itr->first, SPELL_EFFECT_SEND_EVENT);
    }
}

void ScriptMgr::LoadGossipScripts()
{
    LoadScripts(sGossipScripts, "gossip_scripts");

    // checks are done in LoadGossipMenuItems and LoadGossipMenu
}

void ScriptMgr::LoadCreatureMovementScripts()
{
    LoadScripts(sCreatureMovementScripts, "creature_movement_scripts");

    // checks are done in WaypointManager::Load
}

void ScriptMgr::LoadDbScriptStrings()
{
    sObjectMgr.LoadMangosStrings(WorldDatabase, "db_script_string", MIN_DB_SCRIPT_STRING_ID, MAX_DB_SCRIPT_STRING_ID);

    std::set<int32> ids;

    for (int32 i = MIN_DB_SCRIPT_STRING_ID; i < MAX_DB_SCRIPT_STRING_ID; ++i)
        if (sObjectMgr.GetMangosStringLocale(i))
            ids.insert(i);

    CheckScriptTexts(sQuestEndScripts, ids);
    CheckScriptTexts(sQuestStartScripts, ids);
    CheckScriptTexts(sSpellScripts, ids);
    CheckScriptTexts(sGameObjectScripts, ids);
    CheckScriptTexts(sGameObjectTemplateScripts, ids);
    CheckScriptTexts(sEventScripts, ids);
    CheckScriptTexts(sGossipScripts, ids);
    CheckScriptTexts(sCreatureMovementScripts, ids);

    sWaypointMgr.CheckTextsExistance(ids);

    for (std::set<int32>::const_iterator itr = ids.begin(); itr != ids.end(); ++itr)
        sLog.outErrorDb("Table `db_script_string` has unused string id %u", *itr);
}

void ScriptMgr::CheckScriptTexts(ScriptMapMapName const& scripts, std::set<int32>& ids)
{
    for (ScriptMapMap::const_iterator itrMM = scripts.second.begin(); itrMM != scripts.second.end(); ++itrMM)
    {
        for (ScriptMap::const_iterator itrM = itrMM->second.begin(); itrM != itrMM->second.end(); ++itrM)
        {
            if (itrM->second.command == SCRIPT_COMMAND_TALK)
            {
                for (int i = 0; i < MAX_TEXT_ID; ++i)
                {
                    if (itrM->second.textId[i] && !sObjectMgr.GetMangosStringLocale(itrM->second.textId[i]))
                        sLog.outErrorDb("Table `db_script_string` is missing string id %u, used in database script table %s id %u.", itrM->second.textId[i], scripts.first, itrMM->first);

                    if (ids.find(itrM->second.textId[i]) != ids.end())
                        ids.erase(itrM->second.textId[i]);
                }
            }
        }
    }
}

// /////////////////////////////////////////////////////////
//              DB SCRIPT ENGINE
// /////////////////////////////////////////////////////////

/// Helper function to get Object source or target for Script-Command
/// returns false iff an error happened
bool ScriptAction::GetScriptCommandObject(const ObjectGuid guid, bool includeItem, Object*& resultObject)
{
    resultObject = NULL;

    if (!guid)
        return true;

    switch (guid.GetHigh())
    {
        case HIGHGUID_UNIT:
        case HIGHGUID_VEHICLE:
            resultObject = m_map->GetCreature(guid);
            break;
        case HIGHGUID_PET:
            resultObject = m_map->GetPet(guid);
            break;
        case HIGHGUID_PLAYER:
            resultObject = m_map->GetPlayer(guid);
            break;
        case HIGHGUID_GAMEOBJECT:
            resultObject = m_map->GetGameObject(guid);
            break;
        case HIGHGUID_CORPSE:
            resultObject = HashMapHolder<Corpse>::Find(guid);
            break;
        case HIGHGUID_ITEM:
            // case HIGHGUID_CONTAINER: ==HIGHGUID_ITEM
        {
            if (includeItem)
            {
                if (Player* player = m_map->GetPlayer(m_ownerGuid))
                    resultObject = player->GetItemByGuid(guid);
                break;
            }
            // else no break, but display error message
        }
        default:
            sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u with unsupported guid %s, skipping", m_table, m_script->id, m_script->command, guid.GetString().c_str());
            return false;
    }

    if (resultObject && !resultObject->IsInWorld())
        resultObject = NULL;

    return true;
}

/// Select source and target for a script command
/// Returns false iff an error happened
bool ScriptAction::GetScriptProcessTargets(WorldObject* pOrigSource, WorldObject* pOrigTarget, WorldObject*& pFinalSource, WorldObject*& pFinalTarget)
{
    WorldObject* pBuddy = NULL;

    if (m_script->buddyEntry)
    {
        if (!pOrigSource && !pOrigTarget)
        {
            sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u called without buddy %u, but no source for search available, skipping.", m_table, m_script->id, m_script->command, m_script->buddyEntry);
            return false;
        }

        // Prefer non-players as searcher
        WorldObject* pSearcher = pOrigSource ? pOrigSource : pOrigTarget;
        if (pSearcher->GetTypeId() == TYPEID_PLAYER && pOrigTarget && pOrigTarget->GetTypeId() != TYPEID_PLAYER)
            pSearcher = pOrigTarget;

        if (m_script->IsCreatureBuddy())
        {
            Creature* pCreatureBuddy = NULL;

            MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck u_check(*pSearcher, m_script->buddyEntry, true, false, m_script->searchRadius);
            MaNGOS::CreatureLastSearcher<MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(pCreatureBuddy, u_check);

            Cell::VisitGridObjects(pSearcher, searcher, m_script->searchRadius);
            pBuddy = pCreatureBuddy;
        }
        else
        {
            GameObject* pGOBuddy = NULL;

            MaNGOS::NearestGameObjectEntryInObjectRangeCheck u_check(*pSearcher, m_script->buddyEntry, m_script->searchRadius);
            MaNGOS::GameObjectLastSearcher<MaNGOS::NearestGameObjectEntryInObjectRangeCheck> searcher(pGOBuddy, u_check);

            Cell::VisitGridObjects(pSearcher, searcher, m_script->searchRadius);
            pBuddy = pGOBuddy;
        }

        if (!pBuddy)
        {
            sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u has buddy %u not found in range %u of searcher %s (data-flags %u), skipping.", m_table, m_script->id, m_script->command, m_script->buddyEntry, m_script->searchRadius, pSearcher->GetGuidStr().c_str(), m_script->data_flags);
            return false;
        }
    }

    if (m_script->data_flags & SCRIPT_FLAG_BUDDY_AS_TARGET)
    {
        pFinalSource = pOrigSource;
        pFinalTarget = pBuddy;
    }
    else
    {
        pFinalSource = pBuddy ? pBuddy : pOrigSource;
        pFinalTarget = pOrigTarget;
    }

    if (m_script->data_flags & SCRIPT_FLAG_REVERSE_DIRECTION)
        std::swap(pFinalSource, pFinalTarget);

    if (m_script->data_flags & SCRIPT_FLAG_SOURCE_TARGETS_SELF)
        pFinalTarget = pFinalSource;

    return true;
}

/// Helper to log error information
bool ScriptAction::LogIfNotCreature(WorldObject* pWorldObject)
{
    if (!pWorldObject || pWorldObject->GetTypeId() != TYPEID_UNIT)
    {
        sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u call for non-creature, skipping.", m_table, m_script->id, m_script->command);
        return true;
    }
    return false;
}
bool ScriptAction::LogIfNotUnit(WorldObject* pWorldObject)
{
    if (!pWorldObject || !pWorldObject->isType(TYPEMASK_UNIT))
    {
        sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u call for non-unit, skipping.", m_table, m_script->id, m_script->command);
        return true;
    }
    return false;
}
bool ScriptAction::LogIfNotGameObject(WorldObject* pWorldObject)
{
    if (!pWorldObject || pWorldObject->GetTypeId() != TYPEID_GAMEOBJECT)
    {
        sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u call for non-gameobject, skipping.", m_table, m_script->id, m_script->command);
        return true;
    }
    return false;
}

/// Helper to get a player if possible (target preferred)
Player* ScriptAction::GetPlayerTargetOrSourceAndLog(WorldObject* pSource, WorldObject* pTarget)
{
    if ((!pTarget || pTarget->GetTypeId() != TYPEID_PLAYER) && (!pSource || pSource->GetTypeId() != TYPEID_PLAYER))
    {
        sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u call for non player, skipping.", m_table, m_script->id, m_script->command);
        return NULL;
    }

    return pTarget && pTarget->GetTypeId() == TYPEID_PLAYER ? (Player*)pTarget : (Player*)pSource;
}

/// Handle one Script Step
void ScriptAction::HandleScriptStep()
{
    WorldObject* pSource;
    WorldObject* pTarget;
    Object* pSourceOrItem;                                  // Stores a provided pSource (if exists as WorldObject) or source-item

    {                                                       // Add scope for source & target variables so that they are not used below
        Object* source = NULL;
        Object* target = NULL;
        if (!GetScriptCommandObject(m_sourceGuid, true, source))
            return;
        if (!GetScriptCommandObject(m_targetGuid, false, target))
            return;

        // Give some debug log output for easier use
        DEBUG_LOG("DB-SCRIPTS: Process table `%s` id %u, command %u for source %s (%sin world), target %s (%sin world)", m_table, m_script->id, m_script->command, m_sourceGuid.GetString().c_str(), source ? "" : "not ", m_targetGuid.GetString().c_str(), target ? "" : "not ");

        // Get expected source and target (if defined with buddy)
        pSource = source && source->isType(TYPEMASK_WORLDOBJECT) ? (WorldObject*)source : NULL;
        pTarget = target && target->isType(TYPEMASK_WORLDOBJECT) ? (WorldObject*)target : NULL;
        if (!GetScriptProcessTargets(pSource, pTarget, pSource, pTarget))
            return;

        pSourceOrItem = pSource ? pSource : (source && source->isType(TYPEMASK_ITEM) ? source : NULL);
    }

    switch (m_script->command)
    {
        case SCRIPT_COMMAND_TALK:                           // 0
        {
            if (!pSource)
            {
                sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u found no worldobject as source, skipping.", m_table, m_script->id, m_script->command);
                break;
            }

            Unit* unitTarget = pTarget && pTarget->isType(TYPEMASK_UNIT) ? static_cast<Unit*>(pTarget) : NULL;
            int32 textId = m_script->textId[0];

            // May have text for random
            if (m_script->textId[1])
            {
                int i = 2;
                for (; i < MAX_TEXT_ID; ++i)
                {
                    if (!m_script->textId[i])
                        break;
                }

                // Use one random
                textId = m_script->textId[urand(0, i - 1)];
            }

            switch (m_script->talk.chatType)
            {
                case CHAT_TYPE_SAY:
                    pSource->MonsterSay(textId, m_script->talk.language, unitTarget);
                    break;
                case CHAT_TYPE_YELL:
                    pSource->MonsterYell(textId, m_script->talk.language, unitTarget);
                    break;
                case CHAT_TYPE_TEXT_EMOTE:
                    pSource->MonsterTextEmote(textId, unitTarget);
                    break;
                case CHAT_TYPE_BOSS_EMOTE:
                    pSource->MonsterTextEmote(textId, unitTarget, true);
                    break;
                case CHAT_TYPE_WHISPER:
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u attempt to whisper (%u) to %s, skipping.", m_table, m_script->id, m_script->command, m_script->talk.chatType, unitTarget ? unitTarget->GetGuidStr().c_str() : "<no target>");
                        break;
                    }
                    pSource->MonsterWhisper(textId, unitTarget);
                    break;
                case CHAT_TYPE_BOSS_WHISPER:
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                    {
                        sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u attempt to whisper (%u) to %s, skipping.", m_table, m_script->id, m_script->command, m_script->talk.chatType, unitTarget ? unitTarget->GetGuidStr().c_str() : "<no target>");
                        break;
                    }
                    pSource->MonsterWhisper(textId, unitTarget, true);
                    break;
                case CHAT_TYPE_ZONE_YELL:
                    pSource->MonsterYellToZone(textId, m_script->talk.language, unitTarget);
                    break;
                default:
                    break;                                  // must be already checked at load
            }
            break;
        }
        case SCRIPT_COMMAND_EMOTE:                          // 1
        {
            if (LogIfNotUnit(pSource))
                break;

            ((Unit*)pSource)->HandleEmote(m_script->emote.emoteId);
            break;
        }
        case SCRIPT_COMMAND_FIELD_SET:                      // 2
            if (!pSourceOrItem)
            {
                sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u call for NULL object.", m_table, m_script->id, m_script->command);
                break;
            }
            if (m_script->setField.fieldId <= OBJECT_FIELD_ENTRY || m_script->setField.fieldId >= pSourceOrItem->GetValuesCount())
            {
                sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u call for wrong field %u (max count: %u) in %s.",
                              m_table, m_script->id, m_script->command, m_script->setField.fieldId, pSourceOrItem->GetValuesCount(), pSourceOrItem->GetGuidStr().c_str());
                break;
            }
            pSourceOrItem->SetUInt32Value(m_script->setField.fieldId, m_script->setField.fieldValue);
            break;
        case SCRIPT_COMMAND_MOVE_TO:                        // 3
        {
            if (LogIfNotUnit(pSource))
                break;

            // Just turn around
            if ((m_script->x == 0.0f && m_script->y == 0.0f && m_script->z == 0.0f) ||
                    // Check point-to-point distance, hence revert effect of bounding radius
                    ((Unit*)pSource)->IsWithinDist3d(m_script->x, m_script->y, m_script->z, 0.01f - ((Unit*)pSource)->GetObjectBoundingRadius()))
            {
                ((Unit*)pSource)->SetFacingTo(m_script->o);
                break;
            }

            // For command additional teleport the unit
            if (m_script->data_flags & SCRIPT_FLAG_COMMAND_ADDITIONAL)
            {
                ((Unit*)pSource)->NearTeleportTo(m_script->x, m_script->y, m_script->z, m_script->o != 0.0f ? m_script->o : ((Unit*)pSource)->GetOrientation());
                break;
            }

            // Normal Movement
            if (m_script->moveTo.travelSpeed)
                ((Unit*)pSource)->MonsterMoveWithSpeed(m_script->x, m_script->y, m_script->z, m_script->moveTo.travelSpeed * 0.01f);
            else
            {
                ((Unit*)pSource)->GetMotionMaster()->Clear();
                ((Unit*)pSource)->GetMotionMaster()->MovePoint(0, m_script->x, m_script->y, m_script->z);
            }
            break;
        }
        case SCRIPT_COMMAND_FLAG_SET:                       // 4
            if (!pSourceOrItem)
            {
                sLog.outError("SCRIPT_COMMAND_FLAG_SET (script id %u) call for NULL object.", m_script->id);
                break;
            }
            if (m_script->setFlag.fieldId <= OBJECT_FIELD_ENTRY || m_script->setFlag.fieldId >= pSourceOrItem->GetValuesCount())
            {
                sLog.outError("SCRIPT_COMMAND_FLAG_SET (script id %u) call for wrong field %u (max count: %u) in %s.",
                              m_script->id, m_script->setFlag.fieldId, pSourceOrItem->GetValuesCount(), pSourceOrItem->GetGuidStr().c_str());
                break;
            }
            pSourceOrItem->SetFlag(m_script->setFlag.fieldId, m_script->setFlag.fieldValue);
            break;
        case SCRIPT_COMMAND_FLAG_REMOVE:                    // 5
            if (!pSourceOrItem)
            {
                sLog.outError("SCRIPT_COMMAND_FLAG_REMOVE (script id %u) call for NULL object.", m_script->id);
                break;
            }
            if (m_script->removeFlag.fieldId <= OBJECT_FIELD_ENTRY || m_script->removeFlag.fieldId >= pSourceOrItem->GetValuesCount())
            {
                sLog.outError("SCRIPT_COMMAND_FLAG_REMOVE (script id %u) call for wrong field %u (max count: %u) in %s.",
                              m_script->id, m_script->removeFlag.fieldId, pSourceOrItem->GetValuesCount(), pSourceOrItem->GetGuidStr().c_str());
                break;
            }
            pSourceOrItem->RemoveFlag(m_script->removeFlag.fieldId, m_script->removeFlag.fieldValue);
            break;
        case SCRIPT_COMMAND_TELEPORT_TO:                    // 6
        {
            Player* pPlayer = GetPlayerTargetOrSourceAndLog(pSource, pTarget);
            if (!pPlayer)
                break;

            pPlayer->TeleportTo(m_script->teleportTo.mapId, m_script->x, m_script->y, m_script->z, m_script->o);
            break;
        }
        case SCRIPT_COMMAND_QUEST_EXPLORED:                 // 7
        {
            Player* pPlayer = GetPlayerTargetOrSourceAndLog(pSource, pTarget);
            if (!pPlayer)
                break;

            WorldObject* pWorldObject = NULL;
            if (pSource && pSource->isType(TYPEMASK_CREATURE_OR_GAMEOBJECT))
                pWorldObject = pSource;
            else if (pTarget && pTarget->isType(TYPEMASK_CREATURE_OR_GAMEOBJECT))
                pWorldObject = pTarget;

            // if we have a distance, we must have a worldobject
            if (m_script->questExplored.distance != 0 && !pWorldObject)
            {
                sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u called without source worldobject, skipping.", m_table, m_script->id, m_script->command);
                break;
            }

            bool failQuest = false;
            // Creature must be alive for giving credit
            if (pWorldObject && pWorldObject->GetTypeId() == TYPEID_UNIT && !((Creature*)pWorldObject)->isAlive())
                failQuest = true;
            else if (m_script->questExplored.distance != 0 && !pWorldObject->IsWithinDistInMap(pPlayer, float(m_script->questExplored.distance)))
                failQuest = true;

            // quest id and flags checked at script loading
            if (!failQuest)
                pPlayer->AreaExploredOrEventHappens(m_script->questExplored.questId);
            else
                pPlayer->FailQuest(m_script->questExplored.questId);

            break;
        }
        case SCRIPT_COMMAND_KILL_CREDIT:                    // 8
        {
            Player* pPlayer = GetPlayerTargetOrSourceAndLog(pSource, pTarget);
            if (!pPlayer)
                break;

            uint32 creatureEntry = m_script->killCredit.creatureEntry;
            WorldObject* pRewardSource = pSource && pSource->GetTypeId() == TYPEID_UNIT ? pSource : (pTarget && pTarget->GetTypeId() == TYPEID_UNIT ? pTarget : NULL);

            // dynamic effect, take entry of reward Source
            if (!creatureEntry)
            {
                if (pRewardSource)
                    creatureEntry =  pRewardSource->GetEntry();
                else
                {
                    sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u called for dynamic killcredit without creature partner, skipping.", m_table, m_script->id, m_script->command);
                    break;
                }
            }

            if (m_script->killCredit.isGroupCredit)
            {
                WorldObject* pSearcher = pRewardSource ? pRewardSource : (pSource ? pSource : pTarget);
                if (pSearcher != pRewardSource)
                    sLog.outDebug(" DB-SCRIPTS: Process table `%s` id %u, SCRIPT_COMMAND_KILL_CREDIT called for groupCredit without creature as searcher, script might need adjustment.", m_table, m_script->id);
                pPlayer->RewardPlayerAndGroupAtEvent(creatureEntry, pSearcher);
            }
            else
                pPlayer->KilledMonsterCredit(creatureEntry, pRewardSource ? pRewardSource->GetObjectGuid() : ObjectGuid());

            break;
        }
        case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT:             // 9
        {
            GameObject* pGo = NULL;
            uint32 time_to_despawn = m_script->respawnGo.despawnDelay < 5 ? 5 : m_script->respawnGo.despawnDelay;

            if (m_script->respawnGo.goGuid)
            {
                GameObjectData const* goData = sObjectMgr.GetGOData(m_script->respawnGo.goGuid);
                if (!goData)
                    break;                                  // checked at load

                // TODO - This was a change, was before current map of source
                pGo = m_map->GetGameObject(ObjectGuid(HIGHGUID_GAMEOBJECT, goData->id, m_script->respawnGo.goGuid));
            }
            else
            {
                if (LogIfNotGameObject(pSource))
                    break;

                pGo = (GameObject*)pSource;
            }

            if (!pGo)
            {
                sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u failed for gameobject(guid: %u, buddyEntry: %u).", m_table, m_script->id, m_script->command, m_script->respawnGo.goGuid, m_script->buddyEntry);
                break;
            }

            if (pGo->GetGoType() == GAMEOBJECT_TYPE_FISHINGNODE ||
                    pGo->GetGoType() == GAMEOBJECT_TYPE_DOOR        ||
                    pGo->GetGoType() == GAMEOBJECT_TYPE_BUTTON      ||
                    pGo->GetGoType() == GAMEOBJECT_TYPE_TRAP)
            {
                sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u can not be used with gameobject of type %u (guid: %u, buddyEntry: %u).", m_table, m_script->id, m_script->command, uint32(pGo->GetGoType()), m_script->respawnGo.goGuid, m_script->buddyEntry);
                break;
            }

            if (pGo->isSpawned())
                break;                                      // gameobject already spawned

            pGo->SetLootState(GO_READY);
            pGo->SetRespawnTime(time_to_despawn);           // despawn object in ? seconds
            pGo->Refresh();
            break;
        }
        case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:           // 10
        {
            if (!pSource)
            {
                sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u found no worldobject as source, skipping.", m_table, m_script->id, m_script->command);
                break;
            }

            float x = m_script->x;
            float y = m_script->y;
            float z = m_script->z;
            float o = m_script->o;

            Creature* pCreature = pSource->SummonCreature(m_script->summonCreature.creatureEntry, x, y, z, o, m_script->summonCreature.despawnDelay ? TEMPSUMMON_TIMED_OR_DEAD_DESPAWN : TEMPSUMMON_DEAD_DESPAWN, m_script->summonCreature.despawnDelay, (m_script->data_flags & SCRIPT_FLAG_COMMAND_ADDITIONAL) ? true : false);
            if (!pCreature)
            {
                sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u failed for creature (entry: %u).", m_table, m_script->id, m_script->command, m_script->summonCreature.creatureEntry);
                break;
            }

            break;
        }
        case SCRIPT_COMMAND_OPEN_DOOR:                      // 11
        case SCRIPT_COMMAND_CLOSE_DOOR:                     // 12
        {
            GameObject* pDoor;
            uint32 time_to_reset = m_script->changeDoor.resetDelay < 15 ? 15 : m_script->changeDoor.resetDelay;

            if (m_script->changeDoor.goGuid)
            {
                GameObjectData const* goData = sObjectMgr.GetGOData(m_script->changeDoor.goGuid);
                if (!goData)                                // checked at load
                    break;

                // TODO - Was a change, before random map
                pDoor = m_map->GetGameObject(ObjectGuid(HIGHGUID_GAMEOBJECT, goData->id, m_script->changeDoor.goGuid));
            }
            else
            {
                if (LogIfNotGameObject(pSource))
                    break;

                pDoor = (GameObject*)pSource;
            }

            if (!pDoor)
            {
                sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u failed for gameobject(guid: %u, buddyEntry: %u).", m_table, m_script->id, m_script->command, m_script->changeDoor.goGuid, m_script->buddyEntry);
                break;
            }

            if (pDoor->GetGoType() != GAMEOBJECT_TYPE_DOOR)
            {
                sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u failed for non-door(GoType: %u).", m_table, m_script->id, m_script->command, pDoor->GetGoType());
                break;
            }

            if ((m_script->command == SCRIPT_COMMAND_OPEN_DOOR && pDoor->GetGoState() != GO_STATE_READY) ||
                    (m_script->command == SCRIPT_COMMAND_CLOSE_DOOR && pDoor->GetGoState() == GO_STATE_READY))
                break;                                      // to be opened door already open, or to be closed door already closed

            pDoor->UseDoorOrButton(time_to_reset);

            if (pTarget && pTarget->isType(TYPEMASK_GAMEOBJECT) && ((GameObject*)pTarget)->GetGoType() == GAMEOBJECT_TYPE_BUTTON)
                ((GameObject*)pTarget)->UseDoorOrButton(time_to_reset);

            break;
        }
        case SCRIPT_COMMAND_ACTIVATE_OBJECT:                // 13
        {
            if (LogIfNotUnit(pSource))
                break;
            if (LogIfNotGameObject(pTarget))
                break;

            ((GameObject*)pTarget)->Use((Unit*)pSource);
            break;
        }
        case SCRIPT_COMMAND_REMOVE_AURA:                    // 14
        {
            if (LogIfNotUnit(pSource))
                break;

            ((Unit*)pSource)->RemoveAurasDueToSpell(m_script->removeAura.spellId);
            break;
        }
        case SCRIPT_COMMAND_CAST_SPELL:                     // 15
        {
            if (LogIfNotUnit(pSource))
                break;
            if (LogIfNotUnit(pTarget))
                break;

            // TODO: when GO cast implemented, code below must be updated accordingly to also allow GO spell cast
            ((Unit*)pSource)->CastSpell(((Unit*)pTarget), m_script->castSpell.spellId, (m_script->data_flags & SCRIPT_FLAG_COMMAND_ADDITIONAL) != 0);

            break;
        }
        case SCRIPT_COMMAND_PLAY_SOUND:                     // 16
        {
            if (!pSource)
            {
                sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u could not find proper source", m_table, m_script->id, m_script->command);
                break;
            }

            // bitmask: 0/1=anyone/target, 0/2=with distance dependent
            Player* pSoundTarget = NULL;
            if (m_script->playSound.flags & 1)
            {
                pSoundTarget = GetPlayerTargetOrSourceAndLog(pSource, pTarget);
                if (!pSoundTarget)
                    break;
            }

            // bitmask: 0/1=anyone/target, 0/2=with distance dependent
            if (m_script->playSound.flags & 2)
                pSource->PlayDistanceSound(m_script->playSound.soundId, pSoundTarget);
            else
                pSource->PlayDirectSound(m_script->playSound.soundId, pSoundTarget);

            break;
        }
        case SCRIPT_COMMAND_CREATE_ITEM:                    // 17
        {
            Player* pPlayer = GetPlayerTargetOrSourceAndLog(pSource, pTarget);
            if (!pPlayer)
                break;

            if (Item* pItem = pPlayer->StoreNewItemInInventorySlot(m_script->createItem.itemEntry, m_script->createItem.amount))
                pPlayer->SendNewItem(pItem, m_script->createItem.amount, true, false);

            break;
        }
        case SCRIPT_COMMAND_DESPAWN_SELF:                   // 18
        {
            // TODO - Remove this check after a while
            if (pTarget && pTarget->GetTypeId() != TYPEID_UNIT && pSource && pSource->GetTypeId() == TYPEID_UNIT)
            {
                sLog.outErrorDb("DB-SCRIPTS: Process table `%s` id %u, command %u target must be creature, but (only) source is, use data_flags to fix", m_table, m_script->id, m_script->command);
                pTarget = pSource;
            }

            if (LogIfNotCreature(pTarget))
                break;

            ((Creature*)pTarget)->ForcedDespawn(m_script->despawn.despawnDelay);

            break;
        }
        case SCRIPT_COMMAND_PLAY_MOVIE:                     // 19
        {
            Player* pPlayer = GetPlayerTargetOrSourceAndLog(pSource, pTarget);
            if (!pPlayer)
                break;

            pPlayer->SendMovieStart(m_script->playMovie.movieId);

            break;
        }
        case SCRIPT_COMMAND_MOVEMENT:                       // 20
        {
            if (LogIfNotCreature(pSource))
                break;

            // Consider add additional checks for cases where creature should not change movementType
            // (pet? in combat? already using same MMgen as script try to apply?)

            switch (m_script->movement.movementType)
            {
                case IDLE_MOTION_TYPE:
                    ((Creature*)pSource)->GetMotionMaster()->MoveIdle();
                    break;
                case RANDOM_MOTION_TYPE:
                    if (m_script->data_flags & SCRIPT_FLAG_COMMAND_ADDITIONAL)
                        ((Creature*)pSource)->GetMotionMaster()->MoveRandomAroundPoint(pSource->GetPositionX(), pSource->GetPositionY(), pSource->GetPositionZ(), float(m_script->movement.wanderDistance));
                    else
                    {
                        float respX, respY, respZ, respO, wander_distance;
                        ((Creature*)pSource)->GetRespawnCoord(respX, respY, respZ, &respO, &wander_distance);
                        wander_distance = m_script->movement.wanderDistance ? m_script->movement.wanderDistance : wander_distance;
                        ((Creature*)pSource)->GetMotionMaster()->MoveRandomAroundPoint(respX, respY, respZ, wander_distance);
                    }
                    break;
                case WAYPOINT_MOTION_TYPE:
                    ((Creature*)pSource)->GetMotionMaster()->MoveWaypoint();
                    break;
            }

            break;
        }
        case SCRIPT_COMMAND_SET_ACTIVEOBJECT:               // 21
        {
            if (LogIfNotCreature(pSource))
                break;

            ((Creature*)pSource)->SetActiveObjectState(m_script->activeObject.activate);
            break;
        }
        case SCRIPT_COMMAND_SET_FACTION:                    // 22
        {
            if (LogIfNotCreature(pSource))
                break;

            if (m_script->faction.factionId)
                ((Creature*)pSource)->SetFactionTemporary(m_script->faction.factionId, m_script->faction.flags);
            else
                ((Creature*)pSource)->ClearTemporaryFaction();

            break;
        }
        case SCRIPT_COMMAND_MORPH_TO_ENTRY_OR_MODEL:        // 23
        {
            if (LogIfNotCreature(pSource))
                break;

            if (!m_script->morph.creatureOrModelEntry)
                ((Creature*)pSource)->DeMorph();
            else if (m_script->data_flags & SCRIPT_FLAG_COMMAND_ADDITIONAL)
                ((Creature*)pSource)->SetDisplayId(m_script->morph.creatureOrModelEntry);
            else
            {
                CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(m_script->morph.creatureOrModelEntry);
                uint32 display_id = Creature::ChooseDisplayId(ci);

                ((Creature*)pSource)->SetDisplayId(display_id);
            }

            break;
        }
        case SCRIPT_COMMAND_MOUNT_TO_ENTRY_OR_MODEL:        // 24
        {
            if (LogIfNotCreature(pSource))
                break;

            if (!m_script->mount.creatureOrModelEntry)
                ((Creature*)pSource)->Unmount();
            else if (m_script->data_flags & SCRIPT_FLAG_COMMAND_ADDITIONAL)
                ((Creature*)pSource)->Mount(m_script->mount.creatureOrModelEntry);
            else
            {
                CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(m_script->mount.creatureOrModelEntry);
                uint32 display_id = Creature::ChooseDisplayId(ci);

                ((Creature*)pSource)->Mount(display_id);
            }

            break;
        }
        case SCRIPT_COMMAND_SET_RUN:                        // 25
        {
            if (LogIfNotCreature(pSource))
                break;

            ((Creature*)pSource)->SetWalk(!m_script->run.run, true);

            break;
        }
        case SCRIPT_COMMAND_ATTACK_START:                   // 26
        {
            if (LogIfNotCreature(pSource))
                break;
            if (LogIfNotUnit(pTarget))
                break;

            Creature* pAttacker = static_cast<Creature*>(pSource);
            Unit* unitTarget = static_cast<Unit*>(pTarget);

            if (pAttacker->IsFriendlyTo(unitTarget))
            {
                sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u attacker is friendly to target, can not attack (Attacker: %s, Target: %s)", m_table, m_script->id, m_script->command, pAttacker->GetGuidStr().c_str(), unitTarget->GetGuidStr().c_str());
                break;
            }

            pAttacker->AI()->AttackStart(unitTarget);

            break;
        }
        case SCRIPT_COMMAND_GO_LOCK_STATE:                  // 27
        {
            if (LogIfNotGameObject(pSource))
                break;

            GameObject* pGo = static_cast<GameObject*>(pSource);

            /* flag lockState
             * go_lock          0x01
             * go_unlock        0x02
             * go_nonInteract   0x04
             * go_Interact      0x08
             */

            // Lock or Unlock
            if (m_script->goLockState.lockState & 0x01)
                pGo->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED);
            else if (m_script->goLockState.lockState & 0x02)
                pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED);
            // Set Non Interactable or Set Interactable
            if (m_script->goLockState.lockState & 0x04)
                pGo->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
            else if (m_script->goLockState.lockState & 0x08)
                pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);

            break;
        }
        case SCRIPT_COMMAND_STAND_STATE:                    // 28
        {
            if (LogIfNotCreature(pSource))
                break;

            // Must be safe cast to Unit* here
            ((Unit*)pSource)->SetStandState(m_script->standState.stand_state);
            break;
        }
        case SCRIPT_COMMAND_MODIFY_NPC_FLAGS:               // 29
        {
            if (LogIfNotCreature(pSource))
                break;

            // Add Flags
            if (m_script->npcFlag.change_flag & 0x01)
                pSource->SetFlag(UNIT_NPC_FLAGS, m_script->npcFlag.flag);
            // Remove Flags
            else if (m_script->npcFlag.change_flag & 0x02)
                pSource->RemoveFlag(UNIT_NPC_FLAGS, m_script->npcFlag.flag);
            // Toggle Flags
            else
            {
                if (pSource->HasFlag(UNIT_NPC_FLAGS, m_script->npcFlag.flag))
                    pSource->RemoveFlag(UNIT_NPC_FLAGS, m_script->npcFlag.flag);
                else
                    pSource->SetFlag(UNIT_NPC_FLAGS, m_script->npcFlag.flag);
            }

            break;
        }
        case SCRIPT_COMMAND_SEND_TAXI_PATH:                 // 30
        {
            // only Player
            Player* pPlayer = GetPlayerTargetOrSourceAndLog(pSource, pTarget);
            if (!pPlayer)
                break;

            pPlayer->ActivateTaxiPathTo(m_script->sendTaxiPath.taxiPathId);
            break;
        }
        default:
            sLog.outError(" DB-SCRIPTS: Process table `%s` id %u, command %u unknown command used.", m_table, m_script->id, m_script->command);
            break;
    }
}

// /////////////////////////////////////////////////////////
//              Scripting Library Hooks
// /////////////////////////////////////////////////////////

void ScriptMgr::LoadAreaTriggerScripts()
{
    m_AreaTriggerScripts.clear();                           // need for reload case
    QueryResult* result = WorldDatabase.Query("SELECT entry, ScriptName FROM scripted_areatrigger");

    uint32 count = 0;

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded %u scripted areatrigger", count);
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        ++count;
        bar.step();

        Field* fields = result->Fetch();

        uint32 triggerId       = fields[0].GetUInt32();
        const char* scriptName = fields[1].GetString();

        if (!sAreaTriggerStore.LookupEntry(triggerId))
        {
            sLog.outErrorDb("Table `scripted_areatrigger` has area trigger (ID: %u) not listed in `AreaTrigger.dbc`.", triggerId);
            continue;
        }

        m_AreaTriggerScripts[triggerId] = GetScriptId(scriptName);
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u areatrigger scripts", count);
}

void ScriptMgr::LoadEventIdScripts()
{
    m_EventIdScripts.clear();                           // need for reload case
    QueryResult* result = WorldDatabase.Query("SELECT id, ScriptName FROM scripted_event_id");

    uint32 count = 0;

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded %u scripted event id", count);
        return;
    }

    BarGoLink bar(result->GetRowCount());

    // TODO: remove duplicate code below, same way to collect event id's used in LoadEventScripts()
    std::set<uint32> evt_scripts;

    // Load all possible event entries from gameobjects
    for (uint32 i = 1; i < sGOStorage.GetMaxEntry(); ++i)
    {
        if (GameObjectInfo const* goInfo = sGOStorage.LookupEntry<GameObjectInfo>(i))
        {
            if (uint32 eventId = goInfo->GetEventScriptId())
                evt_scripts.insert(eventId);

            if (goInfo->type == GAMEOBJECT_TYPE_CAPTURE_POINT)
            {
                evt_scripts.insert(goInfo->capturePoint.neutralEventID1);
                evt_scripts.insert(goInfo->capturePoint.neutralEventID2);
                evt_scripts.insert(goInfo->capturePoint.contestedEventID1);
                evt_scripts.insert(goInfo->capturePoint.contestedEventID2);
                evt_scripts.insert(goInfo->capturePoint.progressEventID1);
                evt_scripts.insert(goInfo->capturePoint.progressEventID2);
                evt_scripts.insert(goInfo->capturePoint.winEventID1);
                evt_scripts.insert(goInfo->capturePoint.winEventID2);
            }
        }
    }

    // Load all possible event entries from spells
    for (uint32 i = 1; i < sSpellStore.GetNumRows(); ++i)
    {
        SpellEntry const* spell = sSpellStore.LookupEntry(i);
        if (spell)
        {
            for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
            {
                SpellEffectEntry const* spellEffect = spell->GetSpellEffect(SpellEffectIndex(j));
                if (!spellEffect)
                    continue;

                if (spellEffect->Effect == SPELL_EFFECT_SEND_EVENT)
                {
                    if (spellEffect->EffectMiscValue)
                        evt_scripts.insert(spellEffect->EffectMiscValue);
                }
            }
        }
    }

    // Load all possible event entries from taxi path nodes
    for (size_t path_idx = 0; path_idx < sTaxiPathNodesByPath.size(); ++path_idx)
    {
        for (size_t node_idx = 0; node_idx < sTaxiPathNodesByPath[path_idx].size(); ++node_idx)
        {
            TaxiPathNodeEntry const& node = sTaxiPathNodesByPath[path_idx][node_idx];

            if (node.arrivalEventID)
                evt_scripts.insert(node.arrivalEventID);

            if (node.departureEventID)
                evt_scripts.insert(node.departureEventID);
        }
    }

    do
    {
        ++count;
        bar.step();

        Field* fields = result->Fetch();

        uint32 eventId          = fields[0].GetUInt32();
        const char* scriptName  = fields[1].GetString();

        std::set<uint32>::const_iterator itr = evt_scripts.find(eventId);
        if (itr == evt_scripts.end())
            sLog.outErrorDb("Table `scripted_event_id` has id %u not referring to any gameobject_template type 10 data2 field, type 3 data6 field, type 13 data 2 field, type 29 or any spell effect %u or path taxi node data",
                            eventId, SPELL_EFFECT_SEND_EVENT);

        m_EventIdScripts[eventId] = GetScriptId(scriptName);
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u scripted event id", count);
}

void ScriptMgr::LoadScriptNames()
{
    m_scriptNames.push_back("");
    QueryResult* result = WorldDatabase.Query(
                              "SELECT DISTINCT(ScriptName) FROM creature_template WHERE ScriptName <> '' "
                              "UNION "
                              "SELECT DISTINCT(ScriptName) FROM gameobject_template WHERE ScriptName <> '' "
                              "UNION "
                              "SELECT DISTINCT(ScriptName) FROM item_template WHERE ScriptName <> '' "
                              "UNION "
                              "SELECT DISTINCT(ScriptName) FROM scripted_areatrigger WHERE ScriptName <> '' "
                              "UNION "
                              "SELECT DISTINCT(ScriptName) FROM scripted_event_id WHERE ScriptName <> '' "
                              "UNION "
                              "SELECT DISTINCT(ScriptName) FROM instance_template WHERE ScriptName <> '' "
                              "UNION "
                              "SELECT DISTINCT(ScriptName) FROM world_template WHERE ScriptName <> ''");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString();
        sLog.outErrorDb(">> Loaded empty set of Script Names!");
        return;
    }

    BarGoLink bar(result->GetRowCount());
    uint32 count = 0;

    do
    {
        bar.step();
        m_scriptNames.push_back((*result)[0].GetString());
        ++count;
    }
    while (result->NextRow());
    delete result;

    std::sort(m_scriptNames.begin(), m_scriptNames.end());
    sLog.outString();
    sLog.outString(">> Loaded %d Script Names", count);
}

uint32 ScriptMgr::GetScriptId(const char* name) const
{
    // use binary search to find the script name in the sorted vector
    // assume "" is the first element
    if (!name)
        return 0;

    ScriptNameMap::const_iterator itr =
        std::lower_bound(m_scriptNames.begin(), m_scriptNames.end(), name);

    if (itr == m_scriptNames.end() || *itr != name)
        return 0;

    return uint32(itr - m_scriptNames.begin());
}

uint32 ScriptMgr::GetAreaTriggerScriptId(uint32 triggerId) const
{
    AreaTriggerScriptMap::const_iterator itr = m_AreaTriggerScripts.find(triggerId);
    if (itr != m_AreaTriggerScripts.end())
        return itr->second;

    return 0;
}

uint32 ScriptMgr::GetEventIdScriptId(uint32 eventId) const
{
    EventIdScriptMap::const_iterator itr = m_EventIdScripts.find(eventId);
    if (itr != m_EventIdScripts.end())
        return itr->second;

    return 0;
}

char const* ScriptMgr::GetScriptLibraryVersion() const
{
    if (!m_pGetScriptLibraryVersion)
        return "";

    return m_pGetScriptLibraryVersion();
}

CreatureAI* ScriptMgr::GetCreatureAI(Creature* pCreature)
{
    if (!m_pGetCreatureAI)
        return NULL;

    return m_pGetCreatureAI(pCreature);
}

InstanceData* ScriptMgr::CreateInstanceData(Map* pMap)
{
    if (!m_pCreateInstanceData)
        return NULL;

    return m_pCreateInstanceData(pMap);
}

bool ScriptMgr::OnGossipHello(Player* pPlayer, Creature* pCreature)
{
    return m_pOnGossipHello != NULL && m_pOnGossipHello(pPlayer, pCreature);
}

bool ScriptMgr::OnGossipHello(Player* pPlayer, GameObject* pGameObject)
{
    return m_pOnGOGossipHello != NULL && m_pOnGOGossipHello(pPlayer, pGameObject);
}

bool ScriptMgr::OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action, const char* code)
{
    if (code)
        return m_pOnGossipSelectWithCode != NULL && m_pOnGossipSelectWithCode(pPlayer, pCreature, sender, action, code);
    else
        return m_pOnGossipSelect != NULL && m_pOnGossipSelect(pPlayer, pCreature, sender, action);
}

bool ScriptMgr::OnGossipSelect(Player* pPlayer, GameObject* pGameObject, uint32 sender, uint32 action, const char* code)
{
    if (code)
        return m_pOnGOGossipSelectWithCode != NULL && m_pOnGOGossipSelectWithCode(pPlayer, pGameObject, sender, action, code);
    else
        return m_pOnGOGossipSelect != NULL && m_pOnGOGossipSelect(pPlayer, pGameObject, sender, action);
}

bool ScriptMgr::OnQuestAccept(Player* pPlayer, Creature* pCreature, Quest const* pQuest)
{
    return m_pOnQuestAccept != NULL && m_pOnQuestAccept(pPlayer, pCreature, pQuest);
}

bool ScriptMgr::OnQuestAccept(Player* pPlayer, GameObject* pGameObject, Quest const* pQuest)
{
    return m_pOnGOQuestAccept != NULL && m_pOnGOQuestAccept(pPlayer, pGameObject, pQuest);
}

bool ScriptMgr::OnQuestAccept(Player* pPlayer, Item* pItem, Quest const* pQuest)
{
    return m_pOnItemQuestAccept != NULL && m_pOnItemQuestAccept(pPlayer, pItem, pQuest);
}

bool ScriptMgr::OnQuestRewarded(Player* pPlayer, Creature* pCreature, Quest const* pQuest)
{
    return m_pOnQuestRewarded != NULL && m_pOnQuestRewarded(pPlayer, pCreature, pQuest);
}

bool ScriptMgr::OnQuestRewarded(Player* pPlayer, GameObject* pGameObject, Quest const* pQuest)
{
    return m_pOnGOQuestRewarded != NULL && m_pOnGOQuestRewarded(pPlayer, pGameObject, pQuest);
}

uint32 ScriptMgr::GetDialogStatus(Player* pPlayer, Creature* pCreature)
{
    if (!m_pGetNPCDialogStatus)
        return 100;

    return m_pGetNPCDialogStatus(pPlayer, pCreature);
}

uint32 ScriptMgr::GetDialogStatus(Player* pPlayer, GameObject* pGameObject)
{
    if (!m_pGetGODialogStatus)
        return 100;

    return m_pGetGODialogStatus(pPlayer, pGameObject);
}

bool ScriptMgr::OnGameObjectUse(Player* pPlayer, GameObject* pGameObject)
{
    return m_pOnGOUse != NULL && m_pOnGOUse(pPlayer, pGameObject);
}

bool ScriptMgr::OnItemUse(Player* pPlayer, Item* pItem, SpellCastTargets const& targets)
{
    return m_pOnItemUse != NULL && m_pOnItemUse(pPlayer, pItem, targets);
}

bool ScriptMgr::OnAreaTrigger(Player* pPlayer, AreaTriggerEntry const* atEntry)
{
    return m_pOnAreaTrigger != NULL && m_pOnAreaTrigger(pPlayer, atEntry);
}

bool ScriptMgr::OnProcessEvent(uint32 eventId, Object* pSource, Object* pTarget, bool isStart)
{
    return m_pOnProcessEvent != NULL && m_pOnProcessEvent(eventId, pSource, pTarget, isStart);
}

bool ScriptMgr::OnEffectDummy(Unit* pCaster, uint32 spellId, SpellEffectIndex effIndex, Creature* pTarget)
{
    return m_pOnEffectDummyCreature != NULL && m_pOnEffectDummyCreature(pCaster, spellId, effIndex, pTarget);
}

bool ScriptMgr::OnEffectDummy(Unit* pCaster, uint32 spellId, SpellEffectIndex effIndex, GameObject* pTarget)
{
    return m_pOnEffectDummyGO != NULL && m_pOnEffectDummyGO(pCaster, spellId, effIndex, pTarget);
}

bool ScriptMgr::OnEffectDummy(Unit* pCaster, uint32 spellId, SpellEffectIndex effIndex, Item* pTarget)
{
    return m_pOnEffectDummyItem != NULL && m_pOnEffectDummyItem(pCaster, spellId, effIndex, pTarget);
}

bool ScriptMgr::OnAuraDummy(Aura const* pAura, bool apply)
{
    return m_pOnAuraDummy != NULL && m_pOnAuraDummy(pAura, apply);
}

ScriptLoadResult ScriptMgr::LoadScriptLibrary(const char* libName)
{
    UnloadScriptLibrary();

    std::string name = libName;
    name = MANGOS_SCRIPT_PREFIX + name + MANGOS_SCRIPT_SUFFIX;

    m_hScriptLib = MANGOS_LOAD_LIBRARY(name.c_str());

    if (!m_hScriptLib)
        return SCRIPT_LOAD_ERR_NOT_FOUND;

#   define GET_SCRIPT_HOOK_PTR(P,N)             \
        GetScriptHookPtr((P), (N));             \
        if (!(P))                               \
        {                                       \
            /* prevent call before init */      \
            m_pOnFreeScriptLibrary = NULL;      \
            UnloadScriptLibrary();              \
            return SCRIPT_LOAD_ERR_WRONG_API;   \
        }

    // let check used mangosd revision for build library (unsafe use with different revision because changes in inline functions, define and etc)
    char const* (MANGOS_IMPORT * pGetMangosRevStr)();

    GET_SCRIPT_HOOK_PTR(pGetMangosRevStr,              "GetMangosRevStr");

    GET_SCRIPT_HOOK_PTR(m_pOnInitScriptLibrary,        "InitScriptLibrary");
    GET_SCRIPT_HOOK_PTR(m_pOnFreeScriptLibrary,        "FreeScriptLibrary");
    GET_SCRIPT_HOOK_PTR(m_pGetScriptLibraryVersion,    "GetScriptLibraryVersion");

    GET_SCRIPT_HOOK_PTR(m_pGetCreatureAI,              "GetCreatureAI");
    GET_SCRIPT_HOOK_PTR(m_pCreateInstanceData,         "CreateInstanceData");

    GET_SCRIPT_HOOK_PTR(m_pOnGossipHello,              "GossipHello");
    GET_SCRIPT_HOOK_PTR(m_pOnGOGossipHello,            "GOGossipHello");
    GET_SCRIPT_HOOK_PTR(m_pOnGossipSelect,             "GossipSelect");
    GET_SCRIPT_HOOK_PTR(m_pOnGOGossipSelect,           "GOGossipSelect");
    GET_SCRIPT_HOOK_PTR(m_pOnGossipSelectWithCode,     "GossipSelectWithCode");
    GET_SCRIPT_HOOK_PTR(m_pOnGOGossipSelectWithCode,   "GOGossipSelectWithCode");
    GET_SCRIPT_HOOK_PTR(m_pOnQuestAccept,              "QuestAccept");
    GET_SCRIPT_HOOK_PTR(m_pOnGOQuestAccept,            "GOQuestAccept");
    GET_SCRIPT_HOOK_PTR(m_pOnItemQuestAccept,          "ItemQuestAccept");
    GET_SCRIPT_HOOK_PTR(m_pOnQuestRewarded,            "QuestRewarded");
    GET_SCRIPT_HOOK_PTR(m_pOnGOQuestRewarded,          "GOQuestRewarded");
    GET_SCRIPT_HOOK_PTR(m_pGetNPCDialogStatus,         "GetNPCDialogStatus");
    GET_SCRIPT_HOOK_PTR(m_pGetGODialogStatus,          "GetGODialogStatus");
    GET_SCRIPT_HOOK_PTR(m_pOnGOUse,                    "GOUse");
    GET_SCRIPT_HOOK_PTR(m_pOnItemUse,                  "ItemUse");
    GET_SCRIPT_HOOK_PTR(m_pOnAreaTrigger,              "AreaTrigger");
    GET_SCRIPT_HOOK_PTR(m_pOnProcessEvent,             "ProcessEvent");
    GET_SCRIPT_HOOK_PTR(m_pOnEffectDummyCreature,      "EffectDummyCreature");
    GET_SCRIPT_HOOK_PTR(m_pOnEffectDummyGO,            "EffectDummyGameObject");
    GET_SCRIPT_HOOK_PTR(m_pOnEffectDummyItem,          "EffectDummyItem");
    GET_SCRIPT_HOOK_PTR(m_pOnAuraDummy,                "AuraDummy");

#   undef GET_SCRIPT_HOOK_PTR

    if (strcmp(pGetMangosRevStr(), REVISION_NR) != 0)
    {
        m_pOnFreeScriptLibrary = NULL;                      // prevent call before init
        UnloadScriptLibrary();
        return SCRIPT_LOAD_ERR_OUTDATED;
    }

    m_pOnInitScriptLibrary();
    return SCRIPT_LOAD_OK;
}

void ScriptMgr::UnloadScriptLibrary()
{
    if (!m_hScriptLib)
        return;

    if (m_pOnFreeScriptLibrary)
        m_pOnFreeScriptLibrary();

    MANGOS_CLOSE_LIBRARY(m_hScriptLib);
    m_hScriptLib = NULL;

    m_pOnInitScriptLibrary      = NULL;
    m_pOnFreeScriptLibrary      = NULL;
    m_pGetScriptLibraryVersion  = NULL;

    m_pGetCreatureAI            = NULL;
    m_pCreateInstanceData       = NULL;

    m_pOnGossipHello            = NULL;
    m_pOnGOGossipHello          = NULL;
    m_pOnGossipSelect           = NULL;
    m_pOnGOGossipSelect         = NULL;
    m_pOnGossipSelectWithCode   = NULL;
    m_pOnGOGossipSelectWithCode = NULL;
    m_pOnQuestAccept            = NULL;
    m_pOnGOQuestAccept          = NULL;
    m_pOnItemQuestAccept        = NULL;
    m_pOnQuestRewarded          = NULL;
    m_pOnGOQuestRewarded        = NULL;
    m_pGetNPCDialogStatus       = NULL;
    m_pGetGODialogStatus        = NULL;
    m_pOnGOUse                  = NULL;
    m_pOnItemUse                = NULL;
    m_pOnAreaTrigger            = NULL;
    m_pOnProcessEvent           = NULL;
    m_pOnEffectDummyCreature    = NULL;
    m_pOnEffectDummyGO          = NULL;
    m_pOnEffectDummyItem        = NULL;
    m_pOnAuraDummy              = NULL;
}

uint32 GetAreaTriggerScriptId(uint32 triggerId)
{
    return sScriptMgr.GetAreaTriggerScriptId(triggerId);
}

uint32 GetEventIdScriptId(uint32 eventId)
{
    return sScriptMgr.GetEventIdScriptId(eventId);
}

uint32 GetScriptId(const char* name)
{
    return sScriptMgr.GetScriptId(name);
}

char const* GetScriptName(uint32 id)
{
    return sScriptMgr.GetScriptName(id);
}

uint32 GetScriptIdsCount()
{
    return sScriptMgr.GetScriptIdsCount();
}

