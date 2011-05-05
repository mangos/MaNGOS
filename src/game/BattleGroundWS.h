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

#ifndef __BATTLEGROUNDWS_H
#define __BATTLEGROUNDWS_H

#include "BattleGround.h"

#define BG_WS_MAX_TEAM_SCORE      3
#define BG_WS_FLAG_RESPAWN_TIME   (23*IN_MILLISECONDS)
#define BG_WS_FLAG_DROP_TIME      (10*IN_MILLISECONDS)
#define BG_WS_TIME_LIMIT          (25*MINUTE*IN_MILLISECONDS)

enum BG_WS_Sound
{
    BG_WS_SOUND_FLAG_CAPTURED_ALLIANCE  = 8173,
    BG_WS_SOUND_FLAG_CAPTURED_HORDE     = 8213,
    BG_WS_SOUND_FLAG_PLACED             = 8232,
    BG_WS_SOUND_FLAG_RETURNED           = 8192,
    BG_WS_SOUND_HORDE_FLAG_PICKED_UP    = 8212,
    BG_WS_SOUND_ALLIANCE_FLAG_PICKED_UP = 8174,
    BG_WS_SOUND_FLAGS_RESPAWNED         = 8232
};

enum BG_WS_SpellId
{
    BG_WS_SPELL_WARSONG_FLAG            = 23333,
    BG_WS_SPELL_WARSONG_FLAG_DROPPED    = 23334,
    BG_WS_SPELL_SILVERWING_FLAG         = 23335,
    BG_WS_SPELL_SILVERWING_FLAG_DROPPED = 23336
};

enum BG_WS_WorldStates
{
    BG_WS_FLAG_UNK_ALLIANCE       = 1545,
    BG_WS_FLAG_UNK_HORDE          = 1546,
//    FLAG_UNK                      = 1547,
    BG_WS_FLAG_CAPTURES_ALLIANCE  = 1581,
    BG_WS_FLAG_CAPTURES_HORDE     = 1582,
    BG_WS_FLAG_CAPTURES_MAX       = 1601,
    BG_WS_FLAG_STATE_HORDE        = 2338,
    BG_WS_FLAG_STATE_ALLIANCE     = 2339,
    BG_WS_TIME_ENABLED            = 4247,
    BG_WS_TIME_REMAINING          = 4248
};

enum BG_WS_FlagState
{
    BG_WS_FLAG_STATE_ON_BASE      = 0,
    BG_WS_FLAG_STATE_WAIT_RESPAWN = 1,
    BG_WS_FLAG_STATE_ON_PLAYER    = 2,
    BG_WS_FLAG_STATE_ON_GROUND    = 3
};

enum BG_WS_Graveyards
{
    WS_GRAVEYARD_FLAGROOM_ALLIANCE = 769,
    WS_GRAVEYARD_FLAGROOM_HORDE    = 770,
    WS_GRAVEYARD_MAIN_ALLIANCE     = 771,
    WS_GRAVEYARD_MAIN_HORDE        = 772
};

class BattleGroundWGScore : public BattleGroundScore
{
    public:
        BattleGroundWGScore() : FlagCaptures(0), FlagReturns(0) {};
        virtual ~BattleGroundWGScore() {};
        uint32 FlagCaptures;
        uint32 FlagReturns;
};


enum BG_WS_Events
{
    WS_EVENT_FLAG_A               = 0,
    WS_EVENT_FLAG_H               = 1,
    // spiritguides will spawn (same moment, like WS_EVENT_DOOR_OPEN)
    WS_EVENT_SPIRITGUIDES_SPAWN   = 2
};

class BattleGroundWS : public BattleGround
{
    friend class BattleGroundMgr;

    public:
        /* Construction */
        BattleGroundWS();
        ~BattleGroundWS();
        void Update(uint32 diff);

        /* inherited from BattlegroundClass */
        virtual void AddPlayer(Player *plr);
        virtual void StartingEventCloseDoors();
        virtual void StartingEventOpenDoors();

