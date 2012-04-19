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

#include "LootMgr.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "ProgressBar.h"
#include "World.h"
#include "Util.h"
#include "SharedDefines.h"
#include "SpellMgr.h"

static eConfigFloatValues const qualityToRate[MAX_ITEM_QUALITY] = {
    CONFIG_FLOAT_RATE_DROP_ITEM_POOR,                                    // ITEM_QUALITY_POOR
    CONFIG_FLOAT_RATE_DROP_ITEM_NORMAL,                                  // ITEM_QUALITY_NORMAL
    CONFIG_FLOAT_RATE_DROP_ITEM_UNCOMMON,                                // ITEM_QUALITY_UNCOMMON
    CONFIG_FLOAT_RATE_DROP_ITEM_RARE,                                    // ITEM_QUALITY_RARE
    CONFIG_FLOAT_RATE_DROP_ITEM_EPIC,                                    // ITEM_QUALITY_EPIC
    CONFIG_FLOAT_RATE_DROP_ITEM_LEGENDARY,                               // ITEM_QUALITY_LEGENDARY
    CONFIG_FLOAT_RATE_DROP_ITEM_ARTIFACT,                                // ITEM_QUALITY_ARTIFACT
};

LootStore LootTemplates_Creature(     "creature_loot_template",     "creature entry",                 true);
LootStore LootTemplates_Disenchant(   "disenchant_loot_template",   "item disenchant id",             true);
LootStore LootTemplates_Fishing(      "fishing_loot_template",      "area id",                        true);
LootStore LootTemplates_Gameobject(   "gameobject_loot_template",   "gameobject lootid",              true);
LootStore LootTemplates_Item(         "item_loot_template",         "item entry with ITEM_FLAG_LOOTABLE", true);
LootStore LootTemplates_Mail(         "mail_loot_template",         "mail template id",               false);
LootStore LootTemplates_Milling(      "milling_loot_template",      "item entry (herb)",              true);
LootStore LootTemplates_Pickpocketing("pickpocketing_loot_template","creature pickpocket lootid",     true);
LootStore LootTemplates_Prospecting(  "prospecting_loot_template",  "item entry (ore)",               true);
LootStore LootTemplates_Reference(    "reference_loot_template",    "reference id",                   false);
LootStore LootTemplates_Skinning(     "skinning_loot_template",     "creature skinning id",           true);
LootStore LootTemplates_Spell(        "spell_loot_template",        "spell id (random item creating)",false);

class LootTemplate::LootGroup                               // A set of loot definitions for items (refs are not allowed)
{
    public:
        void AddEntry(LootStoreItem& item);                 // Adds an entry to the group (at loading stage)
        bool HasQuestDrop() const;                          // True if group includes at least 1 quest drop entry
        bool HasQuestDropForPlayer(Player const * player) const;
                                                            // The same for active quests of the player
        void Process(Loot& loot) const;                     // Rolls an item from the group (if any) and adds the item to the loot
        float RawTotalChance() const;                       // Overall chance for the group (without equal chanced items)
        float TotalChance() const;                          // Overall chance for the group

        void Verify(LootStore const& lootstore, uint32 id, uint32 group_id) const;
        void CollectLootIds(LootIdSet& set) const;
        void CheckLootRefs(LootIdSet* ref_set) const;
    private:
        LootStoreItemList ExplicitlyChanced;                // Entries with chances defined in DB
        LootStoreItemList EqualChanced;                     // Zero chances - every entry takes the same chance

        LootStoreItem const * Roll() const;                 // Rolls an item from the group, returns NULL if all miss their chances
};

//Remove all data and free all memory
void LootStore::Clear()
{
    for (LootTemplateMap::const_iterator itr=m_LootTemplates.begin(); itr != m_LootTemplates.end(); ++itr)
        delete itr->second;
    m_LootTemplates.clear();
}

// Checks validity of the loot store
// Actual checks are done within LootTemplate::Verify() which is called for every template
void LootStore::Verify() const
{
    for (LootTemplateMap::const_iterator i = m_LootTemplates.begin(); i != m_LootTemplates.end(); ++i )
        i->second->Verify(*this, i->first);
}

// Loads a *_loot_template DB table into loot store
// All checks of the loaded template are called from here, no error reports at loot generation required
void LootStore::LoadLootTable()
{
    LootTemplateMap::const_iterator tab;
    uint32 count = 0;

    // Clearing store (for reloading case)
    Clear();

    sLog.outString( "%s :", GetName());

    //                                                 0      1     2                    3        4              5         6
    QueryResult *result = WorldDatabase.PQuery("SELECT entry, item, ChanceOrQuestChance, groupid, mincountOrRef, maxcount, condition_id FROM %s",GetName());

    if (result)
    {
        BarGoLink bar(result->GetRowCount());

        do
        {
            Field *fields = result->Fetch();
            bar.step();

            uint32 entry               = fields[0].GetUInt32();
            uint32 item                = fields[1].GetUInt32();
            float  chanceOrQuestChance = fields[2].GetFloat();
            uint8  group               = fields[3].GetUInt8();
            int32  mincountOrRef       = fields[4].GetInt32();
            uint32 maxcount            = fields[5].GetUInt32();
            uint16 conditionId         = fields[6].GetUInt16();

            if (maxcount > std::numeric_limits<uint8>::max())
            {
                sLog.outErrorDb("Table '%s' entry %d item %d: maxcount value (%u) to large. must be less %u - skipped", GetName(), entry, item, maxcount,std::numeric_limits<uint8>::max());
                continue;                                   // error already printed to log/console.
            }

            if (mincountOrRef < 0 && conditionId)
            {
                sLog.outErrorDb("Table '%s' entry %u mincountOrRef %i < 0 and not allowed has condition, skipped", GetName(), entry, mincountOrRef);
                continue;
            }

            if (conditionId)
            {
                const PlayerCondition* condition = sConditionStorage.LookupEntry<PlayerCondition>(conditionId);
                if (!condition)
                {
                    sLog.outErrorDb("Table `%s` for entry %u, item %u has condition_id %u that does not exist in `conditions`, ignoring", GetName(), entry,item, conditionId);
                    conditionId = 0;
                }
            }

            LootStoreItem storeitem = LootStoreItem(item, chanceOrQuestChance, group, conditionId, mincountOrRef, maxcount);

            if (!storeitem.IsValid(*this,entry))            // Validity checks
                continue;

            // Looking for the template of the entry
                                                            // often entries are put together
            if (m_LootTemplates.empty() || tab->first != entry)
            {
                // Searching the template (in case template Id changed)
                tab = m_LootTemplates.find(entry);
                if ( tab == m_LootTemplates.end() )
                {
                    std::pair< LootTemplateMap::iterator, bool > pr = m_LootTemplates.insert(LootTemplateMap::value_type(entry, new LootTemplate));
                    tab = pr.first;
                }
            }
            // else is empty - template Id and iter are the same
            // finally iter refers to already existing or just created <entry, LootTemplate>

            // Adds current row to the template
            tab->second->AddEntry(storeitem);
            ++count;

        } while (result->NextRow());

        delete result;

        Verify();                                           // Checks validity of the loot store

        sLog.outString();
        sLog.outString( ">> Loaded %u loot definitions (%lu templates)", count, (unsigned long)m_LootTemplates.size());
    }
    else
    {
        sLog.outString();
        sLog.outErrorDb( ">> Loaded 0 loot definitions. DB table `%s` is empty.",GetName() );
    }
}

