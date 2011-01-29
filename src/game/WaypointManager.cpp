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

#include "WaypointManager.h"
#include "Database/DatabaseEnv.h"
#include "GridDefines.h"
#include "Policies/SingletonImp.h"
#include "ProgressBar.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"

INSTANTIATE_SINGLETON_1(WaypointManager);

bool WaypointBehavior::isEmpty()
{
    if (emote || spell || model1 || model2)
        return false;

    for(int i = 0; i < MAX_WAYPOINT_TEXT; ++i)
        if(textid[i])
            return false;

    return true;
}

WaypointBehavior::WaypointBehavior(const WaypointBehavior &b)
{
    emote = b.emote;
    spell = b.spell;
    model1 = b.model1;
    model2 = b.model2;
    for(int i=0; i < MAX_WAYPOINT_TEXT; ++i)
        textid[i] = b.textid[i];
}

void WaypointManager::Load()
{
    Cleanup();

    uint32 total_paths = 0;
    uint32 total_nodes = 0;
    uint32 total_behaviors = 0;

    std::set<uint32> movementScriptSet;

    for(ScriptMapMap::const_iterator itr = sCreatureMovementScripts.begin(); itr != sCreatureMovementScripts.end(); ++itr)
        movementScriptSet.insert(itr->first);

    // creature_movement
    QueryResult *result = WorldDatabase.Query("SELECT id, COUNT(point) FROM creature_movement GROUP BY id");

    if (!result)
    {
        barGoLink bar(1);
        bar.step();
        sLog.outString();
        sLog.outString( ">> Loaded 0 paths. DB table `creature_movement` is empty." );
    }
    else
    {
        total_paths = (uint32)result->GetRowCount();
        barGoLink bar( total_paths );

        do
        {
            bar.step();
            Field *fields   = result->Fetch();

            uint32 id       = fields[0].GetUInt32();
            uint32 count    = fields[1].GetUInt32();

            m_pathMap[id].resize(count);
            total_nodes += count;
        }
        while(result->NextRow());

        sLog.outString();
        sLog.outString( ">> Paths loaded" );

        delete result;

        //                                   0   1      2           3           4           5         6
        result = WorldDatabase.Query("SELECT id, point, position_x, position_y, position_z, waittime, script_id,"
        //   7        8        9        10       11       12     13     14           15      16
            "textid1, textid2, textid3, textid4, textid5, emote, spell, orientation, model1, model2 FROM creature_movement");

        barGoLink barRow((int)result->GetRowCount());

        // error after load, we check if creature guid corresponding to the path id has proper MovementType
        std::set<uint32> creatureNoMoveType;

        do
        {
            barRow.step();
            Field *fields = result->Fetch();
            uint32 id           = fields[0].GetUInt32();
            uint32 point        = fields[1].GetUInt32();

            const CreatureData* cData = sObjectMgr.GetCreatureData(id);

            if (!cData)
            {
                sLog.outErrorDb("Table creature_movement contain path for creature guid %u, but this creature guid does not exist. Skipping.", id);
                continue;
            }

            if (cData->movementType != WAYPOINT_MOTION_TYPE)
                creatureNoMoveType.insert(id);

            WaypointPath &path  = m_pathMap[id];

            // the cleanup queries make sure the following is true
            MANGOS_ASSERT(point >= 1 && point <= path.size());

            WaypointNode &node  = path[point-1];

            node.x              = fields[2].GetFloat();
            node.y              = fields[3].GetFloat();
            node.z              = fields[4].GetFloat();
            node.orientation    = fields[14].GetFloat();
            node.delay          = fields[5].GetUInt32();
            node.script_id      = fields[6].GetUInt32();

            // prevent using invalid coordinates
            if (!MaNGOS::IsValidMapCoord(node.x, node.y, node.z, node.orientation))
            {
                QueryResult *result1 = WorldDatabase.PQuery("SELECT id, map FROM creature WHERE guid = '%u'", id);
                if (result1)
                    sLog.outErrorDb("Creature (guidlow %d, entry %d) have invalid coordinates in his waypoint %d (X: %f, Y: %f).",
                        id, result1->Fetch()[0].GetUInt32(), point, node.x, node.y);
                else
                    sLog.outErrorDb("Waypoint path %d, have invalid coordinates in his waypoint %d (X: %f, Y: %f).",
                        id, point, node.x, node.y);

                MaNGOS::NormalizeMapCoord(node.x);
                MaNGOS::NormalizeMapCoord(node.y);

                if (result1)
                {
                    node.z = sTerrainMgr.LoadTerrain(result1->Fetch()[1].GetUInt32())->GetHeight(node.x, node.y, node.z);
                    delete result1;
                }

                WorldDatabase.PExecute("UPDATE creature_movement SET position_x = '%f', position_y = '%f', position_z = '%f' WHERE id = '%u' AND point = '%u'", node.x, node.y, node.z, id, point);
            }

            if (node.script_id)
            {
                if (sCreatureMovementScripts.find(node.script_id) == sCreatureMovementScripts.end())
                {
                    sLog.outErrorDb("Table creature_movement for id %u, point %u have script_id %u that does not exist in `creature_movement_scripts`, ignoring", id, point, node.script_id);
                    continue;
                }

                movementScriptSet.erase(node.script_id);
            }

            // WaypointBehavior can be dropped in time. Script_id added may 2010 and can handle all the below behavior.

            WaypointBehavior be;
            be.model1           = fields[15].GetUInt32();
            be.model2           = fields[16].GetUInt32();
            be.emote            = fields[12].GetUInt32();
            be.spell            = fields[13].GetUInt32();

            for(int i = 0; i < MAX_WAYPOINT_TEXT; ++i)
            {
                be.textid[i]    = fields[7+i].GetUInt32();

                if (be.textid[i])
                {
                    if (be.textid[i] < MIN_DB_SCRIPT_STRING_ID || be.textid[i] >= MAX_DB_SCRIPT_STRING_ID)
                    {
                        sLog.outErrorDb( "Table `db_script_string` not have string id  %u", be.textid[i]);
                        continue;
                    }
                }
            }

            if (be.spell && ! sSpellStore.LookupEntry(be.spell))
            {
                sLog.outErrorDb("Table creature_movement references unknown spellid %u. Skipping id %u with point %u.", be.spell, id, point);
                be.spell = 0;
            }

            if (be.emote)
            {
                if (!sEmotesStore.LookupEntry(be.emote))
                    sLog.outErrorDb("Waypoint path %u (Point %u) are using emote %u, but emote does not exist.",id, point, be.emote);
            }

            // save memory by not storing empty behaviors
            if (!be.isEmpty())
            {
                node.behavior = new WaypointBehavior(be);
                ++total_behaviors;
            }
            else
                node.behavior = NULL;
        }
        while(result->NextRow());

        if (!creatureNoMoveType.empty())
        {
            for(std::set<uint32>::const_iterator itr = creatureNoMoveType.begin(); itr != creatureNoMoveType.end(); ++itr)
            {
                const CreatureData* cData = sObjectMgr.GetCreatureData(*itr);
                const CreatureInfo* cInfo = ObjectMgr::GetCreatureTemplate(cData->id);

                sLog.outErrorDb("Table creature_movement has waypoint for creature guid %u (entry %u), but MovementType is not WAYPOINT_MOTION_TYPE(2). Creature will not use this path.", *itr, cData->id);

                if (cInfo->MovementType == WAYPOINT_MOTION_TYPE)
                    sLog.outErrorDb("    creature_template for this entry has MovementType WAYPOINT_MOTION_TYPE(2), did you intend to use creature_movement_template ?");
            }
        }

        sLog.outString();
        sLog.outString( ">> Waypoints and behaviors loaded" );
        sLog.outString();
        sLog.outString( ">>> Loaded %u paths, %u nodes and %u behaviors", total_paths, total_nodes, total_behaviors);

        delete result;
    }

    // creature_movement_template
    result = WorldDatabase.Query("SELECT entry, COUNT(point) FROM creature_movement_template GROUP BY entry");

    if (!result)
    {
        barGoLink bar(1);
        bar.step();
        sLog.outString();
        sLog.outString( ">> Loaded 0 path templates. DB table `creature_movement_template` is empty." );
    }
    else
    {
        total_nodes = 0;
        total_behaviors = 0;
        total_paths = (uint32)result->GetRowCount();
        barGoLink barRow(total_paths);

        do
        {
            barRow.step();
            Field *fields = result->Fetch();

            uint32 entry    = fields[0].GetUInt32();
            uint32 count    = fields[1].GetUInt32();

            m_pathTemplateMap[entry].resize(count);
            total_nodes += count;
        }
        while(result->NextRow());

        delete result;

        sLog.outString();
        sLog.outString(">> Path templates loaded");

        //                                   0      1      2           3           4           5         6
        result = WorldDatabase.Query("SELECT entry, point, position_x, position_y, position_z, waittime, script_id,"
        //   7        8        9        10       11       12     13     14           15      16
            "textid1, textid2, textid3, textid4, textid5, emote, spell, orientation, model1, model2 FROM creature_movement_template");

        barGoLink bar( (int)result->GetRowCount() );

        do
        {
            bar.step();
            Field *fields = result->Fetch();

            uint32 entry        = fields[0].GetUInt32();
            uint32 point        = fields[1].GetUInt32();

            const CreatureInfo* cInfo = ObjectMgr::GetCreatureTemplate(entry);

            if (!cInfo)
            {
                sLog.outErrorDb("Table creature_movement_template references unknown creature template %u. Skipping.", entry);
                continue;
            }

            WaypointPath &path  = m_pathTemplateMap[entry];

            // the cleanup queries make sure the following is true
            MANGOS_ASSERT(point >= 1 && point <= path.size());

            WaypointNode &node  = path[point-1];

            node.x              = fields[2].GetFloat();
            node.y              = fields[3].GetFloat();
            node.z              = fields[4].GetFloat();
            node.orientation    = fields[14].GetFloat();
            node.delay          = fields[5].GetUInt32();
            node.script_id      = fields[6].GetUInt32();

            // prevent using invalid coordinates
            if (!MaNGOS::IsValidMapCoord(node.x, node.y, node.z, node.orientation))
            {
                sLog.outErrorDb("Table creature_movement_template for entry %u (point %u) are using invalid coordinates position_x: %f, position_y: %f)",
                    entry, point, node.x, node.y);

                MaNGOS::NormalizeMapCoord(node.x);
                MaNGOS::NormalizeMapCoord(node.y);

                sLog.outErrorDb("Table creature_movement_template for entry %u (point %u) are auto corrected to normalized position_x=%f, position_y=%f",
                    entry, point, node.x, node.y);

                WorldDatabase.PExecute("UPDATE creature_movement_template SET position_x = '%f', position_y = '%f' WHERE entry = %u AND point = %u", node.x, node.y, entry, point);
            }

            if (node.script_id)
            {
                if (sCreatureMovementScripts.find(node.script_id) == sCreatureMovementScripts.end())
                {
                    sLog.outErrorDb("Table creature_movement_template for entry %u, point %u have script_id %u that does not exist in `creature_movement_scripts`, ignoring", entry, point, node.script_id);
                    continue;
                }

                movementScriptSet.erase(node.script_id);
            }

            WaypointBehavior be;
            be.model1           = fields[15].GetUInt32();
            be.model2           = fields[16].GetUInt32();
            be.emote            = fields[12].GetUInt32();
            be.spell            = fields[13].GetUInt32();

            for(int i = 0; i < MAX_WAYPOINT_TEXT; ++i)
            {
                be.textid[i]    = fields[7+i].GetUInt32();

                if (be.textid[i])
                {
                    if (be.textid[i] < MIN_DB_SCRIPT_STRING_ID || be.textid[i] >= MAX_DB_SCRIPT_STRING_ID)
                    {
                        sLog.outErrorDb( "Table `db_script_string` not have string id %u", be.textid[i]);
                        continue;
                    }
                }
            }

            if (be.spell && ! sSpellStore.LookupEntry(be.spell))
            {
                sLog.outErrorDb("Table creature_movement_template references unknown spellid %u. Skipping id %u with point %u.", be.spell, entry, point);
                be.spell = 0;
            }

            if (be.emote)
            {
                if (!sEmotesStore.LookupEntry(be.emote))
                    sLog.outErrorDb("Waypoint template path %u (point %u) are using emote %u, but emote does not exist.", entry, point, be.emote);
            }

            // save memory by not storing empty behaviors
            if (!be.isEmpty())
            {
                node.behavior   = new WaypointBehavior(be);
                ++total_behaviors;
            }
            else
                node.behavior   = NULL;
        }
        while(result->NextRow());

        delete result;

        sLog.outString();
        sLog.outString( ">> Waypoint templates loaded" );
        sLog.outString();
        sLog.outString( ">>> Loaded %u path templates with %u nodes and %u behaviors", total_paths, total_nodes, total_behaviors);
    }

    if (!movementScriptSet.empty())
    {
        for(std::set<uint32>::const_iterator itr = movementScriptSet.begin(); itr != movementScriptSet.end(); ++itr)
            sLog.outErrorDb("Table `creature_movement_scripts` contain unused script, id %u.", *itr);
    }
}

