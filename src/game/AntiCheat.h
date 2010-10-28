/*
 * Copyright (C) 2010 /dev/rsa for MaNGOS <http://getmangos.com/>
 * based on Xeross code
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
struct MovementInfo;

struct AntiCheat
{

        explicit AntiCheat(Player* player);
        ~AntiCheat();

    public:
        bool CheckNeeded(Player* plMover);
        bool CheckMovement(Player* plMover, MovementInfo& movementInfo, uint32 opcode =0);
        bool CheckOnTransport(MovementInfo& movementInfo);

        uint32     Anti__GetLastTeleTime() const { return m_anti_TeleTime; }
        void       Anti__SetLastTeleTime(uint32 TeleTime) { m_anti_TeleTime = TeleTime; }
        bool       CanFly() const { return m_CanFly;  }
        void       SetCanFly(bool CanFly) { m_CanFly = CanFly; }
//
        uint32     m_anti_lastmovetime;     //last movement time
        float      m_anti_MovedLen;         //Length of traveled way
        uint32     m_anti_NextLenCheck;
        float      m_anti_BeginFallZ;    //alternative falling begin
        uint32     m_anti_lastalarmtime;    //last time when alarm generated
        uint32     m_anti_alarmcount;       //alarm counter
        uint32     m_anti_TeleTime;
        bool       m_CanFly;
    private:
        Player*    GetPlayer() { return m_player;};
        Player*    m_player;
};
