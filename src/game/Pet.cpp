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

#include "Pet.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Formulas.h"
#include "SpellAuras.h"
#include "CreatureAI.h"
#include "Unit.h"
#include "Util.h"

Pet::Pet(PetType type) :
Creature(CREATURE_SUBTYPE_PET),
m_resetTalentsCost(0), m_resetTalentsTime(0), m_usedTalentCount(0),
m_removed(false), m_happinessTimer(7500), m_petType(type), m_duration(0),
m_auraUpdateMask(0), m_loading(true),
m_declinedname(NULL), m_petModeFlags(PET_MODE_DEFAULT),
m_petFollowAngle(PET_FOLLOW_ANGLE), m_needSave(true), m_petCounter(0), m_PetScalingData(NULL), m_createSpellID(0),m_HappinessState(0)
{
    SetName("Pet");
    m_regenTimer = 2000;

    m_baseBonusData = new PetScalingData;

    // pets always have a charminfo, even if they are not actually charmed
    if (!GetCharmInfo())
        InitCharmInfo(this);

    if (type == MINI_PET)                                    // always passive
        GetCharmInfo()->SetReactState(REACT_PASSIVE);
    else if(type == PROTECTOR_PET)                           // always defensive
        GetCharmInfo()->SetReactState(REACT_DEFENSIVE);
    else if (type == GUARDIAN_PET)                           // always aggressive
        GetCharmInfo()->SetReactState(REACT_AGGRESSIVE);

}

Pet::~Pet()
{
    delete m_declinedname;

    if (m_PetScalingData)
        delete m_PetScalingData;

    if (m_baseBonusData)
        delete m_baseBonusData;
}

void Pet::AddToWorld()
{
    ///- Register the pet for guid lookup
    if (!((Creature*)this)->IsInWorld())
    {
        GetMap()->GetObjectsStore().insert<Pet>(GetGUID(), (Pet*)this);
        sObjectAccessor.AddObject(this);
    }

    Unit::AddToWorld();
}

void Pet::RemoveFromWorld()
{
    ///- Remove the pet from the accessor
    if (((Creature*)this)->IsInWorld())
    {
        GetMap()->GetObjectsStore().erase<Pet>(GetGUID(), (Pet*)NULL);
        sObjectAccessor.RemoveObject(this);
    }

    ///- Don't call the function for Creature, normal mobs + totems go in a different storage
    Unit::RemoveFromWorld();
}

bool Pet::LoadPetFromDB( Player* owner, uint32 petentry, uint32 petnumber, bool current,  float x, float y, float z )
{
    m_loading = true;

    uint32 ownerid = owner->GetGUIDLow();

    QueryResult *result;

    if (petnumber)
        // known petnumber entry                  0   1      2(?)   3        4      5    6           7     8     9        10         11       12            13      14        15                 16                 17              18
        result = CharacterDatabase.PQuery("SELECT id, entry, owner, modelid, level, exp, Reactstate, slot, name, renamed, curhealth, curmana, curhappiness, abdata, savetime, resettalents_cost, resettalents_time, CreatedBySpell, PetType "
            "FROM character_pet WHERE owner = '%u' AND id = '%u'",
            ownerid, petnumber);
    else if (current)
        // current pet (slot 0)                   0   1      2(?)   3        4      5    6           7     8     9        10         11       12            13      14        15                 16                 17              18
        result = CharacterDatabase.PQuery("SELECT id, entry, owner, modelid, level, exp, Reactstate, slot, name, renamed, curhealth, curmana, curhappiness, abdata, savetime, resettalents_cost, resettalents_time, CreatedBySpell, PetType "
            "FROM character_pet WHERE owner = '%u' AND slot = '%u'",
            ownerid, PET_SAVE_AS_CURRENT );
    else if (petentry)
        // known petentry entry (unique for summoned pet, but non unique for hunter pet (only from current or not stabled pets)
        //                                        0   1      2(?)   3        4      5    6           7     8     9        10         11       12           13       14        15                 16                 17              18
        result = CharacterDatabase.PQuery("SELECT id, entry, owner, modelid, level, exp, Reactstate, slot, name, renamed, curhealth, curmana, curhappiness, abdata, savetime, resettalents_cost, resettalents_time, CreatedBySpell, PetType "
            "FROM character_pet WHERE owner = '%u' AND entry = '%u' AND (slot = '%u' OR slot > '%u') ",
            ownerid, petentry,PET_SAVE_AS_CURRENT,PET_SAVE_LAST_STABLE_SLOT);
    else
        // any current or other non-stabled pet (for hunter "call pet")
        //                                        0   1      2(?)   3        4      5    6           7     8     9        10         11       12            13      14        15                 16                 17              18
        result = CharacterDatabase.PQuery("SELECT id, entry, owner, modelid, level, exp, Reactstate, slot, name, renamed, curhealth, curmana, curhappiness, abdata, savetime, resettalents_cost, resettalents_time, CreatedBySpell, PetType "
            "FROM character_pet WHERE owner = '%u' AND (slot = '%u' OR slot > '%u') ",
            ownerid,PET_SAVE_AS_CURRENT,PET_SAVE_LAST_STABLE_SLOT);

    if(!result)
        return false;

    Field *fields = result->Fetch();

    uint32 pet_number = fields[0].GetUInt32();

    // update for case of current pet "slot = 0"
    petentry = fields[1].GetUInt32();

    if (!petentry)
    {
        delete result;
        return false;
    }

    setPetType(PetType(fields[18].GetUInt8()));

    if(getPetType() == HUNTER_PET)
    {
        CreatureInfo const* creatureInfo = ObjectMgr::GetCreatureTemplate(petentry);
        if(!creatureInfo || !creatureInfo->isTameable(owner->CanTameExoticPets()))
        {
            delete result;
            return false;
        }
    }

    if (!GetCreateSpellID())
        SetCreateSpellID(fields[17].GetUInt32());

    SpellEntry const* spellInfo = sSpellStore.LookupEntry(GetCreateSpellID());

    if (spellInfo && GetSpellDuration(spellInfo) > 0 )
        SetDuration(GetSpellDuration(spellInfo));

    if (current && owner->IsPetNeedBeTemporaryUnsummoned())
    {
        owner->SetTemporaryUnsummonedPetNumber(pet_number);
        delete result;
        return false;
    }

    if (!Create(owner->GetMap()->GenerateLocalLowGuid(HIGHGUID_PET), owner->GetMap(), owner->GetPhaseMask(), petentry, pet_number, owner))
    {
        delete result;
        return false;
    }

    SetDisplayId(fields[3].GetUInt32());
    SetNativeDisplayId(fields[3].GetUInt32());
    uint32 petlevel = fields[4].GetUInt32();
    SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
    SetName(std::string(fields[8].GetCppString()));
    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, fields[5].GetUInt32());
    m_charmInfo->SetReactState(ReactStates(fields[6].GetUInt8()));

    if (!SetSummonPosition( x, y, z))
    {
        sLog.outError("Pet (guidlow %d, entry %d) not loaded. Suggested coordinates isn't valid (X: %f Y: %f)",
            GetGUIDLow(), GetEntry(), GetPositionX(), GetPositionY());
        delete result;
        return false;
    }

    CreatureInfo const *cinfo = GetCreatureInfo();

    if (cinfo->type == CREATURE_TYPE_CRITTER)
    {
        AIM_Initialize();
        GetMap()->Add((Creature*)this);
        delete result;
        return true;
    }

    switch (getPetType())
    {
        case SUMMON_PET:
            petlevel=owner->getLevel();
            SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);
            SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            break;
        case HUNTER_PET:
            SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_WARRIOR);
            SetByteValue(UNIT_FIELD_BYTES_0, 2, GENDER_NONE);
            SetByteValue(UNIT_FIELD_BYTES_0, 3, POWER_FOCUS);
            SetSheath(SHEATH_STATE_MELEE);
            SetByteFlag(UNIT_FIELD_BYTES_2, 2, (fields[9].GetUInt8() == 0) ? UNIT_CAN_BE_ABANDONED : UNIT_CAN_BE_RENAMED | UNIT_CAN_BE_ABANDONED);
            SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
                                                            // this enables popup window (pet abandon, cancel)
            SetMaxPower(POWER_HAPPINESS, GetCreatePowers(POWER_HAPPINESS));
            SetPower(POWER_HAPPINESS, fields[12].GetUInt32());
            setPowerType(POWER_FOCUS);
            break;
        default:
            sLog.outError("Pet have incorrect type (%u) for pet loading.", getPetType());
    }

    if(owner->IsPvP())
        SetPvP(true);

    if(owner->IsFFAPvP())
        SetFFAPvP(true);

    SetCanModifyStats(true);
    InitStatsForLevel(petlevel);
    InitTalentForLevel();                                   // set original talents points before spell loading

    SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL)));

    uint32 savedhealth = fields[10].GetUInt32();
    uint32 savedmana = fields[11].GetUInt32();

    // set current pet as current
    // 0=current
    // 1..MAX_PET_STABLES in stable slot
    // PET_SAVE_NOT_IN_SLOT(100) = not stable slot (summoning))
    if (fields[7].GetUInt32() != 0)
    {
        CharacterDatabase.BeginTransaction();
        CharacterDatabase.PExecute("UPDATE character_pet SET slot = '%u' WHERE owner = '%u' AND slot = '%u' AND id <> '%u'",
            PET_SAVE_NOT_IN_SLOT, ownerid, PET_SAVE_AS_CURRENT, m_charmInfo->GetPetNumber());
        CharacterDatabase.PExecute("UPDATE character_pet SET slot = '%u' WHERE owner = '%u' AND id = '%u'",
            PET_SAVE_AS_CURRENT, ownerid, m_charmInfo->GetPetNumber());
        CharacterDatabase.CommitTransaction();
    }


    // load action bar, if data broken will fill later by default spells.
    m_charmInfo->LoadPetActionBar(fields[13].GetCppString());
    // since last save (in seconds)
    uint32 timediff = uint32(time(NULL) - fields[14].GetUInt64());

    m_resetTalentsCost = fields[15].GetUInt32();
    m_resetTalentsTime = fields[16].GetUInt64();

    delete result;

    //load spells/cooldowns/auras
    _LoadAuras(timediff);
    _LoadSpells();
    SynchronizeLevelWithOwner();
    InitLevelupSpellsForLevel();
    LearnPetPassives();
    _LoadSpellCooldowns();

    if (getPetType() == SUMMON_PET)
        CastPetAuras(true);
    else
        CastPetAuras(current);

    CastPetPassiveAuras(true);
    CalculateScalingData(true);
    ApplyAllScalingBonuses(true);

    if (isControlled())
     //all (?) summon pets come with full health when called, note by virusav
    {
        SetHealth(GetMaxHealth());
        SetPower(getPowerType(), GetMaxPower(getPowerType()));
        LoadCreatureAddon(true);
    }

    UpdateWalkMode(owner);
    AIM_Initialize();

    GetMap()->Add((Creature*)this);

    owner->SetPet(this);                                    // in DB stored only full controlled creature

    DEBUG_LOG("New pet (loaded from DB) has guid %u", GetGUIDLow());

    m_loading = false;

    if (owner->GetTypeId() == TYPEID_PLAYER)
    {
        CleanupActionBar();                                     // remove unknown spells from action bar after load
        if (isControlled() && !GetPetCounter())
        {
            ((Player*)owner)->PetSpellInitialize();
            ((Player*)owner)->SendTalentsInfoData(true);
        }

    }

    if (owner->GetTypeId() == TYPEID_PLAYER && getPetType() == HUNTER_PET)
    {
        result = CharacterDatabase.PQuery("SELECT genitive, dative, accusative, instrumental, prepositional FROM character_pet_declinedname WHERE owner = '%u' AND id = '%u'", owner->GetGUIDLow(), GetCharmInfo()->GetPetNumber());

        if(result)
        {
            if(m_declinedname)
                delete m_declinedname;

            m_declinedname = new DeclinedName;
            Field *fields2 = result->Fetch();
            for(int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
                m_declinedname->name[i] = fields2[i].GetCppString();

            delete result;
        }
    }

    return true;
}

