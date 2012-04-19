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

#include "Common.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "Database/DatabaseImpl.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "UpdateMask.h"
#include "NPCHandler.h"
#include "Pet.h"
#include "MapManager.h"

void WorldSession::SendNameQueryOpcode(Player *p)
{
    if(!p)
        return;
                                                            // guess size
    WorldPacket data( SMSG_NAME_QUERY_RESPONSE, (8+1+1+1+1+1+10) );
    data << p->GetPackGUID();                               // player guid
    data << uint8(0);                                       // added in 3.1; if > 1, then end of packet
    data << p->GetName();                                   // played name
    data << uint8(0);                                       // realm name for cross realm BG usage
    data << uint8(p->getRace());
    data << uint8(p->getGender());
    data << uint8(p->getClass());
    if(DeclinedName const* names = p->GetDeclinedNames())
    {
        data << uint8(1);                                   // is declined
        for(int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            data << names->name[i];
    }
    else
        data << uint8(0);                                   // is not declined

    SendPacket(&data);
}

void WorldSession::SendNameQueryOpcodeFromDB(ObjectGuid guid)
{
    CharacterDatabase.AsyncPQuery(&WorldSession::SendNameQueryOpcodeFromDBCallBack, GetAccountId(),
        !sWorld.getConfig(CONFIG_BOOL_DECLINED_NAMES_USED) ?
    //   ------- Query Without Declined Names --------
    //          0     1     2     3       4
        "SELECT guid, name, race, gender, class "
        "FROM characters WHERE guid = '%u'"
        :
    //   --------- Query With Declined Names ---------
    //          0                1     2     3       4
        "SELECT characters.guid, name, race, gender, class, "
    //   5         6       7           8             9
        "genitive, dative, accusative, instrumental, prepositional "
        "FROM characters LEFT JOIN character_declinedname ON characters.guid = character_declinedname.guid WHERE characters.guid = '%u'",
        guid.GetCounter());
}

void WorldSession::SendNameQueryOpcodeFromDBCallBack(QueryResult *result, uint32 accountId)
{
    if(!result)
        return;

    WorldSession * session = sWorld.FindSession(accountId);
    if(!session)
    {
        delete result;
        return;
    }

    Field *fields = result->Fetch();
    uint32 lowguid      = fields[0].GetUInt32();
    std::string name = fields[1].GetCppString();
    uint8 pRace = 0, pGender = 0, pClass = 0;
    if(name == "")
        name         = session->GetMangosString(LANG_NON_EXIST_CHARACTER);
    else
    {
        pRace        = fields[2].GetUInt8();
        pGender      = fields[3].GetUInt8();
        pClass       = fields[4].GetUInt8();
    }
                                                            // guess size
    WorldPacket data( SMSG_NAME_QUERY_RESPONSE, (8+1+1+1+1+1+1+10) );
    data << ObjectGuid(HIGHGUID_PLAYER, lowguid).WriteAsPacked();
    data << uint8(0);                                       // added in 3.1; if > 1, then end of packet
    data << name;
    data << uint8(0);                                       // realm name for cross realm BG usage
    data << uint8(pRace);                                   // race
    data << uint8(pGender);                                 // gender
    data << uint8(pClass);                                  // class

    // if the first declined name field (5) is empty, the rest must be too
    if(sWorld.getConfig(CONFIG_BOOL_DECLINED_NAMES_USED) && fields[5].GetCppString() != "")
    {
        data << uint8(1);                                   // is declined
        for(int i = 5; i < MAX_DECLINED_NAME_CASES+5; ++i)
            data << fields[i].GetCppString();
    }
    else
        data << uint8(0);                                   // is not declined

    session->SendPacket( &data );
    delete result;
}

void WorldSession::HandleNameQueryOpcode( WorldPacket & recv_data )
{
    ObjectGuid guid;

    recv_data >> guid;

    Player *pChar = sObjectMgr.GetPlayer(guid);

    if (pChar)
        SendNameQueryOpcode(pChar);
    else
        SendNameQueryOpcodeFromDB(guid);
}

void WorldSession::HandleQueryTimeOpcode( WorldPacket & /*recv_data*/ )
{
    SendQueryTimeResponse();
}

/// Only _static_ data send in this packet !!!
void WorldSession::HandleCreatureQueryOpcode( WorldPacket & recv_data )
{
    uint32 entry;
    recv_data >> entry;
    ObjectGuid guid;
    recv_data >> guid;

    CreatureInfo const *ci = ObjectMgr::GetCreatureTemplate(entry);
    if (ci)
    {
        int loc_idx = GetSessionDbLocaleIndex();

        char const* name = ci->Name;
        char const* subName = ci->SubName;
        sObjectMgr.GetCreatureLocaleStrings(entry, loc_idx, &name, &subName);

        DETAIL_LOG("WORLD: CMSG_CREATURE_QUERY '%s' - Entry: %u.", ci->Name, entry);
                                                            // guess size
        WorldPacket data( SMSG_CREATURE_QUERY_RESPONSE, 100 );
        data << uint32(entry);                              // creature entry
        data << name;
        data << uint8(0) << uint8(0) << uint8(0);           // name2, name3, name4, always empty
        data << subName;
        data << ci->IconName;                               // "Directions" for guard, string for Icons 2.3.0
        data << uint32(ci->type_flags);                     // flags
        data << uint32(ci->type);                           // CreatureType.dbc
        data << uint32(ci->family);                         // CreatureFamily.dbc
        data << uint32(ci->rank);                           // Creature Rank (elite, boss, etc)
        data << uint32(ci->KillCredit[0]);                  // new in 3.1, kill credit
        data << uint32(ci->KillCredit[1]);                  // new in 3.1, kill credit

        for(int i = 0; i < MAX_CREATURE_MODEL; ++i)
            data << uint32(ci->ModelId[i]);

        data << float(ci->unk16);                           // health modifier
        data << float(ci->unk17);                           // power modifier
        data << uint8(ci->RacialLeader);
        for(uint32 i = 0; i < 6; ++i)
            data << uint32(ci->questItems[i]);              // itemId[6], quest drop
        data << uint32(ci->movementId);                     // CreatureMovementInfo.dbc
        SendPacket( &data );
        DEBUG_LOG( "WORLD: Sent SMSG_CREATURE_QUERY_RESPONSE" );
    }
    else
    {
        DEBUG_LOG("WORLD: CMSG_CREATURE_QUERY - Guid: %s Entry: %u NO CREATURE INFO!",
            guid.GetString().c_str(), entry);
        WorldPacket data( SMSG_CREATURE_QUERY_RESPONSE, 4 );
        data << uint32(entry | 0x80000000);
        SendPacket( &data );
        DEBUG_LOG( "WORLD: Sent SMSG_CREATURE_QUERY_RESPONSE" );
    }
}

/// Only _static_ data send in this packet !!!
void WorldSession::HandleGameObjectQueryOpcode( WorldPacket & recv_data )
{
    uint32 entryID;
    recv_data >> entryID;
    ObjectGuid guid;
    recv_data >> guid;

    const GameObjectInfo *info = ObjectMgr::GetGameObjectInfo(entryID);
    if(info)
    {
        std::string Name;
        std::string IconName;
        std::string CastBarCaption;

        Name = info->name;
        IconName = info->IconName;
        CastBarCaption = info->castBarCaption;

        int loc_idx = GetSessionDbLocaleIndex();
        if (loc_idx >= 0)
        {
            GameObjectLocale const *gl = sObjectMgr.GetGameObjectLocale(entryID);
            if (gl)
            {
                if (gl->Name.size() > size_t(loc_idx) && !gl->Name[loc_idx].empty())
                    Name = gl->Name[loc_idx];
                if (gl->CastBarCaption.size() > size_t(loc_idx) && !gl->CastBarCaption[loc_idx].empty())
                    CastBarCaption = gl->CastBarCaption[loc_idx];
            }
        }
        DETAIL_LOG("WORLD: CMSG_GAMEOBJECT_QUERY '%s' - Entry: %u. ", info->name, entryID);
        WorldPacket data ( SMSG_GAMEOBJECT_QUERY_RESPONSE, 150 );
        data << uint32(entryID);
        data << uint32(info->type);
        data << uint32(info->displayId);
        data << Name;
        data << uint8(0) << uint8(0) << uint8(0);           // name2, name3, name4
        data << IconName;                                   // 2.0.3, string. Icon name to use instead of default icon for go's (ex: "Attack" makes sword)
        data << CastBarCaption;                             // 2.0.3, string. Text will appear in Cast Bar when using GO (ex: "Collecting")
        data << info->unk1;                                 // 2.0.3, string
        data.append(info->raw.data, 24);
        data << float(info->size);                          // go size
        for(uint32 i = 0; i < 6; ++i)
            data << uint32(info->questItems[i]);            // itemId[6], quest drop
        SendPacket( &data );
        DEBUG_LOG( "WORLD: Sent SMSG_GAMEOBJECT_QUERY_RESPONSE" );
    }
    else
    {
        DEBUG_LOG("WORLD: CMSG_GAMEOBJECT_QUERY - Guid: %s Entry: %u Missing gameobject info!",
            guid.GetString().c_str(), entryID);
        WorldPacket data ( SMSG_GAMEOBJECT_QUERY_RESPONSE, 4 );
        data << uint32(entryID | 0x80000000);
        SendPacket( &data );
        DEBUG_LOG( "WORLD: Sent SMSG_GAMEOBJECT_QUERY_RESPONSE" );
    }
}

void WorldSession::HandleCorpseQueryOpcode(WorldPacket & /*recv_data*/)
{
    DETAIL_LOG("WORLD: Received MSG_CORPSE_QUERY");

    Corpse *corpse = GetPlayer()->GetCorpse();

    if(!corpse)
    {
        WorldPacket data(MSG_CORPSE_QUERY, 1);
        data << uint8(0);                                   // corpse not found
        SendPacket(&data);
        return;
    }

    uint32 corpsemapid = corpse->GetMapId();
    float x = corpse->GetPositionX();
    float y = corpse->GetPositionY();
    float z = corpse->GetPositionZ();
    int32 mapid = corpsemapid;

    // if corpse at different map
    if (corpsemapid != _player->GetMapId())
    {
        // search entrance map for proper show entrance
        if (MapEntry const* corpseMapEntry = sMapStore.LookupEntry(corpsemapid))
        {
            if (corpseMapEntry->IsDungeon() && corpseMapEntry->ghost_entrance_map >= 0)
            {
                // if corpse map have entrance
                if(TerrainInfo const* entranceMap = sTerrainMgr.LoadTerrain(corpseMapEntry->ghost_entrance_map))
                {
                    mapid = corpseMapEntry->ghost_entrance_map;
                    x = corpseMapEntry->ghost_entrance_x;
                    y = corpseMapEntry->ghost_entrance_y;
                    z = entranceMap->GetHeight(x, y, MAX_HEIGHT);
                }
            }
        }
    }

    WorldPacket data(MSG_CORPSE_QUERY, 1+(6*4));
    data << uint8(1);                                       // corpse found
    data << int32(mapid);
    data << float(x);
    data << float(y);
    data << float(z);
    data << uint32(corpsemapid);
    data << uint32(0);                                      // unknown
    SendPacket(&data);
}

void WorldSession::HandleNpcTextQueryOpcode( WorldPacket & recv_data )
{
    uint32 textID;
    ObjectGuid guid;

    recv_data >> textID;
    recv_data >> guid;

    DETAIL_LOG("WORLD: CMSG_NPC_TEXT_QUERY ID '%u'", textID);

    _player->SetTargetGuid(guid);

    GossipText const* pGossip = sObjectMgr.GetGossipText(textID);

    WorldPacket data( SMSG_NPC_TEXT_UPDATE, 100 );          // guess size
    data << textID;

    if (!pGossip)
    {
        for(uint32 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            data << float(0);
            data << "Greetings $N";
            data << "Greetings $N";
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
        }
    }
    else
    {
        std::string Text_0[MAX_GOSSIP_TEXT_OPTIONS], Text_1[MAX_GOSSIP_TEXT_OPTIONS];
        for (int i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            Text_0[i]=pGossip->Options[i].Text_0;
            Text_1[i]=pGossip->Options[i].Text_1;
        }

        int loc_idx = GetSessionDbLocaleIndex();

        sObjectMgr.GetNpcTextLocaleStringsAll(textID, loc_idx, &Text_0, &Text_1);

        for (int i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            data << pGossip->Options[i].Probability;

            if (Text_0[i].empty())
                data << Text_1[i];
            else
                data << Text_0[i];

            if (Text_1[i].empty())
                data << Text_0[i];
            else
                data << Text_1[i];

            data << pGossip->Options[i].Language;

            for(int j = 0; j < 3; ++j)
            {
                data << pGossip->Options[i].Emotes[j]._Delay;
                data << pGossip->Options[i].Emotes[j]._Emote;
            }
        }
    }

    SendPacket( &data );

    DEBUG_LOG( "WORLD: Sent SMSG_NPC_TEXT_UPDATE" );
}

void WorldSession::HandlePageTextQueryOpcode( WorldPacket & recv_data )
{
    DETAIL_LOG("WORLD: Received CMSG_PAGE_TEXT_QUERY");
    recv_data.hexlike();

    uint32 pageID;
    recv_data >> pageID;
    recv_data.read_skip<uint64>();                          // guid

    while (pageID)
    {
        PageText const *pPage = sPageTextStore.LookupEntry<PageText>( pageID );
                                                            // guess size
        WorldPacket data( SMSG_PAGE_TEXT_QUERY_RESPONSE, 50 );
        data << pageID;

        if (!pPage)
        {
            data << "Item page missing.";
            data << uint32(0);
            pageID = 0;
        }
        else
        {
            std::string Text = pPage->Text;

            int loc_idx = GetSessionDbLocaleIndex();
            if (loc_idx >= 0)
            {
                PageTextLocale const *pl = sObjectMgr.GetPageTextLocale(pageID);
                if (pl)
                {
                    if (pl->Text.size() > size_t(loc_idx) && !pl->Text[loc_idx].empty())
                        Text = pl->Text[loc_idx];
                }
            }

            data << Text;
            data << uint32(pPage->Next_Page);
            pageID = pPage->Next_Page;
        }
        SendPacket( &data );

        DEBUG_LOG( "WORLD: Sent SMSG_PAGE_TEXT_QUERY_RESPONSE" );
    }
}

void WorldSession::HandleCorpseMapPositionQueryOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG( "WORLD: Recv CMSG_CORPSE_MAP_POSITION_QUERY" );

    uint32 unk;
    recv_data >> unk;

    WorldPacket data(SMSG_CORPSE_TRANSPORT_QUERY, 4+4+4+4);
    data << float(0);
    data << float(0);
    data << float(0);
    data << float(0);
    SendPacket(&data);
}

