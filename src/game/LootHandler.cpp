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

#include "Common.h"
#include "WorldPacket.h"
#include "Log.h"
#include "Corpse.h"
#include "GameObject.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "WorldSession.h"
#include "LootMgr.h"
#include "Object.h"
#include "Group.h"
#include "World.h"
#include "Util.h"

void WorldSession::HandleAutostoreLootItemOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_AUTOSTORE_LOOT_ITEM");
    Player  *player =   GetPlayer();
    ObjectGuid lguid = player->GetLootGuid();
    Loot    *loot;
    uint8    lootSlot;
    Item* pItem = NULL;

    recv_data >> lootSlot;

    switch( lguid.GetHigh())
    {
        case HIGHGUID_GAMEOBJECT:
        {
            GameObject *go = player->GetMap()->GetGameObject(lguid);

            // not check distance for GO in case owned GO (fishing bobber case, for example) or Fishing hole GO
            if (!go || ((go->GetOwnerGuid() != _player->GetObjectGuid() && go->GetGoType() != GAMEOBJECT_TYPE_FISHINGHOLE) && !go->IsWithinDistInMap(_player,INTERACTION_DISTANCE)))
            {
                player->SendLootRelease(lguid);
                return;
            }

            loot = &go->loot;
            break;
        }
        case HIGHGUID_ITEM:
        {
            pItem = player->GetItemByGuid( lguid );

            if (!pItem || !pItem->HasGeneratedLoot())
            {
                player->SendLootRelease(lguid);
                return;
            }

            loot = &pItem->loot;
            break;
        }
        case HIGHGUID_CORPSE:
        {
            Corpse *bones = player->GetMap()->GetCorpse(lguid);
            if (!bones)
            {
                player->SendLootRelease(lguid);
                return;
            }
            loot = &bones->loot;
            break;
        }
        case HIGHGUID_UNIT:
        case HIGHGUID_VEHICLE:
        {
            Creature* pCreature = GetPlayer()->GetMap()->GetCreature(lguid);

            bool ok_loot = pCreature && pCreature->isAlive() == (player->getClass()==CLASS_ROGUE && pCreature->lootForPickPocketed);

            if( !ok_loot || !pCreature->IsWithinDistInMap(_player,INTERACTION_DISTANCE) )
            {
                player->SendLootRelease(lguid);
                return;
            }

            loot = &pCreature->loot;
            break;
        }
        default:
        {
            sLog.outError("%s is unsupported for looting.",lguid.GetString().c_str());
            return;
        }
    }

    QuestItem *qitem = NULL;
    QuestItem *ffaitem = NULL;
    QuestItem *conditem = NULL;

    LootItem *item = loot->LootItemInSlot(lootSlot,player,&qitem,&ffaitem,&conditem);

    if(!item)
    {
        player->SendEquipError( EQUIP_ERR_ALREADY_LOOTED, NULL, NULL );
        return;
    }

    // questitems use the blocked field for other purposes
    if (!qitem && item->is_blocked)
    {
        player->SendLootRelease(lguid);
        return;
    }

    if (pItem)
        pItem->SetLootState(ITEM_LOOT_CHANGED);

    ItemPosCountVec dest;
    InventoryResult msg = player->CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, item->itemid, item->count );
    if ( msg == EQUIP_ERR_OK )
    {
        Item * newitem = player->StoreNewItem( dest, item->itemid, true, item->randomPropertyId);

        if (qitem)
        {
            qitem->is_looted = true;
            //freeforall is 1 if everyone's supposed to get the quest item.
            if (item->freeforall || loot->GetPlayerQuestItems().size() == 1)
                player->SendNotifyLootItemRemoved(lootSlot);
            else
                loot->NotifyQuestItemRemoved(qitem->index);
        }
        else
        {
            if (ffaitem)
            {
                //freeforall case, notify only one player of the removal
                ffaitem->is_looted=true;
                player->SendNotifyLootItemRemoved(lootSlot);
            }
            else
            {
                //not freeforall, notify everyone
                if(conditem)
                    conditem->is_looted=true;
                loot->NotifyItemRemoved(lootSlot);
            }
        }

        //if only one person is supposed to loot the item, then set it to looted
        if (!item->freeforall)
            item->is_looted = true;

        --loot->unlootedCount;

        player->SendNewItem(newitem, uint32(item->count), false, false, true);
        player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM, item->itemid, item->count);
        player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE, loot->loot_type, item->count);
        player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM, item->itemid, item->count);
    }
    else
        player->SendEquipError( msg, NULL, NULL, item->itemid );
}

