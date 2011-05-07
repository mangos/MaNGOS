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

/*
creature_movement Table

alter table creature_movement add `textid1` int(11) NOT NULL default '0';
alter table creature_movement add `textid2` int(11) NOT NULL default '0';
alter table creature_movement add `textid3` int(11) NOT NULL default '0';
alter table creature_movement add `textid4` int(11) NOT NULL default '0';
alter table creature_movement add `textid5` int(11) NOT NULL default '0';
alter table creature_movement add `emote` int(10) unsigned default '0';
alter table creature_movement add `spell` int(5) unsigned default '0';
alter table creature_movement add `wpguid` int(11) default '0';

*/

#include <ctime>

#include "WaypointMovementGenerator.h"
#include "ObjectMgr.h"
#include "Creature.h"
#include "DestinationHolderImp.h"
#include "CreatureAI.h"
#include "WaypointManager.h"
#include "WorldPacket.h"
#include "ScriptMgr.h"

#include <cassert>

//-----------------------------------------------//
void WaypointMovementGenerator<Creature>::LoadPath(Creature &creature)
{
    DETAIL_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "LoadPath: loading waypoint path for %s", creature.GetGuidStr().c_str());

    i_path = sWaypointMgr.GetPath(creature.GetGUIDLow());

    // We may LoadPath() for several occasions:

    // 1: When creature.MovementType=2
    //    1a) Path is selected by creature.guid == creature_movement.id
    //    1b) Path for 1a) does not exist and then use path from creature.GetEntry() == creature_movement_template.entry

    // 2: When creature_template.MovementType=2
    //    2a) Creature is summoned and has creature_template.MovementType=2
    //        Creators need to be sure that creature_movement_template is always valid for summons.
    //        Mob that can be summoned anywhere should not have creature_movement_template for example.

    // No movement found for guid
    if (!i_path)
    {
        i_path = sWaypointMgr.GetPathTemplate(creature.GetEntry());

        // No movement found for entry
        if (!i_path)
        {
            sLog.outErrorDb("WaypointMovementGenerator::LoadPath: creature %s (Entry: %u GUID: %u) doesn't have waypoint path",
                creature.GetName(), creature.GetEntry(), creature.GetGUIDLow());
            return;
        }
    }

    // We have to set the destination here (for the first point), right after Initialize. Without, we may not have valid xyz for GetResetPosition
    CreatureTraveller traveller(creature);

    if (creature.CanFly())
        creature.AddSplineFlag(SPLINEFLAG_FLYING);

    const WaypointNode &node = i_path->at(i_currentNode);
    i_destinationHolder.SetDestination(traveller, node.x, node.y, node.z);
    i_nextMoveTime.Reset(i_destinationHolder.GetTotalTravelTime());
}

void WaypointMovementGenerator<Creature>::Initialize(Creature &creature)
{
    LoadPath(creature);
    creature.addUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
}

void WaypointMovementGenerator<Creature>::Finalize(Creature &creature)
{
    creature.clearUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
}

void WaypointMovementGenerator<Creature>::Interrupt(Creature &creature)
{
    creature.clearUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
}

void WaypointMovementGenerator<Creature>::Reset(Creature &creature)
{
    SetStoppedByPlayer(false);
    i_nextMoveTime.Reset(0);
    creature.addUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
}

