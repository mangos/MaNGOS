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

#include "OutdoorPvPZM.h"
#include "WorldPacket.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Object.h"
#include "Creature.h"
#include "GameObject.h"
#include "Player.h"

OutdoorPvPZM::OutdoorPvPZM() : OutdoorPvP(),
    m_graveyardOwner(TEAM_NONE),
    m_graveyardWorldState(WORLD_STATE_ZM_GRAVEYARD_NEUTRAL),
    m_scoutWorldStateAlliance(WORLD_STATE_ZM_FLAG_NOT_READY_ALLIANCE),
    m_scoutWorldStateHorde(WORLD_STATE_ZM_FLAG_NOT_READY_HORDE),
    m_towersAlliance(0),
    m_towersHorde(0)
{
    // init world states
    m_towerWorldState[0] = WORLD_STATE_ZM_BEACON_EAST_UI_NEUTRAL;
    m_towerWorldState[1] = WORLD_STATE_ZM_BEACON_WEST_UI_NEUTRAL;
    m_towerMapState[0] = WORLD_STATE_ZM_BEACON_EAST_NEUTRAL;
    m_towerMapState[1] = WORLD_STATE_ZM_BEACON_WEST_NEUTRAL;

    for (uint8 i = 0; i < MAX_ZM_TOWERS; ++i)
        m_towerOwner[i] = TEAM_NONE;

    // initially set graveyard owner to neither faction
    sObjectMgr.SetGraveYardLinkTeam(GRAVEYARD_ID_TWIN_SPIRE, GRAVEYARD_ZONE_TWIN_SPIRE, TEAM_INVALID);
}

void OutdoorPvPZM::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
    FillInitialWorldState(data, count, m_scoutWorldStateAlliance, WORLD_STATE_ADD);
    FillInitialWorldState(data, count, m_scoutWorldStateHorde, WORLD_STATE_ADD);
    FillInitialWorldState(data, count, m_graveyardWorldState, WORLD_STATE_ADD);

    for (uint8 i = 0; i < MAX_ZM_TOWERS; ++i)
    {
        FillInitialWorldState(data, count, m_towerWorldState[i], WORLD_STATE_ADD);
        FillInitialWorldState(data, count, m_towerMapState[i], WORLD_STATE_ADD);
    }
}

void OutdoorPvPZM::SendRemoveWorldStates(Player* player)
{
    player->SendUpdateWorldState(m_scoutWorldStateAlliance, WORLD_STATE_REMOVE);
    player->SendUpdateWorldState(m_scoutWorldStateHorde, WORLD_STATE_REMOVE);
    player->SendUpdateWorldState(m_graveyardWorldState, WORLD_STATE_REMOVE);

    for (uint8 i = 0; i < MAX_ZM_TOWERS; ++i)
    {
        player->SendUpdateWorldState(m_towerWorldState[i], WORLD_STATE_REMOVE);
        player->SendUpdateWorldState(m_towerMapState[i], WORLD_STATE_REMOVE);
    }
}

void OutdoorPvPZM::HandlePlayerEnterZone(Player* player, bool isMainZone)
{
    OutdoorPvP::HandlePlayerEnterZone(player, isMainZone);

    // remove the buff from the player first; Sometimes on relog players still have the aura
    player->RemoveAurasDueToSpell(SPELL_TWIN_SPIRE_BLESSING);

    // cast buff the the player which enters the zone
    if (player->GetTeam() == m_graveyardOwner)
        player->CastSpell(player, SPELL_TWIN_SPIRE_BLESSING, true);
}

void OutdoorPvPZM::HandlePlayerLeaveZone(Player* player, bool isMainZone)
{
    // remove the buff from the player
    player->RemoveAurasDueToSpell(SPELL_TWIN_SPIRE_BLESSING);

    OutdoorPvP::HandlePlayerLeaveZone(player, isMainZone);
}

