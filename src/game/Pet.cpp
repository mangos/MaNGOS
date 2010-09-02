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

char const* petTypeSuffix[MAX_PET_TYPE] =
{
    "'s Minion",                                            // SUMMON_PET
    "'s Pet",                                               // HUNTER_PET
    "'s Guardian",                                          // GUARDIAN_PET
    "'s Companion"                                          // MINI_PET
};

Pet::Pet(PetType type) :
Creature(CREATURE_SUBTYPE_PET), m_removed(false), m_petType(type), m_happinessTimer(7500), m_duration(0), m_resetTalentsCost(0),
m_bonusdamage(0), m_resetTalentsTime(0), m_usedTalentCount(0), m_auraUpdateMask(0), m_loading(false),
m_declinedname(NULL), m_petModeFlags(PET_MODE_DEFAULT)
{
    m_name = "Pet";
    m_regenTimer = 4000;

    // pets always have a charminfo, even if they are not actually charmed
    CharmInfo* charmInfo = InitCharmInfo(this);

    if(type == MINI_PET)                                    // always passive
        charmInfo->SetReactState(REACT_PASSIVE);
    else if(type == GUARDIAN_PET)                           // always aggressive
        charmInfo->SetReactState(REACT_AGGRESSIVE);
}

Pet::~Pet()
{
    delete m_declinedname;
}

void Pet::AddToWorld()
{
    ///- Register the pet for guid lookup
    if(!IsInWorld())
        GetMap()->GetObjectsStore().insert<Pet>(GetGUID(), (Pet*)this);

    Unit::AddToWorld();
}

void Pet::RemoveFromWorld()
{
    ///- Remove the pet from the accessor
    if(IsInWorld())
        GetMap()->GetObjectsStore().erase<Pet>(GetGUID(), (Pet*)NULL);

    ///- Don't call the function for Creature, normal mobs + totems go in a different storage
    Unit::RemoveFromWorld();
}

