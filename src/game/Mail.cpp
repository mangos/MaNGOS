/*
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "Mail.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "UpdateMask.h"
#include "Unit.h"
#include "Language.h"
#include "DBCStores.h"
#include "BattleGroundMgr.h"
#include "Item.h"

enum MailShowFlags
{
    MAIL_SHOW_UNK0    = 0x0001,
    MAIL_SHOW_DELETE  = 0x0002,                             // forced show delete button instead return button
    MAIL_SHOW_AUCTION = 0x0004,                             // from old comment
    MAIL_SHOW_UNK2    = 0x0008,                             // unknown, COD will be shown even without that flag
    MAIL_SHOW_RETURN  = 0x0010,
};


void MailItemsInfo::deleteIncludedItems( bool inDB /*= false*/ )
{
    for(MailItemMap::iterator mailItemIter = begin(); mailItemIter != end(); ++mailItemIter)
    {
        Item* item = mailItemIter->second;

        if(inDB)
            CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid='%u'", item->GetGUIDLow());

        delete item;
    }

    i_MailItemMap.clear();
}

void MailItemsInfo::AddItem( Item *item )
{
    i_MailItemMap[item->GetGUIDLow()] = item;
}

void Mail::AddAllItems( MailItemsInfo& pMailItemsInfo )
{
    for(MailItemMap::iterator mailItemIter = pMailItemsInfo.begin(); mailItemIter != pMailItemsInfo.end(); ++mailItemIter)
    {
        Item* item = mailItemIter->second;
        AddItem(item->GetGUIDLow(), item->GetEntry());
    }
}

