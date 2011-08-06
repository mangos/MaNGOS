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

#include "AuctionHouseMgr.h"
#include "Database/DatabaseEnv.h"
#include "SQLStorages.h"
#include "DBCStores.h"
#include "ProgressBar.h"

#include "AccountMgr.h"
#include "Item.h"
#include "Language.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Mail.h"

#include "Policies/SingletonImp.h"

INSTANTIATE_SINGLETON_1(AuctionHouseMgr);

AuctionHouseMgr::AuctionHouseMgr()
{
}

AuctionHouseMgr::~AuctionHouseMgr()
{
    for(ItemMap::const_iterator itr = mAitems.begin(); itr != mAitems.end(); ++itr)
        delete itr->second;
}

AuctionHouseObject * AuctionHouseMgr::GetAuctionsMap(AuctionHouseEntry const* house)
{
    if(sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
        return &mAuctions[AUCTION_HOUSE_NEUTRAL];

    // team have linked auction houses
    switch(GetAuctionHouseTeam(house))
    {
        case ALLIANCE: return &mAuctions[AUCTION_HOUSE_ALLIANCE];
        case HORDE:    return &mAuctions[AUCTION_HOUSE_HORDE];
        default:       return &mAuctions[AUCTION_HOUSE_NEUTRAL];
    }
}

uint32 AuctionHouseMgr::GetAuctionDeposit(AuctionHouseEntry const* entry, uint32 time, Item *pItem)
{
    float deposit = float(pItem->GetProto()->SellPrice * pItem->GetCount() * (time / MIN_AUCTION_TIME));

    deposit = deposit * entry->depositPercent * 3.0f / 100.0f;

    float min_deposit = float(sWorld.getConfig(CONFIG_UINT32_AUCTION_DEPOSIT_MIN));

    if (deposit < min_deposit)
        deposit = min_deposit;

    return uint32(deposit * sWorld.getConfig(CONFIG_FLOAT_RATE_AUCTION_DEPOSIT));
}

// does not clear ram
void AuctionHouseMgr::SendAuctionWonMail(AuctionEntry *auction)
{
    Item *pItem = GetAItem(auction->itemGuidLow);
    if (!pItem)
        return;

    ObjectGuid bidder_guid = ObjectGuid(HIGHGUID_PLAYER, auction->bidder);
    Player *bidder = sObjectMgr.GetPlayer(bidder_guid);

    uint32 bidder_accId = 0;

    ObjectGuid ownerGuid = ObjectGuid(HIGHGUID_PLAYER, auction->owner);
    Player* auction_owner = sObjectMgr.GetPlayer(ownerGuid);

    // data for gm.log
    if (sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
    {
        AccountTypes bidder_security = SEC_PLAYER;
        std::string bidder_name;
        if (bidder)
        {
            bidder_accId = bidder->GetSession()->GetAccountId();
            bidder_security = bidder->GetSession()->GetSecurity();
            bidder_name = bidder->GetName();
        }
        else
        {
            bidder_accId = sObjectMgr.GetPlayerAccountIdByGUID(bidder_guid);
            bidder_security = bidder_accId ? sAccountMgr.GetSecurity(bidder_accId) : SEC_PLAYER;

            if (bidder_security > SEC_PLAYER)               // not do redundant DB requests
            {
                if (!sObjectMgr.GetPlayerNameByGUID(bidder_guid, bidder_name))
                    bidder_name = sObjectMgr.GetMangosStringForDBCLocale(LANG_UNKNOWN);
            }
        }

        if (bidder_security > SEC_PLAYER)
        {
            std::string owner_name;
            if (auction_owner)
                owner_name = auction_owner->GetName();
            else if (ownerGuid && !sObjectMgr.GetPlayerNameByGUID(ownerGuid, owner_name))
                owner_name = sObjectMgr.GetMangosStringForDBCLocale(LANG_UNKNOWN);

            uint32 owner_accid = sObjectMgr.GetPlayerAccountIdByGUID(ownerGuid);

            sLog.outCommand(bidder_accId,"GM %s (Account: %u) won item in auction (Entry: %u Count: %u) and pay money: %u. Original owner %s (Account: %u)",
                bidder_name.c_str(), bidder_accId, auction->itemTemplate, auction->itemCount, auction->bid, owner_name.c_str(), owner_accid);
        }
    }
    else if (!bidder)
        bidder_accId = sObjectMgr.GetPlayerAccountIdByGUID(bidder_guid);

    if (auction_owner)
        auction_owner->GetSession()->SendAuctionOwnerNotification(auction);

    // receiver exist
    if (bidder || bidder_accId)
    {
        std::ostringstream msgAuctionWonSubject;
        msgAuctionWonSubject << auction->itemTemplate << ":" << auction->itemRandomPropertyId << ":" << AUCTION_WON;

        std::ostringstream msgAuctionWonBody;
        msgAuctionWonBody.width(16);
        msgAuctionWonBody << std::right << std::hex << auction->owner;
        msgAuctionWonBody << std::dec << ":" << auction->bid << ":" << auction->buyout;
        DEBUG_LOG("AuctionWon body string : %s", msgAuctionWonBody.str().c_str());

        // set owner to bidder (to prevent delete item with sender char deleting)
        // owner in `data` will set at mail receive and item extracting
        CharacterDatabase.PExecute("UPDATE item_instance SET owner_guid = '%u' WHERE guid='%u'", auction->bidder, auction->itemGuidLow);

        if (bidder)
        {
            bidder->GetSession()->SendAuctionBidderNotification(auction);
            // FIXME: for offline player need also
            bidder->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS, 1);
        }

        RemoveAItem(auction->itemGuidLow);                  // we have to remove the item, before we delete it !!
        auction->itemGuidLow = 0;                           // pending list will not use guid data


        // will delete item or place to receiver mail list
        MailDraft(msgAuctionWonSubject.str(), msgAuctionWonBody.str())
            .AddItem(pItem)
            .SendMailTo(MailReceiver(bidder, bidder_guid), auction, MAIL_CHECK_MASK_COPIED);
    }
    // receiver not exist
    else
    {
        CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid='%u'", auction->itemGuidLow);
        RemoveAItem(auction->itemGuidLow);                  // we have to remove the item, before we delete it !!
        auction->itemGuidLow = 0;
        delete pItem;
    }
}

// call this method to send mail to auction owner, when auction is successful, it does not clear ram
void AuctionHouseMgr::SendAuctionSuccessfulMail(AuctionEntry * auction)
{
    ObjectGuid owner_guid = ObjectGuid(HIGHGUID_PLAYER, auction->owner);
    Player *owner = sObjectMgr.GetPlayer(owner_guid);

    uint32 owner_accId = 0;
    if (!owner)
        owner_accId = sObjectMgr.GetPlayerAccountIdByGUID(owner_guid);

    // owner exist
    if (owner || owner_accId)
    {
        std::ostringstream msgAuctionSuccessfulSubject;
        msgAuctionSuccessfulSubject << auction->itemTemplate << ":" << auction->itemRandomPropertyId << ":" << AUCTION_SUCCESSFUL;

        std::ostringstream auctionSuccessfulBody;
        uint32 auctionCut = auction->GetAuctionCut();

        auctionSuccessfulBody.width(16);
        auctionSuccessfulBody << std::right << std::hex << auction->bidder;
        auctionSuccessfulBody << std::dec << ":" << auction->bid << ":" << auction->buyout;
        auctionSuccessfulBody << ":" << auction->deposit << ":" << auctionCut;

        DEBUG_LOG("AuctionSuccessful body string : %s", auctionSuccessfulBody.str().c_str());

        uint32 profit = auction->bid + auction->deposit - auctionCut;

        if (owner)
        {
            // FIXME: what do if owner offline
            owner->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS, profit);
            owner->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD, auction->bid);
        }

        MailDraft(msgAuctionSuccessfulSubject.str(), auctionSuccessfulBody.str())
            .SetMoney(profit)
            .SendMailTo(MailReceiver(owner, owner_guid), auction, MAIL_CHECK_MASK_COPIED);
    }
}