bool Pet::LoadPetFromDB( Player* owner, uint32 petentry, uint32 petnumber, bool current )
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

    // update for case of current pet "slot = 0"
    petentry = fields[1].GetUInt32();
    if (!petentry)
    {
        delete result;
        return false;
    }

    uint32 summon_spell_id = fields[17].GetUInt32();
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(summon_spell_id);

    bool is_temporary_summoned = spellInfo && GetSpellDuration(spellInfo) > 0;

    // check temporary summoned pets like mage water elemental
    if (current && is_temporary_summoned)
    {
        delete result;
        return false;
    }

    PetType pet_type = PetType(fields[18].GetUInt8());
    if(pet_type == HUNTER_PET)
    {
        CreatureInfo const* creatureInfo = ObjectMgr::GetCreatureTemplate(petentry);
        if(!creatureInfo || !creatureInfo->isTameable(owner->CanTameExoticPets()))
        {
            delete result;
            return false;
        }
    }

    uint32 pet_number = fields[0].GetUInt32();

    if (current && owner->IsPetNeedBeTemporaryUnsummoned())
    {
        owner->SetTemporaryUnsummonedPetNumber(pet_number);
        delete result;
        return false;
    }

    Map *map = owner->GetMap();
    uint32 guid = map->GenerateLocalLowGuid(HIGHGUID_PET);
    if (!Create(guid, map, owner->GetPhaseMask(), petentry, pet_number))
    {
        delete result;
        return false;
    }

    float px, py, pz;
    owner->GetClosePoint(px, py, pz, GetObjectBoundingRadius(), PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);

    Relocate(px, py, pz, owner->GetOrientation());

    if (!IsPositionValid())
    {
        sLog.outError("Pet (guidlow %d, entry %d) not loaded. Suggested coordinates isn't valid (X: %f Y: %f)",
            GetGUIDLow(), GetEntry(), GetPositionX(), GetPositionY());
        delete result;
        return false;
    }

    setPetType(pet_type);
    setFaction(owner->getFaction());
    SetUInt32Value(UNIT_CREATED_BY_SPELL, summon_spell_id);

    CreatureInfo const *cinfo = GetCreatureInfo();
    if (cinfo->type == CREATURE_TYPE_CRITTER)
    {
        AIM_Initialize();
        map->Add((Creature*)this);
        delete result;
        return true;
    }

    m_charmInfo->SetPetNumber(pet_number, IsPermanentPetFor(owner));

    SetOwnerGUID(owner->GetGUID());
    SetDisplayId(fields[3].GetUInt32());
    SetNativeDisplayId(fields[3].GetUInt32());
    uint32 petlevel = fields[4].GetUInt32();
    SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
    SetName(fields[8].GetString());

    switch (getPetType())
    {
        case SUMMON_PET:
            petlevel=owner->getLevel();

            SetUInt32Value(UNIT_FIELD_BYTES_0, 2048);
            SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
                                                            // this enables popup window (pet dismiss, cancel)
            break;
        case HUNTER_PET:
            SetUInt32Value(UNIT_FIELD_BYTES_0, 0x02020100);
            SetSheath(SHEATH_STATE_MELEE);
            SetByteFlag(UNIT_FIELD_BYTES_2, 2, fields[9].GetBool() ? UNIT_CAN_BE_ABANDONED : UNIT_CAN_BE_RENAMED | UNIT_CAN_BE_ABANDONED);

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
    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, fields[5].GetUInt32());
    SetCreatorGUID(owner->GetGUID());

    m_charmInfo->SetReactState(ReactStates(fields[6].GetUInt8()));

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
    if (!is_temporary_summoned)
        m_charmInfo->LoadPetActionBar(fields[13].GetCppString());

    // since last save (in seconds)
    uint32 timediff = uint32(time(NULL) - fields[14].GetUInt64());

    m_resetTalentsCost = fields[15].GetUInt32();
    m_resetTalentsTime = fields[16].GetUInt64();

    delete result;

    //load spells/cooldowns/auras
    _LoadAuras(timediff);

    //init AB
    if (is_temporary_summoned)
    {
        // Temporary summoned pets always have initial spell list at load
        InitPetCreateSpells();
    }
    else
    {
        LearnPetPassives();
        CastPetAuras(current);
    }

    if (getPetType() == SUMMON_PET && !current)             //all (?) summon pets come with full health when called, but not when they are current
    {
        SetHealth(GetMaxHealth());
        SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    }
    else
    {
        SetHealth(savedhealth > GetMaxHealth() ? GetMaxHealth() : savedhealth);
        SetPower(POWER_MANA, savedmana > GetMaxPower(POWER_MANA) ? GetMaxPower(POWER_MANA) : savedmana);
    }

    UpdateWalkMode(owner);

    AIM_Initialize();
    map->Add((Creature*)this);

    // Spells should be loaded after pet is added to map, because in CheckCast is check on it
    _LoadSpells();
    InitLevelupSpellsForLevel();

    CleanupActionBar();                                     // remove unknown spells from action bar after load

    _LoadSpellCooldowns();

    owner->SetPet(this);                                    // in DB stored only full controlled creature
    DEBUG_LOG("New Pet has guid %u", GetGUIDLow());

    if (owner->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)owner)->PetSpellInitialize();
        if(((Player*)owner)->GetGroup())
            ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_PET);

        ((Player*)owner)->SendTalentsInfoData(true);
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

    m_loading = false;

    SynchronizeLevelWithOwner();
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
    if(!IS_PLAYER_GUID(GetOwnerGUID()))
        return;

    Player* pOwner = (Player*)GetOwner();
    if (!pOwner)
        return;

    // current/stable/not_in_slot
    if (mode >= PET_SAVE_AS_CURRENT)
    {
        // not save pet as current if another pet temporary unsummoned
        if (mode == PET_SAVE_AS_CURRENT && pOwner->GetTemporaryUnsummonedPetNumber() &&
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
        if (mode != PET_SAVE_AS_CURRENT)
            RemoveAllAuras();

        _SaveSpells();
        _SaveSpellCooldowns();
        _SaveAuras();

        uint32 owner = GUID_LOPART(GetOwnerGUID());
        std::string name = m_name;
        CharacterDatabase.escape_string(name);
        CharacterDatabase.BeginTransaction();
        // remove current data
        CharacterDatabase.PExecute("DELETE FROM character_pet WHERE owner = '%u' AND id = '%u'", owner,m_charmInfo->GetPetNumber() );

        // prevent duplicate using slot (except PET_SAVE_NOT_IN_SLOT)
        if(mode <= PET_SAVE_LAST_STABLE_SLOT)
            CharacterDatabase.PExecute("UPDATE character_pet SET slot = '%u' WHERE owner = '%u' AND slot = '%u'",
                PET_SAVE_NOT_IN_SLOT, owner, uint32(mode) );

        // prevent existence another hunter pet in PET_SAVE_AS_CURRENT and PET_SAVE_NOT_IN_SLOT
        if(getPetType()==HUNTER_PET && (mode==PET_SAVE_AS_CURRENT||mode > PET_SAVE_LAST_STABLE_SLOT))
            CharacterDatabase.PExecute("DELETE FROM character_pet WHERE owner = '%u' AND (slot = '%u' OR slot > '%u')",
                owner,PET_SAVE_AS_CURRENT,PET_SAVE_LAST_STABLE_SLOT);
        // save pet
        std::ostringstream ss;
        ss  << "INSERT INTO character_pet ( id, entry,  owner, modelid, level, exp, Reactstate, slot, name, renamed, curhealth, curmana, curhappiness, abdata, savetime, resettalents_cost, resettalents_time, CreatedBySpell, PetType) "
            << "VALUES ("
            << m_charmInfo->GetPetNumber() << ", "
            << GetEntry() << ", "
            << owner << ", "
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

void Pet::DeleteFromDB(uint32 guidlow)
{
    CharacterDatabase.PExecute("DELETE FROM character_pet WHERE id = '%u'", guidlow);
    CharacterDatabase.PExecute("DELETE FROM character_pet_declinedname WHERE id = '%u'", guidlow);
    CharacterDatabase.PExecute("DELETE FROM pet_aura WHERE guid = '%u'", guidlow);
    CharacterDatabase.PExecute("DELETE FROM pet_spell WHERE guid = '%u'", guidlow);
    CharacterDatabase.PExecute("DELETE FROM pet_spell_cooldown WHERE guid = '%u'", guidlow);
}

void Pet::setDeathState(DeathState s)                       // overwrite virtual Creature::setDeathState and Unit::setDeathState
{
    Creature::setDeathState(s);
    if(getDeathState()==CORPSE)
    {
        //remove summoned pet (no corpse)
        if(getPetType()==SUMMON_PET)
            Remove(PET_SAVE_NOT_IN_SLOT);
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

            SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        }
    }
    else if(getDeathState()==ALIVE)
    {
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        CastPetAuras(true);
    }
}

void Pet::Update(uint32 diff)
{
    if(m_removed)                                           // pet already removed, just wait in remove queue, no updates
        return;

    switch( m_deathState )
    {
        case CORPSE:
        {
            if( m_deathTimer <= diff )
            {
                MANGOS_ASSERT(getPetType()!=SUMMON_PET && "Must be already removed.");
                Remove(PET_SAVE_NOT_IN_SLOT);               //hunters' pets never get removed because of death, NEVER!
                return;
            }
            break;
        }
        case ALIVE:
        {
            // unsummon pet that lost owner
            Unit* owner = GetOwner();
            if(!owner || (!IsWithinDistInMap(owner, GetMap()->GetVisibilityDistance()) && (owner->GetCharmGUID() && (owner->GetCharmGUID() != GetGUID()))) || (isControlled() && !owner->GetPetGUID()))
            {
                Remove(PET_SAVE_NOT_IN_SLOT, true);
                return;
            }

            if(isControlled())
            {
                if( owner->GetPetGUID() != GetGUID() )
                {
                    Remove(getPetType()==HUNTER_PET?PET_SAVE_AS_DELETED:PET_SAVE_NOT_IN_SLOT);
                    return;
                }
            }

            if(m_duration > 0)
            {
                if(m_duration > (int32)diff)
                    m_duration -= (int32)diff;
                else
                {
                    Remove(getPetType() != SUMMON_PET ? PET_SAVE_AS_DELETED:PET_SAVE_NOT_IN_SLOT);
                    return;
                }
            }

            //regenerate focus for hunter pets or energy for deathknight's ghoul
            if(m_regenTimer <= diff)
            {
                switch (getPowerType())
                {
                    case POWER_FOCUS:
                    case POWER_ENERGY:
                        Regenerate(getPowerType());
                        break;
                    default:
                        break;
                }
                m_regenTimer = 4000;
            }
            else
                m_regenTimer -= diff;

            if(getPetType() != HUNTER_PET)
                break;

            if(m_happinessTimer <= diff)
            {
                LooseHappiness();
                m_happinessTimer = 7500;
            }
            else
                m_happinessTimer -= diff;

            break;
        }
        default:
            break;
    }
    Creature::Update(diff);
}

void Pet::Regenerate(Powers power)
{
    uint32 curValue = GetPower(power);
    uint32 maxValue = GetMaxPower(power);

    if (curValue >= maxValue)
        return;

    float addvalue = 0.0f;

    switch (power)
    {
        case POWER_FOCUS:
        {
            // For hunter pets.
            addvalue = 24 * sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_FOCUS);
            break;
        }
        case POWER_ENERGY:
        {
            // For deathknight's ghoul.
            addvalue = 20;
            break;
        }
        default:
            return;
    }

    // Apply modifiers (if any).
    AuraList const& ModPowerRegenPCTAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
    for(AuraList::const_iterator i = ModPowerRegenPCTAuras.begin(); i != ModPowerRegenPCTAuras.end(); ++i)
        if ((*i)->GetModifier()->m_miscvalue == power)
            addvalue *= ((*i)->GetModifier()->m_amount + 100) / 100.0f;

    ModifyPower(power, (int32)addvalue);
}

