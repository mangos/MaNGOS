#include "AuctionHouseBot.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "AuctionHouseMgr.h"

#include "Policies/SingletonImp.h"

INSTANTIATE_SINGLETON_1( AuctionHouseBot );

using namespace std;

AuctionHouseBot::AuctionHouseBot()
{
    AHBSeller = 0;
    AHBBuyer = 0;

    Vendor_Items = 0;
    Loot_Items = 0;
    Other_Items = 0;

    No_Bind = 0;
    Bind_When_Picked_Up = 0;
    Bind_When_Equipped = 0;
    Bind_When_Use = 0;
    Bind_Quest_Item = 0;

    AllianceConfig = AHBConfig(2);
    HordeConfig = AHBConfig(6);
    NeutralConfig = AHBConfig(7);
}

AuctionHouseBot::~AuctionHouseBot()
{

}

void AuctionHouseBot::addNewAuctions(Player *AHBplayer, AHBConfig *config)
{
    if (!AHBSeller)
        return;
    AuctionHouseEntry const* ahEntry = sAuctionHouseStore.LookupEntry(config->GetAHID());
    AuctionHouseObject* auctionHouse = sAuctionMgr.GetAuctionsMap(ahEntry);
    uint32 items = 0;
    uint32 minItems = config->GetMinItems();
    uint32 maxItems = config->GetMaxItems();
    uint32 auctions = auctionHouse->Getcount();
    uint32 AuctioneerGUID = 0;
    switch (config->GetAHID()){
        case 2:
            AuctioneerGUID = 79707; //Human in stormwind.
            break;
        case 6:
            AuctioneerGUID = 4656; //orc in Orgrimmar
            break;
        case 7:
            AuctioneerGUID = 23442; //goblin in GZ
            break;
        default:
            sLog.outError("GetAHID() - Default switch reached");
            AuctioneerGUID = 23442; //default to neutral 7
            break;
    }

    if (auctions >= minItems)
        return;

    if (auctions <= maxItems)
    {
        if ((maxItems - auctions) > ItemsPerCycle)
            items = ItemsPerCycle;
        else
            items = (maxItems - auctions);
    }
    uint32 greyTGcount = config->GetPercents(AHB_GREY_TG);
    uint32 whiteTGcount = config->GetPercents(AHB_WHITE_TG);
    uint32 greenTGcount = config->GetPercents(AHB_GREEN_TG);
    uint32 blueTGcount = config->GetPercents(AHB_BLUE_TG);
    uint32 purpleTGcount = config->GetPercents(AHB_PURPLE_TG);
    uint32 orangeTGcount = config->GetPercents(AHB_ORANGE_TG);
    uint32 yellowTGcount = config->GetPercents(AHB_YELLOW_TG);
    uint32 greyIcount = config->GetPercents(AHB_GREY_I);
    uint32 whiteIcount = config->GetPercents(AHB_WHITE_I);
    uint32 greenIcount = config->GetPercents(AHB_GREEN_I);
    uint32 blueIcount = config->GetPercents(AHB_BLUE_I);
    uint32 purpleIcount = config->GetPercents(AHB_PURPLE_I);
    uint32 orangeIcount = config->GetPercents(AHB_ORANGE_I);
    uint32 yellowIcount = config->GetPercents(AHB_YELLOW_I);
    uint32 total = greyTGcount + whiteTGcount + greenTGcount + blueTGcount
        + purpleTGcount + orangeTGcount + yellowTGcount
        + whiteIcount + greenIcount + blueIcount + purpleIcount
        + orangeIcount + yellowIcount;

    uint32 greyTGoods = 0;
    uint32 whiteTGoods = 0;
    uint32 greenTGoods = 0;
    uint32 blueTGoods = 0;
    uint32 purpleTGoods = 0;
    uint32 orangeTGoods = 0;
    uint32 yellowTGoods = 0;

    uint32 greyItems = 0;
    uint32 whiteItems = 0;
    uint32 greenItems = 0;
    uint32 blueItems = 0;
    uint32 purpleItems = 0;
    uint32 orangeItems = 0;
    uint32 yellowItems = 0;
    uint32 buyBondingK = 1;

    for (AuctionHouseObject::AuctionEntryMap::const_iterator itr = auctionHouse->GetAuctionsBegin();itr != auctionHouse->GetAuctionsEnd();++itr)
    {
        AuctionEntry *Aentry = itr->second;
        Item *item = sAuctionMgr.GetAItem(Aentry->item_guidlow);
        if (item)
        {
            ItemPrototype const *prototype = item->GetProto();
            if (prototype)
            {
                switch (prototype->Quality)
                {
                case 0:
                    if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                        ++greyTGoods;
                    else
                        ++greyItems;
                    break;
                case 1:
                    if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                        ++whiteTGoods;
                    else
                        ++whiteItems;
                    break;
                case 2:
                    if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                        ++greenTGoods;
                    else
                        ++greenItems;
                    break;
                case 3:
                    if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                        ++blueTGoods;
                    else
                        ++blueItems;
                    break;
                case 4:
                    if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                        ++purpleTGoods;
                    else
                        ++purpleItems;
                    break;
                case 5:
                    if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                        ++orangeTGoods;
                    else
                        ++orangeItems;
                    break;
                case 6:
                    if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                        ++yellowTGoods;
                    else
                        ++yellowItems;
                    break;
                }
            }
        }
    }

    // only insert a few at a time, so as not to peg the processor
    for (uint32 cnt = 1;cnt <= items;cnt++)
    {
        uint32 itemID = 0;
        uint32 loopBreaker = 0;                     // This will prevent endless looping condition where AHBot
        while (itemID == 0 && loopBreaker < 50)     //  cannot allocate an item.
        {
            uint32 choice = urand(0, 13);
            switch (choice)
            {
            case 0:
                if ((greyItemsBin.size() > 0) && (greyItems < greyIcount))
                {
                    itemID = greyItemsBin[urand(0, greyItemsBin.size() - 1)];
                    ++greyItems;
                }
                break;
            case 1:
                if ((whiteItemsBin.size() > 0) && (whiteItems < whiteIcount))
                {
                    itemID = whiteItemsBin[urand(0, whiteItemsBin.size() - 1)];
                    ++whiteItems;
                }
                break;
            case 2:
                if ((greenItemsBin.size() > 0) && (greenItems < greenIcount))
                {
                    itemID = greenItemsBin[urand(0, greenItemsBin.size() - 1)];
                    ++greenItems;
                }
                break;
            case 3:
                if ((blueItemsBin.size() > 0) && (blueItems < blueIcount))
                {
                    itemID = blueItemsBin[urand(0, blueItemsBin.size() - 1)];
                    ++blueItems;
                }
                break;
            case 4:
                if ((purpleItemsBin.size() > 0) && (purpleItems < purpleIcount))
                {
                    itemID = purpleItemsBin[urand(0, purpleItemsBin.size() - 1)];
                    ++purpleItems;
                }
                break;
            case 5:
                if ((orangeItemsBin.size() > 0) && (orangeItems < orangeIcount))
                {
                    itemID = orangeItemsBin[urand(0, orangeItemsBin.size() - 1)];
                    ++orangeItems;
                }
                break;
            case 6:
                if ((yellowItemsBin.size() > 0) && (yellowItems < yellowIcount))
                {
                    itemID = yellowItemsBin[urand(0, yellowItemsBin.size() - 1)];
                    ++yellowItems;
                }
                break;
            case 7:
                if ((greyTradeGoodsBin.size() > 0) && (greyTGoods < greyTGcount))
                {
                    itemID = whiteTradeGoodsBin[urand(0, whiteTradeGoodsBin.size() - 1)];
                    ++greyTGoods;
                }
                break;
            case 8:
                if ((whiteTradeGoodsBin.size() > 0) && (whiteTGoods < whiteTGcount))
                {
                    itemID = whiteTradeGoodsBin[urand(0, whiteTradeGoodsBin.size() - 1)];
                    ++whiteTGoods;
                }
                break;
            case 9:
                if ((greenTradeGoodsBin.size() > 0) && (greenTGoods < greenTGcount))
                {
                    itemID = greenTradeGoodsBin[urand(0, greenTradeGoodsBin.size() - 1)];
                    ++greenTGoods;
                }
                break;
            case 10:
                if ((blueTradeGoodsBin.size() > 0) && (blueTGoods < blueTGcount))
                {
                    itemID = blueTradeGoodsBin[urand(0, blueTradeGoodsBin.size() - 1)];
                    ++blueTGoods;
                }
                break;
            case 11:
                if ((purpleTradeGoodsBin.size() > 0) && (purpleTGoods < purpleTGcount))
                {
                    itemID = purpleTradeGoodsBin[urand(0, purpleTradeGoodsBin.size() - 1)];
                    ++purpleTGoods;
                }
                break;
            case 12:
                if ((orangeTradeGoodsBin.size() > 0) && (orangeTGoods < orangeTGcount))
                {
                    itemID = orangeTradeGoodsBin[urand(0, orangeTradeGoodsBin.size() - 1)];
                    ++orangeTGoods;
                }
                break;
            case 13:
                if ((yellowTradeGoodsBin.size() > 0) && (yellowTGoods < yellowTGcount))
                {
                    itemID = yellowTradeGoodsBin[urand(0, yellowTradeGoodsBin.size() - 1)];
                    ++yellowTGoods;
                }
                break;
            default:
                sLog.outString("AuctionHouseBot: itemID Switch - Default Reached");
                break;
            }

            ++loopBreaker;
        }

        if (itemID == 0)
        {
            if (debug_Out)
                sLog.outString("AuctionHouseBot: Item::CreateItem() - Unable to find item");
            continue;
        }

        ItemPrototype const* prototype = sObjectMgr.GetItemPrototype(itemID);
        if (prototype == NULL)
        {
            sLog.outString("AuctionHouseBot: Huh?!?! prototype == NULL");
            continue;
        }

        Item* item = Item::CreateItem(itemID, 1, AHBplayer);
        item->AddToUpdateQueueOf(AHBplayer);
        if (item == NULL)
        {
            sLog.outString("AuctionHouseBot: Item::CreateItem() returned NULL");
            break;
        }

        uint32 randomPropertyId = Item::GenerateItemRandomPropertyId(itemID);
        if (randomPropertyId != 0)
            item->SetItemRandomProperties(randomPropertyId);

        uint32 buyoutPrice;
        uint32 bidPrice = 0;
        uint32 stackCount = urand(1, item->GetMaxStackCount());

        switch (prototype->Bonding)
        {
                case NO_BIND:
                       buyBondingK = No_Bind;
                       break;
                case BIND_WHEN_PICKED_UP:
                       buyBondingK = Bind_When_Picked_Up;
                       break;
                case BIND_WHEN_EQUIPPED:
                       buyBondingK = Bind_When_Equipped;
                       break;
                case BIND_WHEN_USE:
                       buyBondingK = Bind_When_Use;
                       break;
                case BIND_QUEST_ITEM:
                       buyBondingK = Bind_Quest_Item;
                       break;
                default:
                       buyBondingK = 1;
        }

        switch (SellMethod)
        {
        case 0:
            if ( prototype->SellPrice > 8 )
            buyoutPrice  = prototype->SellPrice * item->GetCount();
            else buyoutPrice  = prototype->BuyPrice * item->GetCount() / 8 ;
            break;
        case 1:
            if ( prototype->BuyPrice > 1)
            buyoutPrice  = prototype->BuyPrice * item->GetCount();
            else buyoutPrice  = prototype->SellPrice * item->GetCount() * 8 ;
            break;
        }

        switch (prototype->Quality)
        {
        case 0:
            if (config->GetMaxStack(AHB_GREY) != 0)
            {
                stackCount = urand(1, minValue(item->GetMaxStackCount(), config->GetMaxStack(AHB_GREY)));
            }
            buyoutPrice *= urand(config->GetMinPrice(AHB_GREY), config->GetMaxPrice(AHB_GREY)) * stackCount;
            buyoutPrice /= 100;
            bidPrice = buyoutPrice * urand(config->GetMinBidPrice(AHB_GREY), config->GetMaxBidPrice(AHB_GREY));
            bidPrice /= 100;
            break;
        case 1:
            if (config->GetMaxStack(AHB_WHITE) != 0)
            {
                stackCount = urand(1, minValue(item->GetMaxStackCount(), config->GetMaxStack(AHB_WHITE)));
            }
            buyoutPrice *= urand(config->GetMinPrice(AHB_WHITE), config->GetMaxPrice(AHB_WHITE)) * stackCount;
            buyoutPrice /= 100;
            bidPrice = buyoutPrice * urand(config->GetMinBidPrice(AHB_WHITE), config->GetMaxBidPrice(AHB_WHITE));
            bidPrice /= 100;
            break;
        case 2:
            if (config->GetMaxStack(AHB_GREEN) != 0)
            {
                stackCount = urand(1, minValue(item->GetMaxStackCount(), config->GetMaxStack(AHB_GREEN)));
            }
            buyoutPrice *= urand(config->GetMinPrice(AHB_GREEN), config->GetMaxPrice(AHB_GREEN)) * stackCount;
            buyoutPrice /= 100;
            bidPrice = buyoutPrice * urand(config->GetMinBidPrice(AHB_GREEN), config->GetMaxBidPrice(AHB_GREEN));
            bidPrice /= 100;
            break;
        case 3:
            if (config->GetMaxStack(AHB_BLUE) != 0)
            {
                stackCount = urand(1, minValue(item->GetMaxStackCount(), config->GetMaxStack(AHB_BLUE)));
            }
            buyoutPrice *= urand(config->GetMinPrice(AHB_BLUE), config->GetMaxPrice(AHB_BLUE)) * stackCount;
            buyoutPrice /= 100;
            bidPrice = buyoutPrice * urand(config->GetMinBidPrice(AHB_BLUE), config->GetMaxBidPrice(AHB_BLUE));
            bidPrice /= 100;
            break;
        case 4:
            if (config->GetMaxStack(AHB_PURPLE) != 0)
            {
                stackCount = urand(1, minValue(item->GetMaxStackCount(), config->GetMaxStack(AHB_PURPLE)));
            }
            buyoutPrice *= urand(config->GetMinPrice(AHB_PURPLE), config->GetMaxPrice(AHB_PURPLE)) * stackCount;
            buyoutPrice /= 100;
            bidPrice = buyoutPrice * urand(config->GetMinBidPrice(AHB_PURPLE), config->GetMaxBidPrice(AHB_PURPLE));
            bidPrice /= 100;
            break;
        case 5:
            if (config->GetMaxStack(AHB_ORANGE) != 0)
            {
                stackCount = urand(1, minValue(item->GetMaxStackCount(), config->GetMaxStack(AHB_ORANGE)));
            }
            buyoutPrice *= urand(config->GetMinPrice(AHB_ORANGE), config->GetMaxPrice(AHB_ORANGE)) * stackCount;
            buyoutPrice /= 100;
            bidPrice = buyoutPrice * urand(config->GetMinBidPrice(AHB_ORANGE), config->GetMaxBidPrice(AHB_ORANGE));
            bidPrice /= 100;
            break;
        case 6:
            if (config->GetMaxStack(AHB_YELLOW) != 0)
            {
                stackCount = urand(1, minValue(item->GetMaxStackCount(), config->GetMaxStack(AHB_YELLOW)));
            }
            buyoutPrice *= urand(config->GetMinPrice(AHB_YELLOW), config->GetMaxPrice(AHB_YELLOW)) * stackCount;
            buyoutPrice /= 100;
            bidPrice = buyoutPrice * urand(config->GetMinBidPrice(AHB_YELLOW), config->GetMaxBidPrice(AHB_YELLOW));
            bidPrice /= 100;
            break;
        }

        item->SetCount(stackCount);

        AuctionEntry* auctionEntry = new AuctionEntry;
        auctionEntry->Id = sObjectMgr.GenerateAuctionID();
        auctionEntry->item_guidlow = item->GetGUIDLow();
        auctionEntry->item_template = item->GetEntry();
        auctionEntry->owner = AHBplayer->GetGUIDLow();
        auctionEntry->startbid = bidPrice * buyBondingK;
        auctionEntry->buyout = buyoutPrice * buyBondingK;
        auctionEntry->bidder = 0;
        auctionEntry->bid = 0;
        auctionEntry->deposit = 0;
        auctionEntry->expire_time = (time_t) (urand(config->GetMinTime(), config->GetMaxTime()) * 60 * 60 + time(NULL));
        auctionEntry->auctionHouseEntry = ahEntry;
        item->SaveToDB();
        item->RemoveFromUpdateQueueOf(AHBplayer);
        sAuctionMgr.AddAItem(item);
        auctionHouse->AddAuction(auctionEntry);
        auctionEntry->SaveToDB();
    }
}

