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

#include "Common.h"
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "GossipDef.h"
#include "QuestDef.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "Group.h"

void WorldSession::HandleQuestgiverStatusQueryOpcode( WorldPacket & recv_data )
{
    ObjectGuid guid;
    recv_data >> guid;
    uint8 dialogStatus = DIALOG_STATUS_NONE;

    Object* questgiver = _player->GetObjectByTypeMask(guid, TYPEMASK_CREATURE_OR_GAMEOBJECT);

    if (!questgiver)
    {
        DETAIL_LOG("Error in CMSG_QUESTGIVER_STATUS_QUERY, called for not found questgiver %s", guid.GetString().c_str());
        return;
    }

    DEBUG_LOG("WORLD: Received CMSG_QUESTGIVER_STATUS_QUERY for %s", guid.GetString().c_str());

    switch(questgiver->GetTypeId())
    {
        case TYPEID_UNIT:
        {
            Creature* cr_questgiver = (Creature*)questgiver;

            if (!cr_questgiver->IsHostileTo(_player))       // not show quest status to enemies
            {
                dialogStatus = sScriptMgr.GetDialogStatus(_player, cr_questgiver);

                if (dialogStatus > DIALOG_STATUS_REWARD_REP)
                    dialogStatus = getDialogStatus(_player, cr_questgiver, DIALOG_STATUS_NONE);
            }
            break;
        }
        case TYPEID_GAMEOBJECT:
        {
            GameObject* go_questgiver = (GameObject*)questgiver;
            dialogStatus = sScriptMgr.GetDialogStatus(_player, go_questgiver);

            if (dialogStatus > DIALOG_STATUS_REWARD_REP)
                dialogStatus = getDialogStatus(_player, go_questgiver, DIALOG_STATUS_NONE);

            break;
        }
        default:
            sLog.outError("QuestGiver called for unexpected type %u", questgiver->GetTypeId());
            break;
    }

    //inform client about status of quest
    _player->PlayerTalkClass->SendQuestGiverStatus(dialogStatus, guid);
}

