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
#include "WorldPacket.h"
#include "WorldSession.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Spell.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "Util.h"
#include "Pet.h"

void WorldSession::HandlePetAction( WorldPacket & recv_data )
{
    uint64 guid1;
    uint32 data;
    uint64 guid2;
    recv_data >> guid1;                                     //pet guid
    recv_data >> data;
    recv_data >> guid2;                                     //tag guid

    uint32 spellid = UNIT_ACTION_BUTTON_ACTION(data);
    uint8 flag = UNIT_ACTION_BUTTON_TYPE(data);             //delete = 0x07 CastSpell = C1

    // used also for charmed creature
    Unit* pet= ObjectAccessor::GetUnit(*_player, guid1);
    DETAIL_LOG("HandlePetAction.Pet %u flag is %u, spellid is %u, target %u.", uint32(GUID_LOPART(guid1)), uint32(flag), spellid, uint32(GUID_LOPART(guid2)) );
    if (!pet)
    {
        sLog.outError( "Pet %u not exist.", uint32(GUID_LOPART(guid1)) );
        return;
    }

    if (pet != GetPlayer()->GetPet() && pet != GetPlayer()->GetCharm())
    {
        sLog.outError("HandlePetAction.Pet %u isn't pet of player %s.", uint32(GUID_LOPART(guid1)), GetPlayer()->GetName() );
        return;
    }

    if (!pet->isAlive())
        return;

    if (pet->GetTypeId() == TYPEID_PLAYER)
    {
        // controller player can only do melee attack
        if (!(flag == ACT_COMMAND && spellid == COMMAND_ATTACK))
            return;
    }
    else if (((Creature*)pet)->isPet())
    {
        // pet can have action bar disabled
        if(((Pet*)pet)->GetModeFlags() & PET_MODE_DISABLE_ACTIONS)
            return;
    }

    CharmInfo *charmInfo = pet->GetCharmInfo();
    if(!charmInfo)
    {
        sLog.outError("WorldSession::HandlePetAction: object (GUID: %u TypeId: %u) is considered pet-like but doesn't have a charminfo!", pet->GetGUIDLow(), pet->GetTypeId());
        return;
    }

    switch(flag)
    {
        case ACT_COMMAND:                                   //0x07
            switch(spellid)
            {
                case COMMAND_STAY:                          //flat=1792  //STAY
                    pet->StopMoving();
                    pet->GetMotionMaster()->Clear(false);
                    pet->GetMotionMaster()->MoveIdle();
                    charmInfo->SetCommandState( COMMAND_STAY );
                    break;
                case COMMAND_FOLLOW:                        //spellid=1792  //FOLLOW
                    pet->AttackStop();
                    pet->GetMotionMaster()->MoveFollow(_player,PET_FOLLOW_DIST,PET_FOLLOW_ANGLE);
                    charmInfo->SetCommandState( COMMAND_FOLLOW );
                    break;
                case COMMAND_ATTACK:                        //spellid=1792  //ATTACK
                {
                    const uint64& selguid = _player->GetSelection();
                    Unit *TargetUnit = ObjectAccessor::GetUnit(*_player, selguid);
                    if(!TargetUnit)
                        return;

                    // not let attack friendly units.
                    if(GetPlayer()->IsFriendlyTo(TargetUnit))
                        return;
                    // Not let attack through obstructions
                    if(!pet->IsWithinLOSInMap(TargetUnit))
                        return;

                    // This is true if pet has no target or has target but targets differs.
                    if(pet->getVictim() != TargetUnit)
                    {
                        if (pet->getVictim())
                            pet->AttackStop();

                        if(pet->GetTypeId() != TYPEID_PLAYER)
                        {
                            pet->GetMotionMaster()->Clear();
                            if (((Creature*)pet)->AI())
                                ((Creature*)pet)->AI()->AttackStart(TargetUnit);

                            //10% chance to play special pet attack talk, else growl
                            if(((Creature*)pet)->isPet() && ((Pet*)pet)->getPetType() == SUMMON_PET && pet != TargetUnit && urand(0, 100) < 10)
                                pet->SendPetTalk((uint32)PET_TALK_ATTACK);
                            else
                            {
                                // 90% chance for pet and 100% chance for charmed creature
                                pet->SendPetAIReaction(guid1);
                            }
                        }
                        else                                // charmed player
                        {
                            pet->Attack(TargetUnit,true);
                            pet->SendPetAIReaction(guid1);
                        }
                    }
                    break;
                }
                case COMMAND_ABANDON:                       // abandon (hunter pet) or dismiss (summoned pet)
                    if(((Creature*)pet)->isPet())
                    {
                        Pet* p = (Pet*)pet;
                        if(p->getPetType() == HUNTER_PET)
                            _player->RemovePet(p,PET_SAVE_AS_DELETED);
                        else
                            //dismissing a summoned pet is like killing them (this prevents returning a soulshard...)
                            p->setDeathState(CORPSE);
                    }
                    else                                    // charmed
                        _player->Uncharm();
                    break;
                default:
                    sLog.outError("WORLD: unknown PET flag Action %i and spellid %i.", uint32(flag), spellid);
            }
            break;
        case ACT_REACTION:                                  // 0x6
            switch(spellid)
            {
                case REACT_PASSIVE:                         //passive
                case REACT_DEFENSIVE:                       //recovery
                case REACT_AGGRESSIVE:                      //activete
                    charmInfo->SetReactState( ReactStates(spellid) );
                    break;
            }
            break;
        case ACT_DISABLED:                                  // 0x81    spell (disabled), ignore
        case ACT_PASSIVE:                                   // 0x01
        case ACT_ENABLED:                                   // 0xC1    spell
        {
            Unit* unit_target = NULL;
            if (((Creature*)pet)->GetGlobalCooldown() > 0)
                return;

            if(guid2)
                unit_target = ObjectAccessor::GetUnit(*_player,guid2);

            // do not cast unknown spells
            SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellid );
            if(!spellInfo)
            {
                sLog.outError("WORLD: unknown PET spell id %i", spellid);
                return;
            }

            for(int i = 0; i < MAX_EFFECT_INDEX;++i)
            {
                if(spellInfo->EffectImplicitTargetA[i] == TARGET_ALL_ENEMY_IN_AREA || spellInfo->EffectImplicitTargetA[i] == TARGET_ALL_ENEMY_IN_AREA_INSTANT || spellInfo->EffectImplicitTargetA[i] == TARGET_ALL_ENEMY_IN_AREA_CHANNELED)
                    return;
            }

            // do not cast not learned spells
            if(!pet->HasSpell(spellid) || IsPassiveSpell(spellInfo))
                return;

            pet->clearUnitState(UNIT_STAT_MOVING);

            Spell *spell = new Spell(pet, spellInfo, false);

            SpellCastResult result = spell->CheckPetCast(unit_target);

            //auto turn to target unless possessed
            if(result == SPELL_FAILED_UNIT_NOT_INFRONT && !pet->HasAuraType(SPELL_AURA_MOD_POSSESS))
            {
                if(unit_target)
                {
                    pet->SetInFront(unit_target);
                    if (unit_target->GetTypeId() == TYPEID_PLAYER)
                        pet->SendCreateUpdateToPlayer( (Player*)unit_target );
                }
                else if(Unit *unit_target2 = spell->m_targets.getUnitTarget())
                {
                    pet->SetInFront(unit_target2);
                    if (unit_target2->GetTypeId() == TYPEID_PLAYER)
                        pet->SendCreateUpdateToPlayer( (Player*)unit_target2 );
                }
                if (Unit* powner = pet->GetCharmerOrOwner())
                    if(powner->GetTypeId() == TYPEID_PLAYER)
                        pet->SendCreateUpdateToPlayer((Player*)powner);
                result = SPELL_CAST_OK;
            }

            if(result == SPELL_CAST_OK)
            {
                ((Creature*)pet)->AddCreatureSpellCooldown(spellid);

                unit_target = spell->m_targets.getUnitTarget();

                //10% chance to play special pet attack talk, else growl
                //actually this only seems to happen on special spells, fire shield for imp, torment for voidwalker, but it's stupid to check every spell
                if(((Creature*)pet)->isPet() && (((Pet*)pet)->getPetType() == SUMMON_PET) && (pet != unit_target) && (urand(0, 100) < 10))
                    pet->SendPetTalk((uint32)PET_TALK_SPECIAL_SPELL);
                else
                {
                    pet->SendPetAIReaction(guid1);
                }

                if( unit_target && !GetPlayer()->IsFriendlyTo(unit_target) && !pet->HasAuraType(SPELL_AURA_MOD_POSSESS))
                {
                    // This is true if pet has no target or has target but targets differs.
                    if (pet->getVictim() != unit_target)
                    {
                        if (pet->getVictim())
                            pet->AttackStop();
                        pet->GetMotionMaster()->Clear();
                        if (((Creature*)pet)->AI())
                            ((Creature*)pet)->AI()->AttackStart(unit_target);
                    }
                }

                spell->prepare(&(spell->m_targets));
            }
            else
            {
                if(pet->HasAuraType(SPELL_AURA_MOD_POSSESS))
                    Spell::SendCastResult(GetPlayer(),spellInfo,0,result);
                else
                    pet->SendPetCastFail(spellid, result);

                if (!((Creature*)pet)->HasSpellCooldown(spellid))
                    GetPlayer()->SendClearCooldown(spellid, pet);

                spell->finish(false);
                delete spell;
            }
            break;
        }
        default:
            sLog.outError("WORLD: unknown PET flag Action %i and spellid %i.", uint32(flag), spellid);
    }
}

