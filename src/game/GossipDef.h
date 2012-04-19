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

#ifndef MANGOSSERVER_GOSSIP_H
#define MANGOSSERVER_GOSSIP_H

#include "Common.h"
#include "QuestDef.h"
#include "NPCHandler.h"
#include "ObjectGuid.h"

class WorldSession;

#define GOSSIP_MAX_MENU_ITEMS 64                            // client supported items unknown, but provided number must be enough
#define DEFAULT_GOSSIP_MESSAGE              0xffffff

enum Gossip_Option
{
    GOSSIP_OPTION_NONE              = 0,                    // UNIT_NPC_FLAG_NONE               (0)
    GOSSIP_OPTION_GOSSIP            = 1,                    // UNIT_NPC_FLAG_GOSSIP             (1)
    GOSSIP_OPTION_QUESTGIVER        = 2,                    // UNIT_NPC_FLAG_QUESTGIVER         (2)
    GOSSIP_OPTION_VENDOR            = 3,                    // UNIT_NPC_FLAG_VENDOR             (128)
    GOSSIP_OPTION_TAXIVENDOR        = 4,                    // UNIT_NPC_FLAG_TAXIVENDOR         (8192)
    GOSSIP_OPTION_TRAINER           = 5,                    // UNIT_NPC_FLAG_TRAINER            (16)
    GOSSIP_OPTION_SPIRITHEALER      = 6,                    // UNIT_NPC_FLAG_SPIRITHEALER       (16384)
    GOSSIP_OPTION_SPIRITGUIDE       = 7,                    // UNIT_NPC_FLAG_SPIRITGUIDE        (32768)
    GOSSIP_OPTION_INNKEEPER         = 8,                    // UNIT_NPC_FLAG_INNKEEPER          (65536)
    GOSSIP_OPTION_BANKER            = 9,                    // UNIT_NPC_FLAG_BANKER             (131072)
    GOSSIP_OPTION_PETITIONER        = 10,                   // UNIT_NPC_FLAG_PETITIONER         (262144)
    GOSSIP_OPTION_TABARDDESIGNER    = 11,                   // UNIT_NPC_FLAG_TABARDDESIGNER     (524288)
    GOSSIP_OPTION_BATTLEFIELD       = 12,                   // UNIT_NPC_FLAG_BATTLEFIELDPERSON  (1048576)
    GOSSIP_OPTION_AUCTIONEER        = 13,                   // UNIT_NPC_FLAG_AUCTIONEER         (2097152)
    GOSSIP_OPTION_STABLEPET         = 14,                   // UNIT_NPC_FLAG_STABLE             (4194304)
    GOSSIP_OPTION_ARMORER           = 15,                   // UNIT_NPC_FLAG_ARMORER            (4096)
    GOSSIP_OPTION_UNLEARNTALENTS    = 16,                   // UNIT_NPC_FLAG_TRAINER            (16) (bonus option for GOSSIP_OPTION_TRAINER)
    GOSSIP_OPTION_UNLEARNPETSKILLS  = 17,                   // UNIT_NPC_FLAG_TRAINER            (16) (bonus option for GOSSIP_OPTION_TRAINER)
    GOSSIP_OPTION_MAILBOX           = 18,                   // UNIT_NPC_FLAG_GOSSIP             (1)
    GOSSIP_OPTION_MAX
};

enum GossipOptionIcon
{
    GOSSIP_ICON_CHAT                = 0,                    //white chat bubble
    GOSSIP_ICON_VENDOR              = 1,                    //brown bag
    GOSSIP_ICON_TAXI                = 2,                    //flight
    GOSSIP_ICON_TRAINER             = 3,                    //book
    GOSSIP_ICON_INTERACT_1          = 4,                    //interaction wheel
    GOSSIP_ICON_INTERACT_2          = 5,                    //interaction wheel
    GOSSIP_ICON_MONEY_BAG           = 6,                    //brown bag with yellow dot
    GOSSIP_ICON_TALK                = 7,                    //white chat bubble with black dots
    GOSSIP_ICON_TABARD              = 8,                    //tabard
    GOSSIP_ICON_BATTLE              = 9,                    //two swords
    GOSSIP_ICON_DOT                 = 10,                   //yellow dot
    GOSSIP_ICON_CHAT_11             = 11,                   //This and below are most the same visual as GOSSIP_ICON_CHAT
    GOSSIP_ICON_CHAT_12             = 12,                   //but are still used for unknown reasons.
    GOSSIP_ICON_CHAT_13             = 13,
    GOSSIP_ICON_CHAT_14             = 14,                   // probably invalid
    GOSSIP_ICON_CHAT_15             = 15,                   // probably invalid
    GOSSIP_ICON_CHAT_16             = 16,
    GOSSIP_ICON_CHAT_17             = 17,
    GOSSIP_ICON_CHAT_18             = 18,
    GOSSIP_ICON_CHAT_19             = 19,
    GOSSIP_ICON_CHAT_20             = 20,
    GOSSIP_ICON_MAX
};

