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

#include "ByteBuffer.h"
#include "TargetedMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "DestinationHolderImp.h"
#include "World.h"

#define SMALL_ALPHA 0.05f

#include <cmath>

//-----------------------------------------------//
template<class T, typename D>
void TargetedMovementGeneratorMedium<T,D>::_setTargetLocation(T &owner)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return;

    if (owner.hasUnitState(UNIT_STAT_NOT_MOVE))
        return;

    float x, y, z;

    // prevent redundant micro-movement for pets, other followers.
    if (i_offset && i_target->IsWithinDistInMap(&owner,2*i_offset))
    {
        if (i_destinationHolder.HasDestination())
            return;

        owner.GetPosition(x, y, z);
    }
    else if (!i_offset)
    {
        // to nearest contact position
        i_target->GetContactPoint( &owner, x, y, z );
    }
    else
    {
        // to at i_offset distance from target and i_angle from target facing
        i_target->GetClosePoint(x, y, z, owner.GetObjectBoundingRadius(), i_offset, i_angle);
    }

    /*
        We MUST not check the distance difference and avoid setting the new location for smaller distances.
        By that we risk having far too many GetContactPoint() calls freezing the whole system.
        In TargetedMovementGenerator<T>::Update() we check the distance to the target and at
        some range we calculate a new position. The calculation takes some processor cycles due to vmaps.
        If the distance to the target it too large to ignore,
        but the distance to the new contact point is short enough to be ignored,
        we will calculate a new contact point each update loop, but will never move to it.
        The system will freeze.
        ralf

        //We don't update Mob Movement, if the difference between New destination and last destination is < BothObjectSize
        float  bothObjectSize = i_target->GetObjectBoundingRadius() + owner.GetObjectBoundingRadius() + CONTACT_DISTANCE;
        if( i_destinationHolder.HasDestination() && i_destinationHolder.GetDestinationDiff(x,y,z) < bothObjectSize )
            return;
    */

    // Just a temp hack, GetContactPoint/GetClosePoint in above code use UpdateGroundPositionZ (in GetNearPoint)
    // and then has the wrong z to use when creature try follow unit in the air.
    if (owner.GetTypeId() == TYPEID_UNIT && ((Creature*)&owner)->canFly())
        z = i_target->GetPositionZ();

    Traveller<T> traveller(owner);
    i_destinationHolder.SetDestination(traveller, x, y, z);

    D::_addUnitStateMove(owner);
    if (owner.GetTypeId() == TYPEID_UNIT && ((Creature*)&owner)->canFly())
        ((Creature&)owner).AddSplineFlag(SPLINEFLAG_UNKNOWN7);
}

template<>
void TargetedMovementGeneratorMedium<Player,ChaseMovementGenerator<Player> >::UpdateFinalDistance(float /*fDistance*/)
{
    // nothing to do for Player
}

template<>
void TargetedMovementGeneratorMedium<Player,FollowMovementGenerator<Player> >::UpdateFinalDistance(float /*fDistance*/)
{
    // nothing to do for Player
}

template<>
void TargetedMovementGeneratorMedium<Creature,ChaseMovementGenerator<Creature> >::UpdateFinalDistance(float fDistance)
{
    i_offset = fDistance;
    i_recalculateTravel = true;
}

template<>
void TargetedMovementGeneratorMedium<Creature,FollowMovementGenerator<Creature> >::UpdateFinalDistance(float fDistance)
{
    i_offset = fDistance;
    i_recalculateTravel = true;
}

