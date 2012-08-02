/*
 * Copyright (C) 2010-2012 Strawberry-Pr0jcts <http://strawberry-pr0jcts.com/>
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

#ifndef __BATTLEFIELD_H
#define __BATTLEFIELD_H

#include "BattlefieldMgr.h"
#include "MapManager.h"
#include "Group.h"
#include "GameObject.h"

#include <list>

enum Battlefields
{
    BATTLEFIELD_WG = 1,
    BATTLEFIELD_TB = 21
};

#define MAX_TEAM 2

typedef std::list<Player *> PlayerList;

class Map;

class Battlefield
{
    friend class BattlefieldMgr;

    public:

        Battlefield(uint8 BattleId);
        ~Battlefield();
        
        void Update(uint32 uiDiff);
        void BattleStart();
        void BattleEnd();

        void PlayerJoin(Player* player);
        void PlayerLeave(Player* player);
        bool IsBattleInProgress() const { return m_battleInProgress; }
        bool AddPlayerToGroup(Player * player);

        uint8  GetBattleId() { return m_battleId; }
        uint8  GetControllerTeam() { return m_controlledByTeam; }
        uint32 GetZoneId() { return m_zoneId; }
        uint32 GetTimeToNextBattle() { return m_nextBattleTimer; }
        uint32 GetTimeTIllEndOfWar() { return m_battleDurationTimer; }

    protected:
        uint8           m_defenderTeam;
        uint8           m_attackerTeam;
        uint8           m_controlledByTeam;
        uint8           m_battleId;
        uint32          m_nextBattleTimer;
        uint32          m_battleDurationTimer;
        uint32          m_preBattleTimer;
        uint32          m_zoneId;
        bool            m_battleInProgress;
        bool            m_invitationSent;
        Group*          m_raidGroup[MAX_TEAM];
        Map*            m_map;

    protected:
        // Battlefield API

        //Called before battle has started
        virtual void BeforeBattleStarted() = 0;

        //Called after Battle has ended
        virtual void AfterBattleEnded() = 0;

        //Called on world tick
        virtual void OnUpdate(uint32 uiDiff) = 0;

        //Called when a player enters WG
        virtual void OnPlayerEnter(Player* player) = 0;

        //Called when a player leaves WG
        virtual void OnPlayerExit(Player* player) = 0;

};

#endif