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
#ifndef __BATTLEGROUNDNA_H
#define __BATTLEGROUNDNA_H

class BattleGround;

class BattleGroundNAScore : public BattleGroundScore
{
    public:
        BattleGroundNAScore() {};
        virtual ~BattleGroundNAScore() {};
        // TODO fix me
};

class BattleGroundNA : public BattleGround
{
        friend class BattleGroundMgr;

    public:
        BattleGroundNA();
        ~BattleGroundNA();
        void Update(uint32 diff) override;

        /* inherited from BattlegroundClass */
        virtual void AddPlayer(Player* plr) override;
        virtual void StartingEventCloseDoors() override;
        virtual void StartingEventOpenDoors() override;

        void RemovePlayer(Player* plr, ObjectGuid guid) override;
        void HandleAreaTrigger(Player* source, uint32 trigger) override;
        bool SetupBattleGround() override;
        virtual void Reset() override;
        virtual void FillInitialWorldStates(WorldPacket& d, uint32& count) override;
        void HandleKillPlayer(Player* player, Player* killer) override;
        bool HandlePlayerUnderMap(Player* plr) override;
};
#endif
