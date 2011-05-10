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

#ifndef MANGOS_FLEEINGMOVEMENTGENERATOR_H
#define MANGOS_FLEEINGMOVEMENTGENERATOR_H

#include "MovementGenerator.h"
#include "DestinationHolder.h"
#include "Traveller.h"
#include "ObjectGuid.h"

template<class T>
class MANGOS_DLL_SPEC FleeingMovementGenerator
: public MovementGeneratorMedium< T, FleeingMovementGenerator<T> >
{
    public:
        FleeingMovementGenerator(ObjectGuid fright) : i_frightGuid(fright), i_nextCheckTime(0) {}

        void Initialize(T &);
        void Finalize(T &);
        void Interrupt(T &);
        void Reset(T &);
        bool Update(T &, const uint32 &);

        MovementGeneratorType GetMovementGeneratorType() const { return FLEEING_MOTION_TYPE; }

    private:
        void _setTargetLocation(T &owner);
        bool _getPoint(T &owner, float &x, float &y, float &z);
        bool _setMoveData(T &owner);
        void _Init(T &);

        bool is_water_ok   :1;
        bool is_land_ok    :1;
        bool i_only_forward:1;

        float i_caster_x;
        float i_caster_y;
        float i_caster_z;
        float i_last_distance_from_caster;
        float i_to_distance_from_caster;
        float i_cur_angle;
        ObjectGuid i_frightGuid;
        TimeTracker i_nextCheckTime;

        DestinationHolder< Traveller<T> > i_destinationHolder;
};

class MANGOS_DLL_SPEC TimedFleeingMovementGenerator
: public FleeingMovementGenerator<Creature>
{
    public:
        TimedFleeingMovementGenerator(ObjectGuid fright, uint32 time) :
            FleeingMovementGenerator<Creature>(fright),
            i_totalFleeTime(time) {}

        MovementGeneratorType GetMovementGeneratorType() const { return TIMED_FLEEING_MOTION_TYPE; }
        bool Update(Unit &, const uint32 &);
        void Finalize(Unit &);

    private:
        TimeTracker i_totalFleeTime;
};

#endif
