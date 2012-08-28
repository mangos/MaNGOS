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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef WORLD_PVP_TF
#define WORLD_PVP_TF

#include "Common.h"
#include "OutdoorPvP.h"
#include "Language.h"

enum
{
    MAX_TF_TOWERS                               = 5,

    // gameobjects
    GO_TOWER_BANNER_WEST                        = 183104,
    GO_TOWER_BANNER_NORTH                       = 183411,
    GO_TOWER_BANNER_EAST                        = 183412,
    GO_TOWER_BANNER_SOUTH_EAST                  = 183413,
    GO_TOWER_BANNER_SOUTH                       = 183414,

    // spells
    SPELL_AUCHINDOUN_BLESSING                   = 33377,

    // timers
    TIMER_TF_LOCK_TIME                          = 6 * HOUR * IN_MILLISECONDS,
    //TIMER_TF_UPDATE_TIME                        = MINUTE * IN_MILLISECONDS,

    // quests
    QUEST_SPIRITS_OF_AUCHINDOUM_ALLIANCE        = 11505,
    QUEST_SPIRITS_OF_AUCHINDOUM_HORDE           = 11506,

    // events
    EVENT_WEST_TOWER_PROGRESS_ALLIANCE          = 12226,
    EVENT_WEST_TOWER_PROGRESS_HORDE             = 12225,
    EVENT_WEST_TOWER_NEUTRAL_ALLIANCE           = 12228,
    EVENT_WEST_TOWER_NEUTRAL_HORDE              = 12227,

    EVENT_NORTH_TOWER_PROGRESS_ALLIANCE         = 12497,
    EVENT_NORTH_TOWER_PROGRESS_HORDE            = 12496,
    EVENT_NORTH_TOWER_NEUTRAL_ALLIANCE          = 12490,
    EVENT_NORTH_TOWER_NEUTRAL_HORDE             = 12491,

    EVENT_EAST_TOWER_PROGRESS_ALLIANCE          = 12486,
    EVENT_EAST_TOWER_PROGRESS_HORDE             = 12487,
    EVENT_EAST_TOWER_NEUTRAL_ALLIANCE           = 12488,
    EVENT_EAST_TOWER_NEUTRAL_HORDE              = 12489,

    EVENT_SOUTH_EAST_TOWER_PROGRESS_ALLIANCE    = 12499,
    EVENT_SOUTH_EAST_TOWER_PROGRESS_HORDE       = 12498,
    EVENT_SOUTH_EAST_TOWER_NEUTRAL_ALLIANCE     = 12492,
    EVENT_SOUTH_EAST_TOWER_NEUTRAL_HORDE        = 12493,

    EVENT_SOUTH_TOWER_PROGRESS_ALLIANCE         = 12501,
    EVENT_SOUTH_TOWER_PROGRESS_HORDE            = 12500,
    EVENT_SOUTH_TOWER_NEUTRAL_ALLIANCE          = 12494,
    EVENT_SOUTH_TOWER_NEUTRAL_HORDE             = 12495,

    // world states
    // tower counter before the lock event
    WORLD_STATE_TF_TOWER_COUNT_H                = 2622,
    WORLD_STATE_TF_TOWER_COUNT_A                = 2621,
    WORLD_STATE_TF_TOWERS_CONTROLLED            = 2620,

    // timer for the lock event
    WORLD_STATE_TF_TIME_MIN_FIRST_DIGIT         = 2512,
    WORLD_STATE_TF_TIME_MIN_SECOND_DIGIT        = 2510,
    WORLD_STATE_TF_TIME_HOURS                   = 2509,

    // lock period - factions
    WORLD_STATE_TF_LOCKED_NEUTRAL               = 2508,
    WORLD_STATE_TF_LOCKED_HORDE                 = 2768,
    WORLD_STATE_TF_LOCKED_ALLIANCE              = 2767,

    // tower world states
    WORLD_STATE_TF_WEST_TOWER_ALLIANCE          = 2683,
    WORLD_STATE_TF_WEST_TOWER_HORDE             = 2682,
    WORLD_STATE_TF_WEST_TOWER_NEUTRAL           = 2681,