void WorldSession::HandleQuestgiverHelloOpcode(WorldPacket & recv_data)
{
    ObjectGuid guid;
    recv_data >> guid;

    DEBUG_LOG("WORLD: Received CMSG_QUESTGIVER_HELLO npc: %s", guid.GetString().c_str());

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);

    if (!pCreature)
    {
        DEBUG_LOG ("WORLD: HandleQuestgiverHelloOpcode - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    // Stop the npc if moving
    if (!pCreature->IsStopped())
        pCreature->StopMoving();

    if (sScriptMgr.OnGossipHello(_player, pCreature))
        return;

    _player->PrepareGossipMenu(pCreature, pCreature->GetCreatureInfo()->GossipMenuId);
    _player->SendPreparedGossip(pCreature);
}

void WorldSession::HandleQuestgiverAcceptQuestOpcode( WorldPacket & recv_data )
{
    ObjectGuid guid;
    uint32 quest;
    uint32 unk1;
    recv_data >> guid >> quest >> unk1;

    if (!GetPlayer()->isAlive())
        return;

    DEBUG_LOG("WORLD: Received CMSG_QUESTGIVER_ACCEPT_QUEST npc = %s, quest = %u, unk1 = %u", guid.GetString().c_str(), quest, unk1 );

    Object* pObject = _player->GetObjectByTypeMask(guid, TYPEMASK_CREATURE_GAMEOBJECT_PLAYER_OR_ITEM);

    // no or incorrect quest giver
    if(!pObject
        || (pObject->GetTypeId()!=TYPEID_PLAYER && !pObject->HasQuest(quest))
        || (pObject->GetTypeId()==TYPEID_PLAYER && !((Player*)pObject)->CanShareQuest(quest))
        )
    {
        _player->PlayerTalkClass->CloseGossip();
        _player->SetDivider( 0 );
        return;
    }

    Quest const* qInfo = sObjectMgr.GetQuestTemplate(quest);
    if ( qInfo )
    {
        // prevent cheating
        if(!GetPlayer()->CanTakeQuest(qInfo,true) )
        {
            _player->PlayerTalkClass->CloseGossip();
            _player->SetDivider( 0 );
            return;
        }

        if( _player->GetDivider() != 0 )
        {
            Player *pPlayer = ObjectAccessor::FindPlayer( _player->GetDivider() );
            if( pPlayer )
            {
                pPlayer->SendPushToPartyResponse( _player, QUEST_PARTY_MSG_ACCEPT_QUEST );
                _player->SetDivider( 0 );
            }
        }

        if( _player->CanAddQuest( qInfo, true ) )
        {
            _player->AddQuest( qInfo, pObject );

            if (qInfo->HasQuestFlag(QUEST_FLAGS_PARTY_ACCEPT))
            {
                if (Group* pGroup = _player->GetGroup())
                {
                    for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
                    {
                        Player* pPlayer = itr->getSource();

                        if (!pPlayer || pPlayer == _player)     // not self
                            continue;

                        if (pPlayer->CanTakeQuest(qInfo, true))
                        {
                            pPlayer->SetDivider(_player->GetGUID());

                            //need confirmation that any gossip window will close
                            pPlayer->PlayerTalkClass->CloseGossip();

                            _player->SendQuestConfirmAccept(qInfo, pPlayer);
                        }
                    }
                }
            }

            if ( _player->CanCompleteQuest( quest ) )
                _player->CompleteQuest( quest );

            _player->GetAchievementMgr().StartTimedAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST, quest);

            switch(pObject->GetTypeId())
            {
                case TYPEID_UNIT:
                    sScriptMgr.OnQuestAccept(_player, (Creature*)pObject, qInfo);
                    break;
                case TYPEID_ITEM:
                case TYPEID_CONTAINER:
                {
                    sScriptMgr.OnQuestAccept(_player, (Item*)pObject, qInfo);

                    // destroy not required for quest finish quest starting item
                    bool destroyItem = true;
                    for(int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
                    {
                        if ((qInfo->ReqItemId[i] == ((Item*)pObject)->GetEntry()) && (((Item*)pObject)->GetProto()->MaxCount > 0))
                        {
                            destroyItem = false;
                            break;
                        }
                    }

                    if(destroyItem)
                        _player->DestroyItem(((Item*)pObject)->GetBagSlot(), ((Item*)pObject)->GetSlot(), true);

                    break;
                }
                case TYPEID_GAMEOBJECT:
                    sScriptMgr.OnQuestAccept(_player, (GameObject*)pObject, qInfo);
                    break;
            }
            _player->PlayerTalkClass->CloseGossip();

            if( qInfo->GetSrcSpell() > 0 )
                _player->CastSpell( _player, qInfo->GetSrcSpell(), true);

            return;
        }
    }

    _player->PlayerTalkClass->CloseGossip();
}

void WorldSession::HandleQuestgiverQueryQuestOpcode( WorldPacket & recv_data )
{
    ObjectGuid guid;
    uint32 quest;
    uint8 unk1;
    recv_data >> guid >> quest >> unk1;

    DEBUG_LOG("WORLD: Received CMSG_QUESTGIVER_QUERY_QUEST npc = %s, quest = %u, unk1 = %u", guid.GetString().c_str(), quest, unk1 );

    // Verify that the guid is valid and is a questgiver or involved in the requested quest
    Object* pObject = _player->GetObjectByTypeMask(guid, TYPEMASK_CREATURE_GAMEOBJECT_OR_ITEM);
    if (!pObject || (!pObject->HasQuest(quest) && !pObject->HasInvolvedQuest(quest)))
    {
        _player->PlayerTalkClass->CloseGossip();
        return;
    }

    if (Quest const* pQuest = sObjectMgr.GetQuestTemplate(quest))
        _player->PlayerTalkClass->SendQuestGiverQuestDetails(pQuest, pObject->GetObjectGuid(), true);
}

void WorldSession::HandleQuestQueryOpcode( WorldPacket & recv_data )
{
    uint32 quest;
    recv_data >> quest;
    DEBUG_LOG( "WORLD: Received CMSG_QUEST_QUERY quest = %u",quest );

    Quest const *pQuest = sObjectMgr.GetQuestTemplate(quest);
    if ( pQuest )
    {
        _player->PlayerTalkClass->SendQuestQueryResponse( pQuest );
    }
}

void WorldSession::HandleQuestgiverChooseRewardOpcode( WorldPacket & recv_data )
{
    uint32 quest, reward;
    ObjectGuid guid;
    recv_data >> guid >> quest >> reward;

    if(reward >= QUEST_REWARD_CHOICES_COUNT)
    {
        sLog.outError("Error in CMSG_QUESTGIVER_CHOOSE_REWARD: player %s (guid %d) tried to get invalid reward (%u) (probably packet hacking)", _player->GetName(), _player->GetGUIDLow(), reward);
        return;
    }

    if(!GetPlayer()->isAlive())
        return;

    DEBUG_LOG("WORLD: Received CMSG_QUESTGIVER_CHOOSE_REWARD npc = %s, quest = %u, reward = %u", guid.GetString().c_str(), quest, reward);

    Object* pObject = _player->GetObjectByTypeMask(guid, TYPEMASK_CREATURE_OR_GAMEOBJECT);
    if(!pObject)
        return;

    if(!pObject->HasInvolvedQuest(quest))
        return;

    Quest const *pQuest = sObjectMgr.GetQuestTemplate(quest);
    if( pQuest )
    {
        if( _player->CanRewardQuest( pQuest, reward, true ) )
        {
            _player->RewardQuest( pQuest, reward, pObject );

            // Send next quest
            if (Quest const* nextquest = _player->GetNextQuest(guid, pQuest))
                _player->PlayerTalkClass->SendQuestGiverQuestDetails(nextquest, guid, true);
        }
        else
            _player->PlayerTalkClass->SendQuestGiverOfferReward(pQuest, guid, true);
    }
}

void WorldSession::HandleQuestgiverRequestRewardOpcode( WorldPacket & recv_data )
{
    uint32 quest;
    ObjectGuid guid;
    recv_data >> guid >> quest;

    if (!GetPlayer()->isAlive())
        return;

    DEBUG_LOG("WORLD: Received CMSG_QUESTGIVER_REQUEST_REWARD npc = %s, quest = %u", guid.GetString().c_str(), quest);

    Object* pObject = _player->GetObjectByTypeMask(guid, TYPEMASK_CREATURE_OR_GAMEOBJECT);
    if (!pObject||!pObject->HasInvolvedQuest(quest))
        return;

    if (_player->CanCompleteQuest(quest))
        _player->CompleteQuest(quest);

    if (_player->GetQuestStatus(quest) != QUEST_STATUS_COMPLETE)
        return;

    if (Quest const *pQuest = sObjectMgr.GetQuestTemplate(quest))
        _player->PlayerTalkClass->SendQuestGiverOfferReward(pQuest, guid, true);
}

void WorldSession::HandleQuestgiverCancel(WorldPacket& /*recv_data*/ )
{
    DEBUG_LOG( "WORLD: Received CMSG_QUESTGIVER_CANCEL" );

    _player->PlayerTalkClass->CloseGossip();
}

void WorldSession::HandleQuestLogSwapQuest(WorldPacket& recv_data )
{
    uint8 slot1, slot2;
    recv_data >> slot1 >> slot2;

    if(slot1 == slot2 || slot1 >= MAX_QUEST_LOG_SIZE || slot2 >= MAX_QUEST_LOG_SIZE)
        return;

    DEBUG_LOG( "WORLD: Received CMSG_QUESTLOG_SWAP_QUEST slot 1 = %u, slot 2 = %u", slot1, slot2 );

    GetPlayer()->SwapQuestSlot(slot1,slot2);
}

void WorldSession::HandleQuestLogRemoveQuest(WorldPacket& recv_data)
{
    uint8 slot;
    recv_data >> slot;

    DEBUG_LOG( "WORLD: Received CMSG_QUESTLOG_REMOVE_QUEST slot = %u",slot );

    if( slot < MAX_QUEST_LOG_SIZE )
    {
        if(uint32 quest = _player->GetQuestSlotQuestId(slot))
        {
            if(!_player->TakeQuestSourceItem( quest, true ))
                return;                                     // can't un-equip some items, reject quest cancel

            if (const Quest *pQuest = sObjectMgr.GetQuestTemplate(quest))
            {
                if (pQuest->HasSpecialFlag(QUEST_SPECIAL_FLAG_TIMED))
                    _player->RemoveTimedQuest(quest);
            }

            _player->SetQuestStatus( quest, QUEST_STATUS_NONE);
        }

        _player->SetQuestSlot(slot, 0);

        _player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED, 1);
    }
}

