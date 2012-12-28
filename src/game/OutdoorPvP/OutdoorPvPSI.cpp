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

#include "OutdoorPvPSI.h"
#include "WorldPacket.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Object.h"
#include "Creature.h"
#include "GameObject.h"
#include "Player.h"

OutdoorPvPSI::OutdoorPvPSI() : OutdoorPvP(),
    m_resourcesAlliance(0),
    m_resourcesHorde(0),
    m_zoneOwner(TEAM_NONE)
{
}

// Send initial world states
void OutdoorPvPSI::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
    FillInitialWorldState(data, count, WORLD_STATE_SI_GATHERED_A, m_resourcesAlliance);
    FillInitialWorldState(data, count, WORLD_STATE_SI_GATHERED_H, m_resourcesHorde);
    FillInitialWorldState(data, count, WORLD_STATE_SI_SILITHYST_MAX, MAX_SILITHYST);
}

// Handle buffs when player enters the zone
void OutdoorPvPSI::HandlePlayerEnterZone(Player* player, bool isMainZone)
{
    OutdoorPvP::HandlePlayerEnterZone(player, isMainZone);

    // remove the buff from the player first; Sometimes on relog players still have the aura
    player->RemoveAurasDueToSpell(SPELL_CENARION_FAVOR);

    // buff the player if same team is controlling the zone
    if (player->GetTeam() == m_zoneOwner)
        player->CastSpell(player, SPELL_CENARION_FAVOR, true);
}

// Remove buffs when player leaves zone
void OutdoorPvPSI::HandlePlayerLeaveZone(Player* player, bool isMainZone)
{
    // remove the buff from the player
    player->RemoveAurasDueToSpell(SPELL_CENARION_FAVOR);

    OutdoorPvP::HandlePlayerLeaveZone(player, isMainZone);
}

// Handle case when player returns a silithyst
bool OutdoorPvPSI::HandleAreaTrigger(Player* player, uint32 triggerId)
{
    if (player->isGameMaster() || player->isDead())
        return false;

    switch (triggerId)
    {
        case AREATRIGGER_SILITHUS_ALLIANCE:
            if (player->GetTeam() != ALLIANCE || !player->HasAura(SPELL_SILITHYST))
                return false;

            // update counter
            ++ m_resourcesAlliance;
            SendUpdateWorldState(WORLD_STATE_SI_GATHERED_A, m_resourcesAlliance);

            // handle the case when the faction has reached maximum resources allowed
            if (m_resourcesAlliance == MAX_SILITHYST)
            {
                // NOTE: On retail it would not reset until server restart but we do not support weekly restart :)
                m_zoneOwner = ALLIANCE;
                m_resourcesAlliance = 0;
                m_resourcesHorde = 0;

                // also update the horde counter if resources were reset
                SendUpdateWorldState(WORLD_STATE_SI_GATHERED_H, m_resourcesHorde);

                // apply buff to owner team
                BuffTeam(ALLIANCE, SPELL_CENARION_FAVOR);

                // Send defense message
                sWorld.SendDefenseMessage(ZONE_ID_SILITHUS, LANG_OPVP_SI_CAPTURE_A);
            }

            // give quest credit if necessary
            if (player->GetQuestStatus(QUEST_SCOURING_DESERT_ALLIANCE) == QUEST_STATUS_INCOMPLETE)
                player->KilledMonsterCredit(NPC_SILITHUS_DUST_QUEST_ALLIANCE);
            break;
        case AREATRIGGER_SILITHUS_HORDE:
            if (player->GetTeam() != HORDE || !player->HasAura(SPELL_SILITHYST))
                return false;

            // update counter
            ++ m_resourcesHorde;
            SendUpdateWorldState(WORLD_STATE_SI_GATHERED_H, m_resourcesHorde);

            // handle the case when the faction has reached maximum resources allowed
            if (m_resourcesHorde == MAX_SILITHYST)
            {
                // NOTE: On retail it would not reset until server restart but we do not support weekly restart :)
                m_zoneOwner = HORDE;
                m_resourcesAlliance = 0;
                m_resourcesHorde = 0;

                // also update the alliance counter if resources were reset
                SendUpdateWorldState(WORLD_STATE_SI_GATHERED_A, m_resourcesAlliance);

                // apply buff to owner team
                BuffTeam(HORDE, SPELL_CENARION_FAVOR);

                // Send defense message
                sWorld.SendDefenseMessage(ZONE_ID_SILITHUS, LANG_OPVP_SI_CAPTURE_H);
            }

            // give quest credit if necessary
            if (player->GetQuestStatus(QUEST_SCOURING_DESERT_HORDE) == QUEST_STATUS_INCOMPLETE)
                player->KilledMonsterCredit(NPC_SILITHUS_DUST_QUEST_HORDE);
            break;
        default:
            return false;
    }

    // remove silithyst aura
    player->RemoveAurasDueToSpell(SPELL_SILITHYST);

    // reward the player
    player->CastSpell(player, SPELL_TRACES_OF_SILITHYST, true);
    player->RewardHonor(NULL, 1, HONOR_REWARD_SILITHYST);
    player->GetReputationMgr().ModifyReputation(sFactionStore.LookupEntry(FACTION_CENARION_CIRCLE), REPUTATION_REWARD_SILITHYST);

    return true;
}

// Handle case when player drops flag
// TODO - fix this workaround!
struct SilithusSpawnLocation
{
    float x, y, z;
};
// Area trigger location - workaround to check the flag drop handling
static SilithusSpawnLocation silithusFlagDropLocations[2] =
{
    { -7142.04f, 1397.92f, 4.327f},     // alliance
    { -7588.48f, 756.806f, -16.425f}    // horde
};

bool OutdoorPvPSI::HandleDropFlag(Player* player, uint32 spellId)
{
    if (spellId != SPELL_SILITHYST)
        return false;

    // don't drop flag at area trigger
    // we are checking distance from the AT hard-coded coordinates because it's much faster than checking the area trigger store
    switch (player->GetTeam())
    {
        case ALLIANCE:
            if (player->IsWithinDist3d(silithusFlagDropLocations[0].x, silithusFlagDropLocations[0].y, silithusFlagDropLocations[0].z, 5.0f))
                return false;
            break;
        case HORDE:
            if (player->IsWithinDist3d(silithusFlagDropLocations[1].x, silithusFlagDropLocations[1].y, silithusFlagDropLocations[1].z, 5.0f))
                return false;
            break;
        default:
            break;
    }

    // drop the flag in other case
    player->CastSpell(player, SPELL_SILITHYST_FLAG_DROP, true);
    return true;
}

// Handle the case when player picks a silithyst mound or geyser
// This needs to be done because the spells used by these objects are missing
bool OutdoorPvPSI::HandleGameObjectUse(Player* player, GameObject* go)
{
    if (go->GetEntry() == GO_SILITHYST_MOUND || go->GetEntry() == GO_SILITHYST_GEYSER)
    {
        // Also mark player with pvp on
        player->CastSpell(player, SPELL_SILITHYST, true);
        player->UpdatePvP(true, true);
        player->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP);
        // Despawn the gameobject (workaround)
        go->SetLootState(GO_JUST_DEACTIVATED);
        return true;
    }

    return false;
}