bool WaypointMovementGenerator<Creature>::Update(Creature &creature, const uint32 &diff)
{
    if (!&creature)
        return true;

    // Waypoint movement can be switched on/off
    // This is quite handy for escort quests and other stuff
    if (creature.hasUnitState(UNIT_STAT_NOT_MOVE))
    {
        creature.clearUnitState(UNIT_STAT_ROAMING_MOVE);
        return true;
    }

    // prevent a crash at empty waypoint path.
    if (!i_path || i_path->empty())
    {
        creature.clearUnitState(UNIT_STAT_ROAMING_MOVE);
        return true;
    }

    if (i_currentNode >= i_path->size())
    {
        sLog.outError("WaypointMovement currentNode (%u) is equal or bigger than path size (creature entry %u)", i_currentNode, creature.GetEntry());
        i_currentNode = 0;
    }

    CreatureTraveller traveller(creature);

    i_nextMoveTime.Update(diff);

    if (i_destinationHolder.UpdateTraveller(traveller, diff, false, true))
    {
        if (!IsActive(creature))                            // force stop processing (movement can move out active zone with cleanup movegens list)
            return true;                                    // not expire now, but already lost
    }

    // creature has been stopped in middle of the waypoint segment
    if (!i_destinationHolder.HasArrived() && creature.IsStopped())
    {
        // Timer has elapsed, meaning this part controlled it
        if (i_nextMoveTime.Passed())
        {
            SetStoppedByPlayer(false);

            creature.addUnitState(UNIT_STAT_ROAMING_MOVE);

            if (creature.CanFly())
                creature.AddSplineFlag(SPLINEFLAG_FLYING);

            // Now we re-set destination to same node and start travel
            const WaypointNode &node = i_path->at(i_currentNode);
            i_destinationHolder.SetDestination(traveller, node.x, node.y, node.z);
            i_nextMoveTime.Reset(i_destinationHolder.GetTotalTravelTime());
        }
        else // if( !i_nextMoveTime.Passed())
        {
            // unexpected end of timer && creature stopped && not at end of segment
            if (!IsStoppedByPlayer())
            {
                // Put 30 seconds delay
                i_destinationHolder.IncreaseTravelTime(STOP_TIME_FOR_PLAYER);
                i_nextMoveTime.Reset(STOP_TIME_FOR_PLAYER);
                SetStoppedByPlayer(true);                   // Mark we did it
            }
        }
        return true;                                        // Abort here this update
    }

    if (creature.IsStopped())
    {
        if (!m_isArrivalDone)
        {
            if (i_path->at(i_currentNode).orientation != 100)
                creature.SetOrientation(i_path->at(i_currentNode).orientation);

            if (i_path->at(i_currentNode).script_id)
            {
                DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Creature movement start script %u at point %u for %s.", i_path->at(i_currentNode).script_id, i_currentNode, creature.GetGuidStr().c_str());
                creature.GetMap()->ScriptsStart(sCreatureMovementScripts, i_path->at(i_currentNode).script_id, &creature, &creature);
            }

            // We have reached the destination and can process behavior
            if (WaypointBehavior *behavior = i_path->at(i_currentNode).behavior)
            {
                if (behavior->emote != 0)
                    creature.HandleEmote(behavior->emote);

                if (behavior->spell != 0)
                {
                    creature.CastSpell(&creature, behavior->spell, false);

                    if (!IsActive(creature))                // force stop processing (cast can change movegens list)
                        return true;                        // not expire now, but already lost
                }

                if (behavior->model1 != 0)
                    creature.SetDisplayId(behavior->model1);

                if (behavior->textid[0])
                {
                    // Not only one text is set
                    if (behavior->textid[1])
                    {
                        // Select one from max 5 texts (0 and 1 already checked)
                        int i = 2;
                        for(; i < MAX_WAYPOINT_TEXT; ++i)
                        {
                            if (!behavior->textid[i])
                                break;
                        }

                        creature.MonsterSay(behavior->textid[rand() % i], LANG_UNIVERSAL);
                    }
                    else
                        creature.MonsterSay(behavior->textid[0], LANG_UNIVERSAL);
                }
            }                                               // wpBehaviour found

            // Can only do this once for the node
            m_isArrivalDone = true;

            // Inform script
            MovementInform(creature);

            if (!IsActive(creature))                        // force stop processing (movement can move out active zone with cleanup movegens list)
                return true;                                // not expire now, but already lost

            // prevent a crash at empty waypoint path.
            if (!i_path || i_path->empty() || i_currentNode >= i_path->size())
            {
                creature.clearUnitState(UNIT_STAT_ROAMING_MOVE);
                return true;
            }
        }
    }                                                       // i_creature.IsStopped()

    // This is at the end of waypoint segment (incl. was previously stopped by player, extending the time)
    if (i_nextMoveTime.Passed())
    {
        // If stopped then begin a new move segment
        if (creature.IsStopped())
        {
            creature.addUnitState(UNIT_STAT_ROAMING_MOVE);

            if (creature.CanFly())
                creature.AddSplineFlag(SPLINEFLAG_FLYING);

            if (WaypointBehavior *behavior = i_path->at(i_currentNode).behavior)
            {
                if (behavior->model2 != 0)
                    creature.SetDisplayId(behavior->model2);

                creature.SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
            }

            // behavior for "departure" of the current node is done
            m_isArrivalDone = false;

            // Proceed with increment current node and then send to the next destination
            ++i_currentNode;

            // Oops, end of the line so need to start from the beginning
            if (i_currentNode >= i_path->size())
                i_currentNode = 0;

            if (i_path->at(i_currentNode).orientation != 100)
                creature.SetOrientation(i_path->at(i_currentNode).orientation);

            const WaypointNode &node = i_path->at(i_currentNode);
            i_destinationHolder.SetDestination(traveller, node.x, node.y, node.z);
            i_nextMoveTime.Reset(i_destinationHolder.GetTotalTravelTime());
        }
        else
        {
            // If not stopped then stop it
            creature.clearUnitState(UNIT_STAT_ROAMING_MOVE);

            SetStoppedByPlayer(false);

            // Set TimeTracker to waittime for the current node
            i_nextMoveTime.Reset(i_path->at(i_currentNode).delay);
        }
    }

    return true;
}