// does not clear ram
void AuctionHouseMgr::SendAuctionExpiredMail(AuctionEntry * auction)
{                                                           // return an item in auction to its owner by mail
    Item *pItem = GetAItem(auction->itemGuidLow);
    if (!pItem)
    {
        sLog.outError("Auction item (GUID: %u) not found, and lost.", auction->itemGuidLow);
        return;
    }

    ObjectGuid owner_guid = ObjectGuid(HIGHGUID_PLAYER, auction->owner);
    Player *owner = sObjectMgr.GetPlayer(owner_guid);

    uint32 owner_accId = 0;
    if (!owner)
        owner_accId = sObjectMgr.GetPlayerAccountIdByGUID(owner_guid);

    // owner exist
    if (owner || owner_accId)
    {
        std::ostringstream subject;
        subject << auction->itemTemplate << ":" << auction->itemRandomPropertyId << ":" << AUCTION_EXPIRED << ":" << auction->Id << ":" << auction->itemCount;

        if (owner)
            owner->GetSession()->SendAuctionOwnerNotification(auction);

        RemoveAItem(auction->itemGuidLow);                  // we have to remove the item, before we delete it !!
        auction->itemGuidLow = 0;

        // will delete item or place to receiver mail list
        MailDraft(subject.str(), "")                        // TODO: fix body
            .AddItem(pItem)
            .SendMailTo(MailReceiver(owner, owner_guid), auction, MAIL_CHECK_MASK_COPIED);
    }
    // owner not found
    else
    {
        CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid='%u'", auction->itemGuidLow);
        RemoveAItem(auction->itemGuidLow);                  // we have to remove the item, before we delete it !!
        auction->itemGuidLow = 0;
        delete pItem;
    }
}

