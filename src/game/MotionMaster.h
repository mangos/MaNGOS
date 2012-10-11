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

#ifndef MANGOS_MOTIONMASTER_H
#define MANGOS_MOTIONMASTER_H

#include "Common.h"
#include <stack>
#include <vector>

class MovementGenerator;
class Unit;

// Creature Entry ID used for waypoints show, visible only for GMs
#define VISUAL_WAYPOINT 1

// values 0 ... MAX_DB_MOTION_TYPE-1 used in DB
enum MovementGeneratorType
{
    IDLE_MOTION_TYPE                = 0,                    // IdleMovementGenerator.h
    RANDOM_MOTION_TYPE              = 1,                    // RandomMovementGenerator.h
    WAYPOINT_MOTION_TYPE            = 2,                    // WaypointMovementGenerator.h
    MAX_DB_MOTION_TYPE              = 3,                    // *** this and below motion types can't be set in DB.

    CONFUSED_MOTION_TYPE            = 4,                    // ConfusedMovementGenerator.h
    CHASE_MOTION_TYPE               = 5,                    // TargetedMovementGenerator.h
    HOME_MOTION_TYPE                = 6,                    // HomeMovementGenerator.h
    FLIGHT_MOTION_TYPE              = 7,                    // WaypointMovementGenerator.h
    POINT_MOTION_TYPE               = 8,                    // PointMovementGenerator.h
    FLEEING_MOTION_TYPE             = 9,                    // FleeingMovementGenerator.h
    DISTRACT_MOTION_TYPE            = 10,                   // IdleMovementGenerator.h
    ASSISTANCE_MOTION_TYPE          = 11,                   // PointMovementGenerator.h (first part of flee for assistance)
    ASSISTANCE_DISTRACT_MOTION_TYPE = 12,                   // IdleMovementGenerator.h (second part of flee for assistance)
    TIMED_FLEEING_MOTION_TYPE       = 13,                   // FleeingMovementGenerator.h (alt.second part of flee for assistance)
    FOLLOW_MOTION_TYPE              = 14,                   // TargetedMovementGenerator.h
    EFFECT_MOTION_TYPE              = 15,
};

enum MMCleanFlag
{
    MMCF_NONE   = 0,
    MMCF_UPDATE = 1,                                        // Clear or Expire called from update
    MMCF_RESET  = 2                                         // Flag if need top()->Reset()
};

class MANGOS_DLL_SPEC MotionMaster : private std::stack<MovementGenerator*>
{
    private:
        typedef std::stack<MovementGenerator*> Impl;
        typedef std::vector<MovementGenerator*> ExpireList;
    public:

        explicit MotionMaster(Unit* unit) : m_owner(unit), m_expList(NULL), m_cleanFlag(MMCF_NONE) {}
        ~MotionMaster();

        void Initialize();

        MovementGenerator* operator->(void) { return top(); }

        using Impl::top;
        using Impl::empty;

        typedef Impl::container_type::const_iterator const_iterator;
        const_iterator begin() const { return Impl::c.begin(); }
        const_iterator end() const { return Impl::c.end(); }

        void UpdateMotion(uint32 diff);
        void Clear(bool reset = true, bool all = false)
        {
            if (m_cleanFlag & MMCF_UPDATE)
                DelayedClean(reset, all);
            else
                DirectClean(reset, all);
        }
        void MovementExpired(bool reset = true)
        {
            if (m_cleanFlag & MMCF_UPDATE)
                DelayedExpire(reset);
            else
                DirectExpire(reset);
        }

        void MoveIdle();
        void MoveRandomAroundPoint(float x, float y, float z, float radius, float verticalZ = 0.0f);
        void MoveTargetedHome();
        void MoveFollow(Unit* target, float dist, float angle);
        void MoveChase(Unit* target, float dist = 0.0f, float angle = 0.0f);
        void MoveConfused();
        void MoveFleeing(Unit* enemy, uint32 timeLimit = 0);
        void MovePoint(uint32 id, float x, float y, float z, bool generatePath = true);
        void MoveSeekAssistance(float x, float y, float z);
        void MoveSeekAssistanceDistract(uint32 timer);
        void MoveWaypoint();
        void MoveTaxiFlight(uint32 path, uint32 pathnode);
        void MoveDistract(uint32 timeLimit);
        void MoveJump(float x, float y, float z, float horizontalSpeed, float max_height, uint32 id = 0);
        void MoveFall();

        MovementGeneratorType GetCurrentMovementGeneratorType() const;

        void propagateSpeedChange();

        bool GetDestination(float& x, float& y, float& z);
    private:
        void Mutate(MovementGenerator* m);                  // use Move* functions instead

        void DirectClean(bool reset, bool all);
        void DelayedClean(bool reset, bool all);

        void DirectExpire(bool reset);
        void DelayedExpire(bool reset);

        Unit*       m_owner;
        ExpireList* m_expList;
        uint8       m_cleanFlag;
};
#endif
