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

#ifndef __BATTLEGROUNDEY_H
#define __BATTLEGROUNDEY_H

#include "Language.h"

class BattleGround;

#define EY_FLAG_RESPAWN_TIME            (10 * IN_MILLISECONDS) //10 seconds
#define EY_RESOURCES_UPDATE_TIME        (2 * IN_MILLISECONDS) //2 seconds

enum EYWorldStates
{
    WORLD_STATE_EY_RESOURCES_ALLIANCE                   = 2749,
    WORLD_STATE_EY_RESOURCES_HORDE                      = 2750,
    WORLD_STATE_EY_TOWER_COUNT_ALLIANCE                 = 2752,
    WORLD_STATE_EY_TOWER_COUNT_HORDE                    = 2753,

    WORLD_STATE_EY_BLOOD_ELF_TOWER_ALLIANCE             = 2723,
    WORLD_STATE_EY_BLOOD_ELF_TOWER_HORDE                = 2724,
    WORLD_STATE_EY_BLOOD_ELF_TOWER_NEUTRAL              = 2722,
    //WORLD_STATE_EY_BLOOD_ELF_TOWER_ALLIANCE_CONFLICT  = 2735, // unused on retail
    //WORLD_STATE_EY_BLOOD_ELF_TOWER_HORDE_CONFLICT     = 2736, // unused on retail

    WORLD_STATE_EY_FEL_REAVER_RUINS_ALLIANCE            = 2726,
    WORLD_STATE_EY_FEL_REAVER_RUINS_HORDE               = 2727,
    WORLD_STATE_EY_FEL_REAVER_RUINS_NEUTRAL             = 2725,
    //WORLD_STATE_EY_FEL_REAVER_RUINS_ALLIANCE_CONFLICT = 2739, // unused on retail
    //WORLD_STATE_EY_FEL_REAVER_RUINS_HORDE_CONFLICT    = 2740, // unused on retail

    WORLD_STATE_EY_MAGE_TOWER_ALLIANCE                  = 2730,
    WORLD_STATE_EY_MAGE_TOWER_HORDE                     = 2729,
    WORLD_STATE_EY_MAGE_TOWER_NEUTRAL                   = 2728,
    //WORLD_STATE_EY_MAGE_TOWER_ALLIANCE_CONFLICT       = 2741, // unused on retail
    //WORLD_STATE_EY_MAGE_TOWER_HORDE_CONFLICT          = 2742, // unused on retail

    WORLD_STATE_EY_DRAENEI_RUINS_ALLIANCE               = 2732,
    WORLD_STATE_EY_DRAENEI_RUINS_HORDE                  = 2733,
    WORLD_STATE_EY_DRAENEI_RUINS_NEUTRAL                = 2731,
    //WORLD_STATE_EY_DRAENEI_RUINS_ALLIANCE_CONFLICT    = 2738, // unused on retail
    //WORLD_STATE_EY_DRAENEI_RUINS_HORDE_CONFLICT       = 2737, // unused on retail

    WORLD_STATE_EY_NETHERSTORM_FLAG_READY               = 2757,
    WORLD_STATE_EY_NETHERSTORM_FLAG_STATE_ALLIANCE      = 2769,
    WORLD_STATE_EY_NETHERSTORM_FLAG_STATE_HORDE         = 2770,

    WORLD_STATE_EY_CAPTURE_POINT_SLIDER_DISPLAY         = 2718
};

enum EYCapturePoints
{
    GO_CAPTURE_POINT_BLOOD_ELF_TOWER            = 184080,
    GO_CAPTURE_POINT_FEL_REAVER_RUINS           = 184081,
    GO_CAPTURE_POINT_MAGE_TOWER                 = 184082,
    GO_CAPTURE_POINT_DRAENEI_RUINS              = 184083
};

