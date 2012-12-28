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

#include "OutdoorPvPGH.h"
#include "Map.h"
#include "Object.h"
#include "Creature.h"
#include "GameObject.h"

OutdoorPvPGH::OutdoorPvPGH() : OutdoorPvP(),
    m_zoneOwner(TEAM_NONE)
{
}

void OutdoorPvPGH::HandleCreatureCreate(Creature* creature)
{
    // only handle summoned creatures
    if (!creature->IsTemporarySummon())
        return;

    switch (creature->GetEntry())
    {
        case NPC_HORSE:
        case NPC_BLACKSMITH_JASON_RIGGINS:
        case NPC_STABLE_MASTER_TIM:
        case NPC_VENDOR_ADAMS:
        case NPC_BLACKSMITH_KOLOTH:
        case NPC_STABLE_MASTER_KOR:
        case NPC_VENDOR_PURKOM:
        case NPC_RIDING_WOLF:
            m_teamVendors.push_back(creature->GetObjectGuid());
            break;
    }
}

void OutdoorPvPGH::HandleCreatureDeath(Creature* creature)
{
    switch (creature->GetEntry())
    {
        // Note: even if some soldiers or vendors are killed, they don't respawn on timer.
        // The only way to respawn them is to capture the zone from the other faction.
        case NPC_COMMANDER_HOWSER:
        case NPC_GENERAL_GORLOK:
            UnlockLighthouse(creature);
            break;
    }
}

void OutdoorPvPGH::HandleGameObjectCreate(GameObject* go)
{
    OutdoorPvP::HandleGameObjectCreate(go);

    if (go->GetEntry() == GO_VENTURE_BAY_LIGHTHOUSE)
    {
        m_capturePoint = go->GetObjectGuid();
        go->SetGoArtKit(GetBannerArtKit(m_zoneOwner));
    }
}

// process the capture events
bool OutdoorPvPGH::HandleEvent(uint32 eventId, GameObject* go)
{
    // If we are not using the lighthouse return
    if (go->GetEntry() != GO_VENTURE_BAY_LIGHTHOUSE)
        return false;

    bool eventHandled = true;

    switch (eventId)
    {
        case EVENT_LIGHTHOUSE_WIN_ALLIANCE:
            // Ignore the event if the zone is already in alliance control
            if (m_zoneOwner == ALLIANCE)
                return true;

            // Spawn the npcs only when the tower is fully controlled. Also allow the event to handle summons in DB.
            m_zoneOwner = ALLIANCE;
            LockLighthouse(go);
            DespawnVendors(go);
            eventHandled = false;
            break;
        case EVENT_LIGHTHOUSE_WIN_HORDE:
            // Ignore the event if the zone is already in horde control
            if (m_zoneOwner == HORDE)
                return true;

            // Spawn the npcs only when the tower is fully controlled. Also allow the event to handle summons in DB.
            m_zoneOwner = HORDE;
            LockLighthouse(go);
            DespawnVendors(go);
            eventHandled = false;
            break;
        case EVENT_LIGHTHOUSE_PROGRESS_ALLIANCE:
            SetBannerVisual(go, CAPTURE_ARTKIT_ALLIANCE, CAPTURE_ANIM_ALLIANCE);
            break;
        case EVENT_LIGHTHOUSE_PROGRESS_HORDE:
            SetBannerVisual(go, CAPTURE_ARTKIT_HORDE, CAPTURE_ANIM_HORDE);
            break;
        case EVENT_LIGHTHOUSE_NEUTRAL_ALLIANCE:
        case EVENT_LIGHTHOUSE_NEUTRAL_HORDE:
            m_zoneOwner = TEAM_NONE;
            SetBannerVisual(go, CAPTURE_ARTKIT_NEUTRAL, CAPTURE_ANIM_NEUTRAL);
            break;
    }

    // there are some events which required further DB script
    return eventHandled;
}

// Despawn the vendors when the lighthouse is won by the opposite faction
void OutdoorPvPGH::DespawnVendors(const WorldObject* objRef)
{
    // despawn all team vendors
    for (GuidList::const_iterator itr = m_teamVendors.begin(); itr != m_teamVendors.end(); ++itr)
    {
        if (Creature* vendor = objRef->GetMap()->GetCreature(*itr))
            vendor->ForcedDespawn();
    }
    m_teamVendors.clear();
}

// Handle Lighthouse lock when all the soldiers and the commander are spawned
void OutdoorPvPGH::LockLighthouse(const WorldObject* objRef)
{
    if (GameObject* go = objRef->GetMap()->GetGameObject(m_capturePoint))
        go->SetLootState(GO_JUST_DEACTIVATED);
    else
        // if grid is unloaded, changing the saved slider value is enough
        sOutdoorPvPMgr.SetCapturePointSlider(GO_VENTURE_BAY_LIGHTHOUSE, m_zoneOwner == ALLIANCE ? -CAPTURE_SLIDER_ALLIANCE : -CAPTURE_SLIDER_HORDE);
}

// Handle Lighthouse unlock when the commander is killed
void OutdoorPvPGH::UnlockLighthouse(const WorldObject* objRef)
{
    if (GameObject* go = objRef->GetMap()->GetGameObject(m_capturePoint))
        go->SetCapturePointSlider(m_zoneOwner == ALLIANCE ? CAPTURE_SLIDER_ALLIANCE : CAPTURE_SLIDER_HORDE);
    else
        // if grid is unloaded, resetting the saved slider value is enough
        sOutdoorPvPMgr.SetCapturePointSlider(GO_VENTURE_BAY_LIGHTHOUSE, m_zoneOwner == ALLIANCE ? CAPTURE_SLIDER_ALLIANCE : CAPTURE_SLIDER_HORDE);
}
