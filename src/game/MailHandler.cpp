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
 * @file MailHandler.cpp
 * This file contains handlers for mail opcodes.
 *
 */

#include "Mail.h"
#include "Language.h"
#include "Log.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Item.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"

/**
 * Handles the Packet sent by the client when sending a mail.
 *
 * This methods takes the packet sent by the client and performs the following actions:
 * - Checks whether the mail is valid: i.e. can he send the selected items,
 *   does he have enough money, etc.
 * - Creates a MailDraft and adds the needed items, money, cost data.
 * - Sends the mail.
 *
 * Depending on the outcome of the checks performed the player will recieve a different
 * MailResponseResult.
 *
 * @see MailResponseResult
 * @see SendMailResult()
 *
 * @param recv_data the WorldPacket containing the data sent by the client.
 */
void WorldSession::HandleSendMail(WorldPacket & recv_data )
{
    ObjectGuid mailboxGuid;
    uint64 unk3;
    std::string receiver, subject, body;
    uint32 unk1, unk2, money, COD;
    uint8 unk4;
    recv_data >> mailboxGuid;
    recv_data >> receiver;

    recv_data >> subject;

    recv_data >> body;

    recv_data >> unk1;                                      // stationery?
    recv_data >> unk2;                                      // 0x00000000

    uint8 items_count;
    recv_data >> items_count;                               // attached items count

    if (items_count > MAX_MAIL_ITEMS)                       // client limit
    {
        GetPlayer()->SendMailResult(0, MAIL_SEND, MAIL_ERR_TOO_MANY_ATTACHMENTS);
        recv_data.rpos(recv_data.wpos());                   // set to end to avoid warnings spam
        return;
    }

    ObjectGuid itemGuids[MAX_MAIL_ITEMS];

    for(uint8 i = 0; i < items_count; ++i)
    {
        recv_data.read_skip<uint8>();                       // item slot in mail, not used
        recv_data >> itemGuids[i];
    }

    recv_data >> money >> COD;                              // money and cod
    recv_data >> unk3;                                      // const 0
    recv_data >> unk4;                                      // const 0

    // packet read complete, now do check

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailboxGuid, GAMEOBJECT_TYPE_MAILBOX))
        return;

    if (receiver.empty())
        return;

    Player* pl = _player;

    ObjectGuid rc;
    if (normalizePlayerName(receiver))
        rc = sObjectMgr.GetPlayerGuidByName(receiver);

    if (!rc)
    {
        DETAIL_LOG("%s is sending mail to %s (GUID: nonexistent!) with subject %s and body %s includes %u items, %u copper and %u COD copper with unk1 = %u, unk2 = %u",
            pl->GetGuidStr().c_str(), receiver.c_str(), subject.c_str(), body.c_str(), items_count, money, COD, unk1, unk2);
        pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_RECIPIENT_NOT_FOUND);
        return;
    }

    DETAIL_LOG("%s is sending mail to %s with subject %s and body %s includes %u items, %u copper and %u COD copper with unk1 = %u, unk2 = %u",
        pl->GetGuidStr().c_str(), rc.GetString().c_str(), subject.c_str(), body.c_str(), items_count, money, COD, unk1, unk2);

    if (pl->GetObjectGuid() == rc)
    {
        pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_CANNOT_SEND_TO_SELF);
        return;
    }

    uint32 cost = items_count ? 30 * items_count : 30;      // price hardcoded in client

    uint32 reqmoney = cost + money;

    if (pl->GetMoney() < reqmoney)
    {
        pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_ENOUGH_MONEY);
        return;
    }

    Player *receive = sObjectMgr.GetPlayer(rc);

    Team rc_team;
    uint8 mails_count = 0;                                  // do not allow to send to one player more than 100 mails

    if (receive)
    {
        rc_team = receive->GetTeam();
        mails_count = receive->GetMailSize();
    }
    else
    {
        rc_team = sObjectMgr.GetPlayerTeamByGUID(rc);
        if (QueryResult* result = CharacterDatabase.PQuery("SELECT COUNT(*) FROM mail WHERE receiver = '%u'", rc.GetCounter()))
        {
            Field *fields = result->Fetch();
            mails_count = fields[0].GetUInt32();
            delete result;
        }
    }

    // do not allow to have more than 100 mails in mailbox.. mails count is in opcode uint8!!! - so max can be 255..
    if (mails_count > 100)
    {
        pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_RECIPIENT_CAP_REACHED);
        return;
    }

    // check the receiver's Faction...
    if (!sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_MAIL) && pl->GetTeam() != rc_team && GetSecurity() == SEC_PLAYER)
    {
        pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_YOUR_TEAM);
        return;
    }

    uint32 rc_account = receive
        ? receive->GetSession()->GetAccountId()
        : sObjectMgr.GetPlayerAccountIdByGUID(rc);

    Item* items[MAX_MAIL_ITEMS];

    for(uint8 i = 0; i < items_count; ++i)
    {
        if (!itemGuids[i].IsItem())
        {
            pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_MAIL_ATTACHMENT_INVALID);
            return;
        }

        Item* item = pl->GetItemByGuid(itemGuids[i]);

        // prevent sending bag with items (cheat: can be placed in bag after adding equipped empty bag to mail)
        if(!item)
        {
            pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_MAIL_ATTACHMENT_INVALID);
            return;
        }

        if (!item->CanBeTraded(true))
        {
            pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_MAIL_BOUND_ITEM);
            return;
        }

        if (item->IsBoundAccountWide() && item->IsSoulBound() && pl->GetSession()->GetAccountId() != rc_account)
        {
            pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_ARTEFACTS_ONLY_FOR_OWN_CHARACTERS);
            return;
        }

        if ((item->GetProto()->Flags & ITEM_FLAG_CONJURED) || item->GetUInt32Value(ITEM_FIELD_DURATION))
        {
            pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_MAIL_BOUND_ITEM);
            return;
        }

        if (COD && item->HasFlag(ITEM_FIELD_FLAGS, ITEM_DYNFLAG_WRAPPED))
        {
            pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_CANT_SEND_WRAPPED_COD);
            return;
        }

        items[i] = item;
    }

    pl->SendMailResult(0, MAIL_SEND, MAIL_OK);

    pl->ModifyMoney( -int32(reqmoney) );
    pl->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL, cost);

    bool needItemDelay = false;

    MailDraft draft(subject, body);

    if (items_count > 0 || money > 0)
    {
        if (items_count > 0)
        {
            for(uint8 i = 0; i < items_count; ++i)
            {
                Item* item = items[i];
                if (GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
                {
                    sLog.outCommand(GetAccountId(), "GM %s (Account: %u) mail item: %s (Entry: %u Count: %u) to player: %s (Account: %u)",
                        GetPlayerName(), GetAccountId(), item->GetProto()->Name1, item->GetEntry(), item->GetCount(), receiver.c_str(), rc_account);
                }

                pl->MoveItemFromInventory(items[i]->GetBagSlot(), item->GetSlot(), true);
                CharacterDatabase.BeginTransaction();
                item->DeleteFromInventoryDB();     // deletes item from character's inventory
                item->SaveToDB();                  // recursive and not have transaction guard into self, item not in inventory and can be save standalone
                // owner in data will set at mail receive and item extracting
                CharacterDatabase.PExecute("UPDATE item_instance SET owner_guid = '%u' WHERE guid='%u'", rc.GetCounter(), item->GetGUIDLow());
                CharacterDatabase.CommitTransaction();

                draft.AddItem(item);
            }

            // if item send to character at another account, then apply item delivery delay
            needItemDelay = pl->GetSession()->GetAccountId() != rc_account;
        }

        if (money > 0 &&  GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
        {
            sLog.outCommand(GetAccountId(),"GM %s (Account: %u) mail money: %u to player: %s (Account: %u)",
                GetPlayerName(), GetAccountId(), money, receiver.c_str(), rc_account);
        }
    }

    // If theres is an item, there is a one hour delivery delay if sent to another account's character.
    uint32 deliver_delay = needItemDelay ? sWorld.getConfig(CONFIG_UINT32_MAIL_DELIVERY_DELAY) : 0;

    // will delete item or place to receiver mail list
    draft
        .SetMoney(money)
        .SetCOD(COD)
        .SendMailTo(MailReceiver(receive, rc), pl, body.empty() ? MAIL_CHECK_MASK_COPIED : MAIL_CHECK_MASK_HAS_BODY, deliver_delay);

    CharacterDatabase.BeginTransaction();
    pl->SaveInventoryAndGoldToDB();
    CharacterDatabase.CommitTransaction();
}

/**
 * Handles the Packet sent by the client when reading a mail.
 *
 * This method is called when a client reads a mail that was previously unread.
 * It will add the MAIL_CHECK_MASK_READ flag to the mail being read.
 *
 * @see MailCheckMask
 *
 * @param recv_data the packet containing information about the mail the player read.
 *
 */
void WorldSession::HandleMailMarkAsRead(WorldPacket & recv_data )
{
    ObjectGuid mailboxGuid;
    uint32 mailId;
    recv_data >> mailboxGuid;
    recv_data >> mailId;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailboxGuid, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player *pl = _player;

    if (Mail *m = pl->GetMail(mailId))
    {
        if (pl->unReadMails)
            --pl->unReadMails;
        m->checked = m->checked | MAIL_CHECK_MASK_READ;
        pl->m_mailsUpdated = true;
        m->state = MAIL_STATE_CHANGED;
    }
}

/**
 * Handles the Packet sent by the client when deleting a mail.
 *
 * This method is called when a client deletes a mail in his mailbox.
 *
 * @param recv_data The packet containing information about the mail being deleted.
 *
 */
void WorldSession::HandleMailDelete(WorldPacket & recv_data )
{
    ObjectGuid mailboxGuid;
    uint32 mailId;
    recv_data >> mailboxGuid;
    recv_data >> mailId;
    recv_data.read_skip<uint32>();                          // mailTemplateId

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailboxGuid, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* pl = _player;
    pl->m_mailsUpdated = true;

    if(Mail *m = pl->GetMail(mailId))
    {
        // delete shouldn't show up for COD mails
        if (m->COD)
        {
            pl->SendMailResult(mailId, MAIL_DELETED, MAIL_ERR_INTERNAL_ERROR);
            return;
        }

        m->state = MAIL_STATE_DELETED;
    }
    pl->SendMailResult(mailId, MAIL_DELETED, MAIL_OK);
}
/**
 * Handles the Packet sent by the client when returning a mail to sender.
 * This method is called when a player chooses to return a mail to its sender.
 * It will create a new MailDraft and add the items, money, etc. associated with the mail
 * and then send the mail to the original sender.
 *
 * @param recv_data The packet containing information about the mail being returned.
 *
 */
void WorldSession::HandleMailReturnToSender(WorldPacket & recv_data )
{
    ObjectGuid mailboxGuid;
    uint32 mailId;
    recv_data >> mailboxGuid;
    recv_data >> mailId;
    recv_data.read_skip<uint64>();                          // original sender GUID for return to, not used

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailboxGuid, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player *pl = _player;
    Mail *m = pl->GetMail(mailId);
    if(!m || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL))
    {
        pl->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    //we can return mail now
    //so firstly delete the old one
    CharacterDatabase.BeginTransaction();
    CharacterDatabase.PExecute("DELETE FROM mail WHERE id = '%u'", mailId);
                                                            // needed?
    CharacterDatabase.PExecute("DELETE FROM mail_items WHERE mail_id = '%u'", mailId);
    CharacterDatabase.CommitTransaction();
    pl->RemoveMail(mailId);

    // send back only to existing players and simple drop for other cases
    if (m->messageType == MAIL_NORMAL && m->sender)
    {
        MailDraft draft;
        if (m->mailTemplateId)
            draft.SetMailTemplate(m->mailTemplateId, false);// items already included
        else
            draft.SetSubjectAndBody(m->subject, m->body);

        if(m->HasItems())
        {
            for(MailItemInfoVec::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
            {
                if(Item *item = pl->GetMItem(itr2->item_guid))
                    draft.AddItem(item);

                pl->RemoveMItem(itr2->item_guid);
            }
        }

        draft.SetMoney(m->money).SendReturnToSender(GetAccountId(), m->receiverGuid, ObjectGuid(HIGHGUID_PLAYER, m->sender));
    }

    delete m;                                               // we can deallocate old mail
    pl->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, MAIL_OK);
}

/**
 * Handles the packet sent by the client when taking an item from the mail.
 */
void WorldSession::HandleMailTakeItem(WorldPacket & recv_data )
{
    ObjectGuid mailboxGuid;
    uint32 mailId;
    uint32 itemId;
    recv_data >> mailboxGuid;
    recv_data >> mailId;
    recv_data >> itemId;                                    // item guid low

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailboxGuid, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* pl = _player;

    Mail* m = pl->GetMail(mailId);
    if(!m || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL))
    {
        pl->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    // prevent cheating with skip client money check
    if(pl->GetMoney() < m->COD)
    {
        pl->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_NOT_ENOUGH_MONEY);
        return;
    }

    Item *it = pl->GetMItem(itemId);

    ItemPosCountVec dest;
    uint8 msg = _player->CanStoreItem( NULL_BAG, NULL_SLOT, dest, it, false );
    if( msg == EQUIP_ERR_OK )
    {
        m->RemoveItem(itemId);
        m->removedItems.push_back(itemId);

        if (m->COD > 0)                                     // if there is COD, take COD money from player and send them to sender by mail
        {
            ObjectGuid sender_guid = ObjectGuid(HIGHGUID_PLAYER, m->sender);
            Player *sender = sObjectMgr.GetPlayer(sender_guid);

            uint32 sender_accId = 0;

            if( GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE) )
            {
                std::string sender_name;
                if(sender)
                {
                    sender_accId = sender->GetSession()->GetAccountId();
                    sender_name = sender->GetName();
                }
                else
                {
                    // can be calculated early
                    sender_accId = sObjectMgr.GetPlayerAccountIdByGUID(sender_guid);

                    if(!sObjectMgr.GetPlayerNameByGUID(sender_guid, sender_name))
                        sender_name = sObjectMgr.GetMangosStringForDBCLocale(LANG_UNKNOWN);
                }
                sLog.outCommand(GetAccountId(), "GM %s (Account: %u) receive mail item: %s (Entry: %u Count: %u) and send COD money: %u to player: %s (Account: %u)",
                    GetPlayerName(), GetAccountId(), it->GetProto()->Name1, it->GetEntry(), it->GetCount(), m->COD, sender_name.c_str(), sender_accId);
            }
            else if(!sender)
                sender_accId = sObjectMgr.GetPlayerAccountIdByGUID(sender_guid);

            // check player existence
            if(sender || sender_accId)
            {
                MailDraft(m->subject, "")
                    .SetMoney(m->COD)
                    .SendMailTo(MailReceiver(sender, sender_guid), _player, MAIL_CHECK_MASK_COD_PAYMENT);
            }

            pl->ModifyMoney( -int32(m->COD) );
        }
        m->COD = 0;
        m->state = MAIL_STATE_CHANGED;
        pl->m_mailsUpdated = true;
        pl->RemoveMItem(it->GetGUIDLow());

        uint32 count = it->GetCount();                      // save counts before store and possible merge with deleting
        pl->MoveItemToInventory(dest, it, true);

        CharacterDatabase.BeginTransaction();
        pl->SaveInventoryAndGoldToDB();
        pl->_SaveMail();
        CharacterDatabase.CommitTransaction();

        pl->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_OK, 0, itemId, count);
    }
    else
        pl->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_EQUIP_ERROR, msg);
}
/**
 * Handles the packet sent by the client when taking money from the mail.
 */
