#include "AuctionHouseBot.h"
#include "ProgressBar.h"
#include "Log.h"
#include "../ObjectMgr.h"
#include "../AuctionHouseMgr.h"
#include "SystemConfig.h"

// Format is YYYYMMDDRR where RR is the change in the conf file
// for that day.
#define AUCTIONHOUSEBOT_CONF_VERSION    2010102201

#include "Policies/SingletonImp.h"

struct BuyerAuctionEval
{
    BuyerAuctionEval() : AuctionId(0), LastChecked(0), LastExist(0) {}

    uint32  AuctionId;
    time_t  LastChecked;
    time_t  LastExist;
};

struct BuyerItemInfo
{
    BuyerItemInfo() : ItemCount(0), BuyPrice(0), BidPrice(0), MinBuyPrice(0), MinBidPrice(0) {}

    uint32  ItemCount;
    double  BuyPrice;
    double  BidPrice;
    uint32  MinBuyPrice;
    uint32  MinBidPrice;
};

typedef std::map<uint32, BuyerItemInfo > BuyerItemInfoMap;
typedef std::map<uint32, BuyerAuctionEval > CheckEntryMap;

struct AHB_Buyer_Config
{
    public:
        AHB_Buyer_Config() : m_houseType(AUCTION_HOUSE_NEUTRAL) {}

        void Initialize(AuctionHouseType houseType)
        {
            m_houseType = houseType;
        }

        AuctionHouseType GetHouseType() const { return m_houseType; }

    public:
        BuyerItemInfoMap SameItemInfo;
        CheckEntryMap    CheckedEntry;
        uint32           FactionChance;
        bool             BuyerEnabled;
        uint32           BuyerPriceRatio;

    private:
        AuctionHouseType m_houseType;
};

struct RandomArrayEntry
{
    uint32 color;
    uint32 itemclass;
};

typedef std::vector<RandomArrayEntry> RandomArray;

struct SellerItemClassInfo
{
    SellerItemClassInfo() : AmountOfItems(0), MissItems(0), Quantity(0) {}

    uint32 AmountOfItems;
    uint32 MissItems;
    uint32 Quantity;

};

struct SellerItemInfo
{
    SellerItemInfo() : AmountOfItems(0), MissItems(0), PriceRatio(0) {}

    uint32 AmountOfItems;
    uint32 MissItems;
    uint32 PriceRatio;

    SellerItemClassInfo ItemClassInfos[MAX_ITEM_CLASS];
};

class AHB_Seller_Config
{
    public:
        AHB_Seller_Config() : m_houseType(AUCTION_HOUSE_NEUTRAL)
        {
        }

        ~AHB_Seller_Config() {}

        void Initialize(AuctionHouseType houseType)
        {
            m_houseType = houseType;
        }

        AuctionHouseType GetHouseType() const { return m_houseType; }

        uint32 LastMissedItem;

        void SetMinTime(uint32 value)
        {
            m_minTime = value;
        }
        uint32 GetMinTime() const
        {
            if (m_minTime < 1)
                return 1;
            else if ((m_maxTime) && (m_minTime > m_maxTime))
                return m_maxTime;
            else
                return m_minTime;
        }

        void        SetMaxTime(uint32 value) { m_maxTime = value; }
        uint32      GetMaxTime() const { return m_maxTime; }
        // Data access classified by item class and item quality //
        void        SetItemsAmountPerClass(AuctionQuality quality, ItemClass itemclass, uint32 amount) { m_ItemInfo[quality].ItemClassInfos[itemclass].AmountOfItems=amount * m_ItemInfo[quality].ItemClassInfos[itemclass].Quantity; }
        uint32      GetItemsAmountPerClass(AuctionQuality quality, ItemClass itemclass) const { return m_ItemInfo[quality].ItemClassInfos[itemclass].AmountOfItems; }
        void        SetItemsQuantityPerClass(AuctionQuality quality, ItemClass itemclass, uint32 qty) { m_ItemInfo[quality].ItemClassInfos[itemclass].Quantity=qty; }
        uint32      GetItemsQuantityPerClass(AuctionQuality quality, ItemClass itemclass) const { return m_ItemInfo[quality].ItemClassInfos[itemclass].Quantity; }
        void        SetMissedItemsPerClass(AuctionQuality quality, ItemClass itemclass, uint32 found)
        {
            if (m_ItemInfo[quality].ItemClassInfos[itemclass].AmountOfItems > found )
                m_ItemInfo[quality].ItemClassInfos[itemclass].MissItems=m_ItemInfo[quality].ItemClassInfos[itemclass].AmountOfItems - found;
            else
                m_ItemInfo[quality].ItemClassInfos[itemclass].MissItems = 0;
        }
        uint32      GetMissedItemsPerClass(AuctionQuality quality, ItemClass itemclass) const { return m_ItemInfo[quality].ItemClassInfos[itemclass].MissItems; }

        // Data for every quality of item //
        void        SetItemsAmountPerQuality(AuctionQuality quality, uint32 cnt) { m_ItemInfo[quality].AmountOfItems=cnt; }
        uint32      GetItemsAmountPerQuality(AuctionQuality quality) const { return m_ItemInfo[quality].AmountOfItems; }
        void        SetPriceRatioPerQuality(AuctionQuality quality, uint32 value) { m_ItemInfo[quality].PriceRatio=value; }
        uint32      GetPriceRatioPerQuality(AuctionQuality quality) const { return m_ItemInfo[quality].PriceRatio; }

    private:
        AuctionHouseType m_houseType;
        uint32 m_minTime;
        uint32 m_maxTime;
        SellerItemInfo m_ItemInfo[MAX_AUCTION_QUALITY];
};

// This class handle all Buyer method
// (holder of AuctionBotConfig for each auction house type)
class AuctionBotBuyer : public AuctionBotAgent
{
    public:
        AuctionBotBuyer();
        ~AuctionBotBuyer();

        bool        Initialize() override;
        bool        Update(AuctionHouseType houseType) override;

        void        LoadConfig();
        void        addNewAuctionBuyerBotBid(AHB_Buyer_Config& config);

    private:
        uint32              m_CheckInterval;
        AHB_Buyer_Config    m_HouseConfig[MAX_AUCTION_HOUSE_TYPE];

        void        LoadBuyerValues(AHB_Buyer_Config& config);
        bool        IsBuyableEntry(uint32 buyoutPrice, double InGame_BuyPrice, double MaxBuyablePrice, uint32 MinBuyPrice, uint32 MaxChance, uint32 ChanceRatio);
        bool        IsBidableEntry(uint32 bidPrice, double InGame_BuyPrice, double MaxBidablePrice, uint32 MinBidPrice, uint32 MaxChance, uint32 ChanceRatio);
        void        PlaceBidToEntry(AuctionEntry* auction, uint32 bidPrice);
        void        BuyEntry(AuctionEntry* auction);
        void        PrepareListOfEntry(AHB_Buyer_Config& config);
        uint32      GetBuyableEntry(AHB_Buyer_Config& config);
};

// This class handle all Selling method
// (holder of AHB_Seller_Config data for each auction house type)
class AuctionBotSeller : public AuctionBotAgent
{
    public:
        typedef std::vector<uint32> ItemPool;

        AuctionBotSeller();
        ~AuctionBotSeller();

        bool Initialize() override;
        bool Update(AuctionHouseType houseType) override;

        void addNewAuctions(AHB_Seller_Config& config);
        void SetItemsRatio(uint32 al, uint32 ho, uint32 ne);
        void SetItemsRatioForHouse(AuctionHouseType house, uint32 val);
        void SetItemsAmount(uint32 (&vals) [MAX_AUCTION_QUALITY]);
        void SetItemsAmountForQuality(AuctionQuality quality, uint32 val);
        void LoadConfig();

    private:
        AHB_Seller_Config   m_HouseConfig[MAX_AUCTION_HOUSE_TYPE];

        ItemPool m_ItemPool[MAX_AUCTION_QUALITY][MAX_ITEM_CLASS];

        void        LoadSellerValues(AHB_Seller_Config& config);
        uint32      SetStat(AHB_Seller_Config& config);
        bool        getRandomArray( AHB_Seller_Config& config, RandomArray& ra, const std::vector<std::vector<uint32> >& addedItem  );
        void        SetPricesOfItem(ItemPrototype const *itemProto, AHB_Seller_Config& config, uint32& buyp, uint32& bidp, uint32 stackcnt, ItemQualities itemQuality);
        void        LoadItemsQuantity(AHB_Seller_Config& config);
};

INSTANTIATE_SINGLETON_1( AuctionHouseBot );
INSTANTIATE_SINGLETON_1( AuctionBotConfig );

//== AuctionBotConfig functions ============================

AuctionBotConfig::AuctionBotConfig() : m_configFileName(_AUCTIONHOUSEBOT_CONFIG)
{
}

