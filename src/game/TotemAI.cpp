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

#include "TotemAI.h"
#include "Totem.h"
#include "Creature.h"
#include "DBCStores.h"
#include "ObjectAccessor.h"
#include "SpellMgr.h"

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

int
TotemAI::Permissible(const Creature *creature)
{
    if( creature->isTotem() )
        return PERMIT_BASE_PROACTIVE;

    return PERMIT_BASE_NO;
}

TotemAI::TotemAI(Creature *c) : CreatureAI(c), i_victimGuid(0)
{
}

void
TotemAI::MoveInLineOfSight(Unit *)
{
}

void TotemAI::EnterEvadeMode()
{
    m_creature->CombatStop(true);
}

void
TotemAI::UpdateAI(const uint32 /*diff*/)
{
    if (getTotem().GetTotemType() != TOTEM_ACTIVE)
        return;

    if (!m_creature->isAlive() || m_creature->IsNonMeleeSpellCasted(false))
        return;

    // Search spell
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(getTotem().GetSpell());
    if (!spellInfo)
        return;

    // Get spell rangy
    SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(spellInfo->rangeIndex);
    float max_range = GetSpellMaxRange(srange);

    // SPELLMOD_RANGE not applied in this place just because not existence range mods for attacking totems

    // pointer to appropriate target if found any
    Unit* victim = i_victimGuid ? ObjectAccessor::GetUnit(*m_creature, i_victimGuid) : NULL;

    // Search victim if no, not attackable, or out of range, or friendly (possible in case duel end)
    if( !victim ||
        !victim->isTargetableForAttack() || !m_creature->IsWithinDistInMap(victim, max_range) ||
        m_creature->IsFriendlyTo(victim) || !victim->isVisibleForOrDetect(m_creature,m_creature,false) )
    {
        CellPair p(MaNGOS::ComputeCellPair(m_creature->GetPositionX(),m_creature->GetPositionY()));
        Cell cell(p);
        cell.data.Part.reserved = ALL_DISTRICT;

        victim = NULL;

        MaNGOS::NearestAttackableUnitInObjectRangeCheck u_check(m_creature, m_creature, max_range);
        MaNGOS::UnitLastSearcher<MaNGOS::NearestAttackableUnitInObjectRangeCheck> checker(m_creature,victim, u_check);

        TypeContainerVisitor<MaNGOS::UnitLastSearcher<MaNGOS::NearestAttackableUnitInObjectRangeCheck>, GridTypeMapContainer > grid_object_checker(checker);
        TypeContainerVisitor<MaNGOS::UnitLastSearcher<MaNGOS::NearestAttackableUnitInObjectRangeCheck>, WorldTypeMapContainer > world_object_checker(checker);

        CellLock<GridReadGuard> cell_lock(cell, p);
        cell_lock->Visit(cell_lock, grid_object_checker,  *m_creature->GetMap(), *m_creature, max_range);
        cell_lock->Visit(cell_lock, world_object_checker, *m_creature->GetMap(), *m_creature, max_range);
    }

    // If have target
    if (victim)
    {
        // remember
        i_victimGuid = victim->GetGUID();

        // attack
        m_creature->SetInFront(victim);                      // client change orientation by self
        m_creature->CastSpell(victim, getTotem().GetSpell(), false);
    }
    else
        i_victimGuid = 0;
}

bool
TotemAI::IsVisible(Unit *) const
{
    return false;
}

void
TotemAI::AttackStart(Unit *)
{
}

Totem& TotemAI::getTotem()
{
    return static_cast<Totem&>(*m_creature);
}
