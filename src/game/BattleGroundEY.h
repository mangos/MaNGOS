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

#ifndef __BATTLEGROUNDEY_H
#define __BATTLEGROUNDEY_H

#include "Language.h"

class BattleGround;

#define BG_EY_FLAG_RESPAWN_TIME         (10*IN_MILLISECONDS) //10 seconds
#define BG_EY_FPOINTS_TICK_TIME         (2*IN_MILLISECONDS)  //2 seconds

enum BG_EY_WorldStates
{
    EY_ALLIANCE_RESOURCES           = 2749,
    EY_HORDE_RESOURCES              = 2750,
    EY_ALLIANCE_BASE                = 2752,
    EY_HORDE_BASE                   = 2753,
    DRAENEI_RUINS_HORDE_CONTROL     = 2733,
    DRAENEI_RUINS_ALLIANCE_CONTROL  = 2732,
    DRAENEI_RUINS_UNCONTROL         = 2731,
    MAGE_TOWER_ALLIANCE_CONTROL     = 2730,
    MAGE_TOWER_HORDE_CONTROL        = 2729,
    MAGE_TOWER_UNCONTROL            = 2728,
    FEL_REAVER_HORDE_CONTROL        = 2727,
    FEL_REAVER_ALLIANCE_CONTROL     = 2726,
    FEL_REAVER_UNCONTROL            = 2725,
    BLOOD_ELF_HORDE_CONTROL         = 2724,
    BLOOD_ELF_ALLIANCE_CONTROL      = 2723,
    BLOOD_ELF_UNCONTROL             = 2722,
    PROGRESS_BAR_PERCENT_GREY       = 2720,                 //100 = empty (only grey), 0 = blue|red (no grey)
    PROGRESS_BAR_STATUS             = 2719,                 //50 init!, 48 ... hordak bere .. 33 .. 0 = full 100% hordacky , 100 = full alliance
    PROGRESS_BAR_SHOW               = 2718,                 //1 init, 0 druhy send - bez messagu, 1 = controlled alliance
    NETHERSTORM_FLAG                = 2757,
    //set to 2 when flag is picked up, and to 1 if it is dropped
    NETHERSTORM_FLAG_STATE_ALLIANCE = 2769,
    NETHERSTORM_FLAG_STATE_HORDE    = 2770
};

enum BG_EY_ProgressBarConsts
{
    BG_EY_POINT_MAX_CAPTURERS_COUNT     = 5,
    BG_EY_POINT_RADIUS                  = 70,
    BG_EY_PROGRESS_BAR_DONT_SHOW        = 0,
    BG_EY_PROGRESS_BAR_SHOW             = 1,
    BG_EY_PROGRESS_BAR_PERCENT_GREY     = 40,
    BG_EY_PROGRESS_BAR_STATE_MIDDLE     = 50,
    BG_EY_PROGRESS_BAR_HORDE_CONTROLLED = 0,
    BG_EY_PROGRESS_BAR_NEUTRAL_LOW      = 30,
    BG_EY_PROGRESS_BAR_NEUTRAL_HIGH     = 70,
    BG_EY_PROGRESS_BAR_ALI_CONTROLLED   = 100
};

enum BG_EY_Sounds
{
    //strange ids, but sure about them
    BG_EY_SOUND_FLAG_PICKED_UP_ALLIANCE = 8212,
    BG_EY_SOUND_FLAG_CAPTURED_HORDE     = 8213,
    BG_EY_SOUND_FLAG_PICKED_UP_HORDE    = 8174,
    BG_EY_SOUND_FLAG_CAPTURED_ALLIANCE  = 8173,
    BG_EY_SOUND_FLAG_RESET              = 8192
};

enum BG_EY_Spells
{
    BG_EY_NETHERSTORM_FLAG_SPELL        = 34976,
    BG_EY_PLAYER_DROPPED_FLAG_SPELL     = 34991
};

enum EYBattleGroundPointsTrigger
{
    TR_BLOOD_ELF_POINT        = 4476,
    TR_FEL_REAVER_POINT       = 4514,
    TR_MAGE_TOWER_POINT       = 4516,
    TR_DRAENEI_RUINS_POINT    = 4518,
    TR_BLOOD_ELF_BUFF         = 4568,
    TR_FEL_REAVER_BUFF        = 4569,
    TR_MAGE_TOWER_BUFF        = 4570,
    TR_DRAENEI_RUINS_BUFF     = 4571
};

