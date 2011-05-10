/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
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

#include "CreatureAI.h"
#include "Creature.h"
#include "DBCStores.h"
#include "Spell.h"

CreatureAI::~CreatureAI()
{
}

void CreatureAI::AttackedBy( Unit* attacker )
{
    if(!m_creature->getVictim())
        AttackStart(attacker);
}

CanCastResult CreatureAI::CanCastSpell(Unit* pTarget, const SpellEntry *pSpell, bool isTriggered)
{
    // If not triggered, we check
    if (!isTriggered)
    {
        // State does not allow
        if (m_creature->hasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL))
            return CAST_FAIL_STATE;

        if (pSpell->PreventionType == SPELL_PREVENTION_TYPE_SILENCE && m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
            return CAST_FAIL_STATE;

        if (pSpell->PreventionType == SPELL_PREVENTION_TYPE_PACIFY && m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED))
            return CAST_FAIL_STATE;

        // Check for power (also done by Spell::CheckCast())
        if (m_creature->GetPower((Powers)pSpell->powerType) < Spell::CalculatePowerCost(pSpell, m_creature))
            return CAST_FAIL_POWER;
    }

    if (const SpellRangeEntry *pSpellRange = sSpellRangeStore.LookupEntry(pSpell->rangeIndex))
    {
        if (pTarget != m_creature)
        {
            // pTarget is out of range of this spell (also done by Spell::CheckCast())
            float fDistance = m_creature->GetCombatDistance(pTarget);

            if (fDistance > (m_creature->IsHostileTo(pTarget) ? pSpellRange->maxRange : pSpellRange->maxRangeFriendly))
                return CAST_FAIL_TOO_FAR;

            float fMinRange = m_creature->IsHostileTo(pTarget) ? pSpellRange->minRange : pSpellRange->minRangeFriendly;

            if (fMinRange && fDistance < fMinRange)
                return CAST_FAIL_TOO_CLOSE;
        }

        return CAST_OK;
    }
    else
        return CAST_FAIL_OTHER;
}

CanCastResult CreatureAI::DoCastSpellIfCan(Unit* pTarget, uint32 uiSpell, uint32 uiCastFlags, ObjectGuid uiOriginalCasterGUID)
{
    Unit* pCaster = m_creature;

    if (uiCastFlags & CAST_FORCE_TARGET_SELF)
        pCaster = pTarget;

    // Allowed to cast only if not casting (unless we interrupt ourself) or if spell is triggered
    if (!pCaster->IsNonMeleeSpellCasted(false) || (uiCastFlags & (CAST_TRIGGERED | CAST_INTERRUPT_PREVIOUS)))
    {
        if (const SpellEntry* pSpell = sSpellStore.LookupEntry(uiSpell))
        {
            // If cast flag CAST_AURA_NOT_PRESENT is active, check if target already has aura on them
            if (uiCastFlags & CAST_AURA_NOT_PRESENT)
            {
                if (pTarget->HasAura(uiSpell))
                    return CAST_FAIL_TARGET_AURA;
            }

            // Check if cannot cast spell
            if (!(uiCastFlags & (CAST_FORCE_TARGET_SELF | CAST_FORCE_CAST)))
            {
                CanCastResult castResult = CanCastSpell(pTarget, pSpell, uiCastFlags & CAST_TRIGGERED);

                if (castResult != CAST_OK)
                    return castResult;
            }

            // Interrupt any previous spell
            if (uiCastFlags & CAST_INTERRUPT_PREVIOUS && pCaster->IsNonMeleeSpellCasted(false))
                pCaster->InterruptNonMeleeSpells(false);

            pCaster->CastSpell(pTarget, pSpell, uiCastFlags & CAST_TRIGGERED, NULL, NULL, uiOriginalCasterGUID);
            return CAST_OK;
        }
        else
        {
            sLog.outErrorDb("DoCastSpellIfCan by creature entry %u attempt to cast spell %u but spell does not exist.", m_creature->GetEntry(), uiSpell);
            return CAST_FAIL_OTHER;
        }
    }
    else
        return CAST_FAIL_IS_CASTING;
}

bool CreatureAI::DoMeleeAttackIfReady()
{
    // Check target
    if (!m_creature->getVictim())
        return false;

    // Make sure our attack is ready before checking distance
    if (!m_creature->isAttackReady())
        return false;

    // If we are within range melee the target
    if (!m_creature->CanReachWithMeleeAttack(m_creature->getVictim()))
        return false;

    m_creature->AttackerStateUpdate(m_creature->getVictim());
    m_creature->resetAttackTimer();

    return true;
}