void WaypointManager::Cleanup()
{
    // check if points need to be renumbered and do it
    if (QueryResult *result = WorldDatabase.Query("SELECT 1 from creature_movement As T WHERE point <> (SELECT COUNT(*) FROM creature_movement WHERE id = T.id AND point <= T.point) LIMIT 1"))
    {
        delete result;
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE temp LIKE creature_movement");
        WorldDatabase.DirectExecute("INSERT INTO temp SELECT * FROM creature_movement");
        WorldDatabase.DirectExecute("ALTER TABLE creature_movement DROP PRIMARY KEY");
        WorldDatabase.DirectExecute("UPDATE creature_movement AS T SET point = (SELECT COUNT(*) FROM temp WHERE id = T.id AND point <= T.point)");
        WorldDatabase.DirectExecute("ALTER TABLE creature_movement ADD PRIMARY KEY (id, point)");
        WorldDatabase.DirectExecute("DROP TABLE temp");

        sLog.outErrorDb("Table `creature_movement` was auto corrected for using points out of order (invalid or points missing)");

        MANGOS_ASSERT(!(result = WorldDatabase.Query("SELECT 1 from creature_movement As T WHERE point <> (SELECT COUNT(*) FROM creature_movement WHERE id = T.id AND point <= T.point) LIMIT 1")));
    }

    if (QueryResult *result = WorldDatabase.Query("SELECT 1 from creature_movement_template As T WHERE point <> (SELECT COUNT(*) FROM creature_movement_template WHERE entry = T.entry AND point <= T.point) LIMIT 1"))
    {
        delete result;
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE temp LIKE creature_movement_template");
        WorldDatabase.DirectExecute("INSERT INTO temp SELECT * FROM creature_movement_template");
        WorldDatabase.DirectExecute("ALTER TABLE creature_movement_template DROP PRIMARY KEY");
        WorldDatabase.DirectExecute("UPDATE creature_movement_template AS T SET point = (SELECT COUNT(*) FROM temp WHERE entry = T.entry AND point <= T.point)");
        WorldDatabase.DirectExecute("ALTER TABLE creature_movement_template ADD PRIMARY KEY (entry, point)");
        WorldDatabase.DirectExecute("DROP TABLE temp");

        sLog.outErrorDb("Table `creature_movement_template` was auto corrected for using points out of order (invalid or points missing)");

        MANGOS_ASSERT(!(result = WorldDatabase.Query("SELECT 1 from creature_movement_template As T WHERE point <> (SELECT COUNT(*) FROM creature_movement_template WHERE entry = T.entry AND point <= T.point) LIMIT 1")));
    }
}

