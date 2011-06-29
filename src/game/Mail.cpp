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

/**
 * @addtogroup mailing
 * @{
 *
 * @file Mail.cpp
 * This file contains the code needed for MaNGOS to handle mails.
 *
 */

#include "Mail.h"
#include "Log.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Item.h"
#include "Player.h"
#include "World.h"

/**
 * Creates a new MailSender object.
 *
 * @param sender The object/player sending this mail.
 * @param stationery The stationary associated with this sender.
 */
MailSender::MailSender( Object* sender, MailStationery stationery ) : m_stationery(stationery)
{
    switch(sender->GetTypeId())
    {
        case TYPEID_UNIT:
            m_messageType = MAIL_CREATURE;
            m_senderId = sender->GetEntry();
            break;
        case TYPEID_GAMEOBJECT:
            m_messageType = MAIL_GAMEOBJECT;
            m_senderId = sender->GetEntry();
            break;
        case TYPEID_ITEM:
        case TYPEID_CONTAINER:
            m_messageType = MAIL_ITEM;
            m_senderId = sender->GetEntry();
            break;
        case TYPEID_PLAYER:
            m_messageType = MAIL_NORMAL;
            m_senderId = sender->GetGUIDLow();
            break;
        default:
            m_messageType = MAIL_NORMAL;
            m_senderId = 0;                                 // will show mail from nonexistent player
            sLog.outError( "MailSender::MailSender - Mail have unexpected sender typeid (%u)", sender->GetTypeId());
            break;
    }
}
/**
 * Creates a new MailSender object from an AuctionEntry.
 *
 * @param sender the AuctionEntry from which this mail is generated.
 */
MailSender::MailSender( AuctionEntry* sender )
    : m_messageType(MAIL_AUCTION), m_senderId(sender->GetHouseId()), m_stationery(MAIL_STATIONERY_AUCTION)
{
}

/**
 * Creates a new MailReceiver object.
 *
 * @param receiver The player receiving the mail.
 */
MailReceiver::MailReceiver(Player* receiver) : m_receiver(receiver), m_receiver_guid(receiver->GetObjectGuid())
{
}
/**
 * Creates a new MailReceiver object with a specified GUID.
 *
 * @param receiver The player receiving the mail.
 * @param receiver_lowguid The GUID to use instead of the receivers.
 */
MailReceiver::MailReceiver(Player* receiver, ObjectGuid receiver_guid) : m_receiver(receiver), m_receiver_guid(receiver_guid)
{
    MANGOS_ASSERT(!receiver || receiver->GetObjectGuid() == receiver_guid);
}

/**
 * Adds an item to the MailDraft.
 *
 * @param item The item to be added to the MailDraft.
 * @returns the MailDraft the item was added to.
 */
MailDraft& MailDraft::AddItem( Item* item )
{
    m_items[item->GetGUIDLow()] = item;
    return *this;
}
/**
 * Prepares the items in a MailDraft.
 */
bool MailDraft::prepareItems(Player* receiver)
{
    if (!m_mailTemplateId || !m_mailTemplateItemsNeed)
        return false;

    m_mailTemplateItemsNeed = false;

    Loot mailLoot;

    // can be empty
    mailLoot.FillLoot(m_mailTemplateId, LootTemplates_Mail, receiver, true, true);

    uint32 max_slot = mailLoot.GetMaxSlotInLootFor(receiver);
    for(uint32 i = 0; m_items.size() < MAX_MAIL_ITEMS && i < max_slot; ++i)
    {
        if (LootItem* lootitem = mailLoot.LootItemInSlot(i, receiver))
        {
            if (Item* item = Item::CreateItem(lootitem->itemid, lootitem->count, receiver))
            {
                item->SaveToDB();                           // save for prevent lost at next mail load, if send fail then item will deleted
                AddItem(item);
            }
        }
    }

    return true;
}
/**
 * Deletes the items included in a MailDraft.
 *
 * @param inDB A boolean specifying whether the change should be saved to the database or not.
 */
void MailDraft::deleteIncludedItems( bool inDB /**= false*/ )
{
    for(MailItemMap::iterator mailItemIter = m_items.begin(); mailItemIter != m_items.end(); ++mailItemIter)
    {
        Item* item = mailItemIter->second;

        if(inDB)
            CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid='%u'", item->GetGUIDLow());

        delete item;
    }

    m_items.clear();
}
/**
 * Clone MailDraft from another MailDraft.
 *
 * @param draft Point to source for draft cloning.
 */