//POI icons. Many more exist, list not complete.
enum Poi_Icon
{
    ICON_POI_BLANK              =   0,                      // Blank (not visible), in 2.4.3 have value 15 with 1..15 values in 0..14 range
    ICON_POI_GREY_AV_MINE       =   1,                      // Grey mine lorry
    ICON_POI_RED_AV_MINE        =   2,                      // Red mine lorry
    ICON_POI_BLUE_AV_MINE       =   3,                      // Blue mine lorry
    ICON_POI_BWTOMB             =   4,                      // Blue and White Tomb Stone
    ICON_POI_SMALL_HOUSE        =   5,                      // Small house
    ICON_POI_GREYTOWER          =   6,                      // Grey Tower
    ICON_POI_REDFLAG            =   7,                      // Red Flag w/Yellow !
    ICON_POI_TOMBSTONE          =   8,                      // Normal tomb stone (brown)
    ICON_POI_BWTOWER            =   9,                      // Blue and White Tower
    ICON_POI_REDTOWER           =   10,                     // Red Tower
    ICON_POI_BLUETOWER          =   11,                     // Blue Tower
    ICON_POI_RWTOWER            =   12,                     // Red and White Tower
    ICON_POI_REDTOMB            =   13,                     // Red Tomb Stone
    ICON_POI_RWTOMB             =   14,                     // Red and White Tomb Stone
    ICON_POI_BLUETOMB           =   15,                     // Blue Tomb Stone
    ICON_POI_16                 =   16,                     // Grey ?
    ICON_POI_17                 =   17,                     // Blue/White ?
    ICON_POI_18                 =   18,                     // Blue ?
    ICON_POI_19                 =   19,                     // Red and White ?
    ICON_POI_20                 =   20,                     // Red ?
    ICON_POI_GREYLOGS           =   21,                     // Grey Wood Logs
    ICON_POI_BWLOGS             =   22,                     // Blue and White Wood Logs
    ICON_POI_BLUELOGS           =   23,                     // Blue Wood Logs
    ICON_POI_RWLOGS             =   24,                     // Red and White Wood Logs
    ICON_POI_REDLOGS            =   25,                     // Red Wood Logs
    ICON_POI_26                 =   26,                     // Grey ?
    ICON_POI_27                 =   27,                     // Blue and White ?
    ICON_POI_28                 =   28,                     // Blue ?
    ICON_POI_29                 =   29,                     // Red and White ?
    ICON_POI_30                 =   30,                     // Red ?
    ICON_POI_GREYHOUSE          =   31,                     // Grey House
    ICON_POI_BWHOUSE            =   32,                     // Blue and White House
    ICON_POI_BLUEHOUSE          =   33,                     // Blue House
    ICON_POI_RWHOUSE            =   34,                     // Red and White House
    ICON_POI_REDHOUSE           =   35,                     // Red House
    ICON_POI_GREYHORSE          =   36,                     // Grey Horse
    ICON_POI_BWHORSE            =   37,                     // Blue and White Horse
    ICON_POI_BLUEHORSE          =   38,                     // Blue Horse
    ICON_POI_RWHORSE            =   39,                     // Red and White Horse
    ICON_POI_REDHORSE           =   40                      // Red Horse
};

struct GossipMenuItem
{
    uint8       m_gIcon;
    bool        m_gCoded;
    std::string m_gMessage;
    uint32      m_gSender;
    uint32      m_gOptionId;
    std::string m_gBoxMessage;
    uint32      m_gBoxMoney;
};

typedef std::vector<GossipMenuItem> GossipMenuItemList;

struct GossipMenuItemData
{
    int32  m_gAction_menu;                                  // negative for close gossip
    uint32 m_gAction_poi;
    uint32 m_gAction_script;
};

typedef std::vector<GossipMenuItemData> GossipMenuItemDataList;

struct QuestMenuItem
{
    uint32      m_qId;
    uint8       m_qIcon;
};

typedef std::vector<QuestMenuItem> QuestMenuItemList;

