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

#ifndef _THREATMANAGER
#define _THREATMANAGER

#include "Common.h"
#include "SharedDefines.h"
#include "Utilities/LinkedReference/Reference.h"
#include "UnitEvents.h"

#include <list>

//==============================================================

class Unit;
class Creature;
class ThreatManager;
struct SpellEntry;

//==============================================================
// Class to calculate the real threat based

class ThreatCalcHelper
{
    public:
        static float calcThreat(Unit* pHatedUnit, Unit* pHatingUnit, float threat, bool crit, SpellSchoolMask schoolMask, SpellEntry const *threatSpell);
};

//==============================================================
class MANGOS_DLL_SPEC HostileReference : public Reference<Unit, ThreatManager>
{
    public:
        HostileReference(Unit* pUnit, ThreatManager *pThreatManager, float pThreat);

        //=================================================
        void addThreat(float pMod);

        void setThreat(float pThreat) { addThreat(pThreat - getThreat()); }

        void addThreatPercent(int32 pPercent) { float tmpThreat = iThreat; tmpThreat = tmpThreat * (pPercent+100) / 100; addThreat(tmpThreat-iThreat); }

        float getThreat() const { return iThreat; }

        bool isOnline() const { return iOnline; }

        // The Unit might be in water and the creature can not enter the water, but has range attack
        // in this case online = true, but accessable = false
        bool isAccessable() const { return iAccessible; }

        // used for temporary setting a threat and reducting it later again.
        // the threat modification is stored
        void setTempThreat(float pThreat) { iTempThreatModifyer = pThreat - getThreat(); if(iTempThreatModifyer != 0.0f) addThreat(iTempThreatModifyer);  }

        void resetTempThreat()
        {
            if(iTempThreatModifyer != 0.0f)
            {
                addThreat(-iTempThreatModifyer);  iTempThreatModifyer = 0.0f;
            }
        }

        float getTempThreatModifyer() { return iTempThreatModifyer; }

        //=================================================
        // check, if source can reach target and set the status
        void updateOnlineStatus();

        void setOnlineOfflineState(bool pIsOnline);

        void setAccessibleState(bool pIsAccessible);
        //=================================================

        bool operator ==(const HostileReference& pHostileReference) const { return pHostileReference.getUnitGuid() == getUnitGuid(); }

        //=================================================

        uint64 getUnitGuid() const { return iUnitGuid; }

        //=================================================
        // reference is not needed anymore. realy delete it !

        void removeReference();

        //=================================================

        HostileReference* next() { return ((HostileReference* ) Reference<Unit, ThreatManager>::next()); }

        //=================================================

        // Tell our refTo (target) object that we have a link
        void targetObjectBuildLink();

        // Tell our refTo (taget) object, that the link is cut
        void targetObjectDestroyLink();

        // Tell our refFrom (source) object, that the link is cut (Target destroyed)
        void sourceObjectDestroyLink();
    private:
        // Inform the source, that the status of that reference was changed
        void fireStatusChanged(ThreatRefStatusChangeEvent& pThreatRefStatusChangeEvent);

        Unit* getSourceUnit();
    private:
        float iThreat;
        float iTempThreatModifyer;                          // used for taunt
        uint64 iUnitGuid;
        bool iOnline;
        bool iAccessible;
};

//==============================================================
class ThreatManager;

class MANGOS_DLL_SPEC ThreatContainer
{
    private:
        std::list<HostileReference*> iThreatList;
        bool iDirty;
    protected:
        friend class ThreatManager;

        void remove(HostileReference* pRef) { iThreatList.remove(pRef); }
        void addReference(HostileReference* pHostileReference) { iThreatList.push_back(pHostileReference); }
        void clearReferences();
        // Sort the list if necessary
        void update();
    public:
        ThreatContainer() { iDirty = false; }
        ~ThreatContainer() { clearReferences(); }

        HostileReference* addThreat(Unit* pVictim, float pThreat);

        void modifyThreatPercent(Unit *pVictim, int32 percent);

        HostileReference* selectNextVictim(Creature* pAttacker, HostileReference* pCurrentVictim);

        void setDirty(bool pDirty) { iDirty = pDirty; }

        bool isDirty() { return iDirty; }

        bool empty() { return(iThreatList.empty()); }

        HostileReference* getMostHated() { return iThreatList.empty() ? NULL : iThreatList.front(); }

        HostileReference* getReferenceByTarget(Unit* pVictim);

        std::list<HostileReference*>& getThreatList() { return iThreatList; }
};

//=================================================

class MANGOS_DLL_SPEC ThreatManager
{
    public:
        friend class HostileReference;

        explicit ThreatManager(Unit *pOwner);

        ~ThreatManager() { clearReferences(); }

        void clearReferences();

        void addThreat(Unit* pVictim, float threat, bool crit, SpellSchoolMask schoolMask, SpellEntry const *threatSpell);
        void addThreat(Unit* pVictim, float threat) { addThreat(pVictim,threat,false,SPELL_SCHOOL_MASK_NONE,NULL); }
        void modifyThreatPercent(Unit *pVictim, int32 pPercent);

        float getThreat(Unit *pVictim, bool pAlsoSearchOfflineList = false);

        bool isThreatListEmpty() { return iThreatContainer.empty();}

        void processThreatEvent(ThreatRefStatusChangeEvent* threatRefStatusChangeEvent);

        HostileReference* getCurrentVictim() { return iCurrentVictim; }

        Unit*  getOwner() { return iOwner; }

        Unit* getHostileTarget();

        void tauntApply(Unit* pTaunter);
        void tauntFadeOut(Unit *pTaunter);

        void setCurrentVictim(HostileReference* pHostileReference);

        void setDirty(bool pDirty) { iThreatContainer.setDirty(pDirty); }

        // methods to access the lists from the outside to do sume dirty manipulation (scriping and such)
        // I hope they are used as little as possible.
        std::list<HostileReference*>& getThreatList() { return iThreatContainer.getThreatList(); }
        std::list<HostileReference*>& getOfflieThreatList() { return iThreatOfflineContainer.getThreatList(); }
        ThreatContainer& getOnlineContainer() { return iThreatContainer; }
        ThreatContainer& getOfflineContainer() { return iThreatOfflineContainer; }
    private:
        HostileReference* iCurrentVictim;
        Unit* iOwner;
        ThreatContainer iThreatContainer;
        ThreatContainer iThreatOfflineContainer;
};

//=================================================
#endif
