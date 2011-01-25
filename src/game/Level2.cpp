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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "DBCStores.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "Item.h"
#include "GameObject.h"
#include "Opcodes.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "MapManager.h"
#include "Language.h"
#include "World.h"
#include "GameEventMgr.h"
#include "SpellMgr.h"
#include "PoolManager.h"
#include "AccountMgr.h"
#include "GMTicketMgr.h"
#include "WaypointManager.h"
#include "Util.h"
#include <cctype>
#include <iostream>
#include <fstream>
#include <map>

#include "TargetedMovementGenerator.h"                      // for HandleNpcUnFollowCommand

static uint32 ReputationRankStrIndex[MAX_REPUTATION_RANK] =
{
    LANG_REP_HATED,    LANG_REP_HOSTILE, LANG_REP_UNFRIENDLY, LANG_REP_NEUTRAL,
    LANG_REP_FRIENDLY, LANG_REP_HONORED, LANG_REP_REVERED,    LANG_REP_EXALTED
};

//mute player for some times
bool ChatHandler::HandleMuteCommand(char* args)
{
    char* nameStr = ExtractOptNotLastArg(&args);

    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&nameStr, &target, &target_guid, &target_name))
        return false;

    uint32 notspeaktime;
    if (!ExtractUInt32(&args, notspeaktime))
        return false;

    uint32 account_id = target ? target->GetSession()->GetAccountId() : sObjectMgr.GetPlayerAccountIdByGUID(target_guid);

    // find only player from same account if any
    if (!target)
    {
        if (WorldSession* session = sWorld.FindSession(account_id))
            target = session->GetPlayer();
    }

    // must have strong lesser security level
    if (HasLowerSecurity(target, target_guid, true))
        return false;

    time_t mutetime = time(NULL) + notspeaktime*60;

    if (target)
        target->GetSession()->m_muteTime = mutetime;

    LoginDatabase.PExecute("UPDATE account SET mutetime = " UI64FMTD " WHERE id = '%u'", uint64(mutetime), account_id);

    if (target)
        ChatHandler(target).PSendSysMessage(LANG_YOUR_CHAT_DISABLED, notspeaktime);

    std::string nameLink = playerLink(target_name);

    PSendSysMessage(LANG_YOU_DISABLE_CHAT, nameLink.c_str(), notspeaktime);
    return true;
}

//unmute player
bool ChatHandler::HandleUnmuteCommand(char* args)
{
    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&args, &target, &target_guid, &target_name))
        return false;

    uint32 account_id = target ? target->GetSession()->GetAccountId() : sObjectMgr.GetPlayerAccountIdByGUID(target_guid);

    // find only player from same account if any
    if (!target)
    {
        if (WorldSession* session = sWorld.FindSession(account_id))
            target = session->GetPlayer();
    }

    // must have strong lesser security level
    if (HasLowerSecurity(target, target_guid, true))
        return false;

    if (target)
    {
        if (target->CanSpeak())
        {
            SendSysMessage(LANG_CHAT_ALREADY_ENABLED);
            SetSentErrorMessage(true);
            return false;
        }

        target->GetSession()->m_muteTime = 0;
    }

    LoginDatabase.PExecute("UPDATE account SET mutetime = '0' WHERE id = '%u'", account_id);

    if (target)
        ChatHandler(target).PSendSysMessage(LANG_YOUR_CHAT_ENABLED);

    std::string nameLink = playerLink(target_name);

    PSendSysMessage(LANG_YOU_ENABLE_CHAT, nameLink.c_str());
    return true;
}

void ChatHandler::ShowTriggerTargetListHelper( uint32 id, AreaTrigger const* at, bool subpart /*= false*/ )
{
    if (m_session)
    {
        char dist_buf[50];
        if(!subpart)
        {
            float dist = m_session->GetPlayer()->GetDistance2d(at->target_X, at->target_Y);
            snprintf(dist_buf, 50, GetMangosString(LANG_TRIGGER_DIST), dist);
        }
        else
            dist_buf[0] = '\0';

        PSendSysMessage(LANG_TRIGGER_TARGET_LIST_CHAT,
            subpart ? " -> " : "", id, id, at->target_mapId, at->target_X, at->target_Y, at->target_Z, dist_buf);
    }
    else
        PSendSysMessage(LANG_TRIGGER_TARGET_LIST_CONSOLE,
            subpart ? " -> " : "", id, at->target_mapId, at->target_X, at->target_Y, at->target_Z);
}

void ChatHandler::ShowTriggerListHelper( AreaTriggerEntry const * atEntry )
{

    char const* tavern = sObjectMgr.IsTavernAreaTrigger(atEntry->id) ? GetMangosString(LANG_TRIGGER_TAVERN) : "";
    char const* quest = sObjectMgr.GetQuestForAreaTrigger(atEntry->id) ? GetMangosString(LANG_TRIGGER_QUEST) : "";

    if (m_session)
    {
        float dist = m_session->GetPlayer()->GetDistance2d(atEntry->x, atEntry->y);
        char dist_buf[50];
        snprintf(dist_buf, 50, GetMangosString(LANG_TRIGGER_DIST), dist);

        PSendSysMessage(LANG_TRIGGER_LIST_CHAT,
            atEntry->id, atEntry->id, atEntry->mapid, atEntry->x, atEntry->y, atEntry->z, dist_buf, tavern, quest);
    }
    else
        PSendSysMessage(LANG_TRIGGER_LIST_CONSOLE,
            atEntry->id, atEntry->mapid, atEntry->x, atEntry->y, atEntry->z, tavern, quest);

    if (AreaTrigger const* at = sObjectMgr.GetAreaTrigger(atEntry->id))
        ShowTriggerTargetListHelper(atEntry->id, at, true);
}

bool ChatHandler::HandleTriggerCommand(char* args)
{
    AreaTriggerEntry const* atEntry = NULL;

    Player* pl = m_session ? m_session->GetPlayer() : NULL;

    // select by args
    if (*args)
    {
        uint32 atId;
        if (!ExtractUint32KeyFromLink(&args, "Hareatrigger", atId))
            return false;

        if (!atId)
            return false;

        atEntry = sAreaTriggerStore.LookupEntry(atId);

        if (!atEntry)
        {
            PSendSysMessage(LANG_COMMAND_GOAREATRNOTFOUND, atId);
            SetSentErrorMessage(true);
            return false;
        }
    }
    // find nearest
    else
    {
        if (!m_session)
            return false;

        float dist2 = MAP_SIZE*MAP_SIZE;

        Player* pl = m_session->GetPlayer();

        // Search triggers
        for (uint32 id = 0; id < sAreaTriggerStore.GetNumRows (); ++id)
        {
            AreaTriggerEntry const *atTestEntry = sAreaTriggerStore.LookupEntry (id);
            if (!atTestEntry)
                continue;

            if (atTestEntry->mapid != m_session->GetPlayer()->GetMapId())
                continue;

            float dx = atTestEntry->x - pl->GetPositionX();
            float dy = atTestEntry->y - pl->GetPositionY();

            float test_dist2 = dx*dx + dy*dy;

            if (test_dist2 >= dist2)
                continue;

            dist2 = test_dist2;
            atEntry = atTestEntry;
        }

        if (!atEntry)
        {
            SendSysMessage(LANG_COMMAND_NOTRIGGERFOUND);
            SetSentErrorMessage(true);
            return false;
        }
    }

    ShowTriggerListHelper(atEntry);

    int loc_idx = GetSessionDbLocaleIndex();

    AreaTrigger const* at = sObjectMgr.GetAreaTrigger(atEntry->id);
    if (at)
        PSendSysMessage(LANG_TRIGGER_REQ_LEVEL, at->requiredLevel);

    if (uint32 quest_id = sObjectMgr.GetQuestForAreaTrigger(atEntry->id))
    {
        SendSysMessage(LANG_TRIGGER_EXPLORE_QUEST);
        ShowQuestListHelper(quest_id, loc_idx, pl);
    }

    if (at)
    {
        if (at->requiredItem || at->requiredItem2)
        {
            SendSysMessage(LANG_TRIGGER_REQ_ITEMS);

            if (at->requiredItem)
                ShowItemListHelper(at->requiredItem, loc_idx, pl);
            if (at->requiredItem2)
                ShowItemListHelper(at->requiredItem2, loc_idx, pl);
        }

        if (at->requiredQuest)
        {
            SendSysMessage(LANG_TRIGGER_REQ_QUEST_NORMAL);
            ShowQuestListHelper(at->requiredQuest, loc_idx, pl);
        }

        if (at->heroicKey || at->heroicKey2)
        {
            SendSysMessage(LANG_TRIGGER_REQ_KEYS_HEROIC);

            if (at->heroicKey)
                ShowItemListHelper(at->heroicKey, loc_idx, pl);
            if (at->heroicKey2)
                ShowItemListHelper(at->heroicKey2, loc_idx, pl);
        }

        if (at->requiredQuestHeroic)
        {
            SendSysMessage(LANG_TRIGGER_REQ_QUEST_HEROIC);
            ShowQuestListHelper(at->requiredQuestHeroic, loc_idx, pl);
        }
    }

    return true;
}

bool ChatHandler::HandleTriggerActiveCommand(char* /*args*/)
{
    uint32 counter = 0;                                     // Counter for figure out that we found smth.

    Player* pl = m_session->GetPlayer();

    // Search in AreaTable.dbc
    for (uint32 id = 0; id < sAreaTriggerStore.GetNumRows (); ++id)
    {
        AreaTriggerEntry const *atEntry = sAreaTriggerStore.LookupEntry (id);
        if (!atEntry)
            continue;

        if (!IsPointInAreaTriggerZone(atEntry, pl->GetMapId(), pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ()))
            continue;

        ShowTriggerListHelper(atEntry);

        ++counter;
    }

    if (counter == 0)                                      // if counter == 0 then we found nth
        SendSysMessage (LANG_COMMAND_NOTRIGGERFOUND);

    return true;
}

bool ChatHandler::HandleTriggerNearCommand(char* args)
{
    float distance = (!*args) ? 10.0f : (float)atof(args);
    float dist2 =  distance*distance;
    uint32 counter = 0;                                     // Counter for figure out that we found smth.

    Player* pl = m_session->GetPlayer();

    // Search triggers
    for (uint32 id = 0; id < sAreaTriggerStore.GetNumRows (); ++id)
    {
        AreaTriggerEntry const *atEntry = sAreaTriggerStore.LookupEntry (id);
        if (!atEntry)
            continue;

        if (atEntry->mapid != m_session->GetPlayer()->GetMapId())
            continue;

        float dx = atEntry->x - pl->GetPositionX();
        float dy = atEntry->y - pl->GetPositionY();

        if (dx*dx + dy*dy > dist2)
            continue;

        ShowTriggerListHelper(atEntry);

        ++counter;
    }

    // Search trigger targets
    for (uint32 id = 0; id < sAreaTriggerStore.GetNumRows (); ++id)
    {
        AreaTriggerEntry const *atEntry = sAreaTriggerStore.LookupEntry (id);
        if (!atEntry)
            continue;

        AreaTrigger const* at = sObjectMgr.GetAreaTrigger(atEntry->id);
        if (!at)
            continue;

        if (at->target_mapId != m_session->GetPlayer()->GetMapId())
            continue;

        float dx = at->target_X - pl->GetPositionX();
        float dy = at->target_Y - pl->GetPositionY();

        if (dx*dx + dy*dy > dist2)
            continue;

        ShowTriggerTargetListHelper(atEntry->id, at);

        ++counter;
    }

    if (counter == 0)                                      // if counter == 0 then we found nth
        SendSysMessage (LANG_COMMAND_NOTRIGGERFOUND);

    return true;
}

static char const* const areatriggerKeys[] =
{
    "Hareatrigger",
    "Hareatrigger_target",
    NULL
};

bool ChatHandler::HandleGoTriggerCommand(char* args)
{
    Player* _player = m_session->GetPlayer();

    if (!*args)
        return false;

    char *atIdStr = ExtractKeyFromLink(&args, areatriggerKeys);
    if (!atIdStr)
        return false;

    uint32 atId;
    if (!ExtractUInt32(&atIdStr, atId))
        return false;

    if (!atId)
        return false;

    AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(atId);
    if (!atEntry)
    {
        PSendSysMessage(LANG_COMMAND_GOAREATRNOTFOUND, atId);
        SetSentErrorMessage(true);
        return false;
    }

    bool to_target = ExtractLiteralArg(&args, "target");
    if (!to_target && *args)                                // can be fail also at syntax error
        return false;

    if (to_target)
    {
        AreaTrigger const* at = sObjectMgr.GetAreaTrigger(atId);
        if (!at)
        {
            PSendSysMessage(LANG_AREATRIGER_NOT_HAS_TARGET, atId);
            SetSentErrorMessage(true);
            return false;
        }

        return HandleGoHelper(_player, at->target_mapId, at->target_X, at->target_Y, &at->target_Z);
    }
    else
        return HandleGoHelper(_player, atEntry->mapid, atEntry->x, atEntry->y, &atEntry->z);
}

bool ChatHandler::HandleGoGraveyardCommand(char* args)
{
    Player* _player = m_session->GetPlayer();

    uint32 gyId;
    if (!ExtractUInt32(&args, gyId))
        return false;

    WorldSafeLocsEntry const* gy = sWorldSafeLocsStore.LookupEntry(gyId);
    if (!gy)
    {
        PSendSysMessage(LANG_COMMAND_GRAVEYARDNOEXIST, gyId);
        SetSentErrorMessage(true);
        return false;
    }

    return HandleGoHelper(_player, gy->map_id, gy->x, gy->y, &gy->z);
}

enum CreatureLinkType
{
    CREATURE_LINK_RAW               =-1,                    // non-link case
    CREATURE_LINK_GUID              = 0,
    CREATURE_LINK_ENTRY             = 1,
};

static char const* const creatureKeys[] =
{
    "Hcreature",
    "Hcreature_entry",
    NULL
};

/** \brief Teleport the GM to the specified creature
*
* .go creature <GUID>     --> TP using creature.guid
* .go creature azuregos   --> TP player to the mob with this name
*                             Warning: If there is more than one mob with this name
*                                      you will be teleported to the first one that is found.
* .go creature id 6109    --> TP player to the mob, that has this creature_template.entry
*                             Warning: If there is more than one mob with this "id"
*                                      you will be teleported to the first one that is found.
*/
//teleport to creature
bool ChatHandler::HandleGoCreatureCommand(char* args)
{
    if (!*args)
        return false;

    Player* _player = m_session->GetPlayer();

    // "id" or number or [name] Shift-click form |color|Hcreature:creature_id|h[name]|h|r
    int crType;
    char* pParam1 = ExtractKeyFromLink(&args, creatureKeys, &crType);
    if (!pParam1)
        return false;

    // User wants to teleport to the NPC's template entry
    if (crType == CREATURE_LINK_RAW && strcmp(pParam1, "id") == 0)
    {
        // number or [name] Shift-click form |color|Hcreature_entry:creature_id|h[name]|h|r
        pParam1 = ExtractKeyFromLink(&args, "Hcreature_entry");
        if (!pParam1)
            return false;

        crType = CREATURE_LINK_ENTRY;
    }

    CreatureData const* data = NULL;

    switch(crType)
    {
        case CREATURE_LINK_ENTRY:
        {
            uint32 tEntry;
            if (!ExtractUInt32(&pParam1, tEntry))
                return false;

            if (!tEntry)
                return false;

            if (!ObjectMgr::GetCreatureTemplate(tEntry))
            {
                SendSysMessage(LANG_COMMAND_GOCREATNOTFOUND);
                SetSentErrorMessage(true);
                return false;
            }

            FindCreatureData worker(tEntry, m_session ? m_session->GetPlayer() : NULL);

            sObjectMgr.DoCreatureData(worker);

            CreatureDataPair const* dataPair = worker.GetResult();
            if (!dataPair)
            {
                SendSysMessage(LANG_COMMAND_GOCREATNOTFOUND);
                SetSentErrorMessage(true);
                return false;
            }

            data = &dataPair->second;
            break;
        }
        case CREATURE_LINK_GUID:
        {
            uint32 lowguid;
            if (!ExtractUInt32(&pParam1, lowguid))
                return false;

            data = sObjectMgr.GetCreatureData(lowguid);
            if (!data)
            {
                SendSysMessage(LANG_COMMAND_GOCREATNOTFOUND);
                SetSentErrorMessage(true);
                return false;
            }
            break;
        }
        case CREATURE_LINK_RAW:
        {
            uint32 lowguid;
            if (ExtractUInt32(&pParam1, lowguid))
            {
                data = sObjectMgr.GetCreatureData(lowguid);
                if (!data)
                {
                    SendSysMessage(LANG_COMMAND_GOCREATNOTFOUND);
                    SetSentErrorMessage(true);
                    return false;
                }
            }
            // Number is invalid - maybe the user specified the mob's name
            else
            {
                std::string name = pParam1;
                WorldDatabase.escape_string(name);
                QueryResult *result = WorldDatabase.PQuery("SELECT guid FROM creature, creature_template WHERE creature.id = creature_template.entry AND creature_template.name "_LIKE_" "_CONCAT3_("'%%'","'%s'","'%%'"), name.c_str());
                if (!result)
                {
                    SendSysMessage(LANG_COMMAND_GOCREATNOTFOUND);
                    SetSentErrorMessage(true);
                    return false;
                }

                FindCreatureData worker(0, m_session ? m_session->GetPlayer() : NULL);

                do {
                    Field *fields = result->Fetch();
                    uint32 guid = fields[0].GetUInt32();

                    CreatureDataPair const* cr_data = sObjectMgr.GetCreatureDataPair(guid);
                    if (!cr_data)
                        continue;

                    worker(*cr_data);

                } while (result->NextRow());

                delete result;

                CreatureDataPair const* dataPair = worker.GetResult();
                if (!dataPair)
                {
                    SendSysMessage(LANG_COMMAND_GOCREATNOTFOUND);
                    SetSentErrorMessage(true);
                    return false;
                }

                data = &dataPair->second;
            }
            break;
        }
    }

    return HandleGoHelper(_player, data->mapid, data->posX, data->posY, &data->posZ);
}

enum GameobjectLinkType
{
    GAMEOBJECT_LINK_RAW             =-1,                    // non-link case
    GAMEOBJECT_LINK_GUID            = 0,
    GAMEOBJECT_LINK_ENTRY           = 1,
};

static char const* const gameobjectKeys[] =
{
    "Hgameobject",
    "Hgameobject_entry",
    NULL
};

