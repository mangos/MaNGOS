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

#include "ObjectPosSelector.h"
#include "Object.h"

ObjectPosSelector::ObjectPosSelector(float x, float y, float dist, float searcher_size) :
    m_centerX(x), m_centerY(y), m_searcherDist(dist), m_searcherSize(searcher_size)
{
    // if size == 0, m_anglestep will become 0 -> freeze
    if (m_searcherSize == 0.0f)
        m_searcherSize = DEFAULT_WORLD_OBJECT_SIZE;

    m_searcherHalfSize = asin(m_searcherSize/m_searcherDist);

    // Really init in InitilizeAngle
    m_nextUsedAreaItr[USED_POS_PLUS]  = m_UsedAreaLists[USED_POS_PLUS].begin();
    m_nextUsedAreaItr[USED_POS_MINUS] = m_UsedAreaLists[USED_POS_MINUS].begin();
    m_nextUsedAreaStart[USED_POS_PLUS] = 0.0f;
    m_nextUsedAreaStart[USED_POS_MINUS] = 0.0f;
    m_stepAngle[USED_POS_PLUS]  = 0.0f;
    m_stepAngle[USED_POS_MINUS] = 0.0f;
}

/**
 * Add used area (circle) near target object excluded from possible searcher position
 *
 *
 * @param size  Size of used circle
 * @param angle Angle of used circle center point from target-searcher line
 * @param dist  Distance from target object center point to used circle center point
 *
 * Used circles data stored as projections to searcher dist size circle as angle coordinate and half angle size
 */
void ObjectPosSelector::AddUsedArea(float size, float angle, float dist)
{
    float sr_dist = size + m_searcherSize;

    //  by Law of cosines, angle of searcher/used centers
    float sr_angle = acos((m_searcherDist * m_searcherDist + dist * dist - sr_dist * sr_dist) / (2 * m_searcherDist * dist));

    // skip some unexpected results.
    if (!finite(sr_angle) || sr_angle <= 0)
        return;

    if (angle >= 0)
        m_UsedAreaLists[USED_POS_PLUS].insert(UsedArea(angle, sr_angle));
    else
        m_UsedAreaLists[USED_POS_MINUS].insert(UsedArea(-angle, sr_angle));
}

/**
 * Check searcher circle not intercepting with used circle
 *
 * @param usedArea Used circle as projection to searcher distance circle in angles form
 * @param side     Side of used circle
 * @param angle    Checked angle
 *
 * @return true, if used circle not intercepted with searcher circle in terms projection angles
 */
bool ObjectPosSelector::CheckAngle(UsedArea const& usedArea, UsedAreaSide side, float angle) const
{
    float used_angle = usedArea.first * SignOf(side);
    float used_offset = usedArea.second;

    return fabs(used_angle - angle) > used_offset;
}

/**
 * Check searcher circle not intercepting with used circle at side (only start angle provided)
 *
 * @param side     Side of used circle
 * @param angle    Checked angle at side, positive always
 *
 * @return true, if used circle not intercepted with searcher circle in terms projection angles
 */
bool ObjectPosSelector::CheckSideAngle(UsedAreaSide side, float angle) const
{
    return angle + m_searcherHalfSize < m_nextUsedAreaStart[side];
}

/**
 * Check original (0.0f) angle fit to existed used area excludes
 *
 * @return true, if 0.0f angle with m_searcher_halfangle*2 angle size not intercept with used circles
 */
bool ObjectPosSelector::CheckOriginalAngle() const
{
    // check first left/right used angles if exists
    return (m_UsedAreaLists[USED_POS_PLUS].empty()  || CheckAngle(*m_UsedAreaLists[USED_POS_PLUS].begin(), USED_POS_PLUS, 0.0f)) &&
        (m_UsedAreaLists[USED_POS_MINUS].empty() || CheckAngle(*m_UsedAreaLists[USED_POS_MINUS].begin(), USED_POS_MINUS, 0.0f));
}

/**
 * Initialize data for search angles starting from first possible angle at both sides
 */
void ObjectPosSelector::InitializeAngle()
{
    InitializeAngle(USED_POS_PLUS);
    InitializeAngle(USED_POS_MINUS);
}

/**
 * Initialize data for search angles starting from first possible angle at side
 */
void ObjectPosSelector::InitializeAngle(UsedAreaSide side)
{
    m_nextUsedAreaItr[side] = m_UsedAreaLists[side].begin();
    UpdateNextAreaStart(side);

    // if another side not alow use 0.0f angle calculate possible value in 0..m_searcherHalfSize range
    if (!m_UsedAreaLists[~side].empty())
    {
        UsedArea const& otherArea = *m_UsedAreaLists[~side].begin();

        // if other are near start
        if (otherArea.first < otherArea.second)
            m_stepAngle[side] = otherArea.second - otherArea.first;
        else
            m_stepAngle[side] = 0.0f;
    }
    else
        m_stepAngle[side] = 0.0f;
}

