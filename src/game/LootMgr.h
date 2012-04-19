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

#ifndef MANGOS_LOOTMGR_H
#define MANGOS_LOOTMGR_H

#include "ItemEnchantmentMgr.h"
#include "ByteBuffer.h"
#include "ObjectGuid.h"
#include "Utilities/LinkedReference/RefManager.h"

#include <map>
#include <vector>

#define MAX_NR_LOOT_ITEMS 16
// note: the client cannot show more than 16 items total
#define MAX_NR_QUEST_ITEMS 32
// unrelated to the number of quest items shown, just for reserve

enum PermissionTypes
{
    ALL_PERMISSION    = 0,
    GROUP_PERMISSION  = 1,
    MASTER_PERMISSION = 2,
    OWNER_PERMISSION  = 3,                                  // for single player only loots
    NONE_PERMISSION   = 4
};

enum LootType
{
    LOOT_CORPSE                 = 1,
    LOOT_PICKPOCKETING          = 2,
    LOOT_FISHING                = 3,
    LOOT_DISENCHANTING          = 4,
                                                            // ignored always by client
    LOOT_SKINNING               = 6,
    LOOT_PROSPECTING            = 7,
    LOOT_MILLING                = 8,

    LOOT_FISHINGHOLE            = 20,                       // unsupported by client, sending LOOT_FISHING instead
    LOOT_FISHING_FAIL           = 21,                       // unsupported by client, sending LOOT_FISHING instead
    LOOT_INSIGNIA               = 22                        // unsupported by client, sending LOOT_CORPSE instead
};

enum LootSlotType
{
    LOOT_SLOT_NORMAL  = 0,                                  // can be looted
    LOOT_SLOT_VIEW    = 1,                                  // can be only view (ignore any loot attempts)
    LOOT_SLOT_MASTER  = 2,                                  // can be looted only master (error message)
    LOOT_SLOT_REQS    = 3,                                  // can't be looted (error message about missing reqs)
    LOOT_SLOT_OWNER   = 4,                                  // ignore binding confirmation and etc, for single player looting
    MAX_LOOT_SLOT_TYPE                                      // custom, use for mark skipped from show items
};



class Player;
class LootStore;

struct LootStoreItem
{
    uint32  itemid;                                         // id of the item
    float   chance;                                         // always positive, chance to drop for both quest and non-quest items, chance to be used for refs
    int32   mincountOrRef;                                  // mincount for drop items (positive) or minus referenced TemplateleId (negative)
    uint8   group       :7;
    bool    needs_quest :1;                                 // quest drop (negative ChanceOrQuestChance in DB)
    uint8   maxcount    :8;                                 // max drop count for the item (mincountOrRef positive) or Ref multiplicator (mincountOrRef negative)
    uint16  conditionId :16;                                // additional loot condition Id

    // Constructor, converting ChanceOrQuestChance -> (chance, needs_quest)
    // displayid is filled in IsValid() which must be called after
    LootStoreItem(uint32 _itemid, float _chanceOrQuestChance, int8 _group, uint16 _conditionId, int32 _mincountOrRef, uint8 _maxcount)
        : itemid(_itemid), chance(fabs(_chanceOrQuestChance)), mincountOrRef(_mincountOrRef),
        group(_group), needs_quest(_chanceOrQuestChance < 0), maxcount(_maxcount), conditionId(_conditionId)
         {}

    bool Roll(bool rate) const;                             // Checks if the entry takes it's chance (at loot generation)
    bool IsValid(LootStore const& store, uint32 entry) const;
                                                            // Checks correctness of values
};

struct LootItem
{
    uint32  itemid;
    uint32  randomSuffix;
    int32   randomPropertyId;
    uint16  conditionId       :16;                          // allow compiler pack structure
    uint8   count             : 8;
    bool    is_looted         : 1;
    bool    is_blocked        : 1;
    bool    freeforall        : 1;                          // free for all
    bool    is_underthreshold : 1;
    bool    is_counted        : 1;
    bool    needs_quest       : 1;                          // quest drop

    // Constructor, copies most fields from LootStoreItem, generates random count and random suffixes/properties
    // Should be called for non-reference LootStoreItem entries only (mincountOrRef > 0)
    explicit LootItem(LootStoreItem const& li);

    LootItem(uint32 itemid_, uint32 count_, uint32 randomSuffix_ = 0, int32 randomPropertyId_ = 0);

    // Basic checks for player/item compatibility - if false no chance to see the item in the loot
    bool AllowedForPlayer(Player const * player) const;
    LootSlotType GetSlotTypeForSharedLoot(PermissionTypes permission, Player* viewer, bool condition_ok = false) const;
};

typedef std::vector<LootItem> LootItemList;

struct QuestItem
{
    uint8   index;                                          // position in quest_items;
    bool    is_looted;