void WaypointManager::Unload()
{
    for(WaypointPathMap::iterator itr = m_pathMap.begin(); itr != m_pathMap.end(); ++itr)
        _clearPath(itr->second);
    m_pathMap.clear();
}

void WaypointManager::_clearPath(WaypointPath &path)
{
    for(WaypointPath::const_iterator itr = path.begin(); itr != path.end(); ++itr)
        if(itr->behavior)
            delete itr->behavior;
    path.clear();
}

/// - Insert after the last point
void WaypointManager::AddLastNode(uint32 id, float x, float y, float z, float o, uint32 delay, uint32 wpGuid)
{
    _addNode(id, GetLastPoint(id, 0) + 1, x, y, z, o, delay, wpGuid);
}

/// - Insert after a certain point
void WaypointManager::AddAfterNode(uint32 id, uint32 point, float x, float y, float z, float o, uint32 delay, uint32 wpGuid)
{
    for(uint32 i = GetLastPoint(id, 0); i > point; i--)
        WorldDatabase.PExecuteLog("UPDATE creature_movement SET point=point+1 WHERE id=%u AND point=%u", id, i);

    _addNode(id, point + 1, x, y, z, o, delay, wpGuid);
}

/// - Insert without checking for collision
void WaypointManager::_addNode(uint32 id, uint32 point, float x, float y, float z, float o, uint32 delay, uint32 wpGuid)
{
    if(point == 0) return;                                  // counted from 1 in the DB
    WorldDatabase.PExecuteLog("INSERT INTO creature_movement (id,point,position_x,position_y,position_z,orientation,wpguid,waittime) "
        "VALUES (%u,%u, %f,%f,%f,%f, %u,%u)",
        id, point, x, y, z, o, wpGuid, delay);
    WaypointPathMap::iterator itr = m_pathMap.find(id);
    if(itr == m_pathMap.end())
        itr = m_pathMap.insert(WaypointPathMap::value_type(id, WaypointPath())).first;
    itr->second.insert(itr->second.begin() + (point - 1), WaypointNode(x, y, z, o, delay, 0, NULL));
}