void WorldSession::HandleQuestConfirmAccept(WorldPacket& recv_data)
{
    uint32 quest;
    recv_data >> quest;

    DEBUG_LOG("WORLD: Received CMSG_QUEST_CONFIRM_ACCEPT quest = %u", quest);

    if (const Quest* pQuest = sObjectMgr.GetQuestTemplate(quest))
    {
        if (!pQuest->HasQuestFlag(QUEST_FLAGS_PARTY_ACCEPT))
            return;

        Player* pOriginalPlayer = ObjectAccessor::FindPlayer(_player->GetDivider());

        if (!pOriginalPlayer)
            return;

        if (pQuest->IsAllowedInRaid())
        {
            if (!_player->IsInSameRaidWith(pOriginalPlayer))
                return;
        }
        else
        {
            if (!_player->IsInSameGroupWith(pOriginalPlayer))
                return;
        }

        if (_player->CanAddQuest(pQuest, true))
            _player->AddQuest(pQuest, NULL);                // NULL, this prevent DB script from duplicate running

        _player->SetDivider(0);
    }
}

void WorldSession::HandleQuestgiverCompleteQuest(WorldPacket& recv_data)
{
    uint32 quest;
    ObjectGuid guid;
    recv_data >> guid >> quest;

    if (!GetPlayer()->isAlive())
        return;

    DEBUG_LOG("WORLD: Received CMSG_QUESTGIVER_COMPLETE_QUEST npc = %s, quest = %u", guid.GetString().c_str(), quest);

    if (Quest const *pQuest = sObjectMgr.GetQuestTemplate(quest))
    {
        if (_player->GetQuestStatus( quest ) != QUEST_STATUS_COMPLETE)
        {
            if (pQuest->IsRepeatable())
                _player->PlayerTalkClass->SendQuestGiverRequestItems(pQuest, guid, _player->CanCompleteRepeatableQuest(pQuest), false);
            else
                _player->PlayerTalkClass->SendQuestGiverRequestItems(pQuest, guid, _player->CanRewardQuest(pQuest,false), false);
        }
        else
        {
            if (pQuest->GetReqItemsCount())                 // some items required
                _player->PlayerTalkClass->SendQuestGiverRequestItems(pQuest, guid, _player->CanRewardQuest(pQuest,false), false);
            else                                            // no items required
                _player->PlayerTalkClass->SendQuestGiverOfferReward(pQuest, guid, true);
        }
    }
}

