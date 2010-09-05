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

#include "Totem.h"
#include "WorldPacket.h"
#include "Log.h"
#include "Group.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "CreatureAI.h"

Totem::Totem() : Creature(CREATURE_SUBTYPE_TOTEM)
{
    m_duration = 0;
    m_type = TOTEM_PASSIVE;
}

void Totem::Update( uint32 time )
{
    Unit *owner = GetOwner();
    if (!owner || !owner->isAlive() || !isAlive())
    {
        UnSummon();                                         // remove self
        return;
    }

    if (m_duration <= time)
    {
        UnSummon();                                         // remove self
        return;
    }
    else
        m_duration -= time;

    Creature::Update( time );
}

void Totem::Summon(Unit* owner)
{
    AIM_Initialize();
    owner->GetMap()->Add((Creature*)this);

    if (owner->GetTypeId() == TYPEID_UNIT && ((Creature*)owner)->AI())
        ((Creature*)owner)->AI()->JustSummoned((Creature*)this);

    // there are some totems, which exist just for their visual appeareance
    if (!GetSpell())
        return;

    switch(m_type)
    {
        case TOTEM_PASSIVE:
            CastSpell(this, GetSpell(), true);
            break;
        case TOTEM_STATUE:
            CastSpell(GetOwner(), GetSpell(), true);
            break;
        default: break;
    }
}

void Totem::UnSummon()
{
    CombatStop();
    RemoveAurasDueToSpell(GetSpell());

    if (Unit *owner = GetOwner())
    {
        owner->_RemoveTotem(this);
        owner->RemoveAurasDueToSpell(GetSpell());

        //remove aura all party members too
        if (owner->GetTypeId() == TYPEID_PLAYER)
        {
            ((Player*)owner)->SendAutoRepeatCancel(this);

            // Not only the player can summon the totem (scripted AI)
            if (Group *pGroup = ((Player*)owner)->GetGroup())
            {
                for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
                {
                    Player* Target = itr->getSource();
                    if(Target && pGroup->SameSubGroup((Player*)owner, Target))
                        Target->RemoveAurasDueToSpell(GetSpell());
                }
            }
        }

        if (owner->GetTypeId() == TYPEID_UNIT && ((Creature*)owner)->AI())
            ((Creature*)owner)->AI()->SummonedCreatureDespawn((Creature*)this);
    }

    AddObjectToRemoveList();
}

void Totem::SetOwner(uint64 guid)
{
    SetCreatorGUID(guid);
    SetOwnerGUID(guid);
    if (Unit *owner = GetOwner())
    {
        setFaction(owner->getFaction());
        SetLevel(owner->getLevel());
    }
}

Unit *Totem::GetOwner()
{
    uint64 ownerid = GetOwnerGUID();
    if(!ownerid)
        return NULL;
    return ObjectAccessor::GetUnit(*this, ownerid);
}

void Totem::SetTypeBySummonSpell(SpellEntry const * spellProto)
{
    // Get spell casted by totem
    SpellEntry const * totemSpell = sSpellStore.LookupEntry(GetSpell());
    if (totemSpell)
    {
        // If spell have cast time -> so its active totem
        if (GetSpellCastTime(totemSpell))
            m_type = TOTEM_ACTIVE;
    }
    if(spellProto->SpellIconID == 2056)
        m_type = TOTEM_STATUE;                              //Jewelery statue
}

bool Totem::IsImmunedToSpellEffect(SpellEntry const* spellInfo, SpellEffectIndex index) const
{
    // TODO: possibly all negative auras immune?
    switch(spellInfo->Effect[index])
    {
        case SPELL_EFFECT_ATTACK_ME:
            return true;
        default:
            break;
    }
    switch(spellInfo->EffectApplyAuraName[index])
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_MOD_FEAR:
        case SPELL_AURA_TRANSFORM:
        case SPELL_AURA_MOD_TAUNT:
            return true;
        default:
            break;
    }
    return Creature::IsImmunedToSpellEffect(spellInfo, index);
}
