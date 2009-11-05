/*
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#ifndef __BATTLEGROUNDAV_H
#define __BATTLEGROUNDAV_H

class BattleGround;

#define BG_AV_MAX_NODE_DISTANCE             25              // distance in which players are still counted in range of a banner (for alliance towers this is calculated from the center of the tower)

#define BG_AV_BOSS_KILL_QUEST_SPELL         23658

#define BG_AV_CAPTIME                       240000          // 4 minutes
#define BG_AV_SNOWFALL_FIRSTCAP             300000          // 5 minutes but i also have seen 4:05

#define BG_AV_SCORE_INITIAL_POINTS          600
#define BG_AV_SCORE_NEAR_LOSE               120

// description: KILL = bonushonor kill one kill is 21honor worth at 0
// REP reputation, RES = ressources a team will lose
#define BG_AV_KILL_BOSS                     4
#define BG_AV_REP_BOSS                      350
#define BG_AV_REP_BOSS_HOLIDAY              525

#define BG_AV_KILL_CAPTAIN                  3
#define BG_AV_REP_CAPTAIN                   125
#define BG_AV_REP_CAPTAIN_HOLIDAY           185
#define BG_AV_RES_CAPTAIN                   100

#define BG_AV_KILL_TOWER                    3
#define BG_AV_REP_TOWER                     12
#define BG_AV_REP_TOWER_HOLIDAY             18
#define BG_AV_RES_TOWER                     75

#define BG_AV_KILL_GET_COMMANDER            1               // for a safely returned wingcommander TODO implement it

// bonushonor at the end
#define BG_AV_KILL_SURVIVING_TOWER          2
#define BG_AV_REP_SURVIVING_TOWER           12
#define BG_AV_REP_SURVIVING_TOWER_HOLIDAY   18

#define BG_AV_KILL_SURVIVING_CAPTAIN        2
#define BG_AV_REP_SURVIVING_CAPTAIN         125
#define BG_AV_REP_SURVIVING_CAPTAIN_HOLIDAY 175

#define BG_AV_KILL_MAP_COMPLETE             0
#define BG_AV_KILL_MAP_COMPLETE_HOLIDAY     4

#define BG_AV_REP_OWNED_GRAVE               12
#define BG_AV_REP_OWNED_GRAVE_HOLIDAY       18

#define BG_AV_REP_OWNED_MINE                24
#define BG_AV_REP_OWNED_MINE_HOLIDAY        36

enum BG_AV_Sounds
{
    BG_AV_SOUND_NEAR_LOSE               = 8456,             // not confirmed yet

    BG_AV_SOUND_ALLIANCE_ASSAULTS       = 8212,             // tower,grave + enemy boss if someone tries to attack him
    BG_AV_SOUND_HORDE_ASSAULTS          = 8174,
    BG_AV_SOUND_ALLIANCE_GOOD           = 8173,             // if something good happens for the team:  wins(maybe only through killing the boss), captures mine or grave, destroys tower and defends grave
    BG_AV_SOUND_HORDE_GOOD              = 8213,
    BG_AV_SOUND_BOTH_TOWER_DEFEND       = 8192,

    BG_AV_SOUND_ALLIANCE_CAPTAIN        = 8232,             // gets called when someone attacks them and at the beginning after 3min + rand(x) * 10sec (maybe buff)
    BG_AV_SOUND_HORDE_CAPTAIN           = 8333,
};

enum BG_AV_OTHER_VALUES
{
    BG_AV_NORTH_MINE            = 0,
    BG_AV_SOUTH_MINE            = 1,
    BG_AV_MINE_TICK_TIMER       = 45000,
    BG_AV_MINE_RECLAIM_TIMER    = 1200000,                  // TODO: get the right value.. this is currently 20 minutes
    BG_AV_NEUTRAL_TEAM          = 2,                        // this is the neutral owner of snowfall
    BG_AV_FACTION_A             = 730,
    BG_AV_FACTION_H             = 729,
};
#define BG_AV_MAX_MINES 2

enum BG_AV_ObjectIds
{
    // mine supplies
    BG_AV_OBJECTID_MINE_N               = 178785,
    BG_AV_OBJECTID_MINE_S               = 178784,
};

enum BG_AV_Nodes
{
    BG_AV_NODES_FIRSTAID_STATION        = 0,
    BG_AV_NODES_STORMPIKE_GRAVE         = 1,
    BG_AV_NODES_STONEHEART_GRAVE        = 2,
    BG_AV_NODES_SNOWFALL_GRAVE          = 3,
    BG_AV_NODES_ICEBLOOD_GRAVE          = 4,
    BG_AV_NODES_FROSTWOLF_GRAVE         = 5,
    BG_AV_NODES_FROSTWOLF_HUT           = 6,
    BG_AV_NODES_DUNBALDAR_SOUTH         = 7,
    BG_AV_NODES_DUNBALDAR_NORTH         = 8,
    BG_AV_NODES_ICEWING_BUNKER          = 9,
    BG_AV_NODES_STONEHEART_BUNKER       = 10,
    BG_AV_NODES_ICEBLOOD_TOWER          = 11,
    BG_AV_NODES_TOWER_POINT             = 12,
    BG_AV_NODES_FROSTWOLF_ETOWER        = 13,
    BG_AV_NODES_FROSTWOLF_WTOWER        = 14,
    BG_AV_NODES_ERROR                   = 255,
};
#define BG_AV_NODES_MAX                 15


// for nodeevents we will use event1=node
// event2 is related to BG_AV_States
// 0 = alliance assaulted
// 1 = alliance control
// 2 = horde assaulted
// 3 = horde control
// 4 = neutral assaulted
// 5 = neutral control

// graves have special creatures - their defenders can be in 4 different states
// through some quests with armor scraps
// so i use event1=BG_AV_NODES_MAX+node (15-21)
// and event2=type

#define BG_AV_MINE_BOSSES       46                          // + mineid will be exact event
#define BG_AV_MINE_BOSSES_NORTH 46
#define BG_AV_MINE_BOSSES_SOUTH 47
#define BG_AV_CAPTAIN_A         48
#define BG_AV_CAPTAIN_H         49
#define BG_AV_MINE_EVENT        50                          // + mineid will be exact event
#define BG_AV_MINE_EVENT_NORTH  50
#define BG_AV_MINE_EVENT_SOUTH  51

#define BG_AV_MARSHAL_A_SOUTH   52
#define BG_AV_MARSHAL_A_NORTH   53
#define BG_AV_MARSHAL_A_ICE     54
#define BG_AV_MARSHAL_A_STONE   55
#define BG_AV_MARSHAL_H_ICE     56
#define BG_AV_MARSHAL_H_TOWER   57
#define BG_AV_MARSHAL_H_ETOWER  58
#define BG_AV_MARSHAL_H_WTOWER  59

#define BG_AV_HERALD            60
#define BG_AV_BOSS_A            61
#define BG_AV_BOSS_H            62
#define BG_AV_NodeEventCaptainDead_A 63
#define BG_AV_NodeEventCaptainDead_H 64

enum BG_AV_Graveyards
{
    BG_AV_GRAVE_STORM_AID          = 751,
    BG_AV_GRAVE_STORM_GRAVE        = 689,
    BG_AV_GRAVE_STONE_GRAVE        = 729,
    BG_AV_GRAVE_SNOWFALL           = 169,
    BG_AV_GRAVE_ICE_GRAVE          = 749,
    BG_AV_GRAVE_FROSTWOLF          = 690,
    BG_AV_GRAVE_FROST_HUT          = 750,
    BG_AV_GRAVE_MAIN_ALLIANCE      = 611,
    BG_AV_GRAVE_MAIN_HORDE         = 610
};

const uint32 BG_AV_GraveyardIds[9]= {
    BG_AV_GRAVE_STORM_AID,
    BG_AV_GRAVE_STORM_GRAVE,
    BG_AV_GRAVE_STONE_GRAVE,
    BG_AV_GRAVE_SNOWFALL,
    BG_AV_GRAVE_ICE_GRAVE,
    BG_AV_GRAVE_FROSTWOLF,
    BG_AV_GRAVE_FROST_HUT,
    BG_AV_GRAVE_MAIN_ALLIANCE,
    BG_AV_GRAVE_MAIN_HORDE
};

enum BG_AV_States
{
    POINT_ASSAULTED             = 0,
    POINT_CONTROLLED            = 1
};
#define BG_AV_MAX_STATES 2

enum BG_AV_WorldStates
{
    BG_AV_Alliance_Score        = 3127,
    BG_AV_Horde_Score           = 3128,
    BG_AV_SHOW_H_SCORE          = 3133,
    BG_AV_SHOW_A_SCORE          = 3134,
    AV_SNOWFALL_N               = 1966,
};

// alliance_control horde_control neutral_control
const uint32 BG_AV_MineWorldStates[2][3] = {
    {1358, 1359, 1360},
    {1355, 1356, 1357}
};

// alliance_control alliance_assault h_control h_assault
const uint32 BG_AV_NodeWorldStates[BG_AV_NODES_MAX][4] = {
    // Stormpike first aid station
    {1326,1325,1328,1327},
    // Stormpike Graveyard
    {1335,1333,1336,1334},
    // Stoneheart Grave
    {1304,1302,1303,1301},
    // Snowfall Grave
    {1343,1341,1344,1342},
    // Iceblood grave
    {1348,1346,1349,1347},
    // Frostwolf Grave
    {1339,1337,1340,1338},
    // Frostwolf Hut
    {1331,1329,1332,1330},
    // Dunbaldar South Bunker
    {1375,1361,1378,1370},
    // Dunbaldar North Bunker
    {1374,1362,1379,1371},
    // Icewing Bunker
    {1376,1363,1380,1372},
    // Stoneheart Bunker
    {1377,1364,1381,1373},
    // Iceblood Tower
    {1390,1368,1395,1385},
    // Tower Point
    {1389,1367,1394,1384},
    // Frostwolf East
    {1388,1366,1393,1383},
    // Frostwolf West
    {1387,1365,1392,1382},
};

// through the armorscap-quest 4 different gravedefender exist
#define BG_AV_MAX_GRAVETYPES 4
enum BG_AV_QuestIds
{
    BG_AV_QUEST_A_SCRAPS1       = 7223,                     // first quest
    BG_AV_QUEST_A_SCRAPS2       = 6781,                     // repeatable
    BG_AV_QUEST_H_SCRAPS1       = 7224,
    BG_AV_QUEST_H_SCRAPS2       = 6741,
    BG_AV_QUEST_A_COMMANDER1    = 6942,                     // soldier
    BG_AV_QUEST_H_COMMANDER1    = 6825,
    BG_AV_QUEST_A_COMMANDER2    = 6941,                     // leutnant
    BG_AV_QUEST_H_COMMANDER2    = 6826,
    BG_AV_QUEST_A_COMMANDER3    = 6943,                     // commander
    BG_AV_QUEST_H_COMMANDER3    = 6827,
    BG_AV_QUEST_A_BOSS1         = 7386,                     // 5 cristal/blood
    BG_AV_QUEST_H_BOSS1         = 7385,
    BG_AV_QUEST_A_BOSS2         = 6881,                     // 1
    BG_AV_QUEST_H_BOSS2         = 6801,
    BG_AV_QUEST_A_NEAR_MINE     = 5892,                     // the mine near start location of team
    BG_AV_QUEST_H_NEAR_MINE     = 5893,
    BG_AV_QUEST_A_OTHER_MINE    = 6982,                     // the other mine ;)
    BG_AV_QUEST_H_OTHER_MINE    = 6985,
    BG_AV_QUEST_A_RIDER_HIDE    = 7026,
    BG_AV_QUEST_H_RIDER_HIDE    = 7002,
    BG_AV_QUEST_A_RIDER_TAME    = 7027,
    BG_AV_QUEST_H_RIDER_TAME    = 7001
};

struct BG_AV_NodeInfo
{
    uint32       TotalOwner;
    uint32       Owner;
    uint32       PrevOwner;
    BG_AV_States State;
    BG_AV_States PrevState;
    uint32       Timer;
    bool         Tower;
};

inline BG_AV_Nodes &operator++(BG_AV_Nodes &i)
{
    return i = BG_AV_Nodes(i + 1);
}

class BattleGroundAVScore : public BattleGroundScore
{
    public:
        BattleGroundAVScore() : GraveyardsAssaulted(0), GraveyardsDefended(0), TowersAssaulted(0), TowersDefended(0), SecondaryObjectives(0) {};
        virtual ~BattleGroundAVScore() {};
        uint32 GraveyardsAssaulted;
        uint32 GraveyardsDefended;
        uint32 TowersAssaulted;
        uint32 TowersDefended;
        uint32 SecondaryObjectives;
};

class BattleGroundAV : public BattleGround
{
    friend class BattleGroundMgr;

    public:
        BattleGroundAV();
        ~BattleGroundAV();
        void Update(uint32 diff);

        /* inherited from BattlegroundClass */
        virtual void AddPlayer(Player *plr);

        virtual void StartingEventCloseDoors();
        virtual void StartingEventOpenDoors();
        // world states
        virtual void FillInitialWorldStates(WorldPacket& data);

        void RemovePlayer(Player *plr,uint64 guid);
        void HandleAreaTrigger(Player *Source, uint32 Trigger);
        virtual void Reset();

        /*general stuff*/
        void UpdateScore(BattleGroundTeamId team, int32 points);
        void UpdatePlayerScore(Player *Source, uint32 type, uint32 value);

        /*handle stuff*/ // these are functions which get called from extern scripts
        virtual void EventPlayerClickedOnFlag(Player *source, GameObject* target_obj);
        void HandleKillPlayer(Player* player, Player *killer);
        void HandleKillUnit(Creature *creature, Player *killer);
        void HandleQuestComplete(uint32 questid, Player *player);
        bool PlayerCanDoMineQuest(int32 GOId,uint32 team);

        void EndBattleGround(uint32 winner);

        virtual WorldSafeLocsEntry const* GetClosestGraveYard(Player *plr);

    private:
        /* Nodes occupying */
        void EventPlayerAssaultsPoint(Player* player, BG_AV_Nodes node);
        void EventPlayerDefendsPoint(Player* player, BG_AV_Nodes node);
        void EventPlayerDestroyedPoint(BG_AV_Nodes node);

        void AssaultNode(BG_AV_Nodes node, uint32 team);
        void DestroyNode(BG_AV_Nodes node);
        void InitNode(BG_AV_Nodes node, uint32 team, bool tower);
        void DefendNode(BG_AV_Nodes node, uint32 team);

        void PopulateNode(BG_AV_Nodes node);

        uint32 GetNodeName(BG_AV_Nodes node);
        const bool IsTower(BG_AV_Nodes node) { return (node == BG_AV_NODES_ERROR)? false : m_Nodes[node].Tower; }
        const bool IsGrave(BG_AV_Nodes node) { return (node == BG_AV_NODES_ERROR)? false : !m_Nodes[node].Tower; }

        /*mine*/
        void ChangeMineOwner(uint8 mine, uint32 team);

        /*worldstates*/
        uint8 GetWorldStateType(uint8 state, uint32 team) const { return team * BG_AV_MAX_STATES + state; }
        void SendMineWorldStates(uint32 mine);
        void UpdateNodeWorldState(BG_AV_Nodes node);

        /*variables */
        uint32 m_Team_QuestStatus[BG_TEAMS_COUNT][9];       // [x][y] x=team y=questcounter

        BG_AV_NodeInfo m_Nodes[BG_AV_NODES_MAX];

        int8 m_Mine_Owner[BG_AV_MAX_MINES];
        int8 m_Mine_PrevOwner[BG_AV_MAX_MINES];             // only for worldstates needed
        int32 m_Mine_Timer[BG_AV_MAX_MINES];
        uint32 m_Mine_Reclaim_Timer[BG_AV_MAX_MINES];

        bool m_IsInformedNearLose[BG_TEAMS_COUNT];
        bool m_captainAlive[BG_TEAMS_COUNT];

        uint32 m_HonorMapComplete;
        uint32 m_RepTowerDestruction;
        uint32 m_RepCaptain;
        uint32 m_RepBoss;
        uint32 m_RepOwnedGrave;
        uint32 m_RepOwnedMine;
        uint32 m_RepSurviveCaptain;
        uint32 m_RepSurviveTower;
};

#endif
