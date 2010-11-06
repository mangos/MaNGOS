/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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

#include "GridNotifiers.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "UpdateData.h"
#include "Item.h"
#include "Map.h"
#include "Transports.h"
#include "ObjectAccessor.h"
#include "BattleGroundMgr.h"

using namespace MaNGOS;

void
VisibleChangesNotifier::Visit(CameraMapType &m)
{
    for(CameraMapType::iterator iter=m.begin(); iter != m.end(); ++iter)
    {
        iter->getSource()->UpdateVisibilityOf(&i_object);
    }
}

void
VisibleNotifier::Notify()
{
    Player& player = *i_camera.GetOwner();
    // at this moment i_clientGUIDs have guids that not iterate at grid level checks
    // but exist one case when this possible and object not out of range: transports
    if(Transport* transport = player.GetTransport())
    {
        for(Transport::PlayerSet::const_iterator itr = transport->GetPassengers().begin();itr!=transport->GetPassengers().end();++itr)
        {
            if (i_clientGUIDs.find((*itr)->GetGUID()) != i_clientGUIDs.end())
            {
                // ignore far sight case
                (*itr)->UpdateVisibilityOf(*itr, &player);
                player.UpdateVisibilityOf(&player, *itr, i_data, i_visibleNow);
                i_clientGUIDs.erase((*itr)->GetGUID());
            }
        }
    }

    // generate outOfRange for not iterate objects
    i_data.AddOutOfRangeGUID(i_clientGUIDs);
    for(ObjectGuidSet::iterator itr = i_clientGUIDs.begin();itr!=i_clientGUIDs.end();++itr)
    {
        player.m_clientGUIDs.erase(*itr);

        DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "%s is out of range (no in active cells set) now for %s",
            itr->GetString().c_str(), player.GetObjectGuid().GetString().c_str());
    }

    if (i_data.HasData())
    {
        // send create/outofrange packet to player (except player create updates that already sent using SendUpdateToPlayer)
        WorldPacket packet;
        i_data.BuildPacket(&packet);
        player.GetSession()->SendPacket(&packet);

        // send out of range to other players if need
        ObjectGuidSet const& oor = i_data.GetOutOfRangeGUIDs();
        for(ObjectGuidSet::const_iterator iter = oor.begin(); iter != oor.end(); ++iter)
        {
            if (!iter->IsPlayer())
                continue;

            if (Player* plr = ObjectAccessor::FindPlayer(*iter))
                plr->UpdateVisibilityOf(plr->GetCamera().GetBody(), &player);
        }
    }

    // Now do operations that required done at object visibility change to visible

    // send data at target visibility change (adding to client)
    for(std::set<WorldObject*>::const_iterator vItr = i_visibleNow.begin(); vItr != i_visibleNow.end(); ++vItr)
    {
        // target aura duration for caster show only if target exist at caster client
        if ((*vItr) != &player && (*vItr)->isType(TYPEMASK_UNIT))
            player.SendAurasForTarget((Unit*)(*vItr));

        // non finished movements show to player
        if ((*vItr)->GetTypeId()==TYPEID_UNIT && ((Creature*)(*vItr))->isAlive())
            ((Creature*)(*vItr))->SendMonsterMoveWithSpeedToCurrentDestination(&player);
    }
}

void
MessageDeliverer::Visit(CameraMapType &m)
{
    for(CameraMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
    {
        Player* owner = iter->getSource()->GetOwner();

        if (i_toSelf || owner != &i_player)
        {
            if (!i_player.InSamePhase(iter->getSource()->GetBody()))
                continue;

            if (WorldSession* session = owner->GetSession())
                session->SendPacket(i_message);
        }
    }
}

void MessageDelivererExcept::Visit(CameraMapType &m)
{
    for(CameraMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
    {
        Player* owner = iter->getSource()->GetOwner();

        if (!owner->InSamePhase(i_phaseMask) || owner == i_skipped_receiver)
            continue;

        if (WorldSession* session = owner->GetSession())
            session->SendPacket(i_message);
    }
}


void
ObjectMessageDeliverer::Visit(CameraMapType &m)
{
    for(CameraMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
    {
        if(!iter->getSource()->GetBody()->InSamePhase(i_phaseMask))
            continue;

        if(WorldSession* session = iter->getSource()->GetOwner()->GetSession())
            session->SendPacket(i_message);
    }
}

void
MessageDistDeliverer::Visit(CameraMapType &m)
{
    for(CameraMapType::iterator iter=m.begin(); iter != m.end(); ++iter)
    {
        Player * owner = iter->getSource()->GetOwner();

        if ((i_toSelf || owner != &i_player) &&
            (!i_ownTeamOnly || owner->GetTeam() == i_player.GetTeam()) &&
            (!i_dist || iter->getSource()->GetBody()->IsWithinDist(&i_player,i_dist)))
        {
            if (!i_player.InSamePhase(iter->getSource()->GetBody()))
                continue;

            if (WorldSession* session = owner->GetSession())
                session->SendPacket(i_message);
        }
    }
}

void
ObjectMessageDistDeliverer::Visit(CameraMapType &m)
{
    for(CameraMapType::iterator iter=m.begin(); iter != m.end(); ++iter)
    {
        if (!i_dist || iter->getSource()->GetBody()->IsWithinDist(&i_object,i_dist))
        {
            if (!i_object.InSamePhase(iter->getSource()->GetBody()))
                continue;

            if (WorldSession* session = iter->getSource()->GetOwner()->GetSession())
                session->SendPacket(i_message);
        }
    }
}

template<class T> void
ObjectUpdater::Visit(GridRefManager<T> &m)
{
    for(typename GridRefManager<T>::iterator iter = m.begin(); iter != m.end(); ++iter)
        iter->getSource()->Update(i_realdiff, i_diff);
}

bool CannibalizeObjectCheck::operator()(Corpse* u)
{
    // ignore bones
    if(u->GetType()==CORPSE_BONES)
        return false;

    Player* owner = ObjectAccessor::FindPlayer(u->GetOwnerGUID());

    if( !owner || i_fobj->IsFriendlyTo(owner))
        return false;

    if(i_fobj->IsWithinDistInMap(u, i_range) )
        return true;

    return false;
}

void MaNGOS::RespawnDo::operator()( Creature* u ) const
{
    // prevent respawn creatures for not active BG event
    Map* map = u->GetMap();
    if (map->IsBattleGroundOrArena())
    {
        BattleGroundEventIdx eventId = sBattleGroundMgr.GetCreatureEventIndex(u->GetDBTableGUIDLow());
        if (!((BattleGroundMap*)map)->GetBG()->IsActiveEvent(eventId.event1, eventId.event2))
            return;
    }

    u->Respawn();
}

void MaNGOS::RespawnDo::operator()( GameObject* u ) const
{
    // prevent respawn gameobject for not active BG event
    Map* map = u->GetMap();
    if (map->IsBattleGroundOrArena())
    {
        BattleGroundEventIdx eventId = sBattleGroundMgr.GetGameObjectEventIndex(u->GetDBTableGUIDLow());
        if (!((BattleGroundMap*)map)->GetBG()->IsActiveEvent(eventId.event1, eventId.event2))
            return;
    }

    u->Respawn();
}


template void ObjectUpdater::Visit<GameObject>(GameObjectMapType &);
template void ObjectUpdater::Visit<DynamicObject>(DynamicObjectMapType &);