void WorldSession::HandlePetNameQuery( WorldPacket & recv_data )
{
    DETAIL_LOG( "HandlePetNameQuery. CMSG_PET_NAME_QUERY" );

    uint32 petnumber;
    uint64 petguid;

    recv_data >> petnumber;
    recv_data >> petguid;

    SendPetNameQuery(petguid,petnumber);
}

void WorldSession::SendPetNameQuery( uint64 petguid, uint32 petnumber)
{
    Creature* pet = _player->GetMap()->GetCreatureOrPetOrVehicle(petguid);
    if(!pet || !pet->GetCharmInfo() || pet->GetCharmInfo()->GetPetNumber() != petnumber)
        return;

    std::string name = pet->GetName();

    WorldPacket data(SMSG_PET_NAME_QUERY_RESPONSE, (4+4+name.size()+1));
    data << uint32(petnumber);
    data << name.c_str();
    data << uint32(pet->GetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP));

    if( pet->isPet() && ((Pet*)pet)->GetDeclinedNames() )
    {
        data << uint8(1);
        for(int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            data << ((Pet*)pet)->GetDeclinedNames()->name[i];
    }
    else
        data << uint8(0);

    _player->GetSession()->SendPacket(&data);
}

void WorldSession::HandlePetSetAction( WorldPacket & recv_data )
{
    DETAIL_LOG( "HandlePetSetAction. CMSG_PET_SET_ACTION" );

    uint64 petguid;
    uint8  count;

    recv_data >> petguid;

    Creature* pet = _player->GetMap()->GetCreatureOrPetOrVehicle(petguid);

    if(!pet || (pet != _player->GetPet() && pet != _player->GetCharm()))
    {
        sLog.outError( "HandlePetSetAction: Unknown pet or pet owner." );
        return;
    }

    // pet can have action bar disabled
    if(pet->isPet() && ((Pet*)pet)->GetModeFlags() & PET_MODE_DISABLE_ACTIONS)
        return;

    CharmInfo *charmInfo = pet->GetCharmInfo();
    if(!charmInfo)
    {
        sLog.outError("WorldSession::HandlePetSetAction: object (GUID: %u TypeId: %u) is considered pet-like but doesn't have a charminfo!", pet->GetGUIDLow(), pet->GetTypeId());
        return;
    }

    count = (recv_data.size() == 24) ? 2 : 1;

    uint32 position[2];
    uint32 data[2];
    bool move_command = false;

    for(uint8 i = 0; i < count; ++i)
    {
        recv_data >> position[i];
        recv_data >> data[i];

        uint8 act_state = UNIT_ACTION_BUTTON_TYPE(data[i]);

        //ignore invalid position
        if(position[i] >= MAX_UNIT_ACTION_BAR_INDEX)
            return;

        // in the normal case, command and reaction buttons can only be moved, not removed
        // at moving count ==2, at removing count == 1
        // ignore attempt to remove command|reaction buttons (not possible at normal case)
        if (act_state == ACT_COMMAND || act_state == ACT_REACTION)
        {
            if (count == 1)
                return;

            move_command = true;
        }
    }

    // check swap (at command->spell swap client remove spell first in another packet, so check only command move correctness)
    if (move_command)
    {
        uint8 act_state_0 = UNIT_ACTION_BUTTON_TYPE(data[0]);
        if(act_state_0 == ACT_COMMAND || act_state_0 == ACT_REACTION)
        {
            uint32 spell_id_0 = UNIT_ACTION_BUTTON_ACTION(data[0]);
            UnitActionBarEntry const* actionEntry_1 = charmInfo->GetActionBarEntry(position[1]);
            if (!actionEntry_1 || spell_id_0 != actionEntry_1->GetAction() ||
                act_state_0 != actionEntry_1->GetType())
                return;
        }

        uint8 act_state_1 = UNIT_ACTION_BUTTON_TYPE(data[1]);
        if(act_state_1 == ACT_COMMAND || act_state_1 == ACT_REACTION)
        {
            uint32 spell_id_1 = UNIT_ACTION_BUTTON_ACTION(data[1]);
            UnitActionBarEntry const* actionEntry_0 = charmInfo->GetActionBarEntry(position[0]);
            if (!actionEntry_0 || spell_id_1 != actionEntry_0->GetAction() ||
                act_state_1 != actionEntry_0->GetType())
                return;
        }
    }

    for(uint8 i = 0; i < count; ++i)
    {
        uint32 spell_id = UNIT_ACTION_BUTTON_ACTION(data[i]);
        uint8 act_state = UNIT_ACTION_BUTTON_TYPE(data[i]);

        DETAIL_LOG( "Player %s has changed pet spell action. Position: %u, Spell: %u, State: 0x%X", _player->GetName(), position[i], spell_id, uint32(act_state));

        //if it's act for spell (en/disable/cast) and there is a spell given (0 = remove spell) which pet doesn't know, don't add
        if(!((act_state == ACT_ENABLED || act_state == ACT_DISABLED || act_state == ACT_PASSIVE) && spell_id && !pet->HasSpell(spell_id)))
        {
            //sign for autocast
            if(act_state == ACT_ENABLED && spell_id)
            {
                if(pet->isCharmed())
                    charmInfo->ToggleCreatureAutocast(spell_id, true);
                else
                    ((Pet*)pet)->ToggleAutocast(spell_id, true);
            }
            //sign for no/turn off autocast
            else if(act_state == ACT_DISABLED && spell_id)
            {
                if(pet->isCharmed())
                    charmInfo->ToggleCreatureAutocast(spell_id, false);
                else
                    ((Pet*)pet)->ToggleAutocast(spell_id, false);
            }

            charmInfo->SetActionBar(position[i],spell_id,ActiveStates(act_state));
        }
    }
}