void MailDraft::CloneFrom(MailDraft const& draft)
{
    m_mailTemplateId = draft.GetMailTemplateId();
    m_mailTemplateItemsNeed = draft.m_mailTemplateItemsNeed;

    m_subject = draft.GetSubject();
    m_body = draft.GetBody();
    m_money = draft.GetMoney();
    m_COD = draft.GetCOD();

    for(MailItemMap::const_iterator mailItemIter = draft.m_items.begin(); mailItemIter != draft.m_items.end(); ++mailItemIter)
    {
        Item* item = mailItemIter->second;

        if(Item* newitem = item->CloneItem(item->GetCount()))
        {
            newitem->SaveToDB();
            AddItem(newitem);
        }
    }
}

/*
 * Returns a mail to its sender.
 * @param sender_acc           The id of the account of the sender.
 * @param sender_guid          The low part of the GUID of the sender.
 * @param receiver_guid        The low part of the GUID of the receiver.
 */
void MailDraft::SendReturnToSender(uint32 sender_acc, ObjectGuid sender_guid, ObjectGuid receiver_guid)
{
    Player *receiver = sObjectMgr.GetPlayer(receiver_guid);

    uint32 rc_account = 0;
    if (!receiver)
        rc_account = sObjectMgr.GetPlayerAccountIdByGUID(receiver_guid);

    if (!receiver && !rc_account)                           // sender not exist
    {
        deleteIncludedItems(true);
        return;
    }

    // prepare mail and send in other case
    bool needItemDelay = false;

    if (!m_items.empty())
    {
        // if item send to character at another account, then apply item delivery delay
        needItemDelay = sender_acc != rc_account;

        // set owner to new receiver (to prevent delete item with sender char deleting)
        CharacterDatabase.BeginTransaction();
        for (MailItemMap::iterator mailItemIter = m_items.begin(); mailItemIter != m_items.end(); ++mailItemIter)
        {
            Item* item = mailItemIter->second;
            item->SaveToDB();                               // item not in inventory and can be save standalone
            // owner in data will set at mail receive and item extracting
            CharacterDatabase.PExecute("UPDATE item_instance SET owner_guid = '%u' WHERE guid='%u'", receiver_guid.GetCounter(), item->GetGUIDLow());
        }
        CharacterDatabase.CommitTransaction();
    }

    // If theres is an item, there is a one hour delivery delay.
    uint32 deliver_delay = needItemDelay ? sWorld.getConfig(CONFIG_UINT32_MAIL_DELIVERY_DELAY) : 0;

    // will delete item or place to receiver mail list
    SendMailTo(MailReceiver(receiver,receiver_guid), MailSender(MAIL_NORMAL, sender_guid.GetCounter()), MAIL_CHECK_MASK_RETURNED, deliver_delay);
}
/**
 * Sends a mail.
 *
 * @param receiver             The MailReceiver to which this mail is sent.
 * @param sender               The MailSender from which this mail is originated.
 * @param checked              The mask used to specify the mail.
 * @param deliver_delay        The delay after which the mail is delivered in seconds
 */