void WorldSession::HandleQuestgiverQuestAutoLaunch(WorldPacket& /*recvPacket*/)
{
    DEBUG_LOG( "WORLD: Received CMSG_QUESTGIVER_QUEST_AUTOLAUNCH" );
}

void WorldSession::HandlePushQuestToParty(WorldPacket& recvPacket)
{
    uint32 questId;
    recvPacket >> questId;

    DEBUG_LOG("WORLD: Received CMSG_PUSHQUESTTOPARTY quest = %u", questId);

    if (Quest const *pQuest = sObjectMgr.GetQuestTemplate(questId))
    {
        if (Group* pGroup = _player->GetGroup())
        {
            for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player *pPlayer = itr->getSource();

                if (!pPlayer || pPlayer == _player)         // skip self
                    continue;

                _player->SendPushToPartyResponse(pPlayer, QUEST_PARTY_MSG_SHARING_QUEST);

                if (!pPlayer->SatisfyQuestStatus(pQuest, false))
                {
                    _player->SendPushToPartyResponse(pPlayer, QUEST_PARTY_MSG_HAVE_QUEST);
                    continue;
                }

                if (pPlayer->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
                {
                    _player->SendPushToPartyResponse(pPlayer, QUEST_PARTY_MSG_FINISH_QUEST);
                    continue;
                }

                if (!pPlayer->CanTakeQuest(pQuest, false))
                {
                    _player->SendPushToPartyResponse(pPlayer, QUEST_PARTY_MSG_CANT_TAKE_QUEST);
                    continue;
                }

                if (!pPlayer->SatisfyQuestLog(false))
                {
                    _player->SendPushToPartyResponse(pPlayer, QUEST_PARTY_MSG_LOG_FULL);
                    continue;
                }

                if (pPlayer->GetDivider() != 0)
                {
                    _player->SendPushToPartyResponse(pPlayer, QUEST_PARTY_MSG_BUSY);
                    continue;
                }

                pPlayer->PlayerTalkClass->SendQuestGiverQuestDetails(pQuest, _player->GetObjectGuid(), true);
                pPlayer->SetDivider(_player->GetGUID());
            }
        }
    }
}