uint32 WaypointManager::GetLastPoint(uint32 id, uint32 default_notfound)
{
    uint32 point = default_notfound;
    /*QueryResult *result = WorldDatabase.PQuery( "SELECT MAX(point) FROM creature_movement WHERE id = '%u'", id);
    if( result )
    {
        point = (*result)[0].GetUInt32()+1;
        delete result;
    }*/
    WaypointPathMap::const_iterator itr = m_pathMap.find(id);
    if(itr != m_pathMap.end() && itr->second.size() != 0)
        point = itr->second.size();
    return point;
}

void WaypointManager::DeleteNode(uint32 id, uint32 point)
{
    if(point == 0) return;                                  // counted from 1 in the DB
    WorldDatabase.PExecuteLog("DELETE FROM creature_movement WHERE id=%u AND point=%u", id, point);
    WorldDatabase.PExecuteLog("UPDATE creature_movement SET point=point-1 WHERE id=%u AND point>%u", id, point);
    WaypointPathMap::iterator itr = m_pathMap.find(id);
    if(itr != m_pathMap.end() && point <= itr->second.size())
        itr->second.erase(itr->second.begin() + (point-1));
}

void WaypointManager::DeletePath(uint32 id)
{
    WorldDatabase.PExecuteLog("DELETE FROM creature_movement WHERE id=%u", id);
    WaypointPathMap::iterator itr = m_pathMap.find(id);
    if(itr != m_pathMap.end())
        _clearPath(itr->second);
    // the path is not removed from the map, just cleared
    // WMGs have pointers to the path, so deleting them would crash
    // this wastes some memory, but these functions are
    // only meant to be called by GM commands
}