void WorldSession::HandleSendMail(WorldPacket & recv_data )
{
    uint64 mailbox, unk3;
    std::string receiver, subject, body;
    uint32 unk1, unk2, money, COD;
    uint8 unk4;
    recv_data >> mailbox;
    recv_data >> receiver;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

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

    if (receiver.empty())
        return;

    Player* pl = _player;

    uint64 rc = 0;
    if (normalizePlayerName(receiver))
        rc = objmgr.GetPlayerGUIDByName(receiver);

    if (!rc)
    {
        sLog.outDetail("Player %u is sending mail to %s (GUID: not existed!) with subject %s and body %s includes %u items, %u copper and %u COD copper with unk1 = %u, unk2 = %u",
            pl->GetGUIDLow(), receiver.c_str(), subject.c_str(), body.c_str(), items_count, money, COD, unk1, unk2);
        pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_RECIPIENT_NOT_FOUND);
        return;
    }

    sLog.outDetail("Player %u is sending mail to %s (GUID: %u) with subject %s and body %s includes %u items, %u copper and %u COD copper with unk1 = %u, unk2 = %u", pl->GetGUIDLow(), receiver.c_str(), GUID_LOPART(rc), subject.c_str(), body.c_str(), items_count, money, COD, unk1, unk2);

    if (pl->GetGUID() == rc)
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

    Player *receive = objmgr.GetPlayer(rc);

    uint32 rc_team = 0;
    uint8 mails_count = 0;                                  // do not allow to send to one player more than 100 mails

    if (receive)
    {
        rc_team = receive->GetTeam();
        mails_count = receive->GetMailSize();
    }
    else
    {
        rc_team = objmgr.GetPlayerTeamByGUID(rc);
        if (QueryResult* result = CharacterDatabase.PQuery("SELECT COUNT(*) FROM mail WHERE receiver = '%u'", GUID_LOPART(rc)))
        {
            Field *fields = result->Fetch();
            mails_count = fields[0].GetUInt32();
            delete result;
        }
    }

    //do not allow to have more than 100 mails in mailbox.. mails count is in opcode uint8!!! - so max can be 255..
    if (mails_count > 100)
    {
        pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_RECIPIENT_CAP_REACHED);
        return;
    }

    // check the receiver's Faction...
    if (!sWorld.getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_MAIL) && pl->GetTeam() != rc_team && GetSecurity() == SEC_PLAYER)
    {
        pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_YOUR_TEAM);
        return;
    }

    uint32 rc_account = receive
        ? receive->GetSession()->GetAccountId()
        : objmgr.GetPlayerAccountIdByGUID(rc);

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
        if (!item)
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

        if (item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_CONJURED) || item->GetUInt32Value(ITEM_FIELD_DURATION))
        {
            pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_MAIL_BOUND_ITEM);
            return;
        }

        if (COD && item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_WRAPPED))
        {
            pl->SendMailResult(0, MAIL_SEND, MAIL_ERR_CANT_SEND_WRAPPED_COD);
            return;
        }

        items[i] = item;
    }

    pl->SendMailResult(0, MAIL_SEND, MAIL_OK);

    uint32 itemTextId = !body.empty() ? objmgr.CreateItemText( body ) : 0;

    pl->ModifyMoney( -int32(reqmoney) );
    pl->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL, cost);

    bool needItemDelay = false;

    MailItemsInfo mi;

    if (items_count > 0 || money > 0)
    {
        if (items_count > 0)
        {
            for(uint8 i = 0; i < items_count; ++i)
            {
                Item* item = items[i];
                if (GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_GM_LOG_TRADE))
                {
                    sLog.outCommand(GetAccountId(), "GM %s (Account: %u) mail item: %s (Entry: %u Count: %u) to player: %s (Account: %u)",
                        GetPlayerName(), GetAccountId(), item->GetProto()->Name1, item->GetEntry(), item->GetCount(), receiver.c_str(), rc_account);
                }

                pl->MoveItemFromInventory(items[i]->GetBagSlot(), item->GetSlot(), true);
                CharacterDatabase.BeginTransaction();
                item->DeleteFromInventoryDB();     // deletes item from character's inventory
                item->SaveToDB();                  // recursive and not have transaction guard into self, item not in inventory and can be save standalone
                // owner in data will set at mail receive and item extracting
                CharacterDatabase.PExecute("UPDATE item_instance SET owner_guid = '%u' WHERE guid='%u'", GUID_LOPART(rc), item->GetGUIDLow());
                CharacterDatabase.CommitTransaction();

                mi.AddItem(item);
            }

            // if item send to character at another account, then apply item delivery delay
            needItemDelay = pl->GetSession()->GetAccountId() != rc_account;
        }

        if (money > 0 &&  GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_GM_LOG_TRADE))
        {
            sLog.outCommand(GetAccountId(),"GM %s (Account: %u) mail money: %u to player: %s (Account: %u)",
                GetPlayerName(), GetAccountId(), money, receiver.c_str(), rc_account);
        }
    }

    // If theres is an item, there is a one hour delivery delay if sent to another account's character.
    uint32 deliver_delay = needItemDelay ? sWorld.getConfig(CONFIG_MAIL_DELIVERY_DELAY) : 0;

    // will delete item or place to receiver mail list
    WorldSession::SendMailTo(receive, MAIL_NORMAL, MAIL_STATIONERY_NORMAL, pl->GetGUIDLow(), GUID_LOPART(rc), subject, itemTextId, &mi, money, COD, MAIL_CHECK_MASK_NONE, deliver_delay);

    CharacterDatabase.BeginTransaction();
    pl->SaveInventoryAndGoldToDB();
    CharacterDatabase.CommitTransaction();
}

//called when mail is read
void WorldSession::HandleMailMarkAsRead(WorldPacket & recv_data )
{
    uint64 mailbox;
    uint32 mailId;
    recv_data >> mailbox;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    recv_data >> mailId;
    Player *pl = _player;
    Mail *m = pl->GetMail(mailId);
    if (m)
    {
        if (pl->unReadMails)
            --pl->unReadMails;
        m->checked = m->checked | MAIL_CHECK_MASK_READ;
        // m->expire_time = time(NULL) + (30 * DAY);        // Expire time do not change at reading mail
        pl->m_mailsUpdated = true;
        m->state = MAIL_STATE_CHANGED;
    }
}

//called when client deletes mail
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
    Mail *m = pl->GetMail(mailId);
    if(m)
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

void WorldSession::HandleMailReturnToSender(WorldPacket & recv_data )
{
    uint64 mailbox;
    uint32 mailId;
    recv_data >> mailbox;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    recv_data >> mailId;
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

    MailItemsInfo mi;

    if(m->HasItems())
    {
        for(std::vector<MailItemInfo>::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
        {
            Item *item = pl->GetMItem(itr2->item_guid);
            if(item)
                mi.AddItem(item);
            else
            {
                //WTF?
            }

            pl->RemoveMItem(itr2->item_guid);
        }
    }

    SendReturnToSender(MAIL_NORMAL, GetAccountId(), m->receiver, m->sender, m->subject, m->itemTextId, &mi, m->money, m->mailTemplateId);

    delete m;                                               // we can deallocate old mail
    pl->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, MAIL_OK);
}

