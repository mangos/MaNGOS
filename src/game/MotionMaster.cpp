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

#include "MotionMaster.h"
#include "CreatureAISelector.h"
#include "Creature.h"
#include "Traveller.h"

#include "ConfusedMovementGenerator.h"
#include "FleeingMovementGenerator.h"
#include "HomeMovementGenerator.h"
#include "IdleMovementGenerator.h"
#include "PointMovementGenerator.h"
#include "TargetedMovementGenerator.h"
#include "WaypointMovementGenerator.h"
#include "RandomMovementGenerator.h"

#include <cassert>

inline bool isStatic(MovementGenerator *mv)
{
    return (mv == &si_idleMovement);
}

void
MotionMaster::Initialize()
{
    // stop current move
    if (!i_owner->IsStopped())
        i_owner->StopMoving();

    // clear ALL movement generators (including default)
    Clear(false,true);

    // set new default movement generator
    if (i_owner->GetTypeId() == TYPEID_UNIT && !i_owner->hasUnitState(UNIT_STAT_CONTROLLED))
    {
        MovementGenerator* movement = FactorySelector::selectMovementGenerator((Creature*)i_owner);
        push(movement == NULL ? &si_idleMovement : movement);
        top()->Initialize(*i_owner);
    }
    else
        push(&si_idleMovement);
}

MotionMaster::~MotionMaster()
{
    // clear ALL movement generators (including default)
    DirectClean(false,true);
}

void
MotionMaster::UpdateMotion(uint32 diff)
{
    if( i_owner->hasUnitState(UNIT_STAT_CAN_NOT_MOVE) )
        return;
    MANGOS_ASSERT( !empty() );
    m_cleanFlag |= MMCF_UPDATE;
    if (!top()->Update(*i_owner, diff))
    {
        m_cleanFlag &= ~MMCF_UPDATE;
        MovementExpired();
    }
    else
        m_cleanFlag &= ~MMCF_UPDATE;

    if (m_expList)
    {
        for (size_t i = 0; i < m_expList->size(); ++i)
        {
            MovementGenerator* mg = (*m_expList)[i];
            if (!isStatic(mg))
                delete mg;
        }

        delete m_expList;
        m_expList = NULL;

        if (empty())
            Initialize();

        if (m_cleanFlag & MMCF_RESET)
        {
            top()->Reset(*i_owner);
            m_cleanFlag &= ~MMCF_RESET;
        }
    }
}

void
MotionMaster::DirectClean(bool reset, bool all)
{
    while( all ? !empty() : size() > 1 )
    {
        MovementGenerator *curr = top();
        pop();
        curr->Finalize(*i_owner);
        if (!isStatic( curr ))
            delete curr;
    }

    if (!all && reset)
    {
        MANGOS_ASSERT( !empty() );
        top()->Reset(*i_owner);
    }
}

void
MotionMaster::DelayedClean(bool reset, bool all)
{
    if(reset)
        m_cleanFlag |= MMCF_RESET;
    else
        m_cleanFlag &= ~MMCF_RESET;

    if (empty() || !all && size() == 1)
        return;

    if (!m_expList)
        m_expList = new ExpireList();

    while( all ? !empty() : size() > 1 )
    {
        MovementGenerator *curr = top();
        pop();
        curr->Finalize(*i_owner);
        if (!isStatic( curr ))
            m_expList->push_back(curr);
    }
}

void
MotionMaster::DirectExpire(bool reset)
{
    if (empty() || size() == 1)
        return;

    MovementGenerator *curr = top();
    pop();

    // also drop stored under top() targeted motions
    while (!empty() && (top()->GetMovementGeneratorType() == CHASE_MOTION_TYPE || top()->GetMovementGeneratorType() == FOLLOW_MOTION_TYPE))
    {
        MovementGenerator *temp = top();
        pop();
        temp ->Finalize(*i_owner);
        delete temp;
    }

    // it can add another motions instead
    curr->Finalize(*i_owner);

    if (!isStatic(curr))
        delete curr;

    if (empty())
        Initialize();

    if (reset)
        top()->Reset(*i_owner);
}