//teleport to gameobject
bool ChatHandler::HandleGoObjectCommand(char* args)
{
    Player* _player = m_session->GetPlayer();

    // number or [name] Shift-click form |color|Hgameobject:go_guid|h[name]|h|r
    int goType;
    char* pParam1 = ExtractKeyFromLink(&args, gameobjectKeys, &goType);
    if (!pParam1)
        return false;

    // User wants to teleport to the GO's template entry
    if (goType == GAMEOBJECT_LINK_RAW && strcmp(pParam1, "id") == 0)
    {
        // number or [name] Shift-click form |color|Hgameobject_entry:creature_id|h[name]|h|r
        pParam1 = ExtractKeyFromLink(&args, "Hgameobject_entry");
        if (!pParam1)
            return false;

        goType = GAMEOBJECT_LINK_ENTRY;
    }

    GameObjectData const* data = NULL;

    switch(goType)
    {
        case CREATURE_LINK_ENTRY:
        {
            uint32 tEntry;
            if (!ExtractUInt32(&pParam1, tEntry))
                return false;

            if (!tEntry)
                return false;

            if (!ObjectMgr::GetGameObjectInfo(tEntry))
            {
                SendSysMessage(LANG_COMMAND_GOOBJNOTFOUND);
                SetSentErrorMessage(true);
                return false;
            }

            FindGOData worker(tEntry, m_session ? m_session->GetPlayer() : NULL);

            sObjectMgr.DoGOData(worker);

            GameObjectDataPair const* dataPair = worker.GetResult();

            if (!dataPair)
            {
                SendSysMessage(LANG_COMMAND_GOOBJNOTFOUND);
                SetSentErrorMessage(true);
                return false;
            }

            data = &dataPair->second;
            break;
        }
        case GAMEOBJECT_LINK_GUID:
        {
            uint32 lowguid;
            if (!ExtractUInt32(&pParam1, lowguid))
                return false;

            // by DB guid
            data = sObjectMgr.GetGOData(lowguid);
            if (!data)
            {
                SendSysMessage(LANG_COMMAND_GOOBJNOTFOUND);
                SetSentErrorMessage(true);
                return false;
            }
            break;
        }
        case GAMEOBJECT_LINK_RAW:
        {
            uint32 lowguid;
            if (ExtractUInt32(&pParam1, lowguid))
            {
                // by DB guid
                data = sObjectMgr.GetGOData(lowguid);
                if (!data)
                {
                    SendSysMessage(LANG_COMMAND_GOOBJNOTFOUND);
                    SetSentErrorMessage(true);
                    return false;
                }
            }
            else
            {
                std::string name = pParam1;
                WorldDatabase.escape_string(name);
                QueryResult *result = WorldDatabase.PQuery("SELECT guid FROM gameobject, gameobject_template WHERE gameobject.id = gameobject_template.entry AND gameobject_template.name "_LIKE_" "_CONCAT3_("'%%'","'%s'","'%%'"), name.c_str());
                if (!result)
                {
                    SendSysMessage(LANG_COMMAND_GOOBJNOTFOUND);
                    SetSentErrorMessage(true);
                    return false;
                }

                FindGOData worker(0, m_session ? m_session->GetPlayer() : NULL);

                do {
                    Field *fields = result->Fetch();
                    uint32 guid = fields[0].GetUInt32();

                    GameObjectDataPair const* go_data = sObjectMgr.GetGODataPair(guid);
                    if (!go_data)
                        continue;

                    worker(*go_data);

                } while (result->NextRow());

                delete result;

                GameObjectDataPair const* dataPair = worker.GetResult();
                if (!dataPair)
                {
                    SendSysMessage(LANG_COMMAND_GOOBJNOTFOUND);
                    SetSentErrorMessage(true);
                    return false;
                }

                data = &dataPair->second;
            }
            break;
        }
    }

    return HandleGoHelper(_player, data->mapid, data->posX, data->posY, &data->posZ);
}

bool ChatHandler::HandleGameObjectTargetCommand(char* args)
{
    Player* pl = m_session->GetPlayer();
    QueryResult *result;
    GameEventMgr::ActiveEvents const& activeEventsList = sGameEventMgr.GetActiveEventList();
    if (*args)
    {
        // number or [name] Shift-click form |color|Hgameobject_entry:go_id|h[name]|h|r
        char* cId = ExtractKeyFromLink(&args, "Hgameobject_entry");
        if (!cId)
            return false;

        uint32 id;
        if (ExtractUInt32(&cId, id))
        {
            result = WorldDatabase.PQuery("SELECT guid, id, position_x, position_y, position_z, orientation, map, (POW(position_x - '%f', 2) + POW(position_y - '%f', 2) + POW(position_z - '%f', 2)) AS order_ FROM gameobject WHERE map = '%i' AND id = '%u' ORDER BY order_ ASC LIMIT 1",
                pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(), pl->GetMapId(),id);
        }
        else
        {
            std::string name = cId;
            WorldDatabase.escape_string(name);
            result = WorldDatabase.PQuery(
                "SELECT guid, id, position_x, position_y, position_z, orientation, map, (POW(position_x - %f, 2) + POW(position_y - %f, 2) + POW(position_z - %f, 2)) AS order_ "
                "FROM gameobject,gameobject_template WHERE gameobject_template.entry = gameobject.id AND map = %i AND name "_LIKE_" "_CONCAT3_("'%%'","'%s'","'%%'")" ORDER BY order_ ASC LIMIT 1",
                pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(), pl->GetMapId(),name.c_str());
        }
    }
    else
    {
        std::ostringstream eventFilter;
        eventFilter << " AND (event IS NULL ";
        bool initString = true;

        for (GameEventMgr::ActiveEvents::const_iterator itr = activeEventsList.begin(); itr != activeEventsList.end(); ++itr)
        {
            if (initString)
            {
                eventFilter  <<  "OR event IN (" <<*itr;
                initString =false;
            }
            else
                eventFilter << "," << *itr;
        }

        if (!initString)
            eventFilter << "))";
        else
            eventFilter << ")";

        result = WorldDatabase.PQuery("SELECT gameobject.guid, id, position_x, position_y, position_z, orientation, map, "
            "(POW(position_x - %f, 2) + POW(position_y - %f, 2) + POW(position_z - %f, 2)) AS order_ FROM gameobject "
            "LEFT OUTER JOIN game_event_gameobject on gameobject.guid=game_event_gameobject.guid WHERE map = '%i' %s ORDER BY order_ ASC LIMIT 10",
            m_session->GetPlayer()->GetPositionX(), m_session->GetPlayer()->GetPositionY(), m_session->GetPlayer()->GetPositionZ(), m_session->GetPlayer()->GetMapId(),eventFilter.str().c_str());
    }

    if (!result)
    {
        SendSysMessage(LANG_COMMAND_TARGETOBJNOTFOUND);
        return true;
    }

    bool found = false;
    float x, y, z, o;
    uint32 lowguid, id;
    uint16 mapid, pool_id;

    do
    {
        Field *fields = result->Fetch();
        lowguid = fields[0].GetUInt32();
        id =      fields[1].GetUInt32();
        x =       fields[2].GetFloat();
        y =       fields[3].GetFloat();
        z =       fields[4].GetFloat();
        o =       fields[5].GetFloat();
        mapid =   fields[6].GetUInt16();
        pool_id = sPoolMgr.IsPartOfAPool<GameObject>(lowguid);
        if (!pool_id || sPoolMgr.IsSpawnedObject<GameObject>(lowguid))
            found = true;
    } while (result->NextRow() && (!found));

    delete result;

    if (!found)
    {
        PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST,id);
        return false;
    }

    GameObjectInfo const* goI = ObjectMgr::GetGameObjectInfo(id);

    if (!goI)
    {
        PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST,id);
        return false;
    }

    GameObject* target = m_session->GetPlayer()->GetMap()->GetGameObject(ObjectGuid(HIGHGUID_GAMEOBJECT, id, lowguid));

    PSendSysMessage(LANG_GAMEOBJECT_DETAIL, lowguid, goI->name, lowguid, id, x, y, z, mapid, o);

    if (target)
    {
        time_t curRespawnDelay = target->GetRespawnTimeEx()-time(NULL);
        if (curRespawnDelay < 0)
            curRespawnDelay = 0;

        std::string curRespawnDelayStr = secsToTimeString(curRespawnDelay,true);
        std::string defRespawnDelayStr = secsToTimeString(target->GetRespawnDelay(),true);

        PSendSysMessage(LANG_COMMAND_RAWPAWNTIMES, defRespawnDelayStr.c_str(),curRespawnDelayStr.c_str());

        ShowNpcOrGoSpawnInformation<GameObject>(target->GetDBTableGUIDLow());
    }
    return true;
}

//delete object by selection or guid
bool ChatHandler::HandleGameObjectDeleteCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_guid|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
        return false;

    if (!lowguid)
        return false;

    GameObject* obj = NULL;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
        obj = GetObjectGlobalyWithGuidOrNearWithDbGuid(lowguid,go_data->id);

    if (!obj)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    ObjectGuid ownerGuid = obj->GetOwnerGuid();
    if (!ownerGuid.IsEmpty())
    {
        Unit* owner = ObjectAccessor::GetUnit(*m_session->GetPlayer(), ownerGuid);
        if (!owner || !ownerGuid.IsPlayer())
        {
            PSendSysMessage(LANG_COMMAND_DELOBJREFERCREATURE, obj->GetGUIDLow(), ownerGuid.GetString().c_str());
            SetSentErrorMessage(true);
            return false;
        }

        owner->RemoveGameObject(obj,false);
    }

    obj->SetRespawnTime(0);                                 // not save respawn time
    obj->Delete();
    obj->DeleteFromDB();

    PSendSysMessage(LANG_COMMAND_DELOBJMESSAGE, obj->GetGUIDLow());

    return true;
}

//turn selected object
bool ChatHandler::HandleGameObjectTurnCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_id|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
        return false;

    if (!lowguid)
        return false;

    GameObject* obj = NULL;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
        obj = GetObjectGlobalyWithGuidOrNearWithDbGuid(lowguid,go_data->id);

    if (!obj)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    float o;
    if (!ExtractOptFloat(&args, o, m_session->GetPlayer()->GetOrientation()))
        return false;

    Map* map = obj->GetMap();
    map->Remove(obj,false);

    obj->Relocate(obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ(), o);
    obj->UpdateRotationFields();

    map->Add(obj);

    obj->SaveToDB();
    obj->Refresh();

    PSendSysMessage(LANG_COMMAND_TURNOBJMESSAGE, obj->GetGUIDLow(), obj->GetGOInfo()->name, obj->GetGUIDLow());

    return true;
}

//move selected object
bool ChatHandler::HandleGameObjectMoveCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_guid|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
        return false;

    if (!lowguid)
        return false;

    GameObject* obj = NULL;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
        obj = GetObjectGlobalyWithGuidOrNearWithDbGuid(lowguid,go_data->id);

    if (!obj)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    if (!args)
    {
        Player *chr = m_session->GetPlayer();

        Map* map = obj->GetMap();
        map->Remove(obj,false);

        obj->Relocate(chr->GetPositionX(), chr->GetPositionY(), chr->GetPositionZ(), obj->GetOrientation());

        map->Add(obj);
    }
    else
    {
        float x;
        if (!ExtractFloat(&args, x))
            return false;

        float y;
        if (!ExtractFloat(&args, y))
            return false;

        float z;
        if (!ExtractFloat(&args, z))
            return false;

        if (!MapManager::IsValidMapCoord(obj->GetMapId(), x, y, z))
        {
            PSendSysMessage(LANG_INVALID_TARGET_COORD, x, y, obj->GetMapId());
            SetSentErrorMessage(true);
            return false;
        }

        Map* map = obj->GetMap();
        map->Remove(obj,false);

        obj->Relocate(x, y, z, obj->GetOrientation());

        map->Add(obj);
    }

    obj->SaveToDB();
    obj->Refresh();

    PSendSysMessage(LANG_COMMAND_MOVEOBJMESSAGE, obj->GetGUIDLow(), obj->GetGOInfo()->name, obj->GetGUIDLow());

    return true;
}

//spawn go
bool ChatHandler::HandleGameObjectAddCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject_entry:go_id|h[name]|h|r
    uint32 id;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject_entry", id))
        return false;

    if (!id)
        return false;

    int32 spawntimeSecs;
    if (!ExtractOptInt32(&args, spawntimeSecs, 0))
        return false;

    const GameObjectInfo *gInfo = ObjectMgr::GetGameObjectInfo(id);

    if (!gInfo)
    {
        PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST,id);
        SetSentErrorMessage(true);
        return false;
    }

    if (gInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(gInfo->displayId))
    {
        // report to DB errors log as in loading case
        sLog.outErrorDb("Gameobject (Entry %u GoType: %u) have invalid displayId (%u), not spawned.",id, gInfo->type, gInfo->displayId);
        PSendSysMessage(LANG_GAMEOBJECT_HAVE_INVALID_DATA,id);
        SetSentErrorMessage(true);
        return false;
    }

    Player *chr = m_session->GetPlayer();
    float x = float(chr->GetPositionX());
    float y = float(chr->GetPositionY());
    float z = float(chr->GetPositionZ());
    float o = float(chr->GetOrientation());
    Map *map = chr->GetMap();

    GameObject* pGameObj = new GameObject;
    uint32 db_lowGUID = sObjectMgr.GenerateLowGuid(HIGHGUID_GAMEOBJECT);

    if (!pGameObj->Create(db_lowGUID, gInfo->id, map, chr->GetPhaseMaskForSpawn(), x, y, z, o, 0.0f, 0.0f, 0.0f, 0.0f, GO_ANIMPROGRESS_DEFAULT, GO_STATE_READY))
    {
        delete pGameObj;
        return false;
    }

    if (spawntimeSecs)
        pGameObj->SetRespawnTime(spawntimeSecs);

    // fill the gameobject data and save to the db
    pGameObj->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()),chr->GetPhaseMaskForSpawn());

    // this will generate a new guid if the object is in an instance
    if (!pGameObj->LoadFromDB(db_lowGUID, map))
    {
        delete pGameObj;
        return false;
    }

    DEBUG_LOG(GetMangosString(LANG_GAMEOBJECT_CURRENT), gInfo->name, db_lowGUID, x, y, z, o);

    map->Add(pGameObj);

    // TODO: is it really necessary to add both the real and DB table guid here ?
    sObjectMgr.AddGameobjectToGrid(db_lowGUID, sObjectMgr.GetGOData(db_lowGUID));

    PSendSysMessage(LANG_GAMEOBJECT_ADD,id,gInfo->name,db_lowGUID,x,y,z);
    return true;
}

//set pahsemask for selected object
bool ChatHandler::HandleGameObjectPhaseCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_id|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
        return false;

    if (!lowguid)
        return false;

    GameObject* obj = NULL;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
        obj = GetObjectGlobalyWithGuidOrNearWithDbGuid(lowguid,go_data->id);

    if (!obj)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 phasemask;
    if (!ExtractUInt32(&args, phasemask) || !phasemask)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    obj->SetPhaseMask(phasemask,true);
    obj->SaveToDB();
    return true;
}

bool ChatHandler::HandleGameObjectNearCommand(char* args)
{
    float distance;
    if (!ExtractOptFloat(&args, distance, 10.0f))
        return false;

    uint32 count = 0;

    Player* pl = m_session->GetPlayer();
    QueryResult *result = WorldDatabase.PQuery("SELECT guid, id, position_x, position_y, position_z, map, "
        "(POW(position_x - '%f', 2) + POW(position_y - '%f', 2) + POW(position_z - '%f', 2)) AS order_ "
        "FROM gameobject WHERE map='%u' AND (POW(position_x - '%f', 2) + POW(position_y - '%f', 2) + POW(position_z - '%f', 2)) <= '%f' ORDER BY order_",
        pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(),
        pl->GetMapId(), pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(),distance*distance);

    if (result)
    {
        do
        {
            Field *fields = result->Fetch();
            uint32 guid = fields[0].GetUInt32();
            uint32 entry = fields[1].GetUInt32();
            float x = fields[2].GetFloat();
            float y = fields[3].GetFloat();
            float z = fields[4].GetFloat();
            int mapid = fields[5].GetUInt16();

            GameObjectInfo const * gInfo = ObjectMgr::GetGameObjectInfo(entry);

            if (!gInfo)
                continue;

            PSendSysMessage(LANG_GO_MIXED_LIST_CHAT, guid, PrepareStringNpcOrGoSpawnInformation<GameObject>(guid).c_str(), entry, guid, gInfo->name, x, y, z, mapid);

            ++count;
        } while (result->NextRow());

        delete result;
    }

    PSendSysMessage(LANG_COMMAND_NEAROBJMESSAGE,distance,count);
    return true;
}

bool ChatHandler::HandleGUIDCommand(char* /*args*/)
{
    ObjectGuid guid = m_session->GetPlayer()->GetSelectionGuid();

    if (guid.IsEmpty())
    {
        SendSysMessage(LANG_NO_SELECTION);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_OBJECT_GUID, guid.GetString().c_str());
    return true;
}

void ChatHandler::ShowAchievementListHelper(AchievementEntry const * achEntry, LocaleConstant loc, time_t const* date /*= NULL*/, Player* target /*= NULL */ )
{
    std::string name = achEntry->name[loc];

    ObjectGuid guid = target ? target->GetObjectGuid() : ObjectGuid();

    // |color|Hachievement:achievement_id:player_guid_hex:completed_0_1:mm:dd:yy_from_2000:criteriaMask:0:0:0|h[name]|h|r
    std::ostringstream ss;
    if (m_session)
    {
        ss << achEntry->ID << " - |cffffffff|Hachievement:" << achEntry->ID << ":" << std::hex << guid.GetRawValue() << std::dec;
        if (date)
        {
            // complete date
            tm* aTm = localtime(date);
            ss << ":1:" << aTm->tm_mon+1 << ":" << aTm->tm_mday << ":" << (aTm->tm_year+1900-2000) << ":";

            // complete criteria mask (all bits set)
            ss << uint32(-1) << ":" << uint32(-1) << ":" << uint32(-1) << ":" << uint32(-1) << ":";
        }
        else
        {
            // complete date
            ss << ":0:0:0:-1:";

            // complete criteria mask
            if (target)
            {
                uint32 criteriaMask[4] = {0, 0, 0, 0};

                if (AchievementMgr const* mgr = target ? &target->GetAchievementMgr() : NULL)
                    if (AchievementCriteriaEntryList const* criteriaList = sAchievementMgr.GetAchievementCriteriaByAchievement(achEntry->ID))
                        for (AchievementCriteriaEntryList::const_iterator itr = criteriaList->begin(); itr != criteriaList->end(); ++itr)
                            if (mgr->IsCompletedCriteria(*itr, achEntry))
                                criteriaMask[((*itr)->showOrder - 1) / 32] |= (1 << (((*itr)->showOrder - 1) % 32));

                for (int i = 0; i < 4; ++i)
                    ss << criteriaMask[i] << ":";
            }
            else
                ss << "0:0:0:0:";
        }

        ss << "|h[" << name << " " << localeNames[loc] << "]|h|r";
    }
    else
        ss << achEntry->ID << " - " << name << " " << localeNames[loc];

    if (target && date)
        ss << " [" << TimeToTimestampStr(*date) << "]";

    SendSysMessage(ss.str().c_str());
}