void OutdoorPvPZM::HandleCreatureCreate(Creature* creature)
{
    switch (creature->GetEntry())
    {
        case NPC_PVP_BEAM_RED:
            if (creature->GetPositionY() < 7000.0f)         // East Beam
                m_beamTowerRed[0] = creature->GetObjectGuid();
            else if (creature->GetPositionY() < 7300.0f)    // Center Beam
                m_beamGraveyardRed = creature->GetObjectGuid();
            else                                            // West Beam
                m_beamTowerRed[1] = creature->GetObjectGuid();
            break;
        case NPC_PVP_BEAM_BLUE:
            if (creature->GetPositionY() < 7000.0f)         // East Beam
                m_beamTowerBlue[0] = creature->GetObjectGuid();
            else if (creature->GetPositionY() < 7300.0f)    // Center Beam
                m_beamGraveyardBlue = creature->GetObjectGuid();
            else                                            // West Beam
                m_beamTowerBlue[1] = creature->GetObjectGuid();
            break;
    }
}

void OutdoorPvPZM::HandleGameObjectCreate(GameObject* go)
{
    OutdoorPvP::HandleGameObjectCreate(go);

    switch (go->GetEntry())
    {
        case GO_ZANGA_BANNER_EAST:
            m_towerBanners[0] = go->GetObjectGuid();
            break;
        case GO_ZANGA_BANNER_WEST:
            m_towerBanners[1] = go->GetObjectGuid();
            break;
        case GO_ZANGA_BANNER_CENTER_ALLIANCE:
            m_graveyardBannerAlliance = go->GetObjectGuid();
            break;
        case GO_ZANGA_BANNER_CENTER_HORDE:
            m_graveyardBannerHorde = go->GetObjectGuid();
            break;
        case GO_ZANGA_BANNER_CENTER_NEUTRAL:
            m_graveyardBannerNeutral = go->GetObjectGuid();
            break;
    }
}

// Cast player spell on opponent kill
void OutdoorPvPZM::HandlePlayerKillInsideArea(Player* player)
{
    for (uint8 i = 0; i < MAX_ZM_TOWERS; ++i)
    {
        if (GameObject* capturePoint = player->GetMap()->GetGameObject(m_towerBanners[i]))
        {
            // check capture point range
            GameObjectInfo const* info = capturePoint->GetGOInfo();
            if (info && player->IsWithinDistInMap(capturePoint, info->capturePoint.radius))
            {
                // check capture point team
                if (player->GetTeam() == m_towerOwner[i])
                    player->CastSpell(player, player->GetTeam() == ALLIANCE ? SPELL_ZANGA_TOWER_TOKEN_ALLIANCE : SPELL_ZANGA_TOWER_TOKEN_HORDE, true);

                return;
            }
        }
    }
}

// process the capture events
bool OutdoorPvPZM::HandleEvent(uint32 eventId, GameObject* go)
{
    for (uint8 i = 0; i < MAX_ZM_TOWERS; ++i)
    {
        if (zangarmarshTowers[i] == go->GetEntry())
        {
            for (uint8 j = 0; j < 4; ++j)
            {
                if (zangarmarshTowerEvents[i][j].eventEntry == eventId)
                {
                    // prevent processing if the owner did not change (happens if progress event is called after contest event)
                    if (zangarmarshTowerEvents[i][j].team != m_towerOwner[i])
                    {
                        if (zangarmarshTowerEvents[i][j].defenseMessage)
                            sWorld.SendDefenseMessage(ZONE_ID_ZANGARMARSH, zangarmarshTowerEvents[i][j].defenseMessage);

                        return ProcessCaptureEvent(go, i, zangarmarshTowerEvents[i][j].team, zangarmarshTowerEvents[i][j].worldState, zangarmarshTowerEvents[i][j].mapState);
                    }
                    // no need to iterate other events or towers
                    return false;
                }
            }
            // no need to iterate other towers
            return false;
        }
    }

    return false;
}