void WorldSession::SendReturnToSender(uint8 messageType, uint32 sender_acc, uint32 sender_guid, uint32 receiver_guid, const std::string& subject, uint32 itemTextId, MailItemsInfo *mi, uint32 money, uint16 mailTemplateId )
{
    if(messageType != MAIL_NORMAL)                          // return only to players
    {
        mi->deleteIncludedItems(true);
        return;
    }

    Player *receiver = objmgr.GetPlayer(MAKE_NEW_GUID(receiver_guid, 0, HIGHGUID_PLAYER));

    uint32 rc_account = 0;
    if(!receiver)
        rc_account = objmgr.GetPlayerAccountIdByGUID(MAKE_NEW_GUID(receiver_guid, 0, HIGHGUID_PLAYER));

    if(!receiver && !rc_account)                            // sender not exist
    {
        mi->deleteIncludedItems(true);
        return;
    }

    // prepare mail and send in other case
    bool needItemDelay = false;

    if(mi && !mi->empty())
    {
        // if item send to character at another account, then apply item delivery delay
        needItemDelay = sender_acc != rc_account;

        // set owner to new receiver (to prevent delete item with sender char deleting)
        CharacterDatabase.BeginTransaction();
        for(MailItemMap::iterator mailItemIter = mi->begin(); mailItemIter != mi->end(); ++mailItemIter)
        {
            Item* item = mailItemIter->second;
            item->SaveToDB();                      // item not in inventory and can be save standalone
            // owner in data will set at mail receive and item extracting
            CharacterDatabase.PExecute("UPDATE item_instance SET owner_guid = '%u' WHERE guid='%u'", receiver_guid, item->GetGUIDLow());
        }
        CharacterDatabase.CommitTransaction();
    }

    // If theres is an item, there is a one hour delivery delay.
    uint32 deliver_delay = needItemDelay ? sWorld.getConfig(CONFIG_MAIL_DELIVERY_DELAY) : 0;

    // will delete item or place to receiver mail list
    WorldSession::SendMailTo(receiver, MAIL_NORMAL, MAIL_STATIONERY_NORMAL, sender_guid, receiver_guid, subject, itemTextId, mi, money, 0, MAIL_CHECK_MASK_RETURNED, deliver_delay, mailTemplateId);
}