    QuestItem()
        : index(0), is_looted(false) {}

    QuestItem(uint8 _index, bool _islooted = false)
        : index(_index), is_looted(_islooted) {}
};

struct Loot;
class LootTemplate;

typedef std::vector<QuestItem> QuestItemList;
typedef std::map<uint32, QuestItemList *> QuestItemMap;
typedef std::vector<LootStoreItem> LootStoreItemList;
typedef UNORDERED_MAP<uint32, LootTemplate*> LootTemplateMap;

typedef std::set<uint32> LootIdSet;

class LootStore
{
    public:
        explicit LootStore(char const* name, char const* entryName, bool ratesAllowed)
            : m_name(name), m_entryName(entryName), m_ratesAllowed(ratesAllowed) {}
        virtual ~LootStore() { Clear(); }

        void Verify() const;

        void LoadAndCollectLootIds(LootIdSet& ids_set);
        void CheckLootRefs(LootIdSet* ref_set = NULL) const;// check existence reference and remove it from ref_set
        void ReportUnusedIds(LootIdSet const& ids_set) const;
        void ReportNotExistedId(uint32 id) const;

        bool HaveLootFor(uint32 loot_id) const { return m_LootTemplates.find(loot_id) != m_LootTemplates.end(); }
        bool HaveQuestLootFor(uint32 loot_id) const;
        bool HaveQuestLootForPlayer(uint32 loot_id,Player* player) const;

        LootTemplate const* GetLootFor(uint32 loot_id) const;

        char const* GetName() const { return m_name; }
        char const* GetEntryName() const { return m_entryName; }
        bool IsRatesAllowed() const { return m_ratesAllowed; }
    protected:
        void LoadLootTable();
        void Clear();
    private:
        LootTemplateMap m_LootTemplates;
        char const* m_name;
        char const* m_entryName;
        bool m_ratesAllowed;
};

class LootTemplate
{
    class  LootGroup;                                       // A set of loot definitions for items (refs are not allowed inside)
    typedef std::vector<LootGroup> LootGroups;

    public:
        // Adds an entry to the group (at loading stage)
        void AddEntry(LootStoreItem& item);
        // Rolls for every item in the template and adds the rolled items the the loot
        void Process(Loot& loot, LootStore const& store, bool rate, uint8 GroupId = 0) const;

        // True if template includes at least 1 quest drop entry
        bool HasQuestDrop(LootTemplateMap const& store, uint8 GroupId = 0) const;
        // True if template includes at least 1 quest drop for an active quest of the player
        bool HasQuestDropForPlayer(LootTemplateMap const& store, Player const * player, uint8 GroupId = 0) const;

        // Checks integrity of the template
        void Verify(LootStore const& store, uint32 Id) const;
        void CheckLootRefs(LootIdSet* ref_set) const;
    private:
        LootStoreItemList Entries;                          // not grouped only
        LootGroups        Groups;                           // groups have own (optimised) processing, grouped entries go there
};

//=====================================================

class LootValidatorRef :  public Reference<Loot, LootValidatorRef>
{
    public:
        LootValidatorRef() {}
        void targetObjectDestroyLink() {}
        void sourceObjectDestroyLink() {}
};

//=====================================================

class LootValidatorRefManager : public RefManager<Loot, LootValidatorRef>
{
    public:
        typedef LinkedListHead::Iterator< LootValidatorRef > iterator;

        LootValidatorRef* getFirst() { return (LootValidatorRef*)RefManager<Loot, LootValidatorRef>::getFirst(); }
        LootValidatorRef* getLast() { return (LootValidatorRef*)RefManager<Loot, LootValidatorRef>::getLast(); }

        iterator begin() { return iterator(getFirst()); }
        iterator end() { return iterator(NULL); }
        iterator rbegin() { return iterator(getLast()); }
        iterator rend() { return iterator(NULL); }
};

//=====================================================
struct LootView;

ByteBuffer& operator<<(ByteBuffer& b, LootItem const& li);
ByteBuffer& operator<<(ByteBuffer& b, LootView const& lv);

struct Loot
{
    friend ByteBuffer& operator<<(ByteBuffer& b, LootView const& lv);

    QuestItemMap const& GetPlayerQuestItems() const { return m_playerQuestItems; }
    QuestItemMap const& GetPlayerFFAItems() const { return m_playerFFAItems; }
    QuestItemMap const& GetPlayerNonQuestNonFFAConditionalItems() const { return m_playerNonQuestNonFFAConditionalItems; }

    LootItemList items;
    uint32 gold;
    uint8 unlootedCount;
    LootType loot_type;                                     // required for achievement system

    Loot(uint32 _gold = 0) : gold(_gold), unlootedCount(0), loot_type(LOOT_CORPSE) {}
    ~Loot() { clear(); }

