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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef WORLD_PVP_SI
#define WORLD_PVP_SI

#include "Common.h"
#include "OutdoorPvP.h"
#include "Language.h"

enum
{
    // npcs
    NPC_SILITHUS_DUST_QUEST_ALLIANCE    = 17090,        // dummy npcs for quest credit
    NPC_SILITHUS_DUST_QUEST_HORDE       = 18199,

    // game objects
    GO_SILITHYST_MOUND                  = 181597,       // created when a player drops the flag
    GO_SILITHYST_GEYSER                 = 181598,       // spawn on the map by default

    // spells
    //SPELL_SILITHYST_OBJECT            = 29518,        // unknown, related to the GO
    SPELL_SILITHYST                     = 29519,        // buff received when you are carrying a silithyst
    SPELL_TRACES_OF_SILITHYST           = 29534,        // individual buff received when successfully delivered a silithyst
    SPELL_CENARION_FAVOR                = 30754,        // zone buff received when a team gathers 200 silithyst
    SPELL_SILITHYST_FLAG_DROP           = 29533,        // drop the flag

    // quests
    QUEST_SCOURING_DESERT_ALLIANCE      = 9419,
    QUEST_SCOURING_DESERT_HORDE         = 9422,

    // area triggers
    AREATRIGGER_SILITHUS_ALLIANCE       = 4162,
    AREATRIGGER_SILITHUS_HORDE          = 4168,

    // misc
    FACTION_CENARION_CIRCLE             = 609,
    HONOR_REWARD_SILITHYST              = 19,
    REPUTATION_REWARD_SILITHYST         = 20,
    MAX_SILITHYST                       = 200,

    // world states
    WORLD_STATE_SI_GATHERED_A           = 2313,
    WORLD_STATE_SI_GATHERED_H           = 2314,
    WORLD_STATE_SI_SILITHYST_MAX        = 2317
};

class OutdoorPvPSI : public OutdoorPvP
{
    public:
        OutdoorPvPSI();

        void HandlePlayerEnterZone(Player* player, bool isMainZone) override;
        void HandlePlayerLeaveZone(Player* player, bool isMainZone) override;
        void FillInitialWorldStates(WorldPacket& data, uint32& count) override;

        bool HandleAreaTrigger(Player* player, uint32 triggerId) override;
        bool HandleGameObjectUse(Player* player, GameObject* go) override;
        bool HandleDropFlag(Player* player, uint32 spellId) override;

    private:
        uint8 m_resourcesAlliance;
        uint8 m_resourcesHorde;
        Team m_zoneOwner;
};

#endif