void WorldSession::HandleQuestPushResult(WorldPacket& recvPacket)
{
    ObjectGuid guid;
    uint8 msg;
    recvPacket >> guid >> msg;

    DEBUG_LOG("WORLD: Received MSG_QUEST_PUSH_RESULT");

    if (_player->GetDivider() != 0)
    {
        if (Player *pPlayer = ObjectAccessor::FindPlayer(_player->GetDivider()))
        {
            WorldPacket data( MSG_QUEST_PUSH_RESULT, (8+1) );
            data << ObjectGuid(guid);
            data << uint8(msg);                             // valid values: 0-8
            pPlayer->GetSession()->SendPacket(&data);
            _player->SetDivider( 0 );
        }
    }
}

uint32 WorldSession::getDialogStatus(Player *pPlayer, Object* questgiver, uint32 defstatus)
{
    uint32 dialogStatus = defstatus;

    QuestRelationsMapBounds rbounds;
    QuestRelationsMapBounds irbounds;

    switch(questgiver->GetTypeId())
    {
        case TYPEID_UNIT:
        {
            rbounds = sObjectMgr.GetCreatureQuestRelationsMapBounds(questgiver->GetEntry());
            irbounds = sObjectMgr.GetCreatureQuestInvolvedRelationsMapBounds(questgiver->GetEntry());
            break;
        }
        case TYPEID_GAMEOBJECT:
        {
            rbounds = sObjectMgr.GetGOQuestRelationsMapBounds(questgiver->GetEntry());
            irbounds = sObjectMgr.GetGOQuestInvolvedRelationsMapBounds(questgiver->GetEntry());
            break;
        }
        default:
            //it's impossible, but check ^)
            sLog.outError("Warning: GetDialogStatus called for unexpected type %u", questgiver->GetTypeId());
            return DIALOG_STATUS_NONE;
    }

    for(QuestRelationsMap::const_iterator itr = irbounds.first; itr != irbounds.second; ++itr)
    {
        uint32 dialogStatusNew = 0;
        uint32 quest_id = itr->second;
        Quest const *pQuest = sObjectMgr.GetQuestTemplate(quest_id);

        if (!pQuest || !pQuest->IsActive())
            continue;

        QuestStatus status = pPlayer->GetQuestStatus(quest_id);

        if ((status == QUEST_STATUS_COMPLETE && !pPlayer->GetQuestRewardStatus(quest_id)) ||
            (pQuest->IsAutoComplete() && pPlayer->CanTakeQuest(pQuest, false)))
        {
            if (pQuest->IsAutoComplete() && pQuest->IsRepeatable())
                dialogStatusNew = DIALOG_STATUS_REWARD_REP;
            else
                dialogStatusNew = DIALOG_STATUS_REWARD;
        }
        else if (status == QUEST_STATUS_INCOMPLETE)
            dialogStatusNew = DIALOG_STATUS_INCOMPLETE;

        if (dialogStatusNew > dialogStatus)
            dialogStatus = dialogStatusNew;
    }

    for(QuestRelationsMap::const_iterator itr = rbounds.first; itr != rbounds.second; ++itr)
    {
        uint32 dialogStatusNew = 0;
        uint32 quest_id = itr->second;
        Quest const *pQuest = sObjectMgr.GetQuestTemplate(quest_id);

        if (!pQuest || !pQuest->IsActive())
            continue;

        QuestStatus status = pPlayer->GetQuestStatus(quest_id);

        if (status == QUEST_STATUS_NONE)
        {
            if (pPlayer->CanSeeStartQuest(pQuest))
            {
                if (pPlayer->SatisfyQuestLevel(pQuest, false))
                {
                    if (pQuest->IsAutoComplete() || (pQuest->IsRepeatable() && pPlayer->getQuestStatusMap()[quest_id].m_rewarded))
                    {
                        dialogStatusNew = DIALOG_STATUS_REWARD_REP;
                    }
                    else if (pPlayer->getLevel() <= pPlayer->GetQuestLevelForPlayer(pQuest) + sWorld.getConfig(CONFIG_UINT32_QUEST_LOW_LEVEL_HIDE_DIFF))
                    {
                        if (pQuest->HasQuestFlag(QUEST_FLAGS_DAILY) || pQuest->HasQuestFlag(QUEST_FLAGS_WEEKLY))
                            dialogStatusNew = DIALOG_STATUS_AVAILABLE_REP;
                        else
                            dialogStatusNew = DIALOG_STATUS_AVAILABLE;
                    }
                    else
                        dialogStatusNew = DIALOG_STATUS_LOW_LEVEL_AVAILABLE;
                }
                else
                    dialogStatusNew = DIALOG_STATUS_UNAVAILABLE;
            }
        }

        if (dialogStatusNew > dialogStatus)
            dialogStatus = dialogStatusNew;
    }

    return dialogStatus;
}