void WorldSession::HandleMailTakeMoney(WorldPacket & recv_data )
{
    ObjectGuid mailboxGuid;
    uint32 mailId;
    recv_data >> mailboxGuid;
    recv_data >> mailId;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailboxGuid, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player *pl = _player;

    Mail* m = pl->GetMail(mailId);
    if(!m || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL))
    {
        pl->SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    pl->SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_OK);

    pl->ModifyMoney(m->money);
    m->money = 0;
    m->state = MAIL_STATE_CHANGED;
    pl->m_mailsUpdated = true;

    // save money and mail to prevent cheating
    CharacterDatabase.BeginTransaction();
    pl->SaveGoldToDB();
    pl->_SaveMail();
    CharacterDatabase.CommitTransaction();
}

/**
 * Handles the packet sent by the client when requesting the current mail list.
 * It will send a list of all available mails in the players mailbox to the client.
 */
void WorldSession::HandleGetMailList(WorldPacket & recv_data )
{
    ObjectGuid mailboxGuid;
    recv_data >> mailboxGuid;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailboxGuid, GAMEOBJECT_TYPE_MAILBOX))
        return;

    // client can't work with packets > max int16 value
    const uint32 maxPacketSize = 32767;

    uint32 mailsCount = 0;                                  // send to client mails amount
    uint32 realCount = 0;                                   // real mails amount

    WorldPacket data(SMSG_MAIL_LIST_RESULT, 200);           // guess size
    data << uint32(0);                                      // real mail's count
    data << uint8(0);                                       // mail's count
    time_t cur_time = time(NULL);

    for(PlayerMails::iterator itr = _player->GetMailBegin(); itr != _player->GetMailEnd(); ++itr)
    {
        // packet send mail count as uint8, prevent overflow
        if(mailsCount >= 254)
        {
            realCount += 1;
            continue;
        }

        // skip deleted or not delivered (deliver delay not expired) mails
        if ((*itr)->state == MAIL_STATE_DELETED || cur_time < (*itr)->deliver_time)
            continue;

        uint8 item_count = (*itr)->items.size();            // max count is MAX_MAIL_ITEMS (12)

        size_t next_mail_size = 2+4+1+((*itr)->messageType == MAIL_NORMAL ? 8 : 4)+4*8+((*itr)->subject.size()+1)+((*itr)->body.size()+1)+1+item_count*(1+4+4+7*3*4+4+4+4+4+4+4+1);

        if(data.wpos()+next_mail_size > maxPacketSize)
        {
            realCount += 1;
            continue;
        }

        data << uint16(next_mail_size);                     // Message size
        data << uint32((*itr)->messageID);                  // Message ID
        data << uint8((*itr)->messageType);                 // Message Type

        switch((*itr)->messageType)
        {
            case MAIL_NORMAL:                               // sender guid
                data << ObjectGuid(HIGHGUID_PLAYER, (*itr)->sender);
                break;
            case MAIL_CREATURE:
            case MAIL_GAMEOBJECT:
            case MAIL_AUCTION:
                data << uint32((*itr)->sender);             // creature/gameobject entry, auction id
                break;
            case MAIL_ITEM:                                 // item entry (?) sender = "Unknown", NYI
                data << uint32(0);                          // item entry
                break;
        }

        data << uint32((*itr)->COD);                        // COD
        data << uint32(0);                                  // unknown, probably changed in 3.3.3
        data << uint32((*itr)->stationery);                 // stationery (Stationery.dbc)
        data << uint32((*itr)->money);                      // copper
        data << uint32((*itr)->checked);                    // flags
        data << float(float((*itr)->expire_time - time(NULL)) / float(DAY));// Time
        data << uint32((*itr)->mailTemplateId);             // mail template (MailTemplate.dbc)
        data << (*itr)->subject;                            // Subject string - once 00, when mail type = 3, max 256
        data << (*itr)->body;                               // message? max 8000

        data << uint8(item_count);                          // client limit is 0x10

        for(uint8 i = 0; i < item_count; ++i)
        {
            Item *item = _player->GetMItem((*itr)->items[i].item_guid);
            // item index (0-6?)
            data << uint8(i);
            // item guid low?
            data << uint32(item ? item->GetGUIDLow() : 0);
            // entry
            data << uint32(item ? item->GetEntry() : 0);
            for(uint8 j = 0; j < MAX_INSPECTED_ENCHANTMENT_SLOT; ++j)
            {
                // unsure
                data << uint32(item ? item->GetEnchantmentCharges((EnchantmentSlot)j) : 0);
                // unsure
                data << uint32(item ? item->GetEnchantmentDuration((EnchantmentSlot)j) : 0);
                // unsure
                data << uint32(item ? item->GetEnchantmentId((EnchantmentSlot)j) : 0);
            }
            // can be negative
            data << uint32(item ? item->GetItemRandomPropertyId() : 0);
            // unk
            data << uint32(item ? item->GetItemSuffixFactor() : 0);
            // stack count
            data << uint32(item ? item->GetCount() : 0);
            // charges
            data << uint32(item ? item->GetSpellCharges() : 0);
            // durability
            data << uint32(item ? item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY) : 0);
            // durability
            data << uint32(item ? item->GetUInt32Value(ITEM_FIELD_DURABILITY) : 0);
            // unknown wotlk
            data << uint8(0);
        }

        mailsCount += 1;
        realCount += 1;
    }

    data.put<uint32>(0, realCount);                         // this will display warning about undelivered mail to player if realCount > mailsCount
    data.put<uint8>(4, mailsCount);                         // set real send mails to client
    SendPacket(&data);

    // recalculate m_nextMailDelivereTime and unReadMails
    _player->UpdateNextMailTimeAndUnreads();
}

