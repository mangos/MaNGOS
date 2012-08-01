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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __BATTLEFIELDMGR_H
#define __BATTLEFIELDMGR_H

#include "Battlefield.h"
#include "Player.h"
#include "Common.h"
#include "SharedDefines.h"
#include "WorldSession.h"
#include "Policies/Singleton.h"
#include "ace/Recursive_Thread_Mutex.h"

class Player;
class Battlefield;

typedef std::set<Player*> PlayerQueue;
typedef std::map<uint8,Battlefield* > BattlefieldMap;

class BattlefieldQueue
{
    public:
        friend class BattlefieldMgr;

        BattlefieldQueue(uint8 battleId) { m_queueId = battleId; }
        uint64 GetId() { return m_queueId; }
        bool HasEnoughSpace() { return m_inQueue.size() <= 240 ? true : false ; }
        bool HasPlayerInQueue(Player * plr) { return m_inQueue.find(plr) != m_inQueue.end(); }
        void AddPlayerToQueue(Player * plr) { m_inQueue.insert(plr); }
        void RemovePlayerFromQueue(Player * plr) { m_inQueue.erase(plr); }

    private:
        PlayerQueue     m_inQueue;
        uint64          m_queueId;
};

typedef std::map<uint8,BattlefieldQueue* > BattlefieldQueueMap;

class BattlefieldMgr
{
    friend class Battlefield;

    public:
        BattlefieldMgr();
        ~BattlefieldMgr();

        void Initialize();
        void Update(uint32 uiDiff);

        Battlefield * FindBattlefield(uint8 battleId);
        BattlefieldQueue * GetQueueForBattlefield(uint8 battleId) { return m_queueMap[battleId]; }

        void SendInvitePlayerToQueue(Player * player);
        void SendInvitePlayersToWar(uint8 battleId);
        void SendInvitePlayersInZone(uint8 battleId);
        void ChangeState(Battlefield * battlefield);
        void UpdateWorldState(uint32 stateId, uint32 value);

        void PlayerEnterZone(Player * player, uint32 zoneId);
        void PlayerLeftZone(Player * player, uint32 zoneId);

        void RemovePlayerFromBattlefield(Player * player, Battlefield * battlefield, BFLeaveReason reason, bool relocated);
    private:
        BattlefieldQueueMap     m_queueMap;
        BattlefieldMap          m_battlefieldList;

};

#define sBattlefieldMgr MaNGOS::Singleton<BattlefieldMgr>::Instance()
#endif
