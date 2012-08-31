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

#include "OutdoorPvPHP.h"
#include "WorldPacket.h"
#include "World.h"
#include "Object.h"
#include "Creature.h"
#include "GameObject.h"
#include "Player.h"

OutdoorPvPHP::OutdoorPvPHP() : OutdoorPvP(),
    m_towersAlliance(0),
    m_towersHorde(0)
{
    m_towerWorldState[0] = WORLD_STATE_HP_OVERLOOK_NEUTRAL;
    m_towerWorldState[1] = WORLD_STATE_HP_STADIUM_NEUTRAL;
    m_towerWorldState[2] = WORLD_STATE_HP_BROKEN_HILL_NEUTRAL;

    for (uint8 i = 0; i < MAX_HP_TOWERS; ++i)
        m_towerOwner[i] = TEAM_NONE;
}

void OutdoorPvPHP::FillInitialWorldStates(WorldPacket& data, uint32& count)
{
    FillInitialWorldState(data, count, WORLD_STATE_HP_TOWER_COUNT_ALLIANCE, m_towersAlliance);
    FillInitialWorldState(data, count, WORLD_STATE_HP_TOWER_COUNT_HORDE, m_towersHorde);
    FillInitialWorldState(data, count, WORLD_STATE_HP_TOWER_DISPLAY_A, WORLD_STATE_ADD);
    FillInitialWorldState(data, count, WORLD_STATE_HP_TOWER_DISPLAY_H, WORLD_STATE_ADD);

    for (uint8 i = 0; i < MAX_HP_TOWERS; ++i)
        FillInitialWorldState(data, count, m_towerWorldState[i], WORLD_STATE_ADD);
}

void OutdoorPvPHP::SendRemoveWorldStates(Player* player)
{
    player->SendUpdateWorldState(WORLD_STATE_HP_TOWER_DISPLAY_A, WORLD_STATE_REMOVE);
    player->SendUpdateWorldState(WORLD_STATE_HP_TOWER_DISPLAY_H, WORLD_STATE_REMOVE);

    for (uint8 i = 0; i < MAX_HP_TOWERS; ++i)
        player->SendUpdateWorldState(m_towerWorldState[i], WORLD_STATE_REMOVE);
}

void OutdoorPvPHP::HandlePlayerEnterZone(Player* player, bool isMainZone)
{
    OutdoorPvP::HandlePlayerEnterZone(player, isMainZone);

    // remove the buff from the player first; Sometimes on relog players still have the aura
    player->RemoveAurasDueToSpell(player->GetTeam() == ALLIANCE ? SPELL_HELLFIRE_SUPERIORITY_ALLIANCE : SPELL_HELLFIRE_SUPERIORITY_HORDE);

    // buff the player if same team is controlling all capture points
    if (m_towersAlliance == MAX_HP_TOWERS && player->GetTeam() == ALLIANCE)
        player->CastSpell(player, SPELL_HELLFIRE_SUPERIORITY_ALLIANCE, true);
    else if (m_towersHorde == MAX_HP_TOWERS && player->GetTeam() == HORDE)
        player->CastSpell(player, SPELL_HELLFIRE_SUPERIORITY_HORDE, true);
}

void OutdoorPvPHP::HandlePlayerLeaveZone(Player* player, bool isMainZone)
{
    // remove the buff from the player
    player->RemoveAurasDueToSpell(player->GetTeam() == ALLIANCE ? SPELL_HELLFIRE_SUPERIORITY_ALLIANCE : SPELL_HELLFIRE_SUPERIORITY_HORDE);

    OutdoorPvP::HandlePlayerLeaveZone(player, isMainZone);
}

void OutdoorPvPHP::HandleGameObjectCreate(GameObject* go)
{
    OutdoorPvP::HandleGameObjectCreate(go);

    switch (go->GetEntry())
    {
        case GO_TOWER_BANNER_OVERLOOK:
            m_banners[0] = go->GetObjectGuid();
            go->SetGoArtKit(GetBannerArtKit(m_towerOwner[0], GO_ARTKIT_OVERLOOK_ALLIANCE, GO_ARTKIT_OVERLOOK_HORDE, GO_ARTKIT_OVERLOOK_NEUTRAL));
            break;
        case GO_TOWER_BANNER_STADIUM:
            m_banners[1] = go->GetObjectGuid();
            go->SetGoArtKit(GetBannerArtKit(m_towerOwner[1], GO_ARTKIT_STADIUM_ALLIANCE, GO_ARTKIT_STADIUM_HORDE, GO_ARTKIT_STADIUM_NEUTRAL));
            break;
        case GO_TOWER_BANNER_BROKEN_HILL:
            m_banners[2] = go->GetObjectGuid();
            go->SetGoArtKit(GetBannerArtKit(m_towerOwner[2], GO_ARTKIT_BROKEN_HILL_ALLIANCE, GO_ARTKIT_BROKEN_HILL_HORDE, GO_ARTKIT_BROKEN_HILL_NEUTRAL));
            break;
        case GO_HELLFIRE_BANNER_OVERLOOK:
            m_towers[0] = go->GetObjectGuid();
            go->SetGoArtKit(GetBannerArtKit(m_towerOwner[0]));
            break;
        case GO_HELLFIRE_BANNER_STADIUM:
            m_towers[1] = go->GetObjectGuid();
            go->SetGoArtKit(GetBannerArtKit(m_towerOwner[1]));
            break;
        case GO_HELLFIRE_BANNER_BROKEN_HILL:
            m_towers[2] = go->GetObjectGuid();
            go->SetGoArtKit(GetBannerArtKit(m_towerOwner[2]));
            break;
    }
}