void Pet::SavePetToDB(PetSaveMode mode)
{
    if (!GetEntry())
        return;

    // save only fully controlled creature
    if (!isControlled())
        return;

    // not save not player pets
    if (!GetOwnerGuid().IsPlayer())
        return;

    Player* pOwner = (Player*)GetOwner();
    if (!pOwner)
        return;

    // current/stable/not_in_slot
    if (mode >= PET_SAVE_AS_CURRENT)
    {
        // reagents must be returned before save call
        if (mode == PET_SAVE_REAGENTS)
            mode = PET_SAVE_NOT_IN_SLOT;
        // not save pet as current if another pet temporary unsummoned
        else if (mode == PET_SAVE_AS_CURRENT && pOwner->GetTemporaryUnsummonedPetNumber() &&
            pOwner->GetTemporaryUnsummonedPetNumber() != m_charmInfo->GetPetNumber())
        {
            // pet will lost anyway at restore temporary unsummoned
            if(getPetType()==HUNTER_PET)
                return;

            // for warlock case
            mode = PET_SAVE_NOT_IN_SLOT;
        }

        uint32 curhealth = GetHealth();
        uint32 curmana = GetPower(POWER_MANA);

        // stable and not in slot saves
        if (getPetType() == HUNTER_PET && mode != PET_SAVE_AS_CURRENT)
            RemoveAllAuras();

        //save pet's data as one single transaction
        CharacterDatabase.BeginTransaction();
        _SaveSpells();
        _SaveSpellCooldowns();
        _SaveAuras();

        uint32 ownerLow = GetOwnerGuid().GetCounter();
        std::string name = m_name;
        CharacterDatabase.escape_string(name);
        // remove current data
        CharacterDatabase.PExecute("DELETE FROM character_pet WHERE owner = '%u' AND id = '%u'", ownerLow, m_charmInfo->GetPetNumber());

        // prevent duplicate using slot (except PET_SAVE_NOT_IN_SLOT)
        if (mode <= PET_SAVE_LAST_STABLE_SLOT)
            CharacterDatabase.PExecute("UPDATE character_pet SET slot = '%u' WHERE owner = '%u' AND slot = '%u'",
                PET_SAVE_NOT_IN_SLOT, ownerLow, uint32(mode) );

        // prevent existence another hunter pet in PET_SAVE_AS_CURRENT and PET_SAVE_NOT_IN_SLOT
        if (getPetType()==HUNTER_PET && (mode==PET_SAVE_AS_CURRENT||mode > PET_SAVE_LAST_STABLE_SLOT))
            CharacterDatabase.PExecute("DELETE FROM character_pet WHERE owner = '%u' AND (slot = '%u' OR slot > '%u')",
                ownerLow, PET_SAVE_AS_CURRENT, PET_SAVE_LAST_STABLE_SLOT);
        // save pet
        std::ostringstream ss;
        ss  << "INSERT INTO character_pet ( id, entry,  owner, modelid, level, exp, Reactstate, slot, name, renamed, curhealth, curmana, curhappiness, abdata, savetime, resettalents_cost, resettalents_time, CreatedBySpell, PetType) "
            << "VALUES ("
            << m_charmInfo->GetPetNumber() << ", "
            << GetEntry() << ", "
            << ownerLow << ", "
            << GetNativeDisplayId() << ", "
            << getLevel() << ", "
            << GetUInt32Value(UNIT_FIELD_PETEXPERIENCE) << ", "
            << uint32(m_charmInfo->GetReactState()) << ", "
            << uint32(mode) << ", '"
            << name.c_str() << "', "
            << uint32(HasByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED) ? 0 : 1) << ", "
            << (curhealth < 1 ? 1 : curhealth) << ", "
            << curmana << ", "
            << GetPower(POWER_HAPPINESS) << ", '";

        for(uint32 i = ACTION_BAR_INDEX_START; i < ACTION_BAR_INDEX_END; ++i)
        {
            ss << uint32(m_charmInfo->GetActionBarEntry(i)->GetType()) << " "
               << uint32(m_charmInfo->GetActionBarEntry(i)->GetAction()) << " ";
        };

        ss  << "', "
            << time(NULL) << ", "
            << uint32(m_resetTalentsCost) << ", "
            << uint64(m_resetTalentsTime) << ", "
            << GetUInt32Value(UNIT_CREATED_BY_SPELL) << ", "
            << uint32(getPetType()) << ")";

        CharacterDatabase.Execute( ss.str().c_str() );
        CharacterDatabase.CommitTransaction();
    }
    // delete
    else
    {
        RemoveAllAuras(AURA_REMOVE_BY_DELETE);
        DeleteFromDB(m_charmInfo->GetPetNumber());
    }
}

void Pet::DeleteFromDB(uint32 guidlow, bool separate_transaction)
{
    if(separate_transaction)
        CharacterDatabase.BeginTransaction();

    CharacterDatabase.PExecute("DELETE FROM character_pet WHERE id = '%u'", guidlow);
    CharacterDatabase.PExecute("DELETE FROM character_pet_declinedname WHERE id = '%u'", guidlow);
    CharacterDatabase.PExecute("DELETE FROM pet_aura WHERE guid = '%u'", guidlow);
    CharacterDatabase.PExecute("DELETE FROM pet_spell WHERE guid = '%u'", guidlow);
    CharacterDatabase.PExecute("DELETE FROM pet_spell_cooldown WHERE guid = '%u'", guidlow);

    if(separate_transaction)
        CharacterDatabase.CommitTransaction();
}

void Pet::SetDeathState(DeathState s)                       // overwrite virtual Creature::SetDeathState and Unit::SetDeathState
{
    Creature::SetDeathState(s);
    if(getDeathState()==CORPSE)
    {
        //remove summoned pet (no corpse)
        if(getPetType()==SUMMON_PET)
            Unsummon(PET_SAVE_NOT_IN_SLOT);
        // other will despawn at corpse desppawning (Pet::Update code)
        else
        {
            // pet corpse non lootable and non skinnable
            SetUInt32Value( UNIT_DYNAMIC_FLAGS, 0x00 );
            RemoveFlag (UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

            //lose happiness when died and not in BG/Arena
            MapEntry const* mapEntry = sMapStore.LookupEntry(GetMapId());
            if(!mapEntry || (mapEntry->map_type != MAP_ARENA && mapEntry->map_type != MAP_BATTLEGROUND))
                ModifyPower(POWER_HAPPINESS, -HAPPINESS_LEVEL_SIZE);

            if( HasSpell(55709) && GetOwner() && GetOwner()->GetTypeId() == TYPEID_PLAYER)
                GetOwner()->CastSpell(GetOwner(), 54114, false);

            SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        }
    }
    else if(getDeathState()==ALIVE)
    {
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        CastPetAuras(true);
    }
}

void Pet::Update(uint32 update_diff, uint32 diff)
{
    if (!IsInWorld())                               // pet already removed, just wait in remove queue, no updates
        return;

    switch( m_deathState )
    {
        case CORPSE:
        {
            if (m_corpseDecayTimer <= update_diff)
            {
                MANGOS_ASSERT(getPetType()!=SUMMON_PET && "Must be already removed.");
                Unsummon(PET_SAVE_NOT_IN_SLOT);             //hunters' pets never get removed because of death, NEVER!
                return;
            }
            break;
        }
        case ALIVE:
        {
            // unsummon pet that lost owner
            Unit* owner = GetOwner();
            if (!owner)
            {
                sLog.outError("Pet %d lost owner, removed. ", GetGUID());
                Unsummon(PET_SAVE_NOT_IN_SLOT);
                return;
            }

            if (isControlled() && !IsWithinDistInMap(owner, GetMap()->GetVisibilityDistance()))
            {
                DEBUG_LOG("Pet %d lost control, removed. Owner = %d, distance = %d, pet GUID = ", GetGUID(),owner->GetGUID(), GetDistance2d(owner), owner->GetPetGuid().GetCounter());
                Unsummon(PET_SAVE_REAGENTS);
                return;
            }

            if (isControlled())
            {
                GroupPetList m_groupPets = owner->GetPets();
                if (m_groupPets.find(GetObjectGuid().GetRawValue()) == m_groupPets.end())
                {
                    sLog.outError("Pet %d controlled, but not in list, removed.", GetGUID());
                    Unsummon(getPetType() == HUNTER_PET ? PET_SAVE_AS_DELETED : PET_SAVE_NOT_IN_SLOT, owner);
                    return;
                }
            }
            else 
                if (!IsWithinDistInMap(owner, GetMap()->GetVisibilityDistance()))
                {
                    sLog.outError("Not controlled pet %d lost view from owner, removed. Owner = %d, distance = %d, pet GUID = ", GetGUID(),owner->GetGUID(), GetDistance2d(owner), owner->GetPetGuid().GetCounter());
                    Unsummon(PET_SAVE_AS_DELETED);
                    return;
                }

            if (m_duration > 0)
            {
                if(m_duration > (int32)update_diff)
                    m_duration -= (int32)update_diff;
                else
                {
                    DEBUG_LOG("Pet %d removed with duration expired.", GetGUID());
                    Unsummon(PET_SAVE_AS_DELETED, owner);
                    return;
                }
            }

            //regenerate focus for hunter pets or energy for deathknight's ghoul
            if(m_regenTimer <= update_diff)
            {
                Regenerate(getPowerType(), REGEN_TIME_FULL);
                m_regenTimer = REGEN_TIME_FULL;

                if(getPetType() == HUNTER_PET)
                    Regenerate(POWER_HAPPINESS, REGEN_TIME_FULL);

                if (!isInCombat() || IsPolymorphed())
                    RegenerateHealth(REGEN_TIME_FULL);
            }
            else
                m_regenTimer -= update_diff;

            break;
        }
        default:
            break;
    }
    // Update scaling auras from queue
    while (!m_scalingQueue.empty())
    {
        ApplyScalingBonus(&m_scalingQueue.front());
        m_scalingQueue.pop();
    };

    if (IsInWorld())
        Creature::Update(update_diff, diff);
}

HappinessState Pet::GetHappinessState()
{
    if(GetPower(POWER_HAPPINESS) < HAPPINESS_LEVEL_SIZE)
        return UNHAPPY;
    else if(GetPower(POWER_HAPPINESS) >= HAPPINESS_LEVEL_SIZE * 2)
        return HAPPY;
    else
        return CONTENT;
}

bool Pet::CanTakeMoreActiveSpells(uint32 spellid)
{
    uint8  activecount = 1;
    uint32 chainstartstore[ACTIVE_SPELLS_MAX];

    if(IsPassiveSpell(spellid))
        return true;

    chainstartstore[0] = sSpellMgr.GetFirstSpellInChain(spellid);

    for (PetSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        if(itr->second.state == PETSPELL_REMOVED)
            continue;

        if(IsPassiveSpell(itr->first))
            continue;

        uint32 chainstart = sSpellMgr.GetFirstSpellInChain(itr->first);

        uint8 x;

        for(x = 0; x < activecount; x++)
        {
            if(chainstart == chainstartstore[x])
                break;
        }

        if(x == activecount)                                //spellchain not yet saved -> add active count
        {
            ++activecount;
            if(activecount > ACTIVE_SPELLS_MAX)
                return false;
            chainstartstore[x] = chainstart;
        }
    }
    return true;
}

void Pet::Unsummon(PetSaveMode mode, Unit* owner /*= NULL*/)
{

    if (!owner)
        owner = GetOwner();

    m_removed = true;

    CombatStop();

    if (owner)
    {
        if (GetOwnerGuid() != owner->GetObjectGuid())
            return;

        Player* p_owner = owner->GetTypeId()==TYPEID_PLAYER ? (Player*)owner : NULL;

        if (p_owner)
        {

            // not save secondary permanent pet as current
            if (mode == PET_SAVE_AS_CURRENT && p_owner->GetTemporaryUnsummonedPetNumber() &&
                p_owner->GetTemporaryUnsummonedPetNumber() != GetCharmInfo()->GetPetNumber())
                mode = PET_SAVE_NOT_IN_SLOT;

            SpellEntry const *spellInfo = sSpellStore.LookupEntry(GetCreateSpellID());

            // Special way for remove cooldown if SPELL_ATTR_DISABLED_WHILE_ACTIVE
            if (spellInfo && spellInfo->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
            {
                p_owner->SendCooldownEvent(spellInfo);
            }

            if (mode == PET_SAVE_REAGENTS)
            {
                //returning of reagents only for players, so best done here

                if (spellInfo)
                {
                    for(uint32 i = 0; i < MAX_SPELL_REAGENTS; ++i)
                    {
                        if (spellInfo->Reagent[i] > 0)
                        {
                            ItemPosCountVec dest;           //for succubus, voidwalker, felhunter and felguard credit soulshard when despawn reason other than death (out of range, logout)
                            uint8 msg = p_owner->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, spellInfo->Reagent[i], spellInfo->ReagentCount[i]);
                            if (msg == EQUIP_ERR_OK)
                            {
                                Item* item = p_owner->StoreNewItem(dest, spellInfo->Reagent[i], true);
                                if (p_owner->IsInWorld())
                                    p_owner->SendNewItem(item, spellInfo->ReagentCount[i], true, false);
                            }
                        }
                    }
                }
            }

            if (isControlled()  && !GetPetCounter())
            {
                p_owner->RemovePetActionBar();

                if (p_owner->GetGroup())
                    p_owner->SetGroupUpdateFlag(GROUP_UPDATE_PET);
            }
        }

        // only if current pet in slot
        switch(getPetType())
        {
            case MINI_PET:
                if (p_owner)
                    p_owner->SetMiniPet(NULL);
                break;
            case PROTECTOR_PET:
            case GUARDIAN_PET:
                owner->RemoveGuardian(this);
                break;
            default:
                if (owner->GetPetGuid() == GetObjectGuid())
                    owner->SetPet(NULL);
                break;
        }

        if (GetNeedSave())
            SavePetToDB(mode);
    }

    AddObjectToRemoveList();

}

void Pet::GivePetXP(uint32 xp)
{
    if(getPetType() != HUNTER_PET)
        return;

    if ( xp < 1 )
        return;

    if(!isAlive())
        return;

    uint32 level = getLevel();
    uint32 maxlevel = std::min(sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL), GetOwner()->getLevel());

    // pet not receive xp for level equal to owner level
    if (level >= maxlevel)
        return;

    uint32 nextLvlXP = GetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP);
    uint32 curXP = GetUInt32Value(UNIT_FIELD_PETEXPERIENCE);
    uint32 newXP = curXP + xp;

    while( newXP >= nextLvlXP && level < maxlevel)
    {
        newXP -= nextLvlXP;
        ++level;

        GivePetLevel(level);                              // also update UNIT_FIELD_PETNEXTLEVELEXP and UNIT_FIELD_PETEXPERIENCE to level start

        nextLvlXP = GetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP);
    }

    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, level < maxlevel ? newXP : 0);
}

void Pet::GivePetLevel(uint32 level)
{
    if (!level || level == getLevel())
        return;

    if (getPetType()==HUNTER_PET)
    {
        SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
        SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, sObjectMgr.GetXPForPetLevel(level));
    }

    InitStatsForLevel(level);
    InitLevelupSpellsForLevel();
    InitTalentForLevel();
}