bool LootStore::HaveQuestLootFor(uint32 loot_id) const
{
    LootTemplateMap::const_iterator itr = m_LootTemplates.find(loot_id);
    if(itr == m_LootTemplates.end())
        return false;

    // scan loot for quest items
    return itr->second->HasQuestDrop(m_LootTemplates);
}

bool LootStore::HaveQuestLootForPlayer(uint32 loot_id,Player* player) const
{
    LootTemplateMap::const_iterator tab = m_LootTemplates.find(loot_id);
    if (tab != m_LootTemplates.end())
        if (tab->second->HasQuestDropForPlayer(m_LootTemplates, player))
            return true;

    return false;
}

LootTemplate const* LootStore::GetLootFor(uint32 loot_id) const
{
    LootTemplateMap::const_iterator tab = m_LootTemplates.find(loot_id);

    if (tab == m_LootTemplates.end())
        return NULL;

    return tab->second;
}

void LootStore::LoadAndCollectLootIds(LootIdSet& ids_set)
{
    LoadLootTable();

    for(LootTemplateMap::const_iterator tab = m_LootTemplates.begin(); tab != m_LootTemplates.end(); ++tab)
        ids_set.insert(tab->first);
}

void LootStore::CheckLootRefs(LootIdSet* ref_set) const
{
    for(LootTemplateMap::const_iterator ltItr = m_LootTemplates.begin(); ltItr != m_LootTemplates.end(); ++ltItr)
        ltItr->second->CheckLootRefs(ref_set);
}

void LootStore::ReportUnusedIds(LootIdSet const& ids_set) const
{
    // all still listed ids isn't referenced
    for(LootIdSet::const_iterator itr = ids_set.begin(); itr != ids_set.end(); ++itr)
        sLog.outErrorDb("Table '%s' entry %d isn't %s and not referenced from loot, and then useless.", GetName(), *itr,GetEntryName());
}

void LootStore::ReportNotExistedId(uint32 id) const
{
    sLog.outErrorDb("Table '%s' entry %d (%s) not exist but used as loot id in DB.", GetName(), id,GetEntryName());
}

//
// --------- LootStoreItem ---------
//

// Checks if the entry (quest, non-quest, reference) takes it's chance (at loot generation)
// RATE_DROP_ITEMS is no longer used for all types of entries
bool LootStoreItem::Roll(bool rate) const
{
    if(chance>=100.0f)
        return true;

    if(mincountOrRef < 0)                                   // reference case
        return roll_chance_f(chance* (rate ? sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_ITEM_REFERENCED) : 1.0f));

    ItemPrototype const *pProto = ObjectMgr::GetItemPrototype(itemid);

    float qualityModifier = pProto && rate ? sWorld.getConfig(qualityToRate[pProto->Quality]) : 1.0f;

    return roll_chance_f(chance*qualityModifier);
}

// Checks correctness of values
bool LootStoreItem::IsValid(LootStore const& store, uint32 entry) const
{
    if (group >= 1 << 7)                                    // it stored in 7 bit field
    {
        sLog.outErrorDb("Table '%s' entry %d item %d: group (%u) must be less %u - skipped", store.GetName(), entry, itemid, group, 1 << 7);
        return false;
    }

    if (mincountOrRef == 0)
    {
        sLog.outErrorDb("Table '%s' entry %d item %d: wrong mincountOrRef (%d) - skipped", store.GetName(), entry, itemid, mincountOrRef);
        return false;
    }

    if (mincountOrRef > 0)                                  // item (quest or non-quest) entry, maybe grouped
    {
        ItemPrototype const *proto = ObjectMgr::GetItemPrototype(itemid);
        if(!proto)
        {
            sLog.outErrorDb("Table '%s' entry %d item %d: item entry not listed in `item_template` - skipped", store.GetName(), entry, itemid);
            return false;
        }

        if (chance == 0 && group == 0)                      // Zero chance is allowed for grouped entries only
        {
            sLog.outErrorDb("Table '%s' entry %d item %d: equal-chanced grouped entry, but group not defined - skipped", store.GetName(), entry, itemid);
            return false;
        }

        if (chance != 0 && chance < 0.000001f)              // loot with low chance
        {
            sLog.outErrorDb("Table '%s' entry %d item %d: low chance (%f) - skipped", store.GetName(), entry, itemid, chance);
            return false;
        }

        if (maxcount < mincountOrRef)                       // wrong max count
        {
            sLog.outErrorDb("Table '%s' entry %d item %d: max count (%u) less that min count (%i) - skipped", store.GetName(), entry, itemid, uint32(maxcount), mincountOrRef);
            return false;
        }

    }
    else                                                    // mincountOrRef < 0
    {
        if (needs_quest)
        {
            sLog.outErrorDb("Table '%s' entry %d item %d: negative chance is specified for a reference, skipped", store.GetName(), entry, itemid);
            return false;
        }
        else if (chance == 0)                               // no chance for the reference
        {
            sLog.outErrorDb("Table '%s' entry %d item %d: zero chance is specified for a reference, skipped", store.GetName(), entry, itemid);
            return false;
        }
    }
    return true;                                            // Referenced template existence is checked at whole store level
}

//
// --------- LootItem ---------
//