bool AuctionBotConfig::Initialize()
{
    if (!m_AhBotCfg.SetSource(m_configFileName.c_str()))
    {
        sLog.outString("AHBOT is Disabled. Unable to open configuration file(%s). ", m_configFileName.c_str());
        setConfig(CONFIG_UINT32_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO, 0);
        setConfig(CONFIG_UINT32_AHBOT_HORDE_ITEM_AMOUNT_RATIO, 0);
        setConfig(CONFIG_UINT32_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO, 0);
        return false;
    }
    else
        sLog.outString("AHBot using configuration file %s", m_configFileName.c_str());

    GetConfigFromFile();

    if (!getConfig(CONFIG_BOOL_AHBOT_BUYER_ENABLED) && !getConfig(CONFIG_BOOL_AHBOT_SELLER_ENABLED))
    {
        sLog.outString("AHBOT is Disabled. (If you want to use it please set config in 'ahbot.conf')");
        return false;
    }

    if ((getConfig(CONFIG_UINT32_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO)==0) && (getConfig(CONFIG_UINT32_AHBOT_HORDE_ITEM_AMOUNT_RATIO)==0) && (getConfig(CONFIG_UINT32_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO)==0) &&
        !getConfig(CONFIG_BOOL_AHBOT_BUYER_ALLIANCE_ENABLED) && !getConfig(CONFIG_BOOL_AHBOT_BUYER_HORDE_ENABLED) && !getConfig(CONFIG_BOOL_AHBOT_BUYER_NEUTRAL_ENABLED))
    {
        sLog.outString("All feature of AuctionHouseBot are disabled! (If you want to use it please set config in 'ahbot.conf')");
        return false;
    }
    if ((getConfig(CONFIG_UINT32_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO)==0) && (getConfig(CONFIG_UINT32_AHBOT_HORDE_ITEM_AMOUNT_RATIO)==0) && (getConfig(CONFIG_UINT32_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO)==0))
        sLog.outString("AuctionHouseBot SELLER is disabled! (If you want to use it please set config in 'ahbot.conf')");

    if (!getConfig(CONFIG_BOOL_AHBOT_BUYER_ALLIANCE_ENABLED) && !getConfig(CONFIG_BOOL_AHBOT_BUYER_HORDE_ENABLED) && !getConfig(CONFIG_BOOL_AHBOT_BUYER_NEUTRAL_ENABLED))
        sLog.outString("AuctionHouseBot BUYER is disabled! (If you want to use it please set config in 'ahbot.conf')");

    m_ItemsPerCycleBoost = getConfig(CONFIG_UINT32_AHBOT_ITEMS_PER_CYCLE_BOOST);
    m_ItemsPerCycleNormal = getConfig(CONFIG_UINT32_AHBOT_ITEMS_PER_CYCLE_NORMAL);
    return true;
}

void AuctionBotConfig::setConfig(AuctionBotConfigUInt32Values index, char const* fieldname, uint32 defvalue)
{
    setConfig(index, m_AhBotCfg.GetIntDefault(fieldname,defvalue));
    if (int32(getConfig(index)) < 0)
    {
        sLog.outError("AHBot: %s (%i) can't be negative. Using %u instead.", fieldname, int32(getConfig(index)), defvalue);
        setConfig(index, defvalue);
    }
}

void AuctionBotConfig::setConfigMax(AuctionBotConfigUInt32Values index, char const* fieldname, uint32 defvalue, uint32 maxvalue)
{
    setConfig(index, m_AhBotCfg.GetIntDefault(fieldname,defvalue));
    if (getConfig(index) > maxvalue)
    {
        sLog.outError("AHBot: %s (%u) must be in range 0...%u. Using %u instead.", fieldname, getConfig(index), maxvalue, maxvalue);
        setConfig(index, maxvalue);
    }
}

void AuctionBotConfig::setConfigMinMax(AuctionBotConfigUInt32Values index, char const* fieldname, uint32 defvalue, uint32 minvalue, uint32 maxvalue)
{
    setConfig(index, m_AhBotCfg.GetIntDefault(fieldname,defvalue));
    if (getConfig(index) > maxvalue)
    {
        sLog.outError("AHBot: %s (%u) must be in range %u...%u. Using %u instead.", fieldname, getConfig(index), minvalue, maxvalue, maxvalue);
        setConfig(index, maxvalue);
    }
    if (getConfig(index) < minvalue)
    {
        sLog.outError("AHBot: %s (%u) must be in range %u...%u. Using %u instead.", fieldname, getConfig(index), minvalue, maxvalue, minvalue);
        setConfig(index, minvalue);
    }
}

void AuctionBotConfig::setConfig(AuctionBotConfigBoolValues index, char const* fieldname, bool defvalue)
{
    setConfig(index, m_AhBotCfg.GetBoolDefault(fieldname,defvalue));
}

//Get AuctionHousebot configuration file
void AuctionBotConfig::GetConfigFromFile()
{
    //Check config file version
    if (m_AhBotCfg.GetIntDefault("ConfVersion", 0) != AUCTIONHOUSEBOT_CONF_VERSION)
        sLog.outError("AHBot: Configuration file version doesn't match expected version. Some config variables may be wrong or missing.");

    setConfigMax(CONFIG_UINT32_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO , "AuctionHouseBot.Alliance.Items.Amount.Ratio" , 100, 10000);
    setConfigMax(CONFIG_UINT32_AHBOT_HORDE_ITEM_AMOUNT_RATIO    , "AuctionHouseBot.Horde.Items.Amount.Ratio"    , 100, 10000);
    setConfigMax(CONFIG_UINT32_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO  , "AuctionHouseBot.Neutral.Items.Amount.Ratio"  , 100, 10000);

    SetAHBotIncludes(m_AhBotCfg.GetStringDefault("AuctionHouseBot.forceIncludeItems", ""));
    SetAHBotExcludes(m_AhBotCfg.GetStringDefault("AuctionHouseBot.forceExcludeItems", ""));

    setConfig(CONFIG_BOOL_AHBOT_BUYER_ALLIANCE_ENABLED       , "AuctionHouseBot.Buyer.Alliance.Enabled"      , false);
    setConfig(CONFIG_BOOL_AHBOT_BUYER_HORDE_ENABLED          , "AuctionHouseBot.Buyer.Horde.Enabled"         , false);
    setConfig(CONFIG_BOOL_AHBOT_BUYER_NEUTRAL_ENABLED        , "AuctionHouseBot.Buyer.Neutral.Enabled"       , false);

    setConfig(CONFIG_BOOL_AHBOT_ITEMS_VENDOR                 , "AuctionHouseBot.Items.Vendor"                , false);
    setConfig(CONFIG_BOOL_AHBOT_ITEMS_LOOT                   , "AuctionHouseBot.Items.Loot"                  , true);
    setConfig(CONFIG_BOOL_AHBOT_ITEMS_MISC                   , "AuctionHouseBot.Items.Misc"                  , false);

    setConfig(CONFIG_BOOL_AHBOT_BIND_NO                      , "AuctionHouseBot.Bind.No"                     , true);
    setConfig(CONFIG_BOOL_AHBOT_BIND_PICKUP                  , "AuctionHouseBot.Bind.Pickup"                 , false);
    setConfig(CONFIG_BOOL_AHBOT_BIND_EQUIP                   , "AuctionHouseBot.Bind.Equip"                  , true);
    setConfig(CONFIG_BOOL_AHBOT_BIND_USE                     , "AuctionHouseBot.Bind.Use"                    , true);
    setConfig(CONFIG_BOOL_AHBOT_BIND_QUEST                   , "AuctionHouseBot.Bind.Quest"                  , false);
    setConfig(CONFIG_BOOL_AHBOT_LOCKBOX_ENABLED              , "AuctionHouseBot.LockBox.Enabled"             , false);

    setConfig(CONFIG_BOOL_AHBOT_BUYPRICE_SELLER              , "AuctionHouseBot.BuyPrice.Seller"             , true);

    setConfig(CONFIG_UINT32_AHBOT_ITEMS_PER_CYCLE_BOOST      , "AuctionHouseBot.ItemsPerCycle.Boost"         , 75);
    setConfig(CONFIG_UINT32_AHBOT_ITEMS_PER_CYCLE_NORMAL     , "AuctionHouseBot.ItemsPerCycle.Normal"        , 20);

    setConfig(CONFIG_UINT32_AHBOT_ITEM_MIN_ITEM_LEVEL        , "AuctionHouseBot.Items.ItemLevel.Min"         , 0);
    setConfig(CONFIG_UINT32_AHBOT_ITEM_MAX_ITEM_LEVEL        , "AuctionHouseBot.Items.ItemLevel.Max"         , 0);
    setConfig(CONFIG_UINT32_AHBOT_ITEM_MIN_REQ_LEVEL         , "AuctionHouseBot.Items.ReqLevel.Min"          , 0);
    setConfig(CONFIG_UINT32_AHBOT_ITEM_MAX_REQ_LEVEL         , "AuctionHouseBot.Items.ReqLevel.Max"          , 0);
    setConfig(CONFIG_UINT32_AHBOT_ITEM_MIN_SKILL_RANK        , "AuctionHouseBot.Items.ReqSkill.Min"          , 0);
    setConfig(CONFIG_UINT32_AHBOT_ITEM_MAX_SKILL_RANK        , "AuctionHouseBot.Items.ReqSkill.Max"          , 0);

    setConfig(CONFIG_UINT32_AHBOT_ITEM_GREY_AMOUNT           , "AuctionHouseBot.Items.Amount.Grey"           , 0);
    setConfig(CONFIG_UINT32_AHBOT_ITEM_WHITE_AMOUNT          , "AuctionHouseBot.Items.Amount.White"          , 2000);
    setConfig(CONFIG_UINT32_AHBOT_ITEM_GREEN_AMOUNT          , "AuctionHouseBot.Items.Amount.Green"          , 2500);
    setConfig(CONFIG_UINT32_AHBOT_ITEM_BLUE_AMOUNT           , "AuctionHouseBot.Items.Amount.Blue"           , 1500);
    setConfig(CONFIG_UINT32_AHBOT_ITEM_PURPLE_AMOUNT         , "AuctionHouseBot.Items.Amount.Purple"         , 500);
    setConfig(CONFIG_UINT32_AHBOT_ITEM_ORANGE_AMOUNT         , "AuctionHouseBot.Items.Amount.Orange"         , 0);
    setConfig(CONFIG_UINT32_AHBOT_ITEM_YELLOW_AMOUNT         , "AuctionHouseBot.Items.Amount.Yellow"         , 0);

    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_CONSUMABLE_AMOUNT , "AuctionHouseBot.Class.Consumable"            ,  6, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_CONTAINER_AMOUNT  , "AuctionHouseBot.Class.Container"             ,  4, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_WEAPON_AMOUNT     , "AuctionHouseBot.Class.Weapon"                ,  8, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_GEM_AMOUNT        , "AuctionHouseBot.Class.Gem"                   ,  3, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_ARMOR_AMOUNT      , "AuctionHouseBot.Class.Armor"                 ,  8, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_REAGENT_AMOUNT    , "AuctionHouseBot.Class.Reagent"               ,  1, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_PROJECTILE_AMOUNT , "AuctionHouseBot.Class.Projectile"            ,  2, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_AMOUNT  , "AuctionHouseBot.Class.TradeGood"             , 10, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_GENERIC_AMOUNT    , "AuctionHouseBot.Class.Generic"               ,  1, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_RECIPE_AMOUNT     , "AuctionHouseBot.Class.Recipe"                ,  6, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_QUIVER_AMOUNT     , "AuctionHouseBot.Class.Quiver"                ,  1, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_QUEST_AMOUNT      , "AuctionHouseBot.Class.Quest"                 ,  1, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_KEY_AMOUNT        , "AuctionHouseBot.Class.Key"                   ,  1, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_MISC_AMOUNT       , "AuctionHouseBot.Class.Misc"                  ,  5, 10);
    setConfigMax(CONFIG_UINT32_AHBOT_CLASS_GLYPH_AMOUNT      , "AuctionHouseBot.Class.Glyph"                 ,  3, 10);

    setConfig(CONFIG_UINT32_AHBOT_ALLIANCE_PRICE_RATIO       , "AuctionHouseBot.Alliance.Price.Ratio"        , 200);
    setConfig(CONFIG_UINT32_AHBOT_HORDE_PRICE_RATIO          , "AuctionHouseBot.Horde.Price.Ratio"           , 200);
    setConfig(CONFIG_UINT32_AHBOT_NEUTRAL_PRICE_RATIO        , "AuctionHouseBot.Neutral.Price.Ratio"         , 200);

    setConfig(CONFIG_UINT32_AHBOT_MINTIME                    , "AuctionHouseBot.MinTime"                     , 1);
    setConfig(CONFIG_UINT32_AHBOT_MAXTIME                    , "AuctionHouseBot.MaxTime"                     , 72);

    setConfigMinMax(CONFIG_UINT32_AHBOT_BUYER_CHANCE_RATIO_ALLIANCE, "AuctionHouseBot.Buyer.Alliance.Chance.Ratio", 3, 1, 100);
    setConfigMinMax(CONFIG_UINT32_AHBOT_BUYER_CHANCE_RATIO_HORDE   , "AuctionHouseBot.Buyer.Horde.Chance.Ratio"   , 3, 1, 100);
    setConfigMinMax(CONFIG_UINT32_AHBOT_BUYER_CHANCE_RATIO_NEUTRAL , "AuctionHouseBot.Buyer.Neutral.Chance.Ratio" , 3, 1, 100);
    setConfigMinMax(CONFIG_UINT32_AHBOT_BUYER_RECHECK_INTERVAL     , "AuctionHouseBot.Buyer.Recheck.Interval"     , 20,1, DAY/MINUTE);

    setConfig(CONFIG_BOOL_AHBOT_DEBUG_SELLER                 , "AuctionHouseBot.DEBUG.Seller"               , false);
    setConfig(CONFIG_BOOL_AHBOT_DEBUG_BUYER                  , "AuctionHouseBot.DEBUG.Buyer"                , false);
    setConfig(CONFIG_BOOL_AHBOT_SELLER_ENABLED               , "AuctionHouseBot.Seller.Enabled"             , false);
    setConfig(CONFIG_BOOL_AHBOT_BUYER_ENABLED                , "AuctionHouseBot.Buyer.Enabled"              , false);
    setConfig(CONFIG_BOOL_AHBOT_BUYPRICE_BUYER               , "AuctionHouseBot.Buyer.BuyPrice"             , false);

    setConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_MOUNT_MIN_REQ_LEVEL   , "AuctionHouseBot.Class.Misc.Mount.ReqLevel.Min" , 0);
    setConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_MOUNT_MAX_REQ_LEVEL   , "AuctionHouseBot.Class.Misc.Mount.ReqLevel.Max" , 0);
    setConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_MOUNT_MIN_SKILL_RANK  , "AuctionHouseBot.Class.Misc.Mount.ReqSkill.Min" , 0);
    setConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_MOUNT_MAX_SKILL_RANK  , "AuctionHouseBot.Class.Misc.Mount.ReqSkill.Max" , 0);
    setConfig(CONFIG_UINT32_AHBOT_CLASS_GLYPH_MIN_REQ_LEVEL        , "AuctionHouseBot.Class.Glyph.ReqLevel.Min"      , 0);
    setConfig(CONFIG_UINT32_AHBOT_CLASS_GLYPH_MAX_REQ_LEVEL        , "AuctionHouseBot.Class.Glyph.ReqLevel.Max"      , 0);
    setConfig(CONFIG_UINT32_AHBOT_CLASS_GLYPH_MIN_ITEM_LEVEL       , "AuctionHouseBot.Class.Glyph.ItemLevel.Min"     , 0);
    setConfig(CONFIG_UINT32_AHBOT_CLASS_GLYPH_MAX_ITEM_LEVEL       , "AuctionHouseBot.Class.Glyph.ItemLevel.Max"     , 0);
    setConfig(CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_MIN_ITEM_LEVEL   , "AuctionHouseBot.Class.TradeGood.ItemLevel.Min" , 0);
    setConfig(CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_MAX_ITEM_LEVEL   , "AuctionHouseBot.Class.TradeGood.ItemLevel.Max" , 0);
    setConfig(CONFIG_UINT32_AHBOT_CLASS_CONTAINER_MIN_ITEM_LEVEL   , "AuctionHouseBot.Class.Container.ItemLevel.Min" , 0);
    setConfig(CONFIG_UINT32_AHBOT_CLASS_CONTAINER_MAX_ITEM_LEVEL   , "AuctionHouseBot.Class.Container.ItemLevel.Max" , 0);
}