enum EYBattleGroundGaveyards
{
    EY_GRAVEYARD_MAIN_ALLIANCE     = 1103,
    EY_GRAVEYARD_MAIN_HORDE        = 1104,
    EY_GRAVEYARD_FEL_REAVER        = 1105,
    EY_GRAVEYARD_BLOOD_ELF         = 1106,
    EY_GRAVEYARD_DRAENEI_RUINS     = 1107,
    EY_GRAVEYARD_MAGE_TOWER        = 1108
};

enum BG_EY_Nodes
{
    BG_EY_NODE_FEL_REAVER         = 0,
    BG_EY_NODE_BLOOD_ELF          = 1,
    BG_EY_NODE_DRAENEI_RUINS      = 2,
    BG_EY_NODE_MAGE_TOWER         = 3,

    // special internal node
    BG_EY_PLAYERS_OUT_OF_POINTS   = 4,                      // used for store out of node players data
};

#define BG_EY_NODES_MAX             4
#define BG_EY_NODES_MAX_WITH_SPEIAL 5

// node-events work like this: event1:nodeid, event2:state (0alliance,1horde,2neutral)
#define BG_EYE_NEUTRAL_TEAM 2
#define BG_EY_EVENT_CAPTURE_FLAG 4                          // event1=4, event2=nodeid or 4 for the default center spawn
#define BG_EY_EVENT2_FLAG_CENTER 4                          // maximum node is 3 so 4 for center is ok
// all other event2 are just nodeids, i won't define something here

// x, y, z
// used to check, when player is in range of a node
const float BG_EY_NodePositions[BG_EY_NODES_MAX][3] = {
    {2024.600708f, 1742.819580f, 1195.157715f},             // BG_EY_NODE_FEL_REAVER
    {2050.493164f, 1372.235962f, 1194.563477f},             // BG_EY_NODE_BLOOD_ELF
    {2301.010498f, 1386.931641f, 1197.183472f},             // BG_EY_NODE_DRAENEI_RUINS
    {2282.121582f, 1760.006958f, 1189.707153f}              // BG_EY_NODE_MAGE_TOWER
};

enum EYBattleGroundObjectTypes
{
    //buffs
    BG_EY_OBJECT_SPEEDBUFF_FEL_REAVER           = 1,
    BG_EY_OBJECT_REGENBUFF_FEL_REAVER           = 2,
    BG_EY_OBJECT_BERSERKBUFF_FEL_REAVER         = 3,
    BG_EY_OBJECT_SPEEDBUFF_BLOOD_ELF            = 4,
    BG_EY_OBJECT_REGENBUFF_BLOOD_ELF            = 5,
    BG_EY_OBJECT_BERSERKBUFF_BLOOD_ELF          = 6,
    BG_EY_OBJECT_SPEEDBUFF_DRAENEI_RUINS        = 7,
    BG_EY_OBJECT_REGENBUFF_DRAENEI_RUINS        = 8,
    BG_EY_OBJECT_BERSERKBUFF_DRAENEI_RUINS      = 9,
    BG_EY_OBJECT_SPEEDBUFF_MAGE_TOWER           = 10,
    BG_EY_OBJECT_REGENBUFF_MAGE_TOWER           = 11,
    BG_EY_OBJECT_BERSERKBUFF_MAGE_TOWER         = 12,
    BG_EY_OBJECT_MAX                            = 13
};

#define BG_EY_NotEYWeekendHonorTicks    330
#define BG_EY_EYWeekendHonorTicks       200

enum BG_EY_Score
{
    BG_EY_WARNING_NEAR_VICTORY_SCORE    = 1400,
    BG_EY_MAX_TEAM_SCORE                = 1600
};

enum BG_EY_FlagState
{
    BG_EY_FLAG_STATE_ON_BASE      = 0,
    BG_EY_FLAG_STATE_WAIT_RESPAWN = 1,
    BG_EY_FLAG_STATE_ON_PLAYER    = 2,
    BG_EY_FLAG_STATE_ON_GROUND    = 3
};

enum EYBattleGroundPointState
{
    EY_POINT_NO_OWNER           = 0,
    EY_POINT_STATE_UNCONTROLLED = 0,
    EY_POINT_UNDER_CONTROL      = 3
};

