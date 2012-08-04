################################################
# MANGOS Auction House Bot Configuration file  #
################################################

[AhbotConf]
ConfVersion=2010102201

###################################################################################################################
# AUCTION HOUSE BOT SETTINGS
#
#    AuctionHouseBot.Seller.Enabled
#        General enable or disable AuctionHouseBot Seller fonctionality
#    Default 0 (Disabled)
#
#    AuctionHouseBot.DEBUG.Seller
#        Enable or disable AuctionHouseBot Seller debug mode
#    Default 0 (Disabled)
#
#    AuctionHouseBot.Alliance.Items.Amount.Ratio
#        Enable/Disable (disabled if 0) the part of AHBot that puts items up for auction on Alliance AH
#    Default 100 (Enabled with 100% of items specified in AuctionHouse.Items.Amount.color section)
#
#    AuctionHouseBot.Horde.Items.Amount.Ratio
#        Enable/Disable (disabled if 0) the part of AHBot that puts items up for auction on Horde AH
#    Default 100 (Enabled with 100% of items specified in AuctionHouse.Items.Amount.color section)
#
#    AuctionHouseBot.Neutral.Items.Amount.Ratio
#        Enable/Disable (disabled if 0) the part of AHBot that puts items up for auction on Neutral AH
#    Default 100 (Enabled with 100% of items specified in AuctionHouse.Items.Amount.color section)
#
#    AuctionHouseBot.MinTime
#        Minimum time for the new auction
#    Default 1 (Hour)
#
#    AuctionHouseBot.MaxTime
#        Maximum time for the new auction
#    Default 72 (Hours)
#
#    AuctionHouseBot.Items.Vendor
#        Include items that can be bought from vendors.
#    Default 0
#
#    AuctionHouseBot.Items.Loot
#        Include items that can be looted or fished for.
#    Default 1
#
#    AuctionHouseBot.Items.Misc
#        Include misc. items.
#    Default 0
#
#    AuctionHouseBot.Bind.*
#        Indicates which bonding types to allow the bot to put up for auction
#            No     - Items that don't bind            Default 1 (Allowed)
#            Pickup - Items that bind on pickup        Default 0 (Not Allowed)
#            Equip  - Items that bind on equip         Default 1 (Allowed)
#            Use    - Items that bind on use           Default 1 (Allowed)
#            Quest  - Quest Items                      Default 0 (Not Allowed)
#
#    AuctionHouseBot.LockBox.Enabled
#        Enable or not lockbox in auctionhouse
#    Default 0 (Disabled)
#
#    AuctionHouseBot.ItemsPerCycle.Boost
#        This value is used to fill DB faster than normal when there is more than this value missed items.
#        Normaly this value is used only first start of the server with empty auction table.
#    Default 75
#
#    AuctionHouseBot.ItemsPerCycle.Normal
#        This value is used to fill DB normal way with less cpu/db using.
#        Normaly this value is used always when auction table is already initialised.
#    Default 20
#
#    AuctionHouseBot.BuyPrice.Seller
#        Should the Seller use BuyPrice or SellPrice to determine Bid Prices
#    Default 1 (use SellPrice)
#
#    AuctionHouseBot.Alliance.Price.Ratio
#        Define the price of selled item here for the Alliance Auction House
#    Default 200
#
#    AuctionHouseBot.Horde.Price.Ratio
#        Define the price of selled item here for the Horde Auction House
#    Default 200
#
#    AuctionHouseBot.Neutral.Price.Ratio
#        Define the price of selled item here for the Neutral Auction House
#    Default 200
#
#    AuctionHouseBot.Items.ItemLevel.*
#        Prevent seller from listing items below/above this item level
#    Default 0 (Disabled)
#
#    AuctionHouseBot.Items.ReqLevel.*
#        Prevent seller from listing items below/above this required level
#    Default 0 (Disabled)
#
#    AuctionHouseBot.Items.ReqSkill.*
#        Prevent seller from listing items below/above this skill level
#    Default 0 (Disabled)
#
#    AuctionHouseBot.Items.Amount.*
#        Define here for every items quality how many item you whant to be show in Auction House
#        This value will be adjusted by AuctionHouseBot.FACTION.Items.Amount.Ratio to define exact amount of
#        items will be show on Auction House
#    Default 0, 2000, 2500, 1500, 1000, 0, 0 (grey, white, green, blue, purple, orange, yellow)
#
#    AuctionHouseBot.Class.*
#        Here you can set the class of items you prefer to be show on AH
#        These value is preference value, it's not percentage. So the maximum is 10.
#        The minimum is 0 (disabled).
#    Default 6,4,8,3,8,1,2,10,1,6,1,1,1,5,3
#
#
# ITEM FINE TUNING
#    The following are usefull for limiting what character levels can
#    benefit from the auction house
#
#    AuctionHouseBot.Class.Misc.Mount.ReqLevel.*
#        Prevent seller from listing mounts below/above this required level
#    Default 0
#
#    AuctionHouseBot.Class.Misc.Mount.ReqSkill.*
#        Prevent seller from listing mounts below/above this skill level
#    Default 0
#
#    AuctionHouseBot.Class.Glyph.ReqLevel.*
#        Prevent seller from listing glyphs below/above this required level
#    Default 0
#
#    AuctionHouseBot.Class.Glyph.ItemLevel.*
#        Prevent seller from listing glyphs below/above this item level
#    Default 0
#
#    AuctionHouseBot.Class.TradeGood.ItemLevel.*
#        Prevent seller from listing trade good items below/above this item level
#    Default 0
#
#    AuctionHouseBot.Class.Container.ItemLevel.*
#        Prevent seller from listing contianers below/above this item level
#    Default 0
#
#    AuctionHouseBot.forceIncludeItems
#        Include these items and ignore ALL filters
#        List of ids with delimiter ','
#    Default ""
#
#    AuctionHouseBot.forceExcludeItems
#        Exclude these items even if they would pass the filters
#        List of ids with delimiter ','
#        Example "21878,27774,27811,28117,28122,43949" (this removes zzOld items)
#    Default ""
#
###################################################################################################################