// Constructor, copies most fields from LootStoreItem and generates random count
LootItem::LootItem(LootStoreItem const& li)
{
    itemid      = li.itemid;
    conditionId = li.conditionId;

    ItemPrototype const* proto = ObjectMgr::GetItemPrototype(itemid);
    freeforall  = proto && (proto->Flags & ITEM_FLAG_PARTY_LOOT);

    needs_quest = li.needs_quest;

    count       = urand(li.mincountOrRef, li.maxcount);     // constructor called for mincountOrRef > 0 only
    randomSuffix = GenerateEnchSuffixFactor(itemid);
    randomPropertyId = Item::GenerateItemRandomPropertyId(itemid);
    is_looted = 0;
    is_blocked = 0;
    is_underthreshold = 0;
    is_counted = 0;
}

LootItem::LootItem(uint32 itemid_, uint32 count_, uint32 randomSuffix_, int32 randomPropertyId_)
{
    itemid      = itemid_;
    conditionId = 0;

    ItemPrototype const* proto = ObjectMgr::GetItemPrototype(itemid);
    freeforall  = proto && (proto->Flags & ITEM_FLAG_PARTY_LOOT);

    needs_quest = false;

    count       = count_;
    randomSuffix = randomSuffix_;
    randomPropertyId = randomPropertyId_;
    is_looted = 0;
    is_blocked = 0;
    is_underthreshold = 0;
    is_counted = 0;
}

// Basic checks for player/item compatibility - if false no chance to see the item in the loot
bool LootItem::AllowedForPlayer(Player const * player) const
{
    // DB conditions check
    if (conditionId && !sObjectMgr.IsPlayerMeetToNEWCondition(player, conditionId))
        return false;

    ItemPrototype const *pProto = ObjectMgr::GetItemPrototype(itemid);
    if (!pProto)
        return false;

    // not show loot for not own team
    if ((pProto->Flags2 & ITEM_FLAG2_HORDE_ONLY) && player->GetTeam() != HORDE)
        return false;

    if ((pProto->Flags2 & ITEM_FLAG2_ALLIANCE_ONLY) && player->GetTeam() != ALLIANCE)
        return false;

    if ( needs_quest )
    {
        // Checking quests for quest-only drop (check only quests requirements in this case)
        if( !player->HasQuestForItem(itemid) )
            return false;
    }
    else
    {
        // Not quest only drop (check quest starting items for already accepted non-repeatable quests)
        if (pProto->StartQuest && player->GetQuestStatus(pProto->StartQuest) != QUEST_STATUS_NONE && !player->HasQuestForItem(itemid))
            return false;
    }

    return true;
}

LootSlotType LootItem::GetSlotTypeForSharedLoot(PermissionTypes permission, Player* viewer, bool condition_ok /*= false*/) const
{
    // ignore looted, FFA (each player get own copy) and not allowed items
    if (is_looted || freeforall || (conditionId && !condition_ok) || !AllowedForPlayer(viewer))
        return MAX_LOOT_SLOT_TYPE;

    switch (permission)
    {
        case GROUP_PERMISSION:
            return (is_blocked || is_underthreshold) ? LOOT_SLOT_NORMAL : LOOT_SLOT_VIEW;
        case ALL_PERMISSION:
            return LOOT_SLOT_NORMAL;
        case OWNER_PERMISSION:
            return LOOT_SLOT_OWNER;
        case MASTER_PERMISSION:
            return !is_underthreshold ? LOOT_SLOT_MASTER : LOOT_SLOT_NORMAL;
    }

    return MAX_LOOT_SLOT_TYPE;
}

//
// --------- Loot ---------
//

// Inserts the item into the loot (called by LootTemplate processors)
void Loot::AddItem(LootStoreItem const & item)
{
    if (item.needs_quest)                                   // Quest drop
    {
        if (m_questItems.size() < MAX_NR_QUEST_ITEMS)
            m_questItems.push_back(LootItem(item));
    }
    else if (items.size() < MAX_NR_LOOT_ITEMS)              // Non-quest drop
    {
        items.push_back(LootItem(item));

        // non-conditional one-player only items are counted here,
        // free for all items are counted in FillFFALoot(),
        // non-ffa conditionals are counted in FillNonQuestNonFFAConditionalLoot()
        if (!item.conditionId)
        {
            ItemPrototype const* proto = ObjectMgr::GetItemPrototype(item.itemid);
            if (!proto || !(proto->Flags & ITEM_FLAG_PARTY_LOOT))
                ++unlootedCount;
        }
    }
}

// Calls processor of corresponding LootTemplate (which handles everything including references)
bool Loot::FillLoot(uint32 loot_id, LootStore const& store, Player* loot_owner, bool personal, bool noEmptyError)
{
    // Must be provided
    if(!loot_owner)
        return false;

    LootTemplate const* tab = store.GetLootFor(loot_id);

    if (!tab)
    {
        if (!noEmptyError)
            sLog.outErrorDb("Table '%s' loot id #%u used but it doesn't have records.",store.GetName(),loot_id);
        return false;
    }

    items.reserve(MAX_NR_LOOT_ITEMS);
    m_questItems.reserve(MAX_NR_QUEST_ITEMS);

    tab->Process(*this, store,store.IsRatesAllowed ());     // Processing is done there, callback via Loot::AddItem()

    // Setting access rights for group loot case
    Group * pGroup=loot_owner->GetGroup();
    if(!personal && pGroup)
    {
        for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
            if(Player* pl = itr->getSource())
                FillNotNormalLootFor(pl);
    }
    // ... for personal loot
    else
        FillNotNormalLootFor(loot_owner);

    return true;
}

void Loot::FillNotNormalLootFor(Player* pl)
{
    uint32 plguid = pl->GetGUIDLow();

    QuestItemMap::const_iterator qmapitr = m_playerQuestItems.find(plguid);
    if (qmapitr == m_playerQuestItems.end())
        FillQuestLoot(pl);

    qmapitr = m_playerFFAItems.find(plguid);
    if (qmapitr == m_playerFFAItems.end())
        FillFFALoot(pl);

    qmapitr = m_playerNonQuestNonFFAConditionalItems.find(plguid);
    if (qmapitr == m_playerNonQuestNonFFAConditionalItems.end())
        FillNonQuestNonFFAConditionalLoot(pl);
}

QuestItemList* Loot::FillFFALoot(Player* player)
{
    QuestItemList *ql = new QuestItemList();

    for(uint8 i = 0; i < items.size(); ++i)
    {
        LootItem &item = items[i];
        if(!item.is_looted && item.freeforall && item.AllowedForPlayer(player) )
        {
            ql->push_back(QuestItem(i));
            ++unlootedCount;
        }
    }
    if (ql->empty())
    {
        delete ql;
        return NULL;
    }

    m_playerFFAItems[player->GetGUIDLow()] = ql;
    return ql;
}