    WORLD_STATE_TF_NORTH_TOWER_ALLIANCE         = 2684,
    WORLD_STATE_TF_NORTH_TOWER_HORDE            = 2685,
    WORLD_STATE_TF_NORTH_TOWER_NEUTRAL          = 2686,

    WORLD_STATE_TF_EAST_TOWER_ALLIANCE          = 2688,
    WORLD_STATE_TF_EAST_TOWER_HORDE             = 2689,
    WORLD_STATE_TF_EAST_TOWER_NEUTRAL           = 2690,

    WORLD_STATE_TF_SOUTH_EAST_TOWER_ALLIANCE    = 2694,
    WORLD_STATE_TF_SOUTH_EAST_TOWER_HORDE       = 2695,
    WORLD_STATE_TF_SOUTH_EAST_TOWER_NEUTRAL     = 2696,

    WORLD_STATE_TF_SOUTH_TOWER_ALLIANCE         = 2691,
    WORLD_STATE_TF_SOUTH_TOWER_HORDE            = 2692,
    WORLD_STATE_TF_SOUTH_TOWER_NEUTRAL          = 2693
};

struct TerokkarTowerEvent
{
    uint32  eventEntry;
    Team    team;
    uint32  defenseMessage;
    uint32  worldState;
};

static const TerokkarTowerEvent terokkarTowerEvents[MAX_TF_TOWERS][4] =
{
    {
        {EVENT_WEST_TOWER_PROGRESS_ALLIANCE,        ALLIANCE,   LANG_OPVP_TF_CAPTURE_TOWER_A,   WORLD_STATE_TF_WEST_TOWER_ALLIANCE},
        {EVENT_WEST_TOWER_PROGRESS_HORDE,           HORDE,      LANG_OPVP_TF_CAPTURE_TOWER_H,   WORLD_STATE_TF_WEST_TOWER_HORDE},
        {EVENT_WEST_TOWER_NEUTRAL_HORDE,            TEAM_NONE,  LANG_OPVP_TF_LOSE_TOWER_A,      WORLD_STATE_TF_WEST_TOWER_NEUTRAL},
        {EVENT_WEST_TOWER_NEUTRAL_ALLIANCE,         TEAM_NONE,  LANG_OPVP_TF_LOSE_TOWER_H,      WORLD_STATE_TF_WEST_TOWER_NEUTRAL},
    },
    {
        {EVENT_NORTH_TOWER_PROGRESS_ALLIANCE,       ALLIANCE,   LANG_OPVP_TF_CAPTURE_TOWER_A,   WORLD_STATE_TF_NORTH_TOWER_ALLIANCE},
        {EVENT_NORTH_TOWER_PROGRESS_HORDE,          HORDE,      LANG_OPVP_TF_CAPTURE_TOWER_H,   WORLD_STATE_TF_NORTH_TOWER_HORDE},
        {EVENT_NORTH_TOWER_NEUTRAL_HORDE,           TEAM_NONE,  LANG_OPVP_TF_LOSE_TOWER_A,      WORLD_STATE_TF_NORTH_TOWER_NEUTRAL},
        {EVENT_NORTH_TOWER_NEUTRAL_ALLIANCE,        TEAM_NONE,  LANG_OPVP_TF_LOSE_TOWER_H,      WORLD_STATE_TF_NORTH_TOWER_NEUTRAL},
    },
    {
        {EVENT_EAST_TOWER_PROGRESS_ALLIANCE,        ALLIANCE,   LANG_OPVP_TF_CAPTURE_TOWER_A,   WORLD_STATE_TF_EAST_TOWER_ALLIANCE},
        {EVENT_EAST_TOWER_PROGRESS_HORDE,           HORDE,      LANG_OPVP_TF_CAPTURE_TOWER_H,   WORLD_STATE_TF_EAST_TOWER_HORDE},
        {EVENT_EAST_TOWER_NEUTRAL_HORDE,            TEAM_NONE,  LANG_OPVP_TF_LOSE_TOWER_A,      WORLD_STATE_TF_EAST_TOWER_NEUTRAL},
        {EVENT_EAST_TOWER_NEUTRAL_ALLIANCE,         TEAM_NONE,  LANG_OPVP_TF_LOSE_TOWER_H,      WORLD_STATE_TF_EAST_TOWER_NEUTRAL},
    },
    {
        {EVENT_SOUTH_EAST_TOWER_PROGRESS_ALLIANCE,  ALLIANCE,   LANG_OPVP_TF_CAPTURE_TOWER_A,   WORLD_STATE_TF_SOUTH_EAST_TOWER_ALLIANCE},
        {EVENT_SOUTH_EAST_TOWER_PROGRESS_HORDE,     HORDE,      LANG_OPVP_TF_CAPTURE_TOWER_H,   WORLD_STATE_TF_SOUTH_EAST_TOWER_HORDE},
        {EVENT_SOUTH_EAST_TOWER_NEUTRAL_HORDE,      TEAM_NONE,  LANG_OPVP_TF_LOSE_TOWER_A,      WORLD_STATE_TF_SOUTH_EAST_TOWER_NEUTRAL},
        {EVENT_SOUTH_EAST_TOWER_NEUTRAL_ALLIANCE,   TEAM_NONE,  LANG_OPVP_TF_LOSE_TOWER_H,      WORLD_STATE_TF_SOUTH_EAST_TOWER_NEUTRAL},
    },
    {
        {EVENT_SOUTH_TOWER_PROGRESS_ALLIANCE,       ALLIANCE,   LANG_OPVP_TF_CAPTURE_TOWER_A,   WORLD_STATE_TF_SOUTH_TOWER_ALLIANCE},
        {EVENT_SOUTH_TOWER_PROGRESS_HORDE,          HORDE,      LANG_OPVP_TF_CAPTURE_TOWER_H,   WORLD_STATE_TF_SOUTH_TOWER_HORDE},
        {EVENT_SOUTH_TOWER_NEUTRAL_HORDE,           TEAM_NONE,  LANG_OPVP_TF_LOSE_TOWER_A,      WORLD_STATE_TF_SOUTH_TOWER_NEUTRAL},
        {EVENT_SOUTH_TOWER_NEUTRAL_ALLIANCE,        TEAM_NONE,  LANG_OPVP_TF_LOSE_TOWER_H,      WORLD_STATE_TF_SOUTH_TOWER_NEUTRAL},
    },
};