void AuctionHouseMgr::LoadAuctionItems()
{
    // data needs to be at first place for Item::LoadFromDB 0   1    2        3
    QueryResult *result = CharacterDatabase.Query("SELECT data,text,itemguid,item_template FROM auction JOIN item_instance ON itemguid = guid");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString();
        sLog.outString(">> Loaded 0 auction items");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    uint32 count = 0;

    Field *fields;
    do
    {
        bar.step();

        fields = result->Fetch();
        uint32 item_guid        = fields[2].GetUInt32();
        uint32 item_template    = fields[3].GetUInt32();

        ItemPrototype const *proto = ObjectMgr::GetItemPrototype(item_template);

        if (!proto)
        {
            sLog.outError("AuctionHouseMgr::LoadAuctionItems: Unknown item (GUID: %u id: #%u) in auction, skipped.", item_guid,item_template);
            continue;
        }

        Item *item = NewItemOrBag(proto);

        if (!item->LoadFromDB(item_guid, fields))
        {
            delete item;
            continue;
        }
        AddAItem(item);

        ++count;
    }
    while (result->NextRow());
    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u auction items", count);
}

void AuctionHouseMgr::LoadAuctions()
{
    QueryResult *result = CharacterDatabase.Query("SELECT COUNT(*) FROM auction");
    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString();
        sLog.outString(">> Loaded 0 auctions. DB table `auction` is empty.");
        return;
    }

    Field *fields = result->Fetch();
    uint32 AuctionCount=fields[0].GetUInt32();
    delete result;

    if (!AuctionCount)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString();
        sLog.outString(">> Loaded 0 auctions. DB table `auction` is empty.");
        return;
    }

    result = CharacterDatabase.Query("SELECT id,houseid,itemguid,item_template,item_count,item_randompropertyid,itemowner,buyoutprice,time,moneyTime,buyguid,lastbid,startbid,deposit FROM auction");
    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString();
        sLog.outString(">> Loaded 0 auctions. DB table `auction` is empty.");
        return;
    }

    BarGoLink bar(AuctionCount);

    typedef std::map<uint32, std::wstring> PlayerNames;
    PlayerNames playerNames;                                // caching for load time

    do
    {
        fields = result->Fetch();

        bar.step();

        AuctionEntry *auction = new AuctionEntry;
        auction->Id = fields[0].GetUInt32();
        uint32 houseid  = fields[1].GetUInt32();
        auction->itemGuidLow = fields[2].GetUInt32();
        auction->itemTemplate = fields[3].GetUInt32();
        auction->itemCount = fields[4].GetUInt32();
        auction->itemRandomPropertyId = fields[5].GetUInt32();

        auction->owner = fields[6].GetUInt32();

        if (auction->owner)
        {
            std::wstring& plWName = playerNames[auction->owner];
            if (plWName.empty())
            {
                std::string plName;
                if (!sObjectMgr.GetPlayerNameByGUID(ObjectGuid(HIGHGUID_PLAYER, auction->owner), plName))
                    plName = sObjectMgr.GetMangosStringForDBCLocale(LANG_UNKNOWN);

                Utf8toWStr(plName, plWName);
            }

            auction->ownerName = plWName;
        }

        auction->buyout = fields[7].GetUInt32();
        auction->expireTime = time_t(fields[8].GetUInt64());
        auction->moneyDeliveryTime = time_t(fields[9].GetUInt64());
        auction->bidder = fields[10].GetUInt32();
        auction->bid = fields[11].GetUInt32();
        auction->startbid = fields[12].GetUInt32();
        auction->deposit = fields[13].GetUInt32();
        auction->auctionHouseEntry = NULL;                  // init later

        if (auction->moneyDeliveryTime)
            auction->itemGuidLow = 0;                       // must be 0 if auction delivery pending
        else
        {
            // check if sold item exists for guid
            // and item_template in fact (GetAItem will fail if problematic in result check in AuctionHouseMgr::LoadAuctionItems)
            Item* pItem = GetAItem(auction->itemGuidLow);
            if (!pItem)
            {
                auction->DeleteFromDB();
                sLog.outError("Auction %u has not a existing item : %u, deleted", auction->Id, auction->itemGuidLow);
                delete auction;
                continue;
            }

            // overwrite by real item data
            if ((auction->itemTemplate != pItem->GetEntry()) ||
                (auction->itemCount != pItem->GetCount()) ||
                (auction->itemRandomPropertyId != pItem->GetItemRandomPropertyId()))
            {
                auction->itemTemplate = pItem->GetEntry();
                auction->itemCount    = pItem->GetCount();
                auction->itemRandomPropertyId = pItem->GetItemRandomPropertyId();

                //No SQL injection (no strings)
                CharacterDatabase.PExecute("UPDATE auction SET item_template = %u, item_count = %u, item_randompropertyid = %i WHERE itemguid = %u",
                    auction->itemTemplate, auction->itemCount, auction->itemRandomPropertyId, auction->itemGuidLow);
            }
        }

        auction->auctionHouseEntry = sAuctionHouseStore.LookupEntry(houseid);

        if (!auction->auctionHouseEntry)
        {
            // need for send mail, use goblin auctionhouse
            auction->auctionHouseEntry = sAuctionHouseStore.LookupEntry(7);

            // Attempt send item back to owner
            std::ostringstream msgAuctionCanceledOwner;
            msgAuctionCanceledOwner << auction->itemTemplate << ":"<< auction->itemRandomPropertyId << ":" << AUCTION_CANCELED << ":" << auction->Id << ":" << auction->itemCount;

            if (auction->itemGuidLow)
            {
                Item* pItem = GetAItem(auction->itemGuidLow);

                RemoveAItem(auction->itemGuidLow);
                auction->itemGuidLow = 0;

                // item will deleted or added to received mail list
                MailDraft(msgAuctionCanceledOwner.str(), "")    // TODO: fix body
                    .AddItem(pItem)
                    .SendMailTo(MailReceiver(ObjectGuid(HIGHGUID_PLAYER, auction->owner)), auction, MAIL_CHECK_MASK_COPIED);
            }

            auction->DeleteFromDB();
            delete auction;

            continue;
        }

        GetAuctionsMap(auction->auctionHouseEntry)->AddAuction(auction);

    } while (result->NextRow());
    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u auctions", AuctionCount);
}

