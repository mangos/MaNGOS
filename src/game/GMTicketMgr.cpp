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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "SQLStorages.h"
#include "GMTicketMgr.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "ProgressBar.h"
#include "Policies/SingletonImp.h"
#include "Player.h"

INSTANTIATE_SINGLETON_1(GMTicketMgr);

void GMTicketMgr::LoadGMTickets()
{
    m_GMTicketMap.clear();                                  // For reload case

    QueryResult *result = CharacterDatabase.Query(
        //      0     1            2              3                                  4
        "SELECT guid, ticket_text, response_text, UNIX_TIMESTAMP(ticket_lastchange), ticket_id FROM character_ticket ORDER BY ticket_id ASC");

    if( !result )
    {
        barGoLink bar( 1 );

        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded `character_ticket`, table is empty.");
        return;
    }

    barGoLink bar( (int)result->GetRowCount() );

    do
    {
        bar.step();

        Field* fields = result->Fetch();

        uint32 guidlow = fields[0].GetUInt32();
        if (!guidlow)
            continue;

        ObjectGuid guid = ObjectGuid(HIGHGUID_PLAYER, guidlow);

        GMTicket& ticket = m_GMTicketMap[guid];

        if (ticket.GetPlayerGuid())                         // already exist
        {
            CharacterDatabase.PExecute("DELETE FROM character_ticket WHERE ticket_id = '%u'", fields[4].GetUInt32());
            continue;
        }

        ticket.Init(guid, fields[1].GetCppString(), fields[2].GetCppString(), time_t(fields[3].GetUInt64()));
        m_GMTicketListByCreatingOrder.push_back(&ticket);

    } while (result->NextRow());
    delete result;

    sLog.outString();
    sLog.outString(">> Loaded " SIZEFMTD " GM tickets", GetTicketCount());
}

void GMTicketMgr::DeleteAll()
{
    for(GMTicketMap::const_iterator itr = m_GMTicketMap.begin(); itr != m_GMTicketMap.end(); ++itr)
    {
        if(Player* owner = sObjectMgr.GetPlayer(itr->first))
            owner->GetSession()->SendGMTicketGetTicket(0x0A);
    }
    CharacterDatabase.Execute("DELETE FROM character_ticket");
    m_GMTicketListByCreatingOrder.clear();
    m_GMTicketMap.clear();
}