bool ChatHandler::HandleLookupAchievementCommand(char* args)
{
    if (!*args)
        return false;

    // Can be NULL at console call
    Player *target = getSelectedPlayer();

    std::string namepart = args;
    std::wstring wnamepart;

    if (!Utf8toWStr(namepart, wnamepart))
        return false;

    // converting string that we try to find to lower case
    wstrToLower(wnamepart);

    uint32 counter = 0;                                     // Counter for figure out that we found smth.

    for (uint32 id = 0; id < sAchievementStore.GetNumRows(); ++id)
    {
        AchievementEntry const *achEntry = sAchievementStore.LookupEntry(id);
        if (!achEntry)
            continue;

        int loc = GetSessionDbcLocale();
        std::string name = achEntry->name[loc];
        if (name.empty())
            continue;

        if (!Utf8FitTo(name, wnamepart))
        {
            loc = 0;
            for(; loc < MAX_LOCALE; ++loc)
            {
                if (loc == GetSessionDbcLocale())
                    continue;

                name = achEntry->name[loc];
                if (name.empty())
                    continue;

                if (Utf8FitTo(name, wnamepart))
                    break;
            }
        }

        if (loc < MAX_LOCALE)
        {
            CompletedAchievementData const* completed = target ? target->GetAchievementMgr().GetCompleteData(id) : NULL;
            ShowAchievementListHelper(achEntry, LocaleConstant(loc), completed ? &completed->date : NULL, target);
            counter++;
        }
    }

    if (counter == 0)                                       // if counter == 0 then we found nth
        SendSysMessage(LANG_COMMAND_ACHIEVEMENT_NOTFOUND);
    return true;
}

bool ChatHandler::HandleCharacterAchievementsCommand(char* args)
{
    Player* target;
    if (!ExtractPlayerTarget(&args, &target))
        return false;

    LocaleConstant loc = GetSessionDbcLocale();

    CompletedAchievementMap const& complitedList = target->GetAchievementMgr().GetCompletedAchievements();
    for(CompletedAchievementMap::const_iterator itr = complitedList.begin(); itr != complitedList.end(); ++itr)
    {
        AchievementEntry const *achEntry = sAchievementStore.LookupEntry(itr->first);
        ShowAchievementListHelper(achEntry, loc, &itr->second.date, target);
    }
    return true;
}

void ChatHandler::ShowFactionListHelper( FactionEntry const * factionEntry, LocaleConstant loc, FactionState const* repState /*= NULL*/, Player * target /*= NULL */ )
{
    std::string name = factionEntry->name[loc];

    // send faction in "id - [faction] rank reputation [visible] [at war] [own team] [unknown] [invisible] [inactive]" format
    // or              "id - [faction] [no reputation]" format
    std::ostringstream ss;
    if (m_session)
        ss << factionEntry->ID << " - |cffffffff|Hfaction:" << factionEntry->ID << "|h[" << name << " " << localeNames[loc] << "]|h|r";
    else
        ss << factionEntry->ID << " - " << name << " " << localeNames[loc];

    if (repState)                               // and then target!=NULL also
    {
        ReputationRank rank = target->GetReputationMgr().GetRank(factionEntry);
        std::string rankName = GetMangosString(ReputationRankStrIndex[rank]);

        ss << " " << rankName << "|h|r (" << target->GetReputationMgr().GetReputation(factionEntry) << ")";

        if (repState->Flags & FACTION_FLAG_VISIBLE)
            ss << GetMangosString(LANG_FACTION_VISIBLE);
        if (repState->Flags & FACTION_FLAG_AT_WAR)
            ss << GetMangosString(LANG_FACTION_ATWAR);
        if (repState->Flags & FACTION_FLAG_PEACE_FORCED)
            ss << GetMangosString(LANG_FACTION_PEACE_FORCED);
        if (repState->Flags & FACTION_FLAG_HIDDEN)
            ss << GetMangosString(LANG_FACTION_HIDDEN);
        if (repState->Flags & FACTION_FLAG_INVISIBLE_FORCED)
            ss << GetMangosString(LANG_FACTION_INVISIBLE_FORCED);
        if (repState->Flags & FACTION_FLAG_INACTIVE)
            ss << GetMangosString(LANG_FACTION_INACTIVE);
    }
    else if (target)
        ss << GetMangosString(LANG_FACTION_NOREPUTATION);

    SendSysMessage(ss.str().c_str());
}

bool ChatHandler::HandleLookupFactionCommand(char* args)
{
    if (!*args)
        return false;

    // Can be NULL at console call
    Player *target = getSelectedPlayer();

    std::string namepart = args;
    std::wstring wnamepart;

    if (!Utf8toWStr(namepart, wnamepart))
        return false;

    // converting string that we try to find to lower case
    wstrToLower(wnamepart);

    uint32 counter = 0;                                     // Counter for figure out that we found smth.

    for (uint32 id = 0; id < sFactionStore.GetNumRows(); ++id)
    {
        FactionEntry const *factionEntry = sFactionStore.LookupEntry(id);
        if (factionEntry)
        {
            int loc = GetSessionDbcLocale();
            std::string name = factionEntry->name[loc];
            if (name.empty())
                continue;

            if (!Utf8FitTo(name, wnamepart))
            {
                loc = 0;
                for(; loc < MAX_LOCALE; ++loc)
                {
                    if (loc == GetSessionDbcLocale())
                        continue;

                    name = factionEntry->name[loc];
                    if (name.empty())
                        continue;

                    if (Utf8FitTo(name, wnamepart))
                        break;
                }
            }

            if (loc < MAX_LOCALE)
            {
                FactionState const* repState = target ? target->GetReputationMgr().GetState(factionEntry) : NULL;
                ShowFactionListHelper(factionEntry, LocaleConstant(loc), repState, target);
                counter++;
            }
        }
    }

    if (counter == 0)                                       // if counter == 0 then we found nth
        SendSysMessage(LANG_COMMAND_FACTION_NOTFOUND);
    return true;
}

bool ChatHandler::HandleModifyPowerTypeCommand(char* args)
{
    if(!*args)
        return false;

    int32 type = atoi((char*)args);

    if (type < 0 || type >= MAX_POWERS)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Unit* target = getSelectedUnit();
    if (!target)
    {
        SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    target->setPowerType(Powers(type));

    return true;
}

bool ChatHandler::HandleModifyRepCommand(char* args)
{
    if (!*args)
        return false;

    Player* target = NULL;
    target = getSelectedPlayer();

    if (!target)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(target))
        return false;

    uint32 factionId;
    if (!ExtractUint32KeyFromLink(&args, "Hfaction", factionId))
        return false;

    if (!factionId)
        return false;

    int32 amount = 0;
    if (!ExtractInt32(&args, amount))
    {
        char *rankTxt = ExtractLiteralArg(&args);
        if (!rankTxt)
            return false;

        std::string rankStr = rankTxt;
        std::wstring wrankStr;
        if (!Utf8toWStr(rankStr, wrankStr))
            return false;
        wstrToLower(wrankStr);

        int r = 0;
        amount = -42000;
        for (; r < MAX_REPUTATION_RANK; ++r)
        {
            std::string rank = GetMangosString(ReputationRankStrIndex[r]);
            if (rank.empty())
                continue;

            std::wstring wrank;
            if (!Utf8toWStr(rank, wrank))
                continue;

            wstrToLower(wrank);

            if (wrank.substr(0, wrankStr.size()) == wrankStr)
            {
                int32 delta;
                if (!ExtractInt32(&args, delta) || (delta < 0) || (delta > ReputationMgr::PointsInRank[r] -1))
                {
                    PSendSysMessage(LANG_COMMAND_FACTION_DELTA, (ReputationMgr::PointsInRank[r]-1));
                    SetSentErrorMessage(true);
                    return false;
                }
                amount += delta;
                break;
            }
            amount += ReputationMgr::PointsInRank[r];
        }
        if (r >= MAX_REPUTATION_RANK)
        {
            PSendSysMessage(LANG_COMMAND_FACTION_INVPARAM, rankTxt);
            SetSentErrorMessage(true);
            return false;
        }
    }

    FactionEntry const *factionEntry = sFactionStore.LookupEntry(factionId);

    if (!factionEntry)
    {
        PSendSysMessage(LANG_COMMAND_FACTION_UNKNOWN, factionId);
        SetSentErrorMessage(true);
        return false;
    }

    if (factionEntry->reputationListID < 0)
    {
        PSendSysMessage(LANG_COMMAND_FACTION_NOREP_ERROR, factionEntry->name[GetSessionDbcLocale()], factionId);
        SetSentErrorMessage(true);
        return false;
    }

    target->GetReputationMgr().SetReputation(factionEntry,amount);
    PSendSysMessage(LANG_COMMAND_MODIFY_REP, factionEntry->name[GetSessionDbcLocale()], factionId,
        GetNameLink(target).c_str(), target->GetReputationMgr().GetReputation(factionEntry));
    return true;
}

//-----------------------Npc Commands-----------------------
//add spawn of creature
bool ChatHandler::HandleNpcAddCommand(char* args)
{
    if (!*args)
        return false;

    uint32 id;
    if (!ExtractUint32KeyFromLink(&args, "Hcreature_entry", id))
        return false;

    Player *chr = m_session->GetPlayer();
    float x = chr->GetPositionX();
    float y = chr->GetPositionY();
    float z = chr->GetPositionZ();
    float o = chr->GetOrientation();
    Map *map = chr->GetMap();

    Creature* pCreature = new Creature;
    if (!pCreature->Create(sObjectMgr.GenerateLowGuid(HIGHGUID_UNIT), map, chr->GetPhaseMaskForSpawn(), id))
    {
        delete pCreature;
        return false;
    }

    pCreature->Relocate(x,y,z,o);

    if (!pCreature->IsPositionValid())
    {
        sLog.outError("Creature (guidlow %d, entry %d) not created. Suggested coordinates isn't valid (X: %f Y: %f)",pCreature->GetGUIDLow(),pCreature->GetEntry(),pCreature->GetPositionX(),pCreature->GetPositionY());
        delete pCreature;
        return false;
    }

    pCreature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), chr->GetPhaseMaskForSpawn());

    uint32 db_guid = pCreature->GetDBTableGUIDLow();

    // To call _LoadGoods(); _LoadQuests(); CreateTrainerSpells();
    pCreature->LoadFromDB(db_guid, map);

    map->Add(pCreature);
    sObjectMgr.AddCreatureToGrid(db_guid, sObjectMgr.GetCreatureData(db_guid));
    return true;
}

//add item in vendorlist
bool ChatHandler::HandleNpcAddVendorItemCommand(char* args)
{
    uint32 itemId;
    if (!ExtractUint32KeyFromLink(&args, "Hitem", itemId))
    {
        SendSysMessage(LANG_COMMAND_NEEDITEMSEND);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 maxcount;
    if (!ExtractOptUInt32(&args, maxcount, 0))
        return false;

    uint32 incrtime;
    if (!ExtractOptUInt32(&args, incrtime, 0))
        return false;

    uint32 extendedcost;
    if (!ExtractOptUInt32(&args, extendedcost, 0))
        return false;

    Creature* vendor = getSelectedCreature();

    uint32 vendor_entry = vendor ? vendor->GetEntry() : 0;

    if (!sObjectMgr.IsVendorItemValid(false, "npc_vendor", vendor_entry, itemId, maxcount, incrtime, extendedcost, m_session->GetPlayer()))
    {
        SetSentErrorMessage(true);
        return false;
    }

    sObjectMgr.AddVendorItem(vendor_entry,itemId,maxcount,incrtime,extendedcost);

    ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(itemId);

    PSendSysMessage(LANG_ITEM_ADDED_TO_LIST,itemId,pProto->Name1,maxcount,incrtime,extendedcost);
    return true;
}

//del item from vendor list
bool ChatHandler::HandleNpcDelVendorItemCommand(char* args)
{
    if (!*args)
        return false;

    Creature* vendor = getSelectedCreature();
    if (!vendor || !vendor->isVendor())
    {
        SendSysMessage(LANG_COMMAND_VENDORSELECTION);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 itemId;
    if (!ExtractUint32KeyFromLink(&args, "Hitem", itemId))
    {
        SendSysMessage(LANG_COMMAND_NEEDITEMSEND);
        SetSentErrorMessage(true);
        return false;
    }

    if (!sObjectMgr.RemoveVendorItem(vendor->GetEntry(), itemId))
    {
        PSendSysMessage(LANG_ITEM_NOT_IN_LIST,itemId);
        SetSentErrorMessage(true);
        return false;
    }

    ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(itemId);

    PSendSysMessage(LANG_ITEM_DELETED_FROM_LIST,itemId,pProto->Name1);
    return true;
}

//add move for creature
bool ChatHandler::HandleNpcAddMoveCommand(char* args)
{
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hcreature", lowguid))
        return false;

    uint32 wait;
    if (!ExtractOptUInt32(&args, wait, 0))
        return false;

    CreatureData const* data = sObjectMgr.GetCreatureData(lowguid);
    if (!data)
    {
        PSendSysMessage(LANG_COMMAND_CREATGUIDNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    Player* player = m_session->GetPlayer();

    if (player->GetMapId() != data->mapid)
    {
        PSendSysMessage(LANG_COMMAND_CREATUREATSAMEMAP, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    Creature* pCreature = player->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, data->id, lowguid));

    sWaypointMgr.AddLastNode(lowguid, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation(), wait, 0);

    // update movement type
    WorldDatabase.PExecuteLog("UPDATE creature SET MovementType = '%u' WHERE guid = '%u'", WAYPOINT_MOTION_TYPE,lowguid);
    if (pCreature)
    {
        pCreature->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
        pCreature->GetMotionMaster()->Initialize();
        if (pCreature->isAlive())                            // dead creature will reset movement generator at respawn
        {
            pCreature->SetDeathState(JUST_DIED);
            pCreature->Respawn();
        }
        pCreature->SaveToDB();
    }

    SendSysMessage(LANG_WAYPOINT_ADDED);

    return true;
}

//change level of creature or pet
bool ChatHandler::HandleNpcChangeLevelCommand(char* args)
{
    if (!*args)
        return false;

    uint8 lvl = (uint8) atoi(args);
    if (lvl < 1 || lvl > sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL) + 3)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Creature* pCreature = getSelectedCreature();
    if (!pCreature)
    {
        SendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    if (pCreature->IsPet())
        ((Pet*)pCreature)->GivePetLevel(lvl);
    else
    {
        pCreature->SetMaxHealth(100 + 30*lvl);
        pCreature->SetHealth(100 + 30*lvl);
        pCreature->SetLevel(lvl);
        pCreature->SaveToDB();
    }

    return true;
}

//set npcflag of creature
bool ChatHandler::HandleNpcFlagCommand(char* args)
{
    if (!*args)
        return false;

    uint32 npcFlags = (uint32) atoi(args);

    Creature* pCreature = getSelectedCreature();

    if (!pCreature)
    {
        SendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    pCreature->SetUInt32Value(UNIT_NPC_FLAGS, npcFlags);

    WorldDatabase.PExecuteLog("UPDATE creature_template SET npcflag = '%u' WHERE entry = '%u'", npcFlags, pCreature->GetEntry());

    SendSysMessage(LANG_VALUE_SAVED_REJOIN);

    return true;
}

bool ChatHandler::HandleNpcDeleteCommand(char* args)
{
    Creature* unit = NULL;

    if (*args)
    {
        // number or [name] Shift-click form |color|Hcreature:creature_guid|h[name]|h|r
        uint32 lowguid;
        if (!ExtractUint32KeyFromLink(&args, "Hcreature", lowguid))
            return false;

        if (!lowguid)
            return false;

        if (CreatureData const* data = sObjectMgr.GetCreatureData(lowguid))
            unit = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, data->id, lowguid));
    }
    else
        unit = getSelectedCreature();

    if (!unit || unit->IsPet() || unit->IsTotem())
    {
        SendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    // Delete the creature
    unit->CombatStop();
    unit->DeleteFromDB();
    unit->AddObjectToRemoveList();

    SendSysMessage(LANG_COMMAND_DELCREATMESSAGE);

    return true;
}

//move selected creature
bool ChatHandler::HandleNpcMoveCommand(char* args)
{
    uint32 lowguid = 0;

    Creature* pCreature = getSelectedCreature();

    if (!pCreature)
    {
        // number or [name] Shift-click form |color|Hcreature:creature_guid|h[name]|h|r
        if (!ExtractUint32KeyFromLink(&args, "Hcreature", lowguid))
            return false;

        CreatureData const* data = sObjectMgr.GetCreatureData(lowguid);
        if (!data)
        {
            PSendSysMessage(LANG_COMMAND_CREATGUIDNOTFOUND, lowguid);
            SetSentErrorMessage(true);
            return false;
        }

        Player* player = m_session->GetPlayer();

        if (player->GetMapId() != data->mapid)
        {
            PSendSysMessage(LANG_COMMAND_CREATUREATSAMEMAP, lowguid);
            SetSentErrorMessage(true);
            return false;
        }

        pCreature = player->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, data->id, lowguid));
    }
    else
        lowguid = pCreature->GetDBTableGUIDLow();

    float x = m_session->GetPlayer()->GetPositionX();
    float y = m_session->GetPlayer()->GetPositionY();
    float z = m_session->GetPlayer()->GetPositionZ();
    float o = m_session->GetPlayer()->GetOrientation();

    if (pCreature)
    {
        if (CreatureData const* data = sObjectMgr.GetCreatureData(pCreature->GetDBTableGUIDLow()))
        {
            const_cast<CreatureData*>(data)->posX = x;
            const_cast<CreatureData*>(data)->posY = y;
            const_cast<CreatureData*>(data)->posZ = z;
            const_cast<CreatureData*>(data)->orientation = o;
        }
        pCreature->GetMap()->CreatureRelocation(pCreature,x, y, z,o);
        pCreature->GetMotionMaster()->Initialize();
        if (pCreature->isAlive())                            // dead creature will reset movement generator at respawn
        {
            pCreature->SetDeathState(JUST_DIED);
            pCreature->Respawn();
        }
    }

    WorldDatabase.PExecuteLog("UPDATE creature SET position_x = '%f', position_y = '%f', position_z = '%f', orientation = '%f' WHERE guid = '%u'", x, y, z, o, lowguid);
    PSendSysMessage(LANG_COMMAND_CREATUREMOVED);
    return true;
}

/**HandleNpcSetMoveTypeCommand
 * Set the movement type for an NPC.<br/>
 * <br/>
 * Valid movement types are:
 * <ul>
 * <li> stay - NPC wont move </li>
 * <li> random - NPC will move randomly according to the spawndist </li>
 * <li> way - NPC will move with given waypoints set </li>
 * </ul>
 * additional parameter: NODEL - so no waypoints are deleted, if you
 *                       change the movement type
 */
bool ChatHandler::HandleNpcSetMoveTypeCommand(char* args)
{
    // 3 arguments:
    // GUID (optional - you can also select the creature)
    // stay|random|way (determines the kind of movement)
    // NODEL (optional - tells the system NOT to delete any waypoints)
    //        this is very handy if you want to do waypoints, that are
    //        later switched on/off according to special events (like escort
    //        quests, etc)

    uint32 lowguid;
    Creature* pCreature;
    if (!ExtractUInt32(&args, lowguid))                     // case .setmovetype $move_type (with selected creature)
    {
        pCreature = getSelectedCreature();
        if (!pCreature || pCreature->IsPet())
            return false;
        lowguid = pCreature->GetDBTableGUIDLow();
    }
    else                                                    // case .setmovetype #creature_guid $move_type (with guid)
    {
        CreatureData const* data = sObjectMgr.GetCreatureData(lowguid);
        if (!data)
        {
            PSendSysMessage(LANG_COMMAND_CREATGUIDNOTFOUND, lowguid);
            SetSentErrorMessage(true);
            return false;
        }

        Player* player = m_session->GetPlayer();

        if (player->GetMapId() != data->mapid)
        {
            PSendSysMessage(LANG_COMMAND_CREATUREATSAMEMAP, lowguid);
            SetSentErrorMessage(true);
            return false;
        }

        pCreature = player->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, data->id, lowguid));
    }

    MovementGeneratorType move_type;
    char* type_str = ExtractLiteralArg(&args);
    if (!type_str)
        return false;

    if (strncmp(type_str, "stay", strlen(type_str)) == 0)
        move_type = IDLE_MOTION_TYPE;
    else if (strncmp(type_str, "random", strlen(type_str)) == 0)
        move_type = RANDOM_MOTION_TYPE;
    else if (strncmp(type_str, "way", strlen(type_str)) == 0)
        move_type = WAYPOINT_MOTION_TYPE;
    else
        return false;

    bool doNotDelete = ExtractLiteralArg(&args, "NODEL") != NULL;
    if (!doNotDelete && *args)                              // need fail if false in result wrong literal
        return false;

    // now lowguid is low guid really existing creature
    // and pCreature point (maybe) to this creature or NULL

    // update movement type
    if (!doNotDelete)
        sWaypointMgr.DeletePath(lowguid);

    if (pCreature)
    {
        pCreature->SetDefaultMovementType(move_type);
        pCreature->GetMotionMaster()->Initialize();
        if (pCreature->isAlive())                            // dead creature will reset movement generator at respawn
        {
            pCreature->SetDeathState(JUST_DIED);
            pCreature->Respawn();
        }
        pCreature->SaveToDB();
    }

    if (doNotDelete)
        PSendSysMessage(LANG_MOVE_TYPE_SET_NODEL,type_str);
    else
        PSendSysMessage(LANG_MOVE_TYPE_SET,type_str);

    return true;
}