void AuctionHouseMgr::AddAItem(Item* it)
{
    MANGOS_ASSERT(it);
    MANGOS_ASSERT(mAitems.find(it->GetGUIDLow()) == mAitems.end());
    mAitems[it->GetGUIDLow()] = it;
}

bool AuctionHouseMgr::RemoveAItem(uint32 id)
{
    ItemMap::iterator i = mAitems.find(id);
    if (i == mAitems.end())
    {
        return false;
    }
    mAitems.erase(i);
    return true;
}

void AuctionHouseMgr::Update()
{
    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
        mAuctions[i].Update();
}

uint32 AuctionHouseMgr::GetAuctionHouseTeam(AuctionHouseEntry const* house)
{
    // auction houses have faction field pointing to PLAYER,* factions,
    // but player factions not have filled team field, and hard go from faction value to faction_template value,
    // so more easy just sort by auction house ids
    switch (house->houseId)
    {
        case 1: case 2: case 3:
            return ALLIANCE;
        case 4: case 5: case 6:
            return HORDE;
        case 7:
        default:
            return 0;                                       // neutral
    }
}

AuctionHouseEntry const* AuctionHouseMgr::GetAuctionHouseEntry(Unit* unit)
{
    uint32 houseid = 1;                                     // dwarf auction house (used for normal cut/etc percents)

    if (!sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
    {
        if (unit->GetTypeId() == TYPEID_UNIT)
        {
            // FIXME: found way for proper auctionhouse selection by another way
            // AuctionHouse.dbc have faction field with _player_ factions associated with auction house races.
            // but no easy way convert creature faction to player race faction for specific city
            uint32 factionTemplateId = unit->getFaction();
            switch (factionTemplateId)
            {
                case   12: houseid = 1; break;              // human
                case   29: houseid = 6; break;              // orc, and generic for horde
                case   55: houseid = 2; break;              // dwarf/gnome, and generic for alliance
                case   68: houseid = 4; break;              // undead
                case   80: houseid = 3; break;              // n-elf
                case  104: houseid = 5; break;              // trolls
                case  120: houseid = 7; break;              // booty bay, neutral
                case  474: houseid = 7; break;              // gadgetzan, neutral
                case  534: houseid = 2; break;              // Alliance Generic
                case  855: houseid = 7; break;              // everlook, neutral
                case 1604: houseid = 6; break;              // b-elfs,
                case 1638: houseid = 2; break;              // exodar, alliance
                default:                                    // for unknown case
                {
                    FactionTemplateEntry const* u_entry = sFactionTemplateStore.LookupEntry(factionTemplateId);
                    if (!u_entry)
                        houseid = 7;                        // goblin auction house
                    else if (u_entry->ourMask & FACTION_MASK_ALLIANCE)
                        houseid = 1;                        // human auction house
                    else if (u_entry->ourMask & FACTION_MASK_HORDE)
                        houseid = 6;                        // orc auction house
                    else
                        houseid = 7;                        // goblin auction house
                    break;
                }
            }
        }
        else
        {
            Player* player = (Player*)unit;
            if (player->GetAuctionAccessMode() > 0)
                houseid = 7;
            else
            {
                switch (((Player*)unit)->GetTeam())
                {
                    case ALLIANCE: houseid = player->GetAuctionAccessMode() == 0 ? 1 : 6; break;
                    case HORDE:    houseid = player->GetAuctionAccessMode() == 0 ? 6 : 1; break;
                }
            }
        }
    }

    return sAuctionHouseStore.LookupEntry(houseid);
}

void AuctionHouseObject::Update()
{
    time_t curTime = sWorld.GetGameTime();
    ///- Handle expired auctions
    for (AuctionEntryMap::iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); )
    {
        if (itr->second->moneyDeliveryTime)                 // pending auction
        {
            if (curTime > itr->second->moneyDeliveryTime)
            {
                sAuctionMgr.SendAuctionSuccessfulMail(itr->second);

                itr->second->DeleteFromDB();
                MANGOS_ASSERT(!itr->second->itemGuidLow);   // already removed or send in mail at won
                delete itr->second;
                AuctionsMap.erase(itr++);
                continue;
            }
        }
        else                                                // active auction
        {
            if (curTime > itr->second->expireTime)
            {
                ///- perform the transaction if there was bidder
                if (itr->second->bid)
                    itr->second->AuctionBidWinning();
                ///- cancel the auction if there was no bidder and clear the auction
                else
                {
                    sAuctionMgr.SendAuctionExpiredMail(itr->second);

                    itr->second->DeleteFromDB();
                    delete itr->second;
                    AuctionsMap.erase(itr++);
                    continue;
                }
            }
        }

        ++itr;
    }
}