void
MotionMaster::DelayedExpire(bool reset)
{
    if (reset)
        m_cleanFlag |= MMCF_RESET;
    else
        m_cleanFlag &= ~MMCF_RESET;

    if (empty() || size() == 1)
        return;

    MovementGenerator *curr = top();
    pop();

    if (!m_expList)
        m_expList = new ExpireList();

    // also drop stored under top() targeted motions
    while (!empty() && (top()->GetMovementGeneratorType() == CHASE_MOTION_TYPE || top()->GetMovementGeneratorType() == FOLLOW_MOTION_TYPE))
    {
        MovementGenerator *temp = top();
        pop();
        temp ->Finalize(*i_owner);
        m_expList->push_back(temp );
    }

    curr->Finalize(*i_owner);

    if (!isStatic(curr))
        m_expList->push_back(curr);
}

void MotionMaster::MoveIdle()
{
    if (empty() || !isStatic(top()))
        push(&si_idleMovement);
}

void MotionMaster::MoveRandom()
{
    if (i_owner->GetTypeId() == TYPEID_PLAYER)
    {
        sLog.outError("%s attempt to move random.", i_owner->GetObjectGuid().GetString().c_str());
    }
    else
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s move random.", i_owner->GetObjectGuid().GetString().c_str());
        Mutate(new RandomMovementGenerator<Creature>(*i_owner));
    }
}

void
MotionMaster::MoveTargetedHome()
{
    if(i_owner->hasUnitState(UNIT_STAT_LOST_CONTROL))
        return;

    Clear(false);

    if(i_owner->GetTypeId()==TYPEID_UNIT && !((Creature*)i_owner)->GetCharmerOrOwnerGUID())
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s targeted home", i_owner->GetObjectGuid().GetString().c_str());
        Mutate(new HomeMovementGenerator<Creature>());
    }
    else if(i_owner->GetTypeId()==TYPEID_UNIT && ((Creature*)i_owner)->GetCharmerOrOwnerGUID())
    {
        if (Unit *target = ((Creature*)i_owner)->GetCharmerOrOwner())
        {
            DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s follow to %s", i_owner->GetObjectGuid().GetString().c_str(), target->GetObjectGuid().GetString().c_str());
            Mutate(new FollowMovementGenerator<Creature>(*target,PET_FOLLOW_DIST,PET_FOLLOW_ANGLE));
        }
        else
        {
            DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s attempt but fail to follow owner", i_owner->GetObjectGuid().GetString().c_str());
        }
    }
    else
        sLog.outError("%s attempt targeted home", i_owner->GetObjectGuid().GetString().c_str());
}

void
MotionMaster::MoveConfused()
{
    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s move confused", i_owner->GetObjectGuid().GetString().c_str());

    if(i_owner->GetTypeId()==TYPEID_PLAYER)
        Mutate(new ConfusedMovementGenerator<Player>());
    else
        Mutate(new ConfusedMovementGenerator<Creature>());
}

void
MotionMaster::MoveChase(Unit* target, float dist, float angle)
{
    // ignore movement request if target not exist
    if(!target)
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s chase to %s", i_owner->GetObjectGuid().GetString().c_str(), target->GetObjectGuid().GetString().c_str());

    if(i_owner->GetTypeId()==TYPEID_PLAYER)
        Mutate(new ChaseMovementGenerator<Player>(*target,dist,angle));
    else
        Mutate(new ChaseMovementGenerator<Creature>(*target,dist,angle));
}

void
MotionMaster::MoveFollow(Unit* target, float dist, float angle)
{
    if(i_owner->hasUnitState(UNIT_STAT_LOST_CONTROL))
        return;

    Clear();

    // ignore movement request if target not exist
    if(!target)
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s follow to %s", i_owner->GetObjectGuid().GetString().c_str(), target->GetObjectGuid().GetString().c_str());

    if(i_owner->GetTypeId()==TYPEID_PLAYER)
        Mutate(new FollowMovementGenerator<Player>(*target,dist,angle));
    else
        Mutate(new FollowMovementGenerator<Creature>(*target,dist,angle));
}