void Pet::LooseHappiness()
{
    uint32 curValue = GetPower(POWER_HAPPINESS);
    if (curValue <= 0)
        return;
    int32 addvalue = 670;                                   //value is 70/35/17/8/4 (per min) * 1000 / 8 (timer 7.5 secs)
    if(isInCombat())                                        //we know in combat happiness fades faster, multiplier guess
        addvalue = int32(addvalue * 1.5);
    ModifyPower(POWER_HAPPINESS, -addvalue);
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

void Pet::Remove(PetSaveMode mode, bool returnreagent)
{
    Unit* owner = GetOwner();

    if(owner)
    {
        if(owner->GetTypeId()==TYPEID_PLAYER)
        {
            ((Player*)owner)->RemovePet(this,mode,returnreagent);
            return;
        }

        // only if current pet in slot
        if(owner->GetPetGUID()==GetGUID())
            owner->SetPet(0);
    }

    AddObjectToRemoveList();
    m_removed = true;
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

    // XP to money conversion processed in Player::RewardQuest
    if(level >= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
        return;

    uint32 curXP = GetUInt32Value(UNIT_FIELD_PETEXPERIENCE);
    uint32 nextLvlXP = GetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP);
    uint32 newXP = curXP + xp;

    if(newXP >= nextLvlXP && level+1 > GetOwner()->getLevel())
    {
        SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, nextLvlXP-1);
        return;
    }

    while( newXP >= nextLvlXP && level < sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL) )
    {
        newXP -= nextLvlXP;

        GivePetLevel(level+1);
        SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, sObjectMgr.GetXPForPetLevel(level+1));

        level = getLevel();
        nextLvlXP = GetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP);
    }

    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, newXP);
}