bool AuctionBotConfig::Reload()
{
    if (m_AhBotCfg.Reload())
    {
        GetConfigFromFile();
        return true;
    }
    return false;
}

char const* AuctionBotConfig::GetItemClassName(ItemClass itemclass)
{
    ItemClassEntry const* itemClassEntry = sItemClassStore.LookupEntry(itemclass);
    return itemClassEntry ? itemClassEntry->name[sWorld.GetDefaultDbcLocale()] : "";
}

char const* AuctionBotConfig::GetHouseTypeName(AuctionHouseType houseType)
{
    char const* names[MAX_AUCTION_HOUSE_TYPE] = { "Alliance", "Horde", "Neutral" };
    return names[houseType];
}

uint32 AuctionBotConfig::getConfigItemAmountRatio(AuctionHouseType houseType) const
{
    switch (houseType)
    {
        case AUCTION_HOUSE_ALLIANCE: return getConfig(CONFIG_UINT32_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO);
        case AUCTION_HOUSE_HORDE:    return getConfig(CONFIG_UINT32_AHBOT_HORDE_ITEM_AMOUNT_RATIO);
        default:                     return getConfig(CONFIG_UINT32_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO);
    }
}

bool AuctionBotConfig::getConfigBuyerEnabled(AuctionHouseType houseType) const
{
    switch (houseType)
    {
        case AUCTION_HOUSE_ALLIANCE: return getConfig(CONFIG_BOOL_AHBOT_BUYER_ALLIANCE_ENABLED);
        case AUCTION_HOUSE_HORDE:    return getConfig(CONFIG_BOOL_AHBOT_BUYER_HORDE_ENABLED);
        default:                     return getConfig(CONFIG_BOOL_AHBOT_BUYER_NEUTRAL_ENABLED);
    }
}

uint32 AuctionBotConfig::getConfigItemQualityAmount(AuctionQuality quality) const
{
    switch (quality)
    {
        case AUCTION_QUALITY_GREY:  return getConfig(CONFIG_UINT32_AHBOT_ITEM_GREY_AMOUNT);
        case AUCTION_QUALITY_WHITE: return getConfig(CONFIG_UINT32_AHBOT_ITEM_WHITE_AMOUNT);
        case AUCTION_QUALITY_GREEN: return getConfig(CONFIG_UINT32_AHBOT_ITEM_GREEN_AMOUNT);
        case AUCTION_QUALITY_BLUE:  return getConfig(CONFIG_UINT32_AHBOT_ITEM_BLUE_AMOUNT);
        case AUCTION_QUALITY_PURPLE:return getConfig(CONFIG_UINT32_AHBOT_ITEM_PURPLE_AMOUNT);
        case AUCTION_QUALITY_ORANGE:return getConfig(CONFIG_UINT32_AHBOT_ITEM_ORANGE_AMOUNT);
        default:                    return getConfig(CONFIG_UINT32_AHBOT_ITEM_YELLOW_AMOUNT);
    }
}

//== AuctionBotBuyer functions =============================

AuctionBotBuyer::AuctionBotBuyer()
{
    // Define faction for our main data class.
    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
        m_HouseConfig[i].Initialize(AuctionHouseType(i));
}

AuctionBotBuyer::~AuctionBotBuyer()
{
}

bool AuctionBotBuyer::Initialize()
{
    LoadConfig();

    bool active_house = false;
    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
    {
        if (m_HouseConfig[i].BuyerEnabled)
        {
            active_house = true;
            break;
        }
    }
    if (!active_house)
        return false;

    //load Check interval
    m_CheckInterval = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_BUYER_RECHECK_INTERVAL) * MINUTE;
    DETAIL_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot buyer interval between 2 check = %u", m_CheckInterval);
    sLog.SetLogFilter(LOG_FILTER_AHBOT_BUYER, !sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_DEBUG_BUYER));
    return true;
}

