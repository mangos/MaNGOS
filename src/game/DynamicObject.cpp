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

#include "Common.h"
#include "UpdateMask.h"
#include "Opcodes.h"
#include "World.h"
#include "ObjectAccessor.h"
#include "Database/DatabaseEnv.h"
#include "GridNotifiers.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "SpellMgr.h"

DynamicObject::DynamicObject() : WorldObject()
{
    m_objectType |= TYPEMASK_DYNAMICOBJECT;
    m_objectTypeId = TYPEID_DYNAMICOBJECT;

    m_updateFlag = (UPDATEFLAG_HIGHGUID | UPDATEFLAG_HAS_POSITION | UPDATEFLAG_POSITION);

    m_valuesCount = DYNAMICOBJECT_END;
}

void DynamicObject::AddToWorld()
{
    ///- Register the dynamicObject for guid lookup
    if(!IsInWorld())
        GetMap()->GetObjectsStore().insert<DynamicObject>(GetGUID(), (DynamicObject*)this);

    Object::AddToWorld();
}

void DynamicObject::RemoveFromWorld()
{
    ///- Remove the dynamicObject from the accessor
    if(IsInWorld())
    {
        GetMap()->GetObjectsStore().erase<DynamicObject>(GetGUID(), (DynamicObject*)NULL);
        GetViewPoint().Event_RemovedFromWorld();
    }

    Object::RemoveFromWorld();
}

bool DynamicObject::Create( uint32 guidlow, Unit *caster, uint32 spellId, SpellEffectIndex effIndex, float x, float y, float z, int32 duration, float radius )
{
    WorldObject::_Create(guidlow, HIGHGUID_DYNAMICOBJECT, caster->GetPhaseMask());
    SetMap(caster->GetMap());
    Relocate(x, y, z, 0);

    if(!IsPositionValid())
    {
        sLog.outError("DynamicObject (spell %u eff %u) not created. Suggested coordinates isn't valid (X: %f Y: %f)",spellId,effIndex,GetPositionX(),GetPositionY());
        return false;
    }

    SetEntry(spellId);
    SetObjectScale(DEFAULT_OBJECT_SCALE);

    SetUInt64Value(DYNAMICOBJECT_CASTER, caster->GetGUID());

    /* Bytes field, so it's really 4 bit fields. These flags are unknown, but we do know that 0x00000001 is set for most.
       Farsight for example, does not have this flag, instead it has 0x80000002.
       Flags are set dynamically with some conditions, so one spell may have different flags set, depending on those conditions.
       The size of the visual may be controlled to some degree with these flags.

    uint32 bytes = 0x00000000;
    bytes |= 0x01;
    bytes |= 0x00 << 8;
    bytes |= 0x00 << 16;
    bytes |= 0x00 << 24;
    */
    SetUInt32Value(DYNAMICOBJECT_BYTES, 0x00000001);

    SetUInt32Value(DYNAMICOBJECT_SPELLID, spellId);
    SetFloatValue(DYNAMICOBJECT_RADIUS, radius);
    SetUInt32Value(DYNAMICOBJECT_CASTTIME, getMSTime());    // new 2.4.0

    m_aliveDuration = duration;
    m_radius = radius;
    m_effIndex = effIndex;
    m_spellId = spellId;

    return true;
}

Unit* DynamicObject::GetCaster() const
{
    // can be not found in some cases
    return ObjectAccessor::GetUnit(*this, GetCasterGUID());
}

void DynamicObject::Update(uint32 p_time)
{
    // caster can be not in world at time dynamic object update, but dynamic object not yet deleted in Unit destructor
    Unit* caster = GetCaster();
    if(!caster)
    {
        Delete();
        return;
    }

    bool deleteThis = false;

    if(m_aliveDuration > int32(p_time))
        m_aliveDuration -= p_time;
    else
        deleteThis = true;

    // have radius and work as persistent effect
    if(m_radius)
    {
        // TODO: make a timer and update this in larger intervals
        MaNGOS::DynamicObjectUpdater notifier(*this, caster);
        Cell::VisitAllObjects(this, notifier, m_radius);
    }

    if(deleteThis)
    {
        caster->RemoveDynObjectWithGUID(GetGUID());
        Delete();
    }
}

void DynamicObject::Delete()
{
    SendObjectDeSpawnAnim(GetGUID());
    AddObjectToRemoveList();
}

void DynamicObject::Delay(int32 delaytime)
{
    m_aliveDuration -= delaytime;
    for(AffectedSet::iterator iunit= m_affected.begin(); iunit != m_affected.end(); ++iunit)
        if (*iunit)
            (*iunit)->DelaySpellAuraHolder(m_spellId, delaytime);
}

bool DynamicObject::isVisibleForInState(Player const* u, WorldObject const* viewPoint, bool inVisibleList) const
{
    if(!IsInWorld() || !u->IsInWorld())
        return false;

    // always seen by owner
    if(GetCasterGUID()==u->GetGUID())
        return true;

    // normal case
    return IsWithinDistInMap(viewPoint, World::GetMaxVisibleDistanceForObject() + (inVisibleList ? World::GetVisibleObjectGreyDistance() : 0.0f), false);
}

bool DynamicObject::IsHostileTo( Unit const* unit ) const
{
    if (Unit* owner = GetCaster())
        return owner->IsHostileTo(unit);
    else
        return false;
}

bool DynamicObject::IsFriendlyTo( Unit const* unit ) const
{
    if (Unit* owner = GetCaster())
        return owner->IsFriendlyTo(unit);
    else
        return true;
}