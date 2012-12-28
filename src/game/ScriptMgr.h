/*
 * Copyright (C) 2005-2013 MaNGOS <http://getmangos.com/>
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

#ifndef _SCRIPTMGR_H
#define _SCRIPTMGR_H

#include "Common.h"
#include "Policies/Singleton.h"
#include "ObjectGuid.h"
#include "DBCEnums.h"
#include "ace/Atomic_Op.h"

struct AreaTriggerEntry;
struct SpellEntry;
class Aura;
class Creature;
class CreatureAI;
class GameObject;
class InstanceData;
class Item;
class Map;
class Object;
class Player;
class Quest;
class SpellCastTargets;
class Unit;
class WorldObject;

enum ScriptCommand                                          // resSource, resTarget are the resulting Source/ Target after buddy search is done
{
    SCRIPT_COMMAND_TALK                     = 0,            // resSource = WorldObject, resTarget = Unit/none
    // datalong1 (see enum ChatType for supported CHAT_TYPE_'s), datalong2 = language
    // dataint = text entry from db_script_string -table. dataint2-4 optional for random selected texts.
    SCRIPT_COMMAND_EMOTE                    = 1,            // resSource = Unit, resTarget = Unit/none
    // datalong1 = emote_id
    SCRIPT_COMMAND_FIELD_SET                = 2,            // source = any, datalong = field_id, datalong2 = value
    SCRIPT_COMMAND_MOVE_TO                  = 3,            // resSource = Creature, datalong2 = travel_speed*100, x/y/z
    // data_flags & SCRIPT_FLAG_COMMAND_ADDITIONAL: teleport unit to position
    SCRIPT_COMMAND_FLAG_SET                 = 4,            // source = any, datalong = field_id, datalong2 = bitmask
    SCRIPT_COMMAND_FLAG_REMOVE              = 5,            // source = any, datalong = field_id, datalong2 = bitmask
    SCRIPT_COMMAND_TELEPORT_TO              = 6,            // source or target with Player, datalong2 = map_id, x/y/z
    SCRIPT_COMMAND_QUEST_EXPLORED           = 7,            // one from source or target must be Player, another GO/Creature, datalong=quest_id, datalong2=distance or 0
    SCRIPT_COMMAND_KILL_CREDIT              = 8,            // source or target with Player, datalong = creature entry (or 0 for target-entry), datalong2 = bool (0=personal credit, 1=group credit)
    SCRIPT_COMMAND_RESPAWN_GAMEOBJECT       = 9,            // source = any, datalong=db_guid, datalong2=despawn_delay
    SCRIPT_COMMAND_TEMP_SUMMON_CREATURE     = 10,           // source = any, datalong=creature entry, datalong2=despawn_delay
    // data_flags & SCRIPT_FLAG_COMMAND_ADDITIONAL = summon active
    SCRIPT_COMMAND_OPEN_DOOR                = 11,           // datalong=db_guid (or not provided), datalong2=reset_delay
    SCRIPT_COMMAND_CLOSE_DOOR               = 12,           // datalong=db_guid (or not provided), datalong2=reset_delay
    SCRIPT_COMMAND_ACTIVATE_OBJECT          = 13,           // source = unit, target=GO
    SCRIPT_COMMAND_REMOVE_AURA              = 14,           // resSource = Unit, datalong = spell_id
    SCRIPT_COMMAND_CAST_SPELL               = 15,           // resSource = Unit, cast spell at resTarget = Unit
    // datalong=spellid
    // data_flags &  SCRIPT_FLAG_COMMAND_ADDITIONAL = cast triggered
    SCRIPT_COMMAND_PLAY_SOUND               = 16,           // resSource = WorldObject, target=any/player, datalong (sound_id), datalong2 (bitmask: 0/1=anyone/target, 0/2=with distance dependent, so 1|2 = 3 is target with distance dependent)
    SCRIPT_COMMAND_CREATE_ITEM              = 17,           // source or target must be player, datalong = item entry, datalong2 = amount
    SCRIPT_COMMAND_DESPAWN_SELF             = 18,           // resSource = Creature, datalong = despawn delay
    SCRIPT_COMMAND_PLAY_MOVIE               = 19,           // target can only be a player, datalog = movie id
    SCRIPT_COMMAND_MOVEMENT                 = 20,           // resSource = Creature. datalong = MovementType (0:idle, 1:random or 2:waypoint), datalong2 = wander-distance
    // data_flags &  SCRIPT_FLAG_COMMAND_ADDITIONAL = Random-movement around current position
    SCRIPT_COMMAND_SET_ACTIVEOBJECT         = 21,           // resSource = Creature
    // datalong=bool 0=off, 1=on
    SCRIPT_COMMAND_SET_FACTION              = 22,           // resSource = Creature
    // datalong=factionId, datalong2=faction_flags
    SCRIPT_COMMAND_MORPH_TO_ENTRY_OR_MODEL  = 23,           // resSource = Creature, datalong=creature entry/modelid
    // data_flags &  SCRIPT_FLAG_COMMAND_ADDITIONAL = use datalong value as modelid explicit
    SCRIPT_COMMAND_MOUNT_TO_ENTRY_OR_MODEL  = 24,           // resSource = Creature, datalong=creature entry/modelid
    // data_flags &  SCRIPT_FLAG_COMMAND_ADDITIONAL = use datalong value as modelid explicit
    SCRIPT_COMMAND_SET_RUN                  = 25,           // resSource = Creature
    // datalong= bool 0=off, 1=on
    SCRIPT_COMMAND_ATTACK_START             = 26,           // resSource = Creature, resTarget = Unit
    SCRIPT_COMMAND_GO_LOCK_STATE            = 27,           // resSource = GameObject
    // datalong= 1=lock, 2=unlock, 4=set not-interactable, 8=set interactable
    SCRIPT_COMMAND_STAND_STATE              = 28,           // resSource = Creature
    // datalong = stand state (enum UnitStandStateType)
    SCRIPT_COMMAND_MODIFY_NPC_FLAGS         = 29,           // resSource = Creature
    // datalong=NPCFlags
    // datalong2:0x00=toggle, 0x01=add, 0x02=remove
    SCRIPT_COMMAND_SEND_TAXI_PATH           = 30,           // datalong = taxi path id (source or target must be player)
};

#define MAX_TEXT_ID 4                                       // used for SCRIPT_COMMAND_TALK

enum ScriptInfoDataFlags
{
    // default: s/b -> t
    SCRIPT_FLAG_BUDDY_AS_TARGET             = 0x01,         // s -> b
    SCRIPT_FLAG_REVERSE_DIRECTION           = 0x02,         // t* -> s* (* result after previous flag is evaluated)
    SCRIPT_FLAG_SOURCE_TARGETS_SELF         = 0x04,         // s* -> s* (* result after previous flag is evaluated)
    SCRIPT_FLAG_COMMAND_ADDITIONAL          = 0x08,         // command dependend
};

struct ScriptInfo
{
    uint32 id;
    uint32 delay;
    uint32 command;

    union
    {
        struct                                              // SCRIPT_COMMAND_TALK (0)
        {
            uint32 chatType;                                // datalong
            uint32 language;                                // datalong2
        } talk;

        struct                                              // SCRIPT_COMMAND_EMOTE (1)
        {
            uint32 emoteId;                                 // datalong
            uint32 unused1;                                 // datalong2
        } emote;

        struct                                              // SCRIPT_COMMAND_FIELD_SET (2)
        {
            uint32 fieldId;                                 // datalong
            uint32 fieldValue;                              // datalong2
        } setField;

        struct                                              // SCRIPT_COMMAND_MOVE_TO (3)
        {
            uint32 unused1;                                 // datalong
            uint32 travelSpeed;                             // datalong2
        } moveTo;

        struct                                              // SCRIPT_COMMAND_FLAG_SET (4)
        {
            uint32 fieldId;                                 // datalong
            uint32 fieldValue;                              // datalong2
        } setFlag;

        struct                                              // SCRIPT_COMMAND_FLAG_REMOVE (5)
        {
            uint32 fieldId;                                 // datalong
            uint32 fieldValue;                              // datalong2
        } removeFlag;

        struct                                              // SCRIPT_COMMAND_TELEPORT_TO (6)
        {
            uint32 mapId;                                   // datalong
            uint32 empty;                                   // datalong2
        } teleportTo;

        struct                                              // SCRIPT_COMMAND_QUEST_EXPLORED (7)
        {
            uint32 questId;                                 // datalong
            uint32 distance;                                // datalong2
        } questExplored;

        struct                                              // SCRIPT_COMMAND_KILL_CREDIT (8)
        {
            uint32 creatureEntry;                           // datalong
            uint32 isGroupCredit;                           // datalong2
        } killCredit;

        struct                                              // SCRIPT_COMMAND_RESPAWN_GAMEOBJECT (9)
        {
            uint32 goGuid;                                  // datalong
            uint32 despawnDelay;                            // datalong2
        } respawnGo;

        struct                                              // SCRIPT_COMMAND_TEMP_SUMMON_CREATURE (10)
        {
            uint32 creatureEntry;                           // datalong
            uint32 despawnDelay;                            // datalong2
        } summonCreature;

        // SCRIPT_COMMAND_OPEN_DOOR (11)
        struct                                              // SCRIPT_COMMAND_CLOSE_DOOR (12)
        {
            uint32 goGuid;                                  // datalong
            uint32 resetDelay;                              // datalong2
        } changeDoor;

        struct                                              // SCRIPT_COMMAND_ACTIVATE_OBJECT (13)
        {
            uint32 empty1;                                  // datalong
            uint32 empty2;                                  // datalong;
        } activateObject;

        struct                                              // SCRIPT_COMMAND_REMOVE_AURA (14)
        {
            uint32 spellId;                                 // datalong
            uint32 empty;                                   // datalong2
        } removeAura;

        struct                                              // SCRIPT_COMMAND_CAST_SPELL (15)
        {
            uint32 spellId;                                 // datalong
            uint32 empty;                                   // datalong2
        } castSpell;

        struct                                              // SCRIPT_COMMAND_PLAY_SOUND (16)
        {
            uint32 soundId;                                 // datalong
            uint32 flags;                                   // datalong2
        } playSound;

        struct                                              // SCRIPT_COMMAND_CREATE_ITEM (17)
        {
            uint32 itemEntry;                               // datalong
            uint32 amount;                                  // datalong2
        } createItem;

        struct                                              // SCRIPT_COMMAND_DESPAWN_SELF (18)
        {
            uint32 despawnDelay;                            // datalong
            uint32 empty;                                   // datalong2
        } despawn;

        struct                                              // SCRIPT_COMMAND_PLAY_MOVIE (19)
        {
            uint32 movieId;                                 // datalong
            uint32 empty;                                   // datalong2
        } playMovie;

        struct                                              // SCRIPT_COMMAND_MOVEMENT (20)
        {
            uint32 movementType;                            // datalong
            uint32 wanderDistance;                          // datalong2
        } movement;

        struct                                              // SCRIPT_COMMAND_SET_ACTIVEOBJECT (21)
        {
            uint32 activate;                                // datalong
            uint32 empty;                                   // datalong2
        } activeObject;

        struct                                              // SCRIPT_COMMAND_SET_FACTION (22)
        {
            uint32 factionId;                               // datalong
            uint32 flags;                                   // datalong2
        } faction;

        struct                                              // SCRIPT_COMMAND_MORPH_TO_ENTRY_OR_MODEL (23)
        {
            uint32 creatureOrModelEntry;                    // datalong
            uint32 empty1;                                  // datalong2
        } morph;

        struct                                              // SCRIPT_COMMAND_MOUNT_TO_ENTRY_OR_MODEL (24)
        {
            uint32 creatureOrModelEntry;                    // datalong
            uint32 empty1;                                  // datalong2
        } mount;

        struct                                              // SCRIPT_COMMAND_SET_RUN (25)
        {
            uint32 run;                                     // datalong
            uint32 empty;                                   // datalong2
        } run;

        struct                                              // SCRIPT_COMMAND_ATTACK_START (26)
        {
            uint32 empty1;                                  // datalong
            uint32 empty2;                                  // datalong2
        } attack;

        struct                                              // SCRIPT_COMMAND_GO_LOCK_STATE (27)
        {
            uint32 lockState;                               // datalong
            uint32 empty;                                   // datalong
        } goLockState;

        struct                                              // SCRIPT_COMMAND_STAND_STATE (28)
        {
            uint32 stand_state;                             // datalong
            uint32 unused1;                                 // datalong2
        } standState;

        struct                                              // SCRIPT_COMMAND_MODIFY_NPC_FLAGS (29)
        {
            uint32 flag;                                    // datalong
            uint32 change_flag;                             // datalong2
        } npcFlag;

        struct
        {
            uint32 taxiPathId;                              // datalong
            uint32 empty;
        } sendTaxiPath;

        struct
        {
            uint32 data[2];
        } raw;
    };

    // Buddy system (entry can be npc or go entry, depending on command)
    uint32 buddyEntry;                                      // datalong3 -> buddy_entry
    uint32 searchRadius;                                    // datalong4 -> search_radius
    uint8 data_flags;                                       // data_flags

    int32 textId[MAX_TEXT_ID];                              // dataint to dataint4

    float x;
    float y;
    float z;
    float o;

    // helpers
    uint32 GetGOGuid() const
    {
        switch (command)
        {
            case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT:
                return respawnGo.goGuid;
            case SCRIPT_COMMAND_OPEN_DOOR:
            case SCRIPT_COMMAND_CLOSE_DOOR:
                return changeDoor.goGuid;
            default:
                return 0;
        }
    }

    bool IsCreatureBuddy() const
    {
        switch (command)
        {
            case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT:
            case SCRIPT_COMMAND_OPEN_DOOR:
            case SCRIPT_COMMAND_CLOSE_DOOR:
            case SCRIPT_COMMAND_ACTIVATE_OBJECT:
            case SCRIPT_COMMAND_GO_LOCK_STATE:
                return false;
            default:
                return true;
        }
    }

    bool HasAdditionalScriptFlag() const
    {
        switch (command)
        {
            case SCRIPT_COMMAND_MOVE_TO:
            case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:
            case SCRIPT_COMMAND_CAST_SPELL:
            case SCRIPT_COMMAND_MOVEMENT:
            case SCRIPT_COMMAND_MORPH_TO_ENTRY_OR_MODEL:
            case SCRIPT_COMMAND_MOUNT_TO_ENTRY_OR_MODEL:
                return true;
            default:
                return false;
        }
    }
};

class ScriptAction
{
    public:
        ScriptAction(const char* _table, Map* _map, ObjectGuid _sourceGuid, ObjectGuid _targetGuid, ObjectGuid _ownerGuid, ScriptInfo const* _script) :
            m_table(_table), m_map(_map), m_sourceGuid(_sourceGuid), m_targetGuid(_targetGuid), m_ownerGuid(_ownerGuid), m_script(_script)
        {}

        void HandleScriptStep();

    private:
        const char* m_table;                                // of which table the script was started
        Map* m_map;                                         // Map on which the action will be executed
        ObjectGuid m_sourceGuid;
        ObjectGuid m_targetGuid;
        ObjectGuid m_ownerGuid;                             // owner of source if source is item
        ScriptInfo const* m_script;                         // pointer to static script data

        // Helper functions
        bool GetScriptCommandObject(const ObjectGuid guid, bool includeItem, Object*& resultObject);
        bool GetScriptProcessTargets(WorldObject* pOrigSource, WorldObject* pOrigTarget, WorldObject*& pFinalSource, WorldObject*& pFinalTarget);
        bool LogIfNotCreature(WorldObject* pWorldObject);
        bool LogIfNotUnit(WorldObject* pWorldObject);
        bool LogIfNotGameObject(WorldObject* pWorldObject);
        Player* GetPlayerTargetOrSourceAndLog(WorldObject* pSource, WorldObject* pTarget);
};

typedef std::multimap < uint32 /*delay*/, ScriptInfo > ScriptMap;
typedef std::map < uint32 /*id*/, ScriptMap > ScriptMapMap;
typedef std::pair<const char*, ScriptMapMap> ScriptMapMapName;