void AuctionBotBuyer::LoadBuyerValues(AHB_Buyer_Config& config)
{
    uint32 FactionChance;
    switch(config.GetHouseType())
    {
        case AUCTION_HOUSE_ALLIANCE:
            config.BuyerPriceRatio = sAuctionBotConfig.getConfig( CONFIG_UINT32_AHBOT_ALLIANCE_PRICE_RATIO )+50;
            FactionChance = sAuctionBotConfig.getConfig( CONFIG_UINT32_AHBOT_BUYER_CHANCE_RATIO_ALLIANCE );
            break;
        case AUCTION_HOUSE_HORDE:
            config.BuyerPriceRatio = sAuctionBotConfig.getConfig( CONFIG_UINT32_AHBOT_HORDE_PRICE_RATIO )+50;
            FactionChance = sAuctionBotConfig.getConfig( CONFIG_UINT32_AHBOT_BUYER_CHANCE_RATIO_HORDE );
            break;
        default:
            config.BuyerPriceRatio = sAuctionBotConfig.getConfig( CONFIG_UINT32_AHBOT_NEUTRAL_PRICE_RATIO )+50;
            FactionChance = sAuctionBotConfig.getConfig( CONFIG_UINT32_AHBOT_BUYER_CHANCE_RATIO_NEUTRAL );
            break;
    }
    config.FactionChance=5000*FactionChance;
}

void AuctionBotBuyer::LoadConfig()
{
    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
    {
        m_HouseConfig[i].BuyerEnabled = sAuctionBotConfig.getConfigBuyerEnabled(AuctionHouseType(i));
        if (m_HouseConfig[i].BuyerEnabled)
            LoadBuyerValues(m_HouseConfig[i]);
    }
}

uint32 AuctionBotBuyer::GetBuyableEntry(AHB_Buyer_Config& config)
{
    config.SameItemInfo.clear();
    uint32 count=0;
    time_t Now=time(NULL);

    AuctionHouseObject::AuctionEntryMapBounds bounds = sAuctionMgr.GetAuctionsMap(config.GetHouseType())->GetAuctionsBounds();
    for (AuctionHouseObject::AuctionEntryMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
    {
        AuctionEntry *Aentry = itr->second;
        Item *item = sAuctionMgr.GetAItem(Aentry->itemGuidLow);
        if (item)
        {
            ItemPrototype const *prototype = item->GetProto();
            if (prototype)
            {
                ++config.SameItemInfo[item->GetEntry()].ItemCount;    // Structure constructor will make sure Element are correctly initialised if entry is created here.
                config.SameItemInfo[item->GetEntry()].BuyPrice =config.SameItemInfo[item->GetEntry()].BuyPrice + (itr->second->buyout/item->GetCount());
                config.SameItemInfo[item->GetEntry()].BidPrice =config.SameItemInfo[item->GetEntry()].BidPrice + (itr->second->startbid/item->GetCount());
                if (itr->second->buyout != 0)
                {
                    if (itr->second->buyout/item->GetCount() < config.SameItemInfo[item->GetEntry()].MinBuyPrice)
                        config.SameItemInfo[item->GetEntry()].MinBuyPrice = itr->second->buyout/item->GetCount();
                    else if (config.SameItemInfo[item->GetEntry()].MinBuyPrice == 0)
                        config.SameItemInfo[item->GetEntry()].MinBuyPrice = itr->second->buyout/item->GetCount();
                }
                if (itr->second->startbid/item->GetCount() < config.SameItemInfo[item->GetEntry()].MinBidPrice)
                    config.SameItemInfo[item->GetEntry()].MinBidPrice = itr->second->startbid/item->GetCount();
                else if (config.SameItemInfo[item->GetEntry()].MinBidPrice == 0)
                    config.SameItemInfo[item->GetEntry()].MinBidPrice = itr->second->startbid/item->GetCount();

                if (!Aentry->owner)
                {

                    if ((Aentry->bid!=0) && Aentry->bidder) // Add bided by player
                    {
                        config.CheckedEntry[Aentry->Id].LastExist=Now;
                        config.CheckedEntry[Aentry->Id].AuctionId=Aentry->Id;
                        ++count;
                    }
                }
                else
                {
                    if (Aentry->bid!=0)
                    {
                        if  (Aentry->bidder)
                        {
                            config.CheckedEntry[Aentry->Id].LastExist=Now;
                            config.CheckedEntry[Aentry->Id].AuctionId=Aentry->Id;
                            ++count;
                        }
                    }
                    else
                    {
                        config.CheckedEntry[Aentry->Id].LastExist=Now;
                        config.CheckedEntry[Aentry->Id].AuctionId=Aentry->Id;
                        ++count;
                    }
                }
            }
        }
    }

    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: %u items added to buyable vector for ah type: %u",count, config.GetHouseType());
    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: SameItemInfo size = %u", config.SameItemInfo.size());
    return count;
}

void AuctionBotBuyer::PrepareListOfEntry(AHB_Buyer_Config& config)
{
    time_t Now=time(NULL)-5;

    for (CheckEntryMap::iterator itr=config.CheckedEntry.begin();itr != config.CheckedEntry.end();)
    {
        if (itr->second.LastExist  < (Now-5))
            config.CheckedEntry.erase(itr++);
        else
            ++itr;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: CheckedEntry size = %u",config.CheckedEntry.size());
}

bool AuctionBotBuyer::IsBuyableEntry(uint32 buyoutPrice, double InGame_BuyPrice, double MaxBuyablePrice, uint32 MinBuyPrice, uint32 MaxChance, uint32 ChanceRatio)
{
    double ratio=0;
    uint32 Chance=0;

    if (buyoutPrice <= MinBuyPrice)
    {
        if (buyoutPrice <= MaxBuyablePrice)
            Chance=MaxChance;
        else
        {

            if ((buyoutPrice > 0) && (MaxBuyablePrice > 0))
            {
                    ratio = buyoutPrice / MaxBuyablePrice;
                    if (ratio < 10)
                        Chance=MaxChance - (ratio*(MaxChance/10));
                    else Chance=1;
            }
        }
    }
    else if (buyoutPrice <= InGame_BuyPrice)
    {
        if (buyoutPrice <= MaxBuyablePrice)
            Chance=MaxChance/5;
        else
        {

            if ((buyoutPrice > 0) && (MaxBuyablePrice > 0))
            {
                    ratio = buyoutPrice / MaxBuyablePrice;
                    if (ratio < 10)
                        Chance=(MaxChance/5) - (ratio*(MaxChance/50));
                    else Chance=1;
            }
        }
    }
    else if (buyoutPrice <= MaxBuyablePrice)
        Chance = MaxChance/10;
    else
    {
        if ((buyoutPrice > 0) && (MaxBuyablePrice > 0))
        {
            ratio = buyoutPrice / MaxBuyablePrice;
            if (ratio < 10)
                Chance=(MaxChance/5) - (ratio*(MaxChance/50));
            else Chance=0;
         }
        else Chance = 0;
    }
    uint32 RandNum = urand(1,ChanceRatio);
    if (RandNum<=Chance)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: WIN BUY! Chance = %u, num = %u.",Chance, RandNum);
        return true;
    }
    else
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot:LOOSE BUY! Chance = %u, num = %u.",Chance, RandNum);
        return false;
    }
}

bool AuctionBotBuyer::IsBidableEntry(uint32 bidPrice, double InGame_BuyPrice, double MaxBidablePrice, uint32 MinBidPrice, uint32 MaxChance, uint32 ChanceRatio)
{
    double ratio=0;
    uint32 Chance=0;

    if (bidPrice <= MinBidPrice)
    {
        if ((InGame_BuyPrice != 0) && (bidPrice < (InGame_BuyPrice - (InGame_BuyPrice / 30))))
            Chance=MaxChance;
        else
        {
            if (bidPrice < MaxBidablePrice)
            {
                ratio = MaxBidablePrice / bidPrice;
                if (ratio < 3)
                    Chance = ((MaxChance/500)*ratio);
                else
                    Chance = (MaxChance/500);
            }
        }
    }
    else if (bidPrice < (InGame_BuyPrice - (InGame_BuyPrice / 30)))
        Chance=(MaxChance/10);
    else
    {
        if (bidPrice < MaxBidablePrice)
        {
            ratio = MaxBidablePrice / bidPrice;
            if (ratio < 4)
                Chance = ((MaxChance/1000)*ratio);
            else
                Chance = (MaxChance/1000);
        }
    }
    uint32 RandNum = urand(1,ChanceRatio);
    if (RandNum<=Chance)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: WIN BID! Chance = %u, num = %u.",Chance, RandNum);
        return true;
    } else
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: LOOSE BID! Chance = %u, num = %u.",Chance, RandNum);
        return false;
    }
}

void AuctionBotBuyer::PlaceBidToEntry(AuctionEntry* auction, uint32 bidPrice)
{
    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: Bid placed to entry %u, %.2fg", auction->Id, float(bidPrice) / 10000.0f);
    auction->UpdateBid(bidPrice);
}

void AuctionBotBuyer::BuyEntry(AuctionEntry* auction)
{
   DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: Entry %u buyed at %.2fg", auction->Id, float(auction->buyout) / 10000.0f);
    auction->UpdateBid(auction->buyout);
}