void Pet::GivePetLevel(uint32 level)
{
    if(!level)
        return;

    InitStatsForLevel(level);
    InitLevelupSpellsForLevel();
    InitTalentForLevel();
}

bool Pet::CreateBaseAtCreature(Creature* creature)
{
    if(!creature)
    {
        sLog.outError("CRITICAL: NULL pointer parsed into CreateBaseAtCreature()");
        return false;
    }

    uint32 guid = creature->GetMap()->GenerateLocalLowGuid(HIGHGUID_PET);

    BASIC_LOG("Create pet");
    uint32 pet_number = sObjectMgr.GeneratePetNumber();
    if(!Create(guid, creature->GetMap(), creature->GetPhaseMask(), creature->GetEntry(), pet_number))
        return false;

    Relocate(creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation());

    if(!IsPositionValid())
    {
        sLog.outError("Pet (guidlow %d, entry %d) not created base at creature. Suggested coordinates isn't valid (X: %f Y: %f)",
            GetGUIDLow(), GetEntry(), GetPositionX(), GetPositionY());
        return false;
    }

    CreatureInfo const *cinfo = GetCreatureInfo();
    if(!cinfo)
    {
        sLog.outError("CreateBaseAtCreature() failed, creatureInfo is missing!");
        return false;
    }

    if(cinfo->type == CREATURE_TYPE_CRITTER)
    {
        setPetType(MINI_PET);
        return true;
    }
    SetDisplayId(creature->GetDisplayId());
    SetNativeDisplayId(creature->GetNativeDisplayId());
    SetMaxPower(POWER_HAPPINESS, GetCreatePowers(POWER_HAPPINESS));
    SetPower(POWER_HAPPINESS, 166500);
    setPowerType(POWER_FOCUS);
    SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, 0);
    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
    SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, sObjectMgr.GetXPForPetLevel(creature->getLevel()));
    SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);

    if(CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cinfo->family))
        SetName(cFamily->Name[sWorld.GetDefaultDbcLocale()]);
    else
        SetName(creature->GetNameForLocaleIdx(sObjectMgr.GetDBCLocaleIndex()));

    if(cinfo->type == CREATURE_TYPE_BEAST)
    {
        SetUInt32Value(UNIT_FIELD_BYTES_0, 0x02020100);
        SetSheath(SHEATH_STATE_MELEE);
        SetByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED | UNIT_CAN_BE_ABANDONED);
        SetUInt32Value(UNIT_MOD_CAST_SPEED, creature->GetUInt32Value(UNIT_MOD_CAST_SPEED));
    }
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

    uint32 creature_ID = (getPetType() == HUNTER_PET) ? 1 : cinfo->Entry;

    SetLevel(petlevel);

    SetMeleeDamageSchool(SpellSchools(cinfo->dmgschool));

    SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, float(petlevel*50));

    SetAttackTime(BASE_ATTACK, BASE_ATTACK_TIME);
    SetAttackTime(OFF_ATTACK, BASE_ATTACK_TIME);
    SetAttackTime(RANGED_ATTACK, BASE_ATTACK_TIME);

    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0);

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
    m_bonusdamage = 0;

    int32 createResistance[MAX_SPELL_SCHOOL] = {0,0,0,0,0,0,0};

    if(cinfo && getPetType() != HUNTER_PET)
    {
        createResistance[SPELL_SCHOOL_HOLY]   = cinfo->resistance1;
        createResistance[SPELL_SCHOOL_FIRE]   = cinfo->resistance2;
        createResistance[SPELL_SCHOOL_NATURE] = cinfo->resistance3;
        createResistance[SPELL_SCHOOL_FROST]  = cinfo->resistance4;
        createResistance[SPELL_SCHOOL_SHADOW] = cinfo->resistance5;
        createResistance[SPELL_SCHOOL_ARCANE] = cinfo->resistance6;
    }

    switch(getPetType())
    {
        case SUMMON_PET:
        {
            if(owner->GetTypeId() == TYPEID_PLAYER)
            {
                switch(owner->getClass())
                {
                    case CLASS_WARLOCK:
                    {

                        //the damage bonus used for pets is either fire or shadow damage, whatever is higher
                        uint32 fire  = owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_FIRE);
                        uint32 shadow = owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_SHADOW);
                        uint32 val  = (fire > shadow) ? fire : shadow;

                        SetBonusDamage(int32 (val * 0.15f));
                        //bonusAP += val * 0.57;
                        break;
                    }
                    case CLASS_MAGE:
                    {
                                                            //40% damage bonus of mage's frost damage
                        float val = owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_FROST) * 0.4f;
                        if(val < 0)
                            val = 0;
                        SetBonusDamage( int32(val));
                        break;
                    }
                    default:
                        break;
                }
            }

            SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(petlevel - (petlevel / 4)) );
            SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel + (petlevel / 4)) );

            //SetModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE, float(cinfo->attackpower));

            PetLevelInfo const* pInfo = sObjectMgr.GetPetLevelInfo(creature_ID, petlevel);
            if(pInfo)                                       // exist in DB
            {
                SetCreateHealth(pInfo->health);
                SetCreateMana(pInfo->mana);

                if(pInfo->armor > 0)
                    SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, float(pInfo->armor));

                for(int stat = 0; stat < MAX_STATS; ++stat)
                {
                    SetCreateStat(Stats(stat), float(pInfo->stats[stat]));
                }
            }
            else                                            // not exist in DB, use some default fake data
            {
                sLog.outErrorDb("Summoned pet (Entry: %u) not have pet stats data in DB",cinfo->Entry);

                // remove elite bonuses included in DB values
                SetCreateHealth(uint32(((float(cinfo->maxhealth) / cinfo->maxlevel) / (1 + 2 * cinfo->rank)) * petlevel) );
                SetCreateMana(  uint32(((float(cinfo->maxmana)   / cinfo->maxlevel) / (1 + 2 * cinfo->rank)) * petlevel) );

                SetCreateStat(STAT_STRENGTH, 22);
                SetCreateStat(STAT_AGILITY, 22);
                SetCreateStat(STAT_STAMINA, 25);
                SetCreateStat(STAT_INTELLECT, 28);
                SetCreateStat(STAT_SPIRIT, 27);
            }
            break;
        }
        case HUNTER_PET:
        {
            SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, sObjectMgr.GetXPForPetLevel(petlevel));
            //these formula may not be correct; however, it is designed to be close to what it should be
            //this makes dps 0.5 of pets level
            SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(petlevel - (petlevel / 4)) );
            //damage range is then petlevel / 2
            SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel + (petlevel / 4)) );
            //damage is increased afterwards as strength and pet scaling modify attack power

            //stored standard pet stats are entry 1 in pet_levelinfo
            PetLevelInfo const* pInfo = sObjectMgr.GetPetLevelInfo(creature_ID, petlevel);
            if(pInfo)                                       // exist in DB
            {
                SetCreateHealth(pInfo->health);
                SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, float(pInfo->armor));
                //SetModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE, float(cinfo->attackpower));

                for( int i = STAT_STRENGTH; i < MAX_STATS; ++i)
                {
                    SetCreateStat(Stats(i),  float(pInfo->stats[i]));
                }
            }
            else                                            // not exist in DB, use some default fake data
            {
                sLog.outErrorDb("Hunter pet levelstats missing in DB");

                // remove elite bonuses included in DB values
                SetCreateHealth( uint32(((float(cinfo->maxhealth) / cinfo->maxlevel) / (1 + 2 * cinfo->rank)) * petlevel) );

                SetCreateStat(STAT_STRENGTH, 22);
                SetCreateStat(STAT_AGILITY, 22);
                SetCreateStat(STAT_STAMINA, 25);
                SetCreateStat(STAT_INTELLECT, 28);
                SetCreateStat(STAT_SPIRIT, 27);
            }
            break;
        }
        case GUARDIAN_PET:
            SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
            SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, 1000);

            SetCreateMana(28 + 10*petlevel);
            SetCreateHealth(28 + 30*petlevel);

            // FIXME: this is wrong formula, possible each guardian pet have own damage formula
            //these formula may not be correct; however, it is designed to be close to what it should be
            //this makes dps 0.5 of pets level
            SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(petlevel - (petlevel / 4)));
            //damage range is then petlevel / 2
            SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel + (petlevel / 4)));
            break;
        default:
            sLog.outError("Pet have incorrect type (%u) for levelup.", getPetType());
            break;
    }

    for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
        SetModifierValue(UnitMods(UNIT_MOD_RESISTANCE_START + i), BASE_VALUE, float(createResistance[i]));

    UpdateAllStats();

    SetHealth(GetMaxHealth());
    SetPower(POWER_MANA, GetMaxPower(POWER_MANA));

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
            if(spellproto->procCharges)
            {
                if(remaincharges <= 0 || remaincharges > (int32)spellproto->procCharges)
                    remaincharges = spellproto->procCharges;
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

        //skip all holders from spells that are passive
        //do not save single target holders (unless they were cast by the player)
        if (save && !holder->IsPassive() && (holder->GetCasterGUID() == GetGUID() || !holder->IsSingleTarget()))
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
                "('%u', '" UI64FMTD "', '%u', '%u', '%u', '%u', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%u')",
                m_charmInfo->GetPetNumber(), holder->GetCasterGUID(), GUID_LOPART(holder->GetCastItemGUID()), holder->GetId(), holder->GetStackAmount(), holder->GetAuraCharges(),
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

    if(newspell.active == ACT_ENABLED)
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
        if(!m_loading)
        {
            // need update action bar for last removed rank
            if (Unit* owner = GetOwner())
                if (owner->GetTypeId() == TYPEID_PLAYER)
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

    uint32 level = getLevel();
    uint32 talentPointsForLevel = GetMaxTalentPointsForLevel(level);

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
    if(IsPassiveSpell(spellid))
        return;

    PetSpellMap::iterator itr = m_spells.find(spellid);

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

bool Pet::Create(uint32 guidlow, Map *map, uint32 phaseMask, uint32 Entry, uint32 pet_number)
{
    SetMap(map);
    SetPhaseMask(phaseMask,false);

    Object::_Create(guidlow, pet_number, HIGHGUID_PET);

    m_DBTableGuid = guidlow;
    m_originalEntry = Entry;

    if(!InitEntry(Entry))
        return false;

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
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    switch(getPetType())
    {
        // always same level
        case SUMMON_PET:
            GivePetLevel(owner->getLevel());
            break;
        // can't be greater owner level
        case HUNTER_PET:
            if(getLevel() > owner->getLevel())
            {
                GivePetLevel(owner->getLevel());
                SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, sObjectMgr.GetXPForPetLevel(owner->getLevel()));
                SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, GetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP)-1);
            }
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