void WorldSession::HandleQuestgiverStatusMultipleQuery(WorldPacket& /*recvPacket*/)
{
    DEBUG_LOG("WORLD: Received CMSG_QUESTGIVER_STATUS_MULTIPLE_QUERY");

    uint32 count = 0;

    WorldPacket data(SMSG_QUESTGIVER_STATUS_MULTIPLE, 4);
    data << uint32(count);                                  // placeholder

    for(ObjectGuidSet::const_iterator itr = _player->m_clientGUIDs.begin(); itr != _player->m_clientGUIDs.end(); ++itr)
    {
        uint8 dialogStatus = DIALOG_STATUS_NONE;

        if (itr->IsAnyTypeCreature())
        {
            // need also pet quests case support
            Creature *questgiver = GetPlayer()->GetMap()->GetAnyTypeCreature(*itr);

            if (!questgiver || questgiver->IsHostileTo(_player))
                continue;

            if (!questgiver->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
                continue;

            dialogStatus = sScriptMgr.GetDialogStatus(_player, questgiver);

            if (dialogStatus > DIALOG_STATUS_REWARD_REP)
                dialogStatus = getDialogStatus(_player, questgiver, DIALOG_STATUS_NONE);

            data << questgiver->GetObjectGuid();
            data << uint8(dialogStatus);
            ++count;
        }
        else if (itr->IsGameObject())
        {
            GameObject *questgiver = GetPlayer()->GetMap()->GetGameObject(*itr);

            if (!questgiver)
                continue;

            if (questgiver->GetGoType() != GAMEOBJECT_TYPE_QUESTGIVER)
                continue;

            dialogStatus = sScriptMgr.GetDialogStatus(_player, questgiver);

            if (dialogStatus > DIALOG_STATUS_REWARD_REP)
                dialogStatus = getDialogStatus(_player, questgiver, DIALOG_STATUS_NONE);

            data << questgiver->GetObjectGuid();
            data << uint8(dialogStatus);
            ++count;
        }
    }

    data.put<uint32>(0, count);                             // write real count
    SendPacket(&data);
}