void WorldSession::HandlePetRename( WorldPacket & recv_data )
{
    DETAIL_LOG( "HandlePetRename. CMSG_PET_RENAME" );

    uint64 petguid;
    uint8 isdeclined;

    std::string name;
    DeclinedName declinedname;

    recv_data >> petguid;
    recv_data >> name;
    recv_data >> isdeclined;

    Pet* pet = _player->GetMap()->GetPet(petguid);
                                                            // check it!
    if( !pet || pet->getPetType() != HUNTER_PET ||
        !pet->HasByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED) ||
        pet->GetOwnerGUID() != _player->GetGUID() || !pet->GetCharmInfo() )
        return;

    PetNameInvalidReason res = ObjectMgr::CheckPetName(name);
    if(res != PET_NAME_SUCCESS)
    {
        SendPetNameInvalid(res, name, NULL);
        return;
    }

    if(sObjectMgr.IsReservedName(name))
    {
        SendPetNameInvalid(PET_NAME_RESERVED, name, NULL);
        return;
    }

    pet->SetName(name);

    if(_player->GetGroup())
        _player->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_NAME);

    pet->RemoveByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED);

    if(isdeclined)
    {
        for(int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        {
            recv_data >> declinedname.name[i];
        }

        std::wstring wname;
        Utf8toWStr(name, wname);
        if(!ObjectMgr::CheckDeclinedNames(wname, declinedname))
        {
            SendPetNameInvalid(PET_NAME_DECLENSION_DOESNT_MATCH_BASE_NAME, name, &declinedname);
            return;
        }
    }

    CharacterDatabase.BeginTransaction();
    if(isdeclined)
    {
        for(int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            CharacterDatabase.escape_string(declinedname.name[i]);
        CharacterDatabase.PExecute("DELETE FROM character_pet_declinedname WHERE owner = '%u' AND id = '%u'", _player->GetGUIDLow(), pet->GetCharmInfo()->GetPetNumber());
        CharacterDatabase.PExecute("INSERT INTO character_pet_declinedname (id, owner, genitive, dative, accusative, instrumental, prepositional) VALUES ('%u','%u','%s','%s','%s','%s','%s')",
            pet->GetCharmInfo()->GetPetNumber(), _player->GetGUIDLow(), declinedname.name[0].c_str(), declinedname.name[1].c_str(), declinedname.name[2].c_str(), declinedname.name[3].c_str(), declinedname.name[4].c_str());
    }

    CharacterDatabase.escape_string(name);
    CharacterDatabase.PExecute("UPDATE character_pet SET name = '%s', renamed = '1' WHERE owner = '%u' AND id = '%u'", name.c_str(), _player->GetGUIDLow(), pet->GetCharmInfo()->GetPetNumber());
    CharacterDatabase.CommitTransaction();

    pet->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL)));
}