void WaypointManager::SetNodePosition(uint32 id, uint32 point, float x, float y, float z)
{
    if(point == 0) return;                                  // counted from 1 in the DB
    WorldDatabase.PExecuteLog("UPDATE creature_movement SET position_x=%f, position_y=%f, position_z=%f WHERE id=%u AND point=%u", x, y, z, id, point);
    WaypointPathMap::iterator itr = m_pathMap.find(id);
    if(itr != m_pathMap.end() && point <= itr->second.size())
    {
        itr->second[point-1].x = x;
        itr->second[point-1].y = y;
        itr->second[point-1].z = z;
    }
}

void WaypointManager::SetNodeText(uint32 id, uint32 point, const char *text_field, const char *text)
{
    if(point == 0) return;                                  // counted from 1 in the DB
    if(!text_field) return;
    std::string field = text_field;
    WorldDatabase.escape_string(field);

    if(!text)
    {
        WorldDatabase.PExecuteLog("UPDATE creature_movement SET %s=NULL WHERE id='%u' AND point='%u'", field.c_str(), id, point);
    }
    else
    {
        std::string text2 = text;
        WorldDatabase.escape_string(text2);
        WorldDatabase.PExecuteLog("UPDATE creature_movement SET %s='%s' WHERE id='%u' AND point='%u'", field.c_str(), text2.c_str(), id, point);
    }

    WaypointPathMap::iterator itr = m_pathMap.find(id);
    if(itr != m_pathMap.end() && point <= itr->second.size())
    {
        WaypointNode &node = itr->second[point-1];
        if(!node.behavior) node.behavior = new WaypointBehavior();

//        if(field == "text1") node.behavior->text[0] = text ? text : "";
//        if(field == "text2") node.behavior->text[1] = text ? text : "";
//        if(field == "text3") node.behavior->text[2] = text ? text : "";
//        if(field == "text4") node.behavior->text[3] = text ? text : "";
//        if(field == "text5") node.behavior->text[4] = text ? text : "";
        if(field == "emote") node.behavior->emote   = text ? atoi(text) : 0;
        if(field == "spell") node.behavior->spell   = text ? atoi(text) : 0;
        if(field == "model1") node.behavior->model1 = text ? atoi(text) : 0;
        if(field == "model2") node.behavior->model2 = text ? atoi(text) : 0;
    }
}

