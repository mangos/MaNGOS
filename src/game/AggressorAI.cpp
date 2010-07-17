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

#include "AggressorAI.h"
#include "Errors.h"
#include "Creature.h"
#include "SharedDefines.h"
#include "ObjectAccessor.h"
#include "VMapFactory.h"
#include "World.h"

#include <list>

int
AggressorAI::Permissible(const Creature *creature)
{
    // have some hostile factions, it will be selected by IsHostileTo check at MoveInLineOfSight
    if( !creature->isCivilian() && !creature->IsNeutralToAll() )
        return PERMIT_BASE_PROACTIVE;

    return PERMIT_BASE_NO;
}

AggressorAI::AggressorAI(Creature *c) : CreatureAI(c), i_victimGuid(0), i_state(STATE_NORMAL), i_tracker(TIME_INTERVAL_LOOK)
{
}

void
AggressorAI::MoveInLineOfSight(Unit *u)
{
    // Ignore Z for flying creatures
    if( !m_creature->canFly() && m_creature->GetDistanceZ(u) > CREATURE_Z_ATTACK_RANGE )
        return;

    if (m_creature->CanInitiateAttack() && u->isTargetableForAttack() &&
        m_creature->IsHostileTo(u) && u->isInAccessablePlaceFor(m_creature))
    {
        float attackRadius = m_creature->GetAttackDistance(u);
        if(m_creature->IsWithinDistInMap(u, attackRadius) && m_creature->IsWithinLOSInMap(u) )
        {
            if(!m_creature->getVictim())
            {
                AttackStart(u);
                u->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
            }
            else if(sMapStore.LookupEntry(m_creature->GetMapId())->IsDungeon())
            {
                m_creature->AddThreat(u);
                u->SetInCombatWith(m_creature);
            }
        }
    }
}

void AggressorAI::EnterEvadeMode()
{
    if (!m_creature->isAlive())
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Creature stopped attacking, he is dead [guid=%u]", m_creature->GetGUIDLow());
        i_victimGuid = 0;
        m_creature->CombatStop(true);
        m_creature->DeleteThreatList();
        return;
    }

    Unit* victim = ObjectAccessor::GetUnit(*m_creature, i_victimGuid );

    if (!victim)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Creature stopped attacking, no victim [guid=%u]", m_creature->GetGUIDLow());
    }
    else if (!victim->isAlive())
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Creature stopped attacking, victim is dead [guid=%u]", m_creature->GetGUIDLow());
    }
    else if (victim->HasStealthAura())
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Creature stopped attacking, victim is in stealth [guid=%u]", m_creature->GetGUIDLow());
    }
    else if (victim->IsTaxiFlying())
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Creature stopped attacking, victim is in flight [guid=%u]", m_creature->GetGUIDLow());
    }
    else
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Creature stopped attacking, victim out run him [guid=%u]", m_creature->GetGUIDLow());
        //i_state = STATE_LOOK_AT_VICTIM;
        //i_tracker.Reset(TIME_INTERVAL_LOOK);
    }

    if (!m_creature->isCharmed())
    {
        m_creature->RemoveAllAuras();

        // Remove ChaseMovementGenerator from MotionMaster stack list, and add HomeMovementGenerator instead
        if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
            m_creature->GetMotionMaster()->MoveTargetedHome();
    }

    m_creature->DeleteThreatList();
    i_victimGuid = 0;
    m_creature->CombatStop(true);
    m_creature->SetLootRecipient(NULL);
}

void
AggressorAI::UpdateAI(const uint32 /*diff*/)
{
    // update i_victimGuid if m_creature->getVictim() !=0 and changed
    if(!m_creature->SelectHostileTarget() || !m_creature->getVictim())
        return;

    i_victimGuid = m_creature->getVictim()->GetGUID();

    if( m_creature->isAttackReady() )
    {
        if( m_creature->IsWithinDistInMap(m_creature->getVictim(), ATTACK_DISTANCE))
        {
            m_creature->AttackerStateUpdate(m_creature->getVictim());
            m_creature->resetAttackTimer();
        }
    }
}

bool
AggressorAI::IsVisible(Unit *pl) const
{
    return m_creature->IsWithinDist(pl,sWorld.getConfig(CONFIG_FLOAT_SIGHT_MONSTER))
        && pl->isVisibleForOrDetect(m_creature,m_creature,true);
}

void
AggressorAI::AttackStart(Unit *u)
{
    if( !u )
        return;

    if(m_creature->Attack(u,true))
    {
        i_victimGuid = u->GetGUID();

        m_creature->AddThreat(u);
        m_creature->SetInCombatWith(u);
        u->SetInCombatWith(m_creature);

        m_creature->GetMotionMaster()->MoveChase(u);
    }
}