extern ScriptMapMapName sQuestEndScripts;
extern ScriptMapMapName sQuestStartScripts;
extern ScriptMapMapName sSpellScripts;
extern ScriptMapMapName sGameObjectScripts;
extern ScriptMapMapName sGameObjectTemplateScripts;
extern ScriptMapMapName sEventScripts;
extern ScriptMapMapName sGossipScripts;
extern ScriptMapMapName sCreatureMovementScripts;

enum ScriptLoadResult
{
    SCRIPT_LOAD_OK,
    SCRIPT_LOAD_ERR_NOT_FOUND,
    SCRIPT_LOAD_ERR_WRONG_API,
    SCRIPT_LOAD_ERR_OUTDATED,
};

class ScriptMgr
{
    public:
        ScriptMgr();
        ~ScriptMgr();

        void LoadGameObjectScripts();
        void LoadGameObjectTemplateScripts();
        void LoadQuestEndScripts();
        void LoadQuestStartScripts();
        void LoadEventScripts();
        void LoadSpellScripts();
        void LoadGossipScripts();
        void LoadCreatureMovementScripts();

        void LoadDbScriptStrings();

        void LoadScriptNames();
        void LoadAreaTriggerScripts();
        void LoadEventIdScripts();

        uint32 GetAreaTriggerScriptId(uint32 triggerId) const;
        uint32 GetEventIdScriptId(uint32 eventId) const;

