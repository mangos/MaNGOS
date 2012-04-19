/*
 * Copyright (C) 2009-2012 MaNGOS <http://getmangos.com/>
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

#include "Common.h"
#include "CreatureEventAI.h"
#include "CreatureEventAIMgr.h"
#include "ObjectMgr.h"
#include "Spell.h"
#include "World.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GameEventMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "InstanceData.h"
#include "Chat.h"
#include "Language.h"

bool CreatureEventAIHolder::UpdateRepeatTimer( Creature* creature, uint32 repeatMin, uint32 repeatMax )
{
    if (repeatMin == repeatMax)
        Time = repeatMin;
    else if (repeatMax > repeatMin)
        Time = urand(repeatMin, repeatMax);
    else
    {
        sLog.outErrorDb("CreatureEventAI: Creature %u using Event %u (Type = %u) has RandomMax < RandomMin. Event repeating disabled.", creature->GetEntry(), Event.event_id, Event.event_type);
        Enabled = false;
        return false;
    }

    return true;
}

int CreatureEventAI::Permissible(const Creature *creature)
{
    if( creature->GetAIName() == "EventAI" )
        return PERMIT_BASE_SPECIAL;
    return PERMIT_BASE_NO;
}

void CreatureEventAI::GetAIInformation(ChatHandler& reader)
{
    reader.PSendSysMessage(LANG_NPC_EVENTAI_PHASE, (uint32)m_Phase);
    reader.PSendSysMessage(LANG_NPC_EVENTAI_MOVE, reader.GetOnOffStr(m_CombatMovementEnabled));
    reader.PSendSysMessage(LANG_NPC_EVENTAI_COMBAT, reader.GetOnOffStr(m_MeleeEnabled));
}

CreatureEventAI::CreatureEventAI(Creature *c ) : CreatureAI(c)
{
    // Need make copy for filter unneeded steps and safe in case table reload
    CreatureEventAI_Event_Map::const_iterator creatureEventsItr = sEventAIMgr.GetCreatureEventAIMap().find(m_creature->GetEntry());
    if (creatureEventsItr != sEventAIMgr.GetCreatureEventAIMap().end())
    {
        uint32 events_count = 0;
        for (CreatureEventAI_Event_Vec::const_iterator i = (*creatureEventsItr).second.begin(); i != (*creatureEventsItr).second.end(); ++i)
        {
            //Debug check
            #ifndef MANGOS_DEBUG
            if ((*i).event_flags & EFLAG_DEBUG_ONLY)
                continue;
            #endif
            if (m_creature->GetMap()->IsDungeon())
            {
                if ((1 << (m_creature->GetMap()->GetSpawnMode()+1)) & (*i).event_flags)
                {
                    ++events_count;
                }
            }
            else
                ++events_count;
        }
        //EventMap had events but they were not added because they must be for instance
        if (events_count == 0)
            sLog.outError("CreatureEventAI: Creature %u has events but no events added to list because of instance flags.", m_creature->GetEntry());
        else
        {
            m_CreatureEventAIList.reserve(events_count);
            for (CreatureEventAI_Event_Vec::const_iterator i = (*creatureEventsItr).second.begin(); i != (*creatureEventsItr).second.end(); ++i)
            {

                //Debug check
                #ifndef MANGOS_DEBUG
                if ((*i).event_flags & EFLAG_DEBUG_ONLY)
                    continue;
                #endif
                if (m_creature->GetMap()->IsDungeon())
                {
                    if ((1 << (m_creature->GetMap()->GetSpawnMode()+1)) & (*i).event_flags)
                    {
                        //event flagged for instance mode
                        m_CreatureEventAIList.push_back(CreatureEventAIHolder(*i));
                    }
                }
                else
                    m_CreatureEventAIList.push_back(CreatureEventAIHolder(*i));
            }
        }
    }
    else
        sLog.outError("CreatureEventAI: EventMap for Creature %u is empty but creature is using CreatureEventAI.", m_creature->GetEntry());

    m_bEmptyList = m_CreatureEventAIList.empty();
    m_Phase = 0;
    m_CombatMovementEnabled = true;
    m_MeleeEnabled = true;
    m_AttackDistance = 0.0f;
    m_AttackAngle = 0.0f;

    m_InvinceabilityHpLevel = 0;

    //Handle Spawned Events
    if (!m_bEmptyList)
    {
        for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
            if (SpawnedEventConditionsCheck((*i).Event))
                ProcessEvent(*i);
    }
    Reset();
}

bool CreatureEventAI::ProcessEvent(CreatureEventAIHolder& pHolder, Unit* pActionInvoker)
{
    if (!pHolder.Enabled || pHolder.Time)
        return false;

    //Check the inverse phase mask (event doesn't trigger if current phase bit is set in mask)
    if (pHolder.Event.event_inverse_phase_mask & (1 << m_Phase))
        return false;

    CreatureEventAI_Event const& event = pHolder.Event;

    //Check event conditions based on the event type, also reset events
    switch (event.event_type)
    {
        case EVENT_T_TIMER:
            if (!m_creature->isInCombat())
                return false;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.timer.repeatMin,event.timer.repeatMax);
            break;
        case EVENT_T_TIMER_OOC:
            if (m_creature->isInCombat() || m_creature->IsInEvadeMode())
                return false;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.timer.repeatMin,event.timer.repeatMax);
            break;
        case EVENT_T_HP:
        {
            if (!m_creature->isInCombat() || !m_creature->GetMaxHealth())
                return false;

            uint32 perc = (m_creature->GetHealth()*100) / m_creature->GetMaxHealth();

            if (perc > event.percent_range.percentMax || perc < event.percent_range.percentMin)
                return false;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.percent_range.repeatMin,event.percent_range.repeatMax);
            break;
        }
        case EVENT_T_MANA:
        {
            if (!m_creature->isInCombat() || !m_creature->GetMaxPower(POWER_MANA))
                return false;

            uint32 perc = (m_creature->GetPower(POWER_MANA)*100) / m_creature->GetMaxPower(POWER_MANA);

            if (perc > event.percent_range.percentMax || perc < event.percent_range.percentMin)
                return false;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.percent_range.repeatMin,event.percent_range.repeatMax);
            break;
        }
        case EVENT_T_AGGRO:
            break;
        case EVENT_T_KILL:
            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.kill.repeatMin,event.kill.repeatMax);
            break;
        case EVENT_T_DEATH:
        case EVENT_T_EVADE:
            break;
        case EVENT_T_SPELLHIT:
            //Spell hit is special case, param1 and param2 handled within CreatureEventAI::SpellHit

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.spell_hit.repeatMin,event.spell_hit.repeatMax);
            break;
        case EVENT_T_RANGE:
            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.range.repeatMin,event.range.repeatMax);
            break;
        case EVENT_T_OOC_LOS:
            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.ooc_los.repeatMin,event.ooc_los.repeatMax);
            break;
        case EVENT_T_SPAWNED:
            break;
        case EVENT_T_TARGET_HP:
        {
            if (!m_creature->isInCombat() || !m_creature->getVictim() || !m_creature->getVictim()->GetMaxHealth())
                return false;

            uint32 perc = (m_creature->getVictim()->GetHealth()*100) / m_creature->getVictim()->GetMaxHealth();

            if (perc > event.percent_range.percentMax || perc < event.percent_range.percentMin)
                return false;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.percent_range.repeatMin,event.percent_range.repeatMax);
            break;
        }
        case EVENT_T_TARGET_CASTING:
            if (!m_creature->isInCombat() || !m_creature->getVictim() || !m_creature->getVictim()->IsNonMeleeSpellCasted(false, false, true))
                return false;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.target_casting.repeatMin,event.target_casting.repeatMax);
            break;
        case EVENT_T_FRIENDLY_HP:
        {
            if (!m_creature->isInCombat())
                return false;

            Unit* pUnit = DoSelectLowestHpFriendly((float)event.friendly_hp.radius, event.friendly_hp.hpDeficit);
            if (!pUnit)
                return false;

            pActionInvoker = pUnit;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.friendly_hp.repeatMin,event.friendly_hp.repeatMax);
            break;
        }
        case EVENT_T_FRIENDLY_IS_CC:
        {
            if (!m_creature->isInCombat())
                return false;

            std::list<Creature*> pList;
            DoFindFriendlyCC(pList, (float)event.friendly_is_cc.radius);

            //List is empty
            if (pList.empty())
                return false;

            //We don't really care about the whole list, just return first available
            pActionInvoker = *(pList.begin());

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.friendly_is_cc.repeatMin,event.friendly_is_cc.repeatMax);
            break;
        }
        case EVENT_T_FRIENDLY_MISSING_BUFF:
        {
            std::list<Creature*> pList;
            DoFindFriendlyMissingBuff(pList, (float)event.friendly_buff.radius, event.friendly_buff.spellId);

            //List is empty
            if (pList.empty())
                return false;

            //We don't really care about the whole list, just return first available
            pActionInvoker = *(pList.begin());

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.friendly_buff.repeatMin,event.friendly_buff.repeatMax);
            break;
        }
        case EVENT_T_SUMMONED_UNIT:
        case EVENT_T_SUMMONED_JUST_DIED:
        case EVENT_T_SUMMONED_JUST_DESPAWN:
        {
            //Prevent event from occuring on no unit or non creatures
            if (!pActionInvoker || pActionInvoker->GetTypeId()!=TYPEID_UNIT)
                return false;

            //Creature id doesn't match up
            if (((Creature*)pActionInvoker)->GetEntry() != event.summoned.creatureId)
                return false;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.summoned.repeatMin,event.summoned.repeatMax);
            break;
        }
        case EVENT_T_TARGET_MANA:
        {
            if (!m_creature->isInCombat() || !m_creature->getVictim() || !m_creature->getVictim()->GetMaxPower(POWER_MANA))
                return false;

            uint32 perc = (m_creature->getVictim()->GetPower(POWER_MANA)*100) / m_creature->getVictim()->GetMaxPower(POWER_MANA);

            if (perc > event.percent_range.percentMax || perc < event.percent_range.percentMin)
                return false;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.percent_range.repeatMin,event.percent_range.repeatMax);
            break;
        }
        case EVENT_T_REACHED_HOME:
        case EVENT_T_RECEIVE_EMOTE:
            break;
        case EVENT_T_AURA:
        {
            SpellAuraHolder* holder = m_creature->GetSpellAuraHolder(event.buffed.spellId);
            if (!holder || holder->GetStackAmount() < event.buffed.amount)
                return false;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.buffed.repeatMin,event.buffed.repeatMax);
            break;
        }
        case EVENT_T_TARGET_AURA:
        {
            if (!m_creature->isInCombat() || !m_creature->getVictim())
                return false;

            SpellAuraHolder* holder = m_creature->getVictim()->GetSpellAuraHolder(event.buffed.spellId);
            if(!holder || holder->GetStackAmount() < event.buffed.amount)
                return false;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.buffed.repeatMin,event.buffed.repeatMax);
            break;
        }
        case EVENT_T_MISSING_AURA:
        {
            SpellAuraHolder* holder = m_creature->GetSpellAuraHolder(event.buffed.spellId);
            if (holder && holder->GetStackAmount() >= event.buffed.amount)
                return false;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.buffed.repeatMin,event.buffed.repeatMax);
            break;
        }
        case EVENT_T_TARGET_MISSING_AURA:
        {
            if (!m_creature->isInCombat() || !m_creature->getVictim())
                return false;

            SpellAuraHolder* holder = m_creature->getVictim()->GetSpellAuraHolder(event.buffed.spellId);
            if (holder && holder->GetStackAmount() >= event.buffed.amount)
                return false;

            //Repeat Timers
            pHolder.UpdateRepeatTimer(m_creature,event.buffed.repeatMin,event.buffed.repeatMax);
            break;
        }
        default:
            sLog.outErrorDb("CreatureEventAI: Creature %u using Event %u has invalid Event Type(%u), missing from ProcessEvent() Switch.", m_creature->GetEntry(), pHolder.Event.event_id, pHolder.Event.event_type);
            break;
    }

    //Disable non-repeatable events
    if (!(pHolder.Event.event_flags & EFLAG_REPEATABLE))
        pHolder.Enabled = false;

    //Store random here so that all random actions match up
    uint32 rnd = rand();

    //Return if chance for event is not met
    if (pHolder.Event.event_chance <= rnd % 100)
        return false;

    //Process actions, normal case
    if (!(pHolder.Event.event_flags & EFLAG_RANDOM_ACTION))
    {
        for (uint32 j = 0; j < MAX_ACTIONS; ++j)
            ProcessAction(pHolder.Event.action[j], rnd, pHolder.Event.event_id, pActionInvoker);
    }
    //Process actions, random case
    else
    {
        // amount of real actions
        uint32 count = 0;
        for (uint32 j = 0; j < MAX_ACTIONS; j++)
            if (pHolder.Event.action[j].type != ACTION_T_NONE)
                ++count;

        if (count)
        {
            // select action number from found amount
            uint32 idx = urand(0,count-1);

            // find selected action, skipping not used
            uint32 j = 0;
            for (; ; ++j)
            {
                if (pHolder.Event.action[j].type != ACTION_T_NONE)
                {
                    if (!idx)
                        break;
                    --idx;
                }
            }

            ProcessAction(pHolder.Event.action[j], rnd, pHolder.Event.event_id, pActionInvoker);
        }
    }
    return true;
}

void CreatureEventAI::ProcessAction(CreatureEventAI_Action const& action, uint32 rnd, uint32 EventId, Unit* pActionInvoker)
{
    switch (action.type)
    {
        case ACTION_T_TEXT:
        {
            if (!action.text.TextId[0])
                return;

            int32 temp = 0;

            if (action.text.TextId[1] && action.text.TextId[2])
                temp = action.text.TextId[rand()%3];
            else if (action.text.TextId[1] && urand(0,1))
                temp = action.text.TextId[1];
            else
                temp = action.text.TextId[0];

            if (temp)
            {
                Unit* target = NULL;

                if (pActionInvoker)
                {
                    if (pActionInvoker->GetTypeId() == TYPEID_PLAYER)
                        target = pActionInvoker;
                    else if (Unit* owner = pActionInvoker->GetOwner())
                    {
                        if (owner->GetTypeId() == TYPEID_PLAYER)
                            target = owner;
                    }
                }
                else if ((target = m_creature->getVictim()))
                {
                    if (target->GetTypeId() != TYPEID_PLAYER)
                        if (Unit* owner = target->GetOwner())
                            if (owner->GetTypeId() == TYPEID_PLAYER)
                                target = owner;
                }

                DoScriptText(temp, m_creature, target);
            }
            break;
        }
        case ACTION_T_SET_FACTION:
        {
            if (action.set_faction.factionId)
                m_creature->SetFactionTemporary(action.set_faction.factionId, action.set_faction.factionFlags);
            else                                            // no id provided, assume reset and then use default
                m_creature->ClearTemporaryFaction();

            break;
        }
        case ACTION_T_MORPH_TO_ENTRY_OR_MODEL:
        {
            if (action.morph.creatureId || action.morph.modelId)
            {
                //set model based on entry from creature_template
                if (action.morph.creatureId)
                {
                    if (CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(action.morph.creatureId))
                    {
                        uint32 display_id = Creature::ChooseDisplayId(ci);
                        m_creature->SetDisplayId(display_id);
                    }
                }
                //if no param1, then use value from param2 (modelId)
                else
                    m_creature->SetDisplayId(action.morph.modelId);
            }
            else
                m_creature->DeMorph();
            break;
        }
        case ACTION_T_SOUND:
            m_creature->PlayDirectSound(action.sound.soundId);
            break;
        case ACTION_T_EMOTE:
            m_creature->HandleEmote(action.emote.emoteId);
            break;
        case ACTION_T_RANDOM_SOUND:
        {
            int32 temp = GetRandActionParam(rnd, action.random_sound.soundId1, action.random_sound.soundId2, action.random_sound.soundId3);
            if (temp >= 0)
                m_creature->PlayDirectSound(temp);
            break;
        }
        case ACTION_T_RANDOM_EMOTE:
        {
            int32 temp = GetRandActionParam(rnd, action.random_emote.emoteId1, action.random_emote.emoteId2, action.random_emote.emoteId3);
            if (temp >= 0)
                m_creature->HandleEmote(temp);
            break;
        }
        case ACTION_T_CAST:
        {
            Unit* target = GetTargetByType(action.cast.target, pActionInvoker);

            if (!target)
            {
                sLog.outDebug("CreatureEventAI: NULL target for ACTION_T_CAST creature entry %u casting spell id %u", m_creature->GetEntry(), action.cast.spellId);
                return;
            }

            CanCastResult castResult = DoCastSpellIfCan(target, action.cast.spellId, action.cast.castFlags);

            switch(castResult)
            {
                case CAST_FAIL_POWER:
                case CAST_FAIL_TOO_FAR:
                {
                    // Melee current victim if flag not set
                    if (!(action.cast.castFlags & CAST_NO_MELEE_IF_OOM))
                    {
                        switch(m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType())
                        {
                            case CHASE_MOTION_TYPE:
                            case FOLLOW_MOTION_TYPE:
                                m_AttackDistance = 0.0f;
                                m_AttackAngle = 0.0f;

                                m_creature->GetMotionMaster()->Clear(false);
                                m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim(), m_AttackDistance, m_AttackAngle);
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                }
                default:
                    break;
            }

            break;
        }
        case ACTION_T_SUMMON:
        {
            Unit* target = GetTargetByType(action.summon.target, pActionInvoker);

            Creature* pCreature = NULL;

            if (action.summon.duration)
                pCreature = m_creature->SummonCreature(action.summon.creatureId, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, action.summon.duration);
            else
                pCreature = m_creature->SummonCreature(action.summon.creatureId, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 0);

            if (!pCreature)
                sLog.outErrorDb( "CreatureEventAI: failed to spawn creature %u. Spawn event %d is on creature %d", action.summon.creatureId, EventId, m_creature->GetEntry());
            else if (action.summon.target != TARGET_T_SELF && target)
                pCreature->AI()->AttackStart(target);
            break;
        }
        case ACTION_T_THREAT_SINGLE_PCT:
            if (Unit* target = GetTargetByType(action.threat_single_pct.target, pActionInvoker))
                m_creature->getThreatManager().modifyThreatPercent(target, action.threat_single_pct.percent);
            break;
        case ACTION_T_THREAT_ALL_PCT:
        {
            ThreatList const& threatList = m_creature->getThreatManager().getThreatList();
            for (ThreatList::const_iterator i = threatList.begin(); i != threatList.end(); ++i)
                if(Unit* Temp = m_creature->GetMap()->GetUnit((*i)->getUnitGuid()))
                    m_creature->getThreatManager().modifyThreatPercent(Temp, action.threat_all_pct.percent);
            break;
        }
        case ACTION_T_QUEST_EVENT:
            if (Unit* target = GetTargetByType(action.quest_event.target, pActionInvoker))
                if (target->GetTypeId() == TYPEID_PLAYER)
                    ((Player*)target)->AreaExploredOrEventHappens(action.quest_event.questId);
            break;
        case ACTION_T_CAST_EVENT:
            if (Unit* target = GetTargetByType(action.cast_event.target, pActionInvoker))
                if (target->GetTypeId() == TYPEID_PLAYER)
                    ((Player*)target)->CastedCreatureOrGO(action.cast_event.creatureId, m_creature->GetObjectGuid(), action.cast_event.spellId);
            break;
        case ACTION_T_SET_UNIT_FIELD:
        {
            Unit* target = GetTargetByType(action.set_unit_field.target, pActionInvoker);

            // not allow modify important for integrity object fields
            if (action.set_unit_field.field < OBJECT_END || action.set_unit_field.field >= UNIT_END)
                return;

            if (target)
                target->SetUInt32Value(action.set_unit_field.field, action.set_unit_field.value);

            break;
        }
        case ACTION_T_SET_UNIT_FLAG:
            if (Unit* target = GetTargetByType(action.unit_flag.target, pActionInvoker))
                target->SetFlag(UNIT_FIELD_FLAGS, action.unit_flag.value);
            break;
        case ACTION_T_REMOVE_UNIT_FLAG:
            if (Unit* target = GetTargetByType(action.unit_flag.target, pActionInvoker))
                target->RemoveFlag(UNIT_FIELD_FLAGS, action.unit_flag.value);
            break;
        case ACTION_T_AUTO_ATTACK:
            m_MeleeEnabled = action.auto_attack.state != 0;
            break;
        case ACTION_T_COMBAT_MOVEMENT:
            // ignore no affect case
            if(m_CombatMovementEnabled==(action.combat_movement.state!=0))
                return;

            m_CombatMovementEnabled = action.combat_movement.state != 0;

            //Allow movement (create new targeted movement gen only if idle)
            if (m_CombatMovementEnabled)
            {
                if(action.combat_movement.melee && m_creature->isInCombat())
                    if(Unit* victim = m_creature->getVictim())
                        m_creature->SendMeleeAttackStart(victim);

                if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
                {
                    m_creature->GetMotionMaster()->Clear(false);
                    m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim(), m_AttackDistance, m_AttackAngle);
                }
            }
            else
            {
                if(action.combat_movement.melee && m_creature->isInCombat())
                    if(Unit* victim = m_creature->getVictim())
                        m_creature->SendMeleeAttackStop(victim);

                if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
                {
                    m_creature->GetMotionMaster()->Clear(false);
                    m_creature->GetMotionMaster()->MoveIdle();
                    m_creature->StopMoving();
                }
            }
            break;
        case ACTION_T_SET_PHASE:
            m_Phase = action.set_phase.phase;
            break;
        case ACTION_T_INC_PHASE:
        {
            int32 new_phase = int32(m_Phase)+action.set_inc_phase.step;
            if (new_phase < 0)
            {
                sLog.outErrorDb( "CreatureEventAI: Event %d decrease Phase under 0. CreatureEntry = %d", EventId, m_creature->GetEntry());
                m_Phase = 0;
            }
            else if (new_phase >= MAX_PHASE)
            {
                sLog.outErrorDb( "CreatureEventAI: Event %d incremented Phase above %u. Phase mask cannot be used with phases past %u. CreatureEntry = %d", EventId, MAX_PHASE-1, MAX_PHASE-1, m_creature->GetEntry());
                m_Phase = MAX_PHASE-1;
            }
            else
                m_Phase = new_phase;

            break;
        }
        case ACTION_T_EVADE:
            EnterEvadeMode();
            break;
        case ACTION_T_FLEE_FOR_ASSIST:
            m_creature->DoFleeToGetAssistance();
            break;
        case ACTION_T_QUEST_EVENT_ALL:
            if (pActionInvoker && pActionInvoker->GetTypeId() == TYPEID_PLAYER)
                ((Player*)pActionInvoker)->GroupEventHappens(action.quest_event_all.questId,m_creature);
            break;
        case ACTION_T_CAST_EVENT_ALL:
        {
            ThreatList const& threatList = m_creature->getThreatManager().getThreatList();
            for (ThreatList::const_iterator i = threatList.begin(); i != threatList.end(); ++i)
                if (Player* temp = m_creature->GetMap()->GetPlayer((*i)->getUnitGuid()))
                        temp->CastedCreatureOrGO(action.cast_event_all.creatureId, m_creature->GetObjectGuid(), action.cast_event_all.spellId);
            break;
        }
        case ACTION_T_REMOVEAURASFROMSPELL:
            if (Unit* target = GetTargetByType(action.remove_aura.target, pActionInvoker))
                target->RemoveAurasDueToSpell(action.remove_aura.spellId);
            break;
        case ACTION_T_RANGED_MOVEMENT:
            m_AttackDistance = (float)action.ranged_movement.distance;
            m_AttackAngle = action.ranged_movement.angle/180.0f*M_PI_F;

            if (m_CombatMovementEnabled)
            {
                if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
                {
                    //Drop current movement gen
                    m_creature->GetMotionMaster()->Clear(false);
                    m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim(), m_AttackDistance, m_AttackAngle);
                }
            }
            break;
        case ACTION_T_RANDOM_PHASE:
            m_Phase = GetRandActionParam(rnd, action.random_phase.phase1, action.random_phase.phase2, action.random_phase.phase3);
            break;
        case ACTION_T_RANDOM_PHASE_RANGE:
            if (action.random_phase_range.phaseMax > action.random_phase_range.phaseMin)
                m_Phase = action.random_phase_range.phaseMin + (rnd % (action.random_phase_range.phaseMax - action.random_phase_range.phaseMin));
            else
                sLog.outErrorDb( "CreatureEventAI: ACTION_T_RANDOM_PHASE_RANGE cannot have Param2 <= Param1. Divide by Zero. Event = %d. CreatureEntry = %d", EventId, m_creature->GetEntry());
            break;
        case ACTION_T_SUMMON_ID:
        {
            Unit* target = GetTargetByType(action.summon_id.target, pActionInvoker);

            CreatureEventAI_Summon_Map::const_iterator i = sEventAIMgr.GetCreatureEventAISummonMap().find(action.summon_id.spawnId);
            if (i == sEventAIMgr.GetCreatureEventAISummonMap().end())
            {
                sLog.outErrorDb( "CreatureEventAI: failed to spawn creature %u. Summon map index %u does not exist. EventID %d. CreatureID %d", action.summon_id.creatureId, action.summon_id.spawnId, EventId, m_creature->GetEntry());
                return;
            }

            Creature* pCreature = NULL;
            if ((*i).second.SpawnTimeSecs)
                pCreature = m_creature->SummonCreature(action.summon_id.creatureId, (*i).second.position_x, (*i).second.position_y, (*i).second.position_z, (*i).second.orientation, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, (*i).second.SpawnTimeSecs);
            else
                pCreature = m_creature->SummonCreature(action.summon_id.creatureId, (*i).second.position_x, (*i).second.position_y, (*i).second.position_z, (*i).second.orientation, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 0);

            if (!pCreature)
                sLog.outErrorDb( "CreatureEventAI: failed to spawn creature %u. EventId %d.Creature %d", action.summon_id.creatureId, EventId, m_creature->GetEntry());
            else if (action.summon_id.target != TARGET_T_SELF && target)
                pCreature->AI()->AttackStart(target);

            break;
        }
        case ACTION_T_KILLED_MONSTER:
            //first attempt player/group who tapped creature
            if (Player* pPlayer = m_creature->GetLootRecipient())
                pPlayer->RewardPlayerAndGroupAtEvent(action.killed_monster.creatureId, m_creature);
            else
            {
                //if not available, use pActionInvoker
                if (Unit* pTarget = GetTargetByType(action.killed_monster.target, pActionInvoker))
                    if (Player* pPlayer2 = pTarget->GetCharmerOrOwnerPlayerOrPlayerItself())
                        pPlayer2->RewardPlayerAndGroupAtEvent(action.killed_monster.creatureId, m_creature);
            }
            break;
        case ACTION_T_SET_INST_DATA:
        {
            InstanceData* pInst = m_creature->GetInstanceData();
            if (!pInst)
            {
                sLog.outErrorDb("CreatureEventAI: Event %d attempt to set instance data without instance script. Creature %d", EventId, m_creature->GetEntry());
                return;
            }

            pInst->SetData(action.set_inst_data.field, action.set_inst_data.value);
            break;
        }
        case ACTION_T_SET_INST_DATA64:
        {
            Unit* target = GetTargetByType(action.set_inst_data64.target, pActionInvoker);
            if (!target)
            {
                sLog.outErrorDb("CreatureEventAI: Event %d attempt to set instance data64 but Target == NULL. Creature %d", EventId, m_creature->GetEntry());
                return;
            }

            InstanceData* pInst = m_creature->GetInstanceData();
            if (!pInst)
            {
                sLog.outErrorDb("CreatureEventAI: Event %d attempt to set instance data64 without instance script. Creature %d", EventId, m_creature->GetEntry());
                return;
            }

            pInst->SetData64(action.set_inst_data64.field, target->GetObjectGuid().GetRawValue());
            break;
        }
        case ACTION_T_UPDATE_TEMPLATE:
            if (m_creature->GetEntry() == action.update_template.creatureId)
            {

                sLog.outErrorDb("CreatureEventAI: Event %d ACTION_T_UPDATE_TEMPLATE call with param1 == current entry. Creature %d", EventId, m_creature->GetEntry());
                return;
            }

            m_creature->UpdateEntry(action.update_template.creatureId, action.update_template.team ? HORDE : ALLIANCE);
            break;
        case ACTION_T_DIE:
            if (m_creature->isDead())
            {

                sLog.outErrorDb("CreatureEventAI: Event %d ACTION_T_DIE on dead creature. Creature %d", EventId, m_creature->GetEntry());
                return;
            }
            m_creature->DealDamage(m_creature, m_creature->GetMaxHealth(),NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            break;
        case ACTION_T_ZONE_COMBAT_PULSE:
        {
            m_creature->SetInCombatWithZone();
            break;
        }
        case ACTION_T_CALL_FOR_HELP:
        {
            m_creature->CallForHelp((float)action.call_for_help.radius);
            break;
        }
        case ACTION_T_SET_SHEATH:
        {
            m_creature->SetSheath(SheathState(action.set_sheath.sheath));
            break;
        }
        case ACTION_T_FORCE_DESPAWN:
        {
            m_creature->ForcedDespawn(action.forced_despawn.msDelay);
            break;
        }
        case ACTION_T_SET_INVINCIBILITY_HP_LEVEL:
        {
            if(action.invincibility_hp_level.is_percent)
                m_InvinceabilityHpLevel = m_creature->GetMaxHealth()*action.invincibility_hp_level.hp_level/100;
            else
                m_InvinceabilityHpLevel = action.invincibility_hp_level.hp_level;
            break;
        }
        case ACTION_T_MOUNT_TO_ENTRY_OR_MODEL:
        {
            if (action.mount.creatureId || action.mount.modelId)
            {
                // set model based on entry from creature_template
                if (action.mount.creatureId)
                {
                    if (CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(action.mount.creatureId))
                    {
                        uint32 display_id = Creature::ChooseDisplayId(cInfo);
                        m_creature->Mount(display_id);
                    }
                }
                //if no param1, then use value from param2 (modelId)
                else
                    m_creature->Mount(action.mount.modelId);
            }
            else
                m_creature->Unmount();

            break;
        }
    }
}

void CreatureEventAI::JustRespawned()
{
    Reset();

    if (m_bEmptyList)
        return;

    //Handle Spawned Events
    for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
        if (SpawnedEventConditionsCheck((*i).Event))
            ProcessEvent(*i);
}

void CreatureEventAI::Reset()
{
    m_EventUpdateTime = EVENT_UPDATE_TIME;
    m_EventDiff = 0;

    if (m_bEmptyList)
        return;

    //Reset all events to enabled
    for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
    {
        CreatureEventAI_Event const& event = (*i).Event;
        switch (event.event_type)
        {
            //Reset all out of combat timers
            case EVENT_T_TIMER_OOC:
            {
                if ((*i).UpdateRepeatTimer(m_creature,event.timer.initialMin,event.timer.initialMax))
                    (*i).Enabled = true;
                break;
            }
            //default:
            //TODO: enable below code line / verify this is correct to enable events previously disabled (ex. aggro yell), instead of enable this in void Aggro()
            //(*i).Enabled = true;
            //(*i).Time = 0;
            //break;
        }
    }
}

void CreatureEventAI::JustReachedHome()
{
    if (!m_bEmptyList)
    {
        for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
        {
            if ((*i).Event.event_type == EVENT_T_REACHED_HOME)
                ProcessEvent(*i);
        }
    }

    Reset();
}

void CreatureEventAI::EnterEvadeMode()
{
    m_creature->RemoveAllAuras();
    m_creature->DeleteThreatList();
    m_creature->CombatStop(true);

    if (m_creature->isAlive())
        m_creature->GetMotionMaster()->MoveTargetedHome();

    m_creature->SetLootRecipient(NULL);

    if (m_bEmptyList)
        return;

    //Handle Evade events
    for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
    {
        if ((*i).Event.event_type == EVENT_T_EVADE)
            ProcessEvent(*i);
    }
}

void CreatureEventAI::JustDied(Unit* killer)
{
    Reset();

    if (m_creature->IsGuard())
    {
        //Send Zone Under Attack message to the LocalDefense and WorldDefense Channels
        if (Player* pKiller = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
            m_creature->SendZoneUnderAttackMessage(pKiller);
    }

    if (m_bEmptyList)
        return;

    //Handle Evade events
    for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
    {
        if ((*i).Event.event_type == EVENT_T_DEATH)
            ProcessEvent(*i, killer);
    }

    // reset phase after any death state events
    m_Phase = 0;
}

void CreatureEventAI::KilledUnit(Unit* victim)
{
    if (m_bEmptyList || victim->GetTypeId() != TYPEID_PLAYER)
        return;

    for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
    {
        if ((*i).Event.event_type == EVENT_T_KILL)
            ProcessEvent(*i, victim);
    }
}

void CreatureEventAI::JustSummoned(Creature* pUnit)
{
    if (m_bEmptyList || !pUnit)
        return;

    for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
    {
        if ((*i).Event.event_type == EVENT_T_SUMMONED_UNIT)
            ProcessEvent(*i, pUnit);
    }
}

void CreatureEventAI::SummonedCreatureJustDied(Creature* pUnit)
{
    if (m_bEmptyList || !pUnit)
        return;

    for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
    {
        if ((*i).Event.event_type == EVENT_T_SUMMONED_JUST_DIED)
            ProcessEvent(*i, pUnit);
    }
}

void CreatureEventAI::SummonedCreatureDespawn(Creature* pUnit)
{
    if (m_bEmptyList || !pUnit)
        return;

    for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
    {
        if ((*i).Event.event_type == EVENT_T_SUMMONED_JUST_DESPAWN)
            ProcessEvent(*i, pUnit);
    }
}

void CreatureEventAI::EnterCombat(Unit *enemy)
{
    //Check for on combat start events
    if (!m_bEmptyList)
    {
        for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
        {
            CreatureEventAI_Event const& event = (*i).Event;
            switch (event.event_type)
            {
                case EVENT_T_AGGRO:
                    (*i).Enabled = true;
                    ProcessEvent(*i, enemy);
                    break;
                    //Reset all in combat timers
                case EVENT_T_TIMER:
                    if ((*i).UpdateRepeatTimer(m_creature,event.timer.initialMin,event.timer.initialMax))
                        (*i).Enabled = true;
                    break;
                    //All normal events need to be re-enabled and their time set to 0
                default:
                    (*i).Enabled = true;
                    (*i).Time = 0;
                    break;
            }
        }
    }

    m_EventUpdateTime = EVENT_UPDATE_TIME;
    m_EventDiff = 0;
}

void CreatureEventAI::AttackStart(Unit *who)
{
    if (!who)
        return;

    if (m_creature->Attack(who, m_MeleeEnabled))
    {
        m_creature->AddThreat(who);
        m_creature->SetInCombatWith(who);
        who->SetInCombatWith(m_creature);

        if (m_CombatMovementEnabled)
        {
            m_creature->GetMotionMaster()->MoveChase(who, m_AttackDistance, m_AttackAngle);
        }
        else
        {
            m_creature->GetMotionMaster()->MoveIdle();
            m_creature->StopMoving();
        }
    }
}

void CreatureEventAI::MoveInLineOfSight(Unit *who)
{
    if (!who)
        return;

    //Check for OOC LOS Event
    if (!m_bEmptyList && !m_creature->getVictim())
    {
        for (CreatureEventAIList::iterator itr = m_CreatureEventAIList.begin(); itr != m_CreatureEventAIList.end(); ++itr)
        {
            if ((*itr).Event.event_type == EVENT_T_OOC_LOS)
            {
                //can trigger if closer than fMaxAllowedRange
                float fMaxAllowedRange = (float)(*itr).Event.ooc_los.maxRange;

                //if range is ok and we are actually in LOS
                if (m_creature->IsWithinDistInMap(who, fMaxAllowedRange) && m_creature->IsWithinLOSInMap(who))
                {
                    //if friendly event&&who is not hostile OR hostile event&&who is hostile
                    if (((*itr).Event.ooc_los.noHostile && !m_creature->IsHostileTo(who)) ||
                        ((!(*itr).Event.ooc_los.noHostile) && m_creature->IsHostileTo(who)))
                        ProcessEvent(*itr, who);
                }
            }
        }
    }

    if (m_creature->IsCivilian() || m_creature->IsNeutralToAll())
        return;

    if (m_creature->CanInitiateAttack() && who->isTargetableForAttack() &&
        m_creature->IsHostileTo(who) && who->isInAccessablePlaceFor(m_creature))
    {
        if (!m_creature->CanFly() && m_creature->GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE)
            return;

        float attackRadius = m_creature->GetAttackDistance(who);
        if (m_creature->IsWithinDistInMap(who, attackRadius) && m_creature->IsWithinLOSInMap(who))
        {
            if (!m_creature->getVictim())
            {
                AttackStart(who);
                who->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
            }
            else if (m_creature->GetMap()->IsDungeon())
            {
                m_creature->AddThreat(who);
                who->SetInCombatWith(m_creature);
            }
        }
    }
}

void CreatureEventAI::SpellHit(Unit* pUnit, const SpellEntry* pSpell)
{

    if (m_bEmptyList)
        return;

    for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
        if ((*i).Event.event_type == EVENT_T_SPELLHIT)
            //If spell id matches (or no spell id) & if spell school matches (or no spell school)
            if (!(*i).Event.spell_hit.spellId || pSpell->Id == (*i).Event.spell_hit.spellId)
                if (pSpell->SchoolMask & (*i).Event.spell_hit.schoolMask)
                    ProcessEvent(*i, pUnit);
}

void CreatureEventAI::UpdateAI(const uint32 diff)
{
    //Check if we are in combat (also updates calls threat update code)
    bool Combat = m_creature->SelectHostileTarget() && m_creature->getVictim();

    if (!m_bEmptyList)
    {
        //Events are only updated once every EVENT_UPDATE_TIME ms to prevent lag with large amount of events
        if (m_EventUpdateTime < diff)
        {
            m_EventDiff += diff;

            //Check for time based events
            for (CreatureEventAIList::iterator i = m_CreatureEventAIList.begin(); i != m_CreatureEventAIList.end(); ++i)
            {
                //Decrement Timers
                if ((*i).Time)
                {
                    if ((*i).Time > m_EventDiff)
                    {
                        //Do not decrement timers if event cannot trigger in this phase
                        if (!((*i).Event.event_inverse_phase_mask & (1 << m_Phase)))
                            (*i).Time -= m_EventDiff;

                        //Skip processing of events that have time remaining
                        continue;
                    }
                    else (*i).Time = 0;
                }

                //Events that are updated every EVENT_UPDATE_TIME
                switch ((*i).Event.event_type)
                {
                    case EVENT_T_TIMER_OOC:
                        ProcessEvent(*i);
                        break;
                    case EVENT_T_TIMER:
                    case EVENT_T_MANA:
                    case EVENT_T_HP:
                    case EVENT_T_TARGET_HP:
                    case EVENT_T_TARGET_CASTING:
                    case EVENT_T_FRIENDLY_HP:
                    case EVENT_T_AURA:
                    case EVENT_T_TARGET_AURA:
                    case EVENT_T_MISSING_AURA:
                    case EVENT_T_TARGET_MISSING_AURA:
                        if (Combat)
                            ProcessEvent(*i);
                        break;
                    case EVENT_T_RANGE:
                        if (Combat)
                        {
                            if (m_creature->getVictim() && m_creature->IsInMap(m_creature->getVictim()))
                                if (m_creature->IsInRange(m_creature->getVictim(), (float)(*i).Event.range.minDist, (float)(*i).Event.range.maxDist))
                                    ProcessEvent(*i);
                        }
                        break;
                }
            }

            m_EventDiff = 0;
            m_EventUpdateTime = EVENT_UPDATE_TIME;
        }
        else
        {
            m_EventDiff += diff;
            m_EventUpdateTime -= diff;
        }
    }

    //Melee Auto-Attack
    if (Combat && m_MeleeEnabled)
        DoMeleeAttackIfReady();
}

bool CreatureEventAI::IsVisible(Unit *pl) const
{
    return m_creature->IsWithinDist(pl,sWorld.getConfig(CONFIG_FLOAT_SIGHT_MONSTER))
        && pl->isVisibleForOrDetect(m_creature,m_creature,true);
}

inline uint32 CreatureEventAI::GetRandActionParam(uint32 rnd, uint32 param1, uint32 param2, uint32 param3)
{
    switch (rnd % 3)
    {
        case 0: return param1;
        case 1: return param2;
        case 2: return param3;
    }
    return 0;
}

inline int32 CreatureEventAI::GetRandActionParam(uint32 rnd, int32 param1, int32 param2, int32 param3)
{
    switch (rnd % 3)
    {
        case 0: return param1;
        case 1: return param2;
        case 2: return param3;
    }
    return 0;
}

inline Unit* CreatureEventAI::GetTargetByType(uint32 Target, Unit* pActionInvoker)
{
    switch (Target)
    {
        case TARGET_T_SELF:
            return m_creature;
        case TARGET_T_HOSTILE:
            return m_creature->getVictim();
        case TARGET_T_HOSTILE_SECOND_AGGRO:
            return m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 1);
        case TARGET_T_HOSTILE_LAST_AGGRO:
            return m_creature->SelectAttackingTarget(ATTACKING_TARGET_BOTTOMAGGRO, 0);
        case TARGET_T_HOSTILE_RANDOM:
            return m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);
        case TARGET_T_HOSTILE_RANDOM_NOT_TOP:
            return m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1);
        case TARGET_T_ACTION_INVOKER:
            return pActionInvoker;
        default:
            return NULL;
    };
}

Unit* CreatureEventAI::DoSelectLowestHpFriendly(float range, uint32 MinHPDiff)
{
    Unit* pUnit = NULL;

    MaNGOS::MostHPMissingInRangeCheck u_check(m_creature, range, MinHPDiff);
    MaNGOS::UnitLastSearcher<MaNGOS::MostHPMissingInRangeCheck> searcher(pUnit, u_check);

    /*
    typedef TYPELIST_4(GameObject, Creature*except pets*, DynamicObject, Corpse*Bones*) AllGridObjectTypes;
    This means that if we only search grid then we cannot possibly return pets or players so this is safe
    */
    Cell::VisitGridObjects(m_creature, searcher, range);
    return pUnit;
}