enum EYEvents
{
    //EVENT_BLOOD_ELF_TOWER_WIN_ALLIANCE        = 12965,
    //EVENT_BLOOD_ELF_TOWER_WIN_HORDE           = 12964,
    EVENT_BLOOD_ELF_TOWER_PROGRESS_ALLIANCE     = 12905,
    EVENT_BLOOD_ELF_TOWER_PROGRESS_HORDE        = 12904,
    EVENT_BLOOD_ELF_TOWER_NEUTRAL_ALLIANCE      = 12957,
    EVENT_BLOOD_ELF_TOWER_NEUTRAL_HORDE         = 12956,

    //EVENT_FEL_REAVER_RUINS_WIN_ALLIANCE       = 12969,
    //EVENT_FEL_REAVER_RUINS_WIN_HORDE          = 12968,
    EVENT_FEL_REAVER_RUINS_PROGRESS_ALLIANCE    = 12911,
    EVENT_FEL_REAVER_RUINS_PROGRESS_HORDE       = 12910,
    EVENT_FEL_REAVER_RUINS_NEUTRAL_ALLIANCE     = 12960,
    EVENT_FEL_REAVER_RUINS_NEUTRAL_HORDE        = 12961,

    //EVENT_MAGE_TOWER_WIN_ALLIANCE             = 12971,
    //EVENT_MAGE_TOWER_WIN_HORDE                = 12970,
    EVENT_MAGE_TOWER_PROGRESS_ALLIANCE          = 12909,
    EVENT_MAGE_TOWER_PROGRESS_HORDE             = 12908,
    EVENT_MAGE_TOWER_NEUTRAL_ALLIANCE           = 12962,
    EVENT_MAGE_TOWER_NEUTRAL_HORDE              = 12963,

    //EVENT_DRAENEI_RUINS_WIN_ALLIANCE          = 12967,
    //EVENT_DRAENEI_RUINS_WIN_HORDE             = 12966,
    EVENT_DRAENEI_RUINS_PROGRESS_ALLIANCE       = 12907,
    EVENT_DRAENEI_RUINS_PROGRESS_HORDE          = 12906,
    EVENT_DRAENEI_RUINS_NEUTRAL_ALLIANCE        = 12958,
    EVENT_DRAENEI_RUINS_NEUTRAL_HORDE           = 12959
};

enum EYSounds
{
    EY_SOUND_FLAG_PICKED_UP_ALLIANCE    = 8212,
    EY_SOUND_FLAG_CAPTURED_HORDE        = 8213,
    EY_SOUND_FLAG_PICKED_UP_HORDE       = 8174,
    EY_SOUND_FLAG_CAPTURED_ALLIANCE     = 8173,
    EY_SOUND_FLAG_RESET                 = 8192
};

enum EYSpells
{
    EY_NETHERSTORM_FLAG_SPELL           = 34976,
    EY_PLAYER_DROPPED_FLAG_SPELL        = 34991
};

enum EYPointsTrigger
{
    AREATRIGGER_BLOOD_ELF_TOWER_POINT   = 4476, // also 4512
    AREATRIGGER_FEL_REAVER_RUINS_POINT  = 4514, // also 4515
    AREATRIGGER_MAGE_TOWER_POINT        = 4516, // also 4517
    AREATRIGGER_DRAENEI_RUINS_POINT     = 4518, // also 4519

    AREATRIGGER_BLOOD_ELF_TOWER_BUFF    = 4568,
    AREATRIGGER_FEL_REAVER_RUINS_BUFF   = 4569,
    //AREATRIGGER_FEL_REAVER_RUINS_BUFF_2 = 5866,
    AREATRIGGER_MAGE_TOWER_BUFF         = 4570,
    AREATRIGGER_DRAENEI_RUINS_BUFF      = 4571

    //AREATRIGGER_EY_HORDE_START          = 4530,
    //AREATRIGGER_EY_ALLIANCE_START       = 4531
};

enum EYGaveyards
{
    GRAVEYARD_EY_MAIN_ALLIANCE      = 1103,
    GRAVEYARD_EY_MAIN_HORDE         = 1104,
    GRAVEYARD_FEL_REAVER_RUINS      = 1105,
    GRAVEYARD_BLOOD_ELF_TOWER       = 1106,
    GRAVEYARD_DRAENEI_RUINS         = 1107,
    GRAVEYARD_MAGE_TOWER            = 1108
};

