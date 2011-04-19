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

#include "PointMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "TemporarySummon.h"
#include "DestinationHolderImp.h"
#include "World.h"

//----- Point Movement Generator
template<class T>
void PointMovementGenerator<T>::Initialize(T &unit)
{
    if (!unit.IsStopped())
        unit.StopMoving();

    unit.addUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);

    Traveller<T> traveller(unit);
    i_destinationHolder.SetDestination(traveller, i_x, i_y, i_z);

    if (unit.GetTypeId() == TYPEID_UNIT && ((Creature*)&unit)->CanFly())
        ((Creature&)unit).AddSplineFlag(SPLINEFLAG_FLYING);
}

template<class T>
void PointMovementGenerator<T>::Finalize(T &unit)
{
    unit.clearUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);

    if (i_destinationHolder.HasArrived())
        MovementInform(unit);
}

template<class T>
void PointMovementGenerator<T>::Interrupt(T &unit)
{
    unit.clearUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
}

template<class T>
void PointMovementGenerator<T>::Reset(T &unit)
{
    if (!unit.IsStopped())
        unit.StopMoving();

    unit.addUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
}

template<class T>
bool PointMovementGenerator<T>::Update(T &unit, const uint32 &diff)
{
    if(!&unit)
        return false;

    if(unit.hasUnitState(UNIT_STAT_CAN_NOT_MOVE))
    {
        unit.clearUnitState(UNIT_STAT_ROAMING_MOVE);
        return true;
    }

    unit.addUnitState(UNIT_STAT_ROAMING_MOVE);

    Traveller<T> traveller(unit);
    if (i_destinationHolder.UpdateTraveller(traveller, diff, false))
    {
        if (!IsActive(unit))                                // force stop processing (movement can move out active zone with cleanup movegens list)
            return true;                                    // not expire now, but already lost
    }

    if (i_destinationHolder.HasArrived())
        return false;

    return true;
}

template<>
void PointMovementGenerator<Player>::MovementInform(Player&)
{
}

template <>
void PointMovementGenerator<Creature>::MovementInform(Creature &unit)
{
    if (unit.AI())
        unit.AI()->MovementInform(POINT_MOTION_TYPE, id);

    if (unit.IsTemporarySummon())
    {
        TemporarySummon* pSummon = (TemporarySummon*)(&unit);
        if (pSummon->GetSummonerGuid().IsCreature())
            if(Creature* pSummoner = unit.GetMap()->GetCreature(pSummon->GetSummonerGuid()))
                if (pSummoner->AI())
                    pSummoner->AI()->SummonedMovementInform(&unit, POINT_MOTION_TYPE, id);
    }
}

template void PointMovementGenerator<Player>::Initialize(Player&);
template void PointMovementGenerator<Creature>::Initialize(Creature&);
template void PointMovementGenerator<Player>::Finalize(Player&);
template void PointMovementGenerator<Creature>::Finalize(Creature&);
template void PointMovementGenerator<Player>::Interrupt(Player&);
template void PointMovementGenerator<Creature>::Interrupt(Creature&);
template void PointMovementGenerator<Player>::Reset(Player&);
template void PointMovementGenerator<Creature>::Reset(Creature&);
template bool PointMovementGenerator<Player>::Update(Player &, const uint32 &diff);
template bool PointMovementGenerator<Creature>::Update(Creature&, const uint32 &diff);

void AssistanceMovementGenerator::Finalize(Unit &unit)
{
    unit.clearUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);

    ((Creature*)&unit)->SetNoCallAssistance(false);
    ((Creature*)&unit)->CallAssistance();
    if (unit.isAlive())
        unit.GetMotionMaster()->MoveSeekAssistanceDistract(sWorld.getConfig(CONFIG_UINT32_CREATURE_FAMILY_ASSISTANCE_DELAY));
}