QuestItemList* Loot::FillQuestLoot(Player* player)
{
    if (items.size() == MAX_NR_LOOT_ITEMS) return NULL;
    QuestItemList *ql = new QuestItemList();

    for(uint8 i = 0; i < m_questItems.size(); ++i)
    {
        LootItem &item = m_questItems[i];
        if(!item.is_looted && item.AllowedForPlayer(player) )
        {
            ql->push_back(QuestItem(i));

            // questitems get blocked when they first apper in a
            // player's quest vector
            //
            // increase once if one looter only, looter-times if free for all
            if (item.freeforall || !item.is_blocked)
                ++unlootedCount;

            item.is_blocked = true;

            if (items.size() + ql->size() == MAX_NR_LOOT_ITEMS)
                break;
        }
    }
    if (ql->empty())
    {
        delete ql;
        return NULL;
    }

    m_playerQuestItems[player->GetGUIDLow()] = ql;
    return ql;
}

QuestItemList* Loot::FillNonQuestNonFFAConditionalLoot(Player* player)
{
    QuestItemList *ql = new QuestItemList();

    for(uint8 i = 0; i < items.size(); ++i)
    {
        LootItem &item = items[i];
        if(!item.is_looted && !item.freeforall && item.conditionId && item.AllowedForPlayer(player))
        {
            ql->push_back(QuestItem(i));
            if(!item.is_counted)
            {
                ++unlootedCount;
                item.is_counted=true;
            }
        }
    }
    if (ql->empty())
    {
        delete ql;
        return NULL;
    }

    m_playerNonQuestNonFFAConditionalItems[player->GetGUIDLow()] = ql;
    return ql;
}

//===================================================

void Loot::NotifyItemRemoved(uint8 lootIndex)
{
    // notify all players that are looting this that the item was removed
    // convert the index to the slot the player sees
    PlayersLooting::iterator i_next;
    for (PlayersLooting::iterator i = m_playersLooting.begin(); i != m_playersLooting.end(); i = i_next)
    {
        i_next = i;
        ++i_next;
        if (Player* pl = ObjectAccessor::FindPlayer(*i))
            pl->SendNotifyLootItemRemoved(lootIndex);
        else
            m_playersLooting.erase(i);
    }
}

void Loot::NotifyMoneyRemoved()
{
    // notify all players that are looting this that the money was removed
    PlayersLooting::iterator i_next;
    for (PlayersLooting::iterator i = m_playersLooting.begin(); i != m_playersLooting.end(); i = i_next)
    {
        i_next = i;
        ++i_next;
        if (Player* pl = ObjectAccessor::FindPlayer(*i))
            pl->SendNotifyLootMoneyRemoved();
        else
            m_playersLooting.erase(i);
    }
}

void Loot::NotifyQuestItemRemoved(uint8 questIndex)
{
    // when a free for all questitem is looted
    // all players will get notified of it being removed
    // (other questitems can be looted by each group member)
    // bit inefficient but isnt called often

    PlayersLooting::iterator i_next;
    for (PlayersLooting::iterator i = m_playersLooting.begin(); i != m_playersLooting.end(); i = i_next)
    {
        i_next = i;
        ++i_next;
        if (Player* pl = ObjectAccessor::FindPlayer(*i))
        {
            QuestItemMap::const_iterator pq = m_playerQuestItems.find(pl->GetGUIDLow());
            if (pq != m_playerQuestItems.end() && pq->second)
            {
                // find where/if the player has the given item in it's vector
                QuestItemList& pql = *pq->second;

                uint8 j;
                for (j = 0; j < pql.size(); ++j)
                    if (pql[j].index == questIndex)
                        break;

                if (j < pql.size())
                    pl->SendNotifyLootItemRemoved(items.size()+j);
            }
        }
        else
            m_playersLooting.erase(i);
    }
}

void Loot::generateMoneyLoot( uint32 minAmount, uint32 maxAmount )
{
    if (maxAmount > 0)
    {
        if (maxAmount <= minAmount)
            gold = uint32(maxAmount * sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_MONEY));
        else if ((maxAmount - minAmount) < 32700)
            gold = uint32(urand(minAmount, maxAmount) * sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_MONEY));
        else
            gold = uint32(urand(minAmount >> 8, maxAmount >> 8) * sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_MONEY)) << 8;
    }
}

LootItem* Loot::LootItemInSlot(uint32 lootSlot, Player* player, QuestItem **qitem, QuestItem **ffaitem, QuestItem **conditem)
{
    LootItem* item = NULL;
    bool is_looted = true;
    if (lootSlot >= items.size())
    {
        uint32 questSlot = lootSlot - items.size();
        QuestItemMap::const_iterator itr = m_playerQuestItems.find(player->GetGUIDLow());
        if (itr != m_playerQuestItems.end() && questSlot < itr->second->size())
        {
            QuestItem *qitem2 = &itr->second->at(questSlot);
            if(qitem)
                *qitem = qitem2;
            item = &m_questItems[qitem2->index];
            is_looted = qitem2->is_looted;
        }
    }
    else
    {
        item = &items[lootSlot];
        is_looted = item->is_looted;
        if(item->freeforall)
        {
            QuestItemMap::const_iterator itr = m_playerFFAItems.find(player->GetGUIDLow());
            if (itr != m_playerFFAItems.end())
            {
                for(QuestItemList::const_iterator iter=itr->second->begin(); iter!= itr->second->end(); ++iter)
                    if(iter->index==lootSlot)
                    {
                        QuestItem *ffaitem2 = (QuestItem*)&(*iter);
                        if(ffaitem)
                            *ffaitem = ffaitem2;
                        is_looted = ffaitem2->is_looted;
                        break;
                    }
            }
        }
        else if (item->conditionId)
        {
            QuestItemMap::const_iterator itr = m_playerNonQuestNonFFAConditionalItems.find(player->GetGUIDLow());
            if (itr != m_playerNonQuestNonFFAConditionalItems.end())
            {
                for(QuestItemList::const_iterator iter=itr->second->begin(); iter!= itr->second->end(); ++iter)
                {
                    if(iter->index==lootSlot)
                    {
                        QuestItem *conditem2 = (QuestItem*)&(*iter);
                        if(conditem)
                            *conditem = conditem2;
                        is_looted = conditem2->is_looted;
                        break;
                    }
                }
            }
        }
    }

    if(is_looted)
        return NULL;

    return item;
}