void AuctionHouseObject::BuildListBidderItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount)
{
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin();itr != AuctionsMap.end();++itr)
    {
        AuctionEntry *Aentry = itr->second;
        if (Aentry->moneyDeliveryTime)                      // skip pending sell auctions
            continue;
        if (Aentry->bidder == player->GetGUIDLow())
        {
            if (itr->second->BuildAuctionInfo(data))
                ++count;
            ++totalcount;
        }
    }
}

void AuctionHouseObject::BuildListOwnerItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount)
{
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionEntry *Aentry = itr->second;
        if (Aentry->moneyDeliveryTime)                      // skip pending sell auctions
            continue;
        if (Aentry->owner == player->GetGUIDLow())
        {
            if (Aentry->BuildAuctionInfo(data))
                ++count;
            ++totalcount;
        }
    }
}

int AuctionEntry::CompareAuctionEntry(uint32 column, const AuctionEntry *auc, Player* viewPlayer) const
{
    switch (column)
    {
        case 0:                                             // level = 0
        {
            ItemPrototype const* itemProto1 = ObjectMgr::GetItemPrototype(itemTemplate);
            ItemPrototype const* itemProto2 = ObjectMgr::GetItemPrototype(auc->itemTemplate);
            if (!itemProto2 || !itemProto1)
                return 0;
            if (itemProto1->RequiredLevel < itemProto2->RequiredLevel)
                return -1;
            else if (itemProto1->RequiredLevel > itemProto2->RequiredLevel)
                return +1;
            break;
        }
        case 1:                                             // quality = 1
        {
            ItemPrototype const* itemProto1 = ObjectMgr::GetItemPrototype(itemTemplate);
            ItemPrototype const* itemProto2 = ObjectMgr::GetItemPrototype(auc->itemTemplate);
            if (!itemProto2 || !itemProto1)
                return 0;
            if (itemProto1->Quality < itemProto2->Quality)
                return -1;
            else if (itemProto1->Quality > itemProto2->Quality)
                return +1;
            break;
        }
        case 2:                                             // buyoutthenbid = 2
            if (buyout != auc->buyout)
            {
                if (buyout < auc->buyout)
                    return -1;
                else if (buyout > auc->buyout)
                    return +1;
            }
            else
            {
                if (bid < auc->bid)
                    return -1;
                else if (bid > auc->bid)
                    return +1;
            }
            break;
        case 3:                                             // duration = 3
            if (expireTime < auc->expireTime)
                return -1;
            else if (expireTime > auc->expireTime)
                return +1;
            break;
        case 4:                                             // status = 4
            if (bidder < auc->bidder)
                return -1;
            else if (bidder > auc->bidder)
                return +1;
            break;
        case 5:                                             // name = 5
        {
            ItemPrototype const* itemProto1 = ObjectMgr::GetItemPrototype(itemTemplate);
            ItemPrototype const* itemProto2 = ObjectMgr::GetItemPrototype(auc->itemTemplate);
            if (!itemProto2 || !itemProto1)
                return 0;

            int32 loc_idx = viewPlayer->GetSession()->GetSessionDbLocaleIndex();

            std::string name1 = itemProto1->Name1;
            sObjectMgr.GetItemLocaleStrings(itemProto1->ItemId, loc_idx, &name1);

            std::string name2 = itemProto2->Name1;
            sObjectMgr.GetItemLocaleStrings(itemProto2->ItemId, loc_idx, &name2);

            std::wstring wname1, wname2;
            Utf8toWStr(name1, wname1);
            Utf8toWStr(name2, wname2);
            return wname1.compare(wname2);
        }
        case 6:                                             // minbidbuyout = 6
        {
            uint32 bid1 = bid ? bid : startbid;
            uint32 bid2 = auc->bid ? auc->bid : auc->startbid;

            if (bid1 != bid2)
            {
                if (bid1 < bid2)
                    return -1;
                else if (bid1 > bid2)
                    return +1;
            }
            else
            {
                if (buyout < auc->buyout)
                    return -1;
                else if (buyout > auc->buyout)
                    return +1;
            }

            break;
        }
        case 7:                                             // seller = 7
            return ownerName.compare(auc->ownerName);
        case 8:                                             // bid = 8
        {
            uint32 bid1 = bid ? bid : startbid;
            uint32 bid2 = auc->bid ? auc->bid : auc->startbid;

            if (bid1 < bid2)
                return -1;
            else if (bid1 > bid2)
                    return +1;
            break;
        }
        case 9:                                             // quantity = 9
        {
            if (itemCount < auc->itemCount)
                return -1;
            else if (itemCount > auc->itemCount)
                return +1;
            break;
        }
        case 10:                                            // buyout = 10
            if (buyout < auc->buyout)
                return -1;
            else if (buyout > auc->buyout)
                return +1;
            break;
        case 11:                                            // unused = 11
        default:
            break;
    }

    return 0;
}