/**
 * Handles the packet sent by the client when he copies the body a mail to his inventory.
 *
 * When a player copies the body of a mail to his inventory this method is called. It will create
 * a new item with the text of the mail and store it in the players inventory (if possible).
 *
 */
void WorldSession::HandleMailCreateTextItem(WorldPacket & recv_data )
{
    ObjectGuid mailboxGuid;
    uint32 mailId;

    recv_data >> mailboxGuid;
    recv_data >> mailId;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailboxGuid, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player *pl = _player;

    Mail* m = pl->GetMail(mailId);
    if (!m || (m->body.empty() && !m->mailTemplateId) || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL))
    {
        pl->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    Item *bodyItem = new Item;                              // This is not bag and then can be used new Item.
    if (!bodyItem->Create(sObjectMgr.GenerateItemLowGuid(), MAIL_BODY_ITEM_TEMPLATE, pl))
    {
        delete bodyItem;
        return;
    }

    // in mail template case we need create new item text
    if (m->mailTemplateId)
    {
        MailTemplateEntry const* mailTemplateEntry = sMailTemplateStore.LookupEntry(m->mailTemplateId);
        if (!mailTemplateEntry)
        {
            pl->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
            return;
        }

        bodyItem->SetText(mailTemplateEntry->content[GetSessionDbcLocale()]);
    }
    else
        bodyItem->SetText(m->body);

    bodyItem->SetGuidValue(ITEM_FIELD_CREATOR, ObjectGuid(HIGHGUID_PLAYER, m->sender));
    bodyItem->SetFlag(ITEM_FIELD_FLAGS, ITEM_DYNFLAG_READABLE | ITEM_DYNFLAG_UNK15 | ITEM_DYNFLAG_UNK16);


    DETAIL_LOG("HandleMailCreateTextItem mailid=%u", mailId);

    ItemPosCountVec dest;
    uint8 msg = _player->CanStoreItem( NULL_BAG, NULL_SLOT, dest, bodyItem, false );
    if( msg == EQUIP_ERR_OK )
    {
        m->checked = m->checked | MAIL_CHECK_MASK_COPIED;
        m->state = MAIL_STATE_CHANGED;
        pl->m_mailsUpdated = true;

        pl->StoreItem(dest, bodyItem, true);
        pl->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_OK);
    }
    else
    {
        pl->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_EQUIP_ERROR, msg);
        delete bodyItem;
    }
}