uint32 Loot::GetMaxSlotInLootFor(Player* player) const
{
    QuestItemMap::const_iterator itr = m_playerQuestItems.find(player->GetGUIDLow());
    return items.size() + (itr != m_playerQuestItems.end() ?  itr->second->size() : 0);
}

ByteBuffer& operator<<(ByteBuffer& b, LootItem const& li)
{
    b << uint32(li.itemid);
    b << uint32(li.count);                                  // nr of items of this type
    b << uint32(ObjectMgr::GetItemPrototype(li.itemid)->DisplayInfoID);
    b << uint32(li.randomSuffix);
    b << uint32(li.randomPropertyId);
    //b << uint8(0);                                        // slot type - will send after this function call
    return b;
}

ByteBuffer& operator<<(ByteBuffer& b, LootView const& lv)
{
    if (lv.permission == NONE_PERMISSION)
    {
        b << uint32(0);                                     //gold
        b << uint8(0);                                      // item count
        return b;                                           // nothing output more
    }

    Loot &l = lv.loot;

    uint8 itemsShown = 0;

    //gold
    b << uint32(l.gold);

    size_t count_pos = b.wpos();                            // pos of item count byte
    b << uint8(0);                                          // item count placeholder

    if (lv.permission == NONE_PERMISSION)
        return b;                                           // nothing output more


    for (uint8 i = 0; i < l.items.size(); ++i)
    {
        LootSlotType slot_type = l.items[i].GetSlotTypeForSharedLoot(lv.permission, lv.viewer);
        if (slot_type >= MAX_LOOT_SLOT_TYPE)
            continue;

        b << uint8(i) << l.items[i];
        b << uint8(slot_type);                              // 0 - get 1 - look only 2 - master selection
        ++itemsShown;
    }

    QuestItemMap const& lootPlayerNonQuestNonFFAConditionalItems = l.GetPlayerNonQuestNonFFAConditionalItems();
    QuestItemMap::const_iterator nn_itr = lootPlayerNonQuestNonFFAConditionalItems.find(lv.viewer->GetGUIDLow());
    if (nn_itr != lootPlayerNonQuestNonFFAConditionalItems.end())
    {
        QuestItemList *conditional_list =  nn_itr->second;
        for (QuestItemList::const_iterator ci = conditional_list->begin() ; ci != conditional_list->end(); ++ci)
        {
            LootItem &item = l.items[ci->index];

            LootSlotType slot_type = item.GetSlotTypeForSharedLoot(lv.permission, lv.viewer, !ci->is_looted);
            if (slot_type >= MAX_LOOT_SLOT_TYPE)
                continue;

            b << uint8(ci->index) << item;
            b << uint8(slot_type);                          // allow loot
            ++itemsShown;
        }
    }

    // in next cases used same slot type for all items
    LootSlotType slot_type = lv.permission == OWNER_PERMISSION ? LOOT_SLOT_OWNER : LOOT_SLOT_NORMAL;

    QuestItemMap const& lootPlayerQuestItems = l.GetPlayerQuestItems();
    QuestItemMap::const_iterator q_itr = lootPlayerQuestItems.find(lv.viewer->GetGUIDLow());
    if (q_itr != lootPlayerQuestItems.end())
    {
        QuestItemList *q_list = q_itr->second;
        for (QuestItemList::const_iterator qi = q_list->begin() ; qi != q_list->end(); ++qi)
        {
            LootItem &item = l.m_questItems[qi->index];
            if (!qi->is_looted && !item.is_looted)
            {
                b << uint8(l.items.size() + (qi - q_list->begin()));
                b << item;
                b << uint8(slot_type);                      // allow loot
                ++itemsShown;
            }
        }
    }

    QuestItemMap const& lootPlayerFFAItems = l.GetPlayerFFAItems();
    QuestItemMap::const_iterator ffa_itr = lootPlayerFFAItems.find(lv.viewer->GetGUIDLow());
    if (ffa_itr != lootPlayerFFAItems.end())
    {
        QuestItemList *ffa_list = ffa_itr->second;
        for (QuestItemList::const_iterator fi = ffa_list->begin() ; fi != ffa_list->end(); ++fi)
        {
            LootItem &item = l.items[fi->index];
            if (!fi->is_looted && !item.is_looted)
            {
                b << uint8(fi->index) << item;
                b << uint8(slot_type);                      // allow loot
                ++itemsShown;
            }
        }
    }

    //update number of items shown
    b.put<uint8>(count_pos,itemsShown);

    return b;
}

//
// --------- LootTemplate::LootGroup ---------
//

// Adds an entry to the group (at loading stage)
void LootTemplate::LootGroup::AddEntry(LootStoreItem& item)
{
    if (item.chance != 0)
        ExplicitlyChanced.push_back(item);
    else
        EqualChanced.push_back(item);
}

// Rolls an item from the group, returns NULL if all miss their chances
LootStoreItem const * LootTemplate::LootGroup::Roll() const
{
    if (!ExplicitlyChanced.empty())                         // First explicitly chanced entries are checked
    {
        float Roll = rand_chance_f();

        for (uint32 i=0; i<ExplicitlyChanced.size(); ++i)    //check each explicitly chanced entry in the template and modify its chance based on quality.
        {
            if(ExplicitlyChanced[i].chance>=100.0f)
                return &ExplicitlyChanced[i];

            Roll -= ExplicitlyChanced[i].chance;
            if (Roll < 0)
                return &ExplicitlyChanced[i];
        }
    }
    if (!EqualChanced.empty())                              // If nothing selected yet - an item is taken from equal-chanced part
        return &EqualChanced[irand(0, EqualChanced.size()-1)];

    return NULL;                                            // Empty drop from the group
}

// True if group includes at least 1 quest drop entry
bool LootTemplate::LootGroup::HasQuestDrop() const
{
    for (LootStoreItemList::const_iterator i=ExplicitlyChanced.begin(); i != ExplicitlyChanced.end(); ++i)
        if (i->needs_quest)
            return true;
    for (LootStoreItemList::const_iterator i=EqualChanced.begin(); i != EqualChanced.end(); ++i)
        if (i->needs_quest)
            return true;
    return false;
}