bool Pet::CreateBaseAtCreature(Creature* creature, Unit* owner)
{
    if(!creature || !owner)
    {
        sLog.outError("CRITICAL: NULL pointer parsed into CreateBaseAtCreature()");
        return false;
    }

    CreatureInfo const *cinfo = creature->GetCreatureInfo();
    if(!cinfo)
    {
        sLog.outError("CreateBaseAtCreature() failed, creatureInfo is missing!");
        return false;
    }

    BASIC_LOG("Create new pet from creature %d ", creature->GetEntry());

    if (!Create(owner, creature->GetEntry()))
        return false;

    if (!SetSummonPosition(creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ()))
    {
        sLog.outError("Pet (guidlow %d, entry %d) not created base at creature. Suggested coordinates isn't valid (X: %f Y: %f)",
            GetGUIDLow(), GetEntry(), GetPositionX(), GetPositionY());
        return false;
    }

    if(CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cinfo->family))
        SetName(cFamily->Name[sWorld.GetDefaultDbcLocale()]);
    else
        SetName(creature->GetNameForLocaleIdx(sObjectMgr.GetDBCLocaleIndex()));

    if(cinfo->type == CREATURE_TYPE_CRITTER)
    {
        setPetType(MINI_PET);
        return true;
    }

    if(cinfo->type == CREATURE_TYPE_BEAST)
    {
        SetUInt32Value(UNIT_MOD_CAST_SPEED, creature->GetUInt32Value(UNIT_MOD_CAST_SPEED));
    }

    SetDisplayId(creature->GetDisplayId());
    SetNativeDisplayId(creature->GetNativeDisplayId());

    return true;
}

bool Pet::InitStatsForLevel(uint32 petlevel, Unit* owner)
{

    CreatureInfo const *cinfo = GetCreatureInfo();
    MANGOS_ASSERT(cinfo);

    if(!owner)
    {
        owner = GetOwner();
        if(!owner)
        {
            sLog.outError("attempt to summon pet (Entry %u) without owner! Attempt terminated.", cinfo->Entry);
            return false;
        }
    }

    if (!petlevel)
        petlevel = owner->getLevel();

    SetLevel(petlevel);

    PetLevelInfo const* pInfo = sObjectMgr.GetPetLevelInfo(cinfo->Entry, petlevel);

    SetMeleeDamageSchool(SpellSchools(cinfo->dmgschool));

    int32 createStats[MAX_STATS+7] =  {22,     // STAT_STRENGTH
                                       22,     // STAT_AGILITY
                                       25,     // STAT_STAMINA
                                       28,     // STAT_INTELLECT
                                       27,     // STAT_SPIRIT
                                       42,     // Base HEALTH
                                       20,     // Base POWER/MANA
                                       10,     // Base AttackPower
                                       5,      // Base MinDamage
                                       10,     // Base MaxDamage
                                       1,      // Base MinRangeDamage
                                       3};     // Base MaxRangeDamage

    uint32 createResistance[MAX_SPELL_SCHOOL] = {0,0,0,0,0,0,0};

    if(cinfo) // Default create values (from creature_template)
    {
        createResistance[SPELL_SCHOOL_HOLY]   = cinfo->resistance1;
        createResistance[SPELL_SCHOOL_FIRE]   = cinfo->resistance2;
        createResistance[SPELL_SCHOOL_NATURE] = cinfo->resistance3;
        createResistance[SPELL_SCHOOL_FROST]  = cinfo->resistance4;
        createResistance[SPELL_SCHOOL_SHADOW] = cinfo->resistance5;
        createResistance[SPELL_SCHOOL_ARCANE] = cinfo->resistance6;
        // Armor
        createResistance[SPELL_SCHOOL_NORMAL] = int32(cinfo->armor  * petlevel / cinfo->maxlevel / (1 +  cinfo->rank));

        for (int i = 0; i < MAX_STATS; ++i)
            createStats[i] *= petlevel/10;

        createStats[MAX_STATS]    = int32(cinfo->maxhealth * petlevel / cinfo->maxlevel / (1 +  cinfo->rank));
        createStats[MAX_STATS+1]  = int32(cinfo->maxmana * petlevel / cinfo->maxlevel / (1 +  cinfo->rank));
        createStats[MAX_STATS+2]  = int32(cinfo->attackpower * petlevel / cinfo->maxlevel / (1 +  cinfo->rank));
        createStats[MAX_STATS+3]  = int32( cinfo->mindmg * petlevel / cinfo->maxlevel / (1 + cinfo->rank));
        createStats[MAX_STATS+4]  = int32( cinfo->maxdmg * petlevel / cinfo->maxlevel / (1 + cinfo->rank));
        createStats[MAX_STATS+5]  = int32(cinfo->minrangedmg * petlevel / cinfo->maxlevel/ (1 + cinfo->rank));
        createStats[MAX_STATS+6]  = int32(cinfo->maxrangedmg * petlevel / cinfo->maxlevel/ (1 + cinfo->rank));
        SetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE, float(cinfo->maxrangedmg * petlevel / cinfo->maxlevel));
        setPowerType(Powers(cinfo->powerType));
        SetAttackTime(BASE_ATTACK, cinfo->baseattacktime);
        SetAttackTime(RANGED_ATTACK, cinfo->rangeattacktime);
    }
    else
    {
        SetAttackTime(BASE_ATTACK, BASE_ATTACK_TIME);
        SetAttackTime(RANGED_ATTACK, BASE_ATTACK_TIME);

        for (int i = 0; i < MAX_STATS+7; ++i)
            createStats[i] *= petlevel/10;
        // Armor
        createResistance[SPELL_SCHOOL_NORMAL] = petlevel*50;
    }

    switch(getPetType())
    {
        case SUMMON_PET:
        {
            if (cinfo->family == CREATURE_FAMILY_GHOUL)
                setPowerType(POWER_ENERGY);
            break;
        }
        case HUNTER_PET:
        {
            if (!pInfo)         //If no pet levelstats in DB - use 1 for default hunter pet
                pInfo = sObjectMgr.GetPetLevelInfo(1, petlevel);

            CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cinfo->family);
            if(cFamily && cFamily->minScale > 0.0f && getPetType()==HUNTER_PET)
            {
                float scale;
                if (getLevel() >= cFamily->maxScaleLevel)
                    scale = cFamily->maxScale;
                else if (getLevel() <= cFamily->minScaleLevel)
                    scale = cFamily->minScale;
                else
                    scale = cFamily->minScale + float(getLevel() - cFamily->minScaleLevel) / cFamily->maxScaleLevel * (cFamily->maxScale - cFamily->minScale);

                SetObjectScale(scale);
                UpdateModelData();
            }

            SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, sObjectMgr.GetXPForPetLevel(petlevel));
            setPowerType(POWER_FOCUS);
            break;
        }
        case GUARDIAN_PET:
        case PROTECTOR_PET:
        {
            SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
            SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, 1000);
            // DK ghouls have energy
            if (cinfo->family == CREATURE_FAMILY_GHOUL)
                setPowerType(POWER_ENERGY);
            break;
        }
        default:
        {
            sLog.outError("Pet have incorrect type (%u) for levelup.", getPetType());
            return true;
            break;
        }
    }

    if(pInfo)                                       // exist in DB
    {
        if (pInfo->health)
            SetCreateHealth(pInfo->health);
        else
            SetCreateHealth(createStats[MAX_STATS]);

        if (pInfo->mana)
            SetCreateMana(pInfo->mana);
        else
            SetCreateMana(createStats[MAX_STATS+1]);

        if (pInfo->armor)
            SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, float(pInfo->armor));
        else
            SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE,  float(createResistance[SPELL_SCHOOL_NORMAL]));

        for( int i = STAT_STRENGTH; i < MAX_STATS; ++i)
            if (pInfo->stats[i])
               SetCreateStat(Stats(i), float(pInfo->stats[i]));
            else
               SetCreateStat(Stats(i), float(createStats[i]));

        if (pInfo->attackpower)
            SetModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE, float(pInfo->attackpower));
        else
            SetModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE, float(createStats[MAX_STATS+2]));

        if (pInfo->mindmg)
            SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(pInfo->mindmg));
        else
            SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(createStats[MAX_STATS+3]));

        if (pInfo->maxdmg)
            SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(pInfo->maxdmg));
        else
            SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(createStats[MAX_STATS+4]));

        SetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE,float(createStats[MAX_STATS+5]));
        SetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE,float(createStats[MAX_STATS+6]));

        DEBUG_LOG("Pet %u stats for level initialized (from pet_levelstat values)", cinfo->Entry);
    }
    else                                            // not exist in DB, use some default fake data
    {
        DEBUG_LOG("Summoned pet (Entry: %u) not have pet stats data in DB. Use hardcoded values.",cinfo->Entry);
        for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
            SetCreateStat(Stats(i),float(createStats[i]));

        SetCreateHealth(createStats[MAX_STATS]);
        SetCreateMana(createStats[MAX_STATS+1]);
        SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE,  float(createResistance[SPELL_SCHOOL_NORMAL]));

        SetModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE, float(createStats[MAX_STATS+2]));

        SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(createStats[MAX_STATS+3]));
        SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(createStats[MAX_STATS+4]));

        SetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE, float(createStats[MAX_STATS+5]));
        SetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE, float(createStats[MAX_STATS+6]));

        DEBUG_LOG("Pet %u stats for level initialized (from creature_template values)", cinfo->Entry);
    }

    for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            SetModifierValue(UnitMods(UNIT_MOD_RESISTANCE_START + i), BASE_VALUE, float(createResistance[i]));

    SetModifierValue(UNIT_MOD_ATTACK_POWER, BASE_PCT, 1.0f);
    SetAttackTime(OFF_ATTACK, BASE_ATTACK_TIME);
    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);


    UpdateAllStats();

    SetHealth(GetMaxHealth());
    SetPower(getPowerType(), GetMaxPower(getPowerType()));

    return true;
}

bool Pet::HaveInDiet(ItemPrototype const* item) const
{
    if (!item->FoodType)
        return false;

    CreatureInfo const* cInfo = GetCreatureInfo();
    if(!cInfo)
        return false;

    CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cInfo->family);
    if(!cFamily)
        return false;

    uint32 diet = cFamily->petFoodMask;
    uint32 FoodMask = 1 << (item->FoodType-1);
    return diet & FoodMask;
}

uint32 Pet::GetCurrentFoodBenefitLevel(uint32 itemlevel)
{
    // -5 or greater food level
    if(getLevel() <= itemlevel + 5)                         //possible to feed level 60 pet with level 55 level food for full effect
        return 35000;
    // -10..-6
    else if(getLevel() <= itemlevel + 10)                   //pure guess, but sounds good
        return 17000;
    // -14..-11
    else if(getLevel() <= itemlevel + 14)                   //level 55 food gets green on 70, makes sense to me
        return 8000;
    // -15 or less
    else
        return 0;                                           //food too low level
}

void Pet::_LoadSpellCooldowns()
{
    m_CreatureSpellCooldowns.clear();
    m_CreatureCategoryCooldowns.clear();

    QueryResult *result = CharacterDatabase.PQuery("SELECT spell,time FROM pet_spell_cooldown WHERE guid = '%u'",m_charmInfo->GetPetNumber());

    if(result)
    {
        time_t curTime = time(NULL);

        WorldPacket data(SMSG_SPELL_COOLDOWN, (8+1+size_t(result->GetRowCount())*8));
        data << GetGUID();
        data << uint8(0x0);                                 // flags (0x1, 0x2)

        do
        {
            Field *fields = result->Fetch();

            uint32 spell_id = fields[0].GetUInt32();
            time_t db_time  = (time_t)fields[1].GetUInt64();

            if(!sSpellStore.LookupEntry(spell_id))
            {
                sLog.outError("Pet %u have unknown spell %u in `pet_spell_cooldown`, skipping.",m_charmInfo->GetPetNumber(),spell_id);
                continue;
            }

            // skip outdated cooldown
            if(db_time <= curTime)
                continue;

            data << uint32(spell_id);
            data << uint32(uint32(db_time-curTime)*IN_MILLISECONDS);

            _AddCreatureSpellCooldown(spell_id,db_time);

            DEBUG_LOG("Pet (Number: %u) spell %u cooldown loaded (%u secs).", m_charmInfo->GetPetNumber(), spell_id, uint32(db_time-curTime));
        }
        while( result->NextRow() );

        delete result;

        if(!m_CreatureSpellCooldowns.empty() && GetOwner())
        {
            ((Player*)GetOwner())->GetSession()->SendPacket(&data);
        }
    }
}

void Pet::_SaveSpellCooldowns()
{
    CharacterDatabase.PExecute("DELETE FROM pet_spell_cooldown WHERE guid = '%u'", m_charmInfo->GetPetNumber());

    time_t curTime = time(NULL);

    // remove oudated and save active
    for(CreatureSpellCooldowns::iterator itr = m_CreatureSpellCooldowns.begin();itr != m_CreatureSpellCooldowns.end();)
    {
        if(itr->second <= curTime)
            m_CreatureSpellCooldowns.erase(itr++);
        else
        {
            CharacterDatabase.PExecute("INSERT INTO pet_spell_cooldown (guid,spell,time) VALUES ('%u', '%u', '" UI64FMTD "')", m_charmInfo->GetPetNumber(), itr->first, uint64(itr->second));
            ++itr;
        }
    }
}

void Pet::_LoadSpells()
{
    QueryResult *result = CharacterDatabase.PQuery("SELECT spell,active FROM pet_spell WHERE guid = '%u'",m_charmInfo->GetPetNumber());

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();

            addSpell(fields[0].GetUInt32(), ActiveStates(fields[1].GetUInt8()), PETSPELL_UNCHANGED);
        }
        while( result->NextRow() );

        delete result;
    }
}