void AuctionBotBuyer::addNewAuctionBuyerBotBid(AHB_Buyer_Config& config)
{
    AuctionHouseObject* auctionHouse = sAuctionMgr.GetAuctionsMap(config.GetHouseType());

    PrepareListOfEntry(config);

    time_t Now = time(NULL);
    uint32 BuyCycles;
    if (config.CheckedEntry.size() > sAuctionBotConfig.GetItemPerCycleBoost())
    {
        BuyCycles=sAuctionBotConfig.GetItemPerCycleBoost();
        BASIC_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: Boost value used for Buyer! (if this happens often adjust both ItemsPerCycle in ahbot.conf)");
    }
    else
        BuyCycles=sAuctionBotConfig.GetItemPerCycleNormal();

    for (CheckEntryMap::iterator itr =config.CheckedEntry.begin(); itr != config.CheckedEntry.end();)
    {
        AuctionEntry* auction = auctionHouse->GetAuction(itr->second.AuctionId);
        if (!auction || auction->moneyDeliveryTime)         // is auction not active now
        {
            DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: Entry %u on ah %u doesn't exists, perhaps bought already?",
                itr->second.AuctionId, auction->GetHouseId());

            config.CheckedEntry.erase(itr++);
            continue;
        }

        if ((itr->second.LastChecked != 0) && ((Now - itr->second.LastChecked) <= m_CheckInterval))
        {
            DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: In time interval wait for entry %u!", auction->Id);
            ++itr;
            continue;
        }

        if (BuyCycles == 0)
            break;

        uint32 MaxChance=5000;

        Item *item = sAuctionMgr.GetAItem(auction->itemGuidLow);
        if (!item)                                          // auction item not accessible, possible auction in payment pending mode
        {
            config.CheckedEntry.erase(itr++);
            continue;
        }

        ItemPrototype const *prototype = item->GetProto();

        uint32 BasePrice = sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_BUYPRICE_BUYER) ? prototype->BuyPrice : prototype->SellPrice;
        BasePrice *= item->GetCount();

        double MaxBuyablePrice = ( BasePrice * config.BuyerPriceRatio )/100;
        BuyerItemInfoMap::iterator sameitem_itr = config.SameItemInfo.find(item->GetEntry());
        uint32 buyoutPrice = auction->buyout/item->GetCount();

        uint32 bidPrice;
        uint32 bidPriceByItem;
        if (auction->bid >= auction->startbid)
        {
            bidPrice = auction->GetAuctionOutBid();
            bidPriceByItem = auction->bid/item->GetCount();
        }
        else
        {
            bidPrice = auction->startbid;
            bidPriceByItem = auction->startbid/item->GetCount();
        }

        double InGame_BuyPrice;
        double InGame_BidPrice;
        if (sameitem_itr==config.SameItemInfo.end())
        {
            InGame_BuyPrice=0;
            InGame_BidPrice=0;
        }
        else
        {
            if (sameitem_itr->second.ItemCount == 1) MaxBuyablePrice = MaxBuyablePrice * 5; // if only one item exist can be buyed if the price is high too.
            InGame_BuyPrice=sameitem_itr->second.BuyPrice/sameitem_itr->second.ItemCount;
            InGame_BidPrice=sameitem_itr->second.BidPrice/sameitem_itr->second.ItemCount;
        }

        double MaxBidablePrice = MaxBuyablePrice - ( MaxBuyablePrice / 30); // Max Bidable price defined to 70% of max buyable price

        DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: Auction added with data:");
        DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: MaxPrice of Entry %u is %.1fg.", itr->second.AuctionId, MaxBuyablePrice / 10000);
        DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: GamePrice buy=%.1fg, bid=%.1fg.",InGame_BuyPrice/10000, InGame_BidPrice / 10000);
        DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: Minimal price see in AH Buy=%ug, Bid=%ug.",
            sameitem_itr->second.MinBuyPrice / 10000,sameitem_itr->second.MinBidPrice / 10000);
        DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: Actual Entry price,  Buy=%ug, Bid=%ug.", buyoutPrice / 10000, bidPrice/ 10000);

        if (!auction->owner)                // Original auction owner
        {
            MaxChance = MaxChance / 5;      // if Owner is AHBot this mean player placed bid on this auction. We divide by 5 chance for AhBuyer to place bid on it. (This make more challenge than ignore entry)
        }
        if (auction->buyout != 0)           // Is the item directly buyable?
        {
            if (IsBuyableEntry(buyoutPrice, InGame_BuyPrice, MaxBuyablePrice, sameitem_itr->second.MinBuyPrice, MaxChance, config.FactionChance))
            {
                if (IsBidableEntry(bidPriceByItem, InGame_BuyPrice, MaxBidablePrice, sameitem_itr->second.MinBidPrice, MaxChance/2, config.FactionChance))
                    if (urand(0,5)==0) PlaceBidToEntry(auction, bidPrice); else BuyEntry(auction);
                else
                    BuyEntry(auction);
            }
            else
            {
                if (IsBidableEntry(bidPriceByItem, InGame_BuyPrice, MaxBidablePrice, sameitem_itr->second.MinBidPrice, MaxChance/2, config.FactionChance))
                    PlaceBidToEntry(auction, bidPrice);
            }
        }
        else // buyout = 0 mean only bid are possible
            if (IsBidableEntry(bidPriceByItem, InGame_BuyPrice, MaxBidablePrice, sameitem_itr->second.MinBidPrice,MaxChance, config.FactionChance))
                PlaceBidToEntry(auction, bidPrice);

        itr->second.LastChecked = Now;
        --BuyCycles;

        ++itr;
    }
}

bool AuctionBotBuyer::Update(AuctionHouseType houseType)
{
    if (sAuctionBotConfig.getConfigBuyerEnabled(houseType))
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: %s buying ...", AuctionBotConfig::GetHouseTypeName(houseType));
        if (GetBuyableEntry(m_HouseConfig[houseType]) > 0)
            addNewAuctionBuyerBotBid(m_HouseConfig[houseType]);
        return true;
    }
    else return false;
}

//== AuctionBotSeller functions ============================

AuctionBotSeller::AuctionBotSeller()
{
    // Define faction for our main data class.
    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
        m_HouseConfig[i].Initialize(AuctionHouseType(i));
}

AuctionBotSeller::~AuctionBotSeller()
{
}

