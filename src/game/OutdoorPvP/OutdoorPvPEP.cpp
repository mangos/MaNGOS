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

#include "OutdoorPvPEP.h"
#include "WorldPacket.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Object.h"
#include "Creature.h"
#include "GameObject.h"
#include "Player.h"

OutdoorPvPEP::OutdoorPvPEP() : OutdoorPvP(),
    m_towersAlliance(0),
    m_towersHorde(0)
{
    m_towerWorldState[0] = WORLD_STATE_EP_NORTHPASS_NEUTRAL;
    m_towerWorldState[1] = WORLD_STATE_EP_CROWNGUARD_NEUTRAL;
    m_towerWorldState[2] = WORLD_STATE_EP_EASTWALL_NEUTRAL;
    m_towerWorldState[3] = WORLD_STATE_EP_PLAGUEWOOD_NEUTRAL;

    for (uint8 i = 0; i < MAX_EP_TOWERS; ++i)
        m_towerOwner[i] = TEAM_NONE;

    // initially set graveyard owner to neither faction
    sObjectMgr.SetGraveYardLinkTeam(GRAVEYARD_ID_EASTERN_PLAGUE, GRAVEYARD_ZONE_EASTERN_PLAGUE, TEAM_INVALID);
}

void OutdoorPvPEP::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
    FillInitialWorldState(data, count, WORLD_STATE_EP_TOWER_COUNT_ALLIANCE, m_towersAlliance);
    FillInitialWorldState(data, count, WORLD_STATE_EP_TOWER_COUNT_HORDE, m_towersHorde);

    for (uint8 i = 0; i < MAX_EP_TOWERS; ++i)
        FillInitialWorldState(data, count, m_towerWorldState[i], WORLD_STATE_ADD);
}

void OutdoorPvPEP::SendRemoveWorldStates(Player* player)
{
    for (uint8 i = 0; i < MAX_EP_TOWERS; ++i)
        player->SendUpdateWorldState(m_towerWorldState[i], WORLD_STATE_REMOVE);
}

void OutdoorPvPEP::HandlePlayerEnterZone(Player* player, bool isMainZone)
{
    OutdoorPvP::HandlePlayerEnterZone(player, isMainZone);

    // remove the buff from the player first; Sometimes on relog players still have the aura
    for (uint8 i = 0; i < MAX_EP_TOWERS; ++i)
        player->RemoveAurasDueToSpell(player->GetTeam() == ALLIANCE ? plaguelandsTowerBuffs[i].spellIdAlliance : plaguelandsTowerBuffs[i].spellIdHorde);

    // buff the player
    switch (player->GetTeam())
    {
        case ALLIANCE:
            if (m_towersAlliance > 0)
                player->CastSpell(player, plaguelandsTowerBuffs[m_towersAlliance - 1].spellIdAlliance, true);
            break;
        case HORDE:
            if (m_towersHorde > 0)
                player->CastSpell(player, plaguelandsTowerBuffs[m_towersHorde - 1].spellIdHorde, true);
            break;
        default:
            break;
    }
}

void OutdoorPvPEP::HandlePlayerLeaveZone(Player* player, bool isMainZone)
{
    // remove the buff from the player
    for (uint8 i = 0; i < MAX_EP_TOWERS; ++i)
        player->RemoveAurasDueToSpell(player->GetTeam() == ALLIANCE ? plaguelandsTowerBuffs[i].spellIdAlliance : plaguelandsTowerBuffs[i].spellIdHorde);

    OutdoorPvP::HandlePlayerLeaveZone(player, isMainZone);
}