AuctionHouseBot.Seller.Enabled = 0
AuctionHouseBot.DEBUG.Seller = 0

AuctionHouseBot.Alliance.Items.Amount.Ratio = 100
AuctionHouseBot.Horde.Items.Amount.Ratio = 100
AuctionHouseBot.Neutral.Items.Amount.Ratio = 100

AuctionHouseBot.MinTime = 1
AuctionHouseBot.MaxTime = 72

AuctionHouseBot.Items.Vendor = 0
AuctionHouseBot.Items.Loot = 1
AuctionHouseBot.Items.Misc = 0
AuctionHouseBot.Bind.No = 1
AuctionHouseBot.Bind.Pickup = 0
AuctionHouseBot.Bind.Equip = 1
AuctionHouseBot.Bind.Use = 1
AuctionHouseBot.Bind.Quest = 0
AuctionHouseBot.LockBox.Enabled = 0

AuctionHouseBot.ItemsPerCycle.Boost = 75
AuctionHouseBot.ItemsPerCycle.Normal = 20
AuctionHouseBot.BuyPrice.Seller = 1
AuctionHouseBot.Alliance.Price.Ratio = 200
AuctionHouseBot.Horde.Price.Ratio = 200
AuctionHouseBot.Neutral.Price.Ratio = 200

AuctionHouseBot.Items.ItemLevel.Min = 0
AuctionHouseBot.Items.ItemLevel.Max = 0
AuctionHouseBot.Items.ReqLevel.Min = 0
AuctionHouseBot.Items.ReqLevel.Max = 0
AuctionHouseBot.Items.ReqSkill.Min = 0
AuctionHouseBot.Items.ReqSkill.Max = 0

AuctionHouseBot.Items.Amount.Grey = 0
AuctionHouseBot.Items.Amount.White = 2000
AuctionHouseBot.Items.Amount.Green = 2500
AuctionHouseBot.Items.Amount.Blue = 1500
AuctionHouseBot.Items.Amount.Purple = 1000
AuctionHouseBot.Items.Amount.Orange = 0
AuctionHouseBot.Items.Amount.Yellow = 0