// True if group includes at least 1 quest drop entry for active quests of the player
bool LootTemplate::LootGroup::HasQuestDropForPlayer(Player const * player) const
{
    for (LootStoreItemList::const_iterator i=ExplicitlyChanced.begin(); i != ExplicitlyChanced.end(); ++i)
        if (player->HasQuestForItem(i->itemid))
            return true;
    for (LootStoreItemList::const_iterator i=EqualChanced.begin(); i != EqualChanced.end(); ++i)
        if (player->HasQuestForItem(i->itemid))
            return true;
    return false;
}

// Rolls an item from the group (if any takes its chance) and adds the item to the loot
void LootTemplate::LootGroup::Process(Loot& loot) const
{
    LootStoreItem const * item = Roll();
    if (item != NULL)
        loot.AddItem(*item);
}

// Overall chance for the group without equal chanced items
float LootTemplate::LootGroup::RawTotalChance() const
{
    float result = 0;

    for (LootStoreItemList::const_iterator i=ExplicitlyChanced.begin(); i != ExplicitlyChanced.end(); ++i)
        if ( !i->needs_quest )
            result += i->chance;

    return result;
}

// Overall chance for the group
float LootTemplate::LootGroup::TotalChance() const
{
    float result = RawTotalChance();

    if (!EqualChanced.empty() && result < 100.0f)
        return 100.0f;

    return result;
}

void LootTemplate::LootGroup::Verify(LootStore const& lootstore, uint32 id, uint32 group_id) const
{
    float chance = RawTotalChance();
    if (chance > 101.0f)                                    // TODO: replace with 100% when DBs will be ready
    {
        sLog.outErrorDb("Table '%s' entry %u group %d has total chance > 100%% (%f)", lootstore.GetName(), id, group_id, chance);
    }

    if(chance >= 100.0f && !EqualChanced.empty())
    {
        sLog.outErrorDb("Table '%s' entry %u group %d has items with chance=0%% but group total chance >= 100%% (%f)", lootstore.GetName(), id, group_id, chance);
    }
}

void LootTemplate::LootGroup::CheckLootRefs(LootIdSet* ref_set) const
{
    for (LootStoreItemList::const_iterator ieItr=ExplicitlyChanced.begin(); ieItr != ExplicitlyChanced.end(); ++ieItr)
    {
        if(ieItr->mincountOrRef < 0)
        {
            if(!LootTemplates_Reference.GetLootFor(-ieItr->mincountOrRef))
                LootTemplates_Reference.ReportNotExistedId(-ieItr->mincountOrRef);
            else if(ref_set)
                ref_set->erase(-ieItr->mincountOrRef);
        }
    }

    for (LootStoreItemList::const_iterator ieItr=EqualChanced.begin(); ieItr != EqualChanced.end(); ++ieItr)
    {
        if(ieItr->mincountOrRef < 0)
        {
            if(!LootTemplates_Reference.GetLootFor(-ieItr->mincountOrRef))
                LootTemplates_Reference.ReportNotExistedId(-ieItr->mincountOrRef);
            else if(ref_set)
                ref_set->erase(-ieItr->mincountOrRef);
        }
    }
}

//
// --------- LootTemplate ---------
//

// Adds an entry to the group (at loading stage)
void LootTemplate::AddEntry(LootStoreItem& item)
{
    if (item.group > 0 && item.mincountOrRef > 0)           // Group
    {
        if (item.group >= Groups.size())
            Groups.resize(item.group);                      // Adds new group the the loot template if needed
        Groups[item.group-1].AddEntry(item);                // Adds new entry to the group
    }
    else                                                    // Non-grouped entries and references are stored together
        Entries.push_back(item);
}

// Rolls for every item in the template and adds the rolled items the the loot
void LootTemplate::Process(Loot& loot, LootStore const& store, bool rate, uint8 groupId) const
{
    if (groupId)                                            // Group reference uses own processing of the group
    {
        if (groupId > Groups.size())
            return;                                         // Error message already printed at loading stage

        Groups[groupId-1].Process(loot);
        return;
    }

    // Rolling non-grouped items
    for (LootStoreItemList::const_iterator i = Entries.begin() ; i != Entries.end() ; ++i )
    {
        if (!i->Roll(rate))
            continue;                                       // Bad luck for the entry

        if (i->mincountOrRef < 0)                           // References processing
        {
            LootTemplate const* Referenced = LootTemplates_Reference.GetLootFor(-i->mincountOrRef);

            if(!Referenced)
                continue;                                   // Error message already printed at loading stage

            for (uint32 loop=0; loop < i->maxcount; ++loop )// Ref multiplicator
                Referenced->Process(loot, store, rate, i->group);
        }
        else                                                // Plain entries (not a reference, not grouped)
            loot.AddItem(*i);                               // Chance is already checked, just add
    }

    // Now processing groups
    for (LootGroups::const_iterator i = Groups.begin( ) ; i != Groups.end( ) ; ++i )
        i->Process(loot);
}

// True if template includes at least 1 quest drop entry
bool LootTemplate::HasQuestDrop(LootTemplateMap const& store, uint8 groupId) const
{
    if (groupId)                                            // Group reference
    {
        if (groupId > Groups.size())
            return false;                                   // Error message [should be] already printed at loading stage
        return Groups[groupId-1].HasQuestDrop();
    }

    for (LootStoreItemList::const_iterator i = Entries.begin(); i != Entries.end(); ++i )
    {
        if (i->mincountOrRef < 0)                           // References
        {
            LootTemplateMap::const_iterator Referenced = store.find(-i->mincountOrRef);
            if( Referenced ==store.end() )
                continue;                                   // Error message [should be] already printed at loading stage
            if (Referenced->second->HasQuestDrop(store, i->group) )
                return true;
        }
        else if ( i->needs_quest )
            return true;                                    // quest drop found
    }

    // Now processing groups
    for (LootGroups::const_iterator i = Groups.begin() ; i != Groups.end() ; ++i )
        if (i->HasQuestDrop())
            return true;

    return false;
}