void WorldSession::HandleLootMoneyOpcode( WorldPacket & /*recv_data*/ )
{
    DEBUG_LOG("WORLD: CMSG_LOOT_MONEY");

    Player *player = GetPlayer();
    ObjectGuid guid = player->GetLootGuid();
    if (!guid)
        return;

    Loot *pLoot = NULL;
    Item* pItem = NULL;

    switch(guid.GetHigh())
    {
        case HIGHGUID_GAMEOBJECT:
        {
            GameObject *pGameObject = GetPlayer()->GetMap()->GetGameObject(guid);

            // not check distance for GO in case owned GO (fishing bobber case, for example)
            if( pGameObject && (pGameObject->GetOwnerGuid() == _player->GetObjectGuid() || pGameObject->IsWithinDistInMap(_player,INTERACTION_DISTANCE)) )
                pLoot = &pGameObject->loot;

            break;
        }
        case HIGHGUID_CORPSE:                               // remove insignia ONLY in BG
        {
            Corpse *bones = _player->GetMap()->GetCorpse(guid);

            if (bones && bones->IsWithinDistInMap(_player,INTERACTION_DISTANCE) )
                pLoot = &bones->loot;

            break;
        }
        case HIGHGUID_ITEM:
        {
            pItem = GetPlayer()->GetItemByGuid(guid);
            if (!pItem || !pItem->HasGeneratedLoot())
                return;

            pLoot = &pItem->loot;
            break;
        }
        case HIGHGUID_UNIT:
        case HIGHGUID_VEHICLE:
        {
            Creature* pCreature = GetPlayer()->GetMap()->GetCreature(guid);

            bool ok_loot = pCreature && pCreature->isAlive() == (player->getClass()==CLASS_ROGUE && pCreature->lootForPickPocketed);

            if ( ok_loot && pCreature->IsWithinDistInMap(_player,INTERACTION_DISTANCE) )
                pLoot = &pCreature->loot ;

            break;
        }
        default:
            return;                                         // unlootable type
    }

    if (pLoot)
    {
        pLoot->NotifyMoneyRemoved();

        if (!guid.IsItem() && player->GetGroup())           //item can be looted only single player
        {
            Group *group = player->GetGroup();

            std::vector<Player*> playersNear;
            for(GroupReference *itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* playerGroup = itr->getSource();
                if(!playerGroup)
                    continue;
                if (player->IsWithinDistInMap(playerGroup,sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE),false))
                    playersNear.push_back(playerGroup);
            }

            uint32 money_per_player = uint32((pLoot->gold)/(playersNear.size()));

            for (std::vector<Player*>::const_iterator i = playersNear.begin(); i != playersNear.end(); ++i)
            {
                (*i)->ModifyMoney( money_per_player );
                (*i)->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY, money_per_player);

                WorldPacket data(SMSG_LOOT_MONEY_NOTIFY, 4+1);
                data << uint32(money_per_player);
                data << uint8(playersNear.size() > 1 ? 0 : 1);// 0 is "you share of loot..."

                (*i)->GetSession()->SendPacket(&data);
            }
        }
        else
        {
            player->ModifyMoney( pLoot->gold );
            player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY, pLoot->gold);

            WorldPacket data(SMSG_LOOT_MONEY_NOTIFY, 4+1);
            data << uint32(pLoot->gold);
            data << uint8(1);                               // 1 is "you loot..."
            player->GetSession()->SendPacket(&data);
        }

        pLoot->gold = 0;

        if (pItem)
            pItem->SetLootState(ITEM_LOOT_CHANGED);
    }
}

void WorldSession::HandleLootOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_LOOT");

    ObjectGuid guid;
    recv_data >> guid;

    // Check possible cheat
    if (!_player->isAlive())
        return;

    GetPlayer()->SendLoot(guid, LOOT_CORPSE);
}

void WorldSession::HandleLootReleaseOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_LOOT_RELEASE");

    // cheaters can modify lguid to prevent correct apply loot release code and re-loot
    // use internal stored guid
    recv_data.read_skip<uint64>();                          // guid;

    if (ObjectGuid lootGuid = GetPlayer()->GetLootGuid())
        DoLootRelease(lootGuid);
}

