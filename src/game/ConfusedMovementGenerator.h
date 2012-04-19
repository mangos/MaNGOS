/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
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

#ifndef MANGOS_CONFUSEDMOVEMENTGENERATOR_H
#define MANGOS_CONFUSEDMOVEMENTGENERATOR_H

#include "MovementGenerator.h"
#include "Timer.h"

template<class T>
class MANGOS_DLL_SPEC ConfusedMovementGenerator
: public MovementGeneratorMedium< T, ConfusedMovementGenerator<T> >
{
    public:
        explicit ConfusedMovementGenerator() : i_nextMoveTime(0) {}

        void Initialize(T &);
        void Finalize(T &);
        void Interrupt(T &);
        void Reset(T &);
        bool Update(T &, const uint32 &);

        MovementGeneratorType GetMovementGeneratorType() const { return CONFUSED_MOTION_TYPE; }
    private:
        TimeTracker i_nextMoveTime;
        float i_x, i_y, i_z;
};
#endif