        /* BG Flags */
        ObjectGuid GetAllianceFlagPickerGuid() const{ return m_FlagKeepers[BG_TEAM_ALLIANCE]; }
        ObjectGuid GetHordeFlagPickerGuid() const   { return m_FlagKeepers[BG_TEAM_HORDE]; }
        void SetAllianceFlagPicker(ObjectGuid guid) { m_FlagKeepers[BG_TEAM_ALLIANCE] = guid; }
        void SetHordeFlagPicker(ObjectGuid guid)    { m_FlagKeepers[BG_TEAM_HORDE] = guid; }
        void ClearAllianceFlagPicker()              { m_FlagKeepers[BG_TEAM_ALLIANCE].Clear(); }
        void ClearHordeFlagPicker()                 { m_FlagKeepers[BG_TEAM_HORDE].Clear(); }
        bool IsAllianceFlagPickedup() const         { return !m_FlagKeepers[BG_TEAM_ALLIANCE].IsEmpty(); }
        bool IsHordeFlagPickedup() const            { return !m_FlagKeepers[BG_TEAM_HORDE].IsEmpty(); }
        void RespawnFlag(Team team, bool captured);
        void RespawnFlagAfterDrop(Team team);
        uint8 GetFlagState(Team team)             { return m_FlagState[GetTeamIndexByTeamId(team)]; }

        /* Battleground Events */
        virtual void EventPlayerDroppedFlag(Player *Source);
        virtual void EventPlayerClickedOnFlag(Player *Source, GameObject* target_obj);
        virtual void EventPlayerCapturedFlag(Player *Source);

        void RemovePlayer(Player *plr, ObjectGuid guid);
        void HandleAreaTrigger(Player *Source, uint32 Trigger);
        void HandleKillPlayer(Player *player, Player *killer);
        bool SetupBattleGround();
        virtual void Reset();
        void EndBattleGround(Team winner);
        virtual WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);
        uint32 GetRemainingTimeInMinutes() { return m_EndTimer ? (m_EndTimer-1) / (MINUTE * IN_MILLISECONDS) + 1 : 0; }

        void UpdateFlagState(Team team, uint32 value);
        void UpdateTeamScore(Team team);
        void UpdatePlayerScore(Player *Source, uint32 type, uint32 value);
        void SetDroppedFlagGuid(ObjectGuid guid, Team team)  { m_DroppedFlagGuid[GetTeamIndexByTeamId(team)] = guid;}
        void ClearDroppedFlagGuid(Team team)  { m_DroppedFlagGuid[GetTeamIndexByTeamId(team)].Clear();}
        ObjectGuid const& GetDroppedFlagGuid(Team team) const { return m_DroppedFlagGuid[GetTeamIndexByTeamId(team)];}
        virtual void FillInitialWorldStates(WorldPacket& data, uint32& count);

        /* Scorekeeping */
        uint32 GetTeamScore(Team team) const            { return m_TeamScores[GetTeamIndexByTeamId(team)]; }
        void AddPoint(Team team, uint32 Points = 1)     { m_TeamScores[GetTeamIndexByTeamId(team)] += Points; }
        void SetTeamPoint(Team team, uint32 Points = 0) { m_TeamScores[GetTeamIndexByTeamId(team)] = Points; }
        void RemovePoint(Team team, uint32 Points = 1)  { m_TeamScores[GetTeamIndexByTeamId(team)] -= Points; }
    private:
        ObjectGuid m_FlagKeepers[BG_TEAMS_COUNT];

        ObjectGuid m_DroppedFlagGuid[BG_TEAMS_COUNT];
        uint8 m_FlagState[BG_TEAMS_COUNT];
        int32 m_FlagsTimer[BG_TEAMS_COUNT];
        int32 m_FlagsDropTimer[BG_TEAMS_COUNT];

        uint32 m_ReputationCapture;
        uint32 m_HonorWinKills;
        uint32 m_HonorEndKills;
        uint32 m_EndTimer;
        Team   m_LastCapturedFlagTeam;
};
#endif