enum EYNodes
{
    // TODO: Re-change order after we drop battleground_event and associated tables
    NODE_BLOOD_ELF_TOWER            = 1,
    NODE_FEL_REAVER_RUINS           = 0,
    NODE_MAGE_TOWER                 = 3,
    NODE_DRAENEI_RUINS              = 2
};

#define EY_NODES_MAX 4

// node-events work like this: event1:nodeid, event2:state (0alliance,1horde,2neutral)
#define EY_NEUTRAL_TEAM 2
#define EY_EVENT_CAPTURE_FLAG 4 // event1=4, event2=nodeid or 4 for the default center spawn
#define EY_EVENT2_FLAG_CENTER 4 // maximum node is 3 so 4 for center is ok
// all other event2 are just nodeids, i won't define something here

enum EYBuffs
{
    // buffs
    EY_OBJECT_SPEEDBUFF_FEL_REAVER_RUINS    = 1,
    EY_OBJECT_REGENBUFF_FEL_REAVER_RUINS    = 2,
    EY_OBJECT_BERSERKBUFF_FEL_REAVER_RUINS  = 3,
    EY_OBJECT_SPEEDBUFF_BLOOD_ELF_TOWER     = 4,
    EY_OBJECT_REGENBUFF_BLOOD_ELF_TOWER     = 5,
    EY_OBJECT_BERSERKBUFF_BLOOD_ELF_TOWER   = 6,
    EY_OBJECT_SPEEDBUFF_DRAENEI_RUINS       = 7,
    EY_OBJECT_REGENBUFF_DRAENEI_RUINS       = 8,
    EY_OBJECT_BERSERKBUFF_DRAENEI_RUINS     = 9,
    EY_OBJECT_SPEEDBUFF_MAGE_TOWER          = 10,
    EY_OBJECT_REGENBUFF_MAGE_TOWER          = 11,
    EY_OBJECT_BERSERKBUFF_MAGE_TOWER        = 12,
    EY_OBJECT_MAX                           = 13
};

#define EY_NORMAL_HONOR_INTERVAL        260
#define EY_WEEKEND_HONOR_INTERVAL       160
#define EY_EVENT_START_BATTLE           13180

enum EYScore
{
    EY_WARNING_NEAR_VICTORY_SCORE       = 1400,
    EY_MAX_TEAM_SCORE                   = 1600
};

enum EYFlagState
{
    EY_FLAG_STATE_ON_BASE               = 0,
    EY_FLAG_STATE_WAIT_RESPAWN          = 1,
    EY_FLAG_STATE_ON_ALLIANCE_PLAYER    = 2,
    EY_FLAG_STATE_ON_HORDE_PLAYER       = 3,
    EY_FLAG_STATE_ON_GROUND             = 4
};

static const uint8 eyTickPoints[EY_NODES_MAX] = {1, 2, 5, 10};
static const uint32 eyFlagPoints[EY_NODES_MAX] = {75, 85, 100, 500};

static const uint32 eyGraveyards[EY_NODES_MAX] = {GRAVEYARD_FEL_REAVER_RUINS, GRAVEYARD_BLOOD_ELF_TOWER, GRAVEYARD_DRAENEI_RUINS, GRAVEYARD_MAGE_TOWER};
static const uint32 eyTriggers[EY_NODES_MAX] = {AREATRIGGER_FEL_REAVER_RUINS_BUFF, AREATRIGGER_BLOOD_ELF_TOWER_BUFF, AREATRIGGER_DRAENEI_RUINS_BUFF, AREATRIGGER_MAGE_TOWER_BUFF};

struct EYTowerEvent
{
    uint32  eventEntry;
    Team    team;
    uint32  message;
    uint32  worldState;
};