//set model of creature
bool ChatHandler::HandleNpcSetModelCommand(char* args)
{
    if (!*args)
        return false;

    uint32 displayId = (uint32) atoi(args);

    Creature *pCreature = getSelectedCreature();

    if (!pCreature || pCreature->IsPet())
    {
        SendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    pCreature->SetDisplayId(displayId);
    pCreature->SetNativeDisplayId(displayId);

    pCreature->SaveToDB();

    return true;
}
//set faction of creature
bool ChatHandler::HandleNpcFactionIdCommand(char* args)
{
    if (!*args)
        return false;

    uint32 factionId = (uint32) atoi(args);

    if (!sFactionTemplateStore.LookupEntry(factionId))
    {
        PSendSysMessage(LANG_WRONG_FACTION, factionId);
        SetSentErrorMessage(true);
        return false;
    }

    Creature* pCreature = getSelectedCreature();

    if (!pCreature)
    {
        SendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    pCreature->setFaction(factionId);

    // faction is set in creature_template - not inside creature

    // update in memory
    if (CreatureInfo const *cinfo = pCreature->GetCreatureInfo())
    {
        const_cast<CreatureInfo*>(cinfo)->faction_A = factionId;
        const_cast<CreatureInfo*>(cinfo)->faction_H = factionId;
    }

    // and DB
    WorldDatabase.PExecuteLog("UPDATE creature_template SET faction_A = '%u', faction_H = '%u' WHERE entry = '%u'", factionId, factionId, pCreature->GetEntry());

    return true;
}
//set spawn dist of creature
bool ChatHandler::HandleNpcSpawnDistCommand(char* args)
{
    if (!*args)
        return false;

    float option = (float)atof(args);
    if (option < 0.0f)
    {
        SendSysMessage(LANG_BAD_VALUE);
        return false;
    }

    MovementGeneratorType mtype = IDLE_MOTION_TYPE;
    if (option >0.0f)
        mtype = RANDOM_MOTION_TYPE;

    Creature *pCreature = getSelectedCreature();
    uint32 u_guidlow = 0;

    if (pCreature)
        u_guidlow = pCreature->GetDBTableGUIDLow();
    else
        return false;

    pCreature->SetRespawnRadius((float)option);
    pCreature->SetDefaultMovementType(mtype);
    pCreature->GetMotionMaster()->Initialize();
    if (pCreature->isAlive())                                // dead creature will reset movement generator at respawn
    {
        pCreature->SetDeathState(JUST_DIED);
        pCreature->Respawn();
    }

    WorldDatabase.PExecuteLog("UPDATE creature SET spawndist=%f, MovementType=%i WHERE guid=%u",option,mtype,u_guidlow);
    PSendSysMessage(LANG_COMMAND_SPAWNDIST,option);
    return true;
}
//spawn time handling
bool ChatHandler::HandleNpcSpawnTimeCommand(char* args)
{
    uint32 stime;
    if (!ExtractUInt32(&args, stime))
        return false;

    Creature *pCreature = getSelectedCreature();
    if (!pCreature)
    {
        PSendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 u_guidlow = pCreature->GetDBTableGUIDLow();

    WorldDatabase.PExecuteLog("UPDATE creature SET spawntimesecs=%i WHERE guid=%u", stime, u_guidlow);
    pCreature->SetRespawnDelay(stime);
    PSendSysMessage(LANG_COMMAND_SPAWNTIME, stime);

    return true;
}
//npc follow handling
bool ChatHandler::HandleNpcFollowCommand(char* /*args*/)
{
    Player *player = m_session->GetPlayer();
    Creature *creature = getSelectedCreature();

    if (!creature)
    {
        PSendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    // Follow player - Using pet's default dist and angle
    creature->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);

    PSendSysMessage(LANG_CREATURE_FOLLOW_YOU_NOW, creature->GetName());
    return true;
}
//npc unfollow handling
bool ChatHandler::HandleNpcUnFollowCommand(char* /*args*/)
{
    Player *player = m_session->GetPlayer();
    Creature *creature = getSelectedCreature();

    if (!creature)
    {
        PSendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    if (creature->GetMotionMaster()->empty() ||
        creature->GetMotionMaster()->GetCurrentMovementGeneratorType ()!=FOLLOW_MOTION_TYPE)
    {
        PSendSysMessage(LANG_CREATURE_NOT_FOLLOW_YOU);
        SetSentErrorMessage(true);
        return false;
    }

    FollowMovementGenerator<Creature> const* mgen
        = static_cast<FollowMovementGenerator<Creature> const*>((creature->GetMotionMaster()->top()));

    if (mgen->GetTarget() != player)
    {
        PSendSysMessage(LANG_CREATURE_NOT_FOLLOW_YOU);
        SetSentErrorMessage(true);
        return false;
    }

    // reset movement
    creature->GetMotionMaster()->MovementExpired(true);

    PSendSysMessage(LANG_CREATURE_NOT_FOLLOW_YOU_NOW, creature->GetName());
    return true;
}

//npc phasemask handling
//change phasemask of creature or pet
bool ChatHandler::HandleNpcSetPhaseCommand(char* args)
{
    if (!*args)
        return false;

    uint32 phasemask = (uint32) atoi(args);
    if (phasemask == 0)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Creature* pCreature = getSelectedCreature();
    if (!pCreature)
    {
        SendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    pCreature->SetPhaseMask(phasemask,true);

    if (!pCreature->IsPet())
        pCreature->SaveToDB();

    return true;
}
//npc deathstate handling
bool ChatHandler::HandleNpcSetDeathStateCommand(char* args)
{
    bool value;
    if (!ExtractOnOff(&args, value))
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    Creature* pCreature = getSelectedCreature();
    if (!pCreature || pCreature->IsPet())
    {
        SendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    if (value)
        pCreature->SetDeadByDefault(true);
    else
        pCreature->SetDeadByDefault(false);

    pCreature->SaveToDB();
    pCreature->Respawn();

    return true;
}

//TODO: NpcCommands that need to be fixed :

bool ChatHandler::HandleNpcNameCommand(char* /*args*/)
{
    /* Temp. disabled
    if (!*args)
        return false;

    if (strlen((char*)args)>75)
    {
        PSendSysMessage(LANG_TOO_LONG_NAME, strlen((char*)args)-75);
        return true;
    }

    for (uint8 i = 0; i < strlen(args); ++i)
    {
        if (!isalpha(args[i]) && args[i]!=' ')
        {
            SendSysMessage(LANG_CHARS_ONLY);
            return false;
        }
    }

    ObjectGuid guid = m_session->GetPlayer()->GetSelectionGuid();
    if (guid.IsEmpty())
    {
        SendSysMessage(LANG_NO_SELECTION);
        return true;
    }

    Creature* pCreature = ObjectAccessor::GetCreature(*m_session->GetPlayer(), guid);

    if (!pCreature)
    {
        SendSysMessage(LANG_SELECT_CREATURE);
        return true;
    }

    pCreature->SetName(args);
    uint32 idname = sObjectMgr.AddCreatureTemplate(pCreature->GetName());
    pCreature->SetUInt32Value(OBJECT_FIELD_ENTRY, idname);

    pCreature->SaveToDB();
    */

    return true;
}

bool ChatHandler::HandleNpcSubNameCommand(char* /*args*/)
{
    /* Temp. disabled

    if (!*args)
        args = "";

    if (strlen((char*)args)>75)
    {

        PSendSysMessage(LANG_TOO_LONG_SUBNAME, strlen((char*)args)-75);
        return true;
    }

    for (uint8 i = 0; i < strlen(args); i++)
    {
        if (!isalpha(args[i]) && args[i]!=' ')
        {
            SendSysMessage(LANG_CHARS_ONLY);
            return false;
        }
    }

    ObjectGuid guid = m_session->GetPlayer()->GetSelectionGuid();
    if (guid.IsEmpty())
    {
        SendSysMessage(LANG_NO_SELECTION);
        return true;
    }

    Creature* pCreature = ObjectAccessor::GetCreature(*m_session->GetPlayer(), guid);

    if (!pCreature)
    {
        SendSysMessage(LANG_SELECT_CREATURE);
        return true;
    }

    uint32 idname = sObjectMgr.AddCreatureSubName(pCreature->GetName(),args,pCreature->GetUInt32Value(UNIT_FIELD_DISPLAYID));
    pCreature->SetUInt32Value(OBJECT_FIELD_ENTRY, idname);

    pCreature->SaveToDB();
    */
    return true;
}

//move item to other slot
bool ChatHandler::HandleItemMoveCommand(char* args)
{
    if (!*args)
        return false;
    uint8 srcslot, dstslot;

    char* pParam1 = strtok(args, " ");
    if (!pParam1)
        return false;

    char* pParam2 = strtok(NULL, " ");
    if (!pParam2)
        return false;

    srcslot = (uint8)atoi(pParam1);
    dstslot = (uint8)atoi(pParam2);

    if (srcslot == dstslot)
        return true;

    if (!m_session->GetPlayer()->IsValidPos(INVENTORY_SLOT_BAG_0, srcslot, true))
        return false;

    // can be autostore pos
    if (!m_session->GetPlayer()->IsValidPos(INVENTORY_SLOT_BAG_0, dstslot, false))
        return false;

    uint16 src = ((INVENTORY_SLOT_BAG_0 << 8) | srcslot);
    uint16 dst = ((INVENTORY_SLOT_BAG_0 << 8) | dstslot);

    m_session->GetPlayer()->SwapItem(src, dst);

    return true;
}

//demorph player or unit
bool ChatHandler::HandleDeMorphCommand(char* /*args*/)
{
    Unit *target = getSelectedUnit();
    if (!target)
        target = m_session->GetPlayer();


    // check online security
    else if (target->GetTypeId() == TYPEID_PLAYER && HasLowerSecurity((Player*)target))
        return false;

    target->DeMorph();

    return true;
}

//morph creature or player
bool ChatHandler::HandleModifyMorphCommand(char* args)
{
    if (!*args)
        return false;

    uint16 display_id = (uint16)atoi(args);

    Unit *target = getSelectedUnit();
    if (!target)
        target = m_session->GetPlayer();

    // check online security
    else if (target->GetTypeId() == TYPEID_PLAYER && HasLowerSecurity((Player*)target))
        return false;

    target->SetDisplayId(display_id);

    return true;
}

//kick player
bool ChatHandler::HandleKickPlayerCommand(char *args)
{
    Player* target;
    if (!ExtractPlayerTarget(&args, &target))
        return false;

    if (m_session && target == m_session->GetPlayer())
    {
        SendSysMessage(LANG_COMMAND_KICKSELF);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(target))
        return false;

    // send before target pointer invalidate
    PSendSysMessage(LANG_COMMAND_KICKMESSAGE, GetNameLink(target).c_str());
    target->GetSession()->KickPlayer();
    return true;
}

//set temporary phase mask for player
bool ChatHandler::HandleModifyPhaseCommand(char* args)
{
    if (!*args)
        return false;

    uint32 phasemask = (uint32)atoi(args);

    Unit *target = getSelectedUnit();
    if (!target)
        target = m_session->GetPlayer();

    // check online security
    else if (target->GetTypeId() == TYPEID_PLAYER && HasLowerSecurity((Player*)target))
        return false;

    target->SetPhaseMask(phasemask,true);

    return true;
}

//show info of player
bool ChatHandler::HandlePInfoCommand(char* args)
{
    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&args, &target, &target_guid, &target_name))
        return false;

    uint32 accId = 0;
    uint32 money = 0;
    uint32 total_player_time = 0;
    uint32 level = 0;
    uint32 latency = 0;

    // get additional information from Player object
    if (target)
    {
        // check online security
        if (HasLowerSecurity(target))
            return false;

        accId = target->GetSession()->GetAccountId();
        money = target->GetMoney();
        total_player_time = target->GetTotalPlayedTime();
        level = target->getLevel();
        latency = target->GetSession()->GetLatency();
    }
    // get additional information from DB
    else
    {
        // check offline security
        if (HasLowerSecurity(NULL, target_guid))
            return false;

        //                                                     0          1      2      3
        QueryResult *result = CharacterDatabase.PQuery("SELECT totaltime, level, money, account FROM characters WHERE guid = '%u'", target_guid.GetCounter());
        if (!result)
            return false;

        Field *fields = result->Fetch();
        total_player_time = fields[0].GetUInt32();
        level = fields[1].GetUInt32();
        money = fields[2].GetUInt32();
        accId = fields[3].GetUInt32();
        delete result;
    }

    std::string username = GetMangosString(LANG_ERROR);
    std::string last_ip = GetMangosString(LANG_ERROR);
    AccountTypes security = SEC_PLAYER;
    std::string last_login = GetMangosString(LANG_ERROR);

    QueryResult* result = LoginDatabase.PQuery("SELECT username,gmlevel,last_ip,last_login FROM account WHERE id = '%u'",accId);
    if (result)
    {
        Field* fields = result->Fetch();
        username = fields[0].GetCppString();
        security = (AccountTypes)fields[1].GetUInt32();

        if (GetAccessLevel() >= security)
        {
            last_ip = fields[2].GetCppString();
            last_login = fields[3].GetCppString();
        }
        else
        {
            last_ip = "-";
            last_login = "-";
        }

        delete result;
    }

    std::string nameLink = playerLink(target_name);

    PSendSysMessage(LANG_PINFO_ACCOUNT, (target?"":GetMangosString(LANG_OFFLINE)), nameLink.c_str(), target_guid.GetCounter(), username.c_str(), accId, security, last_ip.c_str(), last_login.c_str(), latency);

    std::string timeStr = secsToTimeString(total_player_time,true,true);
    uint32 gold = money /GOLD;
    uint32 silv = (money % GOLD) / SILVER;
    uint32 copp = (money % GOLD) % SILVER;
    PSendSysMessage(LANG_PINFO_LEVEL,  timeStr.c_str(), level, gold,silv,copp);

    return true;
}

//show tickets
void ChatHandler::ShowTicket(GMTicket const* ticket)
{
    std::string lastupdated = TimeToTimestampStr(ticket->GetLastUpdate());

    std::string name;
    if (!sObjectMgr.GetPlayerNameByGUID(ticket->GetPlayerGuid(), name))
        name = GetMangosString(LANG_UNKNOWN);

    std::string nameLink = playerLink(name);

    char const* response = ticket->GetResponse();

    PSendSysMessage(LANG_COMMAND_TICKETVIEW, nameLink.c_str(), lastupdated.c_str(), ticket->GetText());
    if (strlen(response))
        PSendSysMessage(LANG_COMMAND_TICKETRESPONSE, ticket->GetResponse());
}

//ticket commands
bool ChatHandler::HandleTicketCommand(char* args)
{
    char* px = ExtractLiteralArg(&args);

    // ticket<end>
    if (!px)
    {
        size_t count = sTicketMgr.GetTicketCount();

        if (m_session)
        {
            bool accept = m_session->GetPlayer()->isAcceptTickets();

            PSendSysMessage(LANG_COMMAND_TICKETCOUNT, count, accept ?  GetMangosString(LANG_ON) : GetMangosString(LANG_OFF));
        }
        else
            PSendSysMessage(LANG_COMMAND_TICKETCOUNT_CONSOLE, count);

        return true;
    }

    // ticket on
    if (strncmp(px, "on", 3) == 0)
    {
        if (!m_session)
        {
            SendSysMessage(LANG_PLAYER_NOT_FOUND);
            SetSentErrorMessage(true);
            return false;
        }

        m_session->GetPlayer()->SetAcceptTicket(true);
        SendSysMessage(LANG_COMMAND_TICKETON);
        return true;
    }

    // ticket off
    if (strncmp(px, "off", 4) == 0)
    {
        if (!m_session)
        {
            SendSysMessage(LANG_PLAYER_NOT_FOUND);
            SetSentErrorMessage(true);
            return false;
        }

        m_session->GetPlayer()->SetAcceptTicket(false);
        SendSysMessage(LANG_COMMAND_TICKETOFF);
        return true;
    }

    // ticket respond
    if (strncmp(px, "respond", 8) == 0)
    {
        GMTicket* ticket = NULL;

        // ticket respond #num
        uint32 num;
        if (ExtractUInt32(&args, num))
        {
            if (num == 0)
                return false;

            // mgr numbering tickets start from 0
            ticket = sTicketMgr.GetGMTicketByOrderPos(num-1);

            if (!ticket)
            {
                PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST, num);
                SetSentErrorMessage(true);
                return false;
            }
        }
        else
        {
            ObjectGuid target_guid;
            std::string target_name;
            if (!ExtractPlayerTarget(&args, NULL, &target_guid, &target_name))
                return false;

            // ticket respond $char_name
            ticket = sTicketMgr.GetGMTicket(target_guid);

            if (!ticket)
            {
                PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST_NAME, target_name.c_str());
                SetSentErrorMessage(true);
                return false;
            }
        }

        // no response text?
        if (!*args)
            return false;

        ticket->SetResponseText(args);

        if (Player* pl = sObjectMgr.GetPlayer(ticket->GetPlayerGuid()))
            pl->GetSession()->SendGMResponse(ticket);

        return true;
    }

    // ticket #num
    uint32 num;
    if (ExtractUInt32(&px, num))
    {
        if (num == 0)
            return false;

        // mgr numbering tickets start from 0
        GMTicket* ticket = sTicketMgr.GetGMTicketByOrderPos(num-1);
        if (!ticket)
        {
            PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST, num);
            SetSentErrorMessage(true);
            return false;
        }

        ShowTicket(ticket);
        return true;
    }

    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&px, NULL, &target_guid, &target_name))
        return false;

    // ticket $char_name
    GMTicket* ticket = sTicketMgr.GetGMTicket(target_guid);
    if (!ticket)
    {
        PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST_NAME, target_name.c_str());
        SetSentErrorMessage(true);
        return false;
    }

    ShowTicket(ticket);

    return true;
}

