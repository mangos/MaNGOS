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

#ifndef MANGOS_TARGETEDMOVEMENTGENERATOR_H
#define MANGOS_TARGETEDMOVEMENTGENERATOR_H

#include "MovementGenerator.h"
#include "DestinationHolder.h"
#include "Traveller.h"
#include "FollowerReference.h"

class MANGOS_DLL_SPEC TargetedMovementGeneratorBase
{
    public:
        TargetedMovementGeneratorBase(Unit &target) { i_target.link(&target, this); }
        void stopFollowing() { }
    protected:
        FollowerReference i_target;
};

template<class T>
class MANGOS_DLL_SPEC TargetedMovementGeneratorMedium
: public MovementGeneratorMedium< T, TargetedMovementGeneratorMedium<T> >, public TargetedMovementGeneratorBase
{
    protected:
        TargetedMovementGeneratorMedium() : MovementGeneratorMedium< T, TargetedMovementGeneratorMedium<T> >(), TargetedMovementGeneratorBase() {}

        TargetedMovementGeneratorMedium(Unit &target)
            : TargetedMovementGeneratorBase(target), i_offset(0), i_angle(0), i_recalculateTravel(false) {}
        TargetedMovementGeneratorMedium(Unit &target, float offset, float angle)
            : TargetedMovementGeneratorBase(target), i_offset(offset), i_angle(angle), i_recalculateTravel(false) {}
        ~TargetedMovementGeneratorMedium() {}

    public:
        void Initialize(T &) {}
        void Finalize(T &) {}
        void Interrupt(T &) {}
        void Reset(T &) {}
        bool Update(T &, const uint32 &);

        Unit* GetTarget() const;

        bool GetDestination(float &x, float &y, float &z) const
        {
            if(!i_destinationHolder.HasDestination()) return false;
            i_destinationHolder.GetDestination(x,y,z);
            return true;
        }

        void unitSpeedChanged() { i_recalculateTravel=true; }
        void UpdateFinalDistance(float fDistance);

    protected:

        void _setTargetLocation(T &);

        float i_offset;
        float i_angle;
        DestinationHolder< Traveller<T> > i_destinationHolder;
        bool i_recalculateTravel;
};

template<class T>
class MANGOS_DLL_SPEC ChaseMovementGenerator : public TargetedMovementGeneratorMedium<T>
{
    public:
        ChaseMovementGenerator(Unit &target) : TargetedMovementGeneratorMedium(target) {}
        ChaseMovementGenerator(Unit &target, float offset, float angle)
            : TargetedMovementGeneratorMedium(target, offset, angle) {}
        ~ChaseMovementGenerator() {}

        MovementGeneratorType GetMovementGeneratorType() { return CHASE_MOTION_TYPE; }

        void Initialize(T &);
        void Finalize(T &);
        void Interrupt(T &);
        void Reset(T &);
        bool Update(T &, const uint32 &);
};

template<class T>
class MANGOS_DLL_SPEC FollowMovementGenerator : public TargetedMovementGeneratorMedium<T>
{
    public:
        FollowMovementGenerator(Unit &target) : TargetedMovementGeneratorMedium(target) {}
        FollowMovementGenerator(Unit &target, float offset, float angle)
            : TargetedMovementGeneratorMedium(target, offset, angle) {}
        ~FollowMovementGenerator() {}

        MovementGeneratorType GetMovementGeneratorType() { return FOLLOW_MOTION_TYPE; }

        void Initialize(T &);
        void Finalize(T &);
        void Interrupt(T &);
        void Reset(T &);
        bool Update(T &, const uint32 &);
};

#endif