void WorldSession::DoLootRelease(ObjectGuid lguid)
{
    Player  *player = GetPlayer();
    Loot    *loot;

    player->SetLootGuid(ObjectGuid());
    player->SendLootRelease(lguid);

    player->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_LOOTING);

    if(!player->IsInWorld())
        return;

    switch(lguid.GetHigh())
    {
        case HIGHGUID_GAMEOBJECT:
        {
            GameObject *go = GetPlayer()->GetMap()->GetGameObject(lguid);

            // not check distance for GO in case owned GO (fishing bobber case, for example) or Fishing hole GO
            if (!go || ((go->GetOwnerGuid() != _player->GetObjectGuid() && go->GetGoType() != GAMEOBJECT_TYPE_FISHINGHOLE) && !go->IsWithinDistInMap(_player,INTERACTION_DISTANCE)))
                return;

            loot = &go->loot;

            if (go->GetGoType() == GAMEOBJECT_TYPE_DOOR)
            {
                // locked doors are opened with spelleffect openlock, prevent remove its as looted
                go->UseDoorOrButton();
            }
            else if (loot->isLooted() || go->GetGoType() == GAMEOBJECT_TYPE_FISHINGNODE)
            {
                // GO is mineral vein? so it is not removed after its looted
                if(go->GetGoType() == GAMEOBJECT_TYPE_CHEST)
                {
                    uint32 go_min = go->GetGOInfo()->chest.minSuccessOpens;
                    uint32 go_max = go->GetGOInfo()->chest.maxSuccessOpens;

                    // only vein pass this check
                    if(go_min != 0 && go_max > go_min)
                    {
                        float amount_rate = sWorld.getConfig(CONFIG_FLOAT_RATE_MINING_AMOUNT);
                        float min_amount = go_min*amount_rate;
                        float max_amount = go_max*amount_rate;

                        go->AddUse();
                        float uses = float(go->GetUseCount());

                        if(uses < max_amount)
                        {
                            if(uses >= min_amount)
                            {
                                float chance_rate = sWorld.getConfig(CONFIG_FLOAT_RATE_MINING_NEXT);

                                int32 ReqValue = 175;
                                LockEntry const *lockInfo = sLockStore.LookupEntry(go->GetGOInfo()->chest.lockId);
                                if(lockInfo)
                                    ReqValue = lockInfo->Skill[0];
                                float skill = float(player->GetSkillValue(SKILL_MINING))/(ReqValue+25);
                                double chance = pow(0.8*chance_rate,4*(1/double(max_amount))*double(uses));
                                if(roll_chance_f(float(100.0f*chance+skill)))
                                {
                                    go->SetLootState(GO_READY);
                                }
                                else                            // not have more uses
                                    go->SetLootState(GO_JUST_DEACTIVATED);
                            }
                            else                                // 100% chance until min uses
                                go->SetLootState(GO_READY);
                        }
                        else                                    // max uses already
                            go->SetLootState(GO_JUST_DEACTIVATED);
                    }
                    else                                        // not vein
                        go->SetLootState(GO_JUST_DEACTIVATED);
                }
                else if (go->GetGoType() == GAMEOBJECT_TYPE_FISHINGHOLE)
                {                                               // The fishing hole used once more
                    go->AddUse();                               // if the max usage is reached, will be despawned at next tick
                    if (go->GetUseCount() >= urand(go->GetGOInfo()->fishinghole.minSuccessOpens,go->GetGOInfo()->fishinghole.maxSuccessOpens))
                    {
                        go->SetLootState(GO_JUST_DEACTIVATED);
                    }
                    else
                        go->SetLootState(GO_READY);
                }
                else // not chest (or vein/herb/etc)
                    go->SetLootState(GO_JUST_DEACTIVATED);

                loot->clear();
            }
            else
                // not fully looted object
                go->SetLootState(GO_ACTIVATED);
            break;
        }
        case HIGHGUID_CORPSE:                               // ONLY remove insignia at BG
        {
            Corpse *corpse = _player->GetMap()->GetCorpse(lguid);
            if (!corpse || !corpse->IsWithinDistInMap(_player,INTERACTION_DISTANCE) )
                return;

            loot = &corpse->loot;

            if (loot->isLooted())
            {
                loot->clear();
                corpse->RemoveFlag(CORPSE_FIELD_DYNAMIC_FLAGS, CORPSE_DYNFLAG_LOOTABLE);
            }
            break;
        }
        case HIGHGUID_ITEM:
        {
            Item *pItem = player->GetItemByGuid(lguid );
            if (!pItem)
                return;

            switch (pItem->loot.loot_type)
            {
                // temporary loot in stacking items, clear loot state, no auto loot move
                case LOOT_MILLING:
                case LOOT_PROSPECTING:
                {
                    uint32 count = pItem->GetCount();

                    // >=5 checked in spell code, but will work for cheating cases also with removing from another stacks.
                    if(count > 5)
                        count = 5;

                    // reset loot for allow repeat looting if stack > 5
                    pItem->loot.clear();
                    pItem->SetLootState(ITEM_LOOT_REMOVED);

                    player->DestroyItemCount(pItem, count, true);
                    break;
                }
                // temporary loot, auto loot move
                case LOOT_DISENCHANTING:
                {
                    if (!pItem->loot.isLooted())
                        player->AutoStoreLoot(pItem->loot); // can be lost if no space
                    pItem->loot.clear();
                    pItem->SetLootState(ITEM_LOOT_REMOVED);
                    player->DestroyItem( pItem->GetBagSlot(),pItem->GetSlot(), true);
                    break;
                }
                // normal persistence loot
                default:
                {
                    // must be destroyed only if no loot
                    if (pItem->loot.isLooted())
                    {
                        pItem->SetLootState(ITEM_LOOT_REMOVED);
                        player->DestroyItem( pItem->GetBagSlot(),pItem->GetSlot(), true);
                    }
                    break;
                }
            }
            return;                                         // item can be looted only single player
        }
        case HIGHGUID_UNIT:
        case HIGHGUID_VEHICLE:
        {
            Creature* pCreature = GetPlayer()->GetMap()->GetCreature(lguid);

            bool ok_loot = pCreature && pCreature->isAlive() == (player->getClass()==CLASS_ROGUE && pCreature->lootForPickPocketed);
            if ( !ok_loot || !pCreature->IsWithinDistInMap(_player,INTERACTION_DISTANCE) )
                return;

            loot = &pCreature->loot;

            // update next looter
            if(Group* group = pCreature->GetGroupLootRecipient())
                if (group->GetLooterGuid() == player->GetObjectGuid())
                    group->UpdateLooterGuid(pCreature);

            if (loot->isLooted())
            {
                // for example skinning after normal loot
                pCreature->PrepareBodyLootState();

                if(!pCreature->isAlive())
                    pCreature->AllLootRemovedFromCorpse();
            }
            break;
        }
        default:
        {
            sLog.outError("%s is unsupported for looting.", lguid.GetString().c_str());
            return;
        }
    }

    //Player is not looking at loot list, he doesn't need to see updates on the loot list
    loot->RemoveLooter(player->GetObjectGuid());
}

