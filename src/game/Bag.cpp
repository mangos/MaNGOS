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

#include "Bag.h"
#include "ObjectMgr.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "UpdateData.h"

Bag::Bag( ): Item()
{
    m_objectType |= (TYPEMASK_ITEM | TYPEMASK_CONTAINER);
    m_objectTypeId = TYPEID_CONTAINER;

    m_valuesCount = CONTAINER_END;

    memset(m_bagslot, 0, sizeof(Item *) * MAX_BAG_SIZE);
}

Bag::~Bag()
{
    for(int i = 0; i < MAX_BAG_SIZE; ++i)
        if (m_bagslot[i])
            delete m_bagslot[i];
}

void Bag::AddToWorld()
{
    Item::AddToWorld();

    for(uint32 i = 0;  i < GetBagSize(); ++i)
        if(m_bagslot[i])
            m_bagslot[i]->AddToWorld();
}

void Bag::RemoveFromWorld()
{
    for(uint32 i = 0; i < GetBagSize(); ++i)
        if(m_bagslot[i])
            m_bagslot[i]->RemoveFromWorld();

    Item::RemoveFromWorld();
}

bool Bag::Create(uint32 guidlow, uint32 itemid, Player const* owner)
{
    ItemPrototype const * itemProto = ObjectMgr::GetItemPrototype(itemid);

    if(!itemProto || itemProto->ContainerSlots > MAX_BAG_SIZE)
        return false;

    Object::_Create(ObjectGuid(HIGHGUID_CONTAINER, guidlow));

    SetEntry(itemid);
    SetObjectScale(DEFAULT_OBJECT_SCALE);

    SetGuidValue(ITEM_FIELD_OWNER, owner ? owner->GetObjectGuid() : ObjectGuid());
    SetGuidValue(ITEM_FIELD_CONTAINED, owner ? owner->GetObjectGuid() : ObjectGuid());

    SetUInt32Value(ITEM_FIELD_MAXDURABILITY, itemProto->MaxDurability);
    SetUInt32Value(ITEM_FIELD_DURABILITY, itemProto->MaxDurability);
    SetUInt32Value(ITEM_FIELD_STACK_COUNT, 1);

    // Setting the number of Slots the Container has
    SetUInt32Value(CONTAINER_FIELD_NUM_SLOTS, itemProto->ContainerSlots);

    // Cleaning 20 slots
    for (uint8 i = 0; i < MAX_BAG_SIZE; ++i)
    {
        SetGuidValue(CONTAINER_FIELD_SLOT_1 + (i*2), ObjectGuid());
        m_bagslot[i] = NULL;
    }

    return true;
}

void Bag::SaveToDB()
{
    Item::SaveToDB();
}

bool Bag::LoadFromDB(uint32 guidLow, Field *fields, ObjectGuid ownerGuid)
{
    if (!Item::LoadFromDB(guidLow, fields, ownerGuid))
        return false;

    // cleanup bag content related item value fields (its will be filled correctly from `character_inventory`)
    for (int i = 0; i < MAX_BAG_SIZE; ++i)
    {
        SetGuidValue(CONTAINER_FIELD_SLOT_1 + (i*2), ObjectGuid());
        if (m_bagslot[i])
        {
            delete m_bagslot[i];
            m_bagslot[i] = NULL;
        }
    }

    return true;
}

void Bag::DeleteFromDB()
{
    for (int i = 0; i < MAX_BAG_SIZE; ++i)
        if (m_bagslot[i])
            m_bagslot[i]->DeleteFromDB();

    Item::DeleteFromDB();
}

uint32 Bag::GetFreeSlots() const
{
    uint32 slots = 0;
    for (uint32 i=0; i < GetBagSize(); ++i)
        if (!m_bagslot[i])
            ++slots;

    return slots;
}

void Bag::RemoveItem( uint8 slot, bool /*update*/ )
{
    MANGOS_ASSERT(slot < MAX_BAG_SIZE);

    if (m_bagslot[slot])
        m_bagslot[slot]->SetContainer(NULL);

    m_bagslot[slot] = NULL;
    SetGuidValue(CONTAINER_FIELD_SLOT_1 + (slot*2), ObjectGuid());
}

void Bag::StoreItem( uint8 slot, Item *pItem, bool /*update*/ )
{
    MANGOS_ASSERT(slot < MAX_BAG_SIZE);

    if( pItem )
    {
        m_bagslot[slot] = pItem;
        SetGuidValue(CONTAINER_FIELD_SLOT_1 + (slot * 2), pItem->GetObjectGuid());
        pItem->SetGuidValue(ITEM_FIELD_CONTAINED, GetObjectGuid());
        pItem->SetGuidValue(ITEM_FIELD_OWNER, GetOwnerGuid());
        pItem->SetContainer(this);
        pItem->SetSlot(slot);
    }
}

void Bag::BuildCreateUpdateBlockForPlayer( UpdateData *data, Player *target ) const
{
    Item::BuildCreateUpdateBlockForPlayer( data, target );

    for (uint32 i = 0; i < GetBagSize(); ++i)
        if(m_bagslot[i])
            m_bagslot[i]->BuildCreateUpdateBlockForPlayer( data, target );
}

// If the bag is empty returns true
bool Bag::IsEmpty() const
{
    for(uint32 i = 0; i < GetBagSize(); ++i)
        if (m_bagslot[i])
            return false;

    return true;
}

Item* Bag::GetItemByEntry( uint32 item ) const
{
    for(uint32 i = 0; i < GetBagSize(); ++i)
        if (m_bagslot[i] && m_bagslot[i]->GetEntry() == item)
            return m_bagslot[i];

    return NULL;
}

Item* Bag::GetItemByLimitedCategory(uint32 limitedCategory) const
{
    for(uint32 i = 0; i < GetBagSize(); ++i)
        if (m_bagslot[i] && m_bagslot[i]->GetProto()->ItemLimitCategory == limitedCategory)
            return m_bagslot[i];

    return NULL;
}

uint32 Bag::GetItemCount(uint32 item, Item* eItem) const
{
    uint32 count = 0;
    for(uint32 i=0; i < GetBagSize(); ++i)
        if (m_bagslot[i])
            if (m_bagslot[i] != eItem && m_bagslot[i]->GetEntry() == item)
                count += m_bagslot[i]->GetCount();

    if (eItem && eItem->GetProto()->GemProperties)
        for(uint32 i=0; i < GetBagSize(); ++i)
            if (m_bagslot[i])
                if (m_bagslot[i] != eItem && m_bagslot[i]->GetProto()->Socket[0].Color)
                    count += m_bagslot[i]->GetGemCountWithID(item);

    return count;
}

uint32 Bag::GetItemCountWithLimitCategory(uint32 limitCategory, Item* eItem) const
{
    uint32 count = 0;
    for(uint32 i = 0; i < GetBagSize(); ++i)
        if (m_bagslot[i])
            if (m_bagslot[i] != eItem && m_bagslot[i]->GetProto()->ItemLimitCategory == limitCategory )
                count += m_bagslot[i]->GetCount();

    return count;
}

uint8 Bag::GetSlotByItemGUID(uint64 guid) const
{
    for(uint32 i = 0; i < GetBagSize(); ++i)
        if(m_bagslot[i] != 0)
            if(m_bagslot[i]->GetGUID() == guid)
                return i;

    return NULL_SLOT;
}

Item* Bag::GetItemByPos( uint8 slot ) const
{
    if( slot < GetBagSize() )
        return m_bagslot[slot];

    return NULL;
}