bool AuctionSorter::operator()(const AuctionEntry *auc1, const AuctionEntry *auc2) const
{
    if (m_sort[0] == MAX_AUCTION_SORT)                      // not sorted
        return false;

    for (uint32 i = 0; i < MAX_AUCTION_SORT; ++i)
    {
        if (m_sort[i] == MAX_AUCTION_SORT)                  // end of sort
            return false;

        int res = auc1->CompareAuctionEntry(m_sort[i] & ~AUCTION_SORT_REVERSED, auc2, m_viewPlayer);
        // "equal" by used column
        if (res == 0)
            continue;
        // less/greater and normal/reversed ordered
        return (res < 0) == ((m_sort[i] & AUCTION_SORT_REVERSED) == 0);
    }

    return false;                                           // "equal" by all sorts
}

void WorldSession::BuildListAuctionItems(std::vector<AuctionEntry*> const& auctions, WorldPacket& data, std::wstring const& wsearchedname, uint32 listfrom, uint32 levelmin,
    uint32 levelmax, uint32 usable, uint32 inventoryType, uint32 itemClass, uint32 itemSubClass, uint32 quality, uint32& count, uint32& totalcount, bool isFull)
{
    int loc_idx = _player->GetSession()->GetSessionDbLocaleIndex();

    for (std::vector<AuctionEntry*>::const_iterator itr = auctions.begin(); itr != auctions.end(); ++itr)
    {
        AuctionEntry *Aentry = *itr;
        if (Aentry->moneyDeliveryTime)
            continue;
        Item *item = sAuctionMgr.GetAItem(Aentry->itemGuidLow);
        if (!item)
            continue;

        if (isFull)
        {
            ++count;
            Aentry->BuildAuctionInfo(data);
        }
        else
        {
            ItemPrototype const *proto = item->GetProto();

            if (itemClass != 0xffffffff && proto->Class != itemClass)
                continue;

            if (itemSubClass != 0xffffffff && proto->SubClass != itemSubClass)
                continue;

            if (inventoryType != 0xffffffff && proto->InventoryType != inventoryType)
                continue;

            if (quality != 0xffffffff && proto->Quality < quality)
                continue;

            if (levelmin != 0x00 && (proto->RequiredLevel < levelmin || (levelmax != 0x00 && proto->RequiredLevel > levelmax)))
                continue;

            if (usable != 0x00 && _player->CanUseItem(item) != EQUIP_ERR_OK)
                continue;

            std::string name = proto->Name1;
            sObjectMgr.GetItemLocaleStrings(proto->ItemId, loc_idx, &name);

            if (!wsearchedname.empty() && !Utf8FitTo(name, wsearchedname))
                continue;

            if (count < 50 && totalcount >= listfrom)
            {
                ++count;
                Aentry->BuildAuctionInfo(data);
            }
        }

        ++totalcount;
    }
}