/**
 * No idea when this is called.
 */
void WorldSession::HandleQueryNextMailTime(WorldPacket & /**recv_data*/ )
{
    WorldPacket data(MSG_QUERY_NEXT_MAIL_TIME, 8);

    if( _player->unReadMails > 0 )
    {
        data << uint32(0);                                  // float
        data << uint32(0);                                  // count

        uint32 count = 0;
        time_t now = time(NULL);
        for(PlayerMails::iterator itr = _player->GetMailBegin(); itr != _player->GetMailEnd(); ++itr)
        {
            Mail *m = (*itr);
            // must be not checked yet
            if(m->checked & MAIL_CHECK_MASK_READ)
                continue;

            // and already delivered
            if(now < m->deliver_time)
                continue;

            data << ObjectGuid(HIGHGUID_PLAYER, m->sender); // sender guid

            switch(m->messageType)
            {
                case MAIL_AUCTION:
                    data << uint32(m->sender);              // auction house id
                    data << uint32(MAIL_AUCTION);           // message type
                    break;
                default:
                    data << uint32(0);
                    data << uint32(0);
                    break;
            }

            data << uint32(m->stationery);
            data << uint32(0xC6000000);                     // float unk, time or something

            ++count;
            if(count == 2)                                  // do not display more than 2 mails
                break;
        }
        data.put<uint32>(4, count);
    }
    else
    {
        data << uint32(0xC7A8C000);
        data << uint32(0x00000000);
    }
    SendPacket(&data);
}

/*! @} */