//dell all tickets
bool ChatHandler::HandleDelTicketCommand(char *args)
{
    char* px = ExtractLiteralArg(&args);
    if (!px)
        return false;

    // delticket all
    if (strncmp(px, "all", 4) == 0)
    {
        sTicketMgr.DeleteAll();
        SendSysMessage(LANG_COMMAND_ALLTICKETDELETED);
        return true;
    }

    uint32 num;

    // delticket #num
    if (ExtractUInt32(&px, num))
    {
        if (num ==0)
            return false;

        // mgr numbering tickets start from 0
        GMTicket* ticket = sTicketMgr.GetGMTicketByOrderPos(num-1);

        if (!ticket)
        {
            PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST, num);
            SetSentErrorMessage(true);
            return false;
        }

        ObjectGuid guid = ticket->GetPlayerGuid();

        sTicketMgr.Delete(guid);

        //notify player
        if (Player* pl = sObjectMgr.GetPlayer(guid))
        {
            pl->GetSession()->SendGMTicketGetTicket(0x0A);
            PSendSysMessage(LANG_COMMAND_TICKETPLAYERDEL, GetNameLink(pl).c_str());
        }
        else
            PSendSysMessage(LANG_COMMAND_TICKETDEL);

        return true;
    }

    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&px, &target, &target_guid, &target_name))
        return false;

    // delticket $char_name
    sTicketMgr.Delete(target_guid);

    // notify players about ticket deleting
    if (target)
        target->GetSession()->SendGMTicketGetTicket(0x0A);

    std::string nameLink = playerLink(target_name);

    PSendSysMessage(LANG_COMMAND_TICKETPLAYERDEL,nameLink.c_str());
    return true;
}

/**
 * Add a waypoint to a creature.
 *
 * The user can either select an npc or provide its GUID.
 *
 * The user can even select a visual waypoint - then the new waypoint
 * is placed *after* the selected one - this makes insertion of new
 * waypoints possible.
 *
 * eg:
 * .wp add 12345
 * -> adds a waypoint to the npc with the GUID 12345
 *
 * .wp add
 * -> adds a waypoint to the currently selected creature
 *
 *
 * @param args if the user did not provide a GUID, it is NULL
 *
 * @return true - command did succeed, false - something went wrong
 */
bool ChatHandler::HandleWpAddCommand(char* args)
{
    DEBUG_LOG("DEBUG: HandleWpAddCommand");

    // optional
    char* guid_str = NULL;

    if (*args)
    {
        guid_str = strtok(args, " ");
    }

    uint32 lowguid = 0;
    uint32 point = 0;
    Creature* target = getSelectedCreature();
    // Did player provide a GUID?
    if (!guid_str)
    {
        DEBUG_LOG("DEBUG: HandleWpAddCommand - No GUID provided");

        // No GUID provided
        // -> Player must have selected a creature

        if (!target || target->IsPet())
        {
            SendSysMessage(LANG_SELECT_CREATURE);
            SetSentErrorMessage(true);
            return false;
        }
        if (target->GetEntry() == VISUAL_WAYPOINT)
        {
            DEBUG_LOG("DEBUG: HandleWpAddCommand - target->GetEntry() == VISUAL_WAYPOINT (1) ");

            QueryResult *result =
                WorldDatabase.PQuery("SELECT id, point FROM creature_movement WHERE wpguid = %u",
                target->GetGUIDLow());
            if (!result)
            {
                PSendSysMessage(LANG_WAYPOINT_NOTFOUNDSEARCH, target->GetGUIDLow());
                // User selected a visual spawnpoint -> get the NPC
                // Select NPC GUID
                // Since we compare float values, we have to deal with
                // some difficulties.
                // Here we search for all waypoints that only differ in one from 1 thousand
                // (0.001) - There is no other way to compare C++ floats with mySQL floats
                // See also: http://dev.mysql.com/doc/refman/5.0/en/problems-with-float.html
                const char* maxDIFF = "0.01";
                result = WorldDatabase.PQuery("SELECT id, point FROM creature_movement WHERE (abs(position_x - %f) <= %s ) and (abs(position_y - %f) <= %s ) and (abs(position_z - %f) <= %s )",
                    target->GetPositionX(), maxDIFF, target->GetPositionY(), maxDIFF, target->GetPositionZ(), maxDIFF);
                if (!result)
                {
                    PSendSysMessage(LANG_WAYPOINT_NOTFOUNDDBPROBLEM, target->GetGUIDLow());
                    SetSentErrorMessage(true);
                    return false;
                }
            }
            do
            {
                Field *fields = result->Fetch();
                lowguid = fields[0].GetUInt32();
                point   = fields[1].GetUInt32();
            } while (result->NextRow());
            delete result;

            CreatureData const* data = sObjectMgr.GetCreatureData(lowguid);
            if (!data)
            {
                PSendSysMessage(LANG_WAYPOINT_CREATNOTFOUND, lowguid);
                SetSentErrorMessage(true);
                return false;
            }

            target = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, data->id, lowguid));
            if (!target)
            {
                PSendSysMessage(LANG_WAYPOINT_NOTFOUNDDBPROBLEM, lowguid);
                SetSentErrorMessage(true);
                return false;
            }
        }
        else
        {
            lowguid = target->GetDBTableGUIDLow();
        }
    }
    else
    {
        DEBUG_LOG("DEBUG: HandleWpAddCommand - GUID provided");

        // GUID provided
        // Warn if player also selected a creature
        // -> Creature selection is ignored <-
        if (target)
        {
            SendSysMessage(LANG_WAYPOINT_CREATSELECTED);
        }
        lowguid = atoi((char*)guid_str);

        CreatureData const* data = sObjectMgr.GetCreatureData(lowguid);
        if (!data)
        {
            PSendSysMessage(LANG_WAYPOINT_CREATNOTFOUND, lowguid);
            SetSentErrorMessage(true);
            return false;
        }

        target = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, data->id, lowguid));
        if (!target || target->IsPet())
        {
            PSendSysMessage(LANG_WAYPOINT_CREATNOTFOUND, lowguid);
            SetSentErrorMessage(true);
            return false;
        }
    }
    // lowguid -> GUID of the NPC
    // point   -> number of the waypoint (if not 0)
    DEBUG_LOG("DEBUG: HandleWpAddCommand - danach");

    DEBUG_LOG("DEBUG: HandleWpAddCommand - point == 0");

    Player* player = m_session->GetPlayer();
    sWaypointMgr.AddLastNode(lowguid, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation(), 0, 0);

    // update movement type
    if (target)
    {
        target->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
        target->GetMotionMaster()->Initialize();
        if (target->isAlive())                               // dead creature will reset movement generator at respawn
        {
            target->SetDeathState(JUST_DIED);
            target->Respawn();
        }
        target->SaveToDB();
    }
    else
        WorldDatabase.PExecuteLog("UPDATE creature SET MovementType = '%u' WHERE guid = '%u'", WAYPOINT_MOTION_TYPE,lowguid);

    PSendSysMessage(LANG_WAYPOINT_ADDED, point, lowguid);

    return true;
}                                                           // HandleWpAddCommand

/**
 * .wp modify emote | spell | text | del | move | add
 *
 * add -> add a WP after the selected visual waypoint
 *        User must select a visual waypoint and then issue ".wp modify add"
 *
 * emote <emoteID>
 *   User has selected a visual waypoint before.
 *   <emoteID> is added to this waypoint. Everytime the
 *   NPC comes to this waypoint, the emote is called.
 *
 * emote <GUID> <WPNUM> <emoteID>
 *   User has not selected visual waypoint before.
 *   For the waypoint <WPNUM> for the NPC with <GUID>
 *   an emote <emoteID> is added.
 *   Everytime the NPC comes to this waypoint, the emote is called.
 *
 *
 * info <GUID> <WPNUM> -> User did not select a visual waypoint and
 */
bool ChatHandler::HandleWpModifyCommand(char* args)
{
    DEBUG_LOG("DEBUG: HandleWpModifyCommand");

    if (!*args)
        return false;

    // first arg: add del text emote spell waittime move
    char* show_str = strtok(args, " ");
    if (!show_str)
    {
        return false;
    }

    std::string show = show_str;
    // Check
    // Remember: "show" must also be the name of a column!
    if ((show != "emote") && (show != "spell") && (show != "textid1") && (show != "textid2")
        && (show != "textid3") && (show != "textid4") && (show != "textid5")
        && (show != "waittime") && (show != "del") && (show != "move") && (show != "add")
        && (show != "model1") && (show != "model2") && (show != "orientation"))
    {
        return false;
    }

    // Next arg is: <GUID> <WPNUM> <ARGUMENT>

    // Did user provide a GUID
    // or did the user select a creature?
    // -> variable lowguid is filled with the GUID of the NPC
    uint32 lowguid = 0;
    uint32 point = 0;
    uint32 wpGuid = 0;
    Creature* target = getSelectedCreature();

    if (target)
    {
        DEBUG_LOG("DEBUG: HandleWpModifyCommand - User did select an NPC");

        // Did the user select a visual spawnpoint?
        if (target->GetEntry() != VISUAL_WAYPOINT)
        {
            PSendSysMessage(LANG_WAYPOINT_VP_SELECT);
            SetSentErrorMessage(true);
            return false;
        }

        wpGuid = target->GetGUIDLow();

        // The visual waypoint
        QueryResult *result =
            WorldDatabase.PQuery("SELECT id, point FROM creature_movement WHERE wpguid = %u LIMIT 1",
            target->GetGUIDLow());
        if (!result)
        {
            PSendSysMessage(LANG_WAYPOINT_NOTFOUNDDBPROBLEM, wpGuid);
            SetSentErrorMessage(true);
            return false;
        }
        DEBUG_LOG("DEBUG: HandleWpModifyCommand - After getting wpGuid");

        Field *fields = result->Fetch();
        lowguid = fields[0].GetUInt32();
        point   = fields[1].GetUInt32();

        // Cleanup memory
        DEBUG_LOG("DEBUG: HandleWpModifyCommand - Cleanup memory");
        delete result;
    }
    else
    {
        // User did provide <GUID> <WPNUM>

        char* guid_str = strtok((char*)NULL, " ");
        if (!guid_str)
        {
            SendSysMessage(LANG_WAYPOINT_NOGUID);
            return false;
        }
        lowguid = atoi((char*)guid_str);

        CreatureData const* data = sObjectMgr.GetCreatureData(lowguid);
        if (!data)
        {
            PSendSysMessage(LANG_WAYPOINT_CREATNOTFOUND, lowguid);
            SetSentErrorMessage(true);
            return false;
        }

        PSendSysMessage("DEBUG: GUID provided: %d", lowguid);

        char* point_str = strtok((char*)NULL, " ");
        if (!point_str)
        {
            SendSysMessage(LANG_WAYPOINT_NOWAYPOINTGIVEN);
            return false;
        }
        point    = atoi((char*)point_str);

        PSendSysMessage("DEBUG: wpNumber provided: %d", point);

        // Now we need the GUID of the visual waypoint
        // -> "del", "move", "add" command

        QueryResult *result = WorldDatabase.PQuery("SELECT wpguid FROM creature_movement WHERE id = '%u' AND point = '%u' LIMIT 1", lowguid, point);
        if (!result)
        {
            PSendSysMessage(LANG_WAYPOINT_NOTFOUNDSEARCH, lowguid, point);
            SetSentErrorMessage(true);
            return false;
        }

        Field *fields = result->Fetch();
        wpGuid  = fields[0].GetUInt32();

        // Free memory
        delete result;
    }

    char* arg_str = NULL;
    // Check for argument
    if ((show.find("text") == std::string::npos) && (show != "del") && (show != "move") && (show != "add"))
    {
        // Text is enclosed in "<>", all other arguments not
        if (show.find("text") != std::string::npos)
            arg_str = strtok((char*)NULL, "<>");
        else
            arg_str = strtok((char*)NULL, " ");

        if (!arg_str)
        {
            PSendSysMessage(LANG_WAYPOINT_ARGUMENTREQ, show_str);
            return false;
        }
    }

    DEBUG_LOG("DEBUG: HandleWpModifyCommand - Parameters parsed - now execute the command");

    // wpGuid  -> GUID of the waypoint creature
    // lowguid -> GUID of the NPC
    // point   -> waypoint number

    // Special functions:
    // add - move - del -> no args commands
    // Add a waypoint after the selected visual
    if (show == "add" && target)
    {
        PSendSysMessage("DEBUG: wp modify add, GUID: %u", lowguid);

        // Get the creature for which we read the waypoint
        CreatureData const* data = sObjectMgr.GetCreatureData(lowguid);
        if (!data)
        {
            PSendSysMessage(LANG_WAYPOINT_CREATNOTFOUND, lowguid);
            SetSentErrorMessage(true);
            return false;
        }

        Creature* npcCreature = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, data->id, lowguid));

        if (!npcCreature)
        {
            PSendSysMessage(LANG_WAYPOINT_NPCNOTFOUND);
            SetSentErrorMessage(true);
            return false;
        }

        DEBUG_LOG("DEBUG: HandleWpModifyCommand - add -- npcCreature");

        // What to do:
        // Add the visual spawnpoint (DB only)
        // Adjust the waypoints
        // Respawn the owner of the waypoints
        DEBUG_LOG("DEBUG: HandleWpModifyCommand - add");

        Player* chr = m_session->GetPlayer();
        Map *map = chr->GetMap();

        if (npcCreature)
        {
            npcCreature->GetMotionMaster()->Initialize();
            if (npcCreature->isAlive())                      // dead creature will reset movement generator at respawn
            {
                npcCreature->SetDeathState(JUST_DIED);
                npcCreature->Respawn();
            }
        }

        // create the waypoint creature
        wpGuid = 0;
        Creature* wpCreature = new Creature;
        if (!wpCreature->Create(sObjectMgr.GenerateLowGuid(HIGHGUID_UNIT), map, chr->GetPhaseMaskForSpawn(), VISUAL_WAYPOINT))
        {
            PSendSysMessage(LANG_WAYPOINT_VP_NOTCREATED, VISUAL_WAYPOINT);
            delete wpCreature;
        }
        else
        {
            wpCreature->Relocate(chr->GetPositionX(), chr->GetPositionY(), chr->GetPositionZ(), chr->GetOrientation());

            if (!wpCreature->IsPositionValid())
            {
                sLog.outError("Creature (guidlow %d, entry %d) not created. Suggested coordinates isn't valid (X: %f Y: %f)",wpCreature->GetGUIDLow(),wpCreature->GetEntry(),wpCreature->GetPositionX(),wpCreature->GetPositionY());
                delete wpCreature;
            }
            else
            {
                wpCreature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), chr->GetPhaseMaskForSpawn());
                // To call _LoadGoods(); _LoadQuests(); CreateTrainerSpells();
                wpCreature->LoadFromDB(wpCreature->GetDBTableGUIDLow(), map);
                map->Add(wpCreature);
                wpGuid = wpCreature->GetGUIDLow();
            }
        }

        sWaypointMgr.AddAfterNode(lowguid, point, chr->GetPositionX(), chr->GetPositionY(), chr->GetPositionZ(), 0, 0, wpGuid);

        if (!wpGuid)
            return false;

        PSendSysMessage(LANG_WAYPOINT_ADDED_NO, point+1);
        return true;
    }                                                       // add

    if (show == "del" && target)
    {
        PSendSysMessage("DEBUG: wp modify del, GUID: %u", lowguid);

        // Get the creature for which we read the waypoint
        CreatureData const* data = sObjectMgr.GetCreatureData(lowguid);
        if (!data)
        {
            PSendSysMessage(LANG_WAYPOINT_CREATNOTFOUND, lowguid);
            SetSentErrorMessage(true);
            return false;
        }

        Creature* npcCreature = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, data->id, lowguid));

        // wpCreature
        Creature* wpCreature = NULL;
        if (wpGuid != 0)
        {
            wpCreature = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, VISUAL_WAYPOINT, wpGuid));
            wpCreature->DeleteFromDB();
            wpCreature->AddObjectToRemoveList();
        }

        // What to do:
        // Remove the visual spawnpoint
        // Adjust the waypoints
        // Respawn the owner of the waypoints

        sWaypointMgr.DeleteNode(lowguid, point);

        if (npcCreature)
        {
            // Any waypoints left?
            QueryResult *result2 = WorldDatabase.PQuery("SELECT point FROM creature_movement WHERE id = '%u'",lowguid);
            if (!result2)
            {
                npcCreature->SetDefaultMovementType(RANDOM_MOTION_TYPE);
            }
            else
            {
                npcCreature->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
                delete result2;
            }
            npcCreature->GetMotionMaster()->Initialize();
            if (npcCreature->isAlive())                      // dead creature will reset movement generator at respawn
            {
                npcCreature->SetDeathState(JUST_DIED);
                npcCreature->Respawn();
            }
            npcCreature->SaveToDB();
        }

        PSendSysMessage(LANG_WAYPOINT_REMOVED);
        return true;
    }                                                       // del

    if (show == "move" && target)
    {
        PSendSysMessage("DEBUG: wp move, GUID: %u", lowguid);

        Player *chr = m_session->GetPlayer();
        Map *map = chr->GetMap();
        {
            // Get the creature for which we read the waypoint
            CreatureData const* data = sObjectMgr.GetCreatureData(lowguid);
            if (!data)
            {
                PSendSysMessage(LANG_WAYPOINT_CREATNOTFOUND, lowguid);
                SetSentErrorMessage(true);
                return false;
            }

            Creature* npcCreature = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, data->id, lowguid));

            // wpCreature
            Creature* wpCreature = NULL;
            // What to do:
            // Move the visual spawnpoint
            // Respawn the owner of the waypoints
            if (wpGuid != 0)
            {
                wpCreature = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, VISUAL_WAYPOINT, wpGuid));
                wpCreature->DeleteFromDB();
                wpCreature->AddObjectToRemoveList();
                // re-create
                Creature* wpCreature2 = new Creature;
                if (!wpCreature2->Create(sObjectMgr.GenerateLowGuid(HIGHGUID_UNIT), map, chr->GetPhaseMaskForSpawn(), VISUAL_WAYPOINT))
                {
                    PSendSysMessage(LANG_WAYPOINT_VP_NOTCREATED, VISUAL_WAYPOINT);
                    delete wpCreature2;
                    return false;
                }

                wpCreature2->Relocate(chr->GetPositionX(), chr->GetPositionY(), chr->GetPositionZ(), chr->GetOrientation());

                if (!wpCreature2->IsPositionValid())
                {
                    sLog.outError("Creature (guidlow %d, entry %d) not created. Suggested coordinates isn't valid (X: %f Y: %f)",wpCreature2->GetGUIDLow(),wpCreature2->GetEntry(),wpCreature2->GetPositionX(),wpCreature2->GetPositionY());
                    delete wpCreature2;
                    return false;
                }

                wpCreature2->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), chr->GetPhaseMaskForSpawn());
                // To call _LoadGoods(); _LoadQuests(); CreateTrainerSpells();
                wpCreature2->LoadFromDB(wpCreature2->GetDBTableGUIDLow(), map);
                map->Add(wpCreature2);
                //npcCreature->GetMap()->Add(wpCreature2);
            }

            sWaypointMgr.SetNodePosition(lowguid, point, chr->GetPositionX(), chr->GetPositionY(), chr->GetPositionZ());

            if (npcCreature)
            {
                npcCreature->GetMotionMaster()->Initialize();
                if (npcCreature->isAlive())                  // dead creature will reset movement generator at respawn
                {
                    npcCreature->SetDeathState(JUST_DIED);
                    npcCreature->Respawn();
                }
            }
            PSendSysMessage(LANG_WAYPOINT_CHANGED);
        }
        return true;
    }                                                       // move

    // Create creature - npc that has the waypoint
    CreatureData const* data = sObjectMgr.GetCreatureData(lowguid);
    if (!data)
    {
        PSendSysMessage(LANG_WAYPOINT_CREATNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    // set in game textids not supported
    if (show == "textid1" || show == "textid2" || show == "textid3" ||
        show == "textid4" || show == "textid5")
    {
        return false;
    }

    sWaypointMgr.SetNodeText(lowguid, point, show_str, arg_str);

    Creature* npcCreature = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, data->id, lowguid));
    if (npcCreature)
    {
        npcCreature->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
        npcCreature->GetMotionMaster()->Initialize();
        if (npcCreature->isAlive())                          // dead creature will reset movement generator at respawn
        {
            npcCreature->SetDeathState(JUST_DIED);
            npcCreature->Respawn();
        }
    }
    PSendSysMessage(LANG_WAYPOINT_CHANGED_NO, show_str);

    return true;
}