void WaypointManager::CheckTextsExistance(std::set<int32>& ids)
{
    WaypointPathMap::const_iterator pmItr = m_pathMap.begin();
    for ( ; pmItr != m_pathMap.end(); ++pmItr)
    {
        for (size_t i = 0; i < pmItr->second.size(); ++i)
        {
            WaypointBehavior* be = pmItr->second[i].behavior;
            if (!be)
                continue;

            // Now we check text existence and put all zero texts ids to the end of array

            // Counting leading zeros for futher textid shift
            int zeroCount = 0;
            for (int j = 0; j < MAX_WAYPOINT_TEXT; ++j)
            {
                if (!be->textid[j])
                {
                    ++zeroCount;
                    continue;
                }
                else
                {
                    if (!sObjectMgr.GetMangosStringLocale(be->textid[j]))
                    {
                        sLog.outErrorDb("Some waypoint has textid%u with not existing %u text.", j, be->textid[j]);
                        be->textid[j] = 0;
                        ++zeroCount;
                        continue;
                    }
                    else
                        ids.erase(be->textid[j]);

                    // Shifting check
                    if (zeroCount)
                    {
                        // Correct textid but some zeros leading, so move it forward.
                        be->textid[j-zeroCount] = be->textid[j];
                        be->textid[j] = 0;
                    }
                }
            }
        }
    }

    WaypointPathTemplateMap::const_iterator wptItr = m_pathTemplateMap.begin();
    for ( ; wptItr != m_pathTemplateMap.end(); ++wptItr)
    {
        for (size_t i = 0; i < wptItr->second.size(); ++i)
        {
            WaypointBehavior* be = wptItr->second[i].behavior;
            if (!be)
                continue;

            // Now we check text existence and put all zero texts ids to the end of array

            // Counting leading zeros for futher textid shift
            int zeroCount = 0;
            for (int j = 0; j < MAX_WAYPOINT_TEXT; ++j)
            {
                if (!be->textid[j])
                {
                    ++zeroCount;
                    continue;
                }
                else
                {
                    if (!sObjectMgr.GetMangosStringLocale(be->textid[j]))
                    {
                        sLog.outErrorDb("Some waypoint has textid%u with not existing %u text.", j, be->textid[j]);
                        be->textid[j] = 0;
                        ++zeroCount;
                        continue;
                    }
                    else
                        ids.erase(be->textid[j]);

                    // Shifting check
                    if (zeroCount)
                    {
                        // Correct textid but some zeros leading, so move it forward.
                        be->textid[j-zeroCount] = be->textid[j];
                        be->textid[j] = 0;
                    }
                }
            }
        }
    }
}
