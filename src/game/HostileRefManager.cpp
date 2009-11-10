/*
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "HostileRefManager.h"
#include "ThreatManager.h"
#include "Unit.h"
#include "DBCStructure.h"
#include "SpellMgr.h"

HostileRefManager::~HostileRefManager()
{
    deleteReferences();
}

//=================================================
// send threat to all my hateres for the pVictim
// The pVictim is hated than by them as well
// use for buffs and healing threat functionality

void HostileRefManager::threatAssist(Unit *pVictim, float pThreat, SpellEntry const *pThreatSpell, bool pSingleTarget)
{
    HostileReference* ref;

    uint32 size = pSingleTarget ? 1 : getSize();            // if pSingleTarget do not devide threat
    ref = getFirst();
    while(ref != NULL)
    {
        float threat = ThreatCalcHelper::calcThreat(pVictim, iOwner, pThreat, false, (pThreatSpell ? GetSpellSchoolMask(pThreatSpell) : SPELL_SCHOOL_MASK_NORMAL), pThreatSpell);
        if(pVictim == getOwner())
            ref->addThreat(float (threat) / size);          // It is faster to modify the threat durectly if possible
        else
            ref->getSource()->addThreat(pVictim, float (threat) / size);
        ref = ref->next();
    }
}

//=================================================

void HostileRefManager::addThreatPercent(int32 pValue)
{
    HostileReference* ref;

    ref = getFirst();
    while(ref != NULL)
    {
        ref->addThreatPercent(pValue);
        ref = ref->next();
    }
}

//=================================================
// The online / offline status is given to the method. The calculation has to be done before

void HostileRefManager::setOnlineOfflineState(bool pIsOnline)
{
    HostileReference* ref;

    ref = getFirst();
    while(ref != NULL)
    {
        ref->setOnlineOfflineState(pIsOnline);
        ref = ref->next();
    }
}

//=================================================
// The online / offline status is calculated and set

void HostileRefManager::updateThreatTables()
{
    HostileReference* ref = getFirst();
    while(ref)
    {
        ref->updateOnlineStatus();
        ref = ref->next();
    }
}

//=================================================
// The references are not needed anymore
// tell the source to remove them from the list and free the mem

void HostileRefManager::deleteReferences()
{
    HostileReference* ref = getFirst();
    while(ref)
    {
        HostileReference* nextRef = ref->next();
        ref->removeReference();
        delete ref;
        ref = nextRef;
    }
}

//=================================================
// delete one reference, defined by Unit

void HostileRefManager::deleteReference(Unit *pCreature)
{
    HostileReference* ref = getFirst();
    while(ref)
    {
        HostileReference* nextRef = ref->next();
        if(ref->getSource()->getOwner() == pCreature)
        {
            ref->removeReference();
            delete ref;
            break;
        }
        ref = nextRef;
    }
}

//=================================================
// set state for one reference, defined by Unit

void HostileRefManager::setOnlineOfflineState(Unit *pCreature,bool pIsOnline)
{
    HostileReference* ref = getFirst();
    while(ref)
    {
        HostileReference* nextRef = ref->next();
        if(ref->getSource()->getOwner() == pCreature)
        {
            ref->setOnlineOfflineState(pIsOnline);
            break;
        }
        ref = nextRef;
    }
}

//=================================================