void AuctionHouseBot::addNewAuctionBuyerBotBid(Player *AHBplayer, AHBConfig *config, WorldSession *session)
{
    if (!AHBBuyer)
        return;

    // Fetches content of selected AH
    AuctionHouseEntry const* ahEntry = sAuctionHouseStore.LookupEntry(config->GetAHID());
    AuctionHouseObject* auctionHouse = sAuctionMgr.GetAuctionsMap(ahEntry);
    vector<uint32> possibleBids;

    for (AuctionHouseObject::AuctionEntryMap::const_iterator itr = auctionHouse->GetAuctionsBegin();itr != auctionHouse->GetAuctionsEnd();++itr)
    {
        // Check if the auction is ours
        // if it is, we skip this iteration.
        if (ObjectGuid(HIGHGUID_PLAYER,itr->second->owner) == AHBplayerGUID)
        {
            continue;
        }
        // Check that we haven't bidded in this auction already.
        if (ObjectGuid(HIGHGUID_PLAYER,itr->second->bidder) != AHBplayerGUID)
        {
            uint32 tmpdata = itr->second->Id;
            possibleBids.push_back(tmpdata);
        }
    }

    for (uint32 count = 0;count < config->GetBidsPerInterval();++count)
    {

        // Do we have anything to bid? If not, stop here.
        if (possibleBids.empty())
        {
            count = config->GetBidsPerInterval();
            continue;
        }

        // Choose random auction from possible auctions
        uint32 vectorPos = urand(0, possibleBids.size() - 1);
        uint32 auctionID = possibleBids[vectorPos];

        // Erase the auction from the vector to prevent bidding on item in next iteration.
        vector<uint32>::iterator iter = possibleBids.begin();
        advance(iter, vectorPos);
        possibleBids.erase(iter);

        // from auctionhousehandler.cpp, creates auction pointer & player pointer
        AuctionEntry* auction = auctionHouse->GetAuction(auctionID);
        if (!auction)
        {
            sLog.outError("Item doesn't exists, perhaps bought already?");
            continue;
        }

        // get exact item information
        Item *pItem = sAuctionMgr.GetAItem(auction->item_guidlow);
        if (!pItem)
        {
            sLog.outError("Item doesn't exists, perhaps bought already?");
            continue;
        }

        // get item prototype
        ItemPrototype const* prototype = sObjectMgr.GetItemPrototype(auction->item_template);

        // check which price we have to use, startbid or if it is bidded already
        if (debug_Out)
        {
            sLog.outError("Auction Number: %u", auction->Id);
            sLog.outError("Item Template: %u", auction->item_template);
            sLog.outError("Buy Price: %u", prototype->BuyPrice);
            sLog.outError("Sell Price: %u", prototype->SellPrice);
            sLog.outError("Quality: %u", prototype->Quality);
        }
        uint32 currentprice;
        if (auction->bid)
        {
            currentprice = auction->bid;
            if (debug_Out)
            {
                sLog.outError("Current Price: %u", auction->bid);
            }
        }
        else
        {
            currentprice = auction->startbid;
            if (debug_Out)
            {
                sLog.outError("Current Price: %u", auction->startbid);
            }
        }
        uint32 bidprice;

        // Prepare portion from maximum bid
        uint32 tmprate2 = urand(0, 100);
        double tmprate = static_cast<double>(tmprate2);
        if (debug_Out)
        {
            sLog.outError("tmprate: %f", tmprate);
        }

        double bidrate = tmprate / 100;
        if (debug_Out)
        {
            sLog.outError("bidrate: %f", bidrate);
        }

        long double bidMax = 0;

        long double protoSellPrice = prototype->SellPrice;
        long double protoBuyPrice  = prototype->BuyPrice;

        if ( protoSellPrice <= 10000 && protoBuyPrice  <= 10000) // вещи меньше 1г не рассматриваем
        {
            continue;
        }

        if (protoBuyPrice <= 1)
        {
            protoBuyPrice = protoSellPrice * 8 ;
        }

        if (protoSellPrice <= 1)
        {
            protoSellPrice = protoBuyPrice / 8 ;
        }

        // check that bid has acceptable value and take bid based on vendorprice, stacksize and quality
        switch (BuyMethod)
        {
        case 0:
            switch (prototype->Quality)
            {
            case 0:
                if (currentprice < protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_GREY))
                {
                    bidMax = protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_GREY);
                }
                break;
            case 1:
                if (currentprice < protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_WHITE))
                {
                    bidMax = protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_WHITE);
                }
                break;
            case 2:
                if (currentprice < protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_GREEN))
                {
                    bidMax = protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_GREEN);
                }
                break;
            case 3:
                if (currentprice < protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_BLUE))
                {
                    bidMax = protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_BLUE);
                }
                break;
            case 4:
                if (currentprice < protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_PURPLE))
                {
                    bidMax = protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_PURPLE);
                }
                break;
            case 5:
                if (currentprice < protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_ORANGE))
                {
                    bidMax = protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_ORANGE);
                }
                break;
            case 6:
                if (currentprice < protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_YELLOW))
                {
                    bidMax = protoSellPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_YELLOW);
                }
                break;
            default:
                // quality is something it shouldn't be, let's get out of here
                if (debug_Out)
                {
                    sLog.outError("bidMax(fail): %f", bidMax);
                }
                continue;
                break;
            }
            break;
        case 1:
            switch (prototype->Quality)
            {
            case 0:
                if (currentprice < protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_GREY))
                {
                    bidMax = protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_GREY);
                }
                break;
            case 1:
                if (currentprice < prototype->BuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_WHITE))
                {
                    bidMax = protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_WHITE);
                }
                break;
            case 2:
                if (currentprice < protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_GREEN))
                {
                    bidMax = protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_GREEN);
                }
                break;
            case 3:
                if (currentprice < protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_BLUE))
                {
                    bidMax = protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_BLUE);
                }
                break;
            case 4:
                if (currentprice < protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_PURPLE))
                {
                    bidMax = protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_PURPLE);
                }
                break;
            case 5:
                if (currentprice < protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_ORANGE))
                {
                    bidMax = protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_ORANGE);
                }
                break;
            case 6:
                if (currentprice < protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_YELLOW))
                {
                    bidMax = protoBuyPrice * pItem->GetCount() * config->GetBuyerPrice(AHB_YELLOW);
                }
                break;
            default:
                // quality is something it shouldn't be, let's get out of here
                if (debug_Out)
                {
                    sLog.outError("bidMax(fail): %f", bidMax);
                }
                continue;
                break;
            }
            break;
        }

        if (debug_Out)
        {
            sLog.outError("bidMax(succeed): %f", bidMax);
        }

        // check some special items, and do recalculating to their prices
        switch (prototype->Class)
        {
            // ammo
        case 6:
            bidMax = 0;
            break;
        default:
            break;
        }

        if (bidMax == 0)
        {
            // quality check failed to get bidmax, let's get out of here
            continue;
        }

        // Calculate our bid
        long double bidvalue = currentprice + ((bidMax - currentprice) * bidrate);
        if (debug_Out)
        {
            sLog.outError("bidvalue: %f", bidvalue);
        }

        // Convert to uint32
        bidprice = static_cast<uint32>(bidvalue);
        if (debug_Out)
        {
            sLog.outError("bidprice: %u", bidprice);
        }

        // Check our bid is high enough to be valid. If not, correct it to minimum.
        if ((currentprice + auction->GetAuctionOutBid()) > bidprice)
        {
            bidprice = currentprice + auction->GetAuctionOutBid();
            if (debug_Out)
            {
                sLog.outError("bidprice(>): %u", bidprice);
            }
        }

        // Check wether we do normal bid, or buyout
        if ((bidprice < auction->buyout) || (auction->buyout == 0))
        {

            if (auction->bidder > 0)
            {
                if (auction->bidder == AHBplayer->GetGUIDLow())
                {
                    //pl->ModifyMoney(-int32(price - auction->bid));
                }
                else
                {
                    // mail to last bidder and return money
                    session->SendAuctionOutbiddedMail(auction , bidprice);
                    //pl->ModifyMoney(-int32(price));
                }
            }

            auction->bidder = AHBplayer->GetGUIDLow();
            auction->bid = bidprice;

            // Saving auction into database
            CharacterDatabase.PExecute("UPDATE `auction` SET buyguid = '%u',lastbid = '%u' WHERE id = '%u'", auction->bidder, auction->bid, auction->Id);
        }
        else
        {
            //buyout
            if (AHBplayer->GetGUIDLow() == auction->bidder)
            {
                //pl->ModifyMoney(-int32(auction->buyout - auction->bid));
            }
            else
            {
                //pl->ModifyMoney(-int32(auction->buyout));
                if (auction->bidder)
                {
                    session->SendAuctionOutbiddedMail(auction, auction->buyout);
                }
            }
            auction->bidder = AHBplayer->GetGUIDLow();
            auction->bid = auction->buyout;

            // Send mails to buyer & seller
            sAuctionMgr.SendAuctionSuccessfulMail(auction);
            sAuctionMgr.SendAuctionWonMail(auction);

            // Remove item from auctionhouse
            sAuctionMgr.RemoveAItem(auction->item_guidlow);
            // Remove auction
            auctionHouse->RemoveAuction(auction->Id);
            // Remove from database
            auction->DeleteFromDB();
        }
    }
}