void Pet::_SaveSpells()
{
    for (PetSpellMap::iterator itr = m_spells.begin(), next = m_spells.begin(); itr != m_spells.end(); itr = next)
    {
        ++next;

        // prevent saving family passives to DB
        if (itr->second.type == PETSPELL_FAMILY)
            continue;

        switch(itr->second.state)
        {
            case PETSPELL_REMOVED:
                CharacterDatabase.PExecute("DELETE FROM pet_spell WHERE guid = '%u' and spell = '%u'", m_charmInfo->GetPetNumber(), itr->first);
                m_spells.erase(itr);
                continue;
            case PETSPELL_CHANGED:
                CharacterDatabase.PExecute("DELETE FROM pet_spell WHERE guid = '%u' and spell = '%u'", m_charmInfo->GetPetNumber(), itr->first);
                CharacterDatabase.PExecute("INSERT INTO pet_spell (guid,spell,active) VALUES ('%u', '%u', '%u')", m_charmInfo->GetPetNumber(), itr->first, itr->second.active);
                break;
            case PETSPELL_NEW:
                CharacterDatabase.PExecute("INSERT INTO pet_spell (guid,spell,active) VALUES ('%u', '%u', '%u')", m_charmInfo->GetPetNumber(), itr->first, itr->second.active);
                break;
            case PETSPELL_UNCHANGED:
                continue;
        }

        itr->second.state = PETSPELL_UNCHANGED;
    }
}

void Pet::_LoadAuras(uint32 timediff)
{
    RemoveAllAuras();

    QueryResult *result = CharacterDatabase.PQuery("SELECT caster_guid,item_guid,spell,stackcount,remaincharges,basepoints0,basepoints1,basepoints2,maxduration0,maxduration1,maxduration2,remaintime0,remaintime1,remaintime2,effIndexMask FROM pet_aura WHERE guid = '%u'",m_charmInfo->GetPetNumber());

    if(result)
    {
        do
        {
            Field *fields = result->Fetch();
            uint64 caster_guid = fields[0].GetUInt64();
            uint32 item_lowguid = fields[1].GetUInt32();
            uint32 spellid = fields[2].GetUInt32();
            uint32 stackcount= fields[3].GetUInt32();
            int32 remaincharges = (int32)fields[4].GetUInt32();
            int32 damage[MAX_EFFECT_INDEX];
            int32 maxduration[MAX_EFFECT_INDEX];
            int32 remaintime[MAX_EFFECT_INDEX];
            for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
            {
                damage[i]  = (int32)fields[i+5].GetUInt32();
                maxduration[i] = (int32)fields[i+8].GetUInt32();
                remaintime[i] = (int32)fields[i+11].GetUInt32();
            }
            uint32 effIndexMask = (int32)fields[14].GetUInt32();

            SpellEntry const* spellproto = sSpellStore.LookupEntry(spellid);
            if(!spellproto)
            {
                sLog.outError("Unknown spell (spellid %u), ignore.",spellid);
                continue;
            }

            // do not load single target auras (unless they were cast by the player)
            if (caster_guid != GetGUID() && IsSingleTargetSpell(spellproto))
                continue;

            // prevent wrong values of remaincharges
            uint32 procCharges = spellproto->procCharges;
            if(procCharges)
            {
                if(remaincharges <= 0 || remaincharges > (int32)procCharges)
                    remaincharges = procCharges;
            }
            else
                remaincharges = 0;

            if (spellproto->StackAmount < stackcount)
                stackcount = spellproto->StackAmount;

            SpellAuraHolder *holder = CreateSpellAuraHolder(spellproto, this, NULL);
            for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
            {
                if ((effIndexMask & (1 << i)) == 0)
                    continue;

                if (remaintime[i] != -1 && !IsPositiveEffect(spellid, SpellEffectIndex(i)))
                {
                    if (remaintime[i]/IN_MILLISECONDS <= int32(timediff))
                    continue;

                    remaintime[i] -= timediff*IN_MILLISECONDS;
                }

                Aura* aura = CreateAura(spellproto, SpellEffectIndex(i), NULL, holder, this);
                if (!damage[i])
                    damage[i] = aura->GetModifier()->m_amount;

                aura->SetLoadedState(damage[i], maxduration[i], remaintime[i]);
                holder->AddAura(aura, SpellEffectIndex(i));
            }

            if (!holder->IsEmptyHolder())
            {
                holder->SetLoadedState(caster_guid, ObjectGuid(HIGHGUID_ITEM, item_lowguid), stackcount, remaincharges);
                AddSpellAuraHolder(holder);
            }
            else
                delete holder;
        }
        while( result->NextRow() );

        delete result;
    }
}

void Pet::_SaveAuras()
{
    CharacterDatabase.PExecute("DELETE FROM pet_aura WHERE guid = '%u'", m_charmInfo->GetPetNumber());

    SpellAuraHolderMap const& auraHolders = GetSpellAuraHolderMap();

    if (auraHolders.empty())
        return;

    for(SpellAuraHolderMap::const_iterator itr = auraHolders.begin(); itr != auraHolders.end(); ++itr)
    {
        SpellAuraHolder *holder = itr->second;

        bool save = true;
        for (int32 j = 0; j < MAX_EFFECT_INDEX; ++j)
        {
            SpellEntry const* spellInfo = holder->GetSpellProto();
            if (spellInfo->EffectApplyAuraName[j] == SPELL_AURA_MOD_STEALTH ||
                        spellInfo->Effect[j] == SPELL_EFFECT_APPLY_AREA_AURA_OWNER ||
                        spellInfo->Effect[j] == SPELL_EFFECT_APPLY_AREA_AURA_PET )
            {
                save = false;
                break;
            }
        }

        //skip all holders from spells that are passive or channeled
        //do not save single target holders (unless they were cast by the player)
        if (save && !holder->IsPassive() && !IsChanneledSpell(holder->GetSpellProto()) && (holder->GetCasterGUID() == GetGUID() || !holder->IsSingleTarget()))
        {
            int32 damage[MAX_EFFECT_INDEX];
            int32 remaintime[MAX_EFFECT_INDEX];
            int32 maxduration[MAX_EFFECT_INDEX];
            uint32 effIndexMask = 0;

            for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
            {
                damage[i] = 0;
                remaintime[i] = 0;
                maxduration[i] = 0;

                if (Aura *aur = holder->GetAuraByEffectIndex(SpellEffectIndex(i)))
                {
                    // don't save not own area auras
                    if (aur->IsAreaAura() && holder->GetCasterGUID() != GetGUID())
                        continue;

                    damage[i] = aur->GetModifier()->m_amount;
                    remaintime[i] = aur->GetAuraDuration();
                    maxduration[i] = aur->GetAuraMaxDuration();
                    effIndexMask |= (1 << i);
                }
            }

            if (!effIndexMask)
                continue;

            CharacterDatabase.PExecute("INSERT INTO pet_aura (guid, caster_guid, item_guid, spell, stackcount, remaincharges, basepoints0, basepoints1, basepoints2, maxduration0, maxduration1, maxduration2, remaintime0, remaintime1, remaintime2, effIndexMask) VALUES "
                "('%u', '" UI64FMTD "', '%u', '%u', '%u', '%u', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%u')",
                m_charmInfo->GetPetNumber(), holder->GetCasterGuid().GetRawValue(), holder->GetCastItemGuid().GetCounter(), holder->GetId(), holder->GetStackAmount(), holder->GetAuraCharges(),
                damage[EFFECT_INDEX_0], damage[EFFECT_INDEX_1], damage[EFFECT_INDEX_2],
                maxduration[EFFECT_INDEX_0], maxduration[EFFECT_INDEX_1], maxduration[EFFECT_INDEX_2],
                remaintime[EFFECT_INDEX_0], remaintime[EFFECT_INDEX_1], remaintime[EFFECT_INDEX_2],
                effIndexMask);
        }
    }
}

bool Pet::addSpell(uint32 spell_id,ActiveStates active /*= ACT_DECIDE*/, PetSpellState state /*= PETSPELL_NEW*/, PetSpellType type /*= PETSPELL_NORMAL*/)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spell_id);
    if (!spellInfo)
    {
        // do pet spell book cleanup
        if(state == PETSPELL_UNCHANGED)                     // spell load case
        {
            sLog.outError("Pet::addSpell: nonexistent in SpellStore spell #%u request, deleting for all pets in `pet_spell`.",spell_id);
            CharacterDatabase.PExecute("DELETE FROM pet_spell WHERE spell = '%u'",spell_id);
        }
        else
            sLog.outError("Pet::addSpell: nonexistent in SpellStore spell #%u request.",spell_id);

        return false;
    }

    PetSpellMap::iterator itr = m_spells.find(spell_id);
    if (itr != m_spells.end())
    {
        if (itr->second.state == PETSPELL_REMOVED)
        {
            m_spells.erase(itr);
            state = PETSPELL_CHANGED;
        }
        else if (state == PETSPELL_UNCHANGED && itr->second.state != PETSPELL_UNCHANGED)
        {
            // can be in case spell loading but learned at some previous spell loading
            itr->second.state = PETSPELL_UNCHANGED;

            if(active == ACT_ENABLED)
                ToggleAutocast(spell_id, true);
            else if(active == ACT_DISABLED)
                ToggleAutocast(spell_id, false);

            return false;
        }
        else
            return false;
    }

    uint32 oldspell_id = 0;

    PetSpell newspell;
    newspell.state = state;
    newspell.type = type;

    if(active == ACT_DECIDE)                                //active was not used before, so we save it's autocast/passive state here
    {
        if(IsPassiveSpell(spellInfo))
            newspell.active = ACT_PASSIVE;
        else
            newspell.active = ACT_DISABLED;
    }
    else
        newspell.active = active;

    // talent: unlearn all other talent ranks (high and low)
    if(TalentSpellPos const* talentPos = GetTalentSpellPos(spell_id))
    {
        if(TalentEntry const *talentInfo = sTalentStore.LookupEntry( talentPos->talent_id ))
        {
            for(int i=0; i < MAX_TALENT_RANK; ++i)
            {
                // skip learning spell and no rank spell case
                uint32 rankSpellId = talentInfo->RankID[i];
                if(!rankSpellId || rankSpellId==spell_id)
                    continue;

                // skip unknown ranks
                if(!HasSpell(rankSpellId))
                    continue;
                removeSpell(rankSpellId,false,false);
            }
        }
    }
    else if(sSpellMgr.GetSpellRank(spell_id)!=0)
    {
        for (PetSpellMap::const_iterator itr2 = m_spells.begin(); itr2 != m_spells.end(); ++itr2)
        {
            if(itr2->second.state == PETSPELL_REMOVED) continue;

            if( sSpellMgr.IsRankSpellDueToSpell(spellInfo,itr2->first) )
            {
                // replace by new high rank
                if(sSpellMgr.IsHighRankOfSpell(spell_id,itr2->first))
                {
                    newspell.active = itr2->second.active;

                    if(newspell.active == ACT_ENABLED)
                        ToggleAutocast(itr2->first, false);

                    oldspell_id = itr2->first;
                    unlearnSpell(itr2->first,false,false);
                    break;
                }
                // ignore new lesser rank
                else if(sSpellMgr.IsHighRankOfSpell(itr2->first,spell_id))
                    return false;
            }
        }
    }

    m_spells[spell_id] = newspell;

    if (IsPassiveSpell(spellInfo))
        CastSpell(this, spell_id, true);
    else
        m_charmInfo->AddSpellToActionBar(spell_id, ActiveStates(newspell.active));

    if(newspell.active == ACT_ENABLED || !isControlled())
        ToggleAutocast(spell_id, true);

    uint32 talentCost = GetTalentSpellCost(spell_id);
    if (talentCost)
    {
        m_usedTalentCount+=talentCost;
        UpdateFreeTalentPoints(false);
    }
    return true;
}

bool Pet::learnSpell(uint32 spell_id)
{
    // prevent duplicated entires in spell book
    if (!addSpell(spell_id))
        return false;

    if(!m_loading)
    {
        Unit* owner = GetOwner();
        if(owner && owner->GetTypeId() == TYPEID_PLAYER)
        {
            WorldPacket data(SMSG_PET_LEARNED_SPELL, 4);
            data << uint32(spell_id);
            ((Player*)owner)->GetSession()->SendPacket(&data);

            ((Player*)owner)->PetSpellInitialize();
        }
    }
    return true;
}

void Pet::InitLevelupSpellsForLevel()
{
    uint32 level = getLevel();

    if(PetLevelupSpellSet const *levelupSpells = GetCreatureInfo()->family ? sSpellMgr.GetPetLevelupSpellList(GetCreatureInfo()->family) : NULL)
    {
        // PetLevelupSpellSet ordered by levels, process in reversed order
        for(PetLevelupSpellSet::const_reverse_iterator itr = levelupSpells->rbegin(); itr != levelupSpells->rend(); ++itr)
        {
            // will called first if level down
            if(itr->first > level)
                unlearnSpell(itr->second,true);                 // will learn prev rank if any
            // will called if level up
            else
                learnSpell(itr->second);                        // will unlearn prev rank if any
        }
    }

    int32 petSpellsId = GetCreatureInfo()->PetSpellDataId ? -(int32)GetCreatureInfo()->PetSpellDataId : GetEntry();

    // default spells (can be not learned if pet level (as owner level decrease result for example) less first possible in normal game)
    if(PetDefaultSpellsEntry const *defSpells = sSpellMgr.GetPetDefaultSpellsEntry(petSpellsId))
    {
        for(int i = 0; i < MAX_CREATURE_SPELL_DATA_SLOT; ++i)
        {
            SpellEntry const* spellEntry = sSpellStore.LookupEntry(defSpells->spellid[i]);
            if(!spellEntry)
                continue;

            // will called first if level down
            if(spellEntry->spellLevel > level)
                unlearnSpell(spellEntry->Id,true);
            // will called if level up
            else
                learnSpell(spellEntry->Id);
        }
    }
}

bool Pet::unlearnSpell(uint32 spell_id, bool learn_prev, bool clear_ab)
{
    if(removeSpell(spell_id,learn_prev,clear_ab))
    {
        if(!m_loading)
        {
            if (Unit* owner = GetOwner())
            {
                if(owner->GetTypeId() == TYPEID_PLAYER)
                {
                    WorldPacket data(SMSG_PET_REMOVED_SPELL, 4);
                    data << uint32(spell_id);
                    ((Player*)owner)->GetSession()->SendPacket(&data);
                }
            }
        }
        return true;
    }
    return false;
}