void CreatureEventAI::DoFindFriendlyCC(std::list<Creature*>& _list, float range)
{
    MaNGOS::FriendlyCCedInRangeCheck u_check(m_creature, range);
    MaNGOS::CreatureListSearcher<MaNGOS::FriendlyCCedInRangeCheck> searcher(_list, u_check);
    Cell::VisitGridObjects(m_creature, searcher, range);
}

void CreatureEventAI::DoFindFriendlyMissingBuff(std::list<Creature*>& _list, float range, uint32 spellid)
{
    MaNGOS::FriendlyMissingBuffInRangeCheck u_check(m_creature, range, spellid);
    MaNGOS::CreatureListSearcher<MaNGOS::FriendlyMissingBuffInRangeCheck> searcher(_list, u_check);
    Cell::VisitGridObjects(m_creature,searcher, range);
}

//*********************************
//*** Functions used globally ***

void CreatureEventAI::DoScriptText(int32 textEntry, WorldObject* pSource, Unit* target)
{
    if (!pSource)
    {
        sLog.outErrorDb("CreatureEventAI: DoScriptText entry %i, invalid Source pointer.",textEntry);
        return;
    }

    if (textEntry >= 0)
    {
        sLog.outErrorDb("CreatureEventAI: DoScriptText with source entry %u (TypeId=%u, guid=%u) attempts to process text entry %i, but text entry must be negative.",pSource->GetEntry(),pSource->GetTypeId(),pSource->GetGUIDLow(),textEntry);
        return;
    }

    CreatureEventAI_TextMap::const_iterator i = sEventAIMgr.GetCreatureEventAITextMap().find(textEntry);

    if (i == sEventAIMgr.GetCreatureEventAITextMap().end())
    {
        sLog.outErrorDb("CreatureEventAI: DoScriptText with source entry %u (TypeId=%u, guid=%u) could not find text entry %i.",pSource->GetEntry(),pSource->GetTypeId(),pSource->GetGUIDLow(),textEntry);
        return;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "CreatureEventAI: DoScriptText: text entry=%i, Sound=%u, Type=%u, Language=%u, Emote=%u",textEntry,(*i).second.SoundId,(*i).second.Type,(*i).second.Language,(*i).second.Emote);

    if((*i).second.SoundId)
    {
        if (GetSoundEntriesStore()->LookupEntry((*i).second.SoundId))
            pSource->PlayDirectSound((*i).second.SoundId);
        else
            sLog.outErrorDb("CreatureEventAI: DoScriptText entry %i tried to process invalid sound id %u.",textEntry,(*i).second.SoundId);
    }

    if((*i).second.Emote)
    {
        if (pSource->GetTypeId() == TYPEID_UNIT || pSource->GetTypeId() == TYPEID_PLAYER)
        {
            ((Unit*)pSource)->HandleEmote((*i).second.Emote);
        }
        else
            sLog.outErrorDb("CreatureEventAI: DoScriptText entry %i tried to process emote for invalid TypeId (%u).",textEntry,pSource->GetTypeId());
    }

    switch((*i).second.Type)
    {
        case CHAT_TYPE_SAY:
            pSource->MonsterSay(textEntry, (*i).second.Language, target);
            break;
        case CHAT_TYPE_YELL:
            pSource->MonsterYell(textEntry, (*i).second.Language, target);
            break;
        case CHAT_TYPE_TEXT_EMOTE:
            pSource->MonsterTextEmote(textEntry, target);
            break;
        case CHAT_TYPE_BOSS_EMOTE:
            pSource->MonsterTextEmote(textEntry, target, true);
            break;
        case CHAT_TYPE_WHISPER:
        {
            if (target && target->GetTypeId() == TYPEID_PLAYER)
                pSource->MonsterWhisper(textEntry, target);
            else sLog.outErrorDb("CreatureEventAI: DoScriptText entry %i cannot whisper without target unit (TYPEID_PLAYER).", textEntry);
        }break;
        case CHAT_TYPE_BOSS_WHISPER:
        {
            if (target && target->GetTypeId() == TYPEID_PLAYER)
                pSource->MonsterWhisper(textEntry, target, true);
            else sLog.outErrorDb("CreatureEventAI: DoScriptText entry %i cannot whisper without target unit (TYPEID_PLAYER).", textEntry);
        }break;
        case CHAT_TYPE_ZONE_YELL:
            pSource->MonsterYellToZone(textEntry, (*i).second.Language, target);
            break;
    }
}