        const char* GetScriptName(uint32 id) const { return id < m_scriptNames.size() ? m_scriptNames[id].c_str() : ""; }
        uint32 GetScriptId(const char* name) const;
        uint32 GetScriptIdsCount() const { return m_scriptNames.size(); }

        ScriptLoadResult LoadScriptLibrary(const char* libName);
        void UnloadScriptLibrary();
        bool IsScriptLibraryLoaded() const { return m_hScriptLib != NULL; }

        uint32 IncreaseScheduledScriptsCount() { return (uint32)++m_scheduledScripts; }
        uint32 DecreaseScheduledScriptCount() { return (uint32)--m_scheduledScripts; }
        uint32 DecreaseScheduledScriptCount(size_t count) { return (uint32)(m_scheduledScripts -= count); }
        bool IsScriptScheduled() const { return m_scheduledScripts > 0; }
        static bool CanSpellEffectStartDBScript(SpellEntry const* spellinfo, SpellEffectIndex effIdx);

        CreatureAI* GetCreatureAI(Creature* pCreature);
        InstanceData* CreateInstanceData(Map* pMap);

        char const* GetScriptLibraryVersion() const;
        bool OnGossipHello(Player* pPlayer, Creature* pCreature);
        bool OnGossipHello(Player* pPlayer, GameObject* pGameObject);
        bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action, const char* code);
        bool OnGossipSelect(Player* pPlayer, GameObject* pGameObject, uint32 sender, uint32 action, const char* code);
        bool OnQuestAccept(Player* pPlayer, Creature* pCreature, Quest const* pQuest);
        bool OnQuestAccept(Player* pPlayer, GameObject* pGameObject, Quest const* pQuest);
        bool OnQuestAccept(Player* pPlayer, Item* pItem, Quest const* pQuest);
        bool OnQuestRewarded(Player* pPlayer, Creature* pCreature, Quest const* pQuest);
        bool OnQuestRewarded(Player* pPlayer, GameObject* pGameObject, Quest const* pQuest);
        uint32 GetDialogStatus(Player* pPlayer, Creature* pCreature);
        uint32 GetDialogStatus(Player* pPlayer, GameObject* pGameObject);
        bool OnGameObjectUse(Player* pPlayer, GameObject* pGameObject);
        bool OnItemUse(Player* pPlayer, Item* pItem, SpellCastTargets const& targets);
        bool OnAreaTrigger(Player* pPlayer, AreaTriggerEntry const* atEntry);
        bool OnProcessEvent(uint32 eventId, Object* pSource, Object* pTarget, bool isStart);
        bool OnEffectDummy(Unit* pCaster, uint32 spellId, SpellEffectIndex effIndex, Creature* pTarget);
        bool OnEffectDummy(Unit* pCaster, uint32 spellId, SpellEffectIndex effIndex, GameObject* pTarget);
        bool OnEffectDummy(Unit* pCaster, uint32 spellId, SpellEffectIndex effIndex, Item* pTarget);
        bool OnAuraDummy(Aura const* pAura, bool apply);

    private:
        void LoadScripts(ScriptMapMapName& scripts, const char* tablename);
        void CheckScriptTexts(ScriptMapMapName const& scripts, std::set<int32>& ids);

        template<class T>
        void GetScriptHookPtr(T& ptr, const char* name)
        {
            ptr = (T)MANGOS_GET_PROC_ADDR(m_hScriptLib, name);
        }

        typedef std::vector<std::string> ScriptNameMap;
        typedef UNORDERED_MAP<uint32, uint32> AreaTriggerScriptMap;
        typedef UNORDERED_MAP<uint32, uint32> EventIdScriptMap;

        AreaTriggerScriptMap    m_AreaTriggerScripts;
        EventIdScriptMap        m_EventIdScripts;

        ScriptNameMap           m_scriptNames;
        MANGOS_LIBRARY_HANDLE   m_hScriptLib;

        // atomic op counter for active scripts amount
        ACE_Atomic_Op<ACE_Thread_Mutex, long> m_scheduledScripts;

        void (MANGOS_IMPORT* m_pOnInitScriptLibrary)();
        void (MANGOS_IMPORT* m_pOnFreeScriptLibrary)();
        const char* (MANGOS_IMPORT* m_pGetScriptLibraryVersion)();

        CreatureAI* (MANGOS_IMPORT* m_pGetCreatureAI)(Creature*);
        InstanceData* (MANGOS_IMPORT* m_pCreateInstanceData)(Map*);

        bool (MANGOS_IMPORT* m_pOnGossipHello)(Player*, Creature*);
        bool (MANGOS_IMPORT* m_pOnGOGossipHello)(Player*, GameObject*);
        bool (MANGOS_IMPORT* m_pOnGossipSelect)(Player*, Creature*, uint32, uint32);
        bool (MANGOS_IMPORT* m_pOnGOGossipSelect)(Player*, GameObject*, uint32, uint32);
        bool (MANGOS_IMPORT* m_pOnGossipSelectWithCode)(Player*, Creature*, uint32, uint32, const char*);
        bool (MANGOS_IMPORT* m_pOnGOGossipSelectWithCode)(Player*, GameObject*, uint32, uint32, const char*);
        bool (MANGOS_IMPORT* m_pOnQuestAccept)(Player*, Creature*, Quest const*);
        bool (MANGOS_IMPORT* m_pOnGOQuestAccept)(Player*, GameObject*, Quest const*);
        bool (MANGOS_IMPORT* m_pOnItemQuestAccept)(Player*, Item*, Quest const*);
        bool (MANGOS_IMPORT* m_pOnQuestRewarded)(Player*, Creature*, Quest const*);
        bool (MANGOS_IMPORT* m_pOnGOQuestRewarded)(Player*, GameObject*, Quest const*);
        uint32(MANGOS_IMPORT* m_pGetNPCDialogStatus)(Player*, Creature*);
        uint32(MANGOS_IMPORT* m_pGetGODialogStatus)(Player*, GameObject*);
        bool (MANGOS_IMPORT* m_pOnGOUse)(Player*, GameObject*);
        bool (MANGOS_IMPORT* m_pOnItemUse)(Player*, Item*, SpellCastTargets const&);
        bool (MANGOS_IMPORT* m_pOnAreaTrigger)(Player*, AreaTriggerEntry const*);
        bool (MANGOS_IMPORT* m_pOnProcessEvent)(uint32, Object*, Object*, bool);
        bool (MANGOS_IMPORT* m_pOnEffectDummyCreature)(Unit*, uint32, SpellEffectIndex, Creature*);
        bool (MANGOS_IMPORT* m_pOnEffectDummyGO)(Unit*, uint32, SpellEffectIndex, GameObject*);
        bool (MANGOS_IMPORT* m_pOnEffectDummyItem)(Unit*, uint32, SpellEffectIndex, Item*);
        bool (MANGOS_IMPORT* m_pOnAuraDummy)(Aura const*, bool);
};

#define sScriptMgr MaNGOS::Singleton<ScriptMgr>::Instance()

MANGOS_DLL_SPEC uint32 GetAreaTriggerScriptId(uint32 triggerId);
MANGOS_DLL_SPEC uint32 GetEventIdScriptId(uint32 eventId);
MANGOS_DLL_SPEC uint32 GetScriptId(const char* name);
MANGOS_DLL_SPEC char const* GetScriptName(uint32 id);
MANGOS_DLL_SPEC uint32 GetScriptIdsCount();

#endif