bool Pet::removeSpell(uint32 spell_id, bool learn_prev, bool clear_ab)
{
    PetSpellMap::iterator itr = m_spells.find(spell_id);
    if (itr == m_spells.end())
        return false;

    if(itr->second.state == PETSPELL_REMOVED)
        return false;

    if(itr->second.state == PETSPELL_NEW)
        m_spells.erase(itr);
    else
        itr->second.state = PETSPELL_REMOVED;

    RemoveAurasDueToSpell(spell_id);

    uint32 talentCost = GetTalentSpellCost(spell_id);
    if (talentCost > 0)
    {
        if (m_usedTalentCount > talentCost)
            m_usedTalentCount-=talentCost;
        else
            m_usedTalentCount = 0;

        UpdateFreeTalentPoints(false);
    }

    if (learn_prev)
    {
        if (uint32 prev_id = sSpellMgr.GetPrevSpellInChain (spell_id))
            learnSpell(prev_id);
        else
            learn_prev = false;
    }

    // if remove last rank or non-ranked then update action bar at server and client if need
    if (clear_ab && !learn_prev && m_charmInfo->RemoveSpellFromActionBar(spell_id))
    {
        if(IsInWorld())
        {
            // need update action bar for last removed rank
            if (Unit* owner = GetOwner())
                if (owner->GetTypeId() == TYPEID_PLAYER && !GetPetCounter())
                    ((Player*)owner)->PetSpellInitialize();
        }
    }

    return true;
}


void Pet::CleanupActionBar()
{
    for(int i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
        if(UnitActionBarEntry const* ab = m_charmInfo->GetActionBarEntry(i))
            if(uint32 action = ab->GetAction())
                if(ab->IsActionBarForSpell() && !HasSpell(action))
                    m_charmInfo->SetActionBar(i,0,ACT_DISABLED);
}

void Pet::InitPetCreateSpells()
{
    m_charmInfo->InitPetActionBar();
    m_spells.clear();

    LearnPetPassives();

    CastPetAuras(false);
}

bool Pet::resetTalents(bool no_cost)
{
    Unit *owner = GetOwner();
    if (!owner || owner->GetTypeId()!=TYPEID_PLAYER)
        return false;

    // not need after this call
    if(((Player*)owner)->HasAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS))
        ((Player*)owner)->RemoveAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS,true);

    CreatureInfo const * ci = GetCreatureInfo();
    if(!ci)
        return false;
    // Check pet talent type
    CreatureFamilyEntry const *pet_family = sCreatureFamilyStore.LookupEntry(ci->family);
    if(!pet_family || pet_family->petTalentType < 0)
        return false;

    Player *player = (Player *)owner;

    if (m_usedTalentCount == 0)
    {
        UpdateFreeTalentPoints(false);                      // for fix if need counter
        return false;
    }

    uint32 cost = 0;

    if(!no_cost)
    {
        cost = resetTalentsCost();

        if (player->GetMoney() < cost)
        {
            player->SendBuyError( BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
            return false;
        }
    }

    for (unsigned int i = 0; i < sTalentStore.GetNumRows(); ++i)
    {
        TalentEntry const *talentInfo = sTalentStore.LookupEntry(i);

        if (!talentInfo) continue;

        TalentTabEntry const *talentTabInfo = sTalentTabStore.LookupEntry( talentInfo->TalentTab );

        if(!talentTabInfo)
            continue;

        // unlearn only talents for pets family talent type
        if(!((1 << pet_family->petTalentType) & talentTabInfo->petTalentMask))
            continue;

        for (int j = 0; j < MAX_TALENT_RANK; j++)
            if (talentInfo->RankID[j])
                removeSpell(talentInfo->RankID[j],!IsPassiveSpell(talentInfo->RankID[j]),false);
    }

    UpdateFreeTalentPoints(false);

    if(!no_cost)
    {
        player->ModifyMoney(-(int32)cost);

        m_resetTalentsCost = cost;
        m_resetTalentsTime = time(NULL);
    }
    player->PetSpellInitialize();
    return true;
}

void Pet::resetTalentsForAllPetsOf(Player* owner, Pet* online_pet /*= NULL*/)
{
    // not need after this call
    if(((Player*)owner)->HasAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS))
        ((Player*)owner)->RemoveAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS,true);

    // reset for online
    if(online_pet)
        online_pet->resetTalents(true);

    // now need only reset for offline pets (all pets except online case)
    uint32 except_petnumber = online_pet ? online_pet->GetCharmInfo()->GetPetNumber() : 0;

    QueryResult *resultPets = CharacterDatabase.PQuery(
        "SELECT id FROM character_pet WHERE owner = '%u' AND id <> '%u'",
        owner->GetGUIDLow(),except_petnumber);

    // no offline pets
    if(!resultPets)
        return;

    QueryResult *result = CharacterDatabase.PQuery(
        "SELECT DISTINCT pet_spell.spell FROM pet_spell, character_pet "
        "WHERE character_pet.owner = '%u' AND character_pet.id = pet_spell.guid AND character_pet.id <> %u",
        owner->GetGUIDLow(),except_petnumber);

    if(!result)
    {
        delete resultPets;
        return;
    }

    bool need_comma = false;
    std::ostringstream ss;
    ss << "DELETE FROM pet_spell WHERE guid IN (";

    do
    {
        Field *fields = resultPets->Fetch();

        uint32 id = fields[0].GetUInt32();

        if(need_comma)
            ss << ",";

        ss << id;

        need_comma = true;
    }
    while( resultPets->NextRow() );

    delete resultPets;

    ss << ") AND spell IN (";

    bool need_execute = false;
    do
    {
        Field *fields = result->Fetch();

        uint32 spell = fields[0].GetUInt32();

        if(!GetTalentSpellCost(spell))
            continue;

        if(need_execute)
            ss << ",";

        ss << spell;

        need_execute = true;
    }
    while( result->NextRow() );

    delete result;

    if(!need_execute)
        return;

    ss << ")";

    CharacterDatabase.Execute(ss.str().c_str());
}

void Pet::UpdateFreeTalentPoints(bool resetIfNeed)
{
    uint32 level = getLevel();
    uint32 talentPointsForLevel = GetMaxTalentPointsForLevel(level);
    // Reset talents in case low level (on level down) or wrong points for level (hunter can unlearn TP increase talent)
    if (talentPointsForLevel == 0 || m_usedTalentCount > talentPointsForLevel)
    {
        // Remove all talent points (except for admin pets)
        if (resetIfNeed)
        {
            Unit *owner = GetOwner();
            if (!owner || owner->GetTypeId() != TYPEID_PLAYER || ((Player*)owner)->GetSession()->GetSecurity() < SEC_ADMINISTRATOR)
                resetTalents(true);
            else
                SetFreeTalentPoints(0);
        }
        else
            SetFreeTalentPoints(0);
    }
    else
        SetFreeTalentPoints(talentPointsForLevel - m_usedTalentCount);
}


void Pet::InitTalentForLevel()
{
    UpdateFreeTalentPoints();

    Unit *owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    if(!m_loading)
        ((Player*)owner)->SendTalentsInfoData(true);
}

uint32 Pet::resetTalentsCost() const
{
    uint32 days = uint32(sWorld.GetGameTime() - m_resetTalentsTime)/DAY;

    // The first time reset costs 10 silver; after 1 day cost is reset to 10 silver
    if(m_resetTalentsCost < 10*SILVER || days > 0)
        return 10*SILVER;
    // then 50 silver
    else if(m_resetTalentsCost < 50*SILVER)
        return 50*SILVER;
    // then 1 gold
    else if(m_resetTalentsCost < 1*GOLD)
        return 1*GOLD;
    // then increasing at a rate of 1 gold; cap 10 gold
    else
        return (m_resetTalentsCost + 1*GOLD > 10*GOLD ? 10*GOLD : m_resetTalentsCost + 1*GOLD);
}

uint8 Pet::GetMaxTalentPointsForLevel(uint32 level)
{
    uint8 points = (level >= 20) ? ((level - 16) / 4) : 0;
    // Mod points from owner SPELL_AURA_MOD_PET_TALENT_POINTS
    if (Unit *owner = GetOwner())
        points+=owner->GetTotalAuraModifier(SPELL_AURA_MOD_PET_TALENT_POINTS);
    return points;
}

void Pet::ToggleAutocast(uint32 spellid, bool apply)
{
    if(IsPassiveSpell(spellid) || !isControlled())
        return;

    PetSpellMap::iterator itr = m_spells.find(spellid);

    if (itr == m_spells.end())
        return;

    uint32 i;

    if(apply)
    {
        for (i = 0; i < m_autospells.size() && m_autospells[i] != spellid; ++i)
            ;                                               // just search

        if (i == m_autospells.size())
        {
            m_autospells.push_back(spellid);

            if(itr->second.active != ACT_ENABLED)
            {
                itr->second.active = ACT_ENABLED;
                if(itr->second.state != PETSPELL_NEW)
                    itr->second.state = PETSPELL_CHANGED;
            }
        }
    }
    else
    {
        AutoSpellList::iterator itr2 = m_autospells.begin();
        for (i = 0; i < m_autospells.size() && m_autospells[i] != spellid; ++i, itr2++)
            ;                                               // just search

        if (i < m_autospells.size())
        {
            m_autospells.erase(itr2);
            if(itr->second.active != ACT_DISABLED)
            {
                itr->second.active = ACT_DISABLED;
                if(itr->second.state != PETSPELL_NEW)
                    itr->second.state = PETSPELL_CHANGED;
            }
        }
    }
}

bool Pet::IsPermanentPetFor(Player* owner)
{
    switch(getPetType())
    {
        case SUMMON_PET:
            switch(owner->getClass())
            {
                // oddly enough, Mage's Water Elemental is still treated as temporary pet with Glyph of Eternal Water
                // i.e. does not unsummon at mounting, gets dismissed at teleport etc.
                case CLASS_WARLOCK:
                    return GetCreatureInfo()->type == CREATURE_TYPE_DEMON;
                case CLASS_DEATH_KNIGHT:
                    return GetCreatureInfo()->type == CREATURE_TYPE_UNDEAD;
                default:
                    return false;
            }
        case HUNTER_PET:
            return true;
        default:
            return false;
    }
}

bool Pet::Create(Unit* owner, uint32 Entry)
{
    return Create(owner->GetMap()->GenerateLocalLowGuid(HIGHGUID_PET), owner->GetMap(),
            owner->GetPhaseMask(), Entry, sObjectMgr.GeneratePetNumber(), owner);
}

bool Pet::Create(uint32 guidlow, Map *map, uint32 phaseMask, uint32 Entry, uint32 pet_number, Unit* owner)
{

    if (!owner)
        return false;

    if (map)
        SetMap(map);
    else
        return false;

    m_loading = true;

    if (!guidlow)
        guidlow = map->GenerateLocalLowGuid(HIGHGUID_PET);

    if (!pet_number)
        pet_number = sObjectMgr.GeneratePetNumber();

    Object::_Create(ObjectGuid(HIGHGUID_PET, pet_number, guidlow));

    m_DBTableGuid = guidlow;
    m_originalEntry = Entry;

    if(!InitEntry(Entry))
        return false;

    SetPhaseMask(phaseMask,false);

    if(owner->GetTypeId() == TYPEID_PLAYER)
        m_charmInfo->SetPetNumber(pet_number, IsPermanentPetFor((Player*)owner));
    else
        m_charmInfo->SetPetNumber(pet_number, false);

    setFaction(owner->getFaction());
                                   // Faction be owerwritten later, if ForceFaction present

    SetOwnerGuid(owner->GetObjectGuid());
    SetCreatorGuid(owner->GetObjectGuid());

    if (GetCreateSpellID())
        SetUInt32Value(UNIT_CREATED_BY_SPELL, GetCreateSpellID());


    SetSheath(SHEATH_STATE_MELEE);

    if(getPetType() == MINI_PET)                            // always non-attackable
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

    return true;
}

bool Pet::HasSpell(uint32 spell) const
{
    PetSpellMap::const_iterator itr = m_spells.find(spell);
    return (itr != m_spells.end() && itr->second.state != PETSPELL_REMOVED );
}

// Get all passive spells in our skill line
void Pet::LearnPetPassives()
{
    CreatureInfo const* cInfo = GetCreatureInfo();
    if(!cInfo)
        return;

    CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cInfo->family);
    if(!cFamily)
        return;

    PetFamilySpellsStore::const_iterator petStore = sPetFamilySpellsStore.find(cFamily->ID);
    if(petStore != sPetFamilySpellsStore.end())
    {
        for(PetFamilySpellsSet::const_iterator petSet = petStore->second.begin(); petSet != petStore->second.end(); ++petSet)
            addSpell(*petSet, ACT_DECIDE, PETSPELL_NEW, PETSPELL_FAMILY);
    }
}

void Pet::CastPetAuras(bool current)
{
    Unit* owner = GetOwner();
    if(!owner || owner->GetTypeId()!=TYPEID_PLAYER)
        return;

    for(PetAuraSet::const_iterator itr = owner->m_petAuras.begin(); itr != owner->m_petAuras.end();)
    {
        PetAura const* pa = *itr;
        ++itr;

        if(!current && pa->IsRemovedOnChangePet())
            owner->RemovePetAura(pa);
        else
            CastPetAura(pa);
    }
}

void Pet::CastPetAura(PetAura const* aura)
{
    uint32 auraId = aura->GetAura(GetEntry());
    if(!auraId)
        return;

    if(auraId == 35696)                                       // Demonic Knowledge
    {
        int32 basePoints = int32(aura->GetDamage() * (GetStat(STAT_STAMINA) + GetStat(STAT_INTELLECT)) / 100);
        CastCustomSpell(this, auraId, &basePoints, NULL, NULL, true);
    }
    else
        CastSpell(this, auraId, true);
}

struct DoPetLearnSpell
{
    DoPetLearnSpell(Pet& _pet) : pet(_pet) {}
    void operator() (uint32 spell_id) { pet.learnSpell(spell_id); }
    Pet& pet;
};