void AuctionHouseObject::BuildListPendingSales(WorldPacket& data, Player* player, uint32& count)
{
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionEntry *Aentry = itr->second;
        if (!Aentry->moneyDeliveryTime)                     // skip not pending auctions
            continue;
        if (Aentry->owner == player->GetGUIDLow())
        {
            std::ostringstream str1;
            str1 << Aentry->itemTemplate << ":" << Aentry->itemRandomPropertyId << ":" << AUCTION_SUCCESSFUL << ":" << Aentry->Id << ":" << Aentry->itemCount;

            std::ostringstream str2;
            str2.width(16);
            str2 << std::right << std::hex << Aentry->bidder << std::dec << ":";
            str2 << Aentry->bid << ":" << Aentry->buyout << ":" << Aentry->deposit << ":" << Aentry->GetAuctionCut();

            data << str1.str();                             // string "%d:%d:%d:%d:%d" -> itemId, ItemRandomPropertyId, 2, auctionId, unk1 (stack size?, unused)
            data << str2.str();                             // string "%16I64X:%d:%d:%d:%d" -> bidderGuid, bid, buyout, deposit, auctionCut
            data << uint32(97250);                          // unk1
            data << uint32(68);                             // unk2
            float timeLeft = float(Aentry->moneyDeliveryTime - time(NULL)) / float(DAY);
            data << float(timeLeft);                        // time left
            ++count;
        }
    }
}

AuctionEntry* AuctionHouseObject::AddAuction(AuctionHouseEntry const* auctionHouseEntry, Item* newItem, uint32 etime, uint32 bid, uint32 buyout, uint32 deposit, Player * pl /*= NULL*/)
{
    uint32 auction_time = uint32(etime * sWorld.getConfig(CONFIG_FLOAT_RATE_AUCTION_TIME));

    AuctionEntry *AH = new AuctionEntry;
    AH->Id = sObjectMgr.GenerateAuctionID();
    AH->itemGuidLow = newItem->GetObjectGuid().GetCounter();
    AH->itemTemplate = newItem->GetEntry();
    AH->itemCount = newItem->GetCount();
    AH->itemRandomPropertyId = newItem->GetItemRandomPropertyId();
    AH->owner = pl ? pl->GetGUIDLow() : 0;

    if (pl)
        Utf8toWStr(pl->GetName(), AH->ownerName);

    AH->startbid = bid;
    AH->bidder = 0;
    AH->bid = 0;
    AH->buyout = buyout;
    AH->expireTime = time(NULL) + auction_time;
    AH->moneyDeliveryTime = 0;
    AH->deposit = deposit;
    AH->auctionHouseEntry = auctionHouseEntry;

    AddAuction(AH);

    sAuctionMgr.AddAItem(newItem);

    CharacterDatabase.BeginTransaction();
    newItem->SaveToDB();
    AH->SaveToDB();

    if (pl)
        pl->SaveInventoryAndGoldToDB();
    CharacterDatabase.CommitTransaction();

    return AH;
}

