/*
 * Copyright (C) 2010-2012 Strawberry-Pr0jcts <http://strawberry-pr0jcts.com/>
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

#include "BattlefieldWG.h"

BattlefieldWG::BattlefieldWG() : Battlefield(BATTLEFIELD_WG)
{
    m_zoneId = 4197;
    m_preBattleTimer = 4 * MINUTE * IN_MILLISECONDS;
    m_nextBattleTimer = 10 * MINUTE * IN_MILLISECONDS;
    m_battleDurationTimer = 8 * MINUTE * IN_MILLISECONDS;
    m_controlledByTeam = ALLIANCE;
}

void BattlefieldWG::OnUpdate(uint32 uiDiff)
{
    if(m_preBattleTimer <= uiDiff)
    {
        //make avalaible to join queue
        sBattlefieldMgr.UpdateWorldState(0x1117,true);
    }
}

void BattlefieldWG::BeforeBattleStarted()
{
    m_preBattleTimer = 4 * MINUTE * IN_MILLISECONDS;
    m_nextBattleTimer = 10 * MINUTE * IN_MILLISECONDS;
    if(m_controlledByTeam == ALLIANCE)
    {
        sBattlefieldMgr.UpdateWorldState(0xeda,false);
    }
    else
    {
        sBattlefieldMgr.UpdateWorldState(0xedb,false);
    }
    sBattlefieldMgr.UpdateWorldState(0xe7e,true);
    sBattlefieldMgr.UpdateWorldState(0xec5,uint32(time(NULL) + (m_battleDurationTimer/1000)));
    sBattlefieldMgr.UpdateWorldState(0xED9, !m_battleInProgress);
}

void BattlefieldWG::AfterBattleEnded()
{
    sBattlefieldMgr.UpdateWorldState(0xe7e,false);
    sBattlefieldMgr.UpdateWorldState(0xED9, !m_battleInProgress);
    sBattlefieldMgr.UpdateWorldState(0x1102, uint32(time(NULL) + (m_nextBattleTimer/1000)));
    if(m_controlledByTeam == ALLIANCE)
    {
        sBattlefieldMgr.UpdateWorldState(0xeda,true);
    }
    else
    {
        sBattlefieldMgr.UpdateWorldState(0xedb,true);
    }
    m_battleDurationTimer = 8 * MINUTE * IN_MILLISECONDS;
}

void BattlefieldWG::OnPlayerEnter(Player* player)
{
    if(player->GetTeam() == ALLIANCE)
    {
        player->CastSpell(player,SPELL_ALLIANCE_CONTROLLS,false);
        player->CastSpell(player,SPELL_HORDE_CONTROLLS_FACTORIES,false);
    }
    else
    {
        player->CastSpell(player,SPELL_HORDE_CONTROLLS,false);
        player->CastSpell(player,SPELL_ALLIANCE_CONTROLLS_FACTORIES,false);
    }
}

void BattlefieldWG::OnPlayerExit(Player* player)
{
    if(player->HasAura(SPELL_ALLIANCE_CONTROLLS))
    {
        player->RemoveAurasDueToSpellByCancel(SPELL_ALLIANCE_CONTROLLS);
        player->RemoveAurasDueToSpellByCancel(SPELL_HORDE_CONTROLLS_FACTORIES);
    }
    else if(player->HasAura(SPELL_HORDE_CONTROLLS))
    {
        player->RemoveAurasDueToSpellByCancel(SPELL_HORDE_CONTROLLS);
        player->RemoveAurasDueToSpellByCancel(SPELL_ALLIANCE_CONTROLLS_FACTORIES);
    }
}