void WorldSession::HandlePetAbandon( WorldPacket & recv_data )
{
    uint64 guid;
    recv_data >> guid;                                      //pet guid
    DETAIL_LOG( "HandlePetAbandon. CMSG_PET_ABANDON pet guid is %u", GUID_LOPART(guid) );

    if(!_player->IsInWorld())
        return;

    // pet/charmed
    if (Creature* pet = _player->GetMap()->GetCreatureOrPetOrVehicle(guid))
    {
        if(pet->isPet())
        {
            if(pet->GetGUID() == _player->GetPetGUID())
            {
                uint32 feelty = pet->GetPower(POWER_HAPPINESS);
                pet->SetPower(POWER_HAPPINESS ,(feelty-50000) > 0 ?(feelty-50000) : 0);
            }

            _player->RemovePet((Pet*)pet,PET_SAVE_AS_DELETED);
        }
        else if(pet->GetGUID() == _player->GetCharmGUID())
        {
            _player->Uncharm();
        }
    }
}

void WorldSession::HandlePetUnlearnOpcode(WorldPacket& recvPacket)
{
    DETAIL_LOG("CMSG_PET_UNLEARN");
    uint64 guid;
    recvPacket >> guid;                 // Pet guid

    Pet* pet = _player->GetPet();

    if(!pet || pet->getPetType() != HUNTER_PET || pet->m_usedTalentCount == 0)
        return;

    if(guid != pet->GetGUID())
    {
        sLog.outError( "HandlePetUnlearnOpcode.Pet %u isn't pet of player %s .", uint32(GUID_LOPART(guid)),GetPlayer()->GetName() );
        return;
    }

    CharmInfo *charmInfo = pet->GetCharmInfo();
    if(!charmInfo)
    {
        sLog.outError("WorldSession::HandlePetUnlearnOpcode: object (GUID: %u TypeId: %u) is considered pet-like but doesn't have a charminfo!", pet->GetGUIDLow(), pet->GetTypeId());
        return;
    }
    pet->resetTalents();
    _player->SendTalentsInfoData(true);
}