bool AuctionBotSeller::Initialize()
{
    std::vector<uint32> npcItems;
    std::vector<uint32> lootItems;
    std::vector<uint32> includeItems;
    std::vector<uint32> excludeItems;

    sLog.outString("AHBot seller filters:");
    sLog.outString();

    {
        std::stringstream includeStream(sAuctionBotConfig.GetAHBotIncludes());
        std::string temp;
        while (getline(includeStream,temp, ','))
            includeItems.push_back(atoi(temp.c_str()));
    }

    {
        std::stringstream excludeStream(sAuctionBotConfig.GetAHBotExcludes());
        std::string temp;
        while (getline(excludeStream,temp, ','))
            excludeItems.push_back(atoi(temp.c_str()));

    }
    sLog.outString("Forced Inclusion " SIZEFMTD " items", includeItems.size());
    sLog.outString("Forced Exclusion " SIZEFMTD " items", excludeItems.size());
    sLog.outString();

    sLog.outString("Loading npc vendor items for filter..");
    if (QueryResult* result = WorldDatabase.Query("SELECT DISTINCT item FROM npc_vendor"))
    {
        BarGoLink bar(result->GetRowCount());
        do
        {
            bar.step();
            Field* fields = result->Fetch();
            npcItems.push_back(fields[0].GetUInt32());

        } while (result->NextRow());
        delete result;
    }
    else
    {
        BarGoLink bar(1);
        bar.step();
    }
    sLog.outString("Npc vendor filter has " SIZEFMTD " items", npcItems.size());
    sLog.outString();

    sLog.outString("Loading loot items for filter..");
    if (QueryResult* result = WorldDatabase.PQuery(
        "SELECT item FROM creature_loot_template UNION "
        "SELECT item FROM disenchant_loot_template UNION "
        "SELECT item FROM fishing_loot_template UNION "
        "SELECT item FROM gameobject_loot_template UNION "
        "SELECT item FROM item_loot_template UNION "
        "SELECT item FROM milling_loot_template UNION "
        "SELECT item FROM pickpocketing_loot_template UNION "
        "SELECT item FROM prospecting_loot_template UNION "
        "SELECT item FROM skinning_loot_template UNION "
        "SELECT item FROM spell_loot_template"))
    {
        BarGoLink bar(result->GetRowCount());
        do
        {
            bar.step();
            Field* fields = result->Fetch();

            uint32 entry = fields[0].GetUInt32();
            if (!entry)
                continue;

            lootItems.push_back(fields[0].GetUInt32());
        } while (result->NextRow());
        delete result;
    }
    else
    {
        BarGoLink bar(1);
        bar.step();
    }
    sLog.outString("Loot filter has " SIZEFMTD " items", lootItems.size());
    sLog.outString();

    sLog.outString("Sorting and cleaning items for AHBot seller...");

    uint32 itemsAdded = 0;

    BarGoLink bar(sItemStorage.MaxEntry);
    for (uint32 itemID = 0; itemID < sItemStorage.MaxEntry; ++itemID)
    {
        ItemPrototype const* prototype = sObjectMgr.GetItemPrototype(itemID);

        bar.step();

        if (!prototype)
            continue;

        // skip items with too high quality (code can't propertly work with its)
        if (prototype->Quality >= MAX_AUCTION_QUALITY)
            continue;

        // forced exclude filter
        bool isExcludeItem = false;
        for (size_t i = 0; (i < excludeItems.size() && (!isExcludeItem)); ++i)
            if (itemID ==excludeItems[i])
                isExcludeItem = true;
        if (isExcludeItem)
            continue;

        // forced include filter
        bool isForcedIncludeItem = false;
        for (size_t i = 0; (i < includeItems.size() && (!isForcedIncludeItem)); ++i)
            if (itemID == includeItems[i])
                isForcedIncludeItem = true;

        if (isForcedIncludeItem)
        {
            m_ItemPool[prototype->Quality][prototype->Class].push_back(itemID);
            ++itemsAdded;
            continue;
        }

        // bounding filters
        switch (prototype->Bonding)
        {
            case NO_BIND:
                if (!sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_BIND_NO))
                    continue;
                break;
            case BIND_WHEN_PICKED_UP:
                if (!sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_BIND_PICKUP))
                    continue;
                break;
            case BIND_WHEN_EQUIPPED:
                if (!sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_BIND_EQUIP))
                    continue;
                break;
            case BIND_WHEN_USE:
                if (!sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_BIND_USE))
                    continue;
                break;
            case BIND_QUEST_ITEM:
                if (!sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_BIND_QUEST))
                    continue;
                break;
            default:
                continue;
        }

        // no price filter
        if (sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_BUYPRICE_SELLER))
        {
            if(prototype->BuyPrice == 0)
                continue;
        }
        else
        {
            if (prototype->SellPrice == 0)
                continue;
        }

        // vendor filter
        if (!sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_ITEMS_VENDOR))
        {
            bool isVendorItem = false;
            for (size_t i = 0; (i < npcItems.size()) && (!isVendorItem); ++i)
                if (itemID == npcItems[i])
                    isVendorItem = true;

            if (isVendorItem)
                continue;
        }

        // loot filter
        if (!sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_ITEMS_LOOT))
        {
            bool isLootItem = false;
            for (size_t i = 0; (i < lootItems.size()) && (!isLootItem); ++i)
                if (itemID == lootItems[i])
                    isLootItem = true;
            if (isLootItem)
                continue;
        }

        // not vendor/loot filter
        if (!sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_ITEMS_MISC))
        {
            bool isVendorItem = false;
            bool isLootItem = false;

            for (size_t i = 0; (i < npcItems.size()) && (!isVendorItem); ++i)
                if (itemID == npcItems[i])
                    isVendorItem = true;

            for (size_t i = 0; (i < lootItems.size()) && (!isLootItem); ++i)
                if (itemID == lootItems[i])
                    isLootItem = true;

            if ((!isLootItem) && (!isVendorItem))
                continue;
        }

        // item class/subclass specific filters
        switch (prototype->Class)
        {
            case ITEM_CLASS_ARMOR:
            case ITEM_CLASS_WEAPON:
            {
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_MIN_ITEM_LEVEL))
                    if (prototype->ItemLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_MAX_ITEM_LEVEL))
                    if (prototype->ItemLevel > value)
                        continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_MIN_REQ_LEVEL))
                    if (prototype->RequiredLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_MAX_REQ_LEVEL))
                    if (prototype->RequiredLevel > value)
                        continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_MIN_SKILL_RANK))
                    if (prototype->RequiredSkillRank < value)
                    continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_MAX_SKILL_RANK))
                    if (prototype->RequiredSkillRank > value)
                        continue;
                break;
            }
            case ITEM_CLASS_RECIPE:
            case ITEM_CLASS_CONSUMABLE:
            case ITEM_CLASS_PROJECTILE:
            {
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_MIN_REQ_LEVEL))
                    if (prototype->RequiredLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_MAX_REQ_LEVEL))
                    if (prototype->RequiredLevel > value)
                        continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_MIN_SKILL_RANK))
                    if (prototype->RequiredSkillRank < value)
                        continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_MAX_SKILL_RANK))
                    if (prototype->RequiredSkillRank > value)
                        continue;
                break;
            }
            case ITEM_CLASS_MISC:
                if (prototype->SubClass==ITEM_SUBCLASS_JUNK_MOUNT)
                {
                    if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_MOUNT_MIN_REQ_LEVEL))
                        if (prototype->RequiredLevel < value)
                            continue;
                    if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_MOUNT_MAX_REQ_LEVEL))
                        if (prototype->RequiredLevel > value)
                            continue;
                    if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_MOUNT_MIN_SKILL_RANK))
                        if (prototype->RequiredSkillRank < value)
                            continue;
                    if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_MOUNT_MAX_SKILL_RANK))
                        if (prototype->RequiredSkillRank > value)
                            continue;
                }

                if (prototype->Flags & ITEM_FLAG_LOOTABLE)
                {
                    // skip any not locked lootable items (mostly quest specific or reward cases)
                    if (!prototype->LockID)
                        continue;

                    if (!sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_LOCKBOX_ENABLED))
                        continue;
                }

                break;
            case ITEM_CLASS_GLYPH:
            {
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GLYPH_MIN_REQ_LEVEL))
                    if (prototype->RequiredLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GLYPH_MAX_REQ_LEVEL))
                    if (prototype->RequiredLevel > value)
                        continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GLYPH_MIN_ITEM_LEVEL))
                    if (prototype->RequiredLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GLYPH_MAX_ITEM_LEVEL))
                    if (prototype->RequiredLevel > value)
                        continue;
                break;
            }
            case ITEM_CLASS_TRADE_GOODS:
            {
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_MIN_ITEM_LEVEL))
                    if (prototype->ItemLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_MAX_ITEM_LEVEL))
                    if (prototype->ItemLevel > value)
                        continue;
                break;
            }
            case ITEM_CLASS_CONTAINER:
            case ITEM_CLASS_QUIVER:
            {
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_CONTAINER_MIN_ITEM_LEVEL))
                    if (prototype->ItemLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_CONTAINER_MAX_ITEM_LEVEL))
                    if (prototype->ItemLevel > value)
                        continue;
                break;
            }
        }

        m_ItemPool[prototype->Quality][prototype->Class].push_back(itemID);
        ++itemsAdded;
    }

    if (!itemsAdded)
    {
        sLog.outError("AuctionHouseBot seller not have items, disabled.");
        sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO, 0);
        sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_HORDE_ITEM_AMOUNT_RATIO, 0);
        sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO, 0);
        return false;
    }

    sLog.outString("AuctionHouseBot seller will use %u items to fill auction house (according your config choices)", itemsAdded);

    LoadConfig();

    sLog.outString("Items loaded      \tGrey\tWhite\tGreen\tBlue\tPurple\tOrange\tYellow");
    for (uint32 i = 0; i < MAX_ITEM_CLASS; ++i)
        sLog.outString("%-18s\t" SIZEFMTD "\t" SIZEFMTD "\t" SIZEFMTD "\t" SIZEFMTD "\t" SIZEFMTD "\t" SIZEFMTD "\t" SIZEFMTD,
            sAuctionBotConfig.GetItemClassName(ItemClass(i)),
            m_ItemPool[0][i].size(), m_ItemPool[1][i].size(), m_ItemPool[2][i].size(),
            m_ItemPool[3][i].size(), m_ItemPool[4][i].size(), m_ItemPool[5][i].size(),
            m_ItemPool[6][i].size());

    sLog.outString();
    sLog.outString("AHBot seller configuration data loaded and initilized");

    sLog.SetLogFilter(LOG_FILTER_AHBOT_SELLER, !sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_DEBUG_SELLER));
    return true;
}

void AuctionBotSeller::LoadConfig()
{
    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
        if (sAuctionBotConfig.getConfigItemAmountRatio(AuctionHouseType(i)))
            LoadSellerValues(m_HouseConfig[i]);
}

void AuctionBotSeller::LoadItemsQuantity(AHB_Seller_Config& config)
{
    uint32 ratio = sAuctionBotConfig.getConfigItemAmountRatio(config.GetHouseType());

    config.SetItemsAmountPerQuality(AUCTION_QUALITY_GREY, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_GREY_AMOUNT) * ratio / 100);
    config.SetItemsAmountPerQuality(AUCTION_QUALITY_WHITE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_WHITE_AMOUNT) * ratio / 100);
    config.SetItemsAmountPerQuality(AUCTION_QUALITY_GREEN, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_GREEN_AMOUNT) * ratio / 100);
    config.SetItemsAmountPerQuality(AUCTION_QUALITY_BLUE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_BLUE_AMOUNT) * ratio / 100);
    config.SetItemsAmountPerQuality(AUCTION_QUALITY_PURPLE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_PURPLE_AMOUNT) * ratio / 100);
    config.SetItemsAmountPerQuality(AUCTION_QUALITY_ORANGE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_ORANGE_AMOUNT) * ratio / 100);
    config.SetItemsAmountPerQuality(AUCTION_QUALITY_YELLOW, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ITEM_YELLOW_AMOUNT) * ratio / 100);

    // Set quantity wanted but only on possible item color
    // This avoid any no-exist class-color items selection by random items create function
    // ============================================================================================
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_CONSUMABLE, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_CONTAINER, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_WEAPON, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_WEAPON_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_GEM, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_ARMOR, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_ARMOR_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_REAGENT, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_PROJECTILE, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_TRADE_GOODS, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_GENERIC, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_RECIPE, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_QUIVER, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_QUEST, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_QUEST_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_KEY, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_MISC, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREY, ITEM_CLASS_GLYPH, 0);

    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_CONSUMABLE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_CONSUMABLE_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_CONTAINER, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_CONTAINER_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_WEAPON, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_WEAPON_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_GEM, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GEM_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_ARMOR, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_ARMOR_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_REAGENT, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_REAGENT_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_PROJECTILE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_PROJECTILE_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_TRADE_GOODS, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_GENERIC, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GENERIC_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_RECIPE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_RECIPE_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_QUIVER, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_QUIVER_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_QUEST, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_QUEST_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_KEY, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_KEY_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_MISC, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_WHITE, ITEM_CLASS_GLYPH, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GLYPH_AMOUNT));

    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_CONSUMABLE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_CONSUMABLE_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_CONTAINER, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_CONTAINER_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_WEAPON, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_WEAPON_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_GEM, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GEM_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_ARMOR, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_ARMOR_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_REAGENT, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_PROJECTILE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_PROJECTILE_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_TRADE_GOODS, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_GENERIC, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_RECIPE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_RECIPE_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_QUIVER, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_QUIVER_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_QUEST, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_QUEST_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_KEY, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_KEY_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_MISC, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_GREEN, ITEM_CLASS_GLYPH, 0);

    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_CONSUMABLE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_CONSUMABLE_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_CONTAINER, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_CONTAINER_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_WEAPON, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_WEAPON_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_GEM, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GEM_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_ARMOR, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_ARMOR_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_REAGENT, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_PROJECTILE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_PROJECTILE_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_TRADE_GOODS, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_GENERIC, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_RECIPE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_RECIPE_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_QUIVER, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_QUIVER_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_QUEST, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_QUEST_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_KEY, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_MISC, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_BLUE, ITEM_CLASS_GLYPH, 0);

    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_CONSUMABLE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_CONSUMABLE_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_CONTAINER, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_CONTAINER_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_WEAPON, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_WEAPON_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_GEM, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GEM_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_ARMOR, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_ARMOR_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_REAGENT, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_PROJECTILE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_PROJECTILE_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_TRADE_GOODS, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_GENERIC, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_RECIPE, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_RECIPE_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_QUIVER, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_QUEST, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_QUEST_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_KEY, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_MISC, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_PURPLE, ITEM_CLASS_GLYPH, 0);

    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_CONSUMABLE, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_CONTAINER, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_WEAPON, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_WEAPON_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_GEM, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_ARMOR, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_ARMOR_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_REAGENT, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_PROJECTILE, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_TRADE_GOODS, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_GENERIC, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_RECIPE, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_QUIVER, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_QUEST, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_KEY, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_MISC, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_ORANGE, ITEM_CLASS_GLYPH, 0);

    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_CONSUMABLE, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_CONTAINER, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_WEAPON, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_WEAPON_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_GEM, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_ARMOR, sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_ARMOR_AMOUNT));
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_REAGENT, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_PROJECTILE, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_TRADE_GOODS, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_GENERIC, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_RECIPE, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_QUIVER, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_QUEST, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_KEY, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_MISC, 0);
    config.SetItemsQuantityPerClass(AUCTION_QUALITY_YELLOW, ITEM_CLASS_GLYPH, 0);
    // ============================================================================================

    // Set the best value to get nearest amount of items wanted
    for (uint32 j = 0; j < MAX_AUCTION_QUALITY; ++j)
    {
        uint32 indice = config.GetItemsAmountPerQuality(AuctionQuality(j)) /
            (sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_CONSUMABLE_AMOUNT) + sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_CONTAINER_AMOUNT) + sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_WEAPON_AMOUNT) +
            sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GEM_AMOUNT) + sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_ARMOR_AMOUNT) + sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_REAGENT_AMOUNT) +
            sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_PROJECTILE_AMOUNT) + sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_AMOUNT) + sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GENERIC_AMOUNT) +
            sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_RECIPE_AMOUNT) + sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_QUIVER_AMOUNT) + sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_QUEST_AMOUNT) +
            sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_KEY_AMOUNT) + sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_MISC_AMOUNT) + sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_CLASS_GLYPH_AMOUNT));
        for (uint32 i = 0; i < MAX_ITEM_CLASS; ++i)
            config.SetItemsAmountPerClass(AuctionQuality(j), ItemClass(i), indice);
    }
}