void WorldSession::HandleLootMasterGiveOpcode( WorldPacket & recv_data )
{
    uint8 slotid;
    ObjectGuid lootguid;
    ObjectGuid target_playerguid;

    recv_data >> lootguid >> slotid >> target_playerguid;

    if (!_player->GetGroup() || _player->GetGroup()->GetLooterGuid() != _player->GetObjectGuid())
    {
        _player->SendLootRelease(GetPlayer()->GetLootGuid());
        return;
    }

    Player *target = ObjectAccessor::FindPlayer(target_playerguid);
    if (!target)
        return;

    DEBUG_LOG("WorldSession::HandleLootMasterGiveOpcode (CMSG_LOOT_MASTER_GIVE, 0x02A3) Target = %s [%s].", target_playerguid.GetString().c_str(), target->GetName());

    if (_player->GetLootGuid() != lootguid)
        return;

    Loot *pLoot = NULL;

    if (lootguid.IsCreatureOrVehicle())
    {
        Creature *pCreature = GetPlayer()->GetMap()->GetCreature(lootguid);
        if(!pCreature)
            return;

        pLoot = &pCreature->loot;
    }
    else if(lootguid.IsGameObject())
    {
        GameObject *pGO = GetPlayer()->GetMap()->GetGameObject(lootguid);
        if(!pGO)
            return;

        pLoot = &pGO->loot;
    }
    else
        return;

    if (slotid > pLoot->items.size())
    {
        DEBUG_LOG("AutoLootItem: Player %s might be using a hack! (slot %d, size %lu)",GetPlayer()->GetName(), slotid, (unsigned long)pLoot->items.size());
        return;
    }

    LootItem& item = pLoot->items[slotid];

    ItemPosCountVec dest;
    InventoryResult msg = target->CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, item.itemid, item.count );
    if ( msg != EQUIP_ERR_OK )
    {
        target->SendEquipError( msg, NULL, NULL, item.itemid );

        // send duplicate of error massage to master looter
        _player->SendEquipError( msg, NULL, NULL, item.itemid );
        return;
    }

    // now move item from loot to target inventory
    Item * newitem = target->StoreNewItem( dest, item.itemid, true, item.randomPropertyId );
    target->SendNewItem(newitem, uint32(item.count), false, false, true );
    target->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM, item.itemid, item.count);
    target->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE, pLoot->loot_type, item.count);
    target->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM, item.itemid, item.count);

    // mark as looted
    item.count=0;
    item.is_looted=true;

    pLoot->NotifyItemRemoved(slotid);
    --pLoot->unlootedCount;
}