template<class T, typename D>
bool TargetedMovementGeneratorMedium<T,D>::Update(T &owner, const uint32 & time_diff)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return false;

    if (!owner.isAlive())
        return true;

    if (owner.hasUnitState(UNIT_STAT_NOT_MOVE))
    {
        D::_clearUnitStateMove(owner);
        return true;
    }

    // prevent movement while casting spells with cast time or channel time
    if (owner.IsNonMeleeSpellCasted(false, false,  true))
    {
        if (!owner.IsStopped())
            owner.StopMoving();
        return true;
    }

    // prevent crash after creature killed pet
    if (static_cast<D*>(this)->_lostTarget(owner))
    {
        D::_clearUnitStateMove(owner);
        return true;
    }

    Traveller<T> traveller(owner);

    if (!i_destinationHolder.HasDestination())
        _setTargetLocation(owner);
    if (owner.IsStopped() && !i_destinationHolder.HasArrived())
    {
        D::_addUnitStateMove(owner);
        if (owner.GetTypeId() == TYPEID_UNIT && ((Creature*)&owner)->canFly())
            ((Creature&)owner).AddSplineFlag(SPLINEFLAG_UNKNOWN7);

        i_destinationHolder.StartTravel(traveller);
        return true;
    }

    if (i_destinationHolder.UpdateTraveller(traveller, time_diff, false))
    {
        if (!IsActive(owner))                               // force stop processing (movement can move out active zone with cleanup movegens list)
            return true;                                    // not expire now, but already lost

        // put targeted movement generators on a higher priority
        if (owner.GetObjectBoundingRadius())
            i_destinationHolder.ResetUpdate(50);

        float dist = i_target->GetObjectBoundingRadius() + owner.GetObjectBoundingRadius() + sWorld.getConfig(CONFIG_FLOAT_RATE_TARGET_POS_RECALCULATION_RANGE);

        //More distance let have better performance, less distance let have more sensitive reaction at target move.

        // try to counter precision differences
        if (i_destinationHolder.GetDistance3dFromDestSq(*i_target.getTarget()) >= dist * dist)
        {
            owner.SetInFront(i_target.getTarget());         // Set new Angle For Map::
            _setTargetLocation(owner);                      //Calculate New Dest and Send data To Player
        }
        // Update the Angle of the target only for Map::, no need to send packet for player
        else if (!i_angle && !owner.HasInArc(0.01f, i_target.getTarget()))
            owner.SetInFront(i_target.getTarget());

        if ((owner.IsStopped() && !i_destinationHolder.HasArrived()) || i_recalculateTravel)
        {
            i_recalculateTravel = false;
            //Angle update will take place into owner.StopMoving()
            owner.SetInFront(i_target.getTarget());

            owner.StopMoving();
            static_cast<D*>(this)->_reachTarget(owner);
        }
    }
    return true;
}

//-----------------------------------------------//
template<class T>
void ChaseMovementGenerator<T>::_reachTarget(T &owner)
{
    if(owner.canReachWithAttack(this->i_target.getTarget()))
        owner.Attack(this->i_target.getTarget(),true);
}

template<>
void ChaseMovementGenerator<Player>::Initialize(Player &owner)
{
    owner.addUnitState(UNIT_STAT_CHASE|UNIT_STAT_CHASE_MOVE);
    _setTargetLocation(owner);
}

template<>
void ChaseMovementGenerator<Creature>::Initialize(Creature &owner)
{
    owner.addUnitState(UNIT_STAT_CHASE|UNIT_STAT_CHASE_MOVE);
    owner.RemoveSplineFlag(SPLINEFLAG_WALKMODE);

    if (((Creature*)&owner)->canFly())
        owner.AddSplineFlag(SPLINEFLAG_UNKNOWN7);

    _setTargetLocation(owner);
}

template<class T>
void ChaseMovementGenerator<T>::Finalize(T &owner)
{
    owner.clearUnitState(UNIT_STAT_CHASE|UNIT_STAT_CHASE_MOVE);
}

template<class T>
void ChaseMovementGenerator<T>::Interrupt(T &owner)
{
    owner.clearUnitState(UNIT_STAT_CHASE|UNIT_STAT_CHASE_MOVE);
}

template<class T>
void ChaseMovementGenerator<T>::Reset(T &owner)
{
    Initialize(owner);
}

//-----------------------------------------------//
template<>
void FollowMovementGenerator<Creature>::_updateWalkMode(Creature &u)
{
    if (i_target.isValid() && u.isPet())
        u.UpdateWalkMode(i_target.getTarget());
}

template<>
void FollowMovementGenerator<Player>::_updateWalkMode(Player &)
{
}