bool OutdoorPvPZM::ProcessCaptureEvent(GameObject* go, uint32 towerId, Team team, uint32 newWorldState, uint32 newMapState)
{
    if (team == ALLIANCE)
    {
        // update counter
        SetBeaconArtKit(go, m_beamTowerBlue[towerId], SPELL_BEAM_BLUE);
        ++m_towersAlliance;

        if (m_towersAlliance == MAX_ZM_TOWERS)
        {
            // Send this defense message before updating scout state as this sends another
            sWorld.SendDefenseMessage(ZONE_ID_ZANGARMARSH, LANG_OPVP_ZM_CAPTURE_BOTH_BEACONS_A);

            // only add flag to scouts if team does not have captured graveyard already
            if (m_graveyardOwner != ALLIANCE)
                UpdateScoutState(ALLIANCE, true);
        }
    }
    else if (team == HORDE)
    {
        // update counter
        SetBeaconArtKit(go, m_beamTowerRed[towerId], SPELL_BEAM_RED);
        ++m_towersHorde;

        if (m_towersHorde == MAX_ZM_TOWERS)
        {
            // Send this defense message before updating scout state as this sends another
            sWorld.SendDefenseMessage(ZONE_ID_ZANGARMARSH, LANG_OPVP_ZM_CAPTURE_BOTH_BEACONS_H);

            // only add flag to scouts if team does not already have captured graveyard
            if (m_graveyardOwner != HORDE)
                UpdateScoutState(HORDE, true);
        }
    }
    else
    {
        if (m_towerOwner[towerId] == ALLIANCE)
        {
            SetBeaconArtKit(go, m_beamTowerBlue[towerId], 0);

            // only remove flag from scouts if team does not already have captured graveyard
            if (m_towersAlliance == MAX_ZM_TOWERS && m_graveyardOwner != ALLIANCE)
                UpdateScoutState(ALLIANCE, false);

            // update counter
            --m_towersAlliance;
        }
        else
        {
            SetBeaconArtKit(go, m_beamTowerRed[towerId], 0);

            // only remove flag from scouts if team does not already have captured graveyard
            if (m_towersHorde == MAX_ZM_TOWERS && m_graveyardOwner != HORDE)
                UpdateScoutState(HORDE, false);

            // update counter
            --m_towersHorde;
        }
    }

    // update tower state
    SendUpdateWorldState(m_towerWorldState[towerId], WORLD_STATE_REMOVE);
    m_towerWorldState[towerId] = newWorldState;
    SendUpdateWorldState(m_towerWorldState[towerId], WORLD_STATE_ADD);

    SendUpdateWorldState(m_towerMapState[towerId], WORLD_STATE_REMOVE);
    m_towerMapState[towerId] = newMapState;
    SendUpdateWorldState(m_towerMapState[towerId], WORLD_STATE_ADD);;

    // update capture point owner
    m_towerOwner[towerId] = team;

    // the are no DB exceptions in this case
    return true;
}

// Handle scout activation, when both beacons are captured
void OutdoorPvPZM::UpdateScoutState(Team team, bool spawned)
{
    if (team == ALLIANCE)
    {
        SendUpdateWorldState(m_scoutWorldStateAlliance, WORLD_STATE_REMOVE);
        m_scoutWorldStateAlliance = spawned ? WORLD_STATE_ZM_FLAG_READY_ALLIANCE : WORLD_STATE_ZM_FLAG_NOT_READY_ALLIANCE;
        SendUpdateWorldState(m_scoutWorldStateAlliance, WORLD_STATE_ADD);

        if (spawned)
            sWorld.SendDefenseMessage(ZONE_ID_ZANGARMARSH, LANG_OPVP_ZM_SPAWN_FIELD_SCOUT_A);
    }
    else
    {
        SendUpdateWorldState(m_scoutWorldStateHorde, WORLD_STATE_REMOVE);
        m_scoutWorldStateHorde = spawned ? WORLD_STATE_ZM_FLAG_READY_HORDE : WORLD_STATE_ZM_FLAG_NOT_READY_HORDE;
        SendUpdateWorldState(m_scoutWorldStateHorde, WORLD_STATE_ADD);

        if (spawned)
            sWorld.SendDefenseMessage(ZONE_ID_ZANGARMARSH, LANG_OPVP_ZM_SPAWN_FIELD_SCOUT_H);
    }
}

