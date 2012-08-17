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

#ifndef WORLD_PVP_GH
#define WORLD_PVP_GH

#include "Common.h"
#include "OutdoorPvP.h"

enum
{
    // npcs
    // alliance
    NPC_WESTFALL_BRIGADE_DEFENDER           = 27758,
    NPC_COMMANDER_HOWSER                    = 27759,
    NPC_BLACKSMITH_JASON_RIGGINS            = 29252,
    NPC_VENDOR_ADAMS                        = 27760,
    NPC_STABLE_MASTER_TIM                   = 29250,
    NPC_HORSE                               = 385,

    // horde
    NPC_CONQUEST_HOLD_DEFENDER              = 27748,
    NPC_GENERAL_GORLOK                      = 27708,
    NPC_BLACKSMITH_KOLOTH                   = 29253,
    NPC_VENDOR_PURKOM                       = 27730,
    NPC_STABLE_MASTER_KOR                   = 29251,
    NPC_RIDING_WOLF                         = 5774,

    // gameobjects
    GO_VENTURE_BAY_LIGHTHOUSE               = 189310,

    // events
    EVENT_LIGHTHOUSE_WIN_ALLIANCE           = 18035,
    EVENT_LIGHTHOUSE_WIN_HORDE              = 18036,
    //EVENT_LIGHTHOUSE_CONTEST_ALLIANCE     = 18037,
    //EVENT_LIGHTHOUSE_CONTEST_HORDE        = 18038,
    EVENT_LIGHTHOUSE_PROGRESS_ALLIANCE      = 18039,
    EVENT_LIGHTHOUSE_PROGRESS_HORDE         = 18040,
    EVENT_LIGHTHOUSE_NEUTRAL_ALLIANCE       = 18041,
    EVENT_LIGHTHOUSE_NEUTRAL_HORDE          = 18042,
};

class OutdoorPvPGH : public OutdoorPvP
{
    public:
        OutdoorPvPGH();

        bool HandleEvent(uint32 eventId, GameObject* go) override;

        void HandleCreatureCreate(Creature* creature) override;
        void HandleCreatureDeath(Creature* creature) override;
        void HandleGameObjectCreate(GameObject* go) override;

    private:
        // despawn team vendors, if not killed already
        void DespawnVendors(const WorldObject* objRef);

        void LockLighthouse(const WorldObject* objRef);
        void UnlockLighthouse(const WorldObject* objRef);

        Team m_zoneOwner;

        GuidList m_teamVendors;

        ObjectGuid m_capturePoint;
};

#endif
