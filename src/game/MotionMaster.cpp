/*
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include <cassert>

inline bool isStatic(MovementGenerator *mv)
{
    return (mv == &si_idleMovement);
}

void
MotionMaster::Initialize()
{
    // clear ALL movement generators (including default)
    while(!empty())
    {
        MovementGenerator *curr = top();
        pop();
        curr->Finalize(*i_owner);
        if( !isStatic( curr ) )
            delete curr;
    }

    // set new default movement generator
    if(i_owner->GetTypeId() == TYPEID_UNIT)
    {
        MovementGenerator* movement = FactorySelector::selectMovementGenerator((Creature*)i_owner);
        push(  movement == NULL ? &si_idleMovement : movement );
        top()->Initialize(*i_owner);
    }
    else
        push(&si_idleMovement);
}

MotionMaster::~MotionMaster()
{
    // clear ALL movement generators (including default)
    while(!empty())
    {
        MovementGenerator *curr = top();
        pop();
        curr->Finalize(*i_owner);
        if( !isStatic( curr ) )
            delete curr;
    }
}

void
MotionMaster::UpdateMotion(uint32 diff)
{
    if( i_owner->hasUnitState(UNIT_STAT_ROOT | UNIT_STAT_STUNNED) )
        return;
    assert( !empty() );
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
        for (int i = 0; i < m_expList->size(); ++i)
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
MotionMaster::DirectClean(bool reset)
{
    while( !empty() && size() > 1 )
    {
        MovementGenerator *curr = top();
        pop();
        curr->Finalize(*i_owner);
        if( !isStatic( curr ) )
            delete curr;
    }

    if (reset)
    {
        assert( !empty() );
        top()->Reset(*i_owner);
    }
}

void
MotionMaster::DelayedClean()
{
    if (empty() || size() == 1)
        return;

    if(!m_expList)
        m_expList = new ExpireList();

    while( !empty() && size() > 1 )
    {
        MovementGenerator *curr = top();
        pop();
        curr->Finalize(*i_owner);
        if( !isStatic( curr ) )
            m_expList->push_back(curr);
    }
}

void
MotionMaster::DirectExpire(bool reset)
{
    if( empty() || size() == 1 )
        return;

    MovementGenerator *curr = top();
    pop();

    // also drop stored under top() targeted motions
    while( !empty() && top()->GetMovementGeneratorType() == TARGETED_MOTION_TYPE )
    {
        MovementGenerator *temp = top();
        pop();
        temp ->Finalize(*i_owner);
        delete temp;
    }

    // it can add another motions instead
    curr->Finalize(*i_owner);

    if( !isStatic(curr) )
        delete curr;

    if( empty() )
        Initialize();

    if (reset) top()->Reset(*i_owner);
}

void
MotionMaster::DelayedExpire()
{
    if( empty() || size() == 1 )
        return;

    MovementGenerator *curr = top();
    pop();

    if(!m_expList)
        m_expList = new ExpireList();

    // also drop stored under top() targeted motions
    while( !empty() && top()->GetMovementGeneratorType() == TARGETED_MOTION_TYPE )
    {
        MovementGenerator *temp = top();
        pop();
        temp ->Finalize(*i_owner);
        m_expList->push_back(temp );
    }

    curr->Finalize(*i_owner);

    if( !isStatic(curr) )
        m_expList->push_back(curr);
}

void MotionMaster::MoveIdle()
{
    if( empty() || !isStatic( top() ) )
        push( &si_idleMovement );
}

void
MotionMaster::MoveTargetedHome()
{
    if(i_owner->hasUnitState(UNIT_STAT_FLEEING))
        return;

    Clear(false);

    if(i_owner->GetTypeId()==TYPEID_UNIT && !((Creature*)i_owner)->GetCharmerOrOwnerGUID())
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) targeted home", i_owner->GetEntry(), i_owner->GetGUIDLow());
        Mutate(new HomeMovementGenerator<Creature>());
    }
    else if(i_owner->GetTypeId()==TYPEID_UNIT && ((Creature*)i_owner)->GetCharmerOrOwnerGUID())
    {
        DEBUG_LOG("Pet or controlled creature (Entry: %u GUID: %u) targeting home",
            i_owner->GetEntry(), i_owner->GetGUIDLow() );
        Unit *target = ((Creature*)i_owner)->GetCharmerOrOwner();
        if(target)
        {
            i_owner->addUnitState(UNIT_STAT_FOLLOW);
            DEBUG_LOG("Following %s (GUID: %u)",
                target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
                target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : ((Creature*)target)->GetDBTableGUIDLow() );
            Mutate(new TargetedMovementGenerator<Creature>(*target,PET_FOLLOW_DIST,PET_FOLLOW_ANGLE));
        }
    }
    else
    {
        sLog.outError("Player (GUID: %u) attempt targeted home", i_owner->GetGUIDLow() );
    }
}

void
MotionMaster::MoveConfused()
{
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) move confused", i_owner->GetGUIDLow() );
        Mutate(new ConfusedMovementGenerator<Player>());
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) move confused",
            i_owner->GetEntry(), i_owner->GetGUIDLow() );
        Mutate(new ConfusedMovementGenerator<Creature>());
    }
}

void
MotionMaster::MoveChase(Unit* target, float dist, float angle)
{
    // ignore movement request if target not exist
    if(!target)
        return;

    i_owner->clearUnitState(UNIT_STAT_FOLLOW);
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) chase to %s (GUID: %u)",
            i_owner->GetGUIDLow(),
            target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            target->GetTypeId()==TYPEID_PLAYER ? i_owner->GetGUIDLow() : ((Creature*)i_owner)->GetDBTableGUIDLow() );
        Mutate(new TargetedMovementGenerator<Player>(*target,dist,angle));
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) chase to %s (GUID: %u)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(),
            target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : ((Creature*)target)->GetDBTableGUIDLow() );
        Mutate(new TargetedMovementGenerator<Creature>(*target,dist,angle));
    }
}

void
MotionMaster::MoveFollow(Unit* target, float dist, float angle)
{
    Clear();

    // ignore movement request if target not exist
    if(!target)
        return;

    i_owner->addUnitState(UNIT_STAT_FOLLOW);
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) follow to %s (GUID: %u)", i_owner->GetGUIDLow(),
            target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            target->GetTypeId()==TYPEID_PLAYER ? i_owner->GetGUIDLow() : ((Creature*)i_owner)->GetDBTableGUIDLow() );
        Mutate(new TargetedMovementGenerator<Player>(*target,dist,angle));
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) follow to %s (GUID: %u)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(),
            target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : ((Creature*)target)->GetDBTableGUIDLow() );
        Mutate(new TargetedMovementGenerator<Creature>(*target,dist,angle));
    }
}

void
MotionMaster::MovePoint(uint32 id, float x, float y, float z)
{
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) targeted point (Id: %u X: %f Y: %f Z: %f)", i_owner->GetGUIDLow(), id, x, y, z );
        Mutate(new PointMovementGenerator<Player>(id,x,y,z));
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) targeted point (ID: %u X: %f Y: %f Z: %f)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(), id, x, y, z );
        Mutate(new PointMovementGenerator<Creature>(id,x,y,z));
    }
}

void
MotionMaster::MoveSeekAssistance(float x, float y, float z)
{
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        sLog.outError("Player (GUID: %u) attempt to seek assistance",i_owner->GetGUIDLow());
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) seek assistance (X: %f Y: %f Z: %f)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(), x, y, z );
        Mutate(new AssistanceMovementGenerator(x,y,z));
    }
}

void
MotionMaster::MoveSeekAssistanceDistract(uint32 time)
{
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        sLog.outError("Player (GUID: %u) attempt to call distract after assistance",i_owner->GetGUIDLow());
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) is distracted after assistance call (Time: %u)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(), time );
        Mutate(new AssistanceDistractMovementGenerator(time));
    }
}

void
MotionMaster::MoveFleeing(Unit* enemy, uint32 time)
{
    if(!enemy)
        return;

    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) flee from %s (GUID: %u)", i_owner->GetGUIDLow(),
            enemy->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            enemy->GetTypeId()==TYPEID_PLAYER ? enemy->GetGUIDLow() : ((Creature*)enemy)->GetDBTableGUIDLow() );
        Mutate(new FleeingMovementGenerator<Player>(enemy->GetGUID()));
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) flee from %s (GUID: %u)%s",
            i_owner->GetEntry(), i_owner->GetGUIDLow(),
            enemy->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            enemy->GetTypeId()==TYPEID_PLAYER ? enemy->GetGUIDLow() : ((Creature*)enemy)->GetDBTableGUIDLow(),
            time ? " for a limited time" : "");
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
        DEBUG_LOG("Player (GUID: %u) taxi to (Path %u node %u)", i_owner->GetGUIDLow(), path, pathnode);
        FlightPathMovementGenerator* mgen = new FlightPathMovementGenerator(path,pathnode);
        Mutate(mgen);
    }
    else
    {
        sLog.outError("Creature (Entry: %u GUID: %u) attempt taxi to (Path %u node %u)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(), path, pathnode );
    }
}

void
MotionMaster::MoveDistract(uint32 timer)
{
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) distracted (timer: %u)", i_owner->GetGUIDLow(), timer);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) (timer: %u)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(), timer);
    }

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