// Handle the graveyard banner use
bool OutdoorPvPZM::HandleGameObjectUse(Player* player, GameObject* go)
{
    Team team = player->GetTeam();

    switch (go->GetEntry())
    {
        case GO_ZANGA_BANNER_CENTER_NEUTRAL:
            break;
        case GO_ZANGA_BANNER_CENTER_ALLIANCE:
            if (team == ALLIANCE || !player->HasAura(SPELL_BATTLE_STANDARD_HORDE))
                return false;
            break;
        case GO_ZANGA_BANNER_CENTER_HORDE:
            if (team == HORDE || !player->HasAura(SPELL_BATTLE_STANDARD_ALLIANCE))
                return false;
            break;
        default:
            return false;
    }

    // disable old banners - note the alliance and horde banners can despawn by self
    if (m_graveyardOwner == ALLIANCE)
    {
        //RespawnGO(go, m_graveyardBannerAlliance, false);
        SetBeaconArtKit(go, m_beamGraveyardBlue, 0);
    }
    else if (m_graveyardOwner == HORDE)
    {
        //RespawnGO(go, m_graveyardBannerHorde, false);
        SetBeaconArtKit(go, m_beamGraveyardRed, 0);
    }
    else
        RespawnGO(go, m_graveyardBannerNeutral, false);

    if (team == ALLIANCE)
    {
        // change banners
        RespawnGO(go, m_graveyardBannerAlliance, true);
        SetBeaconArtKit(go, m_beamGraveyardBlue, SPELL_BEAM_BLUE);

        // update world state
        SendUpdateWorldState(m_graveyardWorldState, WORLD_STATE_REMOVE);
        m_graveyardWorldState = WORLD_STATE_ZM_GRAVEYARD_ALLIANCE;
        SendUpdateWorldState(m_graveyardWorldState, WORLD_STATE_ADD);

        // remove player flag aura
        player->RemoveAurasDueToSpell(SPELL_BATTLE_STANDARD_ALLIANCE);

        // send defense message
        sWorld.SendDefenseMessage(ZONE_ID_ZANGARMARSH, LANG_OPVP_ZM_CAPTURE_GRAVEYARD_A);
    }
    else
    {
        // change banners
        RespawnGO(go, m_graveyardBannerHorde, true);
        SetBeaconArtKit(go, m_beamGraveyardRed, SPELL_BEAM_RED);

        // update world state
        SendUpdateWorldState(m_graveyardWorldState, WORLD_STATE_REMOVE);
        m_graveyardWorldState = WORLD_STATE_ZM_GRAVEYARD_HORDE;
        SendUpdateWorldState(m_graveyardWorldState, WORLD_STATE_ADD);

        // remove player flag aura
        player->RemoveAurasDueToSpell(SPELL_BATTLE_STANDARD_HORDE);

        // send defense message
        sWorld.SendDefenseMessage(ZONE_ID_ZANGARMARSH, LANG_OPVP_ZM_CAPTURE_GRAVEYARD_H);
    }

    // change the graveyard link
    sObjectMgr.SetGraveYardLinkTeam(GRAVEYARD_ID_TWIN_SPIRE, GRAVEYARD_ZONE_TWIN_SPIRE, team);

    // apply zone buff
    if (m_graveyardOwner != TEAM_NONE)
        BuffTeam(m_graveyardOwner, SPELL_TWIN_SPIRE_BLESSING, true);
    BuffTeam(team, SPELL_TWIN_SPIRE_BLESSING);

    // reset scout so that team cannot take flag
    UpdateScoutState(team, false);

    // update graveyard owner
    m_graveyardOwner = team;

    return false;
}

// ToDo: Handle the case when the player drops the flag
//bool OutdoorPvPZM::HandleDropFlag(Player* player, uint32 spellId)
//{
//    if (spellId == SPELL_BATTLE_STANDARD_HORDE || spellId == SPELL_BATTLE_STANDARD_ALLIANCE)
//    {
//        // ToDo: implement this when the scout DB conditions are implemented
//        // The scouts gossip options should check a DB condition if the gossip is pvp available
//        // The idea is to set the Outdoor PvP condition to false on flag take - this will allow only one player to use the flag
//        // on flag drop the condition can be set back to true if necessary, so the players can retake the flag
//        return true;
//    }
//
//    return false;
//}

// Handle the ZM beacons - this is done by npcs which have certain auras
void OutdoorPvPZM::SetBeaconArtKit(const WorldObject* objRef, ObjectGuid creatureGuid, uint32 auraId)
{
    if (Creature* beam = objRef->GetMap()->GetCreature(creatureGuid))
    {
        if (auraId)
            beam->CastSpell(beam, auraId, true);
        else
            beam->RemoveAllAuras();
    }
}