/**
 * .wp show info | on | off
 *
 * info -> User has selected a visual waypoint before
 *
 * info <GUID> <WPNUM> -> User did not select a visual waypoint and
 *                        provided the GUID of the NPC and the number of
 *                        the waypoint.
 *
 * on -> User has selected an NPC; all visual waypoints for this
 *       NPC are added to the world
 *
 * on <GUID> -> User did not select an NPC - instead the GUID of the
 *              NPC is provided. All visual waypoints for this NPC
 *              are added from the world.
 *
 * off -> User has selected an NPC; all visual waypoints for this
 *        NPC are removed from the world.
 *
 * on <GUID> -> User did not select an NPC - instead the GUID of the
 *              NPC is provided. All visual waypoints for this NPC
 *              are removed from the world.
 *
 *
 */
bool ChatHandler::HandleWpShowCommand(char* args)
{
    DEBUG_LOG("DEBUG: HandleWpShowCommand");

    if (!*args)
        return false;

    // first arg: on, off, first, last
    char* show_str = strtok(args, " ");
    if (!show_str)
    {
        return false;
    }
    // second arg: GUID (optional, if a creature is selected)
    char* guid_str = strtok((char*)NULL, " ");
    DEBUG_LOG("DEBUG: HandleWpShowCommand: show_str: %s guid_str: %s", show_str, guid_str);
    //if (!guid_str) {
    //    return false;
    //}

    // Did user provide a GUID
    // or did the user select a creature?
    // -> variable lowguid is filled with the GUID
    Creature* target = getSelectedCreature();
    // Did player provide a GUID?
    if (!guid_str)
    {
        DEBUG_LOG("DEBUG: HandleWpShowCommand: !guid_str");
        // No GUID provided
        // -> Player must have selected a creature

        if (!target)
        {
            SendSysMessage(LANG_SELECT_CREATURE);
            SetSentErrorMessage(true);
            return false;
        }
    }
    else
    {
        DEBUG_LOG("DEBUG: HandleWpShowCommand: GUID provided");
        // GUID provided
        // Warn if player also selected a creature
        // -> Creature selection is ignored <-
        if (target)
        {
            SendSysMessage(LANG_WAYPOINT_CREATSELECTED);
        }

        uint32 lowguid = atoi((char*)guid_str);

        CreatureData const* data = sObjectMgr.GetCreatureData(lowguid);
        if (!data)
        {
            PSendSysMessage(LANG_WAYPOINT_CREATNOTFOUND, lowguid);
            SetSentErrorMessage(true);
            return false;
        }

        target = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, data->id, lowguid));

        if (!target)
        {
            PSendSysMessage(LANG_WAYPOINT_CREATNOTFOUND, lowguid);
            SetSentErrorMessage(true);
            return false;
        }
    }

    uint32 lowguid = target->GetDBTableGUIDLow();

    std::string show = show_str;
    uint32 Maxpoint;

    DEBUG_LOG("DEBUG: HandleWpShowCommand: lowguid: %u show: %s", lowguid, show_str);

    // Show info for the selected waypoint
    if (show == "info")
    {
        PSendSysMessage("DEBUG: wp info, GUID: %u", lowguid);

        // Check if the user did specify a visual waypoint
        if (target->GetEntry() != VISUAL_WAYPOINT)
        {
            PSendSysMessage(LANG_WAYPOINT_VP_SELECT);
            SetSentErrorMessage(true);
            return false;
        }

        //PSendSysMessage("wp on, GUID: %u", lowguid);

        //pCreature->GetPositionX();

        QueryResult *result =
            WorldDatabase.PQuery("SELECT id, point, waittime, emote, spell, textid1, textid2, textid3, textid4, textid5, model1, model2 FROM creature_movement WHERE wpguid = %u",
            target->GetGUIDLow());
        if (!result)
        {
            // Since we compare float values, we have to deal with
            // some difficulties.
            // Here we search for all waypoints that only differ in one from 1 thousand
            // (0.001) - There is no other way to compare C++ floats with mySQL floats
            // See also: http://dev.mysql.com/doc/refman/5.0/en/problems-with-float.html
            const char* maxDIFF = "0.01";
            PSendSysMessage(LANG_WAYPOINT_NOTFOUNDSEARCH, target->GetGUID());

            result = WorldDatabase.PQuery("SELECT id, point, waittime, emote, spell, textid1, textid2, textid3, textid4, textid5, model1, model2 FROM creature_movement WHERE (abs(position_x - %f) <= %s ) and (abs(position_y - %f) <= %s ) and (abs(position_z - %f) <= %s )",
                target->GetPositionX(), maxDIFF, target->GetPositionY(), maxDIFF, target->GetPositionZ(), maxDIFF);
            if (!result)
            {
                PSendSysMessage(LANG_WAYPOINT_NOTFOUNDDBPROBLEM, lowguid);
                SetSentErrorMessage(true);
                return false;
            }
        }
        do
        {
            Field *fields = result->Fetch();
            uint32 wpGuid           = fields[0].GetUInt32();
            uint32 point            = fields[1].GetUInt32();
            int waittime            = fields[2].GetUInt32();
            uint32 emote            = fields[3].GetUInt32();
            uint32 spell            = fields[4].GetUInt32();
            uint32 textid[MAX_WAYPOINT_TEXT];
            for(int i = 0;  i < MAX_WAYPOINT_TEXT; ++i)
                textid[i]           = fields[5+i].GetUInt32();
            uint32 model1           = fields[10].GetUInt32();
            uint32 model2           = fields[11].GetUInt32();

            // Get the creature for which we read the waypoint
            Creature* wpCreature = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, VISUAL_WAYPOINT, wpGuid));

            PSendSysMessage(LANG_WAYPOINT_INFO_TITLE, point, (wpCreature ? wpCreature->GetName() : "<not found>"), wpGuid);
            PSendSysMessage(LANG_WAYPOINT_INFO_WAITTIME, waittime);
            PSendSysMessage(LANG_WAYPOINT_INFO_MODEL, 1, model1);
            PSendSysMessage(LANG_WAYPOINT_INFO_MODEL, 2, model2);
            PSendSysMessage(LANG_WAYPOINT_INFO_EMOTE, emote);
            PSendSysMessage(LANG_WAYPOINT_INFO_SPELL, spell);
            for(int i = 0;  i < MAX_WAYPOINT_TEXT; ++i)
                PSendSysMessage(LANG_WAYPOINT_INFO_TEXT, i+1, textid[i], (textid[i] ? GetMangosString(textid[i]) : ""));

        } while (result->NextRow());
        // Cleanup memory
        delete result;
        return true;
    }

    if (show == "on")
    {
        PSendSysMessage("DEBUG: wp on, GUID: %u", lowguid);

        QueryResult *result = WorldDatabase.PQuery("SELECT point, position_x,position_y,position_z FROM creature_movement WHERE id = '%u'",lowguid);
        if (!result)
        {
            PSendSysMessage(LANG_WAYPOINT_NOTFOUND, lowguid);
            SetSentErrorMessage(true);
            return false;
        }
        // Delete all visuals for this NPC
        QueryResult *result2 = WorldDatabase.PQuery("SELECT wpguid FROM creature_movement WHERE id = '%u' and wpguid <> 0", lowguid);
        if (result2)
        {
            bool hasError = false;
            do
            {
                Field *fields = result2->Fetch();
                uint32 wpGuid = fields[0].GetUInt32();
                Creature* pCreature = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, VISUAL_WAYPOINT, wpGuid));

                if (!pCreature)
                {
                    PSendSysMessage(LANG_WAYPOINT_NOTREMOVED, wpGuid);
                    hasError = true;
                    WorldDatabase.PExecuteLog("DELETE FROM creature WHERE guid = '%u'", wpGuid);
                }
                else
                {
                    pCreature->DeleteFromDB();
                    pCreature->AddObjectToRemoveList();
                }

            } while (result2->NextRow());
            delete result2;
            if (hasError)
            {
                PSendSysMessage(LANG_WAYPOINT_TOOFAR1);
                PSendSysMessage(LANG_WAYPOINT_TOOFAR2);
                PSendSysMessage(LANG_WAYPOINT_TOOFAR3);
            }
        }

        do
        {
            Field *fields = result->Fetch();
            uint32 point    = fields[0].GetUInt32();
            float x         = fields[1].GetFloat();
            float y         = fields[2].GetFloat();
            float z         = fields[3].GetFloat();

            uint32 id = VISUAL_WAYPOINT;

            Player *chr = m_session->GetPlayer();
            Map *map = chr->GetMap();
            float o = chr->GetOrientation();

            Creature* wpCreature = new Creature;
            if (!wpCreature->Create(sObjectMgr.GenerateLowGuid(HIGHGUID_UNIT), map, chr->GetPhaseMaskForSpawn(), id))
            {
                PSendSysMessage(LANG_WAYPOINT_VP_NOTCREATED, id);
                delete wpCreature;
                delete result;
                return false;
            }

            wpCreature->Relocate(x, y, z, o);

            if (!wpCreature->IsPositionValid())
            {
                sLog.outError("Creature (guidlow %d, entry %d) not created. Suggested coordinates isn't valid (X: %f Y: %f)",wpCreature->GetGUIDLow(),wpCreature->GetEntry(),wpCreature->GetPositionX(),wpCreature->GetPositionY());
                delete wpCreature;
                delete result;
                return false;
            }

            wpCreature->SetVisibility(VISIBILITY_OFF);
            DEBUG_LOG("DEBUG: UPDATE creature_movement SET wpguid = '%u", wpCreature->GetGUIDLow());
            // set "wpguid" column to the visual waypoint
            WorldDatabase.PExecuteLog("UPDATE creature_movement SET wpguid = '%u' WHERE id = '%u' and point = '%u'", wpCreature->GetGUIDLow(), lowguid, point);

            wpCreature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), chr->GetPhaseMaskForSpawn());
            // To call _LoadGoods(); _LoadQuests(); CreateTrainerSpells();
            wpCreature->LoadFromDB(wpCreature->GetDBTableGUIDLow(),map);
            map->Add(wpCreature);
            //wpCreature->GetMap()->Add(wpCreature);
        } while (result->NextRow());

        // Cleanup memory
        delete result;
        return true;
    }

    if (show == "first")
    {
        PSendSysMessage("DEBUG: wp first, GUID: %u", lowguid);

        QueryResult *result = WorldDatabase.PQuery("SELECT position_x,position_y,position_z FROM creature_movement WHERE point='1' AND id = '%u'",lowguid);
        if (!result)
        {
            PSendSysMessage(LANG_WAYPOINT_NOTFOUND, lowguid);
            SetSentErrorMessage(true);
            return false;
        }

        Field *fields = result->Fetch();
        float x         = fields[0].GetFloat();
        float y         = fields[1].GetFloat();
        float z         = fields[2].GetFloat();
        uint32 id = VISUAL_WAYPOINT;

        Player *chr = m_session->GetPlayer();
        float o = chr->GetOrientation();
        Map *map = chr->GetMap();

        Creature* pCreature = new Creature;
        if (!pCreature->Create(sObjectMgr.GenerateLowGuid(HIGHGUID_UNIT),map, chr->GetPhaseMaskForSpawn(), id))
        {
            PSendSysMessage(LANG_WAYPOINT_VP_NOTCREATED, id);
            delete pCreature;
            delete result;
            return false;
        }

        pCreature->Relocate(x, y, z, o);

        if (!pCreature->IsPositionValid())
        {
            sLog.outError("Creature (guidlow %d, entry %d) not created. Suggested coordinates isn't valid (X: %f Y: %f)",pCreature->GetGUIDLow(),pCreature->GetEntry(),pCreature->GetPositionX(),pCreature->GetPositionY());
            delete pCreature;
            delete result;
            return false;
        }

        pCreature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), chr->GetPhaseMaskForSpawn());
        pCreature->LoadFromDB(pCreature->GetDBTableGUIDLow(), map);
        map->Add(pCreature);
        //player->PlayerTalkClass->SendPointOfInterest(x, y, 6, 6, 0, "First Waypoint");

        // Cleanup memory
        delete result;
        return true;
    }

    if (show == "last")
    {
        PSendSysMessage("DEBUG: wp last, GUID: %u", lowguid);

        QueryResult *result = WorldDatabase.PQuery("SELECT MAX(point) FROM creature_movement WHERE id = '%u'", lowguid);
        if (result)
        {
            Maxpoint = (*result)[0].GetUInt32();

            delete result;
        }
        else
            Maxpoint = 0;

        result = WorldDatabase.PQuery("SELECT position_x,position_y,position_z FROM creature_movement WHERE point ='%u' AND id = '%u'", Maxpoint, lowguid);
        if (!result)
        {
            PSendSysMessage(LANG_WAYPOINT_NOTFOUNDLAST, lowguid);
            SetSentErrorMessage(true);
            return false;
        }
        Field *fields = result->Fetch();
        float x         = fields[0].GetFloat();
        float y         = fields[1].GetFloat();
        float z         = fields[2].GetFloat();
        uint32 id = VISUAL_WAYPOINT;

        Player *chr = m_session->GetPlayer();
        float o = chr->GetOrientation();
        Map *map = chr->GetMap();

        Creature* pCreature = new Creature;
        if (!pCreature->Create(sObjectMgr.GenerateLowGuid(HIGHGUID_UNIT), map, chr->GetPhaseMaskForSpawn(), id))
        {
            PSendSysMessage(LANG_WAYPOINT_NOTCREATED, id);
            delete pCreature;
            delete result;
            return false;
        }

        pCreature->Relocate(x, y, z, o);

        if (!pCreature->IsPositionValid())
        {
            sLog.outError("Creature (guidlow %d, entry %d) not created. Suggested coordinates isn't valid (X: %f Y: %f)",pCreature->GetGUIDLow(),pCreature->GetEntry(),pCreature->GetPositionX(),pCreature->GetPositionY());
            delete pCreature;
            delete result;
            return false;
        }

        pCreature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), chr->GetPhaseMaskForSpawn());
        pCreature->LoadFromDB(pCreature->GetDBTableGUIDLow(), map);
        map->Add(pCreature);
        //player->PlayerTalkClass->SendPointOfInterest(x, y, 6, 6, 0, "Last Waypoint");
        // Cleanup memory
        delete result;
        return true;
    }

    if (show == "off")
    {
        QueryResult *result = WorldDatabase.PQuery("SELECT guid FROM creature WHERE id = '%u'", VISUAL_WAYPOINT);
        if (!result)
        {
            SendSysMessage(LANG_WAYPOINT_VP_NOTFOUND);
            SetSentErrorMessage(true);
            return false;
        }
        bool hasError = false;
        do
        {
            Field *fields = result->Fetch();
            uint32 wpGuid = fields[0].GetUInt32();
            Creature* pCreature = m_session->GetPlayer()->GetMap()->GetCreature(ObjectGuid(HIGHGUID_UNIT, VISUAL_WAYPOINT, wpGuid));
            if (!pCreature)
            {
                PSendSysMessage(LANG_WAYPOINT_NOTREMOVED, wpGuid);
                hasError = true;
                WorldDatabase.PExecuteLog("DELETE FROM creature WHERE guid = '%u'", wpGuid);
            }
            else
            {
                pCreature->DeleteFromDB();
                pCreature->AddObjectToRemoveList();
            }
        }while(result->NextRow());
        // set "wpguid" column to "empty" - no visual waypoint spawned
        WorldDatabase.PExecuteLog("UPDATE creature_movement SET wpguid = '0' WHERE wpguid <> '0'");

        if (hasError)
        {
            PSendSysMessage(LANG_WAYPOINT_TOOFAR1);
            PSendSysMessage(LANG_WAYPOINT_TOOFAR2);
            PSendSysMessage(LANG_WAYPOINT_TOOFAR3);
        }

        SendSysMessage(LANG_WAYPOINT_VP_ALLREMOVED);
        // Cleanup memory
        delete result;

        return true;
    }

    PSendSysMessage("DEBUG: wpshow - no valid command found");

    return true;
}                                                           // HandleWpShowCommand