void WaypointMovementGenerator<Creature>::MovementInform(Creature &creature)
{
    if (creature.AI())
        creature.AI()->MovementInform(WAYPOINT_MOTION_TYPE, i_currentNode);
}

bool WaypointMovementGenerator<Creature>::GetResetPosition(Creature&, float& x, float& y, float& z)
{
    return PathMovementBase<Creature, WaypointPath const*>::GetPosition(x,y,z);
}

//----------------------------------------------------//
uint32 FlightPathMovementGenerator::GetPathAtMapEnd() const
{
    if (i_currentNode >= i_path->size())
        return i_path->size();

    uint32 curMapId = (*i_path)[i_currentNode].mapid;

    for(uint32 i = i_currentNode; i < i_path->size(); ++i)
    {
        if ((*i_path)[i].mapid != curMapId)
            return i;
    }

    return i_path->size();
}

void FlightPathMovementGenerator::Initialize(Player &player)
{
    Reset(player);
}

void FlightPathMovementGenerator::Finalize(Player & player)
{
    // remove flag to prevent send object build movement packets for flight state and crash (movement generator already not at top of stack)
    player.clearUnitState(UNIT_STAT_TAXI_FLIGHT);

    float x, y, z;
    i_destinationHolder.GetLocationNow(player.GetMap(), x, y, z);
    player.SetPosition(x, y, z, player.GetOrientation());

    player.Unmount();
    player.RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_TAXI_FLIGHT);

    if(player.m_taxi.empty())
    {
        player.getHostileRefManager().setOnlineOfflineState(true);
        if(player.pvpInfo.inHostileArea)
            player.CastSpell(&player, 2479, true);

        // update z position to ground and orientation for landing point
        // this prevent cheating with landing  point at lags
        // when client side flight end early in comparison server side
        player.StopMoving();
    }
}

void FlightPathMovementGenerator::Interrupt(Player & player)
{
    player.clearUnitState(UNIT_STAT_TAXI_FLIGHT);
}

void FlightPathMovementGenerator::Reset(Player & player)
{
    player.getHostileRefManager().setOnlineOfflineState(false);
    player.addUnitState(UNIT_STAT_TAXI_FLIGHT);
    player.SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_TAXI_FLIGHT);
    Traveller<Player> traveller(player);
    // do not send movement, it was sent already
    i_destinationHolder.SetDestination(traveller, (*i_path)[i_currentNode].x, (*i_path)[i_currentNode].y, (*i_path)[i_currentNode].z, false);

    player.SendMonsterMoveByPath(GetPath(),GetCurrentNode(),GetPathAtMapEnd(), SplineFlags(SPLINEFLAG_WALKMODE|SPLINEFLAG_FLYING));
}