void AuctionBotSeller::LoadSellerValues(AHB_Seller_Config& config)
{
    LoadItemsQuantity(config);
    uint32 PriceRatio;
    switch(config.GetHouseType())
    {
        case AUCTION_HOUSE_ALLIANCE:
            PriceRatio = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_ALLIANCE_PRICE_RATIO);
            break;
        case AUCTION_HOUSE_HORDE:
            PriceRatio = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_HORDE_PRICE_RATIO);
            break;
        default:
            PriceRatio = sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_NEUTRAL_PRICE_RATIO);
            break;
    }
    config.SetPriceRatioPerQuality(AUCTION_QUALITY_GREY,PriceRatio);
    config.SetPriceRatioPerQuality(AUCTION_QUALITY_WHITE,PriceRatio);
    config.SetPriceRatioPerQuality(AUCTION_QUALITY_GREEN,PriceRatio);
    config.SetPriceRatioPerQuality(AUCTION_QUALITY_BLUE,PriceRatio);
    config.SetPriceRatioPerQuality(AUCTION_QUALITY_PURPLE,PriceRatio);
    config.SetPriceRatioPerQuality(AUCTION_QUALITY_ORANGE,PriceRatio);
    config.SetPriceRatioPerQuality(AUCTION_QUALITY_YELLOW,PriceRatio);

    //load min and max auction times
    config.SetMinTime(sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_MINTIME));
    config.SetMaxTime(sAuctionBotConfig.getConfig(CONFIG_UINT32_AHBOT_MAXTIME));

    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: minTime = %u", config.GetMinTime());
    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: maxTime = %u", config.GetMaxTime());

    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: For AH type %u",config.GetHouseType());
    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: GreyItems = %u", config.GetItemsAmountPerQuality(AUCTION_QUALITY_GREY));
    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: WhiteItems = %u", config.GetItemsAmountPerQuality(AUCTION_QUALITY_WHITE));
    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: GreenItems = %u", config.GetItemsAmountPerQuality(AUCTION_QUALITY_GREEN));
    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: BlueItems = %u", config.GetItemsAmountPerQuality(AUCTION_QUALITY_BLUE));
    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: PurpleItems = %u", config.GetItemsAmountPerQuality(AUCTION_QUALITY_PURPLE));
    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: OrangeItems = %u", config.GetItemsAmountPerQuality(AUCTION_QUALITY_ORANGE));
    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: YellowItems = %u", config.GetItemsAmountPerQuality(AUCTION_QUALITY_YELLOW));

}

// Set static of items on one AH faction.
// Fill ItemInfos object with real content of AH.
uint32 AuctionBotSeller::SetStat(AHB_Seller_Config& config)
{
    std::vector<std::vector<uint32> > ItemsInAH(MAX_AUCTION_QUALITY, std::vector< uint32 > (MAX_ITEM_CLASS));

    AuctionHouseObject::AuctionEntryMapBounds bounds = sAuctionMgr.GetAuctionsMap(config.GetHouseType())->GetAuctionsBounds();
    for (AuctionHouseObject::AuctionEntryMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
    {
        AuctionEntry *Aentry = itr->second;
        Item *item = sAuctionMgr.GetAItem(Aentry->itemGuidLow);
        if (item)
        {
            ItemPrototype const *prototype = item->GetProto();
            if (prototype)
            {
                if (!Aentry->owner)                         // Add only ahbot items
                {
                    ++ItemsInAH[prototype->Quality][prototype->Class];
                }
            }
        }
    }
    uint32 count=0;
    for (uint32 j=0; j<MAX_AUCTION_QUALITY; ++j)
    {
        for (uint32 i=0;i<MAX_ITEM_CLASS;++i)
        {
            config.SetMissedItemsPerClass((AuctionQuality) j, (ItemClass) i, ItemsInAH[j][i]);
            count+=config.GetMissedItemsPerClass((AuctionQuality) j, (ItemClass) i);
        }
    }

    DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: Missed Item       \tGrey\tWhite\tGreen\tBlue\tPurple\tOrange\tYellow");
    for (uint32 i=0; i<MAX_ITEM_CLASS;++i)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: %-18s\t%u\t%u\t%u\t%u\t%u\t%u\t%u",
            sAuctionBotConfig.GetItemClassName(ItemClass(i)),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_GREY, (ItemClass) i),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_WHITE, (ItemClass) i),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_GREEN, (ItemClass) i),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_BLUE, (ItemClass) i),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_PURPLE, (ItemClass) i),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_ORANGE, (ItemClass) i),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_YELLOW, (ItemClass) i));
    }
    config.LastMissedItem = count;
    return count;
}

// getRandomArray is used to make aviable the possibility to add any of missed item in place of first one to last one.
bool AuctionBotSeller::getRandomArray( AHB_Seller_Config& config, RandomArray& ra, const std::vector<std::vector<uint32> >& addedItem  )
{
    ra.clear();
    bool Ok=false;

    for (uint32 j = 0; j < MAX_AUCTION_QUALITY; ++j)
    {
        for (uint32 i = 0; i < MAX_ITEM_CLASS; ++i)
        {
            if ((config.GetMissedItemsPerClass(AuctionQuality(j), ItemClass(i)) > addedItem[j][i]) && !m_ItemPool[j][i].empty())
            {
                RandomArrayEntry miss_item;
                miss_item.color = j;
                miss_item.itemclass = i;
                ra.push_back(miss_item);
                Ok=true;
            }
        }
    }
    return Ok;
}

// Set items price. All important value are passed by address.
void AuctionBotSeller::SetPricesOfItem(ItemPrototype const *itemProto, AHB_Seller_Config& config, uint32& buyp, uint32& bidp, uint32 stackcnt, ItemQualities itemQuality)
{
    double temp_buyp = buyp * stackcnt *
        (itemQuality < MAX_AUCTION_QUALITY ? config.GetPriceRatioPerQuality(AuctionQuality(itemQuality)) : 1) ;

    double randrange = temp_buyp * 0.4;
    buyp = (urand(temp_buyp-randrange, temp_buyp+randrange)/100)+1;
    double urandrange=buyp*40;
    double temp_bidp = buyp*50;
    bidp = (urand(temp_bidp-urandrange, temp_bidp+urandrange)/100)+1;
}

void AuctionBotSeller::SetItemsRatio(uint32 al, uint32 ho, uint32 ne)
{
    sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO, al < 10000 ? al : 10000);
    sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_HORDE_ITEM_AMOUNT_RATIO, ho < 10000 ? ho : 10000);
    sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO, ne < 10000 ? ne : 10000);

    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
        LoadItemsQuantity(m_HouseConfig[i]);
}

void AuctionBotSeller::SetItemsRatioForHouse(AuctionHouseType house, uint32 val)
{
    if (val > 10000)                                        // apply same upper limit as used for config load
        val = 10000;

    switch (house)
    {
        case AUCTION_HOUSE_ALLIANCE: sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO, val); break;
        case AUCTION_HOUSE_HORDE:    sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_HORDE_ITEM_AMOUNT_RATIO, val); break;
        default:                     sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO, val); break;
    }

    LoadItemsQuantity(m_HouseConfig[house]);
}

void AuctionBotSeller::SetItemsAmount(uint32 (&vals) [MAX_AUCTION_QUALITY])
{
    sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_GREY_AMOUNT, vals[AUCTION_QUALITY_GREY]);
    sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_WHITE_AMOUNT, vals[AUCTION_QUALITY_WHITE]);
    sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_GREEN_AMOUNT, vals[AUCTION_QUALITY_GREEN]);
    sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_BLUE_AMOUNT, vals[AUCTION_QUALITY_BLUE]);
    sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_PURPLE_AMOUNT, vals[AUCTION_QUALITY_PURPLE]);
    sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_ORANGE_AMOUNT, vals[AUCTION_QUALITY_ORANGE]);
    sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_YELLOW_AMOUNT, vals[AUCTION_QUALITY_YELLOW]);

    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
        LoadItemsQuantity(m_HouseConfig[i]);
}