void WorldSession::HandlePetSpellAutocastOpcode( WorldPacket& recvPacket )
{
    DETAIL_LOG("CMSG_PET_SPELL_AUTOCAST");
    uint64 guid;
    uint32 spellid;
    uint8  state;                                           //1 for on, 0 for off
    recvPacket >> guid >> spellid >> state;

    if(!_player->GetPet() && !_player->GetCharm())
        return;

    Creature* pet = _player->GetMap()->GetCreatureOrPetOrVehicle(guid);

    if(!pet || (pet != _player->GetPet() && pet != _player->GetCharm()))
    {
        sLog.outError( "HandlePetSpellAutocastOpcode.Pet %u isn't pet of player %s .", uint32(GUID_LOPART(guid)),GetPlayer()->GetName() );
        return;
    }

    // do not add not learned spells/ passive spells
    if(!pet->HasSpell(spellid) || IsPassiveSpell(spellid))
        return;

    CharmInfo *charmInfo = pet->GetCharmInfo();
    if(!charmInfo)
    {
        sLog.outError("WorldSession::HandlePetSpellAutocastOpcod: object (GUID: %u TypeId: %u) is considered pet-like but doesn't have a charminfo!", pet->GetGUIDLow(), pet->GetTypeId());
        return;
    }

    if(pet->isCharmed())
                                                            //state can be used as boolean
        pet->GetCharmInfo()->ToggleCreatureAutocast(spellid, state);
    else
        ((Pet*)pet)->ToggleAutocast(spellid, state);

    charmInfo->SetSpellAutocast(spellid,state);
}