static const uint32 terokkarTowers[MAX_TF_TOWERS] = {GO_TOWER_BANNER_WEST, GO_TOWER_BANNER_NORTH, GO_TOWER_BANNER_EAST, GO_TOWER_BANNER_SOUTH_EAST, GO_TOWER_BANNER_SOUTH};

class OutdoorPvPTF : public OutdoorPvP
{
    friend class OutdoorPvPMgr;

    public:
        OutdoorPvPTF();

        void HandlePlayerEnterZone(Player* player, bool isMainZone) override;
        void HandlePlayerLeaveZone(Player* player, bool isMainZone) override;
        void FillInitialWorldStates(WorldPacket& data, uint32& count) override;
        void SendRemoveWorldStates(Player* player) override;

        bool HandleEvent(uint32 eventId, GameObject* go) override;
        void HandleObjectiveComplete(uint32 eventId, std::list<Player*> players, Team team) override;

        void HandleGameObjectCreate(GameObject* go) override;
        void Update(uint32 diff) override;

    private:
        void UpdateTimerWorldState();

        // process capture events
        bool ProcessCaptureEvent(GameObject* go, uint32 towerId, Team team, uint32 newWorldState);

        void LockZone(GameObject* go, uint32 towerId, Team team, uint32 newWorldState);
        void UnlockZone();

        void LockTowers(const WorldObject* objRef);
        void ResetTowers(const WorldObject* objRef);

        uint32 m_towerWorldState[MAX_TF_TOWERS];
        uint32 m_zoneWorldState;

        Team m_towerOwner[MAX_TF_TOWERS];
        Team m_zoneOwner;

        uint32 m_zoneLockTimer;
        //uint32 m_zoneUpdateTimer;

        uint8 m_towersAlliance;
        uint8 m_towersHorde;

        ObjectGuid m_towerBanners[MAX_TF_TOWERS];
};

#endif