bool CreatureEventAI::CanCast(Unit* Target, SpellEntry const *Spell, bool Triggered)
{
    //No target so we can't cast
    if (!Target || !Spell)
        return false;

    //Silenced so we can't cast
    if (!Triggered && (m_creature->hasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL) ||
        m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED)))
        return false;

    //Check for power
    if (!Triggered && m_creature->GetPower((Powers)Spell->powerType) < Spell::CalculatePowerCost(Spell, m_creature))
        return false;

    SpellRangeEntry const *TempRange = NULL;

    TempRange = GetSpellRangeStore()->LookupEntry(Spell->rangeIndex);

    //Spell has invalid range store so we can't use it
    if (!TempRange)
        return false;

    //Unit is out of range of this spell
    if (!m_creature->IsInRange(Target,TempRange->minRange,TempRange->maxRange))
        return false;

    return true;
}

void CreatureEventAI::ReceiveEmote(Player* pPlayer, uint32 text_emote)
{
    if (m_bEmptyList)
        return;

    for (CreatureEventAIList::iterator itr = m_CreatureEventAIList.begin(); itr != m_CreatureEventAIList.end(); ++itr)
    {
        if ((*itr).Event.event_type == EVENT_T_RECEIVE_EMOTE)
        {
            if ((*itr).Event.receive_emote.emoteId != text_emote)
                return;

            PlayerCondition pcon(0, (*itr).Event.receive_emote.condition,(*itr).Event.receive_emote.conditionValue1,(*itr).Event.receive_emote.conditionValue2);
            if (pcon.Meets(pPlayer))
            {
                DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "CreatureEventAI: ReceiveEmote CreatureEventAI: Condition ok, processing");
                ProcessEvent(*itr, pPlayer);
            }
        }
    }
}

void CreatureEventAI::DamageTaken( Unit* /*done_by*/, uint32& damage )
{
    if(m_InvinceabilityHpLevel > 0 && m_creature->GetHealth() < m_InvinceabilityHpLevel+damage)
    {
        if(m_creature->GetHealth() <= m_InvinceabilityHpLevel)
            damage = 0;
        else
            damage = m_creature->GetHealth() - m_InvinceabilityHpLevel;
    }
}

bool CreatureEventAI::SpawnedEventConditionsCheck(CreatureEventAI_Event const& event)
{
    if(event.event_type != EVENT_T_SPAWNED)
        return false;

    switch (event.spawned.condition)
    {
        case SPAWNED_EVENT_ALWAY:
            // always
            return true;
        case SPAWNED_EVENT_MAP:
            // map ID check
            return m_creature->GetMapId() == event.spawned.conditionValue1;
        case SPAWNED_EVENT_ZONE:
        {
            // zone ID check
            uint32 zone, area;
            m_creature->GetZoneAndAreaId(zone,area);
            return zone == event.spawned.conditionValue1 || area == event.spawned.conditionValue1;
        }
        default:
            break;
    }

    return false;
}