bool ChatHandler::HandleWpExportCommand(char *args)
{
    if (!*args)
        return false;

    // Next arg is: <GUID> <ARGUMENT>

    // Did user provide a GUID
    // or did the user select a creature?
    // -> variable lowguid is filled with the GUID of the NPC
    uint32 lowguid = 0;
    Creature* target = getSelectedCreature();
    char* arg_str = NULL;
    if (target)
    {
        if (target->GetEntry() != VISUAL_WAYPOINT)
            lowguid = target->GetGUIDLow();
        else
        {
            QueryResult *result = WorldDatabase.PQuery("SELECT id FROM creature_movement WHERE wpguid = %u LIMIT 1", target->GetGUIDLow());
            if (!result)
            {
                PSendSysMessage(LANG_WAYPOINT_NOTFOUNDDBPROBLEM, target->GetGUIDLow());
                return true;
            }
            Field *fields = result->Fetch();
            lowguid = fields[0].GetUInt32();;
            delete result;
        }

        arg_str = strtok(args, " ");
    }
    else
    {
        // user provided <GUID>
        char* guid_str = strtok(args, " ");
        if (!guid_str)
        {
            SendSysMessage(LANG_WAYPOINT_NOGUID);
            return false;
        }
        lowguid = atoi((char*)guid_str);

        arg_str = strtok((char*)NULL, " ");
    }

    if (!arg_str)
    {
        PSendSysMessage(LANG_WAYPOINT_ARGUMENTREQ, "export");
        return false;
    }

    PSendSysMessage("DEBUG: wp export, GUID: %u", lowguid);

    QueryResult *result = WorldDatabase.PQuery(
    //          0      1           2           3           4            5       6       7         8      9      10       11       12       13       14       15
        "SELECT point, position_x, position_y, position_z, orientation, model1, model2, waittime, emote, spell, textid1, textid2, textid3, textid4, textid5, id FROM creature_movement WHERE id = '%u' ORDER BY point", lowguid);

    if (!result)
    {
        PSendSysMessage(LANG_WAYPOINT_NOTHINGTOEXPORT);
        SetSentErrorMessage(true);
        return false;
    }

    std::ofstream outfile;
    outfile.open (arg_str);

    do
    {
        Field *fields = result->Fetch();

        outfile << "INSERT INTO creature_movement ";
        outfile << "(id, point, position_x, position_y, position_z, orientation, model1, model2, waittime, emote, spell, textid1, textid2, textid3, textid4, textid5) VALUES ";

        outfile << "( ";
        outfile << fields[15].GetUInt32();                  // id
        outfile << ", ";
        outfile << fields[0].GetUInt32();                   // point
        outfile << ", ";
        outfile << fields[1].GetFloat();                    // position_x
        outfile << ", ";
        outfile << fields[2].GetFloat();                    // position_y
        outfile << ", ";
        outfile << fields[3].GetUInt32();                   // position_z
        outfile << ", ";
        outfile << fields[4].GetUInt32();                   // orientation
        outfile << ", ";
        outfile << fields[5].GetUInt32();                   // model1
        outfile << ", ";
        outfile << fields[6].GetUInt32();                   // model2
        outfile << ", ";
        outfile << fields[7].GetUInt16();                   // waittime
        outfile << ", ";
        outfile << fields[8].GetUInt32();                   // emote
        outfile << ", ";
        outfile << fields[9].GetUInt32();                   // spell
        outfile << ", ";
        outfile << fields[10].GetUInt32();                  // textid1
        outfile << ", ";
        outfile << fields[11].GetUInt32();                  // textid2
        outfile << ", ";
        outfile << fields[12].GetUInt32();                  // textid3
        outfile << ", ";
        outfile << fields[13].GetUInt32();                  // textid4
        outfile << ", ";
        outfile << fields[14].GetUInt32();                  // textid5
        outfile << ");\n ";

    } while (result->NextRow());
    delete result;

    PSendSysMessage(LANG_WAYPOINT_EXPORTED);
    outfile.close();

    return true;
}

bool ChatHandler::HandleWpImportCommand(char *args)
{
    if (!*args)
        return false;

    char* arg_str = strtok(args, " ");
    if (!arg_str)
        return false;

    std::string line;
    std::ifstream infile (arg_str);
    if (infile.is_open())
    {
        while (! infile.eof())
        {
            getline (infile,line);
            //cout << line << endl;
            QueryResult *result = WorldDatabase.Query(line.c_str());
            delete result;
        }
        infile.close();
    }
    PSendSysMessage(LANG_WAYPOINT_IMPORTED);

    return true;
}

//rename characters
bool ChatHandler::HandleCharacterRenameCommand(char* args)
{
    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&args, &target, &target_guid, &target_name))
        return false;

    if (target)
    {
        // check online security
        if (HasLowerSecurity(target))
            return false;

        PSendSysMessage(LANG_RENAME_PLAYER, GetNameLink(target).c_str());
        target->SetAtLoginFlag(AT_LOGIN_RENAME);
        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '1' WHERE guid = '%u'", target->GetGUIDLow());
    }
    else
    {
        // check offline security
        if (HasLowerSecurity(NULL, target_guid))
            return false;

        std::string oldNameLink = playerLink(target_name);

        PSendSysMessage(LANG_RENAME_PLAYER_GUID, oldNameLink.c_str(), target_guid.GetCounter());
        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '1' WHERE guid = '%u'", target_guid.GetCounter());
    }

    return true;
}

// customize characters
bool ChatHandler::HandleCharacterCustomizeCommand(char* args)
{
    Player* target;
    ObjectGuid target_guid;
    std::string target_name;
    if (!ExtractPlayerTarget(&args, &target, &target_guid, &target_name))
        return false;

    if (target)
    {
        PSendSysMessage(LANG_CUSTOMIZE_PLAYER, GetNameLink(target).c_str());
        target->SetAtLoginFlag(AT_LOGIN_CUSTOMIZE);
        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '8' WHERE guid = '%u'", target->GetGUIDLow());
    }
    else
    {
        std::string oldNameLink = playerLink(target_name);

        PSendSysMessage(LANG_CUSTOMIZE_PLAYER_GUID, oldNameLink.c_str(), target_guid.GetCounter());
        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '8' WHERE guid = '%u'", target_guid.GetCounter());
    }

    return true;
}

bool ChatHandler::HandleCharacterReputationCommand(char* args)
{
    Player* target;
    if (!ExtractPlayerTarget(&args, &target))
        return false;

    LocaleConstant loc = GetSessionDbcLocale();

    FactionStateList const& targetFSL = target->GetReputationMgr().GetStateList();
    for(FactionStateList::const_iterator itr = targetFSL.begin(); itr != targetFSL.end(); ++itr)
    {
        FactionEntry const *factionEntry = sFactionStore.LookupEntry(itr->second.ID);

        ShowFactionListHelper(factionEntry, loc, &itr->second, target);
    }
    return true;
}

//change standstate
bool ChatHandler::HandleModifyStandStateCommand(char* args)
{
    uint32 anim_id;
    if (!ExtractUInt32(&args, anim_id))
        return false;

    if (!sEmotesStore.LookupEntry(anim_id))
        return false;

    m_session->GetPlayer()->HandleEmoteState(anim_id);

    return true;
}

bool ChatHandler::HandleHonorAddCommand(char* args)
{
    if (!*args)
        return false;

    Player *target = getSelectedPlayer();
    if (!target)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(target))
        return false;

    float amount = (float)atof(args);
    target->RewardHonor(NULL, 1, amount);
    return true;
}

bool ChatHandler::HandleHonorAddKillCommand(char* /*args*/)
{
    Unit *target = getSelectedUnit();
    if (!target)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (target->GetTypeId() == TYPEID_PLAYER && HasLowerSecurity((Player*)target))
        return false;

    m_session->GetPlayer()->RewardHonor(target, 1);
    return true;
}

bool ChatHandler::HandleHonorUpdateCommand(char* /*args*/)
{
    Player *target = getSelectedPlayer();
    if (!target)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(target))
        return false;

    target->UpdateHonorFields();
    return true;
}

bool ChatHandler::HandleLookupEventCommand(char* args)
{
    if (!*args)
        return false;

    std::string namepart = args;
    std::wstring wnamepart;

    // converting string that we try to find to lower case
    if (!Utf8toWStr(namepart,wnamepart))
        return false;

    wstrToLower(wnamepart);

    uint32 counter = 0;

    GameEventMgr::GameEventDataMap const& events = sGameEventMgr.GetEventMap();
    GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr.GetActiveEventList();

    for(uint32 id = 0; id < events.size(); ++id)
    {
        GameEventData const& eventData = events[id];

        std::string descr = eventData.description;
        if (descr.empty())
            continue;

        if (Utf8FitTo(descr, wnamepart))
        {
            char const* active = activeEvents.find(id) != activeEvents.end() ? GetMangosString(LANG_ACTIVE) : "";

            if (m_session)
                PSendSysMessage(LANG_EVENT_ENTRY_LIST_CHAT, id, id, eventData.description.c_str(), active);
            else
                PSendSysMessage(LANG_EVENT_ENTRY_LIST_CONSOLE, id, eventData.description.c_str(), active);

            ++counter;
        }
    }

    if (counter==0)
        SendSysMessage(LANG_NOEVENTFOUND);

    return true;
}

bool ChatHandler::HandleEventListCommand(char* args)
{
    uint32 counter = 0;
    bool all = false;
    std::string arg = args;
    if (arg == "all")
        all = true;

    GameEventMgr::GameEventDataMap const& events = sGameEventMgr.GetEventMap();
    GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr.GetActiveEventList();

    char const* active = GetMangosString(LANG_ACTIVE);
    char const* inactive = GetMangosString(LANG_FACTION_INACTIVE);
    char const* state = "";

    for (uint32 event_id = 0; event_id < events.size(); ++event_id)
    {
        if (activeEvents.find(event_id) == activeEvents.end())
        {
            if (!all)
                continue;
            state = inactive;
        }
        else
            state = active;

        GameEventData const& eventData = events[event_id];

        if (m_session)
            PSendSysMessage(LANG_EVENT_ENTRY_LIST_CHAT, event_id, event_id, eventData.description.c_str(), state);
        else
            PSendSysMessage(LANG_EVENT_ENTRY_LIST_CONSOLE, event_id, eventData.description.c_str(), state);

        ++counter;
    }

    if (counter==0)
        SendSysMessage(LANG_NOEVENTFOUND);

    return true;
}

bool ChatHandler::HandleEventInfoCommand(char* args)
{
    if (!*args)
        return false;

    // id or [name] Shift-click form |color|Hgameevent:id|h[name]|h|r
    uint32 event_id;
    if (!ExtractUint32KeyFromLink(&args, "Hgameevent", event_id))
        return false;

    GameEventMgr::GameEventDataMap const& events = sGameEventMgr.GetEventMap();

    if (event_id >=events.size())
    {
        SendSysMessage(LANG_EVENT_NOT_EXIST);
        SetSentErrorMessage(true);
        return false;
    }

    GameEventData const& eventData = events[event_id];
    if (!eventData.isValid())
    {
        SendSysMessage(LANG_EVENT_NOT_EXIST);
        SetSentErrorMessage(true);
        return false;
    }

    GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr.GetActiveEventList();
    bool active = activeEvents.find(event_id) != activeEvents.end();
    char const* activeStr = active ? GetMangosString(LANG_ACTIVE) : "";

    std::string startTimeStr = TimeToTimestampStr(eventData.start);
    std::string endTimeStr = TimeToTimestampStr(eventData.end);

    uint32 delay = sGameEventMgr.NextCheck(event_id);
    time_t nextTime = time(NULL)+delay;
    std::string nextStr = nextTime >= eventData.start && nextTime < eventData.end ? TimeToTimestampStr(time(NULL)+delay) : "-";

    std::string occurenceStr = secsToTimeString(eventData.occurence * MINUTE);
    std::string lengthStr = secsToTimeString(eventData.length * MINUTE);

    PSendSysMessage(LANG_EVENT_INFO,event_id,eventData.description.c_str(),activeStr,
        startTimeStr.c_str(),endTimeStr.c_str(),occurenceStr.c_str(),lengthStr.c_str(),
        nextStr.c_str());
    return true;
}

bool ChatHandler::HandleEventStartCommand(char* args)
{
    if (!*args)
        return false;

    // id or [name] Shift-click form |color|Hgameevent:id|h[name]|h|r
    uint32 event_id;
    if (!ExtractUint32KeyFromLink(&args, "Hgameevent", event_id))
        return false;

    GameEventMgr::GameEventDataMap const& events = sGameEventMgr.GetEventMap();

    if (event_id < 1 || event_id >= events.size())
    {
        SendSysMessage(LANG_EVENT_NOT_EXIST);
        SetSentErrorMessage(true);
        return false;
    }

    GameEventData const& eventData = events[event_id];
    if (!eventData.isValid())
    {
        SendSysMessage(LANG_EVENT_NOT_EXIST);
        SetSentErrorMessage(true);
        return false;
    }

    GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr.GetActiveEventList();
    if (activeEvents.find(event_id) != activeEvents.end())
    {
        PSendSysMessage(LANG_EVENT_ALREADY_ACTIVE,event_id);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_EVENT_STARTED, event_id, eventData.description.c_str());
    sGameEventMgr.StartEvent(event_id,true);
    return true;
}

bool ChatHandler::HandleEventStopCommand(char* args)
{
    if (!*args)
        return false;

    // id or [name] Shift-click form |color|Hgameevent:id|h[name]|h|r
    uint32 event_id;
    if (!ExtractUint32KeyFromLink(&args, "Hgameevent", event_id))
        return false;

    GameEventMgr::GameEventDataMap const& events = sGameEventMgr.GetEventMap();

    if (event_id < 1 || event_id >= events.size())
    {
        SendSysMessage(LANG_EVENT_NOT_EXIST);
        SetSentErrorMessage(true);
        return false;
    }

    GameEventData const& eventData = events[event_id];
    if (!eventData.isValid())
    {
        SendSysMessage(LANG_EVENT_NOT_EXIST);
        SetSentErrorMessage(true);
        return false;
    }

    GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr.GetActiveEventList();

    if (activeEvents.find(event_id) == activeEvents.end())
    {
        PSendSysMessage(LANG_EVENT_NOT_ACTIVE,event_id);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_EVENT_STOPPED, event_id, eventData.description.c_str());
    sGameEventMgr.StopEvent(event_id,true);
    return true;
}

bool ChatHandler::HandleCombatStopCommand(char* args)
{
    Player* target;
    if (!ExtractPlayerTarget(&args, &target))
        return false;

    // check online security
    if (HasLowerSecurity(target))
        return false;

    target->CombatStop();
    target->getHostileRefManager().deleteReferences();
    return true;
}

void ChatHandler::HandleLearnSkillRecipesHelper(Player* player,uint32 skill_id)
{
    uint32 classmask = player->getClassMask();

    for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
    {
        SkillLineAbilityEntry const *skillLine = sSkillLineAbilityStore.LookupEntry(j);
        if (!skillLine)
            continue;

        // wrong skill
        if (skillLine->skillId != skill_id)
            continue;

        // not high rank
        if (skillLine->forward_spellid)
            continue;

        // skip racial skills
        if (skillLine->racemask != 0)
            continue;

        // skip wrong class skills
        if (skillLine->classmask && (skillLine->classmask & classmask) == 0)
            continue;

        SpellEntry const* spellInfo = sSpellStore.LookupEntry(skillLine->spellId);
        if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo,player,false))
            continue;

        player->learnSpell(skillLine->spellId, false);
    }
}

bool ChatHandler::HandleLearnAllCraftsCommand(char* /*args*/)
{
    for (uint32 i = 0; i < sSkillLineStore.GetNumRows(); ++i)
    {
        SkillLineEntry const *skillInfo = sSkillLineStore.LookupEntry(i);
        if (!skillInfo)
            continue;

        if ((skillInfo->categoryId == SKILL_CATEGORY_PROFESSION || skillInfo->categoryId == SKILL_CATEGORY_SECONDARY) &&
            skillInfo->canLink)                             // only prof. with recipes have
        {
            HandleLearnSkillRecipesHelper(m_session->GetPlayer(),skillInfo->id);
        }
    }

    SendSysMessage(LANG_COMMAND_LEARN_ALL_CRAFT);
    return true;
}

