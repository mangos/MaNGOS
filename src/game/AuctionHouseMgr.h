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

#ifndef _AUCTION_HOUSE_MGR_H
#define _AUCTION_HOUSE_MGR_H

#include "SharedDefines.h"
#include "Policies/Singleton.h"

class Item;

#define MIN_AUCTION_TIME (12*HOUR)

enum AuctionError
{
    AUCTION_OK = 0,
    AUCTION_INTERNAL_ERROR = 2,
    AUCTION_NOT_ENOUGHT_MONEY = 3,
    AUCTION_ITEM_NOT_FOUND = 4,
    CANNOT_BID_YOUR_AUCTION_ERROR = 10
};

enum AuctionAction
{
    AUCTION_SELL_ITEM = 0,
    AUCTION_CANCEL = 1,
    AUCTION_PLACE_BID = 2
};

enum AuctionLocation
{
    AUCTION_ALLIANCE = 2,
    AUCTION_HORDE    = 6,
    AUCTION_NEUTRAL  = 7
};

inline bool IsValidAuctionLocation(uint32 loc) { return loc == AUCTION_ALLIANCE || loc == AUCTION_HORDE || loc == AUCTION_NEUTRAL; }

struct AuctionEntry
{
    uint32 Id;
    uint32 auctioneer;
    uint32 item_guidlow;
    uint32 item_template;
    uint32 owner;
    uint32 startbid;                                        //maybe useless
    uint32 bid;
    uint32 buyout;
    time_t time;
    uint32 bidder;
    uint32 deposit;                                         //deposit can be calculated only when creating auction
    AuctionLocation location;
};

//this class is used as auctionhouse instance
class AuctionHouseObject
{
    public:
        AuctionHouseObject() {}
        ~AuctionHouseObject()
        {
            for (AuctionEntryMap::iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
                delete itr->second;
        }

        typedef std::map<uint32, AuctionEntry*> AuctionEntryMap;

        uint32 Getcount() { return AuctionsMap.size(); }

        AuctionEntryMap::iterator GetAuctionsBegin() {return AuctionsMap.begin();}
        AuctionEntryMap::iterator GetAuctionsEnd() {return AuctionsMap.end();}

        void AddAuction(AuctionEntry *ah)
        {
            ASSERT( ah );
            AuctionsMap[ah->Id] = ah;
        }

        AuctionEntry* GetAuction(uint32 id) const
        {
            AuctionEntryMap::const_iterator itr = AuctionsMap.find( id );
            return itr != AuctionsMap.end() ? itr->second : NULL;
        }

        bool RemoveAuction(uint32 id)
        {
            return AuctionsMap.erase(id) ? true : false;
        }

    private:
        AuctionEntryMap AuctionsMap;
};

class AuctionHouseMgr
{
    public:
        AuctionHouseMgr();
        ~AuctionHouseMgr();

        typedef UNORDERED_MAP<uint32, Item*> ItemMap;

        AuctionHouseObject * GetAuctionsMap( AuctionLocation location );

        Item* GetAItem(uint32 id)
        {
            ItemMap::const_iterator itr = mAitems.find(id);
            if (itr != mAitems.end())
            {
                return itr->second;
            }
            return NULL;
        }

        void AddAItem(Item* it);

        bool RemoveAItem(uint32 id);

        //auction messages
        void SendAuctionWonMail( AuctionEntry * auction );
        void SendAuctionSalePendingMail( AuctionEntry * auction );
        void SendAuctionSuccessfulMail( AuctionEntry * auction );
        void SendAuctionExpiredMail( AuctionEntry * auction );
        static uint32 GetAuctionCut( AuctionLocation location, uint32 highBid );
        static uint32 GetAuctionDeposit(AuctionLocation location, uint32 time, Item *pItem);
        static uint32 GetAuctionOutBid(uint32 currentBid);
    public:
        //load first auction items, because of check if item exists, when loading
        void LoadAuctionItems();
        void LoadAuctions();
    private:
        AuctionHouseObject  mHordeAuctions;
        AuctionHouseObject  mAllianceAuctions;
        AuctionHouseObject  mNeutralAuctions;

        ItemMap             mAitems;
};

#define auctionmgr MaNGOS::Singleton<AuctionHouseMgr>::Instance()

#endif
