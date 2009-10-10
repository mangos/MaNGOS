/*
 * Copyright (C) 2005-2008 MaNGOS <http://getmangos.com/>
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

#include "OutdoorPvPMgr.h"
#include "OutdoorPvPHP.h"
#include "OutdoorPvPNA.h"
#include "OutdoorPvPTF.h"
#include "OutdoorPvPZM.h"
#include "OutdoorPvPSI.h"
#include "OutdoorPvPEP.h"
#include "OutdoorPvPLA.h"
#include "Player.h"
#include "Policies/SingletonImp.h"

INSTANTIATE_SINGLETON_1( OutdoorPvPMgr );

OutdoorPvPMgr::OutdoorPvPMgr()
{
    sLog.outDebug("OutdoorPvPMgr: Instantiating");
}

OutdoorPvPMgr::~OutdoorPvPMgr()
{
    sLog.outDebug("OutdoorPvPMgr: Deleting");
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        (*itr)->DeleteSpawns();
    }
}

void OutdoorPvPMgr::CreateOutdoorPvP(uint32 typeId)
{
    OutdoorPvP * pOP = NULL;
    switch (typeId)
    {
        case OUTDOOR_PVP_HP: pOP = new OutdoorPvPHP; break;
        case OUTDOOR_PVP_NA: pOP = new OutdoorPvPNA; break;
        case OUTDOOR_PVP_TF: pOP = new OutdoorPvPTF; break;
        case OUTDOOR_PVP_ZM: pOP = new OutdoorPvPZM; break;
        case OUTDOOR_PVP_SI: pOP = new OutdoorPvPSI; break;
        case OUTDOOR_PVP_EP: pOP = new OutdoorPvPEP; break;
        case OUTDOOR_PVP_LA: pOP = new OutdoorPvPLA; break;
        default: break;
    }
    if(!pOP)
    {
        sLog.outDebug("OutdoorPvPMgr: init failed for %i.", typeId);
        return;
    }
    if(!pOP->SetupOutdoorPvP())
    {
        sLog.outDebug("OutdoorPvPMgr: SetupOutdoorPvP failed for %i.", typeId);
        delete pOP;
        return;
    }
    pOP->SetTypeId(typeId);
    m_OutdoorPvPSet.insert(pOP);
    sLog.outDebug("OutdoorPvPMgr: init successfull for %i.", typeId);

}

void OutdoorPvPMgr::InitOutdoorPvP()
{
    CreateOutdoorPvP(OUTDOOR_PVP_HP);
    CreateOutdoorPvP(OUTDOOR_PVP_NA);
    CreateOutdoorPvP(OUTDOOR_PVP_TF);
    CreateOutdoorPvP(OUTDOOR_PVP_ZM);
    CreateOutdoorPvP(OUTDOOR_PVP_SI);
    CreateOutdoorPvP(OUTDOOR_PVP_EP);
    CreateOutdoorPvP(OUTDOOR_PVP_LA);
}

void OutdoorPvPMgr::AddZone(uint32 zoneid, OutdoorPvP *handle)
{
    m_OutdoorPvPMap[zoneid] = handle;
}


void OutdoorPvPMgr::HandlePlayerEnterZone(Player *plr, uint32 zoneid)
{
    OutdoorPvPMap::iterator itr = m_OutdoorPvPMap.find(zoneid);
    if(itr == m_OutdoorPvPMap.end())
    {
        // no handle for this zone, return
        return;
    }
    // add possibly beneficial buffs to plr for zone
    itr->second->HandlePlayerEnterZone(plr, zoneid);
    //plr->SendInitWorldStates(); TODO: look if this is needed
    sLog.outDebug("OutdoorPvPMgr: Player %u entered outdoorpvp id %u", plr->GetGUIDLow(), itr->second->GetTypeId());
}

void OutdoorPvPMgr::HandlePlayerLeaveZone(Player *plr, uint32 zoneid)
{
    OutdoorPvPMap::iterator itr = m_OutdoorPvPMap.find(zoneid);
    if(itr == m_OutdoorPvPMap.end())
    {
        // no handle for this zone, return
        return;
    }
    // inform the OutdoorPvP class of the leaving, it should remove the player from all objectives
    itr->second->HandlePlayerLeaveZone(plr, zoneid);
    sLog.outDebug("OutdoorPvPMgr: Player %u left outdoorpvp id %u", plr->GetGUIDLow(), itr->second->GetTypeId());
}

OutdoorPvP * OutdoorPvPMgr::GetOutdoorPvPToZoneId(uint32 zoneid)
{
    OutdoorPvPMap::iterator itr = m_OutdoorPvPMap.find(zoneid);
    if(itr == m_OutdoorPvPMap.end())
    {
        // no handle for this zone, return
        return NULL;
    }
    return itr->second;
}

void OutdoorPvPMgr::Update(uint32 diff)
{
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        (*itr)->Update(diff);
    }
}

bool OutdoorPvPMgr::HandleCustomSpell(Player *plr, uint32 spellId, GameObject * go)
{
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        if((*itr)->HandleCustomSpell(plr,spellId,go))
            return true;
    }
    return false;
}

bool OutdoorPvPMgr::HandleOpenGo(Player *plr, uint64 guid)
{
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        if((*itr)->HandleOpenGo(plr,guid))
            return true;
    }
    return false;
}

bool OutdoorPvPMgr::HandleCaptureCreaturePlayerMoveInLos(Player * plr, Creature * c)
{
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        if((*itr)->HandleCaptureCreaturePlayerMoveInLos(plr,c))
            return true;
    }
    return false;
}

void OutdoorPvPMgr::HandleGossipOption(Player *plr, uint64 guid, uint32 gossipid)
{
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        if((*itr)->HandleGossipOption(plr,guid,gossipid))
            return;
    }
}

bool OutdoorPvPMgr::CanTalkTo(Player * plr, Creature * c, GossipOption & gso)
{
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        if((*itr)->CanTalkTo(plr,c,gso))
            return true;
    }
    return false;
}

void OutdoorPvPMgr::HandleDropFlag(Player *plr, uint32 spellId)
{
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        if((*itr)->HandleDropFlag(plr,spellId))
            return;
    }
}