// True if template includes at least 1 quest drop for an active quest of the player
bool LootTemplate::HasQuestDropForPlayer(LootTemplateMap const& store, Player const* player, uint8 groupId) const
{
    if (groupId)                                            // Group reference
    {
        if (groupId > Groups.size())
            return false;                                   // Error message already printed at loading stage
        return Groups[groupId-1].HasQuestDropForPlayer(player);
    }

    // Checking non-grouped entries
    for (LootStoreItemList::const_iterator i = Entries.begin() ; i != Entries.end() ; ++i )
    {
        if (i->mincountOrRef < 0)                           // References processing
        {
            LootTemplateMap::const_iterator Referenced = store.find(-i->mincountOrRef);
            if (Referenced == store.end() )
                continue;                                   // Error message already printed at loading stage
            if (Referenced->second->HasQuestDropForPlayer(store, player, i->group) )
                return true;
        }
        else if ( player->HasQuestForItem(i->itemid) )
            return true;                                    // active quest drop found
    }

    // Now checking groups
    for (LootGroups::const_iterator i = Groups.begin(); i != Groups.end(); ++i )
        if (i->HasQuestDropForPlayer(player))
            return true;

    return false;
}

// Checks integrity of the template
void LootTemplate::Verify(LootStore const& lootstore, uint32 id) const
{
    // Checking group chances
    for (uint32 i=0; i < Groups.size(); ++i)
        Groups[i].Verify(lootstore,id,i+1);

    // TODO: References validity checks
}

void LootTemplate::CheckLootRefs(LootIdSet* ref_set) const
{
    for(LootStoreItemList::const_iterator ieItr = Entries.begin(); ieItr != Entries.end(); ++ieItr)
    {
        if(ieItr->mincountOrRef < 0)
        {
            if(!LootTemplates_Reference.GetLootFor(-ieItr->mincountOrRef))
                LootTemplates_Reference.ReportNotExistedId(-ieItr->mincountOrRef);
            else if(ref_set)
                ref_set->erase(-ieItr->mincountOrRef);
        }
    }

    for(LootGroups::const_iterator grItr = Groups.begin(); grItr != Groups.end(); ++grItr)
        grItr->CheckLootRefs(ref_set);
}

void LoadLootTemplates_Creature()
{
    LootIdSet ids_set, ids_setUsed;
    LootTemplates_Creature.LoadAndCollectLootIds(ids_set);

    // remove real entries and check existence loot
    for(uint32 i = 1; i < sCreatureStorage.MaxEntry; ++i )
    {
        if(CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(i))
        {
            if(uint32 lootid = cInfo->lootid)
            {
                if (ids_set.find(lootid) == ids_set.end())
                    LootTemplates_Creature.ReportNotExistedId(lootid);
                else
                    ids_setUsed.insert(lootid);
            }
        }
    }
    for(LootIdSet::const_iterator itr = ids_setUsed.begin(); itr != ids_setUsed.end(); ++itr)
        ids_set.erase(*itr);

    // for alterac valley we've defined Player-loot inside creature_loot_template id=0
    // this hack is used, so that we won't need to create an extra table player_loot_template for just one case
    ids_set.erase(0);

    // output error for any still listed (not referenced from appropriate table) ids
    LootTemplates_Creature.ReportUnusedIds(ids_set);
}

void LoadLootTemplates_Disenchant()
{
    LootIdSet ids_set, ids_setUsed;
    LootTemplates_Disenchant.LoadAndCollectLootIds(ids_set);

    // remove real entries and check existence loot
    for(uint32 i = 1; i < sItemStorage.MaxEntry; ++i )
    {
        if(ItemPrototype const* proto = sItemStorage.LookupEntry<ItemPrototype>(i))
        {
            if(uint32 lootid = proto->DisenchantID)
            {
                if (ids_set.find(lootid) == ids_set.end())
                    LootTemplates_Disenchant.ReportNotExistedId(lootid);
                else
                    ids_setUsed.insert(lootid);
            }
        }
    }
    for(LootIdSet::const_iterator itr = ids_setUsed.begin(); itr != ids_setUsed.end(); ++itr)
        ids_set.erase(*itr);
    // output error for any still listed (not referenced from appropriate table) ids
    LootTemplates_Disenchant.ReportUnusedIds(ids_set);
}

void LoadLootTemplates_Fishing()
{
    LootIdSet ids_set;
    LootTemplates_Fishing.LoadAndCollectLootIds(ids_set);

    // remove real entries and check existence loot
    for(uint32 i = 1; i < sAreaStore.GetNumRows(); ++i )
    {
        if(AreaTableEntry const* areaEntry = sAreaStore.LookupEntry(i))
            if (ids_set.find(areaEntry->ID) != ids_set.end())
                ids_set.erase(areaEntry->ID);
    }

    // by default (look config options) fishing at fail provide junk loot, entry 0 use for store this loot
    ids_set.erase(0);

    // output error for any still listed (not referenced from appropriate table) ids
    LootTemplates_Fishing.ReportUnusedIds(ids_set);
}

void LoadLootTemplates_Gameobject()
{
    LootIdSet ids_set, ids_setUsed;
    LootTemplates_Gameobject.LoadAndCollectLootIds(ids_set);

    // remove real entries and check existence loot
    for(uint32 i = 1; i < sGOStorage.MaxEntry; ++i )
    {
        if(GameObjectInfo const* gInfo = sGOStorage.LookupEntry<GameObjectInfo>(i))
        {
            if(uint32 lootid = gInfo->GetLootId())
            {
                if (ids_set.find(lootid) == ids_set.end())
                    LootTemplates_Gameobject.ReportNotExistedId(lootid);
                else
                    ids_setUsed.insert(lootid);
            }
        }
    }
    for(LootIdSet::const_iterator itr = ids_setUsed.begin(); itr != ids_setUsed.end(); ++itr)
        ids_set.erase(*itr);

    // output error for any still listed (not referenced from appropriate table) ids
    LootTemplates_Gameobject.ReportUnusedIds(ids_set);
}

void LoadLootTemplates_Item()
{
    LootIdSet ids_set;
    LootTemplates_Item.LoadAndCollectLootIds(ids_set);

    // remove real entries and check existence loot
    for(uint32 i = 1; i < sItemStorage.MaxEntry; ++i )
    {
        if(ItemPrototype const* proto = sItemStorage.LookupEntry<ItemPrototype>(i))
        {
            if  (!(proto->Flags & ITEM_FLAG_LOOTABLE))
                continue;

            if (ids_set.find(proto->ItemId) != ids_set.end() || proto->MaxMoneyLoot > 0)
                ids_set.erase(proto->ItemId);
            // wdb have wrong data cases, so skip by default
            else if (!sLog.HasLogFilter(LOG_FILTER_DB_STRICTED_CHECK))
                LootTemplates_Item.ReportNotExistedId(proto->ItemId);
        }
    }

    // output error for any still listed (not referenced from appropriate table) ids
    LootTemplates_Item.ReportUnusedIds(ids_set);
}