void OutdoorPvPEP::HandleCreatureCreate(Creature* creature)
{
    switch (creature->GetEntry())
    {
        case NPC_SPECTRAL_FLIGHT_MASTER:
            m_flightMaster = creature->GetObjectGuid();
            creature->setFaction(m_towerOwner[TOWER_ID_PLAGUEWOOD] == ALLIANCE ? FACTION_FLIGHT_MASTER_ALLIANCE : FACTION_FLIGHT_MASTER_HORDE);
            creature->CastSpell(creature, m_towerOwner[TOWER_ID_PLAGUEWOOD] == ALLIANCE ? SPELL_SPIRIT_PARTICLES_BLUE : SPELL_SPIRIT_PARTICLES_RED, true);
            break;
        case NPC_LORDAERON_COMMANDER:
        case NPC_LORDAERON_SOLDIER:
        case NPC_LORDAERON_VETERAN:
        case NPC_LORDAERON_FIGHTER:
            m_soldiers.push_back(creature->GetObjectGuid());
            break;
    }
}

void OutdoorPvPEP::HandleGameObjectCreate(GameObject* go)
{
    OutdoorPvP::HandleGameObjectCreate(go);

    switch (go->GetEntry())
    {
        case GO_TOWER_BANNER_NORTHPASS:
            InitBanner(go, TOWER_ID_NORTHPASS);
            break;
        case GO_TOWER_BANNER_CROWNGUARD:
            InitBanner(go, TOWER_ID_CROWNGUARD);
            break;
        case GO_TOWER_BANNER_EASTWALL:
            InitBanner(go, TOWER_ID_EASTWALL);
            break;
        case GO_TOWER_BANNER_PLAGUEWOOD:
            InitBanner(go, TOWER_ID_PLAGUEWOOD);
            break;
        case GO_TOWER_BANNER:
            // sort banners
            if (go->IsWithinDist2d(plaguelandsTowerLocations[TOWER_ID_NORTHPASS][0], plaguelandsTowerLocations[TOWER_ID_NORTHPASS][1], 50.0f))
                InitBanner(go, TOWER_ID_NORTHPASS);
            else if (go->IsWithinDist2d(plaguelandsTowerLocations[TOWER_ID_CROWNGUARD][0], plaguelandsTowerLocations[TOWER_ID_CROWNGUARD][1], 50.0f))
                InitBanner(go, TOWER_ID_CROWNGUARD);
            else if (go->IsWithinDist2d(plaguelandsTowerLocations[TOWER_ID_EASTWALL][0], plaguelandsTowerLocations[TOWER_ID_EASTWALL][1], 50.0f))
                InitBanner(go, TOWER_ID_EASTWALL);
            else if (go->IsWithinDist2d(plaguelandsTowerLocations[TOWER_ID_PLAGUEWOOD][0], plaguelandsTowerLocations[TOWER_ID_PLAGUEWOOD][1], 50.0f))
                InitBanner(go, TOWER_ID_PLAGUEWOOD);
            break;
        case GO_LORDAERON_SHRINE_ALLIANCE:
            m_lordaeronShrineAlliance = go->GetObjectGuid();
            break;
        case GO_LORDAERON_SHRINE_HORDE:
            m_lordaeronShrineHorde = go->GetObjectGuid();
            break;
    }
}

void OutdoorPvPEP::HandleObjectiveComplete(uint32 eventId, std::list<Player*> players, Team team)
{
    uint32 credit = 0;

    switch (eventId)
    {
        case EVENT_CROWNGUARD_PROGRESS_ALLIANCE:
        case EVENT_CROWNGUARD_PROGRESS_HORDE:
            credit = NPC_CROWNGUARD_TOWER_QUEST_DOODAD;
            break;
        case EVENT_EASTWALL_PROGRESS_ALLIANCE:
        case EVENT_EASTWALL_PROGRESS_HORDE:
            credit = NPC_EASTWALL_TOWER_QUEST_DOODAD;
            break;
        case EVENT_NORTHPASS_PROGRESS_ALLIANCE:
        case EVENT_NORTHPASS_PROGRESS_HORDE:
            credit = NPC_NORTHPASS_TOWER_QUEST_DOODAD;
            break;
        case EVENT_PLAGUEWOOD_PROGRESS_ALLIANCE:
        case EVENT_PLAGUEWOOD_PROGRESS_HORDE:
            credit = NPC_PLAGUEWOOD_TOWER_QUEST_DOODAD;
            break;
        default:
            return;
    }

    for (std::list<Player*>::iterator itr = players.begin(); itr != players.end(); ++itr)
    {
        if ((*itr) && (*itr)->GetTeam() == team)
        {
            (*itr)->KilledMonsterCredit(credit);
            (*itr)->RewardHonor(NULL, 1, HONOR_REWARD_PLAGUELANDS);
        }
    }
}