/**
 * Update next used area start angle for current m_nextUsedAreaItr value at side
 */
void ObjectPosSelector::UpdateNextAreaStart(UsedAreaSide side)
{
    // not last next area at side
    if (m_nextUsedAreaItr[side] != m_UsedAreaLists[side].end())
    {
        m_nextUsedAreaStart[side] = m_nextUsedAreaItr[side]->first - m_nextUsedAreaItr[side]->second + m_searcherHalfSize;
        return;
    }

    // last area at side and not another side areas
    if (m_UsedAreaLists[~side].empty())
    {
        m_nextUsedAreaStart[side] = M_PI_F + m_searcherHalfSize + 0.01f;
        return;
    }

    UsedArea const& lastArea = *m_UsedAreaLists[~side].rbegin();

    // another side have used area near to end (near to PI)
    if (lastArea.first + lastArea.second > M_PI_F - m_searcherHalfSize)
    {
        m_nextUsedAreaStart[side] = M_PI_F + (M_PI_F - lastArea.first - lastArea.second) + m_searcherHalfSize;
        return;
    }

    // last area and fail find any used area at another side, prepare fake data as stopper
    m_nextUsedAreaStart[side] = M_PI_F + m_searcherHalfSize + 0.01f;
}

/**
 * Find next angle in free area
 *
 * @param angle    Return at success found angle
 *
 * @return true, if angle found
 */
bool ObjectPosSelector::NextAngle(float& angle)
{
    // loop until both side fail and leave 0..PI
    for(;;)
    {
        // ++ direction less updated
        if (m_stepAngle[USED_POS_PLUS] < M_PI_F && m_stepAngle[USED_POS_PLUS] <= m_stepAngle[USED_POS_MINUS])
        {
            if (NextSideAngle(USED_POS_PLUS, angle))
                return true;
        }
        // -- direction less updated
        else if (m_stepAngle[USED_POS_MINUS] < M_PI_F)
        {
            if (NextSideAngle(USED_POS_MINUS, angle))
                return true;
        }
        // both sides finishes
        else
            break;
    }

    // no angles
    return false;
}

/**
 * Find next angle at side
 *
 * @param side     Side of angle
 * @param angle    Return at success found angle
 *
 * @return true, if angle found
 *
 */
bool ObjectPosSelector::NextSideAngle(UsedAreaSide side, float &angle )
{
    // next possible angle
    m_stepAngle[side] += (m_searcherHalfSize + 0.01);

    // prevent jump to another side
    if (m_stepAngle[side] > M_PI_F)
        return false;

    // update angle at attempt jump after next used area
    while (m_stepAngle[side] <= M_PI_F && m_stepAngle[side] + m_searcherHalfSize >= m_nextUsedAreaStart[side])
    {
        // no used area for pass
        if (m_nextUsedAreaItr[side] == m_UsedAreaLists[side].end())
        {
            m_stepAngle[side] = M_PI_F + m_searcherHalfSize;// prevent continue search at side
            return false;
        }

        // angle set at first possible pos after passed m_nextUsedAreaItr
        m_stepAngle[side] = m_nextUsedAreaItr[side]->first + m_nextUsedAreaItr[side]->second;

        ++m_nextUsedAreaItr[side];
        UpdateNextAreaStart(side);
    }

    angle = m_stepAngle[side] * SignOf(side);

    // if next node not allow use selected angle, mark and fail
    return CheckSideAngle(side, m_stepAngle[side]);
}

/**
 * Find next angle in used area, that used if no angle found in free area with LoS
 *
 * @param angle    Return at success found angle
 *
 * @return true, if angle found
 */
bool ObjectPosSelector::NextUsedAngle(float& angle)
{
    if (m_nextUsedAreaItr[USED_POS_PLUS] == m_UsedAreaLists[USED_POS_PLUS].end() &&
        m_nextUsedAreaItr[USED_POS_MINUS] == m_UsedAreaLists[USED_POS_MINUS].end())
        return false;

    // ++ direction less updated
    if (m_nextUsedAreaItr[USED_POS_PLUS] != m_UsedAreaLists[USED_POS_PLUS].end() &&
        (m_nextUsedAreaItr[USED_POS_MINUS] == m_UsedAreaLists[USED_POS_MINUS].end() ||
        m_nextUsedAreaItr[USED_POS_PLUS]->first <= m_nextUsedAreaItr[USED_POS_MINUS]->first))
    {
        angle = m_nextUsedAreaItr[USED_POS_PLUS]->first * SignOf(USED_POS_PLUS);
        ++m_nextUsedAreaItr[USED_POS_PLUS];
    }
    else
    {
        angle = m_nextUsedAreaItr[USED_POS_MINUS]->first * SignOf(USED_POS_MINUS);
        ++m_nextUsedAreaItr[USED_POS_MINUS];
    }

    return true;
}
