/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "GossipDef.h"
#include "UpdateMask.h"
#include "ScriptCalls.h"
#include "ObjectAccessor.h"
#include "Creature.h"
#include "Pet.h"
#include "Guild.h"
#include "Chat.h"

enum StableResultCode
{
    STABLE_ERR_MONEY        = 0x01,                         // "you don't have enough money"
    STABLE_ERR_STABLE       = 0x06,                         // currently used in most fail cases
    STABLE_SUCCESS_STABLE   = 0x08,                         // stable success
    STABLE_SUCCESS_UNSTABLE = 0x09,                         // unstable/swap success
    STABLE_SUCCESS_BUY_SLOT = 0x0A,                         // buy slot success
    STABLE_ERR_EXOTIC       = 0x0C,                         // "you are unable to control exotic creatures"
};

void WorldSession::HandleTabardVendorActivateOpcode( WorldPacket & recv_data )
{
    uint64 guid;
    recv_data >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid,UNIT_NPC_FLAG_TABARDDESIGNER);
    if (!unit)
    {
        DEBUG_LOG( "WORLD: HandleTabardVendorActivateOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)) );
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    SendTabardVendorActivate(guid);
}

void WorldSession::SendTabardVendorActivate( uint64 guid )
{
    WorldPacket data( MSG_TABARDVENDOR_ACTIVATE, 8 );
    data << guid;
    SendPacket( &data );
}

void WorldSession::HandleBankerActivateOpcode( WorldPacket & recv_data )
{
    uint64 guid;

    DEBUG_LOG(  "WORLD: Received CMSG_BANKER_ACTIVATE" );

    recv_data >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid,UNIT_NPC_FLAG_BANKER);
    if (!unit)
    {
        DEBUG_LOG( "WORLD: HandleBankerActivateOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)) );
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    SendShowBank(guid);
}

void WorldSession::SendShowBank( uint64 guid )
{
    WorldPacket data( SMSG_SHOW_BANK, 8 );
    data << guid;
    SendPacket( &data );
}

void WorldSession::HandleTrainerListOpcode( WorldPacket & recv_data )
{
    uint64 guid;

    recv_data >> guid;
    SendTrainerList( guid );
}

void WorldSession::SendTrainerList( uint64 guid )
{
    std::string str = GetMangosString(LANG_NPC_TAINER_HELLO);
    SendTrainerList( guid, str );
}

void WorldSession::SendTrainerList( uint64 guid, const std::string& strTitle )
{
    DEBUG_LOG( "WORLD: SendTrainerList" );

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid,UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        DEBUG_LOG( "WORLD: SendTrainerList - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)) );
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    // trainer list loaded at check;
    if(!unit->isCanTrainingOf(_player,true))
        return;

    CreatureInfo const *ci = unit->GetCreatureInfo();

    if (!ci)
    {
        DEBUG_LOG( "WORLD: SendTrainerList - (GUID: %u) NO CREATUREINFO!",GUID_LOPART(guid) );
        return;
    }

    TrainerSpellData const* trainer_spells = unit->GetTrainerSpells();
    if(!trainer_spells)
    {
        DEBUG_LOG( "WORLD: SendTrainerList - Training spells not found for creature (GUID: %u Entry: %u)",
            GUID_LOPART(guid), unit->GetEntry());
        return;
    }

    WorldPacket data( SMSG_TRAINER_LIST, 8+4+4+trainer_spells->spellList.size()*38 + strTitle.size()+1);
    data << uint64(guid);
    data << uint32(trainer_spells->trainerType);

    size_t count_pos = data.wpos();
    data << uint32(trainer_spells->spellList.size());

    // reputation discount
    float fDiscountMod = _player->GetReputationPriceDiscount(unit);
    bool can_learn_primary_prof = GetPlayer()->GetFreePrimaryProfessionPoints() > 0;

    uint32 count = 0;
    for(TrainerSpellMap::const_iterator itr = trainer_spells->spellList.begin(); itr != trainer_spells->spellList.end(); ++itr)
    {
        TrainerSpell const* tSpell = &itr->second;

        if(!_player->IsSpellFitByClassAndRace(tSpell->learnedSpell))
            continue;

        bool primary_prof_first_rank = sSpellMgr.IsPrimaryProfessionFirstRankSpell(tSpell->learnedSpell);
        SpellChainNode const* chain_node = sSpellMgr.GetSpellChainNode(tSpell->learnedSpell);
        TrainerSpellState state = _player->GetTrainerSpellState(tSpell);

        data << uint32(tSpell->spell);                      // learned spell (or cast-spell in profession case)
        data << uint8(state==TRAINER_SPELL_GREEN_DISABLED ? TRAINER_SPELL_GREEN : state);
        data << uint32(floor(tSpell->spellCost * fDiscountMod));

        data << uint32(primary_prof_first_rank && can_learn_primary_prof ? 1 : 0);
                                                            // primary prof. learn confirmation dialog
        data << uint32(primary_prof_first_rank ? 1 : 0);    // must be equal prev. field to have learn button in enabled state
        data << uint8(tSpell->reqLevel);
        data << uint32(tSpell->reqSkill);
        data << uint32(tSpell->reqSkillValue);
        data << uint32(!tSpell->IsCastable() && chain_node ? (chain_node->prev ? chain_node->prev : chain_node->req) : 0);
        data << uint32(!tSpell->IsCastable() && chain_node && chain_node->prev ? chain_node->req : 0);
        data << uint32(0);

        ++count;
    }

    data << strTitle;

    data.put<uint32>(count_pos,count);
    SendPacket( &data );
}

void WorldSession::HandleTrainerBuySpellOpcode( WorldPacket & recv_data )
{
    uint64 guid;
    uint32 spellId = 0;

    recv_data >> guid >> spellId;
    DEBUG_LOG( "WORLD: Received CMSG_TRAINER_BUY_SPELL NpcGUID=%u, learn spell id is: %u",uint32(GUID_LOPART(guid)), spellId );

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        DEBUG_LOG( "WORLD: HandleTrainerBuySpellOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)) );
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    if(!unit->isCanTrainingOf(_player,true))
        return;

    // check present spell in trainer spell list
    TrainerSpellData const* trainer_spells = unit->GetTrainerSpells();
    if(!trainer_spells)
        return;

    // not found, cheat?
    TrainerSpell const* trainer_spell = trainer_spells->Find(spellId);
    if(!trainer_spell)
        return;

    // can't be learn, cheat? Or double learn with lags...
    if(_player->GetTrainerSpellState(trainer_spell) != TRAINER_SPELL_GREEN)
        return;

    // apply reputation discount
    uint32 nSpellCost = uint32(floor(trainer_spell->spellCost * _player->GetReputationPriceDiscount(unit)));

    // check money requirement
    if(_player->GetMoney() < nSpellCost )
        return;

    _player->ModifyMoney( -int32(nSpellCost) );

    WorldPacket data(SMSG_PLAY_SPELL_VISUAL, 12);           // visual effect on trainer
    data << uint64(guid);
    data << uint32(0xB3);                                   // index from SpellVisualKit.dbc
    SendPacket(&data);

    data.Initialize(SMSG_PLAY_SPELL_IMPACT, 12);            // visual effect on player
    data << uint64(_player->GetGUID());
    data << uint32(0x016A);                                 // index from SpellVisualKit.dbc
    SendPacket(&data);

    // learn explicitly or cast explicitly
    if(trainer_spell->IsCastable())
        _player->CastSpell(_player, trainer_spell->spell, true);
    else
        _player->learnSpell(spellId, false);

    data.Initialize(SMSG_TRAINER_BUY_SUCCEEDED, 12);
    data << uint64(guid);
    data << uint32(spellId);                                // should be same as in packet from client
    SendPacket(&data);
}

void WorldSession::HandleGossipHelloOpcode(WorldPacket & recv_data)
{
    DEBUG_LOG("WORLD: Received CMSG_GOSSIP_HELLO");

    uint64 guid;
    recv_data >> guid;

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);

    if (!pCreature)
    {
        DEBUG_LOG("WORLD: HandleGossipHelloOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    if (!pCreature->IsStopped())
        pCreature->StopMoving();

    if (pCreature->isSpiritGuide())
        pCreature->SendAreaSpiritHealerQueryOpcode(_player);

    if (!Script->GossipHello(_player, pCreature))
    {
        _player->PrepareGossipMenu(pCreature, pCreature->GetCreatureInfo()->GossipMenuId);
        _player->SendPreparedGossip(pCreature);
    }
}

void WorldSession::HandleGossipSelectOptionOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_GOSSIP_SELECT_OPTION");

    uint32 gossipListId;
    uint32 menuId;
    ObjectGuid guid;
    std::string code = "";

    recv_data >> guid >> menuId >> gossipListId;

    if (_player->PlayerTalkClass->GossipOptionCoded(gossipListId))
    {
        recv_data >> code;
        DEBUG_LOG("Gossip code: %s", code.c_str());
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    // can vehicle have gossip? If so, need check for this also.
    if (guid.IsCreatureOrPet())
    {
        Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);

        if (!pCreature)
        {
            DEBUG_LOG("WORLD: HandleGossipSelectOptionOpcode - %s not found or you can't interact with it.", guid.GetString().c_str());
            return;
        }

        if (!code.empty())
        {
            if (!Script->GossipSelectWithCode(_player, pCreature, _player->PlayerTalkClass->GossipOptionSender(gossipListId), _player->PlayerTalkClass->GossipOptionAction(gossipListId), code.c_str()))
                _player->OnGossipSelect(pCreature, gossipListId, menuId);
        }
        else
        {
            if (!Script->GossipSelect(_player, pCreature, _player->PlayerTalkClass->GossipOptionSender(gossipListId), _player->PlayerTalkClass->GossipOptionAction(gossipListId)))
                _player->OnGossipSelect(pCreature, gossipListId, menuId);
        }
    }
    else if (guid.IsGameobject())
    {
        GameObject *pGo = GetPlayer()->GetGameObjectIfCanInteractWith(guid);

        if (!pGo)
        {
            DEBUG_LOG("WORLD: HandleGossipSelectOptionOpcode - %s not found or you can't interact with it.", guid.GetString().c_str());
            return;
        }

        if (!code.empty())
        {
            if (!Script->GOGossipSelectWithCode(_player, pGo, _player->PlayerTalkClass->GossipOptionSender(gossipListId), _player->PlayerTalkClass->GossipOptionAction(gossipListId), code.c_str()))
                _player->OnGossipSelect(pGo, gossipListId, menuId);
        }
        else
        {
            if (!Script->GOGossipSelect(_player, pGo, _player->PlayerTalkClass->GossipOptionSender(gossipListId), _player->PlayerTalkClass->GossipOptionAction(gossipListId)))
                _player->OnGossipSelect(pGo, gossipListId, menuId);
        }
    }
}

void WorldSession::HandleSpiritHealerActivateOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_SPIRIT_HEALER_ACTIVATE");

    uint64 guid;

    recv_data >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_SPIRITHEALER);
    if (!unit)
    {
        DEBUG_LOG( "WORLD: HandleSpiritHealerActivateOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)) );
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    SendSpiritResurrect();
}

void WorldSession::SendSpiritResurrect()
{
    _player->ResurrectPlayer(0.5f, true);

    _player->DurabilityLossAll(0.25f,true);

    // get corpse nearest graveyard
    WorldSafeLocsEntry const *corpseGrave = NULL;
    Corpse *corpse = _player->GetCorpse();
    if(corpse)
        corpseGrave = sObjectMgr.GetClosestGraveYard(
            corpse->GetPositionX(), corpse->GetPositionY(), corpse->GetPositionZ(), corpse->GetMapId(), _player->GetTeam() );

    // now can spawn bones
    _player->SpawnCorpseBones();

    // teleport to nearest from corpse graveyard, if different from nearest to player ghost
    if(corpseGrave)
    {
        WorldSafeLocsEntry const *ghostGrave = sObjectMgr.GetClosestGraveYard(
            _player->GetPositionX(), _player->GetPositionY(), _player->GetPositionZ(), _player->GetMapId(), _player->GetTeam() );

        if(corpseGrave != ghostGrave)
            _player->TeleportTo(corpseGrave->map_id, corpseGrave->x, corpseGrave->y, corpseGrave->z, _player->GetOrientation());
        // or update at original position
        else
        {
            _player->GetCamera().UpdateVisibilityForOwner();
            _player->UpdateObjectVisibility();
        }
    }
    // or update at original position
    else
    {
        _player->GetCamera().UpdateVisibilityForOwner();
        _player->UpdateObjectVisibility();
    }
}

void WorldSession::HandleBinderActivateOpcode( WorldPacket & recv_data )
{
    uint64 npcGUID;
    recv_data >> npcGUID;

    if(!GetPlayer()->IsInWorld() || !GetPlayer()->isAlive())
        return;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(npcGUID,UNIT_NPC_FLAG_INNKEEPER);
    if (!unit)
    {
        DEBUG_LOG( "WORLD: HandleBinderActivateOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(npcGUID)) );
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    SendBindPoint(unit);
}

void WorldSession::SendBindPoint(Creature *npc)
{
    // prevent set homebind to instances in any case
    if(GetPlayer()->GetMap()->Instanceable())
        return;

    // send spell for bind 3286 bind magic
    npc->CastSpell(_player, 3286, true);                    // Bind

    WorldPacket data( SMSG_TRAINER_BUY_SUCCEEDED, (8+4));
    data << uint64(npc->GetGUID());
    data << uint32(3286);                                   // Bind
    SendPacket( &data );

    _player->PlayerTalkClass->CloseGossip();
}

void WorldSession::HandleListStabledPetsOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: Recv MSG_LIST_STABLED_PETS");
    ObjectGuid npcGUID;

    recv_data >> npcGUID;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(npcGUID, UNIT_NPC_FLAG_STABLEMASTER);
    if (!unit)
    {
        DEBUG_LOG( "WORLD: HandleListStabledPetsOpcode - %s not found or you can't interact with him.", npcGUID.GetString().c_str());
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    SendStablePet(npcGUID);
}

void WorldSession::SendStablePet( ObjectGuid guid )
{
    DEBUG_LOG("WORLD: Recv MSG_LIST_STABLED_PETS Send.");

    WorldPacket data(MSG_LIST_STABLED_PETS, 200);           // guess size
    data << guid;

    Pet *pet = _player->GetPet();

    size_t wpos = data.wpos();
    data << uint8(0);                                       // place holder for slot show number

    data << uint8(GetPlayer()->m_stableSlots);

    uint8 num = 0;                                          // counter for place holder

    // not let move dead pet in slot
    if(pet && pet->isAlive() && pet->getPetType()==HUNTER_PET)
    {
        data << uint32(pet->GetCharmInfo()->GetPetNumber());
        data << uint32(pet->GetEntry());
        data << uint32(pet->getLevel());
        data << pet->GetName();                             // petname
        data << uint8(1);                                   // 1 = current, 2/3 = in stable (any from 4,5,... create problems with proper show)
        ++num;
    }

    //                                                     0      1   2      3      4
    QueryResult* result = CharacterDatabase.PQuery("SELECT owner, id, entry, level, name FROM character_pet WHERE owner = '%u' AND slot >= '%u' AND slot <= '%u' ORDER BY slot",
        _player->GetGUIDLow(),PET_SAVE_FIRST_STABLE_SLOT,PET_SAVE_LAST_STABLE_SLOT);

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();

            data << uint32(fields[1].GetUInt32());          // petnumber
            data << uint32(fields[2].GetUInt32());          // creature entry
            data << uint32(fields[3].GetUInt32());          // level
            data << fields[4].GetString();                  // name
            data << uint8(2);                               // 1 = current, 2/3 = in stable (any from 4,5,... create problems with proper show)

            ++num;
        }while( result->NextRow() );

        delete result;
    }

    data.put<uint8>(wpos, num);                             // set real data to placeholder
    SendPacket(&data);
}

void WorldSession::SendStableResult(uint8 res)
{
    WorldPacket data(SMSG_STABLE_RESULT, 1);
    data << uint8(res);
    SendPacket(&data);
}

bool WorldSession::CheckStableMaster(ObjectGuid guid)
{
    // spell case or GM
    if (guid == GetPlayer()->GetObjectGuid())
    {
        // command case will return only if player have real access to command
        if (!GetPlayer()->HasAuraType(SPELL_AURA_OPEN_STABLE) && !ChatHandler(GetPlayer()).FindCommand("stable"))
        {
            DEBUG_LOG("%s attempt open stable in cheating way.", guid.GetString().c_str());
            return false;
        }
    }
    // stable master case
    else
    {
        if (!GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_STABLEMASTER))
        {
            DEBUG_LOG("Stablemaster %s not found or you can't interact with him.", guid.GetString().c_str());
            return false;
        }
    }

    return true;
}

void WorldSession::HandleStablePet( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: Recv CMSG_STABLE_PET");
    ObjectGuid npcGUID;

    recv_data >> npcGUID;

    if(!GetPlayer()->isAlive())
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    if (!CheckStableMaster(npcGUID))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    Pet *pet = _player->GetPet();

    // can't place in stable dead pet
    if(!pet||!pet->isAlive()||pet->getPetType()!=HUNTER_PET)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    uint32 free_slot = 1;

    QueryResult *result = CharacterDatabase.PQuery("SELECT owner,slot,id FROM character_pet WHERE owner = '%u'  AND slot >= '%u' AND slot <= '%u' ORDER BY slot ",
        _player->GetGUIDLow(),PET_SAVE_FIRST_STABLE_SLOT,PET_SAVE_LAST_STABLE_SLOT);
    if(result)
    {
        do
        {
            Field *fields = result->Fetch();

            uint32 slot = fields[1].GetUInt32();

            // slots ordered in query, and if not equal then free
            if(slot!=free_slot)
                break;

            // this slot not free, skip
            ++free_slot;
        }while( result->NextRow() );

        delete result;
    }

    if( free_slot > 0 && free_slot <= GetPlayer()->m_stableSlots)
    {
        _player->RemovePet(pet,PetSaveMode(free_slot));
        SendStableResult(STABLE_SUCCESS_STABLE);
    }
    else
        SendStableResult(STABLE_ERR_STABLE);
}

void WorldSession::HandleUnstablePet( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: Recv CMSG_UNSTABLE_PET.");
    ObjectGuid npcGUID;
    uint32 petnumber;

    recv_data >> npcGUID >> petnumber;

    if (!CheckStableMaster(npcGUID))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    uint32 creature_id = 0;

    {
        QueryResult *result = CharacterDatabase.PQuery("SELECT entry FROM character_pet WHERE owner = '%u' AND id = '%u' AND slot >='%u' AND slot <= '%u'",
            _player->GetGUIDLow(),petnumber,PET_SAVE_FIRST_STABLE_SLOT,PET_SAVE_LAST_STABLE_SLOT);
        if(result)
        {
            Field *fields = result->Fetch();
            creature_id = fields[0].GetUInt32();
            delete result;
        }
    }

    if(!creature_id)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    CreatureInfo const* creatureInfo = ObjectMgr::GetCreatureTemplate(creature_id);
    if(!creatureInfo || !creatureInfo->isTameable(_player->CanTameExoticPets()))
    {
        // if problem in exotic pet
        if (creatureInfo && creatureInfo->isTameable(true))
            SendStableResult(STABLE_ERR_EXOTIC);
        else
            SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    Pet* pet = _player->GetPet();
    if(pet && pet->isAlive())
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // delete dead pet
    if(pet)
        _player->RemovePet(pet,PET_SAVE_AS_DELETED);

    Pet *newpet = new Pet(HUNTER_PET);
    if(!newpet->LoadPetFromDB(_player,creature_id,petnumber))
    {
        delete newpet;
        newpet = NULL;
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    SendStableResult(STABLE_SUCCESS_UNSTABLE);
}

void WorldSession::HandleBuyStableSlot( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: Recv CMSG_BUY_STABLE_SLOT.");
    ObjectGuid npcGUID;

    recv_data >> npcGUID;

    if (!CheckStableMaster(npcGUID))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    if(GetPlayer()->m_stableSlots < MAX_PET_STABLES)
    {
        StableSlotPricesEntry const *SlotPrice = sStableSlotPricesStore.LookupEntry(GetPlayer()->m_stableSlots+1);
        if(_player->GetMoney() >= SlotPrice->Price)
        {
            ++GetPlayer()->m_stableSlots;
            _player->ModifyMoney(-int32(SlotPrice->Price));
            SendStableResult(STABLE_SUCCESS_BUY_SLOT);
        }
        else
            SendStableResult(STABLE_ERR_MONEY);
    }
    else
        SendStableResult(STABLE_ERR_STABLE);
}

void WorldSession::HandleStableRevivePet( WorldPacket &/* recv_data */)
{
    DEBUG_LOG("HandleStableRevivePet: Not implemented");
}

void WorldSession::HandleStableSwapPet( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: Recv CMSG_STABLE_SWAP_PET.");
    ObjectGuid npcGUID;
    uint32 pet_number;

    recv_data >> npcGUID >> pet_number;

    if (!CheckStableMaster(npcGUID))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);


    Pet* pet = _player->GetPet();

    if(!pet || pet->getPetType()!=HUNTER_PET)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // find swapped pet slot in stable
    QueryResult *result = CharacterDatabase.PQuery("SELECT slot,entry FROM character_pet WHERE owner = '%u' AND id = '%u'",
        _player->GetGUIDLow(),pet_number);
    if(!result)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    Field *fields = result->Fetch();

    uint32 slot        = fields[0].GetUInt32();
    uint32 creature_id = fields[1].GetUInt32();
    delete result;

    if(!creature_id)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    CreatureInfo const* creatureInfo = ObjectMgr::GetCreatureTemplate(creature_id);
    if(!creatureInfo || !creatureInfo->isTameable(_player->CanTameExoticPets()))
    {
        // if problem in exotic pet
        if (creatureInfo && creatureInfo->isTameable(true))
            SendStableResult(STABLE_ERR_EXOTIC);
        else
            SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // move alive pet to slot or delete dead pet
    _player->RemovePet(pet,pet->isAlive() ? PetSaveMode(slot) : PET_SAVE_AS_DELETED);

    // summon unstabled pet
    Pet *newpet = new Pet;
    if(!newpet->LoadPetFromDB(_player,creature_id,pet_number))
    {
        delete newpet;
        SendStableResult(STABLE_ERR_STABLE);
    }
    else
        SendStableResult(STABLE_SUCCESS_UNSTABLE);
}

void WorldSession::HandleRepairItemOpcode( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_REPAIR_ITEM");

    uint64 npcGUID, itemGUID;
    uint8 guildBank;                                        // new in 2.3.2, bool that means from guild bank money

    recv_data >> npcGUID >> itemGUID >> guildBank;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(npcGUID, UNIT_NPC_FLAG_REPAIR);
    if (!unit)
    {
        DEBUG_LOG( "WORLD: HandleRepairItemOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(npcGUID)) );
        return;
    }

    // remove fake death
    if(GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    // reputation discount
    float discountMod = _player->GetReputationPriceDiscount(unit);

    uint32 TotalCost = 0;
    if (itemGUID)
    {
        DEBUG_LOG("ITEM: Repair item, itemGUID = %u, npcGUID = %u", GUID_LOPART(itemGUID), GUID_LOPART(npcGUID));

        Item* item = _player->GetItemByGuid(itemGUID);

        if(item)
            TotalCost= _player->DurabilityRepair(item->GetPos(),true,discountMod,guildBank>0?true:false);
    }
    else
    {
        DEBUG_LOG("ITEM: Repair all items, npcGUID = %u", GUID_LOPART(npcGUID));

        TotalCost = _player->DurabilityRepairAll(true,discountMod,guildBank>0?true:false);
    }
    if (guildBank)
    {
        uint32 GuildId = _player->GetGuildId();
        if (!GuildId)
            return;
        Guild *pGuild = sObjectMgr.GetGuildById(GuildId);
        if (!pGuild)
            return;
        pGuild->LogBankEvent(GUILD_BANK_LOG_REPAIR_MONEY, 0, _player->GetGUIDLow(), TotalCost);
        pGuild->SendMoneyInfo(this, _player->GetGUIDLow());
    }
}