void OutdoorPvPHP::HandleObjectiveComplete(uint32 eventId, std::list<Player*> players, Team team)
{
    uint32 credit = 0;

    switch (eventId)
    {
        case EVENT_OVERLOOK_PROGRESS_ALLIANCE:
        case EVENT_OVERLOOK_PROGRESS_HORDE:
            credit = NPC_CAPTURE_CREDIT_OVERLOOK;
            break;
        case EVENT_STADIUM_PROGRESS_ALLIANCE:
        case EVENT_STADIUM_PROGRESS_HORDE:
            credit = NPC_CAPTURE_CREDIT_STADIUM;
            break;
        case EVENT_BROKEN_HILL_PROGRESS_ALLIANCE:
        case EVENT_BROKEN_HILL_PROGRESS_HORDE:
            credit = NPC_CAPTURE_CREDIT_BROKEN_HILL;
            break;
        default:
            return;
    }

    for (std::list<Player*>::iterator itr = players.begin(); itr != players.end(); ++itr)
    {
        if ((*itr) && (*itr)->GetTeam() == team)
        {
            (*itr)->KilledMonsterCredit(credit);
            (*itr)->RewardHonor(NULL, 1, HONOR_REWARD_HELLFIRE);
        }
    }
}

// Cast player spell on opponent kill
void OutdoorPvPHP::HandlePlayerKillInsideArea(Player* player)
{
    for (uint8 i = 0; i < MAX_HP_TOWERS; ++i)
    {
        if (GameObject* capturePoint = player->GetMap()->GetGameObject(m_towers[i]))
        {
            // check capture point range
            GameObjectInfo const* info = capturePoint->GetGOInfo();
            if (info && player->IsWithinDistInMap(capturePoint, info->capturePoint.radius))
            {
                // check capture point team
                if (player->GetTeam() == m_towerOwner[i])
                    player->CastSpell(player, player->GetTeam() == ALLIANCE ? SPELL_HELLFIRE_TOWER_TOKEN_ALLIANCE : SPELL_HELLFIRE_TOWER_TOKEN_HORDE, true);

                return;
            }
        }
    }
}

// process the capture events
bool OutdoorPvPHP::HandleEvent(uint32 eventId, GameObject* go)
{
    for (uint8 i = 0; i < MAX_HP_TOWERS; ++i)
    {
        if (hellfireBanners[i] == go->GetEntry())
        {
            for (uint8 j = 0; j < 4; ++j)
            {
                if (hellfireTowerEvents[i][j].eventEntry == eventId)
                {
                    // prevent processing if the owner did not change (happens if progress event is called after contest event)
                    if (hellfireTowerEvents[i][j].team != m_towerOwner[i])
                    {
                        if (hellfireTowerEvents[i][j].defenseMessage)
                            sWorld.SendDefenseMessage(ZONE_ID_HELLFIRE_PENINSULA, hellfireTowerEvents[i][j].defenseMessage);

                        return ProcessCaptureEvent(go, i, hellfireTowerEvents[i][j].team, hellfireTowerEvents[i][j].worldState, hellfireTowerEvents[i][j].towerArtKit, hellfireTowerEvents[i][j].towerAnim);
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

bool OutdoorPvPHP::ProcessCaptureEvent(GameObject* go, uint32 towerId, Team team, uint32 newWorldState, uint32 towerArtKit, uint32 towerAnim)
{
    // set artkits and process buffs
    if (team == ALLIANCE)
    {
        SetBannerVisual(go, CAPTURE_ARTKIT_ALLIANCE, CAPTURE_ANIM_ALLIANCE);

        // update counter state
        ++m_towersAlliance;
        SendUpdateWorldState(WORLD_STATE_HP_TOWER_COUNT_ALLIANCE, m_towersAlliance);

        if (m_towersAlliance == MAX_HP_TOWERS)
            BuffTeam(ALLIANCE, SPELL_HELLFIRE_SUPERIORITY_ALLIANCE);
    }
    else if (team == HORDE)
    {
        SetBannerVisual(go, CAPTURE_ARTKIT_HORDE, CAPTURE_ANIM_HORDE);

        // update counter
        ++m_towersHorde;
        SendUpdateWorldState(WORLD_STATE_HP_TOWER_COUNT_HORDE, m_towersHorde);

        if (m_towersHorde == MAX_HP_TOWERS)
            BuffTeam(HORDE, SPELL_HELLFIRE_SUPERIORITY_HORDE);
    }
    else
    {
        SetBannerVisual(go, CAPTURE_ARTKIT_NEUTRAL, CAPTURE_ANIM_NEUTRAL);

        if (m_towerOwner[towerId] == ALLIANCE)
        {
            if (m_towersAlliance == MAX_HP_TOWERS)
                BuffTeam(ALLIANCE, SPELL_HELLFIRE_SUPERIORITY_ALLIANCE, true);

            // update counter
            --m_towersAlliance;
            SendUpdateWorldState(WORLD_STATE_HP_TOWER_COUNT_ALLIANCE, m_towersAlliance);
        }
        else
        {
            if (m_towersHorde == MAX_HP_TOWERS)
                BuffTeam(HORDE, SPELL_HELLFIRE_SUPERIORITY_HORDE, true);

            // update counter
            --m_towersHorde;
            SendUpdateWorldState(WORLD_STATE_HP_TOWER_COUNT_HORDE, m_towersHorde);
        }
    }

    SetBannerVisual(go, m_banners[towerId], towerArtKit, towerAnim);

    // update tower state
    SendUpdateWorldState(m_towerWorldState[towerId], WORLD_STATE_REMOVE);
    m_towerWorldState[towerId] = newWorldState;
    SendUpdateWorldState(m_towerWorldState[towerId], WORLD_STATE_ADD);

    // update capture point owner
    m_towerOwner[towerId] = team;

    // the are no DB exceptions in this case
    return true;
}
