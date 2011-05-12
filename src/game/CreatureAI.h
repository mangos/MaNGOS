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

#ifndef MANGOS_CREATUREAI_H
#define MANGOS_CREATUREAI_H

#include "Common.h"
#include "Platform/Define.h"
#include "Policies/Singleton.h"
#include "Dynamic/ObjectRegistry.h"
#include "Dynamic/FactoryHolder.h"
#include "ObjectGuid.h"

class WorldObject;
class GameObject;
class Unit;
class Creature;
class Player;
struct SpellEntry;

#define TIME_INTERVAL_LOOK   5000
#define VISIBILITY_RANGE    10000

enum CanCastResult
{
    CAST_OK                     = 0,
    CAST_FAIL_IS_CASTING        = 1,
    CAST_FAIL_OTHER             = 2,
    CAST_FAIL_TOO_FAR           = 3,
    CAST_FAIL_TOO_CLOSE         = 4,
    CAST_FAIL_POWER             = 5,
    CAST_FAIL_STATE             = 6,
    CAST_FAIL_TARGET_AURA       = 7
};

enum CastFlags
{
    CAST_INTERRUPT_PREVIOUS     = 0x01,                     //Interrupt any spell casting
    CAST_TRIGGERED              = 0x02,                     //Triggered (this makes spell cost zero mana and have no cast time)
    CAST_FORCE_CAST             = 0x04,                     //Forces cast even if creature is out of mana or out of range
    CAST_NO_MELEE_IF_OOM        = 0x08,                     //Prevents creature from entering melee if out of mana or out of range
    CAST_FORCE_TARGET_SELF      = 0x10,                     //Forces the target to cast this spell on itself
    CAST_AURA_NOT_PRESENT       = 0x20,                     //Only casts the spell if the target does not have an aura from the spell
};

class MANGOS_DLL_SPEC CreatureAI
{
    public:
        explicit CreatureAI(Creature* creature) : m_creature(creature) {}

        virtual ~CreatureAI();

        ///== Reactions At =================================

        // Called if IsVisible(Unit *who) is true at each *who move, reaction at visibility zone enter
        virtual void MoveInLineOfSight(Unit *) {}

        // Called for reaction at enter to combat if not in combat yet (enemy can be NULL)
        virtual void EnterCombat(Unit* /*enemy*/) {}

        // Called for reaction at stopping attack at no attackers or targets
        virtual void EnterEvadeMode() {}

        // Called at reaching home after evade
        virtual void JustReachedHome() {}

        // Called at any heal cast/item used (call non implemented)
        virtual void HealBy(Unit * /*healer*/, uint32 /*amount_healed*/) {}

        // Helper functions for cast spell
        virtual CanCastResult CanCastSpell(Unit* pTarget, const SpellEntry *pSpell, bool isTriggered);

        // Called at any Damage to any victim (before damage apply)
        virtual void DamageDeal(Unit * /*done_to*/, uint32 & /*damage*/) {}

        // Called at any Damage from any attacker (before damage apply)
        // Note: it for recalculation damage or special reaction at damage
        // for attack reaction use AttackedBy called for not DOT damage in Unit::DealDamage also
        virtual void DamageTaken(Unit * /*done_by*/, uint32 & /*damage*/) {}

        // Called when the creature is killed
        virtual void JustDied(Unit *) {}

        // Called when the creature summon is killed
        virtual void SummonedCreatureJustDied(Creature* /*unit*/) {}

        // Called when the creature kills a unit
        virtual void KilledUnit(Unit *) {}

        // Called when owner of m_creature (if m_creature is PROTECTOR_PET) kills a unit
        virtual void OwnerKilledUnit(Unit *) {}

        // Called when the creature summon successfully other creature
        virtual void JustSummoned(Creature* ) {}

        // Called when the creature summon successfully a gameobject
        virtual void JustSummoned(GameObject* ) {}

        // Called when the creature summon despawn
        virtual void SummonedCreatureDespawn(Creature* /*unit*/) {}

        // Called when hit by a spell
        virtual void SpellHit(Unit*, const SpellEntry*) {}

        // Called when spell hits creature's target
        virtual void SpellHitTarget(Unit*, const SpellEntry*) {}

        // Called when the creature is target of hostile action: swing, hostile spell landed, fear/etc)
        virtual void AttackedBy(Unit* attacker);

        // Called when creature is spawned or respawned (for reseting variables)
        virtual void JustRespawned() {}

        // Called at waypoint reached or point movement finished
        virtual void MovementInform(uint32 /*MovementType*/, uint32 /*Data*/) {}

        // Called if a temporary summoned of m_creature reach a move point
        virtual void SummonedMovementInform(Creature* /*summoned*/, uint32 /*motion_type*/, uint32 /*point_id*/) {}

        // Called at text emote receive from player
        virtual void ReceiveEmote(Player* /*pPlayer*/, uint32 /*text_emote*/) {}

        ///== Triggered Actions Requested ==================

        // Called when creature attack expected (if creature can and no have current victim)
        // Note: for reaction at hostile action must be called AttackedBy function.
        virtual void AttackStart(Unit *) {}

        // Called at World update tick
        virtual void UpdateAI(const uint32 /*diff*/) {}

        ///== State checks =================================

        // Is unit visible for MoveInLineOfSight
        virtual bool IsVisible(Unit *) const { return false; }

        // called when the corpse of this creature gets removed
        virtual void CorpseRemoved(uint32 & /*respawnDelay*/) {}

        // Called when victim entered water and creature can not enter water
        virtual bool canReachByRangeAttack(Unit*) { return false; }

        ///== Helper functions =============================
        bool DoMeleeAttackIfReady();
        CanCastResult DoCastSpellIfCan(Unit* pTarget, uint32 uiSpell, uint32 uiCastFlags = 0, ObjectGuid uiOriginalCasterGUID = ObjectGuid());

        ///== Fields =======================================

        // Pointer to controlled by AI creature
        Creature* const m_creature;
};

struct SelectableAI : public FactoryHolder<CreatureAI>, public Permissible<Creature>
{

    SelectableAI(const char *id) : FactoryHolder<CreatureAI>(id) {}
};

template<class REAL_AI>
struct CreatureAIFactory : public SelectableAI
{
    CreatureAIFactory(const char *name) : SelectableAI(name) {}

    CreatureAI* Create(void *) const;

    int Permit(const Creature *c) const { return REAL_AI::Permissible(c); }
};

enum Permitions
{
    PERMIT_BASE_NO                 = -1,
    PERMIT_BASE_IDLE               = 1,
    PERMIT_BASE_REACTIVE           = 100,
    PERMIT_BASE_PROACTIVE          = 200,
    PERMIT_BASE_FACTION_SPECIFIC   = 400,
    PERMIT_BASE_SPECIAL            = 800
};

typedef FactoryHolder<CreatureAI> CreatureAICreator;
typedef FactoryHolder<CreatureAI>::FactoryHolderRegistry CreatureAIRegistry;
typedef FactoryHolder<CreatureAI>::FactoryHolderRepository CreatureAIRepository;
#endif