static const EYTowerEvent eyTowerEvents[EY_NODES_MAX][4] =
{
    {
        {EVENT_FEL_REAVER_RUINS_PROGRESS_ALLIANCE,  ALLIANCE,   LANG_BG_EY_HAS_TAKEN_A_B_TOWER, WORLD_STATE_EY_FEL_REAVER_RUINS_ALLIANCE},
        {EVENT_FEL_REAVER_RUINS_PROGRESS_HORDE,     HORDE,      LANG_BG_EY_HAS_TAKEN_H_F_RUINS, WORLD_STATE_EY_FEL_REAVER_RUINS_HORDE},
        {EVENT_FEL_REAVER_RUINS_NEUTRAL_HORDE,      TEAM_NONE,  LANG_BG_EY_HAS_LOST_A_F_RUINS,  WORLD_STATE_EY_FEL_REAVER_RUINS_NEUTRAL},
        {EVENT_FEL_REAVER_RUINS_NEUTRAL_ALLIANCE,   TEAM_NONE,  LANG_BG_EY_HAS_LOST_H_F_RUINS,  WORLD_STATE_EY_FEL_REAVER_RUINS_NEUTRAL},
    },
    {
        {EVENT_BLOOD_ELF_TOWER_PROGRESS_ALLIANCE,   ALLIANCE,   LANG_BG_EY_HAS_TAKEN_A_B_TOWER, WORLD_STATE_EY_BLOOD_ELF_TOWER_ALLIANCE},
        {EVENT_BLOOD_ELF_TOWER_PROGRESS_HORDE,      HORDE,      LANG_BG_EY_HAS_TAKEN_H_B_TOWER, WORLD_STATE_EY_BLOOD_ELF_TOWER_HORDE},
        {EVENT_BLOOD_ELF_TOWER_NEUTRAL_HORDE,       TEAM_NONE,  LANG_BG_EY_HAS_LOST_A_B_TOWER,  WORLD_STATE_EY_BLOOD_ELF_TOWER_NEUTRAL},
        {EVENT_BLOOD_ELF_TOWER_NEUTRAL_ALLIANCE,    TEAM_NONE,  LANG_BG_EY_HAS_LOST_H_B_TOWER,  WORLD_STATE_EY_BLOOD_ELF_TOWER_NEUTRAL},
    },
    {
        {EVENT_DRAENEI_RUINS_PROGRESS_ALLIANCE,     ALLIANCE,   LANG_BG_EY_HAS_TAKEN_A_D_RUINS, WORLD_STATE_EY_DRAENEI_RUINS_ALLIANCE},
        {EVENT_DRAENEI_RUINS_PROGRESS_HORDE,        HORDE,      LANG_BG_EY_HAS_TAKEN_H_D_RUINS, WORLD_STATE_EY_DRAENEI_RUINS_HORDE},
        {EVENT_DRAENEI_RUINS_NEUTRAL_HORDE,         TEAM_NONE,  LANG_BG_EY_HAS_LOST_A_D_RUINS,  WORLD_STATE_EY_DRAENEI_RUINS_NEUTRAL},
        {EVENT_DRAENEI_RUINS_NEUTRAL_ALLIANCE,      TEAM_NONE,  LANG_BG_EY_HAS_LOST_H_D_RUINS,  WORLD_STATE_EY_DRAENEI_RUINS_NEUTRAL},
    },
    {
        {EVENT_MAGE_TOWER_PROGRESS_ALLIANCE,        ALLIANCE,   LANG_BG_EY_HAS_TAKEN_A_M_TOWER, WORLD_STATE_EY_MAGE_TOWER_ALLIANCE},
        {EVENT_MAGE_TOWER_PROGRESS_HORDE,           HORDE,      LANG_BG_EY_HAS_TAKEN_H_M_TOWER, WORLD_STATE_EY_MAGE_TOWER_HORDE},
        {EVENT_MAGE_TOWER_NEUTRAL_HORDE,            TEAM_NONE,  LANG_BG_EY_HAS_LOST_A_M_TOWER,  WORLD_STATE_EY_MAGE_TOWER_NEUTRAL},
        {EVENT_MAGE_TOWER_NEUTRAL_ALLIANCE,         TEAM_NONE,  LANG_BG_EY_HAS_LOST_H_M_TOWER,  WORLD_STATE_EY_MAGE_TOWER_NEUTRAL},
    },
};

