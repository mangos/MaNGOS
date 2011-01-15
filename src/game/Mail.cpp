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
 * This file contains the the code needed for MaNGOS to handle mails.
 *
 */

#include "Mail.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "UpdateMask.h"
#include "Unit.h"
#include "Language.h"
#include "AuctionHouseBot.h"
#include "DBCStores.h"
#include "Item.h"
#include "AuctionHouseMgr.h"

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
    uint64 mailbox, unk3;
    std::string receiver, subject, body;
    uint32 unk1, unk2, money, COD;
    uint8 unk4;
    recv_data >> mailbox;
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

    uint64 itemGUIDs[MAX_MAIL_ITEMS];

    for(uint8 i = 0; i < items_count; ++i)
    {
        recv_data.read_skip<uint8>();                       // item slot in mail, not used
        recv_data >> itemGUIDs[i];
    }

    recv_data >> money >> COD;                              // money and cod
    recv_data >> unk3;                                      // const 0
    recv_data >> unk4;                                      // const 0

    // packet read complete, now do check

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    if (receiver.empty())
        return;

    Player* pl = _player;

    ObjectGuid rc;
    if (normalizePlayerName(receiver))
        rc = sObjectMgr.GetPlayerGUIDByName(receiver);

    if (rc.IsEmpty())
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
        if (!itemGUIDs[i])
        {
            pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_MAIL_ATTACHMENT_INVALID);
            return;
        }

        Item* item = pl->GetItemByGuid(itemGUIDs[i]);

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

        if (item->IsBoundAccountWide() /*&& item->IsSoulBound() */&& pl->GetSession()->GetAccountId() != rc_account)
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
    uint64 mailbox;
    uint32 mailId;
    recv_data >> mailbox;
    recv_data >> mailId;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
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
    uint64 mailbox;
    uint32 mailId;
    recv_data >> mailbox;
    recv_data >> mailId;
    recv_data.read_skip<uint32>();                          // mailTemplateId

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
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
    uint64 mailbox;
    uint32 mailId;
    recv_data >> mailbox;
    recv_data >> mailId;
    recv_data.read_skip<uint64>();                          // original sender GUID for return to, not used

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
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
    uint64 mailbox;
    uint32 mailId;
    uint32 itemId;
    recv_data >> mailbox;
    recv_data >> mailId;
    recv_data >> itemId;                                    // item guid low

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
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
    uint64 mailbox;
    uint32 mailId;
    recv_data >> mailbox;
    recv_data >> mailId;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
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
    uint64 mailbox;
    recv_data >> mailbox;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
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
        data << float(((*itr)->expire_time-time(NULL))/DAY);// Time
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
    uint64 mailbox;
    uint32 mailId;

    recv_data >> mailbox;
    recv_data >> mailId;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player *pl = _player;

    Mail* m = pl->GetMail(mailId);
    if (!m || (m->body.empty() && !m->mailTemplateId) || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL))
    {
        pl->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    Item *bodyItem = new Item;                              // This is not bag and then can be used new Item.
    if(!bodyItem->Create(sObjectMgr.GenerateLowGuid(HIGHGUID_ITEM), MAIL_BODY_ITEM_TEMPLATE, pl))
    {
        delete bodyItem;
        return;
    }

    // in mail template case we need create new item text
    if(m->mailTemplateId)
    {
        MailTemplateEntry const* mailTemplateEntry = sMailTemplateStore.LookupEntry(m->mailTemplateId);
        if(!mailTemplateEntry)
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
void MailDraft::prepareItems(Player* receiver)
{
    if (!m_mailTemplateId || !m_mailTemplateItemsNeed)
        return;

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
    if(!receiver)
        rc_account = sObjectMgr.GetPlayerAccountIdByGUID(receiver_guid);

    if(!receiver && !rc_account)                            // sender not exist
    {
        deleteIncludedItems(true);
        return;
    }

    // Hack - if aucbot functional, drop returned to this mail anywhere
    if (sender_guid == auctionbot.GetAHBplayerGUID())
    {
        deleteIncludedItems(true);
        return;
    }

    // prepare mail and send in other case
    bool needItemDelay = false;

    if(!m_items.empty())
    {
        // if item send to character at another account, then apply item delivery delay
        needItemDelay = sender_acc != rc_account;

        // set owner to new receiver (to prevent delete item with sender char deleting)
        CharacterDatabase.BeginTransaction();
        for(MailItemMap::iterator mailItemIter = m_items.begin(); mailItemIter != m_items.end(); ++mailItemIter)
        {
            Item* item = mailItemIter->second;
            item->SaveToDB();                      // item not in inventory and can be save standalone
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
    // Hack - if aucbot functional, drop sended to this mail anywhere
    if (receiver.GetPlayerGuid() == auctionbot.GetAHBplayerGUID())
    {
        if (!m_items.empty())
        {
            deleteIncludedItems(true);
        }
        return;
    }

    Player* pReceiver = receiver.GetPlayer();               // can be NULL

    if (pReceiver)
        prepareItems(pReceiver);                            // generate mail template items


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
        mailId, sender.GetMailMessageType(), sender.GetStationery(), GetMailTemplateId(), sender.GetSenderId(), receiver.GetPlayerGuid().GetCounter(), safe_subject.c_str(), safe_body.c_str(), (m_items.empty() ? 0 : 1), (uint64)expire_time, (uint64)deliver_time, m_money, m_COD, checked);

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
/*! @} */