void AuctionBotSeller::SetItemsAmountForQuality(AuctionQuality quality, uint32 val)
{
    switch (quality)
    {
        case AUCTION_QUALITY_GREY:  sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_GREY_AMOUNT, val); break;
        case AUCTION_QUALITY_WHITE: sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_WHITE_AMOUNT, val); break;
        case AUCTION_QUALITY_GREEN: sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_GREEN_AMOUNT, val); break;
        case AUCTION_QUALITY_BLUE:  sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_BLUE_AMOUNT, val); break;
        case AUCTION_QUALITY_PURPLE:sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_PURPLE_AMOUNT, val); break;
        case AUCTION_QUALITY_ORANGE:sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_ORANGE_AMOUNT, val); break;
        default:                    sAuctionBotConfig.setConfig(CONFIG_UINT32_AHBOT_ITEM_YELLOW_AMOUNT, val); break;
    }

    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
        LoadItemsQuantity(m_HouseConfig[i]);
}

// Add new auction to one of the factions.
// Faction and setting assossiated is defined passed argument ( config )
void AuctionBotSeller::addNewAuctions(AHB_Seller_Config& config)
{
    uint32 items;

    // If there is large amount of items missed we can use boost value to get fast filled AH
    if (config.LastMissedItem > sAuctionBotConfig.GetItemPerCycleBoost())
    {
        items=sAuctionBotConfig.GetItemPerCycleBoost();
        BASIC_FILTER_LOG(LOG_FILTER_AHBOT_BUYER, "AHBot: Boost value used to fill AH! (if this happens often adjust both ItemsPerCycle in ahbot.conf)");
    }
    else items=sAuctionBotConfig.GetItemPerCycleNormal();

    uint32 houseid;
    switch (config.GetHouseType())
    {
        case AUCTION_HOUSE_ALLIANCE: houseid =  1; break;
        case AUCTION_HOUSE_HORDE:    houseid =  6; break;
        default:                     houseid =  7; break;
    }

    AuctionHouseEntry const* ahEntry = sAuctionHouseStore.LookupEntry(houseid);

    AuctionHouseObject* auctionHouse = sAuctionMgr.GetAuctionsMap(config.GetHouseType());

    RandomArray randArray;
    std::vector<std::vector<uint32> > ItemsAdded(MAX_AUCTION_QUALITY,std::vector<uint32> (MAX_ITEM_CLASS));
    // Main loop
    // getRandomArray will give what categories of items should be added (return true if there is at least 1 items missed)
    while (getRandomArray(config,randArray, ItemsAdded) && (items>0))
    {
        --items;

        // Select random position from missed items table
        uint32 pos =  (urand(0,randArray.size()-1));

        // Set itemID with random item ID for selected categories and color, from m_ItemPool table
        uint32 itemID = m_ItemPool[randArray[pos].color][randArray[pos].itemclass][urand(0,m_ItemPool[randArray[pos].color][randArray[pos].itemclass].size()-1)];
        ++ ItemsAdded[randArray[pos].color][randArray[pos].itemclass]; // Helper table to avoid rescan from DB in this loop. (has we add item in random orders)

        if (!itemID)
        {
            DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: Item entry 0 auction creating attempt.");
            continue;
        }

        ItemPrototype const* prototype = sObjectMgr.GetItemPrototype(itemID);
        if (!prototype)
        {
            DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: Unknown item %u auction creating attempt.", itemID);
            continue;
        }

        uint32 stackCount = urand(1, prototype->GetMaxStackSize());


        Item* item = Item::CreateItem(itemID, stackCount);
        if (!item)
        {
            sLog.outError("AHBot: Item::CreateItem() returned NULL for item %u (stack: %u)", itemID, stackCount);
            return;
        }

        uint32 buyoutPrice;
        uint32 bidPrice = 0;
        // Not sure if i will keep the next test
        if (sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_BUYPRICE_SELLER))
            buyoutPrice  = prototype->BuyPrice * item->GetCount();
        else
            buyoutPrice  = prototype->SellPrice * item->GetCount();
        // Price of items are set here
        SetPricesOfItem(prototype, config, buyoutPrice, bidPrice, stackCount, ItemQualities(prototype->Quality));

        auctionHouse->AddAuction(ahEntry, item, urand(config.GetMinTime(), config.GetMaxTime()) * HOUR, bidPrice, buyoutPrice);
    }
}

bool AuctionBotSeller::Update(AuctionHouseType houseType)
{
    if (sAuctionBotConfig.getConfigItemAmountRatio(houseType) > 0)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AHBOT_SELLER, "AHBot: %s selling ...", AuctionBotConfig::GetHouseTypeName(houseType));
        if (SetStat(m_HouseConfig[houseType]))
            addNewAuctions(m_HouseConfig[houseType]);
        return true;
    }
    else
        return false;
}

//== AuctionHouseBot functions =============================

AuctionHouseBot::AuctionHouseBot() : m_Buyer(NULL), m_Seller(NULL), m_OperationSelector(0)
{
}

AuctionHouseBot::~AuctionHouseBot()
{
    delete m_Buyer;
    delete m_Seller;
}

void AuctionHouseBot::InitilizeAgents()
{
    if (sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_SELLER_ENABLED))
    {
        delete m_Seller;
        m_Seller = new AuctionBotSeller();
        if (!m_Seller->Initialize())
        {
            delete m_Seller;
            m_Seller = NULL;
        }
    }
    if (sAuctionBotConfig.getConfig(CONFIG_BOOL_AHBOT_BUYER_ENABLED))
    {
        delete m_Buyer;
        m_Buyer = new AuctionBotBuyer();
        if (!m_Buyer->Initialize())
        {
            delete m_Buyer;
            m_Buyer = NULL;
        }
    }
}

void AuctionHouseBot::Initialize()
{
    if (sAuctionBotConfig.Initialize())
        InitilizeAgents();
}

void AuctionHouseBot::SetItemsRatio(uint32 al, uint32 ho, uint32 ne)
{
    if (AuctionBotSeller* seller = dynamic_cast<AuctionBotSeller*>(m_Seller))
        seller->SetItemsRatio(al, ho, ne);
}

void AuctionHouseBot::SetItemsRatioForHouse(AuctionHouseType house, uint32 val)
{
    if (AuctionBotSeller* seller = dynamic_cast<AuctionBotSeller*>(m_Seller))
        seller->SetItemsRatioForHouse(house, val);
}

void AuctionHouseBot::SetItemsAmount(uint32 (&vals) [MAX_AUCTION_QUALITY])
{
    if (AuctionBotSeller* seller = dynamic_cast<AuctionBotSeller*>(m_Seller))
        seller->SetItemsAmount(vals);
}

void AuctionHouseBot::SetItemsAmountForQuality(AuctionQuality quality, uint32 val)
{
    if (AuctionBotSeller* seller = dynamic_cast<AuctionBotSeller*>(m_Seller))
        seller->SetItemsAmountForQuality(quality, val);
}

bool AuctionHouseBot::ReloadAllConfig()
{
    if (!sAuctionBotConfig.Reload())
    {
        sLog.outError("AHBot: Error while trying to reload config from file!");
        return false;
    }

    InitilizeAgents();
    return true;
}

void AuctionHouseBot::PrepareStatusInfos(AuctionHouseBotStatusInfo& statusInfo)
{
    for (uint32 i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
    {
        statusInfo[i].ItemsCount = 0;

        for (int j = 0; j < MAX_AUCTION_QUALITY; ++j)
            statusInfo[i].QualityInfo[j] = 0;

        AuctionHouseObject::AuctionEntryMapBounds bounds = sAuctionMgr.GetAuctionsMap(AuctionHouseType(i))->GetAuctionsBounds();
        for (AuctionHouseObject::AuctionEntryMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
        {
            AuctionEntry *Aentry = itr->second;
            if (Item *item = sAuctionMgr.GetAItem(Aentry->itemGuidLow))
            {
                ItemPrototype const *prototype = item->GetProto();
                if (!Aentry->owner)                         // Add only ahbot items
                {
                    if (prototype->Quality < MAX_AUCTION_QUALITY)
                        ++statusInfo[i].QualityInfo[prototype->Quality];

                    ++statusInfo[i].ItemsCount;
                }
            }
        }
    }
}

void AuctionHouseBot::Rebuild(bool all)
{
    for (uint32 i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
    {
        AuctionHouseObject::AuctionEntryMapBounds bounds = sAuctionMgr.GetAuctionsMap(AuctionHouseType(i))->GetAuctionsBounds();
        for (AuctionHouseObject::AuctionEntryMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
            if (!itr->second->owner)                        // ahbot auction
                if (all || itr->second->bid == 0)           // expire now auction if no bid or forced
                    itr->second->expireTime = sWorld.GetGameTime();
    }
}

void AuctionHouseBot::Update()
{
    // nothing do...
    if (!m_Buyer && !m_Seller)
         return;

    // scan all possible update cases until first success
    for (uint32 count = 0; count < 2*MAX_AUCTION_HOUSE_TYPE; ++count)
    {
        bool successStep = false;

        if (m_OperationSelector < MAX_AUCTION_HOUSE_TYPE)
        {
            if (m_Seller)
                successStep = m_Seller->Update(AuctionHouseType(m_OperationSelector));
        }
        else
        {
            if (m_Buyer)
                successStep = m_Buyer->Update(AuctionHouseType(m_OperationSelector - MAX_AUCTION_HOUSE_TYPE));
        }

        ++m_OperationSelector;
        if (m_OperationSelector >= 2*MAX_AUCTION_HOUSE_TYPE)
            m_OperationSelector = 0;

        // one success update per call
        if (successStep)
            break;
    }
}
