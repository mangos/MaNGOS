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

#include "Player.h"
#include "BattleGround.h"
#include "BattleGroundRB.h"
#include "Language.h"

BattleGroundRB::BattleGroundRB()
{
    // TODO FIX ME!
    m_StartMessageIds[BG_STARTING_EVENT_FIRST]  = 0;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_WS_START_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_WS_START_HALF_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_WS_HAS_BEGUN;
}

BattleGroundRB::~BattleGroundRB()
{

}

void BattleGroundRB::Update(uint32 diff)
{
    BattleGround::Update(diff);
}

void BattleGroundRB::StartingEventCloseDoors()
{
}

void BattleGroundRB::StartingEventOpenDoors()
{
}

void BattleGroundRB::AddPlayer(Player* plr)
{
    BattleGround::AddPlayer(plr);
    // create score and add it to map, default values are set in constructor
    BattleGroundABGScore* sc = new BattleGroundABGScore;

    m_PlayerScores[plr->GetObjectGuid()] = sc;
}

void BattleGroundRB::RemovePlayer(Player* /*plr*/, ObjectGuid /*guid*/)
{

}

void BattleGroundRB::HandleAreaTrigger(Player* /*source*/, uint32 /*trigger*/)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;
}

void BattleGroundRB::UpdatePlayerScore(Player* source, uint32 type, uint32 value)
{

    BattleGroundScoreMap::iterator itr = m_PlayerScores.find(source->GetObjectGuid());

    if (itr == m_PlayerScores.end())                        // player not found...
        return;

    BattleGround::UpdatePlayerScore(source, type, value);
}