void Pet::learnSpellHighRank(uint32 spellid)
{
    learnSpell(spellid);

    DoPetLearnSpell worker(*this);
    sSpellMgr.doForHighRanks(spellid,worker);
}

void Pet::SynchronizeLevelWithOwner()
{
    Unit* owner = GetOwner();
    if (!owner)
        return;

    if (owner->GetTypeId() != TYPEID_PLAYER)
    {
        GivePetLevel(owner->getLevel());
        return;
    }

    switch(getPetType())
    {
        // always same level
        case SUMMON_PET:
            GivePetLevel(owner->getLevel());
            break;
        // can't be greater owner level
        case HUNTER_PET:
            if (getLevel() > owner->getLevel())
                GivePetLevel(owner->getLevel());
            else if (getLevel() + 5 < owner->getLevel())
                GivePetLevel(owner->getLevel() - 5);
            break;
        default:
            break;
    }
}

void Pet::ApplyModeFlags(PetModeFlags mode, bool apply)
{
    if (apply)
        m_petModeFlags = PetModeFlags(m_petModeFlags | mode);
    else
        m_petModeFlags = PetModeFlags(m_petModeFlags & ~mode);

    Unit* owner = GetOwner();
    if(!owner || owner->GetTypeId()!=TYPEID_PLAYER)
        return;

    WorldPacket data(SMSG_PET_MODE, 12);
    data << GetObjectGuid();
    data << uint32(m_petModeFlags);
    ((Player*)owner)->GetSession()->SendPacket(&data);
}

bool Pet::SetSummonPosition(float x, float y, float z)
{
    Unit* owner = GetOwner();

    if (!owner)
        return false;

    float px, py, pz;

    // Summon location setting
    if (GetPetCounter() == 1)
        SetPetFollowAngle(PET_FOLLOW_ANGLE*3);
    else if (GetPetCounter() == 2)
        SetPetFollowAngle(PET_FOLLOW_ANGLE*2);
    else
        SetPetFollowAngle(PET_FOLLOW_ANGLE);

    if (getPetType() == MINI_PET)
        SetPetFollowAngle(M_PI_F*1.25f);


    if (x == 0.0f && y == 0.0f && z == 0.0f)
        owner->GetClosePoint(x, y, z, GetObjectBoundingRadius()*4, PET_FOLLOW_DIST, GetPetFollowAngle());

    GetRandomPoint(x, y, z, GetObjectBoundingRadius()*4, px, py, pz);

    Relocate(px, py, pz, -owner->GetOrientation());
    SetSummonPoint(px, py, pz, -owner->GetOrientation());

    if (!IsPositionValid()) return false;
        else return true;
}

void Pet::ApplyStatScalingBonus(Stats stat, bool apply)
{
    if(stat > STAT_SPIRIT || stat < STAT_STRENGTH )
        return;

    Unit* owner = GetOwner();

    // Don't apply scaling bonuses if no owner or owner is not player
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER || m_removed)
        return;

    UnitMods unitMod = UnitMods(stat);

    int32 newStat = owner->GetTotalStatValue(stat);

    if (m_baseBonusData->statScale[stat] == newStat && !apply)
        return;

    m_baseBonusData->statScale[stat] = newStat;

    int32 basePoints = int32(m_baseBonusData->statScale[stat] * (CalculateScalingData()->statScale[stat] / 100.0f));

    bool needRecalculateStat = false;

    if (basePoints == 0)
        needRecalculateStat = true;

    AuraList const& scalingAuras = GetAurasByType(SPELL_AURA_MOD_STAT);

    for (AuraList::const_iterator itr = scalingAuras.begin(); itr != scalingAuras.end(); ++itr)
    {
        Aura* _aura = (*itr);
        if (!_aura || _aura->IsInUse())
            continue;

        SpellAuraHolder* holder = _aura->GetHolder();

        if (!holder || holder->IsDeleted() || holder->IsEmptyHolder() || holder->GetCasterGUID() != GetGUID())
            continue;

        SpellEntry const *spellproto = holder->GetSpellProto();

        if (!spellproto)
            continue;

        SpellEffectIndex i = _aura->GetEffIndex();

        if (Stats(spellproto->EffectMiscValue[i]) == stat
                && spellproto->EffectBasePoints[i] == 0)
        {
            SetCanModifyStats(false);
            if (ReapplyScalingAura(holder, spellproto, i, basePoints))
                needRecalculateStat = true;
            SetCanModifyStats(true);
            break;
        }
    }
    if(needRecalculateStat)
        UpdateStats(stat);
}

void Pet::ApplyResistanceScalingBonus(uint32 school, bool apply)
{
    if(school < SPELL_SCHOOL_NORMAL || school > SPELL_SCHOOL_ARCANE)
        return;

    Unit* owner = GetOwner();

    // Don't apply scaling bonuses if no owner or owner is not player
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER || m_removed)
        return;

    int32 newResistance;

    if (school == SPELL_SCHOOL_NORMAL)
        newResistance = owner->GetArmor();
    else
        newResistance = owner->GetResistance(SpellSchools(school));

    if (m_baseBonusData->resistanceScale[school] == newResistance && !apply)
        return;

    m_baseBonusData->resistanceScale[school] = newResistance;

    int32 basePoints = int32(m_baseBonusData->resistanceScale[school] * (CalculateScalingData()->resistanceScale[school] / 100.0f));

    bool needRecalculateStat = false;

    if (basePoints == 0)
        needRecalculateStat = true;

    AuraList const& scalingAuras = GetAurasByType(SPELL_AURA_MOD_RESISTANCE);

    for(AuraList::const_iterator itr = scalingAuras.begin(); itr != scalingAuras.end(); ++itr)
    {
        Aura* _aura = (*itr);
        if (!_aura || _aura->IsInUse())
            continue;

        SpellAuraHolder* holder = _aura->GetHolder();

        if (!holder || holder->IsDeleted() || holder->IsEmptyHolder() || holder->GetCasterGUID() != GetGUID())
            continue;

        SpellEntry const *spellproto = holder->GetSpellProto();

        if (!spellproto)
            continue;

        SpellEffectIndex i = _aura->GetEffIndex();

        if (spellproto->EffectBasePoints[i] == 0
            && (spellproto->EffectMiscValue[i] & (1 << SpellSchools(school))))
        {
            SetCanModifyStats(false);
            if (ReapplyScalingAura(holder, spellproto, i, basePoints))
                needRecalculateStat = true;
            SetCanModifyStats(true);
            break;
        }
    }

    if(needRecalculateStat)
    {
        if (school == SPELL_SCHOOL_NORMAL)
            UpdateArmor();
        else
            UpdateResistances(school);
    }
}

void Pet::ApplyAttackPowerScalingBonus(bool apply)
{
    Unit* owner = GetOwner();

    // Don't apply scaling bonuses if no owner or owner is not player
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER || m_removed)
        return;

    int32 newAPBonus;

    switch(getPetType())
    {
        case GUARDIAN_PET:
        case PROTECTOR_PET:
        {
            if (owner->getClass() == CLASS_SHAMAN)
            {
                newAPBonus = owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_NATURE);
                break;
            }
                             // No break another case!
        }
        case SUMMON_PET:
        {
            switch(owner->getClass())
            {
                case CLASS_WARLOCK:
                {
                    newAPBonus = std::max(owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_SHADOW),owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE));
                    break;
                }
                case CLASS_DEATH_KNIGHT:
                    newAPBonus = owner->GetTotalAttackPowerValue(BASE_ATTACK);
                    break;
                case CLASS_PRIEST:
                    newAPBonus = owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_SHADOW);
                    break;
                case CLASS_SHAMAN:
                    newAPBonus = owner->GetTotalAttackPowerValue(BASE_ATTACK);
                    break;
                case CLASS_MAGE:
                {
                   newAPBonus = std::max(owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FROST),owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE));
                   break;
                }
                default:
                    newAPBonus = 0;
                    break;
            }
            break;
        }
        case HUNTER_PET:
            newAPBonus = owner->GetTotalAttackPowerValue(RANGED_ATTACK);
            break;
        default:
            newAPBonus = 0;
            break;
    }

    if(newAPBonus < 0)
        newAPBonus = 0;

    if (m_baseBonusData->attackpowerScale == newAPBonus && !apply)
        return;

    m_baseBonusData->attackpowerScale = newAPBonus;

    int32 basePoints = int32(m_baseBonusData->attackpowerScale * (CalculateScalingData()->attackpowerScale / 100.0f));

    bool needRecalculateStat = false;

    if (basePoints == 0)
        needRecalculateStat = true;

    AuraList const& scalingAuras = GetAurasByType(SPELL_AURA_MOD_ATTACK_POWER);

    for(AuraList::const_iterator itr = scalingAuras.begin(); itr != scalingAuras.end(); ++itr)
    {
        Aura* _aura = (*itr);
        if (!_aura || _aura->IsInUse())
            continue;

        SpellAuraHolder* holder = _aura->GetHolder();

        if (!holder || holder->IsDeleted() || holder->IsEmptyHolder() || holder->GetCasterGUID() != GetGUID())
            continue;

        SpellEntry const *spellproto = holder->GetSpellProto();

        if (!spellproto)
            continue;

        SpellEffectIndex i = _aura->GetEffIndex();

        if (spellproto->EffectBasePoints[i] == 0)
        {
            SetCanModifyStats(false);
            if (ReapplyScalingAura(holder, spellproto, i, basePoints))
                needRecalculateStat = true;
            SetCanModifyStats(true);
            break;
        }
    }

    if(needRecalculateStat)
    {
        UpdateAttackPowerAndDamage();
        UpdateAttackPowerAndDamage(true);
    }
}

void Pet::ApplyDamageScalingBonus(bool apply)
{
    // SpellPower for pets exactly same DamageBonus.
    //    m_baseBonusData->damageScale
    Unit* owner = GetOwner();

    // Don't apply scaling bonuses if no owner or owner is not player
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER || m_removed)
        return;

    int32 newDamageBonus;

    switch(getPetType())
    {
        case SUMMON_PET:
        case GUARDIAN_PET:
        case PROTECTOR_PET:
        case HUNTER_PET:
        {
            switch(owner->getClass())
            {
                case CLASS_DEATH_KNIGHT:
                    newDamageBonus = owner->GetTotalAttackPowerValue(BASE_ATTACK);
                    break;
                case CLASS_PRIEST:
                    newDamageBonus = owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_SHADOW);
                    break;
                default:
                    newDamageBonus = 0;
                    break;
            }
            break;
        }
        default:
            newDamageBonus = 0;
            break;
    }

    if (newDamageBonus < 0)
        newDamageBonus = 0;

    if (m_baseBonusData->damageScale == newDamageBonus && !apply)
        return;

    m_baseBonusData->damageScale = newDamageBonus;

    int32 basePoints = int32(m_baseBonusData->damageScale * (CalculateScalingData()->damageScale / 100.0f));

    bool needRecalculateStat = false;

    if (basePoints == 0)
        needRecalculateStat = true;

    AuraList const& scalingAuras = GetAurasByType(SPELL_AURA_MOD_DAMAGE_DONE);

    for(AuraList::const_iterator itr = scalingAuras.begin(); itr != scalingAuras.end(); ++itr)
    {
        Aura* _aura = (*itr);
        if (!_aura || _aura->IsInUse())
            continue;

        SpellAuraHolder* holder = _aura->GetHolder();

        if (!holder || holder->IsDeleted() || holder->IsEmptyHolder() || holder->GetCasterGUID() != GetGUID())
            continue;

        SpellEntry const *spellproto = holder->GetSpellProto();

        if (!spellproto)
            continue;

        SpellEffectIndex i = _aura->GetEffIndex();
                                                                            // First scan aura with 127 mask
        if (spellproto->EffectBasePoints[i] == 0
                && spellproto->EffectMiscValue[i] == SPELL_SCHOOL_MASK_ALL)
        {
            SetCanModifyStats(false);
            if (ReapplyScalingAura(holder, spellproto, i, basePoints))
                needRecalculateStat = true;
            SetCanModifyStats(true);
            break;
        }
    }

    if (needRecalculateStat)
    {
        UpdateDamagePhysical(BASE_ATTACK);
        UpdateDamagePhysical(RANGED_ATTACK);
    }
}

void Pet::ApplySpellDamageScalingBonus(bool apply)
{
    Unit* owner = GetOwner();

    // Don't apply scaling bonuses if no owner or owner is not player
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER || m_removed)
        return;

    int32 newDamageBonus;

    switch(getPetType())
    {
        case GUARDIAN_PET:
        case PROTECTOR_PET:
        {
            if (owner->getClass() == CLASS_SHAMAN)
            {
                newDamageBonus = owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_NATURE);
                break;
            }
                             // No break another case!
        }
        case SUMMON_PET:
        {
            switch(owner->getClass())
            {
                case CLASS_WARLOCK:
                    newDamageBonus = std::max(owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_SHADOW),owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE));
                    break;
                case CLASS_PRIEST:
                    newDamageBonus = owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_SHADOW);
                    break;
                case CLASS_DEATH_KNIGHT:
                    newDamageBonus = owner->GetTotalAttackPowerValue(BASE_ATTACK);
                    break;
                case CLASS_SHAMAN:
                    newDamageBonus = owner->GetTotalAttackPowerValue(BASE_ATTACK);
                    break;
                case CLASS_MAGE:
                    newDamageBonus = std::max(owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FROST),owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE));
                    break;
                default:
                    newDamageBonus = 0;
                    break;
            }
            break;
        }
        case HUNTER_PET:
            newDamageBonus = owner->GetTotalAttackPowerValue(RANGED_ATTACK);
            break;
        default:
            newDamageBonus = 0;
            break;
    }

    if (newDamageBonus < 0)
        newDamageBonus = 0;

    if (m_baseBonusData->damageScale == newDamageBonus && !apply)
        return;

    m_baseBonusData->spelldamageScale = newDamageBonus;

    int32 basePoints = int32(m_baseBonusData->spelldamageScale * (CalculateScalingData()->spelldamageScale / 100.0f));

    bool needRecalculateStat = false;

    if (basePoints == 0)
        needRecalculateStat = true;

    AuraList const& scalingAuras = GetAurasByType(SPELL_AURA_MOD_DAMAGE_DONE);

    for(AuraList::const_iterator itr = scalingAuras.begin(); itr != scalingAuras.end(); ++itr)
    {
        Aura* _aura = (*itr);
        if (!_aura || _aura->IsInUse())
            continue;

        SpellAuraHolder* holder = _aura->GetHolder();

        if (!holder || holder->IsDeleted() || holder->IsEmptyHolder() || holder->GetCasterGUID() != GetGUID())
            continue;

        SpellEntry const *spellproto = holder->GetSpellProto();

        if (!spellproto)
            continue;

        SpellEffectIndex i = _aura->GetEffIndex();

        if (spellproto->EffectBasePoints[i] == 0
            && spellproto->EffectMiscValue[i] == SPELL_SCHOOL_MASK_MAGIC)
        {
            SetCanModifyStats(false);
            if (ReapplyScalingAura(holder, spellproto, i, basePoints))
                needRecalculateStat = true;
            SetCanModifyStats(true);
            break;
        }
    }

    if (needRecalculateStat)
        UpdateSpellPower();
}