AuctionHouseBot.Class.Consumable = 6
AuctionHouseBot.Class.Container = 4
AuctionHouseBot.Class.Weapon = 8
AuctionHouseBot.Class.Gem = 3
AuctionHouseBot.Class.Armor = 8
AuctionHouseBot.Class.Reagent = 1
AuctionHouseBot.Class.Projectile = 2
AuctionHouseBot.Class.TradeGood = 10
AuctionHouseBot.Class.Generic = 1
AuctionHouseBot.Class.Recipe = 6
AuctionHouseBot.Class.Quiver = 1
AuctionHouseBot.Class.Quest = 1
AuctionHouseBot.Class.Key = 1
AuctionHouseBot.Class.Misc = 5
AuctionHouseBot.Class.Glyph = 3

AuctionHouseBot.Class.Misc.Mount.ReqLevel.Min = 0
AuctionHouseBot.Class.Misc.Mount.ReqLevel.Max = 0
AuctionHouseBot.Class.Misc.Mount.ReqSkill.Min = 0
AuctionHouseBot.Class.Misc.Mount.ReqSkill.Max = 0
AuctionHouseBot.Class.Glyph.ReqLevel.Min = 0
AuctionHouseBot.Class.Glyph.ReqLevel.Max = 0
AuctionHouseBot.Class.Glyph.ItemLevel.Min = 0
AuctionHouseBot.Class.Glyph.ItemLevel.Max = 0
AuctionHouseBot.Class.TradeGood.ItemLevel.Min = 0
AuctionHouseBot.Class.TradeGood.ItemLevel.Max = 0
AuctionHouseBot.Class.Container.ItemLevel.Min = 0
AuctionHouseBot.Class.Container.ItemLevel.Max = 0

AuctionHouseBot.forceIncludeItems = ""
AuctionHouseBot.forceExcludeItems = ""

###################################################################################################################
# Buyer config
#
#    AuctionHouseBot.Buyer.Enabled
#        General enable or disable AuctionHouseBot Buyer fonctionality
#    Default 0 (Disabled)
#
#    AuctionHouseBot.DEBUG.Buyer
#        Enable or disable AuctionHouseBot Buyer debug mode
#    Default 0 (Disabled)
#
#    AuctionHouseBot.Buyer.FACTION.Enabled
#        Enable or disable buyer independently by faction
#
#    AuctionHouseBot.Buyer.BuyPrice
#        Should the Buyer use BuyPrice or SellPrice to determine Bid Prices
#    Default 0 (use SellPrice)
#
#    AuctionHouseBot.Buyer.Recheck.Interval
#        This specify time interval (in minute) between two evaluation of the same selled item.
#        The less this value is, the more you give chance for item to be buyed by ahbot.
#    Default 20 (20min.)
#
#    AuctionHouseBot.Buyer.Alliance.Chance.Ratio
#       When the evaluation of the entry is done you will have "x" chance for this entry to be buyed.
#       The chance ratio is simply (x/chance ratio)
#       For ex : If the evaluation give you 5000(maximum chance) chance and ratio is set 3
#       you will have 5000 chance on 15000(3*5000) random number
#       This for every faction independently
#    Default 3 (literaly 1 chance by 3)
#
###################################################################################################################

AuctionHouseBot.Buyer.Enabled = 0
AuctionHouseBot.DEBUG.Buyer = 0

AuctionHouseBot.Buyer.Alliance.Enabled = 1
AuctionHouseBot.Buyer.Horde.Enabled = 1
AuctionHouseBot.Buyer.Neutral.Enabled = 1

AuctionHouseBot.Buyer.BuyPrice = 0

AuctionHouseBot.Buyer.Recheck.Interval = 20

AuctionHouseBot.Buyer.Alliance.Chance.Ratio = 3
AuctionHouseBot.Buyer.Horde.Chance.Ratio = 3
AuctionHouseBot.Buyer.Neutral.Chance.Ratio = 3