bool ChatHandler::HandleLearnAllRecipesCommand(char* args)
{
    //  Learns all recipes of specified profession and sets skill to max
    //  Example: .learn all_recipes enchanting

    Player* target = getSelectedPlayer();
    if (!target)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        return false;
    }

    if (!*args)
        return false;

    std::wstring wnamepart;

    if (!Utf8toWStr(args, wnamepart))
        return false;

    // converting string that we try to find to lower case
    wstrToLower(wnamepart);

    std::string name;

    SkillLineEntry const *targetSkillInfo = NULL;
    for (uint32 i = 1; i < sSkillLineStore.GetNumRows(); ++i)
    {
        SkillLineEntry const *skillInfo = sSkillLineStore.LookupEntry(i);
        if (!skillInfo)
            continue;

        if ((skillInfo->categoryId != SKILL_CATEGORY_PROFESSION &&
            skillInfo->categoryId != SKILL_CATEGORY_SECONDARY) ||
            !skillInfo->canLink)                            // only prof with recipes have set
            continue;

        int loc = GetSessionDbcLocale();
        name = skillInfo->name[loc];
        if (name.empty())
            continue;

        if (!Utf8FitTo(name, wnamepart))
        {
            loc = 0;
            for(; loc < MAX_LOCALE; ++loc)
            {
                if (loc==GetSessionDbcLocale())
                    continue;

                name = skillInfo->name[loc];
                if (name.empty())
                    continue;

                if (Utf8FitTo(name, wnamepart))
                    break;
            }
        }

        if (loc < MAX_LOCALE)
        {
            targetSkillInfo = skillInfo;
            break;
        }
    }

    if (!targetSkillInfo)
        return false;

    HandleLearnSkillRecipesHelper(target,targetSkillInfo->id);

    uint16 maxLevel = target->GetPureMaxSkillValue(targetSkillInfo->id);
    target->SetSkill(targetSkillInfo->id, maxLevel, maxLevel);
    PSendSysMessage(LANG_COMMAND_LEARN_ALL_RECIPES, name.c_str());
    return true;
}

bool ChatHandler::HandleLookupAccountEmailCommand(char* args)
{
    char* emailStr = ExtractQuotedOrLiteralArg(&args);
    if (!emailStr)
        return false;

    uint32 limit;
    if (!ExtractOptUInt32(&args, limit, 100))
        return false;

    std::string email = emailStr;
    LoginDatabase.escape_string(email);
    //                                                 0   1         2        3        4
    QueryResult *result = LoginDatabase.PQuery("SELECT id, username, last_ip, gmlevel, expansion FROM account WHERE email "_LIKE_" "_CONCAT3_("'%%'","'%s'","'%%'"), email.c_str ());

    return ShowAccountListHelper(result, &limit);
}

bool ChatHandler::HandleLookupAccountIpCommand(char* args)
{
    char* ipStr = ExtractQuotedOrLiteralArg(&args);
    if (!ipStr)
        return false;

    uint32 limit;
    if (!ExtractOptUInt32(&args, limit, 100))
        return false;

    std::string ip = ipStr;
    LoginDatabase.escape_string(ip);

    //                                                 0   1         2        3        4
    QueryResult *result = LoginDatabase.PQuery("SELECT id, username, last_ip, gmlevel, expansion FROM account WHERE last_ip "_LIKE_" "_CONCAT3_("'%%'","'%s'","'%%'"), ip.c_str ());

    return ShowAccountListHelper(result,&limit);
}

bool ChatHandler::HandleLookupAccountNameCommand(char* args)
{
    char* accountStr = ExtractQuotedOrLiteralArg(&args);
    if (!accountStr)
        return false;

    uint32 limit;
    if (!ExtractOptUInt32(&args, limit, 100))
        return false;

    std::string account = accountStr;
    if (!AccountMgr::normalizeString(account))
        return false;

    LoginDatabase.escape_string(account);
    //                                                 0   1         2        3        4
    QueryResult *result = LoginDatabase.PQuery("SELECT id, username, last_ip, gmlevel, expansion FROM account WHERE username "_LIKE_" "_CONCAT3_("'%%'","'%s'","'%%'"), account.c_str ());

    return ShowAccountListHelper(result, &limit);
}

bool ChatHandler::ShowAccountListHelper(QueryResult* result, uint32* limit, bool title, bool error)
{
    if (!result)
    {
        if (error)
            SendSysMessage(LANG_ACCOUNT_LIST_EMPTY);
        return true;
    }

    ///- Display the list of account/characters online
    if (!m_session && title)                                // not output header for online case
    {
        SendSysMessage(LANG_ACCOUNT_LIST_BAR);
        SendSysMessage(LANG_ACCOUNT_LIST_HEADER);
        SendSysMessage(LANG_ACCOUNT_LIST_BAR);
    }

    ///- Circle through accounts
    do
    {
        // check limit
        if (limit)
        {
            if (*limit == 0)
                break;
            --*limit;
        }

        Field *fields = result->Fetch();
        uint32 account = fields[0].GetUInt32();

        WorldSession* session = sWorld.FindSession(account);
        Player* player = session ? session->GetPlayer() : NULL;
        char const* char_name = player ? player->GetName() : " - ";

        if (m_session)
            PSendSysMessage(LANG_ACCOUNT_LIST_LINE_CHAT,
            account,fields[1].GetString(),char_name,fields[2].GetString(),fields[3].GetUInt32(),fields[4].GetUInt32());
        else
            PSendSysMessage(LANG_ACCOUNT_LIST_LINE_CONSOLE,
            account,fields[1].GetString(),char_name,fields[2].GetString(),fields[3].GetUInt32(),fields[4].GetUInt32());

    }while(result->NextRow());

    delete result;

    if (!m_session)                                         // not output header for online case
        SendSysMessage(LANG_ACCOUNT_LIST_BAR);

    return true;
}

bool ChatHandler::HandleLookupPlayerIpCommand(char* args)
{
    char* ipStr = ExtractQuotedOrLiteralArg(&args);
    if (!ipStr)
        return false;

    uint32 limit;
    if (!ExtractOptUInt32(&args, limit, 100))
        return false;

    std::string ip = ipStr;
    LoginDatabase.escape_string(ip);

    QueryResult* result = LoginDatabase.PQuery ("SELECT id,username FROM account WHERE last_ip "_LIKE_" "_CONCAT3_("'%%'","'%s'","'%%'"), ip.c_str ());

    return LookupPlayerSearchCommand(result, &limit);
}

bool ChatHandler::HandleLookupPlayerAccountCommand(char* args)
{
    char* accountStr = ExtractQuotedOrLiteralArg(&args);
    if (!accountStr)
        return false;

    uint32 limit;
    if (!ExtractOptUInt32(&args, limit, 100))
        return false;

    std::string account = accountStr;
    if (!AccountMgr::normalizeString(account))
        return false;

    LoginDatabase.escape_string(account);

    QueryResult* result = LoginDatabase.PQuery("SELECT id,username FROM account WHERE username "_LIKE_" "_CONCAT3_("'%%'","'%s'","'%%'"), account.c_str ());

    return LookupPlayerSearchCommand(result, &limit);
}

bool ChatHandler::HandleLookupPlayerEmailCommand(char* args)
{
    char* emailStr = ExtractQuotedOrLiteralArg(&args);
    if (!emailStr)
        return false;

    uint32 limit;
    if (!ExtractOptUInt32(&args, limit, 100))
        return false;

    std::string email = emailStr;
    LoginDatabase.escape_string(email);

    QueryResult* result = LoginDatabase.PQuery("SELECT id,username FROM account WHERE email "_LIKE_" "_CONCAT3_("'%%'","'%s'","'%%'"), email.c_str ());

    return LookupPlayerSearchCommand(result, &limit);
}

bool ChatHandler::LookupPlayerSearchCommand(QueryResult* result, uint32* limit)
{
    if (!result)
    {
        PSendSysMessage(LANG_NO_PLAYERS_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 limit_original = limit ? *limit : 100;

    uint32 limit_local = limit_original;

    if (!limit)
        limit = &limit_local;

    do
    {
        if (limit && *limit == 0)
            break;

        Field* fields = result->Fetch();
        uint32 acc_id = fields[0].GetUInt32();
        std::string acc_name = fields[1].GetCppString();

        ///- Get the characters for account id
        QueryResult *chars = CharacterDatabase.PQuery("SELECT guid, name, race, class, level FROM characters WHERE account = %u", acc_id);
        if (chars)
        {
            if (chars->GetRowCount())
            {
                PSendSysMessage(LANG_LOOKUP_PLAYER_ACCOUNT,acc_name.c_str(),acc_id);
                ShowPlayerListHelper(chars,limit,true,false);
            }
            else
                delete chars;
        }
    } while(result->NextRow());

    delete result;

    if (*limit == limit_original)                           // empty accounts only
    {
        PSendSysMessage(LANG_NO_PLAYERS_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    return true;
}

/// Triggering corpses expire check in world
bool ChatHandler::HandleServerCorpsesCommand(char* /*args*/)
{
    sObjectAccessor.RemoveOldCorpses();
    return true;
}

bool ChatHandler::HandleRepairitemsCommand(char* args)
{
    Player* target;
    if (!ExtractPlayerTarget(&args, &target))
        return false;

    // check online security
    if (HasLowerSecurity(target))
        return false;

    // Repair items
    target->DurabilityRepairAll(false, 0, false);

    PSendSysMessage(LANG_YOU_REPAIR_ITEMS, GetNameLink(target).c_str());
    if (needReportToTarget(target))
        ChatHandler(target).PSendSysMessage(LANG_YOUR_ITEMS_REPAIRED, GetNameLink().c_str());
    return true;
}

bool ChatHandler::HandleWaterwalkCommand(char* args)
{
    bool value;
    if (!ExtractOnOff(&args, value))
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    Player *player = getSelectedPlayer();

    if (!player)
    {
        PSendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(player))
        return false;

    if (value)
        player->SetMovement(MOVE_WATER_WALK);               // ON
    else
        player->SetMovement(MOVE_LAND_WALK);                // OFF

    PSendSysMessage(LANG_YOU_SET_WATERWALK, args, GetNameLink(player).c_str());
    if (needReportToTarget(player))
        ChatHandler(player).PSendSysMessage(LANG_YOUR_WATERWALK_SET, args, GetNameLink().c_str());
    return true;
}

bool ChatHandler::HandleLookupTitleCommand(char* args)
{
    if (!*args)
        return false;

    // can be NULL in console call
    Player* target = getSelectedPlayer();

    // title name have single string arg for player name
    char const* targetName = target ? target->GetName() : "NAME";

    std::string namepart = args;
    std::wstring wnamepart;

    if (!Utf8toWStr(namepart, wnamepart))
        return false;

    // converting string that we try to find to lower case
    wstrToLower(wnamepart);

    uint32 counter = 0;                                     // Counter for figure out that we found smth.

    // Search in CharTitles.dbc
    for (uint32 id = 0; id < sCharTitlesStore.GetNumRows(); id++)
    {
        CharTitlesEntry const *titleInfo = sCharTitlesStore.LookupEntry(id);
        if (titleInfo)
        {
            int loc = GetSessionDbcLocale();
            std::string name = titleInfo->name[loc];
            if (name.empty())
                continue;

            if (!Utf8FitTo(name, wnamepart))
            {
                loc = 0;
                for(; loc < MAX_LOCALE; ++loc)
                {
                    if (loc == GetSessionDbcLocale())
                        continue;

                    name = titleInfo->name[loc];
                    if (name.empty())
                        continue;

                    if (Utf8FitTo(name, wnamepart))
                        break;
                }
            }

            if (loc < MAX_LOCALE)
            {
                char const* knownStr = target && target->HasTitle(titleInfo) ? GetMangosString(LANG_KNOWN) : "";

                char const* activeStr = target && target->GetUInt32Value(PLAYER_CHOSEN_TITLE)==titleInfo->bit_index
                    ? GetMangosString(LANG_ACTIVE)
                    : "";

                char titleNameStr[80];
                snprintf(titleNameStr,80,name.c_str(),targetName);

                // send title in "id (idx:idx) - [namedlink locale]" format
                if (m_session)
                    PSendSysMessage(LANG_TITLE_LIST_CHAT,id,titleInfo->bit_index,id,titleNameStr,localeNames[loc],knownStr,activeStr);
                else
                    PSendSysMessage(LANG_TITLE_LIST_CONSOLE,id,titleInfo->bit_index,titleNameStr,localeNames[loc],knownStr,activeStr);

                ++counter;
            }
        }
    }
    if (counter == 0)                                       // if counter == 0 then we found nth
        SendSysMessage(LANG_COMMAND_NOTITLEFOUND);
    return true;
}

bool ChatHandler::HandleTitlesAddCommand(char* args)
{
    // number or [name] Shift-click form |color|Htitle:title_id|h[name]|h|r
    uint32 id;
    if (!ExtractUint32KeyFromLink(&args, "Htitle", id))
        return false;

    if (id <= 0)
    {
        PSendSysMessage(LANG_INVALID_TITLE_ID, id);
        SetSentErrorMessage(true);
        return false;
    }

    Player * target = getSelectedPlayer();
    if (!target)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(target))
        return false;

    CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(id);
    if (!titleInfo)
    {
        PSendSysMessage(LANG_INVALID_TITLE_ID, id);
        SetSentErrorMessage(true);
        return false;
    }

    std::string tNameLink = GetNameLink(target);

    char const* targetName = target->GetName();
    char titleNameStr[80];
    snprintf(titleNameStr,80,titleInfo->name[GetSessionDbcLocale()],targetName);

    target->SetTitle(titleInfo);
    PSendSysMessage(LANG_TITLE_ADD_RES, id, titleNameStr, tNameLink.c_str());

    return true;
}

bool ChatHandler::HandleTitlesRemoveCommand(char* args)
{
    // number or [name] Shift-click form |color|Htitle:title_id|h[name]|h|r
    uint32 id;
    if (!ExtractUint32KeyFromLink(&args, "Htitle", id))
        return false;

    if (id <= 0)
    {
        PSendSysMessage(LANG_INVALID_TITLE_ID, id);
        SetSentErrorMessage(true);
        return false;
    }

    Player * target = getSelectedPlayer();
    if (!target)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(target))
        return false;

    CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(id);
    if (!titleInfo)
    {
        PSendSysMessage(LANG_INVALID_TITLE_ID, id);
        SetSentErrorMessage(true);
        return false;
    }

    target->SetTitle(titleInfo,true);

    std::string tNameLink = GetNameLink(target);

    char const* targetName = target->GetName();
    char titleNameStr[80];
    snprintf(titleNameStr,80,titleInfo->name[GetSessionDbcLocale()],targetName);

    PSendSysMessage(LANG_TITLE_REMOVE_RES, id, titleNameStr, tNameLink.c_str());

    if (!target->HasTitle(target->GetInt32Value(PLAYER_CHOSEN_TITLE)))
    {
        target->SetUInt32Value(PLAYER_CHOSEN_TITLE,0);
        PSendSysMessage(LANG_CURRENT_TITLE_RESET, tNameLink.c_str());
    }

    return true;
}

//Edit Player KnownTitles
bool ChatHandler::HandleTitlesSetMaskCommand(char* args)
{
    if (!*args)
        return false;

    uint64 titles = 0;

    sscanf(args, UI64FMTD, &titles);

    Player *target = getSelectedPlayer();
    if (!target)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(target))
        return false;

    uint64 titles2 = titles;

    for(uint32 i = 1; i < sCharTitlesStore.GetNumRows(); ++i)
        if (CharTitlesEntry const* tEntry = sCharTitlesStore.LookupEntry(i))
            titles2 &= ~(uint64(1) << tEntry->bit_index);

    titles &= ~titles2;                                     // remove nonexistent titles

    target->SetUInt64Value(PLAYER__FIELD_KNOWN_TITLES, titles);
    SendSysMessage(LANG_DONE);

    if (!target->HasTitle(target->GetInt32Value(PLAYER_CHOSEN_TITLE)))
    {
        target->SetUInt32Value(PLAYER_CHOSEN_TITLE, 0);
        PSendSysMessage(LANG_CURRENT_TITLE_RESET, GetNameLink(target).c_str());
    }

    return true;
}

bool ChatHandler::HandleCharacterTitlesCommand(char* args)
{
    Player* target;
    if (!ExtractPlayerTarget(&args, &target))
        return false;

    LocaleConstant loc = GetSessionDbcLocale();
    char const* targetName = target->GetName();
    char const* knownStr = GetMangosString(LANG_KNOWN);

    // Search in CharTitles.dbc
    for (uint32 id = 0; id < sCharTitlesStore.GetNumRows(); id++)
    {
        CharTitlesEntry const *titleInfo = sCharTitlesStore.LookupEntry(id);
        if (titleInfo && target->HasTitle(titleInfo))
        {
            std::string name = titleInfo->name[loc];
            if (name.empty())
                continue;

            char const* activeStr = target && target->GetUInt32Value(PLAYER_CHOSEN_TITLE) == titleInfo->bit_index
                ? GetMangosString(LANG_ACTIVE)
                : "";

            char titleNameStr[80];
            snprintf(titleNameStr,80,name.c_str(),targetName);

            // send title in "id (idx:idx) - [namedlink locale]" format
            if (m_session)
                PSendSysMessage(LANG_TITLE_LIST_CHAT, id, titleInfo->bit_index, id, titleNameStr, localeNames[loc], knownStr, activeStr);
            else
                PSendSysMessage(LANG_TITLE_LIST_CONSOLE, id, titleInfo->bit_index, name.c_str(), localeNames[loc], knownStr, activeStr);
        }
    }
    return true;
}

bool ChatHandler::HandleTitlesCurrentCommand(char* args)
{
    // number or [name] Shift-click form |color|Htitle:title_id|h[name]|h|r
    uint32 id;
    if (!ExtractUint32KeyFromLink(&args, "Htitle", id))
        return false;

    if (id <= 0)
    {
        PSendSysMessage(LANG_INVALID_TITLE_ID, id);
        SetSentErrorMessage(true);
        return false;
    }

    Player * target = getSelectedPlayer();
    if (!target)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // check online security
    if (HasLowerSecurity(target))
        return false;

    CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(id);
    if (!titleInfo)
    {
        PSendSysMessage(LANG_INVALID_TITLE_ID, id);
        SetSentErrorMessage(true);
        return false;
    }

    std::string tNameLink = GetNameLink(target);

    target->SetTitle(titleInfo);                            // to be sure that title now known
    target->SetUInt32Value(PLAYER_CHOSEN_TITLE,titleInfo->bit_index);

    PSendSysMessage(LANG_TITLE_CURRENT_RES, id, titleInfo->name[GetSessionDbcLocale()], tNameLink.c_str());

    return true;
}