void Pet::ApplyAllScalingBonuses(bool apply)
{
    for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
        ApplyStatScalingBonus(Stats(i),apply);

    for (int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        ApplyResistanceScalingBonus(SpellSchools(i), apply);

    ApplyAttackPowerScalingBonus(apply);
    ApplySpellDamageScalingBonus(apply);
    ApplyDamageScalingBonus(apply);
    ApplyHitScalingBonus(apply);
    ApplySpellHitScalingBonus(apply);
    ApplyExpertizeScalingBonus(apply);
    ApplyPowerregenScalingBonus(apply);
}

void Pet::ApplyHitScalingBonus(bool apply)
{
    Unit* owner = GetOwner();

    // Don't apply scaling bonuses if no owner or owner is not player
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER || m_removed)
        return;
    int32 m_MeleeHitChance = owner->GetTotalAuraModifier(SPELL_AURA_MOD_HIT_CHANCE);
    m_MeleeHitChance +=  ((Player*)owner)->GetRatingBonusValue(CR_HIT_MELEE);

    if (m_baseBonusData->meleeHitScale == m_MeleeHitChance && !apply)
        return;

    m_baseBonusData->meleeHitScale = m_MeleeHitChance;

    int32 basePoints = int32(m_baseBonusData->meleeHitScale * (CalculateScalingData()->meleeHitScale / 100.0f));

    bool needRecalculateStat = false;

    if (basePoints == 0)
        needRecalculateStat = true;

    AuraList const& scalingAuras = GetAurasByType(SPELL_AURA_MOD_HIT_CHANCE);

    for(AuraList::const_iterator itr = scalingAuras.begin(); itr != scalingAuras.end(); ++itr)
    {
        Aura* _aura = (*itr);
        if (!_aura || _aura->IsInUse())
            continue;

        SpellAuraHolder* holder = _aura->GetHolder();

        if (!holder || holder->IsDeleted() || holder->IsEmptyHolder() || holder->GetCasterGUID() != GetGUID())
            continue;

        SpellEntry const *spellproto = holder->GetSpellProto();

        if (!spellproto)
            continue;

        SpellEffectIndex i = _aura->GetEffIndex();

        if (spellproto->EffectBasePoints[i] == 0)
        {
            SetCanModifyStats(false);
            if (ReapplyScalingAura(holder, spellproto, i, basePoints))
                needRecalculateStat = true;
            SetCanModifyStats(true);
            break;
        }
    }
}

void Pet::ApplySpellHitScalingBonus(bool apply)
{
    Unit* owner = GetOwner();

    // Don't apply scaling bonuses if no owner or owner is not player
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER || m_removed)
        return;

    int32 m_SpellHitChance = owner->GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_HIT_CHANCE);
    m_SpellHitChance += ((Player*)owner)->GetRatingBonusValue(CR_HIT_SPELL);

    if (m_baseBonusData->spellHitScale == m_SpellHitChance && !apply)
        return;

    m_baseBonusData->spellHitScale = m_SpellHitChance;

    int32 basePoints = int32(m_baseBonusData->spellHitScale * (CalculateScalingData()->spellHitScale / 100.0f));

    bool needRecalculateStat = false;

    if (basePoints == 0)
        needRecalculateStat = true;

    AuraList const& scalingAuras = GetAurasByType(SPELL_AURA_MOD_SPELL_HIT_CHANCE);

    for(AuraList::const_iterator itr = scalingAuras.begin(); itr != scalingAuras.end(); ++itr)
    {
        Aura* _aura = (*itr);
        if (!_aura || _aura->IsInUse())
            continue;

        SpellAuraHolder* holder = _aura->GetHolder();

        if (!holder || holder->IsDeleted() || holder->IsEmptyHolder() || holder->GetCasterGUID() != GetGUID())
            continue;

        SpellEntry const *spellproto = holder->GetSpellProto();

        if (!spellproto)
            continue;

        SpellEffectIndex i = _aura->GetEffIndex();

        if (spellproto->EffectBasePoints[i] == 0)
        {
            SetCanModifyStats(false);
            if (ReapplyScalingAura(holder, spellproto, i, basePoints))
                needRecalculateStat = true;
            SetCanModifyStats(true);
            break;
        }
    }
}

void Pet::ApplyExpertizeScalingBonus(bool apply)
{
    Unit* owner = GetOwner();

    // Don't apply scaling bonuses if no owner or owner is not player
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER || m_removed)
        return;
    int32 m_expertize = owner->GetUInt32Value(PLAYER_EXPERTISE);

    if (m_baseBonusData->expertizeScale == m_expertize && !apply)
        return;

    m_baseBonusData->expertizeScale = m_expertize;

    int32 basePoints = int32(m_baseBonusData->expertizeScale * (CalculateScalingData()->expertizeScale / 100.0f));

    bool needRecalculateStat = false;

    if (basePoints == 0)
        needRecalculateStat = true;

    AuraList const& scalingAuras = GetAurasByType(SPELL_AURA_MOD_EXPERTISE);

    for(AuraList::const_iterator itr = scalingAuras.begin(); itr != scalingAuras.end(); ++itr)
    {
        Aura* _aura = (*itr);
        if (!_aura || _aura->IsInUse())
            continue;

        SpellAuraHolder* holder = _aura->GetHolder();

        if (!holder || holder->IsDeleted() || holder->IsEmptyHolder() || holder->GetCasterGUID() != GetGUID())
            continue;

        SpellEntry const *spellproto = holder->GetSpellProto();

        if (!spellproto)
            continue;

        SpellEffectIndex i = _aura->GetEffIndex();

        if (spellproto->EffectBasePoints[i] == 0)
        {
            SetCanModifyStats(false);
            if (ReapplyScalingAura(holder, spellproto, i, basePoints))
                needRecalculateStat = true;
            SetCanModifyStats(true);
            break;
        }
    }

}

void Pet::ApplyPowerregenScalingBonus(bool apply)
{
    Unit* owner = GetOwner();

    // Don't apply scaling bonuses if no owner or owner is not player
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER || m_removed)
        return;

    int32 m_manaregen = int32(owner->GetFloatValue(UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER));

    if (m_baseBonusData->powerregenScale == m_manaregen && !apply)
        return;

    m_baseBonusData->powerregenScale = m_manaregen;

    int32 basePoints = int32(m_baseBonusData->powerregenScale * (CalculateScalingData()->powerregenScale / 100.0f));

    bool needRecalculateStat = false;

    if (basePoints == 0)
        needRecalculateStat = true;

    AuraList const& scalingAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN);

    for(AuraList::const_iterator itr = scalingAuras.begin(); itr != scalingAuras.end(); ++itr)
    {
        Aura* _aura = (*itr);
        if (!_aura || _aura->IsInUse())
            continue;

        SpellAuraHolder* holder = _aura->GetHolder();

        if (!holder || holder->IsDeleted() || holder->IsEmptyHolder() || holder->GetCasterGUID() != GetGUID())
            continue;

        SpellEntry const *spellproto = holder->GetSpellProto();

        if (!spellproto)
            continue;

        SpellEffectIndex i = _aura->GetEffIndex();

        if (spellproto->EffectBasePoints[i] == 0)
        {
            SetCanModifyStats(false);
            if (ReapplyScalingAura(holder, spellproto, i, basePoints))
                needRecalculateStat = true;
            SetCanModifyStats(true);
            break;
        }
    }

    if(needRecalculateStat)
        UpdateManaRegen();
}

bool Pet::Summon()
{
    Unit* owner = GetOwner();

    if (!owner)
        return false;

    Map* map = GetMap();

    if (!map)
        return false;

    uint16 level = getLevel() ? getLevel() : owner->getLevel();;

    if (GetCreateSpellID())
        SetUInt32Value(UNIT_CREATED_BY_SPELL, GetCreateSpellID());

    if (isTemporarySummoned() && getPetType() !=  PROTECTOR_PET)
        GetCharmInfo()->SetReactState(REACT_AGGRESSIVE);
    else
        GetCharmInfo()->SetReactState(REACT_DEFENSIVE);

    SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL)));

    switch (getPetType())
    {
        case GUARDIAN_PET:
        case PROTECTOR_PET:
        {
            SetUInt32Value(UNIT_NPC_FLAGS, GetCreatureInfo()->npcflag);
            SetUInt32Value(UNIT_FIELD_FLAGS, 0);
            SetUInt32Value(UNIT_FIELD_BYTES_1, 0);
            SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
//            SetName("");
            SetNeedSave(false);
            owner->AddGuardian(this);
            break;
        }
        case SUMMON_PET:
        {
            level = owner->getLevel();
            SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
            SetUInt32Value(UNIT_FIELD_BYTES_0, 2048);
            SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
            SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, 1000);
//            SetName("");
            SetNeedSave(true);
            owner->SetPet(this);
            break;
        }
        case HUNTER_PET:  // Called only if new tamed pet created
        {
            SetSheath(SHEATH_STATE_MELEE);
            SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
            SetUInt32Value(UNIT_FIELD_BYTES_0, 0x02020100);
            SetByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED | UNIT_CAN_BE_ABANDONED);
            SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
            SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, sObjectMgr.GetXPForPetLevel(getLevel()));
            SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, time(NULL));
            SetMaxPower(POWER_HAPPINESS, GetCreatePowers(POWER_HAPPINESS));
            SetPower(POWER_HAPPINESS, HAPPINESS_LEVEL_SIZE);
            SetNeedSave(true);
            owner->SetPet(this);
            break;
        }
        case MINI_PET:
        {
            SelectLevel(GetCreatureInfo());
            SetUInt32Value(UNIT_NPC_FLAGS, GetCreatureInfo()->npcflag);
            SetName("");
            owner->SetMiniPet((Unit*)this);
            InitPetCreateSpells();
            AIM_Initialize();
            SetNeedSave(false);
            map->Add((Creature*)this);
            m_loading = false;
            return true;
            break;
        }
        default:
        {
            sLog.outError("Pet have incorrect type (%u) for pet creating.", getPetType());
            return false;
        }
    }

    if (owner->GetTypeId() == TYPEID_PLAYER)
        ((Player*)owner)->AddKnownPetName(GetCharmInfo()->GetPetNumber(),GetName());

    if(owner->IsPvP())
        SetPvP(true);

    if(owner->IsFFAPvP())
        SetFFAPvP(true);

    SetCanModifyStats(true);
    InitStatsForLevel(level, owner);
    InitTalentForLevel();
    InitPetCreateSpells();
    SynchronizeLevelWithOwner();
    InitLevelupSpellsForLevel();
    LearnPetPassives();
    CastPetAuras(true);
    if (getPetType() != HUNTER_PET)
        LoadCreatureAddon(true);

    if (owner->GetTypeId() == TYPEID_PLAYER)
    {
        CastPetPassiveAuras(true);
        ApplyAllScalingBonuses(true);
    }

    SetHealth(GetMaxHealth());
    SetPower(getPowerType(), GetMaxPower(getPowerType()));
    UpdateWalkMode(owner);
    AIM_Initialize();

    map->Add((Creature*)this);

    m_loading = false;

    if (owner->GetTypeId() == TYPEID_PLAYER)
    {
        CleanupActionBar();                                     // remove unknown spells from action bar after load
        if (isControlled() && !GetPetCounter())
        {
            if (!GetPetCounter())
                ((Player*)owner)->PetSpellInitialize();
            ((Player*)owner)->SendTalentsInfoData(true);
        }

        if(((Player*)owner)->GetGroup())
            ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_PET);

        if (!GetPetCounter() && getPetType() == HUNTER_PET)
            SavePetToDB(PET_SAVE_AS_CURRENT);
        else if (getPetType() == SUMMON_PET)
            SavePetToDB(PET_SAVE_NOT_IN_SLOT);
        else
            SetNeedSave(false);
    }
    else
    {
        SetNeedSave(false);
        if (((Creature*)owner)->AI())
            ((Creature*)owner)->AI()->JustSummoned((Creature*)this);
    }

    return true;
}

Unit* Pet::GetOwner() const
{
    Unit* owner = Unit::GetOwner();

    if (!owner)
        if (!GetOwnerGuid().IsEmpty())
            if (Map* pMap = GetMap())
                owner = pMap->GetAnyTypeCreature(GetOwnerGuid());

    if (owner && owner->GetTypeId() == TYPEID_UNIT && ((Creature*)owner)->IsTotem())
        if (Unit* ownerOfOwner = owner->GetOwner())
            return ownerOfOwner;

    if (owner)
        return owner;

    return NULL;
}