struct BattleGroundEYPointIconsStruct
{
    BattleGroundEYPointIconsStruct(uint32 _WorldStateControlIndex, uint32 _WorldStateAllianceControlledIndex, uint32 _WorldStateHordeControlledIndex)
        : WorldStateControlIndex(_WorldStateControlIndex), WorldStateAllianceControlledIndex(_WorldStateAllianceControlledIndex), WorldStateHordeControlledIndex(_WorldStateHordeControlledIndex) {}
    uint32 WorldStateControlIndex;
    uint32 WorldStateAllianceControlledIndex;
    uint32 WorldStateHordeControlledIndex;
};

struct BattleGroundEYLoosingPointStruct
{
    BattleGroundEYLoosingPointStruct(uint32 _MessageIdAlliance, uint32 _MessageIdHorde)
        : MessageIdAlliance(_MessageIdAlliance), MessageIdHorde(_MessageIdHorde)
    {}

    uint32 MessageIdAlliance;
    uint32 MessageIdHorde;
};

struct BattleGroundEYCapturingPointStruct
{
    BattleGroundEYCapturingPointStruct(uint32 _MessageIdAlliance, uint32 _MessageIdHorde, uint32 _GraveYardId)
        : MessageIdAlliance(_MessageIdAlliance), MessageIdHorde(_MessageIdHorde), GraveYardId(_GraveYardId)
    {}
    uint32 MessageIdAlliance;
    uint32 MessageIdHorde;
    uint32 GraveYardId;
};

const uint8  BG_EY_TickPoints[BG_EY_NODES_MAX] = {1, 2, 5, 10};
const uint32 BG_EY_FlagPoints[BG_EY_NODES_MAX] = {75, 85, 100, 500};

//constant arrays:
const BattleGroundEYPointIconsStruct PointsIconStruct[BG_EY_NODES_MAX] =
{
    BattleGroundEYPointIconsStruct(FEL_REAVER_UNCONTROL, FEL_REAVER_ALLIANCE_CONTROL, FEL_REAVER_HORDE_CONTROL),
    BattleGroundEYPointIconsStruct(BLOOD_ELF_UNCONTROL, BLOOD_ELF_ALLIANCE_CONTROL, BLOOD_ELF_HORDE_CONTROL),
    BattleGroundEYPointIconsStruct(DRAENEI_RUINS_UNCONTROL, DRAENEI_RUINS_ALLIANCE_CONTROL, DRAENEI_RUINS_HORDE_CONTROL),
    BattleGroundEYPointIconsStruct(MAGE_TOWER_UNCONTROL, MAGE_TOWER_ALLIANCE_CONTROL, MAGE_TOWER_HORDE_CONTROL)
};
const BattleGroundEYLoosingPointStruct LoosingPointTypes[BG_EY_NODES_MAX] =
{
    BattleGroundEYLoosingPointStruct(LANG_BG_EY_HAS_LOST_A_F_RUINS, LANG_BG_EY_HAS_LOST_H_F_RUINS),
    BattleGroundEYLoosingPointStruct(LANG_BG_EY_HAS_LOST_A_B_TOWER, LANG_BG_EY_HAS_LOST_H_B_TOWER),
    BattleGroundEYLoosingPointStruct(LANG_BG_EY_HAS_LOST_A_D_RUINS, LANG_BG_EY_HAS_LOST_H_D_RUINS),
    BattleGroundEYLoosingPointStruct(LANG_BG_EY_HAS_LOST_A_M_TOWER, LANG_BG_EY_HAS_LOST_H_M_TOWER)
};
const BattleGroundEYCapturingPointStruct CapturingPointTypes[BG_EY_NODES_MAX] =
{
    BattleGroundEYCapturingPointStruct(LANG_BG_EY_HAS_TAKEN_A_F_RUINS, LANG_BG_EY_HAS_TAKEN_H_F_RUINS, EY_GRAVEYARD_FEL_REAVER),
    BattleGroundEYCapturingPointStruct(LANG_BG_EY_HAS_TAKEN_A_B_TOWER, LANG_BG_EY_HAS_TAKEN_H_B_TOWER, EY_GRAVEYARD_BLOOD_ELF),
    BattleGroundEYCapturingPointStruct(LANG_BG_EY_HAS_TAKEN_A_D_RUINS, LANG_BG_EY_HAS_TAKEN_H_D_RUINS, EY_GRAVEYARD_DRAENEI_RUINS),
    BattleGroundEYCapturingPointStruct(LANG_BG_EY_HAS_TAKEN_A_M_TOWER, LANG_BG_EY_HAS_TAKEN_H_M_TOWER, EY_GRAVEYARD_MAGE_TOWER)
};