// process the capture events
bool OutdoorPvPEP::HandleEvent(uint32 eventId, GameObject* go)
{
    for (uint8 i = 0; i < MAX_EP_TOWERS; ++i)
    {
        if (plaguelandsBanners[i] == go->GetEntry())
        {
            for (uint8 j = 0; j < 4; ++j)
            {
                if (plaguelandsTowerEvents[i][j].eventEntry == eventId)
                {
                    // prevent processing if the owner did not change (happens if progress event is called after contest event)
                    if (plaguelandsTowerEvents[i][j].team != m_towerOwner[i])
                    {
                        if (plaguelandsTowerEvents[i][j].defenseMessage)
                            sWorld.SendDefenseMessage(ZONE_ID_EASTERN_PLAGUELANDS, plaguelandsTowerEvents[i][j].defenseMessage);

                        return ProcessCaptureEvent(go, i, plaguelandsTowerEvents[i][j].team, plaguelandsTowerEvents[i][j].worldState);
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

bool OutdoorPvPEP::ProcessCaptureEvent(GameObject* go, uint32 towerId, Team team, uint32 newWorldState)
{
    if (team == ALLIANCE)
    {
        // update banner
        for (GuidList::const_iterator itr = m_towerBanners[towerId].begin(); itr != m_towerBanners[towerId].end(); ++itr)
            SetBannerVisual(go, (*itr), CAPTURE_ARTKIT_ALLIANCE, CAPTURE_ANIM_ALLIANCE);

        // update counter
        ++m_towersAlliance;
        SendUpdateWorldState(WORLD_STATE_EP_TOWER_COUNT_ALLIANCE, m_towersAlliance);

        // buff players
        BuffTeam(ALLIANCE, plaguelandsTowerBuffs[m_towersAlliance - 1].spellIdAlliance);
    }
    else if (team == HORDE)
    {
        // update banner
        for (GuidList::const_iterator itr = m_towerBanners[towerId].begin(); itr != m_towerBanners[towerId].end(); ++itr)
            SetBannerVisual(go, (*itr), CAPTURE_ARTKIT_HORDE, CAPTURE_ANIM_HORDE);

        // update counter
        ++m_towersHorde;
        SendUpdateWorldState(WORLD_STATE_EP_TOWER_COUNT_HORDE, m_towersHorde);

        // buff players
        BuffTeam(HORDE, plaguelandsTowerBuffs[m_towersHorde - 1].spellIdHorde);
    }
    else
    {
        // update banner
        for (GuidList::const_iterator itr = m_towerBanners[towerId].begin(); itr != m_towerBanners[towerId].end(); ++itr)
            SetBannerVisual(go, (*itr), CAPTURE_ARTKIT_NEUTRAL, CAPTURE_ANIM_NEUTRAL);

        if (m_towerOwner[towerId] == ALLIANCE)
        {
            // update counter
            --m_towersAlliance;
            SendUpdateWorldState(WORLD_STATE_EP_TOWER_COUNT_ALLIANCE, m_towersAlliance);

            if (m_towersAlliance == 0)
                BuffTeam(ALLIANCE, plaguelandsTowerBuffs[0].spellIdAlliance, true);
        }
        else
        {
            // update counter
            --m_towersHorde;
            SendUpdateWorldState(WORLD_STATE_EP_TOWER_COUNT_HORDE, m_towersHorde);

            if (m_towersHorde == 0)
                BuffTeam(HORDE, plaguelandsTowerBuffs[0].spellIdHorde, true);
        }
    }

    bool eventHandled = true;

    if (team != TEAM_NONE)
    {
        // update capture point owner before rewards are applied
        m_towerOwner[towerId] = team;

        // apply rewards of changed tower
        switch (towerId)
        {
            case TOWER_ID_NORTHPASS:
                RespawnGO(go, team == ALLIANCE ? m_lordaeronShrineAlliance : m_lordaeronShrineHorde, true);
                break;
            case TOWER_ID_CROWNGUARD:
                sObjectMgr.SetGraveYardLinkTeam(GRAVEYARD_ID_EASTERN_PLAGUE, GRAVEYARD_ZONE_EASTERN_PLAGUE, team);
                break;
            case TOWER_ID_EASTWALL:
                // Return false - allow the DB to handle summons
                if (m_towerOwner[TOWER_ID_NORTHPASS] != team)
                    eventHandled = false;
                break;
            case TOWER_ID_PLAGUEWOOD:
                // Return false - allow the DB to handle summons
                eventHandled = false;
                break;
        }
    }
    else
    {
        // remove rewards of changed tower
        switch (towerId)
        {
            case TOWER_ID_NORTHPASS:
                RespawnGO(go, m_towerOwner[TOWER_ID_NORTHPASS] == ALLIANCE ? m_lordaeronShrineAlliance : m_lordaeronShrineHorde, false);
                break;
            case TOWER_ID_CROWNGUARD:
                sObjectMgr.SetGraveYardLinkTeam(GRAVEYARD_ID_EASTERN_PLAGUE, GRAVEYARD_ZONE_EASTERN_PLAGUE, TEAM_INVALID);
                break;
            case TOWER_ID_EASTWALL:
                UnsummonSoldiers(go);
                break;
            case TOWER_ID_PLAGUEWOOD:
                UnsummonFlightMaster(go);
                break;
        }

        // update capture point owner after rewards have been removed
        m_towerOwner[towerId] = team;
    }

    // update tower state
    SendUpdateWorldState(m_towerWorldState[towerId], WORLD_STATE_REMOVE);
    m_towerWorldState[towerId] = newWorldState;
    SendUpdateWorldState(m_towerWorldState[towerId], WORLD_STATE_ADD);

    // there are some events which required further DB script
    return eventHandled;
}

bool OutdoorPvPEP::HandleGameObjectUse(Player* /*player*/, GameObject* go)
{
    // prevent despawning after go use
    if (go->GetEntry() == GO_LORDAERON_SHRINE_ALLIANCE || go->GetEntry() == GO_LORDAERON_SHRINE_HORDE)
        go->SetRespawnTime(0);

    return false;
}

void OutdoorPvPEP::InitBanner(GameObject* go, uint32 towerId)
{
    m_towerBanners[towerId].push_back(go->GetObjectGuid());
    go->SetGoArtKit(GetBannerArtKit(m_towerOwner[towerId]));
}

// Handle the unsummon of the spectral flight master when the Plaguewood tower is lost
void OutdoorPvPEP::UnsummonFlightMaster(const WorldObject* objRef)
{
    if (Creature* flightMaster = objRef->GetMap()->GetCreature(m_flightMaster))
        flightMaster->ForcedDespawn();
}

// Handle the unsummon of the soldiers when the Eastwall tower is lost
void OutdoorPvPEP::UnsummonSoldiers(const WorldObject* objRef)
{
    for (GuidList::const_iterator itr = m_soldiers.begin(); itr != m_soldiers.end(); ++itr)
    {
        if (Creature* soldier = objRef->GetMap()->GetCreature(*itr))
            soldier->ForcedDespawn();
    }

    m_soldiers.clear();
}