class MANGOS_DLL_SPEC GossipMenu
{
    public:
        explicit GossipMenu(WorldSession* session);
        ~GossipMenu();

        void AddMenuItem(uint8 Icon, const std::string& Message, bool Coded = false);
        void AddMenuItem(uint8 Icon, const std::string& Message, uint32 dtSender, uint32 dtAction, const std::string& BoxMessage, uint32 BoxMoney, bool Coded = false);

        // for using from scripts, don't must be inlined
        void AddMenuItem(uint8 Icon, char const* Message, bool Coded = false);
        void AddMenuItem(uint8 Icon, char const* Message, uint32 dtSender, uint32 dtAction, char const* BoxMessage, uint32 BoxMoney, bool Coded = false);

        void AddMenuItem(uint8 Icon, int32 itemText, uint32 dtSender, uint32 dtAction, int32 boxText, uint32 BoxMoney, bool Coded = false);

        void SetMenuId(uint32 menu_id) { m_gMenuId = menu_id; }
        uint32 GetMenuId() { return m_gMenuId; }

        void AddGossipMenuItemData(int32 action_menu, uint32 action_poi, uint32 action_script);

        unsigned int MenuItemCount() const
        {
            return m_gItems.size();
        }

        bool Empty() const
        {
            return m_gItems.empty();
        }

        GossipMenuItem const& GetItem( unsigned int Id )
        {
            return m_gItems[ Id ];
        }

        GossipMenuItemData const& GetItemData(unsigned int indexId)
        {
            return m_gItemsData[indexId];
        }

        uint32 MenuItemSender( unsigned int ItemId );
        uint32 MenuItemAction( unsigned int ItemId );
        bool MenuItemCoded( unsigned int ItemId );

        void ClearMenu();

        WorldSession* GetMenuSession() const { return m_session; }

    protected:
        GossipMenuItemList      m_gItems;
        GossipMenuItemDataList  m_gItemsData;

        uint32 m_gMenuId;

    private:
        WorldSession* m_session;
};

class QuestMenu
{
    public:
        QuestMenu();
        ~QuestMenu();

        void AddMenuItem( uint32 QuestId, uint8 Icon);
        void ClearMenu();

        uint8 MenuItemCount() const
        {
            return m_qItems.size();
        }

        bool Empty() const
        {
            return m_qItems.empty();
        }

        bool HasItem( uint32 questid );

        QuestMenuItem const& GetItem( uint16 Id )
        {
            return m_qItems[ Id ];
        }

    protected:
        QuestMenuItemList m_qItems;
};

class MANGOS_DLL_SPEC PlayerMenu
{
    private:
        GossipMenu mGossipMenu;
        QuestMenu  mQuestMenu;

    public:
        explicit PlayerMenu(WorldSession *Session);
        ~PlayerMenu();

        GossipMenu& GetGossipMenu() { return mGossipMenu; }
        QuestMenu& GetQuestMenu() { return mQuestMenu; }

        WorldSession* GetMenuSession() const { return mGossipMenu.GetMenuSession(); }

        bool Empty() const { return mGossipMenu.Empty() && mQuestMenu.Empty(); }

        void ClearMenus();
        uint32 GossipOptionSender( unsigned int Selection );
        uint32 GossipOptionAction( unsigned int Selection );
        bool GossipOptionCoded( unsigned int Selection );

        void SendGossipMenu(uint32 titleTextId, ObjectGuid objectGuid);
        void CloseGossip();
        void SendPointOfInterest( float X, float Y, uint32 Icon, uint32 Flags, uint32 Data, const char * locName );
        void SendPointOfInterest( uint32 poi_id );
        void SendTalking( uint32 textID );
        void SendTalking( char const * title, char const * text );

        /*********************************************************/
        /***                    QUEST SYSTEM                   ***/
        /*********************************************************/
        void SendQuestGiverStatus(uint8 questStatus, ObjectGuid npcGUID);

        void SendQuestGiverQuestList(QEmote eEmote, const std::string& Title, ObjectGuid npcGUID);

        void SendQuestQueryResponse(Quest const *pQuest);
        void SendQuestGiverQuestDetails(Quest const *pQuest, ObjectGuid npcGUID, bool ActivateAccept);

        void SendQuestGiverOfferReward(Quest const* pQuest, ObjectGuid npcGUID, bool EnbleNext);
        void SendQuestGiverRequestItems(Quest const *pQuest, ObjectGuid npcGUID, bool Completable, bool CloseOnCancel);
};
#endif