bool Pet::ReapplyScalingAura(SpellAuraHolder* holder, SpellEntry const *spellproto, SpellEffectIndex index, int32 basePoints)
{
    if (!holder || holder->IsDeleted() || holder->IsEmptyHolder() || holder->IsInUse())
        return false;

    holder->SetInUse(true);

    Aura* oldaura = holder->GetAuraByEffectIndex(index);

    if (oldaura)
    {
//    RemoveSingleAuraFromSpellAuraHolder(holder, index, AURA_REMOVE_BY_STACK);
        RemoveAura(oldaura, AURA_REMOVE_BY_STACK);
    }

    Aura* aura = CreateAura(spellproto, index, &basePoints, holder, this, this, NULL);
    aura->SetAuraDuration(aura->GetAuraMaxDuration());
    holder->AddAura(aura, index);
    AddAuraToModList(aura);
    aura->ApplyModifier(true,true);

    holder->SetInUse(false);

    return true;
}

void Pet::CastPetPassiveAuras(bool current)
{
    Unit* owner = GetOwner();

    if(!owner || owner->GetTypeId()!=TYPEID_PLAYER)
        return;

    // Cast pet passive aura (if not casted as passive)
    uint32 creature_id;

    switch(getPetType())
    {
        case SUMMON_PET:
        case GUARDIAN_PET:
        case PROTECTOR_PET:
            creature_id = GetEntry();
            break;
        case HUNTER_PET:
            creature_id = 1;
            break;
        default:
            creature_id = 0;
            break;
    }

    PetPassiveAuraList const* pPassiveAuraList  =  sSpellMgr.GetPetPassiveAuraList(creature_id);

    if (!pPassiveAuraList || pPassiveAuraList->empty())
        return;

    for (PetPassiveAuraList::const_iterator itr = pPassiveAuraList->begin(); itr != pPassiveAuraList->end(); ++itr)
    {
        PetAura const petAura = *itr;

        uint32 auraID = petAura.GetAura(creature_id);

        if (!current && HasAura(auraID))
            RemoveAurasDueToSpell(auraID);
        else if (current && !HasAura(auraID))
        {
            CastSpell(this, auraID, true);
            DEBUG_LOG("Cast passive pet aura %u", auraID);
        }
    }
}

PetScalingData* Pet::CalculateScalingData(bool recalculate)
{
    if (m_PetScalingData && !recalculate)
        return m_PetScalingData;

    delete m_PetScalingData;

    m_PetScalingData = new PetScalingData;

    Unit* owner = GetOwner();

    PetScalingDataList const* pScalingDataList;

    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)        // if no owner ising record for creature_id = 0. Must be exist.
        pScalingDataList = sObjectMgr.GetPetScalingData(0);
    else if (getPetType() == HUNTER_PET)                      // Using creature_id = 1 for hunter pets
        pScalingDataList = sObjectMgr.GetPetScalingData(1);
    else if (getPetType() == SUMMON_PET || getPetType() == GUARDIAN_PET || getPetType() == PROTECTOR_PET )
    {
        pScalingDataList = sObjectMgr.GetPetScalingData(GetEntry());
        if (!pScalingDataList)
        {
            DEBUG_LOG("No data list for pet %u! Get zero values", GetEntry());
            pScalingDataList = sObjectMgr.GetPetScalingData(0);
        }
    }
    else
    {
        DEBUG_LOG("No selection type data list for pet %u! Get zero values", GetEntry());
        pScalingDataList = sObjectMgr.GetPetScalingData(0);
    }

    if (!pScalingDataList || pScalingDataList->empty())                            // Zero values...
        return m_PetScalingData;

    for (PetScalingDataList::const_iterator itr = pScalingDataList->begin(); itr != pScalingDataList->end(); ++itr)
    {
         const PetScalingData* pData = &*itr;

         if (!pData->creatureID || (owner && (!pData->requiredAura || owner->HasSpell(pData->requiredAura) || owner->HasAura(pData->requiredAura) || HasSpell(pData->requiredAura) || HasAura(pData->requiredAura))))
         {
             m_PetScalingData->healthBasepoint  += pData->healthBasepoint;
             m_PetScalingData->healthScale      += pData->healthScale;
             m_PetScalingData->powerBasepoint   += pData->powerBasepoint;
             m_PetScalingData->powerScale       += pData->powerScale;
             m_PetScalingData->APBasepoint      += pData->APBasepoint;
             m_PetScalingData->APBaseScale      += pData->APBaseScale;
             m_PetScalingData->attackpowerScale += pData->attackpowerScale;
             m_PetScalingData->damageScale      += pData->damageScale;
             m_PetScalingData->spelldamageScale += pData->spelldamageScale;
             m_PetScalingData->spellHitScale    += pData->spellHitScale;
             m_PetScalingData->meleeHitScale    += pData->meleeHitScale;
             m_PetScalingData->expertizeScale   += pData->expertizeScale;
             m_PetScalingData->attackspeedScale += pData->attackspeedScale;
             m_PetScalingData->critScale        += pData->critScale;
             m_PetScalingData->powerregenScale  += pData->powerregenScale;
             for (int i = 0; i < MAX_STATS; i++)
             {
                  m_PetScalingData->statScale[i] += pData->statScale[i];
             }
             for (int i = 0; i < MAX_SPELL_SCHOOL; i++)
             {
                  m_PetScalingData->resistanceScale[i] += pData->resistanceScale[i];
             }
         }
    }
    return m_PetScalingData;
}

// diff contains the time in milliseconds since last regen.
void Pet::Regenerate(Powers power, uint32 diff)
{
    int32 curValue = GetPower(power);
    uint32 maxValue = GetMaxPower(power);

    float addvalue = 0.0f;

    switch (power)
    {
        case POWER_MANA:
        {
            float ManaIncreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_MANA);
            if (IsUnderLastManaUseEffect())
            {
                // Mangos Updates Mana in intervals of 2s, which is correct
                addvalue = GetFloatValue(UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER) *  ManaIncreaseRate * 2.00f;
            }
            else
            {
                addvalue = GetFloatValue(UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER) * ManaIncreaseRate * 2.00f;
            }
            break;
        }
        case POWER_RAGE:                                    // Regenerate rage ?
        {
            addvalue = -20 * sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RAGE_LOSS);
            break;
        }
        case POWER_ENERGY:                                  // Regenerate energy (ghoul)
        {
            addvalue = 20 * sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_ENERGY);
            break;
        }
        case POWER_FOCUS:                                   // Hunter pets
        {
            addvalue = 10 * sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_FOCUS);
            break;
        }

        case POWER_HAPPINESS:
        {
            addvalue = -178.0f;                                   //value is 70/35/17/8/4 (per min) * 1000 / 8 (timer 2 secs)
            if(isInCombat())                                     //we know in combat happiness fades faster, multiplier guess
                addvalue = addvalue * 1.5f;
            ApplyHappinessBonus(true);
            break;
        }
        case POWER_RUNIC_POWER:
        case POWER_RUNE:
        case POWER_HEALTH:
        default:
            addvalue = 0.0f;
            break;
    }

    // Mana regen calculated in Pet::UpdateManaRegen()
    // Exist only for POWER_MANA, POWER_ENERGY, POWER_FOCUS auras
    if(power != POWER_MANA)
    {
        AuraList const& ModPowerRegenAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN);
        for(AuraList::const_iterator i = ModPowerRegenAuras.begin(); i != ModPowerRegenAuras.end(); ++i)
            if ((*i)->GetModifier()->m_miscvalue == (int32)power)
                addvalue += (*i)->GetModifier()->m_amount;

        AuraList const& ModPowerRegenPCTAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
        for(AuraList::const_iterator i = ModPowerRegenPCTAuras.begin(); i != ModPowerRegenPCTAuras.end(); ++i)
            if ((*i)->GetModifier()->m_miscvalue == (int32)power)
                addvalue *= ((*i)->GetModifier()->m_amount + 100) / 100.0f;
    }

    // addvalue computed on a 2sec basis. => update to diff time
    addvalue *= float(diff) / REGEN_TIME_FULL;

    curValue += int32(addvalue);

    if (curValue < 0)
        curValue = 0;
    else if (curValue > maxValue)
        curValue = maxValue;

    SetPower(power, curValue);
}

void Pet::RegenerateHealth(uint32 diff)
{
    uint32 curValue = GetHealth();
    uint32 maxValue = GetMaxHealth();

    if (curValue >= maxValue)
        return;

    float addvalue = 0.0f;
    float HealthIncreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_HEALTH);

    // polymorphed case
    if ( IsPolymorphed() )
        addvalue = (float)GetMaxHealth() / 3.0f;
    // normal regen case (maybe partly in combat case)
    else if (!isInCombat() || HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT) )
    {
        addvalue = OCTRegenHPPerSpirit() * HealthIncreaseRate;
        if (!isInCombat())
        {
            AuraList const& mModHealthRegenPct = GetAurasByType(SPELL_AURA_MOD_HEALTH_REGEN_PERCENT);
            for(AuraList::const_iterator i = mModHealthRegenPct.begin(); i != mModHealthRegenPct.end(); ++i)
                addvalue *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
        }
        else if(HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT))
            addvalue *= GetTotalAuraModifier(SPELL_AURA_MOD_REGEN_DURING_COMBAT) / 100.0f;
    }
    // always regeneration bonus (including combat)
    addvalue += GetTotalAuraModifier(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT);

    if(addvalue < 0)
        addvalue = 0;

    addvalue *= (float)diff / REGEN_TIME_FULL;

    ModifyHealth(int32(addvalue));
}

void Pet::ApplyScalingBonus(ScalingAction* action)
{
    if (!IsInWorld())
        return;

    switch (action->target)
    {
        case SCALING_TARGET_ALL:
            ApplyAllScalingBonuses(action->apply);
            break;
        case SCALING_TARGET_STAT:
            ApplyStatScalingBonus(Stats(action->stat),action->apply);
            break;
        case SCALING_TARGET_RESISTANCE:
            ApplyResistanceScalingBonus(action->stat, action->apply);
            break;
        case SCALING_TARGET_ATTACKPOWER:
            ApplyAttackPowerScalingBonus(action->apply);
            break;
        case SCALING_TARGET_DAMAGE:
            ApplyDamageScalingBonus(action->apply);
            break;
        case SCALING_TARGET_SPELLDAMAGE:
            ApplySpellDamageScalingBonus(action->apply);
            break;
        case SCALING_TARGET_HIT:
            ApplyHitScalingBonus(action->apply);
            break;
        case SCALING_TARGET_SPELLHIT:
            ApplySpellHitScalingBonus(action->apply);
            break;
        case SCALING_TARGET_EXPERTIZE:
            ApplyExpertizeScalingBonus(action->apply);
            break;
        case SCALING_TARGET_POWERREGEN:
            ApplyPowerregenScalingBonus(action->apply);
            break;
        case SCALING_TARGET_MAX:
        default:
            break;
    }
}

void Pet::AddScalingAction(ScalingTarget target, uint32 stat, bool apply)
{
    m_scalingQueue.push(ScalingAction(target,stat,apply));
}

void ApplyScalingBonusWithHelper::operator() (Unit* unit) const
{
    if (!unit || !unit->GetObjectGuid().IsPet())
        return;

    Pet* pet = (Pet*)unit;

    if (pet->IsInWorld())
        pet->AddScalingAction(target, stat, apply);
}

void Pet::ApplyHappinessBonus(bool apply)
{
    if (!IsInWorld())
        return;

    if (GetHappinessState() == m_HappinessState)
        return;
    else
        m_HappinessState = GetHappinessState();

    if (apply)
    {
        RemoveAurasDueToSpell(8875);
        int32 basePoints = 0;
        switch (HappinessState(m_HappinessState))
        {
            case HAPPY:
                // 125% of normal damage
                basePoints = 25;
                break;
            case CONTENT:
                // 100% of normal damage, nothing to modify
                basePoints = 0;
                break;
            case UNHAPPY:
                // 75% of normal damage
                basePoints = -25;
                break;
            default:
                basePoints = 0;
                break;
        }

        CastCustomSpell(this, 8875, &basePoints, NULL, NULL, true);

        UpdateDamagePhysical(BASE_ATTACK);
        UpdateDamagePhysical(RANGED_ATTACK);
        UpdateSpellPower();
    }
}

float Pet::OCTRegenHPPerSpirit()
{
    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return ( GetPower(POWER_MANA) > 0 ) ? (GetStat(STAT_SPIRIT) * 0.25f) : (GetStat(STAT_SPIRIT) * 0.80f);

    uint32 level = ((Player*)owner)->getLevel();
    uint32 pclass = ((Player*)owner)->getClass();

    if (level > GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtOCTRegenHPEntry     const *baseRatio = sGtOCTRegenHPStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    GtRegenHPPerSptEntry  const *moreRatio = sGtRegenHPPerSptStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);

    if (baseRatio == NULL || moreRatio == NULL)
        return 0.0f;

    // Formula from PaperDollFrame script
    float spirit = GetStat(STAT_SPIRIT);
    float baseSpirit = spirit;
    if (baseSpirit > 50) baseSpirit = 50;
    float moreSpirit = spirit - baseSpirit;
    float regen = baseSpirit * baseRatio->ratio + moreSpirit * moreRatio->ratio;
    return regen;
}

float Pet::OCTRegenMPPerSpirit()
{
    Unit* owner = GetOwner();

    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return ((GetStat(STAT_SPIRIT) / 5.0f + 17.0f)/sqrt(GetStat(STAT_INTELLECT)));

    uint32 level = ((Player*)owner)->getLevel();
    uint32 pclass = ((Player*)owner)->getClass();

    if (level > GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtRegenMPPerSptEntry  const *moreRatio = sGtRegenMPPerSptStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);

    if (moreRatio == NULL)
        return 0.0f;

    // Formula get from PaperDollFrame script
    float spirit    = GetStat(STAT_SPIRIT);
    float regen     = spirit * moreRatio->ratio;
    return regen;
}