void WorldSession::HandleQueryQuestsCompletedOpcode( WorldPacket & /*recv_data */)
{
    uint32 count = 0;

    WorldPacket data(SMSG_ALL_QUESTS_COMPLETED, 4+4*count);
    data << uint32(count);

    for(QuestStatusMap::const_iterator itr = _player->getQuestStatusMap().begin(); itr != _player->getQuestStatusMap().end(); ++itr)
    {
        if(itr->second.m_rewarded)
        {
            data << uint32(itr->first);
            count++;
        }
    }
    data.put<uint32>(0, count);
    SendPacket(&data);
}

void WorldSession::HandleQuestPOIQueryOpcode(WorldPacket& recv_data)
{
    uint32 count;
    recv_data >> count;                                     // quest count, max=25

    if(count > MAX_QUEST_LOG_SIZE)
    {
        recv_data.rpos(recv_data.wpos());                   // set to end to avoid warnings spam
        return;
    }

    WorldPacket data(SMSG_QUEST_POI_QUERY_RESPONSE, 4+(4+4)*count);
    data << uint32(count);                                  // count

    for(uint32 i = 0; i < count; ++i)
    {
        uint32 questId;
        recv_data >> questId;                               // quest id

        bool questOk = false;

        uint16 questSlot = _player->FindQuestSlot(questId);

        if(questSlot != MAX_QUEST_LOG_SIZE)
            questOk =_player->GetQuestSlotQuestId(questSlot) == questId;

        if(questOk)
        {
            QuestPOIVector const *POI = sObjectMgr.GetQuestPOIVector(questId);

            if(POI)
            {
                data << uint32(questId);                    // quest ID
                data << uint32(POI->size());                // POI count

                for(QuestPOIVector::const_iterator itr = POI->begin(); itr != POI->end(); ++itr)
                {
                    data << uint32(itr->PoiId);             // POI index
                    data << int32(itr->ObjectiveIndex);     // objective index
                    data << uint32(itr->MapId);             // mapid
                    data << uint32(itr->MapAreaId);         // world map area id
                    data << uint32(itr->FloorId);           // floor id
                    data << uint32(itr->Unk3);              // unknown
                    data << uint32(itr->Unk4);              // unknown
                    data << uint32(itr->points.size());     // POI points count

                    for(std::vector<QuestPOIPoint>::const_iterator itr2 = itr->points.begin(); itr2 != itr->points.end(); ++itr2)
                    {
                        data << int32(itr2->x);             // POI point x
                        data << int32(itr2->y);             // POI point y
                    }
                }
            }
            else
            {
                data << uint32(questId);                    // quest ID
                data << uint32(0);                          // POI count
            }
        }
        else
        {
            data << uint32(questId);                        // quest ID
            data << uint32(0);                              // POI count
        }
    }

    SendPacket(&data);
}

void WorldSession::SendQueryTimeResponse()
{
    WorldPacket data(SMSG_QUERY_TIME_RESPONSE, 4+4);
    data << uint32(time(NULL));
    data << uint32(sWorld.GetNextDailyQuestsResetTime() - time(NULL));
    SendPacket(&data);
}