//called when player takes item attached in mail
void WorldSession::HandleMailTakeItem(WorldPacket & recv_data )
{
    uint64 mailbox;
    uint32 mailId;
    uint32 itemId;
    recv_data >> mailbox;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    recv_data >> mailId;
    recv_data >> itemId;                                    // item guid low?
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
            uint64 sender_guid = MAKE_NEW_GUID(m->sender, 0, HIGHGUID_PLAYER);
            Player *receive = objmgr.GetPlayer(sender_guid);

            uint32 sender_accId = 0;

            if( GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_GM_LOG_TRADE) )
            {
                std::string sender_name;
                if(receive)
                {
                    sender_accId = receive->GetSession()->GetAccountId();
                    sender_name = receive->GetName();
                }
                else
                {
                    // can be calculated early
                    sender_accId = objmgr.GetPlayerAccountIdByGUID(sender_guid);

                    if(!objmgr.GetPlayerNameByGUID(sender_guid, sender_name))
                        sender_name = objmgr.GetMangosStringForDBCLocale(LANG_UNKNOWN);
                }
                sLog.outCommand(GetAccountId(), "GM %s (Account: %u) receive mail item: %s (Entry: %u Count: %u) and send COD money: %u to player: %s (Account: %u)",
                    GetPlayerName(), GetAccountId(), it->GetProto()->Name1, it->GetEntry(), it->GetCount(), m->COD, sender_name.c_str(), sender_accId);
            }
            else if(!receive)
                sender_accId = objmgr.GetPlayerAccountIdByGUID(sender_guid);

            // check player existence
            if(receive || sender_accId)
            {
                WorldSession::SendMailTo(receive, MAIL_NORMAL, MAIL_STATIONERY_NORMAL, m->receiver, m->sender, m->subject, 0, NULL, m->COD, 0, MAIL_CHECK_MASK_COD_PAYMENT);
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

//called when player lists his received mails
void WorldSession::HandleGetMailList(WorldPacket & recv_data )
{
    uint64 mailbox;
    recv_data >> mailbox;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    // load players mails, and mailed items
    if(!_player->m_mailsLoaded)
        _player->_LoadMail();

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
        // skip deleted or not delivered (deliver delay not expired) mails
        if ((*itr)->state == MAIL_STATE_DELETED || cur_time < (*itr)->deliver_time)
            continue;

        uint8 item_count = (*itr)->items.size();            // max count is MAX_MAIL_ITEMS (12)

        size_t next_mail_size = 2+4+1+8+4*8+((*itr)->subject.size()+1)+1+item_count*(1+4+4+6*3*4+4+4+1+4+4+4);

        if(data.wpos()+next_mail_size > maxPacketSize)
        {
            realCount += 1;
            continue;
        }

        uint32 show_flags = 0;
        if ((*itr)->messageType != MAIL_NORMAL)
            show_flags |= MAIL_SHOW_DELETE;
        if ((*itr)->messageType == MAIL_AUCTION)
            show_flags |= MAIL_SHOW_AUCTION;
        if ((*itr)->HasItems() && (*itr)->messageType == MAIL_NORMAL)
            show_flags |= MAIL_SHOW_RETURN;

        data << uint16(0x0040);                             // unknown 2.3.0, different values
        data << uint32((*itr)->messageID);                  // Message ID
        data << uint8((*itr)->messageType);                 // Message Type

        switch((*itr)->messageType)
        {
            case MAIL_NORMAL:                               // sender guid
                data << uint64(MAKE_NEW_GUID((*itr)->sender, 0, HIGHGUID_PLAYER));
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
        data << uint32((*itr)->itemTextId);                 // sure about this
        data << uint32(0);                                  // unknown
        data << uint32((*itr)->stationery);                 // stationery (Stationery.dbc)
        data << uint32((*itr)->money);                      // Gold
        data << uint32(show_flags);                         // unknown, 0x4 - auction, 0x10 - normal
        data << float(((*itr)->expire_time-time(NULL))/DAY);// Time
        data << uint32((*itr)->mailTemplateId);             // mail template (MailTemplate.dbc)
        data << (*itr)->subject;                            // Subject string - once 00, when mail type = 3

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

///this function is called when client needs mail message body, or when player clicks on item which has ITEM_FIELD_ITEM_TEXT_ID > 0
void WorldSession::HandleItemTextQuery(WorldPacket & recv_data )
{
    uint32 itemTextId;
    uint32 mailId;                                          // this value can be item id in bag, but it is also mail id
    uint32 unk;                                             // maybe something like state - 0x70000000

    recv_data >> itemTextId >> mailId >> unk;

    //some check needed, if player has item with guid mailId, or has mail with id mailId

    sLog.outDebug("CMSG_ITEM_TEXT_QUERY itemguid: %u, mailId: %u, unk: %u", itemTextId, mailId, unk);

    WorldPacket data(SMSG_ITEM_TEXT_QUERY_RESPONSE, (4+10));// guess size
    data << itemTextId;
    data << objmgr.GetItemText( itemTextId );
    SendPacket(&data);
}

//used when player copies mail body to his inventory
void WorldSession::HandleMailCreateTextItem(WorldPacket & recv_data )
{
    uint64 mailbox;
    uint32 mailId;

    recv_data >> mailbox;
    recv_data >> mailId;
    recv_data.read_skip<uint32>();                          // mailTemplateId, non need, Mail store own 100% correct value anyway

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player *pl = _player;

    Mail* m = pl->GetMail(mailId);
    if(!m || !m->itemTextId && !m->mailTemplateId || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL))
    {
        pl->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    uint32 itemTextId = m->itemTextId;

    // in mail template case we need create new text id
    if(!itemTextId)
    {
        MailTemplateEntry const* mailTemplateEntry = sMailTemplateStore.LookupEntry(m->mailTemplateId);
        if(!mailTemplateEntry)
        {
            pl->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
            return;
        }

        itemTextId = objmgr.CreateItemText(mailTemplateEntry->content[GetSessionDbcLocale()]);
    }

    Item *bodyItem = new Item;                              // This is not bag and then can be used new Item.
    if(!bodyItem->Create(objmgr.GenerateLowGuid(HIGHGUID_ITEM), MAIL_BODY_ITEM_TEMPLATE, pl))
    {
        delete bodyItem;
        return;
    }

    bodyItem->SetUInt32Value( ITEM_FIELD_ITEM_TEXT_ID, itemTextId );
    bodyItem->SetUInt32Value( ITEM_FIELD_CREATOR, m->sender);

    sLog.outDetail("HandleMailCreateTextItem mailid=%u",mailId);

    ItemPosCountVec dest;
    uint8 msg = _player->CanStoreItem( NULL_BAG, NULL_SLOT, dest, bodyItem, false );
    if( msg == EQUIP_ERR_OK )
    {
        m->itemTextId = 0;
        m->state = MAIL_STATE_CHANGED;
        pl->m_mailsUpdated = true;

        pl->StoreItem(dest, bodyItem, true);
        //bodyItem->SetState(ITEM_NEW, pl); is set automatically
        pl->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_OK);
    }
    else
    {
        pl->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_EQUIP_ERROR, msg);
        delete bodyItem;
    }
}

//TODO Fix me! ... this void has probably bad condition, but good data are sent
void WorldSession::HandleQueryNextMailTime(WorldPacket & /*recv_data*/ )
{
    WorldPacket data(MSG_QUERY_NEXT_MAIL_TIME, 8);

    if(!_player->m_mailsLoaded)
        _player->_LoadMail();

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

            data << uint64(m->sender);                      // sender guid

            switch(m->messageType)
            {
                case MAIL_AUCTION:
                    data << uint32(2);
                    data << uint32(2);
                    data << uint32(m->stationery);
                    break;
                default:
                    data << uint32(0);
                    data << uint32(0);
                    data << uint32(m->stationery);
                    break;
            }

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

void WorldSession::SendMailTo(Player* receiver, uint8 messageType, uint8 stationery, uint32 sender_guidlow_or_entry, uint32 receiver_guidlow, std::string subject, uint32 itemTextId, MailItemsInfo* mi, uint32 money, uint32 COD, uint32 checked, uint32 deliver_delay, uint16 mailTemplateId)
{
    uint32 mailId = objmgr.GenerateMailID();

    time_t deliver_time = time(NULL) + deliver_delay;

    uint32 expire_delay;
    // auction mail without any items and money (auction sale note) pending 1 hour
    if (messageType == MAIL_AUCTION && !mi && !money)
        expire_delay = HOUR;
    // mail from battlemaster (rewardmarks) should last only one day
    else if (messageType == MAIL_CREATURE && sBattleGroundMgr.GetBattleMasterBG(sender_guidlow_or_entry) != BATTLEGROUND_TYPE_NONE)
        expire_delay = DAY;
    // default case: expire time if COD 3 days, if no COD 30 days
    else
        expire_delay = (COD > 0) ? 3 * DAY : 30 * DAY;



    time_t expire_time = deliver_time + expire_delay;

    if (mailTemplateId && !sMailTemplateStore.LookupEntry(mailTemplateId))
    {
        sLog.outError( "WorldSession::SendMailTo - Mail have not existed MailTemplateId (%u), remove at send", mailTemplateId);
        mailTemplateId = 0;
    }

    // Add to DB
    CharacterDatabase.BeginTransaction();
    CharacterDatabase.escape_string(subject);
    CharacterDatabase.PExecute("INSERT INTO mail (id,messageType,stationery,mailTemplateId,sender,receiver,subject,itemTextId,has_items,expire_time,deliver_time,money,cod,checked) "
        "VALUES ('%u', '%u', '%u', '%u', '%u', '%u', '%s', '%u', '%u', '" UI64FMTD "','" UI64FMTD "', '%u', '%u', '%d')",
        mailId, messageType, stationery, mailTemplateId, sender_guidlow_or_entry, receiver_guidlow, subject.c_str(), itemTextId, (mi && !mi->empty() ? 1 : 0), (uint64)expire_time, (uint64)deliver_time, money, COD, checked);

    if (mi)
    {
        for(MailItemMap::const_iterator mailItemIter = mi->begin(); mailItemIter != mi->end(); ++mailItemIter)
        {
            Item* item = mailItemIter->second;
            CharacterDatabase.PExecute("INSERT INTO mail_items (mail_id,item_guid,item_template,receiver) VALUES ('%u', '%u', '%u','%u')", mailId, item->GetGUIDLow(), item->GetEntry(), receiver_guidlow);
        }
    }
    CharacterDatabase.CommitTransaction();

    // For online receiver update in game mail status and data
    if (receiver)
    {
        receiver->AddNewMailDeliverTime(deliver_time);

        if (receiver->IsMailsLoaded())
        {
            Mail *m = new Mail;
            m->messageID = mailId;
            m->messageType = messageType;
            m->stationery = stationery;
            m->mailTemplateId = mailTemplateId;
            m->sender = sender_guidlow_or_entry;
            m->receiver = receiver->GetGUIDLow();
            m->subject = subject;
            m->itemTextId = itemTextId;

            if (mi)
                m->AddAllItems(*mi);

            m->expire_time = expire_time;
            m->deliver_time = deliver_time;
            m->money = money;
            m->COD = COD;
            m->checked = checked;
            m->state = MAIL_STATE_UNCHANGED;

            receiver->AddMail(m);                           // to insert new mail to beginning of maillist

            if (mi)
            {
                for(MailItemMap::iterator mailItemIter = mi->begin(); mailItemIter != mi->end(); ++mailItemIter)
                    receiver->AddMItem(mailItemIter->second);
            }
        }
        else if (mi)
            mi->deleteIncludedItems();
    }
    else if (mi)
        mi->deleteIncludedItems();
}

void WorldSession::SendMailTo(Player* receiver, Object* sender, uint8 stationery, uint32 receiver_guidlow, std::string subject, uint32 itemTextId, MailItemsInfo* mi, uint32 money, uint32 COD, uint32 checked, uint32 deliver_delay, uint16 mailTemplateId)
{
    MailMessageType mailType;
    uint32 senderGuidOrEntry;
    switch(sender->GetTypeId())
    {
        case TYPEID_UNIT:
            mailType = MAIL_CREATURE;
            senderGuidOrEntry = sender->GetEntry();
            break;
        case TYPEID_GAMEOBJECT:
            mailType = MAIL_GAMEOBJECT;
            senderGuidOrEntry = sender->GetEntry();
            break;
        case TYPEID_ITEM:
            mailType = MAIL_ITEM;
            senderGuidOrEntry = sender->GetEntry();
            break;
        case TYPEID_PLAYER:
            mailType = MAIL_NORMAL;
            senderGuidOrEntry = sender->GetGUIDLow();
            break;
        default:
            mailType = MAIL_NORMAL;
            senderGuidOrEntry = receiver_guidlow;
            sLog.outError( "WorldSession::SendMailTo - Mail have unexpected sender typeid (%u), sent from receiver to self", sender->GetTypeId());
            break;
    }

    SendMailTo(receiver, mailType, stationery, senderGuidOrEntry, receiver_guidlow, subject, itemTextId, mi, money, COD, checked,deliver_delay,mailTemplateId);
}

void WorldSession::SendMailTemplateTo(Player* receiver, Object* sender, uint8 stationery, uint16 mailTemplateId, uint32 money, uint32 COD, uint32 checked, uint32 deliver_delay)
{
    Loot mailLoot;

    mailLoot.FillLoot(mailTemplateId, LootTemplates_Mail, receiver,true);

    // fill mail
    MailItemsInfo mi;                                   // item list preparing

    uint32 max_slot = mailLoot.GetMaxSlotInLootFor(receiver);
    for(uint32 i = 0; mi.size() < MAX_MAIL_ITEMS && i < max_slot; ++i)
    {
        if (LootItem* lootitem = mailLoot.LootItemInSlot(i,receiver))
        {
            if (Item* item = Item::CreateItem(lootitem->itemid,lootitem->count,receiver))
            {
                item->SaveToDB();                       // save for prevent lost at next mail load, if send fail then item will deleted
                mi.AddItem(item);
            }
        }
    }

    WorldSession::SendMailTo(receiver, sender, stationery, receiver->GetGUIDLow(), "", 0, &mi, money, COD, checked,deliver_delay,mailTemplateId);
}
