/*
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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
#include "Database/SQLStorage.h"
#include "ProgressBar.h"

#include "AccountMgr.h"
#include "AuctionHouseMgr.h"
#include "Item.h"
#include "Language.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"

#include "Policies/SingletonImp.h"

INSTANTIATE_SINGLETON_1( AuctionHouseMgr );


AuctionHouseMgr::AuctionHouseMgr()
{
}

AuctionHouseMgr::~AuctionHouseMgr()
{
    for(ItemMap::iterator itr = mAitems.begin(); itr != mAitems.end(); ++itr)
        delete itr->second;
}

AuctionHouseObject * AuctionHouseMgr::GetAuctionsMap( AuctionLocation location )
{
    switch ( location )
    {
    case AUCTION_HORDE:
        return & mHordeAuctions;
        break;
    case AUCTION_ALLIANCE:
        return & mAllianceAuctions;
        break;
    default:                                            //neutral
        return & mNeutralAuctions;
    }
}

uint32 AuctionHouseMgr::GetAuctionCut(AuctionLocation location, uint32 highBid)
{
    if (location == AUCTION_NEUTRAL && !sWorld.getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
        return (uint32) (0.15f * highBid * sWorld.getRate(RATE_AUCTION_CUT));
    else
        return (uint32) (0.05f * highBid * sWorld.getRate(RATE_AUCTION_CUT));
}

uint32 AuctionHouseMgr::GetAuctionDeposit(AuctionLocation location, uint32 time, Item *pItem)
{
    float percentance;                                      // in 0..1
    if (location == AUCTION_NEUTRAL && !sWorld.getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
        percentance = 0.75f;
    else
        percentance = 0.15f;

    percentance *= sWorld.getRate(RATE_AUCTION_DEPOSIT);

    return uint32( percentance * pItem->GetProto()->SellPrice * pItem->GetCount() * (time / MIN_AUCTION_TIME ) );
}

/// the sum of outbid is (1% from current bid)*5, if bid is very small, it is 1c
uint32 AuctionHouseMgr::GetAuctionOutBid(uint32 currentBid)
{
    uint32 outbid = (currentBid / 100) * 5;
    if (!outbid)
        outbid = 1;
    return outbid;
}

//does not clear ram
void AuctionHouseMgr::SendAuctionWonMail( AuctionEntry *auction )
{
    Item *pItem = GetAItem(auction->item_guidlow);
    if(!pItem)
        return;

    uint64 bidder_guid = MAKE_NEW_GUID(auction->bidder, 0, HIGHGUID_PLAYER);
    Player *bidder = objmgr.GetPlayer(bidder_guid);

    uint32 bidder_accId = 0;

    // data for gm.log
    if( sWorld.getConfig(CONFIG_GM_LOG_TRADE) )
    {
        uint32 bidder_security = 0;
        std::string bidder_name;
        if (bidder)
        {
            bidder_accId = bidder->GetSession()->GetAccountId();
            bidder_security = bidder->GetSession()->GetSecurity();
            bidder_name = bidder->GetName();
        }
        else
        {
            bidder_accId = objmgr.GetPlayerAccountIdByGUID(bidder_guid);
            bidder_security = accmgr.GetSecurity(bidder_accId);

            if(bidder_security > SEC_PLAYER )               // not do redundant DB requests
            {
                if(!objmgr.GetPlayerNameByGUID(bidder_guid,bidder_name))
                    bidder_name = objmgr.GetMangosStringForDBCLocale(LANG_UNKNOWN);
            }
        }

        if( bidder_security > SEC_PLAYER )
        {
            std::string owner_name;
            if(!objmgr.GetPlayerNameByGUID(auction->owner,owner_name))
                owner_name = objmgr.GetMangosStringForDBCLocale(LANG_UNKNOWN);

            uint32 owner_accid = objmgr.GetPlayerAccountIdByGUID(auction->owner);

            sLog.outCommand(bidder_accId,"GM %s (Account: %u) won item in auction: %s (Entry: %u Count: %u) and pay money: %u. Original owner %s (Account: %u)",
                bidder_name.c_str(),bidder_accId,pItem->GetProto()->Name1,pItem->GetEntry(),pItem->GetCount(),auction->bid,owner_name.c_str(),owner_accid);
        }
    }
    else if(!bidder)
        bidder_accId = objmgr.GetPlayerAccountIdByGUID(bidder_guid);

    // receiver exist
    if(bidder || bidder_accId)
    {
        std::ostringstream msgAuctionWonSubject;
        msgAuctionWonSubject << auction->item_template << ":0:" << AUCTION_WON;

        std::ostringstream msgAuctionWonBody;
        msgAuctionWonBody.width(16);
        msgAuctionWonBody << std::right << std::hex << auction->owner;
        msgAuctionWonBody << std::dec << ":" << auction->bid << ":" << auction->buyout;
        sLog.outDebug( "AuctionWon body string : %s", msgAuctionWonBody.str().c_str() );

        //prepare mail data... :
        uint32 itemTextId = objmgr.CreateItemText( msgAuctionWonBody.str() );

        // set owner to bidder (to prevent delete item with sender char deleting)
        // owner in `data` will set at mail receive and item extracting
        CharacterDatabase.PExecute("UPDATE item_instance SET owner_guid = '%u' WHERE guid='%u'",auction->bidder,pItem->GetGUIDLow());
        CharacterDatabase.CommitTransaction();

        MailItemsInfo mi;
        mi.AddItem(auction->item_guidlow, auction->item_template, pItem);

        if (bidder)
            bidder->GetSession()->SendAuctionBidderNotification( auction->location, auction->Id, bidder_guid, 0, 0, auction->item_template);
        else
            RemoveAItem(pItem->GetGUIDLow());               // we have to remove the item, before we delete it !!

        // will delete item or place to receiver mail list
        WorldSession::SendMailTo(bidder, MAIL_AUCTION, MAIL_STATIONERY_AUCTION, auction->location, auction->bidder, msgAuctionWonSubject.str(), itemTextId, &mi, 0, 0, MAIL_CHECK_MASK_AUCTION);
    }
    // receiver not exist
    else
    {
        CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid='%u'", pItem->GetGUIDLow());
        RemoveAItem(pItem->GetGUIDLow());                   // we have to remove the item, before we delete it !!
        delete pItem;
    }
}

void AuctionHouseMgr::SendAuctionSalePendingMail( AuctionEntry * auction )
{
    uint64 owner_guid = MAKE_NEW_GUID(auction->owner, 0, HIGHGUID_PLAYER);
    Player *owner = objmgr.GetPlayer(owner_guid);

    // owner exist (online or offline)
    if(owner || objmgr.GetPlayerAccountIdByGUID(owner_guid))
    {
        std::ostringstream msgAuctionSalePendingSubject;
        msgAuctionSalePendingSubject << auction->item_template << ":0:" << AUCTION_SALE_PENDING;

        std::ostringstream msgAuctionSalePendingBody;
        uint32 auctionCut = GetAuctionCut(auction->location, auction->bid);

        time_t distrTime = time(NULL) + HOUR;

        msgAuctionSalePendingBody.width(16);
        msgAuctionSalePendingBody << std::right << std::hex << auction->bidder;
        msgAuctionSalePendingBody << std::dec << ":" << auction->bid << ":" << auction->buyout;
        msgAuctionSalePendingBody << ":" << auction->deposit << ":" << auctionCut << ":0:";
        msgAuctionSalePendingBody << secsToTimeBitFields(distrTime);

        sLog.outDebug("AuctionSalePending body string : %s", msgAuctionSalePendingBody.str().c_str());

        uint32 itemTextId = objmgr.CreateItemText( msgAuctionSalePendingBody.str() );

        WorldSession::SendMailTo(owner, MAIL_AUCTION, MAIL_STATIONERY_AUCTION, auction->location, auction->owner, msgAuctionSalePendingSubject.str(), itemTextId, NULL, 0, 0, MAIL_CHECK_MASK_AUCTION);
    }
}

//call this method to send mail to auction owner, when auction is successful, it does not clear ram
void AuctionHouseMgr::SendAuctionSuccessfulMail( AuctionEntry * auction )
{
    uint64 owner_guid = MAKE_NEW_GUID(auction->owner, 0, HIGHGUID_PLAYER);
    Player *owner = objmgr.GetPlayer(owner_guid);

    uint32 owner_accId = 0;
    if(!owner)
        owner_accId = objmgr.GetPlayerAccountIdByGUID(owner_guid);

    // owner exist
    if(owner || owner_accId)
    {
        std::ostringstream msgAuctionSuccessfulSubject;
        msgAuctionSuccessfulSubject << auction->item_template << ":0:" << AUCTION_SUCCESSFUL;

        std::ostringstream auctionSuccessfulBody;
        uint32 auctionCut = GetAuctionCut(auction->location, auction->bid);

        auctionSuccessfulBody.width(16);
        auctionSuccessfulBody << std::right << std::hex << auction->bidder;
        auctionSuccessfulBody << std::dec << ":" << auction->bid << ":" << auction->buyout;
        auctionSuccessfulBody << ":" << auction->deposit << ":" << auctionCut;

        sLog.outDebug("AuctionSuccessful body string : %s", auctionSuccessfulBody.str().c_str());

        uint32 itemTextId = objmgr.CreateItemText( auctionSuccessfulBody.str() );

        uint32 profit = auction->bid + auction->deposit - auctionCut;

        if (owner)
        {
            //send auction owner notification, bidder must be current!
            owner->GetSession()->SendAuctionOwnerNotification( auction );
        }

        WorldSession::SendMailTo(owner, MAIL_AUCTION, MAIL_STATIONERY_AUCTION, auction->location, auction->owner, msgAuctionSuccessfulSubject.str(), itemTextId, NULL, profit, 0, MAIL_CHECK_MASK_AUCTION, HOUR);
    }
}

//does not clear ram
void AuctionHouseMgr::SendAuctionExpiredMail( AuctionEntry * auction )
{                                                           //return an item in auction to its owner by mail
    Item *pItem = GetAItem(auction->item_guidlow);
    if(!pItem)
    {
        sLog.outError("Auction item (GUID: %u) not found, and lost.",auction->item_guidlow);
        return;
    }

    uint64 owner_guid = MAKE_NEW_GUID(auction->owner, 0, HIGHGUID_PLAYER);
    Player *owner = objmgr.GetPlayer(owner_guid);

    uint32 owner_accId = 0;
    if(!owner)
        owner_accId = objmgr.GetPlayerAccountIdByGUID(owner_guid);

    // owner exist
    if(owner || owner_accId)
    {
        std::ostringstream subject;
        subject << auction->item_template << ":0:" << AUCTION_EXPIRED;

        if ( owner )
            owner->GetSession()->SendAuctionOwnerNotification( auction );
        else
            RemoveAItem(pItem->GetGUIDLow());               // we have to remove the item, before we delete it !!

        MailItemsInfo mi;
        mi.AddItem(auction->item_guidlow, auction->item_template, pItem);

        // will delete item or place to receiver mail list
        WorldSession::SendMailTo(owner, MAIL_AUCTION, MAIL_STATIONERY_AUCTION, auction->location, GUID_LOPART(owner_guid), subject.str(), 0, &mi, 0, 0, MAIL_CHECK_MASK_NONE);
    }
    // owner not found
    else
    {
        CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid='%u'",pItem->GetGUIDLow());
        RemoveAItem(pItem->GetGUIDLow());                   // we have to remove the item, before we delete it !!
        delete pItem;
    }
}

void AuctionHouseMgr::LoadAuctionItems()
{
    // data needs to be at first place for Item::LoadFromDB
    QueryResult *result = CharacterDatabase.Query( "SELECT data,itemguid,item_template FROM auctionhouse JOIN item_instance ON itemguid = guid" );

    if( !result )
    {
        barGoLink bar(1);
        bar.step();
        sLog.outString("");
        sLog.outString(">> Loaded 0 auction items");
        return;
    }

    barGoLink bar( result->GetRowCount() );

    uint32 count = 0;

    Field *fields;
    do
    {
        bar.step();

        fields = result->Fetch();
        uint32 item_guid        = fields[1].GetUInt32();
        uint32 item_template    = fields[2].GetUInt32();

        ItemPrototype const *proto = objmgr.GetItemPrototype(item_template);

        if(!proto)
        {
            sLog.outError( "ObjectMgr::LoadAuctionItems: Unknown item (GUID: %u id: #%u) in auction, skipped.", item_guid,item_template);
            continue;
        }

        Item *item = NewItemOrBag(proto);

        if(!item->LoadFromDB(item_guid,0, result))
        {
            delete item;
            continue;
        }
        AddAItem(item);

        ++count;
    }
    while( result->NextRow() );
    delete result;

    sLog.outString();
    sLog.outString( ">> Loaded %u auction items", count );
}

void AuctionHouseMgr::LoadAuctions()
{
    QueryResult *result = CharacterDatabase.Query("SELECT COUNT(*) FROM auctionhouse");
    if( !result )
    {
        barGoLink bar(1);
        bar.step();
        sLog.outString("");
        sLog.outString(">> Loaded 0 auctions. DB table `auctionhouse` is empty.");
        return;
    }

    Field *fields = result->Fetch();
    uint32 AuctionCount=fields[0].GetUInt32();
    delete result;

    if(!AuctionCount)
    {
        barGoLink bar(1);
        bar.step();
        sLog.outString("");
        sLog.outString(">> Loaded 0 auctions. DB table `auctionhouse` is empty.");
        return;
    }

    result = CharacterDatabase.Query( "SELECT id,auctioneerguid,itemguid,item_template,itemowner,buyoutprice,time,buyguid,lastbid,startbid,deposit,location FROM auctionhouse" );
    if( !result )
    {
        barGoLink bar(1);
        bar.step();
        sLog.outString("");
        sLog.outString(">> Loaded 0 auctions. DB table `auctionhouse` is empty.");
        return;
    }

    barGoLink bar( AuctionCount );

    AuctionEntry *aItem;

    do
    {
        fields = result->Fetch();

        bar.step();

        aItem = new AuctionEntry;
        aItem->Id = fields[0].GetUInt32();
        aItem->auctioneer = fields[1].GetUInt32();
        aItem->item_guidlow = fields[2].GetUInt32();
        aItem->item_template = fields[3].GetUInt32();
        aItem->owner = fields[4].GetUInt32();
        aItem->buyout = fields[5].GetUInt32();
        aItem->time = fields[6].GetUInt32();
        aItem->bidder = fields[7].GetUInt32();
        aItem->bid = fields[8].GetUInt32();
        aItem->startbid = fields[9].GetUInt32();
        aItem->deposit = fields[10].GetUInt32();

        uint32 loc = fields[11].GetUInt8();
        if(!IsValidAuctionLocation(loc))
        {
            CharacterDatabase.PExecute("DELETE FROM auctionhouse WHERE id = '%u'",aItem->Id);
            sLog.outError("Auction %u has wrong auction location (%u)", aItem->Id, loc);
            delete aItem;
            continue;
        }
        aItem->location = AuctionLocation(loc);

        // check if sold item exists for guid
        // and item_template in fact (GetAItem will fail if problematic in result check in ObjectMgr::LoadAuctionItems)
        if ( !GetAItem( aItem->item_guidlow ) )
        {
            CharacterDatabase.PExecute("DELETE FROM auctionhouse WHERE id = '%u'",aItem->Id);
            sLog.outError("Auction %u has not a existing item : %u", aItem->Id, aItem->item_guidlow);
            delete aItem;
            continue;
        }

        if(aItem->location)

            GetAuctionsMap( aItem->location )->AddAuction(aItem);

    } while (result->NextRow());
    delete result;

    sLog.outString();
    sLog.outString( ">> Loaded %u auctions", AuctionCount );
}

void AuctionHouseMgr::AddAItem( Item* it )
{
    ASSERT( it );
    ASSERT( mAitems.find(it->GetGUIDLow()) == mAitems.end());
    mAitems[it->GetGUIDLow()] = it;
}

bool AuctionHouseMgr::RemoveAItem( uint32 id )
{
    ItemMap::iterator i = mAitems.find(id);
    if (i == mAitems.end())
    {
        return false;
    }
    mAitems.erase(i);
    return true;
}