void MailDraft::SendMailTo(MailReceiver const& receiver, MailSender const& sender, MailCheckMask checked, uint32 deliver_delay)
{
    Player* pReceiver = receiver.GetPlayer();               // can be NULL

    uint32 pReceiverAccount = 0;
    if (!pReceiver)
        pReceiverAccount = sObjectMgr.GetPlayerAccountIdByGUID(receiver.GetPlayerGuid());

    if (!pReceiver && !pReceiverAccount)                    // receiver not exist
    {
        deleteIncludedItems(true);
        return;
    }

    bool has_items = !m_items.empty();

    // generate mail template items for online player, for offline player items will generated at open
    if (pReceiver)
    {
        if (prepareItems(pReceiver))
            has_items = true;
    }

    uint32 mailId = sObjectMgr.GenerateMailID();

    time_t deliver_time = time(NULL) + deliver_delay;

    uint32 expire_delay;
    // auction mail without any items and money (auction sale note) pending 1 hour
    if (sender.GetMailMessageType() == MAIL_AUCTION && m_items.empty() && !m_money)
        expire_delay = HOUR;
    // default case: expire time if COD 3 days, if no COD 30 days
    else
        expire_delay = (m_COD > 0) ? 3 * DAY : 30 * DAY;

    time_t expire_time = deliver_time + expire_delay;

    // Add to DB
    std::string safe_subject = GetSubject();
    CharacterDatabase.escape_string(safe_subject);

    std::string safe_body = GetBody();
    CharacterDatabase.escape_string(safe_body);

    CharacterDatabase.BeginTransaction();
    CharacterDatabase.PExecute("INSERT INTO mail (id,messageType,stationery,mailTemplateId,sender,receiver,subject,body,has_items,expire_time,deliver_time,money,cod,checked) "
        "VALUES ('%u', '%u', '%u', '%u', '%u', '%u', '%s', '%s', '%u', '" UI64FMTD "','" UI64FMTD "', '%u', '%u', '%u')",
        mailId, sender.GetMailMessageType(), sender.GetStationery(), GetMailTemplateId(), sender.GetSenderId(), receiver.GetPlayerGuid().GetCounter(), safe_subject.c_str(), safe_body.c_str(), (has_items ? 1 : 0), (uint64)expire_time, (uint64)deliver_time, m_money, m_COD, checked);

    for(MailItemMap::const_iterator mailItemIter = m_items.begin(); mailItemIter != m_items.end(); ++mailItemIter)
    {
        Item* item = mailItemIter->second;
        CharacterDatabase.PExecute("INSERT INTO mail_items (mail_id,item_guid,item_template,receiver) VALUES ('%u', '%u', '%u','%u')",
            mailId, item->GetGUIDLow(), item->GetEntry(), receiver.GetPlayerGuid().GetCounter());
    }
    CharacterDatabase.CommitTransaction();

    // For online receiver update in game mail status and data
    if (pReceiver)
    {
        pReceiver->AddNewMailDeliverTime(deliver_time);

        Mail *m = new Mail;
        m->messageID = mailId;
        m->mailTemplateId = GetMailTemplateId();
        m->subject = GetSubject();
        m->body = GetBody();
        m->money = GetMoney();
        m->COD = GetCOD();

        for(MailItemMap::const_iterator mailItemIter = m_items.begin(); mailItemIter != m_items.end(); ++mailItemIter)
        {
            Item* item = mailItemIter->second;
            m->AddItem(item->GetGUIDLow(), item->GetEntry());
        }

        m->messageType = sender.GetMailMessageType();
        m->stationery = sender.GetStationery();
        m->sender = sender.GetSenderId();
        m->receiverGuid = receiver.GetPlayerGuid();
        m->expire_time = expire_time;
        m->deliver_time = deliver_time;
        m->checked = checked;
        m->state = MAIL_STATE_UNCHANGED;

        pReceiver->AddMail(m);                           // to insert new mail to beginning of maillist

        if (!m_items.empty())
        {
            for(MailItemMap::iterator mailItemIter = m_items.begin(); mailItemIter != m_items.end(); ++mailItemIter)
                pReceiver->AddMItem(mailItemIter->second);
        }
    }
    else if (!m_items.empty())
        deleteIncludedItems();
}

/**
 * Generate items from template at mails loading (this happens when mail with mail template items send in time when receiver has been offline)
 *
 * @param receiver             reciver of mail
 */

void Mail::prepareTemplateItems( Player* receiver )
{
    if (!mailTemplateId || !items.empty())
        return;

    has_items = true;

    Loot mailLoot;

    // can be empty
    mailLoot.FillLoot(mailTemplateId, LootTemplates_Mail, receiver, true, true);

    CharacterDatabase.BeginTransaction();
    CharacterDatabase.PExecute("UPDATE mail SET has_items = 1 WHERE id = %u", messageID);

    uint32 max_slot = mailLoot.GetMaxSlotInLootFor(receiver);
    for(uint32 i = 0; items.size() < MAX_MAIL_ITEMS && i < max_slot; ++i)
    {
        if (LootItem* lootitem = mailLoot.LootItemInSlot(i, receiver))
        {
            if (Item* item = Item::CreateItem(lootitem->itemid, lootitem->count, receiver))
            {
                item->SaveToDB();

                AddItem(item->GetGUIDLow(), item->GetEntry());

                receiver->AddMItem(item);

                CharacterDatabase.PExecute("INSERT INTO mail_items (mail_id,item_guid,item_template,receiver) VALUES ('%u', '%u', '%u','%u')",
                    messageID, item->GetGUIDLow(), item->GetEntry(), receiver->GetGUIDLow());
            }
        }
    }

    CharacterDatabase.CommitTransaction();
}

/*! @} */