void LoadLootTemplates_Milling()
{
    LootIdSet ids_set;
    LootTemplates_Milling.LoadAndCollectLootIds(ids_set);

    // remove real entries and check existence loot
    for(uint32 i = 1; i < sItemStorage.MaxEntry; ++i )
    {
        ItemPrototype const* proto = sItemStorage.LookupEntry<ItemPrototype>(i);
        if(!proto)
            continue;

        if (!(proto->Flags & ITEM_FLAG_MILLABLE))
            continue;

        if (ids_set.find(proto->ItemId) != ids_set.end())
            ids_set.erase(proto->ItemId);
        else
            LootTemplates_Milling.ReportNotExistedId(proto->ItemId);
    }

    // output error for any still listed (not referenced from appropriate table) ids
    LootTemplates_Milling.ReportUnusedIds(ids_set);
}

void LoadLootTemplates_Pickpocketing()
{
    LootIdSet ids_set, ids_setUsed;
    LootTemplates_Pickpocketing.LoadAndCollectLootIds(ids_set);

    // remove real entries and check existence loot
    for(uint32 i = 1; i < sCreatureStorage.MaxEntry; ++i )
    {
        if(CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(i))
        {
            if(uint32 lootid = cInfo->pickpocketLootId)
            {
                if (ids_set.find(lootid) == ids_set.end())
                    LootTemplates_Pickpocketing.ReportNotExistedId(lootid);
                else
                    ids_setUsed.insert(lootid);
            }
        }
    }
    for(LootIdSet::const_iterator itr = ids_setUsed.begin(); itr != ids_setUsed.end(); ++itr)
        ids_set.erase(*itr);

    // output error for any still listed (not referenced from appropriate table) ids
    LootTemplates_Pickpocketing.ReportUnusedIds(ids_set);
}

void LoadLootTemplates_Prospecting()
{
    LootIdSet ids_set;
    LootTemplates_Prospecting.LoadAndCollectLootIds(ids_set);

    // remove real entries and check existence loot
    for(uint32 i = 1; i < sItemStorage.MaxEntry; ++i )
    {
        ItemPrototype const* proto = sItemStorage.LookupEntry<ItemPrototype>(i);
        if (!proto)
            continue;

        if (!(proto->Flags & ITEM_FLAG_PROSPECTABLE))
            continue;

        if (ids_set.find(proto->ItemId) != ids_set.end())
            ids_set.erase(proto->ItemId);
        //else -- exist some cases that possible can be prospected but not expected have any result loot
        //    LootTemplates_Prospecting.ReportNotExistedId(proto->ItemId);
    }

    // output error for any still listed (not referenced from appropriate table) ids
    LootTemplates_Prospecting.ReportUnusedIds(ids_set);
}

void LoadLootTemplates_Mail()
{
    LootIdSet ids_set;
    LootTemplates_Mail.LoadAndCollectLootIds(ids_set);

    // remove real entries and check existence loot
    for(uint32 i = 1; i < sMailTemplateStore.GetNumRows(); ++i )
        if(sMailTemplateStore.LookupEntry(i))
            if (ids_set.find(i) != ids_set.end())
                ids_set.erase(i);

    // output error for any still listed (not referenced from appropriate table) ids
    LootTemplates_Mail.ReportUnusedIds(ids_set);
}

void LoadLootTemplates_Skinning()
{
    LootIdSet ids_set, ids_setUsed;
    LootTemplates_Skinning.LoadAndCollectLootIds(ids_set);

    // remove real entries and check existence loot
    for(uint32 i = 1; i < sCreatureStorage.MaxEntry; ++i )
    {
        if(CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(i))
        {
            if(uint32 lootid = cInfo->SkinLootId)
            {
                if (ids_set.find(lootid) == ids_set.end())
                    LootTemplates_Skinning.ReportNotExistedId(lootid);
                else
                    ids_setUsed.insert(lootid);
            }
        }
    }
    for(LootIdSet::const_iterator itr = ids_setUsed.begin(); itr != ids_setUsed.end(); ++itr)
        ids_set.erase(*itr);

    // output error for any still listed (not referenced from appropriate table) ids
    LootTemplates_Skinning.ReportUnusedIds(ids_set);
}

void LoadLootTemplates_Spell()
{
    LootIdSet ids_set;
    LootTemplates_Spell.LoadAndCollectLootIds(ids_set);

    // remove real entries and check existence loot
    for(uint32 spell_id = 1; spell_id < sSpellStore.GetNumRows(); ++spell_id)
    {
        SpellEntry const* spellInfo = sSpellStore.LookupEntry (spell_id);
        if(!spellInfo)
            continue;

        // possible cases
        if( !IsLootCraftingSpell(spellInfo))
            continue;

        if (ids_set.find(spell_id) == ids_set.end())
        {
            // not report about not trainable spells (optionally supported by DB)
            // ignore 61756 (Northrend Inscription Research (FAST QA VERSION) for example
            if (!(spellInfo->Attributes & SPELL_ATTR_NOT_SHAPESHIFT) || (spellInfo->Attributes & SPELL_ATTR_TRADESPELL))
            {
                LootTemplates_Spell.ReportNotExistedId(spell_id);
            }
        }
        else
            ids_set.erase(spell_id);
    }

    // output error for any still listed (not referenced from appropriate table) ids
    LootTemplates_Spell.ReportUnusedIds(ids_set);
}

void LoadLootTemplates_Reference()
{
    LootIdSet ids_set;
    LootTemplates_Reference.LoadAndCollectLootIds(ids_set);

    // check references and remove used
    LootTemplates_Creature.CheckLootRefs(&ids_set);
    LootTemplates_Fishing.CheckLootRefs(&ids_set);
    LootTemplates_Gameobject.CheckLootRefs(&ids_set);
    LootTemplates_Item.CheckLootRefs(&ids_set);
    LootTemplates_Milling.CheckLootRefs(&ids_set);
    LootTemplates_Pickpocketing.CheckLootRefs(&ids_set);
    LootTemplates_Skinning.CheckLootRefs(&ids_set);
    LootTemplates_Disenchant.CheckLootRefs(&ids_set);
    LootTemplates_Prospecting.CheckLootRefs(&ids_set);
    LootTemplates_Mail.CheckLootRefs(&ids_set);
    LootTemplates_Reference.CheckLootRefs(&ids_set);

    // output error for any still listed ids (not referenced from any loot table)
    LootTemplates_Reference.ReportUnusedIds(ids_set);
}
