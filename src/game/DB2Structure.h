/*
 * Copyright (C) 2005-2013 MaNGOS <http://getmangos.com/>
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

#ifndef MANGOS_DB2STRUCTURE_H
#define MANGOS_DB2STRUCTURE_H

#include "Common.h"
#include "DBCEnums.h"
#include "Path.h"
#include "Platform/Define.h"

#include <map>
#include <set>
#include <vector>

// Structures using to access raw DB2 data and required packing to portability
struct ItemEntry
{
   uint32   ID;                                             // 0
   uint32   Class;                                          // 1
   uint32   SubClass;                                       // 2
   int32    Unk0;                                           // 3
   int32    Material;                                       // 4
   uint32   DisplayId;                                      // 5
   uint32   InventoryType;                                  // 6
   uint32   Sheath;                                         // 7
};

struct ItemCurrencyCostEntry
{
    //uint32 id;                                            // 0
    uint32 itemid;                                          // 1
};

#define MAX_EXTENDED_COST_ITEMS         5
#define MAX_EXTENDED_COST_CURRENCIES    5

enum ItemExtendedCostFlags
{
    ITEM_EXTENDED_COST_FLAG_UNK                 = 0x01, // guild related
    ITEM_EXTENDED_COST_FLAG_SEASON_IN_INDEX_0   = 0x02, // currency requirements under these indexes require season count
    ITEM_EXTENDED_COST_FLAG_SEASON_IN_INDEX_1   = 0x04,
    ITEM_EXTENDED_COST_FLAG_SEASON_IN_INDEX_2   = 0x08,
    ITEM_EXTENDED_COST_FLAG_SEASON_IN_INDEX_3   = 0x10,
    ITEM_EXTENDED_COST_FLAG_SEASON_IN_INDEX_4   = 0x20,
};

struct ItemExtendedCostEntry
{
    uint32      Id;                                         // 0
    //                                                      // 1        unk, old reqhonorpoints
    //                                                      // 2        unk, old reqarenapoints
    uint32      reqarenaslot;                               // 3        m_arenaBracket
    uint32      reqitem[MAX_EXTENDED_COST_ITEMS];           // 5-8      m_itemID
    uint32      reqitemcount[MAX_EXTENDED_COST_ITEMS];      // 9-13     m_itemCount
    uint32      reqpersonalarenarating;                     // 14       m_requiredArenaRating
    //uint32                                                // 15       m_itemPurchaseGroup
    uint32      reqcur[MAX_EXTENDED_COST_CURRENCIES];       // 16-20
    uint32      reqcurrcount[MAX_EXTENDED_COST_CURRENCIES]; // 21-25
                                                            // 26       reputation-related
                                                            // 27       reputation-related
    uint32      flags;                                      // 28
    //                                                      // 29
    //                                                      // 30

    bool IsSeasonCurrencyRequirement(uint32 i) const
    {
        MANGOS_ASSERT(i < MAX_EXTENDED_COST_CURRENCIES);

        // start from ITEM_EXTENDED_COST_FLAG_SEASON_IN_INDEX_0
        return flags & 1 << (i + 1);
    }
};

#endif