static const uint32 eyTowers[EY_NODES_MAX] = {GO_CAPTURE_POINT_FEL_REAVER_RUINS, GO_CAPTURE_POINT_BLOOD_ELF_TOWER, GO_CAPTURE_POINT_DRAENEI_RUINS, GO_CAPTURE_POINT_MAGE_TOWER};

class BattleGroundEYScore : public BattleGroundScore
{
    public:
        BattleGroundEYScore() : FlagCaptures(0) {};
        virtual ~BattleGroundEYScore() {};
        uint32 FlagCaptures;
};

class BattleGroundEY : public BattleGround
{
        friend class BattleGroundMgr;

    public:
        BattleGroundEY();
        ~BattleGroundEY();
        void Update(uint32 diff) override;

        /* inherited from BattlegroundClass */
        virtual void AddPlayer(Player* plr) override;
        virtual void StartingEventCloseDoors() override;
        virtual void StartingEventOpenDoors() override;

        /* BG Flags */
        ObjectGuid const& GetFlagCarrierGuid() const { return m_flagCarrier; }
        void SetFlagCarrier(ObjectGuid guid) { m_flagCarrier = guid; }
        void ClearFlagCarrier() { m_flagCarrier.Clear(); }
        bool IsFlagPickedUp() const  { return !m_flagCarrier.IsEmpty(); }
        uint8 GetFlagState() const { return m_flagState; }
        void RespawnFlag();
        void RespawnDroppedFlag();

        void RemovePlayer(Player* plr, ObjectGuid guid) override;
        bool HandleEvent(uint32 eventId, GameObject* go) override;
        void HandleGameObjectCreate(GameObject* go) override;
        void HandleAreaTrigger(Player* source, uint32 trigger) override;
        void HandleKillPlayer(Player* player, Player* killer) override;

        virtual WorldSafeLocsEntry const* GetClosestGraveYard(Player* player) override;
        virtual bool SetupBattleGround() override;
        virtual void Reset() override;
        void UpdateTeamScore(Team team);
        void EndBattleGround(Team winner) override;
        void UpdatePlayerScore(Player* source, uint32 type, uint32 value) override;
        virtual void FillInitialWorldStates(WorldPacket& data, uint32& count) override;
        void SetDroppedFlagGuid(ObjectGuid guid)     { m_DroppedFlagGuid = guid;}
        void ClearDroppedFlagGuid()                  { m_DroppedFlagGuid.Clear();}
        ObjectGuid const& GetDroppedFlagGuid() const { return m_DroppedFlagGuid;}

        /* Battleground Events */
        virtual void EventPlayerClickedOnFlag(Player* source, GameObject* target_obj) override;
        virtual void EventPlayerDroppedFlag(Player* source) override;

        /* achievement req. */
        bool IsAllNodesControlledByTeam(Team team) const override;

    private:
        // process capture events
        void ProcessCaptureEvent(GameObject* go, uint32 towerId, Team team, uint32 newWorldState, uint32 message);
        void EventPlayerCapturedFlag(Player* source, EYNodes node);     // NOTE: virtual BattleGround::EventPlayerCapturedFlag has different parameters list
        void UpdateResources();

        /* Scorekeeping */
        void AddPoints(Team team, uint32 points);

        EYFlagState m_flagState;
        ObjectGuid m_flagCarrier;
        ObjectGuid m_DroppedFlagGuid;

        uint8 m_towersAlliance;
        uint8 m_towersHorde;

        uint32 m_towerWorldState[EY_NODES_MAX];

        Team m_towerOwner[EY_NODES_MAX];
        ObjectGuid m_towers[EY_NODES_MAX];

        uint32 m_honorTicks;
        uint32 m_honorScoreTicks[BG_TEAMS_COUNT];

        uint32 m_flagRespawnTimer;
        uint32 m_resourceUpdateTimer;
};
#endif