    // if loot becomes invalid this reference is used to inform the listener
    void addLootValidatorRef(LootValidatorRef* pLootValidatorRef)
    {
        m_LootValidatorRefManager.insertFirst(pLootValidatorRef);
    }

    // void clear();
    void clear()
    {
        for (QuestItemMap::const_iterator itr = m_playerQuestItems.begin(); itr != m_playerQuestItems.end(); ++itr)
            delete itr->second;
        m_playerQuestItems.clear();

        for (QuestItemMap::const_iterator itr = m_playerFFAItems.begin(); itr != m_playerFFAItems.end(); ++itr)
            delete itr->second;
        m_playerFFAItems.clear();

        for (QuestItemMap::const_iterator itr = m_playerNonQuestNonFFAConditionalItems.begin(); itr != m_playerNonQuestNonFFAConditionalItems.end(); ++itr)
            delete itr->second;
        m_playerNonQuestNonFFAConditionalItems.clear();

        m_playersLooting.clear();
        items.clear();
        m_questItems.clear();
        gold = 0;
        unlootedCount = 0;
        m_LootValidatorRefManager.clearReferences();
    }

    bool empty() const { return items.empty() && gold == 0; }
    bool isLooted() const { return gold == 0 && unlootedCount == 0; }

    void NotifyItemRemoved(uint8 lootIndex);
    void NotifyQuestItemRemoved(uint8 questIndex);
    void NotifyMoneyRemoved();
    void AddLooter(ObjectGuid guid) { m_playersLooting.insert(guid); }
    void RemoveLooter(ObjectGuid guid) { m_playersLooting.erase(guid); }

    void generateMoneyLoot(uint32 minAmount, uint32 maxAmount);
    bool FillLoot(uint32 loot_id, LootStore const& store, Player* loot_owner, bool personal, bool noEmptyError = false);

    // Inserts the item into the loot (called by LootTemplate processors)
    void AddItem(LootStoreItem const & item);

    LootItem* LootItemInSlot(uint32 lootslot, Player* player, QuestItem** qitem = NULL, QuestItem** ffaitem = NULL, QuestItem** conditem = NULL);
    uint32 GetMaxSlotInLootFor(Player* player) const;

    private:
        void FillNotNormalLootFor(Player* player);
        QuestItemList* FillFFALoot(Player* player);
        QuestItemList* FillQuestLoot(Player* player);
        QuestItemList* FillNonQuestNonFFAConditionalLoot(Player* player);

        LootItemList m_questItems;

        typedef std::set<ObjectGuid> PlayersLooting;
        PlayersLooting m_playersLooting;

        QuestItemMap m_playerQuestItems;
        QuestItemMap m_playerFFAItems;
        QuestItemMap m_playerNonQuestNonFFAConditionalItems;

        // All rolls are registered here. They need to know, when the loot is not valid anymore
        LootValidatorRefManager m_LootValidatorRefManager;
};

struct LootView
{
    Loot &loot;
    Player *viewer;
    PermissionTypes permission;
    LootView(Loot &_loot, Player *_viewer,PermissionTypes _permission = ALL_PERMISSION)
        : loot(_loot), viewer(_viewer), permission(_permission) {}
};

extern LootStore LootTemplates_Creature;
extern LootStore LootTemplates_Fishing;
extern LootStore LootTemplates_Gameobject;
extern LootStore LootTemplates_Item;
extern LootStore LootTemplates_Mail;
extern LootStore LootTemplates_Milling;
extern LootStore LootTemplates_Pickpocketing;
extern LootStore LootTemplates_Skinning;
extern LootStore LootTemplates_Disenchant;
extern LootStore LootTemplates_Prospecting;
extern LootStore LootTemplates_Spell;

void LoadLootTemplates_Creature();
void LoadLootTemplates_Fishing();
void LoadLootTemplates_Gameobject();
void LoadLootTemplates_Item();
void LoadLootTemplates_Mail();
void LoadLootTemplates_Milling();
void LoadLootTemplates_Pickpocketing();
void LoadLootTemplates_Skinning();
void LoadLootTemplates_Disenchant();
void LoadLootTemplates_Prospecting();

void LoadLootTemplates_Spell();
void LoadLootTemplates_Reference();

inline void LoadLootTables()
{
    LoadLootTemplates_Creature();
    LoadLootTemplates_Fishing();
    LoadLootTemplates_Gameobject();
    LoadLootTemplates_Item();
    LoadLootTemplates_Mail();
    LoadLootTemplates_Milling();
    LoadLootTemplates_Pickpocketing();
    LoadLootTemplates_Skinning();
    LoadLootTemplates_Disenchant();
    LoadLootTemplates_Prospecting();
    LoadLootTemplates_Spell();

    LoadLootTemplates_Reference();
}

#endif