template<>
void FollowMovementGenerator<Player>::_updateSpeed(Player &/*u*/)
{
    // nothing to do for Player
}

template<>
void FollowMovementGenerator<Creature>::_updateSpeed(Creature &u)
{
    // pet only sync speed with owner
    if (!((Creature&)u).isPet() || !i_target.isValid() || i_target->GetGUID() != u.GetOwnerGUID())
        return;

    u.UpdateSpeed(MOVE_RUN,true);
    u.UpdateSpeed(MOVE_WALK,true);
    u.UpdateSpeed(MOVE_SWIM,true);
}

template<>
void FollowMovementGenerator<Player>::Initialize(Player &owner)
{
    owner.addUnitState(UNIT_STAT_FOLLOW|UNIT_STAT_FOLLOW_MOVE);
    _updateWalkMode(owner);
    _updateSpeed(owner);
    _setTargetLocation(owner);
}

template<>
void FollowMovementGenerator<Creature>::Initialize(Creature &owner)
{
    owner.addUnitState(UNIT_STAT_FOLLOW|UNIT_STAT_FOLLOW_MOVE);
    _updateWalkMode(owner);
    _updateSpeed(owner);

    if (((Creature*)&owner)->canFly())
        owner.AddSplineFlag(SPLINEFLAG_UNKNOWN7);

    _setTargetLocation(owner);
}

template<class T>
void FollowMovementGenerator<T>::Finalize(T &owner)
{
    owner.clearUnitState(UNIT_STAT_FOLLOW|UNIT_STAT_FOLLOW_MOVE);
    _updateWalkMode(owner);
    _updateSpeed(owner);
}

template<class T>
void FollowMovementGenerator<T>::Interrupt(T &owner)
{
    owner.clearUnitState(UNIT_STAT_FOLLOW|UNIT_STAT_FOLLOW_MOVE);
    _updateWalkMode(owner);
    _updateSpeed(owner);
}

template<class T>
void FollowMovementGenerator<T>::Reset(T &owner)
{
    Initialize(owner);
}

//-----------------------------------------------//
template void TargetedMovementGeneratorMedium<Player,ChaseMovementGenerator<Player> >::_setTargetLocation(Player &);
template void TargetedMovementGeneratorMedium<Player,FollowMovementGenerator<Player> >::_setTargetLocation(Player &);
template void TargetedMovementGeneratorMedium<Creature,ChaseMovementGenerator<Creature> >::_setTargetLocation(Creature &);
template void TargetedMovementGeneratorMedium<Creature,FollowMovementGenerator<Creature> >::_setTargetLocation(Creature &);
template bool TargetedMovementGeneratorMedium<Player,ChaseMovementGenerator<Player> >::Update(Player &, const uint32 &);
template bool TargetedMovementGeneratorMedium<Player,FollowMovementGenerator<Player> >::Update(Player &, const uint32 &);
template bool TargetedMovementGeneratorMedium<Creature,ChaseMovementGenerator<Creature> >::Update(Creature &, const uint32 &);
template bool TargetedMovementGeneratorMedium<Creature,FollowMovementGenerator<Creature> >::Update(Creature &, const uint32 &);

template void ChaseMovementGenerator<Player>::_reachTarget(Player &);
template void ChaseMovementGenerator<Creature>::_reachTarget(Creature &);
template void ChaseMovementGenerator<Player>::Finalize(Player &);
template void ChaseMovementGenerator<Creature>::Finalize(Creature &);
template void ChaseMovementGenerator<Player>::Interrupt(Player &);
template void ChaseMovementGenerator<Creature>::Interrupt(Creature &);
template void ChaseMovementGenerator<Player>::Reset(Player &);
template void ChaseMovementGenerator<Creature>::Reset(Creature &);

template void FollowMovementGenerator<Player>::Finalize(Player &);
template void FollowMovementGenerator<Creature>::Finalize(Creature &);
template void FollowMovementGenerator<Player>::Interrupt(Player &);
template void FollowMovementGenerator<Creature>::Interrupt(Creature &);
template void FollowMovementGenerator<Player>::Reset(Player &);
template void FollowMovementGenerator<Creature>::Reset(Creature &);
