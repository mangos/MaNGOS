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

#include "GuardAI.h"
#include "Errors.h"
#include "Creature.h"
#include "Player.h"
#include "World.h"

int GuardAI::Permissible(const Creature *creature)
{
    if (creature->IsGuard())
        return PERMIT_BASE_SPECIAL;

    return PERMIT_BASE_NO;
}

GuardAI::GuardAI(Creature *c) : CreatureAI(c), i_state(STATE_NORMAL), i_tracker(TIME_INTERVAL_LOOK)
{
}

void GuardAI::MoveInLineOfSight(Unit *u)
{
    // Ignore Z for flying creatures
    if (!m_creature->CanFly() && m_creature->GetDistanceZ(u) > CREATURE_Z_ATTACK_RANGE)
        return;

    if (!m_creature->getVictim() && u->isTargetableForAttack() &&
        ( u->IsHostileToPlayers() || m_creature->IsHostileTo(u) /*|| u->getVictim() && m_creature->IsFriendlyTo(u->getVictim())*/ ) &&
        u->isInAccessablePlaceFor(m_creature))
    {
        float attackRadius = m_creature->GetAttackDistance(u);
        if (m_creature->IsWithinDistInMap(u,attackRadius))
        {
            //Need add code to let guard support player
            AttackStart(u);
            u->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
        }
    }
}

void GuardAI::EnterEvadeMode()
{
    if (!m_creature->isAlive())
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "Creature stopped attacking because he's dead [guid=%u]", m_creature->GetGUIDLow());
        m_creature->StopMoving();
        m_creature->GetMotionMaster()->MoveIdle();

        i_state = STATE_NORMAL;

        i_victimGuid.Clear();
        m_creature->CombatStop(true);
        m_creature->DeleteThreatList();
        return;
    }

    Unit* victim = m_creature->GetMap()->GetUnit(i_victimGuid);

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
    }

    m_creature->RemoveAllAuras();
    m_creature->DeleteThreatList();
    i_victimGuid.Clear();
    m_creature->CombatStop(true);
    i_state = STATE_NORMAL;

    // Remove ChaseMovementGenerator from MotionMaster stack list, and add HomeMovementGenerator instead
    if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
        m_creature->GetMotionMaster()->MoveTargetedHome();
}

void GuardAI::UpdateAI(const uint32 /*diff*/)
{
    // update i_victimGuid if i_creature.getVictim() !=0 and changed
    if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
        return;

    i_victimGuid = m_creature->getVictim()->GetObjectGuid();

    DoMeleeAttackIfReady();
}

bool GuardAI::IsVisible(Unit *pl) const
{
    return m_creature->IsWithinDist(pl,sWorld.getConfig(CONFIG_FLOAT_SIGHT_GUARDER))
        && pl->isVisibleForOrDetect(m_creature,m_creature,true);
}

void GuardAI::AttackStart(Unit *u)
{
    if( !u )
        return;

    if(m_creature->Attack(u,true))
    {
        i_victimGuid = u->GetObjectGuid();
        m_creature->AddThreat(u);
        m_creature->SetInCombatWith(u);
        u->SetInCombatWith(m_creature);

        m_creature->GetMotionMaster()->MoveChase(u);
    }
}

void GuardAI::JustDied(Unit *killer)
{
    if(Player* pkiller = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
        m_creature->SendZoneUnderAttackMessage(pkiller);
}