void
MotionMaster::MovePoint(uint32 id, float x, float y, float z)
{
    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s targeted point (Id: %u X: %f Y: %f Z: %f)", i_owner->GetObjectGuid().GetString().c_str(), id, x, y, z );

    if(i_owner->GetTypeId()==TYPEID_PLAYER)
        Mutate(new PointMovementGenerator<Player>(id,x,y,z));
    else
        Mutate(new PointMovementGenerator<Creature>(id,x,y,z));
}

void
MotionMaster::MoveSeekAssistance(float x, float y, float z)
{
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        sLog.outError("%s attempt to seek assistance",i_owner->GetObjectGuid().GetString().c_str());
    }
    else
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s seek assistance (X: %f Y: %f Z: %f)",
            i_owner->GetObjectGuid().GetString().c_str(), x, y, z );
        Mutate(new AssistanceMovementGenerator(x,y,z));
    }
}

void
MotionMaster::MoveSeekAssistanceDistract(uint32 time)
{
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        sLog.outError("%s attempt to call distract after assistance",i_owner->GetObjectGuid().GetString().c_str());
    }
    else
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s is distracted after assistance call (Time: %u)",
            i_owner->GetObjectGuid().GetString().c_str(), time );
        Mutate(new AssistanceDistractMovementGenerator(time));
    }
}

void
MotionMaster::MoveFleeing(Unit* enemy, uint32 time)
{
    if(!enemy)
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s flee from %s", i_owner->GetObjectGuid().GetString().c_str(), enemy->GetObjectGuid().GetString().c_str());

    if(i_owner->GetTypeId()==TYPEID_PLAYER)
        Mutate(new FleeingMovementGenerator<Player>(enemy->GetGUID()));
    else
    {
        if (time)
            Mutate(new TimedFleeingMovementGenerator(enemy->GetGUID(), time));
        else
            Mutate(new FleeingMovementGenerator<Creature>(enemy->GetGUID()));
    }
}

void
MotionMaster::MoveTaxiFlight(uint32 path, uint32 pathnode)
{
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        if(path < sTaxiPathNodesByPath.size())
        {
            DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s taxi to (Path %u node %u)", i_owner->GetObjectGuid().GetString().c_str(), path, pathnode);
            FlightPathMovementGenerator* mgen = new FlightPathMovementGenerator(sTaxiPathNodesByPath[path],pathnode);
            Mutate(mgen);
        }
        else
        {
            sLog.outError("%s attempt taxi to (nonexistent Path %u node %u)",
                i_owner->GetObjectGuid().GetString().c_str(), path, pathnode );
        }
    }
    else
    {
        sLog.outError("%s attempt taxi to (Path %u node %u)",
            i_owner->GetObjectGuid().GetString().c_str(), path, pathnode );
    }
}

void
MotionMaster::MoveDistract(uint32 timer)
{
    DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "%s distracted (timer: %u)", i_owner->GetObjectGuid().GetString().c_str(), timer);
    DistractMovementGenerator* mgen = new DistractMovementGenerator(timer);
    Mutate(mgen);
}

void MotionMaster::Mutate(MovementGenerator *m)
{
    if (!empty())
    {
        switch(top()->GetMovementGeneratorType())
        {
            // HomeMovement is not that important, delete it if meanwhile a new comes
            case HOME_MOTION_TYPE:
            // DistractMovement interrupted by any other movement
            case DISTRACT_MOTION_TYPE:
                MovementExpired(false);
            default:
                break;
        }

        if (!empty())
            top()->Interrupt(*i_owner);
    }
    m->Initialize(*i_owner);
    push(m);
}

void MotionMaster::propagateSpeedChange()
{
    Impl::container_type::iterator it = Impl::c.begin();
    for ( ;it != end(); ++it)
    {
        (*it)->unitSpeedChanged();
    }
}

MovementGeneratorType MotionMaster::GetCurrentMovementGeneratorType() const
{
   if(empty())
        return IDLE_MOTION_TYPE;

   return top()->GetMovementGeneratorType();
}

bool MotionMaster::GetDestination(float &x, float &y, float &z)
{
   if(empty())
        return false;

   return top()->GetDestination(x,y,z);
}

void MotionMaster::UpdateFinalDistanceToTarget(float fDistance)
{
    if (!empty())
        top()->UpdateFinalDistance(fDistance);
}
