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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _HOSTILEREFMANAGER
#define _HOSTILEREFMANAGER

#include "Common.h"
#include "ObjectGuid.h"
#include "Utilities/LinkedReference/RefManager.h"

class Unit;
class ThreatManager;
class HostileReference;
struct SpellEntry;

//=================================================

class HostileRefManager : public RefManager<Unit, ThreatManager>
{
    public:
        explicit HostileRefManager(Unit *pOwner);
        ~HostileRefManager();

        Unit* getOwner() { return iOwner; }

        // send threat to all my hateres for the pVictim
        // The pVictim is hated than by them as well
        // use for buffs and healing threat functionality
        void threatAssist(Unit *pVictim, float threat, SpellEntry const *threatSpell = 0, bool pSingleTarget=false);

        void addThreatPercent(int32 pValue);

        // The references are not needed anymore
        // tell the source to remove them from the list and free the mem
        void deleteReferences();

        // Remove specific faction references
        void deleteReferencesForFaction(uint32 faction);

        HostileReference* getFirst() { return ((HostileReference* ) RefManager<Unit, ThreatManager>::getFirst()); }

        void updateThreatTables();

        void setOnlineOfflineState(bool pIsOnline);

        // set state for one reference, defined by Unit
        void setOnlineOfflineState(Unit *pCreature,bool pIsOnline);

        // delete one reference, defined by Unit
        void deleteReference(Unit *pCreature);

        // redirection threat data
        void SetThreatRedirection(ObjectGuid guid, uint32 pct)
        {
            m_redirectionTargetGuid = guid;
            m_redirectionMod = pct/100.0f;
        }

        void ResetThreatRedirection()
        {
            m_redirectionTargetGuid.Clear();
            m_redirectionMod = 0.0f;
        }

        float GetThreatRedirectionMod() const { return m_redirectionMod; }
        Unit*  GetThreatRedirectionTarget() const;

    private:
        Unit* iOwner;                                       // owner of manager variable, back ref. to it, always exist

        float      m_redirectionMod;
        ObjectGuid m_redirectionTargetGuid;
};
//=================================================
#endif