bool FlightPathMovementGenerator::Update(Player &player, const uint32 &diff)
{
    if (MovementInProgress())
    {
        Traveller<Player> traveller(player);
        if( i_destinationHolder.UpdateTraveller(traveller, diff, false) )
        {
            if (!IsActive(player))                          // force stop processing (movement can move out active zone with cleanup movegens list)
                return true;                                // not expire now, but already lost

            i_destinationHolder.ResetUpdate(FLIGHT_TRAVEL_UPDATE);
            if (i_destinationHolder.HasArrived())
            {
                DoEventIfAny(player,(*i_path)[i_currentNode],false);

                uint32 curMap = (*i_path)[i_currentNode].mapid;
                ++i_currentNode;
                if (MovementInProgress())
                {
                    DoEventIfAny(player,(*i_path)[i_currentNode],true);

                    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "loading node %u for player %s", i_currentNode, player.GetName());
                    if ((*i_path)[i_currentNode].mapid == curMap)
                    {
                        // do not send movement, it was sent already
                        i_destinationHolder.SetDestination(traveller, (*i_path)[i_currentNode].x, (*i_path)[i_currentNode].y, (*i_path)[i_currentNode].z, false);
                    }
                    return true;
                }
            }
            else
                return true;
        }
        else
            return true;
    }

    // we have arrived at the end of the path
    return false;
}

void FlightPathMovementGenerator::SetCurrentNodeAfterTeleport()
{
    if (i_path->empty())
        return;

    uint32 map0 = (*i_path)[0].mapid;

    for (size_t i = 1; i < i_path->size(); ++i)
    {
        if ((*i_path)[i].mapid != map0)
        {
            i_currentNode = i;
            return;
        }
    }
}

void FlightPathMovementGenerator::DoEventIfAny(Player& player, TaxiPathNodeEntry const& node, bool departure)
{
    if (uint32 eventid = departure ? node.departureEventID : node.arrivalEventID)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Taxi %s event %u of node %u of path %u for player %s", departure ? "departure" : "arrival", eventid, node.index, node.path, player.GetName());

        if (!sScriptMgr.OnProcessEvent(eventid, &player, &player, departure))
            player.GetMap()->ScriptsStart(sEventScripts, eventid, &player, &player);
    }
}

//
// Unique1's ASTAR Pathfinding Code... For future use & reference...
//

#ifdef __PATHFINDING__

int GetFCost(int to, int num, int parentNum, float *gcost); // Below...

int ShortenASTARRoute(short int *pathlist, int number)
{                                                           // Wrote this to make the routes a little smarter (shorter)... No point looping back to the same places... Unique1
    short int temppathlist[MAX_PATHLIST_NODES];
    int count = 0;
    //    int count2 = 0;
    int temp, temp2;
    int link;
    int upto = 0;

    for (temp = number; temp >= 0; temp--)
    {
        qboolean shortened = qfalse;

        for (temp2 = 0; temp2 < temp; temp2++)
        {
            for (link = 0; link < nodes[pathlist[temp]].enodenum; link++)
            {
                if (nodes[pathlist[temp]].links[link].flags & PATH_BLOCKED)
                    continue;

                //if ((bot->client->ps.eFlags & EF_TANK) && nodes[bot->current_node].links[link].flags & PATH_NOTANKS)    //if this path is blocked, skip it
                //    continue;

                //if (nodes[nodes[pathlist[temp]].links[link].targetNode].origin[2] > nodes[pathlist[temp]].origin[2] + 32)
                //    continue;

                if (nodes[pathlist[temp]].links[link].targetNode == pathlist[temp2])
                {                                           // Found a shorter route...
                    //if (OrgVisible(nodes[pathlist[temp2]].origin, nodes[pathlist[temp]].origin, -1))
                    {
                        temppathlist[count] = pathlist[temp2];
                        temp = temp2;
                        ++count;
                        shortened = qtrue;
                    }
                }
            }
        }

        if (!shortened)
        {
            temppathlist[count] = pathlist[temp];
            ++count;
        }
    }

    upto = count;

    for (temp = 0; temp < count; temp++)
    {
        pathlist[temp] = temppathlist[upto];
        --upto;
    }

    G_Printf("ShortenASTARRoute: Path size reduced from %i to %i nodes...n", number, count);
    return count;
}