// this function inserts to WorldPacket auction's data
bool AuctionEntry::BuildAuctionInfo(WorldPacket & data) const
{
    Item *pItem = sAuctionMgr.GetAItem(itemGuidLow);
    if (!pItem)
    {
        sLog.outError("auction to item, that doesn't exist !!!!");
        return false;
    }
    data << uint32(Id);
    data << uint32(pItem->GetEntry());

    for (uint8 i = 0; i < MAX_INSPECTED_ENCHANTMENT_SLOT; ++i)
    {
        data << uint32(pItem->GetEnchantmentId(EnchantmentSlot(i)));
        data << uint32(pItem->GetEnchantmentDuration(EnchantmentSlot(i)));
        data << uint32(pItem->GetEnchantmentCharges(EnchantmentSlot(i)));
    }

    data << uint32(pItem->GetItemRandomPropertyId());       // random item property id
    data << uint32(pItem->GetItemSuffixFactor());           // SuffixFactor
    data << uint32(pItem->GetCount());                      // item->count
    data << uint32(pItem->GetSpellCharges());               // item->charge FFFFFFF
    data << uint32(0);                                      // item flags (dynamic?) (0x04 no lockId?)
    data << ObjectGuid(HIGHGUID_PLAYER, owner);             // Auction->owner
    data << uint32(startbid);                               // Auction->startbid (not sure if useful)
    data << uint32(bid ? GetAuctionOutBid() : 0);           // minimal outbid
    data << uint32(buyout);                                 // auction->buyout
    data << uint32((expireTime-time(NULL))*IN_MILLISECONDS);// time left
    data << ObjectGuid(HIGHGUID_PLAYER, bidder);            // auction->bidder current
    data << uint32(bid);                                    // current bid
    return true;
}

uint32 AuctionEntry::GetAuctionCut() const
{
    return uint32(auctionHouseEntry->cutPercent * bid * sWorld.getConfig(CONFIG_FLOAT_RATE_AUCTION_CUT) / 100.0f);
}

/// the sum of outbid is (1% from current bid)*5, if bid is very small, it is 1c
uint32 AuctionEntry::GetAuctionOutBid() const
{
    uint32 outbid = (bid / 100) * 5;
    if (!outbid)
        outbid = 1;
    return outbid;
}

void AuctionEntry::DeleteFromDB() const
{
    //No SQL injection (Id is integer)
    CharacterDatabase.PExecute("DELETE FROM auction WHERE id = '%u'", Id);
}

void AuctionEntry::SaveToDB() const
{
    //No SQL injection (no strings)
    CharacterDatabase.PExecute("INSERT INTO auction (id,houseid,itemguid,item_template,item_count,item_randompropertyid,itemowner,buyoutprice,time,moneyTime,buyguid,lastbid,startbid,deposit) "
        "VALUES ('%u', '%u', '%u', '%u', '%u', '%i', '%u', '%u', '" UI64FMTD "', '" UI64FMTD "', '%u', '%u', '%u', '%u')",
        Id, auctionHouseEntry->houseId, itemGuidLow, itemTemplate, itemCount, itemRandomPropertyId, owner, buyout, (uint64)expireTime, (uint64)moneyDeliveryTime, bidder, bid, startbid, deposit);
}

void AuctionEntry::AuctionBidWinning(Player* newbidder)
{
    moneyDeliveryTime = time(NULL) + HOUR;

    CharacterDatabase.BeginTransaction();
    CharacterDatabase.PExecute("UPDATE auction SET itemguid = 0, moneyTime = '" UI64FMTD "', buyguid = '%u', lastbid = '%u' WHERE id = '%u'", (uint64)moneyDeliveryTime, bidder, bid, Id);
    if (newbidder)
        newbidder->SaveInventoryAndGoldToDB();
    CharacterDatabase.CommitTransaction();

    sAuctionMgr.SendAuctionWonMail(this);
}

bool AuctionEntry::UpdateBid(uint32 newbid, Player* newbidder /*=NULL*/)
{
    Player* auction_owner = owner ? sObjectMgr.GetPlayer(ObjectGuid(HIGHGUID_PLAYER, owner)) : NULL;

    // bid can't be greater buyout
    if (buyout && newbid > buyout)
        newbid = buyout;

    if (newbidder && newbidder->GetGUIDLow() == bidder)
    {
        newbidder->ModifyMoney(-int32(newbid - bid));
    }
    else
    {
        if (newbidder)
            newbidder->ModifyMoney(-int32(newbid));

        if (bidder)                                     // return money to old bidder if present
            WorldSession::SendAuctionOutbiddedMail(this);
    }

    bidder = newbidder ? newbidder->GetGUIDLow() : 0;
    bid = newbid;

    if ((newbid < buyout) || (buyout == 0))                 // bid
    {

        if (auction_owner)
            auction_owner->GetSession()->SendAuctionOwnerNotification(this);

        // after this update we should save player's money ...
        CharacterDatabase.BeginTransaction();
        CharacterDatabase.PExecute("UPDATE auction SET buyguid = '%u', lastbid = '%u' WHERE id = '%u'", bidder, bid, Id);
        if (newbidder)
            newbidder->SaveInventoryAndGoldToDB();
        CharacterDatabase.CommitTransaction();
        return true;
    }
    else                                                    // buyout
    {
        AuctionBidWinning(newbidder);
        return false;
    }
}