class BattleGroundEYScore : public BattleGroundScore
{
    public:
        BattleGroundEYScore () : FlagCaptures(0) {};
        virtual ~BattleGroundEYScore() {};
        uint32 FlagCaptures;
};

class BattleGroundEY : public BattleGround
{
    friend class BattleGroundMgr;

    public:
        BattleGroundEY();
        ~BattleGroundEY();
        void Update(uint32 diff);

        /* inherited from BattlegroundClass */
        virtual void AddPlayer(Player *plr);
        virtual void StartingEventCloseDoors();
        virtual void StartingEventOpenDoors();

        /* BG Flags */
        ObjectGuid const& GetFlagPickerGuid() const { return m_FlagKeeper; }
        void SetFlagPicker(ObjectGuid guid) { m_FlagKeeper = guid; }
        void ClearFlagPicker()              { m_FlagKeeper.Clear(); }
        bool IsFlagPickedup() const         { return !m_FlagKeeper.IsEmpty(); }
        uint8 GetFlagState() const          { return m_FlagState; }
        void RespawnFlag(bool send_message);
        void RespawnFlagAfterDrop();

        void RemovePlayer(Player *plr, ObjectGuid guid);
        void HandleAreaTrigger(Player *Source, uint32 Trigger);
        void HandleKillPlayer(Player *player, Player *killer);
        virtual WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);
        virtual bool SetupBattleGround();
        virtual void Reset();
        void UpdateTeamScore(Team team);
        void EndBattleGround(Team winner);
        void UpdatePlayerScore(Player *Source, uint32 type, uint32 value);
        virtual void FillInitialWorldStates(WorldPacket& data, uint32& count);
        void SetDroppedFlagGuid(ObjectGuid guid)     { m_DroppedFlagGuid = guid;}
        void ClearDroppedFlagGuid()                  { m_DroppedFlagGuid.Clear();}
        ObjectGuid const& GetDroppedFlagGuid() const { return m_DroppedFlagGuid;}

        /* Battleground Events */
        virtual void EventPlayerClickedOnFlag(Player *Source, GameObject* target_obj);
        virtual void EventPlayerDroppedFlag(Player *Source);

        /* achievement req. */
        bool IsAllNodesConrolledByTeam(Team team) const;

    private:
        void EventPlayerCapturedFlag(Player *Source, BG_EY_Nodes node);
        void EventTeamCapturedPoint(Player *Source, uint32 Point);
        void EventTeamLostPoint(Player *Source, uint32 Point);
        void UpdatePointsCount(Team team);
        void UpdatePointsIcons(Team team, uint32 Point);

        /* Point status updating procedures */
        void CheckSomeoneLeftPoint();
        void CheckSomeoneJoinedPoint();
        void UpdatePointStatuses();

        /* Scorekeeping */
        uint32 GetTeamScore(Team team) const { return m_TeamScores[GetTeamIndexByTeamId(team)]; }
        void AddPoints(Team team, uint32 Points);

        void RemovePoint(Team team, uint32 Points = 1) { m_TeamScores[GetTeamIndexByTeamId(team)] -= Points; }
        void SetTeamPoint(Team team, uint32 Points = 0) { m_TeamScores[GetTeamIndexByTeamId(team)] = Points; }

        uint32 m_HonorScoreTics[2];
        uint32 m_TeamPointsCount[BG_TEAMS_COUNT];

        uint32 m_Points_Trigger[BG_EY_NODES_MAX];

        ObjectGuid m_FlagKeeper;                            // keepers guid
        ObjectGuid m_DroppedFlagGuid;
        uint8 m_FlagState;                                  // for checking flag state
        int32 m_FlagsTimer;
        int32 m_TowerCapCheckTimer;

        Team m_PointOwnedByTeam[BG_EY_NODES_MAX];
        uint8 m_PointState[BG_EY_NODES_MAX];
        int32 m_PointBarStatus[BG_EY_NODES_MAX];
        typedef std::vector<ObjectGuid> PlayersNearPointType;
        PlayersNearPointType m_PlayersNearPoint[BG_EY_NODES_MAX_WITH_SPEIAL];
        uint8 m_CurrentPointPlayersCount[2*BG_EY_NODES_MAX];

        int32 m_PointAddingTimer;
        uint32 m_HonorTics;
};
#endif