/*
===========================================================================
CreatePathAStar
This function uses the A* pathfinding algorithm to determine the
shortest path between any two nodes.
It's fairly complex, so I'm not really going to explain it much.
Look up A* and binary heaps for more info.
pathlist stores the ideal path between the nodes, in reverse order,
and the return value is the number of nodes in that path
===========================================================================
*/
int CreatePathAStar(gentity_t *bot, int from, int to, short int *pathlist)
{
    //all the data we have to hold...since we can't do dynamic allocation, has to be MAX_NODES
    //we can probably lower this later - eg, the open list should never have more than at most a few dozen items on it
    short int openlist[MAX_NODES+1];                        //add 1 because it's a binary heap, and they don't use 0 - 1 is the first used index
    float gcost[MAX_NODES];
    int fcost[MAX_NODES];
    char list[MAX_NODES];                                   //0 is neither, 1 is open, 2 is closed - char because it's the smallest data type
    short int parent[MAX_NODES];

    short int numOpen = 0;
    short int atNode, temp, newnode=-1;
    qboolean found = qfalse;
    int count = -1;
    float gc;
    int i, u, v, m;
    vec3_t vec;

    //clear out all the arrays
    memset(openlist, 0, sizeof(short int)*(MAX_NODES+1));
    memset(fcost, 0, sizeof(int)*MAX_NODES);
    memset(list, 0, sizeof(char)*MAX_NODES);
    memset(parent, 0, sizeof(short int)*MAX_NODES);
    memset(gcost, -1, sizeof(float)*MAX_NODES);

    //make sure we have valid data before calculating everything
    if ((from == NODE_INVALID) || (to == NODE_INVALID) || (from >= MAX_NODES) || (to >= MAX_NODES) || (from == to))
        return -1;

    openlist[1] = from;                                     //add the starting node to the open list
    ++numOpen;
    gcost[from] = 0;                                        //its f and g costs are obviously 0
    fcost[from] = 0;

    while (1)
    {
        if (numOpen != 0)                                   //if there are still items in the open list
        {
            //pop the top item off of the list
            atNode = openlist[1];
            list[atNode] = 2;                               //put the node on the closed list so we don't check it again
            --numOpen;

            openlist[1] = openlist[numOpen+1];              //move the last item in the list to the top position
            v = 1;

            //this while loop reorders the list so that the new lowest fcost is at the top again
            while (1)
            {
                u = v;
                if ((2*u+1) < numOpen)                      //if both children exist
                {
                    if (fcost[openlist[u]] >= fcost[openlist[2*u]])
                        v = 2*u;
                    if (fcost[openlist[v]] >= fcost[openlist[2*u+1]])
                        v = 2*u+1;
                }
                else
                {
                    if ((2*u) < numOpen)                    //if only one child exists
                    {
                        if (fcost[openlist[u]] >= fcost[openlist[2*u]])
                            v = 2*u;
                    }
                }

                if (u != v)                                 //if they're out of order, swap this item with its parent
                {
                    temp = openlist[u];
                    openlist[u] = openlist[v];
                    openlist[v] = temp;
                }
                else
                    break;
            }

            for (i = 0; i < nodes[atNode].enodenum; ++i)    //loop through all the links for this node
            {
                newnode = nodes[atNode].links[i].targetNode;

                //if this path is blocked, skip it
                if (nodes[atNode].links[i].flags & PATH_BLOCKED)
                    continue;
                //if this path is blocked, skip it
                if (bot->client && (bot->client->ps.eFlags & EF_TANK) && nodes[atNode].links[i].flags & PATH_NOTANKS)
                    continue;
                //skip any unreachable nodes
                if (bot->client && (nodes[newnode].type & NODE_ALLY_UNREACHABLE) && (bot->client->sess.sessionTeam == TEAM_ALLIES))
                    continue;
                if (bot->client && (nodes[newnode].type & NODE_AXIS_UNREACHABLE) && (bot->client->sess.sessionTeam == TEAM_AXIS))
                    continue;

                if (list[newnode] == 2)                     //if this node is on the closed list, skip it
                    continue;

                if (list[newnode] != 1)                     //if this node is not already on the open list
                {
                    openlist[++numOpen] = newnode;          //add the new node to the open list
                    list[newnode] = 1;
                    parent[newnode] = atNode;               //record the node's parent

                    if (newnode == to)                      //if we've found the goal, don't keep computing paths!
                        break;                              //this will break the 'for' and go all the way to 'if (list[to] == 1)'

                    //store it's f cost value
                    fcost[newnode] = GetFCost(to, newnode, parent[newnode], gcost);

                    //this loop re-orders the heap so that the lowest fcost is at the top
                    m = numOpen;
                    while (m != 1)                          //while this item isn't at the top of the heap already
                    {
                        //if it has a lower fcost than its parent
                        if (fcost[openlist[m]] <= fcost[openlist[m/2]])
                        {
                            temp = openlist[m/2];
                            openlist[m/2] = openlist[m];
                            openlist[m] = temp;             //swap them
                            m /= 2;
                        }
                        else
                            break;
                    }
                }
                else                                        //if this node is already on the open list
                {
                    gc = gcost[atNode];
                    VectorSubtract(nodes[newnode].origin, nodes[atNode].origin, vec);
                    gc += VectorLength(vec);                //calculate what the gcost would be if we reached this node along the current path

                    if (gc < gcost[newnode])                //if the new gcost is less (ie, this path is shorter than what we had before)
                    {
                        parent[newnode] = atNode;           //set the new parent for this node
                        gcost[newnode] = gc;                //and the new g cost

                        for (i = 1; i < numOpen; ++i)       //loop through all the items on the open list
                        {
                            if (openlist[i] == newnode)     //find this node in the list
                            {
                                //calculate the new fcost and store it
                                fcost[newnode] = GetFCost(to, newnode, parent[newnode], gcost);

                                //reorder the list again, with the lowest fcost item on top
                                m = i;
                                while (m != 1)
                                {
                                    //if the item has a lower fcost than it's parent
                                    if (fcost[openlist[m]] < fcost[openlist[m/2]])
                                    {
                                        temp = openlist[m/2];
                                        openlist[m/2] = openlist[m];
                                        openlist[m] = temp; //swap them
                                        m /= 2;
                                    }
                                    else
                                        break;
                                }
                                break;                      //exit the 'for' loop because we already changed this node
                            }                               //if
                        }                                   //for
                    }                                       //if (gc < gcost[newnode])
                }                                           //if (list[newnode] != 1) --> else
            }                                               //for (loop through links)
        }                                                   //if (numOpen != 0)
        else
        {
            found = qfalse;                                 //there is no path between these nodes
            break;
        }

        if (list[to] == 1)                                  //if the destination node is on the open list, we're done
        {
            found = qtrue;
            break;
        }
    }                                                       //while (1)

    if (found == qtrue)                                     //if we found a path
    {
        //G_Printf("%s - path found!n", bot->client->pers.netname);
        count = 0;

        temp = to;                                          //start at the end point
        while (temp != from)                                //travel along the path (backwards) until we reach the starting point
        {
            pathlist[count++] = temp;                       //add the node to the pathlist and increment the count
            temp = parent[temp];                            //move to the parent of this node to continue the path
        }

        pathlist[count++] = from;                           //add the beginning node to the end of the pathlist

        #ifdef __BOT_SHORTEN_ROUTING__
        count = ShortenASTARRoute(pathlist, count);         // This isn't working... Dunno why.. Unique1
        #endif                                              //__BOT_SHORTEN_ROUTING__
    }
    else
    {
        //G_Printf("^1*** ^4BOT DEBUG^5: (CreatePathAStar) There is no route between node ^7%i^5 and node ^7%i^5.n", from, to);
        count = CreateDumbRoute(from, to, pathlist);

        if (count > 0)
        {
            #ifdef __BOT_SHORTEN_ROUTING__
            count = ShortenASTARRoute(pathlist, count);     // This isn't working... Dunno why.. Unique1
            #endif                                          //__BOT_SHORTEN_ROUTING__
            return count;
        }
    }

    return count;                                           //return the number of nodes in the path, -1 if not found
}

/*
===========================================================================
GetFCost
Utility function used by A* pathfinding to calculate the
cost to move between nodes towards a goal.  Using the A*
algorithm F = G + H, G here is the distance along the node
paths the bot must travel, and H is the straight-line distance
to the goal node.
Returned as an int because more precision is unnecessary and it
will slightly speed up heap access
===========================================================================
*/
int GetFCost(int to, int num, int parentNum, float *gcost)
{
    float gc = 0;
    float hc = 0;
    vec3_t v;

    if (gcost[num] == -1)
    {
        if (parentNum != -1)
        {
            gc = gcost[parentNum];
            VectorSubtract(nodes[num].origin, nodes[parentNum].origin, v);
            gc += VectorLength(v);
        }
        gcost[num] = gc;
    }
    else
        gc = gcost[num];

    VectorSubtract(nodes[to].origin, nodes[num].origin, v);
    hc = VectorLength(v);

    return (int)(gc + hc);
}
#endif                                                      //__PATHFINDING__
