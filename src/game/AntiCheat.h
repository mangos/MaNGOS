/*
 * Copyright (C) 2010 /dev/rsa for MaNGOS <http://getmangos.com/>
 * based on gimly && CWN code
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
#include <string>

class Player;
class MovementInfo;

enum AntiCheatCheck
{
    // Check types
    CHECK_NULL              = 0,
    CHECK_MOVEMENT          = 1,
    CHECK_SPELL             = 2,
    CHECK_QUEST             = 3,
    CHECK_TRANSPORT         = 4,
    CHECK_DAMAGE            = 5,
    // Check subtypes
    // Movement checks
    CHECK_MOVEMENT_SPEED        = 100 * CHECK_MOVEMENT + 1,
    CHECK_MOVEMENT_FLY          = 100 * CHECK_MOVEMENT + 2,
    CHECK_MOVEMENT_MOUNTAIN     = 100 * CHECK_MOVEMENT + 3,
    CHECK_MOVEMENT_WATERWALKING = 100 * CHECK_MOVEMENT + 4,
    CHECK_MOVEMENT_TP2PLANE     = 100 * CHECK_MOVEMENT + 5,
    CHECK_MOVEMENT_AIRJUMP      = 100 * CHECK_MOVEMENT + 6,
    CHECK_MOVEMENT_TELEPORT     = 100 * CHECK_MOVEMENT + 7,
    CHECK_MOVEMENT_FALL         = 100 * CHECK_MOVEMENT + 8,
    // Spell checks
    CHECK_SPELL_VALID           = 100 * CHECK_SPELL + 1,
    CHECK_SPELL_ONDEATH         = 100 * CHECK_SPELL + 2,
    CHECK_SPELL_FAMILY          = 100 * CHECK_SPELL + 3,
    CHECK_SPELL_INBOOK          = 100 * CHECK_SPELL + 4,
    // Damage checks
    CHECK_DAMAGE_SPELL          = 100 * CHECK_DAMAGE + 1,
    CHECK_DAMAGE_MELEE          = 100 * CHECK_DAMAGE + 2,
    // End of list
    CHECK_MAX
};

enum AntiCheatAction
{
    ANTICHEAT_ACTION_NULL             = 0,
    ANTICHEAT_ACTION_LOG              = 1,
    ANTICHEAT_ACTION_ANNOUNCE_GM      = 2,
    ANTICHEAT_ACTION_ANNOUNCE_ALL     = 3,
    ANTICHEAT_ACTION_KICK             = 4,
    ANTICHEAT_ACTION_BAN              = 5,
    ANTICHEAT_ACTION_SHEEP            = 6,
    ANTICHEAT_ACTION_STUN             = 7,
    ANTICHEAT_ACTION_SICKNESS         = 8,
};

struct AntiCheatCheckEntry;
struct AntiCheatConfig;

class AntiCheat
{

    public:
        explicit AntiCheat(Player* player);
        ~AntiCheat();

        // External used for set variables
        void       SetTimeSkipped(uint32 time_skipped) { m_currentTimeSkipped = time_skipped; }
        void       SetInFall(bool isFall) { m_isFall = isFall; }
        bool       isCanFly();
        bool       isInFall();
        bool       isActiveMover() { return m_isActiveMover; }
        void       SetActiveMover(bool isActive) { m_isActiveMover = isActive; }
        bool       isImmune();
        void       SetImmune(uint32 timeDelta);
        void       SetLastLiveState(DeathState state);

        // Checks
        bool CheckNeeded(AntiCheatCheck checktype);

        // Check selectors
        bool DoAntiCheatCheck(AntiCheatCheck checkType, MovementInfo& movementInfo, uint32 opcode = 0)
            {
                m_currentmovementInfo = &movementInfo; 
                m_currentOpcode = opcode;
                return _DoAntiCheatCheck(checkType);
            }

        bool DoAntiCheatCheck(AntiCheatCheck checkType, uint32 spellID, uint32 opcode = 0, uint32 damage = 0)
            {
                m_currentspellID = spellID;
                m_currentOpcode = opcode;
                m_currentDamage = damage;
                return _DoAntiCheatCheck(checkType);
            }

        void DoAntiCheatAction(AntiCheatCheck checkType, std::string reason);

        // Check functions
        bool CheckNull();

        // movement
        bool CheckMovement();
        bool CheckTp2Plane();
        bool CheckFly();
        bool CheckAirJump();
        bool CheckMountain();
        bool CheckWaterWalking();
        bool CheckSpeed();
        bool CheckTeleport();
        bool CheckFall();

        // Transport
        bool CheckOnTransport();

        // quest
        bool CheckQuest();

        // spell
        bool CheckSpell();
        bool CheckSpellValid();
        bool CheckSpellOndeath();
        bool CheckSpellFamily();
        bool CheckSpellInbook();

        // damage
        bool CheckDamage();
        bool CheckSpellDamage();
        bool CheckMeleeDamage();

    private:

        // Internal fuctions
        bool                       CheckTimer(AntiCheatCheck checkType);
        bool                       _DoAntiCheatCheck(AntiCheatCheck checktype);
        AntiCheatCheckEntry*       _FindCheck(AntiCheatCheck checktype);
        AntiCheatConfig const*     _FindConfig(AntiCheatCheck checktype);
        Player*                    GetPlayer() { return m_player;};
        Unit*                      GetMover()  { return m_currentMover;};

        // Saved variables
        float                                   m_MovedLen;          //Length of traveled way
        uint32                                  m_immuneTime;
        bool                                    m_isFall;
        bool                                    m_isTeleported;
        bool                                    m_isActiveMover;
        uint32                                  m_lastfalltime;
        uint32                                  m_lastClientTime;
        DeathState                              m_lastLiveState;
        float                                   m_lastfallz;
        Player*                                 m_player;
        std::map<AntiCheatCheck, uint32>        m_counters;               // counter of alarms
        std::map<AntiCheatCheck, uint32>        m_oldCheckTime;           // last time when check processed
        std::map<AntiCheatCheck, uint32>        m_lastalarmtime;          // last time when alarm generated
        std::map<AntiCheatCheck, uint32>        m_lastactiontime;         // last time when action is called

        // Variables for current check
        Unit*                      m_currentMover;
        MovementInfo*              m_currentmovementInfo;
        uint32                     m_currentDamage;
        uint32                     m_currentspellID;
        uint32                     m_currentOpcode;
        uint32                     m_currentTimeSkipped;
        std::string                m_currentCheckResult;
        AntiCheatConfig const*     m_currentConfig;
        float                      m_currentDelta;
        float                      m_current_angle;

};

struct AntiCheatCheckEntry
{
    public:
        bool           active;
        AntiCheatCheck checkType;
        bool           (AntiCheat::*Handler)();
};