void AuctionHouseBot::Update()
{
    time_t _newrun = time(NULL);
    if ((!AHBSeller) && (!AHBBuyer))
        return;

    WorldSession _session(AHBplayerAccount, NULL, SEC_PLAYER, true, 0, LOCALE_enUS);
    Player _AHBplayer(&_session);
    _AHBplayer.MinimalLoadFromDB(AHBplayerGUID.GetCounter());
    ObjectAccessor::Instance().AddObject(&_AHBplayer);

    // Add New Bids
    if (!sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
    {
        addNewAuctions(&_AHBplayer, &AllianceConfig);
        if (((_newrun - _lastrun_a) > (AllianceConfig.GetBiddingInterval() * 60)) && (AllianceConfig.GetBidsPerInterval() > 0))
        {
            addNewAuctionBuyerBotBid(&_AHBplayer, &AllianceConfig, &_session);
            _lastrun_a = _newrun;
        }

        addNewAuctions(&_AHBplayer, &HordeConfig);
        if (((_newrun - _lastrun_h) > (HordeConfig.GetBiddingInterval() *60)) && (HordeConfig.GetBidsPerInterval() > 0))
        {
            addNewAuctionBuyerBotBid(&_AHBplayer, &HordeConfig, &_session);
            _lastrun_h = _newrun;
        }
    }
    addNewAuctions(&_AHBplayer, &NeutralConfig);
    if (((_newrun - _lastrun_n) > (NeutralConfig.GetBiddingInterval() * 60)) && (NeutralConfig.GetBidsPerInterval() > 0))
    {
        addNewAuctionBuyerBotBid(&_AHBplayer, &NeutralConfig, &_session);
        _lastrun_n = _newrun;
    }

    ObjectAccessor::Instance().RemoveObject(&_AHBplayer);
}

void AuctionHouseBot::Initialize()
{
    AHBSeller = sConfig.GetBoolDefault("AuctionHouseBot.EnableSeller", 0);
    AHBBuyer = sConfig.GetBoolDefault("AuctionHouseBot.EnableBuyer", 0);
    AHBplayerAccount = sConfig.GetIntDefault("AuctionHouseBot.Account", 0);
    AHBplayerGUID = sConfig.GetIntDefault("AuctionHouseBot.GUID", 0);
    debug_Out = sConfig.GetIntDefault("AuctionHouseBot.DEBUG", 0);
    No_Bind = sConfig.GetIntDefault("AuctionHouseBot.No_Bind", 1);
    Bind_When_Picked_Up = sConfig.GetIntDefault("AuctionHouseBot.Bind_When_Picked_Up", 0);
    Bind_When_Equipped = sConfig.GetIntDefault("AuctionHouseBot.Bind_When_Equipped", 1);
    Bind_When_Use = sConfig.GetIntDefault("AuctionHouseBot.Bind_When_Use", 1);
    Bind_Quest_Item = sConfig.GetIntDefault("AuctionHouseBot.Bind_Quest_Item", 0);
    ItemsPerCycle = sConfig.GetIntDefault("AuctionHouseBot.ItemsPerCycle", 200);
    SellMethod = sConfig.GetIntDefault("AuctionHouseBot.UseBuyPriceForSeller", 1);
    BuyMethod = sConfig.GetIntDefault("AuctionHouseBot.UseBuyPriceForBuyer", 0);
    MaxItemLevel = sConfig.GetIntDefault("AuctionHouseBot.MaxItemLevel", 200);

    if (!sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
    {
        LoadValues(&AllianceConfig);
        LoadValues(&HordeConfig);
    }
    LoadValues(&NeutralConfig);

    if (AHBSeller)
    {
        Vendor_Items = sConfig.GetBoolDefault("AuctionHouseBot.VendorItems", 0);
        Loot_Items = sConfig.GetBoolDefault("AuctionHouseBot.LootItems", 1);
        Other_Items = sConfig.GetBoolDefault("AuctionHouseBot.OtherItems", 0);

        QueryResult* results = (QueryResult*) NULL;
        char npcQuery[] = "SELECT distinct `item` FROM `npc_vendor`";
        results = WorldDatabase.PQuery(npcQuery);
        if (results != NULL)
        {
            do
            {
                Field* fields = results->Fetch();
                npcItems.push_back(fields[0].GetUInt32());

            } while (results->NextRow());

            delete results;
        }
        else
        {
            sLog.outString("AuctionHouseBot: \"%s\" failed", npcQuery);
        }

        char lootQuery[] = "SELECT `item` FROM `creature_loot_template` UNION "
            "SELECT `item` FROM `disenchant_loot_template` UNION "
            "SELECT `item` FROM `fishing_loot_template` UNION "
            "SELECT `item` FROM `gameobject_loot_template` UNION "
            "SELECT `item` FROM `item_loot_template` UNION "
            "SELECT `item` FROM `milling_loot_template` UNION "
            "SELECT `item` FROM `pickpocketing_loot_template` UNION "
            "SELECT `item` FROM `prospecting_loot_template` UNION "
            "SELECT `item` FROM `skinning_loot_template`";

        results = WorldDatabase.PQuery(lootQuery);
        if (results != NULL)
        {
            do
            {
                Field* fields = results->Fetch();
                lootItems.push_back(fields[0].GetUInt32());

            } while (results->NextRow());

            delete results;
        }
        else
        {
            sLog.outString("AuctionHouseBot: \"%s\" failed", lootQuery);
        }

        for (uint32 itemID = 0; itemID < sItemStorage.MaxEntry; itemID++)
        {
            ItemPrototype const* prototype = sObjectMgr.GetItemPrototype(itemID);

            if (prototype == NULL)
                continue;

            if (prototype->ItemLevel >= MaxItemLevel)
                continue;

            switch (prototype->Bonding)
            {
            case NO_BIND:
                if (No_Bind == 0)
                    continue;
                break;
            case BIND_WHEN_PICKED_UP:
                if (Bind_When_Picked_Up == 0)
                    continue;
                break;
            case BIND_WHEN_EQUIPPED:
                if (Bind_When_Equipped == 0)
                    continue;
                break;
            case BIND_WHEN_USE:
                if (Bind_When_Use == 0)
                    continue;
                break;
            case BIND_QUEST_ITEM:
                if (Bind_Quest_Item == 0)
                    continue;
                break;
            default:
                continue;
                break;
            }

            switch (SellMethod)
            {
            case 0:
                if (prototype->SellPrice == 0)
                    continue;
                break;
            case 1:
                if (prototype->BuyPrice == 0)
                    continue;
                break;
            }

            if ((prototype->Quality < 0) || (prototype->Quality > 6))
                continue;

            if (Vendor_Items == 0)
            {
                bool isVendorItem = false;

                for (unsigned int i = 0; (i < npcItems.size()) && (!isVendorItem); i++)
                {
                    if (itemID == npcItems[i])
                        isVendorItem = true;
                }

                if (isVendorItem)
                    continue;
            }

            if (Loot_Items == 0)
            {
                bool isLootItem = false;

                for (unsigned int i = 0; (i < lootItems.size()) && (!isLootItem); i++)
                {
                    if (itemID == lootItems[i])
                        isLootItem = true;
                }

                if (isLootItem)
                    continue;
            }

            if (Other_Items == 0)
            {
                bool isVendorItem = false;
                bool isLootItem = false;

                for (unsigned int i = 0; (i < npcItems.size()) && (!isVendorItem); i++)
                {
                    if (itemID == npcItems[i])
                        isVendorItem = true;
                }
                for (unsigned int i = 0; (i < lootItems.size()) && (!isLootItem); i++)
                {
                    if (itemID == lootItems[i])
                        isLootItem = true;
                }
                if ((!isLootItem) && (!isVendorItem))
                    continue;
            }

            switch (prototype->Quality)
            {
            case 0:
                if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                    greyTradeGoodsBin.push_back(itemID);
                else
                    greyItemsBin.push_back(itemID);
                break;

            case 1:
                if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                    whiteTradeGoodsBin.push_back(itemID);
                else
                    whiteItemsBin.push_back(itemID);
                break;

            case 2:
                if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                    greenTradeGoodsBin.push_back(itemID);
                else
                    greenItemsBin.push_back(itemID);
                break;

            case 3:
                if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                    blueTradeGoodsBin.push_back(itemID);
                else
                    blueItemsBin.push_back(itemID);
                break;

            case 4:
                if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                    purpleTradeGoodsBin.push_back(itemID);
                else
                    purpleItemsBin.push_back(itemID);
                break;

            case 5:
                if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                    orangeTradeGoodsBin.push_back(itemID);
                else
                    orangeItemsBin.push_back(itemID);
                break;

            case 6:
                if (prototype->Class == ITEM_CLASS_TRADE_GOODS)
                    yellowTradeGoodsBin.push_back(itemID);
                else
                    yellowItemsBin.push_back(itemID);
                break;
            }
        }

        if (
            (greyTradeGoodsBin.size() == 0) &&
            (whiteTradeGoodsBin.size() == 0) &&
            (greenTradeGoodsBin.size() == 0) &&
            (blueTradeGoodsBin.size() == 0) &&
            (purpleTradeGoodsBin.size() == 0) &&
            (orangeTradeGoodsBin.size() == 0) &&
            (yellowTradeGoodsBin.size() == 0) &&
            (greyItemsBin.size() == 0) &&
            (whiteItemsBin.size() == 0) &&
            (greenItemsBin.size() == 0) &&
            (blueItemsBin.size() == 0) &&
            (purpleItemsBin.size() == 0) &&
            (orangeItemsBin.size() == 0) &&
            (yellowItemsBin.size() == 0)
            )
        {
            sLog.outString("AuctionHouseBot: No items");
            AHBSeller = 0;
        }

        sLog.outString("AuctionHouseBot loading progress:");
        sLog.outString("loaded %d grey trade goods", greyTradeGoodsBin.size());
        sLog.outString("loaded %d white trade goods", whiteTradeGoodsBin.size());
        sLog.outString("loaded %d green trade goods", greenTradeGoodsBin.size());
        sLog.outString("loaded %d blue trade goods", blueTradeGoodsBin.size());
        sLog.outString("loaded %d purple trade goods", purpleTradeGoodsBin.size());
        sLog.outString("loaded %d orange trade goods", orangeTradeGoodsBin.size());
        sLog.outString("loaded %d yellow trade goods", yellowTradeGoodsBin.size());
        sLog.outString("loaded %d grey items", greyItemsBin.size());
        sLog.outString("loaded %d white items", whiteItemsBin.size());
        sLog.outString("loaded %d green items", greenItemsBin.size());
        sLog.outString("loaded %d blue items", blueItemsBin.size());
        sLog.outString("loaded %d purple items", purpleItemsBin.size());
        sLog.outString("loaded %d orange items", orangeItemsBin.size());
        sLog.outString("loaded %d yellow items", yellowItemsBin.size());
    }
    sLog.outString("AHBot-AHBuyer modified by /dev/rsa is now loaded");
}

void AuctionHouseBot::Commands(uint32 command, uint32 ahMapID, uint32 col, char* args)
{
    AHBConfig *config;
    switch (ahMapID)
    {
    case 2:
        config = &AllianceConfig;
        break;
    case 6:
        config = &HordeConfig;
        break;
    case 7:
        config = &NeutralConfig;
        break;
    }
    std::string color;
    switch (col)
    {
    case AHB_GREY:
        color = "grey";
        break;
    case AHB_WHITE:
        color = "white";
        break;
    case AHB_GREEN:
        color = "green";
        break;
    case AHB_BLUE:
        color = "blue";
        break;
    case AHB_PURPLE:
        color = "purple";
        break;
    case AHB_ORANGE:
        color = "orange";
        break;
    case AHB_YELLOW:
        color = "yellow";
        break;
    default:
        break;
    }
    switch (command)
    {
    case 0:     //ahexpire
        {
            AuctionHouseEntry const* ahEntry = sAuctionHouseStore.LookupEntry(config->GetAHID());
            AuctionHouseObject* auctionHouse = sAuctionMgr.GetAuctionsMap(ahEntry);

            AuctionHouseObject::AuctionEntryMap::iterator itr;
            itr = auctionHouse->GetAuctionsBegin();

            while (itr != auctionHouse->GetAuctionsEnd())
            {
                if (ObjectGuid(HIGHGUID_PLAYER,itr->second->owner) == AHBplayerGUID)
                    itr->second->expire_time = sWorld.GetGameTime();

                ++itr;
            }
        }break;
    case 1:     //min items
        {
            char * param1 = strtok(args, " ");
            uint32 minItems = (uint32) strtoul(param1, NULL, 0);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET minitems = '%u' WHERE auctionhouse = '%u'", minItems, ahMapID);
            config->SetMinItems(minItems);
        }break;
    case 2:     //max items
        {
            char * param1 = strtok(args, " ");
            uint32 maxItems = (uint32) strtoul(param1, NULL, 0);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET maxitems = '%u' WHERE auctionhouse = '%u'", maxItems, ahMapID);
            config->SetMaxItems(maxItems);
        }break;
    case 3:     //min time
        {
            char * param1 = strtok(args, " ");
            uint32 minTime = (uint32) strtoul(param1, NULL, 0);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET mintime = '%u' WHERE auctionhouse = '%u'", minTime, ahMapID);
            config->SetMinTime(minTime);
        }break;
    case 4:     //max time
        {
            char * param1 = strtok(args, " ");
            uint32 maxTime = (uint32) strtoul(param1, NULL, 0);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET maxtime = '%u' WHERE auctionhouse = '%u'", maxTime, ahMapID);
            config->SetMaxTime(maxTime);
        }break;
    case 5:     //percentages
        {
            char * param1 = strtok(args, " ");
            char * param2 = strtok(NULL, " ");
            char * param3 = strtok(NULL, " ");
            char * param4 = strtok(NULL, " ");
            char * param5 = strtok(NULL, " ");
            char * param6 = strtok(NULL, " ");
            char * param7 = strtok(NULL, " ");
            char * param8 = strtok(NULL, " ");
            char * param9 = strtok(NULL, " ");
            char * param10 = strtok(NULL, " ");
            char * param11 = strtok(NULL, " ");
            char * param12 = strtok(NULL, " ");
            char * param13 = strtok(NULL, " ");
            char * param14 = strtok(NULL, " ");
            uint32 greytg = (uint32) strtoul(param1, NULL, 0);
            uint32 whitetg = (uint32) strtoul(param2, NULL, 0);
            uint32 greentg = (uint32) strtoul(param3, NULL, 0);
            uint32 bluetg = (uint32) strtoul(param4, NULL, 0);
            uint32 purpletg = (uint32) strtoul(param5, NULL, 0);
            uint32 orangetg = (uint32) strtoul(param6, NULL, 0);
            uint32 yellowtg = (uint32) strtoul(param7, NULL, 0);
            uint32 greyi = (uint32) strtoul(param8, NULL, 0);
            uint32 whitei = (uint32) strtoul(param9, NULL, 0);
            uint32 greeni = (uint32) strtoul(param10, NULL, 0);
            uint32 bluei = (uint32) strtoul(param11, NULL, 0);
            uint32 purplei = (uint32) strtoul(param12, NULL, 0);
            uint32 orangei = (uint32) strtoul(param13, NULL, 0);
            uint32 yellowi = (uint32) strtoul(param14, NULL, 0);

            CharacterDatabase.BeginTransaction();
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentgreytradegoods = '%u' WHERE auctionhouse = '%u'", greytg, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentwhitetradegoods = '%u' WHERE auctionhouse = '%u'", whitetg, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentgreentradegoods = '%u' WHERE auctionhouse = '%u'", greentg, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentbluetradegoods = '%u' WHERE auctionhouse = '%u'", bluetg, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentpurpletradegoods = '%u' WHERE auctionhouse = '%u'", purpletg, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentorangetradegoods = '%u' WHERE auctionhouse = '%u'", orangetg, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentyellowtradegoods = '%u' WHERE auctionhouse = '%u'", yellowtg, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentgreyitems = '%u' WHERE auctionhouse = '%u'", greyi, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentwhiteitems = '%u' WHERE auctionhouse = '%u'", whitei, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentgreenitems = '%u' WHERE auctionhouse = '%u'", greeni, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentblueitems = '%u' WHERE auctionhouse = '%u'", bluei, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentpurpleitems = '%u' WHERE auctionhouse = '%u'", purplei, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentorangeitems = '%u' WHERE auctionhouse = '%u'", orangei, ahMapID);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET percentyellowitems = '%u' WHERE auctionhouse = '%u'", yellowi, ahMapID);
            CharacterDatabase.CommitTransaction();
            config->SetPercentages(greytg, whitetg, greentg, bluetg, purpletg, orangetg, yellowtg, greyi, whitei, greeni, bluei, purplei, orangei, yellowi);
        }break;
    case 6:     //min prices
        {
            char * param1 = strtok(args, " ");
            uint32 minPrice = (uint32) strtoul(param1, NULL, 0);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET minprice%s = '%u' WHERE auctionhouse = '%u'",color.c_str(), minPrice, ahMapID);
            config->SetMinPrice(col, minPrice);
        }break;
    case 7:     //max prices
        {
            char * param1 = strtok(args, " ");
            uint32 maxPrice = (uint32) strtoul(param1, NULL, 0);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET maxprice%s = '%u' WHERE auctionhouse = '%u'",color.c_str(), maxPrice, ahMapID);
            config->SetMaxPrice(col, maxPrice);
        }break;
    case 8:     //min bid price
        {
            char * param1 = strtok(args, " ");
            uint32 minBidPrice = (uint32) strtoul(param1, NULL, 0);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET minbidprice%s = '%u' WHERE auctionhouse = '%u'",color.c_str(), minBidPrice, ahMapID);
            config->SetMinBidPrice(col, minBidPrice);
        }break;
    case 9:     //max bid price
        {
            char * param1 = strtok(args, " ");
            uint32 maxBidPrice = (uint32) strtoul(param1, NULL, 0);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET maxbidprice%s = '%u' WHERE auctionhouse = '%u'",color.c_str(), maxBidPrice, ahMapID);
            config->SetMaxBidPrice(col, maxBidPrice);
        }break;
    case 10:        //max stacks
        {
            char * param1 = strtok(args, " ");
            uint32 maxStack = (uint32) strtoul(param1, NULL, 0);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET maxstack%s = '%u' WHERE auctionhouse = '%u'",color.c_str(), maxStack, ahMapID);
            config->SetMaxStack(col, maxStack);
        }break;
    case 11:        //buyer bid prices
        {
            char * param1 = strtok(args, " ");
            uint32 buyerPrice = (uint32) strtoul(param1, NULL, 0);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET buyerprice%s = '%u' WHERE auctionhouse = '%u'",color.c_str(), buyerPrice, ahMapID);
            config->SetBuyerPrice(col, buyerPrice);
        }break;
    case 12:        //buyer bidding interval
        {
            char * param1 = strtok(args, " ");
            uint32 bidInterval = (uint32) strtoul(param1, NULL, 0);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET buyerbiddinginterval = '%u' WHERE auctionhouse = '%u'", bidInterval, ahMapID);
            config->SetBiddingInterval(bidInterval);
        }break;
    case 13:        //buyer bids per interval
        {
            char * param1 = strtok(args, " ");
            uint32 bidsPerInterval = (uint32) strtoul(param1, NULL, 0);
            CharacterDatabase.PExecute("UPDATE auctionhousebot SET buyerbidsperinterval = '%u' WHERE auctionhouse = '%u'", bidsPerInterval, ahMapID);
            config->SetBidsPerInterval(bidsPerInterval);
        }break;
    default:
        break;
    }
}

void AuctionHouseBot::LoadValues(AHBConfig *config)
{
    QueryResult* result;
//                                                        0       1           2           3          4          5                        6
    result = CharacterDatabase.PQuery("SELECT `auctionhouse`, `name`, `minitems`, `maxitems`, `mintime`, `maxtime`, `percentgreytradegoods`, "
//                                                           7                         8                        9                         10
                                      "`percentwhitetradegoods`, `percentgreentradegoods`, `percentbluetradegoods`, `percentpurpletradegoods`, "
//                                                           11                         12                  13                   14                   15
                                      "`percentorangetradegoods`, `percentyellowtradegoods`, `percentgreyitems`, `percentwhiteitems`, `percentgreenitems`, "
//                                                    16                    17                    18                    19              20              21
                                      "`percentblueitems`, `percentpurpleitems`, `percentorangeitems`, `percentyellowitems`, `minpricegrey`, `maxpricegrey`, "
//                                                 22               23               24               25              26              27                28
                                      "`minpricewhite`, `maxpricewhite`, `minpricegreen`, `maxpricegreen`, `minpriceblue`, `maxpriceblue`, `minpricepurple`, "
//                                                  29                30                31                32                33                 34
                                      "`maxpricepurple`, `minpriceorange`, `maxpriceorange`, `minpriceyellow`, `maxpriceyellow`, `minbidpricegrey`, "
//                                                   35                  36                  37                  38                  39                 40
                                      "`maxbidpricegrey`, `minbidpricewhite`, `maxbidpricewhite`, `minbidpricegreen`, `maxbidpricegreen`, `minbidpriceblue`, "
//                                                   41                   42                   43                   44                   45                   46
                                      "`maxbidpriceblue`, `minbidpricepurple`, `maxbidpricepurple`, `minbidpriceorange`, `maxbidpriceorange`, `minbidpriceyellow`, "
//                                                     47              48               49               50              51                52                53
                                      "`maxbidpriceyellow`, `maxstackgrey`, `maxstackwhite`, `maxstackgreen`, `maxstackblue`, `maxstackpurple`, `maxstackorange`, "
//                                                  54                55                 56                 57                58                  59
                                      "`maxstackyellow`, `buyerpricegrey`, `buyerpricewhite`, `buyerpricegreen`, `buyerpriceblue`, `buyerpricepurple`, "
//                                                    60                  61                      62                      63
                                      "`buyerpriceorange`, `buyerpriceyellow`, `buyerbiddinginterval`, `buyerbidsperinterval` FROM `auctionhousebot` "
                                      "WHERE auctionhouse = %u",config->GetAHID());

    if(!result)
        return;

    Field* fields = result->Fetch();

    if (AHBSeller)
    {
        //load min and max items
        config->SetMinItems(fields[2].GetUInt32());
        config->SetMaxItems(fields[3].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minItems = %u", config->GetMinItems());
            sLog.outError("maxItems = %u", config->GetMaxItems());
        }
        //load min and max auction times
        config->SetMinTime(fields[4].GetUInt32());
        config->SetMaxTime(fields[5].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minTime = %u", config->GetMinTime());
            sLog.outError("maxTime = %u", config->GetMaxTime());
        }
        //load percentages
        uint32 greytg   = fields[6].GetUInt32();
        uint32 whitetg  = fields[7].GetUInt32();
        uint32 greentg  = fields[8].GetUInt32();
        uint32 bluetg   = fields[9].GetUInt32();
        uint32 purpletg = fields[10].GetUInt32();
        uint32 orangetg = fields[11].GetUInt32();
        uint32 yellowtg = fields[12].GetUInt32();
        uint32 greyi    = fields[13].GetUInt32();
        uint32 whitei   = fields[14].GetUInt32();
        uint32 greeni   = fields[15].GetUInt32();
        uint32 bluei    = fields[16].GetUInt32();
        uint32 purplei  = fields[17].GetUInt32();
        uint32 orangei  = fields[18].GetUInt32();
        uint32 yellowi  = fields[19].GetUInt32();
        config->SetPercentages(greytg, whitetg, greentg, bluetg, purpletg, orangetg, yellowtg, greyi, whitei, greeni, bluei, purplei, orangei, yellowi);
        if (debug_Out)
        {
            sLog.outError("percentGreyTradeGoods = %u", config->GetPercentages(AHB_GREY_TG));
            sLog.outError("percentWhiteTradeGoods = %u", config->GetPercentages(AHB_WHITE_TG));
            sLog.outError("percentGreenTradeGoods = %u", config->GetPercentages(AHB_GREEN_TG));
            sLog.outError("percentBlueTradeGoods = %u", config->GetPercentages(AHB_BLUE_TG));
            sLog.outError("percentPurpleTradeGoods = %u", config->GetPercentages(AHB_PURPLE_TG));
            sLog.outError("percentOrangeTradeGoods = %u", config->GetPercentages(AHB_ORANGE_TG));
            sLog.outError("percentYellowTradeGoods = %u", config->GetPercentages(AHB_YELLOW_TG));
            sLog.outError("percentGreyItems = %u", config->GetPercentages(AHB_GREY_I));
            sLog.outError("percentWhiteItems = %u", config->GetPercentages(AHB_WHITE_I));
            sLog.outError("percentGreenItems = %u", config->GetPercentages(AHB_GREEN_I));
            sLog.outError("percentBlueItems = %u", config->GetPercentages(AHB_BLUE_I));
            sLog.outError("percentPurpleItems = %u", config->GetPercentages(AHB_PURPLE_I));
            sLog.outError("percentOrangeItems = %u", config->GetPercentages(AHB_ORANGE_I));
            sLog.outError("percentYellowItems = %u", config->GetPercentages(AHB_YELLOW_I));
        }
        //load min and max prices
        config->SetMinPrice(AHB_GREY, fields[20].GetUInt32());
        config->SetMaxPrice(AHB_GREY, fields[21].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minPriceGrey = %u", config->GetMinPrice(AHB_GREY));
            sLog.outError("maxPriceGrey = %u", config->GetMaxPrice(AHB_GREY));
        }
        config->SetMinPrice(AHB_WHITE, fields[22].GetUInt32());
        config->SetMaxPrice(AHB_WHITE, fields[23].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minPriceWhite = %u", config->GetMinPrice(AHB_WHITE));
            sLog.outError("maxPriceWhite = %u", config->GetMaxPrice(AHB_WHITE));
        }
        config->SetMinPrice(AHB_GREEN, fields[24].GetUInt32());
        config->SetMaxPrice(AHB_GREEN, fields[25].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minPriceGreen = %u", config->GetMinPrice(AHB_GREEN));
            sLog.outError("maxPriceGreen = %u", config->GetMaxPrice(AHB_GREEN));
        }
        config->SetMinPrice(AHB_BLUE, fields[26].GetUInt32());
        config->SetMaxPrice(AHB_BLUE, fields[27].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minPriceBlue = %u", config->GetMinPrice(AHB_BLUE));
            sLog.outError("maxPriceBlue = %u", config->GetMaxPrice(AHB_BLUE));
        }
        config->SetMinPrice(AHB_PURPLE, fields[28].GetUInt32());
        config->SetMaxPrice(AHB_PURPLE, fields[29].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minPricePurple = %u", config->GetMinPrice(AHB_PURPLE));
            sLog.outError("maxPricePurple = %u", config->GetMaxPrice(AHB_PURPLE));
        }
        config->SetMinPrice(AHB_ORANGE, fields[30].GetUInt32());
        config->SetMaxPrice(AHB_ORANGE, fields[31].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minPriceOrange = %u", config->GetMinPrice(AHB_ORANGE));
            sLog.outError("maxPriceOrange = %u", config->GetMaxPrice(AHB_ORANGE));
        }
        config->SetMinPrice(AHB_YELLOW, fields[32].GetUInt32());
        config->SetMaxPrice(AHB_YELLOW, fields[33].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minPriceYellow = %u", config->GetMinPrice(AHB_YELLOW));
            sLog.outError("maxPriceYellow = %u", config->GetMaxPrice(AHB_YELLOW));
        }
        //load min and max bid prices
        config->SetMinBidPrice(AHB_GREY, fields[34].GetUInt32());
        config->SetMaxBidPrice(AHB_GREY, fields[35].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minBidPriceGrey = %u", config->GetMinBidPrice(AHB_GREY));
            sLog.outError("maxBidPriceGrey = %u", config->GetMaxBidPrice(AHB_GREY));
        }
        config->SetMinBidPrice(AHB_WHITE, fields[36].GetUInt32());
        config->SetMaxBidPrice(AHB_WHITE, fields[37].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minBidPriceWhite = %u", config->GetMinBidPrice(AHB_WHITE));
            sLog.outError("maxBidPriceWhite = %u", config->GetMaxBidPrice(AHB_WHITE));
        }
        config->SetMinBidPrice(AHB_GREEN, fields[38].GetUInt32());
        config->SetMaxBidPrice(AHB_GREEN, fields[39].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minBidPriceGreen = %u", config->GetMinBidPrice(AHB_GREEN));
            sLog.outError("maxBidPriceGreen = %u", config->GetMaxBidPrice(AHB_GREEN));
        }
        config->SetMinBidPrice(AHB_BLUE, fields[40].GetUInt32());
        config->SetMaxBidPrice(AHB_BLUE, fields[41].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minBidPriceBlue = %u", config->GetMinBidPrice(AHB_BLUE));
            sLog.outError("maxBidPriceBlue = %u", config->GetMinBidPrice(AHB_BLUE));
        }
        config->SetMinBidPrice(AHB_PURPLE, fields[42].GetUInt32());
        config->SetMaxBidPrice(AHB_PURPLE, fields[43].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minBidPricePurple = %u", config->GetMinBidPrice(AHB_PURPLE));
            sLog.outError("maxBidPricePurple = %u", config->GetMaxBidPrice(AHB_PURPLE));
        }
        config->SetMinBidPrice(AHB_ORANGE, fields[44].GetUInt32());
        config->SetMaxBidPrice(AHB_ORANGE, fields[45].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minBidPriceOrange = %u", config->GetMinBidPrice(AHB_ORANGE));
            sLog.outError("maxBidPriceOrange = %u", config->GetMaxBidPrice(AHB_ORANGE));
        }
        config->SetMinBidPrice(AHB_YELLOW,  fields[46].GetUInt32());
        config->SetMaxBidPrice(AHB_YELLOW,  fields[47].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("minBidPriceYellow = %u", config->GetMinBidPrice(AHB_YELLOW));
            sLog.outError("maxBidPriceYellow = %u", config->GetMaxBidPrice(AHB_YELLOW));
        }
        //load max stacks
        config->SetMaxStack(AHB_GREY, fields[48].GetUInt32());
        config->SetMaxStack(AHB_WHITE, fields[49].GetUInt32());
        config->SetMaxStack(AHB_GREEN, fields[50].GetUInt32());
        config->SetMaxStack(AHB_BLUE, fields[51].GetUInt32());
        config->SetMaxStack(AHB_PURPLE, fields[52].GetUInt32());
        config->SetMaxStack(AHB_ORANGE, fields[53].GetUInt32());
        config->SetMaxStack(AHB_YELLOW, fields[54].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("maxStackGrey = %u", config->GetMaxStack(AHB_GREY));
            sLog.outError("maxStackWhite = %u", config->GetMaxStack(AHB_WHITE));
            sLog.outError("maxStackGreen = %u", config->GetMaxStack(AHB_GREEN));
            sLog.outError("maxStackBlue = %u", config->GetMaxStack(AHB_BLUE));
            sLog.outError("maxStackPurple = %u", config->GetMaxStack(AHB_PURPLE));
            sLog.outError("maxStackOrange = %u", config->GetMaxStack(AHB_ORANGE));
            sLog.outError("maxStackYellow = %u", config->GetMaxStack(AHB_YELLOW));
        }
    }
    if (AHBBuyer)
    {
        //load buyer bid prices
        config->SetBuyerPrice(AHB_GREY,  fields[55].GetUInt32());
        config->SetBuyerPrice(AHB_WHITE,  fields[56].GetUInt32());
        config->SetBuyerPrice(AHB_GREEN,  fields[57].GetUInt32());
        config->SetBuyerPrice(AHB_BLUE,  fields[58].GetUInt32());
        config->SetBuyerPrice(AHB_PURPLE,  fields[59].GetUInt32());
        config->SetBuyerPrice(AHB_ORANGE,  fields[60].GetUInt32());
        config->SetBuyerPrice(AHB_YELLOW,  fields[61].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("buyerPriceGrey = %u", config->GetBuyerPrice(AHB_GREY));
            sLog.outError("buyerPriceWhite = %u", config->GetBuyerPrice(AHB_WHITE));
            sLog.outError("buyerPriceGreen = %u", config->GetBuyerPrice(AHB_GREEN));
            sLog.outError("buyerPriceBlue = %u", config->GetBuyerPrice(AHB_BLUE));
            sLog.outError("buyerPricePurple = %u", config->GetBuyerPrice(AHB_PURPLE));
            sLog.outError("buyerPriceOrange = %u", config->GetBuyerPrice(AHB_ORANGE));
            sLog.outError("buyerPriceYellow = %u", config->GetBuyerPrice(AHB_YELLOW));
        }
        //load bidding interval
        config->SetBiddingInterval(fields[62].GetUInt32());
        config->SetBidsPerInterval(fields[63].GetUInt32());
        if (debug_Out)
        {
            sLog.outError("buyerBiddingInterval = %u", config->GetBiddingInterval());
            sLog.outError("buyerBidsPerInterval = %u", config->GetBidsPerInterval());
        }
    }

    if (result)
        delete result;
}