void WorldSession::HandlePetCastSpellOpcode( WorldPacket& recvPacket )
{
    DETAIL_LOG("WORLD: CMSG_PET_CAST_SPELL");

    uint64 guid;
    uint32 spellid;
    uint8  cast_count;
    uint8  unk_flags;                                       // flags (if 0x02 - some additional data are received)

    recvPacket >> guid >> cast_count >> spellid >> unk_flags;

    DEBUG_LOG("WORLD: CMSG_PET_CAST_SPELL, cast_count: %u, spellid %u, unk_flags %u", cast_count, spellid, unk_flags);

    if (!_player->GetPet() && !_player->GetCharm())
        return;

    Creature* pet = _player->GetMap()->GetCreatureOrPetOrVehicle(guid);

    if (!pet || (pet != _player->GetPet() && pet!= _player->GetCharm()))
    {
        sLog.outError( "HandlePetCastSpellOpcode: Pet %u isn't pet of player %s .", uint32(GUID_LOPART(guid)),GetPlayer()->GetName() );
        return;
    }

    if (pet->GetGlobalCooldown() > 0)
        return;

    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellid);
    if (!spellInfo)
    {
        sLog.outError("WORLD: unknown PET spell id %i", spellid);
        return;
    }

    // do not cast not learned spells
    if (!pet->HasSpell(spellid) || IsPassiveSpell(spellInfo))
        return;

    SpellCastTargets targets;

    recvPacket >> targets.ReadForCaster(pet);

    pet->clearUnitState(UNIT_STAT_MOVING);

    Spell *spell = new Spell(pet, spellInfo, false);
    spell->m_cast_count = cast_count;                       // probably pending spell cast
    spell->m_targets = targets;

    SpellCastResult result = spell->CheckPetCast(NULL);
    if (result == SPELL_CAST_OK)
    {
        pet->AddCreatureSpellCooldown(spellid);
        if (pet->isPet())
        {
            //10% chance to play special pet attack talk, else growl
            //actually this only seems to happen on special spells, fire shield for imp, torment for voidwalker, but it's stupid to check every spell
            if(((Pet*)pet)->getPetType() == SUMMON_PET && (urand(0, 100) < 10))
                pet->SendPetTalk((uint32)PET_TALK_SPECIAL_SPELL);
            else
                pet->SendPetAIReaction(guid);
        }

        spell->prepare(&(spell->m_targets));
    }
    else
    {
        pet->SendPetCastFail(spellid, result);
        if (!pet->HasSpellCooldown(spellid))
            GetPlayer()->SendClearCooldown(spellid, pet);

        spell->finish(false);
        delete spell;
    }
}

void WorldSession::SendPetNameInvalid(uint32 error, const std::string& name, DeclinedName *declinedName)
{
    WorldPacket data(SMSG_PET_NAME_INVALID, 4 + name.size() + 1 + 1);
    data << uint32(error);
    data << name;
    if(declinedName)
    {
        data << uint8(1);
        for(uint32 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            data << declinedName->name[i];
    }
    else
        data << uint8(0);
    SendPacket(&data);
}

void WorldSession::HandlePetLearnTalent( WorldPacket & recv_data )
{
    DEBUG_LOG("WORLD: CMSG_PET_LEARN_TALENT");

    uint64 guid;
    uint32 talent_id, requested_rank;
    recv_data >> guid >> talent_id >> requested_rank;

    _player->LearnPetTalent(guid, talent_id, requested_rank);
    _player->SendTalentsInfoData(true);
}

void WorldSession::HandleLearnPreviewTalentsPet( WorldPacket & recv_data )
{
    DEBUG_LOG("CMSG_LEARN_PREVIEW_TALENTS_PET");

    uint64 guid;
    recv_data >> guid;

    uint32 talentsCount;
    recv_data >> talentsCount;

    uint32 talentId, talentRank;

    for(uint32 i = 0; i < talentsCount; ++i)
    {
        recv_data >> talentId >> talentRank;

        _player->LearnPetTalent(guid, talentId, talentRank);
    }

    _player->SendTalentsInfoData(true);
}